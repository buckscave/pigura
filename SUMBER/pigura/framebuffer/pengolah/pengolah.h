/*
 * PIGURA OS - PENGOLAH.H
 * ======================
 * Header utama subsistem pengolah grafis Pigura OS.
 *
 * Modul pengolah menyediakan operasi menggambar 2D tingkat tinggi:
 *   titik, garis, kotak, lingkaran, elip, busur, poligon,
 *   kurva, dan pengisian area.
 *
 * Setiap operasi memiliki tiga backend:
 *   - CPU (software rendering)
 *   - GPU (akselerasi hardware via akselerasi_gpu)
 *   - Hybrid (otomatis memilih CPU/GPU berdasarkan ukuran)
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

#ifndef PENGOLAH_H
#define PENGOLAH_H

/*
 * ===========================================================================
 * KONSTANTA PENGOLAH
 * ===========================================================================
 */

/* Magic number pengolah */
#define PENGOLAH_MAGIC         0x504E474F  /* "PENG" */
#define PENGOLAH_VERSI         0x0100

/* Mode backend pengolah */
#define PENGOLAH_MODE_CPU      0
#define PENGOLAH_MODE_GPU      1
#define PENGOLAH_MODE_HYBRID   2

/* Kapabilitas operasi */
#define PENGOLAH_OP_TITIK      0x00000001
#define PENGOLAH_OP_GARIS      0x00000002
#define PENGOLAH_OP_KOTAK      0x00000004
#define PENGOLAH_OP_ISI        0x00000008
#define PENGOLAH_OP_LINGKARAN  0x00000010
#define PENGOLAH_OP_ELIP       0x00000020
#define PENGOLAH_OP_BUSUR      0x00000040
#define PENGOLAH_OP_POLIGON    0x00000080
#define PENGOLAH_OP_KURVA      0x00000100
#define PENGOLAH_OP_PRIMITIF   0x00000200

/* Flag gambar */
#define PENGOLAH_FLAG_ANTI_ALIAS  0x01
#define PENGOLAH_FLAG_KLIP        0x02
#define PENGOLAH_FLAG_BENDELA     0x04

/* Threshold hybrid: area >= ini pakai GPU */
#define PENGOLAH_HYBRID_AMBANG    256

/* Jumlah titik maksimum poligon */
#define PENGOLAH_POLIGON_MAKS     256

/* Jumlah titik maksimum kurva Bezier */
#define PENGOLAH_KURVA_MAKS       64

/* Jumlah substep rendering kurva */
#define PENGOLAH_KURVA_SUBSTEP    32

/*
 * ===========================================================================
 * STRUKTUR DATA PENGOLAH
 * ===========================================================================
 */

/*
 * Titik 2D untuk operasi grafis.
 * Menggunakan koordinat signed untuk mendukung
 * posisi negatif dalam transformasi.
 */
struct p_titik {
    tanda32_t x;
    tanda32_t y;
};

/*
 * Warna RGBA 8-bit.
 */
struct p_warna {
    tak_bertanda8_t r;
    tak_bertanda8_t g;
    tak_bertanda8_t b;
    tak_bertanda8_t a;
};

/*
 * Persegi panjang untuk area operasi.
 */
struct p_persegi {
    tanda32_t x, y;
    tak_bertanda32_t lebar;
    tak_bertanda32_t tinggi;
};

/*
 * Konfigurasi state gambar.
 * Berisi parameter yang mempengaruhi bagaimana
 * operasi menggambar dilaksanakan.
 */
struct p_konfig {
    tak_bertanda32_t mode;       /* PENGOLAH_MODE_*    */
    tak_bertanda32_t flags;      /* PENGOLAH_FLAG_*    */
    struct p_warna warna;        /* Warna aktif        */
    struct p_warna warna_bg;     /* Warna background   */
    tak_bertanda8_t ketebalan;   /* Ketebalan garis    */
    bool_t klip_aktif;           /* Apakah klip aktif  */
    struct p_persegi klip;       /* Area klip          */
};

/*
 * Statistik operasi pengolah.
 */
struct p_statistik {
    tak_bertanda64_t jumlah_operasi;
    tak_bertanda64_t jumlah_cpu;
    tak_bertanda64_t jumlah_gpu;
    tak_bertanda64_t piksel_diproses;
};

