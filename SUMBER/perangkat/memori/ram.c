/*
 * PIGURA OS - RAM.C
 * =================
 * Implementasi RAM controller untuk Pigura OS.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi untuk mengelola
 * RAM (Random Access Memory) controller dan deteksi memori.
 *
 * Kepatuhan standar:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Mendukung semua arsitektur (x86, x86_64, ARM, ARMv7, ARM64)
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "memori.h"

/*
 * ===========================================================================
 * KONSTANTA LOKAL
 * ===========================================================================
 */

/* Ukuran halaman standar */
#define RAM_HALAMAN_UKURAN     4096

/* Konstanta untuk deteksi memori */
#define RAM_TEST_PATTERN1      0xAAAAAAAA
#define RAM_TEST_PATTERN2      0x55555555
#define RAM_TEST_PATTERN3      0xFF00FF00
#define RAM_TEST_PATTERN4      0x00FF00FF

/* BIOS data area untuk deteksi memori (x86) */
#define RAM_BDA_BASE           0x400
#define RAM_BDA_MEMORY_SIZE    0x413

/* CMOS register untuk memori (x86) */
#define RAM_CMOS_PORT          0x70
#define RAM_CMOS_DATA          0x71
#define RAM_CMOS_BASE_LOW      0x15
#define RAM_CMOS_BASE_HIGH     0x16
#define RAM_CMOS_EXT_LOW       0x17
#define RAM_CMOS_EXT_HIGH      0x18
#define RAM_CMOS_HIGH_LOW      0x30
#define RAM_CMOS_HIGH_HIGH     0x31

/* Tipe DRAM */
#define RAM_TYPE_UNKNOWN       0
#define RAM_TYPE_FPM           1
#define RAM_TYPE_EDO           2
#define RAM_TYPE_SDRAM         3
#define RAM_TYPE_DDR           4
#define RAM_TYPE_DDR2          5
#define RAM_TYPE_DDR3          6
#define RAM_TYPE_DDR4          7
#define RAM_TYPE_DDR5          8
#define RAM_TYPE_LPDDR4        9
#define RAM_TYPE_LPDDR5        10

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

/* RAM controller global */
static ram_controller_t g_ram_controller;

/* Status inisialisasi */
static bool_t g_ram_initialized = SALAH;

/* Buffer untuk error message */
static char g_ram_error[256];

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * ram_set_error - Set pesan error
 *
 * Parameter:
 *   pesan - Pesan error
 */
static void ram_set_error(const char *pesan)
{
    ukuran_t i;

    if (pesan == NULL) {
        return;
    }

    i = 0;
    while (pesan[i] != '\0' && i < 255) {
        g_ram_error[i] = pesan[i];
        i++;
    }
    g_ram_error[i] = '\0';
}

/*
 * ram_cmos_read - Baca CMOS register
 *
 * Parameter:
 *   reg - Register number
 *
 * Return: Nilai yang dibaca
 */
static tak_bertanda8_t ram_cmos_read(tak_bertanda8_t reg)
{
    outb(RAM_CMOS_PORT, reg);
    return inb(RAM_CMOS_DATA);
}

/*
 * ram_detect_bios_memory - Deteksi memori dari BIOS (x86)
 *
 * Parameter:
 *   ctrl - Pointer ke controller
 */
static void ram_detect_bios_memory(ram_controller_t *ctrl)
{
    tak_bertanda16_t base_mem;
    tak_bertanda16_t ext_mem;
    tak_bertanda32_t high_mem;

    /* Baca base memory (dalam KB) */
    base_mem = ram_cmos_read(RAM_CMOS_BASE_LOW);
    base_mem |= (tak_bertanda16_t)(ram_cmos_read(RAM_CMOS_BASE_HIGH) << 8);

    /* Baca extended memory (dalam KB) */
    ext_mem = ram_cmos_read(RAM_CMOS_EXT_LOW);
    ext_mem |= (tak_bertanda16_t)(ram_cmos_read(RAM_CMOS_EXT_HIGH) << 8);

    /* Baca high memory (dalam KB, 16MB+) */
    high_mem = ram_cmos_read(RAM_CMOS_HIGH_LOW);
    high_mem |= (tak_bertanda32_t)(ram_cmos_read(RAM_CMOS_HIGH_HIGH) << 8);

    /* Hitung total memori */
    /* Base memory biasanya 640KB */
    ctrl->total_size = (ukuran_t)base_mem * 1024;

    /* Extended memory sampai 16MB */
    if (ext_mem > 0) {
        ctrl->total_size += (ukuran_t)ext_mem * 1024;
    }

    /* High memory 16MB+ */
    if (high_mem > 0) {
        ctrl->total_size += (ukuran_t)high_mem * 64 * 1024;
    }

    /* Minimum deteksi */
    if (ctrl->total_size < (16 * 1024 * 1024)) {
        ctrl->total_size = 16 * 1024 * 1024;
    }
}

