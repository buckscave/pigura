/*
 * PIGURA OS - VIDEO_INIT.C
 * =========================
 * Inisialisasi perangkat tampilan video.
 *
 * Ini adalah HARDWARE INIT yang mendeteksi dan menginisialisasi
 * controller video (VBE atau UEFI GOP).
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../../cpu/cpu.h"

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

typedef enum {
    VIDEO_SOURCE_NONE = 0,
    VIDEO_SOURCE_VBE,
    VIDEO_SOURCE_UEFI_GOP,
    VIDEO_SOURCE_LEGACY_VGA
} video_source_t;

static video_source_t g_video_source = VIDEO_SOURCE_NONE;
static bool_t g_video_initialized = SALAH;

/* Info framebuffer dari hardware */
static tak_bertanda64_t g_fb_physical_addr = 0;
static ukuran_t g_fb_size = 0;
static tak_bertanda32_t g_fb_width = 0;
static tak_bertanda32_t g_fb_height = 0;
static tak_bertanda32_t g_fb_bpp = 0;
static tak_bertanda32_t g_fb_pitch = 0;

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

status_t video_init_from_multiboot(void *mb_info)
{
    /*
     * Inisialisasi dari informasi multiboot.
     * Multiboot menyediakan info framebuffer dari bootloader (GRUB, dll).
     */
    if (mb_info == NULL) {
        return STATUS_PARAM_KOSONG;
    }
    
    if (g_video_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
    /* Parse multiboot info untuk mendapatkan framebuffer info */
    /* Placeholder - implementasi penuh memerlukan parsing struktur multiboot */
    
    g_video_source = VIDEO_SOURCE_VBE;
    g_video_initialized = BENAR;
    
    return STATUS_BERHASIL;
}

status_t video_init_from_uefi(void *gop_handle)
{
    /*
     * Inisialisasi dari UEFI GOP handle.
     * UEFI menyediakan GOP interface yang sudah dikonfigurasi.
     */
    if (gop_handle == NULL) {
        return STATUS_PARAM_KOSONG;
    }
    
    if (g_video_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
    status_t status = uefi_gop_init(gop_handle);
    if (status != STATUS_BERHASIL) {
        return status;
    }
    
    /* Dapatkan info dari GOP */
    status = uefi_gop_get_info(gop_handle, &g_fb_width, &g_fb_height,
                                &g_fb_bpp, &g_fb_physical_addr, &g_fb_size);
    if (status != STATUS_BERHASIL) {
        return status;
    }
    
    g_fb_pitch = g_fb_width * (g_fb_bpp / 8);
    g_video_source = VIDEO_SOURCE_UEFI_GOP;
    g_video_initialized = BENAR;
    
    return STATUS_BERHASIL;
}

status_t video_init_vesa(tak_bertanda32_t width, tak_bertanda32_t height,
                          tak_bertanda32_t bpp)
{
    /*
     * Inisialisasi menggunakan VESA BIOS Extensions.
     * Memerlukan BIOS dan berjalan di real mode atau menggunakan vm86.
     */
    if (g_video_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
    status_t status = vbe_init();
    if (status != STATUS_BERHASIL) {
        return status;
    }
    
    /* Cari mode yang sesuai */
    tak_bertanda32_t mode = vbe_find_mode(width, height, bpp);
    if (mode == 0) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Set mode */
    status = vbe_set_mode((tak_bertanda16_t)mode);
    if (status != STATUS_BERHASIL) {
        return status;
    }
    
    g_fb_width = width;
    g_fb_height = height;
    g_fb_bpp = bpp;
    g_fb_pitch = width * (bpp / 8);
    g_fb_physical_addr = vbe_get_framebuffer_address();
    g_fb_size = g_fb_pitch * height;
    
    g_video_source = VIDEO_SOURCE_VBE;
    g_video_initialized = BENAR;
    
    return STATUS_BERHASIL;
}

status_t video_init_legacy(tak_bertanda32_t width, tak_bertanda32_t height)
{
    /*
     * Inisialisasi mode VGA legacy.
     * Untuk fallback jika VBE/UEFI tidak tersedia.
     */
    if (g_video_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
    (void)width;
    (void)height;
    
    /* VGA mode 0x13 = 320x200x256 */
    g_fb_width = 320;
    g_fb_height = 200;
    g_fb_bpp = 8;
    g_fb_pitch = 320;
    g_fb_physical_addr = 0xA0000;  /* VGA framebuffer */
    g_fb_size = 64000;
    
    g_video_source = VIDEO_SOURCE_LEGACY_VGA;
    g_video_initialized = BENAR;
    
    return STATUS_BERHASIL;
}

status_t video_get_info(tak_bertanda64_t *fb_addr, ukuran_t *fb_size,
                         tak_bertanda32_t *width, tak_bertanda32_t *height,
                         tak_bertanda32_t *bpp, tak_bertanda32_t *pitch)
{
    if (!g_video_initialized) {
        return STATUS_BELUM_SIAP;
    }
    
    if (fb_addr != NULL) {
        *fb_addr = g_fb_physical_addr;
    }
    if (fb_size != NULL) {
        *fb_size = g_fb_size;
    }
    if (width != NULL) {
        *width = g_fb_width;
    }
    if (height != NULL) {
        *height = g_fb_height;
    }
    if (bpp != NULL) {
        *bpp = g_fb_bpp;
    }
    if (pitch != NULL) {
        *pitch = g_fb_pitch;
    }
    
    return STATUS_BERHASIL;
}

bool_t video_is_initialized(void)
{
    return g_video_initialized;
}

video_source_t video_get_source(void)
{
    return g_video_source;
}

void video_shutdown(void)
{
    switch (g_video_source) {
    case VIDEO_SOURCE_VBE:
        vbe_shutdown();
        break;
    case VIDEO_SOURCE_UEFI_GOP:
        uefi_gop_shutdown();
        break;
    default:
        break;
    }
    
    g_video_source = VIDEO_SOURCE_NONE;
    g_video_initialized = SALAH;
    g_fb_physical_addr = 0;
    g_fb_size = 0;
    g_fb_width = 0;
    g_fb_height = 0;
    g_fb_bpp = 0;
    g_fb_pitch = 0;
}
