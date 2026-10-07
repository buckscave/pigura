/* -------------------------------------------------------------------------- *
 * pigura/panel.h - Panel (entitas UI mandiri)
 * -------------------------------------------------------------------------- *
 * Panel adalah entitas UI tunggal: punya header + body + child widget.
 * Bisa floating (di luar dock) atau docked (di dalam dock).
 *
 * Flag panel menentukan dock mana yang bisa menampungnya:
 *   - PG_PANEL_FLAG_TOOLBAR    → hanya dock bertipe TOOLBAR
 *   - PG_PANEL_FLAG_RIBBON     → hanya dock bertipe RIBBON
 *   - PG_PANEL_FLAG_PROPERTIES → hanya dock bertipe PROPERTIES
 *
 * Header mode otomatis berdasarkan konteks parent:
 *   - Floating                   → FULL (biru, judul + 3 tombol kanan)
 *   - Docked di PROPERTIES       → THIN (warna body, judul + 3 tombol)
 *   - Docked di TOOLBAR/RIBBON   → GRIP (box gelap, no judul, no tombol)
 *
 * Header mode GRIP: orientasi mengikuti dock
 *   - Dock vertikal (kiri/kanan) → grip di atas panel
 *   - Dock horizontal (atas/bawah) → grip di kiri panel
 *
 * Tombol header (untuk mode FULL/THIN):
 *   - Lipat (collapse): panel jadi tipis, hanya header tersisa
 *   - Semula (restore): toggle docked ↔ floating
 *   - Tutup (close):    sembunyikan panel
 * -------------------------------------------------------------------------- */
#ifndef PIGURA_PANEL_H
#define PIGURA_PANEL_H

#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"
#include "pigura/permukaan.h"
#include "pigura/aksi.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PIGURA_PANEL_T_DEFINED
#define PIGURA_PANEL_T_DEFINED
typedef struct pg_panel pg_panel_t;
#endif

/* Flag/jenis panel. */
typedef enum {
        PG_PANEL_FLAG_TOOLBAR = 0,
        PG_PANEL_FLAG_RIBBON = 1,
        PG_PANEL_FLAG_PROPERTIES = 2,
        PG_PANEL_FLAG_KANVAS = 3   /* area kerja, floating only — tidak
                                    * bisa masuk dock manapun */
} pg_panel_flag_t;

/* Mode header (auto-set oleh parent dock). */
typedef enum {
        PG_PANEL_HEADER_FULL = 0,  /* floating: biru + judul + 3 tombol */
        PG_PANEL_HEADER_THIN = 1,  /* docked properties: body color + judul + 3 tombol */
        PG_PANEL_HEADER_GRIP = 2   /* docked toolbar/ribbon: box gelap, no judul */
} pg_panel_header_mode_t;

/* State panel. */
typedef enum {
        PG_PANEL_FLOATING = 0,
        PG_PANEL_DOCKED = 1
} pg_panel_mode_t;

/* Callback signatures. */
typedef void (*pg_panel_cb)(pg_panel_t *p, void *ctx);
typedef void (*pg_panel_lipat_cb)(pg_panel_t *p, pg_bool collapsed, void *ctx);
typedef void (*pg_panel_drag_cb)(pg_panel_t *p, int x, int y, void *ctx);
typedef void (*pg_panel_drag_gerak_cb)(pg_panel_t *p, int x, int y,
                                          void *ctx);

/* Buat panel. judul boleh NULL. w/h = ukuran awal floating. */
pg_panel_t *pg_buat_panel(const char *judul, pg_panel_flag_t flag,
                           int w, int h, pg_font_t *font);

/* Hancurkan panel (TIDAK menghancurkan child widget — app yang punya). */
void pg_panel_hancur(pg_panel_t *p);

/* Setel child widget di body. */
void pg_panel_setel_anak(pg_panel_t *p, pg_widget_t *child);

/* Ambil child widget. */
pg_widget_t *pg_panel_anak(pg_panel_t *p);

/* Identitas. */
pg_panel_flag_t pg_panel_flag(const pg_panel_t *p);
const char *pg_panel_judul(const pg_panel_t *p);
void pg_panel_setel_judul(pg_panel_t *p, const char *judul);

/* State. */
pg_panel_mode_t pg_panel_mode(const pg_panel_t *p);
pg_bool pg_panel_collapsed(const pg_panel_t *p);
void pg_panel_setel_collapsed(pg_panel_t *p, pg_bool collapsed);

/* Header mode (auto-set oleh dock, manual untuk floating). */
pg_panel_header_mode_t pg_panel_header_mode(const pg_panel_t *p);
void pg_panel_setel_header_mode(pg_panel_t *p, pg_panel_header_mode_t mode);

/* Posisi & ukuran. */
void pg_panel_pindah(pg_panel_t *p, int x, int y);
void pg_panel_setel_ukuran(pg_panel_t *p, int w, int h);
pg_kotak_t pg_panel_kotak(const pg_panel_t *p);

/* Tampil/sembunyi. */
void pg_panel_tampil(pg_panel_t *p);
void pg_panel_sembunyi(pg_panel_t *p);
pg_bool pg_panel_terlihat(const pg_panel_t *p);

/* Callbacks. */
void pg_panel_saat_tutup(pg_panel_t *p, pg_panel_cb cb, void *ctx);
void pg_panel_saat_semula(pg_panel_t *p, pg_panel_cb cb, void *ctx);
void pg_panel_saat_lipat(pg_panel_t *p, pg_panel_lipat_cb cb, void *ctx);
void pg_panel_saat_drag_selesai(pg_panel_t *p, pg_panel_drag_cb cb, void *ctx);

/* Setel callback saat panel sedang di-drag (dipanggil tiap TETIK_GERAK).
 * Berguna untuk real-time feedback: mis. app cek apakah mouse di atas
 * dock, tampilkan hint outline. */
void pg_panel_saat_drag_gerak(pg_panel_t *p, pg_panel_drag_gerak_cb cb,
                                void *ctx);

/* Render ke dest (always-on-top). */
void pg_panel_catat(pg_panel_t *p, pg_permukaan_t *dest);

/* Tangani peristiwa. Return BENAR bila dikonsumsi. */
pg_bool pg_panel_tangani(pg_panel_t *p, const pg_aksi_t *e);

/* Ambil widget dasar (untuk parent-child tree). */
pg_widget_t *pg_panel_widget(pg_panel_t *p);

/* Hit-test apakah titik (layar) di dalam panel. */
pg_bool pg_panel_berisi(const pg_panel_t *p, pg_titik_t pt);

/* ===== API internal (dipanggil oleh dock) ===== */

/* Setel orientasi grip (vertikal/horizontal). Hanya relevan untuk
 * header mode GRIP. Dipanggil otomatis oleh dock saat menambah panel. */
void pg_panel_setel_grip_vertikal(pg_panel_t *p, pg_bool vertikal);

/* Setel mode panel (floating/docked). Dipanggil otomatis oleh dock. */
void pg_panel_setel_mode(pg_panel_t *p, pg_panel_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_PANEL_H */
