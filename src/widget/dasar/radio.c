/* ----------------------------------------------------------------------------------------------
 * pigura widget: radio.c - radio button bergrup (polish v0.x)
 * ----------------------------------------------------------------------------------------------
 * Radio button selalu bagian dari sebuah grup. Dalam grup, tepat
 * satu radio terpilih pada satu waktu. Klik radio menjadikannya
 * terpilih dan otomatis mencabut pilihan radio lain. Grup menyimpan
 * array pointer radio; field terpilih menunjuk ke radio aktif (NULL
 * bila grup kosong).
 *
 * State visual:
 *   - idle     : lingkaran border HOVER_OUTLINE, kosong
 *   - hover    : lingkaran border FOKUS (lebih terang)
 *   - fokus    : lingkaran border FOKUS
 *   - terpilih : lingkaran border HOVER_OUTLINE (atau FOKUS bila
 *                hover/fokus), dot dalam warna FOKUS
 *   - disabled : lingkaran border ABU_TERANG, fg ABU
 *
 * Layout posisi lingkaran relatif label: KIRI/KANAN/ATAS/BAWAH.
 * Auto-size min_w/min_h dihitung dari ukuran lingkaran + label + padding.
 *
 * Keyboard:
 *   - Space/Enter    : pilih radio ini
 *   - Panah atas/kiri: pilih radio sebelumnya dalam grup
 *   - Panah bawah/kanan: pilih radio sesudahnya dalam grup
 *
 * Membebaskan grup TIDAK membebaskan radio anggota; pemilik bertanggung
 * jawab membebaskan masing-masing radio.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/radio.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/peristiwa.h"
#include "pigura/widget.h"

#include <stdlib.h>
#include <string.h>

#define PG_RADIO_UKURAN_DEFAULT 14
#define PG_RADIO_PADDING_DEFAULT 4

struct pg_radio {
        pg_widget_t      base;
        pg_radio_grup_t *grup;
        char            *label;
        pg_font_t       *font;
        pg_posisi_t      posisi;
        int              ukuran;
        int              padding;

        /* Warna. */
        pg_warna_t       fg;
        pg_warna_t       lingk_batas;

        /* State. */
        pg_bool          hover;
        pg_bool          aktif;

        pg_radio_cb      cb;
        void            *ctx;
};

struct pg_radio_grup {
        pg_radio_t **radio;
        int          n_radio;
        int          cap_radio;
        pg_radio_t  *terpilih;
};

/* ------------------------------------------------------------ helper */

static char *pg_radio_dup(const char *s)
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

static pg_radio_t *pg_radio_dari(pg_widget_t *w)
{
        return (pg_radio_t *)w;
}

