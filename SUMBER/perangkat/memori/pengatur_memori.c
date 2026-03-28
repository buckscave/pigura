/*
 * PIGURA OS - PENGATUR_MEMORI.C
 * ==============================
 * Implementasi pengatur memori untuk Pigura OS.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi untuk mengelola
 * seluruh subsistem memori termasuk RAM, cache, dan PCI.
 *
 * Kepatuhan standar:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Mendukung semua arsitektur (x86, x86_64, ARM, ARMv7, ARM64)
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "memori.h"
#include "../../inti/kernel.h"

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

memori_konteks_t g_memori_konteks;
bool_t g_memori_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

/* Buffer untuk error message */
static char g_memori_error[256];

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * memori_set_error - Set pesan error
 *
 * Parameter:
 *   pesan - Pesan error
 */
static void memori_set_error(const char *pesan)
{
    ukuran_t i;

    if (pesan == NULL) {
        return;
    }

    i = 0;
    while (pesan[i] != '\0' && i < 255) {
        g_memori_error[i] = pesan[i];
        i++;
    }
    g_memori_error[i] = '\0';
}

/*
 * memori_init_regions - Inisialisasi memory regions
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
static status_t memori_init_regions(void)
{
    tak_bertanda32_t i;

    /* Inisialisasi semua region */
    for (i = 0; i < MEMORI_REGION_MAKS; i++) {
        g_memori_konteks.regions[i].magic = 0;
        g_memori_konteks.regions[i].id = i;
        g_memori_konteks.regions[i].mulai = 0;
        g_memori_konteks.regions[i].akhir = 0;
        g_memori_konteks.regions[i].ukuran = 0;
        g_memori_konteks.regions[i].tipe = MEM_REGION_TYPE_TIDAK_ADA;
        g_memori_konteks.regions[i].flags = 0;
        g_memori_konteks.regions[i].tersedia = SALAH;
        g_memori_konteks.regions[i].reserved = SALAH;
    }

    g_memori_konteks.region_count = 0;

    return STATUS_BERHASIL;
}

/*
 * memori_init_ram - Inisialisasi RAM controller
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
static status_t memori_init_ram(void)
{
    status_t hasil;
    ram_controller_t *ctrl;

    ctrl = &g_memori_konteks.ram_ctrl;

    /* Reset controller */
    kernel_memset(ctrl, 0, sizeof(ram_controller_t));

    /* Set magic */
    ctrl->magic = RAM_CONTROLLER_MAGIC;

    /* Inisialisasi RAM */
    hasil = ram_init();
    if (hasil != STATUS_BERHASIL && hasil != STATUS_SUDAH_ADA) {
        memori_set_error("Gagal inisialisasi RAM");
        return hasil;
    }

    /* Copy info dari RAM controller global */
    hasil = ram_info(ctrl);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    /* Copy regions ke konteks */
    kernel_memcpy(g_memori_konteks.regions, ctrl->regions,
                  sizeof(mem_region_t) * ctrl->region_count);
    g_memori_konteks.region_count = ctrl->region_count;

    /* Set statistik */
    g_memori_konteks.total_memory = ctrl->total_size;
    g_memori_konteks.available_memory = ctrl->available_size;
    g_memori_konteks.used_memory = ctrl->used_size;

    return STATUS_BERHASIL;
}

/*
 * memori_init_cache - Inisialisasi cache controller
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
static status_t memori_init_cache(void)
{
    status_t hasil;
    cache_controller_t *ctrl;

    ctrl = &g_memori_konteks.cache_ctrl;

    /* Reset controller */
    kernel_memset(ctrl, 0, sizeof(cache_controller_t));

    /* Set magic */
    ctrl->magic = CACHE_CTRL_MAGIC;

    /* Inisialisasi cache */
    hasil = cache_init();
    if (hasil != STATUS_BERHASIL && hasil != STATUS_SUDAH_ADA) {
        /* Cache init tidak kritis, lanjutkan */
        ctrl->enabled = SALAH;
    } else {
        ctrl->enabled = BENAR;
    }

    return STATUS_BERHASIL;
}

