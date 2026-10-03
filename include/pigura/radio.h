/* ----------------------------------------------------------------------------------------------
 * pigura/radio.h - Widget radio button bergrup
 * ----------------------------------------------------------------------------------------------
 * Radio button selalu bagian dari sebuah grup. Dalam grup, tepat
 * satu radio terpilih pada satu waktu. Klik radio menjadikannya
 * terpilih dan otomatis mencabut pilihan radio lain. Grup menyimpan
 * array pointer radio; field terpilih menunjuk ke radio aktif (NULL
 * bila grup kosong).
 *
 * Layout posisi lingkaran relatif label dapat diatur via
 * pg_radio_setel_posisi() (KIRI/KANAN/ATAS/BAWAH).
 *
 * Visual:
 *   - lingkaran AA, warna border = HOVER_OUTLINE (idle)
 *   - border lingkaran = FOKUS saat hover atau fokus keyboard
 *   - dot dalam = FOKUS saat terpilih
 *   - disabled = render abu, tidak respon input
 *
 * Membebaskan grup TIDAK membebaskan radio anggota; pemilik bertanggung
 * jawab membebaskan masing-masing radio.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_RADIO_H
#define PIGURA_RADIO_H

#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_radio_grup pg_radio_grup_t;
typedef struct pg_radio pg_radio_t;

/* Dipanggil saat radio terpilih. */
typedef void (*pg_radio_cb)(pg_radio_t *r, void *ctx);

/* Buat grup radio kosong. */
pg_radio_grup_t *pg_buat_radio_grup(void);

/* Bebaskan grup (TIDAK membebaskan radio anggota). */
void pg_radio_grup_hancur(pg_radio_grup_t *g);

/* Buat radio dalam grup. label boleh NULL. */
pg_radio_t *pg_buat_radio(pg_radio_grup_t *g, const char *label,
                           pg_font_t *font);

/* Bebaskan radio (dan keluarkan dari grup). */
void pg_radio_hancur(pg_radio_t *r);

/* ===== State ===== */

/* BENAR jika radio ini terpilih di grupnya. */
pg_bool pg_radio_terpilih(pg_radio_t *r);

/* Pilih radio ini (cabut pilihan radio lain). */
void pg_radio_pilih(pg_radio_t *r);

/* ===== Layout ===== */

/* Setel posisi lingkaran relatif label (default KIRI). */
void pg_radio_setel_posisi(pg_radio_t *r, pg_posisi_t pos);

/* Setel ukuran (diameter) lingkaran (default 14 px). */
void pg_radio_setel_ukuran(pg_radio_t *r, int px);

/* Setel padding antara lingkaran dan label (default 4 px). */
void pg_radio_setel_padding(pg_radio_t *r, int padding);

/* ===== Warna ===== */

/* Setel warna: fg (teks), lingk_batas (border lingkaran). */
void pg_radio_setel_warna(pg_radio_t *r, pg_warna_t fg,
                           pg_warna_t lingk_batas);

/* ===== Callback ===== */

/* Setel callback perubahan. */
void pg_radio_saatberubah(pg_radio_t *r, pg_radio_cb cb, void *ctx);

/* ===== Disabled ===== */

/* Setel aktif (BENAR) atau disabled (SALAH). Disabled = abu, tidak
 * respon input. */
void pg_radio_setel_aktif(pg_radio_t *r, pg_bool aktif);

/* ===== Akses widget ===== */

/* Akses widget dasar. */
pg_widget_t *pg_radio_widget(pg_radio_t *r);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_RADIO_H */
