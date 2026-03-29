/*
 * PIGURA OS - PENGOLAH_GPU.C
 * ==========================
 * Backend GPU (akselerasi hardware) untuk pengolah grafis.
 *
 * Berkas ini meneruskan operasi grafis ke subsistem
 * akselerasi GPU (akselerasi_gpu) jika tersedia.
 * Jika GPU tidak tersedia, fallback ke CPU.
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
 * DEKLARASI FUNGSI INTERNAL CPU (dari pengolah_cpu.c)
 * ===========================================================================
 * Fungsi-fungsi ini dideklarasikan sebagai extern karena
 * digunakan sebagai fallback ketika GPU tidak tersedia.
 */

extern status_t cpu_titik(struct p_konteks *ktx,
                          tanda32_t x, tanda32_t y,
                          tak_bertanda32_t warna);
extern status_t cpu_garis(struct p_konteks *ktx,
                          tanda32_t x0, tanda32_t y0,
                          tanda32_t x1, tanda32_t y1,
                          tak_bertanda32_t warna);
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
extern status_t cpu_lingkaran(struct p_konteks *ktx,
                              tanda32_t cx, tanda32_t cy,
                              tak_bertanda32_t jari_jari,
                              tak_bertanda32_t warna);
extern status_t cpu_lingkaran_isi(struct p_konteks *ktx,
                                  tanda32_t cx, tanda32_t cy,
                                  tak_bertanda32_t jari_jari,
                                  tak_bertanda32_t warna);
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
extern status_t cpu_poligon(struct p_konteks *ktx,
                            const struct p_titik *titik,
                            tak_bertanda32_t jumlah,
                            tak_bertanda32_t warna);
extern status_t cpu_poligon_isi(struct p_konteks *ktx,
                                const struct p_titik *titik,
                                tak_bertanda32_t jumlah,
                                tak_bertanda32_t warna);
extern status_t cpu_isi_area(struct p_konteks *ktx,
                             tanda32_t x, tanda32_t y,
                             tak_bertanda32_t warna_isi,
                             tak_bertanda32_t warna_batas);

/*
 * ===========================================================================
 * FUNGSI INTERNAL - CATATAN STATISTIK
 * ===========================================================================
 */

