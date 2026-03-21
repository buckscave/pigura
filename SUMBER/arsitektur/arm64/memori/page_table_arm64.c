/*
 * PIGURA OS - PAGE_TABLE_ARM64.C
 * ------------------------------
 * Implementasi page table untuk ARM64 (AArch64).
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola page table
 * pada prosesor ARM64.
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

/* Page table levels for 4KB pages */
#define PT_LEVEL_0              0
#define PT_LEVEL_1              1
#define PT_LEVEL_2              2
#define PT_LEVEL_3              3

/* Page table entries per level */
#define PT_ENTRIES              512

/* Page sizes */
#define PT_PAGE_SIZE            4096ULL
#define PT_BLOCK_SIZE_L1        (512ULL * 1024 * 1024 * 1024)  /* 512GB */
#define PT_BLOCK_SIZE_L2        (2ULL * 1024 * 1024)           /* 2MB */

/* Page table entry bits */
#define PTE_VALID               (1ULL << 0)
#define PTE_TABLE               (1ULL << 1)
#define PTE_BLOCK               0
#define PTE_PAGE                (1ULL << 1)
#define PTE_ATTR_IDX(x)         ((tak_bertanda64_t)(x) << 2)
#define PTE_NS                  (1ULL << 5)
#define PTE_AP_USER_RW          (0ULL << 6)
#define PTE_AP_USER_RO          (1ULL << 6)
#define PTE_AP_KERNEL_RW        (1ULL << 7)
#define PTE_AP_KERNEL_RO        (3ULL << 6)
#define PTE_SH_NS               (0ULL << 8)
#define PTE_SH_OS               (2ULL << 8)
#define PTE_SH_IS               (3ULL << 8)
#define PTE_AF                  (1ULL << 10)
#define PTE_NG                  (1ULL << 11)
#define PTE_PXN                 (1ULL << 53)
#define PTE_UXN                 (1ULL << 54)

/* Physical address mask (48-bit) */
#define PTE_PA_MASK             0x0000FFFFFFFFF000ULL

/* Virtual address masks and shifts */
#define VA_INDEX_MASK           0x1FF
#define VA_INDEX_SHIFT_L0       39
#define VA_INDEX_SHIFT_L1       30
#define VA_INDEX_SHIFT_L2       21
#define VA_INDEX_SHIFT_L3       12

/* MAIR indices */
#define MAIR_IDX_NORMAL         4
#define MAIR_IDX_NORMAL_NC      3
#define MAIR_IDX_DEVICE         1

/*
 * ============================================================================
 * TIPE DATA LOKAL (LOCAL DATA TYPES)
 * ============================================================================
 */

/* Page table entry type */
typedef tak_bertanda64_t pte_t;

/* Page table structure */
typedef struct {
    pte_t *table;
    tak_bertanda32_t level;
} page_table_t;

/*
 * ============================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ============================================================================
 */

/* Kernel page table */
static pte_t *g_kernel_pgd = NULL;

/* Page table pool */
static pte_t *g_pt_pool = NULL;
static tak_bertanda32_t g_pt_pool_count = 0;
static tak_bertanda32_t g_pt_pool_next = 0;

/*
 * ============================================================================
 * FUNGSI BANTUAN (HELPER FUNCTIONS)
 * ============================================================================
 */

/*
 * _get_index
 * ----------
 * Get page table index for given level.
 */
static inline tak_bertanda32_t _get_index(tak_bertanda64_t va,
                                           tak_bertanda32_t level)
{
    tak_bertanda32_t shift;

    switch (level) {
        case 0: shift = VA_INDEX_SHIFT_L0; break;
        case 1: shift = VA_INDEX_SHIFT_L1; break;
        case 2: shift = VA_INDEX_SHIFT_L2; break;
        case 3: shift = VA_INDEX_SHIFT_L3; break;
        default: return 0;
    }

    return (tak_bertanda32_t)((va >> shift) & VA_INDEX_MASK);
}

/*
 * _pte_valid
 * ----------
 * Check if PTE is valid.
 */
static inline bool_t _pte_valid(pte_t pte)
{
    return (pte & PTE_VALID) ? BENAR : SALAH;
}

/*
 * _pte_table
 * ----------
 * Check if PTE is a table descriptor.
 */
static inline bool_t _pte_table(pte_t pte)
{
    return (pte & PTE_TABLE) ? BENAR : SALAH;
}

/*
 * _pte_block
 * ----------
 * Check if PTE is a block descriptor.
 */
static inline bool_t _pte_block(pte_t pte, tak_bertanda32_t level)
{
    return _pte_valid(pte) && !_pte_table(pte) && (level < 3);
}

/*
 * _pte_page
 * ---------
 * Check if PTE is a page descriptor.
 */
static inline bool_t _pte_page(pte_t pte, tak_bertanda32_t level)
{
    return _pte_valid(pte) && _pte_table(pte) && (level == 3);
}

/*
 * _pte_to_pa
 * ----------
 * Convert PTE to physical address.
 */
