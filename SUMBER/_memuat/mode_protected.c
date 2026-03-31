/*
 * =============================================================================
 * PIGURA OS - MODE_PROTECTED.C
 * ============================
 * Implementasi transisi ke Protected Mode (32-bit) untuk Pigura OS.
 *
 * Berkas ini berisi fungsi-fungsi untuk melakukan transisi dari
 * Real Mode (16-bit) ke Protected Mode (32-bit) pada arsitektur x86.
 *
 * CATATAN: Berkas ini hanya untuk x86 32-bit. Untuk x86_64, gunakan
 *          mode_long.c untuk transisi ke long mode.
 *
 * Arsitektur: x86 (32-bit only)
 * Versi: 1.0
 * =============================================================================
 */

/* Hanya untuk x86 32-bit */
#if defined(__i386__) && !defined(__x86_64__)

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

/* Control register 0 bits */
#define CR0_PE          0x00000001      /* Protection Enable */
#define CR0_MP          0x00000002      /* Monitor Co-processor */
#define CR0_EM          0x00000004      /* Emulation */
#define CR0_TS          0x00000008      /* Task Switched */
#define CR0_ET          0x00000010      /* Extension Type */
#define CR0_NE          0x00000020      /* Numeric Error */
#define CR0_WP          0x00010000      /* Write Protect */
#define CR0_AM          0x00040000      /* Alignment Mask */
#define CR0_NW          0x20000000      /* Not Write-through */
#define CR0_CD          0x40000000      /* Cache Disable */
#define CR0_PG          0x80000000      /* Paging Enable */

/* Segment selectors */
#define SELECTOR_KERNEL_CS      0x08
#define SELECTOR_KERNEL_DS      0x10

/* MSR addresses */
#define MSR_EFER                0xC0000080
#define MSR_EFER_LME            0x00000100      /* Long Mode Enable */

/* Ports */
#define PORT_PIC1_CMD           0x20
#define PORT_PIC2_CMD           0xA0
#define PORT_PIC1_DATA          0x21
#define PORT_PIC2_DATA          0xA1
#define PORT_KBD_DATA           0x60
#define PORT_KBD_CMD            0x64

/* =============================================================================
 * TIPE DATA
 * =============================================================================
 */

/* GDT entry untuk protected mode */
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t flags_limit_high;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_pm_t;

/* GDT pointer */
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_ptr_pm_t;

/* =============================================================================
 * VARIABEL GLOBAL
 * =============================================================================
 */

/* GDT untuk protected mode */
static gdt_entry_pm_t g_gdt_pm[3];

/* GDT pointer */
static gdt_ptr_pm_t g_gdt_ptr_pm;

/* Status protected mode */
static int g_protected_mode_enabled = 0;

/* =============================================================================
 * FUNGSI INLINE ASSEMBLY
 * =============================================================================
 */

/*
 * _read_cr0
 * ---------
 * Baca CR0 register.
 */
static inline uint32_t _read_cr0(void)
{
    uint32_t val;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(val));
    return val;
}

/*
 * _write_cr0
 * ----------
 * Tulis CR0 register.
 */
static inline void _write_cr0(uint32_t val)
{
    __asm__ __volatile__(
        "mov %0, %%cr0"
        :
        : "r"(val)
        : "memory"
    );
}

/*
 * _read_cr2
 * ---------
 * Baca CR2 register (page fault address).
 */
static inline uint32_t _read_cr2(void)
{
    uint32_t val;
    __asm__ __volatile__("mov %%cr2, %0" : "=r"(val));
    return val;
}

/*
 * _read_cr3
 * ---------
 * Baca CR3 register (page directory).
 */
static inline uint32_t _read_cr3(void)
{
    uint32_t val;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(val));
    return val;
}

/*
 * _write_cr3
 * ----------
 * Tulis CR3 register.
 */
static inline void _write_cr3(uint32_t val)
{
    __asm__ __volatile__(
        "mov %0, %%cr3"
        :
        : "r"(val)
        : "memory"
    );
}

/*
 * _read_cr4
 * ---------
 * Baca CR4 register.
 */
static inline uint32_t _read_cr4(void)
{
    uint32_t val;
    __asm__ __volatile__("mov %%cr4, %0" : "=r"(val));
    return val;
}

/*
 * _write_cr4
 * ----------
 * Tulis CR4 register.
 */
static inline void _write_cr4(uint32_t val)
{
    __asm__ __volatile__(
        "mov %0, %%cr4"
        :
        : "r"(val)
        : "memory"
    );
}

/*
 * _outb
 * -----
 * Tulis byte ke port I/O.
 */
