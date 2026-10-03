#ifndef PIGURA_SEARCHBOX_H
#define PIGURA_SEARCHBOX_H
#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct pg_searchbox pg_searchbox_t;
typedef void (*pg_searchbox_cb)(pg_searchbox_t *sb, const char *teks, void *ctx);
pg_searchbox_t *pg_buat_searchbox(const char *placeholder, int panjang_maks, pg_font_t *font);
void pg_searchbox_hancur(pg_searchbox_t *sb);
const char *pg_searchbox_ambil_teks(pg_searchbox_t *sb);
void pg_searchbox_setel_teks(pg_searchbox_t *sb, const char *teks);
void pg_searchbox_saatberubah(pg_searchbox_t *sb, pg_searchbox_cb cb, void *ctx);
pg_widget_t *pg_searchbox_widget(pg_searchbox_t *sb);
#ifdef __cplusplus
}
#endif
#endif
