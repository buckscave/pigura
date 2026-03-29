/*
 * PIGURA OS - WIDGET.H
 * =====================
 * Header utama subsistem widget Pigura OS.
 *
 * Subsistem widget menyediakan komponen GUI yang dapat
 * digunakan kembali (reusable). Setiap widget memiliki:
 *   - Struktur dasar (widget_t) dengan properti umum
 *   - Data spesifik per tipe widget
 *   - Fungsi render dan event handler
 *   - Manajemen siklus hidup (buat, pakai, hapus)
 *
 * Tipe widget yang tersedia:
 *   - tombol        : Tombol tekan
 *   - kotak_teks    : Kotak input teks
 *   - kotak_centang : Kotak centang (checkbox)
 *   - bar_gulir     : Bar gulir (scrollbar)
 *   - menu          : Menu tarik-turun
 *   - dialog        : Kotak dialog
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

#ifndef WIDGET_H
#define WIDGET_H

#include "../inti/kernel.h"
#include "framebuffer/framebuffer.h"
#include "../peristiwa/peristiwa.h"

/*
 * ===========================================================================
 * KONSTANTA WIDGET
 * ===========================================================================
 */

/* Magic number modul widget */
#define WIDGET_MAGIC             0x57444754  /* "WDGT" */

/* Jumlah maksimum widget yang dapat terdaftar */
#define WIDGET_MAKS              128

/* Panjang maksimum teks label */
#define WIDGET_LABEL_PANJANG     128

/* Panjang maksimum teks input */
#define WIDGET_TEKS_PANJANG      256

/* Tinggi default bar judul widget */
#define WIDGET_JUDUL_TINGGI      20

/* Lebar default border widget */
#define WIDGET_BORDER_LEBAR      1

/* Padding default dalam widget */
#define WIDGET_PADDING           4

/* Ukuran minimum widget */
#define WIDGET_LEBAR_MIN         16
#define WIDGET_TINGGI_MIN        16

/* Ukuran default checkbox */
#define WIDGET_CENTANG_UKURAN    14

/* Ukuran default scrollbar handle */
#define WIDGET_GULIR_HANDLE_MIN  16

/* Jumlah maksimum item menu */
#define WIDGET_MENU_ITEM_MAKS    32

/* Tinggi item menu default */
#define WIDGET_MENU_TINGGI_ITEM  22

/* Jumlah maksimum tombol dialog */
#define WIDGET_DIALOG_TOMBOL_MAKS 4

/* Tinggi default dialog */
#define WIDGET_DIALOG_TINGGI     120

/* Panjang maksimum judul dialog */
#define WIDGET_DIALOG_JUDUL_PANJANG 64

/*
 * ===========================================================================
 * TIPE WIDGET
 * ===========================================================================
 */

/* Tipe tidak ada / kosong */
#define WIDGET_TIDAK_ADA         0

/* Tombol tekan */
#define WIDGET_TOMBOL            1

/* Kotak input teks */
#define WIDGET_KOTAK_TEKS        2

/* Kotak centang (checkbox) */
#define WIDGET_KOTAK_CENTANG     3

/* Bar gulir (scrollbar) */
#define WIDGET_BAR_GULIR         4

/* Menu tarik-turun */
#define WIDGET_MENU              5

/* Kotak dialog */
#define WIDGET_DIALOG            6

/* Jumlah total tipe widget */
#define WIDGET_TIPE_JUMLAH       7

/*
 * ===========================================================================
 * FLAG WIDGET
 * ===========================================================================
 */

/* Widget terlihat (visible) */
#define WIDGET_FLAG_TERLIHAT     0x0001

/* Widget aktif (enabled) */
#define WIDGET_FLAG_AKTIF        0x0002

/* Widget memiliki fokus */
#define WIDGET_FLAG_FOKUS        0x0004

/* Widget perlu di-render ulang (dirty) */
#define WIDGET_FLAG_KOTOR        0x0008

/* Widget di-klik (pressed state) */
#define WIDGET_FLAG_TEKAN        0x0010

/* Widget hover (kursor di atas) */
#define WIDGET_FLAG_ARAH         0x0020

/* Widget disabled (abu-abu, tidak responsif) */
#define WIDGET_FLAG_NONAKTIF     0x0040

/* Widget modal (tangkap semua event) */
#define WIDGET_FLAG_MODAL        0x0080

