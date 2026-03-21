/*
 * =============================================================================
 * PIGURA OS - LOAD_KERNEL.C
 * =========================
 * Implementasi kernel loader untuk Pigura OS.
 *
 * Berkas ini berisi fungsi-fungsi untuk memuat kernel dari disk
 * dan mempersiapkan eksekusi kernel.
 *
 * Arsitektur: x86/x86_64
 * Versi: 1.0
 * =============================================================================
 */

#include <stdint.h>
#include <stddef.h>

/* =============================================================================
 * KONSTANTA
 * =============================================================================
 */

/* Alamat memori kernel */
#define KERNEL_LOAD_ADDRESS     0x00100000      /* 1 MB */
#define KERNEL_ENTRY_ADDRESS    0x00100000
#define KERNEL_MAX_SIZE         (4 * 1024 * 1024)   /* 4 MB max */

/* Alamat stack */
#define BOOT_STACK_ADDRESS      0x00008000      /* 32 KB */

/* Alamat page tables */
#define PAGE_TABLE_ADDRESS      0x00007000

/* Magic number kernel */
#define KERNEL_MAGIC            0x4B524E4C      /* "KRNL" */

/* ELF magic */
#define ELF_MAGIC_0             0x7F
#define ELF_MAGIC_1             'E'
#define ELF_MAGIC_2             'L'
#define ELF_MAGIC_3             'F'

/* ELF class */
#define ELFCLASS32              1
#define ELFCLASS64              2

/* ELF machine */
#define EM_386                  3
#define EM_X86_64               62

/* ELF type */
#define ET_EXEC                 2

/* Multiboot magic */
#define MULTIBOOT_MAGIC         0x2BADB002

/* Port I/O */
#define PORT_FLOPPY_BASE        0x3F0
#define PORT_ATA1_BASE          0x1F0
#define PORT_ATA2_BASE          0x170

/* Sector size */
#define SECTOR_SIZE             512

/* =============================================================================
 * TIPE DATA
 * =============================================================================
 */

/* ELF header (32-bit) */
typedef struct {
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf32_hdr_t;

/* ELF header (64-bit) */
typedef struct {
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf64_hdr_t;

/* ELF program header (32-bit) */
typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} elf32_phdr_t;

/* ELF program header (64-bit) */
typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} elf64_phdr_t;

/* Multiboot info */
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
} multiboot_info_t;

/* Kernel info */
typedef struct {
    uint32_t entry_point;
    uint32_t size;
    uint32_t load_address;
    int is_64bit;
} kernel_info_t;

/* =============================================================================
 * VARIABEL GLOBAL
 * =============================================================================
 */

/* Kernel info */
static kernel_info_t g_kernel_info;

/* Status loading */
static int g_kernel_loaded = 0;

/* =============================================================================
 * FUNGSI INLINE ASSEMBLY
 * =============================================================================
 */

/*
 * _outb
 * -----
 * Tulis byte ke port.
 */
