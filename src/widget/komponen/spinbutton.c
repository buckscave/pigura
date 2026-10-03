#include "pigura/spinbutton.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/peristiwa.h"
#include "pigura/widget.h"
#include <stdlib.h>
struct pg_spinbutton {
	pg_widget_t base;
	pg_warna_t fg, btn_bg, batas;
	pg_font_t *font;
	pg_spinbutton_cb cb; void *ctx;
	pg_bool naik_hover, turun_hover, naik_tekan, turun_tekan;
};
static pg_spinbutton_t *d(pg_widget_t *w) { return (pg_spinbutton_t*)w; }
static void catat_v(pg_widget_t *w, pg_permukaan_t *s) {
	pg_spinbutton_t *sb=d(w); int sw,sh;
	sw=pg_permukaan_lebar(s); sh=pg_permukaan_tinggi(s);
	pg_isi_permukaan_kotak(s,pg_buat_kotak(0,0,sw,sh/2),
		sb->naik_tekan?PG_ABU:(sb->naik_hover?PG_ABU_TERANG:sb->btn_bg));
	pg_isi_permukaan_kotak(s,pg_buat_kotak(0,sh/2,sw,sh-sh/2),
		sb->turun_tekan?PG_ABU:(sb->turun_hover?PG_ABU_TERANG:sb->btn_bg));
	pg_kotak_permukaan(s,pg_buat_kotak(0,0,sw,sh),sb->batas);
	pg_garis_h_permukaan(s,0,sw,sh/2,sb->batas);
	{ int cx=sw/2, cy=sh/4;
	  pg_garis_h_permukaan(s,cx-3,cx+3,cy,sb->fg);
	  pg_garis_h_permukaan(s,cx-2,cx+2,cy+1,sb->fg);
	  pg_setel_piksel_permukaan(s,cx,cy+2,sb->fg); }
	{ int cx=sw/2, cy=sh*3/4;
	  pg_setel_piksel_permukaan(s,cx,cy-2,sb->fg);
	  pg_garis_h_permukaan(s,cx-2,cx+2,cy-1,sb->fg);
	  pg_garis_h_permukaan(s,cx-3,cx+3,cy,sb->fg); }
}
static pg_bool peristiwa_v(pg_widget_t *w, const pg_peristiwa_t *e) {
	pg_spinbutton_t *sb=d(w); int sh=w->kotak.h;
	if(e->tipe==PG_PERISTIWA_TETIK_TURUN && e->tetik_tombol==PG_TETIK_KIRI) {
		if(e->tetik_pos.y<sh/2) { sb->naik_tekan=PG_BENAR; if(sb->cb)sb->cb(sb,1,sb->ctx); }
		else { sb->turun_tekan=PG_BENAR; if(sb->cb)sb->cb(sb,0,sb->ctx); }
		pg_widget_kotor(w); return PG_BENAR;
	}
	if(e->tipe==PG_PERISTIWA_TETIK_NAIK) { sb->naik_tekan=PG_SALAH; sb->turun_tekan=PG_SALAH; pg_widget_kotor(w); return PG_BENAR; }
	if(e->tipe==PG_PERISTIWA_TETIK_GERAK) {
		pg_bool nh=(e->tetik_pos.y<sh/2)?PG_BENAR:PG_SALAH;
		pg_bool th=!nh;
		if(nh!=sb->naik_hover||th!=sb->turun_hover) { sb->naik_hover=nh; sb->turun_hover=th; pg_widget_kotor(w); }
		return PG_BENAR;
	}
	return PG_SALAH;
}
static void hancur_v(pg_widget_t *w) {}
static void bebas_v(pg_widget_t *w) { free(w); }
static const pg_widget_vtable_t vt = { catat_v, peristiwa_v, NULL, hancur_v, NULL, NULL, bebas_v };
pg_spinbutton_t *pg_buat_spinbutton(pg_font_t *font) {
	pg_spinbutton_t *sb=calloc(1,sizeof(*sb)); if(!sb) return NULL;
	pg_widget_init(&sb->base,PG_WIDGET_DASAR,&vt);
	sb->font=font; sb->fg=PG_HITAM; sb->btn_bg=PG_ABU_TERANG; sb->batas=PG_ABU_GELAP;
	return sb;
}
void pg_spinbutton_hancur(pg_spinbutton_t *sb) { if(!sb)return; pg_widget_hancur(&sb->base); free(sb); }
void pg_spinbutton_saatklik(pg_spinbutton_t *sb, pg_spinbutton_cb cb, void *ctx) { if(!sb)return; sb->cb=cb; sb->ctx=ctx; }
pg_widget_t *pg_spinbutton_widget(pg_spinbutton_t *sb) { return sb?&sb->base:NULL; }
