/* ----------------------------------------------------------------------------------------------
 * pigura widget: label.c - widget label teks statis (polish v0.20)
 * ----------------------------------------------------------------------------------------------
 * Default:
 *   - Perataan KIRI
 *   - Padding 4px (internal)
 *   - Tidak ada latar (transparan)
 *
 * Bila label punya warna latar, latar mengisi area termasuk padding.
 *
 * Margin ke parent diatur via pg_kotak_tambah_lengkap (parameter
 * padding di layout manager) — bukan tanggung jawab label.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/label.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/widget.h"
#include "pigura/galat.h"

#include <stdlib.h>
#include <string.h>

#define PG_LABEL_PADDING_DEFAULT 4

struct pg_label {
	pg_widget_t  base;
	char        *teks;
	pg_warna_t   warna;       /* warna teks */
	pg_warna_t   latar;       /* warna latar (bila punya_latar=BENAR) */
	pg_bool      punya_latar; /* BENAR = render latar sendiri */
	pg_font_t   *font;
	char        *font_path;   /* path font TTF (untuk setel_ukuran_teks) */
	int          font_ukuran;/* ukuran font saat ini */
	pg_label_perataan_t perataan;
	int          padding;
	pg_bool      bungkus;     /* multi-line wrap */
};

static char *pg_label_dup(const char *s)
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

static pg_label_t *pg_label_dari(pg_widget_t *w)
{
	return (pg_label_t *)w;
}

/* Update min_w/min_h sesuai teks + font + padding. */
static void pg_label_update_min(pg_label_t *l)
{
	int tw = 0, th = 0;
	int pad = l->padding * 2;
	if (l->font && l->teks) {
		tw = pg_font_lebar_teks(l->font, l->teks);
		th = pg_font_tinggi(l->font);
	}
	if (tw < 1) tw = 1;
	if (th < 1) th = 8;
	pg_widget_setel_ukuran_min(&l->base, tw + pad, th + pad);
}

/* ===== Render ===== */

/* Render satu baris teks dengan perataan. */
static void pg_label_render_baris(pg_label_t *l, pg_permukaan_t *s,
                                    const char *baris, int y_baseline,
                                    int area_x, int area_w)
{
	int tw = pg_font_lebar_teks(l->font, baris);
	int x;
	switch (l->perataan) {
	case PG_LABEL_PERATAAN_KANAN:
		x = area_x + area_w - tw;
		break;
	case PG_LABEL_PERATAAN_TENGAH:
		x = area_x + (area_w - tw) / 2;
		break;
	case PG_LABEL_PERATAAN_KIRI:
	default:
		x = area_x;
		break;
	}
	if (x < 0) x = 0;
	pg_font_gambar_teks(l->font, baris, s, x, y_baseline, l->warna);
}

/* Hitung jumlah baris untuk wrap. Sederhana: pecah di spasi. */
static int pg_label_hitung_baris(pg_label_t *l, int area_w)
{
	int n = 1;
	const char *p;
	int cur_w = 0;
	int spasi_w;
	if (!l->teks || !l->font || !l->bungkus) return 1;
	if (area_w <= 0) return 1;
	spasi_w = pg_font_lebar_teks(l->font, " ");
	p = l->teks;
	while (*p) {
		const char *word_start = p;
		const char *word_end = p;
		int word_w;
		while (*word_end && *word_end != ' ' && *word_end != '\n')
			word_end++;
		{
			char tmp[256];
			int wl = (int)(word_end - word_start);
			if (wl > 255) wl = 255;
			memcpy(tmp, word_start, wl);
			tmp[wl] = 0;
			word_w = pg_font_lebar_teks(l->font, tmp);
		}
		if (cur_w + word_w > area_w && cur_w > 0) {
			n++;
			cur_w = word_w + spasi_w;
		} else {
			cur_w += word_w + spasi_w;
		}
		if (*word_end == '\n') n++;
		p = (*word_end == 0) ? word_end : word_end + 1;
	}
	return n;
}

