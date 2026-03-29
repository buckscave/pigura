/*
 * PIGURA OS - TEKS.H
 * ===================
 * Header utama subsistem teks (text rendering) Pigura OS.
 *
 * Modul teks menyediakan layanan rendering karakter dan string
 * ke permukaan framebuffer. Mendukung font bitmap dan TrueType
 * dengan cache glyph untuk performa optimal.
 *
 * Submodul:
 *   - glyph   : Operasi data glyph individual
 *   - font    : Manajemen font (daftar, buka, tutup)
 *   - font_bitmap : Font bitmap sederhana (8x8, 8x16)
 *   - font_ttf    : Parser dan renderer TrueType
 *   - font_cache  : Cache glyph yang sudah di-render
 *   - teks    : Rendering teks ke permukaan
 *   - ukuran  : Pengukuran lebar/tinggi string
 *   - pengolah_teks : Operasi tingkat tinggi (baris, paragraf)
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

#ifndef TEKS_H
#define TEKS_H

/*
 * ===========================================================================
 * KONSTANTA TEKS
 * ===========================================================================
 */

/* Magic number modul teks */
#define TEKS_MAGIC            0x54455854  /* "TEKS" */

/* Versi modul */
#define TEKS_VERSI            0x0100

/* Magic number font */
#define FONT_MAGIC            0x464F4E54  /* "FONT" */

/* Jumlah maksimum font yang terdaftar */
#define FONT_DAFTAR_MAKS      32

/* Jumlah maksimum glyph per font */
#define FONT_GLYPH_MAKS       1024

/* Ukuran default font bitmap */
#define FONT_BITMAP_LEBAR_8   8
#define FONT_BITMAP_TINGGI_8  8
#define FONT_BITMAP_LEBAR_16  8
#define FONT_BITMAP_TINGGI_16 16

/* Ukuran atlas cache glyph (piksel) */
#define GLYPH_ATLAS_LEBAR     256
#define GLYPH_ATLAS_TINGGI    256

/* Jumlah maksimum glyph dalam cache */
#define GLYPH_CACHE_MAKS      256

/* Ukuran maksimum satu glyph (piksel) */
#define GLYPH_UKURAN_MAKS     64

/* Panjang string maksimum untuk operasi teks */
#define TEKS_PANJANG_MAKS     4096

/* Jumlah baris maksimum dalam paragraf */
#define TEKS_BARIS_MAKS       512

/* Encoding */
#define TEKS_ENCODING_ASCII   0
#define TEKS_ENCODING_UTF8    1

/* Style teks */
#define TEKS_STYLE_NORMAL     0x00
#define TEKS_STYLE_TEBAL      0x01
#define TEKS_STYLE_MIRING     0x02
#define TEKS_STYLE_GARIS_BAWAH 0x04

/* Alignment teks */
#define TEKS_RATA_KIRI        0
#define TEKS_RATA_TENGAH      1
#define TEKS_RATA_KANAN       2

/* Wrap mode */
#define TEKS_WRAP_KATA        0
#define TEKS_WRAP_KARAKTER    1

/*
 * ===========================================================================
 * STRUKTUR DATA GLYPH
 * ===========================================================================
 */

/*
 * Bitmap glyph sederhana.
 * Setiap piksel disimpan sebagai 1 bit dalam larik byte.
 * baris[j] merepresentasikan baris ke-j dari atas.
 */
typedef struct glyph_bitmap {
    tak_bertanda8_t lebar;    /* Lebar glyph dalam piksel */
    tak_bertanda8_t tinggi;   /* Tinggi glyph dalam piksel */
    tanda8_t x_ofset;         /* Offset horizontal */
    tanda8_t y_ofset;         /* Offset vertikal */
    tanda8_t x_lanjut;        /* Jarak horizontal ke glyph berikutnya */
    tak_bertanda8_t ukuran;   /* Ukuran data bitmap dalam byte */
    tak_bertanda8_t *data;    /* Data bitmap (1 bit per piksel) */
} glyph_bitmap_t;

/*
 * Glyph yang sudah di-render ke buffer piksel ARGB.
 * Digunakan oleh cache untuk hasil render font TrueType.
 */
