/*******************************************************************************
 * PIGURA.H - Header Utama Sistem Operasi Pigura
 *
 * Sistem Operasi Pigura - OS berbasis framebuffer dengan kernel Linux
 * Bahasa: C89 + POSIX.1-2008
 *
 * Header ini berisi semua deklarasi publik dari modul-modul sistem.
 *
 * Copyright (c) 2024 Pigura OS Project
 ******************************************************************************/

#ifndef PIGURA_H
#define PIGURA_H

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 600

/*******************************************************************************
 * BAGIAN 1: INCLUDE STANDAR
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <signal.h>
#include <errno.h>
#include <stddef.h>      /* untuk size_t */
#include <sys/ioctl.h>
#include <sys/mman.h>   /* untuk pid_t */
#include <sys/stat.h>
#include <sys/select.h>   /* untuk pid_t */
#include <sys/types.h>   /* untuk pid_t */
#include <sys/time.h>    /* untuk struct timeval */
#include <sys/wait.h>    /* untuk struct timeval */
#include <termios.h>     /* untuk struct termios */
#include <time.h>     /* untuk struct termios */
/*******************************************************************************
 * BAGIAN 2: KONSTANTA GLOBAL
 ******************************************************************************/

/* Versi sistem */
#define PIGURA_VERSI_UTAMA    0
#define PIGURA_VERSI_MINOR    1
#define PIGURA_VERSI_TAMBAHAN 0

/* Ukuran font standar */
#define HURUF_LEBAR           8
#define HURUF_TINGGI          16

/* Batas maksimum */
#define MAKS_JALUR            256
#define MAKS_ARGUMEN          64
#define MAKS_LINGKUNGAN       128
#define MAKS_BUFFER           4096
#define MAKS_ENTRI_KLIP       16

/*******************************************************************************
 * BAGIAN 3: TIPE DATA DASAR
 ******************************************************************************/

/* Struktur warna RGB */
typedef struct {
    unsigned char r;  /* Komponen merah (0-255) */
    unsigned char g;  /* Komponen hijau (0-255) */
    unsigned char b;  /* Komponen biru (0-255) */
} WarnaRGB;

/* Struktur informasi framebuffer */
typedef struct {
    void *ptr;         /* Pointer ke memori framebuffer */
    int fd;            /* File descriptor framebuffer */
    int lebar;         /* Lebar layar dalam piksel */
    int tinggi;        /* Tinggi layar dalam piksel */
    int bpp;           /* Bit per piksel (16, 24, atau 32) */
    int panjang_baris; /* Panjang satu baris dalam byte */
    size_t ukuran_mem; /* Ukuran memori framebuffer */
} InfoFramebuffer;

/*******************************************************************************
 * BAGIAN 4: MODUL HURUF (FONT RENDERING)
 ******************************************************************************/

/* Inisialisasi modul huruf */
extern int huruf_init(void);

/* Gambar satu karakter ASCII */
extern void huruf_gambar_karakter(InfoFramebuffer *fb, int x, int y,
    char ch, WarnaRGB latar, WarnaRGB belakang);

/* Gambar string ASCII */
extern void huruf_gambar_string(InfoFramebuffer *fb, int x, int y,
    const char *str, WarnaRGB latar, WarnaRGB belakang);

/* Hitung lebar teks dalam piksel */
extern int huruf_lebar_teks(const char *str);

/* Hitung tinggi teks dalam piksel */
extern int huruf_tinggi_teks(const char *str);

/*******************************************************************************
 * BAGIAN 5: MODUL UNIKODE (UNICODE SUPPORT)
 ******************************************************************************/

/* Jumlah karakter Unicode yang didukung */
#define UNIKODE_JUMLAH_KARAKTER 256

/* Konversi UTF-8 ke codepoint */
extern int unikode_utf8_ke_codepoint(const char *str, unsigned int *codepoint);

/* Dapatkan data font dari codepoint */
extern const unsigned char* unikode_data_font(unsigned int codepoint);

/* Gambar satu karakter Unicode */
extern void unikode_gambar_karakter(InfoFramebuffer *fb, int x, int y,
    unsigned int codepoint, WarnaRGB latar, WarnaRGB belakang);

/* Gambar string UTF-8 */
extern void unikode_gambar_string(InfoFramebuffer *fb, int x, int y,
    const char *str, WarnaRGB latar, WarnaRGB belakang);

/* Hitung lebar teks UTF-8 dalam piksel */
extern int unikode_lebar_teks(const char *str);

/* Hitung tinggi teks UTF-8 dalam piksel */
extern int unikode_tinggi_teks(const char *str);

/*******************************************************************************
 * BAGIAN 6: MODUL ANSI (ANSI ESCAPE PARSER)
 ******************************************************************************/

