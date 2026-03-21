/*
 * PIGURA OS - PAGE_TABLE_ARM.C
 * -----------------------------
 * Implementasi manajemen page table untuk ARM (32-bit).
 *
 * Arsitektur: ARM (ARMv4/v5/v6 - 32-bit)
 * Versi: 1.0
 */

#include "../../../inti/types.h"
#include "../../../inti/konstanta.h"
#include "../../../inti/hal/hal.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* Page table levels */
#define PT_LEVEL_1          1       /* Coarse page table (section) */
#define PT_LEVEL_2          2       /* Fine page table (4KB pages) */

/* L1 descriptor types */
#define L1_TYPE_FAULT       0x00    /* Invalid entry */
#define L1_TYPE_COARSE      0x01    /* Coarse page table */
#define L1_TYPE_SECTION     0x02    /* Section (1 MB) */
#define L1_TYPE_FINE        0x03    /* Fine page table */

/* L2 descriptor types */
#define L2_TYPE_FAULT       0x00    /* Invalid entry */
#define L2_TYPE_LARGE       0x01    /* Large page (64 KB) */
#define L2_TYPE_SMALL       0x02    /* Small page (4 KB) */
#define L2_TYPE_TINY        0x03    /* Tiny page (1 KB) */

/* Page flags */
#define PAGE_B              (1 << 2)    /* Bufferable */
#define PAGE_C              (1 << 3)    /* Cacheable */
#define PAGE_AP_SHIFT       10
#define PAGE_DOMAIN_SHIFT   5

/* Access permissions */
#define AP_NO_ACCESS        0x00
#define AP_PRIV_RW          0x01
#define AP_PRIV_RO          0x02
#define AP_FULL_RW          0x03

/* Page sizes */
#define SECTION_SIZE        (1024 * 1024)  /* 1 MB */
#define LARGE_PAGE_SIZE     (64 * 1024)    /* 64 KB */
#define SMALL_PAGE_SIZE     (4 * 1024)     /* 4 KB */

/* Number of entries per level */
#define L1_ENTRIES          4096
#define L2_ENTRIES_COARSE   256

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Kernel page table (L1) */
static tak_bertanda32_t g_kernel_l1[L1_ENTRIES]
    __attribute__((aligned(16384)));

/* L2 page tables pool */
#define MAX_L2_TABLES       256
static tak_bertanda32_t g_l2_pool[MAX_L2_TABLES][L2_ENTRIES_COARSE]
    __attribute__((aligned(1024)));
static tak_bertanda32_t g_l2_table_used[MAX_L2_TABLES];

/*
 * ============================================================================
 * FUNGSI ALOKASI L2 TABLE (L2 TABLE ALLOCATION FUNCTIONS)
 * ============================================================================
 */

/*
 * page_table_alloc_l2
 * -------------------
 * Alokasikan L2 page table.
 *
 * Return: Index L2 table, atau -1 jika gagal
 */
static tanda32_t page_table_alloc_l2(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < MAX_L2_TABLES; i++) {
        if (!g_l2_table_used[i]) {
            g_l2_table_used[i] = 1;
            return (tanda32_t)i;
        }
    }

    return -1;
}

/*
 * page_table_free_l2
 * ------------------
 * Bebaskan L2 page table.
 *
 * Parameter:
 *   index - Index L2 table
 */
static void page_table_free_l2(tak_bertanda32_t index)
{
    if (index < MAX_L2_TABLES) {
        g_l2_table_used[index] = 0;
    }
}

/*
 * ============================================================================
 * FUNGSI L1 PAGE TABLE (L1 PAGE TABLE FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_page_table_l1_set_section
 * -----------------------------
 * Set L1 entry sebagai section.
 *
 * Parameter:
 *   index    - Index dalam L1 table
 *   physical - Alamat fisik section
 *   flags    - Flags untuk section
 *
 * Return: Status operasi
 */
