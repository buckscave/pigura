/* ----------------------------------------------------------------------------------------------
 * pigura widget: kotak.c - kontainer layout linear (polish v0.21)
 * ----------------------------------------------------------------------------------------------
 * Kotak menyusun anak-anaknya secara horizontal atau vertikal.
 *
 * Layout algorithm:
 *   avail = box_size - 2*box_padding
 *   total_min = sum(max(child.min, 1) + 2*child_pad) + spasi*(n-1)
 *   sisa = max(0, avail - total_min)
 *   extra_per = sisa / n_expand (bila n_expand > 0)
 *   Homogen: main_size = avail / n (semua sama, override min)
 *
 * Padding:
 *   - box_padding: internal box (semua sisi)
 *   - child padding: per-child (sekitar child individual)
 *
 * Visual:
 *   - Bila punya_latar: render latar (rounded bila radius > 0)
 *   - Bila punya_batas: render outline (rounded bila radius > 0)
 *   - Default: transparan (tidak render latar/batas)
 *
 * Layout dihitung ulang saat:
 *   - tambah/hapus child
 *   - setel_padding_kotak / setel_spasi / setel_orientasi / setel_homogen
 *   - ubah_ukuran (parent resize)
 *   - pg_kotak_tata() dipanggil manual (saat child min_size berubah)
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/kotak.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/peristiwa.h"
#include "pigura/widget.h"

#include <stdlib.h>
#include <string.h>

#define PG_KOTAK_MIN_CHILD 1   /* min absolut ukuran child */

typedef struct pg_anak_kotak {
	pg_widget_t *w;
	pg_bool      expand;
	pg_bool      fill;
	int          padding;
} pg_anak_kotak_t;

struct pg_kotak_widget {
	pg_widget_t      base;
	int              orientasi;
	int              spasi;
	int              padding;       /* box padding (semua sisi) */
	pg_anak_kotak_t *anak;
	int              n_anak;
	int              cap_anak;
	pg_bool          homogen;
	/* Visual. */
	pg_warna_t       latar;
	pg_bool          punya_latar;
	pg_warna_t       batas;
	pg_bool          punya_batas;
	/* Guard recursion. */
	pg_bool          sedang_layout;
};

static pg_kotak_widget_t *pg_kotak_dari(pg_widget_t *w)
{
	return (pg_kotak_widget_t *)w;
}

static void pg_kotak_posisikan(pg_widget_t *a, int x, int y,
                                int w, int h)
{
	pg_bool ubah;
	if (w < PG_KOTAK_MIN_CHILD) w = PG_KOTAK_MIN_CHILD;
	if (h < PG_KOTAK_MIN_CHILD) h = PG_KOTAK_MIN_CHILD;
	ubah = (a->kotak.w != w || a->kotak.h != h) ?
		PG_BENAR : PG_SALAH;
	if (ubah)
		pg_widget_setel_kotak(a,
			pg_buat_kotak(x, y, w, h));
	else
		pg_widget_pindah(a, x, y);
}

