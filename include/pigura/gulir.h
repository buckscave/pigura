/* ----------------------------------------------------------------------------------------------
 * pigura/gulir.h - Widget scroll view dengan scrollbar
 * ----------------------------------------------------------------------------------------------
 * Gulir adalah kontainer yang menampung satu anak berukuran lebih
 * besar dari viewport. Anak di-blit ke permukaan gulir dengan offset
 * (-gulir_x, -gulir_y); viewport otomatis diklem ke (0, 0, w-bar,
 * h-bar) di mana bar = 12 piksel bila scrollbar vertikal/horizontal
 * dibutuhkan (konten > viewport).
 *
 * Scrollbar:
 *   - Vertikal: lebar 12 piksel di sisi kanan, tampak bila
 *     konten_h > viewport_h. Thumb tinggi proporsional ke
 *     viewport_h / konten_h; thumb_y proporsional ke gulir_y.
 *   - Horizontal: tinggi 12 piksel di sisi bawah, tampak bila
 *     konten_w > viewport_w. Rumus sama, sumbu di-swap.
 *   - Pojok kanan-bawah (12x12) diisi abu gelap bila keduanya
 *     tampak.
 *
 * Interaksi:
 *   - Roda mouse: gulir_y +/- 24 piksel, diklem ke [0, maks].
 *   - Klik kiri pada thumb: mulai seret. Gerak mouse ubah
 *     gulir_y/x proporsional ke thumb_y/x baru.
 *   - Klik kiri pada track (di luar thumb): lompat satu halaman
 *     ke arah klik.
 *   - Klik di viewport: teruskan ke anak dengan offset digulir.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_GULIR_H
#define PIGURA_GULIR_H

#include "pigura/tipe.h"
#include "pigura/widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_gulir pg_gulir_t;

/* Buat scroll view kosong. */
pg_gulir_t *pg_buat_gulir(void);

/* Bebaskan scroll view (tidak menghancurkan anak). */
void pg_gulir_hancur(pg_gulir_t *g);

/* Setel anak + ukuran konten logis. Anak TIDAK di-resize; ukuran
 * permukaan anak harus >= (konten_w, konten_h) bila ingin seluruh
 * konten terender. Bila permukaan anak lebih kecil, area yang
 * melampaui tetap berlatar widget. */
void pg_gulir_setel_anak(pg_gulir_t *g, pg_widget_t *anak,
                          int konten_w, int konten_h);

/* Gulir ke posisi absolut (diklem ke rentang konten). */
void pg_gulir_gulir_ke(pg_gulir_t *g, int x, int y);

/* Akses widget dasar. */
pg_widget_t *pg_gulir_widget(pg_gulir_t *g);

/* Update ukuran konten tanpa set anak ulang. */
void pg_gulir_setel_ukuran_konten(pg_gulir_t *g, int w, int h);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_GULIR_H */
