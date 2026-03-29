/*
 * PIGURA OS - PENATA.H
 * =====================
 * Header utama subsistem penata (compositor) Pigura OS.
 *
 * Modul penata menyediakan layanan compositor tingkat tinggi:
 *   - Manajemen lapisan (layer stacking, z-order)
 *   - Efek visual (blur, bayangan, transparansi, animasi)
 *   - Klip area (rectangle clipping, region, intersect, union)
 *   - Orkestrasi render (compositing pipeline)
 *
 * Submodul:
 *   - inti      : Inisialisasi, shutdown, proses render utama
 *   - efek      : Efek visual (blur, shadow, transparan, animasi)
 *   - klip      : Operasi klip area persegi panjang
 *   - lapisan   : Manajemen lapisan dan z-order
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

#ifndef PENATA_H
#define PENATA_H

/*
 * ===========================================================================
 * KONSTANTA PENATA
 * ===========================================================================
 */

/* Magic number modul penata */
#define PENATA_MAGIC            0x504E5441  /* "PNTA" */

/* Versi modul */
#define PENATA_VERSI            0x0100

/* Jumlah maksimum lapisan */
#define PENATA_LAPISAN_MAKS     32

/* Jumlah maksimum rect per region */
#define PENATA_REGION_RECT_MAKS 64

/* Jumlah maksimum efek aktif */
#define PENATA_EFEK_MAKS        8

/* Jumlah maksimum bingkai animasi */
#define PENATA_ANIMASI_FRAMA_MAKS 128

/* Tinggi blur minimum (1 = tanpa blur) */
#define PENATA_BLUR_MIN         1

/* Tinggi blur maksimum */
#define PENATA_BLUR_MAKS        16

/* Radius bayangan default */
#define PENATA_BAYANGAN_RADIUS  4

/* Offset bayangan default */
#define PENATA_BAYANGAN_OFFSET  3

/* Warna bayangan default (hitam semi-transparan) */
#define PENATA_BAYANGAN_WARNA   0x80000000

/* Ambang animasi (fixed-point 16.16) */
#define PENATA_ANIMASI_AMBANG_FP 65536

/* Fixed-point 16.16 konstanta */
#define FP_SATU                 65536
#define FP_SETENGAH             32768
#define FP_NOL                  0

/* Tipe efek */
#define PENATA_EFEK_TIDAK_ADA   0
#define PENATA_EFEK_BLUR        1
#define PENATA_EFEK_BAYANGAN    2
#define PENATA_EFEK_TRANSPARAN  3

/* Status animasi */
#define PENATA_ANIMASI_STOP     0
#define PENATA_ANIMASI_JALAN    1
#define PENATA_ANIMASI_SELESAI  2
#define PENATA_ANIMASI_PAUSE    3

/* Tipe interpolasi animasi */
#define PENATA_INTERPOLASI_LINIER    0
#define PENATA_INTERPOLASI_EASE_IN  1
#define PENATA_INTERPOLASI_EASE_OUT 2

/* Flag lapisan */
#define PENATA_FLAG_VISIBLE     0x01
#define PENATA_FLAG_AKTIF       0x02
#define PENATA_FLAG_KLIP        0x04
#define PENATA_FLAG_TRANSPARAN  0x08
#define PENATA_FLAG_EFEK        0x10
#define PENATA_FLAG_MUTLAK      0x20

/*
 * ===========================================================================
 * STRUKTUR RECT PENATA (PERSEGI PANJANG)
 * ===========================================================================
 */

typedef struct penata_rect {
    tanda32_t x;
    tanda32_t y;
    tak_bertanda32_t lebar;
    tak_bertanda32_t tinggi;
} penata_rect_t;

/*
 * ===========================================================================
 * STRUKTUR REGION (KUMPULAN RECT)
 * ===========================================================================
 */

typedef struct penata_region {
    tak_bertanda32_t jumlah;
    penata_rect_t rect[PENATA_REGION_RECT_MAKS];
} penata_region_t;

/*
 * ===========================================================================
 * STRUKTUR EFEK
 * ===========================================================================
 */

/* Konfigurasi efek blur */
typedef struct penata_efek_blur {
    tak_bertanda32_t radius;
    tak_bertanda32_t iterasi;
} penata_efek_blur_t;

/* Konfigurasi efek bayangan */
typedef struct penata_efek_bayangan {
    tak_bertanda32_t offset_x;
    tak_bertanda32_t offset_y;
    tak_bertanda32_t radius;
    tak_bertanda32_t warna;
} penata_efek_bayangan_t;

