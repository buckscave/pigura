/*
 * PIGURA OS - KANVAS.C
 * ====================
 * Inti subsistem kanvas (canvas) untuk Pigura OS.
 *
 * Modul ini menyediakan abstraksi kanvas tingkat tinggi yang
 * membungkus buffer_piksel dan menyediakan state gambar
 * (warna, klip, transformasi) untuk operasi grafis.
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
/* KONSTANTA BUFFER PIKSEL (lokal, cocok dengan buffer.c)                     */
/* ========================================================================== */

#define BUFFER_MAGIC           0x42554642  /* "BUFB" */
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

/* ========================================================================== */
/* KONSTANTA KANVAS                                                           */
/* ========================================================================== */

#define KANVAS_MAGIC        0x4B564153  /* "KVAS" */
#define KANVAS_VERSI        0x0100
#define KANVAS_NAMA_MAKS    64

/* Flag kanvas */
#define KANVAS_FLAG_READ_ONLY    0x01
#define KANVAS_FLAG_DOUBLE_BUF   0x02
#define KANVAS_FLAG_TRANSPARAN   0x04

/* ========================================================================== */
/* STRUKTUR BUFFER PIKSEL (lokal, cocok dengan buffer.c)                     */
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
/* STRUKTUR KANVAS                                                            */
/* ========================================================================== */

/* State transformasi */
struct kanvas_transformasi {
    tak_bertanda32_t jenis;       /* 0=identitas, 1=translasi */
    tanda32_t tx, ty;             /* Offset translasi */
    tak_bertanda32_t sx, sy;      /* Faktor skala (16.16) */
};

/* Persegi panjang klip */
struct kanvas_klip {
    bool_t aktif;
    tak_bertanda32_t x, y, lebar, tinggi;
};

/* Struktur utama kanvas */
struct kanvas {
    tak_bertanda32_t magic;
    tak_bertanda16_t versi;

    /* Buffer internal */
    struct buffer_piksel *buffer;

    /* Identitas */
    char nama[KANVAS_NAMA_MAKS];
    tak_bertanda32_t id;

    /* State */
    bool_t aktif;
    tak_bertanda32_t flags;

    /* Kunci */
    bool_t terkunci;
    tak_bertanda32_t pemilik_kunci;

    /* Warna gambar saat ini */
    tak_bertanda8_t warna_r, warna_g, warna_b, warna_a;

    /* Transformasi dan klip */
    struct kanvas_transformasi transformasi;
    struct kanvas_klip klip;

    /* Penghitung */
    tak_bertanda32_t jumlah_gambar;
    bool_t kotor;
};

/* ========================================================================== */
/* VARIABEL GLOBAL STATIS                                                     */
/* ========================================================================== */

/* ID unik untuk kanvas berikutnya */
static tak_bertanda32_t g_kanvas_id_berikutnya = 1;

/* Status inisialisasi subsistem */
static bool_t g_kanvas_sistem_init = SALAH;

/* ========================================================================== */
/* FUNGSI HELPER INTERNAL                                                     */
/* ========================================================================== */

/*
 * kv_hitung_pitch
 * --------------
 * Menghitung pitch (byte per baris) dari lebar dan format.
 * Pitch diratakan ke atas ke kelipatan 4 byte.
 */
static tak_bertanda32_t kv_hitung_pitch(tak_bertanda32_t lebar,
                                         tak_bertanda8_t format)
{
    tak_bertanda32_t bpp;
    bpp = (tak_bertanda32_t)bpp_dari_format(format);
    return RATAKAN_ATAS(lebar * bpp, 4);
}

/*
 * kv_buat_buffer
 * --------------
 * Membuat buffer_piksel baru secara internal.
 * Meniru logika buffer_piksel_buat() dari buffer.c
 * karena tidak ada header bersama.
 *
 * Return: pointer buffer baru, atau NULL jika gagal.
 */
