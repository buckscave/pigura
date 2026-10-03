/* ----------------------------------------------------------------------------------------------
 * pigura widget: kemajuan.c - widget progress bar
 * ----------------------------------------------------------------------------------------------
 * Kemajuan adalah progress bar horizontal read-only. Menampilkan
 * nilai dalam rentang [min, maks] sebagai bagian terisi proporsional.
 * Tidak menerima peristiwa input.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/kemajuan.h"
#include "pigura/permukaan.h"
#include "pigura/widget.h"

#include <stdlib.h>

struct pg_kemajuan {
	pg_widget_t base;
	int         min;
	int         maks;
	int         nilai;
	pg_warna_t  latar;
	pg_warna_t  fg;
	pg_warna_t  batas;
};

static pg_kemajuan_t *pg_kemajuan_dari(pg_widget_t *w)
{
	return (pg_kemajuan_t *)w;
}

/* vtable: kotak batas + isi fg proporsional. */
static void pg_kemajuan_catat_v(pg_widget_t *w,
				 pg_permukaan_t *s)
{
	pg_kemajuan_t *k = pg_kemajuan_dari(w);
	int sw, sh, rentang, isi_w;
	pg_kotak_t r;
	sw = pg_permukaan_lebar(s);
	sh = pg_permukaan_tinggi(s);
	r = pg_buat_kotak(0, 0, sw, sh);
	pg_kotak_permukaan(s, r, k->batas);
	rentang = k->maks - k->min;
	if (rentang <= 0) isi_w = 0;
	else
		isi_w = (k->nilai - k->min) * (sw - 2) / rentang;
	if (isi_w < 0) isi_w = 0;
	if (isi_w > sw - 2) isi_w = sw - 2;
	if (isi_w > 0)
		pg_isi_permukaan_kotak(s,
			pg_buat_kotak(1, 1, isi_w, sh - 2),
			k->fg);
}

/* Deallocator struktur turunan — dipanggil oleh
 * pg_widget_hancur_penuh() setelah hancur_v. */
static void pg_kemajuan_bebas_v(pg_widget_t *w)
{
	free(w);
}

static const pg_widget_vtable_t pg_kemajuan_vtable = {
	pg_kemajuan_catat_v,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	pg_kemajuan_bebas_v
};

/* ---------------------------------------------------------------- API */

pg_kemajuan_t *pg_buat_kemajuan(int min, int maks)
{
	pg_kemajuan_t *k;
	k = (pg_kemajuan_t *)calloc(1, sizeof(*k));
	if (!k) return NULL;
	pg_widget_init(&k->base, PG_WIDGET_KEMAJUAN,
		       &pg_kemajuan_vtable);
	k->min = min;
	k->maks = maks;
	if (k->maks < k->min) k->maks = k->min;
	k->nilai = k->min;
	k->latar = PG_ABU_TERANG;
	k->fg = PG_BIRU;
	k->batas = PG_ABU_GELAP;
	pg_widget_setel_latar(&k->base, k->latar);
	return k;
}

void pg_kemajuan_hancur(pg_kemajuan_t *k)
{
	if (!k) return;
	pg_widget_hancur(&k->base);
	free(k);
}

int pg_kemajuan_nilai(pg_kemajuan_t *k)
{
	return k ? k->nilai : 0;
}

void pg_kemajuan_setel_nilai(pg_kemajuan_t *k, int nilai)
{
	if (!k) return;
	if (nilai < k->min) nilai = k->min;
	if (nilai > k->maks) nilai = k->maks;
	if (nilai == k->nilai) return;
	k->nilai = nilai;
	pg_widget_kotor(&k->base);
}

pg_widget_t *pg_kemajuan_widget(pg_kemajuan_t *k)
{
	return k ? &k->base : NULL;
}