/* Konstanta parser ANSI */
#define ANSI_PARAM_MAKS        32
#define ANSI_SEQ_MAKS          128

/* State parser ANSI */
#define ANSI_STATE_NORMAL      0
#define ANSI_STATE_ESCAPE      1
#define ANSI_STATE_CSI         2
#define ANSI_STATE_OSC         3
#define ANSI_STATE_STRING      4

/* Struktur atribut karakter */
typedef struct {
    unsigned int tebal:1;        /* Tebal/terang */
    unsigned int samar:1;        /* Samar/gelap */
    unsigned int miring:1;       /* Miring */
    unsigned int garis_bawah:1;  /* Garis bawah */
    unsigned int berkedip:1;     /* Berkedip */
    unsigned int terbalik:1;     /* Video terbalik */
    unsigned int sembunyi:1;     /* Tersembunyi */
    unsigned int coret:1;        /* Dicoret */
    unsigned int tipe_latar:2;   /* 0=default, 1=16, 2=256, 3=RGB */
    unsigned int tipe_blkg:2;    /* 0=default, 1=16, 2=256, 3=RGB */
    unsigned char warna_latar;    /* Warna latar */
    unsigned char warna_blkg;     /* Warna background */
    WarnaRGB rgb_latar;           /* RGB latar */
    WarnaRGB rgb_blkg;            /* RGB background */
} AtributKarakter;

/* Struktur sel terminal */
typedef struct {
    char ch;              /* Karakter */
    AtributKarakter attr; /* Atribut karakter */
} SelTerminal;

/* Struktur buffer terminal */
typedef struct {
    SelTerminal **sel;        /* Array 2D sel terminal */
    int baris;                /* Jumlah baris */
    int kolom;                /* Jumlah kolom */
    int kursor_baris;         /* Posisi kursor (baris) */
    int kursor_kolom;         /* Posisi kursor (kolom) */
    int kursor_terlihat;      /* Visibilitas kursor */
    int kursor_simpan_baris;  /* Posisi kursor tersimpan */
    int kursor_simpan_kolom;  /* Posisi kursor tersimpan */
    AtributKarakter attr;     /* Atribut saat ini */
    AtributKarakter attr_def; /* Atribut default */
    int scroll_atas;          /* Area scroll atas */
    int scroll_bawah;         /* Area scroll bawah */
    int *tab_stop;            /* Posisi tab stops */
    int mode_origin;          /* Origin mode */
    int auto_wrap;            /* Auto wrap mode */
    int mode_sisip;           /* Insert mode */
    int mode_layar;           /* 0=normal, 1=alternate */
    SelTerminal **layar_simpan; /* Layar tersimpan */
    int mode_tetikus;         /* Mode tetikus */
    int encoding_tetikus;     /* Encoding tetikus */
    int bracketed_paste;      /* Bracketed paste mode */
    struct {
        int param[ANSI_PARAM_MAKS];
        int jumlah;
    } parser_param;
} BufferTerminal;

/* Struktur parser ANSI */
typedef struct {
    int state;                       /* State parser */
    char seq_buf[ANSI_SEQ_MAKS];     /* Buffer sequence */
    int seq_pos;                     /* Posisi dalam buffer */
    int params[ANSI_PARAM_MAKS];     /* Parameter */
    int param_jumlah;                /* Jumlah parameter */
    int privasi;                     /* Private sequence flag */
    int intermediate;                /* Intermediate character */
} ParserANSI;

/* Inisialisasi atribut default */
extern void ansi_init_attr(AtributKarakter *attr);

/* Inisialisasi buffer terminal */
extern int ansi_init_terminal(BufferTerminal *term, int baris, int kolom);

/* Bebaskan buffer terminal */
extern void ansi_bebas_terminal(BufferTerminal *term);

/* Inisialisasi parser ANSI */
extern void ansi_init_parser(ParserANSI *parser);

/* Proses karakter untuk parser */
extern void ansi_proses_karakter(BufferTerminal *term, ParserANSI *parser,
    char ch);

/* Proses string untuk parser */
extern void ansi_proses_string(BufferTerminal *term, ParserANSI *parser,
    const char *str);

/* Dapatkan warna RGB dari atribut */
extern WarnaRGB ansi_warna(AtributKarakter *attr, int latar);

/* Render terminal ke framebuffer */
extern void ansi_render(BufferTerminal *term, InfoFramebuffer *fb,
    void (*gambar_karakter)(InfoFramebuffer*, int, int, unsigned int,
        WarnaRGB, WarnaRGB));

/*******************************************************************************
 * BAGIAN 7: MODUL PAPAN KETIK (KEYBOARD HANDLER)
 ******************************************************************************/

