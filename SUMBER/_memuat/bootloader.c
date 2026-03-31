/*
 * =============================================================================
 * PIGURA OS - BOOTLOADER.C
 * ========================
 * Implementasi bootloader stage 2 untuk Pigura OS.
 *
 * Berkas ini berisi kode bootloader yang memuat dan mengeksekusi kernel.
 * Bootloader ini didesain untuk kompatibilitas Multiboot.
 *
 * Arsitektur: x86/x86_64
 * Versi: 1.0
 * =============================================================================
 */

#include <stdint.h>
#include <stddef.h>

/* Kompatibilitas C89: inline tidak tersedia */
#ifndef inline
#ifdef __GNUC__
    #define inline __inline__
#else
    #define inline
#endif
#endif

/* =============================================================================
 * KONSTANTA
 * =============================================================================
 */

/* Alamat memori */
#define ALAMAT_KERNEL_LOAD      0x00100000      /* 1 MB - lokasi kernel */
#define ALAMAT_KERNEL_ENTRY     0x00100000      /* Entry point kernel */
#define ALAMAT_BOOT_INFO        0x00007000      /* Info dari bootloader */
#define ALAMAT_STACK            0x00008000      /* Stack untuk bootloader */

/* Ukuran */
#define UKURAN_SEKTOR           512
#define UKURAN_KERNEL_MAX       (1024 * 1024)   /* 1 MB max kernel size */

/* Port I/O */
#define PORT_FLOPPY             0x3F2
#define PORT_KEYBOARD           0x60
#define PORT_VGA_CRTC           0x3D4

/* =============================================================================
 * TIPE DATA
 * =============================================================================
 */

/* Multiboot info structure */
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
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
} multiboot_info_t;

/* Memory map entry */
typedef struct {
    uint32_t size;
    uint32_t base_low;
    uint32_t base_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
} mmap_entry_t;

/* =============================================================================
 * VARIABEL GLOBAL
 * =============================================================================
 */

/* Boot info dari multiboot */
static multiboot_info_t *g_boot_info = NULL;

/* Nama bootloader */
static const char *g_nama_bootloader = "Pigura Bootloader v1.0";

/* Status error */
static int g_error_code = 0;

/* =============================================================================
 * FUNGSI ASSEMBLY INLINE
 * =============================================================================
 */

/*
 * _outb
 * -----
 * Menulis byte ke port I/O.
 */
static inline void _outb(uint8_t nilai, uint16_t port)
{
    __asm__ __volatile__(
        "outb %0, %1"
        :
        : "a"(nilai), "Nd"(port)
    );
}

/*
 * _inb
 * ----
 * Membaca byte dari port I/O.
 */
static inline uint8_t _inb(uint16_t port)
{
    uint8_t nilai;
    __asm__ __volatile__(
        "inb %1, %0"
        : "=a"(nilai)
        : "Nd"(port)
    );
    return nilai;
}

/*
 * _halt
 * -----
 * Menghentikan CPU.
 */
static inline void _halt(void)
{
    __asm__ __volatile__("hlt");
}

/*
 * _enable_interrupts
 * ------------------
 * Mengaktifkan interrupts.
 */
static inline void _enable_interrupts(void)
{
    __asm__ __volatile__("sti");
}

/*
 * _disable_interrupts
 * -------------------
 * Menonaktifkan interrupts.
 */
static inline void _disable_interrupts(void)
{
    __asm__ __volatile__("cli");
}

/*
 * _far_jump
 * ---------
 * Far jump ke segment:offset.
 */
static inline void _far_jump(uint16_t segment, uint32_t offset)
{
    __asm__ __volatile__(
        "ljmp *%0"
        :
        : "m"((struct { uint32_t offset; uint16_t segment; }){ offset, segment })
    );
}

/* =============================================================================
 * FUNGSI UTILITAS
 * =============================================================================
 */

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
 * _memset
 * -------
 * Mengisi memori dengan nilai tertentu.
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
 * _puts
 * -----
 * Menampilkan string ke console (VGA).
 */
