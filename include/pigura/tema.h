/* ----------------------------------------------------------------------------------------------
 * pigura/tema.h - Theme system untuk widget toolkit
 * ----------------------------------------------------------------------------------------------
 * Set warna global untuk semua widget, atau override per-widget.
 * v0.1: hanya set latar widget dasar (pg_widget->latar) dan
 * latar_widget. Per-widget override (tombol, cek, isian_teks) butuh
 * casting ke struct impl masing-masing, yang akan ditangani v0.3+.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_TEMA_H
#define PIGURA_TEMA_H

#include "pigura/tipe.h"
#include "pigura/widget.h"
#include "pigura/kotak.h"
#include "pigura/font.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_tema {
        /* Warna dasar. */
        pg_warna_t latar;          /* background window */
        pg_warna_t latar_widget;   /* background widget (button, dll) */
        pg_warna_t latar_input;    /* background input (lineedit) */
        pg_warna_t tepi;           /* border widget */
        pg_warna_t teks;           /* text color */
        pg_warna_t teks_terpilih;   /* selected text color */
        pg_warna_t latar_terpilih;  /* selected background */
        pg_warna_t latar_hover;     /* hover background */
        pg_warna_t aksen;          /* accent (focus, highlight) */
        pg_warna_t judul;          /* title bar background */
        pg_warna_t judul_teks;     /* title bar text */

        /* Font. */
        pg_font_t *font;
        int        ukuran_font;

        /* Metrics. */
        int padding;       /* inner padding widget */
        int spasi;          /* spacing antar widget */
        int radius_sudut;  /* border radius (0 = kotak) */
} pg_tema_t;

/* Tema default (mirip GTK Adwaita light). */
pg_tema_t *pg_tema_buat(void);

/* Tema gelap. */
pg_tema_t *pg_tema_buat_gelap(void);

/* Terapkan tema ke widget. */
void pg_tema_terapkan_widget(pg_widget_t *w, const pg_tema_t *t);

/* Terapkan tema ke semua widget di container (recursive). */
void pg_tema_terapkan_kotak(pg_kotak_widget_t *box,
                             const pg_tema_t *t);

/* Hancurkan tema. Font tidak ikut dihancurkan. */
void pg_tema_hancur(pg_tema_t *t);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_TEMA_H */