static void pg_label_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
	pg_label_t *l = pg_label_dari(w);
	int sw, sh;
	int pad = l->padding;
	int area_x, area_y, area_w, area_h;
	int th, baseline;
	int radius = w->radius;

	if (!l->teks || !l->font) return;
	sw = pg_permukaan_lebar(s);
	sh = pg_permukaan_tinggi(s);
	area_x = pad;
	area_y = pad;
	area_w = sw - pad * 2;
	area_h = sh - pad * 2;
	if (area_w < 0) area_w = 0;
	if (area_h < 0) area_h = 0;

	/* Latar (bila punya_latar). */
	if (l->punya_latar) {
		pg_kotak_t r = pg_buat_kotak(0, 0, sw, sh);
		if (radius > 0)
			pg_gambar_kotak_tumpul_isi_aa(s, r, radius, l->latar);
		else
			pg_isi_permukaan(s, l->latar);
	}

	th = pg_font_tinggi(l->font);
	if (th <= 0) th = 8;

	/* Multi-line wrap. */
	if (l->bungkus && area_w > 0) {
		int n_baris = pg_label_hitung_baris(l, area_w);
		int baris_h = th + 2;
		int total_h = n_baris * baris_h;
		int start_y = area_y;
		/* Center vertikal blok teks bila kurang dari area. */
		if (total_h < area_h)
			start_y = area_y + (area_h - total_h) / 2;
		{
			const char *p = l->teks;
			int baris_idx = 0;
			char buf[256];
			int buf_len = 0;
			int cur_w = 0;
			int spasi_w = pg_font_lebar_teks(l->font, " ");
			while (*p) {
				const char *word_start = p;
				const char *word_end = p;
				int word_w;
				while (*word_end && *word_end != ' ' &&
				       *word_end != '\n')
					word_end++;
				{
					int wl = (int)(word_end - word_start);
					if (wl > 255 - buf_len) wl = 255 - buf_len;
					if (wl > 0) {
						/* Hitung word width. */
						char wtmp[64];
						int wlen = (wl < 63) ? wl : 63;
						memcpy(wtmp, word_start, wlen);
						wtmp[wlen] = 0;
						word_w = pg_font_lebar_teks(l->font, wtmp);
						if (cur_w + word_w > area_w &&
						    buf_len > 0) {
							/* Flush baris. */
							buf[buf_len] = 0;
							pg_label_render_baris(l, s,
											 buf,
											 start_y + baris_idx * baris_h + th,
											 area_x, area_w);
							baris_idx++;
							buf_len = 0;
							cur_w = 0;
						}
						memcpy(buf + buf_len, word_start, wl);
						buf_len += wl;
						cur_w += word_w + spasi_w;
					}
				}
				if (*word_end == '\n') {
					/* Force break. */
					buf[buf_len] = 0;
					pg_label_render_baris(l, s, buf,
						start_y + baris_idx * baris_h + th,
						area_x, area_w);
					baris_idx++;
					buf_len = 0;
					cur_w = 0;
				}
				p = (*word_end == 0) ? word_end : word_end + 1;
			}
			if (buf_len > 0) {
				buf[buf_len] = 0;
				pg_label_render_baris(l, s, buf,
					start_y + baris_idx * baris_h + th,
					area_x, area_w);
			}
		}
		return;
	}

	/* Single line. */
	baseline = pg_font_baseline_tengah(l->font, sh);
	pg_label_render_baris(l, s, l->teks, baseline, area_x, area_w);
}

static void pg_label_hancur_v(pg_widget_t *w)
{
	pg_label_t *l = pg_label_dari(w);
	if (l->teks) {
		free(l->teks);
		l->teks = NULL;
	}
	if (l->font_path) {
		free(l->font_path);
		l->font_path = NULL;
	}
	/* Catatan: font TIDAK dihancurkan — shared dengan app. */
}

static void pg_label_bebas_v(pg_widget_t *w)
{
	free(w);
}

static const pg_widget_vtable_t pg_label_vtable = {
	pg_label_catat_v,
	NULL,
	NULL,
	pg_label_hancur_v,
	NULL,
	NULL,
	pg_label_bebas_v
};

/* ===== API publik ===== */