/* Widget berubah ukuran otomatis */
#define WIDGET_FLAG_AUTO_UKURAN  0x0100

/* Widget memiliki border */
#define WIDGET_FLAG_BORDER       0x0200

/* Widget transparan */
#define WIDGET_FLAG_TRANSPARAN   0x0400

/*
 * ===========================================================================
 * WARNA DEFAULT WIDGET
 * ===========================================================================
 */

/* Warna latar widget default */
#define WIDGET_WARNA_LATAR       0xFFE0E0E0

/* Warna latar tombol default */
#define WIDGET_WARNA_TOMBOL      0xFFD0D0D0

/* Warna tombol ditekan */
#define WIDGET_WARNA_TEKAN       0xFFA0A0A0

/* Warna border */
#define WIDGET_WARNA_BORDER      0xFF808080

/* Warna teks */
#define WIDGET_WARNA_TEKS        0xFF000000

/* Warna teks disabled */
#define WIDGET_WARNA_TEKS_NONAKTIF 0xFF808080

/* Warna fokus ring */
#define WIDGET_WARNA_FOKUS       0xFF0066CC

/* Warna checkbox centang */
#define WIDGET_WARNA_CENTANG     0xFF006600

/* Warna scrollbar track */
#define WIDGET_WARNA_GULIR_LINTASAN 0xFFC8C8C8

/* Warna scrollbar handle */
#define WIDGET_WARNA_GULIR_HANDLE   0xFFA0A0A0

/* Warna menu latar */
#define WIDGET_WARNA_MENU_LATAR  0xFFF0F0F0

/* Warna menu item hover */
#define WIDGET_WARNA_MENU_ARAH   0xFFD0E8FF

/* Warna dialog latar */
#define WIDGET_WARNA_DIALOG_LATAR  0xFFF0F0F0

/*
 * ===========================================================================
 * STRUKTUR EVENT WIDGET
 * ===========================================================================
 *
 * Event yang diterima oleh widget dari sistem peristiwa.
 */

/* Tipe event widget */
#define WEVENT_TIDAK_ADA         0
#define WEVENT_KLIK              1
#define WEVENT_TEKAN             2
#define WEVENT_LEPAS             3
#define WEVENT_GERAK             4
#define WEVENT_ARAH_MASUK        5
#define WEVENT_ARAH_KELUAR       6
#define WEVENT_FOKUS_MASUK       7
#define WEVENT_FOKUS_KELUAR      8
#define WEVENT_KUNCI_TEKAN       9
#define WEVENT_KUNCI_LEPAS       10
#define WEVENT_KUNCI_ULANG       11
#define WEVENT_GULIR             12
#define WEVENT_KONFIRMASI        13
#define WEVENT_BATAL             14

typedef struct {
    tak_bertanda32_t tipe;
    tak_bertanda32_t id_widget;
    tak_bertanda32_t x;
    tak_bertanda32_t y;
    tak_bertanda32_t tombol;
    tak_bertanda32_t kode_kunci;
    tak_bertanda32_t modifier;
    tanda32_t gulir_nilai;
    tanda32_t gulir_delta;
} widget_event_t;

/*
 * ===========================================================================
 * CALLBACK WIDGET
 * ===========================================================================
 *
 * Fungsi callback untuk event dan render widget.
 * Menggunakan pointer ke void agar generik.
 */

/* Callback event: menerima event dan data pengguna */
typedef void (*widget_event_cb)(const widget_event_t *evt,
                                void *data);

/* Callback render: menggambar widget ke permukaan */
typedef status_t (*widget_render_cb)(void *widget_data,
                                     permukaan_t *target,
                                     tak_bertanda32_t ox,
                                     tak_bertanda32_t oy);

/*
 * ===========================================================================
 * STRUKTUR DASAR WIDGET (BASE)
 * ===========================================================================
 *
 * Semua tipe widget mewarisi field dari struktur ini
 * sebagai anggota pertama (embedded struct pattern).
 */

struct widget {
    /* Identifikasi */
    tak_bertanda32_t magic;
    tak_bertanda32_t id;
    tak_bertanda32_t tipe;

    /* Posisi dan ukuran */
    tanda32_t x;
    tanda32_t y;
    tak_bertanda32_t lebar;
    tak_bertanda32_t tinggi;

    /* Flag dan status */
    tak_bertanda32_t flags;

    /* Warna */
    tak_bertanda32_t warna_latar;
    tak_bertanda32_t warna_border;
    tak_bertanda32_t warna_teks;

