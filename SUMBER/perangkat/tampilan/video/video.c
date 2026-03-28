/*
 * PIGURA OS - VIDEO.C
 * ====================
 * Modul utama perangkat video/display.
 *
 * Versi: 1.0
 * Tanggal: 2025
 *
 * Deskripsi:
 * Modul ini mengelola perangkat video/display hardware seperti
 * VBE (VESA BIOS Extensions) dan UEFI GOP.
 */

#include "../cpu/cpu.h"

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static bool_t g_video_initialized = SALAH;

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

status_t video_init(void)
{
    status_t status;

    /* Inisialisasi VBE */
    status = vbe_init();
    if (status != STATUS_BERHASIL) {
        /* VBE tidak tersedia, coba GOP */
    }

    /* Inisialisasi UEFI GOP */
    status = uefi_gop_init();
    if (status != STATUS_BERHASIL) {
        /* GOP tidak tersedia */
    }

    g_video_initialized = BENAR;
    return STATUS_BERHASIL;
}

status_t video_get_info(void *info)
{
    if (!g_video_initialized) {
        return STATUS_TIDAK_SIAP;
    }

    return STATUS_TIDAK_DUKUNG;
}

status_t video_set_mode(tak_bertanda32_t width, tak_bertanda32_t height,
                        tak_bertanda32_t bpp)
{
    if (!g_video_initialized) {
        return STATUS_TIDAK_SIAP;
    }

    /* Coba VBE dulu */
    tak_bertanda32_t mode = vbe_find_mode(width, height, bpp);
    if (mode != 0) {
        return vbe_set_mode((tak_bertanda16_t)mode);
    }

    return STATUS_TIDAK_DUKUNG;
}
