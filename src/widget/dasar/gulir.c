/* ----------------------------------------------------------------------------------------------
 * pigura widget: gulir.c - widget scroll view dengan scrollbar
 * ----------------------------------------------------------------------------------------------
 * Gulir adalah kontainer dengan satu anak berukuran lebih besar dari
 * viewport. Anak di-blit ke permukaan gulir via sub-rect
 * (gulir_x, gulir_y, vp_w, vp_h); viewport otomatis diklem ke
 * (0, 0, w-bar, h-bar). Scrollbar vertikal/horizontal tampak bila
 * konten > viewport; thumb tinggi/lebar proporsional ke
 * viewport/konten; thumb posisi proporsional ke gulir/konten.
 *
 * Interaksi:
 *   - Roda mouse: gulir_y +/- 24 piksel, diklem ke [0, maks].
 *   - Klik kiri pada thumb: mulai seret. GERAK ubah gulir
 *     proporsional ke thumb posisi baru.
 *   - Klik kiri pada track (di luar thumb): lompat satu halaman.
 *   - Klik di viewport: teruskan ke anak dengan offset digulir.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/gulir.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/aksi.h"
#include "pigura/widget.h"

#include <stdlib.h>

/* Lebar scrollbar vertikal & tinggi scrollbar horizontal. */
#define PG_GULIR_BAR 12

/* Loncatan roda mouse per takik (piksel). */
#define PG_GULIR_RODA_LONCATAN 24

struct pg_gulir {
        pg_widget_t  base;
        pg_widget_t *anak;
        int          konten_w;
        int          konten_h;
        int          gulir_x;
        int          gulir_y;
        pg_warna_t   latar;
        pg_bool      menyeret_v;
        pg_bool      menyeret_h;
        int          seret_ofs_v;
        int          seret_ofs_h;
        /* Hover state untuk visual feedback thumb. */
        pg_bool      hover_v;   /* mouse di atas thumb vertikal */
        pg_bool      hover_h;   /* mouse di atas thumb horizontal */
};

static pg_gulir_t *pg_gulir_dari(pg_widget_t *w)
{
        return (pg_gulir_t *)w;
}

/* Hitung viewport & flag scrollbar berdasarkan ukuran widget dan
 * konten. Hasil: vp_w, vp_h, need_vbar, need_hbar. */
static void pg_gulir_hitung_viewport(pg_gulir_t *g, int sw, int sh,
                                       int *vp_w, int *vp_h,
                                       pg_bool *need_vbar,
                                       pg_bool *need_hbar)
{
        int vw, vh, nv, nh;
        vw = sw - PG_GULIR_BAR;
        vh = sh - PG_GULIR_BAR;
        if (vw < 0) vw = 0;
        if (vh < 0) vh = 0;
        nv = (g->konten_h > vh) ? PG_BENAR : PG_SALAH;
        nh = (g->konten_w > vw) ? PG_BENAR : PG_SALAH;
        /* Bila salah satu scrollbar tidak dibutuhkan, area yang
         * dialokasikan untuk scrollbar itu dipulihkan ke viewport. */
        if (!nv) vw = sw;
        if (!nh) vh = sh;
        *vp_w = vw;
        *vp_h = vh;
        *need_vbar = nv;
        *need_hbar = nh;
}

/* Hitung geometri thumb vertikal. */
static void pg_gulir_thumb_v(pg_gulir_t *g, int vp_h,
                              int *thumb_h, int *thumb_y)
{
        int th = (vp_h * vp_h) / g->konten_h;
        int denom;
        if (th < PG_GULIR_BAR) th = PG_GULIR_BAR;
        if (th > vp_h) th = vp_h;
        denom = g->konten_h - vp_h;
        if (denom > 0)
                *thumb_y = (g->gulir_y * (vp_h - th)) / denom;
        else
                *thumb_y = 0;
        *thumb_h = th;
}

