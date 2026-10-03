/* ----------------------------------------------------------------------------------------------
 * pigura/kemajuan.h - Widget progress bar
 * ----------------------------------------------------------------------------------------------
 * Kemajuan adalah progress bar horizontal yang menampilkan nilai
 * integer dalam rentang [min, maks] sebagai bagian terisi. Widget
 * ini read-only: tidak menerima peristiwa input.
 *
 * Bagian terisi = (nilai - min) / (maks - min) * lebar.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_KEMAJUAN_H
#define PIGURA_KEMAJUAN_H

#include "pigura/tipe.h"
#include "pigura/widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_kemajuan pg_kemajuan_t;

/* Buat progress bar. */
pg_kemajuan_t *pg_buat_kemajuan(int min, int maks);

/* Bebaskan progress bar. */
void pg_kemajuan_hancur(pg_kemajuan_t *k);

/* Ambil nilai saat ini. */
int pg_kemajuan_nilai(pg_kemajuan_t *k);

/* Setel nilai (diklem ke rentang). */
void pg_kemajuan_setel_nilai(pg_kemajuan_t *k, int nilai);

/* Akses widget dasar. */
pg_widget_t *pg_kemajuan_widget(pg_kemajuan_t *k);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_KEMAJUAN_H */
