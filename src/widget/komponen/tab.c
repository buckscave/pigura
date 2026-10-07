/* ----------------------------------------------------------------------------------------------
 * pigura widget: tab.c - panel bertab
 * ----------------------------------------------------------------------------------------------
 * Tab adalah kontainer multi-halaman: hanya satu child yang aktif.
 * Tab bar di atas berisi tombol per tab. Klik tombol = pilih child.
 *
 * Layout:
 *   [ tab0 | tab1 | tab2 ]   <- tab bar (tinggi = tab_h)
 *   +---------------------+
 *   |                     |
 *   |  child aktif        |  <- content area
 *   |                     |
 *   +---------------------+
 *
 * Tab aktif digambar dengan latar terang; tab tidak aktif digambar
 * dengan latar gelap. Garis bawah tab aktif berwarna aksen.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/tab.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/aksi.h"
#include "pigura/widget.h"

#include <stdlib.h>
#include <string.h>

typedef struct pg_tab_item {
        char        *judul;
        pg_widget_t *child;
} pg_tab_item_t;

struct pg_tab {
        pg_widget_t    base;
        pg_font_t     *font;
        pg_tab_item_t *item;
        int            n_item;
        int            cap_item;
        int            aktif;
        int            tab_h;
        pg_warna_t     latar_bar;
        pg_warna_t     latar_aktif;
        pg_warna_t     latar_hover;
        pg_warna_t     fg;
        pg_warna_t     aksen;
        pg_bool        hover_tab;
        int            hover_idx;
        pg_tab_cb      cb;
        void          *ctx;
};

static char *pg_tab_dup(const char *s)
{
        size_t n;
        char *out;
        if (!s) return NULL;
        n = strlen(s) + 1;
        out = (char *)malloc(n);
        if (!out) return NULL;
        memcpy(out, s, n);
        return out;
}

static pg_tab_t *pg_tab_dari(pg_widget_t *w)
{
        return (pg_tab_t *)w;
}

static int pg_tab_hitung_h(pg_tab_t *t)
{
        int h;
        if (!t->font) return 24;
        h = pg_font_tinggi(t->font);
        if (h <= 0) h = 8;
        return h + 8;
}

/* Hitung lebar per-tab: maksimum 80 piksel, minimum 24. */
static int pg_tab_lebar_tab(pg_tab_t *t, int idx)
{
        int w;
        if (!t->font || !t->item[idx].judul) return 24;
        w = pg_font_lebar_teks(t->font, t->item[idx].judul);
        return w + 16;
}

/* vtable: catat tab bar + child aktif. */
static void pg_tab_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
        pg_tab_t *t = pg_tab_dari(w);
        int       sw, sh, x, i, baseline;
        sw = pg_permukaan_lebar(s);
        sh = pg_permukaan_tinggi(s);
        /* Tab bar background. */
        pg_isi_permukaan_kotak(s, pg_buat_kotak(0, 0, sw, t->tab_h),
                t->latar_bar);
        /* Setiap tab. */
        x = 0;
        for (i = 0; i < t->n_item; i++) {
                int tw = pg_tab_lebar_tab(t, i);
                pg_warna_t bg;
                if (i == t->aktif)         bg = t->latar_aktif;
                else if (i == t->hover_idx) bg = t->latar_hover;
                else                       bg = t->latar_bar;
                pg_isi_permukaan_kotak(s,
                        pg_buat_kotak(x, 0, tw, t->tab_h - 2), bg);
                /* Garis pemisah kanan. */
                pg_garis_v_permukaan(s, x + tw - 1, 0, t->tab_h - 2,
                        t->latar_bar);
                /* Aksen underline untuk tab aktif. */
                if (i == t->aktif)
                        pg_garis_h_permukaan(s, x, x + tw - 1,
                                t->tab_h - 3, t->aksen);
                /* Label. */
                if (t->item[i].judul && t->font) {
                        baseline = pg_font_baseline_tengah(t->font,
                                t->tab_h - 2);
                        pg_font_gambar_teks(t->font,
                                t->item[i].judul, s, x + 8, baseline,
                                t->fg);
                }
                x += tw;
        }
        /* Garis bawah tab bar. */
        pg_garis_h_permukaan(s, 0, sw, t->tab_h - 2, t->latar_bar);
        /* Content area: hanya child aktif. */
        if (t->aktif >= 0 && t->aktif < t->n_item &&
            t->item[t->aktif].child) {
                pg_widget_t *c = t->item[t->aktif].child;
                int sx, sy, sw_c, sh_c;
                sx = c->kotak.x;
                sy = c->kotak.y;
                sw_c = c->kotak.w;
                sh_c = c->kotak.h;
                c->kotak.x = 0;
                c->kotak.y = t->tab_h;
                /* Pastikan ukuran child = area konten. */
                if (c->kotak.w != sw || c->kotak.h != sh - t->tab_h) {
                        c->kotak.x = 0;
                        c->kotak.y = t->tab_h;
                        pg_widget_setel_kotak(c, pg_buat_kotak(
                                0, t->tab_h, sw, sh - t->tab_h));
                }
                pg_widget_catat(c, s);
                /* Restore (child disimpan dengan offset lokal). */
                (void)sx; (void)sy; (void)sw_c; (void)sh_c;
                c->kotak.x = 0;
                c->kotak.y = t->tab_h;
        }
}

