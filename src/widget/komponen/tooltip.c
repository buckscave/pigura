/* ----------------------------------------------------------------------------------------------
 * pigura widget: tooltip.c - sistem tooltip global
 * ----------------------------------------------------------------------------------------------
 * Tooltip adalah label melayang yang muncul dekat kursor setelah
 * mouse hover di widget selama 500 ms. Implementasi:
 *
 *   - Registry global: linked list (widget, teks) untuk semua widget
 *     yang dipasang tooltip.
 *   - Tracking hover: saat TETIK_GERAK, cari widget terdaftar yang
 *     kotak-nya berisi posisi mouse. Bila beda dengan hover sebelumnya,
 *     restart timer 500ms.
 *   - Timer satu-kali: saat fire, setel visible=BENAR.
 *   - Render: bila visible, gambar kotak latar kuning pucat + teks
 *     hitam pada (mouse_x+10, mouse_y+10), clip ke dest.
 *
 * Registry tidak mengambil ownership widget; pemilik harus memastikan
 * widget tetap hidup selama tooltip terdaftar.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/tooltip.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/peristiwa.h"
#include "pigura/widget.h"
#include "pigura/timer.h"

#include <stdlib.h>
#include <string.h>

#define PG_TOOLTIP_DELAY_MS 500
#define PG_TOOLTIP_PAD_X 6
#define PG_TOOLTIP_PAD_Y 4
#define PG_TOOLTIP_OFS_X 12
#define PG_TOOLTIP_OFS_Y 12

typedef struct pg_tt_entry {
	pg_widget_t        *widget;
	char               *teks;
	struct pg_tt_entry *berikutnya;
} pg_tt_entry_t;

static pg_tt_entry_t *g_tt_head   = NULL;
static pg_font_t     *g_tt_font   = NULL;
static unsigned       g_tt_delay  = PG_TOOLTIP_DELAY_MS;
static pg_widget_t   *g_tt_hover  = NULL;
static pg_timer_t   *g_tt_timer   = NULL;
static pg_bool        g_tt_visible = PG_SALAH;
static pg_bool        g_tt_inited  = PG_SALAH;

static char *pg_tt_dup(const char *s)
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

static pg_tt_entry_t *pg_tt_cari(pg_widget_t *w)
{
	pg_tt_entry_t *e;
	for (e = g_tt_head; e; e = e->berikutnya)
		if (e->widget == w) return e;
	return NULL;
}

/* Callback timer: setel visible. */
static void pg_tt_timer_cb(void *ctx)
{
	(void)ctx;
	if (g_tt_hover)
		g_tt_visible = PG_BENAR;
}

static void pg_tt_timer_hentikan(void)
{
	if (g_tt_timer) {
		pg_timer_hancur(g_tt_timer);
		g_tt_timer = NULL;
	}
}

static void pg_tt_timer_mulai(void)
{
	pg_tt_timer_hentikan();
	if (g_tt_delay == 0) {
		/* Delay 0 = langsung visible. */
		g_tt_visible = PG_BENAR;
		return;
	}
	g_tt_visible = PG_SALAH;
	g_tt_timer = pg_timer_buat(g_tt_delay, pg_tt_timer_cb, NULL);
}

/* Cari widget terdaftar yang berisi titik p. */
static pg_widget_t *pg_tt_widget_di(pg_titik_t p)
{
	pg_tt_entry_t *e;
	for (e = g_tt_head; e; e = e->berikutnya) {
		if (e->widget && pg_widget_berisi(e->widget, p))
			return e->widget;
	}
	return NULL;
}

/* ---------------------------------------------------------------- API */

void pg_widget_setel_tooltip(pg_widget_t *w, const char *teks)
{
	pg_tt_entry_t *e;
	if (!w) return;
	e = pg_tt_cari(w);
	if (!teks) {
		/* Hapus entry. */
		pg_tt_entry_t **pp = &g_tt_head;
		while (*pp) {
			if (*pp == e) {
				*pp = e->berikutnya;
				if (e->teks) free(e->teks);
				free(e);
				break;
			}
			pp = &(*pp)->berikutnya;
		}
		if (g_tt_hover == w) {
			g_tt_hover = NULL;
			g_tt_visible = PG_SALAH;
			pg_tt_timer_hentikan();
		}
		return;
	}
	if (e) {
		char *baru = pg_tt_dup(teks);
		if (!baru) return;
		if (e->teks) free(e->teks);
		e->teks = baru;
		return;
	}
	e = (pg_tt_entry_t *)calloc(1, sizeof(*e));
	if (!e) return;
	e->widget = w;
	e->teks = pg_tt_dup(teks);
	if (!e->teks) {
		free(e);
		return;
	}
	e->berikutnya = g_tt_head;
	g_tt_head = e;
}

