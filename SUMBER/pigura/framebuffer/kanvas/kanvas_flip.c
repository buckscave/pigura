/*
 * PIGURA OS - KANVAS_FLIP.C
 * =========================
 * Operasi pertukaran buffer (page-flip) dan double buffering.
 *
 * Modul ini menyediakan fungsi-fungsi untuk menukar buffer depan
 * dan belakang pada kanvas, mendukung mekanisme page-flip klasik
 * maupun salin memori untuk buffer yang dipetakan ke hardware.
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
/* KONSTANTA                                                                  */
/* ========================================================================== */

#define KANVAS_MAGIC           0x4B564153
#define KANVAS_VERSI           0x0100
#define BUFFER_MAGIC           0x42554642
#define BUFFER_FMT_INVALID     0
#define BUFFER_FMT_RGB332      1
#define BUFFER_FMT_RGB565      2
#define BUFFER_FMT_RGB888      3
#define BUFFER_FMT_ARGB8888    4
#define BUFFER_FMT_XRGB8888    5
#define BUFFER_FMT_BGRA8888    6
#define BUFFER_FMT_BGR565      7
#define BUFFER_FMT_MAKS        7
#define BUFFER_FLAG_ALOKASI    0x01
#define BUFFER_FLAG_EKSTERNAL  0x02
#define BUFFER_FLAG_KOTOR      0x08

#define bpp_dari_format(fmt) \
    (((fmt) == BUFFER_FMT_RGB332) ? 1 : \
     (((fmt) == BUFFER_FMT_RGB565) || \
      ((fmt) == BUFFER_FMT_BGR565)) ? 2 : \
     ((fmt) == BUFFER_FMT_RGB888) ? 3 : 4)

#define KANVAS_FLAG_READ_ONLY  0x01
#define KANVAS_FLAG_DOUBLE_BUF 0x02

/* ========================================================================== */
/* STRUKTUR DATA                                                              */
/* ========================================================================== */

struct buffer_piksel {
    tak_bertanda32_t magic;
    tak_bertanda16_t versi;
    tak_bertanda32_t lebar;
    tak_bertanda32_t tinggi;
    tak_bertanda8_t  format;
    tak_bertanda8_t  bit_per_piksel;
    tak_bertanda32_t pitch;
    ukuran_t         ukuran;
    tak_bertanda8_t *data;
    tak_bertanda32_t flags;
    tak_bertanda32_t referensi;
};

struct kanvas_transformasi {
    tak_bertanda32_t jenis;
    tanda32_t tx, ty;
    tak_bertanda32_t sx, sy;
};

struct kanvas_klip {
    bool_t aktif;
    tak_bertanda32_t x, y, lebar, tinggi;
};

struct kanvas {
    tak_bertanda32_t magic;
    tak_bertanda16_t versi;
    struct buffer_piksel *buffer;
    char nama[64];
    tak_bertanda32_t id;
    bool_t aktif;
    tak_bertanda32_t flags;
    bool_t terkunci;
    tak_bertanda32_t pemilik_kunci;
    tak_bertanda8_t warna_r, warna_g, warna_b, warna_a;
    struct kanvas_transformasi transformasi;
    struct kanvas_klip klip;
    tak_bertanda32_t jumlah_gambar;
    bool_t kotor;
};

/* ========================================================================== */
/* FUNGSI HELPER                                                              */
/* ========================================================================== */

static bool_t kv_valid(const struct kanvas *k)
{
    return (k != NULL && k->magic == KANVAS_MAGIC &&
            k->buffer != NULL &&
            k->buffer->magic == BUFFER_MAGIC &&
            k->buffer->data != NULL) ? BENAR : SALAH;
}

static tak_bertanda32_t kv_hitung_pitch(tak_bertanda32_t lebar,
                                         tak_bertanda8_t format)
{
    tak_bertanda32_t bpp = (tak_bertanda32_t)
        bpp_dari_format(format);
    tak_bertanda32_t kasar = lebar * bpp;
    return RATAKAN_ATAS(kasar, 4);
}

