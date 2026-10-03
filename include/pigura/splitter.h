/* ----------------------------------------------------------------------------------------------
 * pigura/splitter.h - Widget splitter (resize antar panel)
 * ----------------------------------------------------------------------------------------------
 * Splitter adalah widget garis tipis (4 piksel) yang dipakai untuk
 * mengubah ukuran dua panel berdekatan. Saat di-seret, splitter
 * memanggil kembali cb_ubah dengan posisi baru dan otomatis me-resize
 * kedua panel yang ditautkan via pg_splitter_setel_panel.
 *
 *   PG_SPLITTER_HORI: garis vertikal, drag mengubah posisi X
 *                     (panel1 di kiri, panel2 di kanan).
 *   PG_SPLITTER_VERT: garis horizontal, drag mengubah posisi Y
 *                     (panel1 di atas, panel2 di bawah).
 *
 * Posisi yang dilaporkan cb_ubah adalah koordinat layar (sama dengan
 * pg_widget_kotak(sp).x atau .y, tergantung orientasi).
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_SPLITTER_H
#define PIGURA_SPLITTER_H

#include "pigura/tipe.h"
#include "pigura/widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	PG_SPLITTER_HORI = 0, /* garis vertikal, drag horizontal */
	PG_SPLITTER_VERT  = 1 /* garis horizontal, drag vertikal */
} pg_splitter_orientasi_t;

typedef struct pg_splitter pg_splitter_t;

/* Buat splitter. Lebar/tinggi default 4 piksel. */
pg_splitter_t *pg_buat_splitter(pg_splitter_orientasi_t orientasi);

/* Bebaskan splitter (tidak menghancurkan panel). */
void pg_splitter_hancur(pg_splitter_t *sp);

/* Set widget kiri/atas (panel1) dan kanan/bawah (panel2). */
void pg_splitter_setel_panel(pg_splitter_t *sp,
                              pg_widget_t *panel1,
                              pg_widget_t *panel2);

/* Setel ukuran minimum panel1 dan panel2 (piksel). */
void pg_splitter_setel_min(pg_splitter_t *sp, int min1, int min2);

/* Callback saat posisi splitter berubah. Posisi adalah koordinat
 * layar (x untuk HORI, y untuk VERT) splitter setelah gerakan. */
void pg_splitter_saat_ubah(pg_splitter_t *sp,
    void (*cb)(int posisi, void *ctx), void *ctx);

/* Akses widget dasar. */
pg_widget_t *pg_splitter_widget(pg_splitter_t *sp);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_SPLITTER_H */
