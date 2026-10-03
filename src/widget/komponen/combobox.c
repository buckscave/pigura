#include "pigura/combobox.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/peristiwa.h"
#include "pigura/widget.h"
#include <stdlib.h>
#include <string.h>
#define MAX_ITEMS 32
struct pg_combobox {
	pg_widget_t base;
	char *buf; int cap, len, kursor;
	char *items[MAX_ITEMS]; int n_items;
	int terpilih;
	pg_bool buka;
	int hover_idx;
	pg_warna_t fg, latar, batas, sel_bg, sel_fg, hover_bg;
	pg_font_t *font;
	pg_combobox_cb cb;
	void *ctx;
};
static pg_combobox_t *d(pg_widget_t *w) { return (pg_combobox_t*)w; }
static void fire(pg_combobox_t *cb) { if(cb->cb) cb->cb(cb, cb->terpilih, cb->ctx); }
static int btn_w(void) { return 16; }
static void catat_v(pg_widget_t *w, pg_permukaan_t *s) {
	pg_combobox_t *cb=d(w); int sw,sh,bw,bl,th,y;
	pg_kotak_t r;
	sw=pg_permukaan_lebar(s); sh=pg_permukaan_tinggi(s);
	pg_isi_permukaan(s, cb->latar);
	r=pg_buat_kotak(0,0,sw,sh); pg_gambar_kotak_aa(s,r,pg_widget_punya_fokus(w)?PG_BIRU:cb->batas);
	bw=btn_w(); bl=sw-bw;
	/* Tombol dropdown. */
	pg_isi_permukaan_kotak(s, pg_buat_kotak(bl,0,bw,sh), cb->batas);
	{ int cx=bl+bw/2, cy=sh/2;
	  pg_garis_h_permukaan(s,cx-4,cx+4,cy-1,cb->fg);
	  pg_garis_h_permukaan(s,cx-3,cx+3,cy,cb->fg);
	  pg_setel_piksel_permukaan(s,cx,cy+1,cb->fg); }
	if(cb->font) {
		th=pg_font_tinggi(cb->font); if(th<=0) th=8;
		y=pg_font_baseline_tengah(cb->font,sh);
		pg_font_gambar_teks(cb->font, cb->buf?cb->buf:"", s, 3, y, cb->fg);
		if(pg_widget_punya_fokus(w) && cb->kursor>=0 && cb->kursor<=cb->len) {
			char sv=cb->buf[cb->kursor]; cb->buf[cb->kursor]=0;
			int cx=3+pg_font_lebar_teks(cb->font,cb->buf);
			cb->buf[cb->kursor]=sv;
			pg_garis_v_permukaan(s,cx,y-pg_font_ascent(cb->font),y+2,cb->fg);
		}
	}
}
static void catat_popup_v(pg_widget_t *w, pg_permukaan_t *s) {
	pg_combobox_t *cb=d(w); int i,th,lh,ax,ay,bw,bh;
	if(!cb->buka || cb->n_items<=0) return;
	th=pg_font_tinggi(cb->font); if(th<=0) th=8; lh=th+2;
	bh=w->kotak.h;
	pg_widget_posisi_layar(w,&ax,&ay);
	bw=w->kotak.w;
	{
		pg_kotak_t pr=pg_buat_kotak(ax,ay+bh,bw,cb->n_items*lh);
		pg_isi_permukaan_kotak(s,pr,cb->latar);
		pg_kotak_permukaan(s,pr,cb->batas);
		for(i=0;i<cb->n_items;i++) {
			int iy=ay+bh+i*lh;
			pg_warna_t bg=cb->latar, fg=cb->fg;
			if(i==cb->terpilih) { bg=cb->sel_bg; fg=cb->sel_fg; }
			else if(i==cb->hover_idx) { bg=cb->hover_bg; }
			if(bg!=cb->latar) pg_isi_permukaan_kotak(s,pg_buat_kotak(ax+1,iy,bw-2,lh),bg);
			if(cb->items[i]&&cb->font)
				pg_font_gambar_teks(cb->font,cb->items[i],s,ax+4,iy+pg_font_baseline_tengah(cb->font,lh),fg);
		}
	}
}
static pg_bool peristiwa_v(pg_widget_t *w, const pg_peristiwa_t *e) {
	pg_combobox_t *cb=d(w); int sw=w->kotak.w, bw=btn_w(), bl=sw-bw;
	int bh = w->kotak.h;
	/* GERAK: hover tracking saat popup buka. */
	if(e->tipe==PG_PERISTIWA_TETIK_GERAK) {
		if(cb->buka && e->tetik_pos.y>=bh) {
			int th=pg_font_tinggi(cb->font); if(th<=0) th=8;
			int lh=th+2;
			int idx=(e->tetik_pos.y-bh)/lh;
			if(idx>=0 && idx<cb->n_items) { cb->hover_idx=idx; pg_widget_kotor(w); }
			return PG_BENAR;
		}
		return PG_SALAH;
	}
	if(e->tipe==PG_PERISTIWA_TETIK_TURUN && e->tetik_tombol==PG_TETIK_KIRI) {
		pg_widget_fokus(w);
		/* Klik di popup area (y >= bh)? Pilih item. */
		if(cb->buka && e->tetik_pos.y>=bh) {
			int th=pg_font_tinggi(cb->font); if(th<=0) th=8;
			int lh=th+2;
			int idx=(e->tetik_pos.y-bh)/lh;
			if(idx>=0 && idx<cb->n_items) {
				cb->terpilih=idx;
				if(cb->items[idx]) { strncpy(cb->buf,cb->items[idx],cb->cap-1); cb->buf[cb->cap-1]=0; cb->len=strlen(cb->buf); cb->kursor=cb->len; }
				if(cb->cb) cb->cb(cb, idx, cb->ctx);
			}
			cb->buka=PG_SALAH; cb->hover_idx=-1;
			pg_widget_kotor(w);
			return PG_BENAR;
		}
		/* Klik di button bar (atas). */
		if(e->tetik_pos.x>=bl) { cb->buka=!cb->buka; cb->hover_idx=-1; pg_widget_kotor(w); }
		else { cb->buka=PG_SALAH; pg_widget_kotor(w); }
		return PG_BENAR;
	}
	if(e->tipe!=PG_PERISTIWA_TOMBOL_TURUN) return PG_SALAH;
	/* Keyboard: hanya bila widget punya fokus global. */
	if(!pg_widget_punya_fokus(w)) return PG_SALAH;
	/* Escape: tutup popup. */
	if(e->tombol==PG_TOMBOL_ESCAPE && cb->buka) { cb->buka=PG_SALAH; pg_widget_kotor(w); return PG_BENAR; }
	if(e->tombol==PG_TOMBOL_BACKSPACE) { if(cb->kursor>0&&cb->len>0){int i;for(i=cb->kursor-1;i<cb->len-1;i++)cb->buf[i]=cb->buf[i+1];cb->len--;cb->kursor--;cb->buf[cb->len]=0;pg_widget_kotor(w);fire(cb);} return PG_BENAR; }
	{pg_u32 cp=e->unicode?e->unicode:(pg_u32)e->tombol; if(cp>=32 && cp<=126) { if(cb->len+1<cb->cap){int i;for(i=cb->len;i>=cb->kursor;i--)cb->buf[i+1]=cb->buf[i];cb->buf[cb->kursor]=(char)cp;cb->len++;cb->kursor++;cb->buf[cb->len]=0;pg_widget_kotor(w);fire(cb);} return PG_BENAR; } }
	if(e->tombol==PG_TOMBOL_KIRI) { if(cb->kursor>0){cb->kursor--;pg_widget_kotor(w);} return PG_BENAR; }
	if(e->tombol==PG_TOMBOL_KANAN) { if(cb->kursor<cb->len){cb->kursor++;pg_widget_kotor(w);} return PG_BENAR; }
	return PG_SALAH;
}
static void hancur_v(pg_widget_t *w) { pg_combobox_t *cb=d(w); int i; if(cb->buf)free(cb->buf); for(i=0;i<cb->n_items;i++) if(cb->items[i])free(cb->items[i]); }
static void bebas_v(pg_widget_t *w) { free(w); }
/* berisi_v: cek apakah titik ada di widget ATAU di popup area. */
static pg_bool berisi_v(pg_widget_t *w, pg_titik_t p) {
	pg_combobox_t *cb=d(w);
	if(pg_widget_berisi(w, p)) return PG_BENAR;
	if(!cb->buka || cb->n_items<=0) return PG_SALAH;
	{	int bh=w->kotak.h;
		int th=pg_font_tinggi(cb->font); if(th<=0) th=8;
		int lh=th+2;
		int pop_y0=w->kotak.y+bh;
		int pop_y1=pop_y0+cb->n_items*lh;
		if(p.x>=w->kotak.x && p.x<w->kotak.x+w->kotak.w &&
		   p.y>=pop_y0 && p.y<pop_y1)
			return PG_BENAR;
	}
	return PG_SALAH;
}
static const pg_widget_vtable_t vt = { catat_v, peristiwa_v, NULL, hancur_v, catat_popup_v, berisi_v, bebas_v };
pg_combobox_t *pg_buat_combobox(const char *awal, int pm, pg_font_t *font) {
	pg_combobox_t *cb; int cap;
	if(pm<=0) pm=64;
	cap=pm+1;
	cb=calloc(1,sizeof(*cb)); if(!cb) return NULL;
	pg_widget_init(&cb->base,PG_WIDGET_DASAR,&vt);
	cb->font=font; cb->fg=PG_HITAM; cb->latar=PG_PUTIH; cb->batas=PG_ABU;
	cb->sel_bg=PG_BIRU; cb->sel_fg=PG_PUTIH; cb->hover_bg=PG_ABU;
	cb->cap=cap; cb->terpilih=-1; cb->hover_idx=-1;
	cb->buf=calloc(cap,1); if(!cb->buf){free(cb);return NULL;}
	if(awal) { size_t n=strlen(awal); if(n>(size_t)pm)n=pm; memcpy(cb->buf,awal,n); cb->buf[n]=0; cb->len=n; cb->kursor=n; }
	return cb;
}
void pg_combobox_hancur(pg_combobox_t *cb) { if(!cb)return; pg_widget_hancur(&cb->base); free(cb); }
const char *pg_combobox_ambil_teks(pg_combobox_t *cb) { return cb?(cb->buf?cb->buf:""):""; }
void pg_combobox_setel_teks(pg_combobox_t *cb, const char *t) { if(!cb)return; if(!t){cb->buf[0]=0;cb->len=0;cb->kursor=0;pg_widget_kotor(&cb->base);return;} { size_t n=strlen(t); if(n>=(size_t)cb->cap)n=cb->cap-1; memcpy(cb->buf,t,n); cb->buf[n]=0; cb->len=n; cb->kursor=n; pg_widget_kotor(&cb->base);} }
void pg_combobox_tambah_item(pg_combobox_t *cb, const char *t) { if(!cb||!t||cb->n_items>=MAX_ITEMS)return; cb->items[cb->n_items]=strdup(t); cb->n_items++; }
void pg_combobox_saatberubah(pg_combobox_t *cb, pg_combobox_cb fn, void *ctx) { if(!cb)return; cb->cb=fn; cb->ctx=ctx; }
pg_widget_t *pg_combobox_widget(pg_combobox_t *cb) { return cb?&cb->base:NULL; }
pg_bool pg_combobox_terbuka(pg_combobox_t *cb) { return cb?cb->buka:PG_SALAH; }
void pg_combobox_tutup(pg_combobox_t *cb) { if(!cb||!cb->buka)return; cb->buka=PG_SALAH; cb->hover_idx=-1; pg_widget_kotor(&cb->base); }
