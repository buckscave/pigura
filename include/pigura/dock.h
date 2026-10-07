/* -------------------------------------------------------------------------- *
 * pigura/dock.h - Dock manager (wrapper pattern, multi-panel)
 * -------------------------------------------------------------------------- *
 * Dock = div/wrapper seperti HTML. Menampung multiple panel berurutan,
 * dengan pembatas splitter dragable antar panel.
 *
 * Flag dock menentukan jenis panel yang bisa masuk:
 *   - PG_DOCK_FLAG_TOOLBAR    → hanya panel FLAG_TOOLBAR
 *   - PG_DOCK_FLAG_RIBBON     → hanya panel FLAG_RIBBON
 *   - PG_DOCK_FLAG_PROPERTIES → hanya panel FLAG_PROPERTIES
 *
 * Posisi dock menentukan orientasi & outline:
 *   - ATAS/BAWAH → layout horizontal, outline di sisi bawah/atas
 *   - KIRI/KANAN → layout vertikal, outline di sisi kanan/kiri
 *
 * Header panel auto-set saat masuk dock:
 *   - PROPERTIES dock → header THIN
 *   - TOOLBAR/RIBBON dock → header GRIP (orientasi = dock orientasi)
 *
 * Dock TIDAK create/free panel. App yang punya (lewat parent-child tree).
 * -------------------------------------------------------------------------- */
#ifndef PIGURA_DOCK_H
#define PIGURA_DOCK_H

#include "pigura/tipe.h"
#include "pigura/widget.h"
#include "pigura/font.h"
#include "pigura/permukaan.h"
#include "pigura/aksi.h"
#include "pigura/panel.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_dock pg_dock_t;

/* Flag/jenis dock. */
typedef enum {
        PG_DOCK_FLAG_TOOLBAR = 0,
        PG_DOCK_FLAG_RIBBON = 1,
        PG_DOCK_FLAG_PROPERTIES = 2
} pg_dock_flag_t;

/* Posisi dock di layar. */
typedef enum {
        PG_DOCK_POS_ATAS = 0,
        PG_DOCK_POS_BAWAH = 1,
        PG_DOCK_POS_KIRI = 2,
        PG_DOCK_POS_KANAN = 3
} pg_dock_posisi_t;

/* Buat dock. */
pg_dock_t *pg_buat_dock(pg_dock_flag_t flag, pg_dock_posisi_t posisi);

/* Hancurkan dock struct. Panel TIDAK dihancurkan (app punya). */
void pg_dock_hancur(pg_dock_t *d);

/* Identitas. */
pg_dock_flag_t pg_dock_flag(const pg_dock_t *d);
pg_dock_posisi_t pg_dock_posisi(const pg_dock_t *d);

/* Setel area dock di layar. */
void pg_dock_setel_area(pg_dock_t *d, int x, int y, int w, int h);
pg_kotak_t pg_dock_area(const pg_dock_t *d);

/* Setel font (untuk render pembatas, dll). */
void pg_dock_setel_font(pg_dock_t *d, pg_font_t *font);

/* Tambah panel ke dock di posisi idx (-1 = append).
 * Cek flag cocok — return SALAH bila tidak cocok.
 * Auto-set header mode panel sesuai flag dock. */
pg_bool pg_dock_tambah_panel(pg_dock_t *d, pg_panel_t *p, int idx);

/* Hapus panel dari dock. */
void pg_dock_hapus_panel(pg_dock_t *d, pg_panel_t *p);

/* Cek apakah panel ada di dock ini. */
pg_bool pg_dock_punya_panel(pg_dock_t *d, pg_panel_t *p);

/* Jumlah panel di dock. */
int pg_dock_jumlah_panel(const pg_dock_t *d);

/* Ambil panel di indeks. */
pg_panel_t *pg_dock_panel_di(const pg_dock_t *d, int idx);

/* Hit-test: apakah titik (mx,my) di area dock. */
pg_bool pg_dock_berisi(pg_dock_t *d, int mx, int my);

/* Hit-test flag cocok untuk insert panel.
 * Dipakai saat drag panel untuk cek apakah bisa masuk dock ini. */
pg_bool pg_dock_terima_flag(pg_dock_t *d, pg_panel_flag_t flag);

/* Cari indeks panel pada posisi mouse (untuk insert di posisi mouse).
 * Return indeks untuk insert (0..n_panels). Bila mouse di paruh kiri
 * panel ke-i, return i (insert sebelum panel i). Bila di paruh kanan,
 * return i+1 (insert setelah panel i). */
int pg_dock_indeks_insert(pg_dock_t *d, int mx, int my);

/* Ambil indeks panel di posisi mouse (untuk drag keluar).
 * Return -1 bila tidak ada panel di posisi mouse. */
int pg_dock_indeks_panel_di(pg_dock_t *d, int mx, int my);

/* Layout ulang: susun panel + splitter. */
void pg_dock_tata(pg_dock_t *d);

/* Render: latar + outline + pembatas antar panel.
 * Bila dock kosong (n_panels==0), tidak render apa-apa (invisible). */
void pg_dock_catat(pg_dock_t *d, pg_permukaan_t *dest);

/* Render hint outline biru (mis. saat ada panel di-drag ke atas dock).
 * Hanya render bila dock berisi atau flag cocok untuk highlight. */
void pg_dock_catat_hint(pg_dock_t *d, pg_permukaan_t *dest,
                          int mx, int my, pg_panel_flag_t flag_drag);

/* Tangani peristiwa: splitter drag. */
pg_bool pg_dock_tangani(pg_dock_t *d, const pg_aksi_t *e);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_DOCK_H */
