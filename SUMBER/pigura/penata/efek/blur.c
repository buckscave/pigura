/*
 * PIGURA OS - BLUR.C
 * ==================
 * Efek blur (pemburaman) subsistem penata Pigura OS.
 *
 * Implementasi box blur menggunakan fixed-point 16.16.
 * Box blur bekerja dengan mengambil rata-rata piksel
 * di sekitar piksel target berdasarkan radius kernel.
 *
 * Mendukung multi-iterasi untuk blur yang lebih halus.
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
 * MAKRO INTERNAL
 * ===========================================================================
 */

/* Ambang minimum area untuk blur (terlalu kecil = skip) */
#define BLUR_AREA_MIN   4

/* Nilai alpha channel mask (bit 24-31) */
#define ALPHA_MASK      0xFF000000U

/* Nilai alpha shift */
#define ALPHA_SHIFT     24

/*
 * ===========================================================================
 * FUNGSI HELPER - BACA DAN TULIS PIKSEL AMAN
 * ===========================================================================
 */

/*
 * Baca piksel dari permukaan dengan batas aman.
 * Mengembalikan warna piksel atau 0 jika di luar batas.
 */
static tak_bertanda32_t baca_piksel_aman(permukaan_t *p,
                                         tanda32_t x,
                                         tanda32_t y,
                                         tak_bertanda32_t lebar,
                                         tak_bertanda32_t tinggi)
{
    if (p == NULL) return 0;
    if (x < 0 || y < 0) return 0;
    if ((tak_bertanda32_t)x >= lebar) return 0;
    if ((tak_bertanda32_t)y >= tinggi) return 0;
    return permukaan_baca_piksel(p,
        (tak_bertanda32_t)x, (tak_bertanda32_t)y);
}

/*
 * Tulis piksel ke permukaan dengan batas aman.
 * Tidak melakukan apa-apa jika di luar batas.
 */
static void tulis_piksel_aman(permukaan_t *p,
                              tanda32_t x, tanda32_t y,
                              tak_bertanda32_t warna,
                              tak_bertanda32_t lebar,
                              tak_bertanda32_t tinggi)
{
    if (p == NULL) return;
    if (x < 0 || y < 0) return;
    if ((tak_bertanda32_t)x >= lebar) return;
    if ((tak_bertanda32_t)y >= tinggi) return;
    permukaan_tulis_piksel(p,
        (tak_bertanda32_t)x, (tak_bertanda32_t)y, warna);
}

/*
 * Ekstrak komponen RGBA dari piksel 32-bit.
 * Format diasumsikan: 0xAARRGGBB.
 */
static void pisah_rgba(tak_bertanda32_t piksel,
                       tak_bertanda8_t *r,
                       tak_bertanda8_t *g,
                       tak_bertanda8_t *b,
                       tak_bertanda8_t *a)
{
    if (a != NULL) *a = (tak_bertanda8_t)(piksel >> ALPHA_SHIFT);
    if (r != NULL) *r = (tak_bertanda8_t)((piksel >> 16) & 0xFFU);
    if (g != NULL) *g = (tak_bertanda8_t)((piksel >> 8) & 0xFFU);
    if (b != NULL) *b = (tak_bertanda8_t)(piksel & 0xFFU);
}

/*
 * Gabung komponen RGBA menjadi piksel 32-bit.
 */
static tak_bertanda32_t gabung_rgba(tak_bertanda8_t r,
                                    tak_bertanda8_t g,
                                    tak_bertanda8_t b,
                                    tak_bertanda8_t a)
{
    return ((tak_bertanda32_t)a << ALPHA_SHIFT) |
           ((tak_bertanda32_t)r << 16) |
           ((tak_bertanda32_t)g << 8) |
           (tak_bertanda32_t)b;
}

/*
 * ===========================================================================
 * BLUR HORIZONTAL (SATU PASS)
 * ===========================================================================
 *
 * Box blur horizontal: setiap piksel dihitung sebagai
 * rata-rata piksel dalam jendela [x - radius, x + radius].
 */