/* Hitung geometri thumb horizontal. */
static void pg_gulir_thumb_h(pg_gulir_t *g, int vp_w,
                              int *thumb_w, int *thumb_x)
{
        int tw = (vp_w * vp_w) / g->konten_w;
        int denom;
        if (tw < PG_GULIR_BAR) tw = PG_GULIR_BAR;
        if (tw > vp_w) tw = vp_w;
        denom = g->konten_w - vp_w;
        if (denom > 0)
                *thumb_x = (g->gulir_x * (vp_w - tw)) / denom;
        else
                *thumb_x = 0;
        *thumb_w = tw;
}

/* Klem scroll ke rentang valid. */
static void pg_gulir_klem(pg_gulir_t *g, int vp_w, int vp_h)
{
        int maks_y = g->konten_h - vp_h;
        int maks_x = g->konten_w - vp_w;
        if (maks_y < 0) maks_y = 0;
        if (maks_x < 0) maks_x = 0;
        if (g->gulir_y < 0) g->gulir_y = 0;
        if (g->gulir_y > maks_y) g->gulir_y = maks_y;
        if (g->gulir_x < 0) g->gulir_x = 0;
        if (g->gulir_x > maks_x) g->gulir_x = maks_x;
}

/* vtable: catat viewport + scrollbar. */
static void pg_gulir_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
        pg_gulir_t *g = pg_gulir_dari(w);
        int sw, sh, vp_w, vp_h;
        pg_bool need_vbar, need_hbar;

        sw = pg_permukaan_lebar(s);
        sh = pg_permukaan_tinggi(s);
        pg_gulir_hitung_viewport(g, sw, sh, &vp_w, &vp_h,
                &need_vbar, &need_hbar);
        pg_gulir_klem(g, vp_w, vp_h);

        /* Latar (seluruh permukaan gulir). */
        pg_isi_permukaan(s, g->latar);

        /* Render anak via pg_widget_catat (latar cerdas + alpha blend).
         * pg_widget_catat akan render child ke permukaan child sendiri
         * (ukuran anak->kotak), lalu blit ke dest (permukaan sementara).
         *
         * Tapi kita tidak mau blit ke permukaan gulir langsung (ukuran
         * viewport berbeda dari ukuran anak). Kita render anak ke
         * permukaan anak sendiri dulu, lalu blit sub-rect (gulir_x,
         * gulir_y, vp_w, vp_h) ke permukaan gulir.
         *
         * Trick: panggil pg_widget_catat dengan permukaan dummy yang
         * ukurannya = ukuran anak. Tapi kita sudah punya permukaan anak
         * di anak->permukaan. Kita hanya perlu render ulang bila kotor.
         *
         * Solusi: gunakan pg_widget_catat untuk render anak ke vp,
         * tapi paksa ukuran vp = ukuran anak supaya semua muat. Lalu
         * blit sub-rect ke permukaan gulir. */
        if (g->anak) {
                /* Render anak ke permukaan sementara ukuran anak. */
                int cw = g->anak->kotak.w;
                int ch = g->anak->kotak.h;
                if (cw <= 0) cw = vp_w;
                if (ch <= 0) ch = vp_h;
                {
                        pg_permukaan_t *cp;
                        cp = pg_buat_permukaan(cw, ch);
                        if (cp) {
                                /* Render anak ke cp — pg_widget_catat
                                 * akan isi latar + render + blit. */
                                /* Simpan posisi anak, set ke (0,0). */
                                int ax = g->anak->kotak.x;
                                int ay = g->anak->kotak.y;
                                g->anak->kotak.x = 0;
                                g->anak->kotak.y = 0;
                                pg_widget_catat(g->anak, cp);
                                g->anak->kotak.x = ax;
                                g->anak->kotak.y = ay;
                                /* Blit sub-rect (gulir_x, gulir_y, vp, vp)
                                 * ke permukaan gulir. */
                                pg_blit_sub_permukaan(s, 0, 0, cp,
                                        pg_buat_kotak(g->gulir_x,
                                                g->gulir_y, vp_w, vp_h));
                                pg_hancur_permukaan(cp);
                        }
                }
        }

        /* Scrollbar vertikal.
         * Track: PANEL (#E6E6E6) — sama dengan latar widget lain.
         * Thumb: ABU_TERANG (#C0C0C0) idle, #969696 hover, #707070 tekan.
         * Radius thumb 3 supaya rounded (Cairo-quality via shared SDF). */
        if (need_vbar) {
                int bar_x = sw - PG_GULIR_BAR;
                int thumb_h, thumb_y;
                pg_warna_t thumb_warna;
                pg_gulir_thumb_v(g, vp_h, &thumb_h, &thumb_y);
                /* Track. */
                pg_isi_permukaan_kotak(s,
                        pg_buat_kotak(bar_x, 0, PG_GULIR_BAR, vp_h),
                        PG_WARNA_PANEL);
                /* Thumb color based on state. */
                if (g->menyeret_v)
                        thumb_warna = PG_RGB(0x70, 0x70, 0x70);
                else if (g->hover_v)
                        thumb_warna = PG_WARNA_HOVER_OUTLINE; /* #969696 */
                else
                        thumb_warna = PG_ABU_TERANG;          /* #C0C0C0 */
                /* Thumb dengan rounded rect AA (radius 3). */
                pg_gambar_kotak_tumpul_isi_garis_aa(s,
                        pg_buat_kotak(bar_x + 2, thumb_y,
                                       PG_GULIR_BAR - 4, thumb_h),
                        3, thumb_warna, thumb_warna);
        }

        /* Scrollbar horizontal. */
        if (need_hbar) {
                int bar_y = sh - PG_GULIR_BAR;
                int thumb_w, thumb_x;
                pg_warna_t thumb_warna;
                pg_gulir_thumb_h(g, vp_w, &thumb_w, &thumb_x);
                /* Track. */
                pg_isi_permukaan_kotak(s,
                        pg_buat_kotak(0, bar_y, vp_w, PG_GULIR_BAR),
                        PG_WARNA_PANEL);
                /* Thumb color based on state. */
                if (g->menyeret_h)
                        thumb_warna = PG_RGB(0x70, 0x70, 0x70);
                else if (g->hover_h)
                        thumb_warna = PG_WARNA_HOVER_OUTLINE;
                else
                        thumb_warna = PG_ABU_TERANG;
                pg_gambar_kotak_tumpul_isi_garis_aa(s,
                        pg_buat_kotak(thumb_x, bar_y + 2,
                                       thumb_w, PG_GULIR_BAR - 4),
                        3, thumb_warna, thumb_warna);
        }

        /* Pojok kanan-bawah bila keduanya butuh scrollbar. */
        if (need_vbar && need_hbar) {
                pg_isi_permukaan_kotak(s,
                        pg_buat_kotak(sw - PG_GULIR_BAR,
                                       sh - PG_GULIR_BAR,
                                       PG_GULIR_BAR, PG_GULIR_BAR),
                        PG_WARNA_PANEL);
        }
}

