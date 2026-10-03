#include "pigura/searchbox.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/peristiwa.h"
#include "pigura/widget.h"
#include <stdlib.h>
#include <string.h>
struct pg_searchbox {
	pg_widget_t base;
	char *buf; int cap, len, kursor;
	char *placeholder;
	pg_warna_t fg, batas, btn_bg, ph_fg;
	pg_font_t *font;
	pg_searchbox_cb cb;
	void *ctx;
	pg_bool clear_hover, clear_tekan;
};
static pg_searchbox_t *d(pg_widget_t *w) { return (pg_searchbox_t*)w; }
static void fire(pg_searchbox_t *sb) { if(sb->cb) sb->cb(sb, sb->buf?sb->buf:"", sb->ctx); }
static int btn_w(void) { return 20; }
static void catat_v(pg_widget_t *w, pg_permukaan_t *s) {
	pg_searchbox_t *sb=d(w); int sw,sh,bw,bl,th,y;
	pg_kotak_t r;
	sw=pg_permukaan_lebar(s); sh=pg_permukaan_tinggi(s);
	pg_isi_permukaan(s, PG_PUTIH);
	r=pg_buat_kotak(0,0,sw,sh); pg_gambar_kotak_aa(s,r,pg_widget_punya_fokus(w)?PG_BIRU:sb->batas);
	bw=btn_w(); bl=sw-bw;
	if(sb->len>0) {
		/* Tombol clear (✕). */
		pg_isi_permukaan_kotak(s, pg_buat_kotak(bl,0,bw,sh),
			sb->clear_tekan?PG_ABU:(sb->clear_hover?PG_ABU_TERANG:sb->btn_bg));
		pg_garis_v_permukaan(s, bl, 0, sh, sb->batas);
		{ int cx=bl+bw/2, cy=sh/2;
		  pg_gambar_garis_aa(s,PG_KE_FIXED(cx-4),PG_KE_FIXED(cy-4),
			PG_KE_FIXED(cx+4),PG_KE_FIXED(cy+4),sb->fg);
		  pg_gambar_garis_aa(s,PG_KE_FIXED(cx+4),PG_KE_FIXED(cy-4),
			PG_KE_FIXED(cx-4),PG_KE_FIXED(cy+4),sb->fg); }
	}
	if(sb->font) {
		th=pg_font_tinggi(sb->font); if(th<=0) th=8;
		y=pg_font_baseline_tengah(sb->font,sh);
		if(sb->len>0) pg_font_gambar_teks(sb->font,sb->buf,s,3,y,sb->fg);
		else if(sb->placeholder) pg_font_gambar_teks(sb->font,sb->placeholder,s,3,y,sb->ph_fg);
		if(pg_widget_punya_fokus(w) && sb->kursor>=0 && sb->kursor<=sb->len) {
			char sv; int cx;
			sv=sb->buf[sb->kursor]; sb->buf[sb->kursor]=0;
			cx=3+pg_font_lebar_teks(sb->font,sb->buf);
			sb->buf[sb->kursor]=sv;
			pg_garis_v_permukaan(s,cx,y-pg_font_ascent(sb->font),y+2,sb->fg);
		}
	}
}
static pg_bool peristiwa_v(pg_widget_t *w, const pg_peristiwa_t *e) {
	pg_searchbox_t *sb=d(w); int sw=w->kotak.w, bw=btn_w(), bl=sw-bw;
	if(e->tipe==PG_PERISTIWA_TETIK_TURUN && e->tetik_tombol==PG_TETIK_KIRI) {
		int x=e->tetik_pos.x;
		pg_widget_fokus(w);
		if(x>=bl && sb->len>0) { sb->clear_tekan=PG_BENAR; pg_widget_kotor(w); }
		return PG_BENAR;
	}
	if(e->tipe==PG_PERISTIWA_TETIK_NAIK && e->tetik_tombol==PG_TETIK_KIRI) {
		/* Only handle NAIK if searchbox has fokus or clear_tekan
		 * is active (was clicked). Without this, searchbox would
		 * greedily consume ALL NAIK events. */
		if(!pg_widget_punya_fokus(w) && !sb->clear_tekan)
			return PG_SALAH;
		if(sb->clear_tekan) {
			sb->clear_tekan=PG_SALAH;
			if(e->tetik_pos.x>=bl) { sb->len=0; sb->buf[0]=0; sb->kursor=0; fire(sb); }
			pg_widget_kotor(w);
		}
		return PG_BENAR;
	}
	if(e->tipe==PG_PERISTIWA_TETIK_GERAK) {
		pg_bool ch = (e->tetik_pos.x>=bl && sb->len>0) ? PG_BENAR : PG_SALAH;
		if(ch!=sb->clear_hover) { sb->clear_hover=ch; pg_widget_kotor(w); }
		return PG_BENAR;
	}
	/* Keyboard: hanya bila widget punya fokus global. Pakai
	 * pg_widget_punya_fokus (bukan field lokal) supaya bila fokus
	 * pindah ke widget lain, searchbox tidak lagi konsumsi
	 * keyboard event. */
	if(e->tipe!=PG_PERISTIWA_TOMBOL_TURUN) return PG_SALAH;
	if(!pg_widget_punya_fokus(w)) return PG_SALAH;
	if(e->tombol==PG_TOMBOL_BACKSPACE) {
		if(sb->kursor>0 && sb->len>0) { int i; for(i=sb->kursor-1;i<sb->len-1;i++) sb->buf[i]=sb->buf[i+1]; sb->len--; sb->kursor--; sb->buf[sb->len]=0; pg_widget_kotor(w); fire(sb); }
		return PG_BENAR;
	}
	{pg_u32 cp=e->unicode?e->unicode:(pg_u32)e->tombol; if(cp>=32 && cp<=126) {
		if(sb->len+1<sb->cap) { int i; for(i=sb->len;i>=sb->kursor;i--) sb->buf[i+1]=sb->buf[i]; sb->buf[sb->kursor]=(char)cp; sb->len++; sb->kursor++; sb->buf[sb->len]=0; pg_widget_kotor(w); fire(sb); }
		return PG_BENAR;
	}}
	if(e->tombol==PG_TOMBOL_KIRI) { if(sb->kursor>0){sb->kursor--;pg_widget_kotor(w);} return PG_BENAR; }
	if(e->tombol==PG_TOMBOL_KANAN) { if(sb->kursor<sb->len){sb->kursor++;pg_widget_kotor(w);} return PG_BENAR; }
	return PG_SALAH;
}
static void hancur_v(pg_widget_t *w) { pg_searchbox_t *sb=d(w); if(sb->buf)free(sb->buf); if(sb->placeholder)free(sb->placeholder); }
static void bebas_v(pg_widget_t *w) { free(w); }
static const pg_widget_vtable_t vt = { catat_v, peristiwa_v, NULL, hancur_v, NULL, NULL, bebas_v };
pg_searchbox_t *pg_buat_searchbox(const char *ph, int pm, pg_font_t *font) {
	pg_searchbox_t *sb; int cap;
	if(pm<=0) pm=64;
	cap=pm+1;
	sb=calloc(1,sizeof(*sb)); if(!sb) return NULL;
	pg_widget_init(&sb->base,PG_WIDGET_DASAR,&vt);
	sb->font=font; sb->fg=PG_HITAM; sb->batas=PG_ABU; sb->btn_bg=PG_ABU_TERANG;
	sb->ph_fg=PG_ABU; sb->cap=cap;
	sb->buf=calloc(cap,1); if(!sb->buf){free(sb);return NULL;}
	if(ph) { sb->placeholder=strdup(ph); }
	return sb;
}
void pg_searchbox_hancur(pg_searchbox_t *sb) { if(!sb)return; pg_widget_hancur(&sb->base); free(sb); }
const char *pg_searchbox_ambil_teks(pg_searchbox_t *sb) { return sb?(sb->buf?sb->buf:""):""; }
void pg_searchbox_setel_teks(pg_searchbox_t *sb, const char *t) { if(!sb)return; if(!t){sb->buf[0]=0;sb->len=0;sb->kursor=0;pg_widget_kotor(&sb->base);return;} { size_t n=strlen(t); if(n>=(size_t)sb->cap)n=sb->cap-1; memcpy(sb->buf,t,n); sb->buf[n]=0; sb->len=n; sb->kursor=n; pg_widget_kotor(&sb->base);} }
void pg_searchbox_saatberubah(pg_searchbox_t *sb, pg_searchbox_cb cb, void *ctx) { if(!sb)return; sb->cb=cb; sb->ctx=ctx; }
pg_widget_t *pg_searchbox_widget(pg_searchbox_t *sb) { return sb?&sb->base:NULL; }
