/*
 * PIGURA OS - TUKAR.C
 * =====================
 * Operasi tukar (swap) dan page-flip buffer piksel.
 *
 * Modul ini mengimplementasikan berbagai metode pertukaran buffer
 * untuk subsistem grafis LibPigura. Mendukung double-buffering,
 * triple-buffering, pertukaran parsial, dan salin balik.
 *
 * Fungsi-fungsi di sini dirancang untuk bekerja dengan struct
 * buffer_piksel yang didefinisikan di buffer.c, serta dengan
 * data piksel mentah secara langsung.
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

/* ========================================================================== */
/* KONSTANTA DAN DEFINISI LOKAL                                               */
/* ========================================================================== */

#define BUFFER_MAGIC           0x42554642  /* "BUFB" */
#define BUFFER_VERSI           0x0100
#define BUFFER_DIMENSI_MAKS    16384

/* Format pixel yang didukung */
#define BUFFER_FMT_INVALID     0
#define BUFFER_FMT_RGB332      1   /* 8 bit  : RRRGGGGB       */
#define BUFFER_FMT_RGB565      2   /* 16 bit : RRRRRGGG       */
                                   /*         GGGBBBBB       */
#define BUFFER_FMT_RGB888      3   /* 24 bit : RRGGBB         */
#define BUFFER_FMT_ARGB8888    4   /* 32 bit : AARRGGBB       */
#define BUFFER_FMT_XRGB8888    5   /* 32 bit : XXRRGGBB       */
#define BUFFER_FMT_BGRA8888    6   /* 32 bit : BBGGRRAA       */
#define BUFFER_FMT_BGR565      7   /* 16 bit : BBBBBGGG       */
                                   /*         GGGRRRRR       */
#define BUFFER_FMT_MAKS        7

/* Flag buffer */
#define BUFFER_FLAG_ALOKASI    0x01  /* Memori dialokasikan    */
#define BUFFER_FLAG_EKSTERNAL  0x02  /* Memori dari luar       */
#define BUFFER_FLAG_KUNCI      0x04  /* Buffer sedang dikunci  */
#define BUFFER_FLAG_KOTOR      0x08  /* Buffer perlu disalin   */

/*
 * bpp_dari_format - Mendapatkan byte per piksel dari format.
 * Menggunakan nested ternary yang kompatibel C90.
 */
#define bpp_dari_format(fmt) \
    (((fmt) == BUFFER_FMT_RGB332) ? 1 : \
     (((fmt) == BUFFER_FMT_RGB565) || \
      ((fmt) == BUFFER_FMT_BGR565)) ? 2 : \
     ((fmt) == BUFFER_FMT_RGB888) ? 3 : 4)

/*
 * bit_dari_format - Mendapatkan bit per piksel dari format.
 */
#define bit_dari_format(fmt) \
    ((tak_bertanda8_t)(bpp_dari_format(fmt) * 8))

/* ========================================================================== */
/* STRUKTUR DATA                                                              */
/* ========================================================================== */

/*
 * struct buffer_piksel
 * --------------------
 * Struktur utama buffer pixel (harus cocok dengan buffer.c).
 * Menyimpan data piksel mentah beserta metadata format dan dimensi.
 */
struct buffer_piksel {
    tak_bertanda32_t magic;         /* Magic number validasi   */
    tak_bertanda16_t versi;         /* Versi struktur buffer  */

    /* Dimensi dan format */
    tak_bertanda32_t lebar;         /* Lebar dalam piksel      */
    tak_bertanda32_t tinggi;        /* Tinggi dalam piksel     */
    tak_bertanda8_t  format;        /* Format pixel            */
    tak_bertanda8_t  bit_per_piksel;/* Bit per piksel          */
    tak_bertanda32_t pitch;         /* Byte per baris          */
    ukuran_t         ukuran;        /* Total byte buffer       */

    /* Memori piksel */
    tak_bertanda8_t *data;          /* Pointer ke data piksel  */

    /* Metadata */
    tak_bertanda32_t flags;         /* Flag keadaan buffer     */
    tak_bertanda32_t referensi;     /* Jumlah referensi aktif  */
};

/*
 * struct area_tukar
 * -----------------
 * Mendefinisikan area persegi panjang untuk pertukaran parsial.
 * Digunakan oleh tukar_parsial untuk menentukan region piksel
 * yang akan ditukar antara buffer depan dan belakang.
 */