/* Konstanta keyboard */
#define PAPAN_KETIK_PERANG_MAKS  8
#define PAPAN_KETIK_BUFFER_MAKS  256
#define PAPAN_KETIK_KEYMAP_MAKS  256

/* Struktur pemetaan tombol */
typedef struct {
    char normal;   /* Karakter normal */
    char shift;    /* Karakter dengan Shift */
    char ctrl;     /* Karakter dengan Ctrl */
    char alt;      /* Karakter dengan Alt */
} PemetaanTombol;

/* Struktur state keyboard */
typedef struct {
    int shift_ditekan;    /* Shift ditekan */
    int ctrl_ditekan;     /* Ctrl ditekan */
    int alt_ditekan;      /* Alt ditekan */
    int super_ditekan;    /* Super/Windows ditekan */
    int caps_lock;        /* Caps Lock aktif */
    int num_lock;         /* Num Lock aktif */
    int scroll_lock;      /* Scroll Lock aktif */
} StatePapanKetik;

/* Struktur handler keyboard */
typedef struct {
    int fd;                           /* File descriptor keyboard */
    char jalur_perang[MAKS_JALUR];    /* Path perangkat keyboard */
    PemetaanTombol keymap[PAPAN_KETIK_KEYMAP_MAKS]; /* Keymap */
    StatePapanKetik state;            /* State keyboard saat ini */
    char buffer[PAPAN_KETIK_BUFFER_MAKS]; /* Buffer output */
    int buffer_pos;                   /* Posisi dalam buffer */
    int ulangi_delay;                 /* Delay sebelum repeat (ms) */
    int ulangi_rate;                  /* Rate repeat (ms) */
    struct timeval waktu_tombol_terakhir; /* Waktu key terakhir */
    int kode_tombol_terakhir;         /* Kode key terakhir */
    int nilai_tombol_terakhir;        /* Nilai key terakhir */
} HandlerPapanKetik;

/* Inisialisasi handler keyboard */
extern int papan_ketik_init(HandlerPapanKetik *kbd, const char *jalur);

/* Tutup handler keyboard */
extern void papan_ketik_tutup(HandlerPapanKetik *kbd);

/* Baca input dari keyboard */
extern int papan_ketik_baca(HandlerPapanKetik *kbd, char *buffer, int ukuran);

/* Cek apakah ada data tersedia */
extern int papan_ketik_data_tersedia(HandlerPapanKetik *kbd);

/* Set pengaturan repeat */
extern void papan_ketik_set_ulangi(HandlerPapanKetik *kbd, int delay, int rate);

/* Dapatkan state keyboard */
extern StatePapanKetik* papan_ketik_state(HandlerPapanKetik *kbd);

/* Set keymap custom */
extern void papan_ketik_set_keymap(HandlerPapanKetik *kbd,
    PemetaanTombol *keymap, int ukuran);

/* Dapatkan jalur perangkat */
extern const char* papan_ketik_jalur(HandlerPapanKetik *kbd);

/*******************************************************************************
 * BAGIAN 8: MODUL TETIKUS (MOUSE HANDLER)
 ******************************************************************************/

/* Konstanta tetikus */
#define TETIKUS_PERANG_MAKS   8
#define TETIKUS_BUFFER_MAKS   256
#define TETIKUS_TOMBOL_MAKS   8

/* Mode tetikus */
#define TETIKUS_MODE_MATI     0
#define TETIKUS_MODE_X10      1
#define TETIKUS_MODE_NORMAL   2
#define TETIKUS_MODE_TOMBOL   3
#define TETIKUS_MODE_SEMUA    4

/* Encoding tetikus */
#define TETIKUS_ENCODING_DEFAULT 0
#define TETIKUS_ENCODING_UTF8    1
#define TETIKUS_ENCODING_SGR     2

/* Struktur state tetikus */
typedef struct {
    int x;                        /* Posisi X saat ini */
    int y;                        /* Posisi Y saat ini */
    int tombol[TETIKUS_TOMBOL_MAKS]; /* State tombol */
    int scroll;                   /* Nilai scroll */
    int x_terakhir;               /* Posisi X terakhir */
    int y_terakhir;               /* Posisi Y terakhir */
} StateTetikus;

/* Struktur handler tetikus */
typedef struct {
    int fd;                       /* File descriptor tetikus */
    char jalur_perang[MAKS_JALUR]; /* Path perangkat tetikus */
    StateTetikus state;           /* State tetikus saat ini */
    char buffer[TETIKUS_BUFFER_MAKS]; /* Buffer output */
    int buffer_pos;               /* Posisi dalam buffer */
    int mode;                     /* Mode tetikus saat ini */
    int encoding;                 /* Encoding tetikus saat ini */
    int sel_lebar;                /* Lebar sel */
    int sel_tinggi;               /* Tinggi sel */
    int layar_lebar;              /* Lebar layar dalam sel */
    int layar_tinggi;             /* Tinggi layar dalam sel */
    int grab;                     /* Grab mouse */
} HandlerTetikus;

