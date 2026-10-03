/* textedit.c - multiline text input */
#include "pigura/textedit.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/peristiwa.h"
#include "pigura/widget.h"
#include <stdlib.h>
#include <string.h>

struct pg_textedit {
        pg_widget_t base;
        char *buf;
        int cap, len, kursor;
        int sel_mulai, sel_akhir;
        pg_bool menyeret;
        pg_warna_t fg, batas, sel_bg;
        pg_font_t *font;
        pg_textedit_cb cb;
        void *ctx;
};

static pg_textedit_t *d(pg_widget_t *w) { return (pg_textedit_t*)w; }
static int smin(pg_textedit_t *t) { return t->sel_mulai<0 ? t->kursor : (t->sel_mulai<t->sel_akhir?t->sel_mulai:t->sel_akhir); }
static int smax(pg_textedit_t *t) { return t->sel_mulai<0 ? t->kursor : (t->sel_mulai>t->sel_akhir?t->sel_mulai:t->sel_akhir); }
static int hsel(pg_textedit_t *t) { return t->sel_mulai>=0 && t->sel_mulai!=t->sel_akhir; }
static void csel(pg_textedit_t *t) { t->sel_mulai=-1; t->sel_akhir=-1; t->menyeret=PG_SALAH; }
static void dsel(pg_textedit_t *t) { int lo=smin(t),hi=smax(t),i; for(i=lo;i<t->len-(hi-lo);i++) t->buf[i]=t->buf[i+(hi-lo)]; t->len-=(hi-lo); t->buf[t->len]=0; t->kursor=lo; csel(t); }
static void fcb(pg_textedit_t *t) { if(t->cb) t->cb(t,t->ctx); }

static void catat_v(pg_widget_t *w, pg_permukaan_t *s) {
        pg_textedit_t *t=d(w);
        int sw,sh,th,lh,y,i,ls,asc;
        pg_kotak_t r;
        sw=pg_permukaan_lebar(s); sh=pg_permukaan_tinggi(s);
        pg_isi_permukaan(s,PG_PUTIH);
        r=pg_buat_kotak(0,0,sw,sh);
        pg_gambar_kotak_aa(s,r,pg_widget_punya_fokus(w)?PG_BIRU:t->batas);
        if(!t->font) return;
        th=pg_font_tinggi(t->font); if(th<=0) th=8;
        lh=th+1; asc=pg_font_ascent(t->font); y=3+asc;
        ls=0;
        /* Selection highlight. */
        if(hsel(t)){
                int lo=smin(t),hi=smax(t);
                int sy=3+asc;
                int lstart=0;
                int k;
                for(k=0;k<=t->len&&sy<sh;k++){
                        if(k==t->len||t->buf[k]==10||t->buf[k]==0){
                                if(lstart<hi&&k>lo){
                                        int s0=(lo>lstart?lo:lstart);
                                        int e0=(hi<k?hi:k);
                                        int x0=3,x1=3;
                                        if(s0>lstart){char sv=t->buf[s0];t->buf[s0]=0;x0=3+pg_font_lebar_teks(t->font,t->buf+lstart);t->buf[s0]=sv;}
                                        else x0=3;
                                        if(e0<k){char sv=t->buf[e0];t->buf[e0]=0;x1=3+pg_font_lebar_teks(t->font,t->buf+lstart);t->buf[e0]=sv;}
                                        else{char sv=t->buf[k];t->buf[k]=0;x1=3+pg_font_lebar_teks(t->font,t->buf+lstart);t->buf[k]=sv;}
                                        pg_isi_permukaan_kotak(s,pg_buat_kotak(x0,sy-asc,x1-x0,th+1),t->sel_bg);
                                }
                                sy+=lh;
                                lstart=k+1;
                        }
                }
        }
        for(i=0;i<=t->len&&y<sh;i++){
                if(i==t->len||t->buf[i]=='\n'||t->buf[i]==0){
                        int ll=i-ls;
                        if(ll>0){
                                char tmp[512];
                                if(ll<512){memcpy(tmp,t->buf+ls,ll);tmp[ll]=0;
                                pg_font_gambar_teks(t->font,tmp,s,3,y,t->fg);}
                        }
                        y+=lh; ls=i+1;
                }
        }
        if(pg_widget_punya_fokus(w)&&t->kursor>=0&&t->kursor<=t->len){
                int b=0,k=0,j,cx,cy,ty;
                for(j=0;j<t->kursor;j++){if(t->buf[j]=='\n'){b++;k=0;}else k++;}
                cy=3+b*lh+asc;
                {
                        int lso=t->kursor-k;
                        char tmp[512];
                        if(k>0&&k<512){memcpy(tmp,t->buf+lso,k);tmp[k]=0;
                        cx=3+pg_font_lebar_teks(t->font,tmp);}
                        else cx=3;
                }
                ty=cy-asc; if(ty<1) ty=1;
                pg_garis_v_permukaan(s,cx,ty,cy+2,t->fg);
        }
}

