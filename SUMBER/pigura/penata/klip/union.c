/*
 * PIGURA OS - UNION.C
 * ==================
 * Perhitungan union (gabungan) dua persegi panjang.
 *
 * Modul ini menghitung bounding box dari dua persegi
 * panjang yang mencakup seluruh area kedua rect.
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
 * API PUBLIK - UNION DUA RECT
 * ===========================================================================
 *
 * Menghitung bounding box yang mencakup seluruh area
 * dari dua persegi panjang a dan b.
 * Hasil disimpan di 'hasil'.
 *
 * Catatan: union selalu menghasilkan rect valid (atau kosong
 * jika kedua input kosong).
 */

void penata_union(const penata_rect_t *a,
                  const penata_rect_t *b,
                  penata_rect_t *hasil)
{
    tanda32_t a_x2, a_y2, b_x2, b_y2;
    tanda32_t ux, uy, uw, uh;

    if (hasil == NULL) return;

    /* Jika salah satu NULL, gunakan yang lain */
    if (a == NULL && b == NULL) {
        hasil->x = 0;
        hasil->y = 0;
        hasil->lebar = 0;
        hasil->tinggi = 0;
        return;
    }

    if (a == NULL) {
        *hasil = *b;
        return;
    }

    if (b == NULL) {
        *hasil = *a;
        return;
    }

    /* Hitung batas kanan-bawah */
    a_x2 = a->x + (tanda32_t)a->lebar;
    a_y2 = a->y + (tanda32_t)a->tinggi;
    b_x2 = b->x + (tanda32_t)b->lebar;
    b_y2 = b->y + (tanda32_t)b->tinggi;

    /* Titik kiri-atas union */
    ux = MIN(a->x, b->x);
    uy = MIN(a->y, b->y);

    /* Titik kanan-bawah union */
    b_x2 = MAKS(a_x2, b_x2);
    b_y2 = MAKS(a_y2, b_y2);

    /* Lebar dan tinggi */
    uw = b_x2 - ux;
    uh = b_y2 - uy;

    /* Clamp agar tidak negatif */
    if (uw < 0) uw = 0;
    if (uh < 0) uh = 0;

    hasil->x = ux;
    hasil->y = uy;
    hasil->lebar = (tak_bertanda32_t)uw;
    hasil->tinggi = (tak_bertanda32_t)uh;
}
