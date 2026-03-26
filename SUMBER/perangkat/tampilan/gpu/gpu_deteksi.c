/*
 * PIGURA OS - GPU_DETEKSI.C
 * ==========================
 * Implementasi GPU detection.
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

status_t gpu_deteksi_init(void)
{
    return STATUS_BERHASIL;
}

status_t gpu_deteksi_scan(void)
{
    return STATUS_BERHASIL;
}

const char *gpu_deteksi_get_vendor_name(tak_bertanda32_t vendor_id)
{
    switch (vendor_id) {
    case 0x8086:
        return "Intel";
    case 0x10DE:
        return "NVIDIA";
    case 0x1002:
        return "AMD";
    default:
        return "Unknown";
    }
}

status_t gpu_deteksi_get_info(tak_bertanda32_t dev_id, void *info)
{
    return STATUS_TIDAK_DUKUNG;
}
