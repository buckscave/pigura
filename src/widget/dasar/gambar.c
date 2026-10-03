/* ----------------------------------------------------------------------------------------------
 * pigura widget: gambar.c - widget tampilan piksel RGB
 * ----------------------------------------------------------------------------------------------
 * Gambar-widget menampilkan buffer piksel RGB (array pg_warna_t)
 * berukuran w x h. Buffer disalin ke permukaan widget saat catat.
 * Cocok untuk menampilkan gambar prosedural atau di-decode dari
 * berkas.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/gambar_widget.h"
#include "pigura/permukaan.h"
#include "pigura/widget.h"

#include <stdlib.h>
#include <string.h>

struct pg_gambar_widget {
	pg_widget_t  base;
	pg_warna_t  *piksel;
	int          w;
	int          h;
};

static pg_gambar_widget_t *pg_gambar_widget_dari(pg_widget_t *w)
{
	return (pg_gambar_widget_t *)w;
}

/* vtable: isi hitam + blit piksel. */
static void pg_gambar_widget_catat_v(pg_widget_t *w,
				     pg_permukaan_t *s)
{
	pg_gambar_widget_t *g = pg_gambar_widget_dari(w);
	int sw, sh, y, cw, ch;
	int langkah;
	pg_warna_t *pik;
	pg_isi_permukaan(s, PG_HITAM);
	if (!g->piksel || g->w <= 0 || g->h <= 0) return;
	sw = pg_permukaan_lebar(s);
	sh = pg_permukaan_tinggi(s);
	cw = (g->w < sw) ? g->w : sw;
	ch = (g->h < sh) ? g->h : sh;
	langkah = pg_permukaan_langkah(s);
	pik = (pg_warna_t *)pg_permukaan_piksel_mut(s);
	for (y = 0; y < ch; y++) {
		pg_warna_t *drow = (pg_warna_t *)
			((char *)pik + (size_t)y * langkah);
		memcpy(drow, &g->piksel[(size_t)y * g->w],
		       (size_t)cw * sizeof(pg_warna_t));
	}
}

static void pg_gambar_widget_hancur_v(pg_widget_t *w)
{
	pg_gambar_widget_t *g = pg_gambar_widget_dari(w);
	if (g->piksel) {
		free(g->piksel);
		g->piksel = NULL;
	}
}

/* Deallocator struktur turunan — dipanggil oleh
 * pg_widget_hancur_penuh() setelah hancur_v. */
static void pg_gambar_widget_bebas_v(pg_widget_t *w)
{
	free(w);
}

static const pg_widget_vtable_t pg_gambar_widget_vtable = {
	pg_gambar_widget_catat_v,
	NULL,
	NULL,
	pg_gambar_widget_hancur_v,
	NULL,
	NULL,
	pg_gambar_widget_bebas_v
};

/* ---------------------------------------------------------------- API */

pg_gambar_widget_t *pg_buat_gambar_widget(int w, int h,
					   const pg_warna_t *piksel)
{
	pg_gambar_widget_t *g;
	if (w <= 0 || h <= 0) return NULL;
	g = (pg_gambar_widget_t *)calloc(1, sizeof(*g));
	if (!g) return NULL;
	pg_widget_init(&g->base, PG_WIDGET_GAMBAR,
		       &pg_gambar_widget_vtable);
	g->w = w;
	g->h = h;
	g->piksel = (pg_warna_t *)malloc((size_t)w * h *
					  sizeof(pg_warna_t));
	if (!g->piksel) {
		free(g);
		return NULL;
	}
	if (piksel)
		memcpy(g->piksel, piksel,
		       (size_t)w * h * sizeof(pg_warna_t));
	else
		memset(g->piksel, 0,
		       (size_t)w * h * sizeof(pg_warna_t));
	pg_widget_setel_kotak(&g->base,
			       pg_buat_kotak(0, 0, w, h));
	return g;
}

void pg_gambar_widget_hancur(pg_gambar_widget_t *g)
{
	if (!g) return;
	pg_widget_hancur(&g->base);
	free(g);
}

void pg_gambar_widget_setel_piksel(pg_gambar_widget_t *g, int w,
				    int h, const pg_warna_t *piksel)
{
	pg_warna_t *baru;
	if (!g || w <= 0 || h <= 0) return;
	baru = (pg_warna_t *)malloc((size_t)w * h *
				     sizeof(pg_warna_t));
	if (!baru) return;
	if (piksel)
		memcpy(baru, piksel,
		       (size_t)w * h * sizeof(pg_warna_t));
	else
		memset(baru, 0,
		       (size_t)w * h * sizeof(pg_warna_t));
	if (g->piksel) free(g->piksel);
	g->piksel = baru;
	g->w = w;
	g->h = h;
	if (g->base.kotak.w != w || g->base.kotak.h != h)
		pg_widget_setel_kotak(&g->base,
			pg_buat_kotak(g->base.kotak.x,
				       g->base.kotak.y, w, h));
	else
		pg_widget_kotor(&g->base);
}

pg_widget_t *pg_gambar_widget_widget(pg_gambar_widget_t *g)
{
	return g ? &g->base : NULL;
}
