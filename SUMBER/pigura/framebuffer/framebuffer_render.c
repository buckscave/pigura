/*
 * PIGURA OS - PERMUKAAN_RENDER.C
 * ================================
 * Rendering primitif ke permukaan.
 */

#include "../../inti/kernel.h"

void permukaan_garis(permukaan_t *p, tak_bertanda32_t x1, tak_bertanda32_t y1,
                     tak_bertanda32_t x2, tak_bertanda32_t y2,
                     tak_bertanda32_t warna)
{
    /* Bresenham's line algorithm */
    tanda32_t dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    tanda32_t dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    tanda32_t sx = (x1 < x2) ? 1 : -1;
    tanda32_t sy = (y1 < y2) ? 1 : -1;
    tanda32_t err = dx - dy;
    
    while (1) {
        permukaan_tulis_piksel(p, x1, y1, warna);
        if (x1 == x2 && y1 == y2) break;
        tanda32_t e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx)  { err += dx; y1 += sy; }
    }
}

void permukaan_kotak(permukaan_t *p, tak_bertanda32_t x, tak_bertanda32_t y,
                     tak_bertanda32_t w, tak_bertanda32_t h,
                     tak_bertanda32_t warna, bool_t isi)
{
    if (isi) {
        permukaan_isi_kotak(p, x, y, w, h, warna);
    } else {
        permukaan_garis(p, x, y, x + w - 1, y, warna);
        permukaan_garis(p, x, y + h - 1, x + w - 1, y + h - 1, warna);
        permukaan_garis(p, x, y, x, y + h - 1, warna);
        permukaan_garis(p, x + w - 1, y, x + w - 1, y + h - 1, warna);
    }
}

void permukaan_lingkaran(permukaan_t *p, tak_bertanda32_t cx, tak_bertanda32_t cy,
                          tak_bertanda32_t radius, tak_bertanda32_t warna, bool_t isi)
{
    /* Midpoint circle algorithm */
    tanda32_t x = radius - 1;
    tanda32_t y = 0;
    tanda32_t dx = 1;
    tanda32_t dy = 1;
    tanda32_t err = dx - (radius << 1);
    
    while (x >= y) {
        if (isi) {
            permukaan_garis(p, cx - x, cy + y, cx + x, cy + y, warna);
            permukaan_garis(p, cx - x, cy - y, cx + x, cy - y, warna);
            permukaan_garis(p, cx - y, cy + x, cx + y, cy + x, warna);
            permukaan_garis(p, cx - y, cy - x, cx + y, cy - x, warna);
        } else {
            permukaan_tulis_piksel(p, cx + x, cy + y, warna);
            permukaan_tulis_piksel(p, cx + y, cy + x, warna);
            permukaan_tulis_piksel(p, cx - y, cy + x, warna);
            permukaan_tulis_piksel(p, cx - x, cy + y, warna);
            permukaan_tulis_piksel(p, cx - x, cy - y, warna);
            permukaan_tulis_piksel(p, cx - y, cy - x, warna);
            permukaan_tulis_piksel(p, cx + y, cy - x, warna);
            permukaan_tulis_piksel(p, cx + x, cy - y, warna);
        }
        
        if (err <= 0) {
            y++;
            err += dy;
            dy += 2;
        }
        if (err > 0) {
            x--;
            dx += 2;
            err += dx - (radius << 1);
        }
    }
}
