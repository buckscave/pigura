/* ----------------------------------------------------------------------------------------------
 * pigura widget: menubar.c - menu bar horizontal
 * ----------------------------------------------------------------------------------------------
 * Menubar adalah bar horizontal berisi beberapa menu. Klik menu
 * membuka dropdown berisi item; klik item memicu callback. Item
 * pemisah digambar sebagai garis horizontal. Hanya satu menu yang
 * bisa terbuka pada satu waktu; klik di luar menutup menu yang
 * sedang terbuka.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/menubar.h"
#include "pigura/permukaan.h"
#include "pigura/font.h"
#include "pigura/peristiwa.h"
#include "pigura/widget.h"

#include <stdlib.h>
#include <string.h>

typedef struct pg_menu_item {
        char       *label; /* NULL = pemisah */
        pg_menu_cb  cb;
        void       *ctx;
} pg_menu_item_t;

struct pg_menu {
        char            judul[32];
        pg_menu_item_t *item;
        int             n_item;
        int             cap_item;
        int             x_buka; /* posisi x judul di menubar */
        int             hover_item; /* indeks item hover, -1 */
};

struct pg_menubar {
        pg_widget_t  base;
        pg_menu_t  **menu;
        int          n_menu;
        int          cap_menu;
        int          buka_menu; /* indeks menu terbuka, -1 jika tidak ada */
        int          hover_menu; /* indeks menu di-hover (-1 = tidak ada) */
        pg_warna_t   fg;
        pg_warna_t   latar;
        pg_warna_t   sel_bg;
        pg_warna_t   sel_fg;
        pg_warna_t   hover_bg; /* latar saat hover item popup */
        pg_font_t   *font;
};

static char *pg_mb_dup(const char *s)
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

static pg_menubar_t *pg_menubar_dari(pg_widget_t *w)
{
        return (pg_menubar_t *)w;
}

/* Tinggi title bar. */
static int pg_mb_title_h(pg_menubar_t *mb)
{
        int th;
        if (!mb->font) return 12;
        th = pg_font_tinggi(mb->font);
        if (th <= 0) th = 8;
        return th + 4;
}

/* Lebar judul menu (text + padding). */
static int pg_mb_judul_w(pg_menubar_t *mb, pg_menu_t *m)
{
        int w;
        if (!mb->font) return 8;
        w = pg_font_lebar_teks(mb->font, m->judul) + 8;
        if (w < 8) w = 8;
        return w;
}

/* Lebar dropdown menu = max(lebar judul, lebar item terlebar). */
static int pg_mb_dropdown_w(pg_menubar_t *mb, pg_menu_t *m)
{
        int max_w, j;
        max_w = pg_mb_judul_w(mb, m);
        for (j = 0; j < m->n_item; j++) {
                if (m->item[j].label && mb->font) {
                        int iw = pg_font_lebar_teks(mb->font,
                                                     m->item[j].label) + 8;
                        if (iw > max_w) max_w = iw;
                }
        }
        return max_w;
}

/* vtable: catat menubar (title bar saja). Dropdown menu
 * dirender via catat_popup langsung ke permukaan tujuan
 * supaya tidak terpotong oleh kotak menubar. */