static pg_bool pg_tab_peristiwa_v(pg_widget_t *w,
                                   const pg_aksi_t *e)
{
        pg_tab_t *t = pg_tab_dari(w);
        int       i, x, tw, idx;
        if (e->tipe == PG_AKSI_TETIKUS_TEKAN &&
            e->tetik_tombol == PG_TETIKUS_KIRI &&
            e->tetik_pos.y < t->tab_h) {
                /* Hitung tab mana yang diklik. */
                x = 0;
                idx = -1;
                for (i = 0; i < t->n_item; i++) {
                        tw = pg_tab_lebar_tab(t, i);
                        if (e->tetik_pos.x >= x &&
                            e->tetik_pos.x < x + tw) {
                                idx = i;
                                break;
                        }
                        x += tw;
                }
                if (idx >= 0 && idx != t->aktif) {
                        pg_tab_pilih(t, idx);
                        if (t->cb) t->cb(idx, t->ctx);
                }
                return PG_BENAR;
        }
        if (e->tipe == PG_AKSI_TETIKUS_GERAK &&
            e->tetik_pos.y < t->tab_h) {
                x = 0;
                idx = -1;
                for (i = 0; i < t->n_item; i++) {
                        tw = pg_tab_lebar_tab(t, i);
                        if (e->tetik_pos.x >= x &&
                            e->tetik_pos.x < x + tw) {
                                idx = i;
                                break;
                        }
                        x += tw;
                }
                if (idx != t->hover_idx) {
                        t->hover_idx = idx;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }
        /* Teruskan ke child aktif. */
        if (t->aktif >= 0 && t->aktif < t->n_item &&
            t->item[t->aktif].child) {
                pg_aksi_t e2 = *e;
                e2.tetik_pos.y -= t->tab_h;
                return pg_widget_tangani_aksi(
                        t->item[t->aktif].child, &e2);
        }
        return PG_SALAH;
}

static void pg_tab_ubah_ukuran_v(pg_widget_t *w, int w_, int h)
{
        pg_tab_t *t = pg_tab_dari(w);
        int       i;
        (void)w_; (void)h;
        t->tab_h = pg_tab_hitung_h(t);
        /* Resize semua child ke ukuran area konten. */
        for (i = 0; i < t->n_item; i++) {
                if (t->item[i].child)
                        pg_widget_setel_kotak(t->item[i].child,
                                pg_buat_kotak(0, t->tab_h,
                                        w->kotak.w,
                                        w->kotak.h - t->tab_h));
        }
}

static void pg_tab_hancur_v(pg_widget_t *w)
{
        pg_tab_t *t = pg_tab_dari(w);
        int i;
        for (i = 0; i < t->n_item; i++) {
                if (t->item[i].judul) {
                        free(t->item[i].judul);
                        t->item[i].judul = NULL;
                }
        }
        if (t->item) {
                free(t->item);
                t->item = NULL;
        }
        t->n_item = 0;
        t->cap_item = 0;
}

/* Deallocator struktur turunan — dipanggil oleh
 * pg_widget_hancur_penuh() setelah hancur_v. */
static void pg_tab_bebas_v(pg_widget_t *w)
{
        free(w);
}

static const pg_widget_vtable_t pg_tab_vtable = {
        pg_tab_catat_v,
        pg_tab_peristiwa_v,
        pg_tab_ubah_ukuran_v,
        pg_tab_hancur_v,
        NULL,
        NULL,
        pg_tab_bebas_v
};

/* ---------------------------------------------------------------- API */

pg_tab_t *pg_buat_tab(pg_font_t *font)
{
        pg_tab_t *t;
        t = (pg_tab_t *)calloc(1, sizeof(*t));
        if (!t) return NULL;
        pg_widget_init(&t->base, PG_WIDGET_KOTAK, &pg_tab_vtable);
        /* Set milik=BENAR agar child (tab pages) dihancurkan
         * otomatis via universal anak list. */
        pg_widget_milik(&t->base, PG_BENAR);
        t->font = font;
        t->aktif = -1;
        t->tab_h = pg_tab_hitung_h(t);
        t->latar_bar = PG_ABU_GELAP;
        t->latar_aktif = PG_ABU_TERANG;
        t->latar_hover = PG_ABU;
        t->fg = PG_PUTIH;
        t->aksen = PG_KUNING;
        t->hover_idx = -1;
        pg_widget_setel_kotak(&t->base, pg_buat_kotak(0, 0, 200, 100));
        pg_widget_setel_latar(&t->base, PG_ABU_GELAP);
        pg_widget_setel_ukuran_min(&t->base, 80, 60);
        return t;
}

void pg_tab_hancur(pg_tab_t *t)
{
        if (!t) return;
        pg_widget_hancur(&t->base);
        free(t);
}

int pg_tab_tambah(pg_tab_t *t, const char *judul,
                   pg_widget_t *child)
{
        pg_tab_item_t *baru;
        char           *str;
        if (!t) return -1;
        if (t->n_item >= t->cap_item) {
                int cap_baru = t->cap_item ? t->cap_item * 2 : 4;
                baru = (pg_tab_item_t *)realloc(t->item,
                        (size_t)cap_baru * sizeof(*baru));
                if (!baru) return -1;
                t->item = baru;
                t->cap_item = cap_baru;
        }
        str = pg_tab_dup(judul);
        if (judul && !str) return -1;
        t->item[t->n_item].judul = str;
        t->item[t->n_item].child = child;
        t->n_item++;
        if (child) pg_widget_tambah_anak(&t->base, child);
        if (t->aktif < 0) t->aktif = 0;
        /* Resize child ke area konten. */
        if (child) {
                pg_widget_setel_kotak(child,
                        pg_buat_kotak(0, t->tab_h,
                                t->base.kotak.w,
                                t->base.kotak.h - t->tab_h));
        }
        pg_widget_kotor(&t->base);
        return t->n_item - 1;
}

void pg_tab_hapus(pg_tab_t *t, int index)
{
        int i;
        if (!t || index < 0 || index >= t->n_item) return;
        if (t->item[index].judul) {
                free(t->item[index].judul);
                t->item[index].judul = NULL;
        }
        for (i = index; i < t->n_item - 1; i++)
                t->item[i] = t->item[i + 1];
        t->n_item--;
        if (t->aktif >= t->n_item) t->aktif = t->n_item - 1;
        if (t->aktif < 0 && t->n_item > 0) t->aktif = 0;
        pg_widget_kotor(&t->base);
}

void pg_tab_pilih(pg_tab_t *t, int index)
{
        if (!t) return;
        if (index < 0 || index >= t->n_item) return;
        if (t->aktif == index) return;
        t->aktif = index;
        pg_widget_kotor(&t->base);
}

int pg_tab_aktif(const pg_tab_t *t)
{
        return t ? t->aktif : -1;
}

int pg_tab_jumlah(const pg_tab_t *t)
{
        return t ? t->n_item : 0;
}

void pg_tab_saat_ubah(pg_tab_t *t,
    void (*cb)(int idx, void *ctx), void *ctx)
{
        if (!t) return;
        t->cb = cb;
        t->ctx = ctx;
}

pg_widget_t *pg_tab_widget(pg_tab_t *t)
{
        return t ? &t->base : NULL;
}
