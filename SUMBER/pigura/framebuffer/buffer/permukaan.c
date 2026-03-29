/*
 * PIGURA OS - PERMUKAAN.C
 * ========================
 * Abstraksi permukaan (surface) untuk framebuffer.
 *
 * Modul ini menyediakan lapisan abstraksi tingkat tinggi di atas
 * buffer piksel mentah. Setiap permukaan membungkus buffer piksel
 * dengan metadata tambahan: identitas, penguncian, visibilitas,
 * klip rectangle, dan pelacakan region kotor (dirty region).
 *
 * BERKAS INI BUKAN driver hardware. Ini murni abstraksi software
 * untuk manajemen permukaan grafis.
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
/* KONSTANTA PERMUKAAN                                                        */
/* ========================================================================== */

#define PERMUKAAN_MAGIC           0x50524D46  /* "PRMF"              */
#define PERMUKAAN_VERSI           0x0100
#define PERMUKAAN_DIMENSI_MAKS    16384

/*
 * Konstanta format pixel - disalin dari buffer.c agar
 * permukaan.c mandiri tanpa include buffer.h.
 */
#define BUF_FMT_INVALID           0
#define BUF_FMT_RGB332            1
#define BUF_FMT_RGB565            2
#define BUF_FMT_RGB888            3
#define BUF_FMT_ARGB8888          4
#define BUF_FMT_XRGB8888          5
#define BUF_FMT_BGRA8888          6
#define BUF_FMT_BGR565            7
#define BUF_FMT_MAKS              7

#define BUF_MAGIC                 0x42554642  /* "BUFB" */

/* Flag permukaan */
#define PERMUKAAN_FLAG_DOUBLE_BUFFER  0x01
#define PERMUKAAN_FLAG_GPU            0x02
#define PERMUKAAN_FLAG_READ_ONLY      0x04
#define PERMUKAAN_FLAG_TRANSPARAN     0x08

/* Batas region kotor dan nama */
#define PERMUKAAN_MAKS_REGION_KOTOR   16
#define PERMUKAAN_NAMA_MAKS_LEN       64

/* ========================================================================== */
/* STRUKTUR DATA                                                              */
/* ========================================================================== */

/*
 * struct buffer_piksel
 * --------------------
 * Struktur buffer piksel dari buffer.c. Didefinisikan ulang di sini
 * agar permukaan.c dapat di-compile secara mandiri.
 */
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

/*
 * struct wilayah_kotor
 * --------------------
 * Region persegi panjang yang menandai area piksel berubah.
 * Digunakan untuk optimasi rendering parsial.
 */
struct wilayah_kotor {
    tak_bertanda32_t x1, y1, x2, y2;  /* Bounding rect     */
    bool_t aktif;                       /* Apakah aktif       */
};

/*
 * struct permukaan_fb
 * -------------------
 * Struktur utama permukaan framebuffer. Membungkus buffer piksel
 * dengan metadata tambahan untuk manajemen grafis tingkat tinggi.
 */
struct permukaan_fb {
    tak_bertanda32_t magic;
    tak_bertanda16_t versi;

    /* Buffer piksel internal */
    struct buffer_piksel *buffer;

    /* Identitas permukaan */
    char nama[PERMUKAAN_NAMA_MAKS_LEN];
    tak_bertanda32_t id;

    /* Keadaan permukaan */
    bool_t aktif;
    bool_t terlihat;
    tak_bertanda32_t flags;

    /* Penguncian eksklusif */
    bool_t terkunci;
    tak_bertanda32_t pemilik_kunci;  /* PID pemilik kunci */

    /* Pelacakan region kotor */
    struct wilayah_kotor
        region_kotor[PERMUKAAN_MAKS_REGION_KOTOR];
    tak_bertanda32_t jumlah_region_kotor;

    /* Rectangle klip (batas gambar) */
    tak_bertanda32_t klip_x, klip_y;
    tak_bertanda32_t klip_lebar, klip_tinggi;
    bool_t klip_aktif;
};

/* ========================================================================== */
/* VARIABEL GLOBAL                                                            */
/* ========================================================================== */

/*
 * g_id_berikutnya - Penghitung ID unik untuk setiap permukaan.
 * Dimulai dari 1 agar ID nol dapat digunakan sebagai penanda
 * permukaan tidak valid.
 */
static tak_bertanda32_t g_id_berikutnya = 1;

/* ========================================================================== */
/* MAKRO HELPER                                                               */
/* ========================================================================== */

/*
 * bpp_dari_format - Mendapatkan byte per piksel dari format.
 * Harus konsisten dengan definisi di buffer.c.
 */
#define bpp_dari_format(fmt) \
    (((fmt) == BUF_FMT_RGB332) ? 1 : \
     (((fmt) == BUF_FMT_RGB565) || \
      ((fmt) == BUF_FMT_BGR565)) ? 2 : \
     ((fmt) == BUF_FMT_RGB888) ? 3 : 4)

/*
 * hitung_pitch_lokal - Menghitung pitch dari lebar dan format.
 * Pitch dibulatkan ke atas ke kelipatan 4 byte.
 */
static tak_bertanda32_t hitung_pitch_lokal(tak_bertanda32_t lebar,
                                           tak_bertanda8_t format)
{
    tak_bertanda32_t bpp = (tak_bertanda32_t)bpp_dari_format(format);
    tak_bertanda32_t pitch_kasar = lebar * bpp;
    return RATAKAN_ATAS(pitch_kasar, 4);
}

