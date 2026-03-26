/*
 * PIGURA OS - GPU_MEMORI.C
 * =========================
 * Implementasi GPU memory management.
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

status_t gpu_memori_init(void)
{
    return STATUS_BERHASIL;
}

void *gpu_memori_alloc(tak_bertanda32_t dev_id, ukuran_t size,
                        tak_bertanda32_t flags)
{
    return NULL;
}

void gpu_memori_free(tak_bertanda32_t dev_id, void *ptr)
{
}

status_t gpu_memori_upload(tak_bertanda32_t dev_id, void *gpu_ptr,
                            const void *data, ukuran_t size)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t gpu_memori_download(tak_bertanda32_t dev_id, void *data,
                              const void *gpu_ptr, ukuran_t size)
{
    return STATUS_TIDAK_DUKUNG;
}

ukuran_t gpu_memori_get_vram_total(tak_bertanda32_t dev_id)
{
    return 0;
}

ukuran_t gpu_memori_get_vram_used(tak_bertanda32_t dev_id)
{
    return 0;
}
