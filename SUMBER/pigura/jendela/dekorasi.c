/*
 * PIGURA OS - DEKORASI.C
 * =======================
 * Dekorasi jendela Pigura OS.
 *
 * Menggambar elemen dekorasi pada jendela:
 *   - Judul bar (dengan teks judul)
 *   - Bingkai (garis tepi)
 *   - Tombol tutup, minimize, maksimalkan
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
#include "jendela.h"

/*
 * ===========================================================================
 * FUNGSI HELPER - GAMBAR GARIS HORIZONTAL
 * ===========================================================================
 */

static void gambar_garis_h(permukaan_t *p,
                           tak_bertanda32_t x,
                           tak_bertanda32_t y,
                           tak_bertanda32_t lebar,
                           tak_bertanda32_t warna)
{
    tak_bertanda32_t i;
    if (p == NULL || lebar == 0) return;
    for (i = 0; i < lebar; i++) {
        permukaan_tulis_piksel(p, x + i, y, warna);
    }
}

/*
 * ===========================================================================
 * FUNGSI HELPER - GAMBAR GARIS VERTIKAL
 * ===========================================================================
 */

static void gambar_garis_v(permukaan_t *p,
                           tak_bertanda32_t x,
                           tak_bertanda32_t y,
                           tak_bertanda32_t tinggi,
                           tak_bertanda32_t warna)
{
    tak_bertanda32_t i;
    if (p == NULL || tinggi == 0) return;
    for (i = 0; i < tinggi; i++) {
        permukaan_tulis_piksel(p, x, y + i, warna);
    }
}

/*
 * ===========================================================================
 * FUNGSI HELPER - GAMBAR PERSEGI
 * ===========================================================================
 */

static void gambar_persegi(permukaan_t *p,
                           tak_bertanda32_t x,
                           tak_bertanda32_t y,
                           tak_bertanda32_t lebar,
                           tak_bertanda32_t tinggi,
                           tak_bertanda32_t warna)
{
    if (p == NULL) return;
    /* Atas */
    gambar_garis_h(p, x, y, lebar, warna);
    /* Bawah */
    gambar_garis_h(p, x, y + tinggi - 1, lebar, warna);
    /* Kiri */
    gambar_garis_v(p, x, y, tinggi, warna);
    /* Kanan */
    gambar_garis_v(p, x + lebar - 1, y, tinggi, warna);
}

/*
 * ===========================================================================
 * DEKORASI - GAMBAR JUDUL
 * ===========================================================================
 */

status_t dekorasi_gambar_judul(jendela_t *j,
                                permukaan_t *target)
{
    struct jendela *w = (struct jendela *)j;

    if (w == NULL || w->magic != JENDELA_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    if (target == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Isi background judul bar */
    permukaan_isi_kotak(target,
        w->x, w->y, w->lebar,
        JENDELA_JUDUL_TINGGI,
        w->warna_judul);

    /* Garis pemisah bawah judul */
    gambar_garis_h(target,
        w->x, w->y + JENDELA_JUDUL_TINGGI - 1,
        w->lebar, w->warna_bingkai);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * DEKORASI - GAMBAR BINGKAI
 * ===========================================================================
 */

status_t dekorasi_gambar_bingkai(jendela_t *j,
                                  permukaan_t *target)
{
    struct jendela *w = (struct jendela *)j;

    if (w == NULL || w->magic != JENDELA_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    if (target == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Gambar bingkai sebagai persegi */
    gambar_persegi(target,
        w->x, w->y, w->lebar, w->tinggi,
        w->warna_bingkai);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * DEKORASI - GAMBAR TUTUP
 * ===========================================================================
 *
 * Tombol tutup (X) di pojok kanan judul baris.
 */

status_t dekorasi_gambar_tutup(jendela_t *j,
                                permukaan_t *target)
{
    struct jendela *w = (struct jendela *)j;
    tak_bertanda32_t tx, ty;
    tak_bertanda32_t ukuran;
    tak_bertanda32_t i;

    if (w == NULL || w->magic != JENDELA_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    if (target == NULL) {
        return STATUS_PARAM_NULL;
    }

    ukuran = JENDELA_JUDUL_TINGGI - 6;
    tx = w->x + w->lebar - ukuran - 4;
    ty = w->y + 3;

    /* Latar tombol */
    permukaan_isi_kotak(target,
        tx, ty, ukuran, ukuran,
        JENDELA_WARNA_BINGKAI);

    /* Gambar X */
    for (i = 0; i < ukuran; i++) {
        tak_bertanda32_t px, py;
        px = tx + 2 + i;
        py = ty + 2 + i;
        permukaan_tulis_piksel(target, px, py,
            JENDELA_WARNA_TUTUP);
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * DEKORASI - GAMBAR KONTROL (minimize/maksimalkan)
 * ===========================================================================
 */

status_t dekorasi_gambar_kontrol(jendela_t *j,
                                  permukaan_t *target)
{
    struct jendela *w = (struct jendela *)j;
    tak_bertanda32_t tx, ty;
    tak_bertanda32_t ukuran;

    if (w == NULL || w->magic != JENDELA_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    if (target == NULL) {
        return STATUS_PARAM_NULL;
    }

    ukuran = JENDELA_JUDUL_TINGGI - 6;

    /* Tombol minimize (garis horizontal) */
    tx = w->x + w->lebar - (ukuran * 3) - 8;
    ty = w->y + 3;

    permukaan_isi_kotak(target,
        tx, ty, ukuran, ukuran,
        JENDELA_WARNA_BINGKAI);

    gambar_garis_h(target, tx + 3, ty + ukuran / 2,
                   ukuran - 6, JENDELA_WARNA_TUTUP);

    /* Tombol maksimalkan (persegi) */
    tx = w->x + w->lebar - (ukuran * 2) - 5;
    ty = w->y + 3;

    permukaan_isi_kotak(target,
        tx, ty, ukuran, ukuran,
        JENDELA_WARNA_BINGKAI);

    gambar_persegi(target, tx + 3, ty + 3,
                   ukuran - 6, ukuran - 6,
                   JENDELA_WARNA_TUTUP);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * DEKORASI - GAMBAR SEMUA
 * ===========================================================================
 */

status_t dekorasi_gambar(jendela_t *j,
                          permukaan_t *target)
{
    status_t st;

    if (j == NULL || target == NULL) {
        return STATUS_PARAM_NULL;
    }

    st = dekorasi_gambar_bingkai(j, target);
    if (st != STATUS_BERHASIL) return st;

    st = dekorasi_gambar_judul(j, target);
    if (st != STATUS_BERHASIL) return st;

    st = dekorasi_gambar_tutup(j, target);
    if (st != STATUS_BERHASIL) return st;

    st = dekorasi_gambar_kontrol(j, target);
    return st;
}