/* Hitung ulang posisi & ukuran semua anak. */
static void pg_kotak_layout(pg_kotak_widget_t *k)
{
	int box_w, box_h, box_pad;
	int avail_main, total_min, sisa, n_expand, extra_per;
	int pos, i;
	int horiz;
	int nat_min_w, nat_min_h;
	int homogen_size;

	if (!k->anak || k->n_anak <= 0) return;
	/* Guard recursion: bila sedang layout, jangan re-enter. */
	if (k->sedang_layout) return;
	k->sedang_layout = PG_BENAR;

	box_w = k->base.kotak.w;
	box_h = k->base.kotak.h;
	box_pad = k->padding;
	if (box_pad < 0) box_pad = 0;
	horiz = (k->orientasi == PG_KOTAK_HORIZONTAL);

	/* Hitung natural size dari child. */
	nat_min_w = 0;
	nat_min_h = 0;
	{
		int nat_main = 0;
		int nat_cross = 0;
		for (i = 0; i < k->n_anak; i++) {
			int m, c, pad2;
			pg_widget_t *a = k->anak[i].w;
			if (!a) continue;
			m = horiz ? a->min_w : a->min_h;
			c = horiz ? a->min_h : a->min_w;
			if (m < PG_KOTAK_MIN_CHILD) m = PG_KOTAK_MIN_CHILD;
			if (c < PG_KOTAK_MIN_CHILD) c = PG_KOTAK_MIN_CHILD;
			pad2 = 2 * k->anak[i].padding;
			if (pad2 < 0) pad2 = 0;
			nat_main += m + pad2;
			if (c + pad2 > nat_cross)
				nat_cross = c + pad2;
		}
		nat_main += k->spasi * (k->n_anak - 1);
		if (nat_main < 0) nat_main = 0;
		if (nat_cross < 0) nat_cross = 0;
		nat_main += 2 * box_pad;
		nat_cross += 2 * box_pad;
		if (horiz) {
			nat_min_w = nat_main;
			nat_min_h = nat_cross;
		} else {
			nat_min_w = nat_cross;
			nat_min_h = nat_main;
		}
	}

	/* Auto-size bila box belum di-resize. */
	if (box_w <= 0) box_w = nat_min_w;
	if (box_h <= 0) box_h = nat_min_h;
	k->base.min_w = nat_min_w;
	k->base.min_h = nat_min_h;
	if (k->base.kotak.w != box_w ||
	    k->base.kotak.h != box_h) {
		k->base.kotak.w = box_w;
		k->base.kotak.h = box_h;
		if (k->base.permukaan == NULL && box_w > 0 &&
		    box_h > 0) {
			k->base.permukaan =
				pg_buat_permukaan(box_w, box_h);
		}
	}

	avail_main = horiz ? (box_w - 2 * box_pad)
	                   : (box_h - 2 * box_pad);
	if (avail_main < 0) avail_main = 0;

	/* Homogen mode: semua child ukuran sama. */
	if (k->homogen && k->n_anak > 0) {
		int total_spasi = k->spasi * (k->n_anak - 1);
		int total_pad = 0;
		for (i = 0; i < k->n_anak; i++)
			total_pad += 2 * k->anak[i].padding;
		homogen_size = (avail_main - total_spasi - total_pad) /
			k->n_anak;
		if (homogen_size < PG_KOTAK_MIN_CHILD)
			homogen_size = PG_KOTAK_MIN_CHILD;
	} else {
		homogen_size = 0;
	}

	/* Hitung total minimum dan jumlah child expand. */
	total_min = 0;
	n_expand = 0;
	for (i = 0; i < k->n_anak; i++) {
		int natural, pad2;
		pg_widget_t *a = k->anak[i].w;
		if (!a) continue;
		if (k->homogen) {
			natural = homogen_size;
		} else {
			natural = horiz ? a->min_w : a->min_h;
			if (natural < PG_KOTAK_MIN_CHILD)
				natural = PG_KOTAK_MIN_CHILD;
		}
		pad2 = 2 * k->anak[i].padding;
		if (pad2 < 0) pad2 = 0;
		total_min += natural + pad2;
		if (k->anak[i].expand) n_expand++;
	}
	total_min += k->spasi * (k->n_anak - 1);
	if (total_min < 0) total_min = 0;

	sisa = avail_main - total_min;
	if (sisa < 0) sisa = 0;
	if (n_expand > 0 && !k->homogen)
		extra_per = sisa / n_expand;
	else
		extra_per = 0;

	pos = box_pad;
	for (i = 0; i < k->n_anak; i++) {
		int natural, pad, extra, slot, main_size;
		int avail_in_slot, offset_in_slot;
		int main_pos, cross_size, cross_pos;
		int avail_cross;
		pg_widget_t *a = k->anak[i].w;
		if (!a) continue;

		if (k->homogen) {
			natural = homogen_size;
		} else {
			natural = horiz ? a->min_w : a->min_h;
			if (natural < PG_KOTAK_MIN_CHILD)
				natural = PG_KOTAK_MIN_CHILD;
		}
		pad = k->anak[i].padding;
		if (pad < 0) pad = 0;
		extra = (k->anak[i].expand && !k->homogen) ? extra_per : 0;

		slot = 2 * pad + natural + extra;
		if (slot < 0) slot = 0;

		if (k->anak[i].expand && k->anak[i].fill && !k->homogen)
			main_size = natural + extra;
		else
			main_size = natural;
		if (main_size < PG_KOTAK_MIN_CHILD)
			main_size = PG_KOTAK_MIN_CHILD;

		avail_in_slot = slot - 2 * pad;
		if (avail_in_slot < main_size)
			avail_in_slot = main_size;
		offset_in_slot = (avail_in_slot - main_size) / 2;
		main_pos = pos + pad + offset_in_slot;

		avail_cross = horiz ? (box_h - 2 * box_pad)
		                    : (box_w - 2 * box_pad);
		if (avail_cross < 0) avail_cross = 0;
		if (k->anak[i].fill) {
			cross_size = avail_cross;
			cross_pos = box_pad;
		} else {
			int min_cross = horiz ? a->min_h : a->min_w;
			if (min_cross < PG_KOTAK_MIN_CHILD)
				min_cross = PG_KOTAK_MIN_CHILD;
			if (min_cross > avail_cross)
				min_cross = avail_cross;
			cross_size = min_cross;
			cross_pos = box_pad +
				(avail_cross - min_cross) / 2;
		}
		if (cross_size < PG_KOTAK_MIN_CHILD)
			cross_size = PG_KOTAK_MIN_CHILD;

		if (horiz)
			pg_kotak_posisikan(a, main_pos, cross_pos,
			                    main_size, cross_size);
		else
			pg_kotak_posisikan(a, cross_pos, main_pos,
			                    cross_size, main_size);

		pos += slot + k->spasi;
	}

	k->sedang_layout = PG_SALAH;
}

