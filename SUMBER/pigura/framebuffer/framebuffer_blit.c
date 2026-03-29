/*
 * PIGURA OS - PERMUKAAN_BLIT.C
 * ==============================
 * Operasi blitting permukaan.
 *
 * Menyediakan fungsi copy dan stretch blit antar permukaan.
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
#include "framebuffer.h"

/*
 * ===========================================================================
 * BLIT COPY - Salin area piksel dari sumber ke tujuan
 * ===========================================================================
 */

void permukaan_blit_copy(permukaan_t *src, permukaan_t *dst,
                          tak_bertanda32_t sx, tak_bertanda32_t sy,
                          tak_bertanda32_t dx, tak_bertanda32_t dy,
                          tak_bertanda32_t w, tak_bertanda32_t h)
{
    tak_bertanda32_t j;
    tak_bertanda32_t src_lebar, dst_lebar;
    tak_bertanda32_t baris;
    tak_bertanda32_t salin_w;

    if (src == NULL || dst == NULL) {
        return;
    }

    src_lebar = permukaan_lebar(src);
    dst_lebar = permukaan_lebar(dst);

    /* Batas area salin agar tidak keluar permukaan sumber */
    if (sx >= src_lebar) {
        return;
    }
    if (sy >= permukaan_tinggi(src)) {
        return;
    }

    salin_w = w;
    if (sx + salin_w > src_lebar) {
        salin_w = src_lebar - sx;
    }
    if (dx + salin_w > dst_lebar) {
        salin_w = dst_lebar - dx;
    }

    baris = h;
    if (sy + baris > permukaan_tinggi(src)) {
        baris = permukaan_tinggi(src) - sy;
    }
    if (dy + baris > permukaan_tinggi(dst)) {
        baris = permukaan_tinggi(dst) - dy;
    }

    if (salin_w == 0 || baris == 0) {
        return;
    }

    /* Salin per baris menggunakan baca/tulis piksel */
    for (j = 0; j < baris; j++) {
        tak_bertanda32_t i;
        for (i = 0; i < salin_w; i++) {
            tak_bertanda32_t warna;
            warna = permukaan_baca_piksel(
                src, sx + i, sy + j);
            permukaan_tulis_piksel(
                dst, dx + i, dy + j, warna);
        }
    }
}

/*
 * ===========================================================================
 * BLIT STRETCH - Salin dengan skala ukuran berbeda
 * ===========================================================================
 */

void permukaan_blit_stretch(permukaan_t *src, permukaan_t *dst,
                             tak_bertanda32_t sx, tak_bertanda32_t sy,
                             tak_bertanda32_t sw, tak_bertanda32_t sh,
                             tak_bertanda32_t dx, tak_bertanda32_t dy,
                             tak_bertanda32_t dw, tak_bertanda32_t dh)
{
    tak_bertanda32_t j;
    tak_bertanda32_t src_lebar, dst_lebar;

    if (src == NULL || dst == NULL) {
        return;
    }

    src_lebar = permukaan_lebar(src);
    dst_lebar = permukaan_lebar(dst);

    /* Batas area sumber */
    if (sx >= src_lebar) {
        return;
    }
    if (sy >= permukaan_tinggi(src)) {
        return;
    }

    if (sw == 0 || sh == 0 || dw == 0 || dh == 0) {
        return;
    }

    /* Batas area tujuan */
    if (dx >= dst_lebar) {
        return;
    }
    if (dy >= permukaan_tinggi(dst)) {
        return;
    }

    if (dx + dw > dst_lebar) {
        dw = dst_lebar - dx;
    }
    if (dy + dh > permukaan_tinggi(dst)) {
        dh = permukaan_tinggi(dst) - dy;
    }

    /* Skala menggunakan fixed-point 16.16 */
    for (j = 0; j < dh; j++) {
        tak_bertanda32_t i;
        tak_bertanda32_t src_y;
        /* Fixed-point: (j * sh) / dh */
        src_y = sy + (tak_bertanda32_t)(
            ((tak_bertanda64_t)j * sh) / dh);
        for (i = 0; i < dw; i++) {
            tak_bertanda32_t src_x;
            tak_bertanda32_t warna;
            /* Fixed-point: (i * sw) / dw */
            src_x = sx + (tak_bertanda32_t)(
                ((tak_bertanda64_t)i * sw) / dw);
            warna = permukaan_baca_piksel(
                src, src_x, src_y);
            permukaan_tulis_piksel(
                dst, dx + i, dy + j, warna);
        }
    }
}
