/*
 * PIGURA OS - GPU_2D.C
 * =====================
 * Implementasi GPU 2D acceleration.
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

status_t gpu_2d_init(void)
{
    return STATUS_BERHASIL;
}

status_t gpu_2d_draw_line(tak_bertanda32_t dev_id,
                           tak_bertanda32_t x1, tak_bertanda32_t y1,
                           tak_bertanda32_t x2, tak_bertanda32_t y2,
                           tak_bertanda32_t color)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t gpu_2d_draw_rect(tak_bertanda32_t dev_id,
                           tak_bertanda32_t x, tak_bertanda32_t y,
                           tak_bertanda32_t w, tak_bertanda32_t h,
                           tak_bertanda32_t color, bool_t filled)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t gpu_2d_draw_circle(tak_bertanda32_t dev_id,
                             tak_bertanda32_t cx, tak_bertanda32_t cy,
                             tak_bertanda32_t radius, tak_bertanda32_t color,
                             bool_t filled)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t gpu_2d_copy(tak_bertanda32_t dev_id,
                      tak_bertanda32_t src_x, tak_bertanda32_t src_y,
                      tak_bertanda32_t dst_x, tak_bertanda32_t dst_y,
                      tak_bertanda32_t w, tak_bertanda32_t h)
{
    return STATUS_TIDAK_DUKUNG;
}
