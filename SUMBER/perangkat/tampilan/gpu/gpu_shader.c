/*
 * PIGURA OS - GPU_SHADER.C
 * =========================
 * Implementasi GPU shader system.
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

status_t gpu_shader_init(void)
{
    return STATUS_BERHASIL;
}

tak_bertanda32_t gpu_shader_create(tak_bertanda32_t dev_id,
                                    const char *vertex_src,
                                    const char *fragment_src)
{
    return 0;
}

status_t gpu_shader_destroy(tak_bertanda32_t dev_id,
                             tak_bertanda32_t shader_id)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t gpu_shader_use(tak_bertanda32_t dev_id, tak_bertanda32_t shader_id)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t gpu_shader_set_uniform(tak_bertanda32_t dev_id,
                                 tak_bertanda32_t shader_id,
                                 const char *name, void *value,
                                 tak_bertanda32_t type)
{
    return STATUS_TIDAK_DUKUNG;
}
