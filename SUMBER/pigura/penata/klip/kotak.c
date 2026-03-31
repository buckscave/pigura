/*
 * PIGURA OS - KOTAK.C
 * ==================
 * Klip persegi panjang ke persegi panjang.
 *
 * Menghitung area persegi panjang yang merupakan
 * irisan dari dua persegi panjang. Berguna untuk
 * membatasi area operasi ke area yang valid.
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
 * API PUBLIK - KLIP KOTAK
 * ===========================================================================
 *
 * Klip persegi panjang target terhadap area klip.
 * Hasilnya adalah area irisan yang berada di dalam
 * kedua persegi panjang.
 *
 * Mengembalikan BENAR jika ada irisan (tidak kosong).
 * Mengembalikan SALAH jika tidak ada irisan.
 */

bool_t penata_klip_kotak(const penata_rect_t *klip,
                         penata_rect_t *target)
{
    tanda32_t klip_x2, klip_y2;
    tanda32_t tgt_x2, tgt_y2;
    tanda32_t ix, iy, iw, ih;

    if (klip == NULL || target == NULL) {
        return SALAH;
    }

    /* Batas kanan-bawah klip */
    klip_x2 = klip->x + (tanda32_t)klip->lebar;
    klip_y2 = klip->y + (tanda32_t)klip->tinggi;

    /* Batas kanan-bawah target */
    tgt_x2 = target->x + (tanda32_t)target->lebar;
    tgt_y2 = target->y + (tanda32_t)target->tinggi;

    /* Hitung titik kiri-atas irisan */
    ix = MAKS(klip->x, target->x);
    iy = MAKS(klip->y, target->y);

    /* Hitung titik kanan-bawah irisan */
    klip_x2 = MIN(klip_x2, tgt_x2);
    klip_y2 = MIN(klip_y2, tgt_y2);

    /* Hitung lebar dan tinggi irisan */
    iw = klip_x2 - ix;
    ih = klip_y2 - iy;

    /* Cek apakah irisan valid */
    if (iw <= 0 || ih <= 0) {
        /* Tidak ada irisan */
        target->lebar = 0;
        target->tinggi = 0;
        return SALAH;
    }

    /* Perbarui target dengan hasil klip */
    target->x = ix;
    target->y = iy;
    target->lebar = (tak_bertanda32_t)iw;
    target->tinggi = (tak_bertanda32_t)ih;

    return BENAR;
}

/*
 * ===========================================================================
 * FUNGSI - NORMALISASI RECT
 * ===========================================================================
 *
 * Pastikan lebar dan tinggi rect tidak negatif.
 * Jika negatif, pertukarkan x/y dan lebar/tinggi.
 */

void penata_rect_normalisasi(penata_rect_t *r)
{
    tanda32_t __attribute__((unused)) tmp;

    if (r == NULL) return;

    if ((tanda32_t)r->lebar < 0) {
        r->x = r->x + (tanda32_t)r->lebar;
        r->lebar = (tak_bertanda32_t)(-(tanda32_t)r->lebar);
    }
    if ((tanda32_t)r->tinggi < 0) {
        r->y = r->y + (tanda32_t)r->tinggi;
        r->tinggi = (tak_bertanda32_t)(-(tanda32_t)r->tinggi);
    }
}

/*
 * ===========================================================================
 * FUNGSI - CEK APAKAH RECT KOSONG
 * ===========================================================================
 */

bool_t penata_rect_kosong(const penata_rect_t *r)
{
    if (r == NULL) return BENAR;
    if (r->lebar == 0 || r->tinggi == 0) return BENAR;
    return SALAH;
}

/*
 * ===========================================================================
 * FUNGSI - CEK APAKAH RECT MENGANDUNG TITIK
 * ===========================================================================
 */

bool_t penata_rect_mengandung(const penata_rect_t *r,
                              tanda32_t x, tanda32_t y)
{
    if (r == NULL) return SALAH;
    return penata_klip_titik(r, x, y);
}

/*
 * ===========================================================================
 * FUNGSI - CEK APAKAH DUA RECT BERIRISAN
 * ===========================================================================
 */

bool_t penata_rect_beririsan(const penata_rect_t *a,
                             const penata_rect_t *b)
{
    penata_rect_t tmp;

    if (a == NULL || b == NULL) return SALAH;

    tmp = *a;
    return penata_klip_kotak(b, &tmp);
}
