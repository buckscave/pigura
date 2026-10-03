/* ----------------------------------------------------------------------------------------------
 * pigura widget: dropdown.c - widget dropdown (combobox)
 * ----------------------------------------------------------------------------------------------
 * Dropdown adalah combobox satu baris: klik membuka popup vertikal
 * berisi item; klik item menutup popup dan mengubah terpilih.
 * Berbeda dari menubar, dropdown hanya punya satu kolom item dan
 * menampilkan item terpilih di baris atas.
 *
 * Saat popup terbuka, tinggi widget diperluas untuk menampung popup.
 * Saat ditutup, tinggi kembali ke button_h saja.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/dropdown.h"
#include "pigura/permukaan.h"
#include "pigura/font.h"
#include "pigura/peristiwa.h"
#include "pigura/widget.h"

#include <stdlib.h>
#include <string.h>

typedef struct pg_dropdown_item {
        char *teks;
} pg_dropdown_item_t;

struct pg_dropdown {
        pg_widget_t        base;
        char              *label;
        pg_dropdown_item_t *item;
        int                n_item;
        int                cap_item;
        int                terpilih;
        int                hover_idx;  /* index item yang di-hover */
        pg_bool            buka;
        pg_warna_t         fg;
        pg_warna_t         latar;
        pg_warna_t         sel_bg;
        pg_warna_t         sel_fg;
        pg_warna_t         hover_bg;  /* latar saat hover */
        pg_warna_t         batas;
        pg_font_t         *font;
        pg_dropdown_cb     cb;
        void              *ctx;
};

static char *pg_dropdown_dup(const char *s)
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

static pg_dropdown_t *pg_dropdown_dari(pg_widget_t *w)
{
        return (pg_dropdown_t *)w;
}

/* Tinggi button (baris atas). */
static int pg_dd_button_h(pg_dropdown_t *d)
{
        int th;
        if (!d->font) return 16;
        th = pg_font_tinggi_baris(d->font);
        if (th <= 0) th = pg_font_tinggi(d->font);
        if (th <= 0) th = 8;
        return th + 4;
}

/* Tinggi satu baris item popup. */
static int pg_dd_row_h(pg_dropdown_t *d)
{
        int th;
        if (!d->font) return 10;
        th = pg_font_tinggi_baris(d->font);
        if (th <= 0) th = pg_font_tinggi(d->font);
        if (th <= 0) th = 8;
        return th + 2;
}

/* Tinggi popup terbuka. */
static int pg_dd_popup_h(pg_dropdown_t *d)
{
        return d->n_item * pg_dd_row_h(d);
}

/* Setel buka. Widget TIDAK di-resize (popup dirender via
 * catat_popup langsung ke permukaan tujuan supaya tidak
 * terpotong oleh kotak widget/parent). */
static void pg_dd_setel_buka(pg_dropdown_t *d, pg_bool buka)
{
        if (d->buka == buka) return;
        d->buka = buka;
        pg_widget_kotor(&d->base);
}

/* vtable: catat button bar saja (baris atas). Popup list
 * dirender via catat_popup langsung ke permukaan tujuan. */
static void pg_dropdown_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
        pg_dropdown_t *d = pg_dropdown_dari(w);
        int sw, sh, bh, ty;
        pg_kotak_t r;
        sw = pg_permukaan_lebar(s);
        sh = pg_permukaan_tinggi(s);
        bh = pg_dd_button_h(d);
        ty = pg_font_baseline_tengah(d->font, bh);
        /* Border penuh seluruh widget supaya tidak ada
         * "strip putih kedua" di bawah button bar. */
        r = pg_buat_kotak(0, 0, sw, sh);
        pg_kotak_permukaan(s, r, d->batas);
        /* Label (caption) di kiri. */
        if (d->label && d->font)
                pg_font_gambar_teks(d->font, d->label, s, 4, ty,
                                    d->fg);
        /* Item terpilih di tengah. */
        {
                int lx = 4;
                if (d->label && d->font)
                        lx += pg_font_lebar_teks(d->font, d->label) + 8;
                if (d->terpilih >= 0 && d->terpilih < d->n_item &&
                    d->item[d->terpilih].teks && d->font)
                        pg_font_gambar_teks(d->font,
                                d->item[d->terpilih].teks, s, lx, ty,
                                d->fg);
        }
        /* Indikator panah di kanan. */
        {
                int ax = sw - 10, ay = bh / 2;
                pg_garis_h_permukaan(s, ax, ax + 6, ay - 2, d->fg);
                pg_garis_h_permukaan(s, ax + 1, ax + 5, ay - 1, d->fg);
                pg_garis_h_permukaan(s, ax + 2, ax + 4, ay, d->fg);
                pg_setel_piksel_permukaan(s, ax + 3, ay + 1, d->fg);
        }
}

/* vtable: catat popup overlay - list item bila buka=TRUE.
 * Render langsung ke dest (koordinat absolut) tanpa clip ke
 * kotak widget. Dipanggil SETELAH catat normal. */
