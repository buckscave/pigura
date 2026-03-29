/*
 * PIGURA OS - BUFFERBELAKANG.C
 * ============================
 * Manajemen buffer belakang (double buffering).
 *
 * Modul ini menyediakan sistem double buffering untuk subsistem
 * grafis LibPigura. Buffer belakang memungkinkan penggambaran
 * di latar belakang sementara buffer depan menampilkan konten
 * yang stabil, menghindari flicker pada tampilan.
 *
 * Double buffering bekerja dengan mempertahankan dua salinan
 * buffer piksel:
 *   - Buffer depan  : ditampilkan ke layar (visible)
 *   - Buffer belakang: tempat menggambar (drawing)
 *
 * Setelah penggambaran selesai, kedua buffer ditukar (swap)
 * sehingga hasil gambar baru muncul di buffer depan.
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
/* KONSTANTA FORMAT PIXEL (sama dengan buffer.c)                              */
/* ========================================================================== */

#define BUFFER_FMT_INVALID     0
#define BUFFER_FMT_RGB332      1
#define BUFFER_FMT_RGB565      2
#define BUFFER_FMT_RGB888      3
#define BUFFER_FMT_ARGB8888    4
#define BUFFER_FMT_XRGB8888    5
#define BUFFER_FMT_BGRA8888    6
#define BUFFER_FMT_BGR565      7
#define BUFFER_FMT_MAKS        7

#define BUFFER_MAGIC           0x42554642  /* "BUFB" */

/* ========================================================================== */
/* KONSTANTA BUFFER BELAKANG                                                  */
/* ========================================================================== */

#define BB_MAGIC               0x42424C42  /* "BBLB" */
#define BB_VERSI               0x0100
#define BB_DIMENSI_MAKS        16384

/* Flag keadaan buffer belakang */
#define BB_FLAG_AKTIF          0x01
#define BB_FLAG_TERKUNCI       0x02
#define BB_FLAG_VSYNC          0x04
#define BB_FLAG_KOTOR          0x08

/* ========================================================================== */
/* MAKRO HELPER                                                               */
/* ========================================================================== */

/*
 * bpp_dari_format - Mendapatkan byte per piksel dari format.
 * Nested ternary yang kompatibel C90.
 */
#define bb_bpp_dari_format(fmt) \
    (((fmt) == BUFFER_FMT_RGB332) ? 1 : \
     (((fmt) == BUFFER_FMT_RGB565) || \
      ((fmt) == BUFFER_FMT_BGR565)) ? 2 : \
     ((fmt) == BUFFER_FMT_RGB888) ? 3 : 4)

/*
 * bb_hitung_pitch - Menghitung pitch (byte per baris).
 * Pitch selalu dibulatkan ke atas ke kelipatan 4 byte.
 */
#define bb_hitung_pitch(lebar, fmt) \
    RATAKAN_ATAS((lebar) * bb_bpp_dari_format(fmt), 4)

/*
 * bb_hitung_ukuran - Menghitung total ukuran buffer dalam byte.
 */
#define bb_hitung_ukuran(lebar, tinggi, fmt) \
    ((ukuran_t)bb_hitung_pitch(lebar, fmt) * (ukuran_t)(tinggi))

/* ========================================================================== */
/* STRUKTUR DATA                                                              */
/* ========================================================================== */

/*
 * struct buffer_piksel - Didefinisikan ulang agar modul ini
 * mandiri tanpa bergantung pada buffer.h.
 * Harus identik dengan definisi di buffer.c.
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
 * struct buffer_belakang
 * ----------------------
 * Struktur utama manajemen double buffering.
 * Menyimpan dua pointer buffer_piksel (depan dan belakang)
 * serta metadata operasi swap dan keadaan buffer.
 */
struct buffer_belakang {
    tak_bertanda32_t       magic;       /* Magic validasi      */
    tak_bertanda16_t       versi;       /* Versi struktur      */
    struct buffer_piksel  *depan;       /* Buffer depan (visi) */
    struct buffer_piksel  *belakang;    /* Buffer belakang     */
    tak_bertanda32_t       flags;       /* Flag keadaan        */
    tak_bertanda32_t       jumlah_tukar;/* Jumlah swap         */
};

/* ========================================================================== */
/* VARIABEL GLOBAL                                                            */
/* ========================================================================== */

