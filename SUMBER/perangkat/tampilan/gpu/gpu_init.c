/*
 * PIGURA OS - GPU_INIT.C
 * =======================
 * Implementasi GPU initialization.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../../cpu/cpu.h"

/* Forward declarations */
extern status_t gpu_init(void);
extern status_t gpu_deteksi_init(void);
extern status_t gpu_memori_init(void);
extern status_t gpu_2d_init(void);
extern status_t gpu_3d_init(void);
extern status_t gpu_shader_init(void);
extern status_t gpu_render_init(void);
extern status_t gpu_command_init(void);

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

status_t gpu_init_all(void)
{
    status_t hasil;
    
    hasil = gpu_init();
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    hasil = gpu_deteksi_init();
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    hasil = gpu_memori_init();
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    hasil = gpu_2d_init();
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    hasil = gpu_3d_init();
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    hasil = gpu_shader_init();
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    hasil = gpu_render_init();
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    hasil = gpu_command_init();
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    return STATUS_BERHASIL;
}
