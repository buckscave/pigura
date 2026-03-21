/*
 * PIGURA OS - MMU_ARM64.C
 * -----------------------
 * Implementasi MMU untuk ARM64 (AArch64).
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola Memory
 * Management Unit pada prosesor ARM64.
 *
 * Arsitektur: ARM64/AArch64 (64-bit)
 * Versi: 1.0
 */

#include "../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* Page table entry bits */
#define PTE_VALID               (1ULL << 0)
#define PTE_TABLE               (1ULL << 1)
#define PTE_PAGE                (1ULL << 1)
#define PTE_ATTR_IDX(x)         ((tak_bertanda64_t)(x) << 2)
#define PTE_NS                  (1ULL << 5)
#define PTE_AP_RW_USER          (0ULL << 6)
#define PTE_AP_RO_USER          (1ULL << 6)
#define PTE_AP_RW_KERNEL        (1ULL << 7)
#define PTE_AP_RO_KERNEL        (3ULL << 6)
#define PTE_SH_NS               (0ULL << 8)
#define PTE_SH_OS               (2ULL << 8)
#define PTE_SH_IS               (3ULL << 8)
#define PTE_AF                  (1ULL << 10)
#define PTE_NG                  (1ULL << 11)
#define PTE_DBM                 (1ULL << 51)
#define PTE_CONTIGUOUS          (1ULL << 52)
#define PTE_PXN                 (1ULL << 53)
#define PTE_UXN                 (1ULL << 54)

/* MAIR attribute indices */
#define MAIR_DEVICE_nGnRnE      0
#define MAIR_DEVICE_nGnRE       1
#define MAIR_DEVICE_GRE         2
#define MAIR_NORMAL_NC          3
#define MAIR_NORMAL             4
#define MAIR_NORMAL_WT          5

/* TCR bits */
#define TCR_T0SZ(x)             ((tak_bertanda64_t)(x) << 0)
#define TCR_T1SZ(x)             ((tak_bertanda64_t)(x) << 16)
#define TCR_IRGN0_NC            (0ULL << 8)
#define TCR_IRGN0_WBWA          (1ULL << 8)
#define TCR_IRGN0_WT            (2ULL << 8)
#define TCR_IRGN0_WBnWA         (3ULL << 8)
#define TCR_IRGN1(x)            ((x) << 24)
#define TCR_ORGN0_NC            (0ULL << 10)
#define TCR_ORGN0_WBWA          (1ULL << 10)
#define TCR_ORGN0_WT            (2ULL << 10)
#define TCR_ORGN0_WBnWA         (3ULL << 10)
#define TCR_ORGN1(x)            ((x) << 26)
#define TCR_SH0_NS              (0ULL << 12)
#define TCR_SH0_OS              (2ULL << 12)
#define TCR_SH0_IS              (3ULL << 12)
#define TCR_SH1(x)              ((x) << 28)
#define TCR_TG0_4KB             (0ULL << 14)
#define TCR_TG0_16KB            (2ULL << 14)
#define TCR_TG0_64KB            (1ULL << 14)
#define TCR_TG1_4KB             (2ULL << 30)
#define TCR_TG1_16KB            (1ULL << 30)
#define TCR_TG1_64KB            (0ULL << 30)
#define TCR_IPS_32BIT           (0ULL << 32)
#define TCR_IPS_36BIT           (1ULL << 32)
#define TCR_IPS_40BIT           (2ULL << 32)
#define TCR_IPS_42BIT           (3ULL << 32)
#define TCR_IPS_44BIT           (4ULL << 32)
#define TCR_IPS_48BIT           (5ULL << 32)
#define TCR_IPS_52BIT           (6ULL << 32)
#define TCR_ASID_8BIT           (0ULL << 36)
#define TCR_ASID_16BIT          (1ULL << 36)
#define TCR_TBI0                (1ULL << 37)
#define TCR_TBI1                (1ULL << 38)

/* Page sizes */
#define PAGE_SIZE_4K            4096ULL
#define PAGE_SIZE_16K           16384ULL
#define PAGE_SIZE_64K           65536ULL

