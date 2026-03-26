/*
 * PIGURA OS - FB_RENDER.C
 * ========================
 * Implementasi framebuffer rendering.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../cpu/cpu.h"

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

status_t fb_render_init(void)
{
    return STATUS_BERHASIL;
}

void fb_render_fill_rect(tak_bertanda32_t x, tak_bertanda32_t y,
                          tak_bertanda32_t w, tak_bertanda32_t h,
                          tak_bertanda32_t color)
{
    tak_bertanda32_t i;
    tak_bertanda32_t j;
    
    for (j = y; j < y + h; j++) {
        for (i = x; i < x + w; i++) {
            /* framebuffer_putpixel(i, j, color); */
        }
    }
}

void fb_render_draw_line(tak_bertanda32_t x1, tak_bertanda32_t y1,
                          tak_bertanda32_t x2, tak_bertanda32_t y2,
                          tak_bertanda32_t color)
{
    tanda32_t dx;
    tanda32_t dy;
    tanda32_t sx;
    tanda32_t sy;
    tanda32_t err;
    tanda32_t e2;
    
    dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    sx = (x1 < x2) ? 1 : -1;
    sy = (y1 < y2) ? 1 : -1;
    err = dx - dy;
    
    while (1) {
        /* framebuffer_putpixel(x1, y1, color); */
        
        if (x1 == x2 && y1 == y2) {
            break;
        }
        
        e2 = 2 * err;
        
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void fb_render_draw_circle(tak_bertanda32_t cx, tak_bertanda32_t cy,
                            tak_bertanda32_t radius, tak_bertanda32_t color,
                            bool_t filled)
{
    tanda32_t x;
    tanda32_t y;
    tanda32_t err;
    
    x = radius - 1;
    y = 0;
    err = 0;
    
    while (x >= y) {
        if (filled) {
            /* Draw filled circle */
        } else {
            /* Draw circle outline */
        }
        
        y++;
        err += 1 + 2 * y;
        
        if (2 * (err - x) + 1 > 0) {
            x--;
            err += 1 - 2 * x;
        }
    }
}