status_t hal_page_table_l1_set_section(tak_bertanda32_t index,
    tak_bertanda32_t physical, tak_bertanda32_t flags)
{
    tak_bertanda32_t entry;

    if (index >= L1_ENTRIES) {
        return STATUS_PARAM_INVALID;
    }

    /* Physical address harus 1 MB aligned */
    if (physical & 0x000FFFFF) {
        return STATUS_PARAM_INVALID;
    }

    /* Build section descriptor */
    entry = (physical & 0xFFF00000) | L1_TYPE_SECTION | flags;

    g_kernel_l1[index] = entry;

    return STATUS_BERHASIL;
}

/*
 * hal_page_table_l1_set_coarse
 * ----------------------------
 * Set L1 entry sebagai coarse page table.
 *
 * Parameter:
 *   index       - Index dalam L1 table
 *   l2_physical - Alamat fisik L2 table
 *
 * Return: Status operasi
 */
status_t hal_page_table_l1_set_coarse(tak_bertanda32_t index,
    tak_bertanda32_t l2_physical)
{
    tak_bertanda32_t entry;

    if (index >= L1_ENTRIES) {
        return STATUS_PARAM_INVALID;
    }

    /* L2 table harus 1 KB aligned */
    if (l2_physical & 0x3FF) {
        return STATUS_PARAM_INVALID;
    }

    /* Build coarse descriptor */
    entry = (l2_physical & 0xFFFFFC00) | L1_TYPE_COARSE;

    g_kernel_l1[index] = entry;

    return STATUS_BERHASIL;
}

/*
 * hal_page_table_l1_set_fault
 * ---------------------------
 * Set L1 entry sebagai fault.
 *
 * Parameter:
 *   index - Index dalam L1 table
 *
 * Return: Status operasi
 */
status_t hal_page_table_l1_set_fault(tak_bertanda32_t index)
{
    if (index >= L1_ENTRIES) {
        return STATUS_PARAM_INVALID;
    }

    g_kernel_l1[index] = L1_TYPE_FAULT;

    return STATUS_BERHASIL;
}

/*
 * hal_page_table_l1_get
 * ---------------------
 * Dapatkan L1 entry.
 *
 * Parameter:
 *   index - Index dalam L1 table
 *
 * Return: Nilai L1 entry
 */
tak_bertanda32_t hal_page_table_l1_get(tak_bertanda32_t index)
{
    if (index >= L1_ENTRIES) {
        return 0;
    }

    return g_kernel_l1[index];
}

/*
 * ============================================================================
 * FUNGSI L2 PAGE TABLE (L2 PAGE TABLE FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_page_table_l2_set_small
 * ---------------------------
 * Set L2 entry sebagai small page.
 *
 * Parameter:
 *   l2_table - Pointer ke L2 table
 *   index    - Index dalam L2 table
 *   physical - Alamat fisik halaman
 *   flags    - Flags untuk halaman
 *
 * Return: Status operasi
 */
status_t hal_page_table_l2_set_small(tak_bertanda32_t *l2_table,
    tak_bertanda32_t index, tak_bertanda32_t physical,
    tak_bertanda32_t flags)
{
    tak_bertanda32_t entry;

    if (l2_table == NULL || index >= L2_ENTRIES_COARSE) {
        return STATUS_PARAM_INVALID;
    }

    /* Physical address harus 4 KB aligned */
    if (physical & 0xFFF) {
        return STATUS_PARAM_INVALID;
    }

    /* Build small page descriptor */
    entry = (physical & 0xFFFFF000) | L2_TYPE_SMALL | flags;

    l2_table[index] = entry;

    return STATUS_BERHASIL;
}

/*
 * hal_page_table_l2_set_large
 * ---------------------------
 * Set L2 entry sebagai large page.
 *
 * Parameter:
 *   l2_table - Pointer ke L2 table
 *   index    - Index dalam L2 table
 *   physical - Alamat fisik halaman
 *   flags    - Flags untuk halaman
 *
 * Return: Status operasi
 */
