/*
 * PIGURA OS - DAMPAK.C
 * =====================
 * Operasi blit (block transfer) antar buffer piksel.
 *
 * Modul ini menyediakan fungsi-fungsi penyalinan, penggabungan,
 * pembesaran/pengecilan, pencerminan, dan rotasi antar buffer
 * piksel. Seluruh operasi menggunakan byte-level akses untuk
 * portabilitas lintas arsitektur.
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
/* KONSTANTA BUFFER                                                           */
/* ========================================================================== */

#define BUFFER_MAGIC           0x42554642
#define BUFFER_VERSI           0x0100
#define BUFFER_DIMENSI_MAKS    16384

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
#define BUFFER_FLAG_KUNCI      0x04
#define BUFFER_FLAG_KOTOR      0x08

/* ========================================================================== */
/* MAKRO HELPER                                                               */
/* ========================================================================== */

#define bpp_dari_format(fmt) \
    (((fmt) == BUFFER_FMT_RGB332) ? 1 : \
     (((fmt) == BUFFER_FMT_RGB565) || \
      ((fmt) == BUFFER_FMT_BGR565)) ? 2 : \
     ((fmt) == BUFFER_FMT_RGB888) ? 3 : 4)

/* ========================================================================== */
/* STRUKTUR DATA BUFFER                                                       */
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

/* ========================================================================== */
/* FUNGSI HELPER INTERNAL: BACA/TULIS PIKSEL                                  */
/* ========================================================================== */

static void baca_piksel(const struct buffer_piksel *buf,
                        tak_bertanda32_t x,
                        tak_bertanda32_t y,
                        tak_bertanda8_t *r,
                        tak_bertanda8_t *g,
                        tak_bertanda8_t *b,
                        tak_bertanda8_t *a)
{
    tak_bertanda32_t ofs;
    tak_bertanda8_t bpp;
    const tak_bertanda8_t *pk;

    bpp = (tak_bertanda8_t)bpp_dari_format(buf->format);
    ofs = y * buf->pitch + x * (tak_bertanda32_t)bpp;
    pk = buf->data + ofs;

    switch (bpp) {
    case 1: {
        tak_bertanda8_t v = pk[0];
        *r = (tak_bertanda8_t)((v >> 5) & 0x07) * 255 / 7;
        *g = (tak_bertanda8_t)((v >> 2) & 0x07) * 255 / 7;
        *b = (tak_bertanda8_t)(v & 0x03) * 255 / 3;
        *a = 0xFF;
        break;
    }
    case 2: {
        tak_bertanda16_t v = (tak_bertanda16_t)pk[0];
        v |= (tak_bertanda16_t)((tak_bertanda16_t)pk[1] << 8);
        *r = (tak_bertanda8_t)((v >> 11) & 0x1F) * 255 / 31;
        *g = (tak_bertanda8_t)((v >> 5) & 0x3F) * 255 / 63;
        *b = (tak_bertanda8_t)(v & 0x1F) * 255 / 31;
        *a = 0xFF;
        break;
    }
    case 3:
        *r = pk[0];
        *g = pk[1];
        *b = pk[2];
        *a = 0xFF;
        break;
    case 4:
        *b = pk[0];
        *g = pk[1];
        *r = pk[2];
        *a = pk[3];
        break;
    default:
        *r = 0; *g = 0; *b = 0; *a = 0;
        break;
    }
}

