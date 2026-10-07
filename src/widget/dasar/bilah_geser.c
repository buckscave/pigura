/* ----------------------------------------------------------------------------------------------
 * pigura widget: bilah_geser.c - widget slider (bilah geser) integer
 * ----------------------------------------------------------------------------------------------
 * Slider horizontal dengan knob lingkaran yang bisa diseret. Track
 * pakai rounded rect AA, knob pakai ellipse AA. State visual lengkap
 * (idle/hover/tekan/fokus/disabled) konsisten dengan tema Batch 1.
 *
 * Interaksi:
 *   - Klik kiri pada knob: mulai seret. GERAK ubah nilai proporsional.
 *   - Klik kiri pada track: lompat ke posisi klik.
 *   - Keyboard (bila fokus): Panah Kiri/Kanan = -/+1 langkah.
 *                              Home/End = min/maks.
 *
 * Layout:
 *   Track: y tengah, height 4px (radius 2).
 *   Knob : ellipse radius 6 (untuk AA halus pakai SS adaptif).
 *   Padding: 8px di kiri dan kanan supaya knob tidak mentok tepi.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/bilah_geser.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/aksi.h"
#include "pigura/widget.h"

#include <stdlib.h>

/* Dimensi default. */
#define PG_BG_KNOP_R      6     /* radius knob (lingkaran) */
#define PG_BG_TRACK_H    4     /* tinggi track */
#define PG_BG_TRACK_RAD  2     /* radius track */
#define PG_BG_PAD        8     /* padding kiri/kanan supaya knob tidak mentok */

struct pg_bilah_geser {
        pg_widget_t   base;
        int           min;
        int           maks;
        int           nilai;
        int           langkah;     /* inkrement keyboard (default 1) */
        pg_bool       menyeret;
        pg_bool       hover_knop;  /* mouse di atas knob */
        pg_bool       hover_track; /* mouse di track (tidak di knob) */
        pg_warna_t    fg;
        pg_warna_t    latar;
        pg_warna_t    batas;
        pg_warna_t    kenop;
        pg_bilah_geser_cb cb;
        void         *ctx;
};

static pg_bilah_geser_t *pg_bg_dari(pg_widget_t *w)
{
        return (pg_bilah_geser_t *)w;
}

/* Hitung posisi x knob dari nilai saat ini. */
static int pg_bg_knop_x(pg_bilah_geser_t *g, int sw)
{
        int usable, rentang;
        usable = sw - 2 * PG_BG_PAD - 2 * PG_BG_KNOP_R;
        if (usable <= 0) return sw / 2;
        rentang = g->maks - g->min;
        if (rentang <= 0) return PG_BG_PAD + PG_BG_KNOP_R;
        return PG_BG_PAD + PG_BG_KNOP_R +
               (g->nilai - g->min) * usable / rentang;
}

/* Hitung nilai dari posisi x. */
static int pg_bg_hitung(pg_bilah_geser_t *g, int x, int sw)
{
        int usable, rentang, nilai;
        usable = sw - 2 * PG_BG_PAD - 2 * PG_BG_KNOP_R;
        if (usable <= 0) return g->min;
        rentang = g->maks - g->min;
        if (rentang <= 0) return g->min;
        nilai = g->min + (x - PG_BG_PAD - PG_BG_KNOP_R) * rentang / usable;
        if (nilai < g->min) nilai = g->min;
        if (nilai > g->maks) nilai = g->maks;
        return nilai;
}

/* Cek apakah titik (x, y) ada di dalam knob. */
static pg_bool pg_bg_dalam_knop(pg_bilah_geser_t *g, int x, int y,
                                 int sw, int sh)
{
        int kx, ky, dx, dy;
        kx = pg_bg_knop_x(g, sw);
        ky = sh / 2;
        dx = x - kx;
        dy = y - ky;
        return (dx * dx + dy * dy <= PG_BG_KNOP_R * PG_BG_KNOP_R) ?
                PG_BENAR : PG_SALAH;
}

