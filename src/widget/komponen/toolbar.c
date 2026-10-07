/* ----------------------------------------------------------------------------------------------
 * pigura widget: toolbar.c - baris tombol
 * ----------------------------------------------------------------------------------------------
 * Toolbar adalah kontainer horizontal (atau vertikal) berisi tombol,
 * pemisah, dan spasi. Tombol dibuat internal dengan pg_buat_tombol;
 * saat diklik, callback pemilik dipanggil dengan id tombol.
 *
 * Layout: tiap item punya lebar natural (tombol = font_lebar(label)
 * + 16; pemisah = 4; spasi = expand). Spasi memperoleh sisa ruang
 * terbagi rata dengan spasi lain.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/toolbar.h"
#include "pigura/tombol.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/aksi.h"
#include "pigura/widget.h"

#include <stdlib.h>
#include <string.h>

#define PG_TB_TOMBOL_H 26
#define PG_TB_PEMISAH_W 4
#define PG_TB_TOMBOL_PAD 8

typedef struct pg_tb_item {
        pg_tb_tipe_t tipe;
        pg_tombol_t *tombol; /* hanya untuk PG_TB_TOMBOL */
        int           id;
        int           lebar_min; /* natural */
        void        (*cb_v)(void*);
        void        (*cb_id)(pg_toolbar_t*, int, void*);
        void         *ctx;
} pg_tb_item_t;

struct pg_toolbar {
        pg_widget_t   base;
        pg_bool       vertikal;
        pg_font_t    *font;
        pg_tb_item_t *item;
        int           n_item;
        int           cap_item;
        int           id_berikutnya;
        void        (*cb_global)(pg_toolbar_t*, int, void*);
        void         *ctx_global;
};

static pg_toolbar_t *pg_tb_dari(pg_widget_t *w)
{
        return (pg_toolbar_t *)w;
}

/* Hitung total minimum + jumlah spasi. */
static void pg_tb_hitung(pg_toolbar_t *tb, int *total_min,
                          int *n_spasi, int avail)
{
        int i, tm, ns;
        tm = 0; ns = 0;
        for (i = 0; i < tb->n_item; i++) {
                pg_tb_item_t *it = &tb->item[i];
                if (it->tipe == PG_TB_TOMBOL) {
                        int w = it->lebar_min;
                        tm += w;
                } else if (it->tipe == PG_TB_PEMISAH) {
                        tm += PG_TB_PEMISAH_W;
                } else {
                        /* SPASI: dihitung tapi tidak menambah min. */
                        ns++;
                }
        }
        (void)avail;
        *total_min = tm;
        *n_spasi = ns;
}

/* Layout: posisikan semua item sepanjang sumbu layout. */
static void pg_tb_layout(pg_toolbar_t *tb)
{
        int avail, total_min, n_spasi, extra_per, pos, i;
        int horiz = !tb->vertikal;
        avail = horiz ? tb->base.kotak.w : tb->base.kotak.h;
        pg_tb_hitung(tb, &total_min, &n_spasi, avail);
        if (n_spasi > 0)
                extra_per = (avail - total_min) / n_spasi;
        else
                extra_per = 0;
        if (extra_per < 0) extra_per = 0;
        pos = 0;
        for (i = 0; i < tb->n_item; i++) {
                pg_tb_item_t *it = &tb->item[i];
                int item_w;
                if (it->tipe == PG_TB_TOMBOL) {
                        item_w = it->lebar_min;
                } else if (it->tipe == PG_TB_PEMISAH) {
                        item_w = PG_TB_PEMISAH_W;
                } else {
                        item_w = extra_per;
                }
                if (horiz) {
                        if (it->tombol)
                                pg_widget_setel_kotak(
                                        pg_tombol_widget(it->tombol),
                                        pg_buat_kotak(pos, 0, item_w,
                                                tb->base.kotak.h));
                        pos += item_w;
                } else {
                        if (it->tombol)
                                pg_widget_setel_kotak(
                                        pg_tombol_widget(it->tombol),
                                        pg_buat_kotak(0, pos,
                                                tb->base.kotak.w, item_w));
                        pos += item_w;
                }
        }
}

