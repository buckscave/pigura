/* ----------------------------------------------------------------------------------------------
 * pigura widget: kotak_penanda.c - widget checkbox (kotak penanda)
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/kotak_penanda.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/aksi.h"
#include "pigura/widget.h"

#include <stdlib.h>
#include <string.h>

#define PG_KP_KOTAK_DEFAULT 14
#define PG_KP_PADDING_DEFAULT 4
#define PG_KP_RADIUS_DEFAULT 2
#define PG_KP_INSET 3  /* inset tanda X / dash dari tepi kotak */

struct pg_kotak_penanda {
        pg_widget_t  base;
        char        *label;
        pg_font_t   *font;
        pg_tri_t     tri;
        pg_posisi_t  posisi;
        int          ukuran_kotak;
        int          padding;
        pg_warna_t   fg;
        pg_warna_t   kotak_bg;
        pg_warna_t   kotak_batas;
        pg_bool      hover;
        pg_bool      aktif;
        pg_kotak_penanda_cb cb;
        void        *ctx;
};

static char *pg_kp_dup(const char *s)
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

static pg_kotak_penanda_t *pg_kp_dari(pg_widget_t *w)
{
        return (pg_kotak_penanda_t *)w;
}

static void pg_kp_hitung_preferred(const pg_kotak_penanda_t *c,
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

static void pg_kp_update_min(pg_kotak_penanda_t *c)
{
        int pw, ph;
        pg_kp_hitung_preferred(c, &pw, &ph);
        pg_widget_setel_ukuran_min(&c->base, pw, ph);
}

/* ===== Render ===== */

static void pg_kp_gambar_x(pg_permukaan_t *s, pg_kotak_t kr,
                            pg_warna_t c)
{
        pg_fixed_t x0, y0, x1, y1;
        x0 = ((pg_fixed_t)(kr.x + PG_KP_INSET)) << 16;
        y0 = ((pg_fixed_t)(kr.y + PG_KP_INSET)) << 16;
        x1 = ((pg_fixed_t)(kr.x + kr.w - 1 - PG_KP_INSET)) << 16;
        y1 = ((pg_fixed_t)(kr.y + kr.h - 1 - PG_KP_INSET)) << 16;
        pg_gambar_garis_aa(s, x0, y0, x1, y1, c);
        pg_gambar_garis_aa(s, x0, y0 + (1 << 16), x1, y1 + (1 << 16), c);
        x0 = ((pg_fixed_t)(kr.x + kr.w - 1 - PG_KP_INSET)) << 16;
        y0 = ((pg_fixed_t)(kr.y + PG_KP_INSET)) << 16;
        x1 = ((pg_fixed_t)(kr.x + PG_KP_INSET)) << 16;
        y1 = ((pg_fixed_t)(kr.y + kr.h - 1 - PG_KP_INSET)) << 16;
        pg_gambar_garis_aa(s, x0, y0, x1, y1, c);
        pg_gambar_garis_aa(s, x0, y0 + (1 << 16), x1, y1 + (1 << 16), c);
}

static void pg_kp_gambar_dash(pg_permukaan_t *s, pg_kotak_t kr,
                                pg_warna_t c)
{
        pg_fixed_t x0, y0, x1, y1;
        int ymid = kr.y + kr.h / 2;
        x0 = ((pg_fixed_t)(kr.x + PG_KP_INSET)) << 16;
        y0 = ((pg_fixed_t)(ymid)) << 16;
        x1 = ((pg_fixed_t)(kr.x + kr.w - 1 - PG_KP_INSET)) << 16;
        y1 = ((pg_fixed_t)(ymid)) << 16;
        pg_gambar_garis_aa(s, x0, y0, x1, y1, c);
}

static void pg_kp_layout(const pg_kotak_penanda_t *c, int sw, int sh,
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
                out_kr->x = (sw - ks) / 2;
                if (out_kr->x < 0) out_kr->x = 0;
                out_kr->y = 0;
                out_kr->w = ks;
                out_kr->h = ks;
                if (c->label && c->font) {
                        int tw = pg_font_lebar_teks(c->font, c->label);
                        *out_label_x = (sw - tw) / 2;
                        if (*out_label_x < 0) *out_label_x = 0;
                        *out_label_y = ks + pad +
                                pg_font_baseline_tengah(c->font, sh - ks - pad);
                        if (*out_label_y < 0) *out_label_y = 0;
                }
                return;
        case PG_POSISI_BAWAH:
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

static void pg_kp_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
        pg_kotak_penanda_t *c = pg_kp_dari(w);
        int sw, sh, radius;
        pg_kotak_t kr;
        int label_x = 0, label_y = 0;
        pg_warna_t warna_batas, warna_isi, warna_tanda, warna_fg;

        sw = pg_permukaan_lebar(s);
        sh = pg_permukaan_tinggi(s);
        radius = w->radius;

        pg_kp_layout(c, sw, sh, &kr, &label_x, &label_y);

        if (!c->aktif) {
                warna_isi    = PG_WARNA_PANEL;
                warna_batas  = PG_ABU_TERANG;
                warna_tanda  = PG_ABU;
                warna_fg      = PG_ABU;
        } else {
                if (c->hover) {
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

        /* Latar cerdas. */
        if (PG_A(w->latar) < 255) {
                pg_isi_permukaan(s, PG_TRANSPARAN);
        }

        /* Render isi + border dalam single pass (Cairo-quality). */
        pg_gambar_kotak_tumpul_isi_garis_aa(s, kr, radius,
                                              warna_isi, warna_batas);

        if (c->tri == PG_TRI_TERPILIH) {
                pg_kp_gambar_x(s, kr, warna_tanda);
        } else if (c->tri == PG_TRI_INDETERMINATE) {
                pg_kp_gambar_dash(s, kr, warna_tanda);
        }

        if (c->label && c->font) {
                pg_font_gambar_teks(c->font, c->label, s,
                                     label_x, label_y, warna_fg);
        }
}

/* ===== Event handler ===== */

static pg_bool pg_kp_peristiwa_v(pg_widget_t *w,
                                    const pg_aksi_t *e)
{
        pg_kotak_penanda_t *c = pg_kp_dari(w);
        int mx, my;
        int sw, sh;
        int in_kotak;

        if (!c->aktif) return PG_SALAH;

        mx = e->tetik_pos.x;
        my = e->tetik_pos.y;
        sw = w->kotak.w;
        sh = w->kotak.h;
        in_kotak = (mx >= 0 && mx < sw && my >= 0 && my < sh);

        if (e->tipe == PG_AKSI_TETIKUS_GERAK) {
                pg_bool new_hover = in_kotak ? PG_BENAR : PG_SALAH;
                if (c->hover != new_hover) {
                        c->hover = new_hover;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }

        if (e->tipe == PG_AKSI_TETIKUS_TEKAN &&
            e->tetik_tombol == PG_TETIKUS_KIRI) {
                if (in_kotak) {
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

        if (e->tipe == PG_AKSI_TOMBOL_TURUN && w->fokus &&
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

static void pg_kp_hancur_v(pg_widget_t *w)
{
        pg_kotak_penanda_t *c = pg_kp_dari(w);
        if (c->label) {
                free(c->label);
                c->label = NULL;
        }
}

static void pg_kp_bebas_v(pg_widget_t *w)
{
        free(w);
}

static const pg_widget_vtable_t pg_kp_vtable = {
        pg_kp_catat_v,
        pg_kp_peristiwa_v,
        NULL,
        pg_kp_hancur_v,
        NULL,
        NULL,
        pg_kp_bebas_v
};

/* ===== API publik ===== */

pg_kotak_penanda_t *pg_buat_kotak_penanda(const char *label,
                                            pg_font_t *font)
{
        pg_kotak_penanda_t *c;
        c = (pg_kotak_penanda_t *)calloc(1, sizeof(*c));
        if (!c) return NULL;
        pg_widget_init(&c->base, PG_WIDGET_CEK, &pg_kp_vtable);
        pg_widget_milik(&c->base, PG_BENAR);
        c->font = font;
        c->tri = PG_TRI_UBAH;
        c->posisi = PG_POSISI_KIRI;
        c->ukuran_kotak = PG_KP_KOTAK_DEFAULT;
        c->padding = PG_KP_PADDING_DEFAULT;
        c->aktif = PG_BENAR;
        c->fg = PG_WARNA_TEKS_TOMBOL;
        c->kotak_bg = PG_WARNA_PANEL;
        c->kotak_batas = PG_WARNA_HOVER_OUTLINE;
        c->base.latar = PG_TRANSPARAN;
        c->base.radius = PG_KP_RADIUS_DEFAULT;
        if (label) {
                c->label = pg_kp_dup(label);
                if (!c->label) {
                        free(c);
                        return NULL;
                }
        }
        pg_kp_update_min(c);
        return c;
}

void pg_kotak_penanda_hancur(pg_kotak_penanda_t *c)
{
        if (!c) return;
        pg_widget_hancur(&c->base);
        free(c);
}

pg_bool pg_kotak_penanda_dicek(pg_kotak_penanda_t *c)
{
        return c ? (c->tri == PG_TRI_TERPILIH ? PG_BENAR : PG_SALAH) :
                   PG_SALAH;
}

void pg_kotak_penanda_setel_dicek(pg_kotak_penanda_t *c,
                                    pg_bool dicek)
{
        if (!c) return;
        c->tri = dicek ? PG_TRI_TERPILIH : PG_TRI_UBAH;
        pg_widget_kotor(&c->base);
}

pg_tri_t pg_kotak_penanda_tri(pg_kotak_penanda_t *c)
{
        return c ? c->tri : PG_TRI_UBAH;
}

void pg_kotak_penanda_setel_tri(pg_kotak_penanda_t *c,
                                  pg_tri_t tri)
{
        if (!c) return;
        if (tri < 0) tri = PG_TRI_UBAH;
        if (tri > PG_TRI_INDETERMINATE) tri = PG_TRI_INDETERMINATE;
        c->tri = tri;
        pg_widget_kotor(&c->base);
}

void pg_kotak_penanda_setel_posisi(pg_kotak_penanda_t *c,
                                     pg_posisi_t pos)
{
        if (!c) return;
        c->posisi = pos;
        pg_kp_update_min(c);
        pg_widget_kotor(&c->base);
}

void pg_kotak_penanda_setel_ukuran_kotak(pg_kotak_penanda_t *c,
                                           int px)
{
        if (!c) return;
        if (px < 6) px = 6;
        if (px > 64) px = 64;
        c->ukuran_kotak = px;
        pg_kp_update_min(c);
        pg_widget_kotor(&c->base);
}

void pg_kotak_penanda_setel_padding(pg_kotak_penanda_t *c,
                                      int padding)
{
        if (!c) return;
        if (padding < 0) padding = 0;
        c->padding = padding;
        pg_kp_update_min(c);
        pg_widget_kotor(&c->base);
}

void pg_kotak_penanda_setel_warna(pg_kotak_penanda_t *c,
                                    pg_warna_t fg,
                                    pg_warna_t kotak_bg,
                                    pg_warna_t kotak_batas)
{
        if (!c) return;
        c->fg = fg;
        c->kotak_bg = kotak_bg;
        c->kotak_batas = kotak_batas;
        pg_widget_kotor(&c->base);
}

void pg_kotak_penanda_saatberubah(pg_kotak_penanda_t *c,
                                     pg_kotak_penanda_cb cb,
                                     void *ctx)
{
        if (!c) return;
        c->cb = cb;
        c->ctx = ctx;
}

void pg_kotak_penanda_setel_aktif(pg_kotak_penanda_t *c,
                                    pg_bool aktif)
{
        if (!c) return;
        c->aktif = aktif ? PG_BENAR : PG_SALAH;
        if (aktif) {
                pg_widget_aktifkan(&c->base);
        } else {
                pg_widget_nonaktifkan(&c->base);
                if (c->base.fokus) pg_widget_blur(&c->base);
                c->hover = PG_SALAH;
        }
        pg_widget_kotor(&c->base);
}

pg_widget_t *pg_kotak_penanda_widget(pg_kotak_penanda_t *c)
{
        return c ? &c->base : NULL;
}