static inline void _outb(uint8_t val, uint16_t port)
{
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

/*
 * _inb
 * ----
 * Baca byte dari port.
 */
static inline uint8_t _inb(uint16_t port)
{
    uint8_t val;
    __asm__ __volatile__("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/*
 * _outsw
 * ------
 * Tulis word array ke port.
 */
static inline void _outsw(uint16_t port, const void *buf, int count)
{
    __asm__ __volatile__(
        "rep outsw"
        : "+S"(buf), "+c"(count)
        : "d"(port)
    );
}

/*
 * _insw
 * -----
 * Baca word array dari port.
 */
static inline void _insw(uint16_t port, void *buf, int count)
{
    __asm__ __volatile__(
        "rep insw"
        : "+D"(buf), "+c"(count)
        : "d"(port)
        : "memory"
    );
}

/* =============================================================================
 * FUNGSI UTILITAS
 * =============================================================================
 */

/*
 * _memset
 * -------
 * Mengisi memori.
 */
static void *_memset(void *dest, uint8_t val, size_t n)
{
    uint8_t *d;
    size_t i;

    d = (uint8_t *)dest;
    for (i = 0; i < n; i++) {
        d[i] = val;
    }
    return dest;
}

/*
 * _memcpy
 * -------
 * Menyalin memori.
 */
static void *_memcpy(void *dest, const void *src, size_t n)
{
    uint8_t *d;
    const uint8_t *s;
    size_t i;

    d = (uint8_t *)dest;
    s = (const uint8_t *)src;

    for (i = 0; i < n; i++) {
        d[i] = s[i];
    }

    return dest;
}

/*
 * _strlen
 * -------
 * Menghitung panjang string.
 */
static size_t _strlen(const char *str)
{
    size_t len;

    len = 0;
    while (str[len] != '\0') {
        len++;
    }

    return len;
}

/*
 * _puts
 * -----
 * Menampilkan string.
 */
static void _puts(const char *str)
{
    volatile uint16_t *vga;
    static int pos = 0;
    size_t i;

    vga = (volatile uint16_t *)0xB8000;

    for (i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            pos = ((pos / 80) + 1) * 80;
        } else {
            vga[pos] = (0x07 << 8) | (uint8_t)str[i];
            pos++;
        }

        if (pos >= 80 * 25) {
            pos = 0;
        }
    }
}

/*
 * _puthex
 * -------
 * Menampilkan nilai hex.
 */
static void _puthex(uint32_t val)
{
    char buf[11];
    const char *hex = "0123456789ABCDEF";
    int i;

    buf[0] = '0';
    buf[1] = 'x';

    for (i = 7; i >= 0; i--) {
        buf[2 + i] = hex[val & 0xF];
        val >>= 4;
    }

    buf[10] = '\0';
    _puts(buf);
}

/* =============================================================================
 * FUNGSI DISK I/O
 * =============================================================================
 */

/*
 * _wait_ata_ready
 * ---------------
 * Menunggu ATA controller ready.
 */
static int _wait_ata_ready(uint16_t base)
{
    uint8_t status;
    int timeout;

    timeout = 100000;

    while (timeout-- > 0) {
        status = _inb(base + 7);    /* Status register */
        if ((status & 0x80) == 0) {  /* Not busy */
            if (status & 0x08) {     /* Data ready */
                return 0;
            }
        }
    }

    return -1;      /* Timeout */
}

/*
 * _read_sector_ata
 * ----------------
 * Membaca satu sektor dari ATA disk.
 */
static int _read_sector_ata(uint16_t base, uint32_t lba,
                             void *buffer)
{
    uint8_t status;
    uint8_t lba_low;
    uint8_t lba_mid;
    uint8_t lba_high;
    uint8_t drive_select;

    /* Calculate LBA bytes */
    lba_low = (uint8_t)(lba & 0xFF);
    lba_mid = (uint8_t)((lba >> 8) & 0xFF);
    lba_high = (uint8_t)((lba >> 16) & 0xFF);

    /* Drive select (LBA mode, master drive) */
    drive_select = 0xE0 | ((lba >> 24) & 0x0F);

    /* Wait for drive ready */
    if (_wait_ata_ready(base) != 0) {
        return -1;
    }

    /* Select drive */
    _outb(drive_select, base + 6);

    /* Set sector count */
    _outb(1, base + 2);         /* Sector count = 1 */

    /* Set LBA */
    _outb(lba_low, base + 3);
    _outb(lba_mid, base + 4);
    _outb(lba_high, base + 5);

    /* Send READ command */
    _outb(0x20, base + 7);      /* READ SECTORS */

    /* Wait for data */
    if (_wait_ata_ready(base) != 0) {
        return -1;
    }

    /* Read data (256 words = 512 bytes) */
    _insw(base, buffer, 256);

    /* Wait for completion */
    timeout_wait:
    status = _inb(base + 7);
    if (status & 0x80) {
        /* Still busy */
        goto timeout_wait;
    }

    if (status & 0x01) {
        /* Error */
        return -1;
    }

    return 0;
}

/*
 * _read_sectors
 * -------------
 * Membaca multiple sektor.
 */
static int _read_sectors(uint32_t lba, uint32_t count, void *buffer)
{
    uint32_t i;
    uint8_t *buf;
    int result;

    buf = (uint8_t *)buffer;

    for (i = 0; i < count; i++) {
        result = _read_sector_ata(PORT_ATA1_BASE, lba + i,
                                   buf + i * SECTOR_SIZE);
        if (result != 0) {
            return -1;
        }
    }

    return 0;
}

/* =============================================================================
 * FUNGSI ELF
 * =============================================================================
 */

/*
 * _check_elf_header
 * -----------------
 * Memeriksa validitas ELF header.
 */
static int _check_elf_header(void *header)
{
    uint8_t *ident;

    ident = (uint8_t *)header;

    /* Check ELF magic */
    if (ident[0] != ELF_MAGIC_0 ||
        ident[1] != ELF_MAGIC_1 ||
        ident[2] != ELF_MAGIC_2 ||
        ident[3] != ELF_MAGIC_3) {
        return -1;
    }

    /* Check ELF class (32 or 64 bit) */
    if (ident[4] != ELFCLASS32 && ident[4] != ELFCLASS64) {
        return -1;
    }

    return 0;
}

/*
 * _parse_elf32
 * ------------
 * Parse ELF32 header.
 */
static int _parse_elf32(elf32_hdr_t *header)
{
    uint32_t entry;
    uint16_t phnum;
    uint16_t phentsize;
    uint32_t phoff;
    elf32_phdr_t *phdr;
    uint16_t i;
    uint8_t *kernel_data;

    /* Verify machine type */
    if (header->e_machine != EM_386) {
        _puts("[LOAD] Error: Not i386 ELF\n");
        return -1;
    }

    entry = header->e_entry;
    phnum = header->e_phnum;
    phentsize = header->e_phentsize;
    phoff = header->e_phoff;

    kernel_data = (uint8_t *)KERNEL_LOAD_ADDRESS;

    _puts("[LOAD] ELF32 detected\n");
    _puts("[LOAD] Entry: ");
    _puthex(entry);
    _puts("\n");
    _puts("[LOAD] Program headers: ");
    /* print phnum */
    _puts("\n");

    /* Process program headers */
    for (i = 0; i < phnum; i++) {
        phdr = (elf32_phdr_t *)(kernel_data + phoff + i * phentsize);

        if (phdr->p_type != 1) {    /* PT_LOAD */
            continue;
        }

        /* Copy segment to memory */
        if (phdr->p_filesz > 0) {
            _memcpy((void *)phdr->p_paddr,
                    kernel_data + phdr->p_offset,
                    phdr->p_filesz);
        }

        /* Zero BSS */
        if (phdr->p_memsz > phdr->p_filesz) {
            _memset((void *)(phdr->p_paddr + phdr->p_filesz),
                    0,
                    phdr->p_memsz - phdr->p_filesz);
        }
    }

    g_kernel_info.entry_point = entry;
    g_kernel_info.is_64bit = 0;

    return 0;
}

/*
 * _parse_elf64
 * ------------
 * Parse ELF64 header.
 */
static int _parse_elf64(elf64_hdr_t *header)
{
    uint64_t entry;
    uint16_t phnum;
    uint16_t phentsize;
    uint64_t phoff;
    elf64_phdr_t *phdr;
    uint16_t i;
    uint8_t *kernel_data;

    /* Verify machine type */
    if (header->e_machine != EM_X86_64) {
        _puts("[LOAD] Error: Not x86_64 ELF\n");
        return -1;
    }

    entry = header->e_entry;
    phnum = header->e_phnum;
    phentsize = header->e_phentsize;
    phoff = header->e_phoff;

    kernel_data = (uint8_t *)KERNEL_LOAD_ADDRESS;

    _puts("[LOAD] ELF64 detected\n");
    _puts("[LOAD] Entry: ");
    _puthex((uint32_t)entry);
    _puts("\n");

    /* Process program headers */
    for (i = 0; i < phnum; i++) {
        phdr = (elf64_phdr_t *)(kernel_data + phoff + i * phentsize);

        if (phdr->p_type != 1) {    /* PT_LOAD */
            continue;
        }

        /* Copy segment */
        if (phdr->p_filesz > 0) {
            _memcpy((void *)(uint32_t)phdr->p_paddr,
                    kernel_data + (uint32_t)phdr->p_offset,
                    (size_t)phdr->p_filesz);
        }

        /* Zero BSS */
        if (phdr->p_memsz > phdr->p_filesz) {
            _memset((void *)(uint32_t)(phdr->p_paddr + phdr->p_filesz),
                    0,
                    (size_t)(phdr->p_memsz - phdr->p_filesz));
        }
    }

    g_kernel_info.entry_point = (uint32_t)entry;
    g_kernel_info.is_64bit = 1;

    return 0;
}

/* =============================================================================
 * FUNGSI PUBLIC
 * =============================================================================
 */

/*
 * load_kernel_init
 * ----------------
 * Inisialisasi kernel loader.
 */
int load_kernel_init(void)
{
    _memset(&g_kernel_info, 0, sizeof(kernel_info_t));

    g_kernel_info.load_address = KERNEL_LOAD_ADDRESS;

    return 0;
}

/*
 * load_kernel_from_disk
 * ---------------------
 * Memuat kernel dari disk.
 *
 * Parameter:
 *   start_sector - Sector awal kernel
 *   sector_count - Jumlah sektor
 */
int load_kernel_from_disk(uint32_t start_sector, uint32_t sector_count)
{
    uint8_t *dest;
    int result;

    _puts("[LOAD] Loading kernel from disk...\n");
    _puts("[LOAD] Start sector: ");
    _puthex(start_sector);
    _puts("\n");
    _puts("[LOAD] Sector count: ");
    _puthex(sector_count);
    _puts("\n");

    dest = (uint8_t *)KERNEL_LOAD_ADDRESS;

    result = _read_sectors(start_sector, sector_count, dest);
    if (result != 0) {
        _puts("[LOAD] Error: Failed to read from disk\n");
        return -1;
    }

    g_kernel_info.size = sector_count * SECTOR_SIZE;
    g_kernel_loaded = 1;

    _puts("[LOAD] Kernel loaded successfully\n");

    return 0;
}

/*
 * load_kernel_parse
 * -----------------
 * Parse kernel (ELF or raw binary).
 */
int load_kernel_parse(void)
{
    uint8_t *header;
    uint8_t elf_class;

    if (!g_kernel_loaded) {
        return -1;
    }

    header = (uint8_t *)KERNEL_LOAD_ADDRESS;

    /* Check if ELF */
    if (header[0] == ELF_MAGIC_0 &&
        header[1] == ELF_MAGIC_1 &&
        header[2] == ELF_MAGIC_2 &&
        header[3] == ELF_MAGIC_3) {

        elf_class = header[4];

        if (elf_class == ELFCLASS32) {
            return _parse_elf32((elf32_hdr_t *)header);
        } else if (elf_class == ELFCLASS64) {
            return _parse_elf64((elf64_hdr_t *)header);
        } else {
            _puts("[LOAD] Error: Unknown ELF class\n");
            return -1;
        }
    }

    /* Check for raw binary with magic */
    if (*((uint32_t *)header) == KERNEL_MAGIC) {
        _puts("[LOAD] Raw binary detected\n");
        g_kernel_info.entry_point = KERNEL_ENTRY_ADDRESS;
        g_kernel_info.is_64bit = 0;
        return 0;
    }

    /* Assume raw binary */
    _puts("[LOAD] Assuming raw binary\n");
    g_kernel_info.entry_point = KERNEL_ENTRY_ADDRESS;
    g_kernel_info.is_64bit = 0;

    return 0;
}

/*
 * load_kernel_get_entry
 * ---------------------
 * Dapatkan entry point kernel.
 */
uint32_t load_kernel_get_entry(void)
{
    return g_kernel_info.entry_point;
}

/*
 * load_kernel_get_size
 * --------------------
 * Dapatkan ukuran kernel.
 */
uint32_t load_kernel_get_size(void)
{
    return g_kernel_info.size;
}

/*
 * load_kernel_is_64bit
 * --------------------
 * Cek apakah kernel 64-bit.
 */
int load_kernel_is_64bit(void)
{
    return g_kernel_info.is_64bit;
}

/*
 * load_kernel_is_loaded
 * ---------------------
 * Cek apakah kernel sudah dimuat.
 */
int load_kernel_is_loaded(void)
{
    return g_kernel_loaded;
}

/*
 * load_kernel_jump
 * ----------------
 * Lompat ke kernel.
 */
void load_kernel_jump(uint32_t magic, multiboot_info_t *mbi)
{
    void (*entry)(uint32_t, multiboot_info_t *);

    entry = (void (*)(uint32_t, multiboot_info_t *))g_kernel_info.entry_point;

    _puts("[LOAD] Jumping to kernel at ");
    _puthex(g_kernel_info.entry_point);
    _puts("...\n");

    entry(magic, mbi);

    /* Should not reach here */
    _puts("[LOAD] Error: Kernel returned!\n");

    while (1) {
        __asm__ __volatile__("hlt");
    }
}
