/* ----------------------------------------------------------------------------------------------
 * pigura widget: splitter.c - resize antar panel
 * ----------------------------------------------------------------------------------------------
 * Splitter adalah widget 4 piksel yang dipakai untuk resize dua
 * panel berdekatan. Memakai API high-level pg_widget_saat_seret
 * (sudah ada di widget.c) untuk mendeteksi drag. Saat drag, panel1
 * dan panel2 di-resize agar pas dengan posisi splitter baru.
 *
 * Algoritma:
 *   HORI: span = p1.w + p2.w + sp.w  (tidak berubah saat drag)
 *         new_x = clamp(mouse_x, p1.x+min1, end-min2-sp.w)
 *         p1.w = new_x - p1.x
 *         p2.x = new_x + sp.w ; p2.w = end - p2.x
 *   VERT: sama, ganti x/y, w/h.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/splitter.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/aksi.h"
#include "pigura/widget.h"

#include <stdlib.h>

/* Lebar/tinggi splitter default. */
#define PG_SPLITTER_TEBAL 4

struct pg_splitter {
	pg_widget_t               base;
	pg_splitter_orientasi_t   orientasi;
	pg_widget_t              *panel1;
	pg_widget_t              *panel2;
	int                       min1;
	int                       min2;
	pg_bool                   hover;
	pg_bool                   ditarik;
	void                    (*cb_ubah)(int posisi, void *ctx);
	void                     *ctx;
};

static pg_splitter_t *pg_splitter_dari(pg_widget_t *w)
{
	return (pg_splitter_t *)w;
}

/* vtable: catat kotak abu + grip titik-titik di tengah. */
static void pg_splitter_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
	pg_splitter_t *sp = pg_splitter_dari(w);
	pg_warna_t    c;
	int           sw, sh, i;
	sw = pg_permukaan_lebar(s);
	sh = pg_permukaan_tinggi(s);
	if (sp->ditarik)        c = PG_RGB(0x60, 0x90, 0xc0);
	else if (sp->hover)     c = PG_RGB(0xa0, 0xa0, 0xa0);
	else                    c = PG_ABU;
	pg_isi_permukaan(s, c);
	/* Grip: titik-titik di tengah sepanjang sumbu panjang. */
	if (sp->orientasi == PG_SPLITTER_HORI) {
		for (i = 2; i < sh - 2; i += 4)
			pg_setel_piksel_permukaan(s, sw / 2, i,
						   PG_ABU_GELAP);
	} else {
		for (i = 2; i < sw - 2; i += 4)
			pg_setel_piksel_permukaan(s, i, sh / 2,
						   PG_ABU_GELAP);
	}
}

/* High-level drag: mulai. */
static void pg_splitter_seret_mulai(pg_widget_t *w, int x, int y,
                                     void *ctx)
{
	pg_splitter_t *sp = (pg_splitter_t *)w;
	(void)x; (void)y; (void)ctx;
	sp->ditarik = PG_BENAR;
	pg_widget_kotor(w);
}

/* High-level drag: gerak. Hitung posisi baru, resize panel. */
static void pg_splitter_seret_gerak(pg_widget_t *w, int x, int y,
                                     void *ctx)
{
	pg_splitter_t *sp = (pg_splitter_t *)w;
	pg_widget_t   *p1, *p2;
	int            new_pos, span, end, p1_new, p2_new;
	(void)ctx;
	p1 = sp->panel1;
	p2 = sp->panel2;
	if (!p1 || !p2) return;

	if (sp->orientasi == PG_SPLITTER_HORI) {
		span = p1->kotak.w + p2->kotak.w + w->kotak.w;
		end  = p1->kotak.x + span;
		new_pos = x;
		if (new_pos < p1->kotak.x + sp->min1)
			new_pos = p1->kotak.x + sp->min1;
		if (new_pos > end - sp->min2 - w->kotak.w)
			new_pos = end - sp->min2 - w->kotak.w;
		p1_new = new_pos - p1->kotak.x;
		p2_new = end - (new_pos + w->kotak.w);
		if (p1_new < sp->min1) p1_new = sp->min1;
		if (p2_new < sp->min2) p2_new = sp->min2;
		pg_widget_pindah(w, new_pos, w->kotak.y);
		pg_widget_ubah_ukuran(p1, p1_new, p1->kotak.h);
		pg_widget_pindah(p2, new_pos + w->kotak.w,
				 p2->kotak.y);
		pg_widget_ubah_ukuran(p2, p2_new, p2->kotak.h);
		if (sp->cb_ubah) sp->cb_ubah(new_pos, sp->ctx);
	} else {
		span = p1->kotak.h + p2->kotak.h + w->kotak.h;
		end  = p1->kotak.y + span;
		new_pos = y;
		if (new_pos < p1->kotak.y + sp->min1)
			new_pos = p1->kotak.y + sp->min1;
		if (new_pos > end - sp->min2 - w->kotak.h)
			new_pos = end - sp->min2 - w->kotak.h;
		p1_new = new_pos - p1->kotak.y;
		p2_new = end - (new_pos + w->kotak.h);
		if (p1_new < sp->min1) p1_new = sp->min1;
		if (p2_new < sp->min2) p2_new = sp->min2;
		pg_widget_pindah(w, w->kotak.x, new_pos);
		pg_widget_ubah_ukuran(p1, p1->kotak.w, p1_new);
		pg_widget_pindah(p2, p2->kotak.x,
				 new_pos + w->kotak.h);
		pg_widget_ubah_ukuran(p2, p2->kotak.w, p2_new);
		if (sp->cb_ubah) sp->cb_ubah(new_pos, sp->ctx);
	}
}

