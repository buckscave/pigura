/* ----------------------------------------------------------------------------------------------
 * pigura/isian_teks.h - Widget input teks satu baris (isian teks)
 * ----------------------------------------------------------------------------------------------
 * Sebelumnya dikenal sebagai `ubahiteks`. Ditulis ulang dengan
 * dukungan:
 *   - Visual style konsisten dengan tombol (latar cerah, outline
 *     tipis, border fokus biru, sudut tumpul opsional).
 *   - Placeholder abu-abu saat kosong dan tidak fokus.
 *   - Cursor blink 500 ms (on/off) saat fokus.
 *   - Unicode UTF-8 penuh (input + cursor movement + selection).
 *   - Clipboard asli via pg_papan_klip_t (Ctrl+C / V / X / A).
 *   - Undo/redo lokal per instance (Ctrl+Z / Y / Shift+Z).
 *   - Padding konfigurabel (default 6 px).
 *   - Auto-size berbasis tinggi font dan padding.
 *   - Gulir horizontal otomatis bila teks melebihi viewport.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_ISIAN_TEKS_H
#define PIGURA_ISIAN_TEKS_H

#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"
#include "pigura/papan_klip.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_isian_teks pg_isian_teks_t;

/* Callback saat teks berubah (insert/delete/paste/cut/setel_teks). */
typedef void (*pg_isian_teks_cb)(pg_isian_teks_t *it, void *ctx);

/* Buat instance isian teks.
 *   awal        : teks awal (boleh NULL).
 *   panjang_maks: batas byte buffer (default 64 bila <= 0).
 *   font        : font dipakai (boleh NULL — render no-op). */
pg_isian_teks_t *pg_buat_isian_teks(const char *awal,
                                      int panjang_maks,
                                      pg_font_t *font);

/* Hancurkan instance + semua resource (buffer, placeholder, undo
 * stack, redo stack). */
void pg_isian_teks_hancur(pg_isian_teks_t *it);

/* ---- Teks ---- */

/* Setel seluruh teks (mengganti). Cursor ke akhir, selection clear.
 * Memicu callback saatberubah. */
void pg_isian_teks_setel_teks(pg_isian_teks_t *it, const char *teks);

/* Ambil pointer ke buffer internal (jangan free, jangan modifikasi
 * langsung). String null-terminated UTF-8. */
const char *pg_isian_teks_ambil_teks(pg_isian_teks_t *it);

/* Sisip teks di posisi kursor (replace selection bila ada). Memicu
 * callback saatberubah + push undo stack. */
void pg_isian_teks_sisip(pg_isian_teks_t *it, const char *teks);

/* ---- Placeholder ---- */

/* Setel teks placeholder. NULL = tanpa placeholder. String diduplikasi
 * internal. */
void pg_isian_teks_setel_placeholder(pg_isian_teks_t *it,
                                       const char *teks);

/* ---- Callback ---- */

/* Setel callback perubahan teks. ctx diteruskan apa adanya. */
void pg_isian_teks_saatberubah(pg_isian_teks_t *it,
                                 pg_isian_teks_cb cb, void *ctx);

/* ---- Selection ---- */

/* Ambil teks terpilih (malloc'd, caller free). NULL bila tidak ada
 * selection. */
char *pg_isian_teks_ambil_pilihan(pg_isian_teks_t *it);

/* Setel selection range byte-offset [mulai, akhir). Bila mulai atau
 * akhir negatif, atau mulai==akhir, selection dibersihkan. */
void pg_isian_teks_setel_pilihan(pg_isian_teks_t *it,
                                   int mulai, int akhir);

/* Select all text. */
void pg_isian_teks_pilih_semua(pg_isian_teks_t *it);

/* Clear selection. */
void pg_isian_teks_bersih_pilihan(pg_isian_teks_t *it);

/* ---- Clipboard ---- */

/* Setel handle papan klip. NULL = Ctrl+C/V/X no-op (tapi event tetap
 * di-consume sehingga tidak bocor ke widget lain). */
void pg_isian_teks_setel_papan_klip(pg_isian_teks_t *it,
                                     pg_papan_klip_t *klip);

/* ---- Undo/redo ---- */

/* Setel batas maksimum entry undo stack (default 50). Setelan baru
 * berlaku segera: bila lebih kecil dari ukuran saat ini, entry paling
 * lama di-drop. */
void pg_isian_teks_setel_batas_undo(pg_isian_teks_t *it, int maks);

/* Undo satu langkah (Ctrl+Z). Bila stack kosong, no-op. */
void pg_isian_teks_urungkan(pg_isian_teks_t *it);

/* Redo satu langkah (Ctrl+Y atau Ctrl+Shift+Z). Bila stack kosong,
 * no-op. */
void pg_isian_teks_ulangi(pg_isian_teks_t *it);

/* ---- Padding ---- */

/* Setel padding internal (px). Default 6. */
void pg_isian_teks_setel_padding(pg_isian_teks_t *it, int padding);

/* ---- Warna custom ---- */

/* Setel warna eksplisit. Pass PG_TRANSPARAN (alpha=0) pada salah
 * satu channel untuk kembali ke tema default channel itu.
 *   fg   : warna teks. PG_TRANSPARAN = tema (PG_WARNA_TEKS_TOMBOL).
 *   latar: warna isi.  PG_TRANSPARAN = tema (PG_WARNA_PANEL).
 *   batas: warna border. PG_TRANSPARAN = tema (PG_WARNA_HOVER_OUTLINE). */
void pg_isian_teks_setel_warna(pg_isian_teks_t *it,
                                pg_warna_t fg,
                                pg_warna_t latar,
                                pg_warna_t batas);

/* ---- Akses widget dasar ---- */

pg_widget_t *pg_isian_teks_widget(pg_isian_teks_t *it);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_ISIAN_TEKS_H */
