/*
 * PIGURA OS - PERMUKAAN_RENDER.C
 * ================================
 * Rendering primitif ke permukaan.
 *
 * Menyediakan fungsi gambar garis, kotak, dan lingkaran
 * menggunakan algoritma standar (Bresenham, midpoint).
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Seluruh nama dalam Bahasa Indonesia
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../../inti/kernel.h"
#include "framebuffer.h"

/*
 * ===========================================================================
 * GAMBAR GARIS - Algoritma Bresenham
 * ===========================================================================
 */

void permukaan_garis(permukaan_t *p, tak_bertanda32_t x1,
                     tak_bertanda32_t y1,
                     tak_bertanda32_t x2,
                     tak_bertanda32_t y2,
                     tak_bertanda32_t warna)
{
    /* Bresenham's line algorithm */
    tanda32_t dx = (x2 > x1) ? (tanda32_t)(x2 - x1)
                              : (tanda32_t)(x1 - x2);
    tanda32_t dy = (y2 > y1) ? (tanda32_t)(y2 - y1)
                              : (tanda32_t)(y1 - y2);
    tanda32_t sx = (x1 < x2) ? 1 : -1;
    tanda32_t sy = (y1 < y2) ? 1 : -1;
    tanda32_t err = dx - dy;

    if (p == NULL) {
        return;
    }

    while (1) {
        permukaan_tulis_piksel(p, x1, y1, warna);
        if (x1 == x2 && y1 == y2) {
            break;
        }
        {
            tanda32_t e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x1 += (tak_bertanda32_t)sx;
            }
            if (e2 < dx) {
                err += dx;
                y1 += (tak_bertanda32_t)sy;
            }
        }
    }
}

/*
 * ===========================================================================
 * GAMBAR KOTAK - Outline atau isi penuh
 * ===========================================================================
 */

void permukaan_kotak(permukaan_t *p, tak_bertanda32_t x,
                     tak_bertanda32_t y,
                     tak_bertanda32_t w, tak_bertanda32_t h,
                     tak_bertanda32_t warna, bool_t isi)
{
    if (p == NULL) {
        return;
    }

    if (w == 0 || h == 0) {
        return;
    }

    if (isi) {
        permukaan_isi_kotak(p, x, y, w, h, warna);
    } else {
        /* Gambar outline: 4 garis tepi */
        permukaan_garis(p, x, y, x + w - 1, y, warna);
        permukaan_garis(
            p, x, y + h - 1, x + w - 1, y + h - 1, warna);
        permukaan_garis(p, x, y, x, y + h - 1, warna);
        permukaan_garis(
            p, x + w - 1, y, x + w - 1, y + h - 1, warna);
    }
}

/*
 * ===========================================================================
 * GAMBAR LINGKARAN - Algoritma Midpoint Circle
 * ===========================================================================
 */

void permukaan_lingkaran(permukaan_t *p,
                         tak_bertanda32_t cx,
                         tak_bertanda32_t cy,
                         tak_bertanda32_t radius,
                         tak_bertanda32_t warna,
                         bool_t isi)
{
    /* Midpoint circle algorithm */
    tanda32_t x_pos = (tanda32_t)radius - 1;
    tanda32_t y_pos = 0;
    tanda32_t d_x = 1;
    tanda32_t d_y = 1;
    tanda32_t err = d_x - ((tanda32_t)radius << 1);

    if (p == NULL || radius == 0) {
        return;
    }

    while (x_pos >= y_pos) {
        if (isi) {
            /* Gambar garis horizontal untuk isi */
            permukaan_garis(p,
                cx - (tak_bertanda32_t)x_pos,
                cy + (tak_bertanda32_t)y_pos,
                cx + (tak_bertanda32_t)x_pos,
                cy + (tak_bertanda32_t)y_pos, warna);
            permukaan_garis(p,
                cx - (tak_bertanda32_t)x_pos,
                cy - (tak_bertanda32_t)y_pos,
                cx + (tak_bertanda32_t)x_pos,
                cy - (tak_bertanda32_t)y_pos, warna);
            permukaan_garis(p,
                cx - (tak_bertanda32_t)y_pos,
                cy + (tak_bertanda32_t)x_pos,
                cx + (tak_bertanda32_t)y_pos,
                cy + (tak_bertanda32_t)x_pos, warna);
            permukaan_garis(p,
                cx - (tak_bertanda32_t)y_pos,
                cy - (tak_bertanda32_t)x_pos,
                cx + (tak_bertanda32_t)y_pos,
                cy - (tak_bertanda32_t)x_pos, warna);
        } else {
            /* Gambar 8 titik simetri */
            permukaan_tulis_piksel(p,
                cx + (tak_bertanda32_t)x_pos,
                cy + (tak_bertanda32_t)y_pos, warna);
            permukaan_tulis_piksel(p,
                cx + (tak_bertanda32_t)y_pos,
                cy + (tak_bertanda32_t)x_pos, warna);
            permukaan_tulis_piksel(p,
                cx - (tak_bertanda32_t)y_pos,
                cy + (tak_bertanda32_t)x_pos, warna);
            permukaan_tulis_piksel(p,
                cx - (tak_bertanda32_t)x_pos,
                cy + (tak_bertanda32_t)y_pos, warna);
            permukaan_tulis_piksel(p,
                cx - (tak_bertanda32_t)x_pos,
                cy - (tak_bertanda32_t)y_pos, warna);
            permukaan_tulis_piksel(p,
                cx - (tak_bertanda32_t)y_pos,
                cy - (tak_bertanda32_t)x_pos, warna);
            permukaan_tulis_piksel(p,
                cx + (tak_bertanda32_t)y_pos,
                cy - (tak_bertanda32_t)x_pos, warna);
            permukaan_tulis_piksel(p,
                cx + (tak_bertanda32_t)x_pos,
                cy - (tak_bertanda32_t)y_pos, warna);
        }

        if (err <= 0) {
            y_pos++;
            err += d_y;
            d_y += 2;
        }
        if (err > 0) {
            x_pos--;
            d_x += 2;
            err += d_x - ((tanda32_t)radius << 1);
        }
    }
}