    /* Callback */
    widget_event_cb event_cb;
    void *event_data;

    /* Data spesifik widget (cast ke tipe konkret) */
    void *data;

    /* Pointer ke parent widget (NULL jika top-level) */
    struct widget *parent;
};

typedef struct widget widget_t;

/*
 * ===========================================================================
 * STRUKTUR DATA SPESIFIK - TOMBOL
 * ===========================================================================
 */

struct widget_tombol_data {
    /* Label teks tombol */
    char label[WIDGET_LABEL_PANJANG];

    /* Warna tombol saat normal */
    tak_bertanda32_t warna_normal;

    /* Warna tombol saat ditekan */
    tak_bertanda32_t warna_tekan;

    /* Warna tombol saat hover */
    tak_bertanda32_t warna_arah;

    /* Counter klik (statistik) */
    tak_bertanda64_t jumlah_klik;
};

typedef struct widget_tombol_data widget_tombol_data_t;

/*
 * ===========================================================================
 * STRUKTUR DATA SPESIFIK - KOTAK TEKS
 * ===========================================================================
 */

struct widget_kotak_teks_data {
    /* Buffer teks input */
    char teks[WIDGET_TEKS_PANJANG];

    /* Jumlah karakter dalam buffer */
    tak_bertanda32_t panjang;

    /* Posisi kursor (caret) dalam buffer */
    tak_bertanda32_t posisi_kursor;

    /* Posisi scroll teks (karakter pertama terlihat) */
    tak_bertanda32_t posisi_scroll;

    /* Maksimum karakter yang dapat dimasukkan */
    tak_bertanda32_t panjang_maks;

    /* Placeholder teks (hint) */
    char placeholder[WIDGET_LABEL_PANJANG];

    /* Mode password (tampilkan karakter *) */
    bool_t mode_sandi;

    /* Mode hanya-baca */
    bool_t hanya_baca;

    /* Apakah teks dipilih (selection) */
    bool_t terpilih;

    /* Offset awal dan akhir seleksi */
    tak_bertanda32_t seleksi_awal;
    tak_bertanda32_t seleksi_akhir;

    /* Flag apakah kursor terlihat (blink) */
    bool_t kursor_tampil;

    /* Waktu blink kursor terakhir */
    tak_bertanda64_t kursor_waktu;
};

typedef struct widget_kotak_teks_data
    widget_kotak_teks_data_t;

/*
 * ===========================================================================
 * STRUKTUR DATA SPESIFIK - KOTAK CENTANG
 * ===========================================================================
 */

struct widget_kotak_centang_data {
    /* Label teks checkbox */
    char label[WIDGET_LABEL_PANJANG];

    /* Status tercentang atau tidak */
    bool_t tercentang;

    /* Warna centang */
    tak_bertanda32_t warna_centang;

    /* Ukuran kotak centang */
    tak_bertanda32_t ukuran;

    /* Status tristate: 0=off, 1=on, 2=indeterminate */
    tak_bertanda8_t status_tristate;
};

typedef struct widget_kotak_centang_data
    widget_kotak_centang_data_t;

/*
 * ===========================================================================
 * STRUKTUR DATA SPESIFIK - BAR GULIR
 * ===========================================================================
 */

struct widget_bar_gulir_data {
    /* Orientasi: 0=vertikal, 1=horizontal */
    tak_bertanda32_t orientasi;

    /* Nilai minimum */
    tanda32_t nilai_min;

    /* Nilai maksimum */
    tanda32_t nilai_maks;

    /* Nilai saat ini */
    tanda32_t nilai_sekarang;

    /* Ukuran satu langkah (step) */
    tanda32_t langkah;

    /* Ukuran halaman (page) */
    tanda32_t ukuran_halaman;

    /* Lebar handle */
    tak_bertanda32_t handle_lebar;

    /* Posisi handle relatif terhadap track */
    tak_bertanda32_t handle_posisi;

    /* Flag sedang di-drag */
    bool_t sedang_drag;

    /* Offset drag awal */
    tanda32_t drag_offset;
};

typedef struct widget_bar_gulir_data
    widget_bar_gulir_data_t;

/*
 * ===========================================================================
 * STRUKTUR ITEM MENU
 * ===========================================================================
 */

struct widget_menu_item {
    /* Label teks item */
    char label[WIDGET_LABEL_PANJANG];

