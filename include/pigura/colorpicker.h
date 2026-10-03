#ifndef PIGURA_COLORPICKER_H
#define PIGURA_COLORPICKER_H
#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct pg_colorpicker pg_colorpicker_t;
typedef void (*pg_colorpicker_cb)(pg_colorpicker_t *cp, pg_warna_t warna, void *ctx);
pg_colorpicker_t *pg_buat_colorpicker(pg_font_t *font);
void pg_colorpicker_hancur(pg_colorpicker_t *cp);
pg_warna_t pg_colorpicker_ambil_warna(pg_colorpicker_t *cp);
void pg_colorpicker_setel_warna(pg_colorpicker_t *cp, pg_warna_t w);
void pg_colorpicker_saatberubah(pg_colorpicker_t *cp, pg_colorpicker_cb cb, void *ctx);
pg_widget_t *pg_colorpicker_widget(pg_colorpicker_t *cp);
#ifdef __cplusplus
}
#endif
#endif
