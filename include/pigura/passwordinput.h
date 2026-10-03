#ifndef PIGURA_PASSWORDINPUT_H
#define PIGURA_PASSWORDINPUT_H
#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct pg_passwordinput pg_passwordinput_t;
typedef void (*pg_passwordinput_cb)(pg_passwordinput_t *pi, void *ctx);
pg_passwordinput_t *pg_buat_passwordinput(const char *awal, int panjang_maks, pg_font_t *font);
void pg_passwordinput_hancur(pg_passwordinput_t *pi);
const char *pg_passwordinput_ambil_teks(pg_passwordinput_t *pi);
void pg_passwordinput_setel_teks(pg_passwordinput_t *pi, const char *teks);
void pg_passwordinput_saatberubah(pg_passwordinput_t *pi, pg_passwordinput_cb cb, void *ctx);
pg_widget_t *pg_passwordinput_widget(pg_passwordinput_t *pi);
#ifdef __cplusplus
}
#endif
#endif