/* vtable catat: gambar pemisah + spasi + delegasi tombol. */
static void pg_tb_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
        pg_toolbar_t *tb = pg_tb_dari(w);
        int i, pos = 0;
        int horiz = !tb->vertikal;
        int avail = horiz ? tb->base.kotak.w : tb->base.kotak.h;
        int total_min, n_spasi, extra_per;
        pg_tb_layout(tb);
        pg_tb_hitung(tb, &total_min, &n_spasi, avail);
        extra_per = n_spasi > 0 ? (avail - total_min) / n_spasi : 0;
        if (extra_per < 0) extra_per = 0;
        for (i = 0; i < tb->n_item; i++) {
                pg_tb_item_t *it = &tb->item[i];
                int item_w;
                if (it->tipe == PG_TB_PEMISAH) {
                        item_w = PG_TB_PEMISAH_W;
                        if (horiz)
                                pg_garis_v_permukaan(s, pos + item_w / 2, 4,
                                        tb->base.kotak.h - 4, PG_ABU_GELAP);
                        else
                                pg_garis_h_permukaan(s, 4, item_w - 4,
                                        pos + item_w / 2, PG_ABU_GELAP);
                } else if (it->tipe == PG_TB_TOMBOL) {
                        item_w = it->lebar_min;
                        if (it->tombol)
                                pg_widget_catat(
                                        pg_tombol_widget(it->tombol), s);
                } else {
                        item_w = extra_per;
                }
                pos += item_w;
        }
}

/* vtable peristiwa: teruskan ke tombol anak (reverse order).
 *
 * Bypass pg_widget_tangani_aksi untuk child tombol sebab
 * g_capture statik di widget.c konflik (capture toolbar vs tombol
 * anak). Pakai vtable tombol langsung + flag ditekan internal
 * tombol melacak state tekan-lepas. Koordinat diterjemahkan ke
 * lokal tombol sebelum panggil vtable. */
static pg_bool pg_tb_peristiwa_v(pg_widget_t *w,
                                  const pg_aksi_t *e)
{
        pg_toolbar_t *tb = pg_tb_dari(w);
        int i;
        for (i = tb->n_item - 1; i >= 0; i--) {
                pg_tb_item_t *it = &tb->item[i];
                if (it->tipe == PG_TB_TOMBOL && it->tombol) {
                        pg_widget_t *bw = pg_tombol_widget(
                                it->tombol);
                        if (PG_TITIK_DI_KOTAK(e->tetik_pos,
                                bw->kotak)) {
                                pg_aksi_t e2 = *e;
                                e2.tetik_pos.x -= bw->kotak.x;
                                e2.tetik_pos.y -= bw->kotak.y;
                                if (bw->vtable &&
                                    bw->vtable->aksi &&
                                    bw->vtable->aksi(bw, &e2))
                                        return PG_BENAR;
                        }
                }
        }
        return PG_SALAH;
}

static void pg_tb_ubah_ukuran_v(pg_widget_t *w, int w_, int h)
{
        pg_toolbar_t *tb = pg_tb_dari(w);
        (void)w_; (void)h;
        pg_tb_layout(tb);
}

static void pg_tb_hancur_v(pg_widget_t *w)
{
        pg_toolbar_t *tb = pg_tb_dari(w);
        int i;
        /* JANGAN free tombol di sini. Tombol sudah ada di universal
         * anak list (base.anak[]) lewat pg_widget_tambah_anak().
         * pg_widget_hancur() akan hancurkan mereka bila milik=BENAR.
         *
         * Sebelum v0.9.0, fungsi ini memanggil pg_tombol_hancur()
         * untuk setiap item, yang menyebabkan double-free bila
         * toolbar->milik=BENAR (tombol di-free 2x: satu oleh
         * universal anak list, satu oleh sini). */
        for (i = 0; i < tb->n_item; i++) {
                /* Hanya NULL-kan pointer; struktur di-free lewat
                 * universal anak list. */
                tb->item[i].tombol = NULL;
        }
        if (tb->item) {
                free(tb->item);
                tb->item = NULL;
        }
        tb->n_item = 0;
        tb->cap_item = 0;
}

