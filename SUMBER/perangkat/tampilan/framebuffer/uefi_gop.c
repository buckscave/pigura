/*
 * PIGURA OS - UEFI_GOP.C
 * =======================
 * Implementasi UEFI Graphics Output Protocol.
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

status_t uefi_gop_init(void)
{
    return STATUS_BERHASIL;
}

status_t uefi_gop_get_info(void *gop, tak_bertanda32_t *width,
                            tak_bertanda32_t *height, tak_bertanda32_t *bpp,
                            tak_bertanda64_t *fb_addr, ukuran_t *fb_size)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t uefi_gop_set_mode(void *gop, tak_bertanda32_t mode)
{
    return STATUS_TIDAK_DUKUNG;
}

tak_bertanda32_t uefi_gop_find_mode(void *gop, tak_bertanda32_t width,
                                     tak_bertanda32_t height,
                                     tak_bertanda32_t bpp)
{
    return 0;
}