static bool_t kv_format_cocok(const struct kanvas *a,
                               const struct kanvas *b)
{
    if (a->buffer->lebar != b->buffer->lebar)
        return SALAH;
    if (a->buffer->tinggi != b->buffer->tinggi)
        return SALAH;
    if (a->buffer->format != b->buffer->format)
        return SALAH;
    return BENAR;
}

/*
 * kv_buat_buffer - Buat buffer_piksel baru secara internal.
 */
static struct buffer_piksel *kv_buat_buffer(
    tak_bertanda32_t lebar,
    tak_bertanda32_t tinggi,
    tak_bertanda8_t format)
{
    struct buffer_piksel *buf;

    if (lebar == 0 || tinggi == 0 ||
        format == BUFFER_FMT_INVALID ||
        format > BUFFER_FMT_MAKS) {
        return NULL;
    }

    buf = (struct buffer_piksel *)kmalloc(
        sizeof(struct buffer_piksel));
    if (buf == NULL) {
        return NULL;
    }

    kernel_memset(buf, 0, sizeof(struct buffer_piksel));

    buf->magic = BUFFER_MAGIC;
    buf->versi = KANVAS_VERSI;
    buf->lebar = lebar;
    buf->tinggi = tinggi;
    buf->format = format;
    buf->bit_per_piksel = (tak_bertanda8_t)(
        bpp_dari_format(format) * 8);
    buf->pitch = kv_hitung_pitch(lebar, format);
    buf->ukuran = (ukuran_t)buf->pitch * (ukuran_t)tinggi;

    buf->data = (tak_bertanda8_t *)kmalloc(buf->ukuran);
    if (buf->data == NULL) {
        kernel_memset(buf, 0, sizeof(struct buffer_piksel));
        kfree(buf);
        return NULL;
    }

    kernel_memset(buf->data, 0, buf->ukuran);
    buf->flags = BUFFER_FLAG_ALOKASI;
    buf->referensi = 1;
    return buf;
}

/*
 * kv_hapus_buffer - Hapus buffer_piksel secara internal.
 */
static void kv_hapus_buffer(struct buffer_piksel *buf)
{
    if (buf == NULL) {
        return;
    }
    if ((buf->flags & BUFFER_FLAG_ALOKASI) &&
        buf->data != NULL) {
        kernel_memset(buf->data, 0, buf->ukuran);
        kfree(buf->data);
    }
    buf->magic = 0;
    kfree(buf);
}

/* ========================================================================== */
/* FUNGSI PUBLIK: FLIP (TUKAR POINTER)                                        */
/* ========================================================================== */

status_t kanvas_flip(struct kanvas *kanvas_depan,
                      struct kanvas *kanvas_belakang)
{
    struct buffer_piksel *tmp;

