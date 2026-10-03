/* ----------------------------------------------------------------------------------------------
 * pigura/gambar_widget.h - Widget tampilan piksel RGB
 * ----------------------------------------------------------------------------------------------
 * Gambar-widget menampilkan buffer piksel RGB (array pg_warna_t)
 * berukuran w x h. Buffer disalin ke permukaan widget saat catat.
 * Cocok untuk menampilkan gambar yang di-generate secara prosedural
 * atau di-decode dari berkas.
 *
 * Nama tipe pg_gambar_widget_t (bukan pg_gambar_t) untuk membedakan
 * dari modul pg_gambar (primitif gambar AA).
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_GAMBAR_WIDGET_H
#define PIGURA_GAMBAR_WIDGET_H

#include "pigura/tipe.h"
#include "pigura/widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_gambar_widget pg_gambar_widget_t;

/* Buat widget gambar. piksel boleh NULL (latar hitam). */
pg_gambar_widget_t *pg_buat_gambar_widget(int w, int h,
					    const pg_warna_t *piksel);

/* Bebaskan widget. */
void pg_gambar_widget_hancur(pg_gambar_widget_t *g);

/* Ganti seluruh piksel. Ukuran baru mengubah ukuran widget. */
void pg_gambar_widget_setel_piksel(pg_gambar_widget_t *g, int w,
				    int h, const pg_warna_t *piksel);

/* Akses widget dasar. */
pg_widget_t *pg_gambar_widget_widget(pg_gambar_widget_t *g);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_GAMBAR_WIDGET_H */
