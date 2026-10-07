#include "pigura/tableview.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/aksi.h"
#include "pigura/widget.h"
#include <stdlib.h>
#include <string.h>
#define MAX_COLS 16
#define MAX_ROWS 256
typedef struct { char *judul; int lebar; } tv_col;
struct pg_tableview {
	pg_widget_t base;
	tv_col cols[MAX_COLS]; int n_cols;
	char *cells[MAX_ROWS][MAX_COLS]; int n_rows;
	int sel_row, sel_col, hover_row;
	pg_warna_t fg, latar, batas, sel_bg, sel_fg, hdr_bg, hdr_fg;
	pg_font_t *font;
	pg_tableview_cb cb; void *ctx;
};
static pg_tableview_t *d(pg_widget_t *w) { return (pg_tableview_t*)w; }
static int row_h(pg_tableview_t *tv) { return (tv->font?pg_font_tinggi(tv->font):8)+2; }
static void catat_v(pg_widget_t *w, pg_permukaan_t *s) {
	pg_tableview_t *tv=d(w); int sw,sh,rh,i,x,y,r;
	sw=pg_permukaan_lebar(s); sh=pg_permukaan_tinggi(s);
	pg_isi_permukaan(s, tv->latar);
	rh=row_h(tv);
	/* Header. */
	pg_isi_permukaan_kotak(s, pg_buat_kotak(0,0,sw,rh), tv->hdr_bg);
	for(i=0,x=0;i<tv->n_cols;i++) {
		int cw=tv->cols[i].lebar;
		pg_garis_v_permukaan(s,x,0,sh,tv->batas);
		if(tv->cols[i].judul&&tv->font)
			pg_font_gambar_teks(tv->font,tv->cols[i].judul,s,x+3,
				pg_font_baseline_tengah(tv->font,rh),tv->hdr_fg);
		x+=cw;
	}
	pg_garis_h_permukaan(s,0,sw,rh,tv->batas);
	/* Rows. */
	for(r=0,y=rh;r<tv->n_rows&&y<sh;r++,y+=rh) {
		if(r==tv->sel_row) pg_isi_permukaan_kotak(s,pg_buat_kotak(0,y,sw,rh),tv->sel_bg);
		else if(r==tv->hover_row) pg_isi_permukaan_kotak(s,pg_buat_kotak(0,y,sw,rh),PG_ABU);
		pg_garis_h_permukaan(s,0,sw,y+rh,tv->batas);
		for(i=0,x=0;i<tv->n_cols;i++) {
			if(tv->cells[r][i]&&tv->font)
				pg_font_gambar_teks(tv->font,tv->cells[r][i],s,x+3,
					y+pg_font_baseline_tengah(tv->font,rh),(r==tv->sel_row)?tv->sel_fg:tv->fg);
			x+=tv->cols[i].lebar;
		}
	}
}
static pg_bool peristiwa_v(pg_widget_t *w, const pg_aksi_t *e) {
	pg_tableview_t *tv=d(w); int rh=row_h(tv);
	if(e->tipe==PG_AKSI_TETIKUS_TEKAN && e->tetik_tombol==PG_TETIKUS_KIRI) {
		int row=(e->tetik_pos.y-rh)/rh;
		int x=e->tetik_pos.x, col=0, cx=0, i;
		for(i=0;i<tv->n_cols;i++) { if(x>=cx&&x<cx+tv->cols[i].lebar){col=i;break;} cx+=tv->cols[i].lebar; }
		if(row>=0&&row<tv->n_rows) { tv->sel_row=row; tv->sel_col=col; pg_widget_kotor(w); if(tv->cb)tv->cb(tv,row,col,tv->ctx); }
		return PG_BENAR;
	}
	if(e->tipe==PG_AKSI_TETIKUS_GERAK) {
		int row=(e->tetik_pos.y-rh)/rh;
		if(row>=0&&row<tv->n_rows) { if(row!=tv->hover_row){tv->hover_row=row;pg_widget_kotor(w);} }
		return PG_BENAR;
	}
	return PG_SALAH;
}
static void hancur_v(pg_widget_t *w) { pg_tableview_t *tv=d(w); int i,j; for(i=0;i<tv->n_rows;i++) for(j=0;j<tv->n_cols;j++) if(tv->cells[i][j])free(tv->cells[i][j]); for(i=0;i<tv->n_cols;i++) if(tv->cols[i].judul)free(tv->cols[i].judul); }
static void bebas_v(pg_widget_t *w) { free(w); }
static const pg_widget_vtable_t vt = { catat_v, peristiwa_v, NULL, hancur_v, NULL, NULL, bebas_v };
pg_tableview_t *pg_buat_tableview(pg_font_t *font) {
	pg_tableview_t *tv=calloc(1,sizeof(*tv)); if(!tv) return NULL;
	pg_widget_init(&tv->base,PG_WIDGET_DASAR,&vt);
	tv->font=font; tv->fg=PG_HITAM; tv->latar=PG_PUTIH; tv->batas=PG_ABU;
	tv->sel_bg=PG_BIRU; tv->sel_fg=PG_PUTIH; tv->hdr_bg=PG_ABU_GELAP; tv->hdr_fg=PG_PUTIH;
	tv->sel_row=-1; tv->hover_row=-1;
	return tv;
}
void pg_tableview_hancur(pg_tableview_t *tv) { if(!tv)return; pg_widget_hancur(&tv->base); free(tv); }
void pg_tableview_setel_kolom(pg_tableview_t *tv, int n, const char **judul) {
	int i; if(!tv||n<=0||n>MAX_COLS)return;
	tv->n_cols=n;
	for(i=0;i<n;i++) { tv->cols[i].judul=strdup(judul?judul[i]:""); tv->cols[i].lebar=80; }
}
void pg_tableview_tambah_baris(pg_tableview_t *tv, const char **sel) {
	int i; if(!tv||tv->n_rows>=MAX_ROWS||!sel)return;
	for(i=0;i<tv->n_cols;i++) tv->cells[tv->n_rows][i]=strdup(sel[i]?sel[i]:"");
	tv->n_rows++;
}
void pg_tableview_saatpilih(pg_tableview_t *tv, pg_tableview_cb cb, void *ctx) { if(!tv)return; tv->cb=cb; tv->ctx=ctx; }
pg_widget_t *pg_tableview_widget(pg_tableview_t *tv) { return tv?&tv->base:NULL; }
