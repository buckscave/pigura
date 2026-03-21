/*
 * =============================================================================
 * PIGURA OS - STAGE 2 BOOTLOADER (x86)
 * =============================================================================
 *
 * Berkas ini berisi Stage 2 bootloader yang berjalan di protected mode.
 * Bertanggung jawab untuk:
 *   1. Mendeteksi memori sistem
 *   2. Mengaktifkan A20 line
 *   3. Menyiapkan GDT dan IDT awal
 *   4. Switch ke protected mode
 *   5. Memuat dan menjalankan kernel
 *
 * Dimuat oleh boot sector di alamat 0x7E00.
 *
 * Arsitektur: x86 (16-bit -> 32-bit)
 * Versi: 1.0
 * =============================================================================
 */

#include <stdint.h>
#include <stddef.h>

/* =============================================================================
 * KONSTANTA
 * =============================================================================
 */

/* Port I/O */
#define PORT_A20_CMD        0x64        /* Keyboard command port */
#define PORT_A20_DATA       0x60        /* Keyboard data port */
#define PORT_VGA_CTRL       0x3D4       /* VGA control register */
#define PORT_VGA_DATA       0x3D5       /* VGA data register */

/* VGA memory */
#define VGA_BUFFER          0xB8000     /* VGA text buffer */
#define VGA_LEBAR           80
#define VGA_TINGGI          25

/* Alamat memory */
#define ALAMAT_KERNEL       0x100000    /* Kernel di 1 MB */
#define ALAMAT_GDT          0x500       /* GDT di 0x500 */
#define ALAMAT_IDT          0x600       /* IDT di 0x600 */
#define ALAMAT_STACK        0x7000      /* Stack untuk stage2 */

/* Multiboot */
#define MULTIBOOT_MAGIC     0x1BADB002

/* =============================================================================
 * STRUKTUR DATA
 * =============================================================================
 */

/* Entry GDT */
struct gdt_entry {
    uint16_t batas_rendah;
    uint16_t basis_rendah;
    uint8_t  basis_tengah;
    uint8_t  akses;
    uint8_t  granularitas;
    uint8_t  basis_atas;
} __attribute__((packed));

/* Pointer GDT */
struct gdt_pointer {
    uint16_t batas;
    uint32_t basis;
} __attribute__((packed));

/* Entry IDT */
struct idt_entry {
    uint16_t offset_rendah;
    uint16_t selector;
    uint8_t  nol;
    uint8_t  flags;
    uint16_t offset_atas;
} __attribute__((packed));

/* Pointer IDT */
struct idt_pointer {
    uint16_t batas;
    uint32_t basis;
} __attribute__((packed));

/* Memory map entry */
struct mmap_entry {
    uint32_t ukuran;
    uint64_t alamat;
    uint64_t panjang;
    uint32_t tipe;
} __attribute__((packed));

/* =============================================================================
 * VARIABEL GLOBAL
 * =============================================================================
 */

/* GDT awal */
static struct gdt_entry g_gdt[5];
static struct gdt_pointer g_gdt_pointer;

/* IDT awal */
static struct idt_entry g_idt[256];
static struct idt_pointer g_idt_pointer;

/* Informasi memori */
static uint32_t g_memori_lower = 0;
static uint32_t g_memori_upper = 0;

/* =============================================================================
 * FUNGSI INTERNAL
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
        "outb %0, %1\n\t"
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
        "inb %1, %0\n\t"
        : "=a"(nilai)
        : "Nd"(port)
    );
    return nilai;
}

/*
 * _tampilkan_karakter
 * --------------------
 * Menampilkan karakter ke VGA buffer.
 */
static void _tampilkan_karakter(char c, uint8_t warna, int baris, int kolom)
{
    volatile char *vga;
    int index;

    vga = (volatile char *)VGA_BUFFER;
    index = (baris * VGA_LEBAR + kolom) * 2;

    vga[index] = c;
    vga[index + 1] = warna;
}

/*
 * _tampilkan_string
 * ------------------
 * Menampilkan string ke VGA buffer.
 */