/* Jumlah buffer belakang yang sedang aktif */
static tak_bertanda32_t g_bb_aktif = 0;

/* ========================================================================== */
/* FUNGSI HELPER INTERNAL                                                     */
/* ========================================================================== */

/*
 * bb_buat_buffer_internal
 * ----------------------
 * Membuat satu buffer piksel dengan parameter yang diberikan.
 * Mengalokasikan memori untuk struktur dan data piksel.
 *
 * Parameter:
 *   lebar  - Lebar buffer dalam piksel
 *   tinggi - Tinggi buffer dalam piksel
 *   format - Format piksel (lihat BUFFER_FMT_*)
 *
 * Return: Pointer ke buffer_piksel, atau NULL jika gagal
 */
static struct buffer_piksel *bb_buat_buffer_internal(
    tak_bertanda32_t lebar,
    tak_bertanda32_t tinggi,
    tak_bertanda8_t format)
{
    struct buffer_piksel *buf;

    if (format == BUFFER_FMT_INVALID || format > BUFFER_FMT_MAKS) {
        return NULL;
    }
    if (lebar == 0 || tinggi == 0) {
        return NULL;
    }
    if (lebar > BB_DIMENSI_MAKS || tinggi > BB_DIMENSI_MAKS) {
        return NULL;
    }

    /* Cek overflow pada perhitungan ukuran */
    {
        tak_bertanda64_t uk64 = (tak_bertanda64_t)
            bb_hitung_pitch(lebar, format) *
            (tak_bertanda64_t)tinggi;
#if defined(PIGURA_ARSITEKTUR_64BIT)
        if (uk64 > (tak_bertanda64_t)(-1)) {
            return NULL;
        }
#else
        if (uk64 > (tak_bertanda64_t)TAK_BERTANDA32_MAX) {
            return NULL;
        }
#endif
    }

    buf = (struct buffer_piksel *)kmalloc(
        sizeof(struct buffer_piksel));
    if (buf == NULL) {
        return NULL;
    }

    kernel_memset(buf, 0, sizeof(struct buffer_piksel));

    buf->magic         = BUFFER_MAGIC;
    buf->versi         = BB_VERSI;
    buf->lebar         = lebar;
    buf->tinggi        = tinggi;
    buf->format        = format;
    buf->bit_per_piksel = (tak_bertanda8_t)(
        bb_bpp_dari_format(format) * 8);
    buf->pitch         = bb_hitung_pitch(lebar, format);
    buf->ukuran        = bb_hitung_ukuran(
                             lebar, tinggi, format);

    buf->data = (tak_bertanda8_t *)kmalloc(buf->ukuran);
    if (buf->data == NULL) {
        kernel_memset(buf, 0, sizeof(struct buffer_piksel));
        kfree(buf);
        return NULL;
    }

    kernel_memset(buf->data, 0, buf->ukuran);
    buf->flags     = 0x01;  /* BUFFER_FLAG_ALOKASI */
    buf->referensi = 1;

    return buf;
}

/*
 * bb_hapus_buffer_internal
 * -----------------------
 * Menghapus satu buffer piksel yang dibuat secara internal.
 * Membebaskan data piksel dan struktur buffer itu sendiri.
 *
 * Parameter:
 *   buf - Pointer ke buffer yang akan dihapus
 */
static void bb_hapus_buffer_internal(struct buffer_piksel *buf)
{
    if (buf == NULL) {
        return;
    }
    if (buf->magic != BUFFER_MAGIC) {
        return;
    }

    if (buf->data != NULL) {
        kernel_memset(buf->data, 0, buf->ukuran);
        kfree(buf->data);
        buf->data = NULL;
    }

    buf->magic  = 0;
    buf->flags  = 0;
    buf->ukuran = 0;
    kfree(buf);
}

/*
 * bb_validasi_buffer
 * -----------------
 * Memvalidasi integritas satu buffer piksel.
 *
 * Parameter:
 *   buf - Pointer ke buffer yang akan divalidasi
 *
 * Return: BENAR jika valid, SALAH jika tidak
 */
