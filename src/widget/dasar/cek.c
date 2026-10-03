/* ----------------------------------------------------------------------------------------------
 * pigura widget: cek.c - widget checkbox (polish v0.x)
 * ----------------------------------------------------------------------------------------------
 * Checkbox dengan kotak rounded, label, dan tanda X / dash.
 *
 * State visual:
 *   - idle     : kotak isi PANEL, border HOVER_OUTLINE
 *   - hover    : kotak isi HOVER_ISI, border HOVER_OUTLINE
 *   - fokus    : border FOKUS (biru) menggantikan HOVER_OUTLINE
 *   - disabled : isi PANEL, border ABU_TERANG, fg ABU, tanda ABU
 *
 * Konten:
 *   - UBAH          : kotak kosong
 *   - TERPILIH     : tanda X (Wu line AA, warna FOKUS)
 *   - INDETERMINATE : tanda minus / dash horizontal (warna FOKUS)
 *
 * Layout posisi kotak relatif label: KIRI/KANAN/ATAS/BAWAH.
 * Auto-size min_w/min_h dihitung dari ukuran kotak + label + padding.
 *
 * Keyboard:
 *   - Space/Enter : toggle (bila punya fokus)
 *
 * Smart latar: bila w->latar alpha<255, isi transparan; bila opaque,
 * isi dengan latar. Pattern di widget.c (pg_widget_catat).
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/cek.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/peristiwa.h"
#include "pigura/widget.h"

#include <stdlib.h>
#include <string.h>

#define PG_CEK_KOTAK_DEFAULT 14
#define PG_CEK_PADDING_DEFAULT 4
#define PG_CEK_RADIUS_DEFAULT 2
#define PG_CEK_INSET 3  /* inset tanda X / dash dari tepi kotak */

struct pg_cek {
        pg_widget_t  base;
        char        *label;
        pg_font_t   *font;
        pg_tri_t     tri;
        pg_posisi_t  posisi;
        int          ukuran_kotak;
        int          padding;

        /* Warna (override-able). */
        pg_warna_t   fg;
        pg_warna_t   kotak_bg;
        pg_warna_t   kotak_batas;

        /* State. */
        pg_bool      hover;
        pg_bool      aktif;

        pg_cek_cb    cb;
        void        *ctx;
};

/* ------------------------------------------------------------ helper */

static char *pg_cek_dup(const char *s)
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

static pg_cek_t *pg_cek_dari(pg_widget_t *w)
{
        return (pg_cek_t *)w;
}

/* Hitung ukuran preferred (min_w/min_h). */
static void pg_cek_hitung_preferred(const pg_cek_t *c,
                                     int *out_w, int *out_h)
{
        int ks = c->ukuran_kotak;
        int pad = c->padding;
        int tw = 0, th = 0;
        if (c->label && c->font) {
                tw = pg_font_lebar_teks(c->font, c->label);
                th = pg_font_tinggi(c->font);
                if (th <= 0) th = 8;
        }
        if (tw == 0) {
                /* Kotak saja. */
                *out_w = ks;
                *out_h = ks;
                return;
        }
        switch (c->posisi) {
        case PG_POSISI_KIRI:
        case PG_POSISI_KANAN:
                *out_w = ks + pad + tw;
                *out_h = (ks > th ? ks : th);
                break;
        case PG_POSISI_ATAS:
        case PG_POSISI_BAWAH:
                *out_w = (ks > tw ? ks : tw);
                *out_h = ks + pad + th;
                break;
        default:
                *out_w = ks + pad + tw;
                *out_h = (ks > th ? ks : th);
                break;
        }
}

static void pg_cek_update_min(pg_cek_t *c)
{
        int pw, ph;
        pg_cek_hitung_preferred(c, &pw, &ph);
        pg_widget_setel_ukuran_min(&c->base, pw, ph);
}

/* ===== Render ===== */

/* Gambar tanda X (Wu line AA tebal) di dalam kotak.
 * Tiap diagonal digambar 2x dengan offset 1px supaya lebih tebal. */
