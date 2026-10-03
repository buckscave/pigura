#ifndef PIGURA_TREEVIEW_H
#define PIGURA_TREEVIEW_H
#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct pg_treeview pg_treeview_t;
typedef void (*pg_treeview_cb)(pg_treeview_t *tv, int node_id, void *ctx);
pg_treeview_t *pg_buat_treeview(pg_font_t *font);
void pg_treeview_hancur(pg_treeview_t *tv);
int pg_treeview_tambah_node(pg_treeview_t *tv, int parent, const char *label);
void pg_treeview_saatpilih(pg_treeview_t *tv, pg_treeview_cb cb, void *ctx);
pg_widget_t *pg_treeview_widget(pg_treeview_t *tv);
#ifdef __cplusplus
}
#endif
#endif
