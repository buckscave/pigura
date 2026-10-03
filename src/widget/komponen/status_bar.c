/* ----------------------------------------------------------------------------------------------
 * pigura widget: status_bar.c - baris bawah multi-section
 * ----------------------------------------------------------------------------------------------
 * Status bar menampilkan beberapa seksi teks horizontal. Seksi dengan
 * proporsi > 0 mengisi ruang tersisa secara proporsional; seksi
 * dengan lebar > 0 punya lebar fixed.
 *
 * Layout: posisi sekansi dihitung saat catat. Garis vertikal antar
 * seksi sebagai pemisah. Latar = abu, teks = putih.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/status_bar.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/widget.h"

#include <stdlib.h>
#include <string.h>

#define PG_SB_H_DEFAULT 22

typedef struct pg_sb_seksi {
	int   lebar;     /* 0 bila proporsional */
	int   proporsi;  /* 0 bila fixed */
	char *teks;
	int   x;         /* posisi hitung saat layout */
	int   w;         /* lebar hitung saat layout */
} pg_sb_seksi_t;

struct pg_status_bar {
	pg_widget_t    base;
	pg_font_t     *font;
	pg_sb_seksi_t *seksi;
	int            n_seksi;
	int            cap_seksi;
	pg_warna_t     latar;
	pg_warna_t     fg;
	pg_warna_t     batas;
};

static char *pg_sb_dup(const char *s)
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

static pg_status_bar_t *pg_sb_dari(pg_widget_t *w)
{
	return (pg_status_bar_t *)w;
}

/* Hitung layout: lebar total, lebar fixed total, lebar proporsional. */
static void pg_sb_layout(pg_status_bar_t *sb)
{
	int total_w, fixed_w, total_prop, sisa, i, pos;
	total_w = sb->base.kotak.w;
	fixed_w = 0;
	total_prop = 0;
	for (i = 0; i < sb->n_seksi; i++) {
		if (sb->seksi[i].proporsi > 0)
			total_prop += sb->seksi[i].proporsi;
		else
			fixed_w += sb->seksi[i].lebar;
	}
	sisa = total_w - fixed_w;
	if (sisa < 0) sisa = 0;
	pos = 0;
	for (i = 0; i < sb->n_seksi; i++) {
		int w;
		if (sb->seksi[i].proporsi > 0 && total_prop > 0)
			w = (sisa * sb->seksi[i].proporsi) / total_prop;
		else
			w = sb->seksi[i].lebar;
		sb->seksi[i].x = pos;
		sb->seksi[i].w = w;
		pos += w;
	}
}

/* vtable: catat tiap seksi. */
static void pg_sb_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
	pg_status_bar_t *sb = pg_sb_dari(w);
	int              sw, sh, baseline, i;
	sw = pg_permukaan_lebar(s);
	sh = pg_permukaan_tinggi(s);
	(void)sw;
	pg_sb_layout(sb);
	baseline = sb->font ?
		pg_font_baseline_tengah(sb->font, sh) : sh / 2;
	for (i = 0; i < sb->n_seksi; i++) {
		pg_sb_seksi_t *se = &sb->seksi[i];
		/* Latar seksi (abu terang bila diklik? default abu). */
		pg_isi_permukaan_kotak(s,
			pg_buat_kotak(se->x, 0, se->w, sh),
			sb->latar);
		/* Garis pemisah kanan. */
		if (i < sb->n_seksi - 1 && se->w > 0)
			pg_garis_v_permukaan(s, se->x + se->w - 1, 2,
				sh - 4, sb->batas);
		/* Teks. */
		if (se->teks && sb->font)
			pg_font_gambar_teks(sb->font, se->teks, s,
				se->x + 6, baseline, sb->fg);
	}
	/* Garis atas status bar. */
	pg_garis_h_permukaan(s, 0, sw, 0, sb->batas);
}

static void pg_sb_ubah_ukuran_v(pg_widget_t *w, int w_, int h)
{
	(void)w_; (void)h;
	pg_sb_layout(pg_sb_dari(w));
}