    /* ID unik item */
    tak_bertanda32_t id;

    /* Apakah item enabled */
    bool_t aktif;

    /* Apakah item merupakan separator */
    bool_t pemisah;

    /* Apakah item memiliki sub-menu */
    bool_t punya_submenu;

    /* Apakah item memiliki centang */
    bool_t tercentang;

    /* Data pengguna untuk callback */
    void *data;

    /* Callback saat item dipilih */
    widget_event_cb callback;
};

typedef struct widget_menu_item widget_menu_item_t;

/*
 * ===========================================================================
 * STRUKTUR DATA SPESIFIK - MENU
 * ===========================================================================
 */

struct widget_menu_data {
    /* Daftar item menu */
    widget_menu_item_t item[WIDGET_MENU_ITEM_MAKS];

    /* Jumlah item aktif */
    tak_bertanda32_t jumlah_item;

    /* Indeks item yang dipilih (hover) */
    tanda32_t indeks_arah;

    /* Indeks item terakhir yang diklik */
    tanda32_t indeks_terpilih;

    /* Menu terbuka atau tertutup */
    bool_t terbuka;

    /* Offset scroll menu */
    tak_bertanda32_t scroll_offset;

    /* Jumlah item yang terlihat */
    tak_bertanda32_t item_tampak;
};

typedef struct widget_menu_data widget_menu_data_t;

/*
 * ===========================================================================
 * STRUKTUR TOMBOL DIALOG
 * ===========================================================================
 */

struct widget_dialog_tombol {
    /* Label tombol */
    char label[WIDGET_LABEL_PANJANG];

    /* ID tombol */
    tak_bertanda32_t id;

    /* Apakah ini tombol default */
    bool_t default_tombol;

    /* Callback saat tombol ditekan */
    widget_event_cb callback;

    /* Data pengguna untuk callback */
    void *data;
};

typedef struct widget_dialog_tombol
    widget_dialog_tombol_t;

/*
 * ===========================================================================
 * STRUKTUR DATA SPESIFIK - DIALOG
 * ===========================================================================
 */

struct widget_dialog_data {
    /* Judul dialog */
    char judul[WIDGET_DIALOG_JUDUL_PANJANG];

    /* Pesan dialog */
    char pesan[WIDGET_TEKS_PANJANG];

    /* Daftar tombol dialog */
    widget_dialog_tombol_t tombol
        [WIDGET_DIALOG_TOMBOL_MAKS];

    /* Jumlah tombol aktif */
    tak_bertanda32_t jumlah_tombol;

    /* Indeks tombol default */
    tanda32_t indeks_default;

    /* Tipe dialog: 0=info, 1=peringatan, 2=error */
    tak_bertanda32_t tipe;

    /* Flag modal */
    bool_t modal;
};

typedef struct widget_dialog_data widget_dialog_data_t;

/*
 * ===========================================================================
 * STRUKTUR KONTEKS MANAJER WIDGET
 * ===========================================================================
 */

struct widget_konteks {
    /* Magic number */
    tak_bertanda32_t magic;

    /* Array widget terdaftar */
    widget_t *daftar[WIDGET_MAKS];

    /* Jumlah widget aktif */
    tak_bertanda32_t jumlah;

    /* Counter ID unik widget */
    tak_bertanda32_t id_counter;

    /* ID widget yang sedang fokus */
    tak_bertanda32_t id_fokus;

    /* ID widget yang sedang ditekan */
    tak_bertanda32_t id_tekan;

    /* ID widget yang sedang di-arahkan */
    tak_bertanda32_t id_arah;

    /* Flag modul aktif */
    bool_t diinisialisasi;

    /* Statistik */
    tak_bertanda64_t total_render;
    tak_bertanda64_t total_event;
    tak_bertanda64_t total_klik;
};

typedef struct widget_konteks widget_konteks_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - MANAJER WIDGET (widget.c)
 * ===========================================================================
 */

/* Inisialisasi manajer widget */
status_t widget_init(void);

/* Shutdown manajer widget */
void widget_shutdown(void);

/* Buat widget baru (alokasi + registrasi) */
widget_t *widget_buat(tak_bertanda32_t tipe,
                      tanda32_t x, tanda32_t y,
                      tak_bertanda32_t lebar,
                      tak_bertanda32_t tinggi);

/* Hapus widget (dealokasi + unreg) */
status_t widget_hapus(widget_t *w);