static int klik_kursor(pg_textedit_t *t,int x,int y){
        int th,lh,b,i,ls,kol,best,bd;
        if(!t->buf||t->len<=0) return 0;
        th=pg_font_tinggi(t->font); if(th<=0) th=8;
        lh=th+1;
        b=(y-3)/lh; if(b<0) b=0;
        ls=0;
        for(i=0;i<b&&ls<t->len;i++){while(ls<t->len&&t->buf[ls]!='\n')ls++; if(ls<t->len)ls++;}
        x-=3; if(x<=0) return ls;
        best=0; bd=99999;
        for(kol=0;;kol++){
                int pos=ls+kol,w,dist;
                if(pos>t->len) break;
                if(pos<t->len&&t->buf[pos]=='\n') break;
                {
                        char sv=0;
                        if(pos<t->len){sv=t->buf[pos];t->buf[pos]=0;}
                        w=pg_font_lebar_teks(t->font,t->buf+ls);
                        if(pos<t->len) t->buf[pos]=sv;
                }
                dist=x-w; if(dist<0) dist=-dist;
                if(dist<bd){bd=dist;best=kol;}
                if(pos>=t->len) break;
        }
        return ls+best;
}

static pg_bool peristiwa_v(pg_widget_t *w, const pg_peristiwa_t *e) {
        pg_textedit_t *t=d(w);
        if(e->tipe==PG_PERISTIWA_TETIK_TURUN&&e->tetik_tombol==PG_TETIK_KIRI){
                int pos=klik_kursor(t,e->tetik_pos.x,e->tetik_pos.y);
                if(!pg_widget_punya_fokus(w)) pg_widget_fokus(w);
                if(e->modifier&PG_MOD_SHIFT){if(t->sel_mulai<0)t->sel_mulai=t->kursor;t->sel_akhir=pos;}
                else{t->sel_mulai=pos;t->sel_akhir=pos;t->menyeret=PG_BENAR;}
                t->kursor=pos;
                pg_widget_kotor(w);
                return PG_BENAR;
        }
        /* GERAK: drag selection. */
        if(e->tipe==PG_PERISTIWA_TETIK_GERAK && t->menyeret && t->sel_mulai>=0){
                int pos=klik_kursor(t,e->tetik_pos.x,e->tetik_pos.y);
                t->sel_akhir=pos; t->kursor=pos;
                pg_widget_kotor(w); return PG_BENAR;
        }
        /* NAIK: end drag. Only handle if TE has fokus or is dragging.
         * Without this check, TE greedily consumes ALL NAIK events,
         * preventing other widgets (SearchBox clear button, etc.)
         * from receiving NAIK via pg_kotak. */
        if(e->tipe==PG_PERISTIWA_TETIK_NAIK && e->tetik_tombol==PG_TETIK_KIRI){
                if(!pg_widget_punya_fokus(w) && !t->menyeret)
                        return PG_SALAH;
                t->menyeret=PG_SALAH;
                if(t->sel_mulai==t->sel_akhir) csel(t);
                return PG_BENAR;
        }
        if(e->tipe!=PG_PERISTIWA_TOMBOL_TURUN) return PG_SALAH;
        if(!pg_widget_punya_fokus(w)) return PG_SALAH;
        if((e->modifier&PG_MOD_CTRL)&&(e->tombol=='a'||e->tombol=='A')){t->sel_mulai=0;t->sel_akhir=t->len;t->kursor=t->len;pg_widget_kotor(w);return PG_BENAR;}
        if((e->modifier&PG_MOD_CTRL)&&(e->tombol=='c'||e->tombol=='C')) return PG_BENAR;
        if((e->modifier&PG_MOD_CTRL)&&(e->tombol=='x'||e->tombol=='X')){if(hsel(t))dsel(t);pg_widget_kotor(w);fcb(t);return PG_BENAR;}
        if((e->modifier&PG_MOD_CTRL)&&(e->tombol=='v'||e->tombol=='V')) return PG_BENAR;
        if(e->tombol==PG_TOMBOL_BACKSPACE){
                if(hsel(t)) dsel(t);
                else if(t->kursor>0&&t->len>0){int i;for(i=t->kursor-1;i<t->len-1;i++)t->buf[i]=t->buf[i+1];t->len--;t->kursor--;t->buf[t->len]=0;}
                pg_widget_kotor(w);fcb(t);return PG_BENAR;
        }
        if(e->tombol==PG_TOMBOL_DELETE){
                if(hsel(t)) dsel(t);
                else if(t->kursor<t->len){int i;for(i=t->kursor;i<t->len-1;i++)t->buf[i]=t->buf[i+1];t->len--;t->buf[t->len]=0;}
                pg_widget_kotor(w);fcb(t);return PG_BENAR;
        }
        if(e->tombol==PG_TOMBOL_HOME){
                if(e->modifier&PG_MOD_SHIFT){if(t->sel_mulai<0)t->sel_mulai=t->kursor;while(t->kursor>0&&t->buf[t->kursor-1]!='\n')t->kursor--;t->sel_akhir=t->kursor;}
                else{csel(t);while(t->kursor>0&&t->buf[t->kursor-1]!='\n')t->kursor--;}
                pg_widget_kotor(w);return PG_BENAR;
        }
        if(e->tombol==PG_TOMBOL_END){
                if(e->modifier&PG_MOD_SHIFT){if(t->sel_mulai<0)t->sel_mulai=t->kursor;while(t->kursor<t->len&&t->buf[t->kursor]!='\n')t->kursor++;t->sel_akhir=t->kursor;}
                else{csel(t);while(t->kursor<t->len&&t->buf[t->kursor]!='\n')t->kursor++;}
                pg_widget_kotor(w);return PG_BENAR;
        }
        if(e->tombol==PG_TOMBOL_KIRI){
                if(e->modifier&PG_MOD_SHIFT){if(t->sel_mulai<0)t->sel_mulai=t->kursor;if(t->kursor>0)t->kursor--;t->sel_akhir=t->kursor;}
                else{if(hsel(t)){t->kursor=smin(t);csel(t);}else if(t->kursor>0)t->kursor--;}
                pg_widget_kotor(w);return PG_BENAR;
        }
        if(e->tombol==PG_TOMBOL_KANAN){
                if(e->modifier&PG_MOD_SHIFT){if(t->sel_mulai<0)t->sel_mulai=t->kursor;if(t->kursor<t->len)t->kursor++;t->sel_akhir=t->kursor;}
                else{if(hsel(t)){t->kursor=smax(t);csel(t);}else if(t->kursor<t->len)t->kursor++;}
                pg_widget_kotor(w);return PG_BENAR;
        }
        /* Arrow UP: move to previous line same column. */
        if(e->tombol==PG_TOMBOL_ATAS){
                int b,k,j;
                if(e->modifier&PG_MOD_SHIFT){if(t->sel_mulai<0)t->sel_mulai=t->kursor;}
                /* Hitung baris+kolom saat ini. */
                b=0;k=0;
                for(j=0;j<t->kursor;j++){if(t->buf[j]==10){b++;k=0;}else k++;}
                if(b>0){
                        /* Cari awal baris sebelumnya. */
                        int target_b=b-1;
                        int ls=0,cb=0;
                        for(j=0;j<t->len;j++){
                                if(cb==target_b)break;
                                if(t->buf[j]==10){cb++;ls=j+1;}
                        }
                        /* Hitung akhir baris target. */
                        int le=t->len;
                        for(j=ls;j<t->len;j++){if(t->buf[j]==10){le=j;break;}}
                        /* Pindah kursor ke kolom yang sama (atau akhir baris). */
                        t->kursor=ls+(k<(le-ls)?k:(le-ls));
                }
                if(e->modifier&PG_MOD_SHIFT)t->sel_akhir=t->kursor;
                else csel(t);
                pg_widget_kotor(w);return PG_BENAR;
        }
        /* Arrow DOWN: move to next line same column. */
        if(e->tombol==PG_TOMBOL_BAWAH){
                int b,k,j;
                if(e->modifier&PG_MOD_SHIFT){if(t->sel_mulai<0)t->sel_mulai=t->kursor;}
                b=0;k=0;
                for(j=0;j<t->kursor;j++){if(t->buf[j]==10){b++;k=0;}else k++;}
                /* Cari awal baris berikutnya. */
                int ls=-1,cb=0;
                for(j=0;j<t->len;j++){
                        if(t->buf[j]==10){cb++;if(cb==b+1){ls=j+1;break;}}
                }
                if(ls>=0&&ls<=t->len){
                        int le=t->len;
                        for(j=ls;j<t->len;j++){if(t->buf[j]==10){le=j;break;}}
                        t->kursor=ls+(k<(le-ls)?k:(le-ls));
                }
                if(e->modifier&PG_MOD_SHIFT)t->sel_akhir=t->kursor;
                else csel(t);
                pg_widget_kotor(w);return PG_BENAR;
        }
        if(e->tombol==PG_TOMBOL_ENTER){
                if(hsel(t))dsel(t);
                if(t->len+1<t->cap){int i;for(i=t->len;i>=t->kursor;i--)t->buf[i+1]=t->buf[i];t->buf[t->kursor]='\n';t->len++;t->kursor++;t->buf[t->len]=0;pg_widget_kotor(w);fcb(t);}
                return PG_BENAR;
        }
        {pg_u32 cp=e->unicode?e->unicode:(pg_u32)e->tombol; if(cp>=32&&cp<=126){
                if(hsel(t))dsel(t);
                if(t->len+1<t->cap){int i;for(i=t->len;i>=t->kursor;i--)t->buf[i+1]=t->buf[i];t->buf[t->kursor]=(char)cp;t->len++;t->kursor++;t->buf[t->len]=0;pg_widget_kotor(w);fcb(t);}
                return PG_BENAR;
        }}
        return PG_SALAH;
}

