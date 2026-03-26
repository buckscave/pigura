/*
 * PIGURA OS - FB_MODE.C
 * ======================
 * Implementasi framebuffer mode management.
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

#define FB_MODE_COUNT 16

/*
 * ===========================================================================
 * STRUKTUR DATA
 * ===========================================================================
 */

typedef struct {
    tak_bertanda32_t width;
    tak_bertanda32_t height;
    tak_bertanda32_t bpp;
    tak_bertanda32_t refresh;
    bool_t supported;
} fb_mode_entry_t;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static fb_mode_entry_t g_modes[FB_MODE_COUNT];
static tak_bertanda32_t g_mode_count = 0;
static tak_bertanda32_t g_current_mode = 0;

/* Standard VESA modes */
static const fb_mode_entry_t g_standard_modes[] = {
    { 640,  480,  16, 60, BENAR },
    { 640,  480,  32, 60, BENAR },
    { 800,  600,  16, 60, BENAR },
    { 800,  600,  32, 60, BENAR },
    { 1024, 768,  16, 60, BENAR },
    { 1024, 768,  32, 60, BENAR },
    { 1280, 720,  32, 60, BENAR },
    { 1280, 1024, 32, 60, BENAR },
    { 1920, 1080, 32, 60, BENAR },
    { 0, 0, 0, 0, SALAH }
};

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

status_t fb_mode_init(void)
{
    tak_bertanda32_t i;
    
    kernel_memset(g_modes, 0, sizeof(g_modes));
    
    i = 0;
    while (g_standard_modes[i].width != 0 && i < FB_MODE_COUNT) {
        g_modes[i] = g_standard_modes[i];
        i++;
    }
    
    g_mode_count = i;
    
    return STATUS_BERHASIL;
}

tak_bertanda32_t fb_mode_get_count(void)
{
    return g_mode_count;
}

status_t fb_mode_get_info(tak_bertanda32_t index,
                           tak_bertanda32_t *width, tak_bertanda32_t *height,
                           tak_bertanda32_t *bpp)
{
    if (index >= g_mode_count) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    if (width != NULL) {
        *width = g_modes[index].width;
    }
    if (height != NULL) {
        *height = g_modes[index].height;
    }
    if (bpp != NULL) {
        *bpp = g_modes[index].bpp;
    }
    
    return STATUS_BERHASIL;
}

status_t fb_mode_set(tak_bertanda32_t index)
{
    if (index >= g_mode_count) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    g_current_mode = index;
    
    return STATUS_TIDAK_DUKUNG;
}

tak_bertanda32_t fb_mode_get_current(void)
{
    return g_current_mode;
}