static inline tak_bertanda64_t _pte_to_pa(pte_t pte)
{
    return (pte & PTE_PA_MASK);
}

/*
 * _pa_to_pte
 * ----------
 * Convert physical address to PTE.
 */
static inline pte_t _pa_to_pte(tak_bertanda64_t pa)
{
    return (pa & PTE_PA_MASK);
}

/*
 * _alloc_pt
 * ---------
 * Allocate a page table.
 */
static pte_t *_alloc_pt(void)
{
    pte_t *pt;
    tak_bertanda32_t i;

    if (g_pt_pool != NULL && g_pt_pool_next < g_pt_pool_count) {
        pt = &g_pt_pool[g_pt_pool_next++];
    } else {
        pt = (pte_t *)hal_memory_alloc_page();
    }

    if (pt == NULL) {
        return NULL;
    }

    /* Clear table */
    for (i = 0; i < PT_ENTRIES; i++) {
        pt[i] = 0;
    }

    return pt;
}

/*
 * _free_pt
 * --------
 * Free a page table.
 */
static void _free_pt(pte_t *pt)
{
    if (pt != NULL) {
        hal_memory_free_page((alamat_fisik_t)pt);
    }
}

/*
 * _walk_to_pte
 * ------------
 * Walk page table to get PTE for VA.
 */
static pte_t *_walk_to_pte(pte_t *pgd, tak_bertanda64_t va,
                            bool_t alloc, tak_bertanda32_t *level_out)
{
    pte_t *table;
    pte_t pte;
    tak_bertanda32_t index;
    tak_bertanda32_t level;

    if (pgd == NULL) {
        return NULL;
    }

    table = pgd;

    for (level = 0; level <= 3; level++) {
        index = _get_index(va, level);
        pte = table[index];

        if (level == 3) {
            /* Level 3: Page descriptor */
            if (level_out != NULL) {
                *level_out = level;
            }
            return &table[index];
        }

        if (!_pte_valid(pte)) {
            /* Invalid entry */
            if (alloc) {
                pte_t *new_table = _alloc_pt();
                if (new_table == NULL) {
                    return NULL;
                }
                pte = _pa_to_pte((tak_bertanda64_t)new_table) |
                      PTE_VALID | PTE_TABLE;
                table[index] = pte;
                table = new_table;
            } else {
                if (level_out != NULL) {
                    *level_out = level;
                }
                return &table[index];
            }
        } else if (_pte_table(pte)) {
            /* Table descriptor: continue to next level */
            table = (pte_t *)_pte_to_pa(pte);
        } else {
            /* Block descriptor: stop here */
            if (level_out != NULL) {
                *level_out = level;
            }
            return &table[index];
        }
    }

    return NULL;
}

/*
 * ============================================================================
 * PUBLIC FUNCTIONS
 * ============================================================================
 */

/*
 * page_table_init
 * ---------------
 * Initialize page table subsystem.
 */
status_t page_table_init(void)
{
    kernel_printf("[PT-ARM64] Initializing page table subsystem\n");

    /* Allocate kernel PGD */
    g_kernel_pgd = _alloc_pt();
    if (g_kernel_pgd == NULL) {
        kernel_printf("[PT-ARM64] Failed to allocate kernel PGD\n");
        return STATUS_MEMORI_HABIS;
    }

    kernel_printf("[PT-ARM64] Kernel PGD at 0x%lX\n",
        (tak_bertanda64_t)g_kernel_pgd);

    return STATUS_BERHASIL;
}

/*
 * page_table_create
 * -----------------
 * Create a new page table.
 */
pte_t *page_table_create(void)
{
    pte_t *pgd;

    pgd = _alloc_pt();
    if (pgd == NULL) {
        return NULL;
    }

    return pgd;
}

/*
 * page_table_destroy
 * ------------------
 * Destroy a page table.
 */
void page_table_destroy(pte_t *pgd)
{
    tak_bertanda32_t i0, i1, i2;
    pte_t pte0, pte1, pte2;
    pte_t *table1, *table2, *table3;

    if (pgd == NULL) {
        return;
    }

    /* Walk and free all tables */
    for (i0 = 0; i0 < PT_ENTRIES; i0++) {
        pte0 = pgd[i0];
        if (!_pte_valid(pte0) || !_pte_table(pte0)) {
            continue;
        }

        table1 = (pte_t *)_pte_to_pa(pte0);

        for (i1 = 0; i1 < PT_ENTRIES; i1++) {
            pte1 = table1[i1];
            if (!_pte_valid(pte1) || !_pte_table(pte1)) {
                continue;
            }

            table2 = (pte_t *)_pte_to_pa(pte1);

            for (i2 = 0; i2 < PT_ENTRIES; i2++) {
                pte2 = table2[i2];
                if (!_pte_valid(pte2) || !_pte_table(pte2)) {
                    continue;
                }

                table3 = (pte_t *)_pte_to_pa(pte2);
                _free_pt(table3);
            }

            _free_pt(table2);
        }

        _free_pt(table1);
    }

    _free_pt(pgd);
}