/*
 * memori_init_pci - Inisialisasi PCI controller
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
static status_t memori_init_pci(void)
{
    status_t hasil;
    pci_controller_t *ctrl;

    ctrl = &g_memori_konteks.pci_ctrl;

    /* Reset controller */
    kernel_memset(ctrl, 0, sizeof(pci_controller_t));

    /* Set magic */
    ctrl->magic = PCI_CTRL_MAGIC;

    /* Inisialisasi PCI */
    hasil = pci_init();
    if (hasil != STATUS_BERHASIL && hasil != STATUS_SUDAH_ADA) {
        memori_set_error("Gagal inisialisasi PCI");
        return hasil;
    }

    /* Scan PCI bus */
    hasil = pci_scan();
    if (hasil < 0) {
        /* PCI scan tidak kritis, lanjutkan */
        ctrl->initialized = BENAR;
    } else {
        ctrl->devices_found = (tak_bertanda32_t)hasil;
        ctrl->initialized = BENAR;
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * memori_init - Inisialisasi subsistem memori
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t memori_init(void)
{
    status_t hasil;

    if (g_memori_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }

    /* Reset konteks */
    kernel_memset(&g_memori_konteks, 0, sizeof(memori_konteks_t));

    /* Set magic */
    g_memori_konteks.magic = MEMORI_MAGIC;

    /* Inisialisasi regions */
    hasil = memori_init_regions();
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    /* Inisialisasi RAM */
    hasil = memori_init_ram();
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    /* Inisialisasi cache */
    hasil = memori_init_cache();
    if (hasil != STATUS_BERHASIL) {
        /* Cache tidak kritis */
    }

    /* Inisialisasi PCI */
    hasil = memori_init_pci();
    if (hasil != STATUS_BERHASIL) {
        /* PCI tidak kritis */
    }

    /* Finalisasi */
    g_memori_konteks.initialized = BENAR;
    g_memori_diinisialisasi = BENAR;

    return STATUS_BERHASIL;
}

/*
 * memori_shutdown - Matikan subsistem memori
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t memori_shutdown(void)
{
    if (!g_memori_diinisialisasi) {
        return STATUS_BERHASIL;
    }

    /* Flush cache */
    cache_flush(0);

    /* Nonaktifkan PCI devices */
    /* TODO: Iterate dan disable semua device */

    /* Reset konteks */
    g_memori_konteks.magic = 0;
    g_memori_konteks.initialized = SALAH;
    g_memori_diinisialisasi = SALAH;

    return STATUS_BERHASIL;
}

/*
 * memori_konteks_dapatkan - Dapatkan konteks memori
 *
 * Return: Pointer ke konteks atau NULL
 */
memori_konteks_t *memori_konteks_dapatkan(void)
{
    if (!g_memori_diinisialisasi) {
        return NULL;
    }

    if (g_memori_konteks.magic != MEMORI_MAGIC) {
        return NULL;
    }

    return &g_memori_konteks;
}

/*
 * ===========================================================================
 * FUNGSI MEMORY REGION
 * ===========================================================================
 */

/*
 * mem_region_tambah - Tambah memory region
 *
 * Parameter:
 *   mulai - Alamat awal
 *   akhir - Alamat akhir
 *   tipe - Tipe region
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t mem_region_tambah(alamat_fisik_t mulai, alamat_fisik_t akhir,
                           tak_bertanda32_t tipe)
{
    mem_region_t *region;

    if (!g_memori_diinisialisasi) {
        return STATUS_PARAM_INVALID;
    }

    if (g_memori_konteks.region_count >= MEMORI_REGION_MAKS) {
        return STATUS_PENUH;
    }

    if (mulai >= akhir) {
        return STATUS_PARAM_INVALID;
    }

    region = &g_memori_konteks.regions[g_memori_konteks.region_count];

    region->magic = MEMORI_MAGIC;
    region->id = g_memori_konteks.region_count;
    region->mulai = mulai;
    region->akhir = akhir;
    region->ukuran = (ukuran_t)(akhir - mulai + 1);
    region->tipe = tipe;
    region->flags = 0;

    if (tipe == MEM_REGION_TYPE_USABLE) {
        region->tersedia = BENAR;
        region->reserved = SALAH;
        g_memori_konteks.available_memory += region->ukuran;
    } else {
        region->tersedia = SALAH;
        region->reserved = BENAR;
    }

    g_memori_konteks.total_memory += region->ukuran;
    g_memori_konteks.region_count++;

    return STATUS_BERHASIL;
}

/*
 * mem_region_cari - Cari region berdasarkan alamat
 *
 * Parameter:
 *   addr - Alamat yang dicari
 *
 * Return: Pointer ke region atau NULL
 */
mem_region_t *mem_region_cari(alamat_fisik_t addr)
{
    tak_bertanda32_t i;
    mem_region_t *region;

    if (!g_memori_diinisialisasi) {
        return NULL;
    }

    for (i = 0; i < g_memori_konteks.region_count; i++) {
        region = &g_memori_konteks.regions[i];

        if (addr >= region->mulai && addr <= region->akhir) {
            return region;
        }
    }

    return NULL;
}

/*
 * mem_region_total - Dapatkan total memori
 *
 * Return: Total memori dalam byte
 */
ukuran_t mem_region_total(void)
{
    if (!g_memori_diinisialisasi) {
        return 0;
    }

    return g_memori_konteks.total_memory;
}

