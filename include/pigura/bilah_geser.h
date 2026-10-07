/* ----------------------------------------------------------------------------------------------
 * pigura/bilah_geser.h - Widget slider (bilah geser) integer
 * ----------------------------------------------------------------------------------------------
 * BilahGeser adalah slider horizontal yang merepresentasikan nilai
 * integer dalam rentang [min, maks]. Pengguna menyeret kenop atau
 * mengklik track untuk mengubah nilai; callback dipanggil saat nilai
 * berubah.
 *
 * State visual:
 *   - idle     : latar PANEL, track HOVER_OUTLINE, knob ABU_TERANG
 *   - hover    : track HOVER_OUTLINE, knob HOVER_OUTLINE (gelap)
 *   - tekan    : knob TEKAN_ISI (gelap)
 *   - fokus    : outline FOKUS (biru) di keliling widget
 *   - disabled : latar NONAKTIF_ISI, knob ABU_TERANG, tidak respon input
 *
 * Geometri:
 *   - Track: garis horizontal di tengah, height 4px, radius 2.
 *   - Knob : lingkaran radius 6, warna state-aware.
 *   - Range: dari track_x_start ke track_x_end, padding 8px di kiri/kanan.
 *
 * API:
 *   - pg_buat_bilah_geser(min, maks, nilai)
 *   - pg_bilah_geser_nilai / setel_nilai
 *   - pg_bilah_geser_saatberubah(cb, ctx)
 *   - pg_bilah_geser_setel_aktif(aktif)
 *   - pg_bilah_geser_widget
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_BILAH_GESER_H
#define PIGURA_BILAH_GESER_H

#include "pigura/tipe.h"
#include "pigura/widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_bilah_geser pg_bilah_geser_t;

/* Dipanggil saat nilai slider berubah (drag knob, klik track, atau
 * setel_nilai yang mengubah nilai). */
typedef void (*pg_bilah_geser_cb)(pg_bilah_geser_t *g, int nilai,
                                    void *ctx);

/* Buat slider. nilai awal diklem ke [min, maks]. */
pg_bilah_geser_t *pg_buat_bilah_geser(int min, int maks, int nilai);

/* Bebaskan slider. */
void pg_bilah_geser_hancur(pg_bilah_geser_t *g);

/* Ambil nilai saat ini. */
int pg_bilah_geser_nilai(pg_bilah_geser_t *g);

/* Setel nilai (diklem ke rentang). Memicu callback bila berubah. */
void pg_bilah_geser_setel_nilai(pg_bilah_geser_t *g, int nilai);

/* Setel rentang [min, maks]. Nilai saat ini diklem ke rentang baru. */
void pg_bilah_geser_setel_rentang(pg_bilah_geser_t *g,
                                    int min, int maks);

/* Setel callback perubahan. */
void pg_bilah_geser_saatberubah(pg_bilah_geser_t *g,
                                  pg_bilah_geser_cb cb, void *ctx);

/* Aktif / nonaktif. Nonaktif: tidak respon input, visual disabled. */
void pg_bilah_geser_setel_aktif(pg_bilah_geser_t *g, pg_bool aktif);
pg_bool pg_bilah_geser_aktif(pg_bilah_geser_t *g);

/* Akses widget dasar. */
pg_widget_t *pg_bilah_geser_widget(pg_bilah_geser_t *g);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_BILAH_GESER_H */
