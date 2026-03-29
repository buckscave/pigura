/*
 * PIGURA OS - KANVAS_BLIT.C
 * =========================
 * Operasi blit (block transfer) antar kanvas.
 *
 * Modul ini menyediakan fungsi-fungsi penyalinan blok piksel
 * antara dua kanvas: salin standar, skala, alpha blending,
 * tinting, dan operasi mentah ke/dari memori eksternal.
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
#define BUFFER_FLAG_KOTOR      0x08

#define bpp_dari_format(fmt) \
    (((fmt) == BUFFER_FMT_RGB332) ? 1 : \
     (((fmt) == BUFFER_FMT_RGB565) || \
      ((fmt) == BUFFER_FMT_BGR565)) ? 2 : \
     ((fmt) == BUFFER_FMT_RGB888) ? 3 : 4)

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

static bool_t kv_buf_valid(const struct buffer_piksel *b)
{
    return (b != NULL && b->magic == BUFFER_MAGIC &&
            b->data != NULL && b->format != BUFFER_FMT_INVALID &&
            b->format <= BUFFER_FMT_MAKS) ? BENAR : SALAH;
}

static bool_t kv_valid(const struct kanvas *k)
{
    return (k != NULL && k->magic == KANVAS_MAGIC &&
            kv_buf_valid(k->buffer)) ? BENAR : SALAH;
}

static tak_bertanda32_t kv_offset(const struct buffer_piksel *b,
                                   tak_bertanda32_t x,
                                   tak_bertanda32_t y)
{
    return y * b->pitch +
        x * (tak_bertanda32_t)bpp_dari_format(b->format);
}

/*
 * kv_potong_klip - Batasi area terhadap klip dan dimensi buffer.
 */
static void kv_potong_klip(const struct kanvas *kv,
                            tak_bertanda32_t *x,
                            tak_bertanda32_t *y,
                            tak_bertanda32_t *w,
                            tak_bertanda32_t *h)
{
    tak_bertanda32_t x2 = *x + *w;
    tak_bertanda32_t y2 = *y + *h;

    if (kv->klip.aktif) {
        tak_bertanda32_t kx2 = kv->klip.x + kv->klip.lebar;
        tak_bertanda32_t ky2 = kv->klip.y + kv->klip.tinggi;
        if (*x < kv->klip.x) { *x = kv->klip.x; }
        if (*y < kv->klip.y) { *y = kv->klip.y; }
        if (x2 > kx2) { x2 = kx2; }
        if (y2 > ky2) { y2 = ky2; }
    }

    if (x2 > kv->buffer->lebar) { x2 = kv->buffer->lebar; }
    if (y2 > kv->buffer->tinggi) { y2 = kv->buffer->tinggi; }

    if (*x >= x2 || *y >= y2) {
        *w = 0;
        *h = 0;
        return;
    }
    *w = x2 - *x;
    *h = y2 - *y;
}

/*
 * baca_piksel - Baca piksel dari buffer dalam format apapun.
 */
static void baca_piksel(const struct buffer_piksel *b,
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
        *r = *g = *bl = 0;
        *a = 0;
        break;
    }
}

/*
 * tulis_piksel - Tulis piksel ke buffer dalam format apapun.
 */
