/*
 * PIGURA OS - POLIGON.C
 * =======================
 * Operasi poligon untuk pengolah grafis Pigura OS.
 *
 * Mendukung outline poligon dan poligon terisi menggunakan
 * algoritma scanline. Routing ke backend CPU, GPU, atau Hybrid.
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

extern status_t cpu_poligon(struct p_konteks *ktx,
                            const struct p_titik *titik,
                            tak_bertanda32_t jumlah,
                            tak_bertanda32_t warna);
extern status_t cpu_poligon_isi(struct p_konteks *ktx,
                                const struct p_titik *titik,
                                tak_bertanda32_t jumlah,
                                tak_bertanda32_t warna);
extern status_t gpu_poligon(struct p_konteks *ktx,
                            const struct p_titik *titik,
                            tak_bertanda32_t jumlah,
                            tak_bertanda32_t warna);
extern status_t gpu_poligon_isi(struct p_konteks *ktx,
                                const struct p_titik *titik,
                                tak_bertanda32_t jumlah,
                                tak_bertanda32_t warna);
extern status_t hybrid_poligon(struct p_konteks *ktx,
                               const struct p_titik *titik,
                               tak_bertanda32_t jumlah,
                               tak_bertanda32_t warna);
extern status_t hybrid_poligon_isi(struct p_konteks *ktx,
                                   const struct p_titik *titik,
                                   tak_bertanda32_t jumlah,
                                   tak_bertanda32_t warna);

static status_t poligon_route(struct p_konteks *ktx,
                              const struct p_titik *titik,
                              tak_bertanda32_t jumlah,
                              tak_bertanda32_t warna)
{
    switch (ktx->mode) {
    case PENGOLAH_MODE_GPU:
        return gpu_poligon(ktx, titik, jumlah, warna);
    case PENGOLAH_MODE_HYBRID:
        return hybrid_poligon(ktx, titik, jumlah, warna);
    case PENGOLAH_MODE_CPU:
    default:
        return cpu_poligon(ktx, titik, jumlah, warna);
    }
}

static status_t poligon_isi_route(struct p_konteks *ktx,
                                  const struct p_titik *titik,
                                  tak_bertanda32_t jumlah,
                                  tak_bertanda32_t warna)
{
    switch (ktx->mode) {
    case PENGOLAH_MODE_GPU:
        return gpu_poligon_isi(ktx, titik, jumlah, warna);
    case PENGOLAH_MODE_HYBRID:
        return hybrid_poligon_isi(ktx, titik, jumlah, warna);
    case PENGOLAH_MODE_CPU:
    default:
        return cpu_poligon_isi(ktx, titik, jumlah, warna);
    }
}

/*
 * pengolah_poligon
 * ---------------
 * Menggambar outline poligon dari array titik.
 *
 * Parameter:
 *   ktx    - Pointer ke konteks pengolah
 *   titik  - Array titik-titik poligon
 *   jumlah - Jumlah titik (minimum 3)
 *
 * Return:
 *   STATUS_BERHASIL atau kode error
 */
status_t pengolah_poligon(struct p_konteks *ktx,
                          const struct p_titik *titik,
                          tak_bertanda32_t jumlah)
{
    tak_bertanda32_t warna;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (titik == NULL || jumlah < 3) {
        return STATUS_PARAM_INVALID;
    }
    if (jumlah > PENGOLAH_POLIGON_MAKS) {
        return STATUS_PARAM_JARAK;
    }

    warna = pengolah_rgba_ke_piksel(
        ktx->konfig.warna.r, ktx->konfig.warna.g,
        ktx->konfig.warna.b, ktx->konfig.warna.a);

    return poligon_route(ktx, titik, jumlah, warna);
}

/*
 * pengolah_poligon_isi
 * --------------------
 * Menggambar poligon terisi dari array titik.
 */
status_t pengolah_poligon_isi(struct p_konteks *ktx,
                              const struct p_titik *titik,
                              tak_bertanda32_t jumlah)
{
    tak_bertanda32_t warna;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (titik == NULL || jumlah < 3) {
        return STATUS_PARAM_INVALID;
    }
    if (jumlah > PENGOLAH_POLIGON_MAKS) {
        return STATUS_PARAM_JARAK;
    }

    warna = pengolah_rgba_ke_piksel(
        ktx->konfig.warna.r, ktx->konfig.warna.g,
        ktx->konfig.warna.b, ktx->konfig.warna.a);

    return poligon_isi_route(ktx, titik, jumlah, warna);
}
