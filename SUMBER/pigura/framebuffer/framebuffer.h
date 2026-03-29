/*
 * PIGURA OS - FRAMEBUFFER.H
 * =========================
 * Header utama subsistem permukaan (surface) framebuffer.
 *
 * Permukaan adalah abstraksi software untuk menyimpan piksel.
 * Menyediakan operasi: buat, hapus, tulis, baca, blit, render.
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

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

/*
 * ===========================================================================
 * KONSTANTA PERMUKAAN
 * ===========================================================================
 */

/* Magic number permukaan */
#define PERMUKAAN_MAGIC           0x5045524D  /* "PERM" */

/* Jumlah maksimum permukaan */
#define PERMUKAAN_MAKS            256

/* Format pixel */
#define FORMAT_INVALID            0
#define FORMAT_RGB332             1
#define FORMAT_RGB565             2
#define FORMAT_RGB888             3
#define FORMAT_ARGB8888           4
#define FORMAT_XRGB8888           5

/* Flags permukaan */
#define PERMUKAAN_FLAG_DOUBLE_BUF   0x01
#define PERMUKAAN_FLAG_LINEAR     0x02
#define PERMUKAAN_FLAG_GPU_ACCEL  0x04

/*
 * ===========================================================================
 * TIPE OPAQUE PERMUKAAN
 * ===========================================================================
 *
 * struct permukaan didefinisikan lengkap di framebuffer.c.
 * Di sini hanya forward declaration agar pengguna bisa
 * menyimpan pointer tanpa mengetahui isi struct.
 */

struct permukaan;

/*
 * Handle permukaan (opaque pointer).
 * Digunakan oleh seluruh fungsi publik permukaan.
 */
typedef struct permukaan permukaan_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - INISIALISASI
 * ===========================================================================
 */

/* Inisialisasi subsistem permukaan */
status_t permukaan_init(void);

/* Shutdown dan bersihkan semua permukaan */
void permukaan_shutdown(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - MANAJEMEN PERMUKAAN
 * ===========================================================================
 */

/* Buat permukaan baru di memori */
permukaan_t *permukaan_buat(tak_bertanda32_t lebar,
                            tak_bertanda32_t tinggi,
                            tak_bertanda32_t bpp);

/* Buat permukaan display dari alamat fisik */
permukaan_t *permukaan_buat_display(
    tak_bertanda64_t alamat_fisik,
    tak_bertanda32_t lebar,
    tak_bertanda32_t tinggi,
    tak_bertanda32_t bpp,
    tak_bertanda32_t pitch);

/* Hapus satu permukaan */
void permukaan_hapus(permukaan_t *handle);

/* Hapus semua permukaan */
void permukaan_hapus_semua(void);

/* Dapatkan handle permukaan display */
permukaan_t *permukaan_display(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - OPERASI PIKSEL
 * ===========================================================================
 */

/* Tulis piksel pada koordinat tertentu */
void permukaan_tulis_piksel(permukaan_t *handle,
                            tak_bertanda32_t x,
                            tak_bertanda32_t y,
                            tak_bertanda32_t warna);

/* Baca piksel pada koordinat tertentu */
tak_bertanda32_t permukaan_baca_piksel(permukaan_t *handle,
                                       tak_bertanda32_t x,
                                       tak_bertanda32_t y);

/* Isi seluruh permukaan dengan satu warna */
void permukaan_isi(permukaan_t *handle,
                   tak_bertanda32_t warna);

/* Isi area kotak pada permukaan */
void permukaan_isi_kotak(permukaan_t *handle,
                         tak_bertanda32_t x,
                         tak_bertanda32_t y,
                         tak_bertanda32_t lebar,
                         tak_bertanda32_t tinggi,
                         tak_bertanda32_t warna);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - GETTER
 * ===========================================================================
 */

/* Dapatkan lebar permukaan */
tak_bertanda32_t permukaan_lebar(permukaan_t *handle);

/* Dapatkan tinggi permukaan */
tak_bertanda32_t permukaan_tinggi(permukaan_t *handle);

/* Dapatkan bits per pixel */
tak_bertanda32_t permukaan_bpp(permukaan_t *handle);

/* Dapatkan pitch (byte per baris) */
tak_bertanda32_t permukaan_pitch(permukaan_t *handle);

/* Dapatkan pointer buffer piksel mentah */
void *permukaan_buffer(permukaan_t *handle);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - DOUBLE BUFFERING
 * ===========================================================================
 */

/* Aktifkan double buffering */
status_t permukaan_aktifkan_double_buffer(
    permukaan_t *handle);

/* Tukar buffer depan dan belakang */
void permukaan_flip(permukaan_t *handle);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - BLIT
 * ===========================================================================
 */

/* Copy area dari permukaan sumber ke tujuan */
void permukaan_blit_copy(permukaan_t *src,
                         permukaan_t *dst,
                         tak_bertanda32_t sx,
                         tak_bertanda32_t sy,
                         tak_bertanda32_t dx,
                         tak_bertanda32_t dy,
                         tak_bertanda32_t w,
                         tak_bertanda32_t h);

/* Copy dengan stretch/scale */
void permukaan_blit_stretch(permukaan_t *src,
                            permukaan_t *dst,
                            tak_bertanda32_t sx,
                            tak_bertanda32_t sy,
                            tak_bertanda32_t sw,
                            tak_bertanda32_t sh,
                            tak_bertanda32_t dx,
                            tak_bertanda32_t dy,
                            tak_bertanda32_t dw,
                            tak_bertanda32_t dh);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - RENDER PRIMITIF
 * ===========================================================================
 */

/* Gambar garis (Bresenham) */
void permukaan_garis(permukaan_t *p,
                     tak_bertanda32_t x1,
                     tak_bertanda32_t y1,
                     tak_bertanda32_t x2,
                     tak_bertanda32_t y2,
                     tak_bertanda32_t warna);

/* Gambar kotak (outline atau isi) */
void permukaan_kotak(permukaan_t *p,
                     tak_bertanda32_t x,
                     tak_bertanda32_t y,
                     tak_bertanda32_t w,
                     tak_bertanda32_t h,
                     tak_bertanda32_t warna,
                     bool_t isi);

/* Gambar lingkaran (outline atau isi) */
void permukaan_lingkaran(permukaan_t *p,
                         tak_bertanda32_t cx,
                         tak_bertanda32_t cy,
                         tak_bertanda32_t radius,
                         tak_bertanda32_t warna,
                         bool_t isi);

#endif /* FRAMEBUFFER_H */
