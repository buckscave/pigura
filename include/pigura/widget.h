/* ----------------------------------------------------------------------------------------------
 * pigura/widget.h - Kelas dasar widget
 * ----------------------------------------------------------------------------------------------
 * Widget adalah elemen UI mandiri yang punya permukaan, menerima
 * peristiwa input, dan di-parent ke widget lain atau ke jendela.
 *
 * Di v0.1 widget "ringan": tidak ada layout manager (aplikasi
 * memosisikan eksplisit via pg_widget_setel_kotak).
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_WIDGET_H
#define PIGURA_WIDGET_H

#include "pigura/tipe.h"
#include "pigura/permukaan.h"
#include "pigura/peristiwa.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_widget pg_widget_t;

/* Tipe tag widget. */
typedef enum pg_widget_tipe {
        PG_WIDGET_DASAR = 0,
        PG_WIDGET_LABEL,
        PG_WIDGET_TOMBOL,
        PG_WIDGET_KOTAK,
        PG_WIDGET_ISIAN_TEK,
        PG_WIDGET_GAMBAR,
        PG_WIDGET_GESER,
        PG_WIDGET_CEK,
        PG_WIDGET_RADIO,
        PG_WIDGET_KEMAJUAN,
        PG_WIDGET_GULIR,
        PG_WIDGET_DAFTAR,
        PG_WIDGET_MENUBAR,
        PG_WIDGET_DROPDOWN,
        PG_WIDGET_JENDELA
} pg_widget_tipe_t;

typedef struct pg_widget_vtable pg_widget_vtable_t;

typedef void (*pg_catat_cb)(pg_widget_t *diri, pg_permukaan_t *s);
typedef pg_bool (*pg_peristiwa_cb_w)(pg_widget_t *diri,
                                      const pg_peristiwa_t *e);
typedef void (*pg_hancur_cb)(pg_widget_t *diri);
typedef void (*pg_ubah_ukuran_cb)(pg_widget_t *diri, int w, int h);
typedef pg_bool (*pg_berisi_cb)(pg_widget_t *diri, pg_titik_t p);

/* Pintasan keyboard (shortcut). key = PG_TOMBOL_*, mod = bitmask
 * PG_MOD_*. Dipicu saat widget punya fokus dan key+mod ditekan.
 * Maksimum 16 pintasan per widget. */
typedef struct pg_pintasan {
        int   key;
        int   mod;
        void (*cb)(pg_widget_t *, void *);
} pg_pintasan_t;

#define PG_PINTASAN_MAKS 16

/* vtable widget. catat_popup opsional: dipanggil SETELAH catat
 * normal untuk render konten tambahan (popup overlay) langsung ke
 * permukaan tujuan tanpa clip ke kotak widget. berisi opsional:
 * bila NULL, dipakai default (cek titik di dalam kotak widget).
 *
 * bebas opsional: dipanggil oleh pg_widget_hancur_penuh() untuk
 * membebaskan struktur TURUNAN (pg_label_t, pg_tombol_t, dst.).
 * Implementasi default: tidak melakukan apa-apa (struktur widget
 * tidak di-free). Setiap pg_buat_* yang mengalokasikan struktur
 * turunan WAJIB mengisi `bebas` agar memanggil free(diri).
 *
 * Tanpa `bebas`, mode milik pg_kotak (komposisi otomatis) TIDAK
 * akan benar-benar membebaskan struktur anak — leak tapi tidak
 * crash. Dengan `bebas` terisi, free penuh rekursif aman. */
struct pg_widget_vtable {
        pg_catat_cb        catat;
        pg_peristiwa_cb_w  peristiwa;
        pg_ubah_ukuran_cb  ubah_ukuran;
        pg_hancur_cb       hancur;
        pg_catat_cb        catat_popup;
        pg_berisi_cb       berisi;
        pg_hancur_cb       bebas; /* deallocator struktur turunan */
};

