/*
 * PIGURA OS - TITIK.C
 * ===================
 * Operasi titik (piksel tunggal) untuk pengolah grafis.
 *
 * Modul ini menyediakan fungsi untuk menulis dan membaca
 * piksel individual pada buffer, dengan dukungan klip.
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

extern status_t cpu_titik(struct p_konteks *ktx,
                          tanda32_t x, tanda32_t y,
                          tak_bertanda32_t warna);
extern status_t gpu_titik(struct p_konteks *ktx,
                          tanda32_t x, tanda32_t y,
                          tak_bertanda32_t warna);
extern status_t hybrid_titik(struct p_konteks *ktx,
                             tanda32_t x, tanda32_t y,
                             tak_bertanda32_t warna);

/*
 * ===========================================================================
 * FUNGSI INTERNAL - ROUTING
 * ===========================================================================
 */

/*
 * titik_route
 * -----------
 * Merutekan operasi titik ke backend yang aktif.
 */
static status_t titik_route(struct p_konteks *ktx,
                            tanda32_t x, tanda32_t y,
                            tak_bertanda32_t warna)
{
    switch (ktx->mode) {
    case PENGOLAH_MODE_GPU:
        return gpu_titik(ktx, x, y, warna);
    case PENGOLAH_MODE_HYBRID:
        return hybrid_titik(ktx, x, y, warna);
    case PENGOLAH_MODE_CPU:
    default:
        return cpu_titik(ktx, x, y, warna);
    }
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * pengolah_titik
 * --------------
 * Menulis satu piksel ke buffer menggunakan warna aktif.
 * Mendukung klip area otomatis.
 *
 * Parameter:
 *   ktx - Pointer ke konteks pengolah
 *   x   - Koordinat x
 *   y   - Koordinat y
 *
 * Return:
 *   STATUS_BERHASIL atau kode error
 */
status_t pengolah_titik(struct p_konteks *ktx,
                        tanda32_t x, tanda32_t y)
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

    return titik_route(ktx, x, y, warna);
}

/*
 * pengolah_titik_warna
 * --------------------
 * Menulis satu piksel dengan warna spesifik.
 * Tidak menggunakan warna aktif dari konteks.
 *
 * Parameter:
 *   ktx   - Pointer ke konteks pengolah
 *   x, y  - Koordinat piksel
 *   warna - Nilai piksel 32-bit (ARGB8888)
 *
 * Return:
 *   STATUS_BERHASIL atau kode error
 */
status_t pengolah_titik_warna(struct p_konteks *ktx,
                              tanda32_t x, tanda32_t y,
                              tak_bertanda32_t warna)
{
    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    return titik_route(ktx, x, y, warna);
}