static void hancur_v(pg_widget_t *w) { pg_textedit_t *t=d(w); if(t->buf){free(t->buf);t->buf=NULL;} }
static void bebas_v(pg_widget_t *w) { free(w); }

static const pg_widget_vtable_t vt = {
        catat_v, peristiwa_v, NULL, hancur_v, NULL, NULL, bebas_v
};

pg_textedit_t *pg_textedit_buat(const char *awal, int pm, pg_font_t *font) {
        pg_textedit_t *t; int cap;
        if(pm<=0) pm=1024;
        cap=pm+1;
        t=(pg_textedit_t*)calloc(1,sizeof(*t));
        if(!t) return NULL;
        pg_widget_init(&t->base,PG_WIDGET_ISIAN_TEK,&vt);
        t->font=font; t->fg=PG_HITAM; t->batas=PG_ABU; t->sel_bg=PG_BIRU;
        t->cap=cap; t->sel_mulai=-1; t->sel_akhir=-1;
        t->buf=(char*)calloc(cap,1);
        if(!t->buf){free(t);return NULL;}
        if(awal){size_t n=strlen(awal);if(n>(size_t)pm)n=pm;memcpy(t->buf,awal,n);t->buf[n]=0;t->len=n;t->kursor=n;}
        return t;
}
void pg_textedit_hancur(pg_textedit_t *t) { if(!t)return; pg_widget_hancur(&t->base); free(t); }
void pg_textedit_setel_teks(pg_textedit_t *t, const char *s) {
        size_t n; if(!t)return; if(!s){t->buf[0]=0;t->len=0;t->kursor=0;csel(t);pg_widget_kotor(&t->base);return;}
        n=strlen(s); if(n>=(size_t)t->cap)n=t->cap-1; memcpy(t->buf,s,n); t->buf[n]=0; t->len=n; t->kursor=n; csel(t); pg_widget_kotor(&t->base);
}
const char *pg_textedit_ambil_teks(pg_textedit_t *t) { return t?(t->buf?t->buf:""):""; }
void pg_textedit_saatberubah(pg_textedit_t *t, pg_textedit_cb cb, void *ctx) { if(!t)return; t->cb=cb; t->ctx=ctx; }
char *pg_textedit_ambil_pilihan(pg_textedit_t *t)
{
        int lo,hi,n;
        char *out;
        if(!t||!hsel(t))return NULL;
        lo=smin(t);hi=smax(t);n=hi-lo;
        if(n<=0)return NULL;
        out=(char*)malloc(n+1);
        if(!out)return NULL;
        memcpy(out,t->buf+lo,n);
        out[n]=0;
        return out;
}

void pg_textedit_sisip(pg_textedit_t *t, const char *s)
{
        int n, i;
        if (!t || !s) return;
        n = (int)strlen(s);
        if (t->len + n >= t->cap) n = t->cap - 1 - t->len;
        if (n <= 0) return;
        if (hsel(t)) dsel(t);
        for (i = t->len; i >= t->kursor; i--)
                t->buf[i + n] = t->buf[i];
        memcpy(t->buf + t->kursor, s, n);
        t->len += n;
        t->kursor += n;
        t->buf[t->len] = 0;
        pg_widget_kotor(&t->base);
        fcb(t);
}

pg_widget_t *pg_textedit_widget(pg_textedit_t *t) { return t?&t->base:NULL; }