/* Hapus semua widget */
void widget_hapus_semua(void);

/* Cari widget berdasarkan ID */
widget_t *widget_cari_id(tak_bertanda32_t id);

/* Hit test: widget mana di posisi (x,y)? */
widget_t *widget_hit_test(tak_bertanda32_t x,
                           tak_bertanda32_t y);

/* Set fokus ke widget */
status_t widget_set_fokus(widget_t *w);

/* Dapatkan widget fokus */
widget_t *widget_dapat_fokus(void);

/* Kirim event ke widget target */
status_t widget_kirim_event(widget_t *w,
                             const widget_event_t *evt);

/* Render semua widget ke permukaan */
status_t widget_render_semua(permukaan_t *target);

/* Render satu widget */
status_t widget_render(widget_t *w,
                        permukaan_t *target,
                        tak_bertanda32_t ox,
                        tak_bertanda32_t oy);

/* Set callback event widget */
void widget_set_callback(widget_t *w,
                          widget_event_cb cb,
                          void *data);

/* Cek apakah widget valid */
bool_t widget_valid(const widget_t *w);

/* Set posisi widget */
status_t widget_set_posisi(widget_t *w,
                            tanda32_t x, tanda32_t y);

/* Set ukuran widget */
status_t widget_set_ukuran(widget_t *w,
                            tak_bertanda32_t lebar,
                            tak_bertanda32_t tinggi);

/* Set flag widget */
void widget_set_flag(widget_t *w,
                      tak_bertanda32_t flag);

/* Hapus flag widget */
void widget_hapus_flag(widget_t *w,
                        tak_bertanda32_t flag);

/* Dapatkan jumlah widget aktif */
tak_bertanda32_t widget_dapat_jumlah(void);

/* Dapatkan statistik */
void widget_dapat_statistik(
    tak_bertanda64_t *total_render,
    tak_bertanda64_t *total_event,
    tak_bertanda64_t *total_klik);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - TOMBOL (tombol.c)
 * ===========================================================================
 */

/* Inisialisasi data tombol */
status_t tombol_init_data(widget_t *w,
                            const char *label);

/* Set label tombol */
status_t tombol_set_label(widget_t *w,
                           const char *label);

/* Dapatkan label tombol */
const char *tombol_dapat_label(const widget_t *w);

/* Set warna tombol */
void tombol_set_warna(widget_t *w,
                       tak_bertanda32_t normal,
                       tak_bertanda32_t tekan,
                       tak_bertanda32_t arah);

/* Render tombol */
status_t tombol_render(widget_t *w,
                        permukaan_t *target,
                        tak_bertanda32_t ox,
                        tak_bertanda32_t oy);

/* Proses event tombol */
void tombol_proses_event(widget_t *w,
                          const widget_event_t *evt);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - KOTAK TEKS (kotakteks.c)
 * ===========================================================================
 */

/* Inisialisasi data kotak teks */
status_t kotak_teks_init_data(widget_t *w,
                               const char *placeholder,
                               tak_bertanda32_t panjang_maks);

/* Set teks */
status_t kotak_teks_set_teks(widget_t *w,
                               const char *teks);

/* Dapatkan teks */
const char *kotak_teks_dapat_teks(const widget_t *w);

/* Masukkan karakter */
status_t kotak_teks_masukkan(widget_t *w, char c);

/* Hapus karakter sebelum kursor */
status_t kotak_teks_hapus(widget_t *w);

/* Hapus semua teks */
void kotak_teks_bersihkan(widget_t *w);

/* Set placeholder */
void kotak_teks_set_placeholder(widget_t *w,
                                 const char *teks);

/* Set mode sandi */
void kotak_teks_set_mode_sandi(widget_t *w,
                                bool_t sandi);

/* Set hanya baca */
void kotak_teks_set_hanya_baca(widget_t *w,
                                bool_t hanya_baca);

/* Render kotak teks */
status_t kotak_teks_render(widget_t *w,
                            permukaan_t *target,
                            tak_bertanda32_t ox,
                            tak_bertanda32_t oy);

/* Proses event kotak teks */
void kotak_teks_proses_event(widget_t *w,
                              const widget_event_t *evt);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - KOTAK CENTANG (kotakcentang.c)
 * ===========================================================================
 */

/* Inisialisasi data kotak centang */
status_t kotak_centang_init_data(widget_t *w,
                                  const char *label);

/* Set label */
status_t kotak_centang_set_label(widget_t *w,
                                  const char *label);

