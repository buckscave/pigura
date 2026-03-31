/*
 * =============================================================================
 * PIGURA OS - MODE_LONG.C
 * =======================
 * Implementasi transisi ke Long Mode (64-bit) untuk Pigura OS.
 *
 * Berkas ini berisi fungsi-fungsi untuk melakukan transisi dari
 * Protected Mode (32-bit) ke Long Mode (64-bit) pada arsitektur x86_64.
 *
 * Arsitektur: x86_64
 * Versi: 1.0
 * =============================================================================
 */

#include <stdint.h>
#include <stddef.h>
#include "types.h"

/* =============================================================================
 * KONSTANTA
 * =============================================================================
 */

/* Control register bits */
#define CR0_PE          0x00000001      /* Protection Enable */
#define CR0_PG          0x80000000      /* Paging Enable */

#define CR4_PAE         0x00000020      /* Physical Address Extension */
#define CR4_PGE         0x00000080      /* Page Global Enable */
#define CR4_PSE         0x00000010      /* Page Size Extension */

/* MSR addresses */
#define MSR_EFER        0xC0000080
#define MSR_STAR        0xC0000081
#define MSR_LSTAR       0xC0000082
#define MSR_CSTAR       0xC0000083
#define MSR_SFMASK      0xC0000084
#define MSR_FS_BASE     0xC0000100
#define MSR_GS_BASE     0xC0000101

/* EFER bits */
#define EFER_SCE        0x00000001      /* System Call Extensions */
#define EFER_LME        0x00000100      /* Long Mode Enable */
#define EFER_LMA        0x00000400      /* Long Mode Active */
#define EFER_NXE        0x00000800      /* No-Execute Enable */
#define EFER_SVME       0x00001000      /* Secure Virtual Machine Enable */
#define EFER_FFXSR      0x00004000      /* Fast FXSAVE/FXRSTOR */

/* Page table entry flags */
#define PTE_PRESENT     0x001
#define PTE_WRITABLE    0x002
#define PTE_USER        0x004
#define PTE_PWT         0x008           /* Write-Through */
#define PTE_PCD         0x010           /* Cache Disable */
#define PTE_ACCESSED    0x020
#define PTE_DIRTY       0x040
#define PTE_PS          0x080           /* Page Size (2MB pages) */
#define PTE_GLOBAL      0x100
#define PTE_NX          0x8000000000000000ULL

/* Segment selectors */
#define SELECTOR_KERNEL_CS64    0x08
#define SELECTOR_KERNEL_DS64    0x10
#define SELECTOR_USER_CS64      0x18
#define SELECTOR_USER_DS64      0x20

/* =============================================================================
 * TIPE DATA
 * =============================================================================
 */

/* Page table entry (64-bit) */
typedef uint64_t pte64_t;

/* GDT entry untuk long mode */
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t flags_limit_high;
    uint8_t base_high;
    uint32_t base_upper;        /* For 64-bit base */
    uint32_t reserved;
} __attribute__((packed)) gdt_entry64_t;

/* GDT pointer (64-bit) */
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdt_ptr64_t;

/* IDT pointer (64-bit) */
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_ptr64_t;

/* =============================================================================
 * VARIABEL GLOBAL
 * =============================================================================
 */

/* Status long mode */
static int g_long_mode_enabled = 0;

/* GDT pointer */
static gdt_ptr64_t g_gdt_ptr64;

/*
 * Variabel berikut hanya digunakan pada x86_64.
 * Ditempatkan di luar guard agar stub 32-bit tetap dapat mengakses g_gdt_ptr64.
 */

#ifndef __i386__

/* GDT untuk long mode */
static gdt_entry64_t g_gdt64[5] __attribute__((aligned(16)));

/* Page tables (ditempatkan di alamat tertentu) */
static pte64_t g_pml4[512] __attribute__((aligned(4096)));
static pte64_t g_pdpt[512] __attribute__((aligned(4096)));
static pte64_t g_pd[512] __attribute__((aligned(4096)));

/* =============================================================================
 * FUNGSI INLINE ASSEMBLY
 * =============================================================================
 */

/*
 * _read_cr0
 * ---------
 */
static inline uint64_t _read_cr0(void)
{
    uint64_t val;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(val));
    return val;
}

/*
 * _write_cr0
 * ----------
 */
