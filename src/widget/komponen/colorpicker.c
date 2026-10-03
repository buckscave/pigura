#include "pigura/colorpicker.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/peristiwa.h"
#include "pigura/widget.h"
#include <stdlib.h>
struct pg_colorpicker {
	pg_widget_t base;
	pg_warna_t warna;
	pg_warna_t batas;
	int swatch;
	pg_font_t *font;
	pg_colorpicker_cb cb; void *ctx;
};
static pg_colorpicker_t *d(pg_widget_t *w) { return (pg_colorpicker_t*)w; }
static void fire(pg_colorpicker_t *cp) { if(cp->cb) cp->cb(cp, cp->warna, cp->ctx); }
static void catat_v(pg_widget_t *w, pg_permukaan_t *s) {
	pg_colorpicker_t *cp=d(w); int sw,sh,i,j;
	pg_kotak_t r;
	sw=pg_permukaan_lebar(s); sh=pg_permukaan_tinggi(s);
	pg_isi_permukaan(s, PG_PUTIH);
	r=pg_buat_kotak(0,0,sw,sh); pg_gambar_kotak_aa(s,r,cp->batas);
	/* Color grid 8x4. */
	{ int cw=sw/8, ch=sh/4;
	  for(i=0;i<4;i++) for(j=0;j<8;j++) {
		pg_warna_t c;
		int r_val=(j*255)/7, g_val=(i*255)/3, b_val=128;
		c=PG_RGB(r_val,g_val,b_val);
		pg_isi_permukaan_kotak(s,pg_buat_kotak(j*cw,i*ch,cw,ch),c);
	  }
	}
	/* Selected swatch border. */
	{ int sx=cp->swatch%8, sy=cp->swatch/8;
	  int cw=sw/8, ch=sh/4;
	  pg_gambar_kotak_aa(s,pg_buat_kotak(sx*cw,sy*ch,cw,ch),PG_PUTIH);
	  pg_gambar_kotak_aa(s,pg_buat_kotak(sx*cw+1,sy*ch+1,cw-2,ch-2),PG_HITAM);
	}
}
static pg_bool peristiwa_v(pg_widget_t *w, const pg_peristiwa_t *e) {
	pg_colorpicker_t *cp=d(w); int sw=w->kotak.w, sh=w->kotak.h;
	if(e->tipe==PG_PERISTIWA_TETIK_TURUN && e->tetik_tombol==PG_TETIK_KIRI) {
		int cw=sw/8, ch=sh/4;
		int sx=e->tetik_pos.x/cw, sy=e->tetik_pos.y/ch;
		if(sx>=0&&sx<8&&sy>=0&&sy<4) {
			cp->swatch=sy*8+sx;
			cp->warna=PG_RGB((sx*255)/7,(sy*255)/3,128);
			pg_widget_kotor(w); fire(cp);
		}
		return PG_BENAR;
	}
	return PG_SALAH;
}
static void hancur_v(pg_widget_t *w) {}
static void bebas_v(pg_widget_t *w) { free(w); }
static const pg_widget_vtable_t vt = { catat_v, peristiwa_v, NULL, hancur_v, NULL, NULL, bebas_v };
pg_colorpicker_t *pg_buat_colorpicker(pg_font_t *font) {
	pg_colorpicker_t *cp=calloc(1,sizeof(*cp)); if(!cp) return NULL;
	pg_widget_init(&cp->base,PG_WIDGET_DASAR,&vt);
	cp->font=font; cp->batas=PG_ABU_GELAP; cp->warna=PG_MERAH; cp->swatch=0;
	return cp;
}
void pg_colorpicker_hancur(pg_colorpicker_t *cp) { if(!cp)return; pg_widget_hancur(&cp->base); free(cp); }
pg_warna_t pg_colorpicker_ambil_warna(pg_colorpicker_t *cp) { return cp?cp->warna:PG_HITAM; }
void pg_colorpicker_setel_warna(pg_colorpicker_t *cp, pg_warna_t w) { if(!cp)return; cp->warna=w; pg_widget_kotor(&cp->base); }
void pg_colorpicker_saatberubah(pg_colorpicker_t *cp, pg_colorpicker_cb cb, void *ctx) { if(!cp)return; cp->cb=cb; cp->ctx=ctx; }
pg_widget_t *pg_colorpicker_widget(pg_colorpicker_t *cp) { return cp?&cp->base:NULL; }