static void pg_sb_hancur_v(pg_widget_t *w)
{
	pg_status_bar_t *sb = pg_sb_dari(w);
	int i;
	for (i = 0; i < sb->n_seksi; i++) {
		if (sb->seksi[i].teks) {
			free(sb->seksi[i].teks);
			sb->seksi[i].teks = NULL;
		}
	}
	if (sb->seksi) {
		free(sb->seksi);
		sb->seksi = NULL;
	}
	sb->n_seksi = 0;
	sb->cap_seksi = 0;
}

/* Deallocator struktur turunan — dipanggil oleh
 * pg_widget_hancur_penuh() setelah hancur_v. */
static void pg_sb_bebas_v(pg_widget_t *w)
{
	free(w);
}

static const pg_widget_vtable_t pg_sb_vtable = {
	pg_sb_catat_v,
	NULL,
	pg_sb_ubah_ukuran_v,
	pg_sb_hancur_v,
	NULL,
	NULL,
	pg_sb_bebas_v
};

/* ---------------------------------------------------------------- API */

pg_status_bar_t *pg_buat_status_bar(pg_font_t *font)
{
	pg_status_bar_t *sb;
	sb = (pg_status_bar_t *)calloc(1, sizeof(*sb));
	if (!sb) return NULL;
	pg_widget_init(&sb->base, PG_WIDGET_DASAR, &pg_sb_vtable);
	sb->font = font;
	sb->latar = PG_ABU;
	sb->fg = PG_PUTIH;
	sb->batas = PG_ABU_GELAP;
	pg_widget_setel_kotak(&sb->base,
		pg_buat_kotak(0, 0, 400, PG_SB_H_DEFAULT));
	pg_widget_setel_latar(&sb->base, PG_ABU);
	pg_widget_setel_ukuran_min(&sb->base, 80, PG_SB_H_DEFAULT);
	return sb;
}

void pg_status_bar_hancur(pg_status_bar_t *sb)
{
	if (!sb) return;
	pg_widget_hancur(&sb->base);
	free(sb);
}

int pg_sb_tambah_seksi(pg_status_bar_t *sb, int lebar, int proporsi)
{
	pg_sb_seksi_t *baru;
	if (!sb) return -1;
	if (sb->n_seksi >= sb->cap_seksi) {
		int cap_baru = sb->cap_seksi ? sb->cap_seksi * 2 : 4;
		baru = (pg_sb_seksi_t *)realloc(sb->seksi,
			(size_t)cap_baru * sizeof(*baru));
		if (!baru) return -1;
		sb->seksi = baru;
		sb->cap_seksi = cap_baru;
	}
	sb->seksi[sb->n_seksi].lebar = lebar > 0 ? lebar : 0;
	sb->seksi[sb->n_seksi].proporsi = proporsi > 0 ? proporsi : 0;
	sb->seksi[sb->n_seksi].teks = NULL;
	sb->seksi[sb->n_seksi].x = 0;
	sb->seksi[sb->n_seksi].w = 0;
	sb->n_seksi++;
	pg_widget_kotor(&sb->base);
	return sb->n_seksi - 1;
}

void pg_sb_setel_teks(pg_status_bar_t *sb, int seksi,
    const char *teks)
{
	char *baru;
	if (!sb || seksi < 0 || seksi >= sb->n_seksi) return;
	baru = pg_sb_dup(teks);
	if (teks && !baru) return;
	if (sb->seksi[seksi].teks)
		free(sb->seksi[seksi].teks);
	sb->seksi[seksi].teks = baru;
	pg_widget_kotor(&sb->base);
}

const char *pg_sb_ambil_teks(const pg_status_bar_t *sb, int seksi)
{
	if (!sb || seksi < 0 || seksi >= sb->n_seksi) return NULL;
	return sb->seksi[seksi].teks;
}

int pg_sb_jumlah_seksi(const pg_status_bar_t *sb)
{
	return sb ? sb->n_seksi : 0;
}

pg_widget_t *pg_status_bar_widget(pg_status_bar_t *sb)
{
	return sb ? &sb->base : NULL;
}
