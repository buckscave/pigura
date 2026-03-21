/*
 * PIGURA OS - MEMORI_ARMV7.C
 * ---------------------------
 * Implementasi manajemen memori untuk arsitektur ARMv7.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola memori fisik
 * dan virtual pada prosesor ARMv7 dengan MMU.
 *
 * Arsitektur: ARMv7 (Cortex-A series)
 * Versi: 1.0
 */

#include "../../inti/types.h"
#include "../../inti/konstanta.h"
#include "../../inti/hal/hal.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* Page table constants */
#define PAGE_TABLE_ENTRIES      4096
#define PAGE_TABLE_SIZE         (PAGE_TABLE_ENTRIES * 4)

/* Translation table base address mask */
#define TTBR_ADDR_MASK          0xFFFFC000

/* Page flags (short descriptor format) */
#define PAGE_TYPE_SECTION       0x02
#define PAGE_TYPE_COARSE        0x01
#define PAGE_TYPE_FINE          0x03

#define PAGE_B_BIT              (1 << 2)    /* Bufferable */
#define PAGE_C_BIT              (1 << 3)    /* Cacheable */
#define PAGE_XN_BIT             (1 << 4)    /* Execute Never */
#define PAGE_DOMAIN_SHIFT       5
#define PAGE_DOMAIN_MASK        (0xF << 5)
#define PAGE_AP_SHIFT           10
#define PAGE_AP_MASK            (0x3 << 10)
#define PAGE_TEX_SHIFT          12
#define PAGE_TEX_MASK           (0x7 << 12)
#define PAGE_APX_BIT            (1 << 15)   /* Access Permission X */
#define PAGE_S_BIT              (1 << 16)   /* Shareable */
#define PAGE_NG_BIT             (1 << 17)   /* Not Global */
#define PAGE_NS_BIT             (1 << 19)   /* Non-Secure */

/* Access permissions */
#define AP_PRIV_RW              0x00        /* RW privileged only */
#define AP_RW                   0x01        /* RW privileged, RO user */
#define AP_PRIV_RO              0x02        /* RO privileged only */
#define AP_RO                   0x03        /* RO all */

/* Memory types (TEX + C + B) */
#define MEM_TYPE_STRONG         0x00        /* Strongly-ordered */
#define MEM_TYPE_DEVICE         0x04        /* Device memory */
#define MEM_TYPE_NORMAL_NC      0x08        /* Normal, non-cacheable */
#define MEM_TYPE_NORMAL_WT      0x00        /* Normal, write-through */
#define MEM_TYPE_NORMAL_WB      0x04        /* Normal, write-back */

/* Domain access control */
#define DOMAIN_NO_ACCESS        0x00
#define DOMAIN_CLIENT           0x01
#define DOMAIN_MANAGER          0x03

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Translation table */
static tak_bertanda32_t g_page_table[PAGE_TABLE_ENTRIES]
    __attribute__((aligned(16384)));

/* Secondary page tables for 4KB pages */
#define MAX_SECONDARY_TABLES    256
static tak_bertanda32_t *g_secondary_tables[MAX_SECONDARY_TABLES];
static tak_bertanda32_t g_secondary_table_count = 0;

/* Memory regions */
#define MAX_MEMORY_REGIONS      16
typedef struct {
    alamat_fisik_t mulai;
    alamat_fisik_t akhir;
    tak_bertanda32_t flags;
    bool_t digunakan;
} memori_region_t;

static memori_region_t g_memory_regions[MAX_MEMORY_REGIONS];

/* Physical memory allocator */
#define MAX_PHYSICAL_PAGES      65536   /* 256 MB max */
static tak_bertanda32_t g_phys_page_bitmap[MAX_PHYSICAL_PAGES / 32];
static tak_bertanda64_t g_total_phys_pages = 0;
static tak_bertanda64_t g_free_phys_pages = 0;

/* Flag inisialisasi */
static bool_t g_memory_initialized = SALAH;