/* ===== Vtable ===== */

static void pg_kotak_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
	pg_kotak_widget_t *k = pg_kotak_dari(w);
	int i;
	int radius = w->radius;
	int sw, sh;
	sw = pg_permukaan_lebar(s);
	sh = pg_permukaan_tinggi(s);

	/* Render latar bila punya_latar. */
	if (k->punya_latar) {
		pg_kotak_t r = pg_buat_kotak(0, 0, sw, sh);
		if (radius > 0)
			pg_gambar_kotak_tumpul_isi_aa(s, r, radius,
				k->latar);
		else
			pg_isi_permukaan(s, k->latar);
	}
	/* Render border bila punya_batas. */
	if (k->punya_batas) {
		pg_kotak_t r = pg_buat_kotak(0, 0, sw, sh);
		if (radius > 0)
			pg_gambar_kotak_tumpul_aa(s, r, radius, k->batas);
		else
			pg_gambar_kotak_aa(s, r, k->batas);
	}

	/* Render anak. */
	for (i = 0; i < k->n_anak; i++) {
		if (k->anak[i].w)
			pg_widget_catat(k->anak[i].w, s);
	}
}

static pg_bool pg_kotak_peristiwa_v(pg_widget_t *w,
                                     const pg_peristiwa_t *e)
{
	pg_kotak_widget_t *k = pg_kotak_dari(w);
	int i;
	for (i = k->n_anak - 1; i >= 0; i--) {
		pg_widget_t *a;
		a = k->anak[i].w;
		if (!a) continue;
		if (pg_widget_tangani_peristiwa(a, e))
			return PG_BENAR;
	}
	return PG_SALAH;
}

static void pg_kotak_ubah_ukuran_v(pg_widget_t *w, int w_, int h)
{
	(void)w_;
	(void)h;
	pg_kotak_layout(pg_kotak_dari(w));
}

static void pg_kotak_hancur_v(pg_widget_t *w)
{
	pg_kotak_widget_t *k = pg_kotak_dari(w);
	if (k->anak) {
		free(k->anak);
		k->anak = NULL;
	}
	k->n_anak = 0;
	k->cap_anak = 0;
}

static void pg_kotak_bebas_v(pg_widget_t *w)
{
	free(w);
}

