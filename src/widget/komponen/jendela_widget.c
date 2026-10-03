/* ----------------------------------------------------------------------------------------------
 * pigura widget: jendela_widget.c - frame jendela dengan title bar
 * ----------------------------------------------------------------------------------------------
 * Jendela-widget adalah frame berhias: title bar biru di atas, body
 * di tengah untuk menampung anak, dan tombol close merah di kanan
 * atas title bar. Jendela bisa diseret dengan menahan klik kiri di
 * title bar. Klik tombol close memicu callback cb_tutup.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/jendela_widget.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/peristiwa.h"
#include "pigura/widget.h"

#include <stdlib.h>
#include <string.h>

/* Tinggi title bar dan ukuran tombol close. */
#define PG_JENDELA_TITLE_H  20
#define PG_JENDELA_CLOSE_W  14
#define PG_JENDELA_CLOSE_H  14

struct pg_jendela_widget {
	pg_widget_t          base;
	char                *judul;
	pg_widget_t          *anak;
	pg_bool               menyeret;
pg_bool               resize;
	pg_titik_t            resize_ofs;
	pg_titik_t            seret_ofs;
	pg_warna_t            judul_fg;
	pg_warna_t            judul_bg;
	pg_warna_t            batas;
pg_warna_t            body_bg;
	pg_jendela_widget_cb cb_tutup;
	void                *ctx;
	pg_font_t            *font;
};

static char *pg_jendela_dup(const char *s)
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

static pg_jendela_widget_t *pg_jendela_dari(pg_widget_t *w)
{
	return (pg_jendela_widget_t *)w;
}

/* vtable: catat title bar biru + close merah + body anak. */
static void pg_jendela_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
	pg_jendela_widget_t *j = pg_jendela_dari(w);
	int sw, sh, th;
	pg_kotak_t title_r, close_r;
	sw = pg_permukaan_lebar(s);
	sh = pg_permukaan_tinggi(s);
	th = j->font ? pg_font_tinggi(j->font) : 8;
	if (th <= 0) th = 8;
	title_r = pg_buat_kotak(0, 0, sw, PG_JENDELA_TITLE_H);
	close_r = pg_buat_kotak(sw - PG_JENDELA_CLOSE_W - 3, 3,
				 PG_JENDELA_CLOSE_W, PG_JENDELA_CLOSE_H);
	pg_isi_permukaan_kotak(s, title_r, j->judul_bg);
	pg_isi_permukaan_kotak(s, close_r, PG_MERAH);
	/* Tanda X di close. */
	pg_gambar_garis_aa(s,
		PG_KE_FIXED(close_r.x + 3),
		PG_KE_FIXED(close_r.y + 3),
		PG_KE_FIXED(close_r.x + close_r.w - 3),
		PG_KE_FIXED(close_r.y + close_r.h - 3),
		PG_PUTIH);
	pg_gambar_garis_aa(s,
		PG_KE_FIXED(close_r.x + close_r.w - 3),
		PG_KE_FIXED(close_r.y + 3),
		PG_KE_FIXED(close_r.x + 3),
		PG_KE_FIXED(close_r.y + close_r.h - 3),
		PG_PUTIH);
	if (j->judul && j->font)
		pg_font_gambar_teks(j->font, j->judul, s, 4,
			pg_font_baseline_tengah(j->font, PG_JENDELA_TITLE_H),
			j->judul_fg);
	pg_kotak_permukaan(s, pg_buat_kotak(0, 0, sw, sh), j->batas);
	/* Isi body dengan latar putih supaya tidak transparan. */
	pg_isi_permukaan_kotak(s,
		pg_buat_kotak(0, PG_JENDELA_TITLE_H, sw, sh - PG_JENDELA_TITLE_H),
		j->body_bg);
	/* Body: blit anak. */
	if (j->anak) {
		int sx, sy;
		sx = j->anak->kotak.x;
		sy = j->anak->kotak.y;
		j->anak->kotak.x = 0;
		j->anak->kotak.y = PG_JENDELA_TITLE_H;
		pg_widget_catat(j->anak, s);
		j->anak->kotak.x = sx;
		j->anak->kotak.y = sy;
	}
}

