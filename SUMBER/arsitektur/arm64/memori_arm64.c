/*
 * PIGURA OS - MEMORI_ARM64.C
 * --------------------------
 * Implementasi manajemen memori untuk arsitektur ARM64 (AArch64).
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola memori fisik
 * dan virtual pada prosesor ARM64 dengan MMU.
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

/* Page sizes */
#define UKURAN_HALAMAN_4K        4096UL
#define UKURAN_HALAMAN_16K       16384UL
#define UKURAN_HALAMAN_64K       65536UL

/* Page table levels */
#define PT_LEVEL_0               0
#define PT_LEVEL_1               1
#define PT_LEVEL_2               2
#define PT_LEVEL_3               3

/* Page table entry counts */
#define PT_ENTRIES_4K            512
#define PT_ENTRIES_16K           2048
#define PT_ENTRIES_64K           8192

/* Page table entry masks and shifts */
#define PTE_VALID                (1ULL << 0)
#define PTE_TABLE                (1ULL << 1)
#define PTE_PAGE                 (1ULL << 1)
#define PTE_BLOCK                0

/* Page table attribute bits */
#define PTE_ATTR_IDX(x)          ((tak_bertanda64_t)(x) << 2)
#define PTE_ATTR_IDX_MASK        (7ULL << 2)
#define PTE_NS                   (1ULL << 5)
#define PTE_AP_USER_RO           (1ULL << 6)
#define PTE_AP_USER_RW           (0ULL << 6)
#define PTE_AP_KERNEL_RO         (3ULL << 6)
#define PTE_AP_KERNEL_RW         (1ULL << 6)
#define PTE_SH_NON_SHAREABLE     (0ULL << 8)
#define PTE_SH_OUTER_SHAREABLE   (2ULL << 8)
#define PTE_SH_INNER_SHAREABLE   (3ULL << 8)
#define PTE_AF                   (1ULL << 10)
#define PTE_NG                   (1ULL << 11)
#define PTE_DBM                  (1ULL << 51)
#define PTE_PXN                  (1ULL << 53)
#define PTE_UXN                  (1ULL << 54)
#define PTE_XN                   (1ULL << 54)

/* Memory attribute indices (MAIR) */
#define MAIR_IDX_NORMAL_NC       0
#define MAIR_IDX_NORMAL         1
#define MAIR_IDX_DEVICE_nGnRnE  2
#define MAIR_IDX_DEVICE_nGnRE   3
#define MAIR_IDX_DEVICE_GRE     4
#define MAIR_IDX_NORMAL_IWT     5

/* TCR_EL1 bits */
#define TCR_T0SZ(x)             ((tak_bertanda64_t)(x) << 0)
#define TCR_T1SZ(x)             ((tak_bertanda64_t)(x) << 16)
#define TCR_IRGN0(x)            ((tak_bertanda64_t)(x) << 8)
#define TCR_IRGN1(x)            ((tak_bertanda64_t)(x) << 24)
#define TCR_ORGN0(x)            ((tak_bertanda64_t)(x) << 10)
#define TCR_ORGN1(x)            ((tak_bertanda64_t)(x) << 26)
#define TCR_SH0(x)              ((tak_bertanda64_t)(x) << 12)
#define TCR_SH1(x)              ((tak_bertanda64_t)(x) << 28)
#define TCR_TG0_4K              (0ULL << 14)
#define TCR_TG0_16K             (2ULL << 14)
#define TCR_TG0_64K             (1ULL << 14)
#define TCR_TG1_4K              (2ULL << 30)
#define TCR_TG1_16K             (1ULL << 30)
#define TCR_TG1_64K             (0ULL << 30)
#define TCR_IPS(x)              ((tak_bertanda64_t)(x) << 32)
#define TCR_ASID16              (1ULL << 36)
#define TCR_TBI0                (1ULL << 37)
#define TCR_TBI1                (1ULL << 38)
#define TCR_HA                  (1ULL << 39)
#define TCR_HD                  (1ULL << 40)

