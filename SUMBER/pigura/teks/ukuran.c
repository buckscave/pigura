/*
 * PIGURA OS - UKURAN.C
 * =====================
 * Pengukuran teks Pigura OS.
 *
 * Modul ini menghitung dimensi (lebar, tinggi, jumlah baris)
 * dari string ketika di-render dengan font tertentu.
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
 * UKURAN - KARAKTER
 * ===========================================================================
 */

tak_bertanda32_t ukuran_karakter(font_t *font, char karakter)
{
    const glyph_bitmap_t *g;

    if (font == NULL) {
        return 0;
    }

    g = font_dapat_glyph(font, (tak_bertanda32_t)karakter);
    if (g == NULL) {
        return 0;
    }

    return (tak_bertanda32_t)g->x_lanjut;
}

/*
 * ===========================================================================
 * UKURAN - STRING
 * ===========================================================================
 */

tak_bertanda32_t ukuran_string(font_t *font, const char *teks)
{
    tak_bertanda32_t total = 0;

    if (font == NULL || teks == NULL) {
        return 0;
    }

    while (*teks != '\0') {
        const glyph_bitmap_t *g;
        g = font_dapat_glyph(font, (tak_bertanda32_t)*teks);

        if (g != NULL) {
            total += (tak_bertanda32_t)g->x_lanjut;
        } else {
            /* Glyph tidak ditemukan, gunakan lebar default */
            total += 4;
        }

        teks++;
    }

    return total;
}

/*
 * ===========================================================================
 * UKURAN - BARIS
 * ===========================================================================
 *
 * Hitung panjang substring dari pos_awal yang muat
 * dalam lebar_maks piksel.
 */

tak_bertanda32_t ukuran_baris(font_t *font,
                              const char *teks,
                              tak_bertanda32_t pos_awal,
                              tak_bertanda32_t lebar_maks)
{
    tak_bertanda32_t total = 0;
    tak_bertanda32_t pos = pos_awal;
    tak_bertanda32_t panjang;

    if (font == NULL || teks == NULL) {
        return 0;
    }

    panjang = (tak_bertanda32_t)kernel_strlen(teks);
    if (pos_awal >= panjang) {
        return 0;
    }

    while (teks[pos] != '\0') {
        const glyph_bitmap_t *g;
        tak_bertanda32_t lebar_kar;

        g = font_dapat_glyph(font, (tak_bertanda32_t)teks[pos]);
        if (g != NULL) {
            lebar_kar = (tak_bertanda32_t)g->x_lanjut;
        } else {
            lebar_kar = 4;
        }

        /* Cek apakah karakter berikutnya muat */
        if (total + lebar_kar > lebar_maks) {
            break;
        }

        total += lebar_kar;
        pos++;
    }

    return pos - pos_awal;
}

/*
 * ===========================================================================
 * UKURAN - TEKS LENGKAP
 * ===========================================================================
 *
 * Hitung dimensi lengkap string dengan word wrapping.
 * Mengembalikan lebar terlebar, tinggi total,
 * jumlah baris, dan jumlah karakter.
 */

status_t ukuran_teks(font_t *font, const char *teks,
                     tak_bertanda32_t lebar_maks,
                     teks_ukuran_t *hasil)
{
    tak_bertanda32_t jumlah_baris = 1;
    tak_bertanda32_t lebar_baris = 0;
    tak_bertanda32_t lebar_maks_baris = 0;
    tak_bertanda32_t jumlah_karakter = 0;
    tak_bertanda32_t kata_awal = 0;
    tak_bertanda32_t kata_pos;
    tak_bertanda32_t total = 0;
    font_metrik_t metrik;

    if (font == NULL || teks == NULL || hasil == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memset(hasil, 0, sizeof(teks_ukuran_t));

    /* Dapatkan tinggi baris dari metrik font */
    if (font_dapat_metrik(font, &metrik) != STATUS_BERHASIL) {
        metrik.tinggi_baris = 10;  /* Default fallback */
    }

    /* Jika lebar_maks = 0, tidak ada wrapping */
    if (lebar_maks == 0) {
        hasil->lebar = ukuran_string(font, teks);
        hasil->tinggi = (tak_bertanda32_t)metrik.tinggi_baris;
        hasil->baris = 1;
        hasil->karakter = (tak_bertanda32_t)kernel_strlen(teks);
        return STATUS_BERHASIL;
    }

    while (teks[total] != '\0') {
        bool_t spasi_ditemukan = SALAH;

        /* Cari akhir kata berikutnya */
        kata_pos = total;
        while (teks[kata_pos] != '\0' &&
               teks[kata_pos] != ' ' &&
               teks[kata_pos] != '\n') {
            kata_pos++;
        }

        /* Hitung lebar kata */
        if (kata_pos > total) {
            tak_bertanda32_t i;
            tak_bertanda32_t lebar_kata = 0;

            for (i = total; i < kata_pos; i++) {
                lebar_kata += ukuran_karakter(
                    font, teks[i]);
            }

            /* Cek apakah kata muat di baris ini */
            if (lebar_baris + lebar_kata > lebar_maks &&
                lebar_baris > 0) {
                /* Pindah ke baris baru */
                jumlah_baris++;
                if (lebar_baris > lebar_maks_baris) {
                    lebar_maks_baris = lebar_baris;
                }
                lebar_baris = 0;
            }

            lebar_baris += lebar_kata;
            jumlah_karakter += (kata_pos - total);
            total = kata_pos;
        }

        /* Tangani spasi */
        if (teks[total] == ' ') {
            tak_bertanda32_t lebar_spasi;
            lebar_spasi = ukuran_karakter(font, ' ');
            if (lebar_baris + lebar_spasi > lebar_maks) {
                jumlah_baris++;
                if (lebar_baris > lebar_maks_baris) {
                    lebar_maks_baris = lebar_baris;
                }
                lebar_baris = 0;
            } else {
                lebar_baris += lebar_spasi;
            }
            total++;
            jumlah_karakter++;
            spasi_ditemukan = BENAR;
        }

        /* Tangani newline */
        if (teks[total] == '\n') {
            jumlah_baris++;
            if (lebar_baris > lebar_maks_baris) {
                lebar_maks_baris = lebar_baris;
            }
            lebar_baris = 0;
            total++;
            jumlah_karakter++;
        }

        /* Hindari infinite loop pada karakter lain */
        if (!spasi_ditemukan && teks[total] != '\n' &&
            teks[total] != '\0' &&
            teks[total] == teks[kata_pos]) {
            tak_bertanda32_t lebar_kar;
            lebar_kar = ukuran_karakter(font, teks[total]);
            if (lebar_baris + lebar_kar > lebar_maks) {
                jumlah_baris++;
                if (lebar_baris > lebar_maks_baris) {
                    lebar_maks_baris = lebar_baris;
                }
                lebar_baris = 0;
            }
            lebar_baris += lebar_kar;
            jumlah_karakter++;
            total++;
        }
    }

    /* Update lebar maks baris */
    if (lebar_baris > lebar_maks_baris) {
        lebar_maks_baris = lebar_baris;
    }

    /* Isi hasil */
    hasil->lebar = lebar_maks_baris;
    hasil->tinggi = jumlah_baris *
                    (tak_bertanda32_t)metrik.tinggi_baris;
    hasil->baris = jumlah_baris;
    hasil->karakter = jumlah_karakter;

    return STATUS_BERHASIL;
}
