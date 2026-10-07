#include <stdio.h>
#include "pigura/datepicker.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/aksi.h"
#include "pigura/widget.h"
#include <stdlib.h>
#include <string.h>
static int hari_dlm_bulan(int tahun, int bulan) {
	static int h[] = {31,28,31,30,31,30,31,31,30,31,30,31};
	if(bulan==2) { if((tahun%4==0&&tahun%100!=0)||tahun%400==0) return 29; return 28; }
	return h[bulan-1];
}
struct pg_datepicker {
	pg_widget_t base;
	int tahun, bulan, hari;
	pg_warna_t fg, latar, batas, sel_bg, sel_fg, hdr_bg, hdr_fg;
	pg_font_t *font;
	pg_datepicker_cb cb; void *ctx;
};
static pg_datepicker_t *d(pg_widget_t *w) { return (pg_datepicker_t*)w; }
static void fire(pg_datepicker_t *dp) { if(dp->cb) dp->cb(dp, dp->tahun, dp->bulan, dp->hari, dp->ctx); }
static void catat_v(pg_widget_t *w, pg_permukaan_t *s) {
	pg_datepicker_t *dp=d(w); int sw,sh,i,y,dim,rh,cw;
	char buf[32];
	const char *bln[]={"Jan","Feb","Mar","Apr","Mei","Jun","Jul","Agu","Sep","Okt","Nov","Des"};
	const char *hr[]={"S","S","R","K","J","S","M"};
	sw=pg_permukaan_lebar(s); sh=pg_permukaan_tinggi(s);
	pg_isi_permukaan(s, dp->latar);
	pg_kotak_permukaan(s,pg_buat_kotak(0,0,sw,sh),dp->batas);
	if(!dp->font) return;
	rh=pg_font_tinggi(dp->font)+2; if(rh<=0) rh=10;
	cw=sw/7;
	/* Header bulan/tahun. */
	snprintf(buf,sizeof(buf),"%s %d", bln[dp->bulan-1], dp->tahun);
	pg_isi_permukaan_kotak(s,pg_buat_kotak(0,0,sw,rh),dp->hdr_bg);
	pg_font_gambar_teks(dp->font,buf,s,4,pg_font_baseline_tengah(dp->font,rh),dp->hdr_fg);
	/* Nama hari. */
	for(i=0;i<7;i++) pg_font_gambar_teks(dp->font,hr[i],s,i*cw+3,rh+pg_font_baseline_tengah(dp->font,rh),dp->fg);
	/* Hari. */
	dim=hari_dlm_bulan(dp->tahun,dp->bulan);
	{ int dow=1; /* Simplified: start dari hari 1 = Senin */
	  int row, col, day;
	  for(day=1;day<=dim;day++) {
		col=(day-1+dow)%7; row=(day-1+dow)/7;
		y=rh*2+row*rh;
		if(day==dp->hari) pg_isi_permukaan_kotak(s,pg_buat_kotak(col*cw,y,cw,rh),dp->sel_bg);
		snprintf(buf,sizeof(buf),"%d",day);
		pg_font_gambar_teks(dp->font,buf,s,col*cw+3,y+pg_font_baseline_tengah(dp->font,rh),
			(day==dp->hari)?dp->sel_fg:dp->fg);
	  }
	}
}
static pg_bool peristiwa_v(pg_widget_t *w, const pg_aksi_t *e) {
	pg_datepicker_t *dp=d(w);
	if(e->tipe==PG_AKSI_TETIKUS_TEKAN && e->tetik_tombol==PG_TETIKUS_KIRI) {
		int sw=w->kotak.w, rh=pg_font_tinggi(dp->font)+2, cw=sw/7;
		int dim=hari_dlm_bulan(dp->tahun,dp->bulan);
		int dow=1, row, col, day;
		int y=e->tetik_pos.y;
		if(y<rh*2) return PG_BENAR; /* Header area */
		row=(y-rh*2)/rh; col=e->tetik_pos.x/cw;
		day=row*7+col-dow+1;
		if(day>=1&&day<=dim) { dp->hari=day; pg_widget_kotor(w); fire(dp); }
		return PG_BENAR;
	}
	return PG_SALAH;
}
static void hancur_v(pg_widget_t *w) {}
static void bebas_v(pg_widget_t *w) { free(w); }
static const pg_widget_vtable_t vt = { catat_v, peristiwa_v, NULL, hancur_v, NULL, NULL, bebas_v };
pg_datepicker_t *pg_buat_datepicker(pg_font_t *font) {
	pg_datepicker_t *dp=calloc(1,sizeof(*dp)); if(!dp) return NULL;
	pg_widget_init(&dp->base,PG_WIDGET_DASAR,&vt);
	dp->font=font; dp->fg=PG_HITAM; dp->latar=PG_PUTIH; dp->batas=PG_ABU;
	dp->sel_bg=PG_BIRU; dp->sel_fg=PG_PUTIH; dp->hdr_bg=PG_ABU_GELAP; dp->hdr_fg=PG_PUTIH;
	dp->tahun=2026; dp->bulan=1; dp->hari=1;
	return dp;
}
void pg_datepicker_hancur(pg_datepicker_t *dp) { if(!dp)return; pg_widget_hancur(&dp->base); free(dp); }
void pg_datepicker_setel_tanggal(pg_datepicker_t *dp, int t, int b, int h) { if(!dp)return; dp->tahun=t; dp->bulan=b; dp->hari=h; pg_widget_kotor(&dp->base); }
void pg_datepicker_saatubah(pg_datepicker_t *dp, pg_datepicker_cb cb, void *ctx) { if(!dp)return; dp->cb=cb; dp->ctx=ctx; }
pg_widget_t *pg_datepicker_widget(pg_datepicker_t *dp) { return dp?&dp->base:NULL; }
