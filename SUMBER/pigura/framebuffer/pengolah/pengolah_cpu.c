/*
 * PIGURA OS - PENGOLAH_CPU.C
 * ==========================
 * Backend CPU (software rendering) untuk pengolah grafis.
 *
 * Berkas ini mengimplementasikan semua operasi grafis
 * menggunakan algoritma software rendering murni:
 *   - Bresenham untuk garis
 *   - Midpoint untuk lingkaran dan elip
 *   - Scanline untuk pengisian
 *
 * Semua operasi menggunakan aritmatika integer.
 * Mendukung arsitektur: x86, x86_64, ARM, ARMv7, ARM64
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../../inti/kernel.h"
#include "pengolah.h"

/*
 * ===========================================================================
 * MAKRO INTERNAL
 * ===========================================================================
 */

/* Absolut nilai (integer) */
#define ABS_INT(v) (((v) < 0) ? -(v) : (v))

/* Nilai maksimum dua integer */
#define MAKS_INT(a, b) (((a) > (b)) ? (a) : (b))

/* Nilai minimum dua integer */
#define MIN_INT(a, b) (((a) < (b)) ? (a) : (b))

/* Tukar dua nilai integer */
#define TUKAR_INT(a, b) do { \
    tanda32_t _tmp = (a);   \
    (a) = (b);              \
    (b) = _tmp;             \
} while (0)

/*
 * ===========================================================================
 * FUNGSI INTERNAL CPU - HELPER STATISTIK
 * ===========================================================================
 */

/*
 * cpu_catat_statistik
 * ------------------
 * Mencatat operasi ke statistik pengolah.
 */