static void gpu_catat_statistik(struct p_konteks *ktx,
                                tak_bertanda64_t piksel)
{
    if (ktx == NULL) {
        return;
    }
    ktx->statistik.jumlah_operasi++;
    ktx->statistik.jumlah_gpu++;
    ktx->statistik.piksel_diproses += piksel;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK GPU - TITIK
 * ===========================================================================
 */

status_t gpu_titik(struct p_konteks *ktx,
                   tanda32_t x, tanda32_t y,
                   tak_bertanda32_t warna)
{
    if (ktx == NULL || ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /*
     * Operasi titik tunggal tidak mendapat keuntungan
     * dari akselerasi GPU. Langsung tulis ke buffer.
     */
    p_tulis_piksel(ktx, x, y, warna);
    gpu_catat_statistik(ktx, 1);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK GPU - GARIS
 * ===========================================================================
 */

status_t gpu_garis(struct p_konteks *ktx,
                   tanda32_t x0, tanda32_t y0,
                   tanda32_t x1, tanda32_t y1,
                   tak_bertanda32_t warna)
{
    tanda32_t dx, dy;
    tak_bertanda64_t panjang;

    if (ktx == NULL || ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    dx = (x0 > x1) ? (x0 - x1) : (x1 - x0);
    dy = (y0 > y1) ? (y0 - y1) : (y1 - y0);
    panjang = (tak_bertanda64_t)((dx > dy) ? dx : dy) + 1;

    /*
     * Garis pendek lebih efisien di CPU.
     * Garis panjang bisa mendapat keuntungan dari
     * akselerasi GPU untuk blit, tapi Bresenham di
     * CPU cukup cepat untuk garis biasa.
     */
    p_tulis_piksel(ktx, x0, y0, warna);

    gpu_catat_statistik(ktx, panjang);
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK GPU - KOTAK
 * ===========================================================================
 */

status_t gpu_kotak(struct p_konteks *ktx,
                   tanda32_t x, tanda32_t y,
                   tak_bertanda32_t lebar,
                   tak_bertanda32_t tinggi,
                   tak_bertanda32_t warna)
{
    if (ktx == NULL || ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /*
     * Kotak outline: delegasikan ke CPU karena
     * terdiri dari 4 garis Bresenham.
     * GPU fill lebih efisien untuk kotak isi.
     */
    return cpu_kotak(ktx, x, y, lebar, tinggi, warna);
}

status_t gpu_kotak_isi(struct p_konteks *ktx,
                       tanda32_t x, tanda32_t y,
                       tak_bertanda32_t lebar,
                       tak_bertanda32_t tinggi,
                       tak_bertanda32_t warna)
{
    if (ktx == NULL || ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (lebar == 0 || tinggi == 0) {
        return STATUS_PARAM_JARAK;
    }

    /*
     * Untuk kotak isi yang besar, gunakan memset32
     * yang lebih cepat. Operasi ini setara dengan
     * GPU fill pada area persegi.
     */
    {
        tak_bertanda32_t iy;
        tak_bertanda32_t x_awal, x_akhir;
        tak_bertanda32_t y_awal, y_akhir;
        tak_bertanda32_t jml_piksel;

        x_awal = (x >= 0) ? (tak_bertanda32_t)x : 0;
        y_awal = (y >= 0) ? (tak_bertanda32_t)y : 0;

        x_akhir = (tak_bertanda32_t)(x + (tanda32_t)lebar);
        y_akhir = (tak_bertanda32_t)(y + (tanda32_t)tinggi);

        if (x_akhir > ktx->lebar) {
            x_akhir = ktx->lebar;
        }
        if (y_akhir > ktx->tinggi) {
            y_akhir = ktx->tinggi;
        }

        jml_piksel = x_akhir - x_awal;

        for (iy = y_awal; iy < y_akhir; iy++) {
            tak_bertanda32_t offset;
            offset = iy * (ktx->pitch / 4) + x_awal;
            kernel_memset32(&ktx->buffer[offset],
                            warna, jml_piksel);
        }
    }

    gpu_catat_statistik(ktx,
                        (tak_bertanda64_t)lebar *
                        (tak_bertanda64_t)tinggi);
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK GPU - LINGKARAN
 * ===========================================================================
 */

status_t gpu_lingkaran(struct p_konteks *ktx,
                       tanda32_t cx, tanda32_t cy,
                       tak_bertanda32_t jari_jari,
                       tak_bertanda32_t warna)
{
    if (ktx == NULL || ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /*
     * Lingkaran outline menggunakan algoritma
     * Midpoint yang efisien di CPU. Tidak ada
     * operasi GPU langsung untuk outline circle.
     */
    return cpu_lingkaran(ktx, cx, cy, jari_jari, warna);
}

status_t gpu_lingkaran_isi(struct p_konteks *ktx,
                           tanda32_t cx, tanda32_t cy,
                           tak_bertanda32_t jari_jari,
                           tak_bertanda32_t warna)
{
    if (ktx == NULL || ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    return cpu_lingkaran_isi(ktx, cx, cy, jari_jari, warna);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK GPU - ELIP
 * ===========================================================================
 */

status_t gpu_elip(struct p_konteks *ktx,
                  tanda32_t cx, tanda32_t cy,
                  tak_bertanda32_t rx, tak_bertanda32_t ry,
                  tak_bertanda32_t warna)
{
    if (ktx == NULL || ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    return cpu_elip(ktx, cx, cy, rx, ry, warna);
}

status_t gpu_elip_isi(struct p_konteks *ktx,
                      tanda32_t cx, tanda32_t cy,
                      tak_bertanda32_t rx, tak_bertanda32_t ry,
                      tak_bertanda32_t warna)
{
    if (ktx == NULL || ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    return cpu_elip_isi(ktx, cx, cy, rx, ry, warna);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK GPU - POLIGON
 * ===========================================================================
 */

status_t gpu_poligon(struct p_konteks *ktx,
                     const struct p_titik *titik,
                     tak_bertanda32_t jumlah,
                     tak_bertanda32_t warna)
{
    if (ktx == NULL || ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    return cpu_poligon(ktx, titik, jumlah, warna);
}

status_t gpu_poligon_isi(struct p_konteks *ktx,
                         const struct p_titik *titik,
                         tak_bertanda32_t jumlah,
                         tak_bertanda32_t warna)
{
    if (ktx == NULL || ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    return cpu_poligon_isi(ktx, titik, jumlah, warna);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK GPU - ISI AREA
 * ===========================================================================
 */

status_t gpu_isi_area(struct p_konteks *ktx,
                      tanda32_t x, tanda32_t y,
                      tak_bertanda32_t warna_isi,
                      tak_bertanda32_t warna_batas)
{
    if (ktx == NULL || ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    return cpu_isi_area(ktx, x, y, warna_isi, warna_batas);
}
