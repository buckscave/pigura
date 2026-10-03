#ifndef PIGURA_SPINBOX_H
#define PIGURA_SPINBOX_H
#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct pg_spinbox pg_spinbox_t;
typedef void (*pg_spinbox_cb)(pg_spinbox_t *sb, int nilai, void *ctx);
pg_spinbox_t *pg_buat_spinbox(int min, int maks, int nilai, int langkah, pg_font_t *font);
void pg_spinbox_hancur(pg_spinbox_t *sb);
int pg_spinbox_ambil_nilai(pg_spinbox_t *sb);
void pg_spinbox_setel_nilai(pg_spinbox_t *sb, int nilai);
void pg_spinbox_saatberubah(pg_spinbox_t *sb, pg_spinbox_cb cb, void *ctx);
pg_widget_t *pg_spinbox_widget(pg_spinbox_t *sb);
#ifdef __cplusplus
}
#endif
#endif
