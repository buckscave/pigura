/*
 * PIGURA OS - GLYPH.C
 * ====================
 * Operasi data glyph individual.
 *
 * Modul ini menangani pembuatan, penyalinan, penghapusan,
 * dan rendering glyph bitmap ke permukaan.
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
 * FUNGSI HELPER INTERNAL
 * ===========================================================================
 */

/*
 * Hitung jumlah byte yang diperlukan untuk bitmap glyph.
 * Setiap baris menggunakan (lebar + 7) / 8 byte.
 */
static tak_bertanda32_t hitung_ukuran_bitmap(tak_bertanda8_t lebar,
                                              tak_bertanda8_t tinggi)
{
    tak_bertanda32_t byte_per_baris;
    if (lebar == 0 || tinggi == 0) {
        return 0;
    }
    byte_per_baris = (tak_bertanda32_t)((lebar + 7) / 8);
    return byte_per_baris * (tak_bertanda32_t)tinggi;
}

/*
 * ===========================================================================
 * GLYPH BITMAP - BUAT
 * ===========================================================================
 */

status_t glyph_buat_bitmap(glyph_bitmap_t *g,
                           tak_bertanda8_t lebar,
                           tak_bertanda8_t tinggi,
                           tanda8_t x_ofset,
                           tanda8_t y_ofset,
                           tanda8_t x_lanjut,
                           const tak_bertanda8_t *data)
{
    tak_bertanda32_t ukuran;

    if (g == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memset(g, 0, sizeof(glyph_bitmap_t));

    if (lebar == 0 || tinggi == 0) {
        return STATUS_PARAM_INVALID;
    }

    ukuran = hitung_ukuran_bitmap(lebar, tinggi);
    if (ukuran == 0) {
        return STATUS_PARAM_INVALID;
    }

    if (data != NULL) {
        g->data = (tak_bertanda8_t *)kmalloc(ukuran);
        if (g->data == NULL) {
            return STATUS_MEMORI_TIDAK_CUKUP;
        }
        kernel_memcpy(g->data, data, ukuran);
    }

    g->lebar = lebar;
    g->tinggi = tinggi;
    g->x_ofset = x_ofset;
    g->y_ofset = y_ofset;
    g->x_lanjut = x_lanjut;
    g->ukuran = (tak_bertanda8_t)ukuran;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * GLYPH BITMAP - HAPUS
 * ===========================================================================
 */

void glyph_hapus_bitmap(glyph_bitmap_t *g)
{
    if (g == NULL) {
        return;
    }
    if (g->data != NULL) {
        kfree(g->data);
        g->data = NULL;
    }
    g->lebar = 0;
    g->tinggi = 0;
    g->ukuran = 0;
}

/*
 * ===========================================================================
 * GLYPH BITMAP - SALIN
 * ===========================================================================
 */

status_t glyph_salin_bitmap(const glyph_bitmap_t *src,
                            glyph_bitmap_t *dst)
{
    if (src == NULL || dst == NULL) {
        return STATUS_PARAM_NULL;
    }

    dst->lebar = src->lebar;
    dst->tinggi = src->tinggi;
    dst->x_ofset = src->x_ofset;
    dst->y_ofset = src->y_ofset;
    dst->x_lanjut = src->x_lanjut;
    dst->ukuran = src->ukuran;

    if (src->ukuran > 0 && src->data != NULL) {
        dst->data = (tak_bertanda8_t *)kmalloc(
            (ukuran_t)src->ukuran);
        if (dst->data == NULL) {
            dst->ukuran = 0;
            return STATUS_MEMORI_TIDAK_CUKUP;
        }
        kernel_memcpy(dst->data, src->data,
                      (ukuran_t)src->ukuran);
    } else {
        dst->data = NULL;
        dst->ukuran = 0;
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * GLYPH BITMAP - RENDER KE PERMUKAAN
 * ===========================================================================
 *
 * Render glyph bitmap 1-bit ke permukaan ARGB.
 * Setiap bit yang bernilai 1 digambar dengan warna.
 */

void glyph_render_bitmap(const glyph_bitmap_t *g,
                         permukaan_t *permukaan,
                         tak_bertanda32_t x,
                         tak_bertanda32_t y,
                         tak_bertanda32_t warna)
{
    tak_bertanda8_t i, j;
    tak_bertanda32_t px, py;

    if (g == NULL || permukaan == NULL) {
        return;
    }
    if (g->data == NULL || g->lebar == 0 || g->tinggi == 0) {
        return;
    }

    for (j = 0; j < g->tinggi; j++) {
        for (i = 0; i < g->lebar; i++) {
            tak_bertanda32_t byte_idx;
            tak_bertanda8_t bit_idx;
            tak_bertanda8_t byte_val;

            byte_idx = (tak_bertanda32_t)j *
                       ((g->lebar + 7) / 8) +
                       (i / 8);
            bit_idx = (tak_bertanda8_t)(7 - (i % 8));

            if (byte_idx >= g->ukuran) {
                continue;
            }

            byte_val = g->data[byte_idx];
            if ((byte_val & (1 << bit_idx)) == 0) {
                continue;
            }

            px = x + (tak_bertanda32_t)i + g->x_ofset;
            py = y + (tak_bertanda32_t)j + g->y_ofset;

            permukaan_tulis_piksel(permukaan, px, py, warna);
        }
    }
}

/*
 * ===========================================================================
 * GLYPH TERRENDER - RENDER KE PERMUKAAN
 * ===========================================================================
 *
 * Render glyph yang sudah di-render (buffer ARGB) ke permukaan.
 * Mendukung alpha blending sederhana.
 */

void glyph_render_arbg(const glyph_terrender_t *g,
                       permukaan_t *permukaan,
                       tak_bertanda32_t x,
                       tak_bertanda32_t y)
{
    tak_bertanda8_t j;
    tak_bertanda32_t px, py;

    if (g == NULL || permukaan == NULL) {
        return;
    }
    if (g->piksel == NULL || g->lebar == 0 || g->tinggi == 0) {
        return;
    }

    for (j = 0; j < g->tinggi; j++) {
        tak_bertanda8_t i;
        py = y + (tak_bertanda32_t)j + g->y_bawa;
        for (i = 0; i < g->lebar; i++) {
            tak_bertanda32_t piksel;
            tak_bertanda8_t alpha;

            px = x + (tak_bertanda32_t)i + g->x_bawa;
            piksel = g->piksel[
                (tak_bertanda32_t)j * g->lebar + i];

            /* Ekstrak komponen alpha (byte tertinggi) */
            alpha = (tak_bertanda8_t)(piksel >> 24);

            /* Lewati piksel transparan */
            if (alpha == 0) {
                continue;
            }

            /* Jika fully opaque, tulis langsung */
            if (alpha == 255) {
                permukaan_tulis_piksel(
                    permukaan, px, py, piksel);
            } else {
                /* Alpha blending sederhana */
                tak_bertanda32_t latar;
                tak_bertanda8_t r_s, g_s, b_s;
                tak_bertanda8_t r_d, g_d, b_d;
                tak_bertanda8_t r_h, g_h, b_h;

                latar = permukaan_baca_piksel(
                    permukaan, px, py);

                r_s = (tak_bertanda8_t)(piksel >> 16);
                g_s = (tak_bertanda8_t)(piksel >> 8);
                b_s = (tak_bertanda8_t)(piksel);

                r_d = (tak_bertanda8_t)(latar >> 16);
                g_d = (tak_bertanda8_t)(latar >> 8);
                b_d = (tak_bertanda8_t)(latar);

                r_h = (tak_bertanda8_t)(
                    ((tanda32_t)r_s * alpha +
                     (tanda32_t)r_d *
                     (255 - alpha)) / 255);
                g_h = (tak_bertanda8_t)(
                    ((tanda32_t)g_s * alpha +
                     (tanda32_t)g_d *
                     (255 - alpha)) / 255);
                b_h = (tak_bertanda8_t)(
                    ((tanda32_t)b_s * alpha +
                     (tanda32_t)b_d *
                     (255 - alpha)) / 255);

                permukaan_tulis_piksel(permukaan, px, py,
                    ((tak_bertanda32_t)255 << 24) |
                    ((tak_bertanda32_t)r_h << 16) |
                    ((tak_bertanda32_t)g_h << 8) |
                    (tak_bertanda32_t)b_h);
            }
        }
    }
}