static void pg_dropdown_catat_popup_v(pg_widget_t *w,
                                       pg_permukaan_t *s)
{
        pg_dropdown_t *d = pg_dropdown_dari(w);
        int sw, bh, rh, i;
        int x0, y0;
        pg_kotak_t pr;
        if (!d->buka) return;
        sw = w->kotak.w;
        bh = pg_dd_button_h(d);
        rh = pg_dd_row_h(d);
        /* Posisi absolut di dest (jumlah kotak.x/y widget + induk). */
        {
                int ax = 0, ay = 0;
                pg_widget_posisi_layar(w, &ax, &ay);
                x0 = ax;
                y0 = ay + bh;
        }
        pr = pg_buat_kotak(x0, y0, sw, d->n_item * rh);
        pg_isi_permukaan_kotak(s, pr, d->latar);
        pg_kotak_permukaan(s, pr, d->batas);
        for (i = 0; i < d->n_item; i++) {
                int iy_abs = y0 + i * rh + 1;
                pg_warna_t bg_item = d->latar;
                pg_warna_t fg_item = d->fg;
                if (i == d->terpilih) {
                        bg_item = d->sel_bg;
                        fg_item = d->sel_fg;
                }
                if (i == d->hover_idx && i != d->terpilih) {
                        bg_item = d->hover_bg;
                }
                if (bg_item != d->latar)
                        pg_isi_permukaan_kotak(s,
                                pg_buat_kotak(x0 + 1, y0 + i * rh,
                                              sw - 2, rh),
                                bg_item);
                if (d->item[i].teks && d->font)
                        pg_font_gambar_teks(d->font,
                                d->item[i].teks, s, x0 + 4,
                                iy_abs +
                                pg_font_baseline_tengah(d->font, rh),
                                fg_item);
        }
}

/* vtable: hit-test - bila popup buka, area popup dianggap
 * bagian widget supaya klik di popup terdispatch.
 * Note: kotak widget di sini adalah lokal-induk (bukan absolut)
 * karena e->tetik_pos yang diterima vtable->peristiwa sudah
 * ditranslasikan ke frame induk oleh pg_widget_tangani_peristiwa. */
static pg_bool pg_dropdown_berisi_v(pg_widget_t *w, pg_titik_t p)
{
        pg_dropdown_t *d = pg_dropdown_dari(w);
        if (pg_widget_berisi(w, p)) return PG_BENAR;
        if (!d->buka || d->n_item <= 0) return PG_SALAH;
        {
                int bh = pg_dd_button_h(d);
                int pop_x0 = w->kotak.x;
                int pop_x1 = pop_x0 + w->kotak.w;
                int pop_y0 = w->kotak.y + bh;
                int pop_y1 = pop_y0 + pg_dd_popup_h(d);
                if (p.x >= pop_x0 && p.x < pop_x1 &&
                    p.y >= pop_y0 && p.y < pop_y1)
                        return PG_BENAR;
        }
        return PG_SALAH;
}

static pg_bool pg_dropdown_peristiwa_v(pg_widget_t *w,
                                     const pg_peristiwa_t *e)
{
        pg_dropdown_t *d = pg_dropdown_dari(w);
        int bh, rh;
        bh = pg_dd_button_h(d);
        rh = pg_dd_row_h(d);

        /* GERAK: hover tracking. Sama seperti menubar — selalu
         * handle GERAK jika popup terbuka, tidak bergantung pada
         * g_capture. */
        if (e->tipe == PG_PERISTIWA_TETIK_GERAK) {
                if (d->buka && e->tetik_pos.y >= bh) {
                        int idx = (e->tetik_pos.y - bh) / rh;
                        /* Cek x range juga (seperti menubar). */
                        if (e->tetik_pos.x >= 0 &&
                            e->tetik_pos.x < w->kotak.w &&
                            idx >= 0 && idx < d->n_item) {
                                if (idx != d->hover_idx) {
                                        d->hover_idx = idx;
                                        pg_widget_kotor(w);
                                }
                        }
                        return PG_BENAR;
                }
                /* Hover di button (belum buka) — track untuk
                 * visual feedback (opsional). */
                if (!d->buka && e->tetik_pos.y < bh) {
                        return PG_BENAR;
                }
                return PG_SALAH;
        }

        /* NAIK: tutup popup bila terbuka (klik di luar). */
        if (e->tipe == PG_PERISTIWA_TETIK_NAIK) {
                if (d->buka) {
                        /* Tidak tutup di NAIK — biarkan TURUN
                         * yang handle pilih/tutup. */
                }
                return PG_SALAH;
        }

        if (e->tipe != PG_PERISTIWA_TETIK_TURUN) return PG_SALAH;
        if (e->tetik_tombol != PG_TETIK_KIRI) return PG_SALAH;

        /* Klik di button (atas)? Buka popup. */
        if (e->tetik_pos.y < bh) {
                pg_dd_setel_buka(d, PG_BENAR);
                d->hover_idx = -1;
                pg_widget_kotor(w);
                return PG_BENAR;
        }

        /* Klik di popup (bawah)? Pilih item + tutup. */
        if (d->buka) {
                if (e->tetik_pos.y >= bh) {
                        int idx = (e->tetik_pos.y - bh) / rh;
                        if (e->tetik_pos.x >= 0 &&
                            e->tetik_pos.x < w->kotak.w &&
                            idx >= 0 && idx < d->n_item) {
                                if (idx != d->terpilih) {
                                        d->terpilih = idx;
                                        if (d->cb)
                                                d->cb(d, idx, d->ctx);
                                }
                        }
                }
                /* Tutup popup setelah klik (di item atau di luar). */
                pg_dd_setel_buka(d, PG_SALAH);
                d->hover_idx = -1;
                pg_widget_kotor(w);
                return PG_BENAR;
        }
        return PG_SALAH;
}

