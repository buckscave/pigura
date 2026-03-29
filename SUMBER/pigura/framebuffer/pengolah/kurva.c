/*
 * PIGURA OS - KURVA.C
 * ====================
 * Operasi kurva Bezier untuk pengolah grafis Pigura OS.
 *
 * Mendukung kurva Bezier kuadratik (3 titik kontrol)
 * dan kubik (4 titik kontrol).
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

extern status_t cpu_kurva_bezier2(struct p_konteks *ktx,
                                  tanda32_t x0, tanda32_t y0,
                                  tanda32_t x1, tanda32_t y1,
                                  tanda32_t x2, tanda32_t y2,
                                  tak_bertanda32_t warna);
extern status_t cpu_kurva_bezier3(struct p_konteks *ktx,
                                  tanda32_t x0, tanda32_t y0,
                                  tanda32_t x1, tanda32_t y1,
                                  tanda32_t x2, tanda32_t y2,
                                  tanda32_t x3, tanda32_t y3,
                                  tak_bertanda32_t warna);
extern status_t hybrid_kurva_bezier2(struct p_konteks *ktx,
                                      tanda32_t x0, tanda32_t y0,
                                      tanda32_t x1, tanda32_t y1,
                                      tanda32_t x2, tanda32_t y2,
                                      tak_bertanda32_t warna);
extern status_t hybrid_kurva_bezier3(struct p_konteks *ktx,
                                      tanda32_t x0, tanda32_t y0,
                                      tanda32_t x1, tanda32_t y1,
                                      tanda32_t x2, tanda32_t y2,
                                      tanda32_t x3, tanda32_t y3,
                                      tak_bertanda32_t warna);

/*
 * ===========================================================================
 * FUNGSI INTERNAL - ROUTING
 * ===========================================================================
 */

static status_t kurva_bezier2_route(struct p_konteks *ktx,
                                     tanda32_t x0, tanda32_t y0,
                                     tanda32_t x1, tanda32_t y1,
                                     tanda32_t x2, tanda32_t y2,
                                     tak_bertanda32_t warna)
{
    switch (ktx->mode) {
    case PENGOLAH_MODE_GPU:
        return cpu_kurva_bezier2(ktx, x0, y0, x1, y1,
                                 x2, y2, warna);
    case PENGOLAH_MODE_HYBRID:
        return hybrid_kurva_bezier2(ktx, x0, y0, x1, y1,
                                    x2, y2, warna);
    case PENGOLAH_MODE_CPU:
    default:
        return cpu_kurva_bezier2(ktx, x0, y0, x1, y1,
                                 x2, y2, warna);
    }
}

static status_t kurva_bezier3_route(struct p_konteks *ktx,
                                     tanda32_t x0, tanda32_t y0,
                                     tanda32_t x1, tanda32_t y1,
                                     tanda32_t x2, tanda32_t y2,
                                     tanda32_t x3, tanda32_t y3,
                                     tak_bertanda32_t warna)
{
    switch (ktx->mode) {
    case PENGOLAH_MODE_GPU:
        return cpu_kurva_bezier3(ktx, x0, y0, x1, y1,
                                 x2, y2, x3, y3, warna);
    case PENGOLAH_MODE_HYBRID:
        return hybrid_kurva_bezier3(ktx, x0, y0, x1, y1,
                                    x2, y2, x3, y3, warna);
    case PENGOLAH_MODE_CPU:
    default:
        return cpu_kurva_bezier3(ktx, x0, y0, x1, y1,
                                 x2, y2, x3, y3, warna);
    }
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * pengolah_kurva_bezier2
 * ---------------------
 * Menggambar kurva Bezier kuadratik.
 * Memiliki 3 titik kontrol: awal, kontrol, akhir.
 *
 * Parameter:
 *   ktx - Pointer ke konteks pengolah
 *   x0, y0 - Titik awal kurva
 *   x1, y1 - Titik kontrol (menentukan kelengkungan)
 *   x2, y2 - Titik akhir kurva
 *
 * Return:
 *   STATUS_BERHASIL atau kode error
 */
status_t pengolah_kurva_bezier2(struct p_konteks *ktx,
                                tanda32_t x0, tanda32_t y0,
                                tanda32_t x1, tanda32_t y1,
                                tanda32_t x2, tanda32_t y2)
{
    tak_bertanda32_t warna;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    warna = pengolah_rgba_ke_piksel(
        ktx->konfig.warna.r, ktx->konfig.warna.g,
        ktx->konfig.warna.b, ktx->konfig.warna.a);

    return kurva_bezier2_route(ktx, x0, y0, x1, y1,
                               x2, y2, warna);
}

/*
 * pengolah_kurva_bezier3
 * ---------------------
 * Menggambar kurva Bezier kubik.
 * Memiliki 4 titik kontrol: awal, kontrol1, kontrol2, akhir.
 * Menghasilkan kurva yang lebih halus dibanding kuadratik.
 *
 * Parameter:
 *   ktx - Pointer ke konteks pengolah
 *   x0, y0 - Titik awal kurva
 *   x1, y1 - Titik kontrol pertama
 *   x2, y2 - Titik kontrol kedua
 *   x3, y3 - Titik akhir kurva
 *
 * Return:
 *   STATUS_BERHASIL atau kode error
 */
status_t pengolah_kurva_bezier3(struct p_konteks *ktx,
                                tanda32_t x0, tanda32_t y0,
                                tanda32_t x1, tanda32_t y1,
                                tanda32_t x2, tanda32_t y2,
                                tanda32_t x3, tanda32_t y3)
{
    tak_bertanda32_t warna;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    warna = pengolah_rgba_ke_piksel(
        ktx->konfig.warna.r, ktx->konfig.warna.g,
        ktx->konfig.warna.b, ktx->konfig.warna.a);

    return kurva_bezier3_route(ktx, x0, y0, x1, y1,
                               x2, y2, x3, y3, warna);
}
