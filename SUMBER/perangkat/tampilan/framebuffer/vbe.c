/*
 * PIGURA OS - VBE.C
 * ==================
 * Implementasi VESA BIOS Extensions.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../cpu/cpu.h"

/*
 * ===========================================================================
 * KONSTANTA VBE
 * ===========================================================================
 */

#define VBE_SIGNATURE       0x41534556  /* "VESA" */
#define VBE_MODE_LFB        0x4000

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

status_t vbe_init(void)
{
    return STATUS_BERHASIL;
}

status_t vbe_get_info(void *info)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t vbe_get_mode_info(tak_bertanda16_t mode, void *info)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t vbe_set_mode(tak_bertanda16_t mode)
{
    return STATUS_TIDAK_DUKUNG;
}

tak_bertanda32_t vbe_find_mode(tak_bertanda32_t width,
                                tak_bertanda32_t height,
                                tak_bertanda32_t bpp)
{
    return 0;
}