typedef struct glyph_terrender {
    tak_bertanda32_t kode;    /* Kode Unicode */
    tak_bertanda8_t lebar;    /* Lebar glyph */
    tak_bertanda8_t tinggi;   /* Tinggi glyph */
    tanda8_t x_bawa;          /* Bearing X */
    tanda8_t y_bawa;          /* Bearing Y */
    tanda8_t x_lanjut;        /* Advance X */
    tanda8_t y_lanjut;        /* Advance Y */
    tak_bertanda32_t *piksel; /* Buffer piksel ARGB */
} glyph_terrender_t;

/*
 * ===========================================================================
 * STRUKTUR DATA FONT
 * ===========================================================================
 */

/* Tipe font yang didukung */
#define FONT_JENIS_BERKAS     0  /* Font bitmap dari berkas */
#define FONT_JENIS_TERDEGAR   1  /* Font bitmap terdegar */
#define FONT_JENIS_TTF        2  /* Font TrueType */

/*
 * Informasi metrik font.
 * Menyimpan data global font yang diperlukan
 * untuk layout teks (spasi, tinggi, garis).
 */
typedef struct font_metrik {
    tak_bertanda16_t tinggi_asc;   /* Tinggi ascender */
    tak_bertanda16_t tinggi_desc;  /* Kedalaman descender */
    tak_bertanda16_t tinggi_baris; /* Jarak antar baris */
    tak_bertanda16_t lebar_maks;   /* Lebar glyph terlebar */
    tak_bertanda8_t  ukuran;       /* Ukuran font (pt) */
    tak_bertanda8_t  dpi;          /* DPI target */
} font_metrik_t;

/*
 * Struktur font.
 * Mewakili satu font yang terdaftar dalam sistem.
 * Menggunakan pola opaque: detail internal hanya
 * diketahui oleh modul font.
 */
struct font;

typedef struct font font_t;

/*
 * ===========================================================================
 * STRUKTUR DATA CACHE GLYPH
 * ===========================================================================
 */

/*
 * Entri cache glyph.
 * Setiap entri menyimpan satu glyph yang sudah di-render.
 */
typedef struct glyph_cache_entri {
    tak_bertanda32_t kunci;        /* Kunci hash */
    tak_bertanda32_t kode;        /* Kode Unicode */
    tak_bertanda8_t  ukuran_pt;   /* Ukuran font saat render */
    glyph_terrender_t glyph;      /* Data glyph */
    bool_t dipakai;               /* Apakah slot dipakai */
} glyph_cache_entri_t;

/*
 * Struktur cache glyph.
 * Menggunakan hash table sederhana dengan open addressing.
 */
typedef struct glyph_cache {
    tak_bertanda32_t magic;       /* Magic number */
    tak_bertanda32_t kapasitas;   /* Jumlah slot */
    tak_bertanda32_t terisi;      /* Jumlah slot terisi */
    tak_bertanda32_t pemukulan;   /* Cache hit count */
    tak_bertanda32_t kehilangan;  /* Cache miss count */
    glyph_cache_entri_t *entri;   /* Larik entri cache */
} glyph_cache_t;

/*
 * ===========================================================================
 * STRUKTUR DATA RENDER TEKS
 * ===========================================================================
 */

/*
 * Opsi rendering teks.
 * Mengatur bagaimana teks digambar ke permukaan.
 */
typedef struct teks_opsi {
    tak_bertanda32_t warna_depan;    /* Warna teks (ARGB) */
    tak_bertanda32_t warna_belakang; /* Warna latar (ARGB) */
    tak_bertanda8_t  ukuran;         /* Ukuran font (pt) */
    tak_bertanda8_t  style;          /* Style (TEBAL, dll) */
    tak_bertanda8_t  rata;           /* Alignment */
    tak_bertanda8_t  wrap;           /* Wrap mode */
    bool_t transparan;               /* Latar transparan */
} teks_opsi_t;

/*
 * Hasil pengukuran teks.
 * Menyimpan dimensi teks setelah diukur.
 */