static inline void _outb(uint8_t val, uint16_t port)
{
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

/*
 * _inb
 * ----
 * Baca byte dari port I/O.
 */
static inline uint8_t _inb(uint16_t port)
{
    uint8_t val;
    __asm__ __volatile__("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/*
 * _cli
 * ----
 * Disable interrupts.
 */
static inline void _cli(void)
{
    __asm__ __volatile__("cli");
}

/*
 * _sti
 * ----
 * Enable interrupts.
 */
static inline void _sti(void)
{
    __asm__ __volatile__("sti");
}

/*
 * _lgdt
 * -----
 * Load GDT register.
 */
static inline void _lgdt(gdt_ptr_pm_t *ptr)
{
    __asm__ __volatile__("lgdt %0" : : "m"(*ptr));
}

/*
 * _lidt
 * -----
 * Load IDT register.
 */
static inline void _lidt(void *ptr)
{
    __asm__ __volatile__("lidt %0" : : "m"(*(uint32_t*)ptr));
}

/*
 * _jmp_far
 * --------
 * Far jump ke code segment.
 */
static inline void _jmp_far(uint16_t cs, uint32_t addr)
{
    (void)cs;  /* Unused in this implementation */
    (void)addr;  /* Unused parameter */
    __asm__ __volatile__(
        "ljmp %0, $1f\n\t"
        "1:"
        :
        : "i"(cs)
        : "memory"
    );
}

/* =============================================================================
 * FUNGSI INTERNAL
 * =============================================================================
 */

/*
 * _setup_gdt_pm
 * -------------
 * Setup GDT untuk protected mode.
 */
static void _setup_gdt_pm(void)
{
    /* Null descriptor */
    g_gdt_pm[0].limit_low = 0;
    g_gdt_pm[0].base_low = 0;
    g_gdt_pm[0].base_middle = 0;
    g_gdt_pm[0].access = 0;
    g_gdt_pm[0].flags_limit_high = 0;
    g_gdt_pm[0].base_high = 0;

    /* Kernel code segment: base=0, limit=4GB, 32-bit, read+exec */
    g_gdt_pm[1].limit_low = 0xFFFF;
    g_gdt_pm[1].base_low = 0;
    g_gdt_pm[1].base_middle = 0;
    g_gdt_pm[1].access = 0x9A;      /* Present, Ring 0, Code, Executable, Readable */
    g_gdt_pm[1].flags_limit_high = 0xCF;    /* 4KB granularity, 32-bit, limit 0xF */
    g_gdt_pm[1].base_high = 0;

    /* Kernel data segment: base=0, limit=4GB, 32-bit, read+write */
    g_gdt_pm[2].limit_low = 0xFFFF;
    g_gdt_pm[2].base_low = 0;
    g_gdt_pm[2].base_middle = 0;
    g_gdt_pm[2].access = 0x92;      /* Present, Ring 0, Data, Writable */
    g_gdt_pm[2].flags_limit_high = 0xCF;    /* 4KB granularity, 32-bit, limit 0xF */
    g_gdt_pm[2].base_high = 0;

    /* Set GDT pointer */
    g_gdt_ptr_pm.limit = sizeof(g_gdt_pm) - 1;
    g_gdt_ptr_pm.base = (uint32_t)(uintptr_t)&g_gdt_pm;
}

/*
 * _disable_pic
 * ------------
 * Disable PIC (untuk debugging tanpa interrupts).
 */
static void _disable_pic(void)
{
    /* Mask semua IRQ */
    _outb(0xFF, PORT_PIC1_DATA);
    _outb(0xFF, PORT_PIC2_DATA);
}

/*
 * _enable_a20
 * -----------
 * Enable A20 line.
 */
static int _enable_a20(void)
{
    uint8_t status;
    int timeout;
    int i;

    /* Cek apakah A20 sudah aktif */
    /* Method: bandingkan alamat 0x0:0x500 dan 0xFFFF:0x0510 */

    /* Method 1: Keyboard controller */
    timeout = 100000;

    /* Disable keyboard */
    _outb(0xAD, PORT_KBD_CMD);
    while (--timeout) {
        status = _inb(PORT_KBD_CMD);
        if ((status & 0x02) == 0) {
            break;
        }
    }

    if (timeout == 0) {
        return -1;
    }

    /* Read output port */
    _outb(0xD0, PORT_KBD_CMD);
    timeout = 100000;
    while (--timeout) {
        status = _inb(PORT_KBD_CMD);
        if ((status & 0x01) != 0) {
            break;
        }
    }

    if (timeout == 0) {
        return -1;
    }

    status = _inb(PORT_KBD_DATA);

    /* Set A20 bit */
    status |= 0x02;

    /* Write output port */
    _outb(0xD1, PORT_KBD_CMD);
    timeout = 100000;
    while (--timeout) {
        uint8_t s = _inb(PORT_KBD_CMD);
        if ((s & 0x02) == 0) {
            break;
        }
    }

    _outb(status, PORT_KBD_DATA);

    /* Enable keyboard */
    _outb(0xAE, PORT_KBD_CMD);

    /* Wait for A20 to be enabled */
    for (i = 0; i < 10000; i++) {
        /* Small delay */
        volatile int j;
        for (j = 0; j < 100; j++);
    }

    return 0;
}

/* =============================================================================
 * FUNGSI PUBLIC
 * =============================================================================
 */

/*
 * mode_protected_init
 * -------------------
 * Inisialisasi untuk transisi ke protected mode.
 */
int mode_protected_init(void)
{
    /* Setup GDT */
    _setup_gdt_pm();

    /* Disable interrupts */
    _cli();

    /* Disable PIC */
    _disable_pic();

    /* Enable A20 line */
    if (_enable_a20() != 0) {
        return -1;
    }

    return 0;
}

/*
 * mode_protected_enter
 * --------------------
 * Masuk ke protected mode.
 *
 * Parameter:
 *   entry_point - Alamat entry point kernel
 */
void mode_protected_enter(uint32_t entry_point)
{
    uint32_t cr0;

    /* Load GDT */
    _lgdt(&g_gdt_ptr_pm);

    /* Set PE bit in CR0 */
    cr0 = _read_cr0();
    cr0 |= CR0_PE;
    _write_cr0(cr0);

    /* Far jump ke protected mode */
    __asm__ __volatile__(
        "ljmpl $0x08, $1f\n\t"
        "1:\n\t"
        "movw $0x10, %%ax\n\t"
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"
        "movw %%ax, %%ss"
        :
        :
        : "ax", "memory"
    );

    g_protected_mode_enabled = 1;

    /* Jump ke kernel entry point */
    __asm__ __volatile__(
        "jmp *%0"
        :
        : "r"(entry_point)
        : "memory"
    );
}

/*
 * mode_protected_is_enabled
 * -------------------------
 * Cek apakah protected mode aktif.
 */
int mode_protected_is_enabled(void)
{
    return g_protected_mode_enabled;
}

/*
 * mode_protected_enable_paging
 * ----------------------------
 * Enable paging di protected mode.
 *
 * Parameter:
 *   page_dir - Alamat page directory
 */
void mode_protected_enable_paging(uint32_t page_dir)
{
    uint32_t cr0;
    uint32_t cr4;

    /* Load CR3 dengan page directory */
    _write_cr3(page_dir);

    /* Enable PSE (Page Size Extension) jika tersedia */
    cr4 = _read_cr4();
    cr4 |= 0x00000010;  /* PSE bit */
    _write_cr4(cr4);

    /* Enable paging */
    cr0 = _read_cr0();
    cr0 |= CR0_PG;
    _write_cr0(cr0);
}

/*
 * mode_protected_disable_paging
 * -----------------------------
 * Disable paging.
 */
void mode_protected_disable_paging(void)
{
    uint32_t cr0;

    cr0 = _read_cr0();
    cr0 &= ~CR0_PG;
    _write_cr0(cr0);
}

/*
 * mode_protected_get_cr0
 * ----------------------
 * Dapatkan nilai CR0.
 */
uint32_t mode_protected_get_cr0(void)
{
    return _read_cr0();
}

/*
 * mode_protected_get_cr2
 * ----------------------
 * Dapatkan nilai CR2 (page fault address).
 */
uint32_t mode_protected_get_cr2(void)
{
    return _read_cr2();
}

/*
 * mode_protected_get_cr3
 * ----------------------
 * Dapatkan nilai CR3 (page directory).
 */
uint32_t mode_protected_get_cr3(void)
{
    return _read_cr3();
}

/*
 * mode_protected_get_cr4
 * ----------------------
 * Dapatkan nilai CR4.
 */
uint32_t mode_protected_get_cr4(void)
{
    return _read_cr4();
}

/*
 * mode_protected_invalidate_tlb
 * -----------------------------
 * Invalidate TLB.
 */
void mode_protected_invalidate_tlb(void)
{
    uint32_t cr3;

    cr3 = _read_cr3();
    _write_cr3(cr3);
}

/*
 * mode_protected_invalidate_tlb_page
 * ----------------------------------
 * Invalidate TLB untuk satu halaman.
 */
void mode_protected_invalidate_tlb_page(uint32_t addr)
{
    __asm__ __volatile__(
        "invlpg %0"
        :
        : "m"(*(uint8_t *)(uintptr_t)addr)
        : "memory"
    );
}

/*
 * mode_protected_set_segment
 * --------------------------
 * Set segment registers.
 */
void mode_protected_set_segment(uint16_t ds, uint16_t es,
                                 uint16_t fs, uint16_t gs,
                                 uint16_t ss)
{
    __asm__ __volatile__(
        "movw %0, %%ds\n\t"
        "movw %1, %%es\n\t"
        "movw %2, %%fs\n\t"
        "movw %3, %%gs\n\t"
        "movw %4, %%ss"
        :
        : "r"(ds), "r"(es), "r"(fs), "r"(gs), "r"(ss)
        : "memory"
    );
}

#endif /* __i386__ && !__x86_64__ */
