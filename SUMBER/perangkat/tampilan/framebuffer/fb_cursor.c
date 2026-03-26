/*
 * PIGURA OS - FB_CURSOR.C
 * ========================
 * Implementasi framebuffer cursor.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../cpu/cpu.h"

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static tak_bertanda32_t g_cursor_x = 0;
static tak_bertanda32_t g_cursor_y = 0;
static bool_t g_cursor_visible = BENAR;
static tak_bertanda32_t g_cursor_color = 0xFFFFFF;

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

status_t fb_cursor_init(void)
{
    g_cursor_x = 0;
    g_cursor_y = 0;
    g_cursor_visible = BENAR;
    return STATUS_BERHASIL;
}

void fb_cursor_set_position(tak_bertanda32_t x, tak_bertanda32_t y)
{
    g_cursor_x = x;
    g_cursor_y = y;
}

void fb_cursor_get_position(tak_bertanda32_t *x, tak_bertanda32_t *y)
{
    if (x != NULL) {
        *x = g_cursor_x;
    }
    if (y != NULL) {
        *y = g_cursor_y;
    }
}

void fb_cursor_show(bool_t visible)
{
    g_cursor_visible = visible;
}

void fb_cursor_set_color(tak_bertanda32_t color)
{
    g_cursor_color = color;
}

void fb_cursor_draw(void)
{
    /* Draw cursor at current position */
}