/* Inisialisasi handler tetikus */
extern int tetikus_init(HandlerTetikus *tetikus, const char *jalur);

/* Tutup handler tetikus */
extern void tetikus_tutup(HandlerTetikus *tetikus);

/* Set mode tetikus */
extern void tetikus_set_mode(HandlerTetikus *tetikus, int mode);

/* Set encoding tetikus */
extern void tetikus_set_encoding(HandlerTetikus *tetikus, int encoding);

/* Set ukuran sel */
extern void tetikus_set_ukuran_sel(HandlerTetikus *tetikus, int lebar, int tinggi);

/* Set ukuran layar */
extern void tetikus_set_ukuran_layar(HandlerTetikus *tetikus, int lebar, int tinggi);

/* Set grab tetikus */
extern void tetikus_set_grab(HandlerTetikus *tetikus, int grab);

/* Baca input dari tetikus */
extern int tetikus_baca(HandlerTetikus *tetikus, char *buffer, int ukuran);

/* Cek apakah ada data tersedia */
extern int tetikus_data_tersedia(HandlerTetikus *tetikus);

/* Dapatkan state tetikus */
extern StateTetikus* tetikus_state(HandlerTetikus *tetikus);

/* Dapatkan jalur perangkat */
extern const char* tetikus_jalur(HandlerTetikus *tetikus);

/* Dapatkan mode tetikus */
extern int tetikus_mode(HandlerTetikus *tetikus);

/* Dapatkan encoding tetikus */
extern int tetikus_encoding(HandlerTetikus *tetikus);

/*******************************************************************************
 * BAGIAN 9: MODUL VIRTUAL TERMINAL (VT MANAGER)
 ******************************************************************************/

/* Konstanta VT */
#define VT_JALUR_MAKS    256
#define VT_NAMA_MAKS     32
#define VT_SINYAL_MAKS   10

/* Mode VT */
#define VT_MODE_TEKS     0
#define VT_MODE_GRAFIS   1

/* State VT */
typedef struct {
    int aktif;             /* VT aktif saat ini */
    int asli;              /* VT asli sebelum switch */
    int diminta;           /* VT yang diminta */
    int fd;                /* File descriptor konsol */
    int mode;              /* Mode saat ini */
    int mode_simpan;       /* Mode yang disimpan */
    int mode_kbd_simpan;   /* Mode keyboard yang disimpan */
    int mode_vt_simpan;    /* Mode VT yang disimpan */
    struct vt_mode vt_simpan; /* Struktur mode VT yang disimpan */
    int diakuisisi;        /* Apakah VT sudah diakuisisi */
    int diganti;           /* Apakah sudah switch ke VT baru */
    int lepas_saat_keluar; /* Lepaskan VT saat exit */
    char jalur_konsol[VT_JALUR_MAKS]; /* Path ke konsol */
    char nama_vt[VT_NAMA_MAKS];       /* Nama VT */
    struct termios termios_simpan;    /* Termios yang disimpan */
} StateVT;

/* Struktur untuk VT manager */
typedef struct {
    StateVT state;
    int jumlah_vt;         /* Jumlah VT yang tersedia */
    int vt_pertama;        /* VT pertama yang tersedia */
    int vt_terakhir;       /* VT terakhir yang tersedia */
    int vt_saat_ini;       /* VT saat ini */
    int vt_fd[64];         /* File descriptor untuk setiap VT */
    int vt_aktif;          /* VT yang aktif */
    void (*callback_ganti)(int vt_baru, int vt_lama);
    void (*callback_akuisisi)(int vt);
    void (*callback_lepas)(int vt);
    void (*callback_sinyal)(int sinyal);
    int handler_sinyal[VT_SINYAL_MAKS];
} ManagerVT;

/* Inisialisasi VT manager */
extern int vt_init(ManagerVT *vtm);

/* Tutup VT manager */
extern void vt_tutup(ManagerVT *vtm);

/* Buka VT tertentu */
extern int vt_buka(ManagerVT *vtm, int nomor_vt);

/* Switch ke VT tertentu */
extern int vt_ganti_ke(ManagerVT *vtm, int nomor_vt);

/* Acquire VT */
extern int vt_akuisisi(ManagerVT *vtm, int nomor_vt);

/* Release VT */
extern int vt_lepas(ManagerVT *vtm);

/* Set mode VT */
extern int vt_set_mode(ManagerVT *vtm, int mode);