/*
 * format_valid - Memvalidasi kode format piksel.
 */
static bool_t format_valid(tak_bertanda8_t format)
{
    if (format == BUF_FMT_INVALID || format > BUF_FMT_MAKS) {
        return SALAH;
    }
    return BENAR;
}

/*
 * cek_param_dimensi - Memvalidasi parameter dimensi permukaan.
 */
static status_t cek_param_dimensi(tak_bertanda32_t lebar,
                                  tak_bertanda32_t tinggi,
                                  tak_bertanda8_t format)
{
    if (!format_valid(format)) {
        return STATUS_PARAM_INVALID;
    }
    if (lebar == 0 || tinggi == 0) {
        return STATUS_PARAM_JARAK;
    }
    if (lebar > PERMUKAAN_DIMENSI_MAKS ||
        tinggi > PERMUKAAN_DIMENSI_MAKS) {
        return STATUS_PARAM_JARAK;
    }
    return STATUS_BERHASIL;
}

/*
 * cek_permukaan_valid - Validasi dasar pointer permukaan.
 * Memeriksa magic number dan bahwa buffer internal valid.
 */
static bool_t cek_permukaan_valid(
    const struct permukaan_fb *sf)
{
    if (sf == NULL) {
        return SALAH;
    }
    if (sf->magic != PERMUKAAN_MAGIC) {
        return SALAH;
    }
    if (sf->buffer == NULL) {
        return SALAH;
    }
    if (sf->buffer->magic != BUF_MAGIC) {
        return SALAH;
    }
    return BENAR;
}

/*
 * wilayah_tumpang_tindih - Cek apakah dua wilayah kotor saling
 * tumpang tindih (overlap).
 */
static bool_t wilayah_tumpang_tindih(
    const struct wilayah_kotor *a,
    const struct wilayah_kotor *b)
{
    if (a == NULL || b == NULL) {
        return SALAH;
    }
    if (!a->aktif || !b->aktif) {
        return SALAH;
    }
    if (a->x1 >= b->x2 || b->x1 >= a->x2) {
        return SALAH;
    }
    if (a->y1 >= b->y2 || b->y1 >= a->y2) {
        return SALAH;
    }
    return BENAR;
}

/*
 * gabung_wilayah - Menggabungkan dua wilayah menjadi satu
 * bounding rectangle.
 */
static void gabung_wilayah(struct wilayah_kotor *hasil,
                           const struct wilayah_kotor *a,
                           const struct wilayah_kotor *b)
{
    if (hasil == NULL || a == NULL || b == NULL) {
        return;
    }
    hasil->x1 = MIN(a->x1, b->x1);
    hasil->y1 = MIN(a->y1, b->y1);
    hasil->x2 = MAKS(a->x2, b->x2);
    hasil->y2 = MAKS(a->y2, b->y2);
    hasil->aktif = BENAR;
}

/*
 * salin_data_buffer - Menyalin data piksel dari buffer lama
 * ke buffer baru dengan klip dimensi.
 * Hanya menyalin area yang valid di kedua buffer.
 */
static void salin_data_buffer(
    struct buffer_piksel *buf_baru,
    const struct buffer_piksel *buf_lama)
{
    tak_bertanda32_t lebar_salin, tinggi_salin;
    tak_bertanda32_t baris;
    tak_bertanda32_t bpp, byte_per_baris;
    tak_bertanda32_t pitch_lama, pitch_baru;

    if (buf_baru == NULL || buf_lama == NULL) {
        return;
    }
    if (buf_baru->data == NULL || buf_lama->data == NULL) {
        return;
    }
    if (buf_baru->format != buf_lama->format) {
        return;
    }

    lebar_salin = MIN(buf_baru->lebar, buf_lama->lebar);
    tinggi_salin = MIN(buf_baru->tinggi, buf_lama->tinggi);

    if (lebar_salin == 0 || tinggi_salin == 0) {
        return;
    }

    bpp = (tak_bertanda32_t)bpp_dari_format(buf_baru->format);
    byte_per_baris = lebar_salin * bpp;
    pitch_lama = buf_lama->pitch;
    pitch_baru = buf_baru->pitch;

    for (baris = 0; baris < tinggi_salin; baris++) {
        kernel_memcpy(
            buf_baru->data + (ukuran_t)baris * pitch_baru,
            buf_lama->data + (ukuran_t)baris * pitch_lama,
            byte_per_baris);
    }
}

/* ========================================================================== */
/* FUNGSI PUBLIK: PEMBUATAN DAN PENGHAPUSAN                                   */
/* ========================================================================== */

/*
 * permukaan_fb_buat
 * -----------------
 * Membuat permukaan baru dengan buffer piksel internal.
 * Alokasi memori untuk struktur permukaan dan buffer piksel.
 *
 * Parameter:
 *   nama   - Nama identitas permukaan (boleh NULL)
 *   lebar  - Lebar permukaan dalam piksel
 *   tinggi - Tinggi permukaan dalam piksel
 *   format - Format piksel (BUF_FMT_*)
 *
 * Return: Pointer ke permukaan baru, atau NULL jika gagal
 */