pg_label_t *pg_buat_label(const char *teks, pg_warna_t warna,
                           pg_font_t *font)
{
	pg_label_t *l;
	l = (pg_label_t *)calloc(1, sizeof(*l));
	if (!l) return NULL;
	pg_widget_init(&l->base, PG_WIDGET_LABEL, &pg_label_vtable);
	pg_widget_milik(&l->base, PG_BENAR);
	l->warna = warna;
	l->font = font;
	l->perataan = PG_LABEL_PERATAAN_KIRI;
	l->padding = PG_LABEL_PADDING_DEFAULT;
	l->punya_latar = PG_SALAH;
	l->latar = PG_WARNA_PANEL;
	l->bungkus = PG_SALAH;
	if (teks) {
		l->teks = pg_label_dup(teks);
		if (!l->teks) {
			free(l);
			return NULL;
		}
	}
	if (font) {
		l->font_ukuran = pg_font_ukuran_px(font);
	}
	pg_label_update_min(l);
	return l;
}

void pg_label_hancur(pg_label_t *l)
{
	if (!l) return;
	pg_widget_hancur(&l->base);
	free(l);
}

void pg_label_setel_teks(pg_label_t *l, const char *teks)
{
	char *baru;
	if (!l) return;
	baru = pg_label_dup(teks);
	if (teks && !baru) return;
	if (l->teks) free(l->teks);
	l->teks = baru;
	pg_label_update_min(l);
	pg_widget_kotor(&l->base);
}

void pg_label_setel_perataan(pg_label_t *l, pg_label_perataan_t p)
{
	if (!l) return;
	l->perataan = p;
	pg_widget_kotor(&l->base);
}

void pg_label_setel_padding(pg_label_t *l, int padding)
{
	if (!l) return;
	if (padding < 0) padding = 0;
	l->padding = padding;
	pg_label_update_min(l);
	pg_widget_kotor(&l->base);
}

void pg_label_setel_warna(pg_label_t *l, pg_warna_t warna)
{
	if (!l) return;
	l->warna = warna;
	pg_widget_kotor(&l->base);
}

void pg_label_setel_latar(pg_label_t *l, pg_warna_t latar)
{
	if (!l) return;
	l->latar = latar;
	l->punya_latar = PG_BENAR;
	pg_widget_kotor(&l->base);
}

void pg_label_setel_latar_transparan(pg_label_t *l)
{
	if (!l) return;
	l->punya_latar = PG_SALAH;
	pg_widget_kotor(&l->base);
}

void pg_label_setel_font(pg_label_t *l, pg_font_t *font)
{
	if (!l || !font) return;
	l->font = font;
	l->font_ukuran = pg_font_ukuran_px(font);
	/* Update path bila belum ada (untuk setel_ukuran_teks). */
	pg_label_update_min(l);
	pg_widget_kotor(&l->base);
}

void pg_label_setel_jalur_font(pg_label_t *l, const char *path)
{
	if (!l) return;
	if (l->font_path) {
		free(l->font_path);
		l->font_path = NULL;
	}
	if (path) l->font_path = pg_label_dup(path);
}

void pg_label_setel_ukuran_teks(pg_label_t *l, int ukuran_px)
{
	pg_font_t *font_baru;
	if (!l || !l->font_path || ukuran_px <= 0) return;
	/* Hanya TTF yang bisa resize. */
	if (pg_font_jenis(l->font) != PG_FONT_JENIS_TTF) return;
	font_baru = pg_buat_font_ttf(l->font_path, ukuran_px);
	if (!font_baru) return;
	/* Catatan: font lama TIDAK dihancurkan — app mungkin masih
	 * pegang reference. Label hanya replace pointer-nya sendiri. */
	l->font = font_baru;
	l->font_ukuran = ukuran_px;
	pg_label_update_min(l);
	pg_widget_kotor(&l->base);
}

void pg_label_setel_bungkus(pg_label_t *l, pg_bool bungkus)
{
	if (!l) return;
	l->bungkus = bungkus;
	pg_widget_kotor(&l->base);
}

const char *pg_label_teks(const pg_label_t *l)
{
	return l ? l->teks : NULL;
}

pg_widget_t *pg_label_widget(pg_label_t *l)
{
	return l ? &l->base : NULL;
}
