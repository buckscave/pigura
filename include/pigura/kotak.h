/* ----------------------------------------------------------------------------------------------
 * pigura/kotak.h - Kontainer layout linear (box)
 * ----------------------------------------------------------------------------------------------
 * Kotak adalah kontainer yang menyusun anak-anaknya secara linear
 * (horizontal atau vertikal) dengan spasi tetap di antara setiap
 * anak. Anak yang ditandai "kembang" akan memperoleh bagian dari
 * ruang tersisa secara proporsional.
 *
 * KEPEMILIKAN ANAK (lifecycle):
 *   Secara default, kotak TIDAK memiliki anak. Pemanggil harus
 *   menghancurkan anak sendiri. Untuk mode "komposisi otomatis"
 *   (parent memiliki anak), panggil pg_kotak_milik(BENAR) sebelum
 *   tambah anak. Maka pg_kotak_hancur akan menghancurkan semua anak
 *   secara rekursif — cocok untuk UI deklaratif.
 *
 * Padding:
 *   - pg_kotak_setel_padding_kotak(k, px) = padding internal box
 *     (semua sisi, antara tepi box dan anak pertama/terakhir).
 *   - pg_kotak_tambah_lengkap(k, w, expand, fill, padding) = padding
 *     per-child (sekitar child individual).
 *
 * Homogen mode:
 *   pg_kotak_setel_homogen(k, BENAR) — semua child punya ukuran
 *   sama sepanjang sumbu utama (mis. numeric keypad 3x4).
 *
 * Catatan: nama tipe pg_kotak_widget_t dipakai (bukan pg_kotak_t)
 * karena pg_kotak_t sudah dipakai untuk rect di tipe.h.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_KOTAK_H
#define PIGURA_KOTAK_H

#include "pigura/tipe.h"
#include "pigura/widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_kotak_widget pg_kotak_widget_t;

/* Orientasi layout. */
enum pg_kotak_orientasi {
	PG_KOTAK_HORIZONTAL = 0,
	PG_KOTAK_VERTIKAL   = 1
};

/* Alignment untuk child yang tidak expand. */
typedef enum {
	PG_ALIGN_MULAI  = 0, /* start (left/top) */
	PG_ALIGN_TENGAH = 1, /* center */
	PG_ALIGN_AKHIR  = 2, /* end (right/bottom) */
	PG_ALIGN_ISI    = 3  /* fill available space */
} pg_align_t;

/* Buat kotak kontainer. orientasi: PG_KOTAK_HORIZONTAL/_VERTIKAL. */
pg_kotak_widget_t *pg_buat_kotak_widget(int orientasi, int spasi);

/* Bebaskan kotak. Bila mode milik aktif, SEMUA anak juga
 * dihancurkan secara rekursif (pg_widget_hancur + free). */
void pg_kotak_hancur(pg_kotak_widget_t *k);

/* Setel mode kepemilikan anak. milik=BENAR: pg_kotak_hancur akan
 * menghancurkan SEMUA anak juga. Default SALAH. */
void pg_kotak_milik(pg_kotak_widget_t *k, pg_bool milik);

/* Hapus SEMUA anak dari kotak. Bila milik=BENAR, hancurkan juga. */
void pg_kotak_bersih(pg_kotak_widget_t *k);

/* Tambah anak. kembang=BENAR agar memperoleh ruang tersisa. */
void pg_kotak_tambah(pg_kotak_widget_t *k, pg_widget_t *w,
                     pg_bool kembang);

/* Tambah child dengan konfigurasi layout lengkap.
 *   expand=BENAR: child memperoleh bagian dari ruang tersisa.
 *   fill=BENAR:   child mengisi ruangnya.
 *   padding:       spasi internal di kedua sisi child sepanjang
 *                 sumbu layout. */
void pg_kotak_tambah_lengkap(pg_kotak_widget_t *k, pg_widget_t *w,
                              pg_bool expand, pg_bool fill,
                              int padding);

/* Setel padding internal box (semua sisi, antara tepi dan anak). */
void pg_kotak_setel_padding_kotak(pg_kotak_widget_t *k, int padding);

/* Setel spasi antar child. */
void pg_kotak_setel_spasi(pg_kotak_widget_t *k, int spasi);

/* Setel orientasi runtime (HORIZONTAL/VERTIKAL). */
void pg_kotak_setel_orientasi(pg_kotak_widget_t *k, int orientasi);

/* Setel mode homogen: semua child ukuran sama sepanjang sumbu
 * utama. Default SALAH. Berguna untuk keypad, button grid, dll. */
void pg_kotak_setel_homogen(pg_kotak_widget_t *k, pg_bool homogen);

/* Setel latar kotak. Bila di-set, kotak render latar sendiri
 * (sebelum render anak). Default: transparan (pakai parent). */
void pg_kotak_setel_latar(pg_kotak_widget_t *k, pg_warna_t latar);
void pg_kotak_setel_latar_transparan(pg_kotak_widget_t *k);

/* Setel border (outline) kotak. Bila di-set, kotak render outline. */
void pg_kotak_setel_batas(pg_kotak_widget_t *k, pg_warna_t batas);
void pg_kotak_setel_batas_transparan(pg_kotak_widget_t *k);

/* Paksa re-layout. Panggil bila min_w/min_h child berubah setelah
 * masuk kotak (mis. label setel_teks yang lebih panjang). */
void pg_kotak_tata(pg_kotak_widget_t *k);

/* Hapus anak pada indeks (tidak menghancurkan). */
void pg_kotak_hapus(pg_kotak_widget_t *k, int idx);

/* Jumlah anak di kotak. */
int pg_kotak_jumlah_anak(pg_kotak_widget_t *k);

/* Ambil widget anak pada indeks. NULL bila idx di luar rentang. */
pg_widget_t *pg_kotak_anak(pg_kotak_widget_t *k, int idx);

/* Akses widget dasar. */
pg_widget_t *pg_kotak_widget(pg_kotak_widget_t *k);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_KOTAK_H */