/* Virtual address space layout */
#define VA_USER_BASE            0x0000000000000000ULL
#define VA_USER_END             0x0000FFFFFFFFFFFFULL
#define VA_KERNEL_BASE          0xFFFF000000000000ULL
#define VA_KERNEL_END           0xFFFFFFFFFFFFFFFFULL

/* Physical address mask */
#define PA_MASK_48              0x0000FFFFFFFFF000ULL
#define PA_MASK_52              0x000FFFFFFFFFFFFFULL

/*
 * ============================================================================
 * TIPE DATA LOKAL (LOCAL DATA TYPES)
 * ============================================================================
 */

/* Page table entry */
typedef tak_bertanda64_t pte_t;

/* Memory region descriptor */
typedef struct {
    alamat_fisik_t alamat_fisik;
    alamat_virtual_t alamat_virtual;
    ukuran_t ukuran;
    tak_bertanda32_t attribut;
} region_memori_t;

/* Page table info */
typedef struct {
    pte_t *tabel;
    tak_bertanda32_t level;
    tak_bertanda32_t entries;
} info_pt_t;

/*
 * ============================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ============================================================================
 */

/* Page table directory kernel */
static pte_t *g_kernel_pgd = NULL;

/* Page table directory current */
static pte_t *g_current_pgd = NULL;

/* ASID counter */
static tak_bertanda32_t g_asid_counter = 1;

/* Memory info */
static hal_memory_info_t g_mem_info;

/* Flag inisialisasi */
static bool_t g_memori_diinisalisasi = SALAH;

/*
 * ============================================================================
 * FUNGSI BANTUAN (HELPER FUNCTIONS)
 * ============================================================================
 */

/*
 * _get_mair_value
 * ---------------
 * Dapatkan nilai MAIR_EL1.
 */
static inline tak_bertanda64_t _get_mair_value(void)
{
    /* Setup MAIR for various memory types */
    return ((0x00ULL << (MAIR_IDX_NORMAL_NC * 8))      /* Normal non-cacheable */
          | (0xFFULL << (MAIR_IDX_NORMAL * 8))         /* Normal cacheable */
          | (0x00ULL << (MAIR_IDX_DEVICE_nGnRnE * 8))  /* Device nGnRnE */
          | (0x04ULL << (MAIR_IDX_DEVICE_nGnRE * 8))   /* Device nGnRE */
          | (0x0CULL << (MAIR_IDX_DEVICE_GRE * 8))     /* Device GRE */
          | (0xBBULL << (MAIR_IDX_NORMAL_IWT * 8)));   /* Normal I-WT */
}

/*
 * _get_tcr_value
 * --------------
 * Dapatkan nilai TCR_EL1.
 */
static inline tak_bertanda64_t _get_tcr_value(void)
{
    tak_bertanda64_t tcr;
    tak_bertanda64_t pa_bits;

    /* Default: 48-bit VA, 4KB pages */
    tcr = TCR_T0SZ(16) | TCR_T1SZ(16);    /* 48-bit VA split */
    tcr |= TCR_TG0_4K | TCR_TG1_4K;        /* 4KB granule */
    tcr |= TCR_IRGN0(1) | TCR_IRGN1(1);    /* Inner write-back cacheable */
    tcr |= TCR_ORGN0(1) | TCR_ORGN1(1);    /* Outer write-back cacheable */
    tcr |= TCR_SH0(3) | TCR_SH1(3);        /* Inner shareable */
    tcr |= TCR_ASID16;                      /* 16-bit ASID */

    /* Set IPS based on PA size */
    pa_bits = g_mem_info.physical_address_bits;
    if (pa_bits <= 32) {
        tcr |= TCR_IPS(0);
    } else if (pa_bits <= 36) {
        tcr |= TCR_IPS(1);
    } else if (pa_bits <= 40) {
        tcr |= TCR_IPS(2);
    } else if (pa_bits <= 42) {
        tcr |= TCR_IPS(3);
    } else if (pa_bits <= 44) {
        tcr |= TCR_IPS(4);
    } else if (pa_bits <= 48) {
        tcr |= TCR_IPS(5);
    } else {
        tcr |= TCR_IPS(6);
    }

    return tcr;
}

