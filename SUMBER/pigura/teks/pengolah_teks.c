/*
 * PIGURA OS - PENGOLAH_TEKS.C
 * ============================
 * Pengolah teks tingkat tinggi Pigura OS.
 *
 * Modul ini menyediakan operasi teks tingkat tinggi:
 *   - Word wrapping otomatis
 *   - Alignment teks (kiri, tengah, kanan)
 *   - Pemotongan string
 *   - Multi-baris paragraf
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
 * Salin substring dari teks mulai dari offset sebanyak panjang.
 * Menulis ke buffer keluaran dan menambahkan null terminator.
 */
static tak_bertanda32_t salin_substring(const char *teks,
                                        tak_bertanda32_t offset,
                                        tak_bertanda32_t panjang,
                                        char *keluaran,
                                        tak_bertanda32_t ukuran_max)
{
    tak_bertanda32_t i;
    tak_bertanda32_t teks_len;

    if (teks == NULL || keluaran == NULL || ukuran_max == 0) {
        return 0;
    }

    teks_len = (tak_bertanda32_t)kernel_strlen(teks);

    if (offset >= teks_len) {
        keluaran[0] = '\0';
        return 0;
    }

    if (offset + panjang > teks_len) {
        panjang = teks_len - offset;
    }
    if (panjang >= ukuran_max) {
        panjang = ukuran_max - 1;
    }

    for (i = 0; i < panjang; i++) {
        keluaran[i] = teks[offset + i];
    }
    keluaran[panjang] = '\0';

    return panjang;
}

/*
 * Gambar satu baris teks dengan alignment.
 */
static status_t gambar_baris_rata(permukaan_t *permukaan,
                                  font_t *font,
                                  tak_bertanda32_t x,
                                  tak_bertanda32_t y,
                                  tak_bertanda32_t lebar_area,
                                  const char *baris,
                                  tak_bertanda8_t rata,
                                  tak_bertanda32_t warna)
{
    tak_bertanda32_t lebar_teks;
    tak_bertanda32_t x_mulai;

    lebar_teks = ukuran_string(font, baris);

    switch (rata) {
    case TEKS_RATA_TENGAH:
        if (lebar_teks < lebar_area) {
            x_mulai = x + (lebar_area - lebar_teks) / 2;
        } else {
            x_mulai = x;
        }
        break;
    case TEKS_RATA_KANAN:
        if (lebar_teks < lebar_area) {
            x_mulai = x + lebar_area - lebar_teks;
        } else {
            x_mulai = x;
        }
        break;
    case TEKS_RATA_KIRI:
    default:
        x_mulai = x;
        break;
    }

    return teks_gambar_string(permukaan, font, x_mulai, y,
                              baris, warna);
}

/*
 * ===========================================================================
 * PENGOLAH TEKS - BARIS MULTI-LINE
 * ===========================================================================
 *
 * Gambar teks multi-baris dengan word wrapping otomatis.
 * Memecah teks pada batas kata (spasi) atau karakter jika
 * satu kata melebihi lebar area.
 */

