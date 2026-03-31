/*
 * PIGURA OS - KANVAS_UBAHSUAI.C
 * ==============================
 * Operasi ubahsuai (resize) dan transformasi kanvas.
 *
 * Modul ini menyediakan fungsi-fungsi untuk mengubah ukuran,
 * format, memotong, menggabung, memutar, dan mencerminkan kanvas.
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
#define KANVAS_NAMA_MAKS       64
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
#define BUFFER_FLAG_KOTOR      0x08

#define bpp_dari_format(fmt) \
    (((fmt) == BUFFER_FMT_RGB332) ? 1 : \
     (((fmt) == BUFFER_FMT_RGB565) || \
      ((fmt) == BUFFER_FMT_BGR565)) ? 2 : \
     ((fmt) == BUFFER_FMT_RGB888) ? 3 : 4)

#define KANVAS_FLAG_READ_ONLY  0x01

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
    char nama[KANVAS_NAMA_MAKS];
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
    return RATAKAN_ATAS(lebar * bpp, 4);
}

static ukuran_t kv_hitung_ukuran(tak_bertanda32_t lebar,
                                  tak_bertanda32_t tinggi,
                                  tak_bertanda8_t format)
{
    return (ukuran_t)kv_hitung_pitch(lebar, format)
           * (ukuran_t)tinggi;
}

static struct buffer_piksel *kv_buat_buffer_kosong(
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
    buf->ukuran = kv_hitung_ukuran(lebar, tinggi, format);

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

static void kv_hapus_buffer(struct buffer_piksel *buf)
{
    if (buf == NULL) return;
    if ((buf->flags & BUFFER_FLAG_ALOKASI) &&
        buf->data != NULL) {
        kernel_memset(buf->data, 0, buf->ukuran);
        kfree(buf->data);
    }
    buf->magic = 0;
    kfree(buf);
}

/*
 * baca_piksel_fmt - Baca komponen RGBA dari buffer apapun.
 */
static void baca_piksel_fmt(const struct buffer_piksel *b,
                             tak_bertanda32_t ofs,
                             tak_bertanda8_t *r,
                             tak_bertanda8_t *g,
                             tak_bertanda8_t *bl,
                             tak_bertanda8_t *a)
{
    switch (b->format) {
    case BUFFER_FMT_RGB332: {
        tak_bertanda8_t pk = b->data[ofs];
        *r  = (tak_bertanda8_t)((pk >> 5) & 0x07) * 255 / 7;
        *g  = (tak_bertanda8_t)((pk >> 2) & 0x07) * 255 / 7;
        *bl = (tak_bertanda8_t)(pk & 0x03) * 255 / 3;
        *a  = 0xFF;
        break;
    }
    case BUFFER_FMT_RGB565:
    case BUFFER_FMT_BGR565: {
        tak_bertanda16_t pk = (tak_bertanda16_t)b->data[ofs];
        pk |= (tak_bertanda16_t)b->data[ofs + 1] << 8;
        *r  = (tak_bertanda8_t)((pk >> 11) & 0x1F) * 255 / 31;
        *g  = (tak_bertanda8_t)((pk >> 5) & 0x3F) * 255 / 63;
        *bl = (tak_bertanda8_t)(pk & 0x1F) * 255 / 31;
        *a  = 0xFF;
        break;
    }
    case BUFFER_FMT_RGB888:
        *r  = b->data[ofs];
        *g  = b->data[ofs + 1];
        *bl = b->data[ofs + 2];
        *a  = 0xFF;
        break;
    case BUFFER_FMT_ARGB8888:
    case BUFFER_FMT_XRGB8888:
    case BUFFER_FMT_BGRA8888:
        *bl = b->data[ofs];
        *g  = b->data[ofs + 1];
        *r  = b->data[ofs + 2];
        *a  = b->data[ofs + 3];
        break;
    default:
        *r = *g = *bl = 0; *a = 0;
        break;
    }
}

/*
 * tulis_piksel_fmt - Tulis komponen RGBA ke buffer apapun.
 */
