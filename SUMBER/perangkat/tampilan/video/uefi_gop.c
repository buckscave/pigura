/*
 * PIGURA OS - UEFI_GOP.C
 * =======================
 * Driver UEFI Graphics Output Protocol untuk inisialisasi tampilan.
 *
 * Ini adalah HARDWARE DRIVER yang berkomunikasi dengan UEFI firmware
 * untuk mendapatkan akses framebuffer.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../../cpu/cpu.h"

/*
 * ===========================================================================
 * KONSTANTA UEFI GOP
 * ===========================================================================
 */

#define EFI_SUCCESS                 0
#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID  { 0x9042a9de, 0x23dc, 0x4a38, { 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a } }

#define PIXEL_FORMAT_RGB8           0
#define PIXEL_FORMAT_BGR8           1
#define PIXEL_FORMAT_BITMASK        2
#define PIXEL_FORMAT_BLT_ONLY       3

/*
 * ===========================================================================
 * STRUKTUR DATA UEFI GOP
 * ===========================================================================
 */

typedef struct {
    tak_bertanda32_t red_mask;
    tak_bertanda32_t green_mask;
    tak_bertanda32_t blue_mask;
    tak_bertanda32_t reserved_mask;
} efi_pixel_bitmask_t;

typedef struct {
    tak_bertanda32_t version;
    tak_bertanda32_t horizontal_resolution;
    tak_bertanda32_t vertical_resolution;
    tak_bertanda32_t pixel_format;
    efi_pixel_bitmask_t pixel_information;
    tak_bertanda32_t pixels_per_scan_line;
} efi_gop_mode_info_t;

typedef struct {
    tak_bertanda32_t max_mode;
    tak_bertanda32_t mode;
    efi_gop_mode_info_t *info;
    ukuran_t size_of_info;
    tak_bertanda64_t frame_buffer_base;
    ukuran_t frame_buffer_size;
} efi_gop_mode_t;

typedef struct efi_gop {
    void *query_mode;
    void *set_mode;
    void *blt;
    efi_gop_mode_t *mode;
} efi_gop_t;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static efi_gop_t *g_gop = NULL;
static bool_t g_gop_initialized = SALAH;

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

status_t uefi_gop_init(void *gop_handle)
{
    if (g_gop_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
    g_gop = (efi_gop_t *)gop_handle;
    
    if (g_gop == NULL || g_gop->mode == NULL) {
        return STATUS_PARAM_KOSONG;
    }
    
    g_gop_initialized = BENAR;
    return STATUS_BERHASIL;
}

status_t uefi_gop_get_info(void *gop, tak_bertanda32_t *width,
                            tak_bertanda32_t *height, tak_bertanda32_t *bpp,
                            tak_bertanda64_t *fb_addr, ukuran_t *fb_size)
{
    efi_gop_t *gop_ptr;
    
    if (gop != NULL) {
        gop_ptr = (efi_gop_t *)gop;
    } else if (g_gop != NULL) {
        gop_ptr = g_gop;
    } else {
        return STATUS_BELUM_SIAP;
    }
    
    if (gop_ptr->mode == NULL || gop_ptr->mode->info == NULL) {
        return STATUS_GAGAL;
    }
    
    efi_gop_mode_info_t *info = gop_ptr->mode->info;
    
    if (width != NULL) {
        *width = info->horizontal_resolution;
    }
    if (height != NULL) {
        *height = info->vertical_resolution;
    }
    if (bpp != NULL) {
        /* UEFI GOP biasanya 32-bit */
        *bpp = 32;
    }
    if (fb_addr != NULL) {
        *fb_addr = gop_ptr->mode->frame_buffer_base;
    }
    if (fb_size != NULL) {
        *fb_size = gop_ptr->mode->frame_buffer_size;
    }
    
    return STATUS_BERHASIL;
}

status_t uefi_gop_set_mode(void *gop, tak_bertanda32_t mode)
{
    (void)gop;
    (void)mode;
    /* Memerlukan UEFI runtime services */
    return STATUS_TIDAK_DUKUNG;
}

tak_bertanda32_t uefi_gop_find_mode(void *gop, tak_bertanda32_t width,
                                     tak_bertanda32_t height,
                                     tak_bertanda32_t bpp)
{
    efi_gop_t *gop_ptr = (gop != NULL) ? (efi_gop_t *)gop : g_gop;
    
    if (gop_ptr == NULL || gop_ptr->mode == NULL) {
        return 0;
    }
    
    /* Iterasi mode yang tersedia */
    tak_bertanda32_t i;
    for (i = 0; i < gop_ptr->mode->max_mode; i++) {
        /* Query each mode - placeholder */
    }
    
    (void)width;
    (void)height;
    (void)bpp;
    return gop_ptr->mode->mode;  /* Return current mode */
}

tak_bertanda64_t uefi_gop_get_framebuffer(void)
{
    if (!g_gop_initialized || g_gop == NULL || g_gop->mode == NULL) {
        return 0;
    }
    
    return g_gop->mode->frame_buffer_base;
}

ukuran_t uefi_gop_get_framebuffer_size(void)
{
    if (!g_gop_initialized || g_gop == NULL || g_gop->mode == NULL) {
        return 0;
    }
    
    return g_gop->mode->frame_buffer_size;
}

void uefi_gop_shutdown(void)
{
    g_gop = NULL;
    g_gop_initialized = SALAH;
}