static void pg_bg_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
        pg_bilah_geser_t *g = pg_bg_dari(w);
        int sw, sh, track_y, knob_x;
        pg_warna_t latar_warna, batas_warna, knob_warna, track_warna;
        pg_kotak_t r;
        int radius = w->radius;

        sw = pg_permukaan_lebar(s);
        sh = pg_permukaan_tinggi(s);

        /* Tentukan state visual. */
        if (!w->aktif) {
                latar_warna  = PG_WARNA_NONAKTIF_ISI;
                batas_warna  = PG_ABU_TERANG;
                track_warna  = PG_ABU_TERANG;
                knob_warna   = PG_ABU_TERANG;
        } else {
                latar_warna  = g->latar;
                batas_warna  = g->batas;
                track_warna  = g->batas;
                if (g->menyeret)
                        knob_warna = PG_WARNA_TEKAN_ISI;
                else if (g->hover_knop)
                        knob_warna = PG_WARNA_HOVER_OUTLINE;
                else
                        knob_warna = PG_ABU_TERANG;
                /* Override outline bila fokus. */
                if (w->fokus) batas_warna = PG_WARNA_FOKUS;
        }

        /* Latar cerdas: bila alpha < 255, clear transparan. */
        if (PG_A(w->latar) < 255) {
                pg_isi_permukaan(s, PG_TRANSPARAN);
        }

        /* Latar widget dengan rounded rect AA (fill + outline). */
        r = pg_buat_kotak(0, 0, sw, sh);
        /* Untuk bilah_geser, latar biasanya transparan (ditampilkan
         * tanpa latar kotak, hanya track + knob). Tapi kalau user
         * setel latar eksplisit, gambar. */
        if (PG_A(g->latar) > 0) {
                pg_gambar_kotak_tumpul_isi_garis_aa(s, r, radius,
                                                    latar_warna, batas_warna);
        } else if (w->fokus && w->aktif) {
                /* Fokus tetap tampil outline tipis walau latar transparan. */
                pg_gambar_kotak_tumpul_isi_garis_aa(s, r, radius,
                                                    PG_TRANSPARAN,
                                                    PG_WARNA_FOKUS);
        }

        /* Track: rounded rect horizontal di tengah dengan value progress.
         * Track terbagi 2: filled (sebelum knob, warna aktif) dan
         * empty (setelah knob, warna track idle). */
        track_y = sh / 2 - PG_BG_TRACK_H / 2;
        knob_x = pg_bg_knop_x(g, sw);
        {
                int track_x_start = PG_BG_PAD + PG_BG_KNOP_R;
                int track_x_end   = sw - PG_BG_PAD - PG_BG_KNOP_R;
                int track_w       = track_x_end - track_x_start;
                int filled_w;
                pg_kotak_t track_r;
                if (track_w <= 0) {
                        track_x_start = PG_BG_PAD;
                        track_w = sw - 2 * PG_BG_PAD;
                        if (track_w <= 0) track_w = 1;
                }
                /* Width filled = jarak dari track_start ke knob_x. */
                filled_w = knob_x - track_x_start;
                if (filled_w < 0) filled_w = 0;
                if (filled_w > track_w) filled_w = track_w;
                /* Track latar (empty portion). */
                track_r = pg_buat_kotak(track_x_start, track_y,
                                         track_w, PG_BG_TRACK_H);
                pg_gambar_kotak_tumpul_isi_garis_aa(s, track_r,
                                                    PG_BG_TRACK_RAD,
                                                    track_warna,
                                                    track_warna);
                /* Filled portion (sebelum knob) — pakai warna fokus
                 * biru supaya kontras dengan empty. */
                if (filled_w > 0 && w->aktif) {
                        pg_kotak_t filled_r = pg_buat_kotak(
                                track_x_start, track_y,
                                filled_w, PG_BG_TRACK_H);
                        pg_gambar_kotak_tumpul_isi_garis_aa(s, filled_r,
                            PG_BG_TRACK_RAD,
                            PG_WARNA_FOKUS, PG_WARNA_FOKUS);
                }
        }

        /* Knob: ellipse AA radius 6. */
        {
                int cx = knob_x;
                int cy = sh / 2;
                pg_gambar_lingkaran_isi_aa(s, cx, cy, PG_BG_KNOP_R,
                                            knob_warna, knob_warna);
        }
}