/*
 * mem_region_tersedia - Dapatkan memori tersedia
 *
 * Return: Memori tersedia dalam byte
 */
ukuran_t mem_region_tersedia(void)
{
    if (!g_memori_diinisialisasi) {
        return 0;
    }

    if (g_memori_konteks.available_memory > g_memori_konteks.used_memory) {
        return g_memori_konteks.available_memory - g_memori_konteks.used_memory;
    }

    return 0;
}

/*
 * ===========================================================================
 * FUNGSI UTILITAS
 * ===========================================================================
 */

/*
 * memori_nama_tipe - Dapatkan nama tipe region
 *
 * Parameter:
 *   tipe - Tipe region
 *
 * Return: String nama tipe
 */
const char *memori_nama_tipe(tak_bertanda32_t tipe)
{
    switch (tipe) {
    case MEM_REGION_TYPE_TIDAK_ADA:
        return "None";
    case MEM_REGION_TYPE_USABLE:
        return "Usable";
    case MEM_REGION_TYPE_RESERVED:
        return "Reserved";
    case MEM_REGION_TYPE_ACPI:
        return "ACPI";
    case MEM_REGION_TYPE_NVS:
        return "NVS";
    case MEM_REGION_TYPE_UNUSABLE:
        return "Unusable";
    case MEM_REGION_TYPE_DISABLED:
        return "Disabled";
    case MEM_REGION_TYPE_PMEM:
        return "Persistent";
    default:
        return "Unknown";
    }
}

/*
 * memori_cetak_info - Cetak informasi memori
 */
void memori_cetak_info(void)
{
    tak_bertanda32_t i;
    mem_region_t *region;

    if (!g_memori_diinisialisasi) {
        kernel_printf("Memori: Belum diinisialisasi\n");
        return;
    }

    kernel_printf("\n=== Memory Manager ===\n\n");
    kernel_printf("Total memory: %u MB\n",
                 g_memori_konteks.total_memory / (1024 * 1024));
    kernel_printf("Available: %u MB\n",
                 g_memori_konteks.available_memory / (1024 * 1024));
    kernel_printf("Used: %u MB\n",
                 g_memori_konteks.used_memory / (1024 * 1024));
    kernel_printf("Regions: %u\n\n", g_memori_konteks.region_count);

    kernel_printf("Memory Regions:\n");
    for (i = 0; i < g_memori_konteks.region_count; i++) {
        region = &g_memori_konteks.regions[i];
        kernel_printf("  [%u] 0x%08llX - 0x%08llX %s\n",
                     region->id,
                     (unsigned long long)region->mulai,
                     (unsigned long long)region->akhir,
                     memori_nama_tipe(region->tipe));
    }

    /* Cetak info RAM */
    ram_cetak_info();

    /* Cetak info cache */
    kernel_printf("\nCache Controller:\n");
    kernel_printf("  Enabled: %s\n",
                 g_memori_konteks.cache_ctrl.enabled ? "Ya" : "Tidak");
    kernel_printf("  Levels: %u\n",
                 g_memori_konteks.cache_ctrl.level_count);

    /* Cetak info PCI */
    pci_cetak_info();
}

/*
 * ===========================================================================
 * FUNGSI STATISTIK
 * ===========================================================================
 */

/*
 * memori_statistik_update - Update statistik memori
 *
 * Parameter:
 *   dialokasikan - TRUE jika alokasi, FALSE jika free
 *   ukuran - Ukuran memori
 */
void memori_statistik_update(bool_t dialokasikan, ukuran_t ukuran)
{
    if (!g_memori_diinisialisasi) {
        return;
    }

    if (dialokasikan) {
        g_memori_konteks.used_memory += ukuran;
    } else {
        if (g_memori_konteks.used_memory >= ukuran) {
            g_memori_konteks.used_memory -= ukuran;
        }
    }
}

/*
 * memori_penggunaan_persen - Dapatkan persentase penggunaan memori
 *
 * Return: Persentase penggunaan (0-100)
 */
tak_bertanda32_t memori_penggunaan_persen(void)
{
    ukuran_t persen;

    if (!g_memori_diinisialisasi) {
        return 0;
    }

    if (g_memori_konteks.total_memory == 0) {
        return 0;
    }

    persen = (g_memori_konteks.used_memory * 100) /
             g_memori_konteks.total_memory;

    if (persen > 100) {
        persen = 100;
    }

    return (tak_bertanda32_t)persen;
}

/*
 * memori_cukup - Cek apakah memori cukup
 *
 * Parameter:
 *   ukuran - Ukuran yang diperlukan
 *
 * Return: TRUE jika cukup
 */
bool_t memori_cukup(ukuran_t ukuran)
{
    ukuran_t tersedia;

    tersedia = mem_region_tersedia();

    return (tersedia >= ukuran);
}