/*
 * _pte_to_pa
 * ----------
 * Konversi PTE ke alamat fisik.
 */
static inline alamat_fisik_t _pte_to_pa(pte_t pte)
{
    return (alamat_fisik_t)(pte & PA_MASK_48);
}

/*
 * _pa_to_pte
 * ----------
 * Konversi alamat fisik ke PTE.
 */
static inline pte_t _pa_to_pte(alamat_fisik_t pa)
{
    return (pte_t)(pa & PA_MASK_48);
}

/*
 * _get_pt_index
 * -------------
 * Dapatkan index page table untuk level tertentu.
 */
static inline tak_bertanda32_t _get_pt_index(alamat_virtual_t va,
                                              tak_bertanda32_t level)
{
    tak_bertanda32_t shift;
    tak_bertanda32_t index;

    /* Each level shifts by 9 bits (512 entries) for 4KB pages */
    shift = 12 + (3 - level) * 9;
    index = (tak_bertanda32_t)((va >> shift) & 0x1FF);

    return index;
}

/*
 * _is_pte_valid
 * -------------
 * Cek apakah PTE valid.
 */
static inline bool_t _is_pte_valid(pte_t pte)
{
    return (pte & PTE_VALID) ? BENAR : SALAH;
}

/*
 * _is_pte_table
 * -------------
 * Cek apakah PTE adalah table descriptor.
 */
static inline bool_t _is_pte_table(pte_t pte)
{
    return (pte & PTE_TABLE) ? BENAR : SALAH;
}

/*
 * _is_pte_block
 * -------------
 * Cek apakah PTE adalah block descriptor.
 */
static inline bool_t _is_pte_block(pte_t pte, tak_bertanda32_t level)
{
    return _is_pte_valid(pte) && !_is_pte_table(pte) && (level < 3);
}

/*
 * _is_pte_page
 * ------------
 * Cek apakah PTE adalah page descriptor.
 */
static inline bool_t _is_pte_page(pte_t pte, tak_bertanda32_t level)
{
    return _is_pte_valid(pte) && _is_pte_table(pte) && (level == 3);
}

/*
 * _alloc_page_table
 * -----------------
 * Alokasikan page table baru.
 */
static pte_t *_alloc_page_table(void)
{
    pte_t *pt;
    tak_bertanda32_t i;

    /* Alokasi dari memory pool */
    pt = (pte_t *)hal_memory_alloc_page();
    if (pt == NULL) {
        return NULL;
    }

    /* Clear page table */
    for (i = 0; i < PT_ENTRIES_4K; i++) {
        pt[i] = 0;
    }

    return pt;
}

/*
 * _free_page_table
 * ----------------
 * Bebaskan page table.
 */
static void _free_page_table(pte_t *pt)
{
    if (pt != NULL) {
        hal_memory_free_page((alamat_fisik_t)pt);
    }
}

/*
 * _set_pte
 * --------
 * Set page table entry.
 */
static inline void _set_pte(pte_t *ptep, pte_t pte)
{
    *ptep = pte;
    /* Data synchronization barrier */
    __asm__ __volatile__("dsb sy" ::: "memory");
}

/*
 * _get_pte
 * --------
 * Get page table entry.
 */
static inline pte_t _get_pte(pte_t *ptep)
{
    return *ptep;
}

/*
 * ============================================================================
 * FUNGSI WALK PAGE TABLE (PAGE TABLE WALK FUNCTIONS)
 * ============================================================================
 */

/*
 * _walk_pt
 * --------
 * Walk page table untuk mendapatkan PTE.
 */