/* Konfigurasi efek transparansi */
typedef struct penata_efek_transparan {
    tanda32_t alpha;
} penata_efek_transparan_t;

/* Union konfigurasi efek */
typedef struct penata_efek {
    tak_bertanda32_t tipe;
    union {
        penata_efek_blur_t blur;
        penata_efek_bayangan_t bayangan;
        penata_efek_transparan_t transparan;
    } konfig;
} penata_efek_t;

/*
 * ===========================================================================
 * STRUKTUR ANIMASI
 * ===========================================================================
 */

typedef struct penata_animasi {
    tak_bertanda32_t status;
    tak_bertanda32_t tipe_interpolasi;
    tanda32_t nilai_awal;
    tanda32_t nilai_akhir;
    tak_bertanda32_t durasi;
    tak_bertanda32_t waktu_sekarang;
    tanda32_t nilai_sekarang;
    void (*fungsi)(void *data, tanda32_t nilai);
    void *data;
} penata_animasi_t;

/*
 * ===========================================================================
 * STRUKTUR LAPISAN (LAYER)
 * ===========================================================================
 */

struct penata_lapisan {
    tak_bertanda32_t magic;
    tak_bertanda32_t id;

    /* Posisi dan ukuran */
    tanda32_t x;
    tanda32_t y;
    tak_bertanda32_t lebar;
    tak_bertanda32_t tinggi;

    /* Flag dan status */
    tak_bertanda32_t flags;
    tanda32_t z_urutan;

    /* Area klip (aktif jika PENATA_FLAG_KLIP) */
    penata_rect_t area_klip;

    /* Transparansi global (0-255) */
    tak_bertanda8_t alpha;

    /* Efek yang diterapkan */
    tak_bertanda32_t jumlah_efek;
    penata_efek_t efek[PENATA_EFEK_MAKS];

    /* Pointer ke permukaan konten */
    permukaan_t *permukaan;
};

typedef struct penata_lapisan penata_lapisan_t;

/*
 * ===========================================================================
 * STRUKTUR KONTEKS PENATA UTAMA
 * ===========================================================================
 */

struct penata_konteks {
    tak_bertanda32_t magic;
    tak_bertanda32_t versi;

    /* Permukaan target (desktop/output) */
    permukaan_t *permukaan_target;

    /* Lapisan */
    tak_bertanda32_t jumlah_lapisan;
    tak_bertanda32_t id_counter;
    penata_lapisan_t lapisan[PENATA_LAPISAN_MAKS];

    /* Area klip global */
    penata_region_t region_klip_global;

    /* Animasi aktif */
    tak_bertanda32_t jumlah_animasi;
    penata_animasi_t animasi[PENATA_ANIMASI_FRAMA_MAKS];

    /* Statistik */
    tak_bertanda64_t jumlah_render;
    tak_bertanda64_t jumlah_klip;
    tak_bertanda64_t piksel_diproses;

    /* Status */
    bool_t diinisialisasi;
};

typedef struct penata_konteks penata_konteks_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - INTI
 * ===========================================================================
 */

/* Inisialisasi dan shutdown */
status_t penata_init(penata_konteks_t *ktx,
                     permukaan_t *target);
void penata_shutdown(penata_konteks_t *ktx);
bool_t penata_siap(const penata_konteks_t *ktx);

/* Buat dan hapus konteks */
penata_konteks_t *penata_buat(permukaan_t *target);
void penata_hapus(penata_konteks_t *ktx);

/* Set permukaan target */
status_t penata_set_target(penata_konteks_t *ktx,
                           permukaan_t *target);

/* Proses dan render */
status_t penata_proses(penata_konteks_t *ktx);
status_t penata_render(penata_konteks_t *ktx);

/* Hancurkan semua lapisan */
void penata_hancurkan_semua(penata_konteks_t *ktx);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - LAPISAN
 * ===========================================================================
 */

/* Buat dan hapus lapisan */
penata_lapisan_t *penata_lapisan_buat(penata_konteks_t *ktx);
status_t penata_lapisan_hapus(penata_konteks_t *ktx,
                             tak_bertanda32_t id);

/* Pindah dan ubah ukuran */
status_t penata_lapisan_pindah(penata_konteks_t *ktx,
                               tak_bertanda32_t id,
                               tanda32_t x, tanda32_t y);
status_t penata_lapisan_ubah_ukuran(penata_konteks_t *ktx,
                                    tak_bertanda32_t id,
                                    tak_bertanda32_t lebar,
                                    tak_bertanda32_t tinggi);

/* Set permukaan konten */
status_t penata_lapisan_set_permukaan(penata_konteks_t *ktx,
                                      tak_bertanda32_t id,
                                      permukaan_t *surf);

