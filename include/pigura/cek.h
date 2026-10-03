/* ----------------------------------------------------------------------------------------------
 * pigura/cek.h - Widget checkbox
 * ----------------------------------------------------------------------------------------------
 * Cek adalah checkbox dengan kotak dan label teks. Klik kiri pada
 * widget (kotak atau label) akan toggle status dicek dan memicu
 * callback.
 *
 * Mode state:
 *   - biner  : dicek = BENAR/SALAH (default)
 *   - tri-state : tri = UBAH/TERPILIH/INDETERMINATE
 *
 * Layout posisi kotak relatif label dapat diatur via
 * pg_cek_setel_posisi() (KIRI/KANAN/ATAS/BAWAH).
 *
 * Visual:
 *   - kotak rounded (radius default 2), warna border = HOVER_OUTLINE
 *   - isi kotak = PANEL (idle) / HOVER_ISI (hover)
 *   - border kotak = FOKUS saat fokus keyboard
 *   - tanda X (TERPILIH) atau dash (INDETERMINATE) pakai FOKUS
 *   - disabled = render abu, tidak respon input
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_CEK_H
#define PIGURA_CEK_H

#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_cek pg_cek_t;

/* Tri-state untuk checkbox. */
typedef enum {
        PG_TRI_UBAH          = 0, /* unchecked */
        PG_TRI_TERPILIH     = 1, /* checked, tanda X */
        PG_TRI_INDETERMINATE = 2 /* dash/tanda minus */
} pg_tri_t;

/* Dipanggil saat status dicek berubah. */
typedef void (*pg_cek_cb)(pg_cek_t *c, pg_bool dicek, void *ctx);

/* Buat checkbox. label boleh NULL. */
pg_cek_t *pg_buat_cek(const char *label, pg_font_t *font);

/* Bebaskan checkbox. */
void pg_cek_hancur(pg_cek_t *c);

/* ===== State ===== */

/* Ambil status dicek (biner). */
pg_bool pg_cek_dicek(pg_cek_t *c);

/* Setel status dicek (biner, tanpa callback). */
void pg_cek_setel_dicek(pg_cek_t *c, pg_bool dicek);

/* Ambil status tri-state. */
pg_tri_t pg_cek_tri(pg_cek_t *c);

/* Setel status tri-state (tanpa callback). */
void pg_cek_setel_tri(pg_cek_t *c, pg_tri_t tri);

/* ===== Layout ===== */

/* Setel posisi kotak relatif label (default KIRI). */
void pg_cek_setel_posisi(pg_cek_t *c, pg_posisi_t pos);

/* Setel ukuran kotak (default 14 px). */
void pg_cek_setel_ukuran_kotak(pg_cek_t *c, int px);

/* Setel padding antara kotak dan label (default 4 px). */
void pg_cek_setel_padding(pg_cek_t *c, int padding);

/* ===== Warna ===== */

/* Setel warna: fg (teks), kotak_bg (isi), kotak_batas (border). */
void pg_cek_setel_warna(pg_cek_t *c, pg_warna_t fg, pg_warna_t kotak_bg,
                         pg_warna_t kotak_batas);

/* ===== Callback ===== */

/* Setel callback perubahan. */
void pg_cek_saatberubah(pg_cek_t *c, pg_cek_cb cb, void *ctx);

/* ===== Disabled ===== */

/* Setel aktif (BENAR) atau disabled (SALAH). Disabled = abu, tidak
 * respon input. */
void pg_cek_setel_aktif(pg_cek_t *c, pg_bool aktif);

/* ===== Akses widget ===== */

/* Akses widget dasar. */
pg_widget_t *pg_cek_widget(pg_cek_t *c);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_CEK_H */
