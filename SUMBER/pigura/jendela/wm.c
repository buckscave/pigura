/*
 * PIGURA OS - WM.C
 * =================
 * Window Manager Pigura OS.
 *
 * Orkestrasi global: fokus, render, dan manajemen
 * siklus hidup semua jendela dalam sistem.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Seluruh nama dalam Bahasa Indonesia
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../../inti/kernel.h"
#include "../framebuffer/framebuffer.h"
#include "jendela.h"

/*
 * ===========================================================================
 * VARIABEL GLOBAL WINDOW MANAGER
 * ===========================================================================
 */

static permukaan_t *g_desktop = NULL;
static jendela_t *g_fokus = NULL;
static bool_t g_wm_diinit = SALAH;

/*
 * ===========================================================================
 * WM - INISIALISASI
 * ===========================================================================
 */

status_t wm_init(void)
{
    if (g_wm_diinit) {
        return STATUS_SUDAH_ADA;
    }
    g_desktop = NULL;
    g_fokus = NULL;
    g_wm_diinit = BENAR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * WM - SHUTDOWN
 * ===========================================================================
 */

void wm_shutdown(void)
{
    g_desktop = NULL;
    g_fokus = NULL;
    g_wm_diinit = SALAH;
}

/*
 * ===========================================================================
 * WM - DESKTOP
 * ===========================================================================
 */

status_t wm_set_desktop(permukaan_t *permukaan)
{
    if (permukaan == NULL) {
        return STATUS_PARAM_NULL;
    }
    g_desktop = permukaan;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * WM - FOKUS
 * ===========================================================================
 */

jendela_t *wm_dapat_fokus(void)
{
    if (!g_wm_diinit) {
        return NULL;
    }
    return g_fokus;
}

status_t wm_set_fokus(jendela_t *j)
{
    if (!g_wm_diinit) {
        return STATUS_TIDAK_DUKUNG;
    }
    if (j == NULL) {
        g_fokus = NULL;
        return STATUS_BERHASIL;
    }
    if (!jendela_valid(j)) {
        return STATUS_PARAM_INVALID;
    }
    g_fokus = j;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * WM - CARI JENDELA
 * ===========================================================================
 */

jendela_t *wm_cari_id(tak_bertanda32_t id)
{
    tak_bertanda32_t i;
    tak_bertanda32_t jumlah;

    if (!g_wm_diinit) {
        return NULL;
    }

    jumlah = JENDELA_MAKS;
    for (i = 0; i < jumlah; i++) {
        struct jendela *w;
        /* Akses langsung ke array global */
        extern struct jendela g_jendela_list[];
        w = &g_jendela_list[i];
        if (w->magic == JENDELA_MAGIC && w->id == id) {
            return (jendela_t *)w;
        }
    }

    return NULL;
}

tak_bertanda32_t wm_dapat_jumlah_aktif(void)
{
    if (!g_wm_diinit) {
        return 0;
    }
    /* Akses counter global */
    extern tak_bertanda32_t g_jendela_jumlah;
    return g_jendela_jumlah;
}

/*
 * ===========================================================================
 * WM - RENDER
 * ===========================================================================
 *
 * Render semua jendela visible ke permukaan desktop.
 * Jendela digambar dari bawah ke atas (z-order terbalik).
 */

status_t wm_render_semua(void)
{
    tak_bertanda32_t jumlah, i;

    if (!g_wm_diinit) {
        return STATUS_TIDAK_DUKUNG;
    }
    if (g_desktop == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Isi desktop dengan warna latar */
    permukaan_isi(g_desktop, JENDELA_WARNA_DESKTOP);

    jumlah = JENDELA_MAKS;
    for (i = 0; i < jumlah; i++) {
        struct jendela *w;
        extern struct jendela g_jendela_list[];
        w = &g_jendela_list[i];

        if (w->magic != JENDELA_MAGIC) {
            continue;
        }
        if (w->status == JENDELA_STATUS_INVISIBLE) {
            continue;
        }

        /* Gambar latar jendela */
        permukaan_isi_kotak(g_desktop,
            w->x, w->y, w->lebar, w->tinggi,
            w->warna_latar);

        /* Gambar dekorasi jika aktif */
        if ((w->flags & JENDELA_FLAG_DEKORASI) != 0) {
            dekorasi_gambar((jendela_t *)w, g_desktop);
        }
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * WM - LANGKAH
 * ===========================================================================
 *
 * Proses satu langkah window manager:
 * render ulang desktop jika ada perubahan.
 */

status_t wm_langkah(void)
{
    if (!g_wm_diinit) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Proses peristiwa dari antrian */
    peristiwa_proses_semua();

    return wm_render_semua();
}

/*
 * ===========================================================================
 * WM - PERISTIWA
 * ===========================================================================
 */

status_t wm_kirim_peristiwa(const peristiwa_jendela_t *evt)
{
    if (!g_wm_diinit || evt == NULL) {
        return STATUS_PARAM_NULL;
    }
    return peristiwa_push(evt);
}