static pg_bool pg_jendela_peristiwa_v(pg_widget_t *w,
				     const pg_peristiwa_t *e)
{
	pg_jendela_widget_t *j = pg_jendela_dari(w);
	int sw, sh, close_x, close_y, close_h;
	sw = w->kotak.w;
	sh = w->kotak.h;
	close_x = sw - PG_JENDELA_CLOSE_W - 3;
	close_y = 3;
	close_h = PG_JENDELA_CLOSE_H;
	if (e->tipe == PG_PERISTIWA_TETIK_TURUN &&
	    e->tetik_tombol == PG_TETIK_KIRI) {
		int x = e->tetik_pos.x, y = e->tetik_pos.y;
		/* Tombol close? */
		if (x >= close_x && x < close_x + PG_JENDELA_CLOSE_W &&
		    y >= close_y && y < close_y + close_h) {
			if (j->cb_tutup) j->cb_tutup(j, j->ctx);
			return PG_BENAR;
		}
		/* Pojok kanan-bawah: mulai resize. */
		if (x >= sw - 8 && y >= sh - 8) {
			j->resize = PG_BENAR;
			j->resize_ofs = pg_buat_titik(sw - x, sh - y);
			return PG_BENAR;
		}
		/* Title bar? Mulai seret. */
		if (y < PG_JENDELA_TITLE_H) {
			j->menyeret = PG_BENAR;
			j->seret_ofs = pg_buat_titik(x, y);
			return PG_BENAR;
		}
		/* Body: teruskan ke anak. */
		if (j->anak) {
			pg_peristiwa_t e2 = *e;
			e2.tetik_pos.y -= PG_JENDELA_TITLE_H;
			return pg_widget_tangani_peristiwa(
				j->anak, &e2);
		}
		return PG_SALAH;
	}
	if (e->tipe == PG_PERISTIWA_TETIK_NAIK &&
	    e->tetik_tombol == PG_TETIK_KIRI) {
		if (j->menyeret) {
			j->menyeret = PG_SALAH;
			return PG_BENAR;
		}
		if (j->resize) {
			j->resize = PG_SALAH;
			return PG_BENAR;
		}
		if (j->anak) {
			pg_peristiwa_t e2 = *e;
			e2.tetik_pos.y -= PG_JENDELA_TITLE_H;
			return pg_widget_tangani_peristiwa(
				j->anak, &e2);
		}
		return PG_SALAH;
	}
	if (e->tipe == PG_PERISTIWA_TETIK_GERAK && j->menyeret) {
		int nx, ny;
		nx = w->kotak.x + e->tetik_pos.x - j->seret_ofs.x;
		ny = w->kotak.y + e->tetik_pos.y - j->seret_ofs.y;
		pg_widget_pindah(w, nx, ny);
		return PG_BENAR;
	}
	if (e->tipe == PG_PERISTIWA_TETIK_GERAK && j->resize) {
		int nw, nh;
		nw = e->tetik_pos.x + j->resize_ofs.x;
		nh = e->tetik_pos.y + j->resize_ofs.y;
		if (nw < 80) nw = 80;
		if (nh < 60) nh = 60;
		pg_widget_ubah_ukuran(w, nw, nh);
		return PG_BENAR;
	}
	/* Peristiwa lain: teruskan ke anak bila ada. */
	if (j->anak) {
		pg_peristiwa_t e2 = *e;
		e2.tetik_pos.y -= PG_JENDELA_TITLE_H;
		return pg_widget_tangani_peristiwa(j->anak, &e2);
	}
	return PG_SALAH;
}

/* vtable: ukuran berubah → resize anak di body. */
static void pg_jendela_ubah_ukuran_v(pg_widget_t *w, int w_, int h)
{
	pg_jendela_widget_t *j = pg_jendela_dari(w);
	if (j->anak && h > PG_JENDELA_TITLE_H)
		pg_widget_setel_kotak(j->anak,
			pg_buat_kotak(0, PG_JENDELA_TITLE_H,
				       w_, h - PG_JENDELA_TITLE_H));
}

static void pg_jendela_hancur_v(pg_widget_t *w)
{
	pg_jendela_widget_t *j = pg_jendela_dari(w);
	if (j->judul) {
		free(j->judul);
		j->judul = NULL;
	}
}

/* Deallocator struktur turunan — dipanggil oleh
 * pg_widget_hancur_penuh() setelah hancur_v. */
static void pg_jendela_bebas_v(pg_widget_t *w)
{
	free(w);
}

static const pg_widget_vtable_t pg_jendela_vtable = {
	pg_jendela_catat_v,
	pg_jendela_peristiwa_v,
	pg_jendela_ubah_ukuran_v,
	pg_jendela_hancur_v,
	NULL,
	NULL,
	pg_jendela_bebas_v
};

/* ---------------------------------------------------------------- API */

pg_jendela_widget_t *pg_buat_jendela_widget(const char *judul,
					     int w, int h,
					     pg_font_t *font)
{
	pg_jendela_widget_t *j;
	if (w <= 0 || h < PG_JENDELA_TITLE_H) return NULL;
	j = (pg_jendela_widget_t *)calloc(1, sizeof(*j));
	if (!j) return NULL;
	pg_widget_init(&j->base, PG_WIDGET_JENDELA,
		       &pg_jendela_vtable);
	j->font = font;
	j->judul_fg = PG_PUTIH;
	j->judul_bg = PG_BIRU;
	j->batas = PG_ABU_GELAP;
	j->body_bg = PG_PUTIH;
	if (judul) {
		j->judul = pg_jendela_dup(judul);
		if (!j->judul) {
			free(j);
			return NULL;
		}
	}
	pg_widget_setel_kotak(&j->base, pg_buat_kotak(0, 0, w, h));
	pg_widget_setel_latar(&j->base, PG_ABU_TERANG);
	return j;
}

void pg_jendela_widget_hancur(pg_jendela_widget_t *j)
{
	if (!j) return;
	pg_widget_hancur(&j->base);
	free(j);
}

void pg_jendela_widget_setel_anak(pg_jendela_widget_t *j,
				  pg_widget_t *anak)
{
	if (!j) return;
	j->anak = anak;
	if (anak) {
		int bw, bh;
		bw = j->base.kotak.w;
		bh = j->base.kotak.h - PG_JENDELA_TITLE_H;
		if (bh < 0) bh = 0;
		pg_widget_setel_kotak(anak,
			pg_buat_kotak(0, PG_JENDELA_TITLE_H,
				       bw, bh));
	}
	pg_widget_kotor(&j->base);
}

void pg_jendela_widget_saatutup(pg_jendela_widget_t *j,
				pg_jendela_widget_cb cb,
				void *ctx)
{
	if (!j) return;
	j->cb_tutup = cb;
	j->ctx = ctx;
}

pg_widget_t *pg_jendela_widget_widget(pg_jendela_widget_t *j)
{
	return j ? &j->base : NULL;
}
