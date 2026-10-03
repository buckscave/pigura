/* ----------------------------------------------------------------------------------------------
 * pigura widget: daftar.c - list view vertikal
 * ----------------------------------------------------------------------------------------------
 * Daftar menampilkan daftar string vertikal. Satu item dapat terpilih
 * pada satu waktu. Klik kiri memilih item berdasarkan koordinat Y;
 * roda mouse menggulir konten.
 *
 * Tinggi baris diambil dari font tinggi_baris. Item yang terpotong
 * viewport tidak digambar.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/daftar.h"
#include "pigura/permukaan.h"
#include "pigura/font.h"
#include "pigura/peristiwa.h"
#include "pigura/widget.h"

#include <stdlib.h>
#include <string.h>

typedef struct pg_daftar_item {
	char *teks;
} pg_daftar_item_t;

struct pg_daftar {
	pg_widget_t      base;
	pg_daftar_item_t *item;
	int              n_item;
	int              cap_item;
	int              terpilih;
	int              gulir;
	pg_warna_t       fg;
	pg_warna_t       latar;
	pg_warna_t       sel_fg;
	pg_warna_t       sel_bg;
	pg_font_t       *font;
	pg_daftar_cb     cb;
	void            *ctx;
};

static char *pg_daftar_dup(const char *s)
{
	size_t n;
	char *out;
	if (!s) return NULL;
	n = strlen(s) + 1;
	out = (char *)malloc(n);
	if (!out) return NULL;
	memcpy(out, s, n);
	return out;
}

static pg_daftar_t *pg_daftar_dari(pg_widget_t *w)
{
	return (pg_daftar_t *)w;
}

/* Tinggi baris; aman bila font NULL. */
static int pg_daftar_row_h(pg_daftar_t *d)
{
	int h;
	if (!d->font) return 10;
	h = pg_font_tinggi_baris(d->font);
	if (h <= 0) h = pg_font_tinggi(d->font);
	if (h <= 0) h = 8;
	return h + 2;
}

/* vtable: catat item per baris. */
static void pg_daftar_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
	pg_daftar_t *d = pg_daftar_dari(w);
	int sw, sh, row_h, i;
	sw = pg_permukaan_lebar(s);
	sh = pg_permukaan_tinggi(s);
	row_h = pg_daftar_row_h(d);
	for (i = 0; i < d->n_item; i++) {
		int y = -d->gulir + i * row_h;
		if (y + row_h < 0) continue;
		if (y >= sh) break;

		/* 1. Highlight selection SEBELUM text supaya text
		 *    tampil di atas latar biru (bukan tertutup). */
		if (i == d->terpilih)
			pg_isi_permukaan_kotak(s,
				pg_buat_kotak(0, y, sw, row_h),
				d->sel_bg);

		/* 2. Text di atas highlight. Warna kontras: putih
		 *    bila terpilih (sel_fg), hitam biasa bila tidak. */
		if (d->item[i].teks && d->font)
			pg_font_gambar_teks(d->font,
				d->item[i].teks, s, 2,
				y + pg_font_baseline_tengah(d->font, row_h),
				i == d->terpilih ? d->sel_fg :
				d->fg);
	}
}

static pg_bool pg_daftar_peristiwa_v(pg_widget_t *w,
				     const pg_peristiwa_t *e)
{
	pg_daftar_t *d = pg_daftar_dari(w);
	int row_h, idx, maks_gulir;
	row_h = pg_daftar_row_h(d);
	if (e->tipe == PG_PERISTIWA_TETIK_RODA) {
		maks_gulir = d->n_item * row_h - w->kotak.h;
		if (maks_gulir < 0) maks_gulir = 0;
		d->gulir += e->roda_dy * row_h;
		if (d->gulir < 0) d->gulir = 0;
		if (d->gulir > maks_gulir) d->gulir = maks_gulir;
		pg_widget_kotor(w);
		return PG_BENAR;
	}
	if (e->tipe == PG_PERISTIWA_TETIK_TURUN &&
	    e->tetik_tombol == PG_TETIK_KIRI) {
		if (d->n_item <= 0) return PG_BENAR;
		idx = (e->tetik_pos.y + d->gulir) / row_h;
		if (idx < 0) idx = 0;
		if (idx >= d->n_item) idx = d->n_item - 1;
		if (idx != d->terpilih) {
			d->terpilih = idx;
			pg_widget_kotor(w);
		}
		if (d->cb) d->cb(d, idx, d->ctx);
		return PG_BENAR;
	}
	return PG_SALAH;
}

