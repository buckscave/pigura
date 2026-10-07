/* ----------------------------------------------------------------------------------------------
 * pigura/kotak_gabungan.h - Widget combobox (kotak gabungan)
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_KOTAK_GABUNGAN_H
#define PIGURA_KOTAK_GABUNGAN_H

#include "pigura/tipe.h"
#include "pigura/widget.h"
#include "pigura/font.h"
#include "pigura/papan_klip.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_kotak_gabungan pg_kotak_gabungan_t;

typedef void (*pg_kotak_gabungan_cb)(pg_kotak_gabungan_t *cb,
                                       int idx, void *ctx);

/* Buat combobox editable. pm = panjang maksimum teks (default 64). */
pg_kotak_gabungan_t *pg_buat_kotak_gabungan(const char *awal,
                                              int pm, pg_font_t *font);

void pg_kotak_gabungan_hancur(pg_kotak_gabungan_t *cb);

/* Teks field (editable). */
const char *pg_kotak_gabungan_ambil_teks(pg_kotak_gabungan_t *cb);
void pg_kotak_gabungan_setel_teks(pg_kotak_gabungan_t *cb,
                                    const char *t);

/* Items dropdown. */
void pg_kotak_gabungan_tambah_item(pg_kotak_gabungan_t *cb,
                                     const char *t);
int pg_kotak_gabungan_jumlah_item(pg_kotak_gabungan_t *cb);
const char *pg_kotak_gabungan_item(pg_kotak_gabungan_t *cb, int idx);

/* Index item terpilih (-1 bila belum ada). */
int pg_kotak_gabungan_terpilih(pg_kotak_gabungan_t *cb);
void pg_kotak_gabungan_setel_terpilih(pg_kotak_gabungan_t *cb, int idx);

/* Buka/tutup popup manual. */
pg_bool pg_kotak_gabungan_terbuka(pg_kotak_gabungan_t *cb);
void pg_kotak_gabungan_buka(pg_kotak_gabungan_t *cb);
void pg_kotak_gabungan_tutup(pg_kotak_gabungan_t *cb);

/* Callback perubahan (pilih item ATAU edit teks). */
void pg_kotak_gabungan_saatberubah(pg_kotak_gabungan_t *cb,
                                     pg_kotak_gabungan_cb fn, void *ctx);

void pg_kotak_gabungan_setel_aktif(pg_kotak_gabungan_t *cb,
                                     pg_bool aktif);
pg_bool pg_kotak_gabungan_aktif(pg_kotak_gabungan_t *cb);

/* Setel papan klip untuk Ctrl+C/V/X. NULL = no-op. */
void pg_kotak_gabungan_setel_papan_klip(pg_kotak_gabungan_t *cb,
                                          pg_papan_klip_t *klip);

pg_widget_t *pg_kotak_gabungan_widget(pg_kotak_gabungan_t *cb);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_KOTAK_GABUNGAN_H */
