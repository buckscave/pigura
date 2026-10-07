/* ----------------------------------------------------------------------------------------------
 * pigura/angka_putar.h - Widget spinbox (angka putar) integer
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_ANGKA_PUTAR_H
#define PIGURA_ANGKA_PUTAR_H

#include "pigura/tipe.h"
#include "pigura/widget.h"
#include "pigura/font.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_angka_putar pg_angka_putar_t;

typedef void (*pg_angka_putar_cb)(pg_angka_putar_t *sb,
                                   int nilai, void *ctx);

pg_angka_putar_t *pg_buat_angka_putar(int min, int maks,
                                        int nilai, int langkah,
                                        pg_font_t *font);

void pg_angka_putar_hancur(pg_angka_putar_t *sb);

int pg_angka_putar_nilai(pg_angka_putar_t *sb);
void pg_angka_putar_setel_nilai(pg_angka_putar_t *sb, int nilai);
void pg_angka_putar_setel_rentang(pg_angka_putar_t *sb,
                                    int min, int maks, int langkah);

void pg_angka_putar_saatberubah(pg_angka_putar_t *sb,
                                  pg_angka_putar_cb cb, void *ctx);

void pg_angka_putar_setel_aktif(pg_angka_putar_t *sb, pg_bool aktif);
pg_bool pg_angka_putar_aktif(pg_angka_putar_t *sb);

pg_widget_t *pg_angka_putar_widget(pg_angka_putar_t *sb);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_ANGKA_PUTAR_H */
