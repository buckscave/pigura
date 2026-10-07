/* ----------------------------------------------------------------------------------------------
 * pigura widget: tombol.c - widget tombol tekan (polish v0.20)
 * ----------------------------------------------------------------------------------------------
 * State visual:
 *   - idle   : latar panel (#EBEBEB), no outline
 *   - hover  : latar hover (#E1E1E1), outline hover (#DCDCDC)
 *   - tekan  : latar tekan (#DCDCDC), outline 3D (atas+kiri #F0F0F0,
 *              kanan+bawah #D2D2D2) — efek "turun"
 *   - fokus  : outline biru (#50B9FF) di semua state bila punya fokus
 *
 * Mode konten:
 *   - label saja    : teks centered
 *   - icon saja      : icon centered, tombol square (auto-size)
 *   - icon + label   : layout sesuai posisi_icon (default KIRI)
 *
 * Radius kotak dari base.radius (default 0 = tajam).
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/tombol.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/aksi.h"
#include "pigura/widget.h"

#include <stdlib.h>
#include <string.h>

#define PG_TOMBOL_PADDING_DEFAULT 6
#define PG_TOMBOL_ICON_PAD       4   /* jarak icon ke label */

struct pg_tombol_impl {
        pg_widget_t   base;
        char         *label;
        pg_font_t    *font;
        pg_permukaan_t *icon;
        pg_tombol_posisi_icon_t posisi_icon;
        int           padding;

        /* Warna (override-able). */
        pg_warna_t    fg;
        pg_warna_t    isi_idle;
        pg_warna_t    isi_hover;
        pg_warna_t    isi_tekan;
        pg_bool       warna_custom;

        /* State. */
        pg_bool       ditekan;
        pg_bool       hover;
        pg_bool       terpilih;  /* toggle state untuk toolbar */
        pg_bool       gambar_batas;

        pg_tombol_cb cb;
        void         *ctx;
};

static char *pg_tombol_dup(const char *s)
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

static pg_tombol_t *pg_tombol_dari(pg_widget_t *w)
{
        return (pg_tombol_t *)w;
}

/* ===== Layout konten ===== */

/* Hitung ukuran preferred tombol berdasarkan konten. */
static void pg_tombol_hitung_preferred(const pg_tombol_t *t,
                                         int *out_w, int *out_h)
{
        int pad = t->padding * 2;
        int tw = 0, th = 0;
        int iw = 0, ih = 0;
        if (t->label && t->font) {
                tw = pg_font_lebar_teks(t->font, t->label);
                th = pg_font_tinggi(t->font);
                if (th <= 0) th = 8;
        }
        if (t->icon) {
                iw = pg_permukaan_lebar(t->icon);
                ih = pg_permukaan_tinggi(t->icon);
        }
        if (iw == 0 && tw == 0) {
                *out_w = pad + 8;
                *out_h = pad + 8;
                return;
        }
        if (t->icon && !t->label) {
                /* Icon-only: square. */
                int s = (iw > ih) ? iw : ih;
                *out_w = s + pad;
                *out_h = s + pad;
                return;
        }
        if (t->icon && t->label) {
                /* Hibrida: layout sesuai posisi_icon. */
                switch (t->posisi_icon) {
                case PG_TOMBOL_ICON_KIRI:
                case PG_TOMBOL_ICON_KANAN:
                        *out_w = iw + PG_TOMBOL_ICON_PAD + tw + pad;
                        *out_h = (ih > th ? ih : th) + pad;
                        break;
                case PG_TOMBOL_ICON_ATAS:
                case PG_TOMBOL_ICON_BAWAH:
                        *out_w = (iw > tw ? iw : tw) + pad;
                        *out_h = ih + PG_TOMBOL_ICON_PAD + th + pad;
                        break;
                default:
                        *out_w = tw + pad;
                        *out_h = th + pad;
                        break;
                }
                return;
        }
        /* Label only. */
        *out_w = tw + pad;
        *out_h = th + pad;
}

