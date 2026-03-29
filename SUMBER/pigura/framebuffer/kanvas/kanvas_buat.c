/*
 * PIGURA OS - KANVAS_BUAT.C
 * ==========================
 * Modul pembuatan kanvas: berbagai varian penciptaan kanvas
 * untuk subsistem grafis Pigura OS.
 *
 * Berisi fungsi-fungsi pembuatan kanvas:
 * - kanvas_buat_dari_memori: dari memori eksternal
 * - kanvas_buat_kosong: kanvas piksel nol
 * - kanvas_buat_dengan_warna: kanvas terisi warna
 * - kanvas_buat_subkanvas: cuplikan wilayah kanvas
 * - kanvas_duplikat: salinan mendalam kanvas
 * - kanvas_buat_dari_bpp: dari bit per piksel
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) ketat
 * - Tidak menggunakan fitur C99/C11
 * - Seluruh nama dalam Bahasa Indonesia
 * - Maksimal 80 karakter per baris
 */

#include "../../inti/kernel.h"

/* ========================================================================== */
/* KONSTANTA                                                                  */
/* ========================================================================== */

#define KANVAS_MAGIC       0x4B564153
#define KANVAS_VERSI       0x0100
#define KANVAS_NAMA_MAKS   64

#define KV_BUFFER_VERSI    0x0100
#define KV_DIMENSI_MAKS    16384

#define BUFFER_MAGIC       0x42554642
#define BUFFER_FMT_INVALID 0
#define BUFFER_FMT_RGB332  1
#define BUFFER_FMT_RGB565  2
#define BUFFER_FMT_RGB888  3
#define BUFFER_FMT_ARGB8888 4
#define BUFFER_FMT_XRGB8888 5
#define BUFFER_FMT_BGRA8888 6
#define BUFFER_FMT_BGR565  7
#define BUFFER_FMT_MAKS    7

#define BUFFER_FLAG_ALOKASI  0x01
#define BUFFER_FLAG_EKSTERNAL 0x02
#define BUFFER_FLAG_KOTOR    0x08

#define KANVAS_FLAG_READ_ONLY  0x01
#define KANVAS_FLAG_DOUBLE_BUF 0x02
#define KANVAS_FLAG_TRANSPARAN 0x04

/* ========================================================================== */
/* MAKRO HELPER                                                              */
/* ========================================================================== */

#define bpp_dari_format(fmt) \
    (((fmt) == BUFFER_FMT_RGB332) ? 1 : \
     (((fmt) == BUFFER_FMT_RGB565) || \
      ((fmt) == BUFFER_FMT_BGR565)) ? 2 : \
     ((fmt) == BUFFER_FMT_RGB888) ? 3 : 4)

/* ========================================================================== */
/* STRUKTUR DATA                                                             */
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
/* VARIABEL GLOBAL INTERNAL                                                   */
/* ========================================================================== */

static tak_bertanda32_t g_kanvas_id_berikutnya = 1;

/* ========================================================================== */
/* FUNGSI HELPER INTERNAL: PERHITUNGAN                                        */
/* ========================================================================== */

/*
 * kv_hitung_pitch - Menghitung pitch (byte per baris).
 * Dibulatkan ke atas ke kelipatan 4 byte.
 */
static tak_bertanda32_t kv_hitung_pitch(
    tak_bertanda32_t lebar, tak_bertanda8_t format)
{
    tak_bertanda32_t bpp = (tak_bertanda32_t)bpp_dari_format(
        format);
    tak_bertanda32_t pitch_kasar = lebar * bpp;
    return RATAKAN_ATAS(pitch_kasar, 4);
}

/*
 * kv_hitung_ukuran - Total byte buffer (pitch * tinggi).
 */
static ukuran_t kv_hitung_ukuran(
    tak_bertanda32_t lebar, tak_bertanda32_t tinggi,
    tak_bertanda8_t format)
{
    tak_bertanda32_t p = kv_hitung_pitch(lebar, format);
    return (ukuran_t)p * (ukuran_t)tinggi;
}