struct permukaan_fb *permukaan_fb_buat(const char *nama,
                                       tak_bertanda32_t lebar,
                                       tak_bertanda32_t tinggi,
                                       tak_bertanda8_t format)
{
    struct permukaan_fb *sf;
    struct buffer_piksel *buf;
    tak_bertanda32_t pitch;
    ukuran_t ukuran_data;
    status_t st;

    /* Validasi parameter dimensi */
    st = cek_param_dimensi(lebar, tinggi, format);
    if (st != STATUS_BERHASIL) {
        return NULL;
    }

    /* Cek potensi overflow ukuran buffer */
    pitch = hitung_pitch_lokal(lebar, format);
    ukuran_data = (ukuran_t)pitch * (ukuran_t)tinggi;
    if (ukuran_data == 0) {
        return NULL;
    }

    /* Alokasi struktur permukaan */
    sf = (struct permukaan_fb *)kmalloc(
        sizeof(struct permukaan_fb));
    if (sf == NULL) {
        return NULL;
    }

    kernel_memset(sf, 0, sizeof(struct permukaan_fb));

    /* Alokasi struktur buffer piksel */
    buf = (struct buffer_piksel *)kmalloc(
        sizeof(struct buffer_piksel));
    if (buf == NULL) {
        kfree(sf);
        return NULL;
    }

    kernel_memset(buf, 0, sizeof(struct buffer_piksel));

    /* Alokasi memori piksel */
    buf->data = (tak_bertanda8_t *)kmalloc(ukuran_data);
    if (buf->data == NULL) {
        kfree(buf);
        kfree(sf);
        return NULL;
    }

    /* Inisialisasi buffer piksel */
    kernel_memset(buf->data, 0, ukuran_data);
    buf->magic = BUF_MAGIC;
    buf->versi = PERMUKAAN_VERSI;
    buf->lebar = lebar;
    buf->tinggi = tinggi;
    buf->format = format;
    buf->bit_per_piksel = (tak_bertanda8_t)(
        (tak_bertanda32_t)bpp_dari_format(format) * 8);
    buf->pitch = pitch;
    buf->ukuran = ukuran_data;
    buf->flags = 0x01;  /* ALOKASI */
    buf->referensi = 1;

    /* Inisialisasi struktur permukaan */
    sf->magic = PERMUKAAN_MAGIC;
    sf->versi = PERMUKAAN_VERSI;
    sf->buffer = buf;
    sf->id = g_id_berikutnya++;
    sf->aktif = BENAR;
    sf->terlihat = BENAR;
    sf->terkunci = SALAH;
    sf->pemilik_kunci = 0;
    sf->flags = 0;
    sf->jumlah_region_kotor = 0;

    /* Set nama */
    if (nama != NULL) {
        kernel_strncpy(sf->nama, nama, PERMUKAAN_NAMA_MAKS_LEN - 1);
        sf->nama[PERMUKAAN_NAMA_MAKS_LEN - 1] = '\0';
    } else {
        kernel_snprintf(sf->nama, PERMUKAAN_NAMA_MAKS_LEN,
                        "permukaan_%u", sf->id);
    }

    /* Set klip ke seluruh permukaan */
    sf->klip_x = 0;
    sf->klip_y = 0;
    sf->klip_lebar = lebar;
    sf->klip_tinggi = tinggi;
    sf->klip_aktif = BENAR;

    return sf;
}

/*
 * permukaan_fb_buat_dari_buffer
 * -----------------------------
 * Membuat permukaan yang membungkus buffer piksel yang sudah ada.
 * Menambah referensi buffer (buffer tidak di-free saat permukaan
 * dihapus jika masih ada referensi lain).
 *
 * Parameter:
 *   nama - Nama identitas permukaan (boleh NULL)
 *   buf  - Pointer ke buffer piksel yang sudah ada
 *
 * Return: Pointer ke permukaan baru, atau NULL jika gagal
 */
struct permukaan_fb *permukaan_fb_buat_dari_buffer(
    const char *nama,
    struct buffer_piksel *buf)
{
    struct permukaan_fb *sf;

    /* Validasi buffer */
    if (buf == NULL) {
        return NULL;
    }
    if (buf->magic != BUF_MAGIC) {
        return NULL;
    }
    if (buf->data == NULL) {
        return NULL;
    }

    /* Alokasi struktur permukaan */
    sf = (struct permukaan_fb *)kmalloc(
        sizeof(struct permukaan_fb));
    if (sf == NULL) {
        return NULL;
    }

    kernel_memset(sf, 0, sizeof(struct permukaan_fb));

    /* Inisialisasi permukaan */
    sf->magic = PERMUKAAN_MAGIC;
    sf->versi = PERMUKAAN_VERSI;
    sf->buffer = buf;
    sf->id = g_id_berikutnya++;
    sf->aktif = BENAR;
    sf->terlihat = BENAR;
    sf->terkunci = SALAH;
    sf->pemilik_kunci = 0;
    sf->flags = 0;
    sf->jumlah_region_kotor = 0;

    /* Tambah referensi buffer */
    buf->referensi++;

    /* Set nama */
    if (nama != NULL) {
        kernel_strncpy(sf->nama, nama, PERMUKAAN_NAMA_MAKS_LEN - 1);
        sf->nama[PERMUKAAN_NAMA_MAKS_LEN - 1] = '\0';
    } else {
        kernel_snprintf(sf->nama, PERMUKAAN_NAMA_MAKS_LEN,
                        "permukaan_%u", sf->id);
    }

    /* Set klip ke seluruh permukaan */
    sf->klip_x = 0;
    sf->klip_y = 0;
    sf->klip_lebar = buf->lebar;
    sf->klip_tinggi = buf->tinggi;
    sf->klip_aktif = BENAR;

    return sf;
}

