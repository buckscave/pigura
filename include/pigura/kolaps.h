/* ----------------------------------------------------------------------------------------------
 * pigura/kolaps.h - Panel kolaps (collapsible)
 * ----------------------------------------------------------------------------------------------
 * Kolaps adalah kontainer ber-title-bar yang bisa di-collapse
 * (tinggal title bar saja) atau di-expand (title bar + child).
 * Tombol [▼]/[▶] di kiri title bar untuk toggle.
 *
 * Saat collapse: tinggi widget = title bar (24 piksel default).
 * Saat expand:   tinggi widget = title bar + tinggi child.
 *
 * Karena tinggi berubah-ubah, parent yang memakai layout manager
 * linear (kotak) akan otomatis menyusun ulang anak-anaknya.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_KOLAPS_H
#define PIGURA_KOLAPS_H

#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_kolaps pg_kolaps_t;

/* Buat collapsible panel. judul boleh NULL. */
pg_kolaps_t *pg_buat_kolaps(const char *judul, pg_font_t *font);

/* Bebaskan kolaps (TIDAK menghancurkan child). */
void pg_kolaps_hancur(pg_kolaps_t *k);

/* Setel child. */
void pg_kolaps_setel_anak(pg_kolaps_t *k, pg_widget_t *child);

/* Collapse/expand. */
void pg_kolaps_setel_kolaps(pg_kolaps_t *k, pg_bool kolaps);
pg_bool pg_kolaps_apakah_kolaps(const pg_kolaps_t *k);

/* Toggle kolaps. */
void pg_kolaps_toggle(pg_kolaps_t *k);

/* Setel judul. */
void pg_kolaps_setel_judul(pg_kolaps_t *k, const char *judul);

/* Ambil widget dasar. */
pg_widget_t *pg_kolaps_widget(pg_kolaps_t *k);

/* Tinggi title bar (default 24). */
int pg_kolaps_title_h(const pg_kolaps_t *k);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_KOLAPS_H */
