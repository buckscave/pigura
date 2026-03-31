/*
 * PIGURA OS - GPU_RENDER.C
 * =========================
 * Implementasi GPU rendering functions.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../../cpu/cpu.h"

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

status_t gpu_render_init(void)
{
    return STATUS_BERHASIL;
}

status_t gpu_render_clear(tak_bertanda32_t dev_id, tak_bertanda32_t color)
{
    (void)dev_id;
    (void)color;
    return STATUS_TIDAK_DUKUNG;
}

status_t gpu_render_present(tak_bertanda32_t dev_id)
{
    (void)dev_id;
    return STATUS_TIDAK_DUKUNG;
}

status_t gpu_render_set_viewport(tak_bertanda32_t dev_id,
                                  tak_bertanda32_t x, tak_bertanda32_t y,
                                  tak_bertanda32_t w, tak_bertanda32_t h)
{
    (void)dev_id;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    return STATUS_TIDAK_DUKUNG;
}
