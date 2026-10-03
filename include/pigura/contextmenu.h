#ifndef PIGURA_CONTEXTMENU_H
#define PIGURA_CONTEXTMENU_H
#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct pg_contextmenu pg_contextmenu_t;
typedef void (*pg_contextmenu_cb)(pg_contextmenu_t *cm, int idx, void *ctx);
pg_contextmenu_t *pg_buat_contextmenu(pg_font_t *font);
void pg_contextmenu_hancur(pg_contextmenu_t *cm);
void pg_contextmenu_tambah_item(pg_contextmenu_t *cm, const char *label, pg_contextmenu_cb cb, void *ctx);
void pg_contextmenu_tambah_pemisah(pg_contextmenu_t *cm);
void pg_contextmenu_tampil(pg_contextmenu_t *cm, int x, int y);
void pg_contextmenu_sembunyi(pg_contextmenu_t *cm);
pg_bool pg_contextmenu_terlihat(pg_contextmenu_t *cm);
pg_widget_t *pg_contextmenu_widget(pg_contextmenu_t *cm);
#ifdef __cplusplus
}
#endif
#endif
