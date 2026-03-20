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

#include "kernel.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

#define INIT_STAGE_EARLY        0
#define INIT_STAGE_CORE         1
#define INIT_STAGE_DRIVERS      2
#define INIT_STAGE_SERVICES     3
#define INIT_STAGE_LATE         4

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

typedef struct {
    const char *nama;
    status_t (*init)(void);
    tak_bertanda8_t stage;
    bool_t required;
    bool_t initialized;
} init_entry_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

static init_entry_t init_table[16];
static tak_bertanda32_t init_table_count = 0;

static bool_t inisialisasi_selesai = SALAH;
static tak_bertanda32_t subsistem_diinisialisasi = 0;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
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

status_t kernel_init_early(void)
{
    g_kernel_diinisialisasi = 0;
    g_uptime_ticks = 0;
    g_boot_time = 0;
    g_proses_sekarang = NULL;
    g_cpu_count = 1;
    g_cpu_id = 0;

    kernel_memset(&g_info_sistem, 0, sizeof(g_info_sistem));

    return init_run_stage(INIT_STAGE_EARLY);
}

status_t kernel_init_core(void)
{
    return init_run_stage(INIT_STAGE_CORE);
}

status_t kernel_init_drivers(void)
{
    return init_run_stage(INIT_STAGE_DRIVERS);
}

status_t kernel_init_services(void)
{
    return init_run_stage(INIT_STAGE_SERVICES);
}

status_t kernel_init_late(void)
{
    status_t status;

    status = init_run_stage(INIT_STAGE_LATE);

    if (status == STATUS_BERHASIL) {
        inisialisasi_selesai = BENAR;
    }

    return status;
}

status_t kernel_register_init(const char *nama,
                              status_t (*init)(void),
                              tak_bertanda8_t stage,
                              bool_t required)
{
    if (init_table_count >= 16) {
        return STATUS_BUSY;
    }

    if (nama == NULL) {
        return STATUS_PARAM_INVALID;
    }

    init_table[init_table_count].nama = nama;
    init_table[init_table_count].init = init;
    init_table[init_table_count].stage = stage;
    init_table[init_table_count].required = required;
    init_table[init_table_count].initialized = SALAH;

    init_table_count++;

    return STATUS_BERHASIL;
}

bool_t kernel_get_init_status(void)
{
    return inisialisasi_selesai;
}

tak_bertanda32_t kernel_get_initialized_count(void)
{
    return subsistem_diinisialisasi;
}

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
                  (tak_bertanda64_t)subsistem_diinisialisasi,
                  (tak_bertanda64_t)init_table_count);
    kernel_printf("\n");
}
