/*
 * PIGURA OS - BUSUR.C
 * ====================
 * Operasi busur (arc) untuk pengolah grafis Pigura OS.
 *
 * Menggambar bagian dari lingkaran antara sudut_awal dan
 * sudut_akhir menggunakan pendekatan sudut diskret.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Seluruh nama dalam Bahasa Indonesia
 * - Mendukung: x86, x86_64, ARM, ARMv7, ARM64
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../../inti/kernel.h"
#include "pengolah.h"

extern tanda32_t cpu_sin_tabel(tak_bertanda32_t sudut);
extern tanda32_t cpu_cos_tabel(tak_bertanda32_t sudut);

extern status_t cpu_garis(struct p_konteks *ktx,
                          tanda32_t x0, tanda32_t y0,
                          tanda32_t x1, tanda32_t y1,
                          tak_bertanda32_t warna);

/*
 * ===========================================================================
 * FUNGSI INTERNAL - UTILITAS
 * ===========================================================================
 */

static tak_bertanda32_t warna_aktif(struct p_konteks *ktx)
{
    return pengolah_rgba_ke_piksel(
        ktx->konfig.warna.r, ktx->konfig.warna.g,
        ktx->konfig.warna.b, ktx->konfig.warna.a);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * pengolah_busur
 * --------------
 * Menggambar busur lingkaran dari sudut_awal ke sudut_akhir.
 * Sudut dalam derajat (0-359).
 * Arah: searah jarum jam.
 *
 * Parameter:
 *   ktx         - Pointer ke konteks pengolah
 *   cx, cy      - Pusat lingkaran
 *   jari_jari   - Jari-jari busur
 *   sudut_awal  - Sudut awal dalam derajat
 *   sudut_akhir - Sudut akhir dalam derajat
 *
 * Return:
 *   STATUS_BERHASIL atau kode error
 */
status_t pengolah_busur(struct p_konteks *ktx,
                        tanda32_t cx, tanda32_t cy,
                        tak_bertanda32_t jari_jari,
                        tanda32_t sudut_awal,
                        tanda32_t sudut_akhir)
{
    tak_bertanda32_t langkah, jumlah_langkah, i;
    tak_bertanda32_t warna;
    tanda32_t px_sebelum, py_sebelum;
    tanda32_t px_sekarang, py_sekarang;
    tak_bertanda64_t fx, fy;
    bool_t selesai;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (jari_jari == 0) {
        return STATUS_PARAM_JARAK;
    }

    warna = warna_aktif(ktx);

    /* Normalisasi sudut ke 0-359 */
    while (sudut_awal < 0) {
        sudut_awal += 360;
    }
    while (sudut_akhir < 0) {
        sudut_akhir += 360;
    }
    sudut_awal = sudut_awal % 360;
    sudut_akhir = sudut_akhir % 360;

    /* Hitung langkah: lebih banyak untuk jari-jari besar */
    langkah = jari_jari / 2;
    if (langkah < 2) {
        langkah = 2;
    }
    if (langkah > 180) {
        langkah = 180;
    }

    /* Hitung span sudut */
    if (sudut_awal <= sudut_akhir) {
        jumlah_langkah = (sudut_akhir - sudut_awal) /
                         langkah + 1;
    } else {
        jumlah_langkah =
            (360 - sudut_awal + sudut_akhir) / langkah + 1;
    }
    if (jumlah_langkah < 2) {
        jumlah_langkah = 2;
    }

    /* Titik awal */
    fx = (tak_bertanda64_t)cx * 65536 +
         (tak_bertanda64_t)jari_jari *
         (tak_bertanda64_t)cpu_cos_tabel(sudut_awal);
    fy = (tak_bertanda64_t)cy * 65536 +
         (tak_bertanda64_t)jari_jari *
         (tak_bertanda64_t)cpu_sin_tabel(sudut_awal);

    px_sebelum = (tanda32_t)(fx >> 16);
    py_sebelum = (tanda32_t)(fy >> 16);

    /* Gambar titik-titik busur dengan garis */
    selesai = SALAH;
    for (i = 1; i <= jumlah_langkah && !selesai; i++) {
        tak_bertanda32_t sudut_sekarang;
        tak_bertanda32_t span;

        span = (sudut_awal <= sudut_akhir)
            ? (sudut_akhir - sudut_awal)
            : (360 - sudut_awal + sudut_akhir);

        sudut_sekarang = sudut_awal +
            (tak_bertanda32_t)((tak_bertanda64_t)i *
                               span / jumlah_langkah);

        /* Cek apakah sudah melewati sudut akhir */
        if (sudut_awal <= sudut_akhir) {
            if (sudut_sekarang >= sudut_akhir) {
                sudut_sekarang = sudut_akhir;
                selesai = BENAR;
            }
        } else {
            if (sudut_sekarang >= 360) {
                sudut_sekarang -= 360;
            }
            if (sudut_awal > sudut_akhir) {
                if (sudut_sekarang >= sudut_akhir &&
                    i > 1) {
                    selesai = BENAR;
                    if (sudut_sekarang != sudut_akhir) {
                        sudut_sekarang = sudut_akhir;
                    }
                }
            }
        }

        fx = (tak_bertanda64_t)cx * 65536 +
             (tak_bertanda64_t)jari_jari *
             (tak_bertanda64_t)cpu_cos_tabel(sudut_sekarang);
        fy = (tak_bertanda64_t)cy * 65536 +
             (tak_bertanda64_t)jari_jari *
             (tak_bertanda64_t)cpu_sin_tabel(sudut_sekarang);

        px_sekarang = (tanda32_t)(fx >> 16);
        py_sekarang = (tanda32_t)(fy >> 16);

        cpu_garis(ktx, px_sebelum, py_sebelum,
                  px_sekarang, py_sekarang, warna);

        px_sebelum = px_sekarang;
        py_sebelum = py_sekarang;
    }

    ktx->statistik.jumlah_operasi++;
    ktx->statistik.piksel_diproses +=
        (tak_bertanda64_t)jumlah_langkah;

    return STATUS_BERHASIL;
}