status_t hal_page_table_l2_set_large(tak_bertanda32_t *l2_table,
    tak_bertanda32_t index, tak_bertanda32_t physical,
    tak_bertanda32_t flags)
{
    tak_bertanda32_t entry;
    tak_bertanda32_t i;

    if (l2_table == NULL || index >= L2_ENTRIES_COARSE) {
        return STATUS_PARAM_INVALID;
    }

    /* Large page harus 64 KB aligned, dan index kelipatan 16 */
    if ((physical & 0xFFFF) || (index & 0xF)) {
        return STATUS_PARAM_INVALID;
    }

    /* Build large page descriptor */
    entry = (physical & 0xFFFF0000) | L2_TYPE_LARGE | flags;

    /* Large page occupies 16 consecutive entries */
    for (i = 0; i < 16; i++) {
        l2_table[index + i] = entry;
    }

    return STATUS_BERHASIL;
}

/*
 * hal_page_table_l2_set_fault
 * ---------------------------
 * Set L2 entry sebagai fault.
 *
 * Parameter:
 *   l2_table - Pointer ke L2 table
 *   index    - Index dalam L2 table
 *
 * Return: Status operasi
 */
status_t hal_page_table_l2_set_fault(tak_bertanda32_t *l2_table,
    tak_bertanda32_t index)
{
    if (l2_table == NULL || index >= L2_ENTRIES_COARSE) {
        return STATUS_PARAM_INVALID;
    }

    l2_table[index] = L2_TYPE_FAULT;

    return STATUS_BERHASIL;
}

/*
 * hal_page_table_l2_get
 * ---------------------
 * Dapatkan L2 entry.
 *
 * Parameter:
 *   l2_table - Pointer ke L2 table
 *   index    - Index dalam L2 table
 *
 * Return: Nilai L2 entry
 */
tak_bertanda32_t hal_page_table_l2_get(tak_bertanda32_t *l2_table,
    tak_bertanda32_t index)
{
    if (l2_table == NULL || index >= L2_ENTRIES_COARSE) {
        return 0;
    }

    return l2_table[index];
}

/*
 * ============================================================================
 * FUNGSI MAPPING (MAPPING FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_page_table_map_section
 * --------------------------
 * Map section (1 MB).
 *
 * Parameter:
 *   virtual  - Alamat virtual
 *   physical - Alamat fisik
 *   flags    - Flags untuk mapping
 *
 * Return: Status operasi
 */
status_t hal_page_table_map_section(void *virtual,
    alamat_fisik_t physical, tak_bertanda32_t flags)
{
    tak_bertanda32_t va = (tak_bertanda32_t)(tak_bertanda32_t)virtual;
    tak_bertanda32_t pa = (tak_bertanda32_t)physical;
    tak_bertanda32_t index;

    /* Section harus 1 MB aligned */
    if ((va & 0x000FFFFF) || (pa & 0x000FFFFF)) {
        return STATUS_PARAM_INVALID;
    }

    index = va >> 20;

    return hal_page_table_l1_set_section(index, pa, flags);
}

/*
 * hal_page_table_map_page
 * -----------------------
 * Map halaman 4 KB.
 *
 * Parameter:
 *   virtual  - Alamat virtual
 *   physical - Alamat fisik
 *   flags    - Flags untuk mapping
 *
 * Return: Status operasi
 */