struct pg_widget {
        pg_widget_tipe_t      tipe;
        pg_widget_t          *induk;
        pg_permukaan_t       *permukaan;
        pg_kotak_t            kotak;
        int                   min_w;
        int                   min_h;
        pg_bool              terlihat;
        pg_bool              aktif;
        pg_bool              kotor;
        pg_warna_t           latar;
        const pg_widget_vtable_t *vtable;
        void                 *impl;
        /* Daftar anak universal (parent-child tree). Setiap widget
         * BISA punya anak — tidak hanya pg_kotak. Dipakai untuk:
         *   1. Focus chain traversal (Tab/Shift+Tab)
         *   2. Recursive destroy saat milik=BENAR
         * Kontainer layout (pg_kotak, toolbar, tab, dll.) tetap
         * simpan metadata layout tambahan di struct-nya sendiri,
         * tapi pointer anak disinkronkan dengan array ini lewat
         * pg_widget_tambah_anak(). */
        pg_widget_t         **anak;
        int                   n_anak;
        int                   cap_anak;
        pg_bool              milik; /* hancur anak saat parent dihancurkan? */
        int                   radius; /* radius sudut kotak (0=tajam) */
        /* High-level event state (internal, jangan di-set app). */
        pg_u32               klik_terakhir_ms; /* untuk double klik */
        int                  klik_tombol_terakhir;
        pg_bool              sedang_seret;
        int                  seret_ofs_x;
        int                  seret_ofs_y;
        pg_bool              fokus;
        /* Callback high-level (opsional, default NULL). */
        void (*saat_dobelklik)(pg_widget_t *w, void *ctx);
        void (*saat_seret_mulai)(pg_widget_t *w, int x, int y, void *ctx);
        void (*saat_seret_gerak)(pg_widget_t *w, int x, int y, void *ctx);
        void (*saat_seret_selesai)(pg_widget_t *w, int x, int y, void *ctx);
        void (*saat_fokus)(pg_widget_t *w, void *ctx);
        void (*saat_blur)(pg_widget_t *w, void *ctx);
        void (*saat_klik_kanan)(pg_widget_t *w, int x, int y, void *ctx);
        void  *hl_ctx; /* context untuk callback high-level */
        /* Pintasan keyboard (shortcut) terdaftar untuk widget ini.
        * Dipicu hanya bila widget punya fokus. */
        pg_pintasan_t pintasan[PG_PINTASAN_MAKS];
        int           n_pintasan;
};

/* Lifecycle. */
void pg_widget_init(pg_widget_t *w, pg_widget_tipe_t tipe,
                    const pg_widget_vtable_t *vt);
void pg_widget_hancur(pg_widget_t *w);

/* Geometri. */
pg_kotak_t pg_widget_kotak(const pg_widget_t *w);
void      pg_widget_setel_kotak(pg_widget_t *w, pg_kotak_t r);
void      pg_widget_pindah(pg_widget_t *w, int x, int y);
void      pg_widget_ubah_ukuran(pg_widget_t *w, int w_, int h);

/* Ukuran minimum natural (dipakai layout manager). Default:
 * ukuran permukaan saat setel_kotak pertama kali dipanggil. */
void      pg_widget_setel_ukuran_min(pg_widget_t *w, int w_, int h);

/* Setel radius sudut kotak untuk rendering widget (0 = tajam).
 * Berlaku untuk widget berbasis kotak: tombol, panel, dll.
 * Nilai akan di-clamp otomatis ke min(w,h)/2 saat render. */
void      pg_widget_setel_radius(pg_widget_t *w, int radius);
int       pg_widget_radius(const pg_widget_t *w);

/* Visibilitas / aktif. */
void pg_widget_tampil(pg_widget_t *w);
void pg_widget_sembunyi(pg_widget_t *w);
pg_bool pg_widget_terlihat(const pg_widget_t *w);
void pg_widget_aktifkan(pg_widget_t *w);
void pg_widget_nonaktifkan(pg_widget_t *w);
pg_bool pg_widget_aktif(const pg_widget_t *w);

/* Warna latar. */
void pg_widget_setel_latar(pg_widget_t *w, pg_warna_t c);
pg_warna_t pg_widget_ambil_latar(const pg_widget_t *w);

/* Painting / dispatch peristiwa. */
void pg_widget_catat(pg_widget_t *w, pg_permukaan_t *dest);
pg_bool pg_widget_tangani_peristiwa(pg_widget_t *w,
                                      const pg_peristiwa_t *e);

/* Catat popup overlay (jika ada) langsung ke dest tanpa clip ke
 * kotak widget. Dipanggil SETELAH semua widget normal di-render.
 * Bawaan: tidak melakukan apa-apa (vtable->catat_popup NULL). */
void pg_widget_catat_popup(pg_widget_t *w, pg_permukaan_t *dest);

/* Hitung posisi absolut widget di permukaan tujuan dengan
 * menjumlahkan kotak.x/y widget dan semua induknya. Berguna
 * untuk popup overlay yang harus render ke dest (bukan ke
 * permukaan widget sendiri) pada posisi absolut. */
void pg_widget_posisi_layar(const pg_widget_t *w, int *ax, int *ay);

/* Tandai widget kotor (akan dicat ulang frame berikutnya). */
void pg_widget_kotor(pg_widget_t *w);

/* Hit test. */
pg_bool pg_widget_berisi(const pg_widget_t *w, pg_titik_t p);

/* ===== High-level event API ===== */

/* Set callback double klik. */
void pg_widget_saat_dobelklik(pg_widget_t *w,
                               void (*cb)(pg_widget_t*, void*),
                               void *ctx);