static pte_t *_walk_pt(pte_t *pgd, alamat_virtual_t va,
                       bool_t alloc, tak_bertanda32_t *level_out)
{
    pte_t *pt;
    pte_t pte;
    tak_bertanda32_t level;
    tak_bertanda32_t index;

    if (pgd == NULL) {
        return NULL;
    }

    pt = pgd;

    for (level = 0; level < 4; level++) {
        index = _get_pt_index(va, level);
        pte = _get_pte(&pt[index]);

        if (level == 3) {
            /* Level 3: Page descriptor */
            if (level_out != NULL) {
                *level_out = level;
            }
            return &pt[index];
        }

        if (!_is_pte_valid(pte)) {
            /* Entry tidak valid */
            if (alloc) {
                /* Alokasi table baru */
                pte_t *new_pt = _alloc_page_table();
                if (new_pt == NULL) {
                    return NULL;
                }

                /* Set table descriptor */
                pte = _pa_to_pte((alamat_fisik_t)new_pt) |
                      PTE_VALID | PTE_TABLE;
                _set_pte(&pt[index], pte);
                pt = new_pt;
            } else {
                if (level_out != NULL) {
                    *level_out = level;
                }
                return &pt[index];
            }
        } else if (_is_pte_table(pte)) {
            /* Table descriptor: lanjut ke level berikutnya */
            pt = (pte_t *)_pte_to_pa(pte);
        } else {
            /* Block descriptor: selesai */
            if (level_out != NULL) {
                *level_out = level;
            }
            return &pt[index];
        }
    }

    return NULL;
}

/*
 * ============================================================================
 * FUNGSI MAPPING (MAPPING FUNCTIONS)
 * ============================================================================
 */

/*
 * _map_page
 * ---------
 * Map satu halaman.
 */
