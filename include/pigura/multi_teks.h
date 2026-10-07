/* ----------------------------------------------------------------------------------------------
 * pigura/multi_teks.h - Widget multiline text editor (multi teks)
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_MULTI_TEKS_H
#define PIGURA_MULTI_TEKS_H

#include "pigura/tipe.h"
#include "pigura/widget.h"
#include "pigura/font.h"
#include "pigura/papan_klip.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_multi_teks pg_multi_teks_t;

typedef void (*pg_multi_teks_cb)(pg_multi_teks_t *t, void *ctx);

pg_multi_teks_t *pg_buat_multi_teks(const char *awal, int pm,
                                      pg_font_t *font);

void pg_multi_teks_hancur(pg_multi_teks_t *t);

const char *pg_multi_teks_ambil_teks(pg_multi_teks_t *t);
void pg_multi_teks_setel_teks(pg_multi_teks_t *t, const char *t_);

void pg_multi_teks_saatberubah(pg_multi_teks_t *t,
                                  pg_multi_teks_cb cb, void *ctx);

/* Setel papan klip untuk Ctrl+C/V/X. NULL = no-op. */
void pg_multi_teks_setel_papan_klip(pg_multi_teks_t *t,
                                       pg_papan_klip_t *klip);

void pg_multi_teks_setel_aktif(pg_multi_teks_t *t, pg_bool aktif);
pg_bool pg_multi_teks_aktif(pg_multi_teks_t *t);

/* Setel word wrap: BENAR = baris panjang auto-wrap ke baris visual
 * berikutnya berdasarkan lebar widget. SALAH (default) = baris
 * hanya pecah pada karakter newline (10). */
void pg_multi_teks_setel_bungkus(pg_multi_teks_t *t, pg_bool bungkus);
pg_bool pg_multi_teks_bungkus(pg_multi_teks_t *t);

pg_widget_t *pg_multi_teks_widget(pg_multi_teks_t *t);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_MULTI_TEKS_H */