const char *pg_widget_ambil_tooltip(const pg_widget_t *w)
{
	pg_tt_entry_t *e;
	if (!w) return NULL;
	e = pg_tt_cari((pg_widget_t *)w);
	return e ? e->teks : NULL;
}

void pg_tooltip_init(pg_font_t *font)
{
	g_tt_font = font;
	g_tt_delay = PG_TOOLTIP_DELAY_MS;
	g_tt_hover = NULL;
	g_tt_visible = PG_SALAH;
	g_tt_inited = PG_BENAR;
}

void pg_tooltip_setel_font(pg_font_t *font)
{
	g_tt_font = font;
}

void pg_tooltip_setel_delay(unsigned ms)
{
	g_tt_delay = ms;
}

void pg_tooltip_tangani(const pg_peristiwa_t *e)
{
	pg_widget_t *hover_sekarang;
	if (!e || !g_tt_inited) return;
	if (e->tipe != PG_PERISTIWA_TETIK_GERAK &&
	    e->tipe != PG_PERISTIWA_TETIK_TURUN &&
	    e->tipe != PG_PERISTIWA_TETIK_NAIK)
		return;
	hover_sekarang = pg_tt_widget_di(e->tetik_pos);
	if (hover_sekarang != g_tt_hover) {
		g_tt_hover = hover_sekarang;
		g_tt_visible = PG_SALAH;
		if (g_tt_hover)
			pg_tt_timer_mulai();
		else
			pg_tt_timer_hentikan();
	}
}

void pg_tooltip_catat(pg_permukaan_t *dest, int mouse_x,
                       int mouse_y)
{
	pg_tt_entry_t *e;
	const char *teks;
	int tw, th, w, h, x, y, baseline;
	if (!dest || !g_tt_visible || !g_tt_hover) return;
	if (!g_tt_font) return;
	e = pg_tt_cari(g_tt_hover);
	if (!e || !e->teks) return;
	teks = e->teks;
	tw = pg_font_lebar_teks(g_tt_font, teks);
	th = pg_font_tinggi(g_tt_font);
	if (th <= 0) th = 8;
	w = tw + 2 * PG_TOOLTIP_PAD_X;
	h = th + 2 * PG_TOOLTIP_PAD_Y;
	x = mouse_x + PG_TOOLTIP_OFS_X;
	y = mouse_y + PG_TOOLTIP_OFS_Y;
	/* Clip ke dest. */
	if (x + w > pg_permukaan_lebar(dest))
		x = pg_permukaan_lebar(dest) - w;
	if (y + h > pg_permukaan_tinggi(dest))
		y = pg_permukaan_tinggi(dest) - h;
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	/* Bayangan tipis (offset 1). */
	pg_isi_permukaan_kotak(dest,
		pg_buat_kotak(x + 1, y + 1, w, h), PG_ABU_GELAP);
	/* Latar tooltip. */
	pg_isi_permukaan_kotak(dest,
		pg_buat_kotak(x, y, w, h), PG_KUNING);
	pg_gambar_kotak_aa(dest, pg_buat_kotak(x, y, w, h),
		PG_ABU_GELAP);
	/* Teks. */
	baseline = y + PG_TOOLTIP_PAD_Y +
		pg_font_baseline_tengah(g_tt_font, th);
	pg_font_gambar_teks(g_tt_font, teks, dest,
		x + PG_TOOLTIP_PAD_X, baseline, PG_HITAM);
}

void pg_tooltip_setel_visible(pg_bool visible)
{
	g_tt_visible = visible ? PG_BENAR : PG_SALAH;
}

pg_bool pg_tooltip_visible(void)
{
	return g_tt_visible;
}

void pg_tooltip_reset(void)
{
	pg_tt_timer_hentikan();
	g_tt_hover = NULL;
	g_tt_visible = PG_SALAH;
}

void pg_tooltip_selesai(void)
{
	pg_tt_entry_t *e = g_tt_head;
	pg_tt_timer_hentikan();
	while (e) {
		pg_tt_entry_t *n = e->berikutnya;
		if (e->teks) free(e->teks);
		free(e);
		e = n;
	}
	g_tt_head = NULL;
	g_tt_hover = NULL;
	g_tt_visible = PG_SALAH;
	g_tt_inited = PG_SALAH;
}