static void pg_cek_gambar_x(pg_permukaan_t *s, pg_kotak_t kr,
                              pg_warna_t c)
{
        pg_fixed_t x0, y0, x1, y1;
        x0 = ((pg_fixed_t)(kr.x + PG_CEK_INSET)) << 16;
        y0 = ((pg_fixed_t)(kr.y + PG_CEK_INSET)) << 16;
        x1 = ((pg_fixed_t)(kr.x + kr.w - 1 - PG_CEK_INSET)) << 16;
        y1 = ((pg_fixed_t)(kr.y + kr.h - 1 - PG_CEK_INSET)) << 16;
        pg_gambar_garis_aa(s, x0, y0, x1, y1, c);
        pg_gambar_garis_aa(s, x0, y0 + (1 << 16), x1, y1 + (1 << 16), c);
        x0 = ((pg_fixed_t)(kr.x + kr.w - 1 - PG_CEK_INSET)) << 16;
        y0 = ((pg_fixed_t)(kr.y + PG_CEK_INSET)) << 16;
        x1 = ((pg_fixed_t)(kr.x + PG_CEK_INSET)) << 16;
        y1 = ((pg_fixed_t)(kr.y + kr.h - 1 - PG_CEK_INSET)) << 16;
        pg_gambar_garis_aa(s, x0, y0, x1, y1, c);
        pg_gambar_garis_aa(s, x0, y0 + (1 << 16), x1, y1 + (1 << 16), c);
}

/* Gambar tanda minus / dash horizontal di tengah kotak. */
static void pg_cek_gambar_dash(pg_permukaan_t *s, pg_kotak_t kr,
                                 pg_warna_t c)
{
        pg_fixed_t x0, y0, x1, y1;
        int ymid = kr.y + kr.h / 2;
        x0 = ((pg_fixed_t)(kr.x + PG_CEK_INSET)) << 16;
        y0 = ((pg_fixed_t)(ymid)) << 16;
        x1 = ((pg_fixed_t)(kr.x + kr.w - 1 - PG_CEK_INSET)) << 16;
        y1 = ((pg_fixed_t)(ymid)) << 16;
        pg_gambar_garis_aa(s, x0, y0, x1, y1, c);
}

/* Hitung posisi kotak dan label berdasarkan posisi dan ukuran widget. */
static void pg_cek_layout(const pg_cek_t *c, int sw, int sh,
                           pg_kotak_t *out_kr, int *out_label_x,
                           int *out_label_y)
{
        int ks = c->ukuran_kotak;
        int pad = c->padding;
        int th = 0;
        if (c->label && c->font) {
                th = pg_font_tinggi(c->font);
                if (th <= 0) th = 8;
        }
        switch (c->posisi) {
        case PG_POSISI_KANAN:
                /* label kiri, kotak kanan. */
                out_kr->x = sw - ks;
                out_kr->y = (sh - ks) / 2;
                if (out_kr->y < 0) out_kr->y = 0;
                out_kr->w = ks;
                out_kr->h = ks;
                if (c->label && c->font) {
                        *out_label_x = 0;
                        *out_label_y = pg_font_baseline_tengah(c->font, sh);
                }
                return;
        case PG_POSISI_ATAS:
                /* kotak atas, label bawah. */
                out_kr->x = (sw - ks) / 2;
                if (out_kr->x < 0) out_kr->x = 0;
                out_kr->y = 0;
                out_kr->w = ks;
                out_kr->h = ks;
                if (c->label && c->font) {
                        int tw = pg_font_lebar_teks(c->font, c->label);
                        *out_label_x = (sw - tw) / 2;
                        if (*out_label_x < 0) *out_label_x = 0;
                        /* baseline tengah pada sisa tinggi di bawah kotak */
                        *out_label_y = ks + pad +
                                pg_font_baseline_tengah(c->font, sh - ks - pad);
                        if (*out_label_y < 0) *out_label_y = 0;
                }
                return;
        case PG_POSISI_BAWAH:
                /* label atas, kotak bawah. */
                out_kr->x = (sw - ks) / 2;
                if (out_kr->x < 0) out_kr->x = 0;
                out_kr->y = sh - ks;
                if (out_kr->y < 0) out_kr->y = 0;
                out_kr->w = ks;
                out_kr->h = ks;
                if (c->label && c->font) {
                        int tw = pg_font_lebar_teks(c->font, c->label);
                        *out_label_x = (sw - tw) / 2;
                        if (*out_label_x < 0) *out_label_x = 0;
                        *out_label_y = pg_font_baseline_tengah(c->font,
                                                                sh - ks - pad);
                        if (*out_label_y < 0) *out_label_y = 0;
                }
                return;
        case PG_POSISI_KIRI:
        default:
                /* kotak kiri, label kanan (default). */
                out_kr->x = 0;
                out_kr->y = (sh - ks) / 2;
                if (out_kr->y < 0) out_kr->y = 0;
                out_kr->w = ks;
                out_kr->h = ks;
                if (c->label && c->font) {
                        *out_label_x = ks + pad;
                        *out_label_y = pg_font_baseline_tengah(c->font, sh);
                }
                return;
        }
}