/*
 * ram_test_region - Tes region memori
 *
 * Parameter:
 *   mulai - Alamat awal
 *   ukuran - Ukuran region
 *
 * Return: BENAR jika valid
 */
static bool_t ram_test_region(alamat_fisik_t mulai, ukuran_t ukuran)
{
    volatile tak_bertanda32_t *ptr;
    tak_bertanda32_t nilai_asli;
    tak_bertanda32_t i;
    ukuran_t count;

    /* Align ke 4-byte boundary */
    mulai = (mulai + 3) & ~0x03ULL;
    ukuran = ukuran & ~0x03UL;

    if (ukuran < 4) {
        return BENAR;
    }

    count = ukuran / 4;

    /* Test setiap word */
    for (i = 0; i < count; i++) {
        ptr = (volatile tak_bertanda32_t *)
              (mulai + (i * 4));

        /* Simpan nilai asli */
        nilai_asli = *ptr;

        /* Test pattern 1 */
        *ptr = RAM_TEST_PATTERN1;
        if (*ptr != RAM_TEST_PATTERN1) {
            *ptr = nilai_asli;
            return SALAH;
        }

        /* Test pattern 2 */
        *ptr = RAM_TEST_PATTERN2;
        if (*ptr != RAM_TEST_PATTERN2) {
            *ptr = nilai_asli;
            return SALAH;
        }

        /* Kembalikan nilai asli */
        *ptr = nilai_asli;
    }

    return BENAR;
}

/*
 * ram_detect_regions - Deteksi memory regions
 *
 * Parameter:
 *   ctrl - Pointer ke controller
 */
static void ram_detect_regions(ram_controller_t *ctrl)
{
    mem_region_t *region;
    tak_bertanda32_t i;

    /* Region 0: Conventional memory (0-640KB) */
    region = &ctrl->regions[0];
    region->magic = MEMORI_MAGIC;
    region->id = 0;
    region->mulai = 0x00000000;
    region->akhir = 0x0009FFFF;
    region->ukuran = 640 * 1024;
    region->tipe = MEM_REGION_TYPE_USABLE;
    region->tersedia = BENAR;
    region->reserved = SALAH;
    ctrl->region_count = 1;

    /* Region 1: BIOS area (640KB-1MB) */
    region = &ctrl->regions[1];
    region->magic = MEMORI_MAGIC;
    region->id = 1;
    region->mulai = 0x000A0000;
    region->akhir = 0x000FFFFF;
    region->ukuran = 384 * 1024;
    region->tipe = MEM_REGION_TYPE_RESERVED;
    region->tersedia = SALAH;
    region->reserved = BENAR;
    ctrl->region_count = 2;

    /* Region 2: Extended memory */
    if (ctrl->total_size > (1 * 1024 * 1024)) {
        region = &ctrl->regions[2];
        region->magic = MEMORI_MAGIC;
        region->id = 2;
        region->mulai = 0x00100000;
        region->akhir = ctrl->total_size - 1;
        region->ukuran = ctrl->total_size - (1 * 1024 * 1024);
        region->tipe = MEM_REGION_TYPE_USABLE;
        region->tersedia = BENAR;
        region->reserved = SALAH;
        ctrl->region_count = 3;
    }

    /* Hitung available size */
    ctrl->available_size = 0;
    for (i = 0; i < ctrl->region_count; i++) {
        if (ctrl->regions[i].tipe == MEM_REGION_TYPE_USABLE) {
            ctrl->available_size += ctrl->regions[i].ukuran;
        }
    }

    ctrl->used_size = 0;
}

