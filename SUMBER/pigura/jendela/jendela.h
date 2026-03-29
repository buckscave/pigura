/*
 * PIGURA OS - JENDELA.H
 * ======================
 * Header utama subsistem jendela (window) Pigura OS.
 *
 * Modul jendela menyediakan layanan window manager sederhana:
 *   - Manajemen jendela (buat, hapus, pindah, ubah ukuran)
 *   - Z-order (susunan lapis jendela)
 *   - Routing peristiwa input ke jendela
 *   - Dekorasi jendela (bingkai, judul, tombol)
 *
 * Submodul:
 *   - jendela    : Struktur data dan manajemen jendela
 *   - wm         : Window manager (orkestrasi global)
 *   - dekorasi   : Gambar bingkai, judul, tombol jendela
 *   - peristiwa  : Routing peristiwa input ke jendela
 *   - z_order    : Pengaturan lapis (depth) jendela
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

#ifndef JENDELA_H
#define JENDELA_H

/*
 * ===========================================================================
 * KONSTANTA JENDELA
 * ===========================================================================
 */

/* Magic number modul jendela */
#define JENDELA_MAGIC           0x4A4E4441  /* "JNDA" */

/* Versi modul */
#define JENDELA_VERSI           0x0100

/* Magic number window manager */
#define WM_MAGIC               0x574D414E  /* "WMAN" */

/* Jumlah maksimum jendela */
#define JENDELA_MAKS            64

/* Lebar judul bar default */
#define JENDELA_JUDUL_TINGGI    20

/* Lebar bingkai jendela */
#define JENDELA_BINGKAI_LEBAR    3

/* Ukuran minimum jendela */
#define JENDELA_LEBAR_MIN       32
#define JENDELA_TINGGI_MIN      32

/* Ukuran jendela desktop */
#define JENDELA_DESKTOP_LEBAR    1024
#define JENDELA_DESKTOP_TINGGI   768

/* Warna default */
#define JENDELA_WARNA_LATAR     0xFF000000
#define JENDELA_WARNA_JUDUL     0xFF404040
#define JENDELA_WARNA_BINGKAI    0xFF606060
#define JENDELA_WARNA_DESKTOP   0xFF0080C0
#define JENDELA_WARNA_TUTUP     0xFFC0C0C0
#define JENDELA_WARNA_AKTIF     0xFF000080

/* Status jendela */
#define JENDELA_STATUS_INVISIBLE 0
#define JENDELA_STATUS_VISIBLE   1
#define JENDELA_STATUS_MINIMIZE  2
#define JENDELA_STATUS_MAXIMIZE  3

/* Flag jendela */
#define JENDELA_FLAG_FOKUS      0x01
#define JENDELA_FLAG_MODAL       0x02
#define JENDELA_FLAG_RESIZABLE   0x04
#define JENDELA_FLAG_MOVABLE     0x08
#define JENDELA_FLAG_TERTUTUP    0x10
#define JENDELA_FLAG_DEKORASI    0x20
#define JENDELA_FLAG_AKTIF      0x40
#define JENDELA_FLAG_FULLSCREEN  0x80

/* Komponen dekorasi */
#define JENDELA_DEKOR_JUDUL     0x01
#define JENDELA_DEKOR_BINGKAI    0x02
#define JENDELA_DEKOR_TUTUP     0x04
#define JENDELA_DEKOR_MINIMIZE  0x08
#define JENDELA_DEKOR_MAKSIMAL  0x10

/* Tipe area jendela (untuk hit testing) */
#define JENDELA_AREA_TIDAK_ADA   0
#define JENDELA_AREA_JUDUL      1
#define JENDELA_AREA_BINGKAI    2
#define JENDELA_AREA_ISI        3
#define JENDELA_AREA_TUTUP      4
#define JENDELA_AREA_MINIMIZE  5
#define JENDELA_AREA_MAKSIMAL  6

/* Judul maksimum */
#define JENDELA_JUDUL_PANJANG   128

/*
 * ===========================================================================
 * STRUKTUR PERISTIWA JENDELA
 * ===========================================================================
 */

/* Tipe peristiwa jendela */
#define PERISTIWA_JENDELA_BUAT        0
#define PERISTIWA_JENDELA_HAPUS       1
#define PERISTIWA_JENDELA_PINDAH      2
#define PERISTIWA_JENDELA_UKURAN      3
#define PERISTIWA_JENDELA_FOKUS_MASUK 4
#define PERISTIWA_JENDELA_FOKUS_KELUAR 5
#define PERISTIWA_JENDELA_MINIMIZE   6
#define PERISTIWA_JENDELA_MAKSIMAL   7
#define PERISTIWA_JENDELA_GAMBAR     8
#define PERISTIWA_JENDELA_TUTUP_KLIK 9

