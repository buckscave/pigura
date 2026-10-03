/* ----------------------------------------------------------------------------------------------
 * pigura/tooltip.h - Sistem tooltip global
 * ----------------------------------------------------------------------------------------------
 * Tooltip adalah label melayang yang muncul dekat kursor setelah
 * mouse hover di atas widget selama 500 ms (default). Tooltip
 * ter-hide otomatis saat mouse leave widget.
 *
 * Pakai: panggil pg_widget_setel_tooltip(w, "teks"). Tooltip
 * otomatis muncul saat hover. Untuk render, panggil
 * pg_tooltip_catat(ctx, dest, mouse_x, mouse_y) di akhir frame
 * (setelah semua widget lain). ctx didapat dari pg_tooltip_ctx().
 *
 * Implementasi: timer 500 ms dimulai saat mouse enter widget.
 * Field tooltip_teks di pg_widget_t (ditambahkan via widget.h).
 * Saat timer fire, tooltip_visible=BENAR; render di mouse_x/y + ofs.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_TOOLTIP_H
#define PIGURA_TOOLTIP_H

#include "pigura/tipe.h"
#include "pigura/widget.h"
#include "pigura/permukaan.h"
#include "pigura/peristiwa.h"
#include "pigura/font.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Set tooltip untuk widget. teks akan muncul setelah hover 500ms.
 * String disalin internal; NULL menghapus tooltip. */
void pg_widget_setel_tooltip(pg_widget_t *w, const char *teks);

/* Ambil teks tooltip widget (atau NULL bila tidak ada). */
const char *pg_widget_ambil_tooltip(const pg_widget_t *w);

/* Inisialisasi sistem tooltip global (dipanggil sekali). */
void pg_tooltip_init(pg_font_t *font);

/* Setel font untuk render tooltip. */
void pg_tooltip_setel_font(pg_font_t *font);

/* Setel delay (ms) sebelum tooltip muncul. Default 500. */
void pg_tooltip_setel_delay(unsigned ms);

/* Pompa event ke sistem tooltip. Dipanggil oleh app untuk setiap
 * event mouse. Akan tracking widget yang sedang di-hover dan
 * memulai/hentikan timer. */
void pg_tooltip_tangani(const pg_peristiwa_t *e);

/* Render tooltip ke dest bila visible. Dipanggil paling akhir. */
void pg_tooltip_catat(pg_permukaan_t *dest, int mouse_x,
                       int mouse_y);

/* Setel visible=BENAR paksa (untuk test). */
void pg_tooltip_setel_visible(pg_bool visible);

/* Apakah tooltip saat ini visible? */
pg_bool pg_tooltip_visible(void);

/* Reset state (timer + visible). */
void pg_tooltip_reset(void);

/* Hancurkan state tooltip global. */
void pg_tooltip_selesai(void);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_TOOLTIP_H */