static void pg_menubar_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
        pg_menubar_t *mb = pg_menubar_dari(w);
        int th, title_h, i;
        int bar_h;
        int baseline_y;
        th = mb->font ? pg_font_tinggi(mb->font) : 8;
        if (th <= 0) th = 8;
        title_h = pg_mb_title_h(mb);
        /* Tinggi bar aktual (bisa lebih dari title_h bila app
         * setel kotak.h lebih besar). Center text vertikal
         * di bar ini. */
        bar_h = w->kotak.h > 0 ? w->kotak.h : title_h;
        baseline_y = pg_font_baseline_tengah(mb->font, bar_h);
        for (i = 0; i < mb->n_menu; i++) {
                int mw, mx, my;
                pg_menu_t *m = mb->menu[i];
                mw = pg_mb_judul_w(mb, m);
                mx = m->x_buka;
                my = baseline_y;
                /* Hover track: highlight title bila menu terbuka
                 * ATAU bila kursor di atas title (bila app track
                 * hover). Untuk simpel, kita render highlight
                 * hanya saat menu terbuka (sel_bg). Hover track
                 * untuk title yang belum dibuka: track di sini. */
                if (i == mb->buka_menu) {
                        pg_isi_permukaan_kotak(s,
                                pg_buat_kotak(mx, 0, mw, bar_h),
                                mb->sel_bg);
                } else if (i == mb->hover_menu) {
                        /* Hover track untuk title yang belum
                         * dibuka — gunakan hover_bg supaya
                         * terlihat beda dari selected. */
                        pg_isi_permukaan_kotak(s,
                                pg_buat_kotak(mx, 0, mw, bar_h),
                                mb->hover_bg);
                }
                if (mb->font)
                        pg_font_gambar_teks(mb->font, m->judul, s,
                                mx + 4, my,
                                i == mb->buka_menu ?
                                mb->sel_fg : mb->fg);
        }
}

/* vtable: catat popup overlay - dropdown menu bila buka_menu>=0.
 * Render langsung ke dest (koordinat absolut) tanpa clip ke
 * kotak menubar. Dipanggil SETELAH catat normal. */
static void pg_menubar_catat_popup_v(pg_widget_t *w,
                                     pg_permukaan_t *s)
{
        pg_menubar_t *mb = pg_menubar_dari(w);
        int title_h, mx_abs, ty_abs;
        pg_menu_t *m;
        int max_w, item_h, j;
        pg_kotak_t dr;
        if (mb->buka_menu < 0) return;
        if (mb->buka_menu >= mb->n_menu) return;
        title_h = pg_mb_title_h(mb);
        m = mb->menu[mb->buka_menu];
        /* Posisi absolut di dest (jumlah kotak.x/y widget + induk). */
        {
                int ax = 0, ay = 0;
                pg_widget_posisi_layar(w, &ax, &ay);
                mx_abs = ax + m->x_buka;
                ty_abs = ay + w->kotak.h; /* pakai tinggi widget, bukan title_h */
        }
        max_w = pg_mb_dropdown_w(mb, m);
        item_h = title_h;
        dr = pg_buat_kotak(mx_abs, ty_abs, max_w,
                           m->n_item * item_h + 2);
        pg_isi_permukaan_kotak(s, dr, mb->latar);
        pg_kotak_permukaan(s, dr, mb->fg);
        for (j = 0; j < m->n_item; j++) {
                int iy_abs = ty_abs + j * item_h + 1;
                /* Hover highlight untuk item (bukan pemisah).
                 * Latar hover_bg hanya bila item hover & bukan
                 * item terpilih (sel_bg). Menubar saat ini tidak
                 * punya state terpilih per-item, jadi selalu
                 * kena hover_bg bila hover_item == j. */
                if (m->hover_item == j && m->item[j].label) {
                        pg_isi_permukaan_kotak(s,
                                pg_buat_kotak(mx_abs + 1,
                                        ty_abs + j * item_h,
                                        max_w - 2, item_h),
                                mb->hover_bg);
                }
                if (m->item[j].label) {
                        if (mb->font)
                                pg_font_gambar_teks(mb->font,
                                        m->item[j].label, s,
                                        mx_abs + 4,
                                        iy_abs +
                                        pg_font_baseline_tengah(
                                                mb->font, item_h),
                                        mb->fg);
                } else {
                        pg_garis_h_permukaan(s,
                                mx_abs + 2,
                                mx_abs + max_w - 2,
                                iy_abs + item_h / 2,
                                mb->fg);
                }
        }
}