/* Peristiwa jendela */
typedef struct peristiwa_jendela {
    tak_bertanda32_t tipe;        /* Tipe peristiwa */
    tak_bertanda32_t id_jendela;  /* ID jendela target */
    tak_bertanda32_t x;           /* Koordinat X */
    tak_bertanda32_t y;           /* Koordinat Y */
    tak_bertanda32_t lebar;       /* Lebar (untuk resize) */
    tak_bertanda32_t tinggi;      /* Tinggi (untuk resize) */
    tak_bertanda32_t tombol;      /* Tombol mouse */
} peristiwa_jendela_t;

/* Panjang antrian peristiwa */
#define PERISTIWA_ANTRIAN_MAKS  256

/*
 * ===========================================================================
 * STRUKTUR RECT (Area Persegi Panjang)
 * ===========================================================================
 */

typedef struct rect {
    tak_bertanda32_t x;       /* Koordinat X */
    tak_bertanda32_t y;       /* Koordinat Y */
    tak_bertanda32_t lebar;   /* Lebar */
    tak_bertanda32_t tinggi;  /* Tinggi */
} rect_t;

/*
 * ===========================================================================
 * FORWARD DECLARATION PERMUKAAN
 * ===========================================================================
 * struct permukaan didefinisikan lengkap di framebuffer.h.
 * Di sini hanya perlu forward declaration untuk pointer.
 */

struct permukaan;

typedef struct permukaan permukaan_t;

/*
 * ===========================================================================
 * STRUKTUR JENDELA (lengkap)
 * ===========================================================================
 *
 * Definisi lengkap struct jendela agar semua submodul
 * (peristiwa, wm, dekorasi, z_order) dapat mengakses
 * anggota struct secara langsung.
 */

struct jendela {
    tak_bertanda32_t magic;
    tak_bertanda32_t id;             /* ID unik jendela */
    char judul[JENDELA_JUDUL_PANJANG];

    /* Posisi dan ukuran */
    tak_bertanda32_t x;
    tak_bertanda32_t y;
    tak_bertanda32_t lebar;
    tak_bertanda32_t tinggi;

    /* Status dan flags */
    tak_bertanda8_t  status;
    tak_bertanda32_t flags;

    /* Warna */
    tak_bertanda32_t warna_judul;
    tak_bertanda32_t warna_bingkai;
    tak_bertanda32_t warna_latar;

    /* Z-order */
    tak_bertanda32_t z_urutan;

    /* Properti jendela */
    permukaan_t *permukaan_konten;  /* Buffer konten */
};

typedef struct jendela jendela_t;

/*
 * ===========================================================================
 * STRUKTUR Z-ORDER NODE
 * ===========================================================================
 */

typedef struct z_node {
    jendela_t *jendela;           /* Pointer ke jendela */
    tak_bertanda32_t urutan;      /* Urutan z (0 = bawah) */
    struct z_node *atas;         /* Node di atas */
    struct z_node *bawah;        /* Node di bawah */
} z_node_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - JENDELA (MANAJEMEN)
 * ===========================================================================
 */

/* Inisialisasi subsistem jendela */
status_t jendela_init(void);

/* Shutdown subsistem jendela */
void jendela_shutdown(void);

/* Buat jendela baru */
jendela_t *jendela_buat(tak_bertanda32_t x, tak_bertanda32_t y,
                         tak_bertanda32_t lebar,
                         tak_bertanda32_t tinggi,
                         const char *judul,
                         tak_bertanda32_t flags);

/* Hapus jendela */
status_t jendela_hapus(jendela_t *j);

/* Dapatkan ID jendela */
tak_bertanda32_t jendela_dapat_id(const jendela_t *j);

/* Dapatkan judul jendela */
const char *jendela_dapat_judul(const jendela_t *j);

/* Dapatkan posisi jendela */
status_t jendela_dapat_posisi(const jendela_t *j,
                               tak_bertanda32_t *x,
                               tak_bertanda32_t *y);

/* Dapatkan ukuran jendela */
status_t jendela_dapat_ukuran(const jendela_t *j,
                               tak_bertanda32_t *lebar,
                               tak_bertanda32_t *tinggi);

/* Pindah jendela */
status_t jendela_pindah(jendela_t *j,
                          tak_bertanda32_t x,
                          tak_bertanda32_t y);

/* Ubah ukuran jendela */
status_t jendela_ubah_ukuran(jendela_t *j,
                               tak_bertanda32_t lebar,
                               tak_bertanda32_t tinggi);

/* Set judul jendela */
status_t jendela_set_judul(jendela_t *j, const char *judul);

/* Set flags jendela */
status_t jendela_set_flags(jendela_t *j,
                             tak_bertanda32_t flags);