/*
 * page_table_map
 * --------------
 * Map a page.
 */
status_t page_table_map(pte_t *pgd, tak_bertanda64_t va,
                         tak_bertanda64_t pa, tak_bertanda64_t flags)
{
    pte_t *ptep;
    pte_t pte;
    tak_bertanda32_t level;

    if (pgd == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Align addresses */
    va &= ~0xFFFULL;
    pa &= ~0xFFFULL;

    ptep = _walk_to_pte(pgd, va, BENAR, &level);
    if (ptep == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    /* Create page descriptor */
    pte = _pa_to_pte(pa) | PTE_VALID | PTE_PAGE | PTE_AF;
    pte |= PTE_SH_IS;
    pte |= PTE_ATTR_IDX(MAIR_IDX_NORMAL);
    pte |= flags;

    /* Set UXN/PXN for kernel addresses */
    if (va >= 0xFFFF000000000000ULL) {
        pte |= PTE_UXN | PTE_PXN;
    }

    *ptep = pte;

    return STATUS_BERHASIL;
}

/*
 * page_table_map_block
 * --------------------
 * Map a block (2MB).
 */
status_t page_table_map_block(pte_t *pgd, tak_bertanda64_t va,
                               tak_bertanda64_t pa, tak_bertanda64_t flags)
{
    pte_t *ptep;
    pte_t pte;
    tak_bertanda32_t level;

    if (pgd == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Align to 2MB */
    va &= ~0x1FFFFFULL;
    pa &= ~0x1FFFFFULL;

    /* Get level 2 PTE */
    ptep = _walk_to_pte(pgd, va, BENAR, &level);
    if (ptep == NULL || level > 2) {
        return STATUS_GAGAL;
    }

    /* Create block descriptor */
    pte = _pa_to_pte(pa) | PTE_VALID | PTE_AF;
    pte |= PTE_SH_IS;
    pte |= PTE_ATTR_IDX(MAIR_IDX_NORMAL);
    pte |= flags;

    if (va >= 0xFFFF000000000000ULL) {
        pte |= PTE_UXN | PTE_PXN;
    }

    *ptep = pte;

    return STATUS_BERHASIL;
}

/*
 * page_table_unmap
 * ----------------
 * Unmap a page.
 */
status_t page_table_unmap(pte_t *pgd, tak_bertanda64_t va)
{
    pte_t *ptep;
    tak_bertanda32_t level;

    if (pgd == NULL) {
        return STATUS_PARAM_INVALID;
    }

    ptep = _walk_to_pte(pgd, va, SALAH, &level);
    if (ptep == NULL || !_pte_valid(*ptep)) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Invalidate PTE */
    *ptep = 0;

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
 * page_table_translate
 * --------------------
 * Translate VA to PA.
 */
tak_bertanda64_t page_table_translate(pte_t *pgd, tak_bertanda64_t va)
{
    pte_t *ptep;
    pte_t pte;
    tak_bertanda32_t level;
    tak_bertanda64_t pa;

    if (pgd == NULL) {
        return 0;
    }

    ptep = _walk_to_pte(pgd, va, SALAH, &level);
    if (ptep == NULL) {
        return 0;
    }

    pte = *ptep;
    if (!_pte_valid(pte)) {
        return 0;
    }

    pa = _pte_to_pa(pte);

    /* Add offset */
    if (level == 1) {
        pa |= (va & 0x3FFFFFFF);
    } else if (level == 2) {
        pa |= (va & 0x1FFFFF);
    } else {
        pa |= (va & 0xFFF);
    }

    return pa;
}

/*
 * page_table_get_kernel_pgd
 * -------------------------
 * Get kernel PGD.
 */
pte_t *page_table_get_kernel_pgd(void)
{
    return g_kernel_pgd;
}

/*
 * page_table_set_kernel_pgd
 * -------------------------
 * Set kernel PGD.
 */
void page_table_set_kernel_pgd(pte_t *pgd)
{
    g_kernel_pgd = pgd;
}

/*
 * page_table_dump
 * ---------------
 * Dump page table (for debugging).
 */
void page_table_dump(pte_t *pgd, tak_bertanda32_t max_entries)
{
    tak_bertanda32_t i0, i1, i2, i3;
    pte_t pte;
    tak_bertanda32_t count = 0;

    if (pgd == NULL) {
        kernel_printf("[PT] NULL PGD\n");
        return;
    }

    kernel_printf("[PT] Page table at 0x%lX:\n", (tak_bertanda64_t)pgd);

    /* Simplified dump - just scan PGD entries */
    for (i0 = 0; i0 < PT_ENTRIES && count < max_entries; i0++) {
        pte = pgd[i0];
        if (_pte_valid(pte)) {
            kernel_printf("[PT] L0[%u] = 0x%lX\n", i0, pte);
            count++;
        }
    }
}
