/* ----------------------------------------------------------------------------------------------
 * pigura/tab.h - Widget panel bertab
 * ----------------------------------------------------------------------------------------------
 * Tab adalah kontainer multi-halaman: hanya satu child yang aktif
 * (terlihat) pada satu waktu. Tab bar di atas berisi tombol per tab;
 * klik tab = pilih child aktif. Tinggi tab bar = font_tinggi + 8.
 *
 * Catatan: nama tipe pg_tab_t dipakai (bukan pg_tab_widget_t) untuk
 * keselarasan dengan API publik tugas docking.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_TAB_H
#define PIGURA_TAB_H

#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_tab pg_tab_t;

/* Dipanggil saat tab aktif berubah. */
typedef void (*pg_tab_cb)(int idx, void *ctx);

/* Buat tab panel. */
pg_tab_t *pg_buat_tab(pg_font_t *font);

/* Bebaskan tab panel (TIDAK menghancurkan child). */
void pg_tab_hancur(pg_tab_t *t);

/* Tambah tab dengan child widget. Mengembalikan indeks tab baru. */
int pg_tab_tambah(pg_tab_t *t, const char *judul,
                   pg_widget_t *child);

/* Hapus tab pada indeks (TIDAK menghancurkan child). */
void pg_tab_hapus(pg_tab_t *t, int index);

/* Pilih tab aktif. */
void pg_tab_pilih(pg_tab_t *t, int index);

/* Ambil indeks tab aktif. */
int pg_tab_aktif(const pg_tab_t *t);

/* Jumlah tab. */
int pg_tab_jumlah(const pg_tab_t *t);

/* Callback saat tab berubah. */
void pg_tab_saat_ubah(pg_tab_t *t,
    void (*cb)(int idx, void *ctx), void *ctx);

/* Akses widget dasar. */
pg_widget_t *pg_tab_widget(pg_tab_t *t);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_TAB_H */
