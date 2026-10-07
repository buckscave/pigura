#include "pigura/contextmenu.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/aksi.h"
#include "pigura/widget.h"
#include <stdlib.h>
#include <string.h>
#define MAX_ITEMS 32
typedef struct { char *label; pg_contextmenu_cb cb; void *ctx; int is_sep; } cm_item;
struct pg_contextmenu {
	pg_widget_t base;
	cm_item items[MAX_ITEMS]; int n_items;
	int hover_idx, sel_idx;
	pg_warna_t fg, latar, batas, sel_bg, sel_fg, hover_bg;
	pg_font_t *font;
};
static pg_contextmenu_t *d(pg_widget_t *w) { return (pg_contextmenu_t*)w; }
static int item_h(pg_contextmenu_t *cm) { return (cm->font?pg_font_tinggi(cm->font):8)+4; }
static void catat_v(pg_widget_t *w, pg_permukaan_t *s) {
	pg_contextmenu_t *cm=d(w); int i,ih,y;
	if(!pg_widget_terlihat(w)) return;
	ih=item_h(cm);
	pg_isi_permukaan(s, cm->latar);
	pg_kotak_permukaan(s, pg_buat_kotak(0,0,w->kotak.w,w->kotak.h), cm->batas);
	for(i=0,y=0;i<cm->n_items;i++,y+=ih) {
		if(cm->items[i].is_sep) { pg_garis_h_permukaan(s,2,w->kotak.w-2,y+ih/2,cm->batas); continue; }
		if(i==cm->sel_idx) pg_isi_permukaan_kotak(s,pg_buat_kotak(1,y,w->kotak.w-2,ih),cm->sel_bg);
		else if(i==cm->hover_idx) pg_isi_permukaan_kotak(s,pg_buat_kotak(1,y,w->kotak.w-2,ih),cm->hover_bg);
		if(cm->items[i].label && cm->font)
			pg_font_gambar_teks(cm->font,cm->items[i].label,s,4,y+pg_font_baseline_tengah(cm->font,ih),
				(i==cm->sel_idx)?cm->sel_fg:cm->fg);
	}
}
static pg_bool peristiwa_v(pg_widget_t *w, const pg_aksi_t *e) {
	pg_contextmenu_t *cm=d(w); int ih=item_h(cm);
	if(e->tipe==PG_AKSI_TETIKUS_GERAK) {
		int idx=e->tetik_pos.y/ih;
		if(idx>=0&&idx<cm->n_items&&!cm->items[idx].is_sep) {
			if(idx!=cm->hover_idx) { cm->hover_idx=idx; pg_widget_kotor(w); }
		} else cm->hover_idx=-1;
		return PG_BENAR;
	}
	if(e->tipe==PG_AKSI_TETIKUS_TEKAN && e->tetik_tombol==PG_TETIKUS_KIRI) {
		int idx=e->tetik_pos.y/ih;
		if(idx>=0&&idx<cm->n_items&&!cm->items[idx].is_sep) {
			cm->sel_idx=idx;
			if(cm->items[idx].cb) cm->items[idx].cb(cm, idx, cm->items[idx].ctx);
			pg_widget_sembunyi(w); pg_widget_kotor(w);
		}
		return PG_BENAR;
	}
	return PG_SALAH;
}
static void hancur_v(pg_widget_t *w) { pg_contextmenu_t *cm=d(w); int i; for(i=0;i<cm->n_items;i++) if(cm->items[i].label)free(cm->items[i].label); }
static void bebas_v(pg_widget_t *w) { free(w); }
static const pg_widget_vtable_t vt = { catat_v, peristiwa_v, NULL, hancur_v, NULL, NULL, bebas_v };
pg_contextmenu_t *pg_buat_contextmenu(pg_font_t *font) {
	pg_contextmenu_t *cm=calloc(1,sizeof(*cm)); if(!cm) return NULL;
	pg_widget_init(&cm->base,PG_WIDGET_DASAR,&vt);
	cm->font=font; cm->fg=PG_HITAM; cm->latar=PG_PUTIH; cm->batas=PG_ABU_GELAP;
	cm->sel_bg=PG_BIRU; cm->sel_fg=PG_PUTIH; cm->hover_bg=PG_ABU;
	pg_widget_setel_latar(&cm->base, cm->latar);
	pg_widget_sembunyi(&cm->base);
	return cm;
}
void pg_contextmenu_hancur(pg_contextmenu_t *cm) { if(!cm)return; pg_widget_hancur(&cm->base); free(cm); }
void pg_contextmenu_tambah_item(pg_contextmenu_t *cm, const char *label, pg_contextmenu_cb cb, void *ctx) {
	if(!cm||!label||cm->n_items>=MAX_ITEMS)return;
	cm->items[cm->n_items].label=strdup(label);
	cm->items[cm->n_items].cb=cb; cm->items[cm->n_items].ctx=ctx;
	cm->items[cm->n_items].is_sep=0; cm->n_items++;
	{ int ih=item_h(cm); pg_widget_ubah_ukuran(&cm->base,120,cm->n_items*ih); }
}
void pg_contextmenu_tambah_pemisah(pg_contextmenu_t *cm) {
	if(!cm||cm->n_items>=MAX_ITEMS)return;
	cm->items[cm->n_items].label=NULL; cm->items[cm->n_items].cb=NULL;
	cm->items[cm->n_items].ctx=NULL; cm->items[cm->n_items].is_sep=1; cm->n_items++;
	{ int ih=item_h(cm); pg_widget_ubah_ukuran(&cm->base,120,cm->n_items*ih); }
}
void pg_contextmenu_tampil(pg_contextmenu_t *cm, int x, int y) { if(!cm)return; pg_widget_pindah(&cm->base,x,y); pg_widget_tampil(&cm->base); cm->hover_idx=-1; pg_widget_kotor(&cm->base); }
void pg_contextmenu_sembunyi(pg_contextmenu_t *cm) { if(!cm)return; pg_widget_sembunyi(&cm->base); }
pg_bool pg_contextmenu_terlihat(pg_contextmenu_t *cm) { return cm?pg_widget_terlihat(&cm->base):PG_SALAH; }
pg_widget_t *pg_contextmenu_widget(pg_contextmenu_t *cm) { return cm?&cm->base:NULL; }
