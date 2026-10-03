/* ----------------------------------------------------------------------------------------------
 * pigura/status_bar.h - Widget status bar (baris bawah multi-section)
 * ----------------------------------------------------------------------------------------------
 * Status bar adalah widget horizontal di bawah layar yang menampilkan
 * beberapa seksi teks. Tiap seksi punya lebar (piksel) atau proporsi
 * (factor resize). Seksi dengan proporsi > 0 akan mengisi ruang
 * tersisa proporsional terhadap proporsi tersebut.
 *
 * Contoh: 3 seksi [status utama | indikator mode | waktu].
 *   sb = pg_buat_status_bar(font);
 *   pg_sb_tambah_seksi(sb, 0, 1);    status utama, expand
 *   pg_sb_tambah_seksi(sb, 80, 0);   mode, fixed
 *   pg_sb_tambah_seksi(sb, 80, 0);   waktu, fixed
 *   pg_sb_setel_teks(sb, 0, "Ready");
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_STATUS_BAR_H
#define PIGURA_STATUS_BAR_H

#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_status_bar pg_status_bar_t;

/* Buat status bar. */
pg_status_bar_t *pg_buat_status_bar(pg_font_t *font);

/* Bebaskan status bar. */
void pg_status_bar_hancur(pg_status_bar_t *sb);

/* Tambah section. lebar > 0 = lebar fixed (piksel); lebar = 0 +
 * proporsi > 0 = seksi proporsional. Mengembalikan id seksi. */
int pg_sb_tambah_seksi(pg_status_bar_t *sb, int lebar, int proporsi);

/* Setel teks seksi. String disalin internal. */
void pg_sb_setel_teks(pg_status_bar_t *sb, int seksi,
    const char *teks);

/* Ambil teks seksi. */
const char *pg_sb_ambil_teks(const pg_status_bar_t *sb, int seksi);

/* Jumlah seksi. */
int pg_sb_jumlah_seksi(const pg_status_bar_t *sb);

/* Akses widget dasar. */
pg_widget_t *pg_status_bar_widget(pg_status_bar_t *sb);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_STATUS_BAR_H */
