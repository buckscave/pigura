/* ----------------------------------------------------------------------------------------------
 * pigura/kotak_penanda.h - Widget checkbox (kotak penanda)
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_KOTAK_PENANDA_H
#define PIGURA_KOTAK_PENANDA_H

#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_kotak_penanda pg_kotak_penanda_t;

/* Tri-state untuk checkbox. */
typedef enum {
        PG_TRI_UBAH          = 0, /* unchecked */
        PG_TRI_TERPILIH     = 1, /* checked, tanda X */
        PG_TRI_INDETERMINATE = 2 /* dash/tanda minus */
} pg_tri_t;

typedef void (*pg_kotak_penanda_cb)(pg_kotak_penanda_t *p,
                                      pg_bool dicek, void *ctx);

pg_kotak_penanda_t *pg_buat_kotak_penanda(const char *label,
                                            pg_font_t *font);

void pg_kotak_penanda_hancur(pg_kotak_penanda_t *p);

/* ===== State ===== */

pg_bool pg_kotak_penanda_dicek(pg_kotak_penanda_t *p);
void pg_kotak_penanda_setel_dicek(pg_kotak_penanda_t *p,
                                    pg_bool dicek);

pg_tri_t pg_kotak_penanda_tri(pg_kotak_penanda_t *p);
void pg_kotak_penanda_setel_tri(pg_kotak_penanda_t *p,
                                  pg_tri_t tri);

/* ===== Layout ===== */

void pg_kotak_penanda_setel_posisi(pg_kotak_penanda_t *p,
                                     pg_posisi_t pos);
void pg_kotak_penanda_setel_ukuran_kotak(pg_kotak_penanda_t *p,
                                           int px);
void pg_kotak_penanda_setel_padding(pg_kotak_penanda_t *p,
                                      int padding);

/* ===== Warna ===== */

void pg_kotak_penanda_setel_warna(pg_kotak_penanda_t *p,
                                    pg_warna_t fg,
                                    pg_warna_t kotak_bg,
                                    pg_warna_t kotak_batas);

/* ===== Callback ===== */

void pg_kotak_penanda_saatberubah(pg_kotak_penanda_t *p,
                                     pg_kotak_penanda_cb cb,
                                     void *ctx);

/* ===== Disabled ===== */

void pg_kotak_penanda_setel_aktif(pg_kotak_penanda_t *p,
                                    pg_bool aktif);

/* ===== Akses widget ===== */

pg_widget_t *pg_kotak_penanda_widget(pg_kotak_penanda_t *p);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_KOTAK_PENANDA_H */