/* Update min_w/min_h base. */
static void pg_tombol_update_min(pg_tombol_t *t)
{
        int pw, ph;
        pg_tombol_hitung_preferred(t, &pw, &ph);
        pg_widget_setel_ukuran_min(&t->base, pw, ph);
}

/* ===== Render ===== */

/* Gambar outline 3D untuk state tekan/terpilih.
 * Atas+kiri: terang (raised), kanan+bawah: gelap (shadow). */
/* Gambar inset bevel untuk state tekan/terpilih.
 * Atas+kiri: gelap (shadow inside), kanan+bawah: terang (highlight inside).
 * Efek: tombol kelihatan "turun" / ditekan. */
static void pg_tombol_gambar_inset_bevel(pg_permukaan_t *s,
                                            pg_kotak_t r,
                                            pg_warna_t gelap,
                                            pg_warna_t terang)
{
        int x2 = r.x + r.w - 1;
        int y2 = r.y + r.h - 1;
        /* Atas + kiri: gelap (inside shadow). */
        pg_garis_h_permukaan(s, r.x, r.x+r.w, r.y, gelap);
        pg_garis_v_permukaan(s, r.x, r.y, r.y+r.h, gelap);
        /* Kanan + bawah: terang (inside highlight). */
        pg_garis_h_permukaan(s, r.x, r.x+r.w, y2, terang);
        pg_garis_v_permukaan(s, x2, r.y, r.y+r.h, terang);
}

/* Gambar isi + outline dalam satu pass. Delegate ke shared
 * Cairo-quality renderer di gambar.c (4x4 SS + sqrt(cov) gamma).
 * Lihat pg_gambar_kotak_tumpul_isi_garis_aa untuk detail algoritma. */
static void pg_tombol_gambar_isi_garis(pg_permukaan_t *s,
                                          pg_kotak_t r, int radius,
                                          pg_warna_t isi,
                                          pg_warna_t garis)
{
        pg_gambar_kotak_tumpul_isi_garis_aa(s, r, radius, isi, garis);
}

/* Gambar outline (border) di sekeliling kotak. */
static void pg_tombol_gambar_outline(pg_permukaan_t *s, pg_kotak_t r,
                                        int radius, pg_warna_t c)
{
        if (radius > 0)
                pg_gambar_kotak_tumpul_aa(s, r, radius, c);
        else
                pg_gambar_kotak_aa(s, r, c);
}

/* Gambar isi (fill) kotak. */
static void pg_tombol_gambar_isi(pg_permukaan_t *s, pg_kotak_t r,
                                    int radius, pg_warna_t c)
{
        if (radius > 0)
                pg_gambar_kotak_tumpul_isi_aa(s, r, radius, c);
        else
                pg_isi_permukaan(s, c);
}

/* Render icon dengan alpha blend. Pixel alpha=0 skip,
 * alpha=255 raw copy, alpha<255 blend dengan dst. */
static void pg_tombol_render_icon(pg_permukaan_t *s, pg_permukaan_t *icon,
                                    int dx, int dy, int sw, int sh)
{
        int iw = pg_permukaan_lebar(icon);
        int ih = pg_permukaan_tinggi(icon);
        const pg_warna_t *src = (const pg_warna_t *)pg_permukaan_piksel(icon);
        int ix, iy;
        for (iy = 0; iy < ih && dy+iy < sh; iy++) {
                for (ix = 0; ix < iw && dx+ix < sw; ix++) {
                        pg_warna_t sc = src[iy * iw + ix];
                        int sa = (int)PG_A(sc);
                        if (sa == 0) continue;
                        if (sa == 255) {
                                pg_setel_piksel_permukaan(s, dx+ix, dy+iy, sc);
                        } else {
                                pg_warna_t dc = pg_ambil_piksel_permukaan(s,
                                        dx+ix, dy+iy);
                                int ia = 255 - sa;
                                int r = (PG_R(sc) * sa + PG_R(dc) * ia) / 255;
                                int g = (PG_G(sc) * sa + PG_G(dc) * ia) / 255;
                                int b = (PG_B(sc) * sa + PG_B(dc) * ia) / 255;
                                pg_setel_piksel_permukaan(s, dx+ix, dy+iy,
                                        PG_RGB(r, g, b));
                        }
                }
        }
}