static void tulis_piksel(struct buffer_piksel *b,
                          tak_bertanda32_t ofs,
                          tak_bertanda8_t r,
                          tak_bertanda8_t g,
                          tak_bertanda8_t bl,
                          tak_bertanda8_t a)
{
    switch (b->format) {
    case BUFFER_FMT_RGB332:
        b->data[ofs] = (tak_bertanda8_t)(
            ((r >> 5) << 5) | ((g >> 5) << 2) | (bl >> 6));
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

/* Forward declaration (dipakai oleh kanvas_blit_skala) */
status_t kanvas_blit_tulis_piksel(struct kanvas *kv,
                                  tak_bertanda32_t x,
                                  tak_bertanda32_t y,
                                  tak_bertanda8_t r,
                                  tak_bertanda8_t g,
                                  tak_bertanda8_t b,
                                  tak_bertanda8_t a);

/* ========================================================================== */
/* FUNGSI PUBLIK: BLIT STANDAR                                                */
/* ========================================================================== */

status_t kanvas_blit(struct kanvas *dst,
                      const struct kanvas *src,
                      tak_bertanda32_t dx, tak_bertanda32_t dy,
                      tak_bertanda32_t sx, tak_bertanda32_t sy,
                      tak_bertanda32_t w, tak_bertanda32_t h)
{
    tak_bertanda32_t baris;
    tak_bertanda8_t bpp;

    if (dst == NULL || src == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (!kv_valid(dst) || !kv_valid(src)) {
        return STATUS_PARAM_INVALID;
    }
    if (dst->buffer->format != src->buffer->format) {
        return STATUS_PARAM_INVALID;
    }
    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }

    /* Potong area sumber terhadap batas src */
    if (sx >= src->buffer->lebar || sy >= src->buffer->tinggi) {
        return STATUS_PARAM_JARAK;
    }
    if (sx + w > src->buffer->lebar) {
        w = src->buffer->lebar - sx;
    }
    if (sy + h > src->buffer->tinggi) {
        h = src->buffer->tinggi - sy;
    }

    /* Potong area tujuan terhadap klip dan batas dst */
    kv_potong_klip(dst, &dx, &dy, &w, &h);
    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }

    bpp = (tak_bertanda8_t)bpp_dari_format(dst->buffer->format);

    for (baris = 0; baris < h; baris++) {
        tak_bertanda8_t *sp = src->buffer->data +
            (sy + baris) * src->buffer->pitch +
            sx * (tak_bertanda32_t)bpp;
        tak_bertanda8_t *dp = dst->buffer->data +
            (dy + baris) * dst->buffer->pitch +
            dx * (tak_bertanda32_t)bpp;
        kernel_memcpy(dp, sp, (ukuran_t)w * (ukuran_t)bpp);
    }

    dst->buffer->flags |= BUFFER_FLAG_KOTOR;
    dst->kotor = BENAR;
    dst->jumlah_gambar++;
    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: BLIT PENUH                                                  */
/* ========================================================================== */

status_t kanvas_blit_penuh(struct kanvas *dst,
                             const struct kanvas *src,
                             tak_bertanda32_t dx,
                             tak_bertanda32_t dy)
{
    if (dst == NULL || src == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (!kv_valid(dst) || !kv_valid(src)) {
        return STATUS_PARAM_INVALID;
    }
    if (dst->buffer->format != src->buffer->format) {
        return STATUS_PARAM_INVALID;
    }

    return kanvas_blit(dst, src, dx, dy, 0, 0,
                       src->buffer->lebar, src->buffer->tinggi);
}

/* ========================================================================== */
/* FUNGSI PUBLIK: BLIT SKALA (NEAREST NEIGHBOR)                               */
/* ========================================================================== */

status_t kanvas_blit_skala(struct kanvas *dst,
                            const struct kanvas *src,
                            tak_bertanda32_t dx,
                            tak_bertanda32_t dy,
                            tak_bertanda32_t dw,
                            tak_bertanda32_t dh,
                            tak_bertanda32_t sx,
                            tak_bertanda32_t sy,
                            tak_bertanda32_t sw,
                            tak_bertanda32_t sh)
{
    tak_bertanda32_t px, py;
    tak_bertanda8_t r, g, b, a;
    status_t st;

    if (dst == NULL || src == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (!kv_valid(dst) || !kv_valid(src)) {
        return STATUS_PARAM_INVALID;
    }
    if (dst->buffer->format != src->buffer->format) {
        return STATUS_PARAM_INVALID;
    }
    if (dw == 0 || dh == 0 || sw == 0 || sh == 0) {
        return STATUS_BERHASIL;
    }

    /* Potong area tujuan */
    if (dx >= dst->buffer->lebar ||
        dy >= dst->buffer->tinggi) {
        return STATUS_PARAM_JARAK;
    }
    if (dx + dw > dst->buffer->lebar) {
        dw = dst->buffer->lebar - dx;
    }
    if (dy + dh > dst->buffer->tinggi) {
        dh = dst->buffer->tinggi - dy;
    }

    /* Potong area sumber */
    if (sx >= src->buffer->lebar ||
        sy >= src->buffer->tinggi) {
        return STATUS_PARAM_JARAK;
    }
    if (sx + sw > src->buffer->lebar) {
        sw = src->buffer->lebar - sx;
    }
    if (sy + sh > src->buffer->tinggi) {
        sh = src->buffer->tinggi - sy;
    }

    if (dw == 0 || dh == 0 || sw == 0 || sh == 0) {
        return STATUS_BERHASIL;
    }

    /* Nearest-neighbor: untuk setiap piksel dst, cari piksel src */
    for (py = 0; py < dh; py++) {
        for (px = 0; px < dw; px++) {
            tak_bertanda32_t src_x = sx +
                (px * sw) / dw;
            tak_bertanda32_t src_y = sy +
                (py * sh) / dh;
            tak_bertanda32_t dst_x = dx + px;
            tak_bertanda32_t dst_y = dy + py;

            baca_piksel(src->buffer,
                        kv_offset(src->buffer, src_x, src_y),
                        &r, &g, &b, &a);
            st = kanvas_blit_tulis_piksel(dst, dst_x, dst_y,
                                           r, g, b, a);
            if (st != STATUS_BERHASIL) {
                return st;
            }
        }
    }

    dst->buffer->flags |= BUFFER_FLAG_KOTOR;
    dst->kotor = BENAR;
    dst->jumlah_gambar++;
    return STATUS_BERHASIL;
}

/*
 * kanvas_blit_tulis_piksel - Tulis satu piksel ke kanvas.
 * Dipakai oleh blit_skala; dideklarasikan di sini sebagai publik
 * karena juga berguna untuk modul lain.
 */
status_t kanvas_blit_tulis_piksel(struct kanvas *kv,
                                  tak_bertanda32_t x,
                                  tak_bertanda32_t y,
                                  tak_bertanda8_t r,
                                  tak_bertanda8_t g,
                                  tak_bertanda8_t b,
                                  tak_bertanda8_t a)
{
    tak_bertanda32_t ofs;

    if (kv == NULL || !kv_valid(kv)) {
        return STATUS_PARAM_NULL;
    }
    if (x >= kv->buffer->lebar ||
        y >= kv->buffer->tinggi) {
        return STATUS_PARAM_JARAK;
    }

    /* Periksa klip */
    if (kv->klip.aktif) {
        if (x < kv->klip.x ||
            y < kv->klip.y ||
            x >= kv->klip.x + kv->klip.lebar ||
            y >= kv->klip.y + kv->klip.tinggi) {
            return STATUS_BERHASIL;
        }
    }

    ofs = kv_offset(kv->buffer, x, y);
    tulis_piksel(kv->buffer, ofs, r, g, b, a);
    kv->buffer->flags |= BUFFER_FLAG_KOTOR;
    kv->kotor = BENAR;
    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: BLIT ALPHA BLENDING                                        */
/* ========================================================================== */

status_t kanvas_blit_alpha(struct kanvas *dst,
                            const struct kanvas *src,
                            tak_bertanda32_t dx,
                            tak_bertanda32_t dy,
                            tak_bertanda32_t sx,
                            tak_bertanda32_t sy,
                            tak_bertanda32_t w,
                            tak_bertanda32_t h)
{
    tak_bertanda32_t px, py;
    tak_bertanda8_t bpp;

    if (dst == NULL || src == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (!kv_valid(dst) || !kv_valid(src)) {
        return STATUS_PARAM_INVALID;
    }

    bpp = (tak_bertanda8_t)bpp_dari_format(
        dst->buffer->format);

    /* Alpha blending hanya untuk format 32-bit */
    if (bpp != 4) {
        /* Fallback: salin biasa */
        return kanvas_blit(dst, src, dx, dy, sx, sy, w, h);
    }

    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }

    /* Potong area */
    if (sx >= src->buffer->lebar ||
        sy >= src->buffer->tinggi) {
        return STATUS_PARAM_JARAK;
    }
    if (sx + w > src->buffer->lebar) {
        w = src->buffer->lebar - sx;
    }
    if (sy + h > src->buffer->tinggi) {
        h = src->buffer->tinggi - sy;
    }

    kv_potong_klip(dst, &dx, &dy, &w, &h);
    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }

    for (py = 0; py < h; py++) {
        for (px = 0; px < w; px++) {
            tak_bertanda32_t s_ofs = kv_offset(
                src->buffer, sx + px, sy + py);
            tak_bertanda32_t d_ofs = kv_offset(
                dst->buffer, dx + px, dy + py);
            tak_bertanda8_t sr, sg, sb, sa;
            tak_bertanda8_t dr, dg, db, da;
            tak_bertanda32_t inv_a;

            baca_piksel(src->buffer, s_ofs,
                        &sr, &sg, &sb, &sa);
            baca_piksel(dst->buffer, d_ofs,
                        &dr, &dg, &db, &da);

            inv_a = 255 - (tak_bertanda32_t)sa;
            dr = (tak_bertanda8_t)(
                ((tak_bertanda32_t)sa * sr + inv_a * dr) / 255);
            dg = (tak_bertanda8_t)(
                ((tak_bertanda32_t)sa * sg + inv_a * dg) / 255);
            db = (tak_bertanda8_t)(
                ((tak_bertanda32_t)sa * sb + inv_a * db) / 255);
            da = (tak_bertanda8_t)(
                ((tak_bertanda32_t)sa + inv_a * da) / 255);

            tulis_piksel(dst->buffer, d_ofs,
                          dr, dg, db, da);
        }
    }

    dst->buffer->flags |= BUFFER_FLAG_KOTOR;
    dst->kotor = BENAR;
    dst->jumlah_gambar++;
    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: BLIT TINTED                                                 */
/* ========================================================================== */

status_t kanvas_blit_tinted(struct kanvas *dst,
                              const struct kanvas *src,
                              tak_bertanda32_t dx,
                              tak_bertanda32_t dy,
                              tak_bertanda32_t sx,
                              tak_bertanda32_t sy,
                              tak_bertanda32_t w,
                              tak_bertanda32_t h,
                              tak_bertanda8_t tr,
                              tak_bertanda8_t tg,
                              tak_bertanda8_t tb,
                              tak_bertanda8_t ta)
{
    tak_bertanda32_t px, py;

    if (dst == NULL || src == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (!kv_valid(dst) || !kv_valid(src)) {
        return STATUS_PARAM_INVALID;
    }
    if (dst->buffer->format != src->buffer->format) {
        return STATUS_PARAM_INVALID;
    }
    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }

    if (sx >= src->buffer->lebar ||
        sy >= src->buffer->tinggi) {
        return STATUS_PARAM_JARAK;
    }
    if (sx + w > src->buffer->lebar) {
        w = src->buffer->lebar - sx;
    }
    if (sy + h > src->buffer->tinggi) {
        h = src->buffer->tinggi - sy;
    }

    kv_potong_klip(dst, &dx, &dy, &w, &h);
    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }

    for (py = 0; py < h; py++) {
        for (px = 0; px < w; px++) {
            tak_bertanda32_t s_ofs = kv_offset(
                src->buffer, sx + px, sy + py);
            tak_bertanda32_t d_ofs = kv_offset(
                dst->buffer, dx + px, dy + py);
            tak_bertanda8_t sr, sg, sb, sa;
            tak_bertanda8_t rr, rg, rb, ra;

            baca_piksel(src->buffer, s_ofs,
                        &sr, &sg, &sb, &sa);

            rr = (tak_bertanda8_t)(
                ((tak_bertanda32_t)sr * tr) / 255);
            rg = (tak_bertanda8_t)(
                ((tak_bertanda32_t)sg * tg) / 255);
            rb = (tak_bertanda8_t)(
                ((tak_bertanda32_t)sb * tb) / 255);
            ra = (tak_bertanda8_t)(
                ((tak_bertanda32_t)sa * ta) / 255);

            tulis_piksel(dst->buffer, d_ofs,
                          rr, rg, rb, ra);
        }
    }

    dst->buffer->flags |= BUFFER_FLAG_KOTOR;
    dst->kotor = BENAR;
    dst->jumlah_gambar++;
    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: BLIT DARI/KE MEMORI                                        */
/* ========================================================================== */

status_t kanvas_salin_dari_memori(struct kanvas *dst,
                                   const tak_bertanda8_t *data,
                                   tak_bertanda32_t x,
                                   tak_bertanda32_t y,
                                   tak_bertanda32_t w,
                                   tak_bertanda32_t h,
                                   tak_bertanda32_t pitch,
                                   tak_bertanda8_t format)
{
    tak_bertanda32_t baris;
    tak_bertanda8_t bpp;

    if (dst == NULL || data == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (!kv_valid(dst)) {
        return STATUS_PARAM_INVALID;
    }
    if (dst->buffer->format != format) {
        return STATUS_PARAM_INVALID;
    }
    if (w == 0 || h == 0 || pitch == 0) {
        return STATUS_BERHASIL;
    }

    kv_potong_klip(dst, &x, &y, &w, &h);
    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }

    bpp = (tak_bertanda8_t)bpp_dari_format(format);

    for (baris = 0; baris < h; baris++) {
        const tak_bertanda8_t *sp =
            data + (tak_bertanda32_t)baris * pitch;
        tak_bertanda8_t *dp = dst->buffer->data +
            (y + baris) * dst->buffer->pitch +
            x * (tak_bertanda32_t)bpp;
        kernel_memcpy(dp, sp, (ukuran_t)w * (ukuran_t)bpp);
    }

    dst->buffer->flags |= BUFFER_FLAG_KOTOR;
    dst->kotor = BENAR;
    dst->jumlah_gambar++;
    return STATUS_BERHASIL;
}

status_t kanvas_salin_ke_memori(const struct kanvas *src,
                                 tak_bertanda8_t *data,
                                 tak_bertanda32_t x,
                                 tak_bertanda32_t y,
                                 tak_bertanda32_t w,
                                 tak_bertanda32_t h,
                                 tak_bertanda32_t pitch,
                                 tak_bertanda8_t format)
{
    tak_bertanda32_t baris;
    tak_bertanda8_t bpp;

    if (src == NULL || data == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (!kv_valid(src)) {
        return STATUS_PARAM_INVALID;
    }
    if (src->buffer->format != format) {
        return STATUS_PARAM_INVALID;
    }
    if (w == 0 || h == 0 || pitch == 0) {
        return STATUS_BERHASIL;
    }

    if (x >= src->buffer->lebar ||
        y >= src->buffer->tinggi) {
        return STATUS_PARAM_JARAK;
    }
    if (x + w > src->buffer->lebar) {
        w = src->buffer->lebar - x;
    }
    if (y + h > src->buffer->tinggi) {
        h = src->buffer->tinggi - y;
    }

    bpp = (tak_bertanda8_t)bpp_dari_format(format);

    for (baris = 0; baris < h; baris++) {
        const tak_bertanda8_t *sp = src->buffer->data +
            (y + baris) * src->buffer->pitch +
            x * (tak_bertanda32_t)bpp;
        tak_bertanda8_t *dp =
            data + (tak_bertanda32_t)baris * pitch;
        kernel_memcpy(dp, sp, (ukuran_t)w * (ukuran_t)bpp);
    }

    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: BLIT TERKLIP                                                */
/* ========================================================================== */

status_t kanvas_blit_kliped(struct kanvas *dst,
                              const struct kanvas *src,
                              tak_bertanda32_t klip_x,
                              tak_bertanda32_t klip_y,
                              tak_bertanda32_t klip_w,
                              tak_bertanda32_t klip_h)
{
    return kanvas_blit(dst, src, 0, 0,
                       klip_x, klip_y, klip_w, klip_h);
}

/* ========================================================================== */
/* FUNGSI PUBLIK: ISI KOTAK                                                   */
/* ========================================================================== */

status_t kanvas_isi_kotak(struct kanvas *kv,
                           tak_bertanda32_t x,
                           tak_bertanda32_t y,
                           tak_bertanda32_t w,
                           tak_bertanda32_t h)
{
    tak_bertanda32_t px, py;
    tak_bertanda8_t r, g, b, a;

    if (kv == NULL || !kv_valid(kv)) {
        return STATUS_PARAM_NULL;
    }
    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }

    r = kv->warna_r;
    g = kv->warna_g;
    b = kv->warna_b;
    a = kv->warna_a;

    kv_potong_klip(kv, &x, &y, &w, &h);
    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }

    /* Optimasi untuk format 32-bit */
    if (bpp_dari_format(kv->buffer->format) == 4) {
        tak_bertanda8_t tmp[4];
        tak_bertanda32_t raw;
        tmp[0] = b; tmp[1] = g; tmp[2] = r; tmp[3] = a;
        raw = (tak_bertanda32_t)tmp[0]       |
              ((tak_bertanda32_t)tmp[1] << 8)  |
              ((tak_bertanda32_t)tmp[2] << 16) |
              ((tak_bertanda32_t)tmp[3] << 24);

        for (py = 0; py < h; py++) {
            tak_bertanda32_t ofs = (y + py) *
                kv->buffer->pitch + x * 4;
            kernel_memset32(kv->buffer->data + ofs,
                            raw, (ukuran_t)w);
        }
        kv->buffer->flags |= BUFFER_FLAG_KOTOR;
        kv->kotor = BENAR;
        kv->jumlah_gambar++;
        return STATUS_BERHASIL;
    }

    /* Format lain: piksel per piksel */
    for (py = 0; py < h; py++) {
        for (px = 0; px < w; px++) {
            tak_bertanda32_t ofs = kv_offset(
                kv->buffer, x + px, y + py);
            tulis_piksel(kv->buffer, ofs, r, g, b, a);
        }
    }

    kv->buffer->flags |= BUFFER_FLAG_KOTOR;
    kv->kotor = BENAR;
    kv->jumlah_gambar++;
    return STATUS_BERHASIL;
}