static void pg_cek_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
        pg_cek_t *c = pg_cek_dari(w);
        int sw, sh;
        int radius;
        pg_kotak_t kr;
        int label_x = 0, label_y = 0;
        pg_warna_t warna_batas;
        pg_warna_t warna_isi;
        pg_warna_t warna_tanda;
        pg_warna_t warna_fg;

        sw = pg_permukaan_lebar(s);
        sh = pg_permukaan_tinggi(s);
        radius = w->radius;

        pg_cek_layout(c, sw, sh, &kr, &label_x, &label_y);

        /* Tentukan warna state. */
        if (!c->aktif) {
                warna_isi    = PG_WARNA_PANEL;
                warna_batas  = PG_ABU_TERANG;
                warna_tanda  = PG_ABU;
                warna_fg      = PG_ABU;
        } else {
                if (c->tri != PG_TRI_UBAH) {
                        warna_isi = PG_PUTIH;
                } else if (c->hover) {
                        warna_isi = PG_WARNA_HOVER_ISI;
                } else {
                        warna_isi = c->kotak_bg;
                }
                if (w->fokus) {
                        warna_batas = PG_WARNA_FOKUS;
                } else {
                        warna_batas = c->kotak_batas;
                }
                warna_tanda = PG_WARNA_FOKUS;
                warna_fg     = c->fg;
        }

        /* Isi kotak. */
        if (radius > 0)
                pg_gambar_kotak_tumpul_isi_aa(s, kr, radius, warna_isi);
        else
                pg_isi_permukaan_kotak(s, kr, warna_isi);

        /* Border kotak. */
        if (radius > 0)
                pg_gambar_kotak_tumpul_aa(s, kr, radius, warna_batas);
        else
                pg_gambar_kotak_aa(s, kr, warna_batas);

        /* Inset bevel saat dicek/indeterminate (efek tombol ditekan). */
        if (c->aktif && c->tri != PG_TRI_UBAH) {
                int x2 = kr.x + kr.w - 1;
                int y2 = kr.y + kr.h - 1;
                pg_garis_h_permukaan(s, kr.x, kr.y, kr.w,
                        PG_WARNA_TEKAN_GELAP);
                pg_garis_v_permukaan(s, kr.x, kr.y, kr.h,
                        PG_WARNA_TEKAN_GELAP);
                pg_garis_h_permukaan(s, kr.x, y2, kr.w,
                        PG_WARNA_TEKAN_TERANG);
                pg_garis_v_permukaan(s, x2, kr.y, kr.h,
                        PG_WARNA_TEKAN_TERANG);
        }

        /* Tanda X / dash. */
        if (c->tri == PG_TRI_TERPILIH) {
                pg_cek_gambar_x(s, kr, warna_tanda);
        } else if (c->tri == PG_TRI_INDETERMINATE) {
                pg_cek_gambar_dash(s, kr, warna_tanda);
        }

        /* Label. */
        if (c->label && c->font) {
                pg_font_gambar_teks(c->font, c->label, s,
                                     label_x, label_y, warna_fg);
        }
}

