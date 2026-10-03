/* ----------------------------------------------------------------------------------------------
 * pigura widget: kolaps.c - panel collapsible
 * ----------------------------------------------------------------------------------------------
 * Kolaps adalah kontainer dengan title bar yang bisa di-collapse.
 * Tombol [▼]/[▶] di kiri title bar untuk toggle.
 *
 * Saat collapse: tinggi = title_h saja.
 * Saat expand:   tinggi = title_h + child.h.
 *
 * Implementasi: saat collapse=BENAR, child tidak di-render dan
 * kotak widget di-set ke (x, y, w, title_h). Saat expand, kotak
 * di-set ke (x, y, w, title_h + child.h). Layout manager parent
 * akan menata ulang sibling.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/kolaps.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/peristiwa.h"
#include "pigura/widget.h"

#include <stdlib.h>
#include <string.h>

#define PG_KOLAPS_TITLE_H 24
#define PG_KOLAPS_TOMBOL_W 18

struct pg_kolaps {
        pg_widget_t   base;
        char         *judul;
        pg_widget_t  *anak;
        pg_font_t    *font;
        pg_bool       kolaps;
        int           title_h;
        int           anak_h;      /* tinggi child saat expand */
        pg_warna_t    judul_fg;
        pg_warna_t    judul_bg;
        pg_warna_t    batas;
        pg_warna_t    fg;
        pg_bool       hover_tombol;
        pg_bool       ditekan_tombol;
};

static char *pg_kolaps_dup(const char *s)
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

static pg_kolaps_t *pg_kolaps_dari(pg_widget_t *w)
{
        return (pg_kolaps_t *)w;
}

/* Hitung tinggi total: title_h (+ child saat expand). */
static int pg_kolaps_hitung_h(pg_kolaps_t *k)
{
        int h = k->title_h;
        if (!k->kolaps && k->anak)
                h += k->anak_h;
        return h;
}

/* vtable: catat title bar + tombol + child (jika expand). */
static void pg_kolaps_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
        pg_kolaps_t *k = pg_kolaps_dari(w);
        int          sw, sh, baseline, tombol_x;
        pg_kotak_t   title_r, tombol_r;
        sw = pg_permukaan_lebar(s);
        sh = pg_permukaan_tinggi(s);
        (void)sh;
        title_r = pg_buat_kotak(0, 0, sw, k->title_h);
        tombol_x = 2;
        tombol_r = pg_buat_kotak(tombol_x,
                (k->title_h - 16) / 2, 16, 16);
        /* Title bar background. */
        pg_isi_permukaan_kotak(s, title_r, k->judul_bg);
        /* Tombol [▼]/[▶]: latar kotak + segitiga. */
        {
                pg_warna_t bg;
                if (k->ditekan_tombol)    bg = PG_ABU_GELAP;
                else if (k->hover_tombol) bg = PG_ABU_TERANG;
                else                       bg = PG_ABU;
                pg_isi_permukaan_kotak(s, tombol_r, bg);
                pg_gambar_kotak_aa(s, tombol_r, k->batas);
        }
        /* Segitiga: ▼ bila expand, ▶ bila kolaps. */
        {
                int cx = tombol_r.x + tombol_r.w / 2;
                int cy = tombol_r.y + tombol_r.h / 2;
                pg_titik_t t[3];
                if (k->kolaps) {
                        /* ▶ : segitiga ke kanan. */
                        t[0] = pg_buat_titik(cx - 3, cy - 4);
                        t[1] = pg_buat_titik(cx - 3, cy + 4);
                        t[2] = pg_buat_titik(cx + 4, cy);
                } else {
                        /* ▼ : segitiga ke bawah. */
                        t[0] = pg_buat_titik(cx - 4, cy - 3);
                        t[1] = pg_buat_titik(cx + 4, cy - 3);
                        t[2] = pg_buat_titik(cx, cy + 4);
                }
                pg_gambar_poligon_isi_aa(s, t, 3, k->judul_fg);
        }
        /* Judul teks. */
        if (k->judul && k->font) {
                baseline = pg_font_baseline_tengah(k->font, k->title_h);
                pg_font_gambar_teks(k->font, k->judul, s,
                        tombol_x + 18, baseline, k->judul_fg);
        }
        /* Border. */
        pg_kotak_permukaan(s, pg_buat_kotak(0, 0, sw, k->title_h),
                k->batas);
        /* Body: child (jika expand). */
        if (!k->kolaps && k->anak) {
                int sx, sy;
                sx = k->anak->kotak.x;
                sy = k->anak->kotak.y;
                k->anak->kotak.x = 0;
                k->anak->kotak.y = k->title_h;
                pg_widget_catat(k->anak, s);
                k->anak->kotak.x = sx;
                k->anak->kotak.y = sy;
        }
}

