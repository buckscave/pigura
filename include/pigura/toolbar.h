/* ----------------------------------------------------------------------------------------------
 * pigura/toolbar.h - Widget toolbar (baris tombol)
 * ----------------------------------------------------------------------------------------------
 * Toolbar adalah kontainer horizontal (atau vertikal) berisi tombol,
 * pemisah, dan spasi. Tombol punya label + callback saat diklik.
 *
 * Item types:
 *   PG_TB_TOMBOL  - button biasa dengan label + callback.
 *   PG_TB_PEMISAH - garis vertikal pemisah antar grup tombol.
 *   PG_TB_SPASI   - spasi kosong yang mengisi ruang tersisa
 *                   (expand). Berguna untuk mendorong tombol
 *                   berikutnya ke kanan toolbar.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_TOOLBAR_H
#define PIGURA_TOOLBAR_H

#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"
#include "pigura/tombol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_toolbar pg_toolbar_t;

typedef enum {
        PG_TB_TOMBOL  = 0, /* button biasa */
        PG_TB_PEMISAH = 1, /* garis pemisah */
        PG_TB_SPASI   = 2 /* spasi kosong (expand) */
} pg_tb_tipe_t;

/* Callback saat tombol toolbar diklik. */
typedef void (*pg_tb_cb)(pg_toolbar_t *tb, int id, void *ctx);

/* Buat toolbar. vertikal=BENAR untuk toolbar vertikal. */
pg_toolbar_t *pg_buat_toolbar(pg_bool vertikal);

/* Bebaskan toolbar (menghancurkan tombol internal). */
void pg_toolbar_hancur(pg_toolbar_t *tb);

/* Tambah tombol dengan label/icon. Mengembalikan id tombol. */
int pg_tb_tambah_tombol(pg_toolbar_t *tb, const char *label,
    void (*cb)(void*), void *ctx);

/* Tambah tombol dengan callback gaya pg_tb_cb (terima id + tb). */
int pg_tb_tambah_tombol_v(pg_toolbar_t *tb, const char *label,
    pg_tb_cb cb, void *ctx);

/* Tambah pemisah. */
void pg_tb_tambah_pemisah(pg_toolbar_t *tb);

/* Tambah spasi (expand). */
void pg_tb_tambah_spasi(pg_toolbar_t *tb);

/* Setel font untuk label tombol. */
void pg_tb_setel_font(pg_toolbar_t *tb, pg_font_t *font);

/* Akses widget dasar. */
pg_widget_t *pg_toolbar_widget(pg_toolbar_t *tb);

/* Ambil pointer tombol berdasarkan id. */
pg_tombol_t *pg_tb_ambil_tombol(pg_toolbar_t *tb, int id);

/* Setel icon permukaan untuk tombol berdasarkan id. */
void pg_tb_setel_icon(pg_toolbar_t *tb, int id, pg_permukaan_t *icon);

/* Setel orientasi toolbar. vertikal=BENAR untuk orientasi vertikal
 * (item tersusun atas-bawah), SALAH untuk horizontal (kiri-kanan).
 * Berguna saat toolbar dipindah dari dock atas (horizontal) ke
 * dock kiri/kanan (vertikal) — orientasi disesuaikan otomatis. */
void pg_tb_setel_vertikal(pg_toolbar_t *tb, pg_bool vertikal);

/* Ambil orientasi toolbar saat ini. */
pg_bool pg_tb_vertikal(const pg_toolbar_t *tb);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_TOOLBAR_H */