static void pg_tombol_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
        pg_tombol_t *t = pg_tombol_dari(w);
        pg_warna_t isi;
        pg_kotak_t r;
        int sw, sh;
        int radius = w->radius;
        int show_3d = 0;  /* tampilkan outline 3D tekan */

        sw = pg_permukaan_lebar(s);
        sh = pg_permukaan_tinggi(s);
        r = pg_buat_kotak(0, 0, sw, sh);

        /* Tentukan state visual aktif. */
        if (!w->aktif) {
                isi = PG_WARNA_NONAKTIF_ISI;
        } else if (t->ditekan || t->terpilih) {
                isi = t->isi_tekan;
                show_3d = 1;  /* bevel saat ditekan DAN terpilih */
        } else if (t->hover) {
                isi = t->isi_hover;
        } else {
                isi = t->isi_idle;
        }

        /* Render isi + outline dalam satu pass (tidak double AA). */
        {
                pg_warna_t warna_garis;
                if (w->fokus)
                        warna_garis = PG_WARNA_FOKUS;
                else
                        warna_garis = PG_WARNA_HOVER_OUTLINE;
                pg_tombol_gambar_isi_garis(s, r, radius, isi,
                                            warna_garis);
        }

        /* Inset bevel untuk state tekan/terpilih (efek "turun"). */
        if (show_3d) {
                pg_tombol_gambar_inset_bevel(s, r,
                        PG_WARNA_TEKAN_GELAP, PG_WARNA_TEKAN_TERANG);
        }

        /* === Render konten === */
        {
                int tw = 0, th = 0;
                int iw = 0, ih = 0;
                int pad = t->padding;
                if (t->label && t->font) {
                        tw = pg_font_lebar_teks(t->font, t->label);
                        th = pg_font_tinggi(t->font);
                        if (th <= 0) th = 8;
                }
                if (t->icon) {
                        iw = pg_permukaan_lebar(t->icon);
                        ih = pg_permukaan_tinggi(t->icon);
                }

                if (t->icon && !t->label) {
                        /* Icon-only: centered, square. */
                        int ix = (sw - iw) / 2;
                        int iy = (sh - ih) / 2;
                        if (ix < pad) ix = pad;
                        if (iy < pad) iy = pad;
                        pg_tombol_render_icon(s, t->icon, ix, iy, sw, sh);
                        return;
                }
                if (t->icon && t->label) {
                        /* Hibrida: layout sesuai posisi_icon. */
                        int ix, iy, lx, ly;
                        int bl = pg_font_baseline_tengah(t->font, sh);
                        switch (t->posisi_icon) {
                        case PG_TOMBOL_ICON_KIRI:
                                iy = (sh - ih) / 2;
                                ix = pad;
                                lx = ix + iw + PG_TOMBOL_ICON_PAD;
                                ly = bl;
                                pg_tombol_render_icon(s, t->icon,
                                        ix, iy, sw, sh);
                                pg_font_gambar_teks(t->font, t->label, s,
                                        lx, ly, t->fg);
                                break;
                        case PG_TOMBOL_ICON_KANAN:
                                iy = (sh - ih) / 2;
                                lx = pad;
                                ly = bl;
                                ix = lx + tw + PG_TOMBOL_ICON_PAD;
                                pg_font_gambar_teks(t->font, t->label, s,
                                        lx, ly, t->fg);
                                pg_tombol_render_icon(s, t->icon,
                                        ix, iy, sw, sh);
                                break;
                        case PG_TOMBOL_ICON_ATAS:
                                ix = (sw - iw) / 2;
                                iy = pad;
                                lx = (sw - tw) / 2;
                                ly = iy + ih + PG_TOMBOL_ICON_PAD + th/2;
                                pg_tombol_render_icon(s, t->icon,
                                        ix, iy, sw, sh);
                                pg_font_gambar_teks(t->font, t->label, s,
                                        lx, ly, t->fg);
                                break;
                        case PG_TOMBOL_ICON_BAWAH:
                                lx = (sw - tw) / 2;
                                ly = pad + th/2;
                                ix = (sw - iw) / 2;
                                iy = ly + PG_TOMBOL_ICON_PAD;
                                pg_font_gambar_teks(t->font, t->label, s,
                                        lx, ly, t->fg);
                                pg_tombol_render_icon(s, t->icon,
                                        ix, iy, sw, sh);
                                break;
                        }
                        return;
                }
                /* Label only. */
                if (t->label && t->font) {
                        int lx = (sw - tw) / 2;
                        int ly = pg_font_baseline_tengah(t->font, sh);
                        if (lx < pad) lx = pad;
                        if (ly < 0) ly = 0;
                        pg_font_gambar_teks(t->font, t->label, s,
                                lx, ly, t->fg);
                }
        }
}

