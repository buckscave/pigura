/* ----------------------------------------------------------------------------------------------
 * pigura/jendela_widget.h - Widget jendela dengan title bar
 * ----------------------------------------------------------------------------------------------
 * Jendela-widget adalah frame berhias: title bar biru di atas, body
 * di tengah untuk menampung anak, dan tombol close merah di kanan
 * atas title bar. Jendela bisa diseret dengan menahan klik kiri di
 * title bar.
 *
 * Nama tipe pg_jendela_widget_t (bukan pg_jendela_t) untuk membedakan
 * dari pg_jendela_t (window OS di layar.h).
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_JENDELA_WIDGET_H
#define PIGURA_JENDELA_WIDGET_H

#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_jendela_widget pg_jendela_widget_t;

/* Dipanggil saat tombol close diklik. */
typedef void (*pg_jendela_widget_cb)(pg_jendela_widget_t *w,
				      void *ctx);

/* Buat jendela-widget. judul boleh NULL. */
pg_jendela_widget_t *pg_buat_jendela_widget(const char *judul,
					     int w, int h,
					     pg_font_t *font);

/* Bebaskan jendela-widget (tidak menghancurkan anak). */
void pg_jendela_widget_hancur(pg_jendela_widget_t *w);

/* Setel anak (widget di body jendela). */
void pg_jendela_widget_setel_anak(pg_jendela_widget_t *w,
				   pg_widget_t *anak);

/* Setel callback tombol close. */
void pg_jendela_widget_saatutup(pg_jendela_widget_t *w,
				pg_jendela_widget_cb cb,
				void *ctx);

/* Akses widget dasar. */
pg_widget_t *pg_jendela_widget_widget(pg_jendela_widget_t *w);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_JENDELA_WIDGET_H */