static inline void _write_cr0(uint64_t val)
{
    __asm__ __volatile__("mov %0, %%cr0" : : "r"(val) : "memory");
}

/*
 * _read_cr3
 * ---------
 */
static inline uint64_t _read_cr3(void)
{
    uint64_t val;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(val));
    return val;
}

/*
 * _write_cr3
 * ----------
 */
static inline void _write_cr3(uint64_t val)
{
    __asm__ __volatile__("mov %0, %%cr3" : : "r"(val) : "memory");
}

/*
 * _read_cr4
 * ---------
 */
static inline uint64_t _read_cr4(void)
{
    uint64_t val;
    __asm__ __volatile__("mov %%cr4, %0" : "=r"(val));
    return val;
}

/*
 * _write_cr4
 * ----------
 */
static inline void _write_cr4(uint64_t val)
{
    __asm__ __volatile__("mov %0, %%cr4" : : "r"(val) : "memory");
}

/*
 * _read_msr
 * ---------
 */
static inline uint64_t _read_msr(uint32_t msr)
{
    uint32_t low;
    uint32_t high;

    __asm__ __volatile__(
        "rdmsr"
        : "=a"(low), "=d"(high)
        : "c"(msr)
    );

    return ((uint64_t)high << 32) | low;
}

/*
 * _write_msr
 * ----------
 */
static inline void _write_msr(uint32_t msr, uint64_t val)
{
    uint32_t low;
    uint32_t high;

    low = (uint32_t)(val & 0xFFFFFFFF);
    high = (uint32_t)((val >> 32) & 0xFFFFFFFF);

    __asm__ __volatile__(
        "wrmsr"
        :
        : "a"(low), "d"(high), "c"(msr)
    );
}

/*
 * _cli
 * ----
 */
static inline void _cli(void)
{
    __asm__ __volatile__("cli");
}

/*
 * _lgdt64
 * -------
 */
static inline void _lgdt64(gdt_ptr64_t *ptr)
{
    __asm__ __volatile__("lgdt %0" : : "m"(*ptr));
}

/* =============================================================================
 * FUNGSI INTERNAL
 * =============================================================================
 */

/*
 * _setup_gdt64
 * ------------
 * Setup GDT untuk long mode.
 */
static void _setup_gdt64(void)
{
    int i;
    
    /* Clear GDT */
    for (i = 0; i < 5; i++) {
        g_gdt64[i].limit_low = 0;
        g_gdt64[i].base_low = 0;
        g_gdt64[i].base_middle = 0;
        g_gdt64[i].access = 0;
        g_gdt64[i].flags_limit_high = 0;
        g_gdt64[i].base_high = 0;
        g_gdt64[i].base_upper = 0;
        g_gdt64[i].reserved = 0;
    }

    /* Null descriptor */
    /* Already zero */

    /* Kernel code segment (64-bit) */
    g_gdt64[1].limit_low = 0;
    g_gdt64[1].base_low = 0;
    g_gdt64[1].base_middle = 0;
    g_gdt64[1].access = 0x9A;              /* Present, Ring 0, Code, Executable */
    g_gdt64[1].flags_limit_high = 0xA0;    /* 64-bit code segment */
    g_gdt64[1].base_high = 0;

    /* Kernel data segment */
    g_gdt64[2].limit_low = 0;
    g_gdt64[2].base_low = 0;
    g_gdt64[2].base_middle = 0;
    g_gdt64[2].access = 0x92;              /* Present, Ring 0, Data, Writable */
    g_gdt64[2].flags_limit_high = 0x00;    /* Data segment */
    g_gdt64[2].base_high = 0;

    /* User code segment (64-bit) */
    g_gdt64[3].limit_low = 0;
    g_gdt64[3].base_low = 0;
    g_gdt64[3].base_middle = 0;
    g_gdt64[3].access = 0xFA;              /* Present, Ring 3, Code, Executable */
    g_gdt64[3].flags_limit_high = 0xA0;    /* 64-bit code segment */
    g_gdt64[3].base_high = 0;

    /* User data segment */
    g_gdt64[4].limit_low = 0;
    g_gdt64[4].base_low = 0;
    g_gdt64[4].base_middle = 0;
    g_gdt64[4].access = 0xF2;              /* Present, Ring 3, Data, Writable */
    g_gdt64[4].flags_limit_high = 0x00;    /* Data segment */
    g_gdt64[4].base_high = 0;

    /* Set GDT pointer */
    g_gdt_ptr64.limit = sizeof(g_gdt64) - 1;
    g_gdt_ptr64.base = (uint64_t)(uintptr_t)&g_gdt64;
}

