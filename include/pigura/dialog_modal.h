/* ----------------------------------------------------------------------------------------------
 * pigura/dialog_modal.h - Modal dialog (block input)
 * ----------------------------------------------------------------------------------------------
 * Modal dialog adalah overlay yang memblokir input ke widget lain.
 * Saat aktif (pg_dialog_modal_tampil), overlay gelap di atas semua
 * widget biasa; dialog box di tengah layar menampung child + tombol.
 *
 * Hanya event yang mengenai dialog yang diteruskan; semua event mouse
 * di luar dialog diabaikan (block input).
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_DIALOG_MODAL_H
#define PIGURA_DIALOG_MODAL_H

#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"
#include "pigura/permukaan.h"
#include "pigura/peristiwa.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_dialog_modal pg_dialog_modal_t;

/* Callback tombol. Dipanggil saat user klik tombol. */
typedef void (*pg_dialog_modal_cb)(pg_dialog_modal_t *dm,
                                    void *ctx);

/* Buat modal dialog. */
pg_dialog_modal_t *pg_buat_dialog_modal(const char *judul,
    int w, int h, pg_font_t *font);

/* Bebaskan modal dialog (TIDAK menghancurkan child). */
void pg_dialog_modal_hancur(pg_dialog_modal_t *dm);

/* Setel child widget. */
void pg_dialog_modal_setel_anak(pg_dialog_modal_t *dm,
    pg_widget_t *child);

/* Tambah tombol (OK, Cancel, dll). Mengembalikan id tombol. */
int pg_dialog_modal_tambah_tombol(pg_dialog_modal_t *dm,
    const char *label,
    void (*cb)(pg_dialog_modal_t*, void*), void *ctx);

/* Tampilkan modal (block input ke widget lain). */
void pg_dialog_modal_tampil(pg_dialog_modal_t *dm);

/* Tutup modal. */
void pg_dialog_modal_tutup(pg_dialog_modal_t *dm);

/* Apakah modal aktif? */
pg_bool pg_dialog_modal_aktif(const pg_dialog_modal_t *dm);

/* Catat modal ke dest (overlay gelap + dialog box). */
void pg_dialog_modal_catat(pg_dialog_modal_t *dm,
    pg_permukaan_t *dest, int screen_w, int screen_h);

/* Tangani peristiwa modal. Bila modal tidak aktif, return SALAH. */
pg_bool pg_dialog_modal_tangani(pg_dialog_modal_t *dm,
    const pg_peristiwa_t *e);

/* Posisi dialog: setter + getter. */
void pg_dialog_modal_pindah(pg_dialog_modal_t *dm, int x, int y);
void pg_dialog_modal_posisi(const pg_dialog_modal_t *dm,
    int *x, int *y);

/* Setel ukuran overlay (lebar/tinggi layar). */
void pg_dialog_modal_setel_layar(pg_dialog_modal_t *dm,
    int w, int h);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_DIALOG_MODAL_H */