/*
 * ============================================================================
 * FUNGSI BANTUAN CP15 (CP15 HELPER FUNCTIONS)
 * ============================================================================
 */

/*
 * memori_cp15_read
 * ----------------
 * Baca register CP15.
 */
static inline tak_bertanda32_t memori_cp15_read(tak_bertanda32_t op1,
    tak_bertanda32_t c1, tak_bertanda32_t c2, tak_bertanda32_t op2)
{
    tak_bertanda32_t val;
    __asm__ __volatile__(
        "mrc p15, %1, %0, %2, %3, %4"
        : "=r"(val)
        : "i"(op1), "i"(c1), "i"(c2), "i"(op2)
    );
    return val;
}

/*
 * memori_cp15_write
 * -----------------
 * Tulis register CP15.
 */
static inline void memori_cp15_write(tak_bertanda32_t op1,
    tak_bertanda32_t c1, tak_bertanda32_t c2, tak_bertanda32_t op2,
    tak_bertanda32_t val)
{
    __asm__ __volatile__(
        "mcr p15, %0, %1, %2, %3, %4"
        :
        : "i"(op1), "r"(val), "i"(c1), "i"(c2), "i"(op2)
        : "memory"
    );
}

/*
 * ============================================================================
 * FUNGSI ALOKATOR HALAMAN FISIK (PHYSICAL PAGE ALLOCATOR)
 * ============================================================================
 */

/*
 * memori_phys_find_free_page
 * --------------------------
 * Cari halaman fisik bebas.
 *
 * Return: Index halaman, atau -1 jika tidak ada
 */
static tanda64_t memori_phys_find_free_page(void)
{
    tak_bertanda32_t i;
    tak_bertanda32_t j;

    for (i = 0; i < (MAX_PHYSICAL_PAGES / 32); i++) {
        if (g_phys_page_bitmap[i] != 0xFFFFFFFF) {
            for (j = 0; j < 32; j++) {
                if (!(g_phys_page_bitmap[i] & (1U << j))) {
                    return (tanda64_t)(i * 32 + j);
                }
            }
        }
    }

    return -1;
}

/*
 * memori_phys_find_contiguous_pages
 * ---------------------------------
 * Cari halaman berurutan.
 *
 * Parameter:
 *   count - Jumlah halaman
 *
 * Return: Index halaman pertama, atau -1 jika tidak ada
 */
static tanda64_t memori_phys_find_contiguous_pages(tak_bertanda32_t count)
{
    tak_bertanda32_t i;
    tak_bertanda32_t found;
    tak_bertanda32_t start;

    found = 0;
    start = 0;

    for (i = 0; i < MAX_PHYSICAL_PAGES; i++) {
        tak_bertanda32_t word = i / 32;
        tak_bertanda32_t bit = i % 32;

        if (!(g_phys_page_bitmap[word] & (1U << bit))) {
            if (found == 0) {
                start = i;
            }
            found++;
            if (found == count) {
                return (tanda64_t)start;
            }
        } else {
            found = 0;
        }
    }

    return -1;
}

/*
 * hal_memori_phys_alloc_page
 * --------------------------
 * Alokasikan satu halaman fisik.
 *
 * Return: Alamat fisik, atau 0 jika gagal
 */
alamat_fisik_t hal_memori_phys_alloc_page(void)
{
    tanda64_t index;
    alamat_fisik_t addr;

    if (g_free_phys_pages == 0) {
        return 0;
    }

    index = memori_phys_find_free_page();
    if (index < 0) {
        return 0;
    }

    /* Set bit dalam bitmap */
    g_phys_page_bitmap[index / 32] |= (1U << (index % 32));
    g_free_phys_pages--;

    /* Konversi ke alamat fisik */
    addr = (alamat_fisik_t)(index * UKURAN_HALAMAN);

    return addr;
}

/*
 * hal_memori_phys_free_page
 * -------------------------
 * Bebaskan satu halaman fisik.
 *
 * Parameter:
 *   addr - Alamat fisik
 *
 * Return: Status operasi
 */
