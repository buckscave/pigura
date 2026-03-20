/*
 * PIGURA OS - HAL_MEMORI.C
 * ------------------------
 * Implementasi manajemen memori untuk Hardware Abstraction Layer.
 *
 * Berkas ini berisi implementasi fungsi-fungsi yang berkaitan dengan
 * deteksi dan alokasi memori fisik dasar. Alokasi memori yang lebih
 * kompleks ditangani oleh subsistem memori utama.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "hal.h"
#include "../kernel.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* Memory map entry types */
#define MMAP_TYPE_RAM           1
#define MMAP_TYPE_RESERVED      2
#define MMAP_TYPE_ACPI_RECLAIM  3
#define MMAP_TYPE_NVS           4
#define MMAP_TYPE_UNUSABLE      5

/* Minimum memory required */
#define MINIMUM_MEMORY          (4UL * 1024UL * 1024UL)  /* 4 MB */

/* Memory bitmap granularity */
#define BITMAP_GRANULARITY      4096  /* Satu bit = satu halaman */

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* Memory region */
typedef struct {
    alamat_fisik_t mulai;       /* Alamat awal */
    alamat_fisik_t akhir;       /* Alamat akhir (eksklusif) */
    tak_bertanda32_t tipe;      /* Tipe region */
} memori_region_t;

/* Memory bitmap */
typedef struct {
    tak_bertanda32_t *bitmap;       /* Pointer ke bitmap */
    tak_bertanda64_t total_pages;   /* Total halaman */
    tak_bertanda64_t free_pages;    /* Halaman bebas */
    tak_bertanda64_t last_alloc;    /* Indeks alokasi terakhir */
    tak_bertanda32_t bitmap_size;   /* Ukuran bitmap dalam byte */
} memori_bitmap_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Memory regions (max 32) */
static memori_region_t memori_regions[32];
static tak_bertanda32_t region_count = 0;

/* Memory bitmap */
static memori_bitmap_t memori_bitmap = {0};

/* State */
static bool_t memori_initialized = SALAH;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * bitmap_set_bit
 * --------------
 * Set bit dalam bitmap.
 *
 * Parameter:
 *   index - Indeks bit
 */
static inline void bitmap_set_bit(tak_bertanda64_t index)
{
    tak_bertanda32_t word = index / 32;
    tak_bertanda32_t bit = index % 32;
    memori_bitmap.bitmap[word] |= (1UL << bit);
}

/*
 * bitmap_clear_bit
 * ----------------
 * Clear bit dalam bitmap.
 *
 * Parameter:
 *   index - Indeks bit
 */
static inline void bitmap_clear_bit(tak_bertanda64_t index)
{
    tak_bertanda32_t word = index / 32;
    tak_bertanda32_t bit = index % 32;
    memori_bitmap.bitmap[word] &= ~(1UL << bit);
}

/*
 * bitmap_test_bit
 * ---------------
 * Test bit dalam bitmap.
 *
 * Parameter:
 *   index - Indeks bit
 *
 * Return: 1 jika set, 0 jika clear
 */
static inline bool_t bitmap_test_bit(tak_bertanda64_t index)
{
    tak_bertanda32_t word = index / 32;
    tak_bertanda32_t bit = index % 32;
    return (memori_bitmap.bitmap[word] & (1UL << bit)) ? BENAR : SALAH;
}

/*
 * parse_multiboot_mmap
 * --------------------
 * Parse memory map dari multiboot info.
 *
 * Parameter:
 *   bootinfo - Pointer ke multiboot info
 *
 * Return: Status operasi
 */
static status_t parse_multiboot_mmap(multiboot_info_t *bootinfo)
{
    mmap_entry_t *entry;
    tak_bertanda32_t i;

    if (!(bootinfo->flags & MULTIBOOT_FLAG_MMAP)) {
        /* Tidak ada memory map, gunakan mem_lower/mem_upper */
        memori_regions[0].mulai = 0;
        memori_regions[0].akhir = bootinfo->mem_lower * 1024;
        memori_regions[0].tipe = MMAP_TYPE_RAM;

        memori_regions[1].mulai = 0x100000;  /* 1 MB */
        memori_regions[1].akhir = 0x100000 +
                                 (bootinfo->mem_upper * 1024);
        memori_regions[1].tipe = MMAP_TYPE_RAM;

        region_count = 2;
        return STATUS_BERHASIL;
    }

    /* Parse memory map entries */
    entry = (mmap_entry_t *)bootinfo->mmap_addr;
    i = 0;

    while ((tak_bertanda32_t)entry < bootinfo->mmap_addr +
                                     bootinfo->mmap_length) {
        if (i >= 32) {
            break;
        }

        memori_regions[i].mulai = entry->base_addr;
        memori_regions[i].akhir = entry->base_addr + entry->length;
        memori_regions[i].tipe = entry->type;

        entry = (mmap_entry_t *)((tak_bertanda8_t *)entry +
                                 entry->size + sizeof(entry->size));
        i++;
    }

    region_count = i;

    return STATUS_BERHASIL;
}