struct area_tukar {
    tak_bertanda32_t x;             /* Koordinat X awal        */
    tak_bertanda32_t y;             /* Koordinat Y awal        */
    tak_bertanda32_t lebar;         /* Lebar area dalam piksel */
    tak_bertanda32_t tinggi;        /* Tinggi area dalam piksel*/
};

/* ========================================================================== */
/* FUNGSI HELPER INTERNAL                                                     */
/* ========================================================================== */

/*
 * hitung_pitch
 * -----------
 * Menghitung pitch (byte per baris) berdasarkan lebar dan format.
 * Pitch selalu dibulatkan ke atas ke kelipatan 4 byte untuk
 * kinerja optimal pada semua arsitektur.
 */
static tak_bertanda32_t hitung_pitch(tak_bertanda32_t lebar,
                                     tak_bertanda8_t format)
{
    tak_bertanda32_t bpp = (tak_bertanda32_t)bpp_dari_format(format);
    tak_bertanda32_t pitch_kasar = lebar * bpp;
    return RATAKAN_ATAS(pitch_kasar, 4);
}

/*
 * hitung_ukuran_buffer
 * -------------------
 * Menghitung total ukuran buffer dalam byte.
 */
static ukuran_t hitung_ukuran_buffer(tak_bertanda32_t lebar,
                                     tak_bertanda32_t tinggi,
                                     tak_bertanda8_t format)
{
    tak_bertanda32_t p = hitung_pitch(lebar, format);
    return (ukuran_t)p * (ukuran_t)tinggi;
}

/*
 * buffer_piksel_valid - Periksa apakah pointer buffer valid.
 * Memeriksa magic number, pointer data, dan dimensi dasar.
 */
static bool_t buffer_piksel_valid(const struct buffer_piksel *buf)
{
    if (buf == NULL) {
        return SALAH;
    }
    if (buf->magic != BUFFER_MAGIC) {
        return SALAH;
    }
    if (buf->lebar == 0 || buf->tinggi == 0) {
        return SALAH;
    }
    if (buf->format == BUFFER_FMT_INVALID ||
        buf->format > BUFFER_FMT_MAKS) {
        return SALAH;
    }
    if (buf->data == NULL) {
        return SALAH;
    }
    return BENAR;
}

/*
 * hitung_offset_piksel - Hitung offset byte untuk koordinat (x,y).
 */
static tak_bertanda32_t hitung_offset_piksel(
    const struct buffer_piksel *buf,
    tak_bertanda32_t x, tak_bertanda32_t y)
{
    return y * buf->pitch +
        x * (tak_bertanda32_t)bpp_dari_format(buf->format);
}

/*
 * cek_koordinat_area - Periksa apakah area berada dalam buffer.
 * Memvalidasi bahwa area (x, y, w, h) tidak keluar dari batas.
 */
static bool_t cek_koordinat_area(const struct buffer_piksel *buf,
                                 tak_bertanda32_t x,
                                 tak_bertanda32_t y,
                                 tak_bertanda32_t w,
                                 tak_bertanda32_t h)
{
    if (w == 0 || h == 0) {
        return SALAH;
    }
    if (x >= buf->lebar || y >= buf->tinggi) {
        return SALAH;
    }
    if (x + w > buf->lebar || y + h > buf->tinggi) {
        return SALAH;
    }
    return BENAR;
}

/*
 * cek_tumpang_tindih_area - Periksa apakah dua area tumpang tindih.
 * Mengembalikan BENAR jika area1 dan area2 saling tumpang tindih.
 */
