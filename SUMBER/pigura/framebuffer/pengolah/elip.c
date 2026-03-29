/*
 * PIGURA OS - ELIP.C
 * ===================
 * Operasi elip untuk pengolah grafis Pigura OS.
 *
 * Menggunakan algoritma Midpoint Ellipse untuk outline
 * dan scanline fill untuk elip isi.
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

extern status_t cpu_elip(struct p_konteks *ktx,
                         tanda32_t cx, tanda32_t cy,
                         tak_bertanda32_t rx,
                         tak_bertanda32_t ry,
                         tak_bertanda32_t warna);
extern status_t cpu_elip_isi(struct p_konteks *ktx,
                             tanda32_t cx, tanda32_t cy,
                             tak_bertanda32_t rx,
                             tak_bertanda32_t ry,
                             tak_bertanda32_t warna);
extern status_t gpu_elip(struct p_konteks *ktx,
                         tanda32_t cx, tanda32_t cy,
                         tak_bertanda32_t rx,
                         tak_bertanda32_t ry,
                         tak_bertanda32_t warna);
extern status_t gpu_elip_isi(struct p_konteks *ktx,
                             tanda32_t cx, tanda32_t cy,
                             tak_bertanda32_t rx,
                             tak_bertanda32_t ry,
                             tak_bertanda32_t warna);
extern status_t hybrid_elip(struct p_konteks *ktx,
                            tanda32_t cx, tanda32_t cy,
                            tak_bertanda32_t rx,
                            tak_bertanda32_t ry,
                            tak_bertanda32_t warna);
extern status_t hybrid_elip_isi(struct p_konteks *ktx,
                                tanda32_t cx, tanda32_t cy,
                                tak_bertanda32_t rx,
                                tak_bertanda32_t ry,
                                tak_bertanda32_t warna);

static status_t elip_route(struct p_konteks *ktx,
                           tanda32_t cx, tanda32_t cy,
                           tak_bertanda32_t rx,
                           tak_bertanda32_t ry,
                           tak_bertanda32_t warna)
{
    switch (ktx->mode) {
    case PENGOLAH_MODE_GPU:
        return gpu_elip(ktx, cx, cy, rx, ry, warna);
    case PENGOLAH_MODE_HYBRID:
        return hybrid_elip(ktx, cx, cy, rx, ry, warna);
    case PENGOLAH_MODE_CPU:
    default:
        return cpu_elip(ktx, cx, cy, rx, ry, warna);
    }
}

static status_t elip_isi_route(struct p_konteks *ktx,
                               tanda32_t cx, tanda32_t cy,
                               tak_bertanda32_t rx,
                               tak_bertanda32_t ry,
                               tak_bertanda32_t warna)
{
    switch (ktx->mode) {
    case PENGOLAH_MODE_GPU:
        return gpu_elip_isi(ktx, cx, cy, rx, ry, warna);
    case PENGOLAH_MODE_HYBRID:
        return hybrid_elip_isi(ktx, cx, cy, rx, ry, warna);
    case PENGOLAH_MODE_CPU:
    default:
        return cpu_elip_isi(ktx, cx, cy, rx, ry, warna);
    }
}

status_t pengolah_elip(struct p_konteks *ktx,
                       tanda32_t cx, tanda32_t cy,
                       tak_bertanda32_t rx,
                       tak_bertanda32_t ry)
{
    tak_bertanda32_t warna;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (rx == 0 || ry == 0) {
        return STATUS_PARAM_JARAK;
    }

    warna = pengolah_rgba_ke_piksel(
        ktx->konfig.warna.r, ktx->konfig.warna.g,
        ktx->konfig.warna.b, ktx->konfig.warna.a);

    return elip_route(ktx, cx, cy, rx, ry, warna);
}

status_t pengolah_elip_isi(struct p_konteks *ktx,
                           tanda32_t cx, tanda32_t cy,
                           tak_bertanda32_t rx,
                           tak_bertanda32_t ry)
{
    tak_bertanda32_t warna;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (rx == 0 || ry == 0) {
        return STATUS_PARAM_JARAK;
    }

    warna = pengolah_rgba_ke_piksel(
        ktx->konfig.warna.r, ktx->konfig.warna.g,
        ktx->konfig.warna.b, ktx->konfig.warna.a);

    return elip_isi_route(ktx, cx, cy, rx, ry, warna);
}
