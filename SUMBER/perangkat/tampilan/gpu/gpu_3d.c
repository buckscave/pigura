/*
 * PIGURA OS - GPU_3D.C
 * =====================
 * Implementasi GPU 3D rendering.
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

status_t gpu_3d_init(void)
{
    return STATUS_BERHASIL;
}

status_t gpu_3d_set_projection(tak_bertanda32_t dev_id, float fov,
                                float aspect, float near, float far)
{
    (void)dev_id; (void)fov; (void)aspect;
    (void)near; (void)far;
    return STATUS_TIDAK_DUKUNG;
}

status_t gpu_3d_set_modelview(tak_bertanda32_t dev_id, float *matrix)
{
    (void)dev_id; (void)matrix;
    return STATUS_TIDAK_DUKUNG;
}

status_t gpu_3d_draw_vertices(tak_bertanda32_t dev_id, void *vertices,
                               ukuran_t count, tak_bertanda32_t mode)
{
    (void)dev_id; (void)vertices; (void)count; (void)mode;
    return STATUS_TIDAK_DUKUNG;
}

status_t gpu_3d_set_texture(tak_bertanda32_t dev_id, void *texture,
                             tak_bertanda32_t width, tak_bertanda32_t height)
{
    (void)dev_id; (void)texture; (void)width; (void)height;
    return STATUS_TIDAK_DUKUNG;
}