/* Deallocator struktur turunan — dipanggil oleh
 * pg_widget_hancur_penuh() setelah hancur_v. */
static void pg_tb_bebas_v(pg_widget_t *w)
{
        free(w);
}

static const pg_widget_vtable_t pg_tb_vtable = {
        pg_tb_catat_v,
        pg_tb_peristiwa_v,
        pg_tb_ubah_ukuran_v,
        pg_tb_hancur_v,
        NULL,
        NULL,
        pg_tb_bebas_v
};

/* Wrapper callback tombol -> panggil cb_v + cb_id + cb_global. */
static void pg_tb_tombol_cb(pg_tombol_t *t, void *ctx)
{
        pg_tb_item_t *it = (pg_tb_item_t *)ctx;
        if (!it) return;
        (void)t;
        if (it->cb_v) it->cb_v(it->ctx);
        if (it->cb_id) it->cb_id(NULL, it->id, it->ctx);
        /* cb_global memakai toolbar dari item -> ambil dari base. */
        if (it->tombol) {
                pg_widget_t *bw = pg_tombol_widget(it->tombol);
                if (bw && bw->induk) {
                        pg_toolbar_t *tb = pg_tb_dari(bw->induk);
                        if (tb && tb->cb_global)
                                tb->cb_global(tb, it->id, tb->ctx_global);
                }
        }
}

/* ---------------------------------------------------------------- API */

pg_toolbar_t *pg_buat_toolbar(pg_bool vertikal)
{
        pg_toolbar_t *tb;
        tb = (pg_toolbar_t *)calloc(1, sizeof(*tb));
        if (!tb) return NULL;
        pg_widget_init(&tb->base, PG_WIDGET_KOTAK, &pg_tb_vtable);
        /* Set milik=BENAR agar tombol (anak universal) dihancurkan
         * otomatis saat toolbar dihancurkan. vtable->hancur
         * (pg_tb_hancur_v) TIDAK lagi free tombol manual —
         * universal anak list yang lakukan. */
        pg_widget_milik(&tb->base, PG_BENAR);
        tb->vertikal = vertikal ? PG_BENAR : PG_SALAH;
        tb->id_berikutnya = 1;
        pg_widget_setel_kotak(&tb->base,
                pg_buat_kotak(0, 0, 400, PG_TB_TOMBOL_H));
        pg_widget_setel_latar(&tb->base, PG_ABU_TERANG);
        pg_widget_setel_ukuran_min(&tb->base, 80, PG_TB_TOMBOL_H);
        return tb;
}

void pg_toolbar_hancur(pg_toolbar_t *tb)
{
        if (!tb) return;
        pg_widget_hancur(&tb->base);
        free(tb);
}

/* Tambah item generic. */
static int pg_tb_tambah_item(pg_toolbar_t *tb, pg_tb_tipe_t tipe,
                              const char *label)
{
        pg_tb_item_t *baru;
        int id;
        if (!tb) return -1;
        if (tb->n_item >= tb->cap_item) {
                int cap_baru = tb->cap_item ? tb->cap_item * 2 : 8;
                baru = (pg_tb_item_t *)realloc(tb->item,
                        (size_t)cap_baru * sizeof(*baru));
                if (!baru) return -1;
                tb->item = baru;
                tb->cap_item = cap_baru;
        }
        tb->item[tb->n_item].tipe = tipe;
        tb->item[tb->n_item].tombol = NULL;
        tb->item[tb->n_item].id = tb->id_berikutnya;
        tb->item[tb->n_item].cb_v = NULL;
        tb->item[tb->n_item].cb_id = NULL;
        tb->item[tb->n_item].ctx = NULL;
        id = tb->id_berikutnya;
        tb->id_berikutnya++;
        if (tipe == PG_TB_TOMBOL) {
                int w;
                pg_tombol_t *bt = pg_buat_tombol(label, tb->font);
                if (!bt) return -1;
                tb->item[tb->n_item].tombol = bt;
                /* Sinkronkan universal anak list. */
                pg_widget_tambah_anak(&tb->base,
                        pg_tombol_widget(bt));
                /* Hitung lebar natural label. */
                if (tb->font && label)
                        w = pg_font_lebar_teks(tb->font, label)
                                + 2 * PG_TB_TOMBOL_PAD;
                else
                        w = 60;
                if (w < 24) w = 24;
                tb->item[tb->n_item].lebar_min = w;
                pg_tombol_saatklik(bt, pg_tb_tombol_cb,
                        &tb->item[tb->n_item]);
        } else if (tipe == PG_TB_PEMISAH) {
                tb->item[tb->n_item].lebar_min = PG_TB_PEMISAH_W;
        } else {
                tb->item[tb->n_item].lebar_min = 0;
        }
        tb->n_item++;
        pg_widget_kotor(&tb->base);
        return id;
}

