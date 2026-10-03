#ifndef PIGURA_DATEPICKER_H
#define PIGURA_DATEPICKER_H
#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct pg_datepicker pg_datepicker_t;
typedef void (*pg_datepicker_cb)(pg_datepicker_t *dp, int tahun, int bulan, int hari, void *ctx);
pg_datepicker_t *pg_buat_datepicker(pg_font_t *font);
void pg_datepicker_hancur(pg_datepicker_t *dp);
void pg_datepicker_setel_tanggal(pg_datepicker_t *dp, int tahun, int bulan, int hari);
void pg_datepicker_saatubah(pg_datepicker_t *dp, pg_datepicker_cb cb, void *ctx);
pg_widget_t *pg_datepicker_widget(pg_datepicker_t *dp);
#ifdef __cplusplus
}
#endif
#endif