static void pg_daftar_hancur_v(pg_widget_t *w)
{
	pg_daftar_t *d = pg_daftar_dari(w);
	int i;
	for (i = 0; i < d->n_item; i++) {
		if (d->item[i].teks) {
			free(d->item[i].teks);
			d->item[i].teks = NULL;
		}
	}
	if (d->item) {
		free(d->item);
		d->item = NULL;
	}
	d->n_item = 0;
	d->cap_item = 0;
}

/* Deallocator struktur turunan — dipanggil oleh
 * pg_widget_hancur_penuh() setelah hancur_v. */
static void pg_daftar_bebas_v(pg_widget_t *w)
{
	free(w);
}

static const pg_widget_vtable_t pg_daftar_vtable = {
	pg_daftar_catat_v,
	pg_daftar_peristiwa_v,
	NULL,
	pg_daftar_hancur_v,
	NULL,
	NULL,
	pg_daftar_bebas_v
};

/* ---------------------------------------------------------------- API */

pg_daftar_t *pg_buat_daftar(pg_font_t *font)
{
	pg_daftar_t *d;
	d = (pg_daftar_t *)calloc(1, sizeof(*d));
	if (!d) return NULL;
	pg_widget_init(&d->base, PG_WIDGET_DAFTAR,
		       &pg_daftar_vtable);
	d->font = font;
	d->terpilih = -1;
	d->fg = PG_HITAM;
	d->latar = PG_PUTIH;
	d->sel_fg = PG_PUTIH;
	d->sel_bg = PG_BIRU;
	pg_widget_setel_latar(&d->base, d->latar);
	return d;
}

void pg_daftar_hancur(pg_daftar_t *d)
{
	if (!d) return;
	pg_widget_hancur(&d->base);
	free(d);
}

void pg_daftar_tambah(pg_daftar_t *d, const char *teks)
{
	pg_daftar_item_t *baru;
	char *str;
	if (!d) return;
	str = pg_daftar_dup(teks);
	if (teks && !str) return;
	if (d->n_item >= d->cap_item) {
		int cap_baru = d->cap_item ? d->cap_item * 2 : 8;
		baru = (pg_daftar_item_t *)realloc(d->item,
			(size_t)cap_baru * sizeof(*baru));
		if (!baru) {
			if (str) free(str);
			return;
		}
		d->item = baru;
		d->cap_item = cap_baru;
	}
	d->item[d->n_item].teks = str;
	d->n_item++;
	if (d->terpilih < 0) d->terpilih = 0;
	pg_widget_kotor(&d->base);
}

void pg_daftar_bersih(pg_daftar_t *d)
{
	int i;
	if (!d) return;
	for (i = 0; i < d->n_item; i++) {
		if (d->item[i].teks) {
			free(d->item[i].teks);
			d->item[i].teks = NULL;
		}
	}
	d->n_item = 0;
	d->terpilih = -1;
	d->gulir = 0;
	pg_widget_kotor(&d->base);
}

int pg_daftar_terpilih(pg_daftar_t *d)
{
	return d ? d->terpilih : -1;
}

void pg_daftar_setel_terpilih(pg_daftar_t *d, int idx)
{
	if (!d) return;
	if (d->n_item <= 0) return;
	if (idx < 0) idx = 0;
	if (idx >= d->n_item) idx = d->n_item - 1;
	if (idx == d->terpilih) return;
	d->terpilih = idx;
	pg_widget_kotor(&d->base);
}

void pg_daftar_saatpilih(pg_daftar_t *d, pg_daftar_cb cb, void *ctx)
{
	if (!d) return;
	d->cb = cb;
	d->ctx = ctx;
}

pg_widget_t *pg_daftar_widget(pg_daftar_t *d)
{
	return d ? &d->base : NULL;
}
