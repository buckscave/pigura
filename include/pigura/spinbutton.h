#ifndef PIGURA_SPINBUTTON_H
#define PIGURA_SPINBUTTON_H
#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct pg_spinbutton pg_spinbutton_t;
typedef void (*pg_spinbutton_cb)(pg_spinbutton_t *sb, int naik, void *ctx);
pg_spinbutton_t *pg_buat_spinbutton(pg_font_t *font);
void pg_spinbutton_hancur(pg_spinbutton_t *sb);
void pg_spinbutton_saatklik(pg_spinbutton_t *sb, pg_spinbutton_cb cb, void *ctx);
pg_widget_t *pg_spinbutton_widget(pg_spinbutton_t *sb);
#ifdef __cplusplus
}
#endif
#endif
