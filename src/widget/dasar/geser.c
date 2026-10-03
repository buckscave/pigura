/* ----------------------------------------------------------------------------------------------
 * pigura widget: geser.c - widget slider integer
 * ----------------------------------------------------------------------------------------------
 * Geser adalah slider horizontal yang merepresentasikan nilai
 * integer dalam rentang [min, maks]. Pengguna menyeret kenop atau
 * mengklik track untuk mengubah nilai; callback dipanggil saat nilai
 * berubah.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/geser.h"
#include "pigura/permukaan.h"
#include "pigura/peristiwa.h"
#include "pigura/widget.h"

#include <stdlib.h>

struct pg_geser {
        pg_widget_t  base;
        int          min;
        int          maks;
        int          nilai;
        pg_bool      menyeret;
        pg_warna_t   fg;
        pg_warna_t   latar;
        pg_warna_t   kenop;
        pg_geser_cb  cb;
        void        *ctx;
};

static pg_geser_t *pg_geser_dari(pg_widget_t *w)
{
        return (pg_geser_t *)w;
}

/* Lebar kenop dalam piksel. */
#define PG_GESER_KNOP_W 8

/* Hitung nilai dari posisi x (relatif widget). */
static int pg_geser_hitung(pg_geser_t *g, int x, int sw)
{
        int rentang, usable, nilai;
        rentang = g->maks - g->min;
        if (rentang <= 0) return g->min;
        usable = sw - 4 - PG_GESER_KNOP_W;
        if (usable <= 0) return g->min;
        nilai = g->min + (x - 2) * rentang / usable;
        if (nilai < g->min) nilai = g->min;
        if (nilai > g->maks) nilai = g->maks;
        return nilai;
}

/* vtable: isi latar + track + kenop. */
static void pg_geser_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
        pg_geser_t *g = pg_geser_dari(w);
        int sw, sh, track_y, knop_x, rentang;
        sw = pg_permukaan_lebar(s);
        sh = pg_permukaan_tinggi(s);
        pg_isi_permukaan(s, g->latar);
        track_y = sh / 2;
        pg_garis_h_permukaan(s, 2, sw - 3, track_y, g->fg);
        rentang = g->maks - g->min;
        if (rentang <= 0) knop_x = 2;
        else
                knop_x = 2 + (g->nilai - g->min) *
                        (sw - 4 - PG_GESER_KNOP_W) / rentang;
        pg_isi_permukaan_kotak(s,
                pg_buat_kotak(knop_x, track_y - 4,
                               PG_GESER_KNOP_W, 8),
                g->kenop);
}

static pg_bool pg_geser_peristiwa_v(pg_widget_t *w,
                                    const pg_peristiwa_t *e)
{
        pg_geser_t *g = pg_geser_dari(w);
        int sw, nilai_baru;
        if (e->tipe == PG_PERISTIWA_TETIK_TURUN &&
            e->tetik_tombol == PG_TETIK_KIRI) {
                g->menyeret = PG_BENAR;
                sw = w->kotak.w;
                /* e->tetik_pos sudah dikonversi ke lokal widget. */
                nilai_baru = pg_geser_hitung(g, e->tetik_pos.x, sw);
                if (nilai_baru != g->nilai) {
                        g->nilai = nilai_baru;
                        if (g->cb) g->cb(g, g->nilai, g->ctx);
                }
                pg_widget_kotor(w);
                return PG_BENAR;
        }
        if (e->tipe == PG_PERISTIWA_TETIK_NAIK &&
            e->tetik_tombol == PG_TETIK_KIRI) {
                g->menyeret = PG_SALAH;
                pg_widget_kotor(w);
                return PG_BENAR;
        }
        if (e->tipe == PG_PERISTIWA_TETIK_GERAK && g->menyeret) {
                sw = w->kotak.w;
                nilai_baru = pg_geser_hitung(g, e->tetik_pos.x, sw);
                if (nilai_baru != g->nilai) {
                        g->nilai = nilai_baru;
                        pg_widget_kotor(w);
                        if (g->cb) g->cb(g, g->nilai, g->ctx);
                }
                return PG_BENAR;
        }
        return PG_SALAH;
}

/* Deallocator struktur turunan — dipanggil oleh
 * pg_widget_hancur_penuh() setelah hancur_v. */
static void pg_geser_bebas_v(pg_widget_t *w)
{
	free(w);
}

static const pg_widget_vtable_t pg_geser_vtable = {
        pg_geser_catat_v,
        pg_geser_peristiwa_v,
        NULL,
        NULL,
        NULL,
        NULL,
	pg_geser_bebas_v
};

/* ---------------------------------------------------------------- API */

pg_geser_t *pg_buat_geser(int min, int maks, int nilai)
{
        pg_geser_t *g;
        g = (pg_geser_t *)calloc(1, sizeof(*g));
        if (!g) return NULL;
        pg_widget_init(&g->base, PG_WIDGET_GESER,
                       &pg_geser_vtable);
        g->min = min;
        g->maks = maks;
        if (g->maks < g->min) g->maks = g->min;
        g->nilai = nilai;
        if (g->nilai < g->min) g->nilai = g->min;
        if (g->nilai > g->maks) g->nilai = g->maks;
        g->fg = PG_HITAM;
        g->latar = PG_ABU_TERANG;
        g->kenop = PG_ABU_GELAP;
        return g;
}

void pg_geser_hancur(pg_geser_t *g)
{
        if (!g) return;
        pg_widget_hancur(&g->base);
        free(g);
}

int pg_geser_nilai(pg_geser_t *g)
{
        return g ? g->nilai : 0;
}

void pg_geser_setel_nilai(pg_geser_t *g, int nilai)
{
        if (!g) return;
        if (nilai < g->min) nilai = g->min;
        if (nilai > g->maks) nilai = g->maks;
        if (nilai == g->nilai) return;
        g->nilai = nilai;
        pg_widget_kotor(&g->base);
}

void pg_geser_saatberubah(pg_geser_t *g, pg_geser_cb cb, void *ctx)
{
        if (!g) return;
        g->cb = cb;
        g->ctx = ctx;
}

pg_widget_t *pg_geser_widget(pg_geser_t *g)
{
        return g ? &g->base : NULL;
}
