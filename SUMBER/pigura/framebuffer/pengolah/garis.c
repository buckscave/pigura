/*
 * PIGURA OS - GARIS.C
 * ====================
 * Operasi garis untuk pengolah grafis Pigura OS.
 *
 * Mendukung garis dengan ketebalan melalui drawing
 * beberapa garis sejajar. Routing ke backend CPU,
 * GPU, atau Hybrid berdasarkan mode konteks.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Seluruh nama dalam Bahasa Indonesia
 * - Mendukung: x86, x86_64, ARM, ARMv7, ARM64
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../../inti/kernel.h"
#include "pengolah.h"

/*
 * ===========================================================================
 * DEKLARASI FUNGSI BACKEND
 * ===========================================================================
 */

extern status_t cpu_garis(struct p_konteks *ktx,
                          tanda32_t x0, tanda32_t y0,
                          tanda32_t x1, tanda32_t y1,
                          tak_bertanda32_t warna);
extern status_t gpu_garis(struct p_konteks *ktx,
                          tanda32_t x0, tanda32_t y0,
                          tanda32_t x1, tanda32_t y1,
                          tak_bertanda32_t warna);
extern status_t hybrid_garis(struct p_konteks *ktx,
                             tanda32_t x0, tanda32_t y0,
                             tanda32_t x1, tanda32_t y1,
                             tak_bertanda32_t warna);

/*
 * ===========================================================================
 * FUNGSI INTERNAL - ROUTING
 * ===========================================================================
 */

static status_t garis_route(struct p_konteks *ktx,
                             tanda32_t x0, tanda32_t y0,
                             tanda32_t x1, tanda32_t y1,
                             tak_bertanda32_t warna)
{
    switch (ktx->mode) {
    case PENGOLAH_MODE_GPU:
        return gpu_garis(ktx, x0, y0, x1, y1, warna);
    case PENGOLAH_MODE_HYBRID:
        return hybrid_garis(ktx, x0, y0, x1, y1, warna);
    case PENGOLAH_MODE_CPU:
    default:
        return cpu_garis(ktx, x0, y0, x1, y1, warna);
    }
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - GARIS TEBAL
 * ===========================================================================
 */

/*
 * garis_tebal
 * ----------
 * Menggambar garis dengan ketebalan > 1 piksel.
 * Dilakukan dengan menggambar beberapa garis
 * Bresenham sejajar dengan offset.
 */
static status_t garis_tebal(struct p_konteks *ktx,
                             tanda32_t x0, tanda32_t y0,
                             tanda32_t x1, tanda32_t y1,
                             tak_bertanda32_t warna,
                             tak_bertanda8_t ketebalan)
{
    tanda32_t dx, dy, offset;
    tak_bertanda32_t i;

    if (ketebalan <= 1) {
        return garis_route(ktx, x0, y0, x1, y1, warna);
    }

    dx = (x0 > x1) ? (x0 - x1) : (x1 - x0);
    dy = (y0 > y1) ? (y0 - y1) : (y1 - y0);

    offset = -(tanda32_t)(ketebalan / 2);

    for (i = 0; i < ketebalan; i++) {
        tanda32_t ox = 0, oy = 0;

        if (dy >= dx) {
            /* Garis lebih vertikal: offset horizontal */
            ox = offset + (tanda32_t)i;
        } else {
            /* Garis lebih horizontal: offset vertikal */
            oy = offset + (tanda32_t)i;
        }

        garis_route(ktx, x0 + ox, y0 + oy,
                    x1 + ox, y1 + oy, warna);
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * pengolah_garis
 * --------------
 * Menggambar garis dari (x0,y0) ke (x1,y1) menggunakan
 * warna dan ketebalan aktif dari konteks.
 *
 * Parameter:
 *   ktx - Pointer ke konteks pengolah
 *   x0, y0 - Titik awal garis
 *   x1, y1 - Titik akhir garis
 *
 * Return:
 *   STATUS_BERHASIL atau kode error
 */
status_t pengolah_garis(struct p_konteks *ktx,
                        tanda32_t x0, tanda32_t y0,
                        tanda32_t x1, tanda32_t y1)
{
    tak_bertanda32_t warna;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    warna = pengolah_rgba_ke_piksel(
        ktx->konfig.warna.r,
        ktx->konfig.warna.g,
        ktx->konfig.warna.b,
        ktx->konfig.warna.a);

    return garis_tebal(ktx, x0, y0, x1, y1, warna,
                       ktx->konfig.ketebalan);
}

/*
 * pengolah_garis_warna
 * --------------------
 * Menggambar garis dengan warna spesifik.
 * Menggunakan ketebalan dari konteks.
 *
 * Parameter:
 *   ktx   - Pointer ke konteks pengolah
 *   x0, y0 - Titik awal
 *   x1, y1 - Titik akhir
 *   warna - Nilai piksel 32-bit
 *
 * Return:
 *   STATUS_BERHASIL atau kode error
 */
status_t pengolah_garis_warna(struct p_konteks *ktx,
                              tanda32_t x0, tanda32_t y0,
                              tanda32_t x1, tanda32_t y1,
                              tak_bertanda32_t warna)
{
    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    return garis_tebal(ktx, x0, y0, x1, y1, warna,
                       ktx->konfig.ketebalan);
}