/* Set mode keyboard */
extern int vt_set_mode_kbd(ManagerVT *vtm, int mode);

/* Dapatkan state VT */
extern StateVT* vt_state(ManagerVT *vtm);

/* Dapatkan VT aktif */
extern int vt_aktif(ManagerVT *vtm);

/* Dapatkan VT asli */
extern int vt_asli(ManagerVT *vtm);

/* Dapatkan VT saat ini */
extern int vt_saat_ini(ManagerVT *vtm);

/* Set callback ganti VT */
extern void vt_set_callback_ganti(ManagerVT *vtm, void (*callback)(int, int));

/* Set callback akuisisi VT */
extern void vt_set_callback_akuisisi(ManagerVT *vtm, void (*callback)(int));

/* Set callback lepas VT */
extern void vt_set_callback_lepas(ManagerVT *vtm, void (*callback)(int));

/* Cek apakah VT tersedia */
extern int vt_tersedia(ManagerVT *vtm, int nomor_vt);

/* Dapatkan daftar VT yang tersedia */
extern int vt_daftar_tersedia(ManagerVT *vtm, int *daftar, int maks);

/*******************************************************************************
 * BAGIAN 10: MODUL CANGKANG (SHELL INTEGRATION)
 ******************************************************************************/

/* Konstanta shell */
#define CANGKANG_JALUR_MAKS   256
#define CANGKANG_ARG_MAKS     64
#define CANGKANG_BUFFER_MAKS  4096
#define CANGKANG_LINGK_MAKS   128

/* Mode shell */
#define CANGKANG_MODE_NORMAL  0
#define CANGKANG_MODE_RAW     1

/* Struktur manager shell */
typedef struct {
    pid_t pid;                       /* PID proses shell */
    int fd_master;                   /* File descriptor master PTY */
    int fd_slave;                    /* File descriptor slave PTY */
    char *nama_slave;                /* Nama slave PTY */
    char *jalur_shell;               /* Path ke executable shell */
    char *arg[CANGKANG_ARG_MAKS];    /* Argumen shell */
    int jumlah_arg;                  /* Jumlah argumen shell */
    char *lingkungan[CANGKANG_LINGK_MAKS]; /* Environment variables */
    int jumlah_lingk;                /* Jumlah environment variables */
    struct termios termios_simpan;   /* Termios asli */
    struct termios termios_shell;    /* Termios untuk shell */
    int mode;                        /* Mode shell saat ini */
    int berjalan;                    /* Apakah shell sedang berjalan */
    int selesai;                     /* Apakah shell sudah keluar */
    int status_keluar;               /* Status exit shell */
    int baris;                       /* Jumlah baris terminal */
    int kolom;                       /* Jumlah kolom terminal */
    void (*callback_output)(const char *data, int ukuran, void *user);
    void (*callback_keluar)(int status, void *user);
    void *user_data;
} ManagerCangkang;

/* Inisialisasi manager shell */
extern int cangkang_init(ManagerCangkang *shell, const char *jalur_shell);

/* Tambahkan argumen shell */
extern int cangkang_tambah_arg(ManagerCangkang *shell, const char *arg);

/* Set argumen shell */
extern int cangkang_set_arg(ManagerCangkang *shell, char *arg[], int jumlah);

/* Tambahkan environment variable */
extern int cangkang_tambah_lingk(ManagerCangkang *shell, const char *lingk);

/* Set ukuran terminal */
extern void cangkang_set_ukuran(ManagerCangkang *shell, int baris, int kolom);

/* Set mode shell */
extern void cangkang_set_mode(ManagerCangkang *shell, int mode);

/* Set callback output */
extern void cangkang_set_callback_output(ManagerCangkang *shell,
    void (*callback)(const char*, int, void*), void *user);

/* Set callback keluar */
extern void cangkang_set_callback_keluar(ManagerCangkang *shell,
    void (*callback)(int, void*), void *user);

/* Jalankan shell */
extern int cangkang_jalankan(ManagerCangkang *shell);

/* Baca output dari shell */
extern int cangkang_baca(ManagerCangkang *shell, char *buffer, int ukuran);

/* Tulis input ke shell */
extern int cangkang_tulis(ManagerCangkang *shell, const char *buffer, int ukuran);

/* Kirim sinyal ke shell */
extern int cangkang_sinyal(ManagerCangkang *shell, int sinyal);

/* Cek apakah shell berjalan */
extern int cangkang_berjalan(ManagerCangkang *shell);

/* Cek apakah shell selesai */
extern int cangkang_selesai(ManagerCangkang *shell);

/* Dapatkan status keluar */
extern int cangkang_status_keluar(ManagerCangkang *shell);

/* Dapatkan PID shell */
extern pid_t cangkang_pid(ManagerCangkang *shell);