/* Block sizes */
#define BLOCK_SIZE_4K_L1        (512ULL * 1024 * 1024 * 1024)  /* 512GB */
#define BLOCK_SIZE_4K_L2        (2ULL * 1024 * 1024)           /* 2MB */
#define PAGE_SIZE_4K_L3         PAGE_SIZE_4K                    /* 4KB */

/*
 * ============================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ============================================================================
 */

/* Current page table */
static tak_bertanda64_t *g_current_pgd = NULL;

/* Kernel page table */
static tak_bertanda64_t *g_kernel_pgd = NULL;

/* MAIR value */
static tak_bertanda64_t g_mair_value = 0;

/* TCR value */
static tak_bertanda64_t g_tcr_value = 0;

/* Feature flags */
static bool_t g_mmu_enabled = SALAH;

/*
 * ============================================================================
 * FUNGSI BANTUAN (HELPER FUNCTIONS)
 * ============================================================================
 */

/*
 * _read_sctlr
 * -----------
 * Read SCTLR_EL1.
 */
static inline tak_bertanda64_t _read_sctlr(void)
{
    tak_bertanda64_t sctlr;

    __asm__ __volatile__(
        "mrs %0, sctlr_el1"
        : "=r"(sctlr)
    );

    return sctlr;
}

/*
 * _write_sctlr
 * ------------
 * Write SCTLR_EL1.
 */
static inline void _write_sctlr(tak_bertanda64_t sctlr)
{
    __asm__ __volatile__(
        "msr sctlr_el1, %0\n\t"
        "isb"
        :
        : "r"(sctlr)
        : "memory"
    );
}

/*
 * _read_tcr
 * ---------
 * Read TCR_EL1.
 */
static inline tak_bertanda64_t _read_tcr(void)
{
    tak_bertanda64_t tcr;

    __asm__ __volatile__(
        "mrs %0, tcr_el1"
        : "=r"(tcr)
    );

    return tcr;
}

/*
 * _write_tcr
 * ----------
 * Write TCR_EL1.
 */
static inline void _write_tcr(tak_bertanda64_t tcr)
{
    __asm__ __volatile__(
        "msr tcr_el1, %0\n\t"
        "isb"
        :
        : "r"(tcr)
        : "memory"
    );
}

/*
 * _read_ttbr0
 * -----------
 * Read TTBR0_EL1.
 */
static inline tak_bertanda64_t _read_ttbr0(void)
{
    tak_bertanda64_t ttbr0;

    __asm__ __volatile__(
        "mrs %0, ttbr0_el1"
        : "=r"(ttbr0)
    );

    return ttbr0;
}

/*
 * _write_ttbr0
 * ------------
 * Write TTBR0_EL1.
 */
static inline void _write_ttbr0(tak_bertanda64_t ttbr0)
{
    __asm__ __volatile__(
        "msr ttbr0_el1, %0\n\t"
        "isb"
        :
        : "r"(ttbr0)
        : "memory"
    );
}

/*
 * _read_ttbr1
 * -----------
 * Read TTBR1_EL1.
 */
static inline tak_bertanda64_t _read_ttbr1(void)
{
    tak_bertanda64_t ttbr1;

    __asm__ __volatile__(
        "mrs %0, ttbr1_el1"
        : "=r"(ttbr1)
    );

    return ttbr1;
}

/*
 * _write_ttbr1
 * ------------
 * Write TTBR1_EL1.
 */
static inline void _write_ttbr1(tak_bertanda64_t ttbr1)
{
    __asm__ __volatile__(
        "msr ttbr1_el1, %0\n\t"
        "isb"
        :
        : "r"(ttbr1)
        : "memory"
    );
}

/*
 * _read_mair
 * ----------
 * Read MAIR_EL1.
 */
static inline tak_bertanda64_t _read_mair(void)
{
    tak_bertanda64_t mair;

    __asm__ __volatile__(
        "mrs %0, mair_el1"
        : "=r"(mair)
    );

    return mair;
}

/*
 * _write_mair
 * -----------
 * Write MAIR_EL1.
 */
static inline void _write_mair(tak_bertanda64_t mair)
{
    __asm__ __volatile__(
        "msr mair_el1, %0\n\t"
        "isb"
        :
        : "r"(mair)
        : "memory"
    );
}

/*
 * ============================================================================
 * PUBLIC FUNCTIONS
 * ============================================================================
 */