/* Set callback drag (seret). Mulai saat klik kiri + gerak.
 * Selesai saat mouse dilepas. */
void pg_widget_saat_seret(pg_widget_t *w,
                           void (*mulai)(pg_widget_t*, int, int, void*),
                           void (*gerak)(pg_widget_t*, int, int, void*),
                           void (*selesai)(pg_widget_t*, int, int, void*),
                           void *ctx);

/* Fokus widget (untuk keyboard input). */
void pg_widget_fokus(pg_widget_t *w);
void pg_widget_blur(pg_widget_t *w);
pg_bool pg_widget_punya_fokus(const pg_widget_t *w);

/* Set callback fokus/blur. */
void pg_widget_saat_fokus(pg_widget_t *w,
                           void (*cb)(pg_widget_t*, void*),
                           void *ctx);

/* Set callback klik kanan (context menu). Dipanggil saat tombol
 * kanan mouse ditekan di dalam widget. x, y adalah posisi global
 * mouse (relatif ke permukaan tujuan). */
void pg_widget_saat_klik_kanan(pg_widget_t *w,
    void (*cb)(pg_widget_t*, int x, int y, void*), void *ctx);

/* Set callback keyboard shortcut. key = PG_TOMBOL_*, mod = bitmask
 * PG_MOD_*. Dipanggil saat widget punya fokus dan shortcut ditekan.
 * Maksimum 16 pintasan per widget; kelebihan diabaikan diam-diam. */
void pg_widget_saat_pintasan(pg_widget_t *w, int key, int mod,
    void (*cb)(pg_widget_t*, void*), void *ctx);

/* Focus chain: Tab ke widget berikutnya. */
void pg_widget_fokus_berikutnya(pg_widget_t *w);
void pg_widget_fokus_sebelumnya(pg_widget_t *w);

/* ===== Parent-child tree (universal) =====
 *
 * Setiap widget BISA memiliki anak. Daftar anak ini disinkronkan
 * otomatis dengan panggilan pg_widget_tambah_anak(). Kontainer
 * layout seperti pg_kotak memanggil ini secara internal saat
 * pg_kotak_tambah() dipanggil, jadi pemanggil API tidak perlu
 * panggil eksplisit kecuali membuat kontainer kustom.
 *
 * Hindari set manual `child->induk = parent` — selalu pakai
 * pg_widget_tambah_anak() supaya daftar anak induk juga
 * diperbarui (diperlukan untuk focus chain dan recursive
 * destroy). */

/* Tambah anak ke induk. Idempoten: bila anak sudah terdaftar,
 * no-op. Mengatur anak->induk = induk. */
pg_bool pg_widget_tambah_anak(pg_widget_t *induk, pg_widget_t *anak);

/* Hapus anak dari induk. No-op bila tidak terdaftar.
 * Mengatur anak->induk = NULL (bila masih menunjuk ke induk). */
void pg_widget_hapus_anak(pg_widget_t *induk, pg_widget_t *anak);

/* Setel mode kepemilikan anak. milik=BENAR: pg_widget_hancur
 * akan menghancurkan SEMUA anak secara rekursif (lewat
 * pg_widget_hancur_penuh, yang memanggil vtable->bebas).
 * Default SALAH. */
void pg_widget_milik(pg_widget_t *w, pg_bool milik);

/* Jumlah anak terdaftar di widget. */
int pg_widget_jumlah_anak(const pg_widget_t *w);

/* Ambil widget anak pada indeks. NULL bila idx di luar rentang. */
pg_widget_t *pg_widget_ambil_anak(const pg_widget_t *w, int idx);

/* Bebaskan capture mouse (g_capture). Dipakai setelah operasi
 * yang menyembunyikan/memindahkan widget yang sedang di-capture. */
void pg_widget_bebas_capture(void);

/* Hancurkan widget + struktur turunannya secara penuh.
 *
 * pg_widget_hancur hanya membebaskan resource internal widget
 * (permukaan, vtable cleanup) tetapi TIDAK free struktur turunan
 * (pg_label_t, pg_tombol_t). Fungsi ini memanggil vtable->bebas
 * bila ada, yang membebaskan struktur turunan.
 *
 * Bila widget->milik=BENAR, SEMUA anak juga akan dihancurkan
 * secara rekursif sebelum widget sendiri dihancurkan.
 *
 * Gunakan ini untuk mode milik pg_kotak (komposisi otomatis),
 * di mana parent harus bisa hancurkan anak tanpa tahu tipe asli
 * anak tersebut.
 *
 * Untuk pemakaian manual (luar kontainer), panggil pg_*_hancur
 * (mis. pg_label_hancur) — itu sudah memanggil pg_widget_hancur
 * + free. */
void pg_widget_hancur_penuh(pg_widget_t *w);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_WIDGET_H */