status_t hal_memori_phys_free_page(alamat_fisik_t addr)
{
    tak_bertanda32_t index;

    if (addr % UKURAN_HALAMAN != 0) {
        return STATUS_PARAM_INVALID;
    }

    index = (tak_bertanda32_t)(addr / UKURAN_HALAMAN);
    if (index >= MAX_PHYSICAL_PAGES) {
        return STATUS_PARAM_INVALID;
    }

    /* Cek apakah sudah bebas */
    if (!(g_phys_page_bitmap[index / 32] & (1U << (index % 32)))) {
        return STATUS_PARAM_INVALID;
    }

    /* Clear bit dalam bitmap */
    g_phys_page_bitmap[index / 32] &= ~(1U << (index % 32));
    g_free_phys_pages++;

    return STATUS_BERHASIL;
}

/*
 * hal_memori_phys_alloc_pages
 * ---------------------------
 * Alokasikan halaman berurutan.
 *
 * Parameter:
 *   count - Jumlah halaman
 *
 * Return: Alamat fisik halaman pertama, atau 0 jika gagal
 */
alamat_fisik_t hal_memori_phys_alloc_pages(tak_bertanda32_t count)
{
    tanda64_t start;
    tak_bertanda32_t i;

    if (count == 0 || g_free_phys_pages < count) {
        return 0;
    }

    start = memori_phys_find_contiguous_pages(count);
    if (start < 0) {
        return 0;
    }

    /* Alokasikan halaman */
    for (i = 0; i < count; i++) {
        tak_bertanda32_t index = (tak_bertanda32_t)start + i;
        g_phys_page_bitmap[index / 32] |= (1U << (index % 32));
    }

    g_free_phys_pages -= count;

    return (alamat_fisik_t)(start * UKURAN_HALAMAN);
}

/*
 * hal_memori_phys_free_pages
 * --------------------------
 * Bebaskan halaman berurutan.
 *
 * Parameter:
 *   addr  - Alamat fisik
 *   count - Jumlah halaman
 *
 * Return: Status operasi
 */