/*
 * _setup_page_tables
 * ------------------
 * Setup page tables untuk identity mapping.
 */
static void _setup_page_tables(void)
{
    int i;
    uint64_t addr;

    /* Clear page tables */
    for (i = 0; i < 512; i++) {
        g_pml4[i] = 0;
        g_pdpt[i] = 0;
        g_pd[i] = 0;
    }

    /* Setup PML4[0] -> PDPT */
    g_pml4[0] = ((uint64_t)(uintptr_t)g_pdpt) |
                PTE_PRESENT | PTE_WRITABLE;

    /* Setup PDPT[0] -> PD */
    g_pdpt[0] = ((uint64_t)(uintptr_t)g_pd) |
                PTE_PRESENT | PTE_WRITABLE;

    /* Setup PD entries with 2MB pages (identity mapping first 1GB) */
    for (i = 0; i < 512; i++) {
        addr = (uint64_t)i * 0x200000;    /* 2MB per entry */
        g_pd[i] = addr |
                  PTE_PRESENT | PTE_WRITABLE | PTE_PS;
    }
}

/*
 * _check_cpuid
 * ------------
 * Check apakah CPUID tersedia.
 */
static int _check_cpuid(void)
{
    uint64_t eflags;
    uint64_t eflags_after;

    /* Get EFLAGS - use pushfq/popq for x86_64 */
    __asm__ __volatile__(
        "pushfq\n\t"
        "popq %0"
        : "=r"(eflags)
    );

    /* Flip ID bit (bit 21) */
    eflags_after = eflags ^ 0x00200000ULL;

    /* Set EFLAGS */
    __asm__ __volatile__(
        "pushq %0\n\t"
        "popfq"
        :
        : "r"(eflags_after)
    );

    /* Get EFLAGS again */
    __asm__ __volatile__(
        "pushfq\n\t"
        "popq %0"
        : "=r"(eflags_after)
    );

    /* Restore EFLAGS */
    __asm__ __volatile__(
        "pushq %0\n\t"
        "popfq"
        :
        : "r"(eflags)
    );

    /* If ID bit changed, CPUID is supported */
    return (eflags ^ eflags_after) ? 1 : 0;
}

/*
 * _check_long_mode
 * ----------------
 * Check apakah CPU mendukung long mode.
 */
static int _check_long_mode(void)
{
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;

    /* Check if CPUID is available */
    if (!_check_cpuid()) {
        return 0;
    }

    /* Check max CPUID function */
    __asm__ __volatile__(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0)
    );

    if (eax < 0x80000001) {
        return 0;   /* Extended CPUID not supported */
    }

    /* Check for long mode using extended CPUID */
    __asm__ __volatile__(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0x80000001)
    );

    /* Check LM bit (bit 29 in EDX) */
    return (edx & (1 << 29)) ? 1 : 0;
}

/* =============================================================================
 * FUNGSI PUBLIC
 * =============================================================================
 */

/*
 * mode_long_check_support
 * -----------------------
 * Cek apakah CPU mendukung long mode.
 */
int mode_long_check_support(void)
{
    return _check_long_mode();
}

/*
 * mode_long_init
 * --------------
 * Inisialisasi untuk transisi ke long mode.
 */
int mode_long_init(void)
{
    /* Check CPU support */
    if (!_check_long_mode()) {
        return -1;
    }

    /* Setup GDT */
    _setup_gdt64();

    /* Setup page tables */
    _setup_page_tables();

    return 0;
}

/*
 * mode_long_enter
 * ---------------
 * Masuk ke long mode.
 *
 * Parameter:
 *   entry_point - Alamat entry point kernel 64-bit
 */
