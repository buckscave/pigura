/*
 * PIGURA OS - LINGKARAN.C
 * ========================
 * Operasi lingkaran untuk pengolah grafis Pigura OS.
 *
 * Menggunakan algoritma Midpoint Circle untuk outline
 * dan scanline fill untuk lingkaran isi.
 * Routing ke backend CPU, GPU, atau Hybrid.
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

extern status_t cpu_lingkaran(struct p_konteks *ktx,
                              tanda32_t cx, tanda32_t cy,
                              tak_bertanda32_t jari_jari,
                              tak_bertanda32_t warna);
extern status_t cpu_lingkaran_isi(struct p_konteks *ktx,
                                  tanda32_t cx, tanda32_t cy,
                                  tak_bertanda32_t jari_jari,
                                  tak_bertanda32_t warna);
extern status_t gpu_lingkaran(struct p_konteks *ktx,
                              tanda32_t cx, tanda32_t cy,
                              tak_bertanda32_t jari_jari,
                              tak_bertanda32_t warna);
extern status_t gpu_lingkaran_isi(struct p_konteks *ktx,
                                  tanda32_t cx, tanda32_t cy,
                                  tak_bertanda32_t jari_jari,
                                  tak_bertanda32_t warna);
extern status_t hybrid_lingkaran(struct p_konteks *ktx,
                                 tanda32_t cx, tanda32_t cy,
                                 tak_bertanda32_t jari_jari,
                                 tak_bertanda32_t warna);
extern status_t hybrid_lingkaran_isi(struct p_konteks *ktx,
                                     tanda32_t cx, tanda32_t cy,
                                     tak_bertanda32_t jari_jari,
                                     tak_bertanda32_t warna);

static status_t lingkaran_route(struct p_konteks *ktx,
                                tanda32_t cx, tanda32_t cy,
                                tak_bertanda32_t jari_jari,
                                tak_bertanda32_t warna)
{
    switch (ktx->mode) {
    case PENGOLAH_MODE_GPU:
        return gpu_lingkaran(ktx, cx, cy, jari_jari, warna);
    case PENGOLAH_MODE_HYBRID:
        return hybrid_lingkaran(ktx, cx, cy, jari_jari,
                                warna);
    case PENGOLAH_MODE_CPU:
    default:
        return cpu_lingkaran(ktx, cx, cy, jari_jari, warna);
    }
}

static status_t lingkaran_isi_route(struct p_konteks *ktx,
                                    tanda32_t cx, tanda32_t cy,
                                    tak_bertanda32_t jari_jari,
                                    tak_bertanda32_t warna)
{
    switch (ktx->mode) {
    case PENGOLAH_MODE_GPU:
        return gpu_lingkaran_isi(ktx, cx, cy, jari_jari,
                                  warna);
    case PENGOLAH_MODE_HYBRID:
        return hybrid_lingkaran_isi(ktx, cx, cy, jari_jari,
                                    warna);
    case PENGOLAH_MODE_CPU:
    default:
        return cpu_lingkaran_isi(ktx, cx, cy, jari_jari,
                                  warna);
    }
}

/*
 * pengolah_lingkaran
 * -----------------
 * Menggambar outline lingkaran.
 */
status_t pengolah_lingkaran(struct p_konteks *ktx,
                            tanda32_t cx, tanda32_t cy,
                            tak_bertanda32_t jari_jari)
{
    tak_bertanda32_t warna;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (jari_jari == 0) {
        return STATUS_PARAM_JARAK;
    }

    warna = pengolah_rgba_ke_piksel(
        ktx->konfig.warna.r, ktx->konfig.warna.g,
        ktx->konfig.warna.b, ktx->konfig.warna.a);

    return lingkaran_route(ktx, cx, cy, jari_jari, warna);
}

/*
 * pengolah_lingkaran_isi
 * ----------------------
 * Menggambar lingkaran terisi penuh.
 */
status_t pengolah_lingkaran_isi(struct p_konteks *ktx,
                                tanda32_t cx, tanda32_t cy,
                                tak_bertanda32_t jari_jari)
{
    tak_bertanda32_t warna;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (jari_jari == 0) {
        return STATUS_PARAM_JARAK;
    }

    warna = pengolah_rgba_ke_piksel(
        ktx->konfig.warna.r, ktx->konfig.warna.g,
        ktx->konfig.warna.b, ktx->konfig.warna.a);

    return lingkaran_isi_route(ktx, cx, cy, jari_jari, warna);
}

/*
 * pengolah_lingkaran_warna
 * ------------------------
 * Menggambar outline lingkaran dengan warna spesifik.
 */
status_t pengolah_lingkaran_warna(struct p_konteks *ktx,
                                  tanda32_t cx, tanda32_t cy,
                                  tak_bertanda32_t jari_jari,
                                  tak_bertanda32_t warna)
{
    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (jari_jari == 0) {
        return STATUS_PARAM_JARAK;
    }

    return lingkaran_route(ktx, cx, cy, jari_jari, warna);
}
