/*
 * PIGURA OS - FB_CONSOLE.C
 * =========================
 * Implementasi framebuffer console.
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

static tak_bertanda32_t g_console_x = 0;
static tak_bertanda32_t g_console_y = 0;
static tak_bertanda32_t g_console_cols = 80;
static tak_bertanda32_t g_console_rows = 25;
static tak_bertanda32_t g_console_fg = 0xFFFFFF;
static tak_bertanda32_t g_console_bg = 0x000000;
static tak_bertanda32_t g_char_width = 8;
static tak_bertanda32_t g_char_height = 16;

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

status_t fb_console_init(tak_bertanda32_t width, tak_bertanda32_t height)
{
    g_console_cols = width / g_char_width;
    g_console_rows = height / g_char_height;
    g_console_x = 0;
    g_console_y = 0;
    return STATUS_BERHASIL;
}

void fb_console_putchar(char c)
{
    if (c == '\n') {
        g_console_x = 0;
        g_console_y++;
        
        if (g_console_y >= g_console_rows) {
            g_console_y = g_console_rows - 1;
            /* Scroll up */
        }
        return;
    }
    
    if (c == '\r') {
        g_console_x = 0;
        return;
    }
    
    if (c == '\t') {
        g_console_x = (g_console_x + 8) & ~7;
        return;
    }
    
    /* Draw character */
    
    g_console_x++;
    
    if (g_console_x >= g_console_cols) {
        g_console_x = 0;
        g_console_y++;
        
        if (g_console_y >= g_console_rows) {
            g_console_y = g_console_rows - 1;
        }
    }
}

void fb_console_print(const char *str)
{
    if (str == NULL) {
        return;
    }
    
    while (*str != '\0') {
        fb_console_putchar(*str);
        str++;
    }
}

void fb_console_clear(void)
{
    g_console_x = 0;
    g_console_y = 0;
}

void fb_console_set_color(tak_bertanda32_t fg, tak_bertanda32_t bg)
{
    g_console_fg = fg;
    g_console_bg = bg;
}