status_t hal_page_table_map_page(void *virtual,
    alamat_fisik_t physical, tak_bertanda32_t flags)
{
    tak_bertanda32_t va = (tak_bertanda32_t)(tak_bertanda32_t)virtual;
    tak_bertanda32_t pa = (tak_bertanda32_t)physical;
    tak_bertanda32_t l1_index;
    tak_bertanda32_t l2_index;
    tak_bertanda32_t l1_entry;
    tak_bertanda32_t *l2_table;
    tanda32_t l2_pool_index;

    /* Halaman harus 4 KB aligned */
    if ((va & 0xFFF) || (pa & 0xFFF)) {
        return STATUS_PARAM_INVALID;
    }

    l1_index = va >> 20;
    l2_index = (va >> 12) & 0xFF;

    /* Cek atau buat L2 table */
    l1_entry = g_kernel_l1[l1_index];

    if ((l1_entry & 0x3) == L1_TYPE_FAULT) {
        /* Buat L2 table baru */
        l2_pool_index = page_table_alloc_l2();
        if (l2_pool_index < 0) {
            return STATUS_MEMORI_HABIS;
        }

        l2_table = g_l2_pool[l2_pool_index];

        /* Clear L2 table */
        kernel_memset(l2_table, 0, sizeof(g_l2_pool[0]));

        /* Set L1 entry */
        hal_page_table_l1_set_coarse(l1_index,
            (tak_bertanda32_t)l2_table);
    } else if ((l1_entry & 0x3) == L1_TYPE_COARSE) {
        /* L2 table sudah ada */
        l2_table = (tak_bertanda32_t *)(l1_entry & 0xFFFFFC00);
    } else {
        /* Sudah ada section mapping */
        return STATUS_SUDAH_ADA;
    }

    /* Set L2 entry */
    return hal_page_table_l2_set_small(l2_table, l2_index, pa, flags);
}

/*
 * hal_page_table_unmap
 * --------------------
 * Hapus mapping.
 *
 * Parameter:
 *   virtual - Alamat virtual
 *
 * Return: Status operasi
 */
status_t hal_page_table_unmap(void *virtual)
{
    tak_bertanda32_t va = (tak_bertanda32_t)(tak_bertanda32_t)virtual;
    tak_bertanda32_t l1_index;
    tak_bertanda32_t l1_entry;

    l1_index = va >> 20;
    l1_entry = g_kernel_l1[l1_index];

    if ((l1_entry & 0x3) == L1_TYPE_SECTION) {
        /* Unmap section */
        return hal_page_table_l1_set_fault(l1_index);
    } else if ((l1_entry & 0x3) == L1_TYPE_COARSE) {
        /* Unmap L2 entry */
        tak_bertanda32_t *l2_table = (tak_bertanda32_t *)
            (l1_entry & 0xFFFFFC00);
        tak_bertanda32_t l2_index = (va >> 12) & 0xFF;
        return hal_page_table_l2_set_fault(l2_table, l2_index);
    }

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI INISIALISASI (INITIALIZATION FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_page_table_init
 * -------------------
 * Inisialisasi page table dengan identity mapping.
 *
 * Return: Status operasi
 */
status_t hal_page_table_init(void)
{
    tak_bertanda32_t i;

    /* Clear L1 table */
    kernel_memset(g_kernel_l1, 0, sizeof(g_kernel_l1));

    /* Clear L2 pool usage */
    kernel_memset(g_l2_table_used, 0, sizeof(g_l2_table_used));

    /* Identity mapping untuk 1 GB pertama dengan section */
    for (i = 0; i < 1024; i++) {
        tak_bertanda32_t addr = i << 20;

        /* Section dengan full RW access, cacheable, bufferable */
        hal_page_table_l1_set_section(i, addr,
            PAGE_B | PAGE_C | (AP_FULL_RW << PAGE_AP_SHIFT));
    }

    return STATUS_BERHASIL;
}

/*
 * hal_page_table_get_base
 * -----------------------
 * Dapatkan alamat L1 table.
 *
 * Return: Alamat L1 table
 */
tak_bertanda32_t *hal_page_table_get_base(void)
{
    return g_kernel_l1;
}

/*
 * hal_page_table_get_size
 * -----------------------
 * Dapatkan ukuran L1 table.
 *
 * Return: Ukuran dalam bytes
 */
tak_bertanda32_t hal_page_table_get_size(void)
{
    return sizeof(g_kernel_l1);
}