static void cpu_catat_statistik(struct p_konteks *ktx,
                                tak_bertanda64_t piksel)
{
    if (ktx == NULL) {
        return;
    }
    ktx->statistik.jumlah_operasi++;
    ktx->statistik.jumlah_cpu++;
    ktx->statistik.piksel_diproses += piksel;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL CPU - TITIK
 * ===========================================================================
 */

/*
 * cpu_titik_internal
 * -----------------
 * Menulis satu piksel ke buffer (tanpa statistik).
 * Digunakan sebagai building block operasi lain.
 */
static void cpu_titik_internal(struct p_konteks *ktx,
                               tanda32_t x, tanda32_t y,
                               tak_bertanda32_t warna)
{
    p_tulis_piksel(ktx, x, y, warna);
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL CPU - GARIS (BRESENHAM)
 * ===========================================================================
 */

/*
 * cpu_garis_horizontal
 * -------------------
 * Menggambar garis horizontal cepat.
 * Lebih efisien daripada Bresenham umum.
 */
static void cpu_garis_horizontal(struct p_konteks *ktx,
                                 tanda32_t x0, tanda32_t y0,
                                 tanda32_t x1,
                                 tak_bertanda32_t warna)
{
    tak_bertanda32_t x_awal, x_akhir, x;
    tak_bertanda32_t offset;

    if (x0 <= x1) {
        x_awal = (tak_bertanda32_t)x0;
        x_akhir = (tak_bertanda32_t)x1;
    } else {
        x_awal = (tak_bertanda32_t)x1;
        x_akhir = (tak_bertanda32_t)x0;
    }

    /* Batasi ke lebar buffer */
    if (x_awal >= ktx->lebar) {
        return;
    }
    if (x_akhir >= ktx->lebar) {
        x_akhir = ktx->lebar - 1;
    }

    offset = (tak_bertanda32_t)y0 * (ktx->pitch / 4) + x_awal;
    for (x = x_awal; x <= x_akhir; x++) {
        if (p_klip_titik(ktx, (tanda32_t)x, y0)) {
            ktx->buffer[offset + (x - x_awal)] = warna;
        }
    }
}

/*
 * cpu_garis_vertical
 * -----------------
 * Menggambar garis vertikal cepat.
 */
static void cpu_garis_vertical(struct p_konteks *ktx,
                               tanda32_t x0, tanda32_t y0,
                               tanda32_t y1,
                               tak_bertanda32_t warna)
{
    tak_bertanda32_t y_awal, y_akhir, y;
    tak_bertanda32_t offset;

    if (y0 <= y1) {
        y_awal = (tak_bertanda32_t)y0;
        y_akhir = (tak_bertanda32_t)y1;
    } else {
        y_awal = (tak_bertanda32_t)y1;
        y_akhir = (tak_bertanda32_t)y0;
    }

    if (y_awal >= ktx->tinggi) {
        return;
    }
    if (y_akhir >= ktx->tinggi) {
        y_akhir = ktx->tinggi - 1;
    }

    offset = y_awal * (ktx->pitch / 4) + (tak_bertanda32_t)x0;
    for (y = y_awal; y <= y_akhir; y++) {
        if (p_klip_titik(ktx, x0, (tanda32_t)y)) {
            ktx->buffer[offset] = warna;
        }
        offset += ktx->pitch / 4;
    }
}

/*
 * cpu_garis_bresenham
 * ------------------
 * Algoritma Bresenham untuk garis umum.
 * Menangani semua kemiringan dan oktan.
 */
static void cpu_garis_bresenham(struct p_konteks *ktx,
                                tanda32_t x0, tanda32_t y0,
                                tanda32_t x1, tanda32_t y1,
                                tak_bertanda32_t warna)
{
    tanda32_t dx, dy;
    tanda32_t sx, sy;
    tanda32_t err;
    tanda32_t e2;

    dx = ABS_INT(x1 - x0);
    dy = ABS_INT(y1 - y0);

    sx = (x0 < x1) ? 1 : -1;
    sy = (y0 < y1) ? 1 : -1;

    err = dx - dy;

    while (BENAR) {
        p_tulis_piksel(ktx, x0, y0, warna);

        if (x0 == x1 && y0 == y1) {
            break;
        }

        e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL CPU - LINGKARAN (MIDPOINT)
 * ===========================================================================
 */

/*
 * cpu_lingkaran_plot_8
 * -------------------
 * Menplot 8 titik simetris lingkaran.
 * Memanfaatkan simetri 8-oktan lingkaran.
 */
static void cpu_lingkaran_plot_8(struct p_konteks *ktx,
                                 tanda32_t cx, tanda32_t cy,
                                 tanda32_t x, tanda32_t y,
                                 tak_bertanda32_t warna)
{
    p_tulis_piksel(ktx, cx + (tanda32_t)x, cy - (tanda32_t)y,
                   warna);
    p_tulis_piksel(ktx, cx - (tanda32_t)x, cy - (tanda32_t)y,
                   warna);
    p_tulis_piksel(ktx, cx + (tanda32_t)x, cy + (tanda32_t)y,
                   warna);
    p_tulis_piksel(ktx, cx - (tanda32_t)x, cy + (tanda32_t)y,
                   warna);
    p_tulis_piksel(ktx, cx + (tanda32_t)y, cy - (tanda32_t)x,
                   warna);
    p_tulis_piksel(ktx, cx - (tanda32_t)y, cy - (tanda32_t)x,
                   warna);
    p_tulis_piksel(ktx, cx + (tanda32_t)y, cy + (tanda32_t)x,
                   warna);
    p_tulis_piksel(ktx, cx - (tanda32_t)y, cy + (tanda32_t)x,
                   warna);
}

/*
 * cpu_lingkaran_garis_horisontal
 * -----------------------------
 * Menggambar garis horizontal untuk pengisian lingkaran.
 * Digunakan pada setiap baris saat mengisi.
 */
static void cpu_lingkaran_garis_horisontal(
    struct p_konteks *ktx,
    tanda32_t cx, tanda32_t cy,
    tak_bertanda32_t __attribute__((unused)) x0, tak_bertanda32_t x1,
    tak_bertanda32_t warna)
{
    tanda32_t x_aktual, x_max;
    tak_bertanda32_t offset;

    x_aktual = (tanda32_t)(cx - x1);
    x_max = (tanda32_t)(cx + x1);

    if (x_aktual < 0) { x_aktual = 0; }
    if (x_max >= (tanda32_t)ktx->lebar) {
        x_max = (tanda32_t)(ktx->lebar - 1);
    }

    offset = (tak_bertanda32_t)cy * (ktx->pitch / 4) +
             (tak_bertanda32_t)x_aktual;
    for (; x_aktual <= x_max; x_aktual++) {
        if (p_klip_titik(ktx, x_aktual, cy)) {
            ktx->buffer[offset] = warna;
        }
        offset++;
    }
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL CPU - ELIP (MIDPOINT)
 * ===========================================================================
 */

/*
 * cpu_elip_plot_4
 * --------------
 * Menplot 4 titik simetris elips.
 */
static void cpu_elip_plot_4(struct p_konteks *ktx,
                            tanda32_t cx, tanda32_t cy,
                            tak_bertanda32_t x, tak_bertanda32_t y,
                            tak_bertanda32_t warna)
{
    p_tulis_piksel(ktx, cx + (tanda32_t)x, cy - (tanda32_t)y,
                   warna);
    p_tulis_piksel(ktx, cx - (tanda32_t)x, cy - (tanda32_t)y,
                   warna);
    p_tulis_piksel(ktx, cx + (tanda32_t)x, cy + (tanda32_t)y,
                   warna);
    p_tulis_piksel(ktx, cx - (tanda32_t)x, cy + (tanda32_t)y,
                   warna);
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL CPU - POLIGON (SCANLINE)
 * ===========================================================================
 */

/*
 * cpu_poligon_hitung_tepi
 * -----------------------
 * Menghitung tabel tepi (edge table) untuk poligon.
 * Mengembalikan jumlah tepi yang valid.
 */
static tak_bertanda32_t cpu_poligon_hitung_tepi(
    const struct p_titik *titik,
    tak_bertanda32_t jumlah,
    tak_bertanda32_t y_min,
    tak_bertanda32_t y_maks,
    tak_bertanda32_t *y_tepi)
{
    tak_bertanda32_t i, j, count;

    (void)y_min;
    (void)y_maks;
    count = 0;

    for (i = 0; i < jumlah; i++) {
        j = (i + 1) % jumlah;

        /* Abaikan garis horizontal */
        if (titik[i].y == titik[j].y) {
            continue;
        }

        y_tepi[count++] = titik[i].y;
        y_tepi[count++] = titik[j].y;
    }

    return count;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK CPU - TITIK
 * ===========================================================================
 */

status_t cpu_titik(struct p_konteks *ktx,
                   tanda32_t x, tanda32_t y,
                   tak_bertanda32_t warna)
{
    if (ktx == NULL || ktx->buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    p_tulis_piksel(ktx, x, y, warna);
    cpu_catat_statistik(ktx, 1);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK CPU - GARIS
 * ===========================================================================
 */

status_t cpu_garis(struct p_konteks *ktx,
                   tanda32_t x0, tanda32_t y0,
                   tanda32_t x1, tanda32_t y1,
                   tak_bertanda32_t warna)
{
    tanda32_t dx, dy;
    tak_bertanda64_t panjang;

    if (ktx == NULL || ktx->buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    dx = ABS_INT(x1 - x0);
    dy = ABS_INT(y1 - y0);
    panjang = (tak_bertanda64_t)MAKS_INT(dx, dy) + 1;

    /* Cek apakah garis horizontal atau vertikal */
    if (y0 == y1) {
        cpu_garis_horizontal(ktx, x0, y0, x1, warna);
    } else if (x0 == x1) {
        cpu_garis_vertical(ktx, x0, y0, y1, warna);
    } else {
        cpu_garis_bresenham(ktx, x0, y0, x1, y1, warna);
    }

    cpu_catat_statistik(ktx, panjang);
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK CPU - KOTAK
 * ===========================================================================
 */

status_t cpu_kotak(struct p_konteks *ktx,
                   tanda32_t x, tanda32_t y,
                   tak_bertanda32_t lebar,
                   tak_bertanda32_t tinggi,
                   tak_bertanda32_t warna)
{
    if (ktx == NULL || ktx->buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (lebar == 0 || tinggi == 0) {
        return STATUS_PARAM_JARAK;
    }

    /* Empat sisi kotak */
    cpu_garis_horizontal(ktx, x, y,
                         x + (tanda32_t)lebar - 1, warna);
    cpu_garis_horizontal(ktx, x,
                         y + (tanda32_t)tinggi - 1,
                         x + (tanda32_t)lebar - 1, warna);
    cpu_garis_vertical(ktx, x, y,
                       y + (tanda32_t)tinggi - 1, warna);
    cpu_garis_vertical(ktx, x + (tanda32_t)lebar - 1, y,
                       y + (tanda32_t)tinggi - 1, warna);

    cpu_catat_statistik(ktx,
                        2 * (tak_bertanda64_t)lebar +
                        2 * (tak_bertanda64_t)tinggi - 4);
    return STATUS_BERHASIL;
}

status_t cpu_kotak_isi(struct p_konteks *ktx,
                       tanda32_t x, tanda32_t y,
                       tak_bertanda32_t lebar,
                       tak_bertanda32_t tinggi,
                       tak_bertanda32_t warna)
{
    tak_bertanda32_t iy, ix;
    tak_bertanda32_t x_aktual, y_aktual;

    if (ktx == NULL || ktx->buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (lebar == 0 || tinggi == 0) {
        return STATUS_PARAM_JARAK;
    }

    for (iy = 0; iy < tinggi; iy++) {
        y_aktual = (tak_bertanda32_t)(y + (tanda32_t)iy);
        if (y_aktual >= ktx->tinggi) {
            break;
        }

        for (ix = 0; ix < lebar; ix++) {
            x_aktual = (tak_bertanda32_t)(x + (tanda32_t)ix);
            if (x_aktual >= ktx->lebar) {
                break;
            }
            p_tulis_piksel(ktx, (tanda32_t)x_aktual,
                           (tanda32_t)y_aktual, warna);
        }
    }

    cpu_catat_statistik(ktx,
                        (tak_bertanda64_t)lebar *
                        (tak_bertanda64_t)tinggi);
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK CPU - LINGKARAN
 * ===========================================================================
 */

status_t cpu_lingkaran(struct p_konteks *ktx,
                       tanda32_t cx, tanda32_t cy,
                       tak_bertanda32_t jari_jari,
                       tak_bertanda32_t warna)
{
    tanda32_t x, y;
    tanda32_t p;

    if (ktx == NULL || ktx->buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (jari_jari == 0) {
        return STATUS_PARAM_JARAK;
    }

    /* Algoritma Midpoint Circle */
    x = 0;
    y = (tanda32_t)jari_jari;
    p = 1 - (tanda32_t)jari_jari;

    cpu_lingkaran_plot_8(ktx, cx, cy,
                         (tak_bertanda32_t)x,
                         (tak_bertanda32_t)y, warna);

    while (x < y) {
        x++;
        if (p < 0) {
            p += 2 * x + 1;
        } else {
            y--;
            p += 2 * (x - y) + 1;
        }
        cpu_lingkaran_plot_8(ktx, cx, cy,
                             (tak_bertanda32_t)x,
                             (tak_bertanda32_t)y, warna);
    }

    cpu_catat_statistik(ktx,
                        8 * (tak_bertanda64_t)jari_jari);
    return STATUS_BERHASIL;
}

status_t cpu_lingkaran_isi(struct p_konteks *ktx,
                           tanda32_t cx, tanda32_t cy,
                           tak_bertanda32_t jari_jari,
                           tak_bertanda32_t warna)
{
    tanda32_t x, y;
    tanda32_t p;

    if (ktx == NULL || ktx->buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (jari_jari == 0) {
        return STATUS_PARAM_JARAK;
    }

    x = 0;
    y = (tanda32_t)jari_jari;
    p = 1 - (tanda32_t)jari_jari;

    /* Gambar baris tengah */
    cpu_lingkaran_garis_horisontal(
        ktx, cx, cy, (tak_bertanda32_t)x,
        (tak_bertanda32_t)y, warna);

    while (x < y) {
        x++;
        if (p < 0) {
            p += 2 * x + 1;
        } else {
            y--;
            p += 2 * (x - y) + 1;
        }

        /* Gambar 4 garis horizontal simetris */
        cpu_lingkaran_garis_horisontal(
            ktx, cx, cy + (tanda32_t)x,
            (tak_bertanda32_t)y,
            (tak_bertanda32_t)y, warna);
        cpu_lingkaran_garis_horisontal(
            ktx, cx, cy - (tanda32_t)x,
            (tak_bertanda32_t)y,
            (tak_bertanda32_t)y, warna);
        cpu_lingkaran_garis_horisontal(
            ktx, cx, cy + (tanda32_t)y,
            (tak_bertanda32_t)x,
            (tak_bertanda32_t)x, warna);
        cpu_lingkaran_garis_horisontal(
            ktx, cx, cy - (tanda32_t)y,
            (tak_bertanda32_t)x,
            (tak_bertanda32_t)x, warna);
    }

    cpu_catat_statistik(ktx,
        (tak_bertanda64_t)31416 *
        (tak_bertanda64_t)jari_jari *
        (tak_bertanda64_t)jari_jari / 100000);
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK CPU - ELIP
 * ===========================================================================
 */

status_t cpu_elip(struct p_konteks *ktx,
                  tanda32_t cx, tanda32_t cy,
                  tak_bertanda32_t rx,
                  tak_bertanda32_t ry,
                  tak_bertanda32_t warna)
{
    tak_bertanda64_t rx2, ry2;
    tanda32_t x, y;
    tanda32_t px, py;

    if (ktx == NULL || ktx->buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (rx == 0 || ry == 0) {
        return STATUS_PARAM_JARAK;
    }

    rx2 = (tak_bertanda64_t)rx * rx;
    ry2 = (tak_bertanda64_t)ry * ry;
    x = 0;
    y = (tanda32_t)ry;
    px = 0;
    py = (tanda32_t)(2 * rx2 * (tak_bertanda64_t)y);

    /* Region 1 */
    cpu_elip_plot_4(ktx, cx, cy,
                    (tak_bertanda32_t)x,
                    (tak_bertanda32_t)y, warna);

    while (px < py) {
        x++;
        px += (tanda32_t)(2 * ry2);
        if (2 * (tak_bertanda64_t)rx * y -
            2 * (tak_bertanda64_t)ry * x +
            ry2 + rx2 > 0) {
            /* Tetap di region 1 */
        } else {
            y--;
            py -= (tanda32_t)(2 * rx2);
        }
        cpu_elip_plot_4(ktx, cx, cy,
                        (tak_bertanda32_t)x,
                        (tak_bertanda32_t)y, warna);
    }

    /* Region 2 */
    while (y > 0) {
        y--;
        py -= (tanda32_t)(2 * rx2);
        if (2 * (tak_bertanda64_t)ry * x -
            2 * (tak_bertanda64_t)rx * y +
            rx2 + ry2 <= 0) {
            x++;
            px += (tanda32_t)(2 * ry2);
        }
        cpu_elip_plot_4(ktx, cx, cy,
                        (tak_bertanda32_t)x,
                        (tak_bertanda32_t)y, warna);
    }

    cpu_catat_statistik(ktx,
        (tak_bertanda64_t)31416 *
        (tak_bertanda64_t)rx *
        (tak_bertanda64_t)ry / 100000);
    return STATUS_BERHASIL;
}

status_t cpu_elip_isi(struct p_konteks *ktx,
                      tanda32_t cx, tanda32_t cy,
                      tak_bertanda32_t rx,
                      tak_bertanda32_t ry,
                      tak_bertanda32_t warna)
{
    tak_bertanda32_t iy, ix;
    tak_bertanda64_t rx2, ry2;

    if (ktx == NULL || ktx->buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (rx == 0 || ry == 0) {
        return STATUS_PARAM_JARAK;
    }

    rx2 = (tak_bertanda64_t)rx * rx;
    ry2 = (tak_bertanda64_t)ry * ry;

    /* Isi per baris menggunakan persamaan elips */
    for (iy = 0; iy <= ry; iy++) {
        tanda32_t y1, y2;
        tak_bertanda64_t dy2, sisanya;
        tak_bertanda32_t x_span;
        tak_bertanda32_t x_awal, x_akhir;

        dy2 = (tak_bertanda64_t)iy * iy;
        sisanya = rx2 - (rx2 * dy2) / ry2;

        if (sisanya <= 0) {
            break;
        }

        /* Hitung x menggunakan integer sqrt */
        x_span = 1;
        while ((tak_bertanda64_t)x_span * x_span < sisanya) {
            x_span++;
        }
        if ((tak_bertanda64_t)x_span * x_span > sisanya) {
            x_span--;
        }

        y1 = cy - (tanda32_t)iy;
        y2 = cy + (tanda32_t)iy;

        x_awal = (tak_bertanda32_t)(cx - (tanda32_t)x_span);
        x_akhir = (tak_bertanda32_t)(cx + (tanda32_t)x_span);

        /* Gambar baris atas dan bawah */
        for (ix = x_awal; ix <= x_akhir; ix++) {
            p_tulis_piksel(ktx, (tanda32_t)ix, y1, warna);
            p_tulis_piksel(ktx, (tanda32_t)ix, y2, warna);
        }
    }

    cpu_catat_statistik(ktx,
        (tak_bertanda64_t)31416 *
        (tak_bertanda64_t)rx *
        (tak_bertanda64_t)ry / 100000);
    return STATUS_BERHASIL;
}

/*
 * Forward declarations tabel sinus/cosinus.
 * Diperlukan oleh cpu_busur dan cpu_kurva.
 */
tanda32_t cpu_sin_tabel(tak_bertanda32_t sudut);
tanda32_t cpu_cos_tabel(tak_bertanda32_t sudut);

/*
 * ===========================================================================
 * FUNGSI PUBLIK CPU - BUSUR
 * ===========================================================================
 */

status_t cpu_busur(struct p_konteks *ktx,
                   tanda32_t cx, tanda32_t cy,
                   tak_bertanda32_t jari_jari,
                   tanda32_t sudut_awal,
                   tanda32_t sudut_akhir,
                   tak_bertanda32_t warna)
{
    tak_bertanda32_t langkah;
    tak_bertanda32_t jumlah_langkah, i;
    tak_bertanda32_t x_sebelum, y_sebelum;
    tak_bertanda32_t x_sekarang, y_sekarang;
    tak_bertanda64_t fx, fy;
    tanda32_t sudut_sekarang;
    tak_bertanda32_t span;
    bool_t selesai;

    if (ktx == NULL || ktx->buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (jari_jari == 0) {
        return STATUS_PARAM_JARAK;
    }

    /* Normalisasi sudut ke 0-359 */
    while (sudut_awal < 0) {
        sudut_awal += 360;
    }
    while (sudut_akhir < 0) {
        sudut_akhir += 360;
    }
    sudut_awal = sudut_awal % 360;
    sudut_akhir = sudut_akhir % 360;

    langkah = jari_jari / 2;
    if (langkah < 2) {
        langkah = 2;
    }
    if (langkah > 180) {
        langkah = 180;
    }

    if (sudut_awal <= sudut_akhir) {
        jumlah_langkah =
            (sudut_akhir - sudut_awal) / langkah + 1;
    } else {
        jumlah_langkah =
            (360 - sudut_awal + sudut_akhir) / langkah + 1;
    }
    if (jumlah_langkah < 2) {
        jumlah_langkah = 2;
    }

    span = (sudut_awal <= sudut_akhir)
        ? (sudut_akhir - sudut_awal)
        : (360 - sudut_awal + sudut_akhir);

    fx = (tak_bertanda64_t)cx * 65536 +
         (tak_bertanda64_t)jari_jari *
         (tak_bertanda64_t)cpu_cos_tabel(sudut_awal);
    fy = (tak_bertanda64_t)cy * 65536 +
         (tak_bertanda64_t)jari_jari *
         (tak_bertanda64_t)cpu_sin_tabel(sudut_awal);

    x_sebelum = (tanda32_t)(fx >> 16);
    y_sebelum = (tanda32_t)(fy >> 16);

    selesai = SALAH;
    for (i = 1; i <= jumlah_langkah && !selesai; i++) {
        sudut_sekarang = sudut_awal +
            (tak_bertanda32_t)((tak_bertanda64_t)i *
                               span /
                               jumlah_langkah);

        if (sudut_awal <= sudut_akhir) {
            if (sudut_sekarang >= sudut_akhir) {
                sudut_sekarang = sudut_akhir;
                selesai = BENAR;
            }
        } else {
            if (sudut_sekarang >= 360) {
                sudut_sekarang -= 360;
            }
            if (sudut_sekarang >= sudut_akhir && i > 1) {
                selesai = BENAR;
            }
        }

        fx = (tak_bertanda64_t)cx * 65536 +
             (tak_bertanda64_t)jari_jari *
             (tak_bertanda64_t)cpu_cos_tabel(
                 sudut_sekarang);
        fy = (tak_bertanda64_t)cy * 65536 +
             (tak_bertanda64_t)jari_jari *
             (tak_bertanda64_t)cpu_sin_tabel(
                 sudut_sekarang);

        x_sekarang = (tanda32_t)(fx >> 16);
        y_sekarang = (tanda32_t)(fy >> 16);

        cpu_garis_bresenham(ktx, x_sebelum, y_sebelum,
                            x_sekarang, y_sekarang, warna);

        x_sebelum = x_sekarang;
        y_sebelum = y_sekarang;
    }

    cpu_catat_statistik(ktx,
        (tak_bertanda64_t)31416 *
        (tak_bertanda64_t)jari_jari / 360);
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL CPU - TABEL SINUS/COSINUS
 * ===========================================================================
 */

/*
 * Tabel sinus fixed-point 16.16 untuk 0-359 derajat.
 * Nilai = sin(sudut) * 65536.
 * Dipisahkan agar bisa diakses dari busur dan kurva.
 */

/*
 * cpu_sin_tabel
 * ------------
 * Mendapatkan nilai sinus fixed-point 16.16.
 * Menggunakan tabel precomputed.
 */
tanda32_t cpu_sin_tabel(tak_bertanda32_t sudut)
{
    static const tanda32_t sin_tab[36] = {
          0,   1144,   2287,   3430,   4572,   5712,
       6850,   7987,   9121,  10252,  11380,  12505,
      13626,  14742,  15855,  16962,  18064,  19161,
      20252,  21336,  22415,  23486,  24550,  25607,
      26656,  27697,  28729,  29753,  30767,  31772,
      32768,  33754,  34729,  35693,  36647,  37590
    };

    sudut = sudut % 360;
    return sin_tab[sudut / 10];
}

/*
 * cpu_cos_tabel
 * ------------
 * Mendapatkan nilai kosinus fixed-point 16.16.
 * cos(x) = sin(90 - x).
 */
tanda32_t cpu_cos_tabel(tak_bertanda32_t sudut)
{
    tak_bertanda32_t sudut_cos;

    if (sudut >= 90) {
        sudut_cos = sudut - 90;
    } else {
        sudut_cos = 90 - sudut;
    }
    return cpu_sin_tabel(sudut_cos);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK CPU - POLIGON
 * ===========================================================================
 */

status_t cpu_poligon(struct p_konteks *ktx,
                     const struct p_titik *titik,
                     tak_bertanda32_t jumlah,
                     tak_bertanda32_t warna)
{
    tak_bertanda32_t i;

    if (ktx == NULL || ktx->buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (titik == NULL || jumlah < 3) {
        return STATUS_PARAM_INVALID;
    }

    /* Gambar garis antar titik berurutan */
    for (i = 0; i < jumlah; i++) {
        tak_bertanda32_t j = (i + 1) % jumlah;
        cpu_garis(ktx, titik[i].x, titik[i].y,
                  titik[j].x, titik[j].y, warna);
    }

    cpu_catat_statistik(ktx, (tak_bertanda64_t)jumlah * 2);
    return STATUS_BERHASIL;
}

status_t cpu_poligon_isi(struct p_konteks *ktx,
                         const struct p_titik *titik,
                         tak_bertanda32_t jumlah,
                         tak_bertanda32_t warna)
{
    tak_bertanda32_t y_min, y_max, iy, i;
    tak_bertanda32_t jumlah_titik;

    if (ktx == NULL || ktx->buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (titik == NULL || jumlah < 3) {
        return STATUS_PARAM_INVALID;
    }

    /* Cari batas y */
    y_min = (tak_bertanda32_t)titik[0].y;
    y_max = y_min;
    for (i = 1; i < jumlah; i++) {
        if ((tak_bertanda32_t)titik[i].y < y_min) {
            y_min = (tak_bertanda32_t)titik[i].y;
        }
        if ((tak_bertanda32_t)titik[i].y > y_max) {
            y_max = (tak_bertanda32_t)titik[i].y;
        }
    }

    /* Scanline fill */
    for (iy = y_min; iy <= y_max; iy++) {
        tak_bertanda32_t jml_potong = 0;
        tanda32_t x_potong[64];

        /* Hitung potongan garis pada baris iy */
        for (i = 0; i < jumlah; i++) {
            tak_bertanda32_t j = (i + 1) % jumlah;
            tanda32_t y0 = titik[i].y;
            tanda32_t y1 = titik[j].y;
            tanda32_t x0, x1, x_irisan;

            if ((y0 <= (tanda32_t)iy &&
                 y1 > (tanda32_t)iy) ||
                (y1 <= (tanda32_t)iy &&
                 y0 > (tanda32_t)iy)) {

                x0 = titik[i].x;
                x1 = titik[j].x;

                /* Interpolasi x pada y=iy */
                x_irisan = x0 + (tanda32_t)(
                    ((tak_bertanda64_t)(iy - y0) *
                     (tak_bertanda64_t)(x1 - x0)) /
                    (tak_bertanda64_t)(y1 - y0));

                if (jml_potong < 64) {
                    x_potong[jml_potong++] = x_irisan;
                }
            }
        }

        /* Urutkan titik potong */
        {
            tak_bertanda32_t k, l;
            for (k = 0; k < jml_potong; k++) {
                for (l = k + 1; l < jml_potong; l++) {
                    if (x_potong[k] > x_potong[l]) {
                        tanda32_t tmp = x_potong[k];
                        x_potong[k] = x_potong[l];
                        x_potong[l] = tmp;
                    }
                }
            }
        }

        /* Gambar garis antara pasangan potong */
        {
            tak_bertanda32_t k;
            for (k = 0; k + 1 < jml_potong; k += 2) {
                tanda32_t ix;
                tanda32_t xa = x_potong[k];
                tanda32_t xb = x_potong[k + 1];

                for (ix = xa; ix <= xb; ix++) {
                    p_tulis_piksel(ktx, ix, (tanda32_t)iy,
                                   warna);
                }
            }
        }
    }

    /* Gambar garis outline */
    jumlah_titik = jumlah;
    for (i = 0; i < jumlah_titik; i++) {
        tak_bertanda32_t j = (i + 1) % jumlah_titik;
        cpu_garis(ktx, titik[i].x, titik[i].y,
                  titik[j].x, titik[j].y, warna);
    }

    cpu_catat_statistik(ktx,
        (tak_bertanda64_t)(y_max - y_min + 1) *
        (tak_bertanda64_t)jumlah);
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK CPU - KURVA BEZIER
 * ===========================================================================
 */

status_t cpu_kurva_bezier2(struct p_konteks *ktx,
                           tanda32_t x0, tanda32_t y0,
                           tanda32_t x1, tanda32_t y1,
                           tanda32_t x2, tanda32_t y2,
                           tak_bertanda32_t warna)
{
    tak_bertanda32_t i;
    tak_bertanda32_t langkah = 32;
    tanda32_t px_sebelum, py_sebelum;
    tak_bertanda32_t px_sekarang, py_sekarang;

    if (ktx == NULL || ktx->buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    px_sebelum = x0;
    py_sebelum = y0;

    /* Evaluasi kurva kuadratik: B(t) = (1-t)^2*P0 + 2(1-t)t*P1 + t^2*P2 */
    for (i = 1; i <= langkah; i++) {
        tak_bertanda32_t t_256;
        tak_bertanda64_t t_fp, satu_t_fp;
        tak_bertanda64_t bx, by;

        t_256 = (i * 256) / langkah;
        t_fp = (tak_bertanda64_t)t_256 * 257;
        satu_t_fp = 65536 - t_fp;

        /* (1-t)^2 * P0 */
        bx = (satu_t_fp * satu_t_fp *
              (tak_bertanda64_t)x0) / 65536 / 65536;
        by = (satu_t_fp * satu_t_fp *
              (tak_bertanda64_t)y0) / 65536 / 65536;

        /* 2*(1-t)*t * P1 */
        bx += (2 * satu_t_fp * t_fp *
               (tak_bertanda64_t)x1) / 65536 / 65536;
        by += (2 * satu_t_fp * t_fp *
               (tak_bertanda64_t)y1) / 65536 / 65536;

        /* t^2 * P2 */
        bx += (t_fp * t_fp *
               (tak_bertanda64_t)x2) / 65536 / 65536;
        by += (t_fp * t_fp *
               (tak_bertanda64_t)y2) / 65536 / 65536;

        px_sekarang = (tanda32_t)bx;
        py_sekarang = (tanda32_t)by;

        cpu_garis_bresenham(ktx, px_sebelum, py_sebelum,
                            px_sekarang, py_sekarang,
                            warna);

        px_sebelum = px_sekarang;
        py_sebelum = py_sekarang;
    }

    cpu_catat_statistik(ktx, (tak_bertanda64_t)langkah);
    return STATUS_BERHASIL;
}

status_t cpu_kurva_bezier3(struct p_konteks *ktx,
                           tanda32_t x0, tanda32_t y0,
                           tanda32_t x1, tanda32_t y1,
                           tanda32_t x2, tanda32_t y2,
                           tanda32_t x3, tanda32_t y3,
                           tak_bertanda32_t warna)
{
    tak_bertanda32_t i;
    tak_bertanda32_t langkah = 48;
    tanda32_t px_sebelum, py_sebelum;
    tanda32_t px_sekarang, py_sekarang;

    if (ktx == NULL || ktx->buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    px_sebelum = x0;
    py_sebelum = y0;

    /* Evaluasi kurva kubik:
     * B(t)=(1-t)^3*P0+3(1-t)^2*t*P1
     * +3(1-t)*t^2*P2+t^3*P3 */
    for (i = 1; i <= langkah; i++) {
        tak_bertanda32_t t_256;
        tak_bertanda64_t t_fp, satu_t_fp;
        tak_bertanda64_t bx, by;

        t_256 = (i * 256) / langkah;
        t_fp = (tak_bertanda64_t)t_256 * 257;
        satu_t_fp = 65536 - t_fp;

        bx = (satu_t_fp * satu_t_fp * satu_t_fp *
              (tak_bertanda64_t)x0) /
             65536 / 65536 / 65536;
        by = (satu_t_fp * satu_t_fp * satu_t_fp *
              (tak_bertanda64_t)y0) /
             65536 / 65536 / 65536;

        bx += (3 * satu_t_fp * satu_t_fp * t_fp *
               (tak_bertanda64_t)x1) /
             65536 / 65536 / 65536;
        by += (3 * satu_t_fp * satu_t_fp * t_fp *
               (tak_bertanda64_t)y1) /
             65536 / 65536 / 65536;

        bx += (3 * satu_t_fp * t_fp * t_fp *
               (tak_bertanda64_t)x2) /
             65536 / 65536 / 65536;
        by += (3 * satu_t_fp * t_fp * t_fp *
               (tak_bertanda64_t)y2) /
             65536 / 65536 / 65536;

        bx += (t_fp * t_fp * t_fp *
               (tak_bertanda64_t)x3) /
             65536 / 65536 / 65536;
        by += (t_fp * t_fp * t_fp *
               (tak_bertanda64_t)y3) /
             65536 / 65536 / 65536;

        px_sekarang = (tanda32_t)bx;
        py_sekarang = (tanda32_t)by;

        cpu_garis_bresenham(ktx, px_sebelum, py_sebelum,
                            px_sekarang, py_sekarang,
                            warna);

        px_sebelum = px_sekarang;
        py_sebelum = py_sekarang;
    }

    cpu_catat_statistik(ktx, (tak_bertanda64_t)langkah);
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK CPU - ISI AREA (FLOOD FILL)
 * ===========================================================================
 */

status_t cpu_isi_area(struct p_konteks *ktx,
                      tanda32_t x, tanda32_t y,
                      tak_bertanda32_t warna_isi,
                      tak_bertanda32_t warna_batas)
{
    tak_bertanda32_t *tumpukan_x;
    tak_bertanda32_t *tumpukan_y;
    tak_bertanda32_t tumpukan_idx;
    tak_bertanda32_t tumpukan_maks;
    tak_bertanda32_t warna_target;
    tak_bertanda64_t piksel_diproses;

    if (ktx == NULL || ktx->buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (!p_dalam_buffer(ktx, x, y)) {
        return STATUS_PARAM_JARAK;
    }

    warna_target = p_baca_piksel(ktx, x, y);
    if (warna_target == warna_isi) {
        return STATUS_BERHASIL;
    }
    if (warna_target == warna_batas) {
        return STATUS_BERHASIL;
    }

    /* Alokasi tumpukan untuk flood fill */
    tumpukan_maks = ktx->lebar * ktx->tinggi;
    if (tumpukan_maks > 262144) {
        tumpukan_maks = 262144;
    }

    tumpukan_x = (tak_bertanda32_t *)kmalloc(
        tumpukan_maks * sizeof(tak_bertanda32_t));
    if (tumpukan_x == NULL) {
        return STATUS_MEMORI_TIDAK_CUKUP;
    }

    tumpukan_y = (tak_bertanda32_t *)kmalloc(
        tumpukan_maks * sizeof(tak_bertanda32_t));
    if (tumpukan_y == NULL) {
        kfree(tumpukan_x);
        return STATUS_MEMORI_TIDAK_CUKUP;
    }

    tumpukan_x[0] = (tak_bertanda32_t)x;
    tumpukan_y[0] = (tak_bertanda32_t)y;
    tumpukan_idx = 0;
    piksel_diproses = 0;

    while (tumpukan_idx < tumpukan_maks) {
        tak_bertanda32_t cx, cy;
        tak_bertanda32_t cw;
        tanda32_t arah, langkah;

        cx = tumpukan_x[tumpukan_idx];
        cy = tumpukan_y[tumpukan_idx];
        tumpukan_idx++;

        cw = p_baca_piksel(ktx, (tanda32_t)cx, (tanda32_t)cy);
        if (cw != warna_target) {
            continue;
        }

        p_tulis_piksel(ktx, (tanda32_t)cx, (tanda32_t)cy,
                       warna_isi);
        piksel_diproses++;

        /* Cek 4 arah tetangga */
        for (arah = 0; arah < 4; arah++) {
            tanda32_t nx, ny;

            switch (arah) {
            case 0: nx = (tanda32_t)cx + 1; ny = (tanda32_t)cy; break;
            case 1: nx = (tanda32_t)cx - 1; ny = (tanda32_t)cy; break;
            case 2: nx = (tanda32_t)cx; ny = (tanda32_t)cy + 1; break;
            default: nx = (tanda32_t)cx; ny = (tanda32_t)cy - 1; break;
            }

            if (!p_dalam_buffer(ktx, nx, ny)) {
                continue;
            }

            if (p_baca_piksel(ktx, nx, ny) != warna_target) {
                continue;
            }

            if (p_baca_piksel(ktx, nx, ny) == warna_batas) {
                continue;
            }

            if (tumpukan_idx >= tumpukan_maks) {
                break;
            }

            tumpukan_x[tumpukan_idx] = (tak_bertanda32_t)nx;
            tumpukan_y[tumpukan_idx] = (tak_bertanda32_t)ny;
            tumpukan_idx++;
        }

        (void)langkah;
    }

    kfree(tumpukan_x);
    kfree(tumpukan_y);

    cpu_catat_statistik(ktx, piksel_diproses);
    return STATUS_BERHASIL;
}
