/*
 * PIGURA OS - KOTAK.C
 * ====================
 * Operasi kotak (persegi panjang) untuk pengolah grafis.
 *
 * Mendukung kotak outline dan isi. Routing ke backend
 * CPU, GPU, atau Hybrid berdasarkan mode konteks.
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

extern status_t cpu_kotak(struct p_konteks *ktx,
                          tanda32_t x, tanda32_t y,
                          tak_bertanda32_t lebar,
                          tak_bertanda32_t tinggi,
                          tak_bertanda32_t warna);
extern status_t cpu_kotak_isi(struct p_konteks *ktx,
                              tanda32_t x, tanda32_t y,
                              tak_bertanda32_t lebar,
                              tak_bertanda32_t tinggi,
                              tak_bertanda32_t warna);
extern status_t gpu_kotak(struct p_konteks *ktx,
                          tanda32_t x, tanda32_t y,
                          tak_bertanda32_t lebar,
                          tak_bertanda32_t tinggi,
                          tak_bertanda32_t warna);
extern status_t gpu_kotak_isi(struct p_konteks *ktx,
                              tanda32_t x, tanda32_t y,
                              tak_bertanda32_t lebar,
                              tak_bertanda32_t tinggi,
                              tak_bertanda32_t warna);
extern status_t hybrid_kotak(struct p_konteks *ktx,
                             tanda32_t x, tanda32_t y,
                             tak_bertanda32_t lebar,
                             tak_bertanda32_t tinggi,
                             tak_bertanda32_t warna);
extern status_t hybrid_kotak_isi(struct p_konteks *ktx,
                                 tanda32_t x, tanda32_t y,
                                 tak_bertanda32_t lebar,
                                 tak_bertanda32_t tinggi,
                                 tak_bertanda32_t warna);

static status_t kotak_route(struct p_konteks *ktx,
                            tanda32_t x, tanda32_t y,
                            tak_bertanda32_t lebar,
                            tak_bertanda32_t tinggi,
                            tak_bertanda32_t warna)
{
    switch (ktx->mode) {
    case PENGOLAH_MODE_GPU:
        return gpu_kotak(ktx, x, y, lebar, tinggi, warna);
    case PENGOLAH_MODE_HYBRID:
        return hybrid_kotak(ktx, x, y, lebar, tinggi, warna);
    case PENGOLAH_MODE_CPU:
    default:
        return cpu_kotak(ktx, x, y, lebar, tinggi, warna);
    }
}

static status_t kotak_isi_route(struct p_konteks *ktx,
                                tanda32_t x, tanda32_t y,
                                tak_bertanda32_t lebar,
                                tak_bertanda32_t tinggi,
                                tak_bertanda32_t warna)
{
    switch (ktx->mode) {
    case PENGOLAH_MODE_GPU:
        return gpu_kotak_isi(ktx, x, y, lebar, tinggi,
                             warna);
    case PENGOLAH_MODE_HYBRID:
        return hybrid_kotak_isi(ktx, x, y, lebar, tinggi,
                                warna);
    case PENGOLAH_MODE_CPU:
    default:
        return cpu_kotak_isi(ktx, x, y, lebar, tinggi,
                             warna);
    }
}

status_t pengolah_kotak(struct p_konteks *ktx,
                        tanda32_t x, tanda32_t y,
                        tak_bertanda32_t lebar,
                        tak_bertanda32_t tinggi)
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

    return kotak_route(ktx, x, y, lebar, tinggi, warna);
}

status_t pengolah_kotak_isi(struct p_konteks *ktx,
                            tanda32_t x, tanda32_t y,
                            tak_bertanda32_t lebar,
                            tak_bertanda32_t tinggi)
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

    return kotak_isi_route(ktx, x, y, lebar, tinggi, warna);
}

status_t pengolah_kotak_warna(struct p_konteks *ktx,
                              tanda32_t x, tanda32_t y,
                              tak_bertanda32_t lebar,
                              tak_bertanda32_t tinggi,
                              tak_bertanda32_t warna)
{
    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    return kotak_route(ktx, x, y, lebar, tinggi, warna);
}

status_t pengolah_kotak_isi_warna(struct p_konteks *ktx,
                                  tanda32_t x, tanda32_t y,
                                  tak_bertanda32_t lebar,
                                  tak_bertanda32_t tinggi,
                                  tak_bertanda32_t warna)
{
    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    return kotak_isi_route(ktx, x, y, lebar, tinggi, warna);
}