/* Hitung ukuran preferred (min_w/min_h). */
static void pg_radio_hitung_preferred(const pg_radio_t *r,
                                       int *out_w, int *out_h)
{
        int ks = r->ukuran;
        int pad = r->padding;
        int tw = 0, th = 0;
        if (r->label && r->font) {
                tw = pg_font_lebar_teks(r->font, r->label);
                th = pg_font_tinggi(r->font);
                if (th <= 0) th = 8;
        }
        if (tw == 0) {
                *out_w = ks;
                *out_h = ks;
                return;
        }
        switch (r->posisi) {
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

static void pg_radio_update_min(pg_radio_t *r)
{
        int pw, ph;
        pg_radio_hitung_preferred(r, &pw, &ph);
        pg_widget_setel_ukuran_min(&r->base, pw, ph);
}

/* Hitung posisi lingkaran (cx, cy, radius) dan label berdasarkan
 * posisi dan ukuran widget. */
static void pg_radio_layout(const pg_radio_t *r, int sw, int sh,
                              int *out_cx, int *out_cy, int *out_radius,
                              int *out_label_x, int *out_label_y)
{
        int ks = r->ukuran;
        int pad = r->padding;
        int radius = ks / 2;
        int th = 0;
        if (r->label && r->font) {
                th = pg_font_tinggi(r->font);
                if (th <= 0) th = 8;
        }
        *out_radius = radius - 1;  /* -1 supaya AA edge tidak terpotong */
        switch (r->posisi) {
        case PG_POSISI_KANAN:
                /* label kiri, lingkaran kanan. */
                *out_cx = sw - radius - 2;
                *out_cy = sh / 2;
                if (r->label && r->font) {
                        *out_label_x = 0;
                        *out_label_y = pg_font_baseline_tengah(r->font, sh);
                }
                return;
        case PG_POSISI_ATAS:
                /* lingkaran atas, label bawah. */
                *out_cx = sw / 2;
                *out_cy = radius;
                if (r->label && r->font) {
                        int tw = pg_font_lebar_teks(r->font, r->label);
                        *out_label_x = (sw - tw) / 2;
                        if (*out_label_x < 0) *out_label_x = 0;
                        *out_label_y = ks + pad +
                                pg_font_baseline_tengah(r->font,
                                                         sh - ks - pad);
                        if (*out_label_y < 0) *out_label_y = 0;
                }
                return;
        case PG_POSISI_BAWAH:
                /* label atas, lingkaran bawah. */
                *out_cx = sw / 2;
                *out_cy = sh - radius - 1;
                if (r->label && r->font) {
                        int tw = pg_font_lebar_teks(r->font, r->label);
                        *out_label_x = (sw - tw) / 2;
                        if (*out_label_x < 0) *out_label_x = 0;
                        *out_label_y = pg_font_baseline_tengah(r->font,
                                                                sh - ks - pad);
                        if (*out_label_y < 0) *out_label_y = 0;
                }
                return;
        case PG_POSISI_KIRI:
        default:
                /* lingkaran kiri, label kanan (default). */
                *out_cx = radius + 1;
                *out_cy = sh / 2;
                if (r->label && r->font) {
                        *out_label_x = ks + pad;
                        *out_label_y = pg_font_baseline_tengah(r->font, sh);
                }
                return;
        }
}

/* ===== Render ===== */

static void pg_radio_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
        pg_radio_t *r = pg_radio_dari(w);
        int sw, sh;
        int cx, cy, radius;
        int label_x = 0, label_y = 0;
        pg_bool terpilih;
        pg_warna_t warna_batas;
        pg_warna_t warna_dot;
        pg_warna_t warna_fg;

        sw = pg_permukaan_lebar(s);
        sh = pg_permukaan_tinggi(s);
        pg_radio_layout(r, sw, sh, &cx, &cy, &radius,
                          &label_x, &label_y);

        terpilih = (r->grup && r->grup->terpilih == r) ?
                PG_BENAR : PG_SALAH;

        /* Tentukan warna state. */
        if (!r->aktif) {
                warna_batas = PG_ABU_TERANG;
                warna_dot    = PG_ABU;
                warna_fg      = PG_ABU;
        } else {
                if (w->fokus || r->hover) {
                        warna_batas = PG_WARNA_FOKUS;
                } else {
                        warna_batas = r->lingk_batas;
                }
                warna_dot = PG_WARNA_FOKUS;
                warna_fg   = r->fg;
        }

        /* Lingkaran AA outline. */
        pg_gambar_lingkaran_aa(s, cx, cy, radius, warna_batas);

        /* Dot dalam bila terpilih. */
        if (terpilih) {
                int dot_radius = radius / 2;
                if (dot_radius < 1) dot_radius = 1;
                pg_gambar_lingkaran_isi_aa(s, cx, cy, dot_radius,
                                             warna_dot, warna_dot);
        }

        /* Label. */
        if (r->label && r->font) {
                pg_font_gambar_teks(r->font, r->label, s,
                                     label_x, label_y, warna_fg);
        }
}

/* ===== Event handler ===== */

/* Cari index radio di grup. -1 bila tidak ditemukan. */
static int pg_radio_index_di_grup(pg_radio_t *r)
{
        int i;
        if (!r || !r->grup) return -1;
        for (i = 0; i < r->grup->n_radio; i++) {
                if (r->grup->radio[i] == r) return i;
        }
        return -1;
}

/* Pilih radio pada offset relatif dari r, wrap-around. */
static void pg_radio_pilih_offset(pg_radio_t *r, int offset)
{
        int idx, n, new_idx;
        pg_radio_grup_t *g;
        if (!r || !r->grup) return;
        g = r->grup;
        idx = pg_radio_index_di_grup(r);
        if (idx < 0) return;
        n = g->n_radio;
        if (n <= 0) return;
        new_idx = (idx + offset) % n;
        if (new_idx < 0) new_idx += n;
        if (new_idx == idx) return;
        pg_radio_pilih(g->radio[new_idx]);
}

static pg_bool pg_radio_peristiwa_v(pg_widget_t *w,
                                     const pg_peristiwa_t *e)
{
        pg_radio_t *r = pg_radio_dari(w);
        int mx, my;
        int sw, sh;
        int in_kotak;

        if (!r->aktif) return PG_SALAH;

        mx = e->tetik_pos.x;
        my = e->tetik_pos.y;
        sw = w->kotak.w;
        sh = w->kotak.h;
        in_kotak = (mx >= 0 && mx < sw && my >= 0 && my < sh);

        /* GERAK: update hover state. */
        if (e->tipe == PG_PERISTIWA_TETIK_GERAK) {
                pg_bool new_hover = in_kotak ? PG_BENAR : PG_SALAH;
                if (r->hover != new_hover) {
                        r->hover = new_hover;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }

        /* TETIK_TURUN: pilih radio + set focus. */
        if (e->tipe == PG_PERISTIWA_TETIK_TURUN &&
            e->tetik_tombol == PG_TETIK_KIRI) {
                if (in_kotak) {
                        pg_widget_fokus(w);
                        if (r->grup && r->grup->terpilih != r) {
                                pg_radio_pilih(r);
                        }
                        if (r->cb) r->cb(r, r->ctx);
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }

        /* Keyboard. */
        if (e->tipe == PG_PERISTIWA_TOMBOL_TURUN && w->fokus) {
                switch (e->tombol) {
                case PG_TOMBOL_SPASI:
                case PG_TOMBOL_ENTER:
                        if (r->grup && r->grup->terpilih != r) {
                                pg_radio_pilih(r);
                        }
                        if (r->cb) r->cb(r, r->ctx);
                        pg_widget_kotor(w);
                        return PG_BENAR;
                case PG_TOMBOL_ATAS:
                case PG_TOMBOL_KIRI:
                        pg_radio_pilih_offset(r, -1);
                        return PG_BENAR;
                case PG_TOMBOL_BAWAH:
                case PG_TOMBOL_KANAN:
                        pg_radio_pilih_offset(r, 1);
                        return PG_BENAR;
                default:
                        break;
                }
        }

        return PG_SALAH;
}

static void pg_radio_hancur_v(pg_widget_t *w)
{
        pg_radio_t *r = pg_radio_dari(w);
        if (r->label) {
                free(r->label);
                r->label = NULL;
        }
}

static void pg_radio_bebas_v(pg_widget_t *w)
{
        free(w);
}

static const pg_widget_vtable_t pg_radio_vtable = {
        pg_radio_catat_v,
        pg_radio_peristiwa_v,
        NULL,
        pg_radio_hancur_v,
        NULL,
        NULL,
        pg_radio_bebas_v
};

/* ===== API grup ===== */

pg_radio_grup_t *pg_buat_radio_grup(void)
{
        pg_radio_grup_t *g;
        g = (pg_radio_grup_t *)calloc(1, sizeof(*g));
        return g;
}

void pg_radio_grup_hancur(pg_radio_grup_t *g)
{
        if (!g) return;
        if (g->radio) {
                free(g->radio);
                g->radio = NULL;
        }
        g->n_radio = 0;
        g->cap_radio = 0;
        g->terpilih = NULL;
        free(g);
}

/* ===== API radio ===== */

pg_radio_t *pg_buat_radio(pg_radio_grup_t *g, const char *label,
                           pg_font_t *font)
{
        pg_radio_t *r;
        r = (pg_radio_t *)calloc(1, sizeof(*r));
        if (!r) return NULL;
        pg_widget_init(&r->base, PG_WIDGET_RADIO, &pg_radio_vtable);
        pg_widget_milik(&r->base, PG_BENAR);
        r->grup = g;
        r->font = font;
        r->posisi = PG_POSISI_KIRI;
        r->ukuran = PG_RADIO_UKURAN_DEFAULT;
        r->padding = PG_RADIO_PADDING_DEFAULT;
        r->aktif = PG_BENAR;
        /* Default warna tema. */
        r->fg = PG_WARNA_TEKS_TOMBOL;
        r->lingk_batas = PG_WARNA_HOVER_OUTLINE;
        /* Latar transparan supaya parent show through. */
        r->base.latar = PG_TRANSPARAN;
        if (label) {
                r->label = pg_radio_dup(label);
                if (!r->label) {
                        free(r);
                        return NULL;
                }
        }
        if (g) {
                pg_radio_t **baru;
                int cap_baru;
                if (g->n_radio >= g->cap_radio) {
                        cap_baru = g->cap_radio ? g->cap_radio * 2 : 4;
                        baru = (pg_radio_t **)realloc(g->radio,
                                (size_t)cap_baru * sizeof(*baru));
                        if (!baru) {
                                free(r->label);
                                free(r);
                                return NULL;
                        }
                        g->radio = baru;
                        g->cap_radio = cap_baru;
                }
                g->radio[g->n_radio++] = r;
                if (!g->terpilih) g->terpilih = r;
        }
        pg_radio_update_min(r);
        return r;
}

void pg_radio_hancur(pg_radio_t *r)
{
        int i, j;
        if (!r) return;
        if (r->grup) {
                for (i = 0; i < r->grup->n_radio; i++) {
                        if (r->grup->radio[i] == r) {
                                for (j = i;
                                     j < r->grup->n_radio - 1; j++)
                                        r->grup->radio[j] =
                                                r->grup->radio[j + 1];
                                r->grup->n_radio--;
                                break;
                        }
                }
                if (r->grup->terpilih == r) {
                        r->grup->terpilih =
                                r->grup->n_radio > 0 ?
                                r->grup->radio[0] : NULL;
                }
        }
        pg_widget_hancur(&r->base);
        free(r);
}

pg_bool pg_radio_terpilih(pg_radio_t *r)
{
        if (!r || !r->grup) return PG_SALAH;
        return (r->grup->terpilih == r) ? PG_BENAR : PG_SALAH;
}

void pg_radio_pilih(pg_radio_t *r)
{
        int i;
        pg_radio_t *prev;
        if (!r || !r->grup) return;
        if (r->grup->terpilih == r) return;
        prev = r->grup->terpilih;
        r->grup->terpilih = r;
        /* Kotor radio lama (bila ada) dan radio baru. */
        if (prev) pg_widget_kotor(&prev->base);
        pg_widget_kotor(&r->base);
        /* Panggil callback radio baru bila ada. */
        if (r->cb) r->cb(r, r->ctx);
        /* Tandai semua radio di grup kotor supaya visual konsisten. */
        for (i = 0; i < r->grup->n_radio; i++)
                pg_widget_kotor(&r->grup->radio[i]->base);
}

void pg_radio_setel_posisi(pg_radio_t *r, pg_posisi_t pos)
{
        if (!r) return;
        r->posisi = pos;
        pg_radio_update_min(r);
        pg_widget_kotor(&r->base);
}

void pg_radio_setel_ukuran(pg_radio_t *r, int px)
{
        if (!r) return;
        if (px < 6) px = 6;
        if (px > 64) px = 64;
        r->ukuran = px;
        pg_radio_update_min(r);
        pg_widget_kotor(&r->base);
}

void pg_radio_setel_padding(pg_radio_t *r, int padding)
{
        if (!r) return;
        if (padding < 0) padding = 0;
        r->padding = padding;
        pg_radio_update_min(r);
        pg_widget_kotor(&r->base);
}

void pg_radio_setel_warna(pg_radio_t *r, pg_warna_t fg,
                           pg_warna_t lingk_batas)
{
        if (!r) return;
        r->fg = fg;
        r->lingk_batas = lingk_batas;
        pg_widget_kotor(&r->base);
}

void pg_radio_saatberubah(pg_radio_t *r, pg_radio_cb cb, void *ctx)
{
        if (!r) return;
        r->cb = cb;
        r->ctx = ctx;
}

void pg_radio_setel_aktif(pg_radio_t *r, pg_bool aktif)
{
        if (!r) return;
        r->aktif = aktif ? PG_BENAR : PG_SALAH;
        if (aktif) {
                pg_widget_aktifkan(&r->base);
        } else {
                pg_widget_nonaktifkan(&r->base);
                if (r->base.fokus) pg_widget_blur(&r->base);
                r->hover = PG_SALAH;
        }
        pg_widget_kotor(&r->base);
}

pg_widget_t *pg_radio_widget(pg_radio_t *r)
{
        return r ? &r->base : NULL;
}
