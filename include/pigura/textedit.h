/* ----------------------------------------------------------------------------------------------
 * pigura/textedit.h - Widget input teks multi-baris
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_TEXTEDIT_H
#define PIGURA_TEXTEDIT_H

#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_textedit pg_textedit_t;

typedef void (*pg_textedit_cb)(pg_textedit_t *te, void *ctx);

pg_textedit_t *pg_textedit_buat(const char *awal,
                                int panjang_maks,
                                pg_font_t *font);
void pg_textedit_hancur(pg_textedit_t *te);
void pg_textedit_setel_teks(pg_textedit_t *te, const char *teks);
const char *pg_textedit_ambil_teks(pg_textedit_t *te);
void pg_textedit_saatberubah(pg_textedit_t *te,
                             pg_textedit_cb cb, void *ctx);
char *pg_textedit_ambil_pilihan(pg_textedit_t *te);
void pg_textedit_sisip(pg_textedit_t *te, const char *teks);
pg_widget_t *pg_textedit_widget(pg_textedit_t *te);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_TEXTEDIT_H */
