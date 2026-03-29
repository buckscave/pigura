/*
 * PIGURA OS - INTERSECT.C
 * =======================
 * Perhitungan interseksi (irisan) dua persegi panjang.
 *
 * Modul ini menghitung area persegi panjang yang merupakan
 * irisan dari dua persegi panjang input.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Seluruh nama dalam Bahasa Indonesia
 * - Mendukung: x86, x86_64, ARM, ARMv7, ARM64
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../../inti/kernel.h"
#include "../../framebuffer/framebuffer.h"
#include "../penata.h"

/*
 * ===========================================================================
 * API PUBLIK - INTERSEKSI DUA RECT
 * ===========================================================================
 *
 * Menghitung irisan dari dua persegi panjang a dan b.
 * Hasil disimpan di 'hasil'.
 *
 * Mengembalikan BENAR jika irisan tidak kosong.
 * Mengembalikan SALAH jika tidak ada irisan.
 */

bool_t penata_intersek(const penata_rect_t *a,
                       const penata_rect_t *b,
                       penata_rect_t *hasil)
{
    tanda32_t a_x2, a_y2, b_x2, b_y2;
    tanda32_t ix, iy, iw, ih;

    if (a == NULL || b == NULL || hasil == NULL) {
        return SALAH;
    }

    /* Hitung batas kanan-bawah setiap rect */
    a_x2 = a->x + (tanda32_t)a->lebar;
    a_y2 = a->y + (tanda32_t)a->tinggi;
    b_x2 = b->x + (tanda32_t)b->lebar;
    b_y2 = b->y + (tanda32_t)b->tinggi;

    /* Hitung titik kiri-atas irisan */
    ix = MAKS(a->x, b->x);
    iy = MAKS(a->y, b->y);

    /* Hitung titik kanan-bawah irisan */
    a_x2 = MIN(a_x2, b_x2);
    a_y2 = MIN(a_y2, b_y2);

    /* Hitung lebar dan tinggi irisan */
    iw = a_x2 - ix;
    ih = a_y2 - iy;

    /* Cek apakah irisan valid (tidak kosong) */
    if (iw <= 0 || ih <= 0) {
        hasil->x = 0;
        hasil->y = 0;
        hasil->lebar = 0;
        hasil->tinggi = 0;
        return SALAH;
    }

    hasil->x = ix;
    hasil->y = iy;
    hasil->lebar = (tak_bertanda32_t)iw;
    hasil->tinggi = (tak_bertanda32_t)ih;

    return BENAR;
}
