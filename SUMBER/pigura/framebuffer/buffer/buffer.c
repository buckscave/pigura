/*
 * PIGURA OS - BUFFER.C
 * ====================
 * Inti manajemen buffer pixel (pixel buffer).
 *
 * Modul ini menyediakan abstraksi buffer pixel tingkat rendah
 * yang menjadi fondasi seluruh subsistem grafis LibPigura.
 * Setiap buffer menyimpan data piksel mentah dengan informasi
 * format, dimensi, dan metadata terkait.
 *
 * BERKAS INI BUKAN driver hardware. Ini murni abstraksi software
 * untuk manajemen memori piksel.
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

/* ========================================================================== */
/* MAKRO HELPER                                                               */
/* ========================================================================== */

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
/* STRUKTUR DATA BUFFER                                                      */
/* ========================================================================== */

/*
 * struct buffer_piksel
 * --------------------
 * Struktur utama buffer pixel.
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

/* ========================================================================== */
/* VARIABEL GLOBAL                                                           */
/* ========================================================================== */

/* Jumlah buffer yang sedang aktif */
static tak_bertanda32_t g_buffer_aktif = 0;

/* ========================================================================== */
/* FUNGSI HELPER INTERNAL                                                    */
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
 * format_dari_bpp
 * ---------------
 * Menentukan format pixel dari bits per pixel.
 */
static tak_bertanda8_t format_dari_bpp(tak_bertanda32_t bpp)
{
    switch (bpp) {
    case 8:  return BUFFER_FMT_RGB332;
    case 16: return BUFFER_FMT_RGB565;
    case 24: return BUFFER_FMT_RGB888;
    case 32: return BUFFER_FMT_XRGB8888;
    default: return BUFFER_FMT_INVALID;
    }
}

/*
 * cek_param_buffer
 * ---------------
 * Memvalidasi parameter pembuatan buffer.
 */
static status_t cek_param_buffer(tak_bertanda32_t lebar,
                                 tak_bertanda32_t tinggi,
                                 tak_bertanda8_t format)
{
    if (format == BUFFER_FMT_INVALID || format > BUFFER_FMT_MAKS) {
        return STATUS_PARAM_INVALID;
    }
    if (lebar == 0 || tinggi == 0) {
        return STATUS_PARAM_JARAK;
    }
    if (lebar > BUFFER_DIMENSI_MAKS ||
        tinggi > BUFFER_DIMENSI_MAKS) {
        return STATUS_PARAM_JARAK;
    }
    return STATUS_BERHASIL;
}

/*
 * cek_overflow_dimensi
 * -------------------
 * Memeriksa potensi overflow pada perhitungan ukuran buffer.
 */
static bool_t cek_overflow_dimensi(tak_bertanda32_t lebar,
                                   tak_bertanda32_t tinggi,
                                   tak_bertanda8_t format)
{
    tak_bertanda64_t uk64 = (tak_bertanda64_t)hitung_pitch(lebar,
                              format) * (tak_bertanda64_t)tinggi;
#if defined(PIGURA_ARSITEKTUR_64BIT)
    return (uk64 <= (tak_bertanda64_t)(-1)) ? BENAR : SALAH;
#else
    return (uk64 <= (tak_bertanda64_t)TAK_BERTANDA32_MAX)
           ? BENAR : SALAH;
#endif
}

/* ========================================================================== */
/* FUNGSI TULIS/BAKA PIKSEL PER FORMAT                                      */
/* ========================================================================== */

