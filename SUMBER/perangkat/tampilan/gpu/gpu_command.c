/*
 * PIGURA OS - GPU_COMMAND.C
 * ==========================
 * Implementasi GPU command buffer.
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

status_t gpu_command_init(void)
{
    return STATUS_BERHASIL;
}

void *gpu_command_buffer_create(tak_bertanda32_t dev_id, ukuran_t size)
{
    (void)dev_id; (void)size;
    return NULL;
}

void gpu_command_buffer_destroy(tak_bertanda32_t dev_id, void *buffer)
{
    (void)dev_id; (void)buffer;
}

status_t gpu_command_buffer_begin(void *buffer)
{
    (void)buffer;
    return STATUS_TIDAK_DUKUNG;
}

status_t gpu_command_buffer_end(void *buffer)
{
    (void)buffer;
    return STATUS_TIDAK_DUKUNG;
}

status_t gpu_command_buffer_submit(void *buffer)
{
    (void)buffer;
    return STATUS_TIDAK_DUKUNG;
}