static void pg_dropdown_hancur_v(pg_widget_t *w)
{
        pg_dropdown_t *d = pg_dropdown_dari(w);
        int i;
        if (d->label) {
                free(d->label);
                d->label = NULL;
        }
        for (i = 0; i < d->n_item; i++) {
                if (d->item[i].teks) {
                        free(d->item[i].teks);
                        d->item[i].teks = NULL;
                }
        }
        if (d->item) {
                free(d->item);
                d->item = NULL;
        }
        d->n_item = 0;
        d->cap_item = 0;
}

/* Deallocator struktur turunan — dipanggil oleh
 * pg_widget_hancur_penuh() setelah hancur_v. */
static void pg_dropdown_bebas_v(pg_widget_t *w)
{
	free(w);
}

static const pg_widget_vtable_t pg_dropdown_vtable = {
        pg_dropdown_catat_v,
        pg_dropdown_peristiwa_v,
        NULL,
        pg_dropdown_hancur_v,
        pg_dropdown_catat_popup_v,
        pg_dropdown_berisi_v,
	pg_dropdown_bebas_v
};

/* ---------------------------------------------------------------- API */

pg_dropdown_t *pg_buat_dropdown(const char *label,
                                 pg_font_t *font)
{
        pg_dropdown_t *d;
        d = (pg_dropdown_t *)calloc(1, sizeof(*d));
        if (!d) return NULL;
        pg_widget_init(&d->base, PG_WIDGET_DROPDOWN,
                       &pg_dropdown_vtable);
        d->font = font;
        d->terpilih = -1;
        d->fg = PG_HITAM;
        d->latar = PG_PUTIH;
        d->sel_bg = PG_BIRU;
        d->sel_fg = PG_PUTIH;
        d->hover_bg = PG_RGB(0x40, 0x80, 0xff);
        d->batas = PG_ABU_GELAP;
        d->hover_idx = -1;
        if (label) {
                d->label = pg_dropdown_dup(label);
                if (!d->label) {
                        free(d);
                        return NULL;
                }
        }
        pg_widget_setel_latar(&d->base, d->latar);
        return d;
}

void pg_dropdown_hancur(pg_dropdown_t *d)
{
        if (!d) return;
        pg_widget_hancur(&d->base);
        free(d);
}

void pg_dropdown_tambah_item(pg_dropdown_t *d, const char *teks)
{
        pg_dropdown_item_t *baru;
        char *str;
        int cap_baru;
        if (!d) return;
        str = pg_dropdown_dup(teks);
        if (teks && !str) return;
        if (d->n_item >= d->cap_item) {
                cap_baru = d->cap_item ? d->cap_item * 2 : 8;
                baru = (pg_dropdown_item_t *)realloc(d->item,
                        (size_t)cap_baru * sizeof(*baru));
                if (!baru) {
                        if (str) free(str);
                        return;
                }
                d->item = baru;
                d->cap_item = cap_baru;
        }
        d->item[d->n_item].teks = str;
        d->n_item++;
        if (d->terpilih < 0) d->terpilih = 0;
        /* Bila sedang buka, perluas popup. */
        if (d->buka) pg_dd_setel_buka(d, PG_BENAR);
        pg_widget_kotor(&d->base);
}

int pg_dropdown_terpilih(const pg_dropdown_t *d)
{
        return d ? d->terpilih : -1;
}

void pg_dropdown_setel_terpilih(pg_dropdown_t *d, int idx)
{
        if (!d) return;
        if (d->n_item <= 0) return;
        if (idx < 0) idx = 0;
        if (idx >= d->n_item) idx = d->n_item - 1;
        if (idx == d->terpilih) return;
        d->terpilih = idx;
        pg_widget_kotor(&d->base);
}

void pg_dropdown_saatberubah(pg_dropdown_t *d,
                             pg_dropdown_cb cb, void *ctx)
{
        if (!d) return;
        d->cb = cb;
        d->ctx = ctx;
}

pg_bool pg_dropdown_terbuka(pg_dropdown_t *d)
{
        return d ? d->buka : PG_SALAH;
}

void pg_dropdown_tutup(pg_dropdown_t *d)
{
        if (!d) return;
        if (d->buka) {
                pg_dd_setel_buka(d, PG_SALAH);
                d->hover_idx = -1;
                pg_widget_kotor(&d->base);
        }
}

pg_widget_t *pg_dropdown_widget(pg_dropdown_t *d)
{
        return d ? &d->base : NULL;
}