int pg_tb_tambah_tombol(pg_toolbar_t *tb, const char *label,
    void (*cb)(void*), void *ctx)
{
        int id = pg_tb_tambah_item(tb, PG_TB_TOMBOL, label);
        if (id < 0) return id;
        tb->item[tb->n_item - 1].cb_v = cb;
        tb->item[tb->n_item - 1].ctx = ctx;
        return id;
}

int pg_tb_tambah_tombol_v(pg_toolbar_t *tb, const char *label,
    pg_tb_cb cb, void *ctx)
{
        int id = pg_tb_tambah_item(tb, PG_TB_TOMBOL, label);
        if (id < 0) return id;
        tb->item[tb->n_item - 1].cb_id = cb;
        tb->item[tb->n_item - 1].ctx = ctx;
        return id;
}

void pg_tb_tambah_pemisah(pg_toolbar_t *tb)
{
        pg_tb_tambah_item(tb, PG_TB_PEMISAH, NULL);
}

void pg_tb_tambah_spasi(pg_toolbar_t *tb)
{
        pg_tb_tambah_item(tb, PG_TB_SPASI, NULL);
}

void pg_tb_setel_font(pg_toolbar_t *tb, pg_font_t *font)
{
        int i;
        if (!tb) return;
        tb->font = font;
        /* Update lebar natural semua tombol. */
        for (i = 0; i < tb->n_item; i++) {
                if (tb->item[i].tipe == PG_TB_TOMBOL && tb->font) {
                        const char *lbl = ""; /* tidak disimpan */
                        (void)lbl;
                        /* Lebar tetap dari layout saat ini. */
                }
        }
        pg_widget_kotor(&tb->base);
}

pg_widget_t *pg_toolbar_widget(pg_toolbar_t *tb)
{
        return tb ? &tb->base : NULL;
}

/* Ambil pointer tombol berdasarkan id (return dari pg_tb_tambah_tombol). */
pg_tombol_t *pg_tb_ambil_tombol(pg_toolbar_t *tb, int id)
{
        int i;
        if (!tb) return NULL;
        for (i = 0; i < tb->n_item; i++) {
                if (tb->item[i].tipe == PG_TB_TOMBOL &&
                    tb->item[i].id == id)
                        return tb->item[i].tombol;
        }
        return NULL;
}

/* Setel icon untuk tombol berdasarkan id. */
void pg_tb_setel_icon(pg_toolbar_t *tb, int id, pg_permukaan_t *icon)
{
        pg_tombol_t *t = pg_tb_ambil_tombol(tb, id);
        if (t) pg_tombol_setel_icon(t, icon);
}

/* Setel orientasi toolbar. */
void pg_tb_setel_vertikal(pg_toolbar_t *tb, pg_bool vertikal)
{
        if (!tb) return;
        tb->vertikal = vertikal ? PG_BENAR : PG_SALAH;
        /* Re-layout supaya item posisi sesuai orientasi baru. */
        pg_tb_layout(tb);
        pg_widget_kotor(&tb->base);
}

/* Ambil orientasi saat ini. */
pg_bool pg_tb_vertikal(const pg_toolbar_t *tb)
{
        return tb ? tb->vertikal : PG_SALAH;
}