static struct buffer_piksel *kv_buat_buffer(
    tak_bertanda32_t lebar,
    tak_bertanda32_t tinggi,
    tak_bertanda8_t format)
{
    struct buffer_piksel *buf;
    tak_bertanda32_t pitch;
    ukuran_t ukuran;

    /* Validasi parameter */
    if (format == BUFFER_FMT_INVALID || format > BUFFER_FMT_MAKS) {
        return NULL;
    }
    if (lebar == 0 || tinggi == 0) {
        return NULL;
    }

    /* Alokasi struktur buffer */
    buf = (struct buffer_piksel *)kmalloc(
        sizeof(struct buffer_piksel));
    if (buf == NULL) {
        return NULL;
    }

    kernel_memset(buf, 0, sizeof(struct buffer_piksel));

    /* Isi metadata buffer */
    buf->magic = BUFFER_MAGIC;
    buf->versi = KANVAS_VERSI;
    buf->lebar = lebar;
    buf->tinggi = tinggi;
    buf->format = format;
    buf->bit_per_piksel =
        (tak_bertanda8_t)(bpp_dari_format(format) * 8);

    pitch = kv_hitung_pitch(lebar, format);
    buf->pitch = pitch;
    ukuran = (ukuran_t)pitch * (ukuran_t)tinggi;
    buf->ukuran = ukuran;

    /* Alokasi data piksel */
    buf->data = (tak_bertanda8_t *)kmalloc(ukuran);
    if (buf->data == NULL) {
        kernel_memset(buf, 0, sizeof(struct buffer_piksel));
        kfree(buf);
        return NULL;
    }

    kernel_memset(buf->data, 0, ukuran);
    buf->flags = BUFFER_FLAG_ALOKASI;
    buf->referensi = 1;

    return buf;
}

/*
 * kv_inisialisasi_state
 * --------------------
 * Mengisi state default untuk kanvas baru.
 */