/* ===== Event handler ===== */

static pg_bool pg_tombol_peristiwa_v(pg_widget_t *w,
                                      const pg_aksi_t *e)
{
        pg_tombol_t *t = pg_tombol_dari(w);
        int mx, my;
        int sw, sh;
        int in_kotak;

        mx = e->tetik_pos.x;
        my = e->tetik_pos.y;
        sw = w->kotak.w;
        sh = w->kotak.h;
        in_kotak = (mx >= 0 && mx < sw && my >= 0 && my < sh);

        /* GERAK: update hover state. */
        if (e->tipe == PG_AKSI_TETIKUS_GERAK) {
                pg_bool new_hover = in_kotak ? PG_BENAR : PG_SALAH;
                if (t->hover != new_hover) {
                        t->hover = new_hover;
                        pg_widget_kotor(w);
                }
                /* Bila ditekan tapi mouse keluar, reset ditekan. */
                if (t->ditekan && !in_kotak) {
                        t->ditekan = PG_SALAH;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }

        /* TETIK_TURUN: mulai tekan. */
        if (e->tipe == PG_AKSI_TETIKUS_TEKAN &&
            e->tetik_tombol == PG_TETIKUS_KIRI) {
                if (in_kotak && !t->ditekan) {
                        t->ditekan = PG_BENAR;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }

        /* TETIK_NAIK: selesai tekan — trigger klik bila masih di dalam. */
        if (e->tipe == PG_AKSI_TETIKUS_LEPAS &&
            e->tetik_tombol == PG_TETIKUS_KIRI) {
                if (t->ditekan) {
                        t->ditekan = PG_SALAH;
                        pg_widget_kotor(w);
                        /* Hanya trigger klik bila mouse masih di dalam. */
                        if (in_kotak && t->cb)
                                t->cb(t, t->ctx);
                }
                return PG_BENAR;
        }

        /* Keyboard: Enter/Space = klik (bila punya fokus). */
        if (e->tipe == PG_AKSI_TOMBOL_TURUN && w->fokus &&
            (e->tombol == PG_TOMBOL_ENTER ||
             e->tombol == PG_TOMBOL_SPASI)) {
                if (!t->ditekan) {
                        t->ditekan = PG_BENAR;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }
        if (e->tipe == PG_AKSI_TOMBOL_NAIK && w->fokus &&
            (e->tombol == PG_TOMBOL_ENTER ||
             e->tombol == PG_TOMBOL_SPASI)) {
                if (t->ditekan) {
                        t->ditekan = PG_SALAH;
                        pg_widget_kotor(w);
                        if (t->cb) t->cb(t, t->ctx);
                }
                return PG_BENAR;
        }

        return PG_SALAH;
}

static void pg_tombol_hancur_v(pg_widget_t *w)
{
        pg_tombol_t *t = pg_tombol_dari(w);
        if (t->label) {
                free(t->label);
                t->label = NULL;
        }
        /* Catatan: icon TIDAK dihancurkan — app yang punya. */
}

static void pg_tombol_bebas_v(pg_widget_t *w)
{
        free(w);
}

static const pg_widget_vtable_t pg_tombol_vtable = {
        pg_tombol_catat_v,
        pg_tombol_peristiwa_v,
        NULL,
        pg_tombol_hancur_v,
        NULL,
        NULL,
        pg_tombol_bebas_v
};

/* ===== API publik ===== */

pg_tombol_t *pg_buat_tombol(const char *label, pg_font_t *font)
{
        pg_tombol_t *t;
        t = (pg_tombol_t *)calloc(1, sizeof(*t));
        if (!t) return NULL;
        pg_widget_init(&t->base, PG_WIDGET_TOMBOL,
                       &pg_tombol_vtable);
        pg_widget_milik(&t->base, PG_BENAR);
        t->base.latar = PG_TRANSPARAN;
        t->font = font;
        t->posisi_icon = PG_TOMBOL_ICON_KIRI;
        t->padding = PG_TOMBOL_PADDING_DEFAULT;
        t->gambar_batas = PG_BENAR;
        t->warna_custom = PG_SALAH;
        /* Default warna tema. */
        t->fg = PG_WARNA_TEKS_TOMBOL;
        t->isi_idle = PG_WARNA_PANEL;
        t->isi_hover = PG_WARNA_HOVER_ISI;
        t->isi_tekan = PG_WARNA_TEKAN_ISI;
        if (label) {
                t->label = pg_tombol_dup(label);
                if (!t->label) {
                        free(t);
                        return NULL;
                }
        }
        pg_tombol_update_min(t);
        return t;
}

void pg_tombol_hancur(pg_tombol_t *t)
{
        if (!t) return;
        pg_widget_hancur(&t->base);
        free(t);
}

void pg_tombol_setel_batas(pg_tombol_t *t, pg_bool gambar)
{
        if (!t) return;
        t->gambar_batas = gambar;
        pg_widget_kotor(&t->base);
}

void pg_tombol_saatklik(pg_tombol_t *t, pg_tombol_cb cb, void *ctx)
{
        if (!t) return;
        t->cb = cb;
        t->ctx = ctx;
}

pg_widget_t *pg_tombol_widget(pg_tombol_t *t)
{
        return t ? &t->base : NULL;
}

void pg_tombol_setel_icon(pg_tombol_t *t, pg_permukaan_t *icon)
{
        if (!t) return;
        t->icon = icon;
        pg_tombol_update_min(t);
        pg_widget_kotor(&t->base);
}

void pg_tombol_setel_posisi_icon(pg_tombol_t *t,
                                   pg_tombol_posisi_icon_t pos)
{
        if (!t) return;
        t->posisi_icon = pos;
        pg_tombol_update_min(t);
        pg_widget_kotor(&t->base);
}

void pg_tombol_setel_label(pg_tombol_t *t, const char *label)
{
        if (!t) return;
        if (t->label) {
                free(t->label);
                t->label = NULL;
        }
        if (label) t->label = pg_tombol_dup(label);
        pg_tombol_update_min(t);
        pg_widget_kotor(&t->base);
}

void pg_tombol_setel_warna(pg_tombol_t *t,
                             pg_warna_t fg,
                             pg_warna_t isi_idle,
                             pg_warna_t isi_hover,
                             pg_warna_t isi_tekan)
{
        if (!t) return;
        t->fg = fg;
        t->isi_idle = isi_idle;
        t->isi_hover = isi_hover;
        t->isi_tekan = isi_tekan;
        t->warna_custom = PG_BENAR;
        pg_widget_kotor(&t->base);
}

void pg_tombol_setel_terpilih(pg_tombol_t *t, pg_bool terpilih)
{
        if (!t) return;
        t->terpilih = terpilih;
        pg_widget_kotor(&t->base);
}

pg_bool pg_tombol_terpilih(const pg_tombol_t *t)
{
        return t ? t->terpilih : PG_SALAH;
}

void pg_tombol_setel_padding(pg_tombol_t *t, int padding)
{
        if (!t) return;
        if (padding < 0) padding = 0;
        t->padding = padding;
        pg_tombol_update_min(t);
        pg_widget_kotor(&t->base);
}