static pg_bool pg_kotak_berisi_v(pg_widget_t *w, pg_titik_t p)
{
	pg_kotak_widget_t *k = pg_kotak_dari(w);
	int i;
	if (pg_widget_berisi(w, p)) return PG_BENAR;
	for (i = 0; i < k->n_anak; i++) {
		pg_widget_t *a = k->anak[i].w;
		if (!a) continue;
		if (!a->vtable || !a->vtable->berisi) continue;
		{
			pg_titik_t p_child;
			p_child.x = p.x - a->kotak.x;
			p_child.y = p.y - a->kotak.y;
			if (a->vtable->berisi(a, p_child))
				return PG_BENAR;
		}
	}
	return PG_SALAH;
}

static const pg_widget_vtable_t pg_kotak_vtable = {
	pg_kotak_catat_v,
	pg_kotak_peristiwa_v,
	pg_kotak_ubah_ukuran_v,
	pg_kotak_hancur_v,
	NULL,
	pg_kotak_berisi_v,
	pg_kotak_bebas_v
};

/* ===== API publik ===== */

pg_kotak_widget_t *pg_buat_kotak_widget(int orientasi, int spasi)
{
	pg_kotak_widget_t *k;
	k = (pg_kotak_widget_t *)calloc(1, sizeof(*k));
	if (!k) return NULL;
	pg_widget_init(&k->base, PG_WIDGET_KOTAK,
	               &pg_kotak_vtable);
	pg_widget_milik(&k->base, PG_BENAR);
	k->orientasi = (orientasi == PG_KOTAK_VERTIKAL) ?
		PG_KOTAK_VERTIKAL : PG_KOTAK_HORIZONTAL;
	if (spasi < 0) spasi = 0;
	k->spasi = spasi;
	k->padding = 0;
	k->homogen = PG_SALAH;
	k->punya_latar = PG_SALAH;
	k->punya_batas = PG_SALAH;
	k->latar = PG_WARNA_PANEL;
	k->batas = PG_WARNA_HOVER_OUTLINE;
	k->sedang_layout = PG_SALAH;
	return k;
}

void pg_kotak_hancur(pg_kotak_widget_t *k)
{
	if (!k) return;
	pg_widget_hancur(&k->base);
	free(k);
}

void pg_kotak_milik(pg_kotak_widget_t *k, pg_bool milik)
{
	if (!k) return;
	pg_widget_milik(&k->base, milik);
}

void pg_kotak_bersih(pg_kotak_widget_t *k)
{
	int i;
	if (!k) return;
	if (k->base.milik && k->anak) {
		for (i = 0; i < k->n_anak; i++) {
			pg_widget_t *a = k->anak[i].w;
			if (!a) continue;
			pg_widget_hancur_penuh(a);
			k->anak[i].w = NULL;
		}
	} else if (k->anak) {
		for (i = 0; i < k->n_anak; i++) {
			pg_widget_t *a = k->anak[i].w;
			if (!a) continue;
			pg_widget_hapus_anak(&k->base, a);
			k->anak[i].w = NULL;
		}
	}
	if (k->anak) {
		free(k->anak);
		k->anak = NULL;
	}
	k->n_anak = 0;
	k->cap_anak = 0;
	pg_widget_kotor(&k->base);
}

void pg_kotak_tambah_lengkap(pg_kotak_widget_t *k, pg_widget_t *w,
                              pg_bool expand, pg_bool fill,
                              int padding)
{
	pg_anak_kotak_t *baru;
	if (!k || !w) return;
	if (k->n_anak >= k->cap_anak) {
		int cap_baru = k->cap_anak ? k->cap_anak * 2 : 4;
		baru = (pg_anak_kotak_t *)realloc(k->anak,
			(size_t)cap_baru * sizeof(*baru));
		if (!baru) return;
		k->anak = baru;
		k->cap_anak = cap_baru;
	}
	k->anak[k->n_anak].w = w;
	k->anak[k->n_anak].expand = expand;
	k->anak[k->n_anak].fill = fill;
	k->anak[k->n_anak].padding = padding;
	k->n_anak++;
	pg_widget_tambah_anak(&k->base, w);
	pg_kotak_layout(k);
	pg_widget_kotor(&k->base);
	/* Propagate ke ancestor — bila ukuran natural kita berubah,
	 * ancestor perlu re-layout. Guard di pg_kotak_layout cegah
	 * infinite recursion. */
	if (k->base.induk) {
		pg_widget_t *p = k->base.induk;
		while (p) {
			if (p->vtable && p->vtable->ubah_ukuran) {
				p->vtable->ubah_ukuran(p,
					p->kotak.w, p->kotak.h);
			}
			p = p->induk;
		}
	}
}