static void kv_inisialisasi_state(struct kanvas *kv)
{
    /* Transformasi ke identitas */
    kv->transformasi.jenis = 0;
    kv->transformasi.tx = 0;
    kv->transformasi.ty = 0;
    kv->transformasi.sx = 0x00010000;  /* 1.0 dalam 16.16 */
    kv->transformasi.sy = 0x00010000;

    /* Klip ke seluruh permukaan */
    kv->klip.aktif = SALAH;
    kv->klip.x = 0;
    kv->klip.y = 0;
    kv->klip.lebar = kv->buffer->lebar;
    kv->klip.tinggi = kv->buffer->tinggi;

    /* Warna default: putih */
    kv->warna_r = 255;
    kv->warna_g = 255;
    kv->warna_b = 255;
    kv->warna_a = 255;

    /* Lainnya */
    kv->aktif = BENAR;
    kv->terkunci = SALAH;
    kv->pemilik_kunci = 0;
    kv->flags = 0;
    kv->jumlah_gambar = 0;
    kv->kotor = SALAH;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: INISIALISASI SISTEM                                        */
/* ========================================================================== */

/*
 * kanvas_init_sistem
 * ------------------
 * Menginisialisasi subsistem kanvas.
 * Me-reset counter ID dan menandai sistem sudah siap.
 *
 * Return: STATUS_BERHASIL selalu.
 */
status_t kanvas_init_sistem(void)
{
    g_kanvas_id_berikutnya = 1;
    g_kanvas_sistem_init = BENAR;
    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: PEMBUATAN KANVAS                                           */
/* ========================================================================== */

/*
 * kanvas_buat
 * -----------
 * Membuat kanvas baru dengan buffer piksel milik sendiri.
 * Format default: XRGB8888 (32-bit).
 *
 * Parameter:
 *   nama   - Nama kanvas (bisa NULL)
 *   lebar  - Lebar dalam piksel (> 0)
 *   tinggi - Tinggi dalam piksel (> 0)
 *   format - Format piksel (BUFFER_FMT_*)
 *
 * Return: pointer kanvas baru, atau NULL jika gagal.
 */
struct kanvas *kanvas_buat(const char *nama,
                            tak_bertanda32_t lebar,
                            tak_bertanda32_t tinggi,
                            tak_bertanda8_t format)
{
    struct kanvas *kv;

    /* Validasi format */
    if (format == BUFFER_FMT_INVALID || format > BUFFER_FMT_MAKS) {
        return NULL;
    }
    if (lebar == 0 || tinggi == 0) {
        return NULL;
    }

    /* Alokasi struktur kanvas */
    kv = (struct kanvas *)kmalloc(sizeof(struct kanvas));
    if (kv == NULL) {
        return NULL;
    }

    kernel_memset(kv, 0, sizeof(struct kanvas));

    /* Buat buffer internal */
    kv->buffer = kv_buat_buffer(lebar, tinggi, format);
    if (kv->buffer == NULL) {
        kernel_memset(kv, 0, sizeof(struct kanvas));
        kfree(kv);
        return NULL;
    }

    /* Isi metadata kanvas */
    kv->magic = KANVAS_MAGIC;
    kv->versi = KANVAS_VERSI;
    kv->id = g_kanvas_id_berikutnya;
    g_kanvas_id_berikutnya++;

    /* Set nama jika diberikan */
    if (nama != NULL) {
        kernel_snprintf(kv->nama, KANVAS_NAMA_MAKS, "%s", nama);
    } else {
        kernel_snprintf(kv->nama, KANVAS_NAMA_MAKS, "kanvas_%u",
                         kv->id);
    }

    /* Isi state default */
    kv_inisialisasi_state(kv);

    return kv;
}

/*
 * kanvas_buat_dari_buffer
 * -----------------------
 * Membuat kanvas yang membungkus buffer_piksel yang sudah ada.
 * Kanvas TIDAK memiliki buffer; pemilik tetap pemanggil.
 *
 * Parameter:
 *   nama - Nama kanvas (bisa NULL)
 *   buf  - Pointer ke buffer_piksel (harus valid)
 *
 * Return: pointer kanvas baru, atau NULL jika gagal.
 */
struct kanvas *kanvas_buat_dari_buffer(const char *nama,
                                        struct buffer_piksel *buf)
{
    struct kanvas *kv;

    /* Validasi buffer */
    if (buf == NULL) {
        return NULL;
    }
    if (buf->magic != BUFFER_MAGIC) {
        return NULL;
    }
    if (buf->data == NULL) {
        return NULL;
    }

    /* Alokasi struktur kanvas */
    kv = (struct kanvas *)kmalloc(sizeof(struct kanvas));
    if (kv == NULL) {
        return NULL;
    }

    kernel_memset(kv, 0, sizeof(struct kanvas));

    /* Gunakan buffer yang sudah ada */
    kv->buffer = buf;

    /* Isi metadata kanvas */
    kv->magic = KANVAS_MAGIC;
    kv->versi = KANVAS_VERSI;
    kv->id = g_kanvas_id_berikutnya;
    g_kanvas_id_berikutnya++;

    /* Set nama */
    if (nama != NULL) {
        kernel_snprintf(kv->nama, KANVAS_NAMA_MAKS, "%s", nama);
    } else {
        kernel_snprintf(kv->nama, KANVAS_NAMA_MAKS, "kanvas_%u",
                         kv->id);
    }

    /* Isi state default */
    kv_inisialisasi_state(kv);

    return kv;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: PENGHAPUSAN KANVAS                                         */
/* ========================================================================== */

/*
 * kanvas_hapus
 * ------------
 * Menghapus kanvas dan membebaskan sumber daya.
 * Jika buffer dimiliki (flag BUFFER_FLAG_ALOKASI), buffer juga dihapus.
 * Jika kanvas terkunci, operasi ditolak.
 *
 * Parameter:
 *   kv - Pointer ke kanvas
 *
 * Return: STATUS_BERHASIL atau kode error.
 */
status_t kanvas_hapus(struct kanvas *kv)
{
    if (kv == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (kv->magic != KANVAS_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (kv->terkunci) {
        return STATUS_BUSY;
    }

    /* Hapus buffer jika dimiliki kanvas */
    if (kv->buffer != NULL) {
        if (kv->buffer->magic == BUFFER_MAGIC) {
            if ((kv->buffer->flags & BUFFER_FLAG_ALOKASI) &&
                (kv->buffer->data != NULL)) {
                kernel_memset(kv->buffer->data, 0,
                              kv->buffer->ukuran);
                kfree(kv->buffer->data);
                kv->buffer->data = NULL;
            }
            kv->buffer->magic = 0;
            kv->buffer->flags = 0;
            kv->buffer->ukuran = 0;
        }
        kfree(kv->buffer);
        kv->buffer = NULL;
    }

    /* Invalidate kanvas sebelum bebaskan */
    kv->magic = 0;
    kfree(kv);
    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: WARNA GAMBAR                                               */
/* ========================================================================== */

/*
 * kanvas_set_warna
 * ----------------
 * Mengatur warna gambar saat ini.
 *
 * Parameter:
 *   kv - Pointer ke kanvas
 *   r, g, b, a - Komponen warna (0-255)
 *
 * Return: STATUS_BERHASIL atau kode error.
 */
status_t kanvas_set_warna(struct kanvas *kv,
                           tak_bertanda8_t r,
                           tak_bertanda8_t g,
                           tak_bertanda8_t b,
                           tak_bertanda8_t a)
{
    if (kv == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (kv->magic != KANVAS_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    kv->warna_r = r;
    kv->warna_g = g;
    kv->warna_b = b;
    kv->warna_a = a;

    return STATUS_BERHASIL;
}

/*
 * kanvas_dapat_warna
 * ------------------
 * Mendapatkan warna gambar saat ini.
 *
 * Parameter:
 *   kv          - Pointer ke kanvas
 *   r, g, b, a  - Pointer output komponen warna
 *
 * Return: STATUS_BERHASIL atau kode error.
 */
status_t kanvas_dapat_warna(const struct kanvas *kv,
                             tak_bertanda8_t *r,
                             tak_bertanda8_t *g,
                             tak_bertanda8_t *b,
                             tak_bertanda8_t *a)
{
    if (kv == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (kv->magic != KANVAS_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (r == NULL || g == NULL || b == NULL || a == NULL) {
        return STATUS_PARAM_NULL;
    }

    *r = kv->warna_r;
    *g = kv->warna_g;
    *b = kv->warna_b;
    *a = kv->warna_a;

    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: AREA KLIP                                                  */
/* ========================================================================== */

/*
 * kanvas_set_klip
 * ---------------
 * Mengatur persegi panjang klip. Area klip dibatasi
 * agar tidak melebihi batas kanvas.
 *
 * Parameter:
 *   kv - Pointer ke kanvas
 *   x  - Posisi x klip
 *   y  - Posisi y klip
 *   w  - Lebar klip
 *   h  - Tinggi klip
 *
 * Return: STATUS_BERHASIL atau kode error.
 */
status_t kanvas_set_klip(struct kanvas *kv,
                          tak_bertanda32_t x,
                          tak_bertanda32_t y,
                          tak_bertanda32_t w,
                          tak_bertanda32_t h)
{
    tak_bertanda32_t x_akhir, y_akhir;

    if (kv == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (kv->magic != KANVAS_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (kv->buffer == NULL) {
        return STATUS_GAGAL;
    }

    /* Klip ke batas kanvas */
    x = MAKS(x, 0);
    y = MAKS(y, 0);
    x_akhir = MIN(x + w, kv->buffer->lebar);
    y_akhir = MIN(y + h, kv->buffer->tinggi);

    /* Jika area klip kosong setelah pembatasan */
    if (x >= x_akhir || y >= y_akhir) {
        return STATUS_PARAM_JARAK;
    }

    kv->klip.aktif = BENAR;
    kv->klip.x = x;
    kv->klip.y = y;
    kv->klip.lebar = x_akhir - x;
    kv->klip.tinggi = y_akhir - y;

    return STATUS_BERHASIL;
}

/*
 * kanvas_hapus_klip
 * -----------------
 * Menonaktifkan klip. Seluruh permukaan kanvas aktif.
 *
 * Parameter:
 *   kv - Pointer ke kanvas
 *
 * Return: STATUS_BERHASIL atau kode error.
 */
status_t kanvas_hapus_klip(struct kanvas *kv)
{
    if (kv == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (kv->magic != KANVAS_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (kv->buffer == NULL) {
        return STATUS_GAGAL;
    }

    kv->klip.aktif = SALAH;
    kv->klip.x = 0;
    kv->klip.y = 0;
    kv->klip.lebar = kv->buffer->lebar;
    kv->klip.tinggi = kv->buffer->tinggi;

    return STATUS_BERHASIL;
}

/*
 * kanvas_dapat_klip
 * -----------------
 * Mendapatkan persegi panjang klip saat ini.
 *
 * Parameter:
 *   kv          - Pointer ke kanvas
 *   x, y, w, h  - Pointer output klip
 *
 * Return: STATUS_BERHASIL atau kode error.
 */
status_t kanvas_dapat_klip(const struct kanvas *kv,
                            tak_bertanda32_t *x,
                            tak_bertanda32_t *y,
                            tak_bertanda32_t *w,
                            tak_bertanda32_t *h)
{
    if (kv == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (kv->magic != KANVAS_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (x == NULL || y == NULL || w == NULL || h == NULL) {
        return STATUS_PARAM_NULL;
    }

    *x = kv->klip.x;
    *y = kv->klip.y;
    *w = kv->klip.lebar;
    *h = kv->klip.tinggi;

    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: TRANSFORMASI                                                */
/* ========================================================================== */

/*
 * kanvas_set_transformasi
 * -----------------------
 * Mengatur state transformasi kanvas.
 *
 * Parameter:
 *   kv   - Pointer ke kanvas
 *   jenis - Jenis: 0=identitas, 1=translasi, 2=skala
 *   tx, ty - Offset translasi
 *   sx, sy - Faktor skala (fixed-point 16.16)
 *
 * Return: STATUS_BERHASIL atau kode error.
 */
status_t kanvas_set_transformasi(struct kanvas *kv,
                                  tak_bertanda32_t jenis,
                                  tanda32_t tx,
                                  tanda32_t ty,
                                  tak_bertanda32_t sx,
                                  tak_bertanda32_t sy)
{
    if (kv == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (kv->magic != KANVAS_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    kv->transformasi.jenis = jenis;
    kv->transformasi.tx = tx;
    kv->transformasi.ty = ty;
    kv->transformasi.sx = sx;
    kv->transformasi.sy = sy;

    return STATUS_BERHASIL;
}

/*
 * kanvas_reset_transformasi
 * -------------------------
 * Mengembalikan transformasi ke identitas.
 *
 * Parameter:
 *   kv - Pointer ke kanvas
 *
 * Return: STATUS_BERHASIL atau kode error.
 */
status_t kanvas_reset_transformasi(struct kanvas *kv)
{
    if (kv == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (kv->magic != KANVAS_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    kv->transformasi.jenis = 0;
    kv->transformasi.tx = 0;
    kv->transformasi.ty = 0;
    kv->transformasi.sx = 0x00010000;
    kv->transformasi.sy = 0x00010000;

    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: NAMA DAN IDENTITAS                                         */
/* ========================================================================== */

/*
 * kanvas_set_nama
 * ---------------
 * Mengatur nama kanvas. Nama dipotong jika terlalu panjang.
 *
 * Parameter:
 *   kv   - Pointer ke kanvas
 *   nama - String nama baru
 *
 * Return: STATUS_BERHASIL atau kode error.
 */
status_t kanvas_set_nama(struct kanvas *kv, const char *nama)
{
    if (kv == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (kv->magic != KANVAS_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (nama == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_snprintf(kv->nama, KANVAS_NAMA_MAKS, "%s", nama);

    return STATUS_BERHASIL;
}

/*
 * kanvas_dapat_nama
 * -----------------
 * Mendapatkan nama kanvas.
 *
 * Parameter:
 *   kv - Pointer ke kanvas
 *
 * Return: String nama kanvas, atau NULL jika invalid.
 */
const char *kanvas_dapat_nama(const struct kanvas *kv)
{
    if (kv == NULL) {
        return NULL;
    }
    if (kv->magic != KANVAS_MAGIC) {
        return NULL;
    }

    return (const char *)kv->nama;
}

/*
 * kanvas_dapat_id
 * ---------------
 * Mendapatkan ID unik kanvas.
 *
 * Parameter:
 *   kv - Pointer ke kanvas
 *
 * Return: ID kanvas, atau 0 jika invalid.
 */
tak_bertanda32_t kanvas_dapat_id(const struct kanvas *kv)
{
    if (kv == NULL) {
        return 0;
    }
    if (kv->magic != KANVAS_MAGIC) {
        return 0;
    }

    return kv->id;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: AKSES BUFFER                                               */
/* ========================================================================== */

/*
 * kanvas_dapat_buffer
 * -------------------
 * Mendapatkan pointer ke buffer_piksel internal.
 *
 * Parameter:
 *   kv - Pointer ke kanvas
 *
 * Return: Pointer buffer, atau NULL jika invalid.
 */
struct buffer_piksel *kanvas_dapat_buffer(struct kanvas *kv)
{
    if (kv == NULL) {
        return NULL;
    }
    if (kv->magic != KANVAS_MAGIC) {
        return NULL;
    }

    return kv->buffer;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: INFORMASI DIMENSI                                          */
/* ========================================================================== */

/*
 * kanvas_dapat_lebar
 * ------------------
 * Mendapatkan lebar kanvas dalam piksel.
 */
tak_bertanda32_t kanvas_dapat_lebar(const struct kanvas *kv)
{
    if (kv == NULL || kv->magic != KANVAS_MAGIC) {
        return 0;
    }
    if (kv->buffer == NULL) {
        return 0;
    }
    return kv->buffer->lebar;
}

/*
 * kanvas_dapat_tinggi
 * -------------------
 * Mendapatkan tinggi kanvas dalam piksel.
 */
tak_bertanda32_t kanvas_dapat_tinggi(const struct kanvas *kv)
{
    if (kv == NULL || kv->magic != KANVAS_MAGIC) {
        return 0;
    }
    if (kv->buffer == NULL) {
        return 0;
    }
    return kv->buffer->tinggi;
}

/*
 * kanvas_dapat_format
 * -------------------
 * Mendapatkan format piksel kanvas.
 */
tak_bertanda8_t kanvas_dapat_format(const struct kanvas *kv)
{
    if (kv == NULL || kv->magic != KANVAS_MAGIC) {
        return BUFFER_FMT_INVALID;
    }
    if (kv->buffer == NULL) {
        return BUFFER_FMT_INVALID;
    }
    return kv->buffer->format;
}

/*
 * kanvas_dapat_pitch
 * ------------------
 * Mendapatkan pitch (byte per baris) kanvas.
 */
tak_bertanda32_t kanvas_dapat_pitch(const struct kanvas *kv)
{
    if (kv == NULL || kv->magic != KANVAS_MAGIC) {
        return 0;
    }
    if (kv->buffer == NULL) {
        return 0;
    }
    return kv->buffer->pitch;
}

/*
 * kanvas_dapat_ukuran
 * -------------------
 * Mendapatkan total ukuran buffer kanvas dalam byte.
 */
ukuran_t kanvas_dapat_ukuran(const struct kanvas *kv)
{
    if (kv == NULL || kv->magic != KANVAS_MAGIC) {
        return 0;
    }
    if (kv->buffer == NULL) {
        return 0;
    }
    return kv->buffer->ukuran;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: STATE KOTOR                                                */
/* ========================================================================== */

/*
 * kanvas_apakah_kotor
 * -------------------
 * Memeriksa apakah kanvas dalam keadaan kotor
 * (perlu disalin ke layar).
 *
 * Parameter:
 *   kv - Pointer ke kanvas
 *
 * Return: BENAR jika kotor, SALAH jika bersih atau invalid.
 */
bool_t kanvas_apakah_kotor(const struct kanvas *kv)
{
    if (kv == NULL || kv->magic != KANVAS_MAGIC) {
        return SALAH;
    }
    return kv->kotor;
}

/*
 * kanvas_tandai_bersih
 * --------------------
 * Menandai kanvas sebagai bersih (tidak perlu disalin).
 *
 * Parameter:
 *   kv - Pointer ke kanvas
 *
 * Return: STATUS_BERHASIL atau kode error.
 */
status_t kanvas_tandai_bersih(struct kanvas *kv)
{
    if (kv == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (kv->magic != KANVAS_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    kv->kotor = SALAH;

    /* Juga bersihkan buffer internal */
    if (kv->buffer != NULL && kv->buffer->magic == BUFFER_MAGIC) {
        kv->buffer->flags &=
            (tak_bertanda32_t)(~BUFFER_FLAG_KOTOR);
    }

    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: VALIDASI                                                   */
/* ========================================================================== */

/*
 * kanvas_validasi
 * ---------------
 * Validasi integritas penuh kanvas: magic, buffer, dimensi,
 * format, pitch, dan konsistensi ukuran.
 *
 * Parameter:
 *   kv - Pointer ke kanvas
 *
 * Return: BENAR jika valid, SALAH jika tidak.
 */
bool_t kanvas_validasi(const struct kanvas *kv)
{
    tak_bertanda32_t pitch_diharapkan;
    ukuran_t ukuran_diharapkan;

    if (kv == NULL) {
        return SALAH;
    }
    if (kv->magic != KANVAS_MAGIC) {
        return SALAH;
    }
    if (kv->buffer == NULL) {
        return SALAH;
    }
    if (kv->buffer->magic != BUFFER_MAGIC) {
        return SALAH;
    }
    if (kv->buffer->lebar == 0 || kv->buffer->tinggi == 0) {
        return SALAH;
    }
    if (kv->buffer->format == BUFFER_FMT_INVALID ||
        kv->buffer->format > BUFFER_FMT_MAKS) {
        return SALAH;
    }
    if (kv->buffer->data == NULL) {
        return SALAH;
    }

    /* Verifikasi pitch */
    pitch_diharapkan = kv_hitung_pitch(kv->buffer->lebar,
                                        kv->buffer->format);
    if (kv->buffer->pitch != pitch_diharapkan) {
        return SALAH;
    }

    /* Verifikasi ukuran */
    ukuran_diharapkan = (ukuran_t)kv->buffer->pitch *
                        (ukuran_t)kv->buffer->tinggi;
    if (kv->buffer->ukuran != ukuran_diharapkan) {
        return SALAH;
    }

    /* Verifikasi klip tidak melebihi batas */
    if (kv->klip.x + kv->klip.lebar > kv->buffer->lebar) {
        return SALAH;
    }
    if (kv->klip.y + kv->klip.tinggi > kv->buffer->tinggi) {
        return SALAH;
    }

    return BENAR;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: PENGUNCIAN                                                 */
/* ========================================================================== */

/*
 * kanvas_kunci
 * ------------
 * Mengunci kanvas sehingga tidak dapat dihapus.
 * Hanya pemilik yang dapat membuka kunci.
 *
 * Parameter:
 *   kv      - Pointer ke kanvas
 *   pemilik - ID pemilik kunci
 *
 * Return: STATUS_BERHASIL atau STATUS_BUSY jika terkunci.
 */
status_t kanvas_kunci(struct kanvas *kv,
                       tak_bertanda32_t pemilik)
{
    if (kv == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (kv->magic != KANVAS_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (kv->terkunci) {
        return STATUS_BUSY;
    }

    kv->terkunci = BENAR;
    kv->pemilik_kunci = pemilik;

    return STATUS_BERHASIL;
}

/*
 * kanvas_buka_kunci
 * -----------------
 * Membuka kunci kanvas.
 * Hanya pemilik yang dapat membuka kunci.
 *
 * Parameter:
 *   kv      - Pointer ke kanvas
 *   pemilik - ID pemilik kunci
 *
 * Return: STATUS_BERHASIL, STATUS_PARAM_NULL, atau
 *         STATUS_PARAM_INVALID jika bukan pemilik.
 */
status_t kanvas_buka_kunci(struct kanvas *kv,
                            tak_bertanda32_t pemilik)
{
    if (kv == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (kv->magic != KANVAS_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (!kv->terkunci) {
        return STATUS_BERHASIL;
    }
    if (kv->pemilik_kunci != pemilik) {
        return STATUS_PARAM_INVALID;
    }

    kv->terkunci = SALAH;
    kv->pemilik_kunci = 0;

    return STATUS_BERHASIL;
}
