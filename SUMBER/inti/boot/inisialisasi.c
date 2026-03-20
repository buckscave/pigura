/*
 * PIGURA OS - INISIALISASI.C
 * --------------------------
 * Implementasi fungsi inisialisasi kernel.
 *
 * Berkas ini berisi fungsi-fungsi untuk menginisialisasi berbagai
 * subsistem kernel dalam urutan yang benar.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../kernel.h"
#include "../hal/hal.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* Urutan inisialisasi subsistem */
#define INIT_STAGE_EARLY        0  /* Paling awal, sebelum console */
#define INIT_STAGE_CORE         1  /* Core subsistem */
#define INIT_STAGE_DRIVERS      2  /* Driver */
#define INIT_STAGE_SERVICES     3  /* Layanan kernel */
#define INIT_STAGE_LATE         4  /* Paling akhir */

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* Struktur callback inisialisasi */
typedef struct {
    const char *nama;               /* Nama subsistem */
    status_t (*init)(void);         /* Fungsi inisialisasi */
    tak_bertanda8_t stage;          /* Stage inisialisasi */
    bool_t required;                /* Wajib untuk operasi */
    bool_t initialized;             /* Status inisialisasi */
} init_entry_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Daftar subsistem yang diinisialisasi */
static init_entry_t init_table[] = {
    /* Stage CORE - subsistem inti */
    /* Akan ditambahkan sesuai implementasi subsistem */
};

/* Jumlah entry dalam tabel */
static const tak_bertanda32_t init_table_count =
    sizeof(init_table) / sizeof(init_entry_t);

/* Status inisialisasi */
static bool_t inisialisasi_selesai = SALAH;
static tak_bertanda32_t subsistem_diinisialisasi = 0;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * init_run_stage
 * --------------
 * Jalankan inisialisasi untuk stage tertentu.
 *
 * Parameter:
 *   stage - Stage inisialisasi
 *
 * Return: Status operasi
 */
static status_t init_run_stage(tak_bertanda8_t stage)
{
    tak_bertanda32_t i;
    status_t status;
    init_entry_t *entry;

    for (i = 0; i < init_table_count; i++) {
        entry = &init_table[i];

        if (entry->stage != stage || entry->initialized) {
            continue;
        }

        kernel_printf("[INIT] Inisialisasi %s...\n", entry->nama);

        if (entry->init != NULL) {
            status = entry->init();

            if (status != STATUS_BERHASIL) {
                if (entry->required) {
                    kernel_printf("[INIT] GAGAL: %s (wajib), kode: %d\n",
                                  entry->nama, status);
                    return status;
                } else {
                    kernel_printf("[INIT] GAGAL: %s (opsional), kode: %d\n",
                                  entry->nama, status);
                    /* Lanjutkan meski gagal */
                }
            } else {
                entry->initialized = BENAR;
                subsistem_diinisialisasi++;
                kernel_printf("[INIT] Berhasil: %s\n", entry->nama);
            }
        } else {
            entry->initialized = BENAR;
        }
    }

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * kernel_init_early
 * -----------------
 * Inisialisasi awal kernel.
 * Dipanggil sangat awal, sebelum console siap.
 *
 * Return: Status operasi
 */
status_t kernel_init_early(void)
{
    /* Inisialisasi variabel global */
    g_kernel_diinisialisasi = 0;
    g_uptime_ticks = 0;
    g_boot_time = 0;
    g_proses_sekarang = NULL;
    g_cpu_count = 1;
    g_cpu_id = 0;

    /* Clear informasi sistem */
    kernel_memset(&g_info_sistem, 0, sizeof(g_info_sistem));

    /* Run early stage */
    return init_run_stage(INIT_STAGE_EARLY);
}

/*
 * kernel_init_core
 * ----------------
 * Inisialisasi core subsistem.
 *
 * Return: Status operasi
 */
status_t kernel_init_core(void)
{
    return init_run_stage(INIT_STAGE_CORE);
}

/*
 * kernel_init_drivers
 * -------------------
 * Inisialisasi driver.
 *
 * Return: Status operasi
 */
status_t kernel_init_drivers(void)
{
    return init_run_stage(INIT_STAGE_DRIVERS);
}

/*
 * kernel_init_services
 * --------------------
 * Inisialisasi layanan kernel.
 *
 * Return: Status operasi
 */
status_t kernel_init_services(void)
{
    return init_run_stage(INIT_STAGE_SERVICES);
}

/*
 * kernel_init_late
 * ----------------
 * Inisialisasi akhir.
 *
 * Return: Status operasi
 */
status_t kernel_init_late(void)
{
    status_t status;

    status = init_run_stage(INIT_STAGE_LATE);

    if (status == STATUS_BERHASIL) {
        inisialisasi_selesai = BENAR;
    }

    return status;
}

/*
 * kernel_register_init
 * --------------------
 * Registrasi subsistem untuk inisialisasi.
 *
 * Parameter:
 *   nama     - Nama subsistem
 *   init     - Fungsi inisialisasi
 *   stage    - Stage inisialisasi
 *   required - Apakah wajib
 *
 * Return: Status operasi
 */
status_t kernel_register_init(const char *nama,
                              status_t (*init)(void),
                              tak_bertanda8_t stage,
                              bool_t required)
{
    /* Untuk sekarang, implementasi sederhana */
    /* Nanti bisa dibuat lebih dinamis dengan linked list */
    (void)nama;
    (void)init;
    (void)stage;
    (void)required;

    return STATUS_TIDAK_DUKUNG;
}

/*
 * kernel_get_init_status
 * ----------------------
 * Dapatkan status inisialisasi.
 *
 * Return: BENAR jika sudah selesai
 */
bool_t kernel_get_init_status(void)
{
    return inisialisasi_selesai;
}

/*
 * kernel_get_initialized_count
 * ----------------------------
 * Dapatkan jumlah subsistem yang diinisialisasi.
 *
 * Return: Jumlah subsistem
 */
tak_bertanda32_t kernel_get_initialized_count(void)
{
    return subsistem_diinisialisasi;
}

/*
 * kernel_print_init_status
 * ------------------------
 * Print status inisialisasi semua subsistem.
 */
void kernel_print_init_status(void)
{
    tak_bertanda32_t i;
    init_entry_t *entry;

    kernel_printf("\n[KERNEL] Status Inisialisasi:\n");
    kernel_printf("========================================\n");

    for (i = 0; i < init_table_count; i++) {
        entry = &init_table[i];

        kernel_printf("  %-20s: ", entry->nama);

        if (entry->initialized) {
            kernel_set_color(VGA_HIJAU, VGA_HITAM);
            kernel_printf("[OK]");
        } else {
            kernel_set_color(VGA_MERAH, VGA_HITAM);
            kernel_printf("[GAGAL]");
        }

        kernel_set_color(VGA_ABU_ABU, VGA_HITAM);

        if (entry->required) {
            kernel_printf(" (wajib)");
        } else {
            kernel_printf(" (opsional)");
        }

        kernel_printf("\n");
    }

    kernel_printf("========================================\n");
    kernel_printf("Total: %lu/%lu subsistem diinisialisasi\n",
                  subsistem_diinisialisasi, init_table_count);
    kernel_printf("\n");
}