static void tulis_piksel(struct buffer_piksel *buf,
                         tak_bertanda32_t x,
                         tak_bertanda32_t y,
                         tak_bertanda8_t r,
                         tak_bertanda8_t g,
                         tak_bertanda8_t b,
                         tak_bertanda8_t a)
{
    tak_bertanda32_t ofs;
    tak_bertanda8_t bpp;
    tak_bertanda8_t *pk;

    bpp = (tak_bertanda8_t)bpp_dari_format(buf->format);
    ofs = y * buf->pitch + x * (tak_bertanda32_t)bpp;
    pk = buf->data + ofs;

    switch (bpp) {
    case 1:
        pk[0] = (tak_bertanda8_t)(
            ((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6));
        break;
    case 2: {
        tak_bertanda16_t v = (tak_bertanda16_t)(
            (((tak_bertanda16_t)r >> 3) << 11) |
            (((tak_bertanda16_t)g >> 2) << 5)  |
            ((tak_bertanda16_t)b >> 3));
        pk[0] = (tak_bertanda8_t)(v & 0xFF);
        pk[1] = (tak_bertanda8_t)(v >> 8);
        break;
    }
    case 3:
        pk[0] = r;
        pk[1] = g;
        pk[2] = b;
        break;
    case 4:
        pk[0] = b;
        pk[1] = g;
        pk[2] = r;
        pk[3] = a;
        break;
    default:
        break;
    }
}

/* ========================================================================== */
/* FUNGSI HELPER INTERNAL: VALIDASI                                           */
/* ========================================================================== */

static bool_t buf_valid(const struct buffer_piksel *buf)
{
    if (buf == NULL) { return SALAH; }
    if (buf->magic != BUFFER_MAGIC) { return SALAH; }
    if (buf->data == NULL) { return SALAH; }
    if (buf->format == BUFFER_FMT_INVALID ||
        buf->format > BUFFER_FMT_MAKS) { return SALAH; }
    return BENAR;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: DAMPAK_SALIN                                                */
/* ========================================================================== */

status_t dampak_salin(struct buffer_piksel *dst,
                      const struct buffer_piksel *src,
                      tak_bertanda32_t dx,
                      tak_bertanda32_t dy,
                      tak_bertanda32_t sx,
                      tak_bertanda32_t sy,
                      tak_bertanda32_t w,
                      tak_bertanda32_t h)
{
    tak_bertanda32_t baris;
    tak_bertanda8_t bpp;

    if (!buf_valid(dst) || !buf_valid(src)) {
        return STATUS_PARAM_NULL;
    }
    if (dst->format != src->format) {
        return STATUS_PARAM_INVALID;
    }
    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }
    if (sx >= src->lebar || sy >= src->tinggi ||
        dx >= dst->lebar || dy >= dst->tinggi) {
        return STATUS_PARAM_JARAK;
    }

    w = MIN(w, MIN(src->lebar - sx, dst->lebar - dx));
    h = MIN(h, MIN(src->tinggi - sy, dst->tinggi - dy));
    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }

    bpp = (tak_bertanda8_t)bpp_dari_format(dst->format);

    for (baris = 0; baris < h; baris++) {
        tak_bertanda8_t *sp = src->data +
            (sy + baris) * src->pitch +
            sx * (tak_bertanda32_t)bpp;
        tak_bertanda8_t *dp = dst->data +
            (dy + baris) * dst->pitch +
            dx * (tak_bertanda32_t)bpp;
        kernel_memcpy(dp, sp, (ukuran_t)w * (ukuran_t)bpp);
    }

    dst->flags |= BUFFER_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: DAMPAK_SALIN_PENUH                                         */
/* ========================================================================== */

status_t dampak_salin_penuh(struct buffer_piksel *dst,
                            const struct buffer_piksel *src)
{
    if (!buf_valid(dst) || !buf_valid(src)) {
        return STATUS_PARAM_NULL;
    }
    if (dst->lebar != src->lebar ||
        dst->tinggi != src->tinggi ||
        dst->format != src->format) {
        return STATUS_PARAM_UKURAN;
    }

    kernel_memcpy(dst->data, src->data,
                  MIN(dst->ukuran, src->ukuran));
    dst->flags |= BUFFER_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: DAMPAK_UBAH_SKALA                                          */
/* ========================================================================== */

status_t dampak_ubah_skala(struct buffer_piksel *dst,
                           const struct buffer_piksel *src,
                           tak_bertanda32_t dx,
                           tak_bertanda32_t dy,
                           tak_bertanda32_t dw,
                           tak_bertanda32_t dh,
                           tak_bertanda32_t sx,
                           tak_bertanda32_t sy,
                           tak_bertanda32_t sw,
                           tak_bertanda32_t sh)
{
    tak_bertanda32_t j, i;
    tak_bertanda8_t r, g, b, a;

    if (!buf_valid(dst) || !buf_valid(src)) {
        return STATUS_PARAM_NULL;
    }
    if (dst->format != src->format) {
        return STATUS_PARAM_INVALID;
    }
    if (dw == 0 || dh == 0 || sw == 0 || sh == 0) {
        return STATUS_BERHASIL;
    }
    if (sx >= src->lebar || sy >= src->tinggi ||
        dx >= dst->lebar || dy >= dst->tinggi) {
        return STATUS_PARAM_JARAK;
    }

    sw = MIN(sw, src->lebar - sx);
    sh = MIN(sh, src->tinggi - sy);
    dw = MIN(dw, dst->lebar - dx);
    dh = MIN(dh, dst->tinggi - dy);
    if (dw == 0 || dh == 0 || sw == 0 || sh == 0) {
        return STATUS_BERHASIL;
    }

    for (j = 0; j < dh; j++) {
        tak_bertanda32_t src_y = sy + (j * sh) / dh;
        for (i = 0; i < dw; i++) {
            tak_bertanda32_t src_x = sx + (i * sw) / dw;
            baca_piksel(src, src_x, src_y, &r, &g, &b, &a);
            tulis_piksel(dst, dx + i, dy + j, r, g, b, a);
        }
    }

    dst->flags |= BUFFER_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: DAMPAK_CERMIN_HORIZONTAL                                    */
/* ========================================================================== */

status_t dampak_cermin_horizontal(struct buffer_piksel *buf,
                                  tak_bertanda32_t x,
                                  tak_bertanda32_t y,
                                  tak_bertanda32_t w,
                                  tak_bertanda32_t h)
{
    tak_bertanda32_t baris, kiri, kanan;
    tak_bertanda32_t tengah;
    tak_bertanda8_t bpp;
    tak_bertanda8_t *baris_tmp;
    tak_bertanda32_t ukuran_baris;

    if (!buf_valid(buf)) {
        return STATUS_PARAM_NULL;
    }
    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }
    if (x >= buf->lebar || y >= buf->tinggi) {
        return STATUS_PARAM_JARAK;
    }

    w = MIN(w, buf->lebar - x);
    h = MIN(h, buf->tinggi - y);
    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }

    bpp = (tak_bertanda8_t)bpp_dari_format(buf->format);
    ukuran_baris = (tak_bertanda32_t)w * (tak_bertanda32_t)bpp;

    baris_tmp = (tak_bertanda8_t *)kmalloc((ukuran_t)ukuran_baris);
    if (baris_tmp == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    tengah = w / 2;

    for (baris = 0; baris < h; baris++) {
        tak_bertanda32_t ofs_baris = (y + baris) * buf->pitch +
            x * (tak_bertanda32_t)bpp;

        for (kiri = 0; kiri < tengah; kiri++) {
            kanan = w - 1 - kiri;
            {
                tak_bertanda32_t ofs_kiri = ofs_baris +
                    kiri * (tak_bertanda32_t)bpp;
                tak_bertanda32_t ofs_kanan = ofs_baris +
                    kanan * (tak_bertanda32_t)bpp;
                kernel_memcpy(baris_tmp, buf->data + ofs_kiri,
                              (ukuran_t)bpp);
                kernel_memcpy(buf->data + ofs_kiri,
                              buf->data + ofs_kanan,
                              (ukuran_t)bpp);
                kernel_memcpy(buf->data + ofs_kanan,
                              baris_tmp, (ukuran_t)bpp);
            }
        }
    }

    kernel_memset(baris_tmp, 0, (ukuran_t)ukuran_baris);
    kfree(baris_tmp);
    buf->flags |= BUFFER_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: DAMPAK_CERMIN_VERTIKAL                                      */
/* ========================================================================== */

status_t dampak_cermin_vertikal(struct buffer_piksel *buf,
                                tak_bertanda32_t x,
                                tak_bertanda32_t y,
                                tak_bertanda32_t w,
                                tak_bertanda32_t h)
{
    tak_bertanda32_t baris_atas, baris_bawah;
    tak_bertanda32_t tengah;
    tak_bertanda8_t bpp;
    tak_bertanda8_t *baris_tmp;
    tak_bertanda32_t ukuran_baris;

    if (!buf_valid(buf)) {
        return STATUS_PARAM_NULL;
    }
    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }
    if (x >= buf->lebar || y >= buf->tinggi) {
        return STATUS_PARAM_JARAK;
    }

    w = MIN(w, buf->lebar - x);
    h = MIN(h, buf->tinggi - y);
    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }

    bpp = (tak_bertanda8_t)bpp_dari_format(buf->format);
    ukuran_baris = (tak_bertanda32_t)w * (tak_bertanda32_t)bpp;

    baris_tmp = (tak_bertanda8_t *)kmalloc((ukuran_t)ukuran_baris);
    if (baris_tmp == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    tengah = h / 2;

    for (baris_atas = 0; baris_atas < tengah; baris_atas++) {
        baris_bawah = h - 1 - baris_atas;
        {
            tak_bertanda32_t ofs_atas =
                (y + baris_atas) * buf->pitch +
                x * (tak_bertanda32_t)bpp;
            tak_bertanda32_t ofs_bawah =
                (y + baris_bawah) * buf->pitch +
                x * (tak_bertanda32_t)bpp;
            kernel_memcpy(baris_tmp,
                          buf->data + ofs_atas,
                          (ukuran_t)ukuran_baris);
            kernel_memcpy(buf->data + ofs_atas,
                          buf->data + ofs_bawah,
                          (ukuran_t)ukuran_baris);
            kernel_memcpy(buf->data + ofs_bawah,
                          baris_tmp, (ukuran_t)ukuran_baris);
        }
    }

    kernel_memset(baris_tmp, 0, (ukuran_t)ukuran_baris);
    kfree(baris_tmp);
    buf->flags |= BUFFER_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: DAMPAK_ROTASI_90                                           */
/* ========================================================================== */

struct buffer_piksel *dampak_rotasi_90(
    const struct buffer_piksel *buf,
    tak_bertanda32_t x,
    tak_bertanda32_t y,
    tak_bertanda32_t w,
    tak_bertanda32_t h)
{
    struct buffer_piksel *hasil;
    tak_bertanda32_t j, i;
    tak_bertanda8_t r, g, b, a;

    if (!buf_valid(buf)) {
        return NULL;
    }
    if (w == 0 || h == 0) {
        return NULL;
    }
    if (x >= buf->lebar || y >= buf->tinggi) {
        return NULL;
    }

    w = MIN(w, buf->lebar - x);
    h = MIN(h, buf->tinggi - y);
    if (w == 0 || h == 0) {
        return NULL;
    }

    hasil = (struct buffer_piksel *)kmalloc(
        sizeof(struct buffer_piksel));
    if (hasil == NULL) {
        return NULL;
    }

    kernel_memset(hasil, 0, sizeof(struct buffer_piksel));
    hasil->magic = BUFFER_MAGIC;
    hasil->versi = BUFFER_VERSI;
    hasil->lebar = h;
    hasil->tinggi = w;
    hasil->format = buf->format;
    hasil->bit_per_piksel =
        (tak_bertanda8_t)(bpp_dari_format(buf->format) * 8);
    hasil->pitch = (tak_bertanda32_t)RATAKAN_ATAS(
        hasil->lebar *
        (tak_bertanda32_t)bpp_dari_format(buf->format), 4);
    hasil->ukuran = (ukuran_t)hasil->pitch *
        (ukuran_t)hasil->tinggi;

    hasil->data = (tak_bertanda8_t *)kmalloc(hasil->ukuran);
    if (hasil->data == NULL) {
        kernel_memset(hasil, 0, sizeof(struct buffer_piksel));
        kfree(hasil);
        return NULL;
    }

    kernel_memset(hasil->data, 0, hasil->ukuran);
    hasil->flags = BUFFER_FLAG_ALOKASI;
    hasil->referensi = 1;

    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            baca_piksel(buf, x + i, y + j, &r, &g, &b, &a);
            tulis_piksel(hasil, h - 1 - j, i, r, g, b, a);
        }
    }

    return hasil;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: DAMPAK_ROTASI_180                                          */
/* ========================================================================== */

status_t dampak_rotasi_180(struct buffer_piksel *buf,
                           tak_bertanda32_t x,
                           tak_bertanda32_t y,
                           tak_bertanda32_t w,
                           tak_bertanda32_t h)
{
    status_t st;

    st = dampak_cermin_horizontal(buf, x, y, w, h);
    if (st != STATUS_BERHASIL) {
        return st;
    }

    st = dampak_cermin_vertikal(buf, x, y, w, h);
    return st;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: DAMPAK_GABUNG_ALPHA                                         */
/* ========================================================================== */

status_t dampak_gabung_alpha(struct buffer_piksel *dst,
                             const struct buffer_piksel *src,
                             tak_bertanda32_t dx,
                             tak_bertanda32_t dy,
                             tak_bertanda32_t sx,
                             tak_bertanda32_t sy,
                             tak_bertanda32_t w,
                             tak_bertanda32_t h)
{
    tak_bertanda32_t j, i;
    tak_bertanda8_t bpp;

    if (!buf_valid(dst) || !buf_valid(src)) {
        return STATUS_PARAM_NULL;
    }
    if (dst->format != src->format) {
        return STATUS_PARAM_INVALID;
    }
    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }
    if (sx >= src->lebar || sy >= src->tinggi ||
        dx >= dst->lebar || dy >= dst->tinggi) {
        return STATUS_PARAM_JARAK;
    }

    w = MIN(w, MIN(src->lebar - sx, dst->lebar - dx));
    h = MIN(h, MIN(src->tinggi - sy, dst->tinggi - dy));
    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }

    bpp = (tak_bertanda8_t)bpp_dari_format(dst->format);

    if (bpp == 4 && (dst->format == BUFFER_FMT_ARGB8888 ||
        dst->format == BUFFER_FMT_XRGB8888 ||
        dst->format == BUFFER_FMT_BGRA8888)) {
        tak_bertanda32_t inv_a;
        for (j = 0; j < h; j++) {
            for (i = 0; i < w; i++) {
                tak_bertanda8_t sr, sg, sb, sa;
                tak_bertanda8_t dr, dg, db, da;
                tak_bertanda8_t rr, rg, rb, ra;
                tak_bertanda32_t ofs_s = (sy + j) * src->pitch +
                    (sx + i) * (tak_bertanda32_t)bpp;
                tak_bertanda32_t ofs_d = (dy + j) * dst->pitch +
                    (dx + i) * (tak_bertanda32_t)bpp;

                sb = src->data[ofs_s];
                sg = src->data[ofs_s + 1];
                sr = src->data[ofs_s + 2];
                sa = src->data[ofs_s + 3];

                db = dst->data[ofs_d];
                dg = dst->data[ofs_d + 1];
                dr = dst->data[ofs_d + 2];
                da = dst->data[ofs_d + 3];

                inv_a = 255 - (tak_bertanda32_t)sa;
                rr = (tak_bertanda8_t)(
                    ((tak_bertanda32_t)sr * sa +
                     (tak_bertanda32_t)dr * inv_a) / 255);
                rg = (tak_bertanda8_t)(
                    ((tak_bertanda32_t)sg * sa +
                     (tak_bertanda32_t)dg * inv_a) / 255);
                rb = (tak_bertanda8_t)(
                    ((tak_bertanda32_t)sb * sa +
                     (tak_bertanda32_t)db * inv_a) / 255);
                ra = (tak_bertanda8_t)(
                    ((tak_bertanda32_t)sa * sa +
                     (tak_bertanda32_t)da * inv_a) / 255);

                dst->data[ofs_d]     = rb;
                dst->data[ofs_d + 1] = rg;
                dst->data[ofs_d + 2] = rr;
                dst->data[ofs_d + 3] = ra;
            }
        }
    } else {
        for (j = 0; j < h; j++) {
            tak_bertanda8_t *sp = src->data +
                (sy + j) * src->pitch +
                sx * (tak_bertanda32_t)bpp;
            tak_bertanda8_t *dp = dst->data +
                (dy + j) * dst->pitch +
                dx * (tak_bertanda32_t)bpp;
            kernel_memcpy(dp, sp,
                          (ukuran_t)w * (ukuran_t)bpp);
        }
    }

    dst->flags |= BUFFER_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: DAMPAK_POTONG                                               */
/* ========================================================================== */

status_t dampak_potong(struct buffer_piksel *dst,
                       const struct buffer_piksel *src,
                       tak_bertanda32_t rect_x,
                       tak_bertanda32_t rect_y,
                       tak_bertanda32_t rect_w,
                       tak_bertanda32_t rect_h)
{
    return dampak_salin(dst, src, 0, 0,
                        rect_x, rect_y, rect_w, rect_h);
}

/* ========================================================================== */
/* FUNGSI PUBLIK: DAMPAK_ISI_POLOS                                            */
/* ========================================================================== */

status_t dampak_isi_polos(struct buffer_piksel *dst,
                          tak_bertanda32_t x,
                          tak_bertanda32_t y,
                          tak_bertanda32_t w,
                          tak_bertanda32_t h,
                          tak_bertanda8_t r,
                          tak_bertanda8_t g,
                          tak_bertanda8_t b,
                          tak_bertanda8_t a)
{
    tak_bertanda32_t px, py;
    tak_bertanda32_t x_akhir, y_akhir;
    tak_bertanda8_t bpp;

    if (!buf_valid(dst)) {
        return STATUS_PARAM_NULL;
    }
    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }
    if (x >= dst->lebar || y >= dst->tinggi) {
        return STATUS_PARAM_JARAK;
    }

    x_akhir = MIN(x + w, dst->lebar);
    y_akhir = MIN(y + h, dst->tinggi);
    if (x_akhir <= x || y_akhir <= y) {
        return STATUS_BERHASIL;
    }

    bpp = (tak_bertanda8_t)bpp_dari_format(dst->format);

    if (bpp == 4 && (x_akhir - x) == dst->lebar &&
        y == 0) {
        tak_bertanda8_t tmp[4];
        tak_bertanda32_t raw;
        tmp[0] = b;
        tmp[1] = g;
        tmp[2] = r;
        tmp[3] = a;
        raw = (tak_bertanda32_t)tmp[0]       |
              ((tak_bertanda32_t)tmp[1] << 8)  |
              ((tak_bertanda32_t)tmp[2] << 16) |
              ((tak_bertanda32_t)tmp[3] << 24);
        kernel_memset32(
            dst->data + y * dst->pitch + x * 4,
            raw,
            (ukuran_t)(x_akhir - x) *
            (ukuran_t)(y_akhir - y));
        dst->flags |= BUFFER_FLAG_KOTOR;
        return STATUS_BERHASIL;
    }

    for (py = y; py < y_akhir; py++) {
        for (px = x; px < x_akhir; px++) {
            tulis_piksel(dst, px, py, r, g, b, a);
        }
    }

    dst->flags |= BUFFER_FLAG_KOTOR;
    return STATUS_BERHASIL;
}
