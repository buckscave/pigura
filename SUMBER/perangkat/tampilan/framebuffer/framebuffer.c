/*
 * PIGURA OS - FRAMEBUFFER.C
 * ==========================
 * Implementasi sistem framebuffer.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../cpu/cpu.h"

/*
 * ===========================================================================
 * KONSTANTA
 * ===========================================================================
 */

#define FB_MAGIC          0x46425546  /* "FBUF" */
#define FB_MODE_MAKS      32

/* Format pixel */
#define FB_FORMAT_INVALID 0
#define FB_FORMAT_RGB332  1
#define FB_FORMAT_RGB565  2
#define FB_FORMAT_RGB888  3
#define FB_FORMAT_ARGB8888 4
#define FB_FORMAT_XRGB8888 5

/* Flags */
#define FB_FLAG_DOUBLE_BUFFER 0x01
#define FB_FLAG_LINEAR       0x02

/*
 * ===========================================================================
 * STRUKTUR DATA
 * ===========================================================================
 */

typedef struct {
    tak_bertanda32_t width;
    tak_bertanda32_t height;
    tak_bertanda32_t bpp;
    tak_bertanda32_t pitch;
    tak_bertanda32_t format;
    tak_bertanda64_t fb_address;
    ukuran_t fb_size;
} fb_mode_t;

typedef struct {
    tak_bertanda32_t magic;
    
    /* Mode saat ini */
    fb_mode_t mode;
    
    /* Alamat */
    tak_bertanda8_t *buffer;
    tak_bertanda8_t *back_buffer;
    
    /* Cursor */
    tak_bertanda32_t cursor_x;
    tak_bertanda32_t cursor_y;
    bool_t cursor_visible;
    
    /* Color */
    tak_bertanda32_t fg_color;
    tak_bertanda32_t bg_color;
    
    /* Flags */
    tak_bertanda32_t flags;
    
    /* Statistik */
    tak_bertanda64_t blits;
    tak_bertanda64_t fills;
    
    /* Status */
    bool_t initialized;
} framebuffer_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

static framebuffer_t g_framebuffer;
static bool_t g_fb_initialized = SALAH;

/*
 * ===========================================================================
 * FUNGSI HELPER
 * ===========================================================================
 */