static status_t blur_horizontal(permukaan_t *target,
                                const penata_rect_t *area,
                                tak_bertanda32_t radius)
{
    tak_bertanda32_t x, y;
    tak_bertanda32_t ax, ay, aw, ah;
    tak_bertanda32_t diameter;
    tanda32_t kx;
    tak_bertanda32_t total_r, total_g, total_b, total_a;
    tak_bertanda32_t count;
    tak_bertanda32_t piksel;
    tak_bertanda8_t pr, pg, pb, pa;

    if (target == NULL || area == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (radius == 0) {
        return STATUS_BERHASIL;
    }
    if (radius > PENATA_BLUR_MAKS) {
        return STATUS_PARAM_JARAK;
    }

    ax = (area->x < 0) ? 0 : (tak_bertanda32_t)area->x;
    ay = (area->y < 0) ? 0 : (tak_bertanda32_t)area->y;
    aw = area->lebar;
    ah = area->tinggi;

    if (aw < BLUR_AREA_MIN || ah < BLUR_AREA_MIN) {
        return STATUS_BERHASIL;
    }

    diameter = (radius * 2) + 1;

    for (y = ay; y < ay + ah; y++) {
        for (x = ax; x < ax + aw; x++) {
            total_r = 0;
            total_g = 0;
            total_b = 0;
            total_a = 0;
            count = 0;

            /* Akumulasi piksel dalam jendela horizontal */
            for (kx = -(tanda32_t)radius;
                 kx <= (tanda32_t)radius; kx++) {
                tanda32_t sx = (tanda32_t)x + kx;
                piksel = baca_piksel_aman(
                    target, sx, (tanda32_t)y,
                    permukaan_lebar(target),
                    permukaan_tinggi(target));
                if (piksel != 0 || (sx >= 0 &&
                    (tak_bertanda32_t)sx <
                    permukaan_lebar(target))) {
                    pisah_rgba(piksel, &pr, &pg, &pb, &pa);
                    total_r += pr;
                    total_g += pg;
                    total_b += pb;
                    total_a += pa;
                    count++;
                }
            }

            if (count == 0) {
                count = 1;
            }

            tulis_piksel_aman(target, (tanda32_t)x,
                (tanda32_t)y,
                gabung_rgba(
                    (tak_bertanda8_t)(total_r / count),
                    (tak_bertanda8_t)(total_g / count),
                    (tak_bertanda8_t)(total_b / count),
                    (tak_bertanda8_t)(total_a / count)),
                aw, ah);
        }
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * BLUR VERTIKAL (SATU PASS)
 * ===========================================================================
 *
 * Box blur vertikal: setiap piksel dihitung sebagai
 * rata-rata piksel dalam jendela [y - radius, y + radius].
 */

static status_t blur_vertikal(permukaan_t *target,
                              const penata_rect_t *area,
                              tak_bertanda32_t radius)
{
    tak_bertanda32_t x, y;
    tak_bertanda32_t ax, ay, aw, ah;
    tanda32_t ky;
    tak_bertanda32_t total_r, total_g, total_b, total_a;
    tak_bertanda32_t count;
    tak_bertanda32_t piksel;
    tak_bertanda8_t pr, pg, pb, pa;

    if (target == NULL || area == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (radius == 0) {
        return STATUS_BERHASIL;
    }
    if (radius > PENATA_BLUR_MAKS) {
        return STATUS_PARAM_JARAK;
    }

    ax = (area->x < 0) ? 0 : (tak_bertanda32_t)area->x;
    ay = (area->y < 0) ? 0 : (tak_bertanda32_t)area->y;
    aw = area->lebar;
    ah = area->tinggi;

    if (aw < BLUR_AREA_MIN || ah < BLUR_AREA_MIN) {
        return STATUS_BERHASIL;
    }

    for (y = ay; y < ay + ah; y++) {
        for (x = ax; x < ax + aw; x++) {
            total_r = 0;
            total_g = 0;
            total_b = 0;
            total_a = 0;
            count = 0;

            /* Akumulasi piksel dalam jendela vertikal */
            for (ky = -(tanda32_t)radius;
                 ky <= (tanda32_t)radius; ky++) {
                tanda32_t sy = (tanda32_t)y + ky;
                piksel = baca_piksel_aman(
                    target, (tanda32_t)x, sy,
                    permukaan_lebar(target),
                    permukaan_tinggi(target));
                if (piksel != 0 || (sy >= 0 &&
                    (tak_bertanda32_t)sy <
                    permukaan_tinggi(target))) {
                    pisah_rgba(piksel, &pr, &pg, &pb, &pa);
                    total_r += pr;
                    total_g += pg;
                    total_b += pb;
                    total_a += pa;
                    count++;
                }
            }

            if (count == 0) {
                count = 1;
            }

            tulis_piksel_aman(target, (tanda32_t)x,
                (tanda32_t)y,
                gabung_rgba(
                    (tak_bertanda8_t)(total_r / count),
                    (tak_bertanda8_t)(total_g / count),
                    (tak_bertanda8_t)(total_b / count),
                    (tak_bertanda8_t)(total_a / count)),
                aw, ah);
        }
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - BLUR KOTAK
 * ===========================================================================
 *
 * Terapkan box blur pada area target.
 * Setiap iterasi melakukan satu pass horizontal dan vertikal.
 * Lebih banyak iterasi = blur lebih halus (mendekati Gaussian).
 */

status_t penata_blur_kotak(permukaan_t *target,
                           const penata_rect_t *area,
                           tak_bertanda32_t radius,
                           tak_bertanda32_t iterasi)
{
    tak_bertanda32_t i;
    status_t st;

    if (target == NULL || area == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (radius < PENATA_BLUR_MIN) {
        return STATUS_BERHASIL;
    }
    if (radius > PENATA_BLUR_MAKS) {
        return STATUS_PARAM_JARAK;
    }
    if (iterasi == 0) {
        iterasi = 1;
    }

    /*
     * Box blur separable: lakukan pass horizontal
     * lalu vertikal untuk setiap iterasi.
     * Pendekatan ini lebih efisien O(n * r) dibanding
     * kernel 2D yang O(n * r^2).
     */
    for (i = 0; i < iterasi; i++) {
        st = blur_horizontal(target, area, radius);
        if (st != STATUS_BERHASIL) {
            return st;
        }

        st = blur_vertikal(target, area, radius);
        if (st != STATUS_BERHASIL) {
            return st;
        }
    }

    return STATUS_BERHASIL;
}