static pg_bool pg_gulir_peristiwa_v(pg_widget_t *w,
                                     const pg_aksi_t *e)
{
        pg_gulir_t *g = pg_gulir_dari(w);
        int sw, sh, vp_w, vp_h;
        pg_bool need_vbar, need_hbar;
        int maks_y, maks_x;

        sw = w->kotak.w;
        sh = w->kotak.h;
        pg_gulir_hitung_viewport(g, sw, sh, &vp_w, &vp_h,
                &need_vbar, &need_hbar);
        maks_y = g->konten_h - vp_h;
        if (maks_y < 0) maks_y = 0;
        maks_x = g->konten_w - vp_w;
        if (maks_x < 0) maks_x = 0;

        /* Roda mouse: gulir vertikal. */
        if (e->tipe == PG_AKSI_TETIKUS_GULIR) {
                int step = PG_GULIR_RODA_LONCATAN;
                int baru = g->gulir_y - e->roda_dy * step;
                if (baru < 0) baru = 0;
                if (baru > maks_y) baru = maks_y;
                if (baru != g->gulir_y) {
                        g->gulir_y = baru;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }

        /* GERAK (tanpa klik): update hover state thumb. */
        if (e->tipe == PG_AKSI_TETIKUS_GERAK &&
            !g->menyeret_v && !g->menyeret_h) {
                int mx = e->tetik_pos.x;
                int my = e->tetik_pos.y;
                pg_bool new_hover_v = PG_SALAH;
                pg_bool new_hover_h = PG_SALAH;
                if (need_vbar && mx >= sw - PG_GULIR_BAR && my < vp_h) {
                        int thumb_h, thumb_y;
                        pg_gulir_thumb_v(g, vp_h, &thumb_h, &thumb_y);
                        if (my >= thumb_y && my < thumb_y + thumb_h)
                                new_hover_v = PG_BENAR;
                }
                if (need_hbar && my >= sh - PG_GULIR_BAR && mx < vp_w) {
                        int thumb_w, thumb_x;
                        pg_gulir_thumb_h(g, vp_w, &thumb_w, &thumb_x);
                        if (mx >= thumb_x && mx < thumb_x + thumb_w)
                                new_hover_h = PG_BENAR;
                }
                if (g->hover_v != new_hover_v ||
                    g->hover_h != new_hover_h) {
                        g->hover_v = new_hover_v;
                        g->hover_h = new_hover_h;
                        pg_widget_kotor(w);
                }
                /* Jangan consume — biarkan event lanjut ke anak via
                 * bottom fallthrough. */
        }

        /* Klik kiri: cek scrollbar atau teruskan ke anak. */
        if (e->tipe == PG_AKSI_TETIKUS_TEKAN &&
            e->tetik_tombol == PG_TETIKUS_KIRI) {
                int mx = e->tetik_pos.x;
                int my = e->tetik_pos.y;
                /* Thumb vertikal? */
                if (need_vbar && mx >= sw - PG_GULIR_BAR && my < vp_h) {
                        int thumb_h, thumb_y;
                        pg_gulir_thumb_v(g, vp_h, &thumb_h, &thumb_y);
                        if (my >= thumb_y && my < thumb_y + thumb_h) {
                                g->menyeret_v = PG_BENAR;
                                g->seret_ofs_v = my - thumb_y;
                        } else {
                                int lompat = (my < thumb_y) ?
                                        -vp_h : vp_h;
                                int baru = g->gulir_y + lompat;
                                if (baru < 0) baru = 0;
                                if (baru > maks_y) baru = maks_y;
                                g->gulir_y = baru;
                        }
                        pg_widget_kotor(w);
                        return PG_BENAR;
                }
                /* Thumb horizontal? */
                if (need_hbar && my >= sh - PG_GULIR_BAR && mx < vp_w) {
                        int thumb_w, thumb_x;
                        pg_gulir_thumb_h(g, vp_w, &thumb_w, &thumb_x);
                        if (mx >= thumb_x && mx < thumb_x + thumb_w) {
                                g->menyeret_h = PG_BENAR;
                                g->seret_ofs_h = mx - thumb_x;
                        } else {
                                int lompat = (mx < thumb_x) ?
                                        -vp_w : vp_w;
                                int baru = g->gulir_x + lompat;
                                if (baru < 0) baru = 0;
                                if (baru > maks_x) baru = maks_x;
                                g->gulir_x = baru;
                        }
                        pg_widget_kotor(w);
                        return PG_BENAR;
                }
        }

        /* Drag thumb vertikal sedang aktif. */
        if (e->tipe == PG_AKSI_TETIKUS_GERAK && g->menyeret_v) {
                int thumb_h, thumb_y, denom;
                int baru = 0;
                pg_gulir_thumb_v(g, vp_h, &thumb_h, &thumb_y);
                denom = vp_h - thumb_h;
                thumb_y = e->tetik_pos.y - g->seret_ofs_v;
                if (denom > 0)
                        baru = (thumb_y * maks_y) / denom;
                if (baru < 0) baru = 0;
                if (baru > maks_y) baru = maks_y;
                if (baru != g->gulir_y) {
                        g->gulir_y = baru;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }

        /* Drag thumb horizontal sedang aktif. */
        if (e->tipe == PG_AKSI_TETIKUS_GERAK && g->menyeret_h) {
                int thumb_w, thumb_x, denom;
                int baru = 0;
                pg_gulir_thumb_h(g, vp_w, &thumb_w, &thumb_x);
                denom = vp_w - thumb_w;
                thumb_x = e->tetik_pos.x - g->seret_ofs_h;
                if (denom > 0)
                        baru = (thumb_x * maks_x) / denom;
                if (baru < 0) baru = 0;
                if (baru > maks_x) baru = maks_x;
                if (baru != g->gulir_x) {
                        g->gulir_x = baru;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }

        /* Lepas klik: akhiri drag. */
        if (e->tipe == PG_AKSI_TETIKUS_LEPAS &&
            (g->menyeret_v || g->menyeret_h)) {
                g->menyeret_v = PG_SALAH;
                g->menyeret_h = PG_SALAH;
                /* Reset hover juga supaya thumb kembali ke warna idle
                 * (mouse mungkin sudah di luar thumb saat lepas). */
                g->hover_v = PG_SALAH;
                g->hover_h = PG_SALAH;
                pg_widget_kotor(w);
                return PG_BENAR;
        }

        /* Klik di viewport: teruskan ke anak dengan offset digulir. */
        if (g->anak) {
                pg_aksi_t e2 = *e;
                e2.tetik_pos.x += g->gulir_x;
                e2.tetik_pos.y += g->gulir_y;
                return pg_widget_tangani_aksi(g->anak, &e2);
        }
        return PG_SALAH;
}

/* Deallocator struktur turunan — dipanggil oleh
 * pg_widget_hancur_penuh() setelah hancur_v. */
static void pg_gulir_bebas_v(pg_widget_t *w)
{
        free(w);
}

static const pg_widget_vtable_t pg_gulir_vtable = {
        pg_gulir_catat_v,
        pg_gulir_peristiwa_v,
        NULL,
        NULL,
        NULL,
        NULL,
        pg_gulir_bebas_v
};

/* ---------------------------------------------------------------- API */

pg_gulir_t *pg_buat_gulir(void)
{
        pg_gulir_t *g;
        g = (pg_gulir_t *)calloc(1, sizeof(*g));
        if (!g) return NULL;
        pg_widget_init(&g->base, PG_WIDGET_GULIR,
                       &pg_gulir_vtable);
        g->latar = PG_PUTIH;
        pg_widget_setel_latar(&g->base, g->latar);
        return g;
}

void pg_gulir_hancur(pg_gulir_t *g)
{
        if (!g) return;
        pg_widget_hancur(&g->base);
        free(g);
}

void pg_gulir_setel_anak(pg_gulir_t *g, pg_widget_t *anak,
                         int konten_w, int konten_h)
{
        if (!g) return;
        /* Lepas anak lama dari universal anak list. */
        if (g->anak)
                pg_widget_hapus_anak(&g->base, g->anak);
        g->anak = anak;
        /* Tambah anak baru ke universal anak list supaya
         * pg_widget_kotor propagate dengan benar. */
        if (anak) {
                pg_widget_tambah_anak(&g->base, anak);
                /* Tandai anak kotor supaya pg_gulir_catat_v
                 * render anak di frame pertama. */
                pg_widget_kotor(anak);
        }
        g->konten_w = konten_w;
        g->konten_h = konten_h;
        if (g->gulir_x < 0) g->gulir_x = 0;
        if (g->gulir_y < 0) g->gulir_y = 0;
        if (g->konten_w > 0 && g->gulir_x > g->konten_w)
                g->gulir_x = g->konten_w;
        if (g->konten_h > 0 && g->gulir_y > g->konten_h)
                g->gulir_y = g->konten_h;
        pg_widget_kotor(&g->base);
}

void pg_gulir_gulir_ke(pg_gulir_t *g, int x, int y)
{
        if (!g) return;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (g->konten_w > 0 && x > g->konten_w) x = g->konten_w;
        if (g->konten_h > 0 && y > g->konten_h) y = g->konten_h;
        if (x == g->gulir_x && y == g->gulir_y) return;
        g->gulir_x = x;
        g->gulir_y = y;
        pg_widget_kotor(&g->base);
}

pg_widget_t *pg_gulir_widget(pg_gulir_t *g)
{
        return g ? &g->base : NULL;
}

/* Update ukuran konten tanpa set anak ulang. */
void pg_gulir_setel_ukuran_konten(pg_gulir_t *g, int w, int h)
{
        if (!g) return;
        g->konten_w = w;
        g->konten_h = h;
        pg_widget_kotor(&g->base);
}