/*
 * permukaan_fb_hapus
 * ------------------
 * Menghancurkan permukaan dan melepas referensi ke buffer piksel.
 * Jika buffer tidak memiliki referensi lain, memori buffer dibebaskan.
 * Permukaan yang terkunci tidak dapat dihapus.
 *
 * Parameter:
 *   sf - Pointer ke permukaan yang akan dihapus
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t permukaan_fb_hapus(struct permukaan_fb *sf)
{
    struct buffer_piksel *buf;

    if (sf == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (sf->magic != PERMUKAAN_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Permukaan terkunci tidak boleh dihapus */
    if (sf->terkunci) {
        return STATUS_BUSY;
    }

    buf = sf->buffer;

    /* Lepas referensi buffer */
    if (buf != NULL && buf->magic == BUF_MAGIC) {
        if (buf->referensi > 0) {
            buf->referensi--;
        }

        /* Free buffer jika tidak ada referensi lagi */
        if (buf->referensi == 0) {
            if ((buf->flags & 0x01) && buf->data != NULL) {
                kernel_memset(buf->data, 0, buf->ukuran);
                kfree(buf->data);
                buf->data = NULL;
            }
            buf->magic = 0;
            buf->flags = 0;
            buf->ukuran = 0;
            kfree(buf);
        }
    }

    /* Bersihkan struktur permukaan */
    kernel_memset(sf->nama, 0, PERMUKAAN_NAMA_MAKS_LEN);
    sf->jumlah_region_kotor = 0;
    sf->buffer = NULL;
    sf->aktif = SALAH;
    sf->magic = 0;
    sf->id = 0;

    kfree(sf);
    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: OPERASI UKURAN                                              */
/* ========================================================================== */

/*
 * permukaan_fb_ubah_ukuran
 * -----------------------
 * Mengubah ukuran permukaan dengan membuat buffer piksel baru.
 * Konten lama disalin ke buffer baru (klip ke dimensi minimum).
 * Permukaan harus tidak terkunci.
 *
 * Parameter:
 *   sf           - Pointer ke permukaan
 *   lebar_baru   - Lebar baru
 *   tinggi_baru  - Tinggi baru
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t permukaan_fb_ubah_ukuran(
    struct permukaan_fb *sf,
    tak_bertanda32_t lebar_baru,
    tak_bertanda32_t tinggi_baru)
{
    struct buffer_piksel *buf_baru, *buf_lama;
    tak_bertanda32_t pitch_baru;
    ukuran_t ukuran_data;
    tak_bertanda8_t format_lama;
    status_t st;

    if (sf == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (sf->magic != PERMUKAAN_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Permukaan terkunci tidak boleh diubah */
    if (sf->terkunci) {
        return STATUS_BUSY;
    }

    if (sf->buffer == NULL) {
        return STATUS_GAGAL;
    }

    /* Validasi dimensi baru */
    st = cek_param_dimensi(lebar_baru, tinggi_baru,
                           sf->buffer->format);
    if (st != STATUS_BERHASIL) {
        return st;
    }

    /* Simpan referensi ke buffer lama */
    buf_lama = sf->buffer;
    format_lama = buf_lama->format;

    /* Hitung ukuran buffer baru */
    pitch_baru = hitung_pitch_lokal(lebar_baru, format_lama);
    ukuran_data = (ukuran_t)pitch_baru * (ukuran_t)tinggi_baru;
    if (ukuran_data == 0) {
        return STATUS_PARAM_UKURAN;
    }

    /* Alokasi buffer piksel baru */
    buf_baru = (struct buffer_piksel *)kmalloc(
        sizeof(struct buffer_piksel));
    if (buf_baru == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    kernel_memset(buf_baru, 0, sizeof(struct buffer_piksel));

    /* Alokasi memori piksel baru */
    buf_baru->data = (tak_bertanda8_t *)kmalloc(ukuran_data);
    if (buf_baru->data == NULL) {
        kfree(buf_baru);
        return STATUS_MEMORI_HABIS;
    }

    /* Inisialisasi buffer baru */
    kernel_memset(buf_baru->data, 0, ukuran_data);
    buf_baru->magic = BUF_MAGIC;
    buf_baru->versi = PERMUKAAN_VERSI;
    buf_baru->lebar = lebar_baru;
    buf_baru->tinggi = tinggi_baru;
    buf_baru->format = format_lama;
    buf_baru->bit_per_piksel = (tak_bertanda8_t)(
        (tak_bertanda32_t)bpp_dari_format(format_lama) * 8);
    buf_baru->pitch = pitch_baru;
    buf_baru->ukuran = ukuran_data;
    buf_baru->flags = 0x01;  /* ALOKASI */
    buf_baru->referensi = 1;

    /* Salin data dari buffer lama (klip otomatis) */
    salin_data_buffer(buf_baru, buf_lama);

    /* Pasang buffer baru ke permukaan */
    sf->buffer = buf_baru;

    /* Reset klip ke seluruh permukaan baru */
    sf->klip_x = 0;
    sf->klip_y = 0;
    sf->klip_lebar = lebar_baru;
    sf->klip_tinggi = tinggi_baru;
    sf->klip_aktif = BENAR;

    /* Bersihkan semua region kotor (ukuran berubah) */
    sf->jumlah_region_kotor = 0;
    kernel_memset(sf->region_kotor, 0,
                  sizeof(sf->region_kotor));

    /* Lepas referensi buffer lama */
    if (buf_lama->referensi > 0) {
        buf_lama->referensi--;
    }
    if (buf_lama->referensi == 0) {
        if ((buf_lama->flags & 0x01) &&
            buf_lama->data != NULL) {
            kernel_memset(buf_lama->data, 0, buf_lama->ukuran);
            kfree(buf_lama->data);
            buf_lama->data = NULL;
        }
        buf_lama->magic = 0;
        buf_lama->flags = 0;
        buf_lama->ukuran = 0;
        kfree(buf_lama);
    }

    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: PENGUNCIAN                                                  */
/* ========================================================================== */

/*
 * permukaan_fb_kunci
 * -----------------
 * Mengunci permukaan untuk akses eksklusif. Hanya satu pemilik
 * yang dapat mengunci pada satu waktu. Permukaan yang sudah
 * terkunci oleh pemilik lain akan ditolak.
 *
 * Parameter:
 *   sf     - Pointer ke permukaan
 *   pemilik - PID pemilik kunci (0 untuk kernel)
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t permukaan_fb_kunci(struct permukaan_fb *sf,
                            tak_bertanda32_t pemilik)
{
    CEK_PTR(sf);
    if (sf->magic != PERMUKAAN_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    if (sf->terkunci) {
        if (sf->pemilik_kunci == pemilik) {
            /* Pemilik sama, sudah terkunci */
            return STATUS_SUDAH_ADA;
        }
        return STATUS_BUSY;
    }

    sf->terkunci = BENAR;
    sf->pemilik_kunci = pemilik;
    return STATUS_BERHASIL;
}

