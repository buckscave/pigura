/* ----------------------------------------------------------------------------------------------
 * pigura/tombol.h - Widget tombol tekan
 * ----------------------------------------------------------------------------------------------
 * Tombol adalah widget interaktif dengan 4 state visual:
 *   - idle   : latar panel, no outline
 *   - hover  : latar lebih gelap, outline tipis
 *   - tekan  : latar lebih gelap lagi, outline 3D (atas+kiri terang,
 *              kanan+bawah gelap) — efek tombol "turun"
 *   - fokus  : outline biru (#50B9FF) di semua state bila punya fokus
 *
 * Mode konten:
 *   - label saja           : teks centered
 *   - icon saja            : icon centered, tombol square (w == h)
 *   - icon + label         : icon di posisi tertentu (default kiri)
 *
 * Radius kotak diatur via pg_widget_setel_radius() (default 0 = tajam).
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_TOMBOL_H
#define PIGURA_TOMBOL_H

#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"
#include "pigura/permukaan.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_tombol_impl pg_tombol_t;

/* Color key untuk transparent pada icon (sementara sampai RGBA).
 * Pixel dengan warna ini akan di-skip saat render icon tombol. */
#define PG_TOMBOL_TRANSPARENT_KEY PG_MAGENTA

/* Posisi icon relatif label (untuk mode hibrid). */
typedef enum {
        PG_TOMBOL_ICON_KIRI = 0,
        PG_TOMBOL_ICON_KANAN = 1,
        PG_TOMBOL_ICON_ATAS = 2,
        PG_TOMBOL_ICON_BAWAH = 3
} pg_tombol_posisi_icon_t;

/* Dipanggil saat tombol dilepas setelah ditekan di dalamnya. */
typedef void (*pg_tombol_cb)(pg_tombol_t *t, void *ctx);

/* Buat tombol baru. label boleh NULL (icon-only bila icon di-set). */
pg_tombol_t *pg_buat_tombol(const char *label, pg_font_t *font);

/* Bebaskan tombol. */
void pg_tombol_hancur(pg_tombol_t *t);

/* Setel callback klik + konteks pengguna. */
void pg_tombol_saatklik(pg_tombol_t *t, pg_tombol_cb cb, void *ctx);

/* Setel apakah tombol menggambar border (outline). Default BENAR. */
void pg_tombol_setel_batas(pg_tombol_t *t, pg_bool gambar);

/* Setel icon permukaan untuk tombol. Bila label NULL, icon-only mode.
 * Bila label tidak NULL, icon + label sesuai posisi_icon. */
void pg_tombol_setel_icon(pg_tombol_t *t, pg_permukaan_t *icon);

/* Setel posisi icon relatif label (default KIRI). */
void pg_tombol_setel_posisi_icon(pg_tombol_t *t,
                                   pg_tombol_posisi_icon_t pos);

/* Setel label (ganti teks). NULL = icon-only. */
void pg_tombol_setel_label(pg_tombol_t *t, const char *label);

/* Setel warna custom (NULL = pakai default tema). */
void pg_tombol_setel_warna(pg_tombol_t *t,
                             pg_warna_t fg,
                             pg_warna_t isi_idle,
                             pg_warna_t isi_hover,
                             pg_warna_t isi_tekan);

/* Setel state "terpilih" (untuk toolbar toggle button). Saat terpilih,
 * tombol tampil seperti ditekan (3D inset). */
void pg_tombol_setel_terpilih(pg_tombol_t *t, pg_bool terpilih);
pg_bool pg_tombol_terpilih(const pg_tombol_t *t);

/* Setel padding internal (jarak konten ke tepi tombol). Default 6px. */
void pg_tombol_setel_padding(pg_tombol_t *t, int padding);

/* Akses widget dasar. */
pg_widget_t *pg_tombol_widget(pg_tombol_t *t);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_TOMBOL_H */