static pg_bool pg_kolaps_peristiwa_v(pg_widget_t *w,
                                       const pg_peristiwa_t *e)
{
        pg_kolaps_t *k = pg_kolaps_dari(w);
        int          tombol_x = 2;
        int          tombol_y = (k->title_h - 16) / 2;
        int          tombol_w = 16, tombol_h = 16;
        if (e->tipe == PG_PERISTIWA_TETIK_GERAK) {
                int x = e->tetik_pos.x, y = e->tetik_pos.y;
                pg_bool hover = (x >= tombol_x &&
                        x < tombol_x + tombol_w &&
                        y >= tombol_y && y < tombol_y + tombol_h) ?
                        PG_BENAR : PG_SALAH;
                if (hover != k->hover_tombol) {
                        k->hover_tombol = hover;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }
        if (e->tipe == PG_PERISTIWA_TETIK_TURUN &&
            e->tetik_tombol == PG_TETIK_KIRI) {
                int x = e->tetik_pos.x, y = e->tetik_pos.y;
                if (x >= tombol_x && x < tombol_x + tombol_w &&
                    y >= tombol_y && y < tombol_y + tombol_h) {
                        k->ditekan_tombol = PG_BENAR;
                        pg_widget_kotor(w);
                        return PG_BENAR;
                }
                /* Klik title bar juga toggle. */
                if (y < k->title_h) {
                        pg_kolaps_toggle(k);
                        return PG_BENAR;
                }
                /* Body: teruskan ke child. */
                if (!k->kolaps && k->anak) {
                        pg_peristiwa_t e2 = *e;
                        e2.tetik_pos.y -= k->title_h;
                        return pg_widget_tangani_peristiwa(
                                k->anak, &e2);
                }
                return PG_SALAH;
        }
        if (e->tipe == PG_PERISTIWA_TETIK_NAIK &&
            e->tetik_tombol == PG_TETIK_KIRI) {
                int x = e->tetik_pos.x, y = e->tetik_pos.y;
                if (k->ditekan_tombol) {
                        k->ditekan_tombol = PG_SALAH;
                        if (x >= tombol_x && x < tombol_x + tombol_w &&
                            y >= tombol_y && y < tombol_y + tombol_h) {
                                pg_kolaps_toggle(k);
                        }
                        pg_widget_kotor(w);
                        return PG_BENAR;
                }
                if (!k->kolaps && k->anak) {
                        pg_peristiwa_t e2 = *e;
                        e2.tetik_pos.y -= k->title_h;
                        return pg_widget_tangani_peristiwa(
                                k->anak, &e2);
                }
                return PG_SALAH;
        }
        /* Peristiwa lain: teruskan ke child bila expand. */
        if (!k->kolaps && k->anak) {
                pg_peristiwa_t e2 = *e;
                e2.tetik_pos.y -= k->title_h;
                return pg_widget_tangani_peristiwa(k->anak, &e2);
        }
        return PG_SALAH;
}

/* vtable: saat ukuran widget berubah, atur ukuran child. */
static void pg_kolaps_ubah_ukuran_v(pg_widget_t *w, int w_, int h)
{
        pg_kolaps_t *k = pg_kolaps_dari(w);
        int          body_h;
        (void)h;
        if (k->anak) {
                body_h = k->anak_h;
                if (body_h <= 0) body_h = w->kotak.h - k->title_h;
                if (body_h < 0) body_h = 0;
                pg_widget_setel_kotak(k->anak,
                        pg_buat_kotak(0, k->title_h, w_, body_h));
        }
}

static void pg_kolaps_hancur_v(pg_widget_t *w)
{
        pg_kolaps_t *k = pg_kolaps_dari(w);
        if (k->judul) {
                free(k->judul);
                k->judul = NULL;
        }
}

/* Deallocator struktur turunan — dipanggil oleh
 * pg_widget_hancur_penuh() setelah hancur_v. */
static void pg_kolaps_bebas_v(pg_widget_t *w)
{
        free(w);
}

static const pg_widget_vtable_t pg_kolaps_vtable = {
        pg_kolaps_catat_v,
        pg_kolaps_peristiwa_v,
        pg_kolaps_ubah_ukuran_v,
        pg_kolaps_hancur_v,
        NULL,
        NULL,
        pg_kolaps_bebas_v
};

/* ---------------------------------------------------------------- API */

pg_kolaps_t *pg_buat_kolaps(const char *judul, pg_font_t *font)
{
        pg_kolaps_t *k;
        int           h_font;
        k = (pg_kolaps_t *)calloc(1, sizeof(*k));
        if (!k) return NULL;
        pg_widget_init(&k->base, PG_WIDGET_KOTAK,
                       &pg_kolaps_vtable);
        /* Set milik=BENAR agar child (setel_anak) dihancurkan
         * otomatis via universal anak list. */
        pg_widget_milik(&k->base, PG_BENAR);
        k->font = font;
        k->kolaps = PG_SALAH;
        h_font = font ? pg_font_tinggi(font) : 8;
        if (h_font <= 0) h_font = 8;
        k->title_h = h_font + 12;
        if (k->title_h < PG_KOLAPS_TITLE_H) k->title_h = PG_KOLAPS_TITLE_H;
        k->anak_h = 100;
        k->judul_fg = PG_PUTIH;
        k->judul_bg = PG_BIRU;
        k->batas = PG_ABU_GELAP;
        k->fg = PG_HITAM;
        if (judul) {
                k->judul = pg_kolaps_dup(judul);
                if (!k->judul) {
                        free(k);
                        return NULL;
                }
        }
        pg_widget_setel_kotak(&k->base, pg_buat_kotak(0, 0, 200,
                k->title_h + k->anak_h));
        pg_widget_setel_latar(&k->base, PG_ABU_TERANG);
        pg_widget_setel_ukuran_min(&k->base, 80, k->title_h);
        return k;
}

void pg_kolaps_hancur(pg_kolaps_t *k)
{
        if (!k) return;
        pg_widget_hancur(&k->base);
        free(k);
}

void pg_kolaps_setel_anak(pg_kolaps_t *k, pg_widget_t *child)
{
        int h_baru;
        if (!k) return;
        k->anak = child;
        if (child) {
                /* Ambil tinggi natural child dari min_h bila sudah
                 * disetel; jika tidak pakai 100. */
                k->anak_h = child->min_h > 0 ? child->min_h : 100;
                pg_widget_setel_kotak(child,
                        pg_buat_kotak(0, k->title_h,
                                k->base.kotak.w, k->anak_h));
                /* Sinkronkan universal anak list. */
                pg_widget_tambah_anak(&k->base, child);
        }
        /* Resize diri sendiri: tinggi = title_h (+ child bila expand). */
        h_baru = pg_kolaps_hitung_h(k);
        if (k->base.kotak.h != h_baru)
                pg_widget_ubah_ukuran(&k->base, k->base.kotak.w,
                        h_baru);
        pg_widget_kotor(&k->base);
}

void pg_kolaps_setel_kolaps(pg_kolaps_t *k, pg_bool kolaps)
{
        int h_baru;
        if (!k) return;
        kolaps = kolaps ? PG_BENAR : PG_SALAH;
        if (k->kolaps == kolaps) return;
        k->kolaps = kolaps;
        h_baru = pg_kolaps_hitung_h(k);
        pg_widget_ubah_ukuran(&k->base, k->base.kotak.w, h_baru);
        pg_widget_kotor(&k->base);
}

pg_bool pg_kolaps_apakah_kolaps(const pg_kolaps_t *k)
{
        return k ? k->kolaps : PG_SALAH;
}

void pg_kolaps_toggle(pg_kolaps_t *k)
{
        if (!k) return;
        pg_kolaps_setel_kolaps(k, !k->kolaps);
}

void pg_kolaps_setel_judul(pg_kolaps_t *k, const char *judul)
{
        char *baru;
        if (!k) return;
        baru = pg_kolaps_dup(judul);
        if (judul && !baru) return;
        if (k->judul) free(k->judul);
        k->judul = baru;
        pg_widget_kotor(&k->base);
}

pg_widget_t *pg_kolaps_widget(pg_kolaps_t *k)
{
        return k ? &k->base : NULL;
}

int pg_kolaps_title_h(const pg_kolaps_t *k)
{
        return k ? k->title_h : 0;
}