static void _puts(const char *str)
{
    volatile uint16_t *vga;
    size_t i;
    static int posisi = 0;

    vga = (volatile uint16_t *)0xB8000;

    for (i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            posisi = (posisi / 80 + 1) * 80;
        } else {
            vga[posisi] = (uint16_t)((0x07 << 8) | str[i]);
            posisi++;
        }

        /* Scroll jika perlu */
        if (posisi >= 80 * 25) {
            int j;
            posisi = 80 * 24;
            /* Simple scroll */
            for (j = 0; j < 80 * 24; j++) {
                vga[j] = vga[j + 80];
            }
            for (j = 80 * 24; j < 80 * 25; j++) {
                vga[j] = (0x07 << 8) | ' ';
            }
        }
    }
}

/*
 * _puthex
 * -------
 * Menampilkan nilai hexadecimal.
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

/*
 * _putdec
 * -------
 * Menampilkan nilai desimal.
 */
static void _putdec(uint32_t val)
{
    char buf[11];
    int i;

    buf[10] = '\0';
    i = 9;

    if (val == 0) {
        buf[i] = '0';
        i--;
    } else {
        while (val > 0 && i >= 0) {
            buf[i] = '0' + (val % 10);
            val /= 10;
            i--;
        }
    }

    _puts(&buf[i + 1]);
}

/* =============================================================================
 * FUNGSI BOOTLOADER
 * =============================================================================
 */

/*
 * _check_a20
 * ----------
 * Memeriksa apakah A20 line aktif.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
static int _check_a20(void)
{
    volatile uint8_t *low;
    volatile uint8_t *high;
    uint8_t save_low;
    uint8_t save_high;

    /* Direct memory access for A20 check */
    low = (volatile uint8_t *)(uintptr_t)0x00000500ULL;
    high = (volatile uint8_t *)(uintptr_t)0x10000500ULL;

    save_low = *low;
    save_high = *high;

    *low = 0x00;
    *high = 0xFF;

    if (*low == *high) {
        *low = save_low;
        *high = save_high;
        return 0;   /* A20 tidak aktif */
    }

    *low = save_low;
    *high = save_high;
    return 1;       /* A20 aktif */
}
#pragma GCC diagnostic pop

/*
 * _enable_a20_fast
 * ----------------
 * Mengaktifkan A20 via fast A20 gate.
 */
static void _enable_a20_fast(void)
{
    uint8_t port_a;

    /* Baca output port dari keyboard controller */
    _outb(0xD0, 0x64);
    while (!(_inb(0x64) & 0x01));
    port_a = _inb(0x60);

    /* Set bit A20 */
    port_a |= 0x02;

    /* Tulis kembali */
    _outb(0xD1, 0x64);
    while (_inb(0x64) & 0x02);
    _outb(port_a, 0x60);
}

/*
 * _enable_a20
 * -----------
 * Mengaktifkan A20 line.
 */
static int _enable_a20(void)
{
    int i;

    /* Cek apakah sudah aktif */
    if (_check_a20()) {
        return 1;
    }

    /* Coba fast A20 */
    _enable_a20_fast();

    for (i = 0; i < 100; i++) {
        if (_check_a20()) {
            return 1;
        }
    }

    return 0;
}

/*
 * _print_banner
 * -------------
 * Menampilkan banner bootloader.
 */
static void _print_banner(void)
{
    _puts("\n");
    _puts("========================================\n");
    _puts("        PIGURA OS BOOTLOADER v1.0\n");
    _puts("========================================\n");
    _puts("\n");
}

/*
 * _print_memory_info
 * ------------------
 * Menampilkan informasi memori.
 */
static void _print_memory_info(void)
{
    if (g_boot_info != NULL) {
        _puts("[BOOT] Memory lower: ");
        _putdec(g_boot_info->mem_lower);
        _puts(" KB\n");

        _puts("[BOOT] Memory upper: ");
        _putdec(g_boot_info->mem_upper);
        _puts(" KB\n");

        _puts("[BOOT] Total: ");
        _putdec((g_boot_info->mem_lower + g_boot_info->mem_upper) / 1024);
        _puts(" MB\n");
    }
}

/*
 * _load_kernel_from_disk
 * ----------------------
 * Memuat kernel dari disk.
 * Catatan: Ini adalah implementasi sederhana.
 */
static int _load_kernel_from_disk(void)
{
    /* Dalam implementasi nyata, ini akan membaca dari disk */
    /* Untuk sekarang, kita asumsikan kernel sudah di memori */

    _puts("[BOOT] Kernel already loaded by stage 1\n");
    return 0;
}