static status_t _map_page(pte_t *pgd, alamat_virtual_t va,
                           alamat_fisik_t pa, tak_bertanda64_t attribut)
{
    pte_t *ptep;
    pte_t pte;
    tak_bertanda32_t level;

    ptep = _walk_pt(pgd, va, BENAR, &level);
    if (ptep == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    /* Buat PTE */
    pte = _pa_to_pte(pa) | PTE_VALID | PTE_PAGE | attribut | PTE_AF;

    /* Set access permissions */
    pte |= PTE_SH_INNER_SHAREABLE;
    pte |= PTE_ATTR_IDX(MAIR_IDX_NORMAL);

    /* Kernel pages: not accessible from EL0 */
    if (va >= VA_KERNEL_BASE) {
        pte |= PTE_UXN | PTE_PXN;
    }

    _set_pte(ptep, pte);

    return STATUS_BERHASIL;
}

/*
 * _map_block
 * ----------
 * Map block (2MB atau 1GB).
 */
static status_t _map_block(pte_t *pgd, alamat_virtual_t va,
                            alamat_fisik_t pa, ukuran_t ukuran,
                            tak_bertanda64_t attribut)
{
    pte_t *ptep;
    pte_t pte;
    tak_bertanda32_t level;

    /* Tentukan level berdasarkan ukuran */
    if (ukuran >= 1024 * 1024 * 1024) {
        level = 1;  /* 1GB block */
    } else if (ukuran >= 2 * 1024 * 1024) {
        level = 2;  /* 2MB block */
    } else {
        /* Gunakan page mapping */
        return _map_page(pgd, va, pa, attribut);
    }

    ptep = _walk_pt(pgd, va, BENAR, &level);
    if (ptep == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    /* Buat block PTE */
    pte = _pa_to_pte(pa) | PTE_VALID | attribut | PTE_AF;
    pte |= PTE_SH_INNER_SHAREABLE;
    pte |= PTE_ATTR_IDX(MAIR_IDX_NORMAL);

    if (va >= VA_KERNEL_BASE) {
        pte |= PTE_UXN | PTE_PXN;
    }

    _set_pte(ptep, pte);

    return STATUS_BERHASIL;
}

/*
 * _unmap_page
 * -----------
 * Unmap satu halaman.
 */
static status_t _unmap_page(pte_t *pgd, alamat_virtual_t va)
{
    pte_t *ptep;
    pte_t pte;
    tak_bertanda32_t level;

    ptep = _walk_pt(pgd, va, SALAH, &level);
    if (ptep == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    pte = _get_pte(ptep);
    if (!_is_pte_valid(pte)) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Invalidate PTE */
    _set_pte(ptep, 0);

    /* Invalidate TLB */
    __asm__ __volatile__(
        "tlbi VAAE1, %0\n\t"
        "dsb sy\n\t"
        "isb"
        :
        : "r"(va >> 12)
        : "memory"
    );

    return STATUS_BERHASIL;
}

/*
 * _get_mapping
 * ------------
 * Dapatkan mapping untuk virtual address.
 */
static alamat_fisik_t _get_mapping(pte_t *pgd, alamat_virtual_t va,
                                    tak_bertanda64_t *attribut)
{
    pte_t *ptep;
    pte_t pte;
    tak_bertanda32_t level;
    alamat_fisik_t pa;

    ptep = _walk_pt(pgd, va, SALAH, &level);
    if (ptep == NULL) {
        return ALAMAT_FISIK_INVALID;
    }

    pte = _get_pte(ptep);
    if (!_is_pte_valid(pte)) {
        return ALAMAT_FISIK_INVALID;
    }

    /* Get physical address and mask out lower bits */
    pa = _pte_to_pa(pte);

    /* Add offset within page/block */
    if (level == 1) {
        pa |= (va & 0x3FFFFFFF);      /* 1GB block offset */
    } else if (level == 2) {
        pa |= (va & 0x1FFFFF);        /* 2MB block offset */
    } else {
        pa |= (va & 0xFFF);           /* 4KB page offset */
    }

    if (attribut != NULL) {
        *attribut = pte & ~PA_MASK_48;
    }

    return pa;
}

/*
 * ============================================================================
 * FUNGSI PUBLIC (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * memori_init
 * -----------
 * Inisialisasi subsistem memori.
 */
status_t memori_init(void)
{
    tak_bertanda64_t mair;
    tak_bertanda64_t tcr;

    if (g_memori_diinisalisasi) {
        return STATUS_BERHASIL;
    }

    /* Get memory info from HAL */
    hal_memory_get_info(&g_mem_info);

    /* Default physical address bits */
    if (g_mem_info.physical_address_bits == 0) {
        g_mem_info.physical_address_bits = 48;
    }

    kernel_printf("[MEM-ARM64] Inisialisasi memori...\n");
    kernel_printf("[MEM-ARM64] Total: %lu MB\n",
        g_mem_info.total_bytes / (1024 * 1024));

    /* Alokasi PGD kernel */
    g_kernel_pgd = _alloc_page_table();
    if (g_kernel_pgd == NULL) {
        kernel_printf("[MEM-ARM64] Gagal alokasi PGD!\n");
        return STATUS_MEMORI_HABIS;
    }

    kernel_printf("[MEM-ARM64] PGD kernel di 0x%lX\n",
        (tak_bertanda64_t)g_kernel_pgd);

    /* Setup MAIR_EL1 */
    mair = _get_mair_value();
    __asm__ __volatile__(
        "msr mair_el1, %0\n\t"
        "isb"
        :
        : "r"(mair)
        : "memory"
    );

    /* Setup TCR_EL1 */
    tcr = _get_tcr_value();
    __asm__ __volatile__(
        "msr tcr_el1, %0\n\t"
        "isb"
        :
        : "r"(tcr)
        : "memory"
    );

    /* Identity mapping untuk kernel */
    /* Map 0-2MB sebagai 2MB block */
    _map_block(g_kernel_pgd, 0x0, 0x0, 2 * 1024 * 1024,
               PTE_AP_KERNEL_RW);

    /* Map kernel di higher half */
    /* Map 0x80000 (512KB) ke 0xFFFF800000080000 */
    _map_block(g_kernel_pgd,
               0xFFFF800000000000ULL,
               0x0,
               2 * 1024 * 1024,
               PTE_AP_KERNEL_RW);

    /* Set TTBR0 */
    __asm__ __volatile__(
        "msr ttbr0_el1, %0\n\t"
        "isb"
        :
        : "r"((tak_bertanda64_t)g_kernel_pgd)
        : "memory"
    );

    /* Set TTBR1 (sama dengan TTBR0 untuk awal) */
    __asm__ __volatile__(
        "msr ttbr1_el1, %0\n\t"
        "isb"
        :
        : "r"((tak_bertanda64_t)g_kernel_pgd)
        : "memory"
    );

    /* Invalidate TLB */
    __asm__ __volatile__(
        "tlbi VMALLE1IS\n\t"
        "dsb ish\n\t"
        "isb"
        :
        :
        : "memory"
    );

    /* Enable MMU */
    {
        tak_bertanda64_t sctlr;
        __asm__ __volatile__("mrs %0, sctlr_el1" : "=r"(sctlr));
        sctlr |= (1 << 0);   /* M: MMU enable */
        sctlr |= (1 << 2);   /* C: Data cache */
        sctlr |= (1 << 12);  /* I: Instruction cache */
        __asm__ __volatile__(
            "msr sctlr_el1, %0\n\t"
            "isb"
            :
            : "r"(sctlr)
            : "memory"
        );
    }

    g_current_pgd = g_kernel_pgd;
    g_memori_diinisalisasi = BENAR;

    kernel_printf("[MEM-ARM64] MMU aktif\n");

    return STATUS_BERHASIL;
}

/*
 * memori_map
 * ----------
 * Map region memori.
 */
status_t memori_map(alamat_virtual_t va, alamat_fisik_t pa,
                     ukuran_t ukuran, tak_bertanda32_t attribut)
{
    pte_t *pgd;
    alamat_virtual_t va_curr;
    alamat_fisik_t pa_curr;
    ukuran_t mapped;
    status_t result;

    pgd = g_current_pgd;
    if (pgd == NULL) {
        return STATUS_GAGAL;
    }

    va_curr = va;
    pa_curr = pa;
    mapped = 0;

    while (mapped < ukuran) {
        /* Coba block mapping dulu untuk 2MB atau lebih besar */
        if ((ukuran - mapped) >= 2 * 1024 * 1024 &&
            (va_curr & 0x1FFFFF) == 0 &&
            (pa_curr & 0x1FFFFF) == 0) {
            /* 2MB block mapping */
            result = _map_block(pgd, va_curr, pa_curr,
                                2 * 1024 * 1024, attribut);
            if (result != STATUS_BERHASIL) {
                return result;
            }
            va_curr += 2 * 1024 * 1024;
            pa_curr += 2 * 1024 * 1024;
            mapped += 2 * 1024 * 1024;
        } else {
            /* 4KB page mapping */
            result = _map_page(pgd, va_curr, pa_curr, attribut);
            if (result != STATUS_BERHASIL) {
                return result;
            }
            va_curr += UKURAN_HALAMAN;
            pa_curr += UKURAN_HALAMAN;
            mapped += UKURAN_HALAMAN;
        }
    }

    return STATUS_BERHASIL;
}

/*
 * memori_unmap
 * ------------
 * Unmap region memori.
 */
status_t memori_unmap(alamat_virtual_t va, ukuran_t ukuran)
{
    pte_t *pgd;
    alamat_virtual_t va_curr;
    ukuran_t unmapped;
    status_t result;

    pgd = g_current_pgd;
    if (pgd == NULL) {
        return STATUS_GAGAL;
    }

    va_curr = va;
    unmapped = 0;

    while (unmapped < ukuran) {
        result = _unmap_page(pgd, va_curr);
        if (result != STATUS_BERHASIL) {
            /* Continue unmapping other pages */
        }
        va_curr += UKURAN_HALAMAN;
        unmapped += UKURAN_HALAMAN;
    }

    return STATUS_BERHASIL;
}

/*
 * memori_virtual_ke_fisik
 * -----------------------
 * Konversi alamat virtual ke fisik.
 */
alamat_fisik_t memori_virtual_ke_fisik(alamat_virtual_t va)
{
    return _get_mapping(g_current_pgd, va, NULL);
}

/*
 * memori_fisik_ke_virtual
 * -----------------------
 * Konversi alamat fisik ke virtual (identity mapped).
 */
alamat_virtual_t memori_fisik_ke_virtual(alamat_fisik_t pa)
{
    /* Identity mapping untuk kernel */
    return (alamat_virtual_t)pa;
}

/*
 * memori_alloc_halaman
 * --------------------
 * Alokasikan halaman.
 */
void *memori_alloc_halaman(void)
{
    alamat_fisik_t pa;
    alamat_virtual_t va;

    pa = hal_memory_alloc_page();
    if (pa == ALAMAT_FISIK_INVALID) {
        return NULL;
    }

    /* Identity map */
    va = (alamat_virtual_t)pa;

    return (void *)va;
}

/*
 * memori_free_halaman
 * -------------------
 * Bebaskan halaman.
 */
void memori_free_halaman(void *ptr)
{
    if (ptr == NULL) {
        return;
    }

    hal_memory_free_page((alamat_fisik_t)ptr);
}

/*
 * memori_alloc_pages
 * ------------------
 * Alokasikan multiple halaman.
 */
void *memori_alloc_pages(tak_bertanda32_t count)
{
    alamat_fisik_t pa;
    alamat_virtual_t va;

    if (count == 0) {
        return NULL;
    }

    pa = hal_memory_alloc_pages(count);
    if (pa == ALAMAT_FISIK_INVALID) {
        return NULL;
    }

    va = (alamat_virtual_t)pa;

    return (void *)va;
}

/*
 * memori_free_pages
 * -----------------
 * Bebaskan multiple halaman.
 */
void memori_free_pages(void *ptr, tak_bertanda32_t count)
{
    if (ptr == NULL || count == 0) {
        return;
    }

    hal_memory_free_pages((alamat_fisik_t)ptr, count);
}

/*
 * memori_get_info
 * ---------------
 * Dapatkan informasi memori.
 */
status_t memori_get_info(hal_memory_info_t *info)
{
    if (info == NULL) {
        return STATUS_PARAM_INVALID;
    }

    kernel_memcpy(info, &g_mem_info, sizeof(hal_memory_info_t));

    return STATUS_BERHASIL;
}

/*
 * memori_protect
 * --------------
 * Ubah proteksi memori.
 */
status_t memori_protect(alamat_virtual_t va, ukuran_t ukuran,
                         tak_bertanda32_t attribut)
{
    pte_t *pgd;
    pte_t *ptep;
    pte_t pte;
    alamat_virtual_t va_curr;
    ukuran_t updated;
    tak_bertanda32_t level;

    pgd = g_current_pgd;
    if (pgd == NULL) {
        return STATUS_GAGAL;
    }

    va_curr = va;
    updated = 0;

    while (updated < ukuran) {
        ptep = _walk_pt(pgd, va_curr, SALAH, &level);
        if (ptep == NULL) {
            return STATUS_TIDAK_DITEMUKAN;
        }

        pte = _get_pte(ptep);
        if (!_is_pte_valid(pte)) {
            return STATUS_TIDAK_DITEMUKAN;
        }

        /* Update attributes */
        pte &= ~(PTE_AP_KERNEL_RW | PTE_AP_KERNEL_RO |
                 PTE_AP_USER_RW | PTE_AP_USER_RO);

        if (attribut & 0x1) {
            pte |= PTE_AP_KERNEL_RW;
        } else {
            pte |= PTE_AP_KERNEL_RO;
        }

        if (!(attribut & 0x2)) {
            pte |= PTE_XN;
        }

        _set_pte(ptep, pte);

        /* Invalidate TLB */
        __asm__ __volatile__(
            "tlbi VAAE1, %0"
            :
            : "r"(va_curr >> 12)
            : "memory"
        );

        va_curr += UKURAN_HALAMAN;
        updated += UKURAN_HALAMAN;
    }

    return STATUS_BERHASIL;
}

/*
 * memori_flush_tlb
 * ----------------
 * Flush TLB.
 */
void memori_flush_tlb(void)
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
 * memori_flush_tlb_va
 * -------------------
 * Flush TLB untuk virtual address.
 */
void memori_flush_tlb_va(alamat_virtual_t va)
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
 * memori_flush_cache
 * ------------------
 * Flush data cache.
 */
void memori_flush_cache(void *addr, ukuran_t ukuran)
{
    tak_bertanda64_t line_size;
    tak_bertanda64_t start;
    tak_bertanda64_t end;
    tak_bertanda64_t ctr;

    /* Get cache line size */
    __asm__ __volatile__("mrs %0, ctr_el0" : "=r"(ctr));
    line_size = 4 << (ctr & 0xF);

    start = (tak_bertanda64_t)addr & ~(line_size - 1);
    end = (tak_bertanda64_t)addr + ukuran;

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
 * memori_invalidate_cache
 * -----------------------
 * Invalidate data cache.
 */
void memori_invalidate_cache(void *addr, ukuran_t ukuran)
{
    tak_bertanda64_t line_size;
    tak_bertanda64_t start;
    tak_bertanda64_t end;
    tak_bertanda64_t ctr;

    __asm__ __volatile__("mrs %0, ctr_el0" : "=r"(ctr));
    line_size = 4 << (ctr & 0xF);

    start = (tak_bertanda64_t)addr & ~(line_size - 1);
    end = (tak_bertanda64_t)addr + ukuran;

    while (start < end) {
        __asm__ __volatile__(
            "dc ivau, %0"
            :
            : "r"(start)
            : "memory"
        );
        start += line_size;
    }

    __asm__ __volatile__("dsb sy" ::: "memory");
}

/*
 * memori_get_asid
 * ---------------
 * Dapatkan ASID baru.
 */
tak_bertanda32_t memori_get_asid(void)
{
    tak_bertanda32_t asid;

    asid = g_asid_counter;

    /* ASID is 16-bit */
    g_asid_counter = (g_asid_counter + 1) & 0xFFFF;
    if (g_asid_counter == 0) {
        g_asid_counter = 1;
    }

    return asid;
}

/*
 * memori_switch_context
 * ---------------------
 * Switch memory context (page table).
 */
void memori_switch_context(pte_t *pgd, tak_bertanda32_t asid)
{
    tak_bertanda64_t ttbr0;

    /* Combine PGD address with ASID */
    ttbr0 = (tak_bertanda64_t)pgd;
    ttbr0 |= ((tak_bertanda64_t)asid << 48);

    __asm__ __volatile__(
        "msr ttbr0_el1, %0\n\t"
        "isb"
        :
        : "r"(ttbr0)
        : "memory"
    );

    /* Invalidate TLB for new ASID */
    __asm__ __volatile__(
        "tlbi ASIDE1, %0\n\t"
        "dsb sy\n\t"
        "isb"
        :
        : "r"((tak_bertanda64_t)asid)
        : "memory"
    );

    g_current_pgd = pgd;
}

/*
 * memori_get_kernel_pgd
 * ---------------------
 * Dapatkan PGD kernel.
 */
pte_t *memori_get_kernel_pgd(void)
{
    return g_kernel_pgd;
}