/*
 * calculate_total_memory
 * ----------------------
 * Hitung total memori yang tersedia.
 *
 * Return: Total memori dalam byte
 */
static alamat_fisik_t calculate_total_memory(void)
{
    alamat_fisik_t total = 0;
    tak_bertanda32_t i;

    for (i = 0; i < region_count; i++) {
        if (memori_regions[i].tipe == MMAP_TYPE_RAM) {
            total += memori_regions[i].akhir - memori_regions[i].mulai;
        }
    }

    return total;
}

/*
 * init_memory_bitmap
 * ------------------
 * Inisialisasi bitmap untuk manajemen memori fisik.
 *
 * Parameter:
 *   total_pages - Total jumlah halaman
 *
 * Return: Status operasi
 */
static status_t init_memory_bitmap(tak_bertanda64_t total_pages)
{
    tak_bertanda64_t bitmap_size;
    tak_bertanda32_t i;
    tak_bertanda32_t j;

    /* Hitung ukuran bitmap */
    bitmap_size = (total_pages + 31) / 32 * sizeof(tak_bertanda32_t);

    /* Round up ke ukuran halaman */
    bitmap_size = RATAKAN_ATAS(bitmap_size, UKURAN_HALAMAN);

    /* Cari lokasi untuk bitmap (setelah kernel) */
    /* Untuk sekarang, gunakan alamat setelah 1 MB */
    memori_bitmap.bitmap = (tak_bertanda32_t *)0x100000;

    /* Clear bitmap */
    for (i = 0; i < bitmap_size / sizeof(tak_bertanda32_t); i++) {
        memori_bitmap.bitmap[i] = 0xFFFFFFFF;  /* Semua digunakan dulu */
    }

    memori_bitmap.total_pages = total_pages;
    memori_bitmap.free_pages = 0;
    memori_bitmap.bitmap_size = (tak_bertanda32_t)bitmap_size;
    memori_bitmap.last_alloc = 0;

    /* Mark available RAM sebagai free */
    for (i = 0; i < region_count; i++) {
        if (memori_regions[i].tipe == MMAP_TYPE_RAM) {
            tak_bertanda64_t start_page;
            tak_bertanda64_t end_page;

            start_page = memori_regions[i].mulai / UKURAN_HALAMAN;
            end_page = memori_regions[i].akhir / UKURAN_HALAMAN;

            for (j = (tak_bertanda32_t)start_page;
                 j < (tak_bertanda32_t)end_page &&
                 j < total_pages; j++) {
                bitmap_clear_bit(j);
                memori_bitmap.free_pages++;
            }
        }
    }

    /* Mark kernel dan bitmap sebagai used */
    /* Kernel: 1 MB - 4 MB (default) */
    for (i = 0x100000 / UKURAN_HALAMAN;
         i < 0x400000 / UKURAN_HALAMAN &&
         i < total_pages; i++) {
        if (!bitmap_test_bit(i)) {
            bitmap_set_bit(i);
            memori_bitmap.free_pages--;
        }
    }

    /* Mark first page (NULL pointer protection) */
    bitmap_set_bit(0);
    if (memori_bitmap.free_pages > 0) {
        memori_bitmap.free_pages--;
    }

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_memory_init
 * ---------------
 * Inisialisasi subsistem memori.
 */
status_t hal_memory_init(void *bootinfo)
{
    hal_memory_info_t *info = &g_hal_state.memory;
    multiboot_info_t *mb_info = (multiboot_info_t *)bootinfo;
    alamat_fisik_t total_memory;
    tak_bertanda64_t total_pages;

    if (memori_initialized) {
        return STATUS_SUDAH_ADA;
    }

    if (bootinfo == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Parse memory map */
    if (parse_multiboot_mmap(mb_info) != STATUS_BERHASIL) {
        return STATUS_GAGAL;
    }

    /* Hitung total memori */
    total_memory = calculate_total_memory();

    if (total_memory < MINIMUM_MEMORY) {
        kernel_printf("[HAL] Memori tidak cukup: %lu MB (minimum: %lu MB)\n",
                      total_memory / (1024 * 1024),
                      MINIMUM_MEMORY / (1024 * 1024));
        return STATUS_MEMORI_HABIS;
    }

    /* Hitung jumlah halaman */
    total_pages = total_memory / UKURAN_HALAMAN;

    /* Inisialisasi bitmap */
    if (init_memory_bitmap(total_pages) != STATUS_BERHASIL) {
        return STATUS_GAGAL;
    }

    /* Set informasi memori */
    info->total_bytes = total_memory;
    info->available_bytes = memori_bitmap.free_pages * UKURAN_HALAMAN;
    info->page_size = UKURAN_HALAMAN;
    info->page_count = total_pages;
    info->has_pae = SALAH;  /* Akan di-deteksi nanti */
    info->has_nx = SALAH;   /* Akan di-deteksi nanti */

    memori_initialized = BENAR;

    kernel_printf("[HAL] Memori: %lu MB total, %lu MB tersedia\n",
                  total_memory / (1024 * 1024),
                  info->available_bytes / (1024 * 1024));

    return STATUS_BERHASIL;
}

/*
 * hal_memory_get_info
 * -------------------
 * Dapatkan informasi memori.
 */
status_t hal_memory_get_info(hal_memory_info_t *info)
{
    if (info == NULL) {
        return STATUS_PARAM_INVALID;
    }

    if (!memori_initialized) {
        return STATUS_GAGAL;
    }

    kernel_memcpy(info, &g_hal_state.memory, sizeof(hal_memory_info_t));

    return STATUS_BERHASIL;
}

/*
 * hal_memory_get_free_pages
 * -------------------------
 * Dapatkan jumlah halaman bebas.
 */
tak_bertanda64_t hal_memory_get_free_pages(void)
{
    return memori_bitmap.free_pages;
}

/*
 * hal_memory_get_total_pages
 * --------------------------
 * Dapatkan jumlah total halaman.
 */
tak_bertanda64_t hal_memory_get_total_pages(void)
{
    return memori_bitmap.total_pages;
}

/*
 * hal_memory_alloc_page
 * ---------------------
 * Alokasikan satu halaman fisik.
 */
alamat_fisik_t hal_memory_alloc_page(void)
{
    tak_bertanda64_t i;
    tak_bertanda64_t start;
    tak_bertanda64_t pages;

    if (!memori_initialized || memori_bitmap.free_pages == 0) {
        return ALAMAT_FISIK_INVALID;
    }

    pages = memori_bitmap.total_pages;
    start = memori_bitmap.last_alloc;

    /* Cari halaman bebas (first-fit) */
    for (i = 0; i < pages; i++) {
        tak_bertanda64_t index = (start + i) % pages;

        if (!bitmap_test_bit(index)) {
            /* Ditemukan */
            bitmap_set_bit(index);
            memori_bitmap.free_pages--;
            memori_bitmap.last_alloc = index;

            return index * UKURAN_HALAMAN;
        }
    }

    return ALAMAT_FISIK_INVALID;
}

/*
 * hal_memory_free_page
 * --------------------
 * Bebaskan satu halaman fisik.
 */
status_t hal_memory_free_page(alamat_fisik_t addr)
{
    tak_bertanda64_t index;

    if (!memori_initialized) {
        return STATUS_GAGAL;
    }

    /* Validasi alamat */
    if (addr == 0 || (addr % UKURAN_HALAMAN) != 0) {
        return STATUS_PARAM_INVALID;
    }

    index = addr / UKURAN_HALAMAN;

    if (index >= memori_bitmap.total_pages) {
        return STATUS_PARAM_INVALID;
    }

    /* Cek apakah sudah free */
    if (!bitmap_test_bit(index)) {
        /* Double free! */
        kernel_printf("[HAL] PERINGATAN: Double free pada alamat 0x%lX\n",
                      addr);
        return STATUS_GAGAL;
    }

    /* Free halaman */
    bitmap_clear_bit(index);
    memori_bitmap.free_pages++;

    return STATUS_BERHASIL;
}

/*
 * hal_memory_alloc_pages
 * ----------------------
 * Alokasikan multiple halaman fisik berurutan.
 */
alamat_fisik_t hal_memory_alloc_pages(tak_bertanda32_t count)
{
    tak_bertanda64_t i;
    tak_bertanda64_t j;
    tak_bertanda64_t pages;
    tak_bertanda64_t start;
    bool_t found;

    if (!memori_initialized || count == 0 ||
        memori_bitmap.free_pages < count) {
        return ALAMAT_FISIK_INVALID;
    }

    pages = memori_bitmap.total_pages;
    start = memori_bitmap.last_alloc;

    /* Cari blok berurutan */
    for (i = 0; i < pages; i++) {
        found = BENAR;

        for (j = 0; j < count; j++) {
            if (bitmap_test_bit((start + i + j) % pages)) {
                found = SALAH;
                break;
            }
        }

        if (found) {
            /* Mark semua sebagai used */
            for (j = 0; j < count; j++) {
                bitmap_set_bit((start + i + j) % pages);
                memori_bitmap.free_pages--;
            }

            memori_bitmap.last_alloc = (start + i + count) % pages;

            return ((start + i) % pages) * UKURAN_HALAMAN;
        }
    }

    return ALAMAT_FISIK_INVALID;
}

/*
 * hal_memory_free_pages
 * ---------------------
 * Bebaskan multiple halaman fisik.
 */
status_t hal_memory_free_pages(alamat_fisik_t addr, tak_bertanda32_t count)
{
    tak_bertanda32_t i;
    status_t status;

    if (count == 0) {
        return STATUS_PARAM_INVALID;
    }

    /* Free satu per satu */
    for (i = 0; i < count; i++) {
        status = hal_memory_free_page(addr + (i * UKURAN_HALAMAN));
        if (status != STATUS_BERHASIL) {
            return status;
        }
    }

    return STATUS_BERHASIL;
}
