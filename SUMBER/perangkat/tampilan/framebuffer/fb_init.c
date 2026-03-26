/*
 * PIGURA OS - FB_INIT.C
 * ======================
 * Implementasi inisialisasi framebuffer.
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

status_t fb_init_from_multiboot(void *mb_info)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t fb_init_vesa(tak_bertanda32_t width, tak_bertanda32_t height,
                       tak_bertanda32_t bpp)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t fb_init_uefi(void *gop)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t fb_init_legacy(tak_bertanda32_t width, tak_bertanda32_t height)
{
    return STATUS_TIDAK_DUKUNG;
}