/* vtable: hit-test - bila ada menu terbuka, area dropdown
 * dianggap bagian widget supaya klik di popup terdispatch.
 * Note: kotak widget di sini adalah lokal-induk (bukan absolut)
 * karena e->tetik_pos yang diterima vtable->peristiwa sudah
 * ditranslasikan ke frame induk oleh pg_widget_tangani_peristiwa. */
static pg_bool pg_menubar_berisi_v(pg_widget_t *w, pg_titik_t p)
{
        pg_menubar_t *mb = pg_menubar_dari(w);
        if (pg_widget_berisi(w, p)) return PG_BENAR;
        if (mb->buka_menu < 0 || mb->buka_menu >= mb->n_menu)
                return PG_SALAH;
        {
                pg_menu_t *m = mb->menu[mb->buka_menu];
                int title_h = pg_mb_title_h(mb);
                int max_w = pg_mb_dropdown_w(mb, m);
                int drop_x0 = w->kotak.x + m->x_buka;
                int drop_x1 = drop_x0 + max_w;
                int drop_y0 = w->kotak.y + title_h;
                int drop_y1 = drop_y0 + m->n_item * title_h + 2;
                if (p.x >= drop_x0 && p.x < drop_x1 &&
                    p.y >= drop_y0 && p.y < drop_y1)
                        return PG_BENAR;
        }
        return PG_SALAH;
}

/* Tutup menu aktif: set buka_menu = -1 dan reset hover_item
 * dari menu yang ditutup supaya tidak ada highlight tersisa. */
static void pg_mb_tutup(pg_menubar_t *mb)
{
        if (mb->buka_menu >= 0 && mb->buka_menu < mb->n_menu &&
            mb->menu[mb->buka_menu])
                mb->menu[mb->buka_menu]->hover_item = -1;
        mb->buka_menu = -1;
        mb->hover_menu = -1;
}