static pg_bool pg_bg_peristiwa_v(pg_widget_t *w, const pg_aksi_t *e)
{
        pg_bilah_geser_t *g = pg_bg_dari(w);
        int sw, sh, nilai_baru;

        if (!w->aktif) return PG_SALAH;

        sw = w->kotak.w;
        sh = w->kotak.h;

        /* TETIKUS_TEKAN: mulai drag atau klik track. */
        if (e->tipe == PG_AKSI_TETIKUS_TEKAN &&
            e->tetik_tombol == PG_TETIKUS_KIRI) {
                int x = e->tetik_pos.x;
                int y = e->tetik_pos.y;
                pg_widget_fokus(w);
                if (pg_bg_dalam_knop(g, x, y, sw, sh)) {
                        g->menyeret = PG_BENAR;
                } else {
                        /* Klik track: lompat ke posisi. */
                        g->menyeret = PG_BENAR;
                        nilai_baru = pg_bg_hitung(g, x, sw);
                        if (nilai_baru != g->nilai) {
                                g->nilai = nilai_baru;
                                if (g->cb) g->cb(g, g->nilai, g->ctx);
                        }
                }
                pg_widget_kotor(w);
                return PG_BENAR;
        }

        /* TETIKUS_LEPAS: akhiri drag. */
        if (e->tipe == PG_AKSI_TETIKUS_LEPAS &&
            e->tetik_tombol == PG_TETIKUS_KIRI) {
                if (g->menyeret) {
                        g->menyeret = PG_SALAH;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }

        /* GERAK: update hover + drag. */
        if (e->tipe == PG_AKSI_TETIKUS_GERAK) {
                int x = e->tetik_pos.x;
                int y = e->tetik_pos.y;
                pg_bool new_hover_knop;
                /* Bila sedang menyeret, update nilai. */
                if (g->menyeret) {
                        nilai_baru = pg_bg_hitung(g, x, sw);
                        if (nilai_baru != g->nilai) {
                                g->nilai = nilai_baru;
                                pg_widget_kotor(w);
                                if (g->cb) g->cb(g, g->nilai, g->ctx);
                        }
                        return PG_BENAR;
                }
                /* Update hover state. */
                new_hover_knop = pg_bg_dalam_knop(g, x, y, sw, sh);
                if (new_hover_knop != g->hover_knop) {
                        g->hover_knop = new_hover_knop;
                        pg_widget_kotor(w);
                }
                return PG_SALAH;  /* biarkan event lanjut */
        }

        /* Keyboard: panah kiri/kanan, Home/End. */
        if (e->tipe == PG_AKSI_TOMBOL_TURUN &&
            pg_widget_punya_fokus(w)) {
                if (e->tombol == PG_TOMBOL_KIRI) {
                        nilai_baru = g->nilai - g->langkah;
                        if (nilai_baru < g->min) nilai_baru = g->min;
                        if (nilai_baru != g->nilai) {
                                g->nilai = nilai_baru;
                                pg_widget_kotor(w);
                                if (g->cb) g->cb(g, g->nilai, g->ctx);
                        }
                        return PG_BENAR;
                }
                if (e->tombol == PG_TOMBOL_KANAN) {
                        nilai_baru = g->nilai + g->langkah;
                        if (nilai_baru > g->maks) nilai_baru = g->maks;
                        if (nilai_baru != g->nilai) {
                                g->nilai = nilai_baru;
                                pg_widget_kotor(w);
                                if (g->cb) g->cb(g, g->nilai, g->ctx);
                        }
                        return PG_BENAR;
                }
                if (e->tombol == PG_TOMBOL_HOME) {
                        if (g->min != g->nilai) {
                                g->nilai = g->min;
                                pg_widget_kotor(w);
                                if (g->cb) g->cb(g, g->nilai, g->ctx);
                        }
                        return PG_BENAR;
                }
                if (e->tombol == PG_TOMBOL_END) {
                        if (g->maks != g->nilai) {
                                g->nilai = g->maks;
                                pg_widget_kotor(w);
                                if (g->cb) g->cb(g, g->nilai, g->ctx);
                        }
                        return PG_BENAR;
                }
        }

        return PG_SALAH;
}

static void pg_bg_hancur_v(pg_widget_t *w)
{
        (void)w;
}

static void pg_bg_bebas_v(pg_widget_t *w)
{
        free(w);
}

static const pg_widget_vtable_t pg_bg_vtable = {
        pg_bg_catat_v,
        pg_bg_peristiwa_v,
        NULL,
        pg_bg_hancur_v,
        NULL,
        NULL,
        pg_bg_bebas_v
};

/* ---------------------------------------------------------------- API */

pg_bilah_geser_t *pg_buat_bilah_geser(int min, int maks, int nilai)
{
        pg_bilah_geser_t *g;
        g = (pg_bilah_geser_t *)calloc(1, sizeof(*g));
        if (!g) return NULL;
        pg_widget_init(&g->base, PG_WIDGET_GESER, &pg_bg_vtable);
        pg_widget_milik(&g->base, PG_BENAR);
        g->min = min;
        g->maks = maks;
        if (g->maks < g->min) g->maks = g->min;
        g->nilai = nilai;
        if (g->nilai < g->min) g->nilai = g->min;
        if (g->nilai > g->maks) g->nilai = g->maks;
        g->langkah = 1;
        g->fg = PG_WARNA_TEKS_TOMBOL;
        g->latar = PG_TRANSPARAN;       /* default transparan */
        g->batas = PG_WARNA_HOVER_OUTLINE;
        g->kenop = PG_ABU_TERANG;
        g->base.latar = PG_TRANSPARAN;
        g->base.radius = 0;  /* sudut tajam */
        pg_widget_setel_ukuran_min(&g->base, 80, 16);
        return g;
}

void pg_bilah_geser_hancur(pg_bilah_geser_t *g)
{
        if (!g) return;
        pg_widget_hancur(&g->base);
        free(g);
}

int pg_bilah_geser_nilai(pg_bilah_geser_t *g)
{
        return g ? g->nilai : 0;
}

void pg_bilah_geser_setel_nilai(pg_bilah_geser_t *g, int nilai)
{
        if (!g) return;
        if (nilai < g->min) nilai = g->min;
        if (nilai > g->maks) nilai = g->maks;
        if (nilai == g->nilai) return;
        g->nilai = nilai;
        pg_widget_kotor(&g->base);
        if (g->cb) g->cb(g, g->nilai, g->ctx);
}

void pg_bilah_geser_setel_rentang(pg_bilah_geser_t *g,
                                    int min, int maks)
{
        if (!g) return;
        g->min = min;
        g->maks = maks;
        if (g->maks < g->min) g->maks = g->min;
        /* Klem nilai saat ini ke rentang baru. */
        if (g->nilai < g->min) g->nilai = g->min;
        if (g->nilai > g->maks) g->nilai = g->maks;
        pg_widget_kotor(&g->base);
}

void pg_bilah_geser_saatberubah(pg_bilah_geser_t *g,
                                  pg_bilah_geser_cb cb, void *ctx)
{
        if (!g) return;
        g->cb = cb;
        g->ctx = ctx;
}

void pg_bilah_geser_setel_aktif(pg_bilah_geser_t *g, pg_bool aktif)
{
        if (!g) return;
        if (aktif) {
                pg_widget_aktifkan(&g->base);
        } else {
                pg_widget_nonaktifkan(&g->base);
                if (g->base.fokus) pg_widget_blur(&g->base);
                g->menyeret = PG_SALAH;
                g->hover_knop = PG_SALAH;
                g->hover_track = PG_SALAH;
        }
        pg_widget_kotor(&g->base);
}

pg_bool pg_bilah_geser_aktif(pg_bilah_geser_t *g)
{
        return g ? g->base.aktif : PG_SALAH;
}

pg_widget_t *pg_bilah_geser_widget(pg_bilah_geser_t *g)
{
        return g ? &g->base : NULL;
}
