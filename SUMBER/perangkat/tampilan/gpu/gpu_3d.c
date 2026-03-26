/*
 * PIGURA OS - GPU_3D.C
 * =====================
 * Implementasi GPU 3D rendering.
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

status_t gpu_3d_init(void)
{
    return STATUS_BERHASIL;
}

status_t gpu_3d_set_projection(tak_bertanda32_t dev_id, float fov,
                                float aspect, float near, float far)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t gpu_3d_set_modelview(tak_bertanda32_t dev_id, float *matrix)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t gpu_3d_draw_vertices(tak_bertanda32_t dev_id, void *vertices,
                               ukuran_t count, tak_bertanda32_t mode)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t gpu_3d_set_texture(tak_bertanda32_t dev_id, void *texture,
                             tak_bertanda32_t width, tak_bertanda32_t height)
{
    return STATUS_TIDAK_DUKUNG;
}
