#include "pigura/passwordinput.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/aksi.h"
#include "pigura/widget.h"
#include <stdlib.h>
#include <string.h>
struct pg_passwordinput {
	pg_widget_t base;
	char *buf; int cap, len, kursor;
	pg_warna_t fg, batas;
	pg_font_t *font;
	pg_passwordinput_cb cb;
	void *ctx;
};
static pg_passwordinput_t *d(pg_widget_t *w) { return (pg_passwordinput_t*)w; }
static void fire(pg_passwordinput_t *pi) { if(pi->cb) pi->cb(pi, pi->ctx); }
static void catat_v(pg_widget_t *w, pg_permukaan_t *s) {
	pg_passwordinput_t *pi=d(w); int sw,sh,th,y,i;
	pg_kotak_t r;
	sw=pg_permukaan_lebar(s); sh=pg_permukaan_tinggi(s);
	pg_isi_permukaan(s, PG_PUTIH);
	r=pg_buat_kotak(0,0,sw,sh);
	pg_gambar_kotak_aa(s,r, pg_widget_punya_fokus(w)?PG_BIRU:pi->batas);
	if(!pi->font) return;
	th=pg_font_tinggi(pi->font); if(th<=0) th=8;
	y=pg_font_baseline_tengah(pi->font,sh);
	/* Gambar • per karakter. */
	for(i=0;i<pi->len;i++) pg_font_gambar_teks(pi->font,"\xe2\x80\xa2",s,3+i*8,y,pi->fg);
	if(pg_widget_punya_fokus(w) && pi->kursor>=0 && pi->kursor<=pi->len) {
		int cx=3+pi->kursor*8;
		pg_garis_v_permukaan(s,cx,y-pg_font_ascent(pi->font),y+2,pi->fg);
	}
}
static pg_bool peristiwa_v(pg_widget_t *w, const pg_aksi_t *e) {
	pg_passwordinput_t *pi=d(w);
	if(e->tipe==PG_AKSI_TETIKUS_TEKAN && e->tetik_tombol==PG_TETIKUS_KIRI) {
		if(!pg_widget_punya_fokus(w)) pg_widget_fokus(w);
		return PG_BENAR;
	}
	if(e->tipe!=PG_AKSI_TOMBOL_TURUN || !pg_widget_punya_fokus(w)) return PG_SALAH;
	if(e->tombol==PG_TOMBOL_BACKSPACE) {
		if(pi->kursor>0 && pi->len>0) { int i; for(i=pi->kursor-1;i<pi->len-1;i++) pi->buf[i]=pi->buf[i+1]; pi->len--; pi->kursor--; pi->buf[pi->len]=0; pg_widget_kotor(w); fire(pi); }
		return PG_BENAR;
	}
	if(e->tombol==PG_TOMBOL_KIRI) { if(pi->kursor>0){pi->kursor--;pg_widget_kotor(w);} return PG_BENAR; }
	if(e->tombol==PG_TOMBOL_KANAN) { if(pi->kursor<pi->len){pi->kursor++;pg_widget_kotor(w);} return PG_BENAR; }
	{pg_u32 cp=e->unicode?e->unicode:(pg_u32)e->tombol; if(cp>=32 && cp<=126) {
		if(pi->len+1<pi->cap) { int i; for(i=pi->len;i>=pi->kursor;i--) pi->buf[i+1]=pi->buf[i]; pi->buf[pi->kursor]=(char)cp; pi->len++; pi->kursor++; pi->buf[pi->len]=0; pg_widget_kotor(w); fire(pi); }
		return PG_BENAR;
	}}
	return PG_SALAH;
}
static void hancur_v(pg_widget_t *w) { pg_passwordinput_t *pi=d(w); if(pi->buf)free(pi->buf); }
static void bebas_v(pg_widget_t *w) { free(w); }
static const pg_widget_vtable_t vt = { catat_v, peristiwa_v, NULL, hancur_v, NULL, NULL, bebas_v };
pg_passwordinput_t *pg_buat_passwordinput(const char *awal, int pm, pg_font_t *font) {
	pg_passwordinput_t *pi; int cap;
	if(pm<=0) pm=64;
	cap=pm+1;
	pi=calloc(1,sizeof(*pi)); if(!pi) return NULL;
	pg_widget_init(&pi->base,PG_WIDGET_DASAR,&vt);
	pi->font=font; pi->fg=PG_HITAM; pi->batas=PG_ABU; pi->cap=cap;
	pi->buf=calloc(cap,1); if(!pi->buf){free(pi);return NULL;}
	if(awal) { size_t n=strlen(awal); if(n>(size_t)pm)n=pm; memcpy(pi->buf,awal,n); pi->buf[n]=0; pi->len=n; pi->kursor=n; }
	return pi;
}
void pg_passwordinput_hancur(pg_passwordinput_t *pi) { if(!pi)return; pg_widget_hancur(&pi->base); free(pi); }
const char *pg_passwordinput_ambil_teks(pg_passwordinput_t *pi) { return pi?(pi->buf?pi->buf:""):""; }
void pg_passwordinput_setel_teks(pg_passwordinput_t *pi, const char *t) { if(!pi)return; if(!t){pi->buf[0]=0;pi->len=0;pi->kursor=0;pg_widget_kotor(&pi->base);return;} { size_t n=strlen(t); if(n>=(size_t)pi->cap)n=pi->cap-1; memcpy(pi->buf,t,n); pi->buf[n]=0; pi->len=n; pi->kursor=n; pg_widget_kotor(&pi->base);} }
void pg_passwordinput_saatberubah(pg_passwordinput_t *pi, pg_passwordinput_cb cb, void *ctx) { if(!pi)return; pi->cb=cb; pi->ctx=ctx; }
pg_widget_t *pg_passwordinput_widget(pg_passwordinput_t *pi) { return pi?&pi->base:NULL; }