/*
 * mmu_init
 * --------
 * Initialize MMU.
 */
status_t mmu_init(void)
{
    tak_bertanda64_t mair;
    tak_bertanda64_t tcr;

    /* Setup MAIR_EL1 */
    mair = 0;
    /* Device memory attributes */
    mair |= (0x00ULL << (MAIR_DEVICE_nGnRnE * 8));  /* Device nGnRnE */
    mair |= (0x04ULL << (MAIR_DEVICE_nGnRE * 8));   /* Device nGnRE */
    mair |= (0x0CULL << (MAIR_DEVICE_GRE * 8));     /* Device GRE */
    /* Normal memory attributes */
    mair |= (0x44ULL << (MAIR_NORMAL_NC * 8));      /* Normal NC */
    mair |= (0xFFULL << (MAIR_NORMAL * 8));         /* Normal Cacheable */
    mair |= (0xBBULL << (MAIR_NORMAL_WT * 8));      /* Normal WT */

    g_mair_value = mair;
    _write_mair(mair);

    /* Setup TCR_EL1 */
    /* 48-bit VA, 4KB pages, inner shareable, write-back cacheable */
    tcr = TCR_T0SZ(16) | TCR_T1SZ(16);
    tcr |= TCR_TG0_4KB | TCR_TG1_4KB;
    tcr |= TCR_IRGN0_WBWA | TCR_IRGN1(1);
    tcr |= TCR_ORGN0_WBWA | TCR_ORGN1(1);
    tcr |= TCR_SH0_IS | TCR_SH1(3);
    tcr |= TCR_IPS_48BIT;
    tcr |= TCR_ASID_16BIT;

    g_tcr_value = tcr;
    _write_tcr(tcr);

    kernel_printf("[MMU-ARM64] MAIR=0x%lX TCR=0x%lX\n", mair, tcr);

    return STATUS_BERHASIL;
}

/*
 * mmu_enable
 * ----------
 * Enable MMU.
 */
void mmu_enable(void)
{
    tak_bertanda64_t sctlr;

    sctlr = _read_sctlr();

    /* Enable MMU (M bit) */
    sctlr |= (1 << 0);

    /* Enable caches (C and I bits) */
    sctlr |= (1 << 2);   /* C: Data cache */
    sctlr |= (1 << 12);  /* I: Instruction cache */

    _write_sctlr(sctlr);

    g_mmu_enabled = BENAR;

    kernel_printf("[MMU-ARM64] MMU enabled\n");
}

/*
 * mmu_disable
 * -----------
 * Disable MMU.
 */
void mmu_disable(void)
{
    tak_bertanda64_t sctlr;

    sctlr = _read_sctlr();

    /* Disable MMU */
    sctlr &= ~(1 << 0);

    /* Disable caches */
    sctlr &= ~(1 << 2);
    sctlr &= ~(1 << 12);

    _write_sctlr(sctlr);

    g_mmu_enabled = SALAH;

    kernel_printf("[MMU-ARM64] MMU disabled\n");
}

/*
 * mmu_is_enabled
 * --------------
 * Check if MMU is enabled.
 */
bool_t mmu_is_enabled(void)
{
    return g_mmu_enabled;
}

/*
 * mmu_set_ttbr0
 * -------------
 * Set TTBR0.
 */
void mmu_set_ttbr0(tak_bertanda64_t ttbr0, tak_bertanda16_t asid)
{
    /* Combine TTBR0 with ASID */
    ttbr0 |= ((tak_bertanda64_t)asid << 48);
    _write_ttbr0(ttbr0);
    g_current_pgd = (tak_bertanda64_t *)(ttbr0 & 0xFFFFFFFFF000ULL);
}

/*
 * mmu_set_ttbr1
 * -------------
 * Set TTBR1.
 */
void mmu_set_ttbr1(tak_bertanda64_t ttbr1)
{
    _write_ttbr1(ttbr1);
    g_kernel_pgd = (tak_bertanda64_t *)(ttbr1 & 0xFFFFFFFFF000ULL);
}

/*
 * mmu_get_ttbr0
 * -------------
 * Get TTBR0.
 */
tak_bertanda64_t mmu_get_ttbr0(void)
{
    return _read_ttbr0();
}