/*
 * Konteks pengolah utama.
 * Menyimpan seluruh state untuk sesi menggambar.
 */
struct p_konteks {
    tak_bertanda32_t magic;
    tak_bertanda32_t versi;
    tak_bertanda32_t mode;

    /* Buffer piksel target */
    tak_bertanda32_t *buffer;
    tak_bertanda32_t lebar;
    tak_bertanda32_t tinggi;
    tak_bertanda32_t pitch;      /* Byte per baris    */

    /* State gambar */
    struct p_konfig konfig;

    /* Statistik */
    struct p_statistik statistik;

    /* Status */
    bool_t aktif;
    tak_bertanda32_t kode_error;
};

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - INTI PENGOLAH
 * ===========================================================================
 */

/* Inisialisasi dan shutdown */
status_t pengolah_init(struct p_konteks *ktx,
                       tak_bertanda32_t *buffer,
                       tak_bertanda32_t lebar,
                       tak_bertanda32_t tinggi);
void pengolah_shutdown(struct p_konteks *ktx);
bool_t pengolah_siap(const struct p_konteks *ktx);
status_t pengolah_set_mode(struct p_konteks *ktx,
                           tak_bertanda32_t mode);
tak_bertanda32_t pengolah_dapat_mode(const struct p_konteks *ktx);

/* Konfigurasi gambar */
status_t pengolah_set_warna(struct p_konteks *ktx,
                            tak_bertanda8_t r,
                            tak_bertanda8_t g,
                            tak_bertanda8_t b,
                            tak_bertanda8_t a);
status_t pengolah_set_ketebalan(struct p_konteks *ktx,
                                tak_bertanda8_t ketebalan);
status_t pengolah_set_klip(struct p_konteks *ktx,
                           tanda32_t x, tanda32_t y,
                           tak_bertanda32_t lebar,
                           tak_bertanda32_t tinggi);
void pengolah_hapus_klip(struct p_konteks *ktx);

/* Statistik */
status_t pengolah_dapat_statistik(const struct p_konteks *ktx,
                                  struct p_statistik *stat);
void pengolah_reset_statistik(struct p_konteks *ktx);

/* Konversi warna */
tak_bertanda32_t pengolah_rgba_ke_piksel(tak_bertanda8_t r,
                                         tak_bertanda8_t g,
                                         tak_bertanda8_t b,
                                         tak_bertanda8_t a);
void pengolah_piksel_ke_rgba(tak_bertanda32_t piksel,
                             tak_bertanda8_t *r,
                             tak_bertanda8_t *g,
                             tak_bertanda8_t *b,
                             tak_bertanda8_t *a);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - OPERASI GRAFIS
 * ===========================================================================
 */

/* Titik */
status_t pengolah_titik(struct p_konteks *ktx,
                        tanda32_t x, tanda32_t y);
status_t pengolah_titik_warna(struct p_konteks *ktx,
                              tanda32_t x, tanda32_t y,
                              tak_bertanda32_t warna);

/* Garis */
status_t pengolah_garis(struct p_konteks *ktx,
                        tanda32_t x0, tanda32_t y0,
                        tanda32_t x1, tanda32_t y1);
status_t pengolah_garis_warna(struct p_konteks *ktx,
                              tanda32_t x0, tanda32_t y0,
                              tanda32_t x1, tanda32_t y1,
                              tak_bertanda32_t warna);

/* Kotak */
status_t pengolah_kotak(struct p_konteks *ktx,
                        tanda32_t x, tanda32_t y,
                        tak_bertanda32_t lebar,
                        tak_bertanda32_t tinggi);
status_t pengolah_kotak_isi(struct p_konteks *ktx,
                            tanda32_t x, tanda32_t y,
                            tak_bertanda32_t lebar,
                            tak_bertanda32_t tinggi);
status_t pengolah_kotak_warna(struct p_konteks *ktx,
                              tanda32_t x, tanda32_t y,
                              tak_bertanda32_t lebar,
                              tak_bertanda32_t tinggi,
                              tak_bertanda32_t warna);