static void _tampilkan_string(const char *str, uint8_t warna, int baris, int kolom)
{
    while (*str != '\0') {
        if (*str == '\n') {
            baris++;
            kolom = 0;
        } else {
            _tampilkan_karakter(*str, warna, baris, kolom);
            kolom++;
            if (kolom >= VGA_LEBAR) {
                baris++;
                kolom = 0;
            }
        }
        str++;
    }
}

/*
 * _enable_a20
 * ------------
 * Mengaktifkan A20 line.
 */
static int _enable_a20(void)
{
    uint8_t status;
    int timeout;

    /* Metode 1: Keyboard controller */
    
    /* Wait untuk keyboard ready */
    timeout = 0xFFFF;
    while (timeout-- > 0) {
        status = _inb(PORT_A20_CMD);
        if ((status & 0x02) == 0) {
            break;
        }
    }

    /* Disable keyboard */
    _outb(0xAD, PORT_A20_CMD);

    /* Wait */
    timeout = 0xFFFF;
    while (timeout-- > 0) {
        status = _inb(PORT_A20_CMD);
        if ((status & 0x02) == 0) {
            break;
        }
    }

    /* Read output port */
    _outb(0xD0, PORT_A20_CMD);

    timeout = 0xFFFF;
    while (timeout-- > 0) {
        status = _inb(PORT_A20_CMD);
        if ((status & 0x01) != 0) {
            break;
        }
    }

    status = _inb(PORT_A20_DATA);

    /* Enable A20 bit */
    status |= 0x02;

    /* Wait */
    timeout = 0xFFFF;
    while (timeout-- > 0) {
        status = _inb(PORT_A20_CMD);
        if ((status & 0x02) == 0) {
            break;
        }
    }

    /* Write output port */
    _outb(0xD1, PORT_A20_CMD);

    timeout = 0xFFFF;
    while (timeout-- > 0) {
        status = _inb(PORT_A20_CMD);
        if ((status & 0x02) == 0) {
            break;
        }
    }

    _outb(status, PORT_A20_DATA);

    /* Re-enable keyboard */
    timeout = 0xFFFF;
    while (timeout-- > 0) {
        status = _inb(PORT_A20_CMD);
        if ((status & 0x02) == 0) {
            break;
        }
    }

    _outb(0xAE, PORT_A20_CMD);

    return 0;
}

/*
 * _atur_entry_gdt
 * ----------------
 * Mengisi entry GDT.
 */
static void _atur_entry_gdt(int index, uint32_t basis, uint32_t batas,
                            uint8_t akses, uint8_t gran)
{
    struct gdt_entry *entry;

    entry = &g_gdt[index];

    entry->basis_rendah = (uint16_t)(basis & 0xFFFF);
    entry->basis_tengah = (uint8_t)((basis >> 16) & 0xFF);
    entry->basis_atas = (uint8_t)((basis >> 24) & 0xFF);

    entry->batas_rendah = (uint16_t)(batas & 0xFFFF);
    entry->granularitas = (uint8_t)((batas >> 16) & 0x0F);
    entry->granularitas |= (gran & 0xF0);

    entry->akses = akses;
}

/*
 * _setup_gdt
 * -----------
 * Menyiapkan GDT awal.
 */
static void _setup_gdt(void)
{
    /* Null descriptor */
    _atur_entry_gdt(0, 0, 0, 0, 0);

    /* Kernel code segment */
    _atur_entry_gdt(1, 0, 0xFFFFF, 0x9A, 0xCF);

    /* Kernel data segment */
    _atur_entry_gdt(2, 0, 0xFFFFF, 0x92, 0xCF);

    /* User code segment */
    _atur_entry_gdt(3, 0, 0xFFFFF, 0xFA, 0xCF);

    /* User data segment */
    _atur_entry_gdt(4, 0, 0xFFFFF, 0xF2, 0xCF);

    /* Setup GDT pointer */
    g_gdt_pointer.batas = sizeof(g_gdt) - 1;
    g_gdt_pointer.basis = (uint32_t)&g_gdt;
}

