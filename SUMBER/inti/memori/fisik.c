/*
 * PIGURA OS - FISIK.C
 * -------------------
 * Implementasi manajemen memori fisik.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk mengelola
 * alokasi dan dealokasi memori fisik menggunakan bitmap allocator.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../kernel.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* Ukuran halaman */
#define PAGE_SIZE               4096UL

/* Bit per word dalam bitmap */
#define BITS_PER_WORD           32

/* Maksimum region memori */
#define MAX_MEMORY_REGIONS      32

/* Memory type */
#define MEMORY_TYPE_USABLE      1
#define MEMORY_TYPE_RESERVED    2
#define MEMORY_TYPE_ACPI        3
#define MEMORY_TYPE_NVS         4
#define MEMORY_TYPE_UNUSABLE    5

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* Memory region */
typedef struct {
    alamat_fisik_t mulai;
    alamat_fisik_t akhir;
    tak_bertanda32_t tipe;
} memori_region_t;

/* Physical memory manager state */
typedef struct {
    tak_bertanda32_t *bitmap;           /* Bitmap untuk tracking halaman */
    tak_bertanda64_t total_pages;       /* Total halaman */
    tak_bertanda64_t free_pages;        /* Halaman bebas */
    tak_bertanda64_t used_pages;        /* Halaman terpakai */
    tak_bertanda64_t reserved_pages;    /* Halaman reserved */
    tak_bertanda64_t last_alloc_index;  /* Index alokasi terakhir */
    tak_bertanda32_t bitmap_size;       /* Ukuran bitmap dalam word */
    alamat_fisik_t max_address;         /* Alamat fisik maksimum */
    spinlock_t lock;                    /* Lock untuk thread safety */
} pmm_state_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* PMM state */
static pmm_state_t pmm = {0};

/* Memory regions */
static memori_region_t regions[MAX_MEMORY_REGIONS];
static tak_bertanda32_t region_count = 0;

/* Initialization flag */
static bool_t pmm_initialized = SALAH;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * bitmap_set
 * ----------
 * Set bit dalam bitmap (halaman digunakan).
 *
 * Parameter:
 *   index - Index halaman
 */
static void bitmap_set(tak_bertanda64_t index)
{
    tak_bertanda32_t word = index / BITS_PER_WORD;
    tak_bertanda32_t bit = index % BITS_PER_WORD;
    pmm.bitmap[word] |= (1UL << bit);
}

/*
 * bitmap_clear
 * ------------
 * Clear bit dalam bitmap (halaman bebas).
 *
 * Parameter:
 *   index - Index halaman
 */
static void bitmap_clear(tak_bertanda64_t index)
{
    tak_bertanda32_t word = index / BITS_PER_WORD;
    tak_bertanda32_t bit = index % BITS_PER_WORD;
    pmm.bitmap[word] &= ~(1UL << bit);
}

/*
 * bitmap_test
 * -----------
 * Test bit dalam bitmap.
 *
 * Parameter:
 *   index - Index halaman
 *
 * Return: BENAR jika bit set (digunakan)
 */
static bool_t bitmap_test(tak_bertanda64_t index)
{
    tak_bertanda32_t word = index / BITS_PER_WORD;
    tak_bertanda32_t bit = index % BITS_PER_WORD;
    return (pmm.bitmap[word] & (1UL << bit)) ? BENAR : SALAH;
}

/*
 * find_free_page
 * --------------
 * Cari halaman bebas menggunakan first-fit.
 *
 * Return: Index halaman, atau -1 jika tidak ada
 */
static tanda64_t find_free_page(void)
{
    tak_bertanda64_t i;
    tak_bertanda32_t word;
    tak_bertanda32_t bit;
    tak_bertanda32_t value;

    /* Cari dari last_alloc_index untuk efisiensi */
    for (i = pmm.last_alloc_index; i < pmm.total_pages; i++) {
        word = i / BITS_PER_WORD;
        bit = i % BITS_PER_WORD;

        if (bit == 0) {
            /* Optimasi: cek seluruh word */
            value = pmm.bitmap[word];
            if (value == 0xFFFFFFFF) {
                /* Semua bit set, skip */
                i += BITS_PER_WORD - 1;
                continue;
            }
        }

        if (!bitmap_test(i)) {
            return (tanda64_t)i;
        }
    }

    /* Cari dari awal jika tidak ketemu */
    for (i = 0; i < pmm.last_alloc_index; i++) {
        if (!bitmap_test(i)) {
            return (tanda64_t)i;
        }
    }

    return -1;  /* Tidak ada halaman bebas */
}