status_t pengolah_kotak_isi_warna(struct p_konteks *ktx,
                                  tanda32_t x, tanda32_t y,
                                  tak_bertanda32_t lebar,
                                  tak_bertanda32_t tinggi,
                                  tak_bertanda32_t warna);

/* Lingkaran */
status_t pengolah_lingkaran(struct p_konteks *ktx,
                            tanda32_t cx, tanda32_t cy,
                            tak_bertanda32_t jari_jari);
status_t pengolah_lingkaran_isi(struct p_konteks *ktx,
                                tanda32_t cx, tanda32_t cy,
                                tak_bertanda32_t jari_jari);
status_t pengolah_lingkaran_warna(struct p_konteks *ktx,
                                  tanda32_t cx, tanda32_t cy,
                                  tak_bertanda32_t jari_jari,
                                  tak_bertanda32_t warna);

/* Elip */
status_t pengolah_elip(struct p_konteks *ktx,
                       tanda32_t cx, tanda32_t cy,
                       tak_bertanda32_t rx,
                       tak_bertanda32_t ry);
status_t pengolah_elip_isi(struct p_konteks *ktx,
                           tanda32_t cx, tanda32_t cy,
                           tak_bertanda32_t rx,
                           tak_bertanda32_t ry);

/* Busur */
status_t pengolah_busur(struct p_konteks *ktx,
                        tanda32_t cx, tanda32_t cy,
                        tak_bertanda32_t jari_jari,
                        tanda32_t sudut_awal,
                        tanda32_t sudut_akhir);

/* Poligon */
status_t pengolah_poligon(struct p_konteks *ktx,
                          const struct p_titik *titik,
                          tak_bertanda32_t jumlah);
status_t pengolah_poligon_isi(struct p_konteks *ktx,
                              const struct p_titik *titik,
                              tak_bertanda32_t jumlah);

/* Kurva Bezier */
status_t pengolah_kurva_bezier2(struct p_konteks *ktx,
                                tanda32_t x0, tanda32_t y0,
                                tanda32_t x1, tanda32_t y1,
                                tanda32_t x2, tanda32_t y2);
status_t pengolah_kurva_bezier3(struct p_konteks *ktx,
                                tanda32_t x0, tanda32_t y0,
                                tanda32_t x1, tanda32_t y1,
                                tanda32_t x2, tanda32_t y2,
                                tanda32_t x3, tanda32_t y3);

/* Pengisian area (flood fill) */
status_t pengolah_isi_area(struct p_konteks *ktx,
                           tanda32_t x, tanda32_t y,
                           tak_bertanda32_t warna_isi,
                           tak_bertanda32_t warna_batas);

/* Primitif gabungan */
status_t pengolah_bersihkan(struct p_konteks *ktx,
                            tak_bertanda32_t warna);
status_t pengolah_silang(struct p_konteks *ktx,
                         tanda32_t x, tanda32_t y,
                         tak_bertanda32_t lebar,
                         tak_bertanda32_t tinggi);
status_t pengolah_bingkai(struct p_konteks *ktx,
                          tanda32_t x, tanda32_t y,
                          tak_bertanda32_t lebar,
                          tak_bertanda32_t tinggi,
                          tak_bertanda8_t tebal);
status_t pengolah_grid(struct p_konteks *ktx,
                       tanda32_t x, tanda32_t y,
                       tak_bertanda32_t lebar,
                       tak_bertanda32_t tinggi,
                       tak_bertanda32_t sel_jml_x,
                       tak_bertanda32_t sel_jml_y);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INTERNAL (akses antar-modul pengolah)
 * ===========================================================================
 */

/* Klip titik ke area pengolah */
bool_t p_klip_titik(const struct p_konteks *ktx,
                    tanda32_t x, tanda32_t y);

/* Tulis piksel dengan klip otomatis */
void p_tulis_piksel(const struct p_konteks *ktx,
                    tanda32_t x, tanda32_t y,
                    tak_bertanda32_t warna);

/* Baca piksel dengan batas aman */
tak_bertanda32_t p_baca_piksel(const struct p_konteks *ktx,
                               tanda32_t x, tanda32_t y);

/* Cek apakah koordinat di dalam buffer */
bool_t p_dalam_buffer(const struct p_konteks *ktx,
                      tanda32_t x, tanda32_t y);

#endif /* PENGOLAH_H */