/*
 * _check_kernel
 * -------------
 * Memeriksa validitas kernel.
 */
static int _check_kernel(void)
{
    uint32_t *magic;
    uint32_t kernel_magic = 0x4B524E4C;     /* "KRNL" */

    magic = (uint32_t *)ALAMAT_KERNEL_LOAD;

    if (*magic != kernel_magic) {
        _puts("[BOOT] Warning: Kernel magic not found\n");
        _puts("[BOOT] Expected: ");
        _puthex(kernel_magic);
        _puts(" Found: ");
        _puthex(*magic);
        _puts("\n");
        /* Tidak return error, lanjutkan saja */
    } else {
        _puts("[BOOT] Kernel magic verified\n");
    }

    return 0;
}

/* =============================================================================
 * FUNGSI UTAMA
 * =============================================================================
 */

/*
 * bootloader_main
 * ---------------
 * Entry point utama bootloader stage 2.
 *
 * Parameter:
 *   magic     - Magic number dari multiboot
 *   boot_info - Pointer ke struktur multiboot info
 */
void bootloader_main(uint32_t magic, multiboot_info_t *boot_info)
{
    uint32_t kernel_entry;
    void (*kernel_start)(uint32_t, multiboot_info_t *);

    /* Simpan boot info */
    g_boot_info = boot_info;

    /* Disable interrupts selama setup */
    _disable_interrupts();

    /* Tampilkan banner */
    _print_banner();

    /* Verifikasi multiboot magic */
    if (magic != 0x2BADB002) {
        _puts("[BOOT] Error: Invalid multiboot magic: ");
        _puthex(magic);
        _puts("\n");
        _halt();
        return;
    }

    _puts("[BOOT] Multiboot magic verified\n");

    /* Tampilkan info memori */
    _print_memory_info();

    /* Aktifkan A20 line */
    _puts("[BOOT] Enabling A20 line...\n");
    if (!_enable_a20()) {
        _puts("[BOOT] Error: Failed to enable A20\n");
        _halt();
        return;
    }
    _puts("[BOOT] A20 line enabled\n");

    /* Muat kernel (jika perlu) */
    if (_load_kernel_from_disk() != 0) {
        _puts("[BOOT] Error: Failed to load kernel\n");
        _halt();
        return;
    }

    /* Verifikasi kernel */
    if (_check_kernel() != 0) {
        _puts("[BOOT] Error: Kernel verification failed\n");
        _halt();
        return;
    }

    /* Tampilkan informasi */
    _puts("[BOOT] Kernel loaded at: ");
    _puthex(ALAMAT_KERNEL_LOAD);
    _puts("\n");

    _puts("[BOOT] Boot info at: ");
    _puthex((uint32_t)(uintptr_t)boot_info);
    _puts("\n");

    _puts("[BOOT] Jumping to kernel...\n\n");

    /* Set entry point */
    kernel_entry = ALAMAT_KERNEL_ENTRY;
    kernel_start = (void (*)(uint32_t, multiboot_info_t *))(uintptr_t)kernel_entry;

    /* Enable interrupts sebelum lompat */
    _enable_interrupts();

    /* Lompat ke kernel */
    kernel_start(magic, boot_info);

    /* Tidak akan sampai sini */
    _puts("[BOOT] Error: Kernel returned\n");
    _halt();
}

/*
 * bootloader_error
 * ----------------
 * Handler untuk error bootloader.
 */
void bootloader_error(int code, const char *msg)
{
    _puts("\n[BOOT] ERROR: ");
    _puts(msg);
    _puts("\n");
    _puts("[BOOT] Error code: ");
    _putdec((uint32_t)code);
    _puts("\n");
    _puts("[BOOT] System halted.\n");

    g_error_code = code;
    _halt();
}

/*
 * bootloader_get_info
 * -------------------
 * Mendapatkan boot info.
 */
multiboot_info_t *bootloader_get_info(void)
{
    return g_boot_info;
}

/*
 * bootloader_get_name
 * -------------------
 * Mendapatkan nama bootloader.
 */
const char *bootloader_get_name(void)
{
    return g_nama_bootloader;
}