/* ===== Event handler ===== */

static pg_bool pg_cek_peristiwa_v(pg_widget_t *w,
                                    const pg_peristiwa_t *e)
{
        pg_cek_t *c = pg_cek_dari(w);
        int mx, my;
        int sw, sh;
        int in_kotak;

        if (!c->aktif) return PG_SALAH;

        mx = e->tetik_pos.x;
        my = e->tetik_pos.y;
        sw = w->kotak.w;
        sh = w->kotak.h;
        in_kotak = (mx >= 0 && mx < sw && my >= 0 && my < sh);

        /* GERAK: update hover state. */
        if (e->tipe == PG_PERISTIWA_TETIK_GERAK) {
                pg_bool new_hover = in_kotak ? PG_BENAR : PG_SALAH;
                if (c->hover != new_hover) {
                        c->hover = new_hover;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }

        /* TETIK_TURUN: toggle + set focus. */
        if (e->tipe == PG_PERISTIWA_TETIK_TURUN &&
            e->tetik_tombol == PG_TETIK_KIRI) {
                if (in_kotak) {
                        /* Toggle: UBAH -> TERPILIH; lainnya -> UBAH. */
                        if (c->tri == PG_TRI_UBAH)
                                c->tri = PG_TRI_TERPILIH;
                        else
                                c->tri = PG_TRI_UBAH;
                        pg_widget_fokus(w);
                        pg_widget_kotor(w);
                        if (c->cb)
                                c->cb(c, (c->tri == PG_TRI_TERPILIH) ?
                                         PG_BENAR : PG_SALAH, c->ctx);
                }
                return PG_BENAR;
        }

        /* Keyboard: Space/Enter = toggle (bila punya fokus). */
        if (e->tipe == PG_PERISTIWA_TOMBOL_TURUN && w->fokus &&
            (e->tombol == PG_TOMBOL_ENTER ||
             e->tombol == PG_TOMBOL_SPASI)) {
                if (c->tri == PG_TRI_UBAH)
                        c->tri = PG_TRI_TERPILIH;
                else
                        c->tri = PG_TRI_UBAH;
                pg_widget_kotor(w);
                if (c->cb)
                        c->cb(c, (c->tri == PG_TRI_TERPILIH) ?
                                 PG_BENAR : PG_SALAH, c->ctx);
                return PG_BENAR;
        }

        return PG_SALAH;
}

static void pg_cek_hancur_v(pg_widget_t *w)
{
        pg_cek_t *c = pg_cek_dari(w);
        if (c->label) {
                free(c->label);
                c->label = NULL;
        }
}

static void pg_cek_bebas_v(pg_widget_t *w)
{
        free(w);
}

static const pg_widget_vtable_t pg_cek_vtable = {
        pg_cek_catat_v,
        pg_cek_peristiwa_v,
        NULL,
        pg_cek_hancur_v,
        NULL,
        NULL,
        pg_cek_bebas_v
};

/* ===== API publik ===== */

pg_cek_t *pg_buat_cek(const char *label, pg_font_t *font)
{
        pg_cek_t *c;
        c = (pg_cek_t *)calloc(1, sizeof(*c));
        if (!c) return NULL;
        pg_widget_init(&c->base, PG_WIDGET_CEK, &pg_cek_vtable);
        pg_widget_milik(&c->base, PG_BENAR);
        c->font = font;
        c->tri = PG_TRI_UBAH;
        c->posisi = PG_POSISI_KIRI;
        c->ukuran_kotak = PG_CEK_KOTAK_DEFAULT;
        c->padding = PG_CEK_PADDING_DEFAULT;
        c->aktif = PG_BENAR;
        /* Default warna tema. */
        c->fg = PG_WARNA_TEKS_TOMBOL;
        c->kotak_bg = PG_WARNA_PANEL;
        c->kotak_batas = PG_WARNA_HOVER_OUTLINE;
        /* Latar transparan supaya parent show through. */
        c->base.latar = PG_TRANSPARAN;
        c->base.radius = PG_CEK_RADIUS_DEFAULT;
        if (label) {
                c->label = pg_cek_dup(label);
                if (!c->label) {
                        free(c);
                        return NULL;
                }
        }
        pg_cek_update_min(c);
        return c;
}

void pg_cek_hancur(pg_cek_t *c)
{
        if (!c) return;
        pg_widget_hancur(&c->base);
        free(c);
}

pg_bool pg_cek_dicek(pg_cek_t *c)
{
        return c ? (c->tri == PG_TRI_TERPILIH ? PG_BENAR : PG_SALAH) :
                  PG_SALAH;
}

void pg_cek_setel_dicek(pg_cek_t *c, pg_bool dicek)
{
        if (!c) return;
        c->tri = dicek ? PG_TRI_TERPILIH : PG_TRI_UBAH;
        pg_widget_kotor(&c->base);
}

pg_tri_t pg_cek_tri(pg_cek_t *c)
{
        return c ? c->tri : PG_TRI_UBAH;
}

void pg_cek_setel_tri(pg_cek_t *c, pg_tri_t tri)
{
        if (!c) return;
        /* Clamp ke range valid. */
        if (tri < 0) tri = PG_TRI_UBAH;
        if (tri > PG_TRI_INDETERMINATE) tri = PG_TRI_INDETERMINATE;
        c->tri = tri;
        pg_widget_kotor(&c->base);
}

void pg_cek_setel_posisi(pg_cek_t *c, pg_posisi_t pos)
{
        if (!c) return;
        c->posisi = pos;
        pg_cek_update_min(c);
        pg_widget_kotor(&c->base);
}

void pg_cek_setel_ukuran_kotak(pg_cek_t *c, int px)
{
        if (!c) return;
        if (px < 6) px = 6;     /* minimum 6px supaya tanda X terlihat */
        if (px > 64) px = 64;
        c->ukuran_kotak = px;
        pg_cek_update_min(c);
        pg_widget_kotor(&c->base);
}

void pg_cek_setel_padding(pg_cek_t *c, int padding)
{
        if (!c) return;
        if (padding < 0) padding = 0;
        c->padding = padding;
        pg_cek_update_min(c);
        pg_widget_kotor(&c->base);
}

void pg_cek_setel_warna(pg_cek_t *c, pg_warna_t fg,
                         pg_warna_t kotak_bg, pg_warna_t kotak_batas)
{
        if (!c) return;
        c->fg = fg;
        c->kotak_bg = kotak_bg;
        c->kotak_batas = kotak_batas;
        pg_widget_kotor(&c->base);
}

void pg_cek_saatberubah(pg_cek_t *c, pg_cek_cb cb, void *ctx)
{
        if (!c) return;
        c->cb = cb;
        c->ctx = ctx;
}

void pg_cek_setel_aktif(pg_cek_t *c, pg_bool aktif)
{
        if (!c) return;
        c->aktif = aktif ? PG_BENAR : PG_SALAH;
        /* Disabled tidak respon input — gunakan widget.aktif juga
         * supaya focus chain skip. */
        if (aktif) {
                pg_widget_aktifkan(&c->base);
        } else {
                pg_widget_nonaktifkan(&c->base);
                /* Lepaskan fokus bila sedang fokus. */
                if (c->base.fokus) pg_widget_blur(&c->base);
                c->hover = PG_SALAH;
        }
        pg_widget_kotor(&c->base);
}

pg_widget_t *pg_cek_widget(pg_cek_t *c)
{
        return c ? &c->base : NULL;
}