/*
 * find_free_pages
 * ---------------
 * Cari blok halaman bebas berurutan.
 *
 * Parameter:
 *   count - Jumlah halaman yang dibutuhkan
 *
 * Return: Index halaman pertama, atau -1 jika tidak ada
 */
static tanda64_t find_free_pages(tak_bertanda32_t count)
{
    tak_bertanda64_t i;
    tak_bertanda64_t j;
    bool_t found;

    if (count == 0 || count > pmm.free_pages) {
        return -1;
    }

    for (i = 0; i < pmm.total_pages; i++) {
        /* Cek apakah bisa mulai dari sini */
        if (bitmap_test(i)) {
            continue;
        }

        found = BENAR;

        /* Cek count halaman berikutnya */
        for (j = 0; j < count; j++) {
            if (i + j >= pmm.total_pages || bitmap_test(i + j)) {
                found = SALAH;
                i += j;  /* Skip ke yang sudah dicek */
                break;
            }
        }

        if (found) {
            return (tanda64_t)i;
        }
    }

    return -1;
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * pmm_init
 * --------
 * Inisialisasi physical memory manager.
 *
 * Parameter:
 *   mem_size   - Total ukuran memori dalam byte
 *   bitmap_addr - Alamat untuk bitmap
 *
 * Return: Status operasi
 */
status_t pmm_init(tak_bertanda64_t mem_size, void *bitmap_addr)
{
    tak_bertanda64_t i;

    if (pmm_initialized) {
        return STATUS_SUDAH_ADA;
    }

    /* Hitung jumlah halaman */
    pmm.total_pages = mem_size / PAGE_SIZE;
    pmm.max_address = mem_size;

    /* Hitung ukuran bitmap */
    pmm.bitmap_size = (pmm.total_pages + BITS_PER_WORD - 1) / BITS_PER_WORD;

    /* Set bitmap address */
    pmm.bitmap = (tak_bertanda32_t *)bitmap_addr;

    /* Inisialisasi bitmap - semua halaman digunakan dulu */
    for (i = 0; i < pmm.bitmap_size; i++) {
        pmm.bitmap[i] = 0xFFFFFFFF;
    }

    /* Reset counters */
    pmm.free_pages = 0;
    pmm.used_pages = pmm.total_pages;
    pmm.reserved_pages = 0;
    pmm.last_alloc_index = 0;

    /* Init spinlock */
    spinlock_init(&pmm.lock);

    pmm_initialized = BENAR;

    kernel_printf("[PMM] Initialized: %lu MB total, %lu pages\n",
                  mem_size / (1024 * 1024), pmm.total_pages);

    return STATUS_BERHASIL;
}

/*
 * pmm_add_region
 * --------------
 * Tambah region memori yang tersedia.
 *
 * Parameter:
 *   mulai - Alamat awal
 *   akhir - Alamat akhir (eksklusif)
 *   tipe  - Tipe region
 *
 * Return: Status operasi
 */
status_t pmm_add_region(alamat_fisik_t mulai, alamat_fisik_t akhir,
                        tak_bertanda32_t tipe)
{
    tak_bertanda64_t start_page;
    tak_bertanda64_t end_page;
    tak_bertanda64_t i;

    if (!pmm_initialized) {
        return STATUS_GAGAL;
    }

    if (region_count >= MAX_MEMORY_REGIONS) {
        return STATUS_BUSY;
    }

    /* Simpan region */
    regions[region_count].mulai = mulai;
    regions[region_count].akhir = akhir;
    regions[region_count].tipe = tipe;
    region_count++;

    /* Jika region usable, tandai sebagai free */
    if (tipe == MEMORY_TYPE_USABLE) {
        /* Align ke batas halaman */
        mulai = RATAKAN_ATAS(mulai, PAGE_SIZE);
        akhir = RATAKAN_BAWAH(akhir, PAGE_SIZE);

        if (mulai >= akhir) {
            return STATUS_BERHASIL;
        }

        start_page = mulai / PAGE_SIZE;
        end_page = akhir / PAGE_SIZE;

        /* Mark sebagai free */
        spinlock_kunci(&pmm.lock);

        for (i = start_page; i < end_page && i < pmm.total_pages; i++) {
            if (bitmap_test(i)) {
                bitmap_clear(i);
                pmm.free_pages++;
                pmm.used_pages--;
            }
        }

        spinlock_buka(&pmm.lock);
    }

    return STATUS_BERHASIL;
}

/*
 * pmm_reserve
 * -----------
 * Reserve region memori.
 *
 * Parameter:
 *   mulai - Alamat awal
 *   ukuran - Ukuran region
 *
 * Return: Status operasi
 */
status_t pmm_reserve(alamat_fisik_t mulai, tak_bertanda64_t ukuran)
{
    tak_bertanda64_t start_page;
    tak_bertanda64_t end_page;
    tak_bertanda64_t i;

    if (!pmm_initialized) {
        return STATUS_GAGAL;
    }

    /* Align */
    mulai = RATAKAN_BAWAH(mulai, PAGE_SIZE);
    ukuran = RATAKAN_ATAS(ukuran, PAGE_SIZE);

    start_page = mulai / PAGE_SIZE;
    end_page = (mulai + ukuran) / PAGE_SIZE;

    spinlock_kunci(&pmm.lock);

    for (i = start_page; i < end_page && i < pmm.total_pages; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            pmm.free_pages--;
            pmm.used_pages++;
        }
        pmm.reserved_pages++;
    }

    spinlock_buka(&pmm.lock);

    return STATUS_BERHASIL;
}