/* Dapatkan fd master */
extern int cangkang_fd_master(ManagerCangkang *shell);

/* Tunggu shell selesai */
extern int cangkang_tunggu(ManagerCangkang *shell, int opsi);

/* Paksa shell keluar */
extern int cangkang_hentikan(ManagerCangkang *shell, int sinyal);

/* Reset manager shell */
extern void cangkang_reset(ManagerCangkang *shell);

/* Bebaskan manager shell */
extern void cangkang_bebas(ManagerCangkang *shell);

/*******************************************************************************
 * BAGIAN 11: MODUL KLIP (CLIPBOARD MANAGER)
 ******************************************************************************/

/* Konstanta clipboard */
#define KLIP_ENTRI_MAKS     16
#define KLIP_UKURAN_DEFAULT 4096
#define KLIP_UKURAN_MAKS    1048576  /* 1MB */

/* Struktur entri clipboard */
typedef struct {
    char *data;       /* Data clipboard */
    size_t ukuran;    /* Ukuran data dalam byte */
    size_t kapasitas; /* Kapasitas buffer */
    int tipe;         /* Tipe data (0=teks, 1=binary) */
    unsigned int id;  /* ID unik untuk entri */
} EntriKlip;

/* Struktur manager clipboard */
typedef struct {
    EntriKlip entri[KLIP_ENTRI_MAKS]; /* Entri clipboard */
    int jumlah;                       /* Jumlah entri */
    int saat_ini;                     /* Entri aktif saat ini */
    unsigned int id_berikutnya;       /* ID berikutnya */
    size_t ukuran_default;            /* Ukuran default */
    size_t ukuran_maks;               /* Ukuran maksimum */
    int auto_hapus;                   /* Auto hapus entri lama */
} ManagerKlip;

/* Inisialisasi manager clipboard */
extern int klip_init(ManagerKlip *klip, size_t ukuran_default, size_t ukuran_maks);

/* Bebaskan manager clipboard */
extern void klip_bebas(ManagerKlip *klip);

/* Salin data ke clipboard */
extern int klip_saline(ManagerKlip *klip, const char *data, size_t ukuran, int tipe);

/* Salin string ke clipboard */
extern int klip_saline_string(ManagerKlip *klip, const char *str);

/* Salin binary data ke clipboard */
extern int klip_saline_binary(ManagerKlip *klip, const char *data, size_t ukuran);

/* Tempel data dari clipboard */
extern const char* klip_tempel(ManagerKlip *klip, size_t *ukuran, int *tipe);

/* Tempel string dari clipboard */
extern const char* klip_tempel_string(ManagerKlip *klip);

/* Tempel binary data dari clipboard */
extern const char* klip_tempel_binary(ManagerKlip *klip, size_t *ukuran);

/* Dapatkan ukuran clipboard saat ini */
extern size_t klip_ukuran(ManagerKlip *klip);

/* Dapatkan tipe clipboard saat ini */
extern int klip_tipe(ManagerKlip *klip);

/* Dapatkan ID clipboard saat ini */
extern unsigned int klip_id(ManagerKlip *klip);

/* Pilih entri clipboard */
extern int klip_pilih(ManagerKlip *klip, int indeks);

/* Pilih entri berdasarkan ID */
extern int klip_pilih_id(ManagerKlip *klip, unsigned int id);

/* Dapatkan jumlah entri */
extern int klip_jumlah(ManagerKlip *klip);

/* Hapus entri saat ini */
extern int klip_hapus_saat_ini(ManagerKlip *klip);

/* Hapus semua entri */
extern void klip_hapus_semua(ManagerKlip *klip);

/* Simpan ke file */
extern int klip_simpan(ManagerKlip *klip, const char *namafile);

/* Muat dari file */
extern int klip_muat(ManagerKlip *klip, const char *namafile);

/*******************************************************************************
 * BAGIAN 12: MODUL KONFIGURASI (CONFIG MANAGER)
 ******************************************************************************/

/* Konstanta konfigurasi */
#define KONFIG_SEKSI_MAKS      32
#define KONFIG_KUNCI_MAKS      256
#define KONFIG_BARIS_MAKS      1024
#define KONFIG_JALUR_MAKS      256
#define KONFIG_NILAI_MAKS      512
#define KONFIG_FILE_DEFAULT    "konfigurasi.ini"
#define KONFIG_BACKUP_EXT      ".cadangan"

/* Tipe konfigurasi */
typedef enum {
    KONFIG_TIPE_STRING,
    KONFIG_TIPE_INT,
    KONFIG_TIPE_BOOL,
    KONFIG_TIPE_FLOAT,
    KONFIG_TIPE_WARNA,
    KONFIG_TIPE_TIDAK_DIKETAHUI
} TipeKonfig;

