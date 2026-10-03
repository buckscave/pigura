#ifndef PIGURA_TABLEVIEW_H
#define PIGURA_TABLEVIEW_H
#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct pg_tableview pg_tableview_t;
typedef void (*pg_tableview_cb)(pg_tableview_t *tv, int row, int col, void *ctx);
pg_tableview_t *pg_buat_tableview(pg_font_t *font);
void pg_tableview_hancur(pg_tableview_t *tv);
void pg_tableview_setel_kolom(pg_tableview_t *tv, int n_kolom, const char **judul);
void pg_tableview_tambah_baris(pg_tableview_t *tv, const char **sel);
void pg_tableview_saatpilih(pg_tableview_t *tv, pg_tableview_cb cb, void *ctx);
pg_widget_t *pg_tableview_widget(pg_tableview_t *tv);
#ifdef __cplusplus
}
#endif
#endif