/*
 * pmm_alloc_page
 * --------------
 * Alokasikan satu halaman fisik.
 *
 * Return: Alamat fisik halaman, atau 0 jika gagal
 */
alamat_fisik_t pmm_alloc_page(void)
{
    tanda64_t index;
    alamat_fisik_t addr;

    if (!pmm_initialized || pmm.free_pages == 0) {
        return 0;
    }

    spinlock_kunci(&pmm.lock);

    index = find_free_page();
    if (index < 0) {
        spinlock_buka(&pmm.lock);
        return 0;
    }

    bitmap_set(index);
    pmm.free_pages--;
    pmm.used_pages++;
    pmm.last_alloc_index = index;

    spinlock_buka(&pmm.lock);

    addr = (alamat_fisik_t)index * PAGE_SIZE;

    /* Clear halaman untuk keamanan */
    kernel_memset((void *)(uintptr_t)addr, 0, PAGE_SIZE);

    return addr;
}

/*
 * pmm_free_page
 * -------------
 * Bebaskan satu halaman fisik.
 *
 * Parameter:
 *   addr - Alamat fisik halaman
 *
 * Return: Status operasi
 */
status_t pmm_free_page(alamat_fisik_t addr)
{
    tak_bertanda64_t index;

    if (!pmm_initialized) {
        return STATUS_GAGAL;
    }

    /* Validasi alamat */
    if (addr == 0 || (addr % PAGE_SIZE) != 0) {
        return STATUS_PARAM_INVALID;
    }

    index = addr / PAGE_SIZE;

    if (index >= pmm.total_pages) {
        return STATUS_PARAM_INVALID;
    }

    spinlock_kunci(&pmm.lock);

    /* Cek double free */
    if (!bitmap_test(index)) {
        spinlock_buka(&pmm.lock);
        kernel_printf("[PMM] WARNING: Double free at 0x%lX\n", addr);
        return STATUS_GAGAL;
    }

    bitmap_clear(index);
    pmm.free_pages++;
    pmm.used_pages--;

    spinlock_buka(&pmm.lock);

    return STATUS_BERHASIL;
}

