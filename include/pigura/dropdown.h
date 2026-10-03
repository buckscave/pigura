/* ----------------------------------------------------------------------------------------------
 * pigura/dropdown.h - Widget dropdown (combobox)
 * ----------------------------------------------------------------------------------------------
 * Dropdown adalah combobox satu baris: klik membuka popup vertikal
 * berisi item; klik item menutup popup dan mengubah terpilih.
 * Berbeda dari menubar, dropdown hanya punya satu kolom item dan
 * menampilkan item terpilih di baris atas.
 *
 * Indeks terpilih -1 berarti belum ada item terpilih.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_DROPDOWN_H
#define PIGURA_DROPDOWN_H

#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_dropdown pg_dropdown_t;

/* Dipanggil saat item terpilih berubah. */
typedef void (*pg_dropdown_cb)(pg_dropdown_t *d, int idx,
				void *ctx);

/* Buat dropdown. label boleh NULL (tanpa caption). */
pg_dropdown_t *pg_buat_dropdown(const char *label,
				  pg_font_t *font);

/* Bebaskan dropdown. */
void pg_dropdown_hancur(pg_dropdown_t *d);

/* Tambah item. String disalin internal. */
void pg_dropdown_tambah_item(pg_dropdown_t *d, const char *teks);

/* Ambil indeks terpilih (-1 jika belum). */
int pg_dropdown_terpilih(const pg_dropdown_t *d);

/* Setel terpilih (diklem ke rentang). */
void pg_dropdown_setel_terpilih(pg_dropdown_t *d, int idx);

/* Setel callback perubahan. */
void pg_dropdown_saatberubah(pg_dropdown_t *d,
			      pg_dropdown_cb cb, void *ctx);

/* Cek apakah popup terbuka. */
pg_bool pg_dropdown_terbuka(pg_dropdown_t *d);

/* Tutup popup (bila terbuka). */
void pg_dropdown_tutup(pg_dropdown_t *d);

/* Akses widget dasar. */
pg_widget_t *pg_dropdown_widget(pg_dropdown_t *d);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_DROPDOWN_H */