/*
 * kv_cek_overflow - Periksa overflow perhitungan ukuran.
 */
static bool_t kv_cek_overflow(
    tak_bertanda32_t lebar, tak_bertanda32_t tinggi,
    tak_bertanda8_t format)
{
    tak_bertanda64_t uk64;
    uk64 = (tak_bertanda64_t)kv_hitung_pitch(lebar, format)
           * (tak_bertanda64_t)tinggi;
    return (uk64 <= (tak_bertanda64_t)0xFFFFFFFF)
           ? BENAR : SALAH;
}

/*
 * kv_format_dari_bpp - Konversi bpp ke format.
 */
static tak_bertanda8_t kv_format_dari_bpp(
    tak_bertanda32_t bpp)
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
 * kv_hitung_offset - Offset byte untuk koordinat (x, y).
 */
static tak_bertanda32_t kv_hitung_offset(
    const struct buffer_piksel *buf,
    tak_bertanda32_t x, tak_bertanda32_t y)
{
    return y * buf->pitch +
        x * (tak_bertanda32_t)bpp_dari_format(buf->format);
}

/* ========================================================================== */
/* FUNGSI HELPER INTERNAL: TULIS PIKSEL PER FORMAT                            */
/* ========================================================================== */

static void kv_tulis_piksel_8(tak_bertanda8_t *buf,
    tak_bertanda32_t ofs, tak_bertanda8_t r,
    tak_bertanda8_t g, tak_bertanda8_t b)
{
    buf[ofs] = (tak_bertanda8_t)(
        ((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6));
}

static void kv_tulis_piksel_16(tak_bertanda8_t *buf,
    tak_bertanda32_t ofs, tak_bertanda8_t r,
    tak_bertanda8_t g, tak_bertanda8_t b)
{
    tak_bertanda16_t pk = (tak_bertanda16_t)(
        (((tak_bertanda16_t)r >> 3) << 11) |
        (((tak_bertanda16_t)g >> 2) << 5)  |
        ((tak_bertanda16_t)b >> 3));
    buf[ofs]     = (tak_bertanda8_t)(pk & 0xFF);
    buf[ofs + 1] = (tak_bertanda8_t)(pk >> 8);
}

static void kv_tulis_piksel_24(tak_bertanda8_t *buf,
    tak_bertanda32_t ofs, tak_bertanda8_t r,
    tak_bertanda8_t g, tak_bertanda8_t b)
{
    buf[ofs]     = r;
    buf[ofs + 1] = g;
    buf[ofs + 2] = b;
}

static void kv_tulis_piksel_32(tak_bertanda8_t *buf,
    tak_bertanda32_t ofs, tak_bertanda8_t r,
    tak_bertanda8_t g, tak_bertanda8_t b,
    tak_bertanda8_t a)
{
    buf[ofs]     = b;
    buf[ofs + 1] = g;
    buf[ofs + 2] = r;
    buf[ofs + 3] = a;
}

/*
 * kv_tulis_piksel_fmt - Tulis piksel sesuai format.
 */
static void kv_tulis_piksel_fmt(tak_bertanda8_t *data,
    tak_bertanda32_t ofs, tak_bertanda8_t format,
    tak_bertanda8_t r, tak_bertanda8_t g,
    tak_bertanda8_t b, tak_bertanda8_t a)
{
    switch (format) {
    case BUFFER_FMT_RGB332:
        kv_tulis_piksel_8(data, ofs, r, g, b);
        break;
    case BUFFER_FMT_RGB565:
    case BUFFER_FMT_BGR565:
        kv_tulis_piksel_16(data, ofs, r, g, b);
        break;
    case BUFFER_FMT_RGB888:
        kv_tulis_piksel_24(data, ofs, r, g, b);
        break;
    case BUFFER_FMT_ARGB8888:
    case BUFFER_FMT_XRGB8888:
    case BUFFER_FMT_BGRA8888:
        kv_tulis_piksel_32(data, ofs, r, g, b, a);
        break;
    default:
        break;
    }
}

/* ========================================================================== */
/* FUNGSI HELPER INTERNAL: ALLOKASI BUFFER                                    */
/* ========================================================================== */

/*
 * kv_buat_buffer_internal
 * ----------------------
 * Mengalokasikan dan menginisialisasi struct buffer_piksel
 * dengan memori piksel yang baru dialokasikan (nol).
 * Mengembalikan NULL jika gagal.
 */
static struct buffer_piksel *kv_buat_buffer_internal(
    tak_bertanda32_t lebar, tak_bertanda32_t tinggi,
    tak_bertanda8_t format)
{
    struct buffer_piksel *buf;

    /* Validasi parameter */
    if (format == BUFFER_FMT_INVALID ||
        format > BUFFER_FMT_MAKS) {
        return NULL;
    }
    if (lebar == 0 || tinggi == 0) {
        return NULL;
    }
    if (lebar > KV_DIMENSI_MAKS ||
        tinggi > KV_DIMENSI_MAKS) {
        return NULL;
    }
    if (!kv_cek_overflow(lebar, tinggi, format)) {
        return NULL;
    }

    /* Alokasi struct buffer */
    buf = (struct buffer_piksel *)kmalloc(
        sizeof(struct buffer_piksel));
    if (buf == NULL) {
        return NULL;
    }

    kernel_memset(buf, 0, sizeof(struct buffer_piksel));

    /* Isi metadata buffer */
    buf->magic          = BUFFER_MAGIC;
    buf->versi          = KV_BUFFER_VERSI;
    buf->lebar          = lebar;
    buf->tinggi         = tinggi;
    buf->format         = format;
    buf->bit_per_piksel = (tak_bertanda8_t)(
        bpp_dari_format(format) * 8);
    buf->pitch          = kv_hitung_pitch(lebar, format);
    buf->ukuran         = kv_hitung_ukuran(lebar, tinggi,
                              format);
    buf->data           = NULL;
    buf->flags          = 0;
    buf->referensi      = 1;

    /* Alokasi memori piksel */
    buf->data = (tak_bertanda8_t *)kmalloc(buf->ukuran);
    if (buf->data == NULL) {
        kernel_memset(buf, 0, sizeof(struct buffer_piksel));
        kfree(buf);
        return NULL;
    }

    kernel_memset(buf->data, 0, buf->ukuran);
    buf->flags = BUFFER_FLAG_ALOKASI;

    return buf;
}

/*
 * kv_hapus_buffer_internal
 * -----------------------
 * Membebaskan buffer beserta memori pikselnya
 * jika dialokasikan secara internal.
 */
static void kv_hapus_buffer_internal(
    struct buffer_piksel *buf)
{
    if (buf == NULL) {
        return;
    }
    if ((buf->flags & BUFFER_FLAG_ALOKASI) &&
        buf->data != NULL) {
        kernel_memset(buf->data, 0, buf->ukuran);
        kfree(buf->data);
        buf->data = NULL;
    }
    buf->magic = 0;
    kernel_memset(buf, 0, sizeof(struct buffer_piksel));
    kfree(buf);
}

/* ========================================================================== */
/* FUNGSI HELPER INTERNAL: ALLOKASI KANVAS                                    */
/* ========================================================================== */

/*
 * kv_buat_kanvas_internal
 * ----------------------
 * Mengalokasikan dan menginisialisasi struct kanvas
 * dengan nilai bawaan. TIDAK mengalokasikan buffer.
 */
static struct kanvas *kv_buat_kanvas_internal(
    const char *nama)
{
    struct kanvas *k;

    k = (struct kanvas *)kmalloc(sizeof(struct kanvas));
    if (k == NULL) {
        return NULL;
    }

    kernel_memset(k, 0, sizeof(struct kanvas));

    /* Metadata kanvas */
    k->magic         = KANVAS_MAGIC;
    k->versi         = KANVAS_VERSI;
    k->buffer        = NULL;
    k->id            = g_kanvas_id_berikutnya++;
    k->aktif         = BENAR;
    k->flags         = 0;
    k->terkunci      = SALAH;
    k->pemilik_kunci = 0;

    /* Warna bawaan: putih */
    k->warna_r = 0xFF;
    k->warna_g = 0xFF;
    k->warna_b = 0xFF;
    k->warna_a = 0xFF;

    /* Transformasi: identitas */
    k->transformasi.jenis = 0;
    k->transformasi.tx    = 0;
    k->transformasi.ty    = 0;
    k->transformasi.sx    = 1;
    k->transformasi.sy    = 1;

    /* Klip: nonaktif (penuh) */
    k->klip.aktif  = SALAH;
    k->klip.x      = 0;
    k->klip.y      = 0;
    k->klip.lebar  = 0;
    k->klip.tinggi = 0;

    /* Lainnya */
    k->jumlah_gambar = 0;
    k->kotor         = SALAH;

    /* Salin nama */
    if (nama != NULL) {
        kernel_snprintf(k->nama, KANVAS_NAMA_MAKS,
                        "%s", nama);
    } else {
        kernel_snprintf(k->nama, KANVAS_NAMA_MAKS,
                        "(tanpa nama)");
    }

    return k;
}

/* ========================================================================== */
/* FUNGSI HELPER INTERNAL: ISI WARNA BUFFER                                   */
/* ========================================================================== */

/*
 * kv_isi_warna_buffer
 * ------------------
 * Mengisi seluruh buffer dengan warna tertentu.
 * Format 32-bit: gunakan kernel_memset32 (cepat).
 * Format lain: tulis piksel per piksel.
 */
static void kv_isi_warna_buffer(struct buffer_piksel *buf,
    tak_bertanda8_t r, tak_bertanda8_t g,
    tak_bertanda8_t b, tak_bertanda8_t a)
{
    tak_bertanda32_t x, y, ofs;

    if (buf == NULL || buf->data == NULL) {
        return;
    }

    /* Optimasi: format 32-bit gunakan memset32 */
    if (bpp_dari_format(buf->format) == 4) {
        tak_bertanda8_t tmp[4];
        tak_bertanda32_t raw;

        kv_tulis_piksel_32(tmp, 0, r, g, b, a);
        raw = (tak_bertanda32_t)tmp[0]       |
              ((tak_bertanda32_t)tmp[1] << 8)  |
              ((tak_bertanda32_t)tmp[2] << 16) |
              ((tak_bertanda32_t)tmp[3] << 24);
        kernel_memset32(buf->data, raw,
                        buf->lebar * buf->tinggi);
        buf->flags |= BUFFER_FLAG_KOTOR;
        return;
    }

    /* Format non-32-bit: piksel demi piksel */
    for (y = 0; y < buf->tinggi; y++) {
        for (x = 0; x < buf->lebar; x++) {
            ofs = kv_hitung_offset(buf, x, y);
            kv_tulis_piksel_fmt(buf->data, ofs,
                                buf->format, r, g, b, a);
        }
    }

    buf->flags |= BUFFER_FLAG_KOTOR;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: KANVAS_BUAT_DARI_MEMORI                                    */
/* ========================================================================== */

/*
 * kanvas_buat_dari_memori
 * ----------------------
 * Membuat kanvas dari memori piksel yang sudah ada.
 * Data TIDAK disalin dan TIDAK dibebaskan saat kanvas
 * dihapus (BUFFER_FLAG_EKSTERNAL).
 *
 * Parameter:
 *   nama   - Nama kanvas (boleh NULL)
 *   data   - Pointer ke data piksel (wajib valid)
 *   lebar  - Lebar piksel
 *   tinggi - Tinggi piksel
 *   format - Format piksel
 *   pitch  - Byte per baris (0 = otomatis)
 *
 * Kembalian: pointer kanvas baru atau NULL
 */
struct kanvas *kanvas_buat_dari_memori(const char *nama,
    tak_bertanda8_t *data, tak_bertanda32_t lebar,
    tak_bertanda32_t tinggi, tak_bertanda8_t format,
    tak_bertanda32_t pitch)
{
    struct kanvas *k;
    struct buffer_piksel *buf;
    tak_bertanda32_t pitch_aktual;

    /* Validasi parameter wajib */
    if (data == NULL) {
        return NULL;
    }
    if (format == BUFFER_FMT_INVALID ||
        format > BUFFER_FMT_MAKS) {
        return NULL;
    }
    if (lebar == 0 || tinggi == 0) {
        return NULL;
    }
    if (lebar > KV_DIMENSI_MAKS ||
        tinggi > KV_DIMENSI_MAKS) {
        return NULL;
    }
    if (!kv_cek_overflow(lebar, tinggi, format)) {
        return NULL;
    }

    /* Tentukan pitch */
    pitch_aktual = (pitch != 0) ? pitch :
                   kv_hitung_pitch(lebar, format);

    /* Alokasi struct buffer (tanpa alokasi data) */
    buf = (struct buffer_piksel *)kmalloc(
        sizeof(struct buffer_piksel));
    if (buf == NULL) {
        return NULL;
    }

    kernel_memset(buf, 0, sizeof(struct buffer_piksel));

    buf->magic          = BUFFER_MAGIC;
    buf->versi          = KV_BUFFER_VERSI;
    buf->lebar          = lebar;
    buf->tinggi         = tinggi;
    buf->format         = format;
    buf->bit_per_piksel = (tak_bertanda8_t)(
        bpp_dari_format(format) * 8);
    buf->pitch          = pitch_aktual;
    buf->ukuran         = (ukuran_t)pitch_aktual *
                              (ukuran_t)tinggi;
    buf->data           = data;
    buf->flags          = BUFFER_FLAG_EKSTERNAL;
    buf->referensi      = 1;

    /* Buat struct kanvas */
    k = kv_buat_kanvas_internal(nama);
    if (k == NULL) {
        kfree(buf);
        return NULL;
    }

    k->buffer = buf;
    return k;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: KANVAS_BUAT_KOSONG                                         */
/* ========================================================================== */

/*
 * kanvas_buat_kosong
 * -----------------
 * Membuat kanvas kosong (semua piksel bernilai nol).
 * Memori piksel dialokasikan secara internal.
 *
 * Parameter:
 *   nama   - Nama kanvas (boleh NULL)
 *   lebar  - Lebar piksel
 *   tinggi - Tinggi piksel
 *   format - Format piksel
 *
 * Kembalian: pointer kanvas baru atau NULL
 */
struct kanvas *kanvas_buat_kosong(const char *nama,
    tak_bertanda32_t lebar, tak_bertanda32_t tinggi,
    tak_bertanda8_t format)
{
    struct kanvas *k;
    struct buffer_piksel *buf;

    buf = kv_buat_buffer_internal(lebar, tinggi, format);
    if (buf == NULL) {
        return NULL;
    }

    k = kv_buat_kanvas_internal(nama);
    if (k == NULL) {
        kv_hapus_buffer_internal(buf);
        return NULL;
    }

    k->buffer = buf;
    return k;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: KANVAS_BUAT_DENGAN WARNA                                   */
/* ========================================================================== */

/*
 * kanvas_buat_dengan_warna
 * -----------------------
 * Membuat kanvas yang seluruh pikselnya terisi warna.
 * Format 32-bit: optimasi via kernel_memset32.
 * Format lain: pengisian piksel demi piksel.
 *
 * Parameter:
 *   nama   - Nama kanvas (boleh NULL)
 *   lebar  - Lebar piksel
 *   tinggi - Tinggi piksel
 *   format - Format piksel
 *   r, g, b, a - Komponen warna
 *
 * Kembalian: pointer kanvas baru atau NULL
 */
struct kanvas *kanvas_buat_dengan_warna(const char *nama,
    tak_bertanda32_t lebar, tak_bertanda32_t tinggi,
    tak_bertanda8_t format, tak_bertanda8_t r,
    tak_bertanda8_t g, tak_bertanda8_t b,
    tak_bertanda8_t a)
{
    struct kanvas *k;
    struct buffer_piksel *buf;

    buf = kv_buat_buffer_internal(lebar, tinggi, format);
    if (buf == NULL) {
        return NULL;
    }

    kv_isi_warna_buffer(buf, r, g, b, a);

    k = kv_buat_kanvas_internal(nama);
    if (k == NULL) {
        kv_hapus_buffer_internal(buf);
        return NULL;
    }

    k->buffer  = buf;
    k->warna_r = r;
    k->warna_g = g;
    k->warna_b = b;
    k->warna_a = a;
    return k;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: KANVAS_BUAT_SUBKANVAS                                      */
/* ========================================================================== */

/*
 * kanvas_buat_subkanvas
 * --------------------
 * Membuat sub-kanvas berupa cuplikan (snapshot) wilayah
 * persegi panjang dari kanvas induk. Sub-kanvas memiliki
 * buffer pikselnya sendiri (salinan, bukan referensi).
 * Wilayah dipotong (clipped) agar sesuai batas induk.
 *
 * Parameter:
 *   induk - Kanvas sumber (wajib valid)
 *   x, y  - Posisi pojok kiri-atas wilayah
 *   w, h  - Lebar dan tinggi wilayah
 *
 * Kembalian: pointer kanvas baru atau NULL
 */
struct kanvas *kanvas_buat_subkanvas(
    struct kanvas *induk, tak_bertanda32_t x,
    tak_bertanda32_t y, tak_bertanda32_t w,
    tak_bertanda32_t h)
{
    struct kanvas *k;
    struct buffer_piksel *buf;
    tak_bertanda32_t x_akhir, y_akhir;
    tak_bertanda32_t baris;
    tak_bertanda8_t format, bpp;

    /* Validasi kanvas induk */
    if (induk == NULL) {
        return NULL;
    }
    if (induk->magic != KANVAS_MAGIC) {
        return NULL;
    }
    if (induk->buffer == NULL) {
        return NULL;
    }
    if (induk->buffer->magic != BUFFER_MAGIC) {
        return NULL;
    }
    if (induk->buffer->data == NULL) {
        return NULL;
    }

    /* Validasi dimensi wilayah */
    if (w == 0 || h == 0) {
        return NULL;
    }

    /* Potong wilayah ke batas induk */
    x_akhir = x + w;
    y_akhir = y + h;

    if (x >= induk->buffer->lebar ||
        y >= induk->buffer->tinggi) {
        return NULL;
    }
    if (x_akhir > induk->buffer->lebar) {
        x_akhir = induk->buffer->lebar;
    }
    if (y_akhir > induk->buffer->tinggi) {
        y_akhir = induk->buffer->tinggi;
    }

    w = x_akhir - x;
    h = y_akhir - y;

    if (w == 0 || h == 0) {
        return NULL;
    }

    format = induk->buffer->format;
    bpp = (tak_bertanda8_t)bpp_dari_format(format);

    /* Buat buffer baru untuk sub-kanvas */
    buf = kv_buat_buffer_internal(w, h, format);
    if (buf == NULL) {
        return NULL;
    }

    /* Salin piksel baris demi baris */
    for (baris = 0; baris < h; baris++) {
        tak_bertanda8_t *src_ptr =
            induk->buffer->data +
            (y + baris) * induk->buffer->pitch +
            x * (tak_bertanda32_t)bpp;
        tak_bertanda8_t *dst_ptr = buf->data +
            baris * buf->pitch;
        kernel_memcpy(dst_ptr, src_ptr,
                      (ukuran_t)w * (ukuran_t)bpp);
    }

    /* Buat struct kanvas */
    k = kv_buat_kanvas_internal(induk->nama);
    if (k == NULL) {
        kv_hapus_buffer_internal(buf);
        return NULL;
    }

    k->buffer = buf;

    /* Salin warna dan transformasi dari induk */
    k->warna_r = induk->warna_r;
    k->warna_g = induk->warna_g;
    k->warna_b = induk->warna_b;
    k->warna_a = induk->warna_a;
    k->transformasi = induk->transformasi;

    return k;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: KANVAS_DUPLIKAT                                            */
/* ========================================================================== */

/*
 * kanvas_duplikat
 * --------------
 * Membuat salinan mendalam (deep clone) dari kanvas.
 * Memori buffer baru dialokasikan dan seluruh data piksel
 * disalin. Nama kanvas mendapat akhiran "(salinan)".
 *
 * Parameter:
 *   asal - Kanvas sumber (wajib valid)
 *
 * Kembalian: pointer kanvas baru atau NULL
 */
struct kanvas *kanvas_duplikat(
    const struct kanvas *asal)
{
    struct kanvas *k;
    struct buffer_piksel *buf;

    /* Validasi kanvas sumber */
    if (asal == NULL) {
        return NULL;
    }
    if (asal->magic != KANVAS_MAGIC) {
        return NULL;
    }
    if (asal->buffer == NULL) {
        return NULL;
    }
    if (asal->buffer->magic != BUFFER_MAGIC) {
        return NULL;
    }
    if (asal->buffer->data == NULL) {
        return NULL;
    }

    /* Buat buffer baru dengan dimensi dan format sama */
    buf = kv_buat_buffer_internal(
        asal->buffer->lebar, asal->buffer->tinggi,
        asal->buffer->format);
    if (buf == NULL) {
        return NULL;
    }

    /* Salin seluruh data piksel */
    kernel_memcpy(buf->data, asal->buffer->data,
                  MIN(buf->ukuran, asal->buffer->ukuran));
    buf->flags |= BUFFER_FLAG_KOTOR;

    /* Buat struct kanvas */
    k = kv_buat_kanvas_internal(asal->nama);
    if (k == NULL) {
        kv_hapus_buffer_internal(buf);
        return NULL;
    }

    /* Tambahkan akhiran "(salinan)" pada nama */
    kernel_snprintf(k->nama, KANVAS_NAMA_MAKS,
                    "%s(salinan)", asal->nama);

    /* Salin seluruh state kanvas */
    k->buffer        = buf;
    k->aktif         = asal->aktif;
    k->flags         = asal->flags;
    k->terkunci      = asal->terkunci;
    k->pemilik_kunci = asal->pemilik_kunci;
    k->warna_r       = asal->warna_r;
    k->warna_g       = asal->warna_g;
    k->warna_b       = asal->warna_b;
    k->warna_a       = asal->warna_a;
    k->transformasi  = asal->transformasi;
    k->klip          = asal->klip;
    k->jumlah_gambar = asal->jumlah_gambar;
    k->kotor         = SALAH;

    return k;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: KANVAS_BUAT_DARI_BPP                                       */
/* ========================================================================== */

/*
 * kanvas_buat_dari_bpp
 * -------------------
 * Membuat kanvas kosong dari nilai bit per piksel.
 * Konversi bpp ke format:
 *   8  -> RGB332
 *   16 -> RGB565
 *   24 -> RGB888
 *   32 -> XRGB8888
 *
 * Parameter:
 *   nama   - Nama kanvas (boleh NULL)
 *   lebar  - Lebar piksel
 *   tinggi - Tinggi piksel
 *   bpp    - Bit per piksel
 *
 * Kembalian: pointer kanvas baru atau NULL
 */
struct kanvas *kanvas_buat_dari_bpp(const char *nama,
    tak_bertanda32_t lebar, tak_bertanda32_t tinggi,
    tak_bertanda32_t bpp)
{
    tak_bertanda8_t format;

    format = kv_format_dari_bpp(bpp);
    if (format == BUFFER_FMT_INVALID) {
        return NULL;
    }

    return kanvas_buat_kosong(nama, lebar, tinggi, format);
}