/*
 * pmm_alloc_pages
 * ---------------
 * Alokasikan multiple halaman berurutan.
 *
 * Parameter:
 *   count - Jumlah halaman
 *
 * Return: Alamat fisik halaman pertama, atau 0 jika gagal
 */
alamat_fisik_t pmm_alloc_pages(tak_bertanda32_t count)
{
    tanda64_t index;
    alamat_fisik_t addr;
    tak_bertanda32_t i;

    if (!pmm_initialized || count == 0 || pmm.free_pages < count) {
        return 0;
    }

    spinlock_kunci(&pmm.lock);

    index = find_free_pages(count);
    if (index < 0) {
        spinlock_buka(&pmm.lock);
        return 0;
    }

    /* Mark semua sebagai used */
    for (i = 0; i < count; i++) {
        bitmap_set(index + i);
    }

    pmm.free_pages -= count;
    pmm.used_pages += count;
    pmm.last_alloc_index = index + count - 1;

    spinlock_buka(&pmm.lock);

    addr = (alamat_fisik_t)index * PAGE_SIZE;

    /* Clear semua halaman */
    kernel_memset((void *)(uintptr_t)addr, 0, (ukuran_t)(count * PAGE_SIZE));

    return addr;
}

/*
 * pmm_free_pages
 * --------------
 * Bebaskan multiple halaman.
 *
 * Parameter:
 *   addr  - Alamat fisik halaman pertama
 *   count - Jumlah halaman
 *
 * Return: Status operasi
 */
status_t pmm_free_pages(alamat_fisik_t addr, tak_bertanda32_t count)
{
    tak_bertanda32_t i;
    status_t status;

    for (i = 0; i < count; i++) {
        status = pmm_free_page(addr + (i * PAGE_SIZE));
        if (status != STATUS_BERHASIL) {
            return status;
        }
    }

    return STATUS_BERHASIL;
}

/*
 * pmm_get_stats
 * -------------
 * Dapatkan statistik PMM.
 *
 * Parameter:
 *   total  - Pointer untuk total pages
 *   free   - Pointer untuk free pages
 *   used   - Pointer untuk used pages
 */
void pmm_get_stats(tak_bertanda64_t *total, tak_bertanda64_t *free,
                   tak_bertanda64_t *used)
{
    if (total != NULL) {
        *total = pmm.total_pages;
    }

    if (free != NULL) {
        *free = pmm.free_pages;
    }

    if (used != NULL) {
        *used = pmm.used_pages;
    }
}

/*
 * pmm_print_stats
 * ---------------
 * Print statistik PMM.
 */
void pmm_print_stats(void)
{
    kernel_printf("\n[PMM] Physical Memory Statistics:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Total pages:     %lu (%lu MB)\n",
                  pmm.total_pages,
                  (pmm.total_pages * PAGE_SIZE) / (1024 * 1024));
    kernel_printf("  Free pages:      %lu (%lu MB)\n",
                  pmm.free_pages,
                  (pmm.free_pages * PAGE_SIZE) / (1024 * 1024));
    kernel_printf("  Used pages:      %lu (%lu MB)\n",
                  pmm.used_pages,
                  (pmm.used_pages * PAGE_SIZE) / (1024 * 1024));
    kernel_printf("  Reserved pages:  %lu\n", pmm.reserved_pages);
    kernel_printf("  Bitmap size:     %lu bytes\n",
                  pmm.bitmap_size * sizeof(tak_bertanda32_t));
    kernel_printf("========================================\n");
}

/*
 * pmm_is_allocated
 * ----------------
 * Cek apakah halaman sudah dialokasi.
 *
 * Parameter:
 *   addr - Alamat fisik
 *
 * Return: BENAR jika sudah dialokasi
 */
bool_t pmm_is_allocated(alamat_fisik_t addr)
{
    tak_bertanda64_t index;

    if (!pmm_initialized) {
        return SALAH;
    }

    index = addr / PAGE_SIZE;

    if (index >= pmm.total_pages) {
        return BENAR;  /* Di luar range = dianggap used */
    }

    return bitmap_test(index);
}
