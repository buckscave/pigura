/* ----------------------------------------------------------------------------------------------
 * pigura/kotak_pencarian.h - Widget searchbox (kotak pencarian)
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_KOTAK_PENCARIAN_H
#define PIGURA_KOTAK_PENCARIAN_H

#include "pigura/tipe.h"
#include "pigura/widget.h"
#include "pigura/font.h"
#include "pigura/papan_klip.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_kotak_pencarian pg_kotak_pencarian_t;

typedef void (*pg_kotak_pencarian_cb)(pg_kotak_pencarian_t *sb,
                                        const char *teks, void *ctx);

pg_kotak_pencarian_t *pg_buat_kotak_pencarian(const char *placeholder,
                                                 int pm,
                                                 pg_font_t *font);

void pg_kotak_pencarian_hancur(pg_kotak_pencarian_t *sb);

const char *pg_kotak_pencarian_ambil_teks(pg_kotak_pencarian_t *sb);
void pg_kotak_pencarian_setel_teks(pg_kotak_pencarian_t *sb,
                                     const char *t);

void pg_kotak_pencarian_saatberubah(pg_kotak_pencarian_t *sb,
                                       pg_kotak_pencarian_cb cb,
                                       void *ctx);

/* Setel papan klip untuk Ctrl+C/V/X. NULL = no-op. */
void pg_kotak_pencarian_setel_papan_klip(pg_kotak_pencarian_t *sb,
                                            pg_papan_klip_t *klip);

void pg_kotak_pencarian_setel_aktif(pg_kotak_pencarian_t *sb,
                                       pg_bool aktif);
pg_bool pg_kotak_pencarian_aktif(pg_kotak_pencarian_t *sb);

pg_widget_t *pg_kotak_pencarian_widget(pg_kotak_pencarian_t *sb);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_KOTAK_PENCARIAN_H */