/*
 * ram_detect_type - Deteksi tipe RAM
 *
 * Parameter:
 *   ctrl - Pointer ke controller
 */
static void ram_detect_type(ram_controller_t *ctrl)
{
    /* Default detect dari flags atau konfigurasi */
    ctrl->bus_width = 64;  /* Default 64-bit bus */
    ctrl->speed_mhz = 0;   /* Unknown */
    ctrl->cas_latency = 0;
    ctrl->ranks = 1;
    ctrl->tCL = 0;
    ctrl->tRCD = 0;
    ctrl->tRP = 0;
    ctrl->tRAS = 0;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * ram_init - Inisialisasi RAM controller
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ram_init(void)
{
    if (g_ram_initialized) {
        return STATUS_SUDAH_ADA;
    }

    /* Reset controller */
    kernel_memset(&g_ram_controller, 0, sizeof(ram_controller_t));

    /* Set magic */
    g_ram_controller.magic = RAM_CONTROLLER_MAGIC;

    /* Deteksi memori */
    ram_deteksi();

    g_ram_initialized = BENAR;
    g_ram_controller.initialized = BENAR;

    return STATUS_BERHASIL;
}

/*
 * ram_deteksi - Deteksi RAM
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ram_deteksi(void)
{
    ram_controller_t *ctrl;

    ctrl = &g_ram_controller;

    /* Deteksi dari BIOS (untuk x86) */
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || \
    defined(_M_X64)
    ram_detect_bios_memory(ctrl);
#else
    /* Untuk arsitektur lain, gunakan nilai default */
    ctrl->total_size = 128 * 1024 * 1024;  /* Default 128MB */
#endif

    /* Deteksi memory regions */
    ram_detect_regions(ctrl);

    /* Deteksi tipe RAM */
    ram_detect_type(ctrl);

    return STATUS_BERHASIL;
}

/*
 * ram_ukuran - Dapatkan ukuran RAM
 *
 * Return: Ukuran RAM dalam byte
 */
ukuran_t ram_ukuran(void)
{
    if (!g_ram_initialized) {
        return 0;
    }

    return g_ram_controller.total_size;
}

/*
 * ram_info - Dapatkan info RAM
 *
 * Parameter:
 *   ctrl - Buffer untuk controller info
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ram_info(ram_controller_t *ctrl)
{
    if (ctrl == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (!g_ram_initialized) {
        return STATUS_PARAM_INVALID;
    }

    kernel_memcpy(ctrl, &g_ram_controller, sizeof(ram_controller_t));

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI TAMBAHAN
 * ===========================================================================
 */

/*
 * ram_region_dapatkan - Dapatkan region berdasarkan indeks
 *
 * Parameter:
 *   indeks - Indeks region
 *
 * Return: Pointer ke region atau NULL
 */
mem_region_t *ram_region_dapatkan(tak_bertanda32_t indeks)
{
    if (!g_ram_initialized) {
        return NULL;
    }

    if (indeks >= g_ram_controller.region_count) {
        return NULL;
    }

    return &g_ram_controller.regions[indeks];
}

/*
 * ram_region_count - Dapatkan jumlah region
 *
 * Return: Jumlah region
 */
tak_bertanda32_t ram_region_count(void)
{
    if (!g_ram_initialized) {
        return 0;
    }

    return g_ram_controller.region_count;
}

/*
 * ram_available - Dapatkan ukuran RAM tersedia
 *
 * Return: Ukuran RAM tersedia dalam byte
 */
ukuran_t ram_available(void)
{
    if (!g_ram_initialized) {
        return 0;
    }

    return g_ram_controller.available_size - g_ram_controller.used_size;
}

/*
 * ram_used - Dapatkan ukuran RAM terpakai
 *
 * Return: Ukuran RAM terpakai dalam byte
 */
ukuran_t ram_used(void)
{
    if (!g_ram_initialized) {
        return 0;
    }

    return g_ram_controller.used_size;
}

