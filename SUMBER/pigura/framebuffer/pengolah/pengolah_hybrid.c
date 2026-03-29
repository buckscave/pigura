/*
 * PIGURA OS - PENGOLAH_HYBRID.C
 * =============================
 * Backend Hybrid (otomatis memilih CPU atau GPU) untuk
 * pengolah grafis Pigura OS.
 *
 * Modul ini merutekan operasi ke backend yang paling
 * efisien berdasarkan ukuran area dan ketersediaan GPU:
 *   - Area kecil: gunakan CPU (overhead GPU terlalu besar)
 *   - Area besar: gunakan GPU (akselerasi bermakna)
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
 * DEKLARASI FUNGSI EXTERN - CPU BACKEND
 * ===========================================================================
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
extern status_t cpu_isi_area(struct p_konteks *ktx,
                             tanda32_t x, tanda32_t y,
                             tak_bertanda32_t warna_isi,
                             tak_bertanda32_t warna_batas);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI EXTERN - GPU BACKEND
 * ===========================================================================
 */

extern status_t gpu_titik(struct p_konteks *ktx,
                          tanda32_t x, tanda32_t y,
                          tak_bertanda32_t warna);
extern status_t gpu_garis(struct p_konteks *ktx,
                          tanda32_t x0, tanda32_t y0,
                          tanda32_t x1, tanda32_t y1,
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
extern status_t gpu_lingkaran(struct p_konteks *ktx,
                              tanda32_t cx, tanda32_t cy,
                              tak_bertanda32_t jari_jari,
                              tak_bertanda32_t warna);
extern status_t gpu_lingkaran_isi(struct p_konteks *ktx,
                                  tanda32_t cx, tanda32_t cy,
                                  tak_bertanda32_t jari_jari,
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
extern status_t gpu_poligon(struct p_konteks *ktx,
                            const struct p_titik *titik,
                            tak_bertanda32_t jumlah,
                            tak_bertanda32_t warna);
extern status_t gpu_poligon_isi(struct p_konteks *ktx,
                                const struct p_titik *titik,
                                tak_bertanda32_t jumlah,
                                tak_bertanda32_t warna);
extern status_t gpu_isi_area(struct p_konteks *ktx,
                             tanda32_t x, tanda32_t y,
                             tak_bertanda32_t warna_isi,
                             tak_bertanda32_t warna_batas);

/*
 * ===========================================================================
 * FUNGSI INTERNAL - KEPUTUSAN ROUTING
 * ===========================================================================
 */

/*
 * hybrid_gunakan_gpu_area
 * -----------------------
 * Menentukan apakah area cukup besar untuk
 * menggunakan GPU berdasarkan jumlah piksel.
 *
 * Return:
 *   BENAR jika sebaiknya gunakan GPU
 */
static bool_t hybrid_gunakan_gpu_area(
    tak_bertanda64_t jumlah_piksel)
{
    return jumlah_piksel >= PENGOLAH_HYBRID_AMBANG ? BENAR
                                                    : SALAH;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK HYBRID - TITIK
 * ===========================================================================
 */

status_t hybrid_titik(struct p_konteks *ktx,
                      tanda32_t x, tanda32_t y,
                      tak_bertanda32_t warna)
{
    /* Titik tunggal selalu gunakan CPU */
    return cpu_titik(ktx, x, y, warna);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK HYBRID - GARIS
 * ===========================================================================
 */

status_t hybrid_garis(struct p_konteks *ktx,
                      tanda32_t x0, tanda32_t y0,
                      tanda32_t x1, tanda32_t y1,
                      tak_bertanda32_t warna)
{
    tanda32_t dx, dy;
    tak_bertanda64_t panjang;

    dx = (x0 > x1) ? (x0 - x1) : (x1 - x0);
    dy = (y0 > y1) ? (y0 - y1) : (y1 - y0);
    panjang = (tak_bertanda64_t)((dx > dy) ? dx : dy) + 1;

    if (hybrid_gunakan_gpu_area(panjang)) {
        return gpu_garis(ktx, x0, y0, x1, y1, warna);
    }
    return cpu_garis(ktx, x0, y0, x1, y1, warna);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK HYBRID - KOTAK
 * ===========================================================================
 */

status_t hybrid_kotak(struct p_konteks *ktx,
                      tanda32_t x, tanda32_t y,
                      tak_bertanda32_t lebar,
                      tak_bertanda32_t tinggi,
                      tak_bertanda32_t warna)
{
    return cpu_kotak(ktx, x, y, lebar, tinggi, warna);
}

status_t hybrid_kotak_isi(struct p_konteks *ktx,
                          tanda32_t x, tanda32_t y,
                          tak_bertanda32_t lebar,
                          tak_bertanda32_t tinggi,
                          tak_bertanda32_t warna)
{
    tak_bertanda64_t area;

    area = (tak_bertanda64_t)lebar *
           (tak_bertanda64_t)tinggi;

    if (hybrid_gunakan_gpu_area(area)) {
        return gpu_kotak_isi(ktx, x, y, lebar, tinggi,
                             warna);
    }
    return cpu_kotak_isi(ktx, x, y, lebar, tinggi, warna);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK HYBRID - LINGKARAN
 * ===========================================================================
 */

status_t hybrid_lingkaran(struct p_konteks *ktx,
                          tanda32_t cx, tanda32_t cy,
                          tak_bertanda32_t jari_jari,
                          tak_bertanda32_t warna)
{
    return cpu_lingkaran(ktx, cx, cy, jari_jari, warna);
}

status_t hybrid_lingkaran_isi(struct p_konteks *ktx,
                              tanda32_t cx, tanda32_t cy,
                              tak_bertanda32_t jari_jari,
                              tak_bertanda32_t warna)
{
    tak_bertanda64_t area;

    area = (tak_bertanda64_t)31416 *
           (tak_bertanda64_t)jari_jari *
           (tak_bertanda64_t)jari_jari / 100000;

    if (hybrid_gunakan_gpu_area(area)) {
        return gpu_lingkaran_isi(ktx, cx, cy,
                                 jari_jari, warna);
    }
    return cpu_lingkaran_isi(ktx, cx, cy, jari_jari, warna);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK HYBRID - ELIP
 * ===========================================================================
 */

status_t hybrid_elip(struct p_konteks *ktx,
                     tanda32_t cx, tanda32_t cy,
                     tak_bertanda32_t rx, tak_bertanda32_t ry,
                     tak_bertanda32_t warna)
{
    return cpu_elip(ktx, cx, cy, rx, ry, warna);
}

status_t hybrid_elip_isi(struct p_konteks *ktx,
                         tanda32_t cx, tanda32_t cy,
                         tak_bertanda32_t rx, tak_bertanda32_t ry,
                         tak_bertanda32_t warna)
{
    tak_bertanda64_t area;

    area = (tak_bertanda64_t)31416 *
           (tak_bertanda64_t)rx *
           (tak_bertanda64_t)ry / 100000;

    if (hybrid_gunakan_gpu_area(area)) {
        return gpu_elip_isi(ktx, cx, cy, rx, ry, warna);
    }
    return cpu_elip_isi(ktx, cx, cy, rx, ry, warna);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK HYBRID - POLIGON
 * ===========================================================================
 */

status_t hybrid_poligon(struct p_konteks *ktx,
                        const struct p_titik *titik,
                        tak_bertanda32_t jumlah,
                        tak_bertanda32_t warna)
{
    return cpu_poligon(ktx, titik, jumlah, warna);
}

status_t hybrid_poligon_isi(struct p_konteks *ktx,
                            const struct p_titik *titik,
                            tak_bertanda32_t jumlah,
                            tak_bertanda32_t warna)
{
    return cpu_poligon_isi(ktx, titik, jumlah, warna);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK HYBRID - KURVA BEZIER
 * ===========================================================================
 */

status_t hybrid_kurva_bezier2(struct p_konteks *ktx,
                              tanda32_t x0, tanda32_t y0,
                              tanda32_t x1, tanda32_t y1,
                              tanda32_t x2, tanda32_t y2,
                              tak_bertanda32_t warna)
{
    return cpu_kurva_bezier2(ktx, x0, y0, x1, y1, x2, y2,
                             warna);
}

status_t hybrid_kurva_bezier3(struct p_konteks *ktx,
                              tanda32_t x0, tanda32_t y0,
                              tanda32_t x1, tanda32_t y1,
                              tanda32_t x2, tanda32_t y2,
                              tanda32_t x3, tanda32_t y3,
                              tak_bertanda32_t warna)
{
    return cpu_kurva_bezier3(ktx, x0, y0, x1, y1, x2, y2,
                             x3, y3, warna);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK HYBRID - ISI AREA
 * ===========================================================================
 */

status_t hybrid_isi_area(struct p_konteks *ktx,
                         tanda32_t x, tanda32_t y,
                         tak_bertanda32_t warna_isi,
                         tak_bertanda32_t warna_batas)
{
    return cpu_isi_area(ktx, x, y, warna_isi, warna_batas);
}