/* Dapatkan label */
const char *kotak_centang_dapat_label(
    const widget_t *w);

/* Toggle status centang */
void kotak_centang_toggle(widget_t *w);

/* Set status centang */
void kotak_centang_set_status(widget_t *w, bool_t val);

/* Dapatkan status centang */
bool_t kotak_centang_dapat_status(const widget_t *w);

/* Set warna centang */
void kotak_centang_set_warna(widget_t *w,
                              tak_bertanda32_t warna);

/* Render kotak centang */
status_t kotak_centang_render(widget_t *w,
                               permukaan_t *target,
                               tak_bertanda32_t ox,
                               tak_bertanda32_t oy);

/* Proses event kotak centang */
void kotak_centang_proses_event(widget_t *w,
                                 const widget_event_t *evt);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - BAR GULIR (bargulir.c)
 * ===========================================================================
 */

/* Inisialisasi data bar gulir */
status_t bar_gulir_init_data(widget_t *w,
                              tak_bertanda32_t orientasi,
                              tanda32_t nilai_min,
                              tanda32_t nilai_maks);

/* Set rentang nilai */
void bar_gulir_set_rentang(widget_t *w,
                            tanda32_t min_val,
                            tanda32_t max_val);

/* Set nilai saat ini */
status_t bar_gulir_set_nilai(widget_t *w,
                               tanda32_t nilai);

/* Dapatkan nilai saat ini */
tanda32_t bar_gulir_dapat_nilai(const widget_t *w);

/* Set ukuran halaman */
void bar_gulir_set_halaman(widget_t *w,
                            tanda32_t ukuran);

/* Set langkah */
void bar_gulir_set_langkah(widget_t *w,
                            tanda32_t langkah);

/* Render bar gulir */
status_t bar_gulir_render(widget_t *w,
                           permukaan_t *target,
                           tak_bertanda32_t ox,
                           tak_bertanda32_t oy);

/* Proses event bar gulir */
void bar_gulir_proses_event(widget_t *w,
                             const widget_event_t *evt);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - MENU (menu.c)
 * ===========================================================================
 */

/* Inisialisasi data menu */
status_t menu_init_data(widget_t *w);

/* Tambah item ke menu */
status_t menu_tambah_item(widget_t *w,
                           const char *label,
                           tak_bertanda32_t id,
                           bool_t aktif);

/* Hapus item dari menu */
status_t menu_hapus_item(widget_t *w,
                          tak_bertanda32_t id);

/* Set callback item */
void menu_set_callback(widget_t *w,
                        tak_bertanda32_t id,
                        widget_event_cb cb,
                        void *data);

/* Buka/tutup menu */
void menu_set_terbuka(widget_t *w, bool_t terbuka);

/* Dapatkan status terbuka */
bool_t menu_dapat_terbuka(const widget_t *w);

/* Tambah separator */
status_t menu_tambah_pemisah(widget_t *w);

/* Render menu */
status_t menu_render(widget_t *w,
                      permukaan_t *target,
                      tak_bertanda32_t ox,
                      tak_bertanda32_t oy);

/* Proses event menu */
void menu_proses_event(widget_t *w,
                        const widget_event_t *evt);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - DIALOG (dialog.c)
 * ===========================================================================
 */

/* Inisialisasi data dialog */
status_t dialog_init_data(widget_t *w,
                           const char *judul,
                           const char *pesan);

/* Set judul dialog */
status_t dialog_set_judul(widget_t *w,
                           const char *judul);

/* Set pesan dialog */
status_t dialog_set_pesan(widget_t *w,
                           const char *pesan);

/* Tambah tombol dialog */
status_t dialog_tambah_tombol(widget_t *w,
                                const char *label,
                                tak_bertanda32_t id,
                                widget_event_cb cb,
                                bool_t default_btn);

/* Hitung ulang tata letak dialog */
status_t dialog_hitung_layout(widget_t *w);

/* Set tipe dialog (info/warning/error) */
void dialog_set_tipe(widget_t *w,
                      tak_bertanda32_t tipe);

/* Render dialog */
status_t dialog_render(widget_t *w,
                        permukaan_t *target,
                        tak_bertanda32_t ox,
                        tak_bertanda32_t oy);

/* Proses event dialog */
void dialog_proses_event(widget_t *w,
                          const widget_event_t *evt);

#endif /* WIDGET_H */