static bool_t bb_validasi_buffer(const struct buffer_piksel *buf)
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
    if (buf->pitch != bb_hitung_pitch(buf->lebar,
                                       buf->format)) {
        return SALAH;
    }
    if (buf->ukuran != bb_hitung_ukuran(
            buf->lebar, buf->tinggi, buf->format)) {
        return SALAH;
    }
    return BENAR;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: PEMBUATAN DAN PENGHAPUSAN                                   */
/* ========================================================================== */

/*
 * buffer_belakang_buat
 * -------------------
 * Membuat pasangan buffer double buffering.
 *
 * Mengalokasikan dua buffer piksel berukuran sama untuk
 * buffer depan dan belakang. Kedua buffer diinisialisasi
 * dengan piksel hitam (semua nol).
 *
 * Parameter:
 *   lebar  - Lebar buffer dalam piksel
 *   tinggi - Tinggi buffer dalam piksel
 *   format - Format piksel (BUFFER_FMT_*)
 *
 * Return: Pointer ke struct buffer_belakang, atau NULL
 *         jika gagal mengalokasikan memori
 */
struct buffer_belakang *buffer_belakang_buat(
    tak_bertanda32_t lebar,
    tak_bertanda32_t tinggi,
    tak_bertanda8_t format)
{
    struct buffer_belakang *bb;

    if (format == BUFFER_FMT_INVALID || format > BUFFER_FMT_MAKS) {
        return NULL;
    }
    if (lebar == 0 || tinggi == 0) {
        return NULL;
    }
    if (lebar > BB_DIMENSI_MAKS || tinggi > BB_DIMENSI_MAKS) {
        return NULL;
    }

    bb = (struct buffer_belakang *)kmalloc(
        sizeof(struct buffer_belakang));
    if (bb == NULL) {
        return NULL;
    }

    kernel_memset(bb, 0, sizeof(struct buffer_belakang));

    /* Buat buffer belakang (buffer menggambar) */
    bb->belakang = bb_buat_buffer_internal(
        lebar, tinggi, format);
    if (bb->belakang == NULL) {
        kernel_memset(bb, 0, sizeof(struct buffer_belakang));
        kfree(bb);
        return NULL;
    }

    /* Buat buffer depan (buffer tampilan) */
    bb->depan = bb_buat_buffer_internal(
        lebar, tinggi, format);
    if (bb->depan == NULL) {
        bb_hapus_buffer_internal(bb->belakang);
        bb->belakang = NULL;
        kernel_memset(bb, 0, sizeof(struct buffer_belakang));
        kfree(bb);
        return NULL;
    }

    bb->magic       = BB_MAGIC;
    bb->versi       = BB_VERSI;
    bb->flags       = BB_FLAG_AKTIF;
    bb->jumlah_tukar = 0;
    g_bb_aktif++;

    return bb;
}

/*
 * buffer_belakang_hapus
 * --------------------
 * Menghancurkan pasangan buffer double buffering.
 *
 * Membebaskan buffer depan, buffer belakang, dan struktur
 * buffer_belakang itu sendiri. Jika buffer terkunci,
 * operasi akan ditolak dengan STATUS_BUSY.
 *
 * Parameter:
 *   bb - Pointer ke struct buffer_belakang
 *
 * Return: STATUS_BERHASIL jika berhasil
 *         STATUS_PARAM_NULL jika bb NULL
 *         STATUS_PARAM_INVALID jika magic tidak cocok
 *         STATUS_BUSY jika buffer terkunci
 */