static void tulis_piksel_fmt(struct buffer_piksel *b,
                              tak_bertanda32_t ofs,
                              tak_bertanda8_t r,
                              tak_bertanda8_t g,
                              tak_bertanda8_t bl,
                              tak_bertanda8_t a)
{
    switch (b->format) {
    case BUFFER_FMT_RGB332:
        b->data[ofs] = (tak_bertanda8_t)(
            ((r >> 5) << 5) | ((g >> 5) << 2) |
            (bl >> 6));
        break;
    case BUFFER_FMT_RGB565:
    case BUFFER_FMT_BGR565: {
        tak_bertanda16_t pk = (tak_bertanda16_t)(
            (((tak_bertanda16_t)r >> 3) << 11) |
            (((tak_bertanda16_t)g >> 2) << 5)  |
            ((tak_bertanda16_t)bl >> 3));
        b->data[ofs]     = (tak_bertanda8_t)(pk & 0xFF);
        b->data[ofs + 1] = (tak_bertanda8_t)(pk >> 8);
        break;
    }
    case BUFFER_FMT_RGB888:
        b->data[ofs]     = r;
        b->data[ofs + 1] = g;
        b->data[ofs + 2] = bl;
        break;
    case BUFFER_FMT_ARGB8888:
    case BUFFER_FMT_XRGB8888:
    case BUFFER_FMT_BGRA8888:
        b->data[ofs]     = bl;
        b->data[ofs + 1] = g;
        b->data[ofs + 2] = r;
        b->data[ofs + 3] = a;
        break;
    default:
        break;
    }
}

static tak_bertanda32_t kv_offset(const struct buffer_piksel *b,
                                   tak_bertanda32_t x,
                                   tak_bertanda32_t y)
{
    return y * b->pitch +
        x * (tak_bertanda32_t)bpp_dari_format(b->format);
}

/*
 * kv_inisialisasi_state - Isi state default kanvas.
 */