/* High-level drag: selesai. */
static void pg_splitter_seret_selesai(pg_widget_t *w, int x, int y,
                                       void *ctx)
{
	pg_splitter_t *sp = (pg_splitter_t *)w;
	(void)x; (void)y; (void)ctx;
	sp->ditarik = PG_SALAH;
	pg_widget_kotor(w);
}

/* vtable peristiwa: hover tracking saja. Drag sudah ditangani
 * otomatis oleh pg_widget_tangani_aksi via saat_seret_*. */
static pg_bool pg_splitter_peristiwa_v(pg_widget_t *w,
                                        const pg_aksi_t *e)
{
	pg_splitter_t *sp = pg_splitter_dari(w);
	if (e->tipe == PG_AKSI_TETIKUS_GERAK) {
		if (!sp->hover) {
			sp->hover = PG_BENAR;
			pg_widget_kotor(w);
		}
		return PG_BENAR;
	}
	/* TETIK_TURUN/NAIK: consume agar capture tetap di splitter. */
	return PG_BENAR;
}

/* Deallocator struktur turunan — dipanggil oleh
 * pg_widget_hancur_penuh() setelah hancur_v. */
static void pg_splitter_bebas_v(pg_widget_t *w)
{
	free(w);
}

static const pg_widget_vtable_t pg_splitter_vtable = {
	pg_splitter_catat_v,
	pg_splitter_peristiwa_v,
	NULL,
	NULL,
	NULL,
	NULL,
	pg_splitter_bebas_v
};

/* ---------------------------------------------------------------- API */

pg_splitter_t *pg_buat_splitter(pg_splitter_orientasi_t orientasi)
{
	pg_splitter_t *sp;
	int            w, h;
	sp = (pg_splitter_t *)calloc(1, sizeof(*sp));
	if (!sp) return NULL;
	pg_widget_init(&sp->base, PG_WIDGET_DASAR,
	               &pg_splitter_vtable);
	sp->orientasi = orientasi;
	sp->min1 = 20;
	sp->min2 = 20;
	sp->hover = PG_SALAH;
	sp->ditarik = PG_SALAH;
	w = (orientasi == PG_SPLITTER_HORI) ? PG_SPLITTER_TEBAL : 100;
	h = (orientasi == PG_SPLITTER_HORI) ? 100 : PG_SPLITTER_TEBAL;
	pg_widget_setel_kotak(&sp->base, pg_buat_kotak(0, 0, w, h));
	pg_widget_setel_latar(&sp->base, PG_ABU);
	pg_widget_setel_ukuran_min(&sp->base, w, h);
	pg_widget_saat_seret(&sp->base,
		pg_splitter_seret_mulai,
		pg_splitter_seret_gerak,
		pg_splitter_seret_selesai, NULL);
	return sp;
}

void pg_splitter_hancur(pg_splitter_t *sp)
{
	if (!sp) return;
	pg_widget_hancur(&sp->base);
	free(sp);
}

void pg_splitter_setel_panel(pg_splitter_t *sp,
                              pg_widget_t *panel1,
                              pg_widget_t *panel2)
{
	if (!sp) return;
	sp->panel1 = panel1;
	sp->panel2 = panel2;
}

void pg_splitter_setel_min(pg_splitter_t *sp, int min1, int min2)
{
	if (!sp) return;
	sp->min1 = min1 > 0 ? min1 : 0;
	sp->min2 = min2 > 0 ? min2 : 0;
}

void pg_splitter_saat_ubah(pg_splitter_t *sp,
    void (*cb)(int posisi, void *ctx), void *ctx)
{
	if (!sp) return;
	sp->cb_ubah = cb;
	sp->ctx = ctx;
}

pg_widget_t *pg_splitter_widget(pg_splitter_t *sp)
{
	return sp ? &sp->base : NULL;
}
