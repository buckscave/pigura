/* ----------------------------------------------------------------------------------------------
 * pigura/menubar.h - Widget menu bar horizontal
 * ----------------------------------------------------------------------------------------------
 * Menubar adalah bar horizontal berisi beberapa menu. Klik menu
 * membuka dropdown berisi item. Klik item memicu callback. Item
 * pemisah digambar sebagai garis horizontal.
 *
 * Hanya satu menu yang bisa terbuka pada satu waktu. Klik di luar
 * menu manapun menutup menu yang sedang terbuka.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_MENUBAR_H
#define PIGURA_MENUBAR_H

#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_menubar pg_menubar_t;
typedef struct pg_menu pg_menu_t;

/* Dipanggil saat item menu diklik. idx = indeks item. */
typedef void (*pg_menu_cb)(pg_menu_t *m, int idx, void *ctx);

/* Buat menubar. */
pg_menubar_t *pg_buat_menubar(pg_font_t *font);

/* Bebaskan menubar + semua menu dan item. */
void pg_menubar_hancur(pg_menubar_t *mb);

/* Tambah menu ke menubar. Mengembalikan handle menu. */
pg_menu_t *pg_menubar_tambah_menu(pg_menubar_t *mb,
                                   const char *judul);

/* Tambah item ke menu. */
void pg_menu_tambah_item(pg_menu_t *m, const char *label,
                          pg_menu_cb cb, void *ctx);

/* Tambah pemisah ke menu. */
void pg_menu_tambah_pemisah(pg_menu_t *m);

/* Akses widget dasar. */
pg_widget_t *pg_menubar_widget(pg_menubar_t *mb);

/* Cek apakah ada menu yang sedang terbuka. */
pg_bool pg_menubar_terbuka(pg_menubar_t *mb);

/* Tutup menu aktif (bila ada). */
void pg_menubar_tutup(pg_menubar_t *mb);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_MENUBAR_H */