void mode_long_enter(uint64_t entry_point)
{
    uint64_t cr0;
    uint64_t cr4;
    uint64_t efer;

    /* Disable interrupts */
    _cli();

    /* Enable PAE in CR4 */
    cr4 = _read_cr4();
    cr4 |= CR4_PAE | CR4_PGE;
    _write_cr4(cr4);

    /* Load PML4 into CR3 */
    _write_cr3((uint64_t)(uintptr_t)g_pml4);

    /* Enable long mode in EFER */
    efer = _read_msr(MSR_EFER);
    efer |= EFER_LME | EFER_NXE | EFER_SCE;
    _write_msr(MSR_EFER, efer);

    /* Enable paging in CR0 */
    cr0 = _read_cr0();
    cr0 |= CR0_PG;
    _write_cr0(cr0);

    /* Load 64-bit GDT */
    _lgdt64(&g_gdt_ptr64);

    g_long_mode_enabled = 1;

    /* Jump to 64-bit code - use far return for x86_64 compatibility */
    __asm__ __volatile__(
        "pushq $0x08\n\t"           /* Kernel code segment selector */
        "pushq %0\n\t"              /* Entry point */
        "retfq\n\t"                 /* Far return to jump to 64-bit code */
        "1:\n\t"
        "movw $0x10, %%ax\n\t"
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"
        "movw %%ax, %%ss"
        :
        : "r"(entry_point)
        : "ax", "memory"
    );
}

/*
 * mode_long_is_enabled
 * --------------------
 * Cek apakah long mode aktif.
 */
int mode_long_is_enabled(void)
{
    return g_long_mode_enabled;
}

/*
 * mode_long_get_pml4
 * ------------------
 * Dapatkan alamat PML4.
 */
uint64_t mode_long_get_pml4(void)
{
    return (uint64_t)(uintptr_t)g_pml4;
}

/*
 * mode_long_get_gdt
 * -----------------
 * Dapatkan pointer ke GDT.
 */
gdt_ptr64_t *mode_long_get_gdt(void)
{
    return &g_gdt_ptr64;
}

/*
 * mode_long_invalidate_tlb
 * ------------------------
 * Invalidate TLB.
 */
void mode_long_invalidate_tlb(void)
{
    uint64_t cr3;

    cr3 = _read_cr3();
    _write_cr3(cr3);
}

/*
 * mode_long_invalidate_tlb_page
 * -----------------------------
 * Invalidate TLB untuk satu halaman.
 */
void mode_long_invalidate_tlb_page(uint64_t addr)
{
    __asm__ __volatile__(
        "invlpg (%0)"
        :
        : "r"(addr)
        : "memory"
    );
}

/*
 * mode_long_set_gs_base
 * ---------------------
 * Set GS base address (untuk per-CPU data).
 */
void mode_long_set_gs_base(uint64_t base)
{
    _write_msr(MSR_GS_BASE, base);
}

/*
 * mode_long_get_gs_base
 * ---------------------
 * Get GS base address.
 */
uint64_t mode_long_get_gs_base(void)
{
    return _read_msr(MSR_GS_BASE);
}

/*
 * mode_long_set_fs_base
 * ---------------------
 * Set FS base address.
 */
void mode_long_set_fs_base(uint64_t base)
{
    _write_msr(MSR_FS_BASE, base);
}

/*
 * mode_long_get_fs_base
 * ---------------------
 * Get FS base address.
 */
uint64_t mode_long_get_fs_base(void)
{
    return _read_msr(MSR_FS_BASE);
}

#else /* __i386__ - 32-bit stubs */

/* =============================================================================
 * STUB 32-BIT
 * =============================================================================
 * Long mode tidak tersedia pada x86 32-bit.
 * Fungsi-fungsi ini mengembalikan nilai default/error.
 * =============================================================================
 */

int mode_long_check_support(void)
{
    return 0;
}

int mode_long_init(void)
{
    return -1;
}

void mode_long_enter(uint64_t entry_point)
{
    (void)entry_point;
}

int mode_long_is_enabled(void)
{
    return g_long_mode_enabled;
}

uint64_t mode_long_get_pml4(void)
{
    return 0;
}

gdt_ptr64_t *mode_long_get_gdt(void)
{
    return &g_gdt_ptr64;
}

void mode_long_invalidate_tlb(void)
{
}

void mode_long_invalidate_tlb_page(uint64_t addr)
{
    (void)addr;
}

void mode_long_set_gs_base(uint64_t base)
{
    (void)base;
}

uint64_t mode_long_get_gs_base(void)
{
    return 0;
}

void mode_long_set_fs_base(uint64_t base)
{
    (void)base;
}

uint64_t mode_long_get_fs_base(void)
{
    return 0;
}

#endif /* __i386__ */