/* Struktur nilai konfigurasi */
typedef struct {
    char kunci[KONFIG_NILAI_MAKS];
    char nilai[KONFIG_NILAI_MAKS];
    TipeKonfig tipe;
    int diubah;
} NilaiKonfig;

/* Struktur seksi konfigurasi */
typedef struct {
    char nama[KONFIG_NILAI_MAKS];
    NilaiKonfig nilai[KONFIG_KUNCI_MAKS];
    int jumlah_kunci;
} SeksiKonfig;

/* Struktur manager konfigurasi */
typedef struct {
    SeksiKonfig seksi[KONFIG_SEKSI_MAKS];
    int jumlah_seksi;
    char namafile[KONFIG_JALUR_MAKS];
    int diubah;
    int auto_simpan;
    int buat_backup;
} ManagerKonfig;

/* Inisialisasi manager konfigurasi */
extern int konfig_init(ManagerKonfig *km, const char *namafile);

/* Muat konfigurasi dari file */
extern int konfig_muat(ManagerKonfig *km, const char *namafile);

/* Simpan konfigurasi ke file */
extern int konfig_simpan(ManagerKonfig *km, const char *namafile);

/* Dapatkan nilai string */
extern const char* konfig_string(ManagerKonfig *km, const char *seksi,
    const char *kunci, const char *nilai_default);

/* Dapatkan nilai integer */
extern int konfig_int(ManagerKonfig *km, const char *seksi,
    const char *kunci, int nilai_default);

/* Dapatkan nilai boolean */
extern int konfig_bool(ManagerKonfig *km, const char *seksi,
    const char *kunci, int nilai_default);

/* Dapatkan nilai float */
extern float konfig_float(ManagerKonfig *km, const char *seksi,
    const char *kunci, float nilai_default);

/* Dapatkan nilai warna */
extern int konfig_warna(ManagerKonfig *km, const char *seksi,
    const char *kunci, WarnaRGB *warna_default);

/* Set nilai string */
extern int konfig_set_string(ManagerKonfig *km, const char *seksi,
    const char *kunci, const char *nilai);

/* Set nilai integer */
extern int konfig_set_int(ManagerKonfig *km, const char *seksi,
    const char *kunci, int nilai);

/* Set nilai boolean */
extern int konfig_set_bool(ManagerKonfig *km, const char *seksi,
    const char *kunci, int nilai);

/* Set nilai float */
extern int konfig_set_float(ManagerKonfig *km, const char *seksi,
    const char *kunci, float nilai);

/* Set nilai warna */
extern int konfig_set_warna(ManagerKonfig *km, const char *seksi,
    const char *kunci, WarnaRGB *warna);

/* Set auto-save */
extern void konfig_set_auto_simpan(ManagerKonfig *km, int auto_simpan);

/* Set buat backup */
extern void konfig_set_buat_backup(ManagerKonfig *km, int buat_backup);

/* Cek apakah konfigurasi diubah */
extern int konfig_diubah(ManagerKonfig *km);

/* Dapatkan nama file */
extern const char* konfig_namafile(ManagerKonfig *km);

/* Bebaskan manager konfigurasi */
extern void konfig_bebas(ManagerKonfig *km);

/*******************************************************************************
 * BAGIAN 13: MODUL TERMINAL (TERMINAL APPLICATION)
 ******************************************************************************/

/* Konstanta terminal */
#define TERMINAL_FPS_MAKS           60
#define TERMINAL_WAKTU_FRAME        (1000000 / TERMINAL_FPS_MAKS)
#define TERMINAL_BARIS_DEFAULT      25
#define TERMINAL_KOLOM_DEFAULT      80
#define TERMINAL_FONT_DEFAULT       16
#define TERMINAL_FILE_KONFIG        "terminal.ini"

/* Mode aplikasi */
#define TERMINAL_MODE_NORMAL        0
#define TERMINAL_MODE_PILIH         1
#define TERMINAL_MODE_CARI          2

/* Struktur tema terminal */
typedef struct {
    WarnaRGB latar;         /* Warna latar depan */
    WarnaRGB belakang;      /* Warna background */
    WarnaRGB kursor;        /* Warna kursor */
    WarnaRGB pilih_latar;   /* Warna latar pilihan */
    WarnaRGB pilih_belakang;/* Warna background pilihan */
    WarnaRGB sorot_cari;    /* Warna sorotan pencarian */
} TemaTerminal;