status_t hal_memori_phys_free_pages(alamat_fisik_t addr, tak_bertanda32_t count)
{
    tak_bertanda32_t i;
    tak_bertanda32_t start;

    if (addr % UKURAN_HALAMAN != 0 || count == 0) {
        return STATUS_PARAM_INVALID;
    }

    start = (tak_bertanda32_t)(addr / UKURAN_HALAMAN);
    if (start + count > MAX_PHYSICAL_PAGES) {
        return STATUS_PARAM_INVALID;
    }

    for (i = 0; i < count; i++) {
        tak_bertanda32_t index = start + i;
        if (g_phys_page_bitmap[index / 32] & (1U << (index % 32))) {
            g_phys_page_bitmap[index / 32] &= ~(1U << (index % 32));
            g_free_phys_pages++;
        }
    }

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI PAGE TABLE (PAGE TABLE FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_memori_set_section
 * ----------------------
 * Set section mapping (1 MB).
 *
 * Parameter:
 *   virtual  - Alamat virtual
 *   physical - Alamat fisik
 *   flags    - Flags halaman
 *
 * Return: Status operasi
 */
status_t hal_memori_set_section(void *virtual, alamat_fisik_t physical,
    tak_bertanda32_t flags)
{
    tak_bertanda32_t va;
    tak_bertanda32_t pa;
    tak_bertanda32_t index;
    tak_bertanda32_t entry;

    va = (tak_bertanda32_t)(tak_bertanda32_t)virtual;
    pa = (tak_bertanda32_t)physical;

    /* Section harus 1 MB aligned */
    if ((va & 0x000FFFFF) || (pa & 0x000FFFFF)) {
        return STATUS_PARAM_INVALID;
    }

    /* Index dalam page table */
    index = va >> 20;

    /* Buat section descriptor */
    entry = (pa & 0xFFF00000) | PAGE_TYPE_SECTION | flags;

    g_page_table[index] = entry;

    /* Invalidate TLB untuk alamat ini */
    hal_cpu_invalidate_tlb(virtual);

    return STATUS_BERHASIL;
}

/*
 * hal_memori_clear_section
 * ------------------------
 * Hapus section mapping.
 *
 * Parameter:
 *   virtual - Alamat virtual
 *
 * Return: Status operasi
 */
status_t hal_memori_clear_section(void *virtual)
{
    tak_bertanda32_t va;
    tak_bertanda32_t index;

    va = (tak_bertanda32_t)(tak_bertanda32_t)virtual;
    index = va >> 20;

    g_page_table[index] = 0;

    hal_cpu_invalidate_tlb(virtual);

    return STATUS_BERHASIL;
}

/*
 * hal_memori_map_page
 * -------------------
 * Map satu halaman 4 KB.
 *
 * Parameter:
 *   virtual  - Alamat virtual
 *   physical - Alamat fisik
 *   flags    - Flags halaman
 *
 * Return: Status operasi
 */
status_t hal_memori_map_page(void *virtual, alamat_fisik_t physical,
    tak_bertanda32_t flags)
{
    tak_bertanda32_t va;
    tak_bertanda32_t pa;
    tak_bertanda32_t l1_index;
    tak_bertanda32_t l2_index;
    tak_bertanda32_t *l2_table;
    tak_bertanda32_t entry;

    va = (tak_bertanda32_t)(tak_bertanda32_t)virtual;
    pa = (tak_bertanda32_t)physical;

    /* Halaman harus 4 KB aligned */
    if ((va & 0xFFF) || (pa & 0xFFF)) {
        return STATUS_PARAM_INVALID;
    }

    /* Dapatkan L1 index dan L2 index */
    l1_index = va >> 20;
    l2_index = (va >> 12) & 0xFF;

    /* Cek atau buat L2 table */
    if ((g_page_table[l1_index] & 0x3) != PAGE_TYPE_COARSE) {
        /* Alokasikan L2 table baru */
        alamat_fisik_t l2_phys;
        tak_bertanda32_t *l2_virt;

        l2_phys = hal_memori_phys_alloc_page();
        if (l2_phys == 0) {
            return STATUS_MEMORI_HABIS;
        }

        l2_virt = (tak_bertanda32_t *)l2_phys;  /* Identity mapping */

        /* Clear L2 table */
        kernel_memset(l2_virt, 0, UKURAN_HALAMAN);

        /* Set L1 entry ke L2 table */
        g_page_table[l1_index] = (tak_bertanda32_t)l2_phys | PAGE_TYPE_COARSE;

        l2_table = l2_virt;
    } else {
        l2_table = (tak_bertanda32_t *)(g_page_table[l1_index] & 0xFFFFFC00);
    }

    /* Buat L2 entry (small page) */
    entry = (pa & 0xFFFFF000) | flags | 0x02;  /* Small page */

    l2_table[l2_index] = entry;

    hal_cpu_invalidate_tlb(virtual);

    return STATUS_BERHASIL;
}

/*
 * hal_memori_unmap_page
 * ---------------------
 * Hapus mapping halaman.
 *
 * Parameter:
 *   virtual - Alamat virtual
 *
 * Return: Status operasi
 */
status_t hal_memori_unmap_page(void *virtual)
{
    tak_bertanda32_t va;
    tak_bertanda32_t l1_index;
    tak_bertanda32_t l2_index;
    tak_bertanda32_t *l2_table;

    va = (tak_bertanda32_t)(tak_bertanda32_t)virtual;
    l1_index = va >> 20;
    l2_index = (va >> 12) & 0xFF;

    /* Cek apakah ada L2 table */
    if ((g_page_table[l1_index] & 0x3) != PAGE_TYPE_COARSE) {
        return STATUS_BERHASIL;  /* Tidak ada mapping */
    }

    l2_table = (tak_bertanda32_t *)(g_page_table[l1_index] & 0xFFFFFC00);
    l2_table[l2_index] = 0;

    hal_cpu_invalidate_tlb(virtual);

    return STATUS_BERHASIL;
}

/*
 * hal_memori_virt_to_phys
 * -----------------------
 * Konversi alamat virtual ke fisik.
 *
 * Parameter:
 *   virtual - Alamat virtual
 *
 * Return: Alamat fisik, atau 0 jika tidak mapped
 */
alamat_fisik_t hal_memori_virt_to_phys(void *virtual)
{
    tak_bertanda32_t va;
    tak_bertanda32_t l1_index;
    tak_bertanda32_t l1_entry;
    tak_bertanda32_t pa;

    va = (tak_bertanda32_t)(tak_bertanda32_t)virtual;
    l1_index = va >> 20;

    l1_entry = g_page_table[l1_index];

    switch (l1_entry & 0x3) {
        case PAGE_TYPE_SECTION:
            /* Section mapping (1 MB) */
            pa = (l1_entry & 0xFFF00000) | (va & 0x000FFFFF);
            break;

        case PAGE_TYPE_COARSE:
            /* L2 table (4 KB pages) */
            {
                tak_bertanda32_t l2_index = (va >> 12) & 0xFF;
                tak_bertanda32_t *l2_table = (tak_bertanda32_t *)
                    (l1_entry & 0xFFFFFC00);
                tak_bertanda32_t l2_entry = l2_table[l2_index];

                if ((l2_entry & 0x3) == 0) {
                    return 0;  /* Not mapped */
                }

                pa = (l2_entry & 0xFFFFF000) | (va & 0xFFF);
            }
            break;

        default:
            return 0;  /* Not mapped */
    }

    return (alamat_fisik_t)pa;
}

/*
 * ============================================================================
 * FUNGSI MMU (MMU FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_memori_mmu_enable
 * ---------------------
 * Aktifkan MMU.
 *
 * Return: Status operasi
 */
status_t hal_memori_mmu_enable(void)
{
    tak_bertanda32_t sctlr;

    /* Baca SCTLR */
    sctlr = memori_cp15_read(0, 1, 0, 0);

    /* Enable MMU */
    sctlr |= 0x0001;    /* M bit - MMU enable */
    sctlr |= 0x0004;    /* C bit - Data cache enable */
    sctlr |= 0x1000;    /* I bit - Instruction cache enable */
    sctlr |= 0x0800;    /* Z bit - Branch prediction enable */

    /* Tulis SCTLR */
    memori_cp15_write(0, 1, 0, 0, sctlr);

    /* ISB untuk memastikan perubahan berlaku */
    __asm__ __volatile__("isb");

    return STATUS_BERHASIL;
}

/*
 * hal_memori_mmu_disable
 * ----------------------
 * Nonaktifkan MMU.
 *
 * Return: Status operasi
 */
status_t hal_memori_mmu_disable(void)
{
    tak_bertanda32_t sctlr;

    /* Flush cache terlebih dahulu */
    hal_cpu_cache_flush();

    /* Baca SCTLR */
    sctlr = memori_cp15_read(0, 1, 0, 0);

    /* Disable MMU dan cache */
    sctlr &= ~0x0001;   /* M bit */
    sctlr &= ~0x0004;   /* C bit */
    sctlr &= ~0x1000;   /* I bit */

    /* Tulis SCTLR */
    memori_cp15_write(0, 1, 0, 0, sctlr);

    /* Invalidate TLB */
    hal_cpu_invalidate_tlb_all();

    __asm__ __volatile__("isb");

    return STATUS_BERHASIL;
}

/*
 * hal_memori_mmu_is_enabled
 * -------------------------
 * Cek apakah MMU aktif.
 *
 * Return: BENAR jika aktif, SALAH jika tidak
 */
bool_t hal_memori_mmu_is_enabled(void)
{
    tak_bertanda32_t sctlr;

    sctlr = memori_cp15_read(0, 1, 0, 0);

    return (sctlr & 0x0001) ? BENAR : SALAH;
}

/*
 * ============================================================================
 * FUNGSI INISIALISASI (INITIALIZATION FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_memori_init_page_table
 * --------------------------
 * Inisialisasi page table dengan identity mapping.
 *
 * Return: Status operasi
 */
status_t hal_memori_init_page_table(void)
{
    tak_bertanda32_t i;

    /* Clear page table */
    kernel_memset(g_page_table, 0, sizeof(g_page_table));

    /* Identity mapping untuk 1 GB pertama dengan section entries */
    for (i = 0; i < 1024; i++) {
        tak_bertanda32_t addr = i << 20;
        tak_bertanda32_t entry;

        /* Buat section descriptor */
        entry = addr | PAGE_TYPE_SECTION | PAGE_C_BIT | PAGE_B_BIT |
                (AP_RW << PAGE_AP_SHIFT) | (0 << PAGE_DOMAIN_SHIFT);

        g_page_table[i] = entry;
    }

    return STATUS_BERHASIL;
}

/*
 * hal_memori_arch_init
 * --------------------
 * Inisialisasi subsistem memori arsitektur ARMv7.
 *
 * Parameter:
 *   mem_total - Total memori dalam bytes
 *
 * Return: Status operasi
 */
status_t hal_memori_arch_init(tak_bertanda64_t mem_total)
{
    tak_bertanda32_t i;

    /* Clear physical page bitmap */
    kernel_memset(g_phys_page_bitmap, 0, sizeof(g_phys_page_bitmap));

    /* Hitung jumlah halaman fisik */
    g_total_phys_pages = mem_total / UKURAN_HALAMAN;
    if (g_total_phys_pages > MAX_PHYSICAL_PAGES) {
        g_total_phys_pages = MAX_PHYSICAL_PAGES;
    }
    g_free_phys_pages = g_total_phys_pages;

    /* Clear secondary tables */
    for (i = 0; i < MAX_SECONDARY_TABLES; i++) {
        g_secondary_tables[i] = NULL;
    }
    g_secondary_table_count = 0;

    /* Clear memory regions */
    for (i = 0; i < MAX_MEMORY_REGIONS; i++) {
        g_memory_regions[i].digunakan = SALAH;
    }

    /* Inisialisasi page table */
    hal_memori_init_page_table();

    /* Set TTBR0 */
    memori_cp15_write(0, 2, 0, 0, (tak_bertanda32_t)g_page_table);

    /* Set TTBCR (use TTBR0 only) */
    memori_cp15_write(0, 2, 0, 2, 0);

    /* Set Domain Access Control (client mode for domain 0) */
    memori_cp15_write(0, 3, 0, 0, DOMAIN_CLIENT);

    /* Invalidate TLB */
    hal_cpu_invalidate_tlb_all();

    g_memory_initialized = BENAR;

    return STATUS_BERHASIL;
}

/*
 * hal_memori_arch_shutdown
 * ------------------------
 * Shutdown subsistem memori.
 *
 * Return: Status operasi
 */
status_t hal_memori_arch_shutdown(void)
{
    /* Disable MMU */
    hal_memori_mmu_disable();

    g_memory_initialized = SALAH;

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI UTILITAS (UTILITY FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_memori_get_page_table
 * -------------------------
 * Dapatkan alamat page table.
 *
 * Return: Pointer ke page table
 */
void *hal_memori_get_page_table(void)
{
    return g_page_table;
}

/*
 * hal_memori_get_phys_stats
 * -------------------------
 * Dapatkan statistik memori fisik.
 *
 * Parameter:
 *   total - Pointer untuk total halaman
 *   free  - Pointer untuk halaman bebas
 */
void hal_memori_get_phys_stats(tak_bertanda64_t *total, tak_bertanda64_t *free)
{
    if (total != NULL) {
        *total = g_total_phys_pages;
    }
    if (free != NULL) {
        *free = g_free_phys_pages;
    }
}