typedef struct teks_ukuran {
    tak_bertanda32_t lebar;      /* Lebar total piksel */
    tak_bertanda32_t tinggi;     /* Tinggi total piksel */
    tak_bertanda32_t baris;      /* Jumlah baris */
    tak_bertanda32_t karakter;   /* Jumlah karakter */
} teks_ukuran_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - GLYPH
 * ===========================================================================
 */

/* Inisialisasi bitmap glyph dari data mentah */
status_t glyph_buat_bitmap(glyph_bitmap_t *g,
                           tak_bertanda8_t lebar,
                           tak_bertanda8_t tinggi,
                           tanda8_t x_ofset,
                           tanda8_t y_ofset,
                           tanda8_t x_lanjut,
                           const tak_bertanda8_t *data);

/* Bebaskan memori bitmap glyph */
void glyph_hapus_bitmap(glyph_bitmap_t *g);

/* Salin bitmap glyph */
status_t glyph_salin_bitmap(const glyph_bitmap_t *src,
                            glyph_bitmap_t *dst);

/* Render glyph bitmap ke permukaan */
void glyph_render_bitmap(const glyph_bitmap_t *g,
                         permukaan_t *permukaan,
                         tak_bertanda32_t x,
                         tak_bertanda32_t y,
                         tak_bertanda32_t warna);

/* Render glyph terrender ke permukaan (ARGB) */
void glyph_render_arbg(const glyph_terrender_t *g,
                       permukaan_t *permukaan,
                       tak_bertanda32_t x,
                       tak_bertanda32_t y);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - FONT
 * ===========================================================================
 */

/* Inisialisasi subsistem font */
status_t font_init(void);

/* Shutdown subsistem font */
void font_shutdown(void);

/* Daftarkan font bitmap dari data terdegar */
font_t *font_daftar_bitmap(const char *nama,
                           const glyph_bitmap_t *daftar_glyph,
                           tak_bertanda16_t jumlah_glyph,
                           tak_bertanda8_t ukuran_pt);

/* Daftarkan font dari berkas bitmap */
font_t *font_daftar_berkas(const char *nama_berkas,
                           const char *nama);

/* Daftarkan font TrueType dari berkas */
font_t *font_daftar_ttf(const char *nama_berkas,
                        const char *nama,
                        tak_bertanda8_t ukuran_pt);

/* Hapus font dari sistem */
status_t font_hapus(font_t *f);

/* Cari font berdasarkan nama */
font_t *font_cari(const char *nama);

/* Dapatkan metrik font */
status_t font_dapat_metrik(const font_t *f,
                           font_metrik_t *metrik);

/* Dapatkan glyph dari font */
const glyph_bitmap_t *font_dapat_glyph(const font_t *f,
                                       tak_bertanda32_t kode);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - FONT BITMAP
 * ===========================================================================
 */

/* Buat font bitmap 8x8 terdegar */
font_t *font_bitmap_buat_8x8(const char *nama);

/* Buat font bitmap 8x16 terdegar */
font_t *font_bitmap_buat_8x16(const char *nama);

/* Render glyph bitmap ke buffer piksel */
status_t font_bitmap_render(const glyph_bitmap_t *g,
                            tak_bertanda32_t warna,
                            glyph_terrender_t *hasil);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - FONT TTF
 * ===========================================================================
 */

/* Buka berkas font TrueType */
void *font_ttf_buka(const void *data,
                    tak_bertanda64_t ukuran);

/* Tutup berkas font TrueType */
void font_ttf_tutup(void *konteks);

/* Dapatkan glyph dari font TrueType */
status_t font_ttf_dapat_glyph(void *konteks,
                              tak_bertanda32_t kode,
                              tak_bertanda8_t ukuran_pt,
                              glyph_terrender_t *hasil);

/* Dapatkan metrik font TrueType */
status_t font_ttf_dapat_metrik(void *konteks,
                               tak_bertanda8_t ukuran_pt,
                               font_metrik_t *metrik);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - FONT CACHE
 * ===========================================================================
 */

/* Buat cache glyph */
glyph_cache_t *glyph_cache_buat(tak_bertanda32_t kapasitas);

/* Hapus cache glyph */
void glyph_cache_hapus(glyph_cache_t *cache);