/*
 * permukaan_fb_buka_kunci
 * -----------------------
 * Membuka kunci permukaan. Hanya pemilik kunci yang dapat
 * membuka kunci. Jika PID tidak cocok, operasi ditolak.
 *
 * Parameter:
 *   sf     - Pointer ke permukaan
 *   pemilik - PID pemilik kunci
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t permukaan_fb_buka_kunci(struct permukaan_fb *sf,
                                 tak_bertanda32_t pemilik)
{
    CEK_PTR(sf);
    if (sf->magic != PERMUKAAN_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    if (!sf->terkunci) {
        return STATUS_GAGAL;
    }

    if (sf->pemilik_kunci != pemilik) {
        return STATUS_AKSES_DITOLAK;
    }

    sf->terkunci = SALAH;
    sf->pemilik_kunci = 0;
    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: AKSES BUFFER DAN DATA                                       */
/* ========================================================================== */

/*
 * permukaan_fb_dapat_buffer
 * ------------------------
 * Mendapatkan pointer ke struktur buffer piksel internal.
 * Memverifikasi keabsahan permukaan sebelum mengembalikan pointer.
 *
 * Parameter:
 *   sf - Pointer ke permukaan
 *
 * Return: Pointer ke buffer_piksel, atau NULL
 */
struct buffer_piksel *permukaan_fb_dapat_buffer(
    struct permukaan_fb *sf)
{
    if (!cek_permukaan_valid(sf)) {
        return NULL;
    }
    return sf->buffer;
}

/*
 * permukaan_fb_dapat_data
 * ----------------------
 * Mendapatkan pointer mentah ke data piksel.
 *
 * Parameter:
 *   sf - Pointer ke permukaan
 *
 * Return: Pointer ke data piksel, atau NULL
 */
tak_bertanda8_t *permukaan_fb_dapat_data(
    struct permukaan_fb *sf)
{
    if (!cek_permukaan_valid(sf)) {
        return NULL;
    }
    return sf->buffer->data;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: KLIP RECTANGLE                                              */
/* ========================================================================== */

/*
 * permukaan_fb_set_klip
 * ---------------------
 * Mengatur rectangle klip untuk membatasi area gambar.
 * Koordinat dan ukuran diklip ke batas permukaan.
 *
 * Parameter:
 *   sf - Pointer ke permukaan
 *   x  - Posisi X awal klip
 *   y  - Posisi Y awal klip
 *   w  - Lebar klip
 *   h  - Tinggi klip
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t permukaan_fb_set_klip(struct permukaan_fb *sf,
                               tak_bertanda32_t x,
                               tak_bertanda32_t y,
                               tak_bertanda32_t w,
                               tak_bertanda32_t h)
{
    tak_bertanda32_t lebar_sf, tinggi_sf;
    tak_bertanda32_t x_akhir, y_akhir;

    CEK_PTR(sf);
    if (sf->magic != PERMUKAAN_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (sf->buffer == NULL) {
        return STATUS_GAGAL;
    }

    if (w == 0 || h == 0) {
        return STATUS_PARAM_UKURAN;
    }

    lebar_sf = sf->buffer->lebar;
    tinggi_sf = sf->buffer->tinggi;

    /* Klip koordinat ke batas permukaan */
    x = BATAS(x, 0, lebar_sf);
    y = BATAS(y, 0, tinggi_sf);

    x_akhir = x + w;
    y_akhir = y + h;

    if (x_akhir > lebar_sf) {
        x_akhir = lebar_sf;
    }
    if (y_akhir > tinggi_sf) {
        y_akhir = tinggi_sf;
    }

    sf->klip_x = x;
    sf->klip_y = y;
    sf->klip_lebar = x_akhir - x;
    sf->klip_tinggi = y_akhir - y;
    sf->klip_aktif = BENAR;

    return STATUS_BERHASIL;
}