/* Struktur konfigurasi terminal */
typedef struct {
    int lebar;
    int tinggi;
    int baris;
    int kolom;
    int ukuran_font;
    int pakai_unicode;
    int pakai_tetikus;
    int baris_scrollback;
    char jalur_shell[256];
    int auto_simpan_konfig;
    int buat_backup;
    TemaTerminal tema;
    int transparansi;
    int layar_penuh;
    int bell_hidup;
    int bell_visual;
    int kursor_berkedip;
    int tampilkan_scrollbar;
    int word_wrap;
    int status_keluar;       /* Status keluar shell terakhir */
} KonfigTerminal;

/* Struktur status bar */
typedef struct {
    int terlihat;
    char teks_kiri[64];
    char teks_kanan[64];
    WarnaRGB latar;
    WarnaRGB belakang;
} StatusBar;

/* Struktur seleksi */
typedef struct {
    int aktif;
    int awal_baris;
    int awal_kolom;
    int akhir_baris;
    int akhir_kolom;
} Seleksi;

/* Struktur pencarian */
typedef struct {
    int aktif;
    char kueri[256];
    int cocok_saat_ini;
    int total_cocok;
    int *baris_cocok;
    int jumlah_cocok;
    int kapasitas_cocok;
} Pencarian;

/* Struktur aplikasi terminal */
typedef struct {
    /* Komponen utama */
    InfoFramebuffer fb;
    BufferTerminal term;
    ParserANSI parser;

    /* Input handling */
    HandlerPapanKetik kbd;
    HandlerTetikus tetikus;
    ManagerVT vt;
    ManagerCangkang cangkang;
    ManagerKlip klip;
    ManagerKonfig konfig;

    /* Konfigurasi */
    KonfigTerminal cfg;

    /* Elemen UI */
    StatusBar status_bar;
    Seleksi seleksi;
    Pencarian cari;

    /* State aplikasi */
    int berjalan;
    int mode;
    int fokus;
    int frame_hitung;
    struct timeval waktu_frame_terakhir;
    int kursor_terlihat;
    int kursor_kedip_state;
    struct timeval waktu_kedip_terakhir;

    /* Performansi */
    int fps;
    int waktu_frame;
    int waktu_render;

    /* Callbacks */
    void (*callback_keluar)(int status);
    void (*callback_ubah_ukuran)(int lebar, int tinggi);
} AplikasiTerminal;

/* Inisialisasi aplikasi terminal */
extern int terminal_init(AplikasiTerminal *app, const char *file_konfig);

/* Jalankan aplikasi terminal */
extern void terminal_jalankan(AplikasiTerminal *app);

/* Tutup aplikasi terminal */
extern void terminal_tutup(AplikasiTerminal *app);

/* Set callback keluar */
extern void terminal_set_callback_keluar(AplikasiTerminal *app,
    void (*callback)(int));

/* Set callback ubah ukuran */
extern void terminal_set_callback_ubah_ukuran(AplikasiTerminal *app,
    void (*callback)(int, int));

/*******************************************************************************
 * BAGIAN 14: FUNGSI FRAMEBUFFER
 ******************************************************************************/

/* Inisialisasi framebuffer */
extern int fb_init(const char *jalur, InfoFramebuffer *fb);

/* Bersihkan framebuffer dengan warna hitam */
extern void fb_bersihkan(InfoFramebuffer *fb);

/* Bersihkan framebuffer dengan warna tertentu */
extern void fb_bersihkan_warna(InfoFramebuffer *fb, WarnaRGB warna);

/* Tutup framebuffer */
extern void fb_tutup(InfoFramebuffer *fb);

/* Gambar piksel */
extern void fb_gambar_piksel(InfoFramebuffer *fb, int x, int y,
    unsigned char r, unsigned char g, unsigned char b);

/* Isi area dengan warna */
extern void fb_isi_area(InfoFramebuffer *fb, int x1, int y1, int x2, int y2,
    WarnaRGB warna);

/* Gambar garis */
extern void fb_gambar_garis(InfoFramebuffer *fb, int x1, int y1, int x2, int y2,
    WarnaRGB warna);

/* Gambar kotak */
extern void fb_gambar_kotak(InfoFramebuffer *fb, int x, int y, int lebar, int tinggi,
    WarnaRGB warna, int isi);

/*******************************************************************************
 * BAGIAN 15: FUNGSI UTILITAS
 ******************************************************************************/

/* Salin string dengan aman */
extern char* pigura_strdup(const char *str);

/* Bandingkan string case-insensitive */
extern int pigura_strcasecmp(const char *s1, const char *s2);

/* Konversi string ke boolean */
extern int pigura_string_ke_bool(const char *str);

/* Konversi string ke warna */
extern int pigura_string_ke_warna(const char *str, WarnaRGB *warna);

/* Konversi warna ke string */
extern void pigura_warna_ke_string(WarnaRGB *warna, char *str, int panjang_maks);

#endif /* PIGURA_H */