/*
 * ram_allocate - Alokasikan memori dari region
 *
 * Parameter:
 *   ukuran - Ukuran yang dialokasikan
 *
 * Return: Alamat fisik atau ALAMAT_FISIK_INVALID
 */
alamat_fisik_t ram_allocate(ukuran_t ukuran)
{
    tak_bertanda32_t i;
    mem_region_t *region;
    alamat_fisik_t addr;

    if (!g_ram_initialized) {
        return ALAMAT_FISIK_INVALID;
    }

    /* Align ukuran ke halaman */
    ukuran = RATAKAN_ATAS(ukuran, RAM_HALAMAN_UKURAN);

    /* Cari region dengan ruang cukup */
    for (i = 0; i < g_ram_controller.region_count; i++) {
        region = &g_ram_controller.regions[i];

        if (region->tipe != MEM_REGION_TYPE_USABLE) {
            continue;
        }

        if (!region->tersedia) {
            continue;
        }

        ukuran_t free_size = region->ukuran -
                            (ukuran_t)(region->akhir - region->mulai);

        if (free_size >= ukuran) {
            /* Alokasikan dari awal region */
            addr = region->mulai;
            region->mulai += ukuran;
            region->ukuran -= ukuran;

            g_ram_controller.used_size += ukuran;

            return addr;
        }
    }

    return ALAMAT_FISIK_INVALID;
}

/*
 * ram_free - Bebaskan memori
 *
 * Parameter:
 *   addr - Alamat fisik
 *   ukuran - Ukuran yang dibebaskan
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ram_free(alamat_fisik_t addr, ukuran_t ukuran)
{
    tak_bertanda32_t i;
    mem_region_t *region;

    if (!g_ram_initialized) {
        return STATUS_PARAM_INVALID;
    }

    /* Align ukuran ke halaman */
    ukuran = RATAKAN_ATAS(ukuran, RAM_HALAMAN_UKURAN);

    /* Cari region yang berisi alamat */
    for (i = 0; i < g_ram_controller.region_count; i++) {
        region = &g_ram_controller.regions[i];

        if (addr >= region->mulai && addr < region->akhir) {
            /* Sederhana: update statistik */
            if (g_ram_controller.used_size >= ukuran) {
                g_ram_controller.used_size -= ukuran;
            }

            return STATUS_BERHASIL;
        }
    }

    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * ram_test - Tes integritas RAM
 *
 * Parameter:
 *   addr - Alamat awal
 *   ukuran - Ukuran yang dites
 *
 * Return: STATUS_BERHASIL jika OK
 */
status_t ram_test(alamat_fisik_t addr, ukuran_t ukuran)
{
    if (!g_ram_initialized) {
        return STATUS_PARAM_INVALID;
    }

    /* Tes region memori */
    if (ram_test_region(addr, ukuran)) {
        return STATUS_BERHASIL;
    }

    ram_set_error("RAM test gagal");
    return STATUS_MEMORI_CORRUPT;
}

/*
 * ram_cetak_info - Cetak informasi RAM
 */
void ram_cetak_info(void)
{
    tak_bertanda32_t i;
    mem_region_t *region;

    if (!g_ram_initialized) {
        kernel_printf("RAM: Belum diinisialisasi\n");
        return;
    }

    kernel_printf("\n=== RAM Controller ===\n\n");
    kernel_printf("Total size: %u MB\n",
                 g_ram_controller.total_size / (1024 * 1024));
    kernel_printf("Available: %u MB\n",
                 g_ram_controller.available_size / (1024 * 1024));
    kernel_printf("Used: %u MB\n",
                 g_ram_controller.used_size / (1024 * 1024));
    kernel_printf("Bus width: %u bits\n", g_ram_controller.bus_width);
    kernel_printf("Regions: %u\n\n", g_ram_controller.region_count);

    for (i = 0; i < g_ram_controller.region_count; i++) {
        region = &g_ram_controller.regions[i];
        kernel_printf("Region %u: 0x%08llX - 0x%08llX (%u KB) %s\n",
                     region->id,
                     (unsigned long long)region->mulai,
                     (unsigned long long)region->akhir,
                     region->ukuran / 1024,
                     memori_nama_tipe(region->tipe));
    }
}
