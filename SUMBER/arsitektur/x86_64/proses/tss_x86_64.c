/*
 * PIGURA OS - TSS x86_64
 * -----------------------
 * Implementasi Task State Segment untuk x86_64.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk manajemen
 * TSS (Task State Segment) pada arsitektur x86_64.
 *
 * Arsitektur: x86_64 (64-bit)
 * Versi: 1.0
 */

#include "../../inti/kernel.h"

/*
 * ============================================================================
 * STRUKTUR DATA
 * ============================================================================
 */

/* TSS x86_64 (64-bit) */
struct tss_x86_64 {
    tak_bertanda32_t reserv1;
    tak_bertanda64_t rsp[3];         /* RSP untuk ring 0, 1, 2 */
    tak_bertanda64_t reserv2;
    tak_bertanda64_t ist[7];         /* Interrupt Stack Table */
    tak_bertanda64_t reserv3;
    tak_bertanda16_t reserv4;
    tak_bertanda16_t iomap_base;     /* I/O permission map offset */
} __attribute__((packed));

/*
 * ============================================================================
 * VARIABEL GLOBAL
 * ============================================================================
 */

/* TSS utama */
static struct tss_x86_64 g_tss_utama __attribute__((aligned(16)));

/* Flag inisialisasi */
static bool_t g_tss_diinisialisasi = SALAH;

/*
 * ============================================================================
 * FUNGSI PUBLIC
 * ============================================================================
 */

/*
 * tss_x86_64_init
 * ---------------
 * Inisialisasi TSS.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t tss_x86_64_init(void)
{
    kernel_printf("[TSS-x86_64] Menginisialisasi TSS...\n");

    /* Clear TSS */
    kernel_memset(&g_tss_utama, 0, sizeof(struct tss_x86_64));

    /* Set IOMAP base (di luar TSS = tidak ada IOMAP) */
    g_tss_utama.iomap_base = sizeof(struct tss_x86_64);

    g_tss_diinisialisasi = BENAR;

    kernel_printf("[TSS-x86_64] TSS siap (ukuran: %u byte)\n",
                  (tak_bertanda32_t)sizeof(struct tss_x86_64));

    return STATUS_BERHASIL;
}

/*
 * tss_x86_64_set_rsp0
 * -------------------
 * Mengatur stack pointer untuk ring 0.
 *
 * Parameter:
 *   rsp0 - Stack pointer ring 0
 */
void tss_x86_64_set_rsp0(tak_bertanda64_t rsp0)
{
    g_tss_utama.rsp[0] = rsp0;
}

/*
 * tss_x86_64_set_rsp1
 * -------------------
 * Mengatur stack pointer untuk ring 1.
 *
 * Parameter:
 *   rsp1 - Stack pointer ring 1
 */
void tss_x86_64_set_rsp1(tak_bertanda64_t rsp1)
{
    g_tss_utama.rsp[1] = rsp1;
}

/*
 * tss_x86_64_set_rsp2
 * -------------------
 * Mengatur stack pointer untuk ring 2.
 *
 * Parameter:
 *   rsp2 - Stack pointer ring 2
 */
void tss_x86_64_set_rsp2(tak_bertanda64_t rsp2)
{
    g_tss_utama.rsp[2] = rsp2;
}

/*
 * tss_x86_64_set_ist
 * ------------------
 * Mengatur Interrupt Stack Table entry.
 *
 * Parameter:
 *   index - Index IST (1-7)
 *   ist   - Stack pointer
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t tss_x86_64_set_ist(tak_bertanda32_t index, tak_bertanda64_t ist)
{
    if (index < 1 || index > 7) {
        return STATUS_PARAM_INVALID;
    }

    g_tss_utama.ist[index - 1] = ist;

    return STATUS_BERHASIL;
}

/*
 * tss_x86_64_get_rsp0
 * -------------------
 * Mendapatkan stack pointer ring 0.
 *
 * Return:
 *   Stack pointer ring 0
 */
tak_bertanda64_t tss_x86_64_get_rsp0(void)
{
    return g_tss_utama.rsp[0];
}

/*
 * tss_x86_64_get_rsp1
 * -------------------
 * Mendapatkan stack pointer ring 1.
 *
 * Return:
 *   Stack pointer ring 1
 */
tak_bertanda64_t tss_x86_64_get_rsp1(void)
{
    return g_tss_utama.rsp[1];
}

/*
 * tss_x86_64_get_rsp2
 * -------------------
 * Mendapatkan stack pointer ring 2.
 *
 * Return:
 *   Stack pointer ring 2
 */
tak_bertanda64_t tss_x86_64_get_rsp2(void)
{
    return g_tss_utama.rsp[2];
}

/*
 * tss_x86_64_get_ist
 * ------------------
 * Mendapatkan IST entry.
 *
 * Parameter:
 *   index - Index IST (1-7)
 *
 * Return:
 *   Stack pointer IST
 */
tak_bertanda64_t tss_x86_64_get_ist(tak_bertanda32_t index)
{
    if (index < 1 || index > 7) {
        return 0;
    }

    return g_tss_utama.ist[index - 1];
}

/*
 * tss_x86_64_get_address
 * ----------------------
 * Mendapatkan alamat TSS.
 *
 * Return:
 *   Alamat TSS
 */
tak_bertanda64_t tss_x86_64_get_address(void)
{
    return (tak_bertanda64_t)&g_tss_utama;
}

/*
 * tss_x86_64_get_size
 * -------------------
 * Mendapatkan ukuran TSS.
 *
 * Return:
 *   Ukuran TSS dalam byte
 */
tak_bertanda32_t tss_x86_64_get_size(void)
{
    return (tak_bertanda32_t)sizeof(struct tss_x86_64);
}

/*
 * tss_x86_64_apakah_siap
 * ----------------------
 * Mengecek apakah TSS sudah diinisialisasi.
 *
 * Return:
 *   BENAR jika sudah, SALAH jika belum
 */
bool_t tss_x86_64_apakah_siap(void)
{
    return g_tss_diinisialisasi;
}