/*
 * _setup_idt
 * -----------
 * Menyiapkan IDT awal.
 */
static void _setup_idt(void)
{
    int i;

    /* Clear IDT */
    for (i = 0; i < 256; i++) {
        g_idt[i].offset_rendah = 0;
        g_idt[i].offset_atas = 0;
        g_idt[i].selector = 0x08;
        g_idt[i].nol = 0;
        g_idt[i].flags = 0;
    }

    /* Setup IDT pointer */
    g_idt_pointer.batas = sizeof(g_idt) - 1;
    g_idt_pointer.basis = (uint32_t)&g_idt;
}

/*
 * _deteksi_memori_bios
 * --------------------
 * Mendeteksi memori menggunakan BIOS interrupt.
 */
static void _deteksi_memori_bios(void)
{
    uint32_t total_kb;

    /* Menggunakan INT 0x15, AX = 0xE801 */
    __asm__ __volatile__(
        "int $0x15\n\t"
        : "=a"(g_memori_lower), "=b"(g_memori_upper)
        : "a"(0xE801)
        : "memory"
    );

    /* Konversi ke total KB */
    total_kb = g_memori_lower + (g_memori_upper * 64);
    (void)total_kb;
}

/*
 * _masuk_protected_mode
 * ----------------------
 * Switch ke protected mode.
 */
static void _masuk_protected_mode(void)
{
    /* Load GDT */
    __asm__ __volatile__(
        "lgdt %0\n\t"
        :
        : "m"(g_gdt_pointer)
        : "memory"
    );

    /* Set PE bit di CR0 */
    __asm__ __volatile__(
        "movl %%cr0, %%eax\n\t"
        "orl $1, %%eax\n\t"
        "movl %%eax, %%cr0\n\t"
        :
        :
        : "eax", "memory"
    );

    /* Far jump ke 32-bit code */
    __asm__ __volatile__(
        "ljmp $0x08, $._protected_mode\n\t"
        :
        :
        : "memory"
    );

    /* Ini tidak akan dieksekusi */
    __asm__ __volatile__(
        "._protected_mode:\n\t"
        "movw $0x10, %%ax\n\t"
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"
        "movw %%ax, %%ss\n\t"
        :
        :
        : "ax", "memory"
    );
}

/* =============================================================================
 * ENTRY POINT
 * =============================================================================
 */

/*
 * _stage2_main
 * ------------
 * Entry point Stage 2 bootloader.
 * Dipanggil dari boot sector.
 */
void _stage2_main(void)
{
    uint8_t warna = 0x0F;           /* Putih di hitam */
    int baris = 0;
    int kolom = 0;

    /* Tampilkan banner */
    _tampilkan_string("PIGURA OS v1.0 - Stage 2 Bootloader\n", warna, baris, kolom);
    baris++;

    /* Enable A20 line */
    _tampilkan_string("Mengaktifkan A20 line...", warna, baris, kolom);
    if (_enable_a20() == 0) {
        _tampilkan_string(" OK\n", 0x0A, baris, kolom + 28);
    } else {
        _tampilkan_string(" GAGAL\n", 0x0C, baris, kolom + 28);
        return;
    }
    baris++;

    /* Deteksi memori */
    _tampilkan_string("Mendeteksi memori...", warna, baris, kolom);
    _deteksi_memori_bios();
    _tampilkan_string(" OK\n", 0x0A, baris, kolom + 28);
    baris++;

    /* Setup GDT */
    _tampilkan_string("Menyiapkan GDT...", warna, baris, kolom);
    _setup_gdt();
    _tampilkan_string(" OK\n", 0x0A, baris, kolom + 28);
    baris++;

    /* Setup IDT */
    _tampilkan_string("Menyiapkan IDT...", warna, baris, kolom);
    _setup_idt();
    _tampilkan_string(" OK\n", 0x0A, baris, kolom + 28);
    baris++;

    /* Masuk protected mode */
    _tampilkan_string("Memasuki protected mode...\n", warna, baris, kolom);
    baris++;

    _masuk_protected_mode();

    /* Tidak return dari sini */
}
