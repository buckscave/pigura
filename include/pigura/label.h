/* ----------------------------------------------------------------------------------------------
 * pigura/label.h - Widget label teks statis
 * ----------------------------------------------------------------------------------------------
 * Label adalah widget pasif yang menampilkan teks (satu baris
 * atau multi-line bila bungkus=BENAR) dengan perataan, padding,
 * dan warna sendiri.
 *
 * Default:
 *   - Perataan KIRI (rata kiri)
 *   - Padding 4px (internal)
 *   - Tidak ada latar (transparan, pakai parent)
 *
 * Bila label punya warna latar sendiri (setel_latar), warna
 * tersebut mengisi area termasuk padding (mewarnai paddingnya).
 *
 * Margin ke parent diatur via layout manager (pg_kotak_tambah_lengkap
 * dengan parameter padding) — bukan tanggung jawab label.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_LABEL_H
#define PIGURA_LABEL_H

#include "pigura/tipe.h"
#include "pigura/permukaan.h"
#include "pigura/font.h"
#include "pigura/widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_label pg_label_t;

/* Perataan teks horizontal. */
typedef enum {
	PG_LABEL_PERATAAN_KIRI = 0,
	PG_LABEL_PERATAAN_TENGAH = 1,
	PG_LABEL_PERATAAN_KANAN = 2
} pg_label_perataan_t;

/* Buat label baru. teks boleh NULL. warna = warna teks. */
pg_label_t *pg_buat_label(const char *teks, pg_warna_t warna,
                            pg_font_t *font);

/* Bebaskan label. */
void pg_label_hancur(pg_label_t *l);

/* Ganti teks label. String disalin internal. Update min_w/min_h
 * otomatis sesuai teks + font. */
void pg_label_setel_teks(pg_label_t *l, const char *teks);

/* Setel perataan teks. Default KIRI. */
void pg_label_setel_perataan(pg_label_t *l, pg_label_perataan_t p);

/* Setel padding internal (jarak teks ke tepi widget). Default 4px. */
void pg_label_setel_padding(pg_label_t *l, int padding);

/* Setel warna teks. */
void pg_label_setel_warna(pg_label_t *l, pg_warna_t warna);

/* Setel warna latar. NULL = transparan (pakai parent).
 * Bila di-set, latar mengisi area termasuk padding. */
void pg_label_setel_latar(pg_label_t *l, pg_warna_t latar);
void pg_label_setel_latar_transparan(pg_label_t *l);

/* Setel font (ganti reference, label tidak own font). */
void pg_label_setel_font(pg_label_t *l, pg_font_t *font);

/* Setel jalur file font TTF (untuk enable setel_ukuran_teks).
 * Setelah ini, label bisa re-create font dengan ukuran berbeda. */
void pg_label_setel_jalur_font(pg_label_t *l, const char *path);

/* Setel ukuran teks. Re-create font dari path yang sama dengan
 * ukuran baru. Label harus sudah punya jalur font (via
 * setel_jalur_font). Bila font adalah bitmap font, no-op. */
void pg_label_setel_ukuran_teks(pg_label_t *l, int ukuran_px);

/* Setel wrap (bungkus teks bila tidak muat). Default SALAH. */
void pg_label_setel_bungkus(pg_label_t *l, pg_bool bungkus);

/* Ambil teks label (string internal, jangan free). */
const char *pg_label_teks(const pg_label_t *l);

/* Akses widget dasar. */
pg_widget_t *pg_label_widget(pg_label_t *l);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_LABEL_H */
