/*
 * PIGURA OS - FB_BLIT.C
 * ======================
 * Implementasi framebuffer blit operations.
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

status_t fb_blit_init(void)
{
    return STATUS_BERHASIL;
}

void fb_blit_copy(tak_bertanda32_t src_x, tak_bertanda32_t src_y,
                   tak_bertanda32_t dst_x, tak_bertanda32_t dst_y,
                   tak_bertanda32_t w, tak_bertanda32_t h)
{
}

void fb_blit_copy_with_key(tak_bertanda32_t src_x, tak_bertanda32_t src_y,
                            tak_bertanda32_t dst_x, tak_bertanda32_t dst_y,
                            tak_bertanda32_t w, tak_bertanda32_t h,
                            tak_bertanda32_t color_key)
{
}

void fb_blit_stretch(tak_bertanda32_t src_x, tak_bertanda32_t src_y,
                      tak_bertanda32_t src_w, tak_bertanda32_t src_h,
                      tak_bertanda32_t dst_x, tak_bertanda32_t dst_y,
                      tak_bertanda32_t dst_w, tak_bertanda32_t dst_h)
{
}

void fb_blit_rotate(tak_bertanda32_t x, tak_bertanda32_t y,
                     tak_bertanda32_t w, tak_bertanda32_t h, float angle)
{
}
