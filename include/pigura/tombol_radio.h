/* ----------------------------------------------------------------------------------------------
 * pigura/tombol_radio.h - Widget radio button (tombol radio) bergrup
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_TOMBOL_RADIO_H
#define PIGURA_TOMBOL_RADIO_H

#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_tombol_radio_grup pg_tombol_radio_grup_t;
typedef struct pg_tombol_radio pg_tombol_radio_t;

typedef void (*pg_tombol_radio_cb)(pg_tombol_radio_t *r, void *ctx);

pg_tombol_radio_grup_t *pg_buat_tombol_radio_grup(void);

void pg_tombol_radio_grup_hancur(pg_tombol_radio_grup_t *g);

pg_tombol_radio_t *pg_buat_tombol_radio(pg_tombol_radio_grup_t *g,
                                           const char *label,
                                           pg_font_t *font);

void pg_tombol_radio_hancur(pg_tombol_radio_t *r);

/* ===== State ===== */

pg_bool pg_tombol_radio_terpilih(pg_tombol_radio_t *r);

void pg_tombol_radio_pilih(pg_tombol_radio_t *r);

/* ===== Layout ===== */

void pg_tombol_radio_setel_posisi(pg_tombol_radio_t *r,
                                    pg_posisi_t pos);
void pg_tombol_radio_setel_ukuran(pg_tombol_radio_t *r, int px);
void pg_tombol_radio_setel_padding(pg_tombol_radio_t *r,
                                     int padding);

/* ===== Warna ===== */

void pg_tombol_radio_setel_warna(pg_tombol_radio_t *r,
                                    pg_warna_t fg,
                                    pg_warna_t lingk_batas);

/* ===== Callback ===== */

void pg_tombol_radio_saatberubah(pg_tombol_radio_t *r,
                                    pg_tombol_radio_cb cb,
                                    void *ctx);

/* ===== Disabled ===== */

void pg_tombol_radio_setel_aktif(pg_tombol_radio_t *r,
                                    pg_bool aktif);

/* ===== Akses widget ===== */

pg_widget_t *pg_tombol_radio_widget(pg_tombol_radio_t *r);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_TOMBOL_RADIO_H */