static void kv_inisialisasi_state(struct kanvas *kv)
{
    kv->aktif = BENAR;
    kv->flags = 0;
    kv->terkunci = SALAH;
    kv->pemilik_kunci = 0;
    kv->warna_r = 255;
    kv->warna_g = 255;
    kv->warna_b = 255;
    kv->warna_a = 255;
    kv->transformasi.jenis = 0;
    kv->transformasi.tx = 0;
    kv->transformasi.ty = 0;
    kv->transformasi.sx = 0x10000;
    kv->transformasi.sy = 0x10000;
    kv->klip.aktif = SALAH;
    kv->jumlah_gambar = 0;
    kv->kotor = SALAH;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: UBAH UKURAN                                                 */
/* ========================================================================== */

status_t kanvas_ubah_ukuran(struct kanvas *kv,
                              tak_bertanda32_t lebar_baru,
                              tak_bertanda32_t tinggi_baru)
{
    struct buffer_piksel *buf_baru;
    tak_bertanda32_t copy_w, copy_h, baris;
    ukuran_t uk_baris;

    if (kv == NULL || !kv_valid(kv)) {
        return STATUS_PARAM_NULL;
    }
    if (lebar_baru == 0 || tinggi_baru == 0) {
        return STATUS_PARAM_JARAK;
    }

    buf_baru = kv_buat_buffer_kosong(
        lebar_baru, tinggi_baru, kv->buffer->format);
    if (buf_baru == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    /* Salin area tumpang tindih */
    copy_w = MIN(kv->buffer->lebar, lebar_baru);
    copy_h = MIN(kv->buffer->tinggi, tinggi_baru);
    uk_baris = (ukuran_t)copy_w *
        (ukuran_t)bpp_dari_format(kv->buffer->format);

    for (baris = 0; baris < copy_h; baris++) {
        kernel_memcpy(
            buf_baru->data + (tak_bertanda32_t)baris *
                buf_baru->pitch,
            kv->buffer->data + (tak_bertanda32_t)baris *
                kv->buffer->pitch,
            uk_baris);
    }

    /* Ganti buffer */
    kv_hapus_buffer(kv->buffer);
    kv->buffer = buf_baru;

    /* Perbarui klip ke dimensi baru */
    kv->klip.x = 0;
    kv->klip.y = 0;
    kv->klip.lebar = lebar_baru;
    kv->klip.tinggi = tinggi_baru;

    kv->kotor = BENAR;
    return STATUS_BERHASIL;
}

status_t kanvas_ubah_ukuran_isi(struct kanvas *kv,
                                 tak_bertanda32_t lebar_baru,
                                 tak_bertanda32_t tinggi_baru,
                                 tak_bertanda8_t r,
                                 tak_bertanda8_t g,
                                 tak_bertanda8_t b,
                                 tak_bertanda8_t a)
{
    struct buffer_piksel *buf_baru;
    tak_bertanda32_t copy_w, copy_h, baris, px;
    ukuran_t uk_baris;

    if (kv == NULL || !kv_valid(kv)) {
        return STATUS_PARAM_NULL;
    }
    if (lebar_baru == 0 || tinggi_baru == 0) {
        return STATUS_PARAM_JARAK;
    }

    buf_baru = kv_buat_buffer_kosong(
        lebar_baru, tinggi_baru, kv->buffer->format);
    if (buf_baru == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    /* Isi seluruh buffer baru dengan warna */
    if (bpp_dari_format(kv->buffer->format) == 4) {
        tak_bertanda8_t tmp[4];
        tak_bertanda32_t raw;
        tmp[0] = b; tmp[1] = g; tmp[2] = r; tmp[3] = a;
        raw = (tak_bertanda32_t)tmp[0]       |
              ((tak_bertanda32_t)tmp[1] << 8)  |
              ((tak_bertanda32_t)tmp[2] << 16) |
              ((tak_bertanda32_t)tmp[3] << 24);
        kernel_memset32(buf_baru->data, raw,
                        (ukuran_t)lebar_baru *
                        (ukuran_t)tinggi_baru);
    } else {
        tak_bertanda32_t total =
            lebar_baru * tinggi_baru;
        tak_bertanda32_t i;
        for (i = 0; i < total; i++) {
            tulis_piksel_fmt(buf_baru, i, r, g, b, a);
        }
    }

    /* Salin konten lama yang tumpang tindih */
    copy_w = MIN(kv->buffer->lebar, lebar_baru);
    copy_h = MIN(kv->buffer->tinggi, tinggi_baru);
    uk_baris = (ukuran_t)copy_w *
        (ukuran_t)bpp_dari_format(kv->buffer->format);

    for (baris = 0; baris < copy_h; baris++) {
        kernel_memcpy(
            buf_baru->data + (tak_bertanda32_t)baris *
                buf_baru->pitch,
            kv->buffer->data + (tak_bertanda32_t)baris *
                kv->buffer->pitch,
            uk_baris);
    }

    kv_hapus_buffer(kv->buffer);
    kv->buffer = buf_baru;

    kv->klip.x = 0;
    kv->klip.y = 0;
    kv->klip.lebar = lebar_baru;
    kv->klip.tinggi = tinggi_baru;
    kv->kotor = BENAR;

    TIDAK_DIGUNAKAN_PARAM(px);
    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: UBAH FORMAT                                                 */
/* ========================================================================== */

status_t kanvas_ubah_format(struct kanvas *kv,
                              tak_bertanda8_t format_baru)
{
    struct buffer_piksel *buf_baru;
    tak_bertanda32_t x, y;
    tak_bertanda8_t r, g, b, a;
    tak_bertanda32_t ofs_lama, ofs_baru;

    if (kv == NULL || !kv_valid(kv)) {
        return STATUS_PARAM_NULL;
    }
    if (format_baru == BUFFER_FMT_INVALID ||
        format_baru > BUFFER_FMT_MAKS) {
        return STATUS_PARAM_INVALID;
    }
    if (kv->buffer->format == format_baru) {
        return STATUS_BERHASIL;
    }

    buf_baru = kv_buat_buffer_kosong(
        kv->buffer->lebar, kv->buffer->tinggi,
        format_baru);
    if (buf_baru == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    /* Konversi piksel per piksel */
    for (y = 0; y < kv->buffer->tinggi; y++) {
        for (x = 0; x < kv->buffer->lebar; x++) {
            ofs_lama = kv_offset(kv->buffer, x, y);
            baca_piksel_fmt(kv->buffer, ofs_lama,
                            &r, &g, &b, &a);
            ofs_baru = kv_offset(buf_baru, x, y);
            tulis_piksel_fmt(buf_baru, ofs_baru,
                              r, g, b, a);
        }
    }

    kv_hapus_buffer(kv->buffer);
    kv->buffer = buf_baru;
    kv->kotor = BENAR;
    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: POTONG (CROP)                                               */
/* ========================================================================== */

status_t kanvas_potong(struct kanvas *kv,
                         tak_bertanda32_t x,
                         tak_bertanda32_t y,
                         tak_bertanda32_t w,
                         tak_bertanda32_t h)
{
    struct buffer_piksel *buf_baru;
    tak_bertanda32_t baris;
    ukuran_t uk_baris;

    if (kv == NULL || !kv_valid(kv)) {
        return STATUS_PARAM_NULL;
    }
    if (w == 0 || h == 0) {
        return STATUS_PARAM_JARAK;
    }
    if (x >= kv->buffer->lebar ||
        y >= kv->buffer->tinggi) {
        return STATUS_PARAM_JARAK;
    }
    if (x + w > kv->buffer->lebar) {
        w = kv->buffer->lebar - x;
    }
    if (y + h > kv->buffer->tinggi) {
        h = kv->buffer->tinggi - y;
    }

    buf_baru = kv_buat_buffer_kosong(w, h,
                                      kv->buffer->format);
    if (buf_baru == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    uk_baris = (ukuran_t)w *
        (ukuran_t)bpp_dari_format(kv->buffer->format);

    for (baris = 0; baris < h; baris++) {
        kernel_memcpy(
            buf_baru->data + (tak_bertanda32_t)baris *
                buf_baru->pitch,
            kv->buffer->data + (y + baris) *
                kv->buffer->pitch + x *
                (tak_bertanda32_t)bpp_dari_format(
                    kv->buffer->format),
            uk_baris);
    }

    kv_hapus_buffer(kv->buffer);
    kv->buffer = buf_baru;

    kv->klip.x = 0;
    kv->klip.y = 0;
    kv->klip.lebar = w;
    kv->klip.tinggi = h;
    kv->kotor = BENAR;

    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: GABUNG (CONCATENATE)                                       */
/* ========================================================================== */

struct kanvas *kanvas_gabung_datar(const struct kanvas *a,
                                    const struct kanvas *b)
{
    struct kanvas *hasil;
    tak_bertanda32_t gabung_lebar, gabung_tinggi;
    ukuran_t uk_baris;

    if (a == NULL || b == NULL || !kv_valid(a) || !kv_valid(b)) {
        return NULL;
    }
    if (a->buffer->format != b->buffer->format) {
        return NULL;
    }

    gabung_lebar = a->buffer->lebar + b->buffer->lebar;
    gabung_tinggi = MIN(a->buffer->tinggi, b->buffer->tinggi);

    hasil = (struct kanvas *)kmalloc(sizeof(struct kanvas));
    if (hasil == NULL) {
        return NULL;
    }
    kernel_memset(hasil, 0, sizeof(struct kanvas));
    hasil->magic = KANVAS_MAGIC;
    hasil->versi = KANVAS_VERSI;

    hasil->buffer = kv_buat_buffer_kosong(
        gabung_lebar, gabung_tinggi, a->buffer->format);
    if (hasil->buffer == NULL) {
        kfree(hasil);
        return NULL;
    }

    /* Salin a ke kiri */
    uk_baris = (ukuran_t)a->buffer->lebar *
        (ukuran_t)bpp_dari_format(a->buffer->format);
    {
        tak_bertanda32_t baris;
        for (baris = 0; baris < gabung_tinggi; baris++) {
            kernel_memcpy(
                hasil->buffer->data +
                    (tak_bertanda32_t)baris *
                    hasil->buffer->pitch,
                a->buffer->data +
                    (tak_bertanda32_t)baris *
                    a->buffer->pitch,
                uk_baris);
        }
    }

    /* Salin b ke kanan */
    uk_baris = (ukuran_t)b->buffer->lebar *
        (ukuran_t)bpp_dari_format(b->buffer->format);
    {
        tak_bertanda32_t baris;
        for (baris = 0; baris < gabung_tinggi; baris++) {
            kernel_memcpy(
                hasil->buffer->data +
                    (tak_bertanda32_t)baris *
                    hasil->buffer->pitch +
                    a->buffer->lebar *
                    (tak_bertanda32_t)bpp_dari_format(
                        a->buffer->format),
                b->buffer->data +
                    (tak_bertanda32_t)baris *
                    b->buffer->pitch,
                uk_baris);
        }
    }

    kv_inisialisasi_state(hasil);
    return hasil;
}

struct kanvas *kanvas_gabung_vertikal(const struct kanvas *a,
                                       const struct kanvas *b)
{
    struct kanvas *hasil;
    tak_bertanda32_t gabung_lebar, gabung_tinggi;
    ukuran_t uk_baris;

    if (a == NULL || b == NULL || !kv_valid(a) || !kv_valid(b)) {
        return NULL;
    }
    if (a->buffer->format != b->buffer->format) {
        return NULL;
    }

    gabung_lebar = MIN(a->buffer->lebar, b->buffer->lebar);
    gabung_tinggi = a->buffer->tinggi + b->buffer->tinggi;

    hasil = (struct kanvas *)kmalloc(sizeof(struct kanvas));
    if (hasil == NULL) {
        return NULL;
    }
    kernel_memset(hasil, 0, sizeof(struct kanvas));
    hasil->magic = KANVAS_MAGIC;
    hasil->versi = KANVAS_VERSI;

    hasil->buffer = kv_buat_buffer_kosong(
        gabung_lebar, gabung_tinggi, a->buffer->format);
    if (hasil->buffer == NULL) {
        kfree(hasil);
        return NULL;
    }

    /* Salin a ke atas */
    uk_baris = (ukuran_t)gabung_lebar *
        (ukuran_t)bpp_dari_format(a->buffer->format);
    {
        tak_bertanda32_t baris;
        for (baris = 0; baris < a->buffer->tinggi; baris++) {
            kernel_memcpy(
                hasil->buffer->data +
                    (tak_bertanda32_t)baris *
                    hasil->buffer->pitch,
                a->buffer->data +
                    (tak_bertanda32_t)baris *
                    a->buffer->pitch,
                uk_baris);
        }
    }

    /* Salin b ke bawah */
    {
        tak_bertanda32_t baris;
        for (baris = 0; baris < b->buffer->tinggi; baris++) {
            kernel_memcpy(
                hasil->buffer->data +
                    (a->buffer->tinggi + baris) *
                    hasil->buffer->pitch,
                b->buffer->data +
                    (tak_bertanda32_t)baris *
                    b->buffer->pitch,
                uk_baris);
        }
    }

    kv_inisialisasi_state(hasil);
    return hasil;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: SKALA, PUTAR, CERMIN                                        */
/* ========================================================================== */

/* Forward declarations (dipakai oleh kanvas_putar_180) */
status_t kanvas_cermin_horizontal(struct kanvas *kv,
                                   tak_bertanda32_t x,
                                   tak_bertanda32_t y,
                                   tak_bertanda32_t w,
                                   tak_bertanda32_t h);
status_t kanvas_cermin_vertikal(struct kanvas *kv,
                                 tak_bertanda32_t x,
                                 tak_bertanda32_t y,
                                 tak_bertanda32_t w,
                                 tak_bertanda32_t h);

struct kanvas *kanvas_skala(const struct kanvas *kv,
                              tak_bertanda32_t faktor_x,
                              tak_bertanda32_t faktor_y)
{
    struct kanvas *hasil;
    tak_bertanda32_t baru_lebar, baru_tinggi, px, py;

    if (kv == NULL || !kv_valid(kv)) {
        return NULL;
    }
    if (faktor_x == 0 || faktor_y == 0) {
        return NULL;
    }

    /* Fixed-point 16.16: 0x10000 = 1.0x */
    baru_lebar = (tak_bertanda32_t)(
        ((tak_bertanda64_t)kv->buffer->lebar * faktor_x)
        >> 16);
    baru_tinggi = (tak_bertanda32_t)(
        ((tak_bertanda64_t)kv->buffer->tinggi * faktor_y)
        >> 16);

    if (baru_lebar == 0 || baru_tinggi == 0) {
        baru_lebar = 1;
        baru_tinggi = 1;
    }
    if (baru_lebar > 16384) baru_lebar = 16384;
    if (baru_tinggi > 16384) baru_tinggi = 16384;

    hasil = (struct kanvas *)kmalloc(sizeof(struct kanvas));
    if (hasil == NULL) {
        return NULL;
    }
    kernel_memset(hasil, 0, sizeof(struct kanvas));
    hasil->magic = KANVAS_MAGIC;
    hasil->versi = KANVAS_VERSI;

    hasil->buffer = kv_buat_buffer_kosong(
        baru_lebar, baru_tinggi, kv->buffer->format);
    if (hasil->buffer == NULL) {
        kfree(hasil);
        return NULL;
    }

    /* Nearest-neighbor scaling */
    for (py = 0; py < baru_tinggi; py++) {
        for (px = 0; px < baru_lebar; px++) {
            tak_bertanda32_t src_x = (tak_bertanda32_t)(
                ((tak_bertanda64_t)px << 16) / faktor_x);
            tak_bertanda32_t src_y = (tak_bertanda32_t)(
                ((tak_bertanda64_t)py << 16) / faktor_y);
            tak_bertanda8_t r, g, b, a;

            if (src_x >= kv->buffer->lebar) {
                src_x = kv->buffer->lebar - 1;
            }
            if (src_y >= kv->buffer->tinggi) {
                src_y = kv->buffer->tinggi - 1;
            }

            baca_piksel_fmt(kv->buffer,
                            kv_offset(kv->buffer,
                                       src_x, src_y),
                            &r, &g, &b, &a);
            tulis_piksel_fmt(hasil->buffer,
                              kv_offset(hasil->buffer,
                                         px, py),
                              r, g, b, a);
        }
    }

    kv_inisialisasi_state(hasil);
    return hasil;
}

struct kanvas *kanvas_putar_90(const struct kanvas *kv)
{
    struct kanvas *hasil;
    tak_bertanda32_t baru_lebar, baru_tinggi, px, py;

    if (kv == NULL || !kv_valid(kv)) {
        return NULL;
    }

    baru_lebar = kv->buffer->tinggi;
    baru_tinggi = kv->buffer->lebar;

    hasil = (struct kanvas *)kmalloc(sizeof(struct kanvas));
    if (hasil == NULL) {
        return NULL;
    }
    kernel_memset(hasil, 0, sizeof(struct kanvas));
    hasil->magic = KANVAS_MAGIC;
    hasil->versi = KANVAS_VERSI;

    hasil->buffer = kv_buat_buffer_kosong(
        baru_lebar, baru_tinggi, kv->buffer->format);
    if (hasil->buffer == NULL) {
        kfree(hasil);
        return NULL;
    }

    /* Rotasi 90 derajat searah jarum jam:
     * dst(x,y) = src(y, lebar-1-x) */
    for (py = 0; py < baru_tinggi; py++) {
        for (px = 0; px < baru_lebar; px++) {
            tak_bertanda32_t src_x = kv->buffer->lebar - 1 - py;
            tak_bertanda32_t src_y = px;
            tak_bertanda8_t r, g, b, a;

            baca_piksel_fmt(kv->buffer,
                            kv_offset(kv->buffer,
                                       src_x, src_y),
                            &r, &g, &b, &a);
            tulis_piksel_fmt(hasil->buffer,
                              kv_offset(hasil->buffer, px, py),
                              r, g, b, a);
        }
    }

    kv_inisialisasi_state(hasil);
    return hasil;
}

status_t kanvas_putar_180(struct kanvas *kv)
{
    status_t st;

    if (kv == NULL || !kv_valid(kv)) {
        return STATUS_PARAM_NULL;
    }

    st = kanvas_cermin_horizontal(kv, 0, 0,
                                  kv->buffer->lebar,
                                  kv->buffer->tinggi);
    if (st != STATUS_BERHASIL) {
        return st;
    }

    st = kanvas_cermin_vertikal(kv, 0, 0,
                                 kv->buffer->lebar,
                                 kv->buffer->tinggi);
    return st;
}

status_t kanvas_cermin_horizontal(struct kanvas *kv,
                                   tak_bertanda32_t x,
                                   tak_bertanda32_t y,
                                   tak_bertanda32_t w,
                                   tak_bertanda32_t h)
{
    tak_bertanda8_t bpp;
    tak_bertanda8_t *temp;
    tak_bertanda32_t baris, kiri, kanan;

    if (kv == NULL || !kv_valid(kv)) {
        return STATUS_PARAM_NULL;
    }
    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }
    if (x >= kv->buffer->lebar ||
        y >= kv->buffer->tinggi) {
        return STATUS_PARAM_JARAK;
    }
    if (x + w > kv->buffer->lebar) {
        w = kv->buffer->lebar - x;
    }
    if (y + h > kv->buffer->tinggi) {
        h = kv->buffer->tinggi - y;
    }

    bpp = (tak_bertanda8_t)bpp_dari_format(kv->buffer->format);
    temp = (tak_bertanda8_t *)kmalloc((ukuran_t)w * (ukuran_t)bpp);
    if (temp == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    for (baris = 0; baris < h; baris++) {
        tak_bertanda32_t ofs = (y + baris) *
            kv->buffer->pitch + x * (tak_bertanda32_t)bpp;
        tak_bertanda8_t *baris_ptr = kv->buffer->data + ofs;

        /* Simpan baris ke temp */
        kernel_memcpy(temp, baris_ptr, (ukuran_t)w * (ukuran_t)bpp);

        /* Balikkan piksel dalam baris */
        for (kiri = 0, kanan = w - 1; kiri < kanan;
             kiri++, kanan--) {
            tak_bertanda8_t pik_kiri[4], pik_kanan[4];
            ukuran_t uk_px = (ukuran_t)bpp;

            kernel_memcpy(pik_kiri,
                          baris_ptr + kiri * (tak_bertanda32_t)bpp,
                          uk_px);
            kernel_memcpy(pik_kanan,
                          baris_ptr + kanan * (tak_bertanda32_t)bpp,
                          uk_px);

            kernel_memcpy(
                baris_ptr + kiri * (tak_bertanda32_t)bpp,
                pik_kanan, uk_px);
            kernel_memcpy(
                baris_ptr + kanan * (tak_bertanda32_t)bpp,
                pik_kiri, uk_px);
        }
    }

    kfree(temp);
    kv->buffer->flags |= BUFFER_FLAG_KOTOR;
    kv->kotor = BENAR;
    return STATUS_BERHASIL;
}

status_t kanvas_cermin_vertikal(struct kanvas *kv,
                                 tak_bertanda32_t x,
                                 tak_bertanda32_t y,
                                 tak_bertanda32_t w,
                                 tak_bertanda32_t h)
{
    tak_bertanda8_t bpp;
    tak_bertanda8_t *temp;
    tak_bertanda32_t __attribute__((unused)) baris;
    ukuran_t uk_baris;
    tak_bertanda32_t atas, bawah;

    if (kv == NULL || !kv_valid(kv)) {
        return STATUS_PARAM_NULL;
    }
    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }
    if (x >= kv->buffer->lebar ||
        y >= kv->buffer->tinggi) {
        return STATUS_PARAM_JARAK;
    }
    if (x + w > kv->buffer->lebar) {
        w = kv->buffer->lebar - x;
    }
    if (y + h > kv->buffer->tinggi) {
        h = kv->buffer->tinggi - y;
    }

    bpp = (tak_bertanda8_t)bpp_dari_format(kv->buffer->format);
    uk_baris = (ukuran_t)w * (ukuran_t)bpp;
    temp = (tak_bertanda8_t *)kmalloc(uk_baris);
    if (temp == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    atas = y;
    bawah = y + h - 1;

    while (atas < bawah) {
        tak_bertanda8_t *ptr_atas = kv->buffer->data +
            (tak_bertanda32_t)atas * kv->buffer->pitch +
            x * (tak_bertanda32_t)bpp;
        tak_bertanda8_t *ptr_bawah = kv->buffer->data +
            (tak_bertanda32_t)bawah * kv->buffer->pitch +
            x * (tak_bertanda32_t)bpp;

        kernel_memcpy(temp, ptr_atas, uk_baris);
        kernel_memcpy(ptr_atas, ptr_bawah, uk_baris);
        kernel_memcpy(ptr_bawah, temp, uk_baris);

        atas++;
        bawah--;
    }

    kfree(temp);
    kv->buffer->flags |= BUFFER_FLAG_KOTOR;
    kv->kotor = BENAR;
    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: UBAH OFFSET (SCROLL)                                        */
/* ========================================================================== */

status_t kanvas_ubah_offset(struct kanvas *kv,
                              tanda32_t offset_x,
                              tanda32_t offset_y)
{
    struct buffer_piksel *buf;
    tak_bertanda32_t lebar, tinggi;
    tak_bertanda8_t bpp;
    ukuran_t uk_baris;
    tanda32_t dx, dy;

    if (kv == NULL || !kv_valid(kv)) {
        return STATUS_PARAM_NULL;
    }

    buf = kv->buffer;
    lebar = buf->lebar;
    tinggi = buf->tinggi;
    bpp = (tak_bertanda8_t)bpp_dari_format(buf->format);
    uk_baris = (ukuran_t)lebar * (ukuran_t)bpp;

    if (offset_x == 0 && offset_y == 0) {
        return STATUS_BERHASIL;
    }

    /* Normalisasi offset ke rentang valid */
    dx = offset_x % (tanda32_t)lebar;
    dy = offset_y % (tanda32_t)tinggi;

    if (dx > 0 && dx < (tanda32_t)lebar) {
        tak_bertanda32_t baris;
        ukuran_t uk_geser = (ukuran_t)dx * (ukuran_t)bpp;
        for (baris = 0; baris < tinggi; baris++) {
            tak_bertanda8_t *ptr = buf->data +
                (tak_bertanda32_t)baris * buf->pitch;
            kernel_memmove(ptr, ptr + uk_geser,
                           uk_baris - uk_geser);
            kernel_memset(ptr + uk_baris - uk_geser, 0,
                          uk_geser);
        }
    } else if (dx < 0) {
        dx = -dx;
        if (dx < (tanda32_t)lebar) {
            tak_bertanda32_t baris;
            ukuran_t uk_geser = (ukuran_t)dx * (ukuran_t)bpp;
            for (baris = 0; baris < tinggi; baris++) {
                tak_bertanda8_t *ptr = buf->data +
                    (tak_bertanda32_t)baris * buf->pitch;
                kernel_memmove(ptr + uk_geser, ptr,
                               uk_baris - uk_geser);
                kernel_memset(ptr, 0, uk_geser);
            }
        }
    }

    if (dy > 0 && dy < (tanda32_t)tinggi) {
        tanda32_t baris;
        for (baris = (tanda32_t)tinggi - 1;
             baris >= dy; baris--) {
            kernel_memcpy(
                buf->data +
                    (tak_bertanda32_t)baris * buf->pitch,
                buf->data +
                    (tak_bertanda32_t)(baris - dy) * buf->pitch,
                uk_baris);
        }
        for (baris = 0; baris < dy; baris++) {
            kernel_memset(
                buf->data +
                    (tak_bertanda32_t)baris * buf->pitch,
                0, uk_baris);
        }
    } else if (dy < 0) {
        dy = -dy;
        if (dy < (tanda32_t)tinggi) {
            tanda32_t baris;
            for (baris = 0;
                 baris < (tanda32_t)tinggi - dy;
                 baris++) {
                kernel_memcpy(
                    buf->data +
                        (tak_bertanda32_t)baris * buf->pitch,
                    buf->data +
                        (tak_bertanda32_t)(baris + dy) *
                        buf->pitch,
                    uk_baris);
            }
            for (baris = (tanda32_t)tinggi - dy;
                 baris < (tanda32_t)tinggi;
                 baris++) {
                kernel_memset(
                    buf->data +
                        (tak_bertanda32_t)baris * buf->pitch,
                    0, uk_baris);
            }
        }
    }

    buf->flags |= BUFFER_FLAG_KOTOR;
    kv->kotor = BENAR;
    kv->jumlah_gambar++;
    return STATUS_BERHASIL;
}