/*
 * permukaan_fb_hapus_klip
 * -----------------------
 * Mengatur ulang rectangle klip ke seluruh permukaan.
 *
 * Parameter:
 *   sf - Pointer ke permukaan
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t permukaan_fb_hapus_klip(struct permukaan_fb *sf)
{
    CEK_PTR(sf);
    if (sf->magic != PERMUKAAN_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (sf->buffer == NULL) {
        return STATUS_GAGAL;
    }

    sf->klip_x = 0;
    sf->klip_y = 0;
    sf->klip_lebar = sf->buffer->lebar;
    sf->klip_tinggi = sf->buffer->tinggi;
    sf->klip_aktif = BENAR;

    return STATUS_BERHASIL;
}

/*
 * permukaan_fb_dapat_klip
 * ----------------------
 * Mendapatkan rectangle klip saat ini melalui pointer output.
 *
 * Parameter:
 *   sf - Pointer ke permukaan
 *   x  - Pointer output posisi X
 *   y  - Pointer output posisi Y
 *   w  - Pointer output lebar
 *   h  - Pointer output tinggi
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t permukaan_fb_dapat_klip(struct permukaan_fb *sf,
                                 tak_bertanda32_t *x,
                                 tak_bertanda32_t *y,
                                 tak_bertanda32_t *w,
                                 tak_bertanda32_t *h)
{
    CEK_PTR(sf);
    if (sf->magic != PERMUKAAN_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Minimal satu pointer output harus valid */
    if (x == NULL && y == NULL && w == NULL && h == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (x != NULL) { *x = sf->klip_x; }
    if (y != NULL) { *y = sf->klip_y; }
    if (w != NULL) { *w = sf->klip_lebar; }
    if (h != NULL) { *h = sf->klip_tinggi; }

    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: REGION KOTOR                                                */
/* ========================================================================== */

/*
 * permukaan_fb_tambah_region_kotor
 * -------------------------------
 * Menambahkan region kotor ke daftar tracking.
 * Jika region baru tumpang tindih dengan region yang sudah ada,
 * kedua region digabungkan menjadi satu.
 * Jika daftar sudah penuh, region terluas digabungkan ke region
 * terakhir.
 *
 * Parameter:
 *   sf - Pointer ke permukaan
 *   x  - Posisi X awal region
 *   y  - Posisi Y awal region
 *   w  - Lebar region
 *   h  - Tinggi region
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t permukaan_fb_tambah_region_kotor(
    struct permukaan_fb *sf,
    tak_bertanda32_t x,
    tak_bertanda32_t y,
    tak_bertanda32_t w,
    tak_bertanda32_t h)
{
    struct wilayah_kotor baru;
    tak_bertanda32_t i;
    tak_bertanda32_t lebar_sf, tinggi_sf;
    tak_bertanda32_t x_akhir, y_akhir;

    if (sf == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (sf->magic != PERMUKAAN_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (sf->buffer == NULL) {
        return STATUS_GAGAL;
    }

    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }

    lebar_sf = sf->buffer->lebar;
    tinggi_sf = sf->buffer->tinggi;

    /* Klip region ke batas permukaan */
    x = BATAS(x, 0, lebar_sf);
    y = BATAS(y, 0, tinggi_sf);
    x_akhir = x + w;
    y_akhir = y + h;
    if (x_akhir > lebar_sf) { x_akhir = lebar_sf; }
    if (y_akhir > tinggi_sf) { y_akhir = tinggi_sf; }

    if (x_akhir <= x || y_akhir <= y) {
        return STATUS_PARAM_JARAK;
    }

    /* Bentuk wilayah baru */
    baru.x1 = x;
    baru.y1 = y;
    baru.x2 = x_akhir;
    baru.y2 = y_akhir;
    baru.aktif = BENAR;

    /* Cek tumpang tindih dengan region yang sudah ada */
    for (i = 0; i < sf->jumlah_region_kotor; i++) {
        if (wilayah_tumpang_tindih(&baru,
            &sf->region_kotor[i])) {
            gabung_wilayah(&baru, &baru,
                           &sf->region_kotor[i]);
            /* Hapus region lama */
            sf->region_kotor[i].aktif = SALAH;
        }
    }

    /* Kompaksi array: geser region aktif ke depan */
    {
        tak_bertanda32_t tulis = 0;
        for (i = 0; i < sf->jumlah_region_kotor; i++) {
            if (sf->region_kotor[i].aktif) {
                if (tulis != i) {
                    kernel_memcpy(
                        &sf->region_kotor[tulis],
                        &sf->region_kotor[i],
                        sizeof(struct wilayah_kotor));
                }
                tulis++;
            }
        }
        sf->jumlah_region_kotor = tulis;
    }

    /* Cek apakah masih ada slot kosong */
    if (sf->jumlah_region_kotor <
        PERMUKAAN_MAKS_REGION_KOTOR) {
        kernel_memcpy(
            &sf->region_kotor[sf->jumlah_region_kotor],
            &baru,
            sizeof(struct wilayah_kotor));
        sf->jumlah_region_kotor++;
    } else {
        /* Slot penuh: gabungkan ke region terakhir */
        gabung_wilayah(
            &sf->region_kotor[sf->jumlah_region_kotor - 1],
            &sf->region_kotor[sf->jumlah_region_kotor - 1],
            &baru);
    }

    /* Tandai buffer sebagai kotor */
    if (sf->buffer != NULL) {
        sf->buffer->flags |= 0x08;  /* KOTOR */
    }

    return STATUS_BERHASIL;
}