void pg_kotak_tambah(pg_kotak_widget_t *k, pg_widget_t *w,
                     pg_bool kembang)
{
	pg_kotak_tambah_lengkap(k, w, kembang, kembang, 0);
}

void pg_kotak_setel_padding_kotak(pg_kotak_widget_t *k, int padding)
{
	if (!k) return;
	if (padding < 0) padding = 0;
	k->padding = padding;
	pg_kotak_layout(k);
	pg_widget_kotor(&k->base);
}

void pg_kotak_setel_spasi(pg_kotak_widget_t *k, int spasi)
{
	if (!k) return;
	if (spasi < 0) spasi = 0;
	k->spasi = spasi;
	pg_kotak_layout(k);
	pg_widget_kotor(&k->base);
}

void pg_kotak_setel_orientasi(pg_kotak_widget_t *k, int orientasi)
{
	if (!k) return;
	k->orientasi = (orientasi == PG_KOTAK_VERTIKAL) ?
		PG_KOTAK_VERTIKAL : PG_KOTAK_HORIZONTAL;
	pg_kotak_layout(k);
	pg_widget_kotor(&k->base);
}

void pg_kotak_setel_homogen(pg_kotak_widget_t *k, pg_bool homogen)
{
	if (!k) return;
	k->homogen = homogen;
	pg_kotak_layout(k);
	pg_widget_kotor(&k->base);
}

void pg_kotak_setel_latar(pg_kotak_widget_t *k, pg_warna_t latar)
{
	if (!k) return;
	k->latar = latar;
	k->punya_latar = PG_BENAR;
	pg_widget_kotor(&k->base);
}

void pg_kotak_setel_latar_transparan(pg_kotak_widget_t *k)
{
	if (!k) return;
	k->punya_latar = PG_SALAH;
	pg_widget_kotor(&k->base);
}

void pg_kotak_setel_batas(pg_kotak_widget_t *k, pg_warna_t batas)
{
	if (!k) return;
	k->batas = batas;
	k->punya_batas = PG_BENAR;
	pg_widget_kotor(&k->base);
}

void pg_kotak_setel_batas_transparan(pg_kotak_widget_t *k)
{
	if (!k) return;
	k->punya_batas = PG_SALAH;
	pg_widget_kotor(&k->base);
}

void pg_kotak_tata(pg_kotak_widget_t *k)
{
	if (!k) return;
	pg_kotak_layout(k);
	pg_widget_kotor(&k->base);
}

void pg_kotak_hapus(pg_kotak_widget_t *k, int idx)
{
	int i;
	pg_widget_t *target;
	if (!k || idx < 0 || idx >= k->n_anak) return;
	target = k->anak[idx].w;
	k->anak[idx].w = NULL;
	k->anak[idx].expand = PG_SALAH;
	k->anak[idx].fill = PG_SALAH;
	k->anak[idx].padding = 0;
	for (i = idx; i < k->n_anak - 1; i++)
		k->anak[i] = k->anak[i + 1];
	k->n_anak--;
	if (target)
		pg_widget_hapus_anak(&k->base, target);
	pg_kotak_layout(k);
	pg_widget_kotor(&k->base);
}

pg_widget_t *pg_kotak_widget(pg_kotak_widget_t *k)
{
	return k ? &k->base : NULL;
}

int pg_kotak_jumlah_anak(pg_kotak_widget_t *k)
{
	return k ? k->n_anak : 0;
}

pg_widget_t *pg_kotak_anak(pg_kotak_widget_t *k, int idx)
{
	if (!k || idx < 0 || idx >= k->n_anak) return NULL;
	return k->anak[idx].w;
}