status_t pengolah_teks_baris(permukaan_t *permukaan,
                             font_t *font,
                             tak_bertanda32_t x,
                             tak_bertanda32_t y,
                             tak_bertanda32_t lebar_area,
                             const char *teks,
                             const teks_opsi_t *opsi)
{
    char baris_buf[TEKS_PANJANG_MAKS];
    tak_bertanda32_t pos = 0;
    tak_bertanda32_t baris_ke = 0;
    font_metrik_t metrik;
    tak_bertanda32_t warna;
    tak_bertanda8_t rata;

    if (permukaan == NULL || font == NULL || teks == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Dapatkan warna dan alignment dari opsi */
    if (opsi != NULL) {
        warna = opsi->warna_depan;
        rata = opsi->rata;
    } else {
        warna = 0xFFFFFFFF;
        rata = TEKS_RATA_KIRI;
    }

    /* Dapatkan tinggi baris */
    if (font_dapat_metrik(font, &metrik) != STATUS_BERHASIL) {
        metrik.tinggi_baris = 10;
    }

    while (teks[pos] != '\0') {
        tak_bertanda32_t panjang_baris = 0;
        tak_bertanda32_t kata_awal = 0;
        tak_bertanda32_t lebar_baris = 0;
        tak_bertanda32_t y_baris;

        /* Bangun baris dengan word wrapping */
        while (teks[pos] != '\0' && teks[pos] != '\n') {
            tak_bertanda32_t lebar_kar;
            bool_t kata_berakhir;

            lebar_kar = ukuran_karakter(font, teks[pos]);
            kata_berakhir = (teks[pos] == ' ') ? BENAR : SALAH;

            /* Cek apakah karakter ini muat */
            if (lebar_baris + lebar_kar > lebar_area &&
                lebar_baris > 0) {
                /* Tidak muat, potong di kata sebelumnya */
                if (kata_awal > 0) {
                    panjang_baris = kata_awal;
                    pos -= (panjang_baris > 0) ? 1 : 0;
                }
                break;
            }

            baris_buf[panjang_baris] = teks[pos];
            panjang_baris++;
            lebar_baris += lebar_kar;
            pos++;

            if (kata_berakhir) {
                kata_awal = panjang_baris;
            }
        }

        /* Tangani newline eksplisit */
        if (teks[pos] == '\n') {
            pos++;
        }

        /* Null-terminate baris */
        if (panjang_baris >= TEKS_PANJANG_MAKS) {
            panjang_baris = TEKS_PANJANG_MAKS - 1;
        }
        baris_buf[panjang_baris] = '\0';

        /* Gambar baris */
        y_baris = y + baris_ke *
                  (tak_bertanda32_t)metrik.tinggi_baris;

        gambar_baris_rata(permukaan, font, x, y_baris,
                          lebar_area, baris_buf, rata, warna);

        baris_ke++;

        /* Cek batas baris */
        if (baris_ke >= TEKS_BARIS_MAKS) {
            break;
        }
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * PENGOLAH TEKS - RATA
 * ===========================================================================
 */

status_t pengolah_teks_rata(permukaan_t *permukaan,
                            font_t *font,
                            tak_bertanda32_t x,
                            tak_bertanda32_t y,
                            tak_bertanda32_t lebar_area,
                            const char *teks,
                            tak_bertanda8_t rata,
                            tak_bertanda32_t warna)
{
    if (permukaan == NULL || font == NULL || teks == NULL) {
        return STATUS_PARAM_NULL;
    }

    return gambar_baris_rata(permukaan, font, x, y,
                             lebar_area, teks, rata, warna);
}

/*
 * ===========================================================================
 * PENGOLAH TEKS - POTONG
 * ===========================================================================
 *
 * Potong string agar muat dalam lebar_maks piksel.
 * Menambahkan "..." di akhir jika string terpotong.
 * Mengembalikan panjang string hasil.
 */

tak_bertanda32_t pengolah_teks_potong(font_t *font,
                                      const char *teks,
                                      tak_bertanda32_t lebar_maks,
                                      char *keluaran,
                                      tak_bertanda32_t ukuran_keluaran)
{
    tak_bertanda32_t pos = 0;
    tak_bertanda32_t lebar_total = 0;
    tak_bertanda32_t lebar_elipsis;
    bool_t terpotong = SALAH;
    tak_bertanda32_t pos_potong = 0;

    if (font == NULL || teks == NULL || keluaran == NULL) {
        return 0;
    }
    if (ukuran_keluaran == 0) {
        return 0;
    }

    /* Hitung lebar "..." */
    lebar_elipsis = ukuran_karakter(font, '.') * 3;

    while (teks[pos] != '\0') {
        tak_bertanda32_t lebar_kar;
        lebar_kar = ukuran_karakter(font, teks[pos]);

        if (lebar_total + lebar_kar > lebar_maks) {
            terpotong = BENAR;
            break;
        }

        lebar_total += lebar_kar;
        pos++;
    }

    if (!terpotong) {
        /* String muat utuh, salin seluruhnya */
        return salin_substring(teks, 0, pos, keluaran,
                               ukuran_keluaran);
    }

    /* String perlu dipotong, sisakan ruang untuk "..." */
    pos_potong = 0;
    lebar_total = 0;

    while (teks[pos_potong] != '\0') {
        tak_bertanda32_t lebar_kar;
        tak_bertanda32_t lebar_dengan_elipsis;
        lebar_kar = ukuran_karakter(font, teks[pos_potong]);
        lebar_dengan_elipsis = lebar_total + lebar_kar +
                               lebar_elipsis;

        if (lebar_dengan_elipsis > lebar_maks) {
            break;
        }

        lebar_total += lebar_kar;
        pos_potong++;
    }

    /* Salin string yang terpotong */
    salin_substring(teks, 0, pos_potong, keluaran,
                    ukuran_keluaran);

    /* Tambahkan "..." */
    {
        tak_bertanda32_t len;
        len = (tak_bertanda32_t)kernel_strlen(keluaran);
        if (len + 3 < ukuran_keluaran) {
            keluaran[len] = '.';
            keluaran[len + 1] = '.';
            keluaran[len + 2] = '.';
            keluaran[len + 3] = '\0';
            len += 3;
        }
        return len;
    }
}

/*
 * ===========================================================================
 * PENGOLAH TEKS - HITUNG BARIS
 * ===========================================================================
 *
 * Hitung jumlah baris yang akan dihasilkan
 * setelah word wrapping.
 */

tak_bertanda32_t pengolah_teks_hitung_baris(
    font_t *font,
    const char *teks,
    tak_bertanda32_t lebar_area)
{
    tak_bertanda32_t pos = 0;
    tak_bertanda32_t jumlah_baris = 1;
    tak_bertanda32_t lebar_baris = 0;

    if (font == NULL || teks == NULL) {
        return 0;
    }
    if (lebar_area == 0) {
        return 1;
    }

    while (teks[pos] != '\0') {
        tak_bertanda32_t lebar_kar;
        bool_t adalah_spasi;

        lebar_kar = ukuran_karakter(font, teks[pos]);
        adalah_spasi = (teks[pos] == ' ') ? BENAR : SALAH;

        if (lebar_baris + lebar_kar > lebar_area &&
            lebar_baris > 0) {
            jumlah_baris++;
            lebar_baris = 0;
            /* Kembali ke awal kata */
            if (!adalah_spasi) {
                continue;
            }
        }

        lebar_baris += lebar_kar;
        pos++;

        if (teks[pos - 1] == '\n') {
            jumlah_baris++;
            lebar_baris = 0;
        }
    }

    return jumlah_baris;
}