static void tulis_piksel_8(tak_bertanda8_t *buf,
                            tak_bertanda32_t ofs,
                            tak_bertanda8_t r,
                            tak_bertanda8_t g,
                            tak_bertanda8_t b)
{
    buf[ofs] = (tak_bertanda8_t)(
        ((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6));
}

static void tulis_piksel_16(tak_bertanda8_t *buf,
                             tak_bertanda32_t ofs,
                             tak_bertanda8_t r,
                             tak_bertanda8_t g,
                             tak_bertanda8_t b)
{
    tak_bertanda16_t pk = (tak_bertanda16_t)(
        (((tak_bertanda16_t)r >> 3) << 11) |
        (((tak_bertanda16_t)g >> 2) << 5)  |
        ((tak_bertanda16_t)b >> 3));
    buf[ofs]     = (tak_bertanda8_t)(pk & 0xFF);
    buf[ofs + 1] = (tak_bertanda8_t)(pk >> 8);
}

static void tulis_piksel_24(tak_bertanda8_t *buf,
                             tak_bertanda32_t ofs,
                             tak_bertanda8_t r,
                             tak_bertanda8_t g,
                             tak_bertanda8_t b)
{
    buf[ofs]     = r;
    buf[ofs + 1] = g;
    buf[ofs + 2] = b;
}

static void tulis_piksel_32(tak_bertanda8_t *buf,
                             tak_bertanda32_t ofs,
                             tak_bertanda8_t r,
                             tak_bertanda8_t g,
                             tak_bertanda8_t b,
                             tak_bertanda8_t a)
{
    buf[ofs]     = b;
    buf[ofs + 1] = g;
    buf[ofs + 2] = r;
    buf[ofs + 3] = a;
}

static void baca_piksel_8(const tak_bertanda8_t *buf,
                           tak_bertanda32_t ofs,
                           tak_bertanda8_t *r,
                           tak_bertanda8_t *g,
                           tak_bertanda8_t *b)
{
    tak_bertanda8_t pk = buf[ofs];
    *r = (tak_bertanda8_t)((pk >> 5) & 0x07) * 255 / 7;
    *g = (tak_bertanda8_t)((pk >> 2) & 0x07) * 255 / 7;
    *b = (tak_bertanda8_t)(pk & 0x03) * 255 / 3;
}

static void baca_piksel_16(const tak_bertanda8_t *buf,
                            tak_bertanda32_t ofs,
                            tak_bertanda8_t *r,
                            tak_bertanda8_t *g,
                            tak_bertanda8_t *b)
{
    tak_bertanda16_t pk = (tak_bertanda16_t)buf[ofs];
    pk |= (tak_bertanda16_t)((tak_bertanda16_t)buf[ofs + 1] << 8);
    *r = (tak_bertanda8_t)((pk >> 11) & 0x1F) * 255 / 31;
    *g = (tak_bertanda8_t)((pk >> 5) & 0x3F)  * 255 / 63;
    *b = (tak_bertanda8_t)(pk & 0x1F) * 255 / 31;
}

static void baca_piksel_24(const tak_bertanda8_t *buf,
                            tak_bertanda32_t ofs,
                            tak_bertanda8_t *r,
                            tak_bertanda8_t *g,
                            tak_bertanda8_t *b)
{
    *r = buf[ofs];
    *g = buf[ofs + 1];
    *b = buf[ofs + 2];
}

static void baca_piksel_32(const tak_bertanda8_t *buf,
                            tak_bertanda32_t ofs,
                            tak_bertanda8_t *r,
                            tak_bertanda8_t *g,
                            tak_bertanda8_t *b,
                            tak_bertanda8_t *a)
{
    *b = buf[ofs];
    *g = buf[ofs + 1];
    *r = buf[ofs + 2];
    *a = buf[ofs + 3];
}

/*
 * dispatch_tulis - Menulis piksel sesuai format buffer.
 */
static void dispatch_tulis(struct buffer_piksel *buf,
                           tak_bertanda32_t ofs,
                           tak_bertanda8_t r,
                           tak_bertanda8_t g,
                           tak_bertanda8_t b,
                           tak_bertanda8_t a)
{
    switch (buf->format) {
    case BUFFER_FMT_RGB332:
        tulis_piksel_8(buf->data, ofs, r, g, b);
        break;
    case BUFFER_FMT_RGB565:
    case BUFFER_FMT_BGR565:
        tulis_piksel_16(buf->data, ofs, r, g, b);
        break;
    case BUFFER_FMT_RGB888:
        tulis_piksel_24(buf->data, ofs, r, g, b);
        break;
    case BUFFER_FMT_ARGB8888:
    case BUFFER_FMT_XRGB8888:
    case BUFFER_FMT_BGRA8888:
        tulis_piksel_32(buf->data, ofs, r, g, b, a);
        break;
    default:
        break;
    }
}

/*
 * dispatch_baca - Membaca piksel sesuai format buffer.
 */
static void dispatch_baca(const struct buffer_piksel *buf,
                          tak_bertanda32_t ofs,
                          tak_bertanda8_t *r,
                          tak_bertanda8_t *g,
                          tak_bertanda8_t *b,
                          tak_bertanda8_t *a)
{
    switch (buf->format) {
    case BUFFER_FMT_RGB332:
        baca_piksel_8(buf->data, ofs, r, g, b);
        if (a != NULL) { *a = 0xFF; }
        break;
    case BUFFER_FMT_RGB565:
    case BUFFER_FMT_BGR565:
        baca_piksel_16(buf->data, ofs, r, g, b);
        if (a != NULL) { *a = 0xFF; }
        break;
    case BUFFER_FMT_RGB888:
        baca_piksel_24(buf->data, ofs, r, g, b);
        if (a != NULL) { *a = 0xFF; }
        break;
    case BUFFER_FMT_ARGB8888:
    case BUFFER_FMT_XRGB8888:
    case BUFFER_FMT_BGRA8888:
        baca_piksel_32(buf->data, ofs, r, g, b, a);
        break;
    default:
        *r = *g = *b = 0;
        if (a != NULL) { *a = 0; }
        break;
    }
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

/* ========================================================================== */
/* FUNGSI PUBLIK: PEMBUATAN DAN PENGHAPUSAN BUFFER                        */
/* ========================================================================== */

struct buffer_piksel *buffer_piksel_buat(tak_bertanda32_t lebar,
                                         tak_bertanda32_t tinggi,
                                         tak_bertanda8_t format)
{
    struct buffer_piksel *buf;
    status_t st;

    st = cek_param_buffer(lebar, tinggi, format);
    if (st != STATUS_BERHASIL) {
        return NULL;
    }
    if (!cek_overflow_dimensi(lebar, tinggi, format)) {
        return NULL;
    }

    buf = (struct buffer_piksel *)kmalloc(
        sizeof(struct buffer_piksel));
    if (buf == NULL) {
        return NULL;
    }

    kernel_memset(buf, 0, sizeof(struct buffer_piksel));

    buf->magic         = BUFFER_MAGIC;
    buf->versi         = BUFFER_VERSI;
    buf->lebar         = lebar;
    buf->tinggi        = tinggi;
    buf->format        = format;
    buf->bit_per_piksel = bit_dari_format(format);
    buf->pitch         = hitung_pitch(lebar, format);
    buf->ukuran        = hitung_ukuran_buffer(lebar, tinggi,
                                               format);

    buf->data = (tak_bertanda8_t *)kmalloc(buf->ukuran);
    if (buf->data == NULL) {
        kernel_memset(buf, 0, sizeof(struct buffer_piksel));
        kfree(buf);
        return NULL;
    }

    kernel_memset(buf->data, 0, buf->ukuran);
    buf->flags     = BUFFER_FLAG_ALOKASI;
    buf->referensi = 1;
    g_buffer_aktif++;

    return buf;
}

struct buffer_piksel *buffer_piksel_buat_dari_memori(
    tak_bertanda8_t *data,
    tak_bertanda32_t lebar,
    tak_bertanda32_t tinggi,
    tak_bertanda8_t format,
    tak_bertanda32_t pitch)
{
    struct buffer_piksel *buf;
    status_t st;

    if (data == NULL) {
        return NULL;
    }

    st = cek_param_buffer(lebar, tinggi, format);
    if (st != STATUS_BERHASIL) {
        return NULL;
    }

    if (pitch == 0) {
        pitch = hitung_pitch(lebar, format);
    }

    buf = (struct buffer_piksel *)kmalloc(
        sizeof(struct buffer_piksel));
    if (buf == NULL) {
        return NULL;
    }

    kernel_memset(buf, 0, sizeof(struct buffer_piksel));

    buf->magic         = BUFFER_MAGIC;
    buf->versi         = BUFFER_VERSI;
    buf->lebar         = lebar;
    buf->tinggi        = tinggi;
    buf->format        = format;
    buf->bit_per_piksel = bit_dari_format(format);
    buf->pitch         = pitch;
    buf->ukuran        = (ukuran_t)pitch * (ukuran_t)tinggi;
    buf->data          = data;
    buf->flags         = BUFFER_FLAG_EKSTERNAL;
    buf->referensi     = 1;
    g_buffer_aktif++;

    return buf;
}

struct buffer_piksel *buffer_piksel_buat_dari_bpp(
    tak_bertanda32_t lebar,
    tak_bertanda32_t tinggi,
    tak_bertanda32_t bpp)
{
    tak_bertanda8_t fmt = format_dari_bpp(bpp);

    if (fmt == BUFFER_FMT_INVALID) {
        return NULL;
    }
    return buffer_piksel_buat(lebar, tinggi, fmt);
}

status_t buffer_piksel_tambah_referensi(
    struct buffer_piksel *buf)
{
    if (buf == NULL || buf->magic != BUFFER_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    buf->referensi++;
    return STATUS_BERHASIL;
}

status_t buffer_piksel_hapus(struct buffer_piksel *buf)
{
    if (buf == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (buf->magic != BUFFER_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (buf->flags & BUFFER_FLAG_KUNCI) {
        return STATUS_BUSY;
    }

    if (buf->referensi > 0) {
        buf->referensi--;
    }
    if (buf->referensi > 0) {
        return STATUS_BERHASIL;
    }

    if ((buf->flags & BUFFER_FLAG_ALOKASI) &&
        (buf->data != NULL)) {
        kernel_memset(buf->data, 0, buf->ukuran);
        kfree(buf->data);
        buf->data = NULL;
    }

    buf->magic  = 0;
    buf->flags  = 0;
    buf->ukuran = 0;
    g_buffer_aktif--;
    kfree(buf);
    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: OPERASI PIKSEL                                              */
/* ========================================================================== */

status_t buffer_piksel_tulis(struct buffer_piksel *buf,
                              tak_bertanda32_t x,
                              tak_bertanda32_t y,
                              tak_bertanda8_t r,
                              tak_bertanda8_t g,
                              tak_bertanda8_t b,
                              tak_bertanda8_t a)
{
    tak_bertanda32_t ofs;

    if (buf == NULL || buf->magic != BUFFER_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    if (buf->data == NULL) {
        return STATUS_GAGAL;
    }
    if (x >= buf->lebar || y >= buf->tinggi) {
        return STATUS_PARAM_JARAK;
    }

    ofs = hitung_offset_piksel(buf, x, y);
    dispatch_tulis(buf, ofs, r, g, b, a);
    buf->flags |= BUFFER_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

status_t buffer_piksel_baca(const struct buffer_piksel *buf,
                             tak_bertanda32_t x,
                             tak_bertanda32_t y,
                             tak_bertanda8_t *r,
                             tak_bertanda8_t *g,
                             tak_bertanda8_t *b,
                             tak_bertanda8_t *a)
{
    tak_bertanda32_t ofs;

    if (buf == NULL || buf->magic != BUFFER_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    if (buf->data == NULL) {
        return STATUS_GAGAL;
    }
    if (x >= buf->lebar || y >= buf->tinggi) {
        return STATUS_PARAM_JARAK;
    }

    ofs = hitung_offset_piksel(buf, x, y);
    dispatch_baca(buf, ofs, r, g, b, a);
    return STATUS_BERHASIL;
}

status_t buffer_piksel_tulis_mentaw(struct buffer_piksel *buf,
                                     tak_bertanda32_t x,
                                     tak_bertanda32_t y,
                                     tak_bertanda32_t nilai)
{
    tak_bertanda32_t ofs;
    tak_bertanda8_t bpp;

    if (buf == NULL || buf->magic != BUFFER_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    if (buf->data == NULL) {
        return STATUS_GAGAL;
    }
    if (x >= buf->lebar || y >= buf->tinggi) {
        return STATUS_PARAM_JARAK;
    }

    bpp = (tak_bertanda8_t)bpp_dari_format(buf->format);
    ofs = hitung_offset_piksel(buf, x, y);

    switch (bpp) {
    case 1:
        buf->data[ofs] = (tak_bertanda8_t)nilai;
        break;
    case 2:
        buf->data[ofs]     = (tak_bertanda8_t)(nilai & 0xFF);
        buf->data[ofs + 1] = (tak_bertanda8_t)(nilai >> 8);
        break;
    case 3:
        buf->data[ofs]     = (tak_bertanda8_t)(nilai & 0xFF);
        buf->data[ofs + 1] = (tak_bertanda8_t)((nilai >> 8)
                                                  & 0xFF);
        buf->data[ofs + 2] = (tak_bertanda8_t)((nilai >> 16)
                                                  & 0xFF);
        break;
    case 4:
        buf->data[ofs]     = (tak_bertanda8_t)(nilai & 0xFF);
        buf->data[ofs + 1] = (tak_bertanda8_t)((nilai >> 8)
                                                  & 0xFF);
        buf->data[ofs + 2] = (tak_bertanda8_t)((nilai >> 16)
                                                  & 0xFF);
        buf->data[ofs + 3] = (tak_bertanda8_t)((nilai >> 24)
                                                  & 0xFF);
        break;
    default:
        return STATUS_GAGAL;
    }

    buf->flags |= BUFFER_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

status_t buffer_piksel_baca_mentaw(
    const struct buffer_piksel *buf,
    tak_bertanda32_t x,
    tak_bertanda32_t y,
    tak_bertanda32_t *nilai)
{
    tak_bertanda32_t ofs;
    tak_bertanda8_t bpp;

    if (buf == NULL || buf->magic != BUFFER_MAGIC ||
        nilai == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (buf->data == NULL) {
        return STATUS_GAGAL;
    }
    if (x >= buf->lebar || y >= buf->tinggi) {
        return STATUS_PARAM_JARAK;
    }

    bpp = (tak_bertanda8_t)bpp_dari_format(buf->format);
    ofs = hitung_offset_piksel(buf, x, y);
    *nilai = 0;

    switch (bpp) {
    case 1:
        *nilai = (tak_bertanda32_t)buf->data[ofs];
        break;
    case 2:
        *nilai  = (tak_bertanda32_t)buf->data[ofs];
        *nilai |= (tak_bertanda32_t)buf->data[ofs + 1] << 8;
        break;
    case 3:
        *nilai  = (tak_bertanda32_t)buf->data[ofs];
        *nilai |= (tak_bertanda32_t)buf->data[ofs + 1] << 8;
        *nilai |= (tak_bertanda32_t)buf->data[ofs + 2] << 16;
        break;
    case 4:
        *nilai  = (tak_bertanda32_t)buf->data[ofs];
        *nilai |= (tak_bertanda32_t)buf->data[ofs + 1] << 8;
        *nilai |= (tak_bertanda32_t)buf->data[ofs + 2] << 16;
        *nilai |= (tak_bertanda32_t)buf->data[ofs + 3] << 24;
        break;
    default:
        return STATUS_GAGAL;
    }

    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: OPERASI AREA                                                */
/* ========================================================================== */

status_t buffer_piksel_isi(struct buffer_piksel *buf,
                            tak_bertanda8_t r,
                            tak_bertanda8_t g,
                            tak_bertanda8_t b,
                            tak_bertanda8_t a)
{
    tak_bertanda32_t x, y;

    if (buf == NULL || buf->magic != BUFFER_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    if (buf->data == NULL) {
        return STATUS_GAGAL;
    }

    /* Optimasi: format 32-bit, gunakan memset32 */
    if (bpp_dari_format(buf->format) == 4) {
        tak_bertanda8_t tmp[4];
        tak_bertanda32_t raw;
        tulis_piksel_32(tmp, 0, r, g, b, a);
        raw = (tak_bertanda32_t)tmp[0]       |
              ((tak_bertanda32_t)tmp[1] << 8)  |
              ((tak_bertanda32_t)tmp[2] << 16) |
              ((tak_bertanda32_t)tmp[3] << 24);
        kernel_memset32(buf->data, raw,
                        buf->lebar * buf->tinggi);
        buf->flags |= BUFFER_FLAG_KOTOR;
        return STATUS_BERHASIL;
    }

    for (y = 0; y < buf->tinggi; y++) {
        for (x = 0; x < buf->lebar; x++) {
            tak_bertanda32_t ofs = hitung_offset_piksel(
                buf, x, y);
            dispatch_tulis(buf, ofs, r, g, b, a);
        }
    }

    buf->flags |= BUFFER_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

status_t buffer_piksel_isi_area(struct buffer_piksel *buf,
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

    if (buf == NULL || buf->magic != BUFFER_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    if (buf->data == NULL) {
        return STATUS_GAGAL;
    }
    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }
    if (x >= buf->lebar || y >= buf->tinggi) {
        return STATUS_PARAM_JARAK;
    }

    x_akhir = x + w;
    y_akhir = y + h;
    if (x_akhir > buf->lebar)  { x_akhir = buf->lebar; }
    if (y_akhir > buf->tinggi) { y_akhir = buf->tinggi; }

    /* Optimasi 32-bit lebar penuh per baris */
    if (bpp_dari_format(buf->format) == 4 &&
        (x_akhir - x) == buf->lebar) {
        tak_bertanda8_t tmp[4];
        tak_bertanda32_t raw;
        tulis_piksel_32(tmp, 0, r, g, b, a);
        raw = (tak_bertanda32_t)tmp[0]       |
              ((tak_bertanda32_t)tmp[1] << 8)  |
              ((tak_bertanda32_t)tmp[2] << 16) |
              ((tak_bertanda32_t)tmp[3] << 24);
        kernel_memset32(
            buf->data + y * buf->pitch + x * 4,
            raw,
            (ukuran_t)(x_akhir - x) *
            (ukuran_t)(y_akhir - y));
        buf->flags |= BUFFER_FLAG_KOTOR;
        return STATUS_BERHASIL;
    }

    for (py = y; py < y_akhir; py++) {
        for (px = x; px < x_akhir; px++) {
            tak_bertanda32_t ofs = hitung_offset_piksel(
                buf, px, py);
            dispatch_tulis(buf, ofs, r, g, b, a);
        }
    }

    buf->flags |= BUFFER_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

status_t buffer_piksel_salin_area(
    struct buffer_piksel *dst,
    const struct buffer_piksel *src,
    tak_bertanda32_t dx, tak_bertanda32_t dy,
    tak_bertanda32_t sx, tak_bertanda32_t sy,
    tak_bertanda32_t w, tak_bertanda32_t h)
{
    tak_bertanda32_t baris;
    tak_bertanda8_t bpp;

    if (dst == NULL || src == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (dst->magic != BUFFER_MAGIC ||
        src->magic != BUFFER_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (dst->data == NULL || src->data == NULL) {
        return STATUS_GAGAL;
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

    if (sx + w > src->lebar) { w = src->lebar - sx; }
    if (sy + h > src->tinggi) { h = src->tinggi - sy; }
    if (dx + w > dst->lebar)  { w = dst->lebar - dx; }
    if (dy + h > dst->tinggi) { h = dst->tinggi - dy; }
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
/* FUNGSI PUBLIK: OPERASI BUFFER LENGKAP                                      */
/* ========================================================================== */

status_t buffer_piksel_salin(struct buffer_piksel *dst,
                              const struct buffer_piksel *src)
{
    if (dst == NULL || src == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (dst->magic != BUFFER_MAGIC ||
        src->magic != BUFFER_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (dst->data == NULL || src->data == NULL) {
        return STATUS_GAGAL;
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

bool_t buffer_piksel_bandingkan(
    const struct buffer_piksel *buf1,
    const struct buffer_piksel *buf2)
{
    ukuran_t uk;

    if (buf1 == NULL || buf2 == NULL) {
        return SALAH;
    }
    if (buf1->magic != BUFFER_MAGIC ||
        buf2->magic != BUFFER_MAGIC) {
        return SALAH;
    }
    if (buf1->lebar != buf2->lebar ||
        buf1->tinggi != buf2->tinggi ||
        buf1->format != buf2->format) {
        return SALAH;
    }

    uk = MIN(buf1->ukuran, buf2->ukuran);
    return (kernel_memcmp(buf1->data, buf2->data, uk) == 0)
           ? BENAR : SALAH;
}

status_t buffer_piksel_bersihkan(struct buffer_piksel *buf)
{
    if (buf == NULL || buf->magic != BUFFER_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    if (buf->data == NULL) {
        return STATUS_GAGAL;
    }

    kernel_memset(buf->data, 0, buf->ukuran);
    buf->flags |= BUFFER_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: VALIDASI DAN INFORMASI                                  */
/* ========================================================================== */

bool_t buffer_piksel_validasi(const struct buffer_piksel *buf)
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
    if (buf->pitch != hitung_pitch(buf->lebar, buf->format)) {
        return SALAH;
    }
    if (buf->ukuran != hitung_ukuran_buffer(
            buf->lebar, buf->tinggi, buf->format)) {
        return SALAH;
    }
    return BENAR;
}

tak_bertanda32_t buffer_piksel_dapat_lebar(
    const struct buffer_piksel *buf)
{
    return (buf != NULL && buf->magic == BUFFER_MAGIC)
           ? buf->lebar : 0;
}

tak_bertanda32_t buffer_piksel_dapat_tinggi(
    const struct buffer_piksel *buf)
{
    return (buf != NULL && buf->magic == BUFFER_MAGIC)
           ? buf->tinggi : 0;
}

tak_bertanda8_t buffer_piksel_dapat_format(
    const struct buffer_piksel *buf)
{
    return (buf != NULL && buf->magic == BUFFER_MAGIC)
           ? buf->format : BUFFER_FMT_INVALID;
}

tak_bertanda32_t buffer_piksel_dapat_pitch(
    const struct buffer_piksel *buf)
{
    return (buf != NULL && buf->magic == BUFFER_MAGIC)
           ? buf->pitch : 0;
}

ukuran_t buffer_piksel_dapat_ukuran(
    const struct buffer_piksel *buf)
{
    return (buf != NULL && buf->magic == BUFFER_MAGIC)
           ? buf->ukuran : 0;
}

tak_bertanda8_t *buffer_piksel_dapat_data(
    struct buffer_piksel *buf)
{
    return (buf != NULL && buf->magic == BUFFER_MAGIC)
           ? buf->data : NULL;
}

tak_bertanda8_t buffer_piksel_dapat_bpp(
    const struct buffer_piksel *buf)
{
    return (buf != NULL && buf->magic == BUFFER_MAGIC)
           ? buf->bit_per_piksel : 0;
}

tak_bertanda32_t buffer_piksel_dapat_referensi(
    const struct buffer_piksel *buf)
{
    return (buf != NULL && buf->magic == BUFFER_MAGIC)
           ? buf->referensi : 0;
}

bool_t buffer_piksel_apakah_kotor(
    const struct buffer_piksel *buf)
{
    if (buf == NULL || buf->magic != BUFFER_MAGIC) {
        return SALAH;
    }
    return (buf->flags & BUFFER_FLAG_KOTOR) ? BENAR : SALAH;
}

void buffer_piksel_tandai_bersih(struct buffer_piksel *buf)
{
    if (buf != NULL && buf->magic == BUFFER_MAGIC) {
        buf->flags &= (tak_bertanda32_t)(~BUFFER_FLAG_KOTOR);
    }
}

tak_bertanda32_t buffer_piksel_dapat_jumlah_aktif(void)
{
    return g_buffer_aktif;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: KONVERSI FORMAT                                             */
/* ========================================================================== */

struct buffer_piksel *buffer_piksel_konversi_format(
    const struct buffer_piksel *src,
    tak_bertanda8_t fmt_tujuan)
{
    struct buffer_piksel *dst;
    tak_bertanda32_t x, y;
    tak_bertanda8_t r, g, b, a;
    status_t st;

    if (src == NULL || src->magic != BUFFER_MAGIC) {
        return NULL;
    }
    if (fmt_tujuan == BUFFER_FMT_INVALID ||
        fmt_tujuan > BUFFER_FMT_MAKS) {
        return NULL;
    }

    /* Jika format sama, buat salinan biasa */
    if (src->format == fmt_tujuan) {
        dst = buffer_piksel_buat(src->lebar, src->tinggi,
                                  src->format);
        if (dst == NULL) {
            return NULL;
        }
        kernel_memcpy(dst->data, src->data,
                      MIN(src->ukuran, dst->ukuran));
        return dst;
    }

    dst = buffer_piksel_buat(src->lebar, src->tinggi,
                              fmt_tujuan);
    if (dst == NULL) {
        return NULL;
    }

    for (y = 0; y < src->tinggi; y++) {
        for (x = 0; x < src->lebar; x++) {
            st = buffer_piksel_baca(src, x, y, &r, &g, &b, &a);
            if (st != STATUS_BERHASIL) {
                buffer_piksel_hapus(dst);
                return NULL;
            }
            st = buffer_piksel_tulis(dst, x, y, r, g, b, a);
            if (st != STATUS_BERHASIL) {
                buffer_piksel_hapus(dst);
                return NULL;
            }
        }
    }

    return dst;
}