static tak_bertanda32_t fb_rgb_to_pixel(tak_bertanda8_t r,
                                         tak_bertanda8_t g,
                                         tak_bertanda8_t b)
{
    switch (g_framebuffer.mode.format) {
    case FB_FORMAT_RGB332:
        return ((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6);
    case FB_FORMAT_RGB565:
        return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    case FB_FORMAT_RGB888:
        return (r << 16) | (g << 8) | b;
    case FB_FORMAT_ARGB8888:
    case FB_FORMAT_XRGB8888:
        return 0xFF000000 | (r << 16) | (g << 8) | b;
    default:
        return 0;
    }
}

static tak_bertanda8_t *fb_get_pixel(tak_bertanda32_t x, tak_bertanda32_t y)
{
    if (g_framebuffer.buffer == NULL) {
        return NULL;
    }
    
    if (x >= g_framebuffer.mode.width || y >= g_framebuffer.mode.height) {
        return NULL;
    }
    
    return g_framebuffer.buffer + (y * g_framebuffer.mode.pitch) +
           (x * (g_framebuffer.mode.bpp / 8));
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

status_t framebuffer_init(tak_bertanda64_t address,
                           tak_bertanda32_t width,
                           tak_bertanda32_t height,
                           tak_bertanda32_t bpp,
                           tak_bertanda32_t pitch)
{
    if (g_fb_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
    kernel_memset(&g_framebuffer, 0, sizeof(framebuffer_t));
    
    g_framebuffer.magic = FB_MAGIC;
    g_framebuffer.mode.width = width;
    g_framebuffer.mode.height = height;
    g_framebuffer.mode.bpp = bpp;
    g_framebuffer.mode.pitch = pitch;
    g_framebuffer.mode.fb_address = address;
    g_framebuffer.mode.fb_size = (ukuran_t)pitch * height;
    
    /* Tentukan format */
    switch (bpp) {
    case 8:
        g_framebuffer.mode.format = FB_FORMAT_RGB332;
        break;
    case 16:
        g_framebuffer.mode.format = FB_FORMAT_RGB565;
        break;
    case 24:
        g_framebuffer.mode.format = FB_FORMAT_RGB888;
        break;
    case 32:
        g_framebuffer.mode.format = FB_FORMAT_XRGB8888;
        break;
    default:
        g_framebuffer.mode.format = FB_FORMAT_INVALID;
        return STATUS_PARAM_INVALID;
    }
    
    /* Map buffer */
    g_framebuffer.buffer = (tak_bertanda8_t *)address;
    
    /* Default colors */
    g_framebuffer.fg_color = 0xFFFFFF;
    g_framebuffer.bg_color = 0x000000;
    
    /* Cursor */
    g_framebuffer.cursor_x = 0;
    g_framebuffer.cursor_y = 0;
    g_framebuffer.cursor_visible = BENAR;
    
    g_framebuffer.initialized = BENAR;
    g_fb_initialized = BENAR;
    
    return STATUS_BERHASIL;
}

status_t framebuffer_shutdown(void)
{
    if (!g_fb_initialized) {
        return STATUS_BERHASIL;
    }
    
    g_framebuffer.magic = 0;
    g_framebuffer.initialized = SALAH;
    g_fb_initialized = SALAH;
    
    return STATUS_BERHASIL;
}

void framebuffer_putpixel(tak_bertanda32_t x, tak_bertanda32_t y,
                           tak_bertanda32_t color)
{
    tak_bertanda8_t *pixel;
    
    if (!g_fb_initialized) {
        return;
    }
    
    pixel = fb_get_pixel(x, y);
    if (pixel == NULL) {
        return;
    }
    
    switch (g_framebuffer.mode.bpp) {
    case 8:
        *pixel = (tak_bertanda8_t)color;
        break;
    case 16:
        *(tak_bertanda16_t *)pixel = (tak_bertanda16_t)color;
        break;
    case 24:
        pixel[0] = color & 0xFF;
        pixel[1] = (color >> 8) & 0xFF;
        pixel[2] = (color >> 16) & 0xFF;
        break;
    case 32:
        *(tak_bertanda32_t *)pixel = color;
        break;
    }
}

tak_bertanda32_t framebuffer_getpixel(tak_bertanda32_t x,
                                       tak_bertanda32_t y)
{
    tak_bertanda8_t *pixel;
    
    if (!g_fb_initialized) {
        return 0;
    }
    
    pixel = fb_get_pixel(x, y);
    if (pixel == NULL) {
        return 0;
    }
    
    switch (g_framebuffer.mode.bpp) {
    case 8:
        return *pixel;
    case 16:
        return *(tak_bertanda16_t *)pixel;
    case 24:
        return pixel[0] | (pixel[1] << 8) | (pixel[2] << 16);
    case 32:
        return *(tak_bertanda32_t *)pixel;
    default:
        return 0;
    }
}

void framebuffer_fill(tak_bertanda32_t color)
{
    tak_bertanda32_t x;
    tak_bertanda32_t y;
    
    if (!g_fb_initialized) {
        return;
    }
    
    for (y = 0; y < g_framebuffer.mode.height; y++) {
        for (x = 0; x < g_framebuffer.mode.width; x++) {
            framebuffer_putpixel(x, y, color);
        }
    }
    
    g_framebuffer.fills++;
}

void framebuffer_fill_rect(tak_bertanda32_t x, tak_bertanda32_t y,
                            tak_bertanda32_t w, tak_bertanda32_t h,
                            tak_bertanda32_t color)
{
    tak_bertanda32_t i;
    tak_bertanda32_t j;
    
    if (!g_fb_initialized) {
        return;
    }
    
    for (j = y; j < y + h && j < g_framebuffer.mode.height; j++) {
        for (i = x; i < x + w && i < g_framebuffer.mode.width; i++) {
            framebuffer_putpixel(i, j, color);
        }
    }
}

void framebuffer_draw_line(tak_bertanda32_t x1, tak_bertanda32_t y1,
                            tak_bertanda32_t x2, tak_bertanda32_t y2,
                            tak_bertanda32_t color)
{
    tanda32_t dx;
    tanda32_t dy;
    tanda32_t sx;
    tanda32_t sy;
    tanda32_t err;
    tanda32_t e2;
    
    if (!g_fb_initialized) {
        return;
    }
    
    dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    sx = (x1 < x2) ? 1 : -1;
    sy = (y1 < y2) ? 1 : -1;
    err = dx - dy;
    
    while (1) {
        framebuffer_putpixel(x1, y1, color);
        
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

void framebuffer_draw_rect(tak_bertanda32_t x, tak_bertanda32_t y,
                            tak_bertanda32_t w, tak_bertanda32_t h,
                            tak_bertanda32_t color)
{
    if (!g_fb_initialized) {
        return;
    }
    
    /* Horizontal lines */
    framebuffer_draw_line(x, y, x + w - 1, y, color);
    framebuffer_draw_line(x, y + h - 1, x + w - 1, y + h - 1, color);
    
    /* Vertical lines */
    framebuffer_draw_line(x, y, x, y + h - 1, color);
    framebuffer_draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void framebuffer_scroll(tak_bertanda32_t lines)
{
    tak_bertanda32_t pitch;
    tak_bertanda8_t *src;
    tak_bertanda8_t *dst;
    ukuran_t size;
    
    if (!g_fb_initialized || lines == 0) {
        return;
    }
    
    pitch = g_framebuffer.mode.pitch;
    
    /* Scroll up */
    if (lines < g_framebuffer.mode.height) {
        src = g_framebuffer.buffer + (lines * pitch);
        dst = g_framebuffer.buffer;
        size = (g_framebuffer.mode.height - lines) * pitch;
        
        kernel_memmove(dst, src, size);
        
        /* Clear bottom */
        dst = g_framebuffer.buffer + 
              ((g_framebuffer.mode.height - lines) * pitch);
        kernel_memset(dst, 0, lines * pitch);
    } else {
        /* Clear all */
        kernel_memset(g_framebuffer.buffer, 0, 
                     g_framebuffer.mode.fb_size);
    }
}

void framebuffer_blit(tak_bertanda32_t dest_x, tak_bertanda32_t dest_y,
                      const void *src, tak_bertanda32_t src_w,
                      tak_bertanda32_t src_h, tak_bertanda32_t src_pitch)
{
    const tak_bertanda8_t *src_ptr;
    tak_bertanda8_t *dst_ptr;
    tak_bertanda32_t y;
    ukuran_t copy_size;
    
    if (!g_fb_initialized || src == NULL) {
        return;
    }
    
    copy_size = src_w * (g_framebuffer.mode.bpp / 8);
    
    for (y = 0; y < src_h; y++) {
        if (dest_y + y >= g_framebuffer.mode.height) {
            break;
        }
        
        src_ptr = (const tak_bertanda8_t *)src + (y * src_pitch);
        dst_ptr = g_framebuffer.buffer + 
                  ((dest_y + y) * g_framebuffer.mode.pitch) +
                  (dest_x * (g_framebuffer.mode.bpp / 8));
        
        kernel_memcpy(dst_ptr, src_ptr, copy_size);
    }
    
    g_framebuffer.blits++;
}

tak_bertanda32_t framebuffer_get_width(void)
{
    if (!g_fb_initialized) {
        return 0;
    }
    
    return g_framebuffer.mode.width;
}

tak_bertanda32_t framebuffer_get_height(void)
{
    if (!g_fb_initialized) {
        return 0;
    }
    
    return g_framebuffer.mode.height;
}

tak_bertanda32_t framebuffer_get_bpp(void)
{
    if (!g_fb_initialized) {
        return 0;
    }
    
    return g_framebuffer.mode.bpp;
}

tak_bertanda32_t framebuffer_get_pitch(void)
{
    if (!g_fb_initialized) {
        return 0;
    }
    
    return g_framebuffer.mode.pitch;
}

void *framebuffer_get_buffer(void)
{
    if (!g_fb_initialized) {
        return NULL;
    }
    
    return g_framebuffer.buffer;
}

/* Stub implementations for other framebuffer files */

status_t fb_init_from_vbe(void *vbe_info) { return STATUS_TIDAK_DUKUNG; }
status_t fb_init_from_gop(void *gop_info) { return STATUS_TIDAK_DUKUNG; }
status_t fb_set_mode(tak_bertanda32_t width, tak_bertanda32_t height,
                     tak_bertanda32_t bpp) { return STATUS_TIDAK_DUKUNG; }
void fb_cursor_move(tak_bertanda32_t x, tak_bertanda32_t y) {}
void fb_cursor_show(bool_t visible) {}
void fb_console_putchar(char c) {}
void fb_console_print(const char *str) {}
status_t fb_mode_list(fb_mode_t *modes, tak_bertanda32_t *count) {
    return STATUS_TIDAK_DUKUNG;
}
