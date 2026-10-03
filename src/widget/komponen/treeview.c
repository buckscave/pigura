#include "pigura/treeview.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/peristiwa.h"
#include "pigura/widget.h"
#include <stdlib.h>
#include <string.h>
#define MAX_NODES 128
typedef struct { char *label; int parent; int expanded; int depth; } tv_node;
struct pg_treeview {
	pg_widget_t base;
	tv_node nodes[MAX_NODES]; int n_nodes;
	int sel_idx, hover_idx;
	pg_warna_t fg, latar, batas, sel_bg, sel_fg;
	pg_font_t *font;
	pg_treeview_cb cb; void *ctx;
	int gulir_y;
};
static pg_treeview_t *d(pg_widget_t *w) { return (pg_treeview_t*)w; }
static int node_h(pg_treeview_t *tv) { return (tv->font?pg_font_tinggi(tv->font):8)+2; }
static int visible_count(pg_treeview_t *tv) {
	int i,count=0;
	for(i=0;i<tv->n_nodes;i++) {
		int p=tv->nodes[i].parent, visible=1;
		while(p>=0) { if(!tv->nodes[p].expanded){visible=0;break;} p=tv->nodes[p].parent; }
		if(visible) count++;
	}
	return count;
}
static void catat_v(pg_widget_t *w, pg_permukaan_t *s) {
	pg_treeview_t *tv=d(w); int sw,sh,ih,i,y=0;
	sw=pg_permukaan_lebar(s); sh=pg_permukaan_tinggi(s);
	pg_isi_permukaan(s, tv->latar);
	pg_kotak_permukaan(s, pg_buat_kotak(0,0,sw,sh), tv->batas);
	ih=node_h(tv);
	for(i=0;i<tv->n_nodes&&y<sh;i++) {
		int p=tv->nodes[i].parent, visible=1;
		while(p>=0){if(!tv->nodes[p].expanded){visible=0;break;}p=tv->nodes[p].parent;}
		if(!visible) continue;
		{
			int x=4+tv->nodes[i].depth*12;
			if(i==tv->sel_idx) pg_isi_permukaan_kotak(s,pg_buat_kotak(0,y,sw,ih),tv->sel_bg);
			else if(i==tv->hover_idx) pg_isi_permukaan_kotak(s,pg_buat_kotak(0,y,sw,ih),PG_ABU);
			if(tv->font) {
				if(tv->nodes[i].label) {
					int has_child=0,j;
					for(j=i+1;j<tv->n_nodes;j++) if(tv->nodes[j].parent==i){has_child=1;break;}
					if(has_child) pg_font_gambar_teks(tv->font, tv->nodes[i].expanded?"-":"+", s, x, y+pg_font_baseline_tengah(tv->font,ih), tv->fg);
					pg_font_gambar_teks(tv->font, tv->nodes[i].label, s, x+12, y+pg_font_baseline_tengah(tv->font,ih), (i==tv->sel_idx)?tv->sel_fg:tv->fg);
				}
			}
		}
		y+=ih;
	}
}
static pg_bool peristiwa_v(pg_widget_t *w, const pg_peristiwa_t *e) {
	pg_treeview_t *tv=d(w); int ih=node_h(tv);
	if(e->tipe==PG_PERISTIWA_TETIK_TURUN && e->tetik_tombol==PG_TETIK_KIRI) {
		int idx=e->tetik_pos.y/ih, i, count=0;
		for(i=0;i<tv->n_nodes;i++) {
			int p=tv->nodes[i].parent, vis=1;
			while(p>=0){if(!tv->nodes[p].expanded){vis=0;break;}p=tv->nodes[p].parent;}
			if(!vis) continue;
			if(count==idx) {
				int x=e->tetik_pos.x;
				int has_child=0,j;
				for(j=i+1;j<tv->n_nodes;j++) if(tv->nodes[j].parent==i){has_child=1;break;}
				if(has_child && x < 4+tv->nodes[i].depth*12+12) { tv->nodes[i].expanded=!tv->nodes[i].expanded; }
				else { tv->sel_idx=i; if(tv->cb) tv->cb(tv, i, tv->ctx); }
				pg_widget_kotor(w); return PG_BENAR;
			}
			count++;
		}
		return PG_BENAR;
	}
	if(e->tipe==PG_PERISTIWA_TETIK_GERAK) {
		int idx=e->tetik_pos.y/ih, i, count=0, new_hover=-1;
		for(i=0;i<tv->n_nodes;i++) {
			int p=tv->nodes[i].parent, vis=1;
			while(p>=0){if(!tv->nodes[p].expanded){vis=0;break;}p=tv->nodes[p].parent;}
			if(!vis) continue;
			if(count==idx){new_hover=i;break;}
			count++;
		}
		if(new_hover!=tv->hover_idx){tv->hover_idx=new_hover;pg_widget_kotor(w);}
		return PG_BENAR;
	}
	if(e->tipe==PG_PERISTIWA_TETIK_RODA) { tv->gulir_y+=e->roda_dy*ih; pg_widget_kotor(w); return PG_BENAR; }
	return PG_SALAH;
}
static void hancur_v(pg_widget_t *w) { pg_treeview_t *tv=d(w); int i; for(i=0;i<tv->n_nodes;i++) if(tv->nodes[i].label)free(tv->nodes[i].label); }
static void bebas_v(pg_widget_t *w) { free(w); }
static const pg_widget_vtable_t vt = { catat_v, peristiwa_v, NULL, hancur_v, NULL, NULL, bebas_v };
pg_treeview_t *pg_buat_treeview(pg_font_t *font) {
	pg_treeview_t *tv=calloc(1,sizeof(*tv)); if(!tv) return NULL;
	pg_widget_init(&tv->base,PG_WIDGET_DASAR,&vt);
	tv->font=font; tv->fg=PG_HITAM; tv->latar=PG_PUTIH; tv->batas=PG_ABU;
	tv->sel_bg=PG_BIRU; tv->sel_fg=PG_PUTIH; tv->sel_idx=-1; tv->hover_idx=-1;
	return tv;
}
void pg_treeview_hancur(pg_treeview_t *tv) { if(!tv)return; pg_widget_hancur(&tv->base); free(tv); }
int pg_treeview_tambah_node(pg_treeview_t *tv, int parent, const char *label) {
	if(!tv||tv->n_nodes>=MAX_NODES||!label) return -1;
	tv->nodes[tv->n_nodes].label=strdup(label);
	tv->nodes[tv->n_nodes].parent=parent;
	tv->nodes[tv->n_nodes].expanded=(parent<0)?1:0;
	tv->nodes[tv->n_nodes].depth=(parent<0)?0:tv->nodes[parent].depth+1;
	tv->n_nodes++; return tv->n_nodes-1;
}
void pg_treeview_saatpilih(pg_treeview_t *tv, pg_treeview_cb cb, void *ctx) { if(!tv)return; tv->cb=cb; tv->ctx=ctx; }
pg_widget_t *pg_treeview_widget(pg_treeview_t *tv) { return tv?&tv->base:NULL; }