/*
 * permukaan_fb_bersihkan_region_kotor
 * ----------------------------------
 * Menghapus semua region kotor dari daftar tracking.
 *
 * Parameter:
 *   sf - Pointer ke permukaan
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t permukaan_fb_bersihkan_region_kotor(
    struct permukaan_fb *sf)
{
    CEK_PTR(sf);
    if (sf->magic != PERMUKAAN_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    kernel_memset(sf->region_kotor, 0,
                  sizeof(sf->region_kotor));
    sf->jumlah_region_kotor = 0;

    /* Tandai buffer bersih */
    if (sf->buffer != NULL && sf->buffer->magic == BUF_MAGIC) {
        sf->buffer->flags &= (tak_bertanda32_t)(~0x08);
    }

    return STATUS_BERHASIL;
}

/*
 * permukaan_fb_apakah_kotor
 * -------------------------
 * Memeriksa apakah permukaan memiliki region kotor.
 *
 * Parameter:
 *   sf - Pointer ke permukaan
 *
 * Return: BENAR jika ada region kotor
 */
bool_t permukaan_fb_apakah_kotor(
    const struct permukaan_fb *sf)
{
    if (sf == NULL) {
        return SALAH;
    }
    if (sf->magic != PERMUKAAN_MAGIC) {
        return SALAH;
    }
    return (sf->jumlah_region_kotor > 0) ? BENAR : SALAH;
}

/*
 * permukaan_fb_dapat_region_kotor
 * ------------------------------
 * Mendapatkan region kotor pada indeks tertentu.
 *
 * Parameter:
 *   sf    - Pointer ke permukaan
 *   index - Indeks region
 *   x     - Pointer output posisi X
 *   y     - Pointer output posisi Y
 *   w     - Pointer output lebar
 *   h     - Pointer output tinggi
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t permukaan_fb_dapat_region_kotor(
    struct permukaan_fb *sf,
    tak_bertanda32_t index,
    tak_bertanda32_t *x,
    tak_bertanda32_t *y,
    tak_bertanda32_t *w,
    tak_bertanda32_t *h)
{
    const struct wilayah_kotor *wk;

    if (sf == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (sf->magic != PERMUKAAN_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    if (index >= sf->jumlah_region_kotor) {
        return STATUS_PARAM_JARAK;
    }

    wk = &sf->region_kotor[index];
    if (!wk->aktif) {
        return STATUS_GAGAL;
    }

    if (x != NULL) { *x = wk->x1; }
    if (y != NULL) { *y = wk->y1; }
    if (w != NULL) { *w = wk->x2 - wk->x1; }
    if (h != NULL) { *h = wk->y2 - wk->y1; }

    return STATUS_BERHASIL;
}

/*
 * permukaan_fb_dapat_jumlah_region_kotor
 * -------------------------------------
 * Mendapatkan jumlah region kotor aktif.
 *
 * Parameter:
 *   sf - Pointer ke permukaan
 *
 * Return: Jumlah region kotor
 */