/* Set status jendela */
status_t jendela_set_status(jendela_t *j,
                             tak_bertanda8_t status);

/* Set warna jendela */
status_t jendela_set_warna(jendela_t *j,
                            tak_bertanda32_t judul,
                            tak_bertanda32_t bingkai,
                            tak_bertanda32_t latar);

/* Set visibilitas */
status_t jendela_set_visible(jendela_t *j, bool_t visible);

/* Cek apakah jendela valid */
bool_t jendela_valid(const jendela_t *j);

/* Dapatkan area jendela di koordinat layar */
status_t jendela_dapat_area_layar(const jendela_t *j,
                                   rect_t *area);

/* Dapatkan area konten (tanpa dekorasi) */
status_t jendela_dapat_area_konten(const jendela_t *j,
                                    rect_t *area);

/* Hit test: koindekat mana di jendela? */
tak_bertanda32_t jendela_hit_test(const jendela_t *j,
                                  tak_bertanda32_t x,
                                  tak_bertanda32_t y);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - WINDOW MANAGER
 * ===========================================================================
 */

/* Inisialisasi window manager */
status_t wm_init(void);

/* Shutdown window manager */
void wm_shutdown(void);

/* Proses satu langkah window manager */
status_t wm_langkah(void);

/* Set jendela desktop */
status_t wm_set_desktop(permukaan_t *permukaan);

/* Dapatkan jendela yang sedang fokus */
jendela_t *wm_dapat_fokus(void);

/* Set fokus ke jendela */
status_t wm_set_fokus(jendela_t *j);

/* Cari jendela berdasarkan ID */
jendela_t *wm_cari_id(tak_bertanda32_t id);

/* Kirim peristiwa ke window manager */
status_t wm_kirim_peristiwa(const peristiwa_jendela_t *evt);

/* Dapatkan jumlah jendela aktif */
tak_bertanda32_t wm_dapat_jumlah_aktif(void);

/* Render semua jendela ke desktop */
status_t wm_render_semua(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - DEKORASI
 * ===========================================================================
 */

/* Gambar dekorasi jendela ke permukaan */
status_t dekorasi_gambar(jendela_t *j,
                          permukaan_t *target);

/* Gambar judul bar jendela */
status_t dekorasi_gambar_judul(jendela_t *j,
                                permukaan_t *target);

/* Gambar bingkai jendela */
status_t dekorasi_gambar_bingkai(jendela_t *j,
                                  permukaan_t *target);

/* Gambar tombol tutup */
status_t dekorasi_gambar_tutup(jendela_t *j,
                                permukaan_t *target);

/* Gambar tombol minimize/maximize */
status_t dekorasi_gambar_kontrol(jendela_t *j,
                                  permukaan_t *target);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - PERISTIWA
 * ===========================================================================
 */

/* Inisialisasi subsistem peristiwa */
status_t peristiwa_init(void);

/* Shutdown subsistem peristiwa */
void peristiwa_shutdown(void);

/* Push peristiwa ke antrian */
status_t peristiwa_push(const peristiwa_jendela_t *evt);

/* Pop peristiwa dari antrian */
status_t peristiwa_pop(peristiwa_jendela_t *evt);

/* Route peristiwa mouse ke jendela */
jendela_t *peristiwa_route_mouse(tak_bertanda32_t x,
                                   tak_bertanda32_t y,
                                   tak_bertanda32_t tombol);

/* Route peristiwa keyboard ke jendela fokus */
jendela_t *peristiwa_route_keyboard(void);

/* Proses semua peristiwa dalam antrian */
status_t peristiwa_proses_semua(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - Z-ORDER
 * ===========================================================================
 */

/* Inisialisasi subsistem z-order */
status_t z_order_init(void);

/* Tambah jendela ke z-order */
status_t z_order_tambah(jendela_t *j);

/* Hapus jendela dari z-order */
status_t z_order_hapus(jendela_t *j);

/* Naikkan jendela ke atas */
status_t z_order_naik(jendela_t *j);

/* Turunkan jendela ke bawah */
status_t z_order_turun(jendela_t *j);

/* Dapatkan jendela paling atas */
jendela_t *z_order_dapat_atas(void);

/* Dapatkan jendela berikutnya dari atas */
jendela_t *z_order_dapat_berikutnya(jendela_t *j);

/* Dapatkan urutan z jendela */
tak_bertanda32_t z_order_dapat_urutan(const jendela_t *j);

/* Set urutan z jendela */
status_t z_order_set_urutan(jendela_t *j,
                              tak_bertanda32_t urutan);

/* Hitung jumlah jendela dalam z-order */
tak_bertanda32_t z_order_hitung(void);

/* Bersihkan z-order (hapus semua) */
void z_order_bersihkan(void);

#endif /* JENDELA_H */