    if (kanvas_depan == NULL || kanvas_belakang == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (!kv_valid(kanvas_depan) || !kv_valid(kanvas_belakang)) {
        return STATUS_PARAM_INVALID;
    }
    if (!kv_format_cocok(kanvas_depan, kanvas_belakang)) {
        return STATUS_PARAM_UKURAN;
    }

    /* Tukar pointer buffer */
    tmp = kanvas_depan->buffer;
    kanvas_depan->buffer = kanvas_belakang->buffer;
    kanvas_belakang->buffer = tmp;

    kanvas_depan->jumlah_gambar++;
    kanvas_belakang->jumlah_gambar++;
    kanvas_depan->kotor = BENAR;
    kanvas_belakang->kotor = BENAR;

    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: FLIP DENGAN SALIN                                           */
/* ========================================================================== */

status_t kanvas_flip_dengan_salin(struct kanvas *kanvas_depan,
                                   struct kanvas *kanvas_belakang)
{
    tak_bertanda32_t baris;
    ukuran_t uk_salin;

    if (kanvas_depan == NULL || kanvas_belakang == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (!kv_valid(kanvas_depan) || !kv_valid(kanvas_belakang)) {
        return STATUS_PARAM_INVALID;
    }
    if (!kv_format_cocok(kanvas_depan, kanvas_belakang)) {
        return STATUS_PARAM_UKURAN;
    }

    uk_salin = MIN(kanvas_depan->buffer->ukuran,
                   kanvas_belakang->buffer->ukuran);

    /* Salin baris per baris untuk pitch yang mungkin berbeda */
    for (baris = 0; baris < kanvas_depan->buffer->tinggi;
         baris++) {
        const tak_bertanda8_t *src_ptr =
            kanvas_belakang->buffer->data +
            (tak_bertanda32_t)baris *
            kanvas_belakang->buffer->pitch;
        tak_bertanda8_t *dst_ptr =
            kanvas_depan->buffer->data +
            (tak_bertanda32_t)baris *
            kanvas_depan->buffer->pitch;
        ukuran_t uk_baris = (ukuran_t)
            kanvas_depan->buffer->lebar *
            (ukuran_t)bpp_dari_format(
                kanvas_depan->buffer->format);
        kernel_memcpy(dst_ptr, src_ptr, uk_baris);
    }

    kanvas_depan->buffer->flags |= BUFFER_FLAG_KOTOR;
    kanvas_depan->kotor = BENAR;
    kanvas_depan->jumlah_gambar++;

    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: FLIP SEGERA                                                 */
/* ========================================================================== */

status_t kanvas_flip_segera(struct kanvas *kanvas_depan,
                             struct kanvas *kanvas_belakang)
{
    struct buffer_piksel *tmp;

    if (kanvas_depan == NULL || kanvas_belakang == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (!kv_valid(kanvas_depan) || !kv_valid(kanvas_belakang)) {
        return STATUS_PARAM_INVALID;
    }
    if (!kv_format_cocok(kanvas_depan, kanvas_belakang)) {
        return STATUS_PARAM_UKURAN;
    }

    tmp = kanvas_depan->buffer;
    kanvas_depan->buffer = kanvas_belakang->buffer;
    kanvas_belakang->buffer = tmp;

    kanvas_depan->jumlah_gambar++;
    kanvas_belakang->jumlah_gambar++;
    kanvas_depan->kotor = BENAR;
    kanvas_belakang->kotor = BENAR;

    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: AKTIFKAN/NONAKTIFKAN DOUBLE BUFFER                       */
/* ========================================================================== */

struct buffer_piksel *kanvas_aktifkan_double_buf(
    struct kanvas *kv)
{
    struct buffer_piksel *back;

    if (kv == NULL || !kv_valid(kv)) {
        return NULL;
    }

    if (kv->flags & KANVAS_FLAG_DOUBLE_BUF) {
        return NULL;
    }

    back = kv_buat_buffer(kv->buffer->lebar,
                           kv->buffer->tinggi,
                           kv->buffer->format);
    if (back == NULL) {
        return NULL;
    }

    /* Salin konten depan ke belakang */
    kernel_memcpy(back->data, kv->buffer->data,
                  MIN(back->ukuran, kv->buffer->ukuran));

    kv->flags |= KANVAS_FLAG_DOUBLE_BUF;
    return back;
}

status_t kanvas_nonaktifkan_double_buf(
    struct kanvas *kv,
    struct buffer_piksel *back_buffer)
{
    if (kv == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (kv->magic != KANVAS_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    kv->flags &= (tak_bertanda32_t)(~KANVAS_FLAG_DOUBLE_BUF);

    if (back_buffer != NULL &&
        back_buffer->magic == BUFFER_MAGIC) {
        kv_hapus_buffer(back_buffer);
    }

    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: SALIN KE/DARI BELAKANG                                      */
/* ========================================================================== */

status_t kanvas_salin_ke_belakang(struct kanvas *kv,
                                   const struct buffer_piksel *bb)
{
    tak_bertanda32_t baris;

    if (kv == NULL || bb == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (!kv_valid(kv)) {
        return STATUS_PARAM_INVALID;
    }
    if (bb->magic != BUFFER_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (!kv_format_cocok(
            kv, (const struct kanvas *)bb)) {
        return STATUS_PARAM_UKURAN;
    }

    for (baris = 0; baris < kv->buffer->tinggi; baris++) {
        ukuran_t uk_baris = (ukuran_t)
            kv->buffer->lebar *
            (ukuran_t)bpp_dari_format(kv->buffer->format);
        kernel_memcpy(
            bb->data + (tak_bertanda32_t)baris * bb->pitch,
            kv->buffer->data +
                (tak_bertanda32_t)baris *
                kv->buffer->pitch,
            uk_baris);
    }

    return STATUS_BERHASIL;
}

status_t kanvas_salin_dari_belakang(
    struct kanvas *kv,
    const struct buffer_piksel *bb)
{
    tak_bertanda32_t baris;

    if (kv == NULL || bb == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (!kv_valid(kv)) {
        return STATUS_PARAM_INVALID;
    }
    if (bb->magic != BUFFER_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (!kv_format_cocok(
            kv, (const struct kanvas *)bb)) {
        return STATUS_PARAM_UKURAN;
    }

    for (baris = 0; baris < kv->buffer->tinggi; baris++) {
        ukuran_t uk_baris = (ukuran_t)
            kv->buffer->lebar *
            (ukuran_t)bpp_dari_format(kv->buffer->format);
        kernel_memcpy(
            kv->buffer->data +
                (tak_bertanda32_t)baris *
                kv->buffer->pitch,
            bb->data + (tak_bertanda32_t)baris * bb->pitch,
            uk_baris);
    }

    kv->buffer->flags |= BUFFER_FLAG_KOTOR;
    kv->kotor = BENAR;
    kv->jumlah_gambar++;

    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: TUKAR DAN BANDINGKAN BUFFER                                 */
/* ========================================================================== */

status_t kanvas_tukar_buffer(struct buffer_piksel *a,
                              struct buffer_piksel *b)
{
    struct buffer_piksel *tmp;

    if (a == NULL || b == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (a->magic != BUFFER_MAGIC ||
        b->magic != BUFFER_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    tmp = a;
    TIDAK_DIGUNAKAN_PARAM(tmp);

    /* Tukar seluruh konten struct antara a dan b */
    {
        struct buffer_piksel simpan;
        kernel_memcpy(&simpan, a, sizeof(struct buffer_piksel));
        kernel_memcpy(a, b, sizeof(struct buffer_piksel));
        kernel_memcpy(b, &simpan, sizeof(struct buffer_piksel));
    }

    return STATUS_BERHASIL;
}

bool_t kanvas_bandingkan_buffer(const struct buffer_piksel *a,
                                 const struct buffer_piksel *b)
{
    ukuran_t uk;

    if (a == NULL || b == NULL) {
        return SALAH;
    }
    if (a->magic != BUFFER_MAGIC ||
        b->magic != BUFFER_MAGIC) {
        return SALAH;
    }
    if (a->lebar != b->lebar ||
        a->tinggi != b->tinggi ||
        a->format != b->format) {
        return SALAH;
    }

    uk = MIN(a->ukuran, b->ukuran);
    return (kernel_memcmp(a->data, b->data, uk) == 0)
           ? BENAR : SALAH;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: SALIN PENUH                                                 */
/* ========================================================================== */

status_t kanvas_salin_penuh(struct kanvas *dst,
                              const struct kanvas *src)
{
    if (dst == NULL || src == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (!kv_valid(dst) || !kv_valid(src)) {
        return STATUS_PARAM_INVALID;
    }
    if (!kv_format_cocok(dst, src)) {
        return STATUS_PARAM_UKURAN;
    }

    kernel_memcpy(dst->buffer->data, src->buffer->data,
                  MIN(dst->buffer->ukuran,
                      src->buffer->ukuran));

    dst->buffer->flags |= BUFFER_FLAG_KOTOR;
    dst->kotor = BENAR;
    dst->jumlah_gambar++;

    return STATUS_BERHASIL;
}