tak_bertanda32_t permukaan_fb_dapat_jumlah_region_kotor(
    const struct permukaan_fb *sf)
{
    if (sf == NULL) {
        return 0;
    }
    if (sf->magic != PERMUKAAN_MAGIC) {
        return 0;
    }
    return sf->jumlah_region_kotor;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: VISIBILITAS                                                 */
/* ========================================================================== */

/*
 * permukaan_fb_tandai_terlihat
 * ---------------------------
 * Mengatur flag visibilitas permukaan.
 *
 * Parameter:
 *   sf       - Pointer ke permukaan
 *   terlihat - BENAR untuk terlihat, SALAH untuk tersembunyi
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t permukaan_fb_tandai_terlihat(
    struct permukaan_fb *sf,
    bool_t terlihat)
{
    CEK_PTR(sf);
    if (sf->magic != PERMUKAAN_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    sf->terlihat = terlihat;
    return STATUS_BERHASIL;
}

/*
 * permukaan_fb_apakah_terlihat
 * ---------------------------
 * Memeriksa apakah permukaan ditandai terlihat.
 *
 * Parameter:
 *   sf - Pointer ke permukaan
 *
 * Return: BENAR jika terlihat
 */
bool_t permukaan_fb_apakah_terlihat(
    const struct permukaan_fb *sf)
{
    if (sf == NULL) {
        return SALAH;
    }
    if (sf->magic != PERMUKAAN_MAGIC) {
        return SALAH;
    }
    return sf->terlihat;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: IDENTITAS                                                   */
/* ========================================================================== */

/*
 * permukaan_fb_set_nama
 * ---------------------
 * Mengatur nama identitas permukaan.
 * Nama dipotong jika melebihi batas maksimum.
 *
 * Parameter:
 *   sf   - Pointer ke permukaan
 *   nama - String nama baru (boleh NULL)
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t permukaan_fb_set_nama(struct permukaan_fb *sf,
                               const char *nama)
{
    CEK_PTR(sf);
    if (sf->magic != PERMUKAAN_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    if (nama != NULL) {
        kernel_strncpy(sf->nama, nama,
                       PERMUKAAN_NAMA_MAKS_LEN - 1);
        sf->nama[PERMUKAAN_NAMA_MAKS_LEN - 1] = '\0';
    } else {
        kernel_snprintf(sf->nama, PERMUKAAN_NAMA_MAKS_LEN,
                        "permukaan_%u", sf->id);
    }

    return STATUS_BERHASIL;
}

/*
 * permukaan_fb_dapat_nama
 * ----------------------
 * Mendapatkan nama identitas permukaan.
 *
 * Parameter:
 *   sf - Pointer ke permukaan
 *
 * Return: Pointer ke string nama, atau NULL
 */
const char *permukaan_fb_dapat_nama(
    const struct permukaan_fb *sf)
{
    if (sf == NULL) {
        return NULL;
    }
    if (sf->magic != PERMUKAAN_MAGIC) {
        return NULL;
    }
    return (const char *)sf->nama;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: AKSES DIMENSI                                               */
/* ========================================================================== */

/*
 * permukaan_fb_dapat_lebar
 * -----------------------
 * Mendapatkan lebar permukaan dalam piksel.
 *
 * Parameter:
 *   sf - Pointer ke permukaan
 *
 * Return: Lebar dalam piksel, atau 0 jika tidak valid
 */
tak_bertanda32_t permukaan_fb_dapat_lebar(
    const struct permukaan_fb *sf)
{
    if (!cek_permukaan_valid(sf)) {
        return 0;
    }
    return sf->buffer->lebar;
}

/*
 * permukaan_fb_dapat_tinggi
 * ------------------------
 * Mendapatkan tinggi permukaan dalam piksel.
 *
 * Parameter:
 *   sf - Pointer ke permukaan
 *
 * Return: Tinggi dalam piksel, atau 0 jika tidak valid
 */
tak_bertanda32_t permukaan_fb_dapat_tinggi(
    const struct permukaan_fb *sf)
{
    if (!cek_permukaan_valid(sf)) {
        return 0;
    }
    return sf->buffer->tinggi;
}

/*
 * permukaan_fb_dapat_format
 * ------------------------
 * Mendapatkan format piksel permukaan.
 *
 * Parameter:
 *   sf - Pointer ke permukaan
 *
 * Return: Kode format, atau BUF_FMT_INVALID
 */
tak_bertanda8_t permukaan_fb_dapat_format(
    const struct permukaan_fb *sf)
{
    if (!cek_permukaan_valid(sf)) {
        return BUF_FMT_INVALID;
    }
    return sf->buffer->format;
}

/*
 * permukaan_fb_dapat_pitch
 * -----------------------
 * Mendapatkan pitch (byte per baris) permukaan.
 *
 * Parameter:
 *   sf - Pointer ke permukaan
 *
 * Return: Pitch dalam byte, atau 0 jika tidak valid
 */
tak_bertanda32_t permukaan_fb_dapat_pitch(
    const struct permukaan_fb *sf)
{
    if (!cek_permukaan_valid(sf)) {
        return 0;
    }
    return sf->buffer->pitch;
}

/*
 * permukaan_fb_dapat_id
 * --------------------
 * Mendapatkan ID unik permukaan.
 *
 * Parameter:
 *   sf - Pointer ke permukaan
 *
 * Return: ID unik, atau 0 jika tidak valid
 */
tak_bertanda32_t permukaan_fb_dapat_id(
    const struct permukaan_fb *sf)
{
    if (sf == NULL) {
        return 0;
    }
    if (sf->magic != PERMUKAAN_MAGIC) {
        return 0;
    }
    return sf->id;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: VALIDASI                                                    */
/* ========================================================================== */

/*
 * permukaan_fb_validasi
 * ---------------------
 * Memvalidasi integritas permukaan secara menyeluruh.
 * Memeriksa magic, versi, buffer, dimensi, format, pitch,
 * dan konsistensi data internal.
 *
 * Parameter:
 *   sf - Pointer ke permukaan
 *
 * Return: BENAR jika permukaan valid
 */
bool_t permukaan_fb_validasi(const struct permukaan_fb *sf)
{
    tak_bertanda32_t pitch_diharapkan;

    if (sf == NULL) {
        return SALAH;
    }

    /* Cek magic permukaan */
    if (sf->magic != PERMUKAAN_MAGIC) {
        return SALAH;
    }

    /* Cek versi */
    if (sf->versi != PERMUKAAN_VERSI) {
        return SALAH;
    }

    /* Cek keberadaan buffer */
    if (sf->buffer == NULL) {
        return SALAH;
    }

    /* Cek magic buffer */
    if (sf->buffer->magic != BUF_MAGIC) {
        return SALAH;
    }

    /* Cek dimensi buffer */
    if (sf->buffer->lebar == 0 || sf->buffer->tinggi == 0) {
        return SALAH;
    }

    /* Cek format buffer */
    if (!format_valid(sf->buffer->format)) {
        return SALAH;
    }

    /* Cek keberadaan data piksel */
    if (sf->buffer->data == NULL) {
        return SALAH;
    }

    /* Cek referensi buffer minimal 1 */
    if (sf->buffer->referensi == 0) {
        return SALAH;
    }

    /* Verifikasi konsistensi pitch */
    pitch_diharapkan = hitung_pitch_lokal(
        sf->buffer->lebar, sf->buffer->format);
    if (sf->buffer->pitch != pitch_diharapkan) {
        return SALAH;
    }

    /* Verifikasi konsistensi ukuran buffer */
    if (sf->buffer->ukuran !=
        (ukuran_t)sf->buffer->pitch *
        (ukuran_t)sf->buffer->tinggi) {
        return SALAH;
    }

    /* Verifikasi klip dalam batas permukaan */
    if (sf->klip_aktif) {
        if (sf->klip_x + sf->klip_lebar >
            sf->buffer->lebar) {
            return SALAH;
        }
        if (sf->klip_y + sf->klip_tinggi >
            sf->buffer->tinggi) {
            return SALAH;
        }
    }

    /* Verifikasi jumlah region kotor */
    if (sf->jumlah_region_kotor >
        ARRAY_UKURAN(sf->region_kotor)) {
        return SALAH;
    }

    return BENAR;
}