/* Cari glyph dalam cache */
const glyph_terrender_t *glyph_cache_cari(
    glyph_cache_t *cache,
    tak_bertanda32_t kode,
    tak_bertanda8_t ukuran_pt);

/* Tambah glyph ke cache */
status_t glyph_cache_tambah(glyph_cache_t *cache,
                            tak_bertanda32_t kode,
                            tak_bertanda8_t ukuran_pt,
                            const glyph_terrender_t *glyph);

/* Bersihkan semua entri cache */
void glyph_cache_bersihkan(glyph_cache_t *cache);

/* Dapatkan statistik cache */
void glyph_cache_statistik(const glyph_cache_t *cache,
                           tak_bertanda32_t *terisi,
                           tak_bertanda32_t *pemukulan,
                           tak_bertanda32_t *kehilangan);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - TEKS (RENDERING)
 * ===========================================================================
 */

/* Inisialisasi subsistem teks */
status_t teks_init(void);

/* Shutdown subsistem teks */
void teks_shutdown(void);

/* Gambar satu karakter ke permukaan */
status_t teks_gambar_karakter(permukaan_t *permukaan,
                              font_t *font,
                              tak_bertanda32_t x,
                              tak_bertanda32_t y,
                              char karakter,
                              tak_bertanda32_t warna);

/* Gambar string ke permukaan */
status_t teks_gambar_string(permukaan_t *permukaan,
                            font_t *font,
                            tak_bertanda32_t x,
                            tak_bertanda32_t y,
                            const char *teks,
                            tak_bertanda32_t warna);

/* Gambar string dengan opsi */
status_t teks_gambar(permukaan_t *permukaan,
                     font_t *font,
                     tak_bertanda32_t x,
                     tak_bertanda32_t y,
                     const char *teks,
                     const teks_opsi_t *opsi);

/* Gambar string terpotong (clipped) */
status_t teks_gambar_potong(permukaan_t *permukaan,
                            font_t *font,
                            tak_bertanda32_t x,
                            tak_bertanda32_t y,
                            const char *teks,
                            tak_bertanda32_t warna,
                            tak_bertanda32_t lebar_maks);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - UKURAN TEKS
 * ===========================================================================
 */

/* Ukur lebar satu karakter */
tak_bertanda32_t ukuran_karakter(font_t *font, char karakter);

/* Ukur lebar string */
tak_bertanda32_t ukuran_string(font_t *font, const char *teks);

/* Ukur string lengkap */
status_t ukuran_teks(font_t *font, const char *teks,
                     tak_bertanda32_t lebar_maks,
                     teks_ukuran_t *hasil);

/* Ukur satu baris teks */
tak_bertanda32_t ukuran_baris(font_t *font,
                              const char *teks,
                              tak_bertanda32_t pos_awal,
                              tak_bertanda32_t lebar_maks);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - PENGOLAH TEKS
 * ===========================================================================
 */

/* Gambar teks multi-baris dengan wrap otomatis */
status_t pengolah_teks_baris(permukaan_t *permukaan,
                             font_t *font,
                             tak_bertanda32_t x,
                             tak_bertanda32_t y,
                             tak_bertanda32_t lebar_area,
                             const char *teks,
                             const teks_opsi_t *opsi);

/* Gambar teks dengan alignment */
status_t pengolah_teks_rata(permukaan_t *permukaan,
                            font_t *font,
                            tak_bertanda32_t x,
                            tak_bertanda32_t y,
                            tak_bertanda32_t lebar_area,
                            const char *teks,
                            tak_bertanda8_t rata,
                            tak_bertanda32_t warna);

/* Potong string agar muat dalam lebar */
tak_bertanda32_t pengolah_teks_potong(font_t *font,
                                      const char *teks,
                                      tak_bertanda32_t lebar_maks,
                                      char *keluaran,
                                      tak_bertanda32_t ukuran_keluaran);

/* Hitung jumlah baris setelah wrapping */
tak_bertanda32_t pengolah_teks_hitung_baris(
    font_t *font,
    const char *teks,
    tak_bertanda32_t lebar_area);

#endif /* TEKS_H */
