#ifndef PIGURA_COMBOBOX_H
#define PIGURA_COMBOBOX_H
#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct pg_combobox pg_combobox_t;
typedef void (*pg_combobox_cb)(pg_combobox_t *cb, int idx, void *ctx);
pg_combobox_t *pg_buat_combobox(const char *awal, int panjang_maks, pg_font_t *font);
void pg_combobox_hancur(pg_combobox_t *cb);
const char *pg_combobox_ambil_teks(pg_combobox_t *cb);
void pg_combobox_setel_teks(pg_combobox_t *cb, const char *teks);
void pg_combobox_tambah_item(pg_combobox_t *cb, const char *teks);
void pg_combobox_saatberubah(pg_combobox_t *cb, pg_combobox_cb cb_fn, void *ctx);
pg_widget_t *pg_combobox_widget(pg_combobox_t *cb);
pg_bool pg_combobox_terbuka(pg_combobox_t *cb);
void pg_combobox_tutup(pg_combobox_t *cb);
#ifdef __cplusplus
}
#endif
#endif