static pg_bool pg_menubar_peristiwa_v(pg_widget_t *w,
                                     const pg_peristiwa_t *e)
{
        pg_menubar_t *mb = pg_menubar_dari(w);
        int title_h, i;
        int bar_h;

        bar_h = w->kotak.h > 0 ? w->kotak.h : pg_mb_title_h(mb);
        title_h = pg_mb_title_h(mb);

        /* Hover tracking saat popup menu terbuka.
         * Hitung indeks item dari e->tetik_pos.y, set hover_item,
         * kotor widget supaya popup tergambar ulang. */
        if (mb->buka_menu >= 0 &&
            e->tipe == PG_PERISTIWA_TETIK_GERAK) {
                pg_menu_t *m = mb->menu[mb->buka_menu];
                int max_w, item_h, idx;
                item_h = title_h;
                max_w = pg_mb_dropdown_w(mb, m);
                if (e->tetik_pos.x >= m->x_buka &&
                    e->tetik_pos.x < m->x_buka + max_w &&
                    e->tetik_pos.y >= bar_h) {
                        idx = (e->tetik_pos.y - bar_h) / item_h;
                        if (idx >= 0 && idx < m->n_item) {
                                if (idx != m->hover_item) {
                                        m->hover_item = idx;
                                        pg_widget_kotor(w);
                                }
                        }
                }
                return PG_BENAR;
        }

        /* Hover tracking title bar (menu belum terbuka).
         * Track menu yang di-hover untuk highlight visual. */
        if (mb->buka_menu < 0 &&
            e->tipe == PG_PERISTIWA_TETIK_GERAK) {
                int new_hover = -1;
                if (e->tetik_pos.y < bar_h) {
                        for (i = 0; i < mb->n_menu; i++) {
                                int mw = pg_mb_judul_w(mb,
                                        mb->menu[i]);
                                if (e->tetik_pos.x >=
                                        mb->menu[i]->x_buka &&
                                    e->tetik_pos.x <
                                        mb->menu[i]->x_buka + mw) {
                                        new_hover = i;
                                        break;
                                }
                        }
                }
                if (new_hover != mb->hover_menu) {
                        mb->hover_menu = new_hover;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }

        if (e->tipe != PG_PERISTIWA_TETIK_TURUN) return PG_SALAH;
        if (e->tetik_tombol != PG_TETIK_KIRI) return PG_SALAH;
        /* Klik di title bar? */
        if (e->tetik_pos.y < bar_h) {
                for (i = 0; i < mb->n_menu; i++) {
                        int mw = pg_mb_judul_w(mb, mb->menu[i]);
                        if (e->tetik_pos.x >= mb->menu[i]->x_buka &&
                            e->tetik_pos.x < mb->menu[i]->x_buka + mw) {
                                if (mb->buka_menu == i)
                                        pg_mb_tutup(mb);
                                else {
                                        pg_mb_tutup(mb);
                                        mb->buka_menu = i;
                                }
                                mb->hover_menu = -1;
                                pg_widget_kotor(w);
                                return PG_BENAR;
                        }
                }
                if (mb->buka_menu >= 0) {
                        pg_mb_tutup(mb);
                        pg_widget_kotor(w);
                }
                return PG_SALAH;
        }
        /* Klik di dropdown? */
        if (mb->buka_menu >= 0) {
                pg_menu_t *m = mb->menu[mb->buka_menu];
                int max_w, item_h, idx;
                max_w = pg_mb_dropdown_w(mb, m);
                item_h = title_h;
                if (e->tetik_pos.x >= m->x_buka &&
                    e->tetik_pos.x < m->x_buka + max_w) {
                        idx = (e->tetik_pos.y - bar_h) / item_h;
                        if (idx >= 0 && idx < m->n_item) {
                                if (m->item[idx].label &&
                                    m->item[idx].cb)
                                        m->item[idx].cb(m, idx,
                                                m->item[idx].ctx);
                                pg_mb_tutup(mb);
                                pg_widget_kotor(w);
                                return PG_BENAR;
                        }
                }
                pg_mb_tutup(mb);
                pg_widget_kotor(w);
                return PG_BENAR;
        }
        return PG_SALAH;
}

/* vtable hancur: bebaskan semua menu + item. */
static void pg_menubar_hancur_v(pg_widget_t *w)
{
        pg_menubar_t *mb = pg_menubar_dari(w);
        int i, j;
        for (i = 0; i < mb->n_menu; i++) {
                pg_menu_t *m = mb->menu[i];
                if (!m) continue;
                for (j = 0; j < m->n_item; j++) {
                        if (m->item[j].label) {
                                free(m->item[j].label);
                                m->item[j].label = NULL;
                        }
                }
                if (m->item) free(m->item);
                free(m);
                mb->menu[i] = NULL;
        }
        if (mb->menu) {
                free(mb->menu);
                mb->menu = NULL;
        }
        mb->n_menu = 0;
        mb->cap_menu = 0;
        pg_mb_tutup(mb);
}

/* Deallocator struktur turunan — dipanggil oleh
 * pg_widget_hancur_penuh() setelah hancur_v. */
static void pg_menubar_bebas_v(pg_widget_t *w)
{
        free(w);
}

static const pg_widget_vtable_t pg_menubar_vtable = {
        pg_menubar_catat_v,
        pg_menubar_peristiwa_v,
        NULL,
        pg_menubar_hancur_v,
        pg_menubar_catat_popup_v,
        pg_menubar_berisi_v,
        pg_menubar_bebas_v
};

/* ---------------------------------------------------------------- API */

pg_menubar_t *pg_buat_menubar(pg_font_t *font)
{
        pg_menubar_t *mb;
        mb = (pg_menubar_t *)calloc(1, sizeof(*mb));
        if (!mb) return NULL;
        pg_widget_init(&mb->base, PG_WIDGET_MENUBAR,
                       &pg_menubar_vtable);
        mb->font = font;
        mb->fg = PG_HITAM;
        mb->latar = PG_ABU_TERANG;
        mb->sel_bg = PG_BIRU;
        mb->sel_fg = PG_PUTIH;
        mb->hover_bg = PG_ABU; /* lebih gelak dari latar */
        mb->buka_menu = -1;
        mb->hover_menu = -1;
        pg_widget_setel_latar(&mb->base, mb->latar);
        return mb;
}

void pg_menubar_hancur(pg_menubar_t *mb)
{
        if (!mb) return;
        pg_widget_hancur(&mb->base);
        free(mb);
}

pg_menu_t *pg_menubar_tambah_menu(pg_menubar_t *mb,
                                   const char *judul)
{
        pg_menu_t *m;
        pg_menu_t **arr_baru;
        int cap_baru, i, x_acc;
        if (!mb) return NULL;
        /* Alokasi menu. */
        m = (pg_menu_t *)calloc(1, sizeof(*m));
        if (!m) return NULL;
        if (judul) {
                size_t n = strlen(judul);
                if (n >= sizeof(m->judul))
                        n = sizeof(m->judul) - 1;
                memcpy(m->judul, judul, n);
                m->judul[n] = 0;
        }
        /* Hitung x_buka = jumlah lebar menu sebelumnya. */
        x_acc = 0;
        for (i = 0; i < mb->n_menu; i++)
                x_acc += pg_mb_judul_w(mb, mb->menu[i]);
        m->x_buka = x_acc;
        /* Tambah ke array. */
        if (mb->n_menu >= mb->cap_menu) {
                cap_baru = mb->cap_menu ? mb->cap_menu * 2 : 4;
                arr_baru = (pg_menu_t **)realloc(mb->menu,
                        (size_t)cap_baru * sizeof(*arr_baru));
                if (!arr_baru) {
                        free(m);
                        return NULL;
                }
                mb->menu = arr_baru;
                mb->cap_menu = cap_baru;
        }
        mb->menu[mb->n_menu++] = m;
        pg_widget_kotor(&mb->base);
        return m;
}

void pg_menu_tambah_item(pg_menu_t *m, const char *label,
                          pg_menu_cb cb, void *ctx)
{
        pg_menu_item_t *baru;
        int cap_baru;
        if (!m) return;
        if (m->n_item >= m->cap_item) {
                cap_baru = m->cap_item ? m->cap_item * 2 : 8;
                baru = (pg_menu_item_t *)realloc(m->item,
                        (size_t)cap_baru * sizeof(*baru));
                if (!baru) return;
                m->item = baru;
                m->cap_item = cap_baru;
        }
        m->item[m->n_item].label = pg_mb_dup(label);
        m->item[m->n_item].cb = cb;
        m->item[m->n_item].ctx = ctx;
        m->n_item++;
}

void pg_menu_tambah_pemisah(pg_menu_t *m)
{
        pg_menu_item_t *baru;
        int cap_baru;
        if (!m) return;
        if (m->n_item >= m->cap_item) {
                cap_baru = m->cap_item ? m->cap_item * 2 : 8;
                baru = (pg_menu_item_t *)realloc(m->item,
                        (size_t)cap_baru * sizeof(*baru));
                if (!baru) return;
                m->item = baru;
                m->cap_item = cap_baru;
        }
        m->item[m->n_item].label = NULL;
        m->item[m->n_item].cb = NULL;
        m->item[m->n_item].ctx = NULL;
        m->n_item++;
}

pg_widget_t *pg_menubar_widget(pg_menubar_t *mb)
{
        return mb ? &mb->base : NULL;
}

pg_bool pg_menubar_terbuka(pg_menubar_t *mb)
{
        return mb ? (mb->buka_menu >= 0 ? PG_BENAR : PG_SALAH)
                  : PG_SALAH;
}

void pg_menubar_tutup(pg_menubar_t *mb)
{
        if (!mb) return;
        if (mb->buka_menu >= 0) {
                pg_mb_tutup(mb);
                pg_widget_kotor(&mb->base);
        }
}