status_t buffer_belakang_hapus(struct buffer_belakang *bb)
{
    if (bb == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (bb->magic != BB_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (bb->flags & BB_FLAG_TERKUNCI) {
        return STATUS_BUSY;
    }

    /* Hapus buffer belakang */
    if (bb->belakang != NULL) {
        bb_hapus_buffer_internal(bb->belakang);
        bb->belakang = NULL;
    }

    /* Hapus buffer depan */
    if (bb->depan != NULL) {
        bb_hapus_buffer_internal(bb->depan);
        bb->depan = NULL;
    }

    bb->magic       = 0;
    bb->flags       = 0;
    bb->jumlah_tukar = 0;
    g_bb_aktif--;

    kernel_memset(bb, 0, sizeof(struct buffer_belakang));
    kfree(bb);

    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: OPERASI SWAP DAN SALIN                                      */
/* ========================================================================== */

/*
 * buffer_belakang_tukar
 * --------------------
 * Menukar pointer buffer depan dan belakang.
 *
 * Operasi ini hanya menukar pointer, bukan menyalin data,
 * sehingga sangat efisien. Setelah swap, buffer yang tadinya
 * menjadi buffer belakang (tempat menggambar) sekarang menjadi
 * buffer depan (yang ditampilkan), dan sebaliknya.
 *
 * Buffer belakang akan ditandai kotor setelah swap karena
 * buffer depan yang baru berisi data lama dari belakang.
 *
 * Parameter:
 *   bb - Pointer ke struct buffer_belakang
 *
 * Return: STATUS_BERHASIL jika berhasil
 *         STATUS_PARAM_NULL jika bb NULL
 *         STATUS_PARAM_INVALID jika magic tidak cocok
 *         STATUS_BUSY jika buffer terkunci
 */
status_t buffer_belakang_tukar(struct buffer_belakang *bb)
{
    struct buffer_piksel *tmp;

    if (bb == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (bb->magic != BB_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (bb->flags & BB_FLAG_TERKUNCI) {
        return STATUS_BUSY;
    }
    if (bb->depan == NULL || bb->belakang == NULL) {
        return STATUS_GAGAL;
    }

    /* Tukar pointer depan dan belakang */
    tmp = bb->depan;
    bb->depan = bb->belakang;
    bb->belakang = tmp;

    /* Tandai buffer belakang yang baru sebagai kotor */
    bb->belakang->flags |= 0x08; /* BUFFER_FLAG_KOTOR */

    bb->jumlah_tukar++;

    return STATUS_BERHASIL;
}

/*
 * buffer_belakang_salin_ke_depan
 * -----------------------------
 * Menyalin isi buffer belakang ke buffer depan.
 *
 * Digunakan ketika swap pointer tidak memungkinkan, misalnya
 * pada hardware yang memetakan buffer depan langsung ke
 * memori tampilan (framebuffer fisik).
 *
 * Parameter:
 *   bb - Pointer ke struct buffer_belakang
 *
 * Return: STATUS_BERHASIL jika berhasil
 *         STATUS_PARAM_NULL jika bb NULL
 *         STATUS_PARAM_INVALID jika magic tidak cocok
 *         STATUS_BUSY jika buffer terkunci
 *         STATUS_GAGAL jika buffer depan/belakang NULL
 *         STATUS_PARAM_UKURAN jika ukuran tidak cocok
 */
status_t buffer_belakang_salin_ke_depan(
    struct buffer_belakang *bb)
{
    if (bb == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (bb->magic != BB_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (bb->flags & BB_FLAG_TERKUNCI) {
        return STATUS_BUSY;
    }
    if (bb->depan == NULL || bb->belakang == NULL) {
        return STATUS_GAGAL;
    }
    if (bb->depan->data == NULL || bb->belakang->data == NULL) {
        return STATUS_GAGAL;
    }

    /* Validasi kesesuaian dimensi dan format */
    if (bb->depan->lebar != bb->belakang->lebar ||
        bb->depan->tinggi != bb->belakang->tinggi ||
        bb->depan->format != bb->belakang->format) {
        return STATUS_PARAM_UKURAN;
    }

    kernel_memcpy(bb->depan->data, bb->belakang->data,
                  MIN(bb->depan->ukuran, bb->belakang->ukuran));

    bb->depan->flags |= 0x08; /* BUFFER_FLAG_KOTOR */
    bb->jumlah_tukar++;

    return STATUS_BERHASIL;
}

/*
 * buffer_belakang_salin_dari_depan
 * -------------------------------
 * Menyalin isi buffer depan ke buffer belakang.
 *
 * Digunakan untuk mengambil snapshot keadaan tampilan
 * saat ini ke buffer belakang, misalnya sebelum mulai
 * menggambar ulang.
 *
 * Parameter:
 *   bb - Pointer ke struct buffer_belakang
 *
 * Return: STATUS_BERHASIL jika berhasil
 *         STATUS_PARAM_NULL jika bb NULL
 *         STATUS_PARAM_INVALID jika magic tidak cocok
 *         STATUS_BUSY jika buffer terkunci
 *         STATUS_GAGAL jika buffer depan/belakang NULL
 *         STATUS_PARAM_UKURAN jika ukuran tidak cocok
 */
status_t buffer_belakang_salin_dari_depan(
    struct buffer_belakang *bb)
{
    if (bb == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (bb->magic != BB_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (bb->flags & BB_FLAG_TERKUNCI) {
        return STATUS_BUSY;
    }
    if (bb->depan == NULL || bb->belakang == NULL) {
        return STATUS_GAGAL;
    }
    if (bb->depan->data == NULL || bb->belakang->data == NULL) {
        return STATUS_GAGAL;
    }

    /* Validasi kesesuaian dimensi dan format */
    if (bb->depan->lebar != bb->belakang->lebar ||
        bb->depan->tinggi != bb->belakang->tinggi ||
        bb->depan->format != bb->belakang->format) {
        return STATUS_PARAM_UKURAN;
    }

    kernel_memcpy(bb->belakang->data, bb->depan->data,
                  MIN(bb->depan->ukuran, bb->belakang->ukuran));

    bb->belakang->flags |= 0x08; /* BUFFER_FLAG_KOTOR */

    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: AKSES BUFFER                                                */
/* ========================================================================== */

/*
 * buffer_belakang_dapat_depan
 * --------------------------
 * Mendapatkan pointer ke buffer depan (visible).
 *
 * Parameter:
 *   bb - Pointer ke struct buffer_belakang
 *
 * Return: Pointer ke buffer_piksel depan, atau NULL
 */
struct buffer_piksel *buffer_belakang_dapat_depan(
    struct buffer_belakang *bb)
{
    if (bb == NULL || bb->magic != BB_MAGIC) {
        return NULL;
    }
    return bb->depan;
}

/*
 * buffer_belakang_dapat_belakang
 * -----------------------------
 * Mendapatkan pointer ke buffer belakang (drawing).
 *
 * Parameter:
 *   bb - Pointer ke struct buffer_belakang
 *
 * Return: Pointer ke buffer_piksel belakang, atau NULL
 */
struct buffer_piksel *buffer_belakang_dapat_belakang(
    struct buffer_belakang *bb)
{
    if (bb == NULL || bb->magic != BB_MAGIC) {
        return NULL;
    }
    return bb->belakang;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: PENGUNCIAN                                                  */
/* ========================================================================== */

/*
 * buffer_belakang_kunci
 * --------------------
 * Mengunci buffer belakang untuk akses eksklusif.
 *
 * Mencegah operasi swap dan salin selama buffer sedang
 * digunakan oleh satu pemanggil. Operasi swap dan salin
 * akan gagal dengan STATUS_BUSY selama buffer terkunci.
 *
 * Parameter:
 *   bb - Pointer ke struct buffer_belakang
 *
 * Return: STATUS_BERHASIL jika berhasil
 *         STATUS_PARAM_NULL jika bb NULL
 *         STATUS_PARAM_INVALID jika magic tidak cocok
 *         STATUS_BUSY jika sudah terkunci
 */
status_t buffer_belakang_kunci(struct buffer_belakang *bb)
{
    if (bb == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (bb->magic != BB_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (bb->flags & BB_FLAG_TERKUNCI) {
        return STATUS_BUSY;
    }

    bb->flags |= BB_FLAG_TERKUNCI;

    return STATUS_BERHASIL;
}

/*
 * buffer_belakang_buka_kunci
 * -------------------------
 * Membuka kunci buffer belakang.
 *
 * Mengizinkan operasi swap dan salin kembali setelah
 * penguncian. Jika buffer tidak dalam keadaan terkunci,
 * fungsi ini tidak melakukan apa-apa dan mengembalikan
 * STATUS_BERHASIL (idempotent).
 *
 * Parameter:
 *   bb - Pointer ke struct buffer_belakang
 *
 * Return: STATUS_BERHASIL jika berhasil
 *         STATUS_PARAM_NULL jika bb NULL
 *         STATUS_PARAM_INVALID jika magic tidak cocok
 */
status_t buffer_belakang_buka_kunci(
    struct buffer_belakang *bb)
{
    if (bb == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (bb->magic != BB_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    bb->flags &= (tak_bertanda32_t)(~BB_FLAG_TERKUNCI);

    return STATUS_BERHASIL;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: PENANDAAN KOTOR/BERSIH                                      */
/* ========================================================================== */

/*
 * buffer_belakang_tandai_kotor
 * ---------------------------
 * Menandai buffer belakang sebagai kotor (perlu disalin).
 *
 * Parameter:
 *   bb - Pointer ke struct buffer_belakang
 *
 * Return: STATUS_BERHASIL jika berhasil
 *         STATUS_PARAM_NULL jika bb NULL
 *         STATUS_PARAM_INVALID jika magic tidak cocok
 */
status_t buffer_belakang_tandai_kotor(
    struct buffer_belakang *bb)
{
    if (bb == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (bb->magic != BB_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    bb->flags |= BB_FLAG_KOTOR;

    return STATUS_BERHASIL;
}

/*
 * buffer_belakang_tandai_bersih
 * ----------------------------
 * Menandai buffer belakang sebagai bersih (sudah disalin).
 *
 * Parameter:
 *   bb - Pointer ke struct buffer_belakang
 *
 * Return: STATUS_BERHASIL jika berhasil
 *         STATUS_PARAM_NULL jika bb NULL
 *         STATUS_PARAM_INVALID jika magic tidak cocok
 */
status_t buffer_belakang_tandai_bersih(
    struct buffer_belakang *bb)
{
    if (bb == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (bb->magic != BB_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    bb->flags &= (tak_bertanda32_t)(~BB_FLAG_KOTOR);

    return STATUS_BERHASIL;
}

/*
 * buffer_belakang_apakah_kotor
 * ---------------------------
 * Memeriksa apakah buffer belakang ditandai kotor.
 *
 * Parameter:
 *   bb - Pointer ke struct buffer_belakang
 *
 * Return: BENAR jika kotor, SALAH jika bersih atau error
 */
bool_t buffer_belakang_apakah_kotor(
    const struct buffer_belakang *bb)
{
    if (bb == NULL || bb->magic != BB_MAGIC) {
        return SALAH;
    }
    return (bb->flags & BB_FLAG_KOTOR) ? BENAR : SALAH;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: INFORMASI                                                   */
/* ========================================================================== */

/*
 * buffer_belakang_dapat_jumlah_tukar
 * --------------------------------
 * Mendapatkan jumlah swap yang sudah dilakukan.
 *
 * Parameter:
 *   bb - Pointer ke struct buffer_belakang
 *
 * Return: Jumlah swap, atau 0 jika error
 */
tak_bertanda32_t buffer_belakang_dapat_jumlah_tukar(
    const struct buffer_belakang *bb)
{
    if (bb == NULL || bb->magic != BB_MAGIC) {
        return 0;
    }
    return bb->jumlah_tukar;
}

/*
 * buffer_belakang_dapat_jumlah_aktif
 * ---------------------------------
 * Mendapatkan jumlah buffer belakang yang sedang aktif.
 *
 * Return: Jumlah buffer belakang aktif
 */
tak_bertanda32_t buffer_belakang_dapat_jumlah_aktif(void)
{
    return g_bb_aktif;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: VALIDASI                                                    */
/* ========================================================================== */

/*
 * buffer_belakang_validasi
 * ----------------------
 * Memvalidasi integritas struktur buffer belakang.
 *
 * Memeriksa magic number, keberadaan buffer depan dan
 * belakang, kesesuaian dimensi antara kedua buffer,
 * dan kevalidan masing-masing buffer piksel.
 *
 * Parameter:
 *   bb - Pointer ke struct buffer_belakang
 *
 * Return: BENAR jika valid, SALAH jika tidak
 */
bool_t buffer_belakang_validasi(
    const struct buffer_belakang *bb)
{
    if (bb == NULL) {
        return SALAH;
    }
    if (bb->magic != BB_MAGIC) {
        return SALAH;
    }
    if (bb->depan == NULL || bb->belakang == NULL) {
        return SALAH;
    }

    /* Validasi masing-masing buffer piksel */
    if (!bb_validasi_buffer(bb->depan)) {
        return SALAH;
    }
    if (!bb_validasi_buffer(bb->belakang)) {
        return SALAH;
    }

    /* Pastikan kedua buffer memiliki dimensi dan format sama */
    if (bb->depan->lebar != bb->belakang->lebar) {
        return SALAH;
    }
    if (bb->depan->tinggi != bb->belakang->tinggi) {
        return SALAH;
    }
    if (bb->depan->format != bb->belakang->format) {
        return SALAH;
    }
    if (bb->depan->pitch != bb->belakang->pitch) {
        return SALAH;
    }
    if (bb->depan->ukuran != bb->belakang->ukuran) {
        return SALAH;
    }

    return BENAR;
}

/* ========================================================================== */
/* FUNGSI PUBLIK: SALIN AREA                                                 */
/* ========================================================================== */

/*
 * buffer_belakang_salin_area
 * -------------------------
 * Menyalin area persegi panjang dari buffer belakang
 * ke buffer depan.
 *
 * Berguna untuk update parsial (partial update) dimana
 * hanya bagian layar yang berubah perlu disalin ke
 * buffer depan, menghemat bandwidth memori.
 *
 * Parameter:
 *   bb - Pointer ke struct buffer_belakang
 *   dx - Koordinat x tujuan di buffer depan
 *   dy - Koordinat y tujuan di buffer depan
 *   sx - Koordinat x sumber di buffer belakang
 *   sy - Koordinat y sumber di buffer belakang
 *   w  - Lebar area dalam piksel
 *   h  - Tinggi area dalam piksel
 *
 * Return: STATUS_BERHASIL jika berhasil
 *         STATUS_PARAM_NULL jika bb NULL
 *         STATUS_PARAM_INVALID jika magic tidak cocok
 *         STATUS_BUSY jika buffer terkunci
 *         STATUS_PARAM_JARAK jika koordinat di luar batas
 */
status_t buffer_belakang_salin_area(
    struct buffer_belakang *bb,
    tak_bertanda32_t dx,
    tak_bertanda32_t dy,
    tak_bertanda32_t sx,
    tak_bertanda32_t sy,
    tak_bertanda32_t w,
    tak_bertanda32_t h)
{
    tak_bertanda32_t baris;
    tak_bertanda8_t bpp;

    if (bb == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (bb->magic != BB_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (bb->flags & BB_FLAG_TERKUNCI) {
        return STATUS_BUSY;
    }
    if (bb->depan == NULL || bb->belakang == NULL) {
        return STATUS_GAGAL;
    }
    if (bb->depan->data == NULL ||
        bb->belakang->data == NULL) {
        return STATUS_GAGAL;
    }

    /* Tidak ada yang perlu disalin */
    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }

    /* Validasi koordinat sumber berada dalam buffer belakang */
    if (sx >= bb->belakang->lebar ||
        sy >= bb->belakang->tinggi) {
        return STATUS_PARAM_JARAK;
    }

    /* Validasi koordinat tujuan berada dalam buffer depan */
    if (dx >= bb->depan->lebar ||
        dy >= bb->depan->tinggi) {
        return STATUS_PARAM_JARAK;
    }

    /* Potong area jika melebihi batas buffer sumber */
    if (sx + w > bb->belakang->lebar) {
        w = bb->belakang->lebar - sx;
    }
    if (sy + h > bb->belakang->tinggi) {
        h = bb->belakang->tinggi - sy;
    }

    /* Potong area jika melebihi batas buffer tujuan */
    if (dx + w > bb->depan->lebar) {
        w = bb->depan->lebar - dx;
    }
    if (dy + h > bb->depan->tinggi) {
        h = bb->depan->tinggi - dy;
    }

    /* Setelah pemotongan, pastikan masih ada yang disalin */
    if (w == 0 || h == 0) {
        return STATUS_BERHASIL;
    }

    bpp = (tak_bertanda8_t)bb_bpp_dari_format(
        bb->depan->format);

    /* Salin baris per baris */
    for (baris = 0; baris < h; baris++) {
        tak_bertanda8_t *sp = bb->belakang->data +
            (sy + baris) * bb->belakang->pitch +
            sx * (tak_bertanda32_t)bpp;
        tak_bertanda8_t *dp = bb->depan->data +
            (dy + baris) * bb->depan->pitch +
            dx * (tak_bertanda32_t)bpp;
        kernel_memcpy(dp, sp,
                      (ukuran_t)w * (ukuran_t)bpp);
    }

    bb->depan->flags |= 0x08; /* BUFFER_FLAG_KOTOR */
    bb->flags |= BB_FLAG_KOTOR;

    return STATUS_BERHASIL;
}