/*
 * mmu_get_ttbr1
 * -------------
 * Get TTBR1.
 */
tak_bertanda64_t mmu_get_ttbr1(void)
{
    return _read_ttbr1();
}

/*
 * mmu_get_tcr
 * -----------
 * Get TCR.
 */
tak_bertanda64_t mmu_get_tcr(void)
{
    return _read_tcr();
}

/*
 * mmu_get_mair
 * -----------
 * Get MAIR.
 */
tak_bertanda64_t mmu_get_mair(void)
{
    return _read_mair();
}

/*
 * mmu_invalidate_tlb_all
 * ----------------------
 * Invalidate all TLB entries.
 */
void mmu_invalidate_tlb_all(void)
{
    __asm__ __volatile__(
        "tlbi VMALLE1IS\n\t"
        "dsb ish\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * mmu_invalidate_tlb_va
 * ---------------------
 * Invalidate TLB for virtual address.
 */
void mmu_invalidate_tlb_va(tak_bertanda64_t va)
{
    __asm__ __volatile__(
        "tlbi VAAE1, %0\n\t"
        "dsb sy\n\t"
        "isb"
        :
        : "r"(va >> 12)
        : "memory"
    );
}

/*
 * mmu_invalidate_tlb_asid
 * -----------------------
 * Invalidate TLB for ASID.
 */
void mmu_invalidate_tlb_asid(tak_bertanda16_t asid)
{
    __asm__ __volatile__(
        "tlbi ASIDE1, %0\n\t"
        "dsb sy\n\t"
        "isb"
        :
        : "r"((tak_bertanda64_t)asid)
        : "memory"
    );
}

/*
 * mmu_enable_cache
 * ----------------
 * Enable instruction and data cache.
 */
void mmu_enable_cache(void)
{
    tak_bertanda64_t sctlr;

    sctlr = _read_sctlr();
    sctlr |= (1 << 2);   /* C: Data cache */
    sctlr |= (1 << 12);  /* I: Instruction cache */
    _write_sctlr(sctlr);
}

/*
 * mmu_disable_cache
 * -----------------
 * Disable instruction and data cache.
 */
void mmu_disable_cache(void)
{
    tak_bertanda64_t sctlr;

    sctlr = _read_sctlr();
    sctlr &= ~(1 << 2);   /* Clear C bit */
    sctlr &= ~(1 << 12);  /* Clear I bit */
    _write_sctlr(sctlr);

    /* Clean and invalidate caches */
    __asm__ __volatile__(
        "ic IALLU\n\t"
        "dsb sy\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * mmu_flush_cache
 * ---------------
 * Flush data cache.
 */
void mmu_flush_cache(void *addr, ukuran_t size)
{
    tak_bertanda64_t start;
    tak_bertanda64_t end;
    tak_bertanda64_t line_size;
    tak_bertanda64_t ctr;

    /* Get cache line size */
    __asm__ __volatile__(
        "mrs %0, ctr_el0"
        : "=r"(ctr)
    );

    line_size = 4ULL << (ctr & 0xF);

    start = (tak_bertanda64_t)addr & ~(line_size - 1);
    end = (tak_bertanda64_t)addr + size;

    while (start < end) {
        __asm__ __volatile__(
            "dc civac, %0"
            :
            : "r"(start)
            : "memory"
        );
        start += line_size;
    }

    __asm__ __volatile__("dsb sy" ::: "memory");
}

/*
 * mmu_set_kernel_pgd
 * ------------------
 * Set kernel page table.
 */
void mmu_set_kernel_pgd(tak_bertanda64_t *pgd)
{
    g_kernel_pgd = pgd;
}

/*
 * mmu_get_kernel_pgd
 * ------------------
 * Get kernel page table.
 */
tak_bertanda64_t *mmu_get_kernel_pgd(void)
{
    return g_kernel_pgd;
}

/*
 * mmu_set_current_pgd
 * -------------------
 * Set current page table.
 */
void mmu_set_current_pgd(tak_bertanda64_t *pgd)
{
    g_current_pgd = pgd;
}

/*
 * mmu_get_current_pgd
 * -------------------
 * Get current page table.
 */
tak_bertanda64_t *mmu_get_current_pgd(void)
{
    return g_current_pgd;
}
