/* ----------------------------------------------------------------------------------------------
 * pigura/geser.h - Widget slider integer
 * ----------------------------------------------------------------------------------------------
 * Geser adalah slider horizontal yang merepresentasikan nilai
 * integer dalam rentang [min, maks]. Pengguna menyeret kenop atau
 * mengklik track untuk mengubah nilai; callback dipanggil saat nilai
 * berubah.
 *
 * Nilai diklem ke rentang [min, maks]. Jika min >= maks, widget
 * menjadi read-only (nilai tetap = min).
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_GESER_H
#define PIGURA_GESER_H

#include "pigura/tipe.h"
#include "pigura/widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_geser pg_geser_t;

/* Dipanggil saat nilai slider berubah. */
typedef void (*pg_geser_cb)(pg_geser_t *g, int nilai, void *ctx);

/* Buat slider. nilai awal diklem ke [min,maks]. */
pg_geser_t *pg_buat_geser(int min, int maks, int nilai);

/* Bebaskan slider. */
void pg_geser_hancur(pg_geser_t *g);

/* Ambil nilai saat ini. */
int pg_geser_nilai(pg_geser_t *g);

/* Setel nilai (diklem ke rentang). */
void pg_geser_setel_nilai(pg_geser_t *g, int nilai);

/* Setel callback perubahan. */
void pg_geser_saatberubah(pg_geser_t *g, pg_geser_cb cb,
			   void *ctx);

/* Akses widget dasar. */
pg_widget_t *pg_geser_widget(pg_geser_t *g);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_GESER_H */
