#include <stdio.h>
#include "pigura/spinbox.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/peristiwa.h"
#include "pigura/widget.h"
#include <stdlib.h>
#include <string.h>
struct pg_spinbox {
	pg_widget_t base;
	int min, maks, nilai, langkah;
	pg_warna_t fg, latar, batas, btn_bg;
	pg_font_t *font;
	pg_spinbox_cb cb;
	void *ctx;
	pg_bool naik_hover, turun_hover, naik_tekan, turun_tekan;
};
static pg_spinbox_t *d(pg_widget_t *w) { return (pg_spinbox_t*)w; }
static void kotor(pg_spinbox_t *sb) { pg_widget_kotor(&sb->base); }
static void fire(pg_spinbox_t *sb) { if(sb->cb) sb->cb(sb, sb->nilai, sb->ctx); }
static int btn_w(pg_spinbox_t *sb) { (void)sb; return 20; }
static void naik(pg_spinbox_t *sb) {
	if(sb->nilai + sb->langkah <= sb->maks) { sb->nilai += sb->langkah; kotor(sb); fire(sb); }
}
static void turun(pg_spinbox_t *sb) {
	if(sb->nilai - sb->langkah >= sb->min) { sb->nilai -= sb->langkah; kotor(sb); fire(sb); }
}
static void catat_v(pg_widget_t *w, pg_permukaan_t *s) {
	pg_spinbox_t *sb=d(w); int sw,sh,th,bw,bl;
	pg_kotak_t r; char buf[32];
	sw=pg_permukaan_lebar(s); sh=pg_permukaan_tinggi(s);
	bw=btn_w(sb); bl=sw-bw; /* tombol di KANAN (sama dengan peristiwa_v) */
	pg_isi_permukaan(s, PG_PUTIH);
	r=pg_buat_kotak(0,0,sw,sh); pg_gambar_kotak_aa(s,r,sb->batas);
	/* Tombol naik (atas) + turun (bawah). */
	pg_isi_permukaan_kotak(s, pg_buat_kotak(bl,0,bw,sh/2),
		sb->naik_tekan?PG_ABU:(sb->naik_hover?PG_ABU_TERANG:sb->btn_bg));
	pg_isi_permukaan_kotak(s, pg_buat_kotak(bl,sh/2,bw,sh-sh/2),
		sb->turun_tekan?PG_ABU:(sb->turun_hover?PG_ABU_TERANG:sb->btn_bg));
	/* Divider horizontal HANYA di area tombol (bukan full width). */
	pg_garis_h_permukaan(s, bl, sw, sh/2, sb->batas);
	/* Divider vertikal antara area nilai dan tombol. */
	pg_garis_v_permukaan(s, bl, 0, sh, sb->batas);
	/* Panah atas (▲) — tip di atas, base lebar di bawah. */
	{ int cx=bl+bw/2, cy=sh/4;
	  pg_setel_piksel_permukaan(s,cx,cy-2,sb->fg);
	  pg_garis_h_permukaan(s,cx-2,cx+2,cy-1,sb->fg);
	  pg_garis_h_permukaan(s,cx-3,cx+3,cy,sb->fg); }
	/* Panah bawah (▼) — base lebar di atas, tip di bawah. */
	{ int cx=bl+bw/2, cy=sh*3/4;
	  pg_garis_h_permukaan(s,cx-3,cx+3,cy,sb->fg);
	  pg_garis_h_permukaan(s,cx-2,cx+2,cy+1,sb->fg);
	  pg_setel_piksel_permukaan(s,cx,cy+2,sb->fg); }
	/* Nilai. */
	if(sb->font) {
		th=pg_font_tinggi(sb->font); if(th<=0) th=8;
		snprintf(buf,sizeof(buf),"%d",sb->nilai);
		pg_font_gambar_teks(sb->font,buf,s,4,
			pg_font_baseline_tengah(sb->font,sh),sb->fg);
	}
}
static pg_bool peristiwa_v(pg_widget_t *w, const pg_peristiwa_t *e) {
	pg_spinbox_t *sb=d(w); int sw=w->kotak.w, sh=w->kotak.h, bw=btn_w(sb), bl=sw-bw;
	if(e->tipe==PG_PERISTIWA_TETIK_TURUN && e->tetik_tombol==PG_TETIK_KIRI) {
		int x=e->tetik_pos.x, y=e->tetik_pos.y;
		if(x>=bl) {
			if(y<sh/2) { sb->naik_tekan=PG_BENAR; naik(sb); }
			else { sb->turun_tekan=PG_BENAR; turun(sb); }
			kotor(sb); return PG_BENAR;
		}
		pg_widget_fokus(w); return PG_BENAR;
	}
	if(e->tipe==PG_PERISTIWA_TETIK_NAIK && e->tetik_tombol==PG_TETIK_KIRI) {
		sb->naik_tekan=PG_SALAH; sb->turun_tekan=PG_SALAH; kotor(sb); return PG_BENAR;
	}
	if(e->tipe==PG_PERISTIWA_TETIK_GERAK) {
		int x=e->tetik_pos.x, y=e->tetik_pos.y;
		pg_bool nh=PG_SALAH, th=PG_SALAH;
		if(x>=bl) { if(y<sh/2) nh=PG_BENAR; else th=PG_BENAR; }
		if(nh!=sb->naik_hover||th!=sb->turun_hover) {
			sb->naik_hover=nh; sb->turun_hover=th; kotor(sb);
		}
		return PG_BENAR;
	}
	if(e->tipe==PG_PERISTIWA_TOMBOL_TURUN && pg_widget_punya_fokus(w)) {
		if(e->tombol==PG_TOMBOL_ATAS) { naik(sb); return PG_BENAR; }
		if(e->tombol==PG_TOMBOL_BAWAH) { turun(sb); return PG_BENAR; }
	}
	return PG_SALAH;
}
static void hancur_v(pg_widget_t *w) {}
static void bebas_v(pg_widget_t *w) { free(w); }
static const pg_widget_vtable_t vt = { catat_v, peristiwa_v, NULL, hancur_v, NULL, NULL, bebas_v };
pg_spinbox_t *pg_buat_spinbox(int min, int maks, int nilai, int langkah, pg_font_t *font) {
	pg_spinbox_t *sb=calloc(1,sizeof(*sb)); if(!sb) return NULL;
	pg_widget_init(&sb->base,PG_WIDGET_DASAR,&vt);
	sb->font=font; sb->min=min; sb->maks=maks; sb->nilai=nilai; sb->langkah=langkah;
	sb->fg=PG_HITAM; sb->latar=PG_PUTIH; sb->batas=PG_ABU; sb->btn_bg=PG_ABU_TERANG;
	return sb;
}
void pg_spinbox_hancur(pg_spinbox_t *sb) { if(!sb)return; pg_widget_hancur(&sb->base); free(sb); }
int pg_spinbox_ambil_nilai(pg_spinbox_t *sb) { return sb?sb->nilai:0; }
void pg_spinbox_setel_nilai(pg_spinbox_t *sb, int v) { if(!sb)return; if(v<sb->min)v=sb->min; if(v>sb->maks)v=sb->maks; sb->nilai=v; kotor(sb); }
void pg_spinbox_saatberubah(pg_spinbox_t *sb, pg_spinbox_cb cb, void *ctx) { if(!sb)return; sb->cb=cb; sb->ctx=ctx; }
pg_widget_t *pg_spinbox_widget(pg_spinbox_t *sb) { return sb?&sb->base:NULL; }
