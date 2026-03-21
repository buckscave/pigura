/*
 * PIGURA OS - MEMORI_ARM.C
 * -------------------------
 * Implementasi manajemen memori untuk arsitektur ARM (32-bit).
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola memori fisik
 * dan virtual pada prosesor ARM dengan MMU.
 *
 * Arsitektur: ARM (ARMv4/v5/v6 - 32-bit)
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

#define PAGE_B_BIT              (1 << 2)    /* Bufferable */
#define PAGE_C_BIT              (1 << 3)    /* Cacheable */
#define PAGE_DOMAIN_SHIFT       5
#define PAGE_AP_SHIFT           10
#define PAGE_TEX_SHIFT          12

/* Access permissions */
#define AP_PRIV_RW              0x00
#define AP_RW                   0x01
#define AP_PRIV_RO              0x02
#define AP_RO                   0x03

/* Domain access control */
#define DOMAIN_CLIENT           0x01

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Translation table */
static tak_bertanda32_t g_page_table[PAGE_TABLE_ENTRIES]
    __attribute__((aligned(16384)));

/* Physical memory allocator */
#define MAX_PHYSICAL_PAGES      32768
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
    tak_bertanda32_t i;
    tak_bertanda32_t found;
    tak_bertanda32_t start;

    if (count == 0 || g_free_phys_pages < count) {
        return 0;
    }

    /* Cari blok berurutan */
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
                break;
            }
        } else {
            found = 0;
        }
    }

    if (found < count) {
        return 0;
    }

    /* Alokasikan halaman */
    for (i = 0; i < count; i++) {
        tak_bertanda32_t index = start + i;
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
    tak_bertanda32_t index;
    tak_bertanda32_t entry;
    tak_bertanda32_t pa;

    va = (tak_bertanda32_t)(tak_bertanda32_t)virtual;
    index = va >> 20;

    entry = g_page_table[index];

    /* Cek apakah section */
    if ((entry & 0x3) != PAGE_TYPE_SECTION) {
        return 0;  /* Tidak mapped */
    }

    /* Extract physical address */
    pa = (entry & 0xFFF00000) | (va & 0x000FFFFF);

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
    tak_bertanda32_t ctrl;

    /* Baca Control Register */
    ctrl = memori_cp15_read(0, 1, 0, 0);

    /* Enable MMU */
    ctrl |= 0x0001;    /* M bit - MMU enable */
    ctrl |= 0x0004;    /* C bit - Data cache enable */
    ctrl |= 0x1000;    /* I bit - Instruction cache enable */

    /* Tulis Control Register */
    memori_cp15_write(0, 1, 0, 0, ctrl);

    /* ISB untuk memastikan perubahan berlaku */
    __asm__ __volatile__("mcr p15, 0, r0, c7, c5, 4");

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
    tak_bertanda32_t ctrl;

    /* Flush cache terlebih dahulu */
    hal_cpu_cache_flush();

    /* Baca Control Register */
    ctrl = memori_cp15_read(0, 1, 0, 0);

    /* Disable MMU dan cache */
    ctrl &= ~0x0001;   /* M bit */
    ctrl &= ~0x0004;   /* C bit */
    ctrl &= ~0x1000;   /* I bit */

    /* Tulis Control Register */
    memori_cp15_write(0, 1, 0, 0, ctrl);

    /* Invalidate TLB */
    hal_cpu_invalidate_tlb_all();

    __asm__ __volatile__("mcr p15, 0, r0, c7, c5, 4");

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
    tak_bertanda32_t ctrl;

    ctrl = memori_cp15_read(0, 1, 0, 0);

    return (ctrl & 0x0001) ? BENAR : SALAH;
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
 * Inisialisasi subsistem memori arsitektur ARM.
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
