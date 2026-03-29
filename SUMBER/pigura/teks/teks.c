/*
 * PIGURA OS - TEKS.C
 * ==================
 * Rendering teks ke permukaan Pigura OS.
 *
 * Modul ini menyediakan fungsi utama untuk menggambar
 * karakter dan string ke permukaan framebuffer.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Seluruh nama dalam Bahasa Indonesia
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../../inti/kernel.h"
#include "../framebuffer/framebuffer.h"
#include "teks.h"

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

static bool_t g_teks_diinit = SALAH;

/*
 * ===========================================================================
 * TEKS - INISIALISASI
 * ===========================================================================
 */

status_t teks_init(void)
{
    if (g_teks_diinit) {
        return STATUS_SUDAH_ADA;
    }
    g_teks_diinit = BENAR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * TEKS - SHUTDOWN
 * ===========================================================================
 */

void teks_shutdown(void)
{
    g_teks_diinit = SALAH;
}

/*
 * ===========================================================================
 * TEKS - GAMBAR KARAKTER
 * ===========================================================================
 */

status_t teks_gambar_karakter(permukaan_t *permukaan,
                              font_t *font,
                              tak_bertanda32_t x,
                              tak_bertanda32_t y,
                              char karakter,
                              tak_bertanda32_t warna)
{
    const glyph_bitmap_t *g;

    if (permukaan == NULL || font == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (!g_teks_diinit) {
        return STATUS_TIDAK_DUKUNG;
    }

    g = font_dapat_glyph(font, (tak_bertanda32_t)karakter);
    if (g == NULL) {
        /* Karakter tidak ditemukan, lewati */
        return STATUS_TIDAK_DITEMUKAN;
    }

    glyph_render_bitmap(g, permukaan, x, y, warna);
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * TEKS - GAMBAR STRING
 * ===========================================================================
 */

status_t teks_gambar_string(permukaan_t *permukaan,
                            font_t *font,
                            tak_bertanda32_t x,
                            tak_bertanda32_t y,
                            const char *teks,
                            tak_bertanda32_t warna)
{
    tak_bertanda32_t px;

    if (permukaan == NULL || font == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (teks == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (!g_teks_diinit) {
        return STATUS_TIDAK_DUKUNG;
    }

    px = x;

    while (*teks != '\0') {
        const glyph_bitmap_t *g;
        g = font_dapat_glyph(font, (tak_bertanda32_t)*teks);

        if (g != NULL) {
            glyph_render_bitmap(g, permukaan, px, y, warna);
            px += (tak_bertanda32_t)g->x_lanjut;
        }

        teks++;
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * TEKS - GAMBAR DENGAN OPSI
 * ===========================================================================
 */

status_t teks_gambar(permukaan_t *permukaan,
                     font_t *font,
                     tak_bertanda32_t x,
                     tak_bertanda32_t y,
                     const char *teks,
                     const teks_opsi_t *opsi)
{
    if (permukaan == NULL || font == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (teks == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (!g_teks_diinit) {
        return STATUS_TIDAK_DUKUNG;
    }

    if (opsi != NULL) {
        /* Gambar dengan warna depan dari opsi */
        status_t st = teks_gambar_string(
            permukaan, font, x, y,
            teks, opsi->warna_depan);

        /* Gambar garis bawah jika style meminta */
        if (st == STATUS_BERHASIL &&
            (opsi->style & TEKS_STYLE_GARIS_BAWAH) != 0) {
            tak_bertanda32_t lebar;
            lebar = ukuran_string(font, teks);
            if (lebar > 0) {
                tak_bertanda32_t j;
                tak_bertanda32_t gx;
                font_metrik_t m;
                tak_bertanda32_t garis_y;

                if (font_dapat_metrik(font, &m) ==
                    STATUS_BERHASIL) {
                    garis_y = y + (tak_bertanda32_t)
                        m.tinggi_baris - 1;
                } else {
                    garis_y = y + 8;
                }
                for (gx = x; gx < x + lebar; gx++) {
                    for (j = 0; j < 1; j++) {
                        permukaan_tulis_piksel(
                            permukaan, gx,
                            garis_y + j,
                            opsi->warna_depan);
                    }
                }
            }
        }

        return st;
    }

    /* Tanpa opsi, gunakan default putih */
    return teks_gambar_string(permukaan, font, x, y,
                              teks, 0xFFFFFFFF);
}

/*
 * ===========================================================================
 * TEKS - GAMBAR POTONG (CLIPPED)
 * ===========================================================================
 */

status_t teks_gambar_potong(permukaan_t *permukaan,
                            font_t *font,
                            tak_bertanda32_t x,
                            tak_bertanda32_t y,
                            const char *teks,
                            tak_bertanda32_t warna,
                            tak_bertanda32_t lebar_maks)
{
    tak_bertanda32_t px = x;
    tak_bertanda32_t lebar_permukaan;

    if (permukaan == NULL || font == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (teks == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (!g_teks_diinit) {
        return STATUS_TIDAK_DUKUNG;
    }

    lebar_permukaan = permukaan_lebar(permukaan);
    if (lebar_maks == 0 || lebar_maks > lebar_permukaan) {
        lebar_maks = lebar_permukaan;
    }

    while (*teks != '\0' && px < x + lebar_maks) {
        const glyph_bitmap_t *g;
        g = font_dapat_glyph(font, (tak_bertanda32_t)*teks);

        if (g != NULL && px + (tak_bertanda32_t)g->x_lanjut
            <= x + lebar_maks) {
            glyph_render_bitmap(g, permukaan, px, y, warna);
            px += (tak_bertanda32_t)g->x_lanjut;
        }

        teks++;
    }

    return STATUS_BERHASIL;
}