/* Z-order */
status_t penata_lapisan_naik(penata_konteks_t *ktx,
                             tak_bertanda32_t id);
status_t penata_lapisan_turun(penata_konteks_t *ktx,
                              tak_bertanda32_t id);
status_t penata_lapisan_set_z(penata_konteks_t *ktx,
                              tak_bertanda32_t id,
                              tanda32_t urutan);
status_t penata_lapisan_urutkan(penata_konteks_t *ktx);

/* Flag dan properti */
status_t penata_lapisan_set_flag(penata_konteks_t *ktx,
                                 tak_bertanda32_t id,
                                 tak_bertanda32_t flags);
status_t penata_lapisan_set_alpha(penata_konteks_t *ktx,
                                  tak_bertanda32_t id,
                                  tak_bertanda8_t alpha);

/* Cari lapisan */
penata_lapisan_t *penata_lapisan_cari(penata_konteks_t *ktx,
                                      tak_bertanda32_t id);
penata_lapisan_t *penata_lapisan_teratas(penata_konteks_t *ktx);

/* Validasi */
bool_t penata_lapisan_valid(const penata_lapisan_t *l);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - EFEK
 * ===========================================================================
 */

/* Terapkan efek ke permukaan */
status_t penata_efek_terapkan(const penata_efek_t *efek,
                              permukaan_t *target,
                              const penata_rect_t *area);

/* Blur */
status_t penata_blur_kotak(permukaan_t *target,
                           const penata_rect_t *area,
                           tak_bertanda32_t radius,
                           tak_bertanda32_t iterasi);

/* Bayangan (drop shadow) */
status_t penata_bayangan(permukaan_t *target,
                         const penata_rect_t *area,
                         tak_bertanda32_t offset_x,
                         tak_bertanda32_t offset_y,
                         tak_bertanda32_t radius,
                         tak_bertanda32_t warna);

/* Transparansi (alpha blending) */
status_t penata_campur_alpha(permukaan_t *target,
                             const penata_rect_t *area,
                             tak_bertanda8_t alpha);

/* Animasi */
status_t penata_animasi_mulai(penata_konteks_t *ktx,
                              penata_animasi_t *anim);
status_t penata_animasi_langkah(penata_konteks_t *ktx,
                                tak_bertanda32_t delta_ms);
void penata_animasi_berhenti(penata_animasi_t *anim);
tanda32_t penata_animasi_interpolasi(tanda32_t awal,
                                     tanda32_t akhir,
                                     tak_bertanda32_t t,
                                     tak_bertanda32_t tipe);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - KLIP
 * ===========================================================================
 */

/* Klip titik ke rect */
bool_t penata_klip_titik(const penata_rect_t *klip,
                         tanda32_t x, tanda32_t y);

/* Klip rect ke rect (Cohen-Sutherland) */
bool_t penata_klip_kotak(const penata_rect_t *klip,
                         penata_rect_t *target);

/* Interseksi dua rect */
bool_t penata_intersek(const penata_rect_t *a,
                       const penata_rect_t *b,
                       penata_rect_t *hasil);

/* Union dua rect */
void penata_union(const penata_rect_t *a,
                  const penata_rect_t *b,
                  penata_rect_t *hasil);

/* Region operations */
status_t penata_region_buat(penata_region_t *region);
void penata_region_bersihkan(penata_region_t *region);
status_t penata_region_tambah(penata_region_t *region,
                              const penata_rect_t *rect);
status_t penata_region_klip(penata_region_t *region,
                            const penata_rect_t *klip);
bool_t penata_region_titik_dalam(const penata_region_t *region,
                                 tanda32_t x, tanda32_t y);
tak_bertanda32_t penata_region_dapat_jumlah(
    const penata_region_t *region);

/*
 * ===========================================================================
 * FUNGSI HELPER FIXED-POINT 16.16
 * ===========================================================================
 */

static inline tanda32_t fp_dari_int(tanda32_t nilai)
{
    return nilai << 16;
}

static inline tanda32_t fp_ke_int(tanda32_t fp)
{
    return fp >> 16;
}

static inline tanda32_t fp_kali(tanda32_t a, tanda32_t b)
{
    tanda64_t hasil = ((tanda64_t)a * (tanda64_t)b);
    return (tanda32_t)(hasil >> 16);
}

static inline tanda32_t fp_bagi(tanda32_t a, tanda32_t b)
{
    tanda64_t hasil = ((tanda64_t)a << 16) / (tanda64_t)b;
    return (tanda32_t)hasil;
}

#endif /* PENATA_H */