static bool_t cek_tumpang_tindih_area(
    tak_bertanda32_t x1, tak_bertanda32_t y1,
    tak_bertanda32_t w1, tak_bertanda32_t h1,
    tak_bertanda32_t x2, tak_bertanda32_t y2,
    tak_bertanda32_t w2, tak_bertanda32_t h2)
{
    tak_bertanda32_t x1_akhir = x1 + w1;
    tak_bertanda32_t y1_akhir = y1 + h1;
    tak_bertanda32_t x2_akhir = x2 + w2;
    tak_bertanda32_t y2_akhir = y2 + h2;

    if (x1 >= x2_akhir || x2 >= x1_akhir) {
        return SALAH;
    }
    if (y1 >= y2_akhir || y2 >= y1_akhir) {
        return SALAH;
    }
    return BENAR;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: VALIDASI BUFFER UNTUK PERTUKARAN                            */
/* ========================================================================== */

/*
 * tukar_validasi_buffer
 * --------------------
 * Memvalidasi bahwa dua buffer kompatibel untuk pertukaran.
 * Diperlukan: magic valid, dimensi sama, format sama, data ada.
 *
 * Parameter:
 *   a - Pointer ke buffer pertama
 *   b - Pointer ke buffer kedua
 *
 * Return: BENAR jika kedua buffer kompatibel, SALAH jika tidak.
 */
bool_t tukar_validasi_buffer(const struct buffer_piksel *a,
                              const struct buffer_piksel *b)
{
    if (!buffer_piksel_valid(a) || !buffer_piksel_valid(b)) {
        return SALAH;
    }
    if (a->lebar != b->lebar) {
        return SALAH;
    }
    if (a->tinggi != b->tinggi) {
        return SALAH;
    }
    if (a->format != b->format) {
        return SALAH;
    }
    if (a->ukuran != b->ukuran) {
        return SALAH;
    }
    return BENAR;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: PERTUKARAN POINTER BUFFER                                   */
/* ========================================================================== */

/*
 * tukar_buffer_ptr
 * ---------------
 * Menukar dua pointer buffer_piksel. Ini adalah operasi swap
 * pointer paling sederhana, digunakan oleh manajer double-buffer.
 * Tidak menukar isi struct, hanya pointer yang menunjuk ke struct.
 *
 * Parameter:
 *   buf_a - Pointer ke pointer buffer pertama
 *   buf_b - Pointer ke pointer buffer kedua
 */
void tukar_buffer_ptr(struct buffer_piksel **buf_a,
                       struct buffer_piksel **buf_b)
{
    struct buffer_piksel *sementara;

    if (buf_a == NULL || buf_b == NULL) {
        return;
    }
    if (*buf_a == NULL || *buf_b == NULL) {
        return;
    }

    sementara = *buf_a;
    *buf_a = *buf_b;
    *buf_b = sementara;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: PERTUKARAN DATA BUFFER DEPAN/BELAKANG                       */
/* ========================================================================== */

/*
 * tukar_buffer_depan_belakang
 * --------------------------
 * Menukar pointer data piksel antara dua struct buffer_piksel.
 * Tidak menukar struct itu sendiri, hanya field data dan ukuran.
 * Berguna ketika depan dan belakang adalah objek terpisah.
 * Kedua buffer akan ditandai kotor setelah pertukaran.
 *
 * Parameter:
 *   depan   - Pointer ke struct buffer depan (layar)
 *   belakang - Pointer ke struct buffer belakang (offscreen)
 *
 * Return: STATUS_BERHASIL jika berhasil, kode error jika gagal.
 */
status_t tukar_buffer_depan_belakang(struct buffer_piksel *depan,
                                      struct buffer_piksel *belakang)
{
    tak_bertanda8_t *data_sementara;
    ukuran_t ukuran_sementara;

    /* Validasi kedua buffer */
    CEK_PTR(depan);
    CEK_PTR(belakang);

    if (!buffer_piksel_valid(depan)) {
        return STATUS_PARAM_INVALID;
    }
    if (!buffer_piksel_valid(belakang)) {
        return STATUS_PARAM_INVALID;
    }

    /* Validasi kompatibilitas */
    if (!tukar_validasi_buffer(depan, belakang)) {
        return STATUS_PARAM_UKURAN;
    }

    /* Pertukaran pointer data piksel */
    data_sementara = depan->data;
    depan->data = belakang->data;
    belakang->data = data_sementara;

    /* Pertukaran ukuran buffer */
    ukuran_sementara = depan->ukuran;
    depan->ukuran = belakang->ukuran;
    belakang->ukuran = ukuran_sementara;

    /* Tandai kedua buffer sebagai kotor */
    depan->flags |= BUFFER_FLAG_KOTOR;
    belakang->flags |= BUFFER_FLAG_KOTOR;

    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: PERTUKARAN DATA PIKSEL MENTAH                               */
/* ========================================================================== */

/*
 * tukar_piksel
 * ----------
 * Menukar isi data piksel mentah antara dua buffer byte.
 * Mengalokasikan buffer sementara untuk operasi tiga arah.
 * Digunakan ketika buffer bukan bagian dari struct buffer_piksel.
 *
 * Parameter:
 *   data_a - Pointer ke data piksel pertama
 *   data_b - Pointer ke data piksel kedua
 *   ukuran - Ukuran data dalam byte (harus sama untuk kedua)
 *
 * Return: STATUS_BERHASIL jika berhasil, STATUS_PARAM_NULL jika
 *         pointer NULL, STATUS_MEMORI_HABIS jika gagal alokasi.
 */
status_t tukar_piksel(tak_bertanda8_t *data_a,
                       tak_bertanda8_t *data_b,
                       ukuran_t ukuran)
{
    tak_bertanda8_t *sementara;
    status_t hasil = STATUS_GAGAL;

    CEK_PTR(data_a);
    CEK_PTR(data_b);

    if (ukuran == 0) {
        return STATUS_BERHASIL;
    }

    /* Alokasi buffer sementara untuk pertukaran tiga arah */
    sementara = (tak_bertanda8_t *)kmalloc(ukuran);
    if (sementara == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    /* Salin data_a ke sementara */
    kernel_memcpy(sementara, data_a, ukuran);

    /* Salin data_b ke data_a */
    kernel_memcpy(data_a, data_b, ukuran);

    /* Salin sementara ke data_b */
    kernel_memcpy(data_b, sementara, ukuran);

    hasil = STATUS_BERHASIL;

    /* Bersihkan dan bebaskan buffer sementara */
    kernel_memset(sementara, 0, ukuran);
    kfree(sementara);

    return hasil;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: PERTUKARAN AREA DALAM SATU BUFFER                           */
/* ========================================================================== */

/*
 * tukar_area
 * ---------
 * Menukar dua area persegi panjang dalam buffer yang sama.
 * Menangani area yang tumpang tindih dengan benar menggunakan
 * buffer sementara. Jika area tidak tumpang tindih, tetap
 * menggunakan buffer sementara untuk konsistensi dan keamanan.
 *
 * Parameter:
 *   buf    - Pointer ke struct buffer piksel
 *   area1_x - Koordinat X area pertama
 *   area1_y - Koordinat Y area pertama
 *   area2_x - Koordinat X area kedua
 *   area2_y - Koordinat Y area kedua
 *   w      - Lebar area dalam piksel
 *   h      - Tinggi area dalam piksel
 *
 * Return: STATUS_BERHASIL jika berhasil, kode error jika gagal.
 */
status_t tukar_area(struct buffer_piksel *buf,
                     tak_bertanda32_t area1_x,
                     tak_bertanda32_t area1_y,
                     tak_bertanda32_t area2_x,
                     tak_bertanda32_t area2_y,
                     tak_bertanda32_t w,
                     tak_bertanda32_t h)
{
    tak_bertanda8_t *sementara = NULL;
    tak_bertanda8_t bpp;
    tak_bertanda32_t baris;
    tak_bertanda32_t ukuran_baris;
    tak_bertanda32_t ukuran_total;
    status_t hasil = STATUS_GAGAL;

    CEK_PTR(buf);
    if (!buffer_piksel_valid(buf)) {
        return STATUS_PARAM_INVALID;
    }

    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }

    /* Validasi koordinat kedua area */
    if (!cek_koordinat_area(buf, area1_x, area1_y, w, h)) {
        return STATUS_PARAM_JARAK;
    }
    if (!cek_koordinat_area(buf, area2_x, area2_y, w, h)) {
        return STATUS_PARAM_JARAK;
    }

    bpp = (tak_bertanda8_t)bpp_dari_format(buf->format);
    ukuran_baris = w * (tak_bertanda32_t)bpp;

    /* Cek overflow pada perhitungan ukuran total */
    ukuran_total = ukuran_baris * h;
    if (ukuran_total / ukuran_baris != h) {
        return STATUS_PARAM_UKURAN;
    }

    /* Alokasi buffer sementara untuk satu area penuh */
    sementara = (tak_bertanda8_t *)kmalloc((ukuran_t)ukuran_total);
    if (sementara == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    /* Langkah 1: Salin area1 ke buffer sementara */
    for (baris = 0; baris < h; baris++) {
        tak_bertanda32_t ofs1 = (area1_y + baris) * buf->pitch +
            area1_x * (tak_bertanda32_t)bpp;
        kernel_memcpy(
            sementara + (ukuran_t)baris * (ukuran_t)ukuran_baris,
            buf->data + ofs1,
            (ukuran_t)ukuran_baris);
    }

    /* Langkah 2: Salin area2 ke area1 */
    for (baris = 0; baris < h; baris++) {
        tak_bertanda32_t ofs1 = (area1_y + baris) * buf->pitch +
            area1_x * (tak_bertanda32_t)bpp;
        tak_bertanda32_t ofs2 = (area2_y + baris) * buf->pitch +
            area2_x * (tak_bertanda32_t)bpp;
        kernel_memcpy(buf->data + ofs1,
                      buf->data + ofs2,
                      (ukuran_t)ukuran_baris);
    }

    /* Langkah 3: Salin sementara ke area2 */
    for (baris = 0; baris < h; baris++) {
        tak_bertanda32_t ofs2 = (area2_y + baris) * buf->pitch +
            area2_x * (tak_bertanda32_t)bpp;
        kernel_memcpy(
            buf->data + ofs2,
            sementara + (ukuran_t)baris * (ukuran_t)ukuran_baris,
            (ukuran_t)ukuran_baris);
    }

    buf->flags |= BUFFER_FLAG_KOTOR;
    hasil = STATUS_BERHASIL;

    /* Bersihkan dan bebaskan buffer sementara */
    kernel_memset(sementara, 0, (ukuran_t)ukuran_total);
    kfree(sementara);

    return hasil;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: PERTUKARAN BARIS ANTARA DUA BUFFER                          */
/* ========================================================================== */

/*
 * tukar_baris
 * ---------
 * Menukar baris-baris tertentu antara dua buffer data mentah.
 * Berguna untuk pertukaran parsial tanpa struct buffer_piksel.
 * Menggunakan buffer sementara per baris untuk efisiensi memori.
 *
 * Parameter:
 *   data_a       - Pointer ke data buffer pertama
 *   data_b       - Pointer ke data buffer kedua
 *   ukuran_baris - Ukuran satu baris dalam byte
 *   jumlah_baris - Jumlah baris yang akan ditukar
 *   pitch        - Jarak antar baris dalam byte (untuk padding)
 *
 * Return: STATUS_BERHASIL jika berhasil, kode error jika gagal.
 */
status_t tukar_baris(tak_bertanda8_t *data_a,
                      tak_bertanda8_t *data_b,
                      ukuran_t ukuran_baris,
                      tak_bertanda32_t jumlah_baris,
                      ukuran_t pitch)
{
    tak_bertanda8_t *sementara = NULL;
    tak_bertanda32_t baris;
    status_t hasil = STATUS_GAGAL;

    CEK_PTR(data_a);
    CEK_PTR(data_b);

    if (jumlah_baris == 0 || ukuran_baris == 0) {
        return STATUS_BERHASIL;
    }

    /* Gunakan pitch sebagai offset, atau ukuran_baris jika 0 */
    if (pitch == 0) {
        pitch = ukuran_baris;
    }
    if (pitch < ukuran_baris) {
        return STATUS_PARAM_UKURAN;
    }

    /* Alokasi buffer sementara untuk satu baris */
    sementara = (tak_bertanda8_t *)kmalloc(ukuran_baris);
    if (sementara == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    /* Tukar setiap baris satu per satu */
    for (baris = 0; baris < jumlah_baris; baris++) {
        tak_bertanda8_t *ptr_a = data_a +
            (ukuran_t)baris * pitch;
        tak_bertanda8_t *ptr_b = data_b +
            (ukuran_t)baris * pitch;

        /* data_a -> sementara */
        kernel_memcpy(sementara, ptr_a, ukuran_baris);
        /* data_b -> data_a */
        kernel_memcpy(ptr_a, ptr_b, ukuran_baris);
        /* sementara -> data_b */
        kernel_memcpy(ptr_b, sementara, ukuran_baris);
    }

    hasil = STATUS_BERHASIL;

    /* Bersihkan dan bebaskan buffer sementara */
    kernel_memset(sementara, 0, ukuran_baris);
    kfree(sementara);

    return hasil;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: PERTUKARAN SEGERA TANPA VSYNC                               */
/* ========================================================================== */

/*
 * tukar_segera
 * -----------
 * Menukar pointer data antara buffer depan dan belakang secara
 * langsung tanpa menunggu sinyal vsync. Operasi ini paling cepat
 * tetapi dapat menyebabkan tearing pada tampilan.
 * Kedua buffer akan ditandai kotor setelah pertukaran.
 *
 * Parameter:
 *   buf_depan    - Pointer ke struct buffer depan (layar)
 *   buf_belakang - Pointer ke struct buffer belakang (offscreen)
 *
 * Return: STATUS_BERHASIL jika berhasil, kode error jika gagal.
 */
status_t tukar_segera(struct buffer_piksel *buf_depan,
                       struct buffer_piksel *buf_belakang)
{
    tak_bertanda8_t *data_sementara;
    ukuran_t ukuran_sementara;

    CEK_PTR(buf_depan);
    CEK_PTR(buf_belakang);

    if (!buffer_piksel_valid(buf_depan)) {
        return STATUS_PARAM_INVALID;
    }
    if (!buffer_piksel_valid(buf_belakang)) {
        return STATUS_PARAM_INVALID;
    }

    /* Pastikan ukuran dan format cocok */
    if (buf_depan->lebar != buf_belakang->lebar ||
        buf_depan->tinggi != buf_belakang->tinggi ||
        buf_depan->format != buf_belakang->format) {
        return STATUS_PARAM_UKURAN;
    }

    /* Cek apakah buffer sedang dikunci */
    if ((buf_depan->flags & BUFFER_FLAG_KUNCI) != 0 ||
        (buf_belakang->flags & BUFFER_FLAG_KUNCI) != 0) {
        return STATUS_BUSY;
    }

    /* Tukar pointer data */
    data_sementara = buf_depan->data;
    buf_depan->data = buf_belakang->data;
    buf_belakang->data = data_sementara;

    /* Tukar ukuran */
    ukuran_sementara = buf_depan->ukuran;
    buf_depan->ukuran = buf_belakang->ukuran;
    buf_belakang->ukuran = ukuran_sementara;

    /* Tandai keduanya kotor */
    buf_depan->flags |= BUFFER_FLAG_KOTOR;
    buf_belakang->flags |= BUFFER_FLAG_KOTOR;

    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: PERTUKARAN DENGAN SALIN                                     */
/* ========================================================================== */

/*
 * tukar_dengan_salin
 * -----------------
 * Menyalin data dari buffer belakang ke buffer depan, tanpa
 * menukar pointer. Digunakan ketika buffer depan dipetakan ke
 * hardware (misalnya framebuffer fisik) sehingga pointer tidak
 * dapat diubah. Operasi dilakukan baris per baris untuk efisiensi
 * cache dan mendukung area data yang lebih besar.
 *
 * Parameter:
 *   buf_depan    - Pointer ke struct buffer depan (hardware)
 *   buf_belakang - Pointer ke struct buffer belakang (offscreen)
 *
 * Return: STATUS_BERHASIL jika berhasil, kode error jika gagal.
 */
status_t tukar_dengan_salin(struct buffer_piksel *buf_depan,
                             struct buffer_piksel *buf_belakang)
{
    tak_bertanda32_t baris;
    tak_bertanda32_t ukuran_baris;

    CEK_PTR(buf_depan);
    CEK_PTR(buf_belakang);

    if (!buffer_piksel_valid(buf_depan)) {
        return STATUS_PARAM_INVALID;
    }
    if (!buffer_piksel_valid(buf_belakang)) {
        return STATUS_PARAM_INVALID;
    }

    /* Validasi kompatibilitas dimensi dan format */
    if (buf_depan->lebar != buf_belakang->lebar ||
        buf_depan->tinggi != buf_belakang->tinggi ||
        buf_depan->format != buf_belakang->format) {
        return STATUS_PARAM_UKURAN;
    }

    /* Cek apakah buffer depan dikunci (sedang digunakan hardware) */
    if ((buf_depan->flags & BUFFER_FLAG_KUNCI) != 0) {
        return STATUS_BUSY;
    }

    ukuran_baris = buf_depan->pitch;

    /* Optimasi: jika ukuran sama, salin seluruh buffer sekaligus */
    if (buf_depan->ukuran == buf_belakang->ukuran &&
        buf_depan->pitch == buf_belakang->pitch) {
        kernel_memcpy(buf_depan->data, buf_belakang->data,
                      buf_depan->ukuran);
        buf_depan->flags |= BUFFER_FLAG_KOTOR;
        return STATUS_BERHASIL;
    }

    /* Salin baris per baris untuk menangani pitch berbeda */
    for (baris = 0; baris < buf_depan->tinggi; baris++) {
        tak_bertanda8_t *sumber = buf_belakang->data +
            (ukuran_t)baris * buf_belakang->pitch;
        tak_bertanda8_t *tujuan = buf_depan->data +
            (ukuran_t)baris * buf_depan->pitch;

        kernel_memcpy(tujuan, sumber,
                      (ukuran_t)MIN(ukuran_baris,
                                    buf_belakang->pitch));
    }

    buf_depan->flags |= BUFFER_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: PERTUKARAN PARSIAL BERDASARKAN DAFTAR AREA                  */
/* ========================================================================== */

/*
 * tukar_parsial
 * -----------
 * Menukar hanya area-area tertentu antara buffer depan dan belakang.
 * Mengambil array struct area_tukar yang menentukan region mana saja
 * yang perlu ditukar. Efisien untuk update partial layar.
 * Setiap area ditukar menggunakan buffer sementara kecil.
 *
 * Parameter:
 *   buf_depan    - Pointer ke struct buffer depan
 *   buf_belakang - Pointer ke struct buffer belakang
 *   daftar_area  - Array area yang akan ditukar
 *   jumlah_area  - Jumlah area dalam array
 *
 * Return: STATUS_BERHASIL jika semua area berhasil ditukar,
 *         kode error jika ada kegagalan.
 */
status_t tukar_parsial(struct buffer_piksel *buf_depan,
                        struct buffer_piksel *buf_belakang,
                        const struct area_tukar *daftar_area,
                        tak_bertanda32_t jumlah_area)
{
    tak_bertanda8_t *sementara = NULL;
    tak_bertanda8_t bpp;
    tak_bertanda32_t indeks;
    tak_bertanda32_t baris;
    tak_bertanda32_t ukuran_baris_maks = 0;
    status_t hasil = STATUS_BERHASIL;
    tak_bertanda32_t lebar_maks = 0;
    tak_bertanda32_t tinggi_maks = 0;

    CEK_PTR(buf_depan);
    CEK_PTR(buf_belakang);
    CEK_PTR(daftar_area);

    if (jumlah_area == 0) {
        return STATUS_BERHASIL;
    }

    if (!buffer_piksel_valid(buf_depan)) {
        return STATUS_PARAM_INVALID;
    }
    if (!buffer_piksel_valid(buf_belakang)) {
        return STATUS_PARAM_INVALID;
    }

    /* Validasi kompatibilitas kedua buffer */
    if (!tukar_validasi_buffer(buf_depan, buf_belakang)) {
        return STATUS_PARAM_UKURAN;
    }

    bpp = (tak_bertanda8_t)bpp_dari_format(buf_depan->format);

    /* Cari ukuran baris terbesar untuk alokasi sekali */
    for (indeks = 0; indeks < jumlah_area; indeks++) {
        if (daftar_area[indeks].lebar > lebar_maks) {
            lebar_maks = daftar_area[indeks].lebar;
        }
        if (daftar_area[indeks].tinggi > tinggi_maks) {
            tinggi_maks = daftar_area[indeks].tinggi;
        }
    }

    if (lebar_maks == 0) {
        return STATUS_BERHASIL;
    }

    /* Cek batas dimensi */
    if (lebar_maks > buf_depan->lebar) {
        return STATUS_PARAM_JARAK;
    }
    if (tinggi_maks > buf_depan->tinggi) {
        return STATUS_PARAM_JARAK;
    }

    /* Cek overflow */
    ukuran_baris_maks = lebar_maks * (tak_bertanda32_t)bpp;
    if (ukuran_baris_maks / lebar_maks != (tak_bertanda32_t)bpp) {
        return STATUS_PARAM_UKURAN;
    }

    /* Alokasi buffer sementara untuk satu baris terbesar */
    sementara = (tak_bertanda8_t *)kmalloc(
        (ukuran_t)ukuran_baris_maks);
    if (sementara == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    /* Proses setiap area dalam daftar */
    for (indeks = 0; indeks < jumlah_area; indeks++) {
        tak_bertanda32_t ax = daftar_area[indeks].x;
        tak_bertanda32_t ay = daftar_area[indeks].y;
        tak_bertanda32_t aw = daftar_area[indeks].lebar;
        tak_bertanda32_t ah = daftar_area[indeks].tinggi;
        tak_bertanda32_t ub;
        tak_bertanda32_t ofs_d, ofs_b;

        /* Lewati area kosong */
        if (aw == 0 || ah == 0) {
            continue;
        }

        /* Validasi area berada dalam buffer */
        if (ax >= buf_depan->lebar || ay >= buf_depan->tinggi) {
            hasil = STATUS_PARAM_JARAK;
            break;
        }
        if (ax + aw > buf_depan->lebar) {
            aw = buf_depan->lebar - ax;
        }
        if (ay + ah > buf_depan->tinggi) {
            ah = buf_depan->tinggi - ay;
        }
        if (aw == 0 || ah == 0) {
            continue;
        }

        ub = aw * (tak_bertanda32_t)bpp;

        /* Tukar area baris per baris */
        for (baris = 0; baris < ah; baris++) {
            ofs_d = (ay + baris) * buf_depan->pitch +
                ax * (tak_bertanda32_t)bpp;
            ofs_b = (ay + baris) * buf_belakang->pitch +
                ax * (tak_bertanda32_t)bpp;

            /* depan -> sementara */
            kernel_memcpy(sementara,
                          buf_depan->data + ofs_d,
                          (ukuran_t)ub);
            /* belakang -> depan */
            kernel_memcpy(buf_depan->data + ofs_d,
                          buf_belakang->data + ofs_b,
                          (ukuran_t)ub);
            /* sementara -> belakang */
            kernel_memcpy(buf_belakang->data + ofs_b,
                          sementara,
                          (ukuran_t)ub);
        }
    }

    /* Tandai kedua buffer kotor jika ada pertukaran */
    if (hasil == STATUS_BERHASIL) {
        buf_depan->flags |= BUFFER_FLAG_KOTOR;
        buf_belakang->flags |= BUFFER_FLAG_KOTOR;
    }

    /* Bersihkan dan bebaskan buffer sementara */
    if (sementara != NULL) {
        kernel_memset(sementara, 0, (ukuran_t)ukuran_baris_maks);
        kfree(sementara);
    }

    return hasil;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: PERBANDINGAN AREA DALAM SATU BUFFER                         */
/* ========================================================================== */

/*
 * tukar_bandingkan_area
 * --------------------
 * Membandingkan dua area persegi panjang dalam buffer yang sama.
 * Mengembalikan BENAR jika isi piksel kedua area identik.
 * Berguna untuk mendeteksi area yang berubah sebelum swap parsial.
 *
 * Parameter:
 *   buf - Pointer ke struct buffer piksel
 *   x1  - Koordinat X area pertama
 *   y1  - Koordinat Y area pertama
 *   x2  - Koordinat X area kedua
 *   y2  - Koordinat Y area kedua
 *   w   - Lebar area dalam piksel
 *   h   - Tinggi area dalam piksel
 *
 * Return: BENAR jika kedua area identik, SALAH jika berbeda
 *         atau jika terjadi error.
 */
bool_t tukar_bandingkan_area(const struct buffer_piksel *buf,
                              tak_bertanda32_t x1,
                              tak_bertanda32_t y1,
                              tak_bertanda32_t x2,
                              tak_bertanda32_t y2,
                              tak_bertanda32_t w,
                              tak_bertanda32_t h)
{
    tak_bertanda32_t baris;
    tak_bertanda8_t bpp;

    if (buf == NULL) {
        return SALAH;
    }
    if (!buffer_piksel_valid(buf)) {
        return SALAH;
    }

    if (w == 0 || h == 0) {
        return BENAR;
    }

    /* Validasi koordinat kedua area */
    if (!cek_koordinat_area(buf, x1, y1, w, h)) {
        return SALAH;
    }
    if (!cek_koordinat_area(buf, x2, y2, w, h)) {
        return SALAH;
    }

    bpp = (tak_bertanda8_t)bpp_dari_format(buf->format);

    /* Bandingkan setiap baris */
    for (baris = 0; baris < h; baris++) {
        tak_bertanda32_t ofs1 = (y1 + baris) * buf->pitch +
            x1 * (tak_bertanda32_t)bpp;
        tak_bertanda32_t ofs2 = (y2 + baris) * buf->pitch +
            x2 * (tak_bertanda32_t)bpp;
        ukuran_t ukuran_cmp = (ukuran_t)w * (ukuran_t)bpp;
        int perbedaan;

        perbedaan = kernel_memcmp(buf->data + ofs1,
                                   buf->data + ofs2,
                                   ukuran_cmp);
        if (perbedaan != 0) {
            return SALAH;
        }
    }

    return BENAR;
}
