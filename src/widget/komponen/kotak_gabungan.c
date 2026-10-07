/* ----------------------------------------------------------------------------------------------
 * pigura widget: kotak_gabungan.c - widget combobox (kotak gabungan)
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/kotak_gabungan.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/aksi.h"
#include "pigura/widget.h"
#include "pigura/papan_klip.h"
#include "pigura/utf8.h"

#include <stdlib.h>
#include <string.h>

#define PG_KG_MAX_ITEMS 32
#define PG_KG_BTN_W     18

struct pg_kotak_gabungan {
        pg_widget_t base;
        char *buf; int cap, len, kursor;
        char *items[PG_KG_MAX_ITEMS];
        int n_items;
        int terpilih;
        pg_bool buka;
        int hover_idx;
        pg_bool hover_btn;
        pg_bool tekan_btn;
        pg_bool hover_field;
        pg_font_t *font;
        pg_kotak_gabungan_cb cb;
        void *ctx;
        pg_papan_klip_t *klip;
};

static pg_kotak_gabungan_t *pg_kg_dari(pg_widget_t *w)
{
        return (pg_kotak_gabungan_t *)w;
}

static void pg_kg_fire(pg_kotak_gabungan_t *cb)
{
        if (cb->cb) cb->cb(cb, cb->terpilih, cb->ctx);
}

static void pg_kg_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
        pg_kotak_gabungan_t *cb = pg_kg_dari(w);
        int sw, sh, bw, bl, th, y, radius;
        pg_kotak_t r;
        pg_warna_t latar_warna, batas_warna, fg_warna, btn_warna;

        sw = pg_permukaan_lebar(s);
        sh = pg_permukaan_tinggi(s);
        bw = PG_KG_BTN_W;
        bl = sw - bw;
        radius = w->radius;

        /* Auto-tutup popup bila widget kehilangan fokus (klik di luar). */
        if (cb->buka && !pg_widget_punya_fokus(w) && !cb->tekan_btn) {
                cb->buka = PG_SALAH;
                cb->hover_idx = -1;
        }

        if (!w->aktif) {
                latar_warna = PG_WARNA_NONAKTIF_ISI;
                batas_warna = PG_ABU_TERANG;
                fg_warna    = PG_WARNA_NONAKTIF_TEKS;
                btn_warna   = PG_ABU_TERANG;
        } else {
                latar_warna = PG_WARNA_PANEL;
                fg_warna    = PG_WARNA_TEKS_TOMBOL;
                if (w->fokus) batas_warna = PG_WARNA_FOKUS;
                else if (cb->hover_field || cb->hover_btn)
                        batas_warna = PG_RGB(0x6E, 0x6E, 0x6E);
                else batas_warna = PG_WARNA_HOVER_OUTLINE;
                btn_warna = cb->tekan_btn ? PG_WARNA_TEKAN_ISI :
                            (cb->hover_btn ? PG_WARNA_HOVER_ISI :
                                              PG_WARNA_PANEL);
        }

        /* Latar cerdas. */
        if (PG_A(w->latar) < 255) {
                pg_isi_permukaan(s, PG_TRANSPARAN);
        }

        /* Render latar + outline full widget. */
        r = pg_buat_kotak(0, 0, sw, sh);
        pg_gambar_kotak_tumpul_isi_garis_aa(s, r, radius,
                                            latar_warna, batas_warna);

        /* Tombol dropdown di kanan. */
        {
                pg_kotak_t rb = pg_buat_kotak(bl, 0, bw, sh);
                pg_isi_permukaan_kotak(s, rb, btn_warna);
                /* Divider vertikal antara field dan button. */
                pg_garis_v_permukaan(s, bl, 0, sh, batas_warna);
                /* Panah: ▼ saat tertutup, ▲ saat terbuka. */
                {
                        int cx = bl + bw / 2, cy = sh / 2;
                        if (cb->buka) {
                                /* ▲ — panah atas (terbalik). */
                                pg_setel_piksel_permukaan(s, cx,
                                        cy - 1, fg_warna);
                                pg_garis_h_permukaan(s, cx - 3, cx + 3,
                                        cy, fg_warna);
                                pg_garis_h_permukaan(s, cx - 4, cx + 4,
                                        cy + 1, fg_warna);
                        } else {
                                /* ▼ — panah bawah. */
                                pg_garis_h_permukaan(s, cx - 4, cx + 4,
                                        cy - 1, fg_warna);
                                pg_garis_h_permukaan(s, cx - 3, cx + 3,
                                        cy, fg_warna);
                                pg_setel_piksel_permukaan(s, cx, cy + 1,
                                        fg_warna);
                        }
                }
        }

        /* Teks field. */
        if (cb->font) {
                th = pg_font_tinggi(cb->font);
                if (th <= 0) th = 8;
                y = pg_font_baseline_tengah(cb->font, sh);
                pg_font_gambar_teks(cb->font,
                                     cb->buf ? cb->buf : "", s, 4, y,
                                     fg_warna);
                /* Cursor blink bila fokus. */
                if (w->fokus && cb->kursor >= 0 &&
                    cb->kursor <= cb->len) {
                        char sv = cb->buf[cb->kursor];
                        int cx;
                        cb->buf[cb->kursor] = 0;
                        cx = 4 + pg_font_lebar_teks(cb->font,
                                                      cb->buf);
                        cb->buf[cb->kursor] = sv;
                        pg_garis_v_permukaan(s, cx,
                                              y - pg_font_ascent(cb->font),
                                              y + 2, fg_warna);
                }
        }
}

static void pg_kg_catat_popup_v(pg_widget_t *w, pg_permukaan_t *s)
{
        pg_kotak_gabungan_t *cb = pg_kg_dari(w);
        int i, th, lh, ax, ay, bw, bh;
        int n_filter = 0;  /* jumlah item yang lolos filter */
        if (!cb->buka || cb->n_items <= 0 || !cb->font) return;
        th = pg_font_tinggi(cb->font);
        if (th <= 0) th = 8;
        lh = th + 4;
        bh = w->kotak.h;
        pg_widget_posisi_layar(w, &ax, &ay);
        bw = w->kotak.w;
        /* Hitung dulu berapa item yang lolos filter (bila ada teks). */
        for (i = 0; i < cb->n_items; i++) {
                if (cb->len > 0) {
                        /* Filter: item harus mengandung substring. */
                        if (cb->items[i] &&
                            strstr(cb->items[i], cb->buf) != NULL) {
                                n_filter++;
                        }
                } else {
                        n_filter++;
                }
        }
        if (n_filter == 0) {
                /* Tidak ada item cocok — tampilkan pesan "tidak ada". */
                pg_kotak_t pr = pg_buat_kotak(ax, ay + bh, bw, lh);
                pg_gambar_kotak_tumpul_isi_garis_aa(s, pr, 4,
                                                    PG_PUTIH,
                                                    PG_WARNA_HOVER_OUTLINE);
                pg_font_gambar_teks(cb->font, "(tidak ada)", s,
                                     ax + 6,
                                     ay + bh + pg_font_baseline_tengah(
                                             cb->font, lh),
                                     PG_ABU);
                return;
        }
        {
                int vis_idx = 0;
                pg_kotak_t pr = pg_buat_kotak(ax, ay + bh, bw,
                                                n_filter * lh);
                pg_warna_t popup_bg = PG_PUTIH;
                pg_warna_t popup_batas = PG_WARNA_HOVER_OUTLINE;
                pg_gambar_kotak_tumpul_isi_garis_aa(s, pr, 4,
                                                    popup_bg,
                                                    popup_batas);
                for (i = 0; i < cb->n_items; i++) {
                        int iy;
                        pg_warna_t bg = popup_bg;
                        pg_warna_t fg = PG_WARNA_TEKS_TOMBOL;
                        /* Skip item yang tidak lolos filter. */
                        if (cb->len > 0) {
                                if (!cb->items[i] ||
                                    strstr(cb->items[i], cb->buf)
                                    == NULL) {
                                        continue;
                                }
                        }
                        iy = ay + bh + vis_idx * lh;
                        if (i == cb->terpilih) {
                                bg = PG_WARNA_FOKUS;
                                fg = PG_PUTIH;
                        } else if (i == cb->hover_idx) {
                                bg = PG_WARNA_HOVER_ISI;
                        }
                        if (bg != popup_bg) {
                                pg_kotak_t hi = pg_buat_kotak(
                                        ax + 2, iy + 1,
                                        bw - 4, lh - 2);
                                pg_isi_permukaan_kotak(s, hi, bg);
                        }
                        if (cb->items[i]) {
                                pg_font_gambar_teks(cb->font,
                                     cb->items[i], s, ax + 6,
                                     iy + pg_font_baseline_tengah(
                                             cb->font, lh), fg);
                        }
                        vis_idx++;
                }
        }
}

static pg_bool pg_kg_peristiwa_v(pg_widget_t *w, const pg_aksi_t *e)
{
        pg_kotak_gabungan_t *cb = pg_kg_dari(w);
        int sw = w->kotak.w, sh = w->kotak.h;
        int bw = PG_KG_BTN_W, bl = sw - bw;
        int bh = w->kotak.h;

        if (!w->aktif) return PG_SALAH;

        /* GERAK: hover tracking. */
        if (e->tipe == PG_AKSI_TETIKUS_GERAK) {
                int mx = e->tetik_pos.x;
                int my = e->tetik_pos.y;
                if (cb->buka && my >= bh) {
                        int th = pg_font_tinggi(cb->font);
                        if (th <= 0) th = 8;
                        int lh = th + 4;
                        int idx = (my - bh) / lh;
                        if (idx >= 0 && idx < cb->n_items) {
                                if (cb->hover_idx != idx) {
                                        cb->hover_idx = idx;
                                        pg_widget_kotor(w);
                                }
                        }
                        return PG_BENAR;
                }
                {
                        /* Boundary check: reset hover bila mouse keluar. */
                        pg_bool in_widget = (mx >= 0 && mx < sw &&
                                              my >= 0 && my < sh) ?
                                PG_BENAR : PG_SALAH;
                        pg_bool nh = (in_widget && mx >= bl) ?
                                PG_BENAR : PG_SALAH;
                        pg_bool nfh = (in_widget && mx < bl) ?
                                PG_BENAR : PG_SALAH;
                        if (nh != cb->hover_btn ||
                            nfh != cb->hover_field) {
                                cb->hover_btn = nh;
                                cb->hover_field = nfh;
                                pg_widget_kotor(w);
                        }
                }
                return PG_SALAH;
        }

        if (e->tipe == PG_AKSI_TETIKUS_TEKAN &&
            e->tetik_tombol == PG_TETIKUS_KIRI) {
                pg_widget_fokus(w);
                /* Klik di popup area. */
                if (cb->buka && e->tetik_pos.y >= bh) {
                        int th = pg_font_tinggi(cb->font);
                        if (th <= 0) th = 8;
                        int lh = th + 4;
                        int idx = (e->tetik_pos.y - bh) / lh;
                        if (idx >= 0 && idx < cb->n_items) {
                                cb->terpilih = idx;
                                if (cb->items[idx]) {
                                        strncpy(cb->buf, cb->items[idx],
                                                cb->cap - 1);
                                        cb->buf[cb->cap - 1] = 0;
                                        cb->len = strlen(cb->buf);
                                        cb->kursor = cb->len;
                                }
                                pg_kg_fire(cb);
                        }
                        cb->buka = PG_SALAH;
                        cb->hover_idx = -1;
                        pg_widget_kotor(w);
                        return PG_BENAR;
                }
                /* Klik di button. */
                if (e->tetik_pos.x >= bl) {
                        cb->buka = !cb->buka;
                        cb->hover_idx = -1;
                        cb->tekan_btn = PG_BENAR;
                        pg_widget_kotor(w);
                } else {
                        /* Klik field: tutup popup bila terbuka. */
                        cb->buka = PG_SALAH;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }

        if (e->tipe == PG_AKSI_TETIKUS_LEPAS &&
            e->tetik_tombol == PG_TETIKUS_KIRI) {
                if (cb->tekan_btn) {
                        cb->tekan_btn = PG_SALAH;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }

        if (e->tipe != PG_AKSI_TOMBOL_TURUN) return PG_SALAH;
        if (!pg_widget_punya_fokus(w)) return PG_SALAH;

        /* Escape: tutup popup. */
        if (e->tombol == PG_TOMBOL_ESCAPE && cb->buka) {
                cb->buka = PG_SALAH;
                pg_widget_kotor(w);
                return PG_BENAR;
        }
        /* Panah bawah: buka popup atau pilih next. */
        if (e->tombol == PG_TOMBOL_BAWAH) {
                if (!cb->buka && cb->n_items > 0) {
                        cb->buka = PG_BENAR;
                        cb->hover_idx = (cb->terpilih >= 0) ?
                                cb->terpilih : 0;
                        pg_widget_kotor(w);
                        return PG_BENAR;
                }
                if (cb->buka && cb->n_items > 0) {
                        int next = (cb->hover_idx < 0) ? 0 :
                                (cb->hover_idx + 1) % cb->n_items;
                        cb->hover_idx = next;
                        pg_widget_kotor(w);
                        return PG_BENAR;
                }
        }
        /* Panah atas: pilih prev. */
        if (e->tombol == PG_TOMBOL_ATAS && cb->buka &&
            cb->n_items > 0) {
                int prev = (cb->hover_idx <= 0) ?
                        cb->n_items - 1 : cb->hover_idx - 1;
                cb->hover_idx = prev;
                pg_widget_kotor(w);
                return PG_BENAR;
        }
        /* Enter: pilih hover_idx bila popup buka. */
        if (e->tombol == PG_TOMBOL_ENTER && cb->buka &&
            cb->hover_idx >= 0 && cb->hover_idx < cb->n_items) {
                int idx = cb->hover_idx;
                cb->terpilih = idx;
                if (cb->items[idx]) {
                        strncpy(cb->buf, cb->items[idx],
                                cb->cap - 1);
                        cb->buf[cb->cap - 1] = 0;
                        cb->len = strlen(cb->buf);
                        cb->kursor = cb->len;
                }
                cb->buka = PG_SALAH;
                cb->hover_idx = -1;
                pg_widget_kotor(w);
                pg_kg_fire(cb);
                return PG_BENAR;
        }
        /* Backspace: hapus karakter sebelum kursor. */
        if (e->tombol == PG_TOMBOL_BACKSPACE) {
                if (cb->kursor > 0 && cb->len > 0) {
                        int i;
                        for (i = cb->kursor - 1;
                             i < cb->len - 1; i++)
                                cb->buf[i] = cb->buf[i + 1];
                        cb->len--;
                        cb->kursor--;
                        cb->buf[cb->len] = 0;
                        pg_widget_kotor(w);
                        pg_kg_fire(cb);
                }
                return PG_BENAR;
        }

        /* Ctrl+A: select all (pindahkan kursor ke akhir, mark sel). */
        if ((e->tombol == 'a' || e->tombol == 'A') &&
            (e->modifier & PG_MOD_CTRL)) {
                cb->kursor = cb->len;
                pg_widget_kotor(w);
                return PG_BENAR;
        }

        /* Ctrl+C: copy field teks ke clipboard. */
        if ((e->tombol == 'c' || e->tombol == 'C') &&
            (e->modifier & PG_MOD_CTRL)) {
                if (cb->klip && cb->buf) {
                        pg_papan_klip_tulis_teks(cb->klip, cb->buf);
                }
                return PG_BENAR;
        }

        /* Ctrl+V: paste dari clipboard ke field. */
        if ((e->tombol == 'v' || e->tombol == 'V') &&
            (e->modifier & PG_MOD_CTRL)) {
                if (cb->klip) {
                        char *clip = pg_papan_klip_baca_teks(cb->klip);
                        if (clip) {
                                int n = (int)strlen(clip);
                                int i, j;
                                if (cb->len + n < cb->cap) {
                                        for (i = cb->len;
                                             i >= cb->kursor; i--)
                                                cb->buf[i + n] =
                                                        cb->buf[i];
                                        for (j = 0; j < n; j++)
                                                cb->buf[cb->kursor + j] =
                                                        clip[j];
                                        cb->len += n;
                                        cb->kursor += n;
                                        cb->buf[cb->len] = 0;
                                        pg_widget_kotor(w);
                                        pg_kg_fire(cb);
                                }
                                free(clip);
                        }
                }
                return PG_BENAR;
        }

        /* Ctrl+X: cut field teks ke clipboard. */
        if ((e->tombol == 'x' || e->tombol == 'X') &&
            (e->modifier & PG_MOD_CTRL)) {
                if (cb->klip && cb->buf) {
                        pg_papan_klip_tulis_teks(cb->klip, cb->buf);
                }
                cb->buf[0] = 0;
                cb->len = 0;
                cb->kursor = 0;
                pg_widget_kotor(w);
                pg_kg_fire(cb);
                return PG_BENAR;
        }

        /* Insert char Unicode (UTF-8) atau ASCII fallback.
         * Saat user ketik, auto-buka popup untuk filter item. */
        {
                pg_u32 cp = e->unicode ? e->unicode : (pg_u32)e->tombol;
                if (cp >= 32) {
                        char tmp[4];
                        int n;
                        n = pg_utf8_encode(cp, tmp);
                        if (n > 0 && cb->len + n < cb->cap) {
                                int i, j;
                                for (i = cb->len;
                                     i >= cb->kursor; i--)
                                        cb->buf[i + n] = cb->buf[i];
                                for (j = 0; j < n; j++)
                                        cb->buf[cb->kursor + j] = tmp[j];
                                cb->len += n;
                                cb->kursor += n;
                                cb->buf[cb->len] = 0;
                                /* Auto-buka popup untuk filter. */
                                if (!cb->buka) {
                                        cb->buka = PG_BENAR;
                                        cb->hover_idx = -1;
                                }
                                pg_widget_kotor(w);
                                pg_kg_fire(cb);
                        }
                        return PG_BENAR;
                }
        }
        if (e->tombol == PG_TOMBOL_KIRI) {
                if (cb->kursor > 0) {
                        cb->kursor--;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }
        if (e->tombol == PG_TOMBOL_KANAN) {
                if (cb->kursor < cb->len) {
                        cb->kursor++;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }
        return PG_SALAH;
}

static void pg_kg_hancur_v(pg_widget_t *w)
{
        pg_kotak_gabungan_t *cb = pg_kg_dari(w);
        int i;
        if (cb->buf) free(cb->buf);
        for (i = 0; i < cb->n_items; i++)
                if (cb->items[i]) free(cb->items[i]);
}

static void pg_kg_bebas_v(pg_widget_t *w)
{
        free(w);
}

static pg_bool pg_kg_berisi_v(pg_widget_t *w, pg_titik_t p)
{
        pg_kotak_gabungan_t *cb = pg_kg_dari(w);
        if (pg_widget_berisi(w, p)) return PG_BENAR;
        if (!cb->buka || cb->n_items <= 0 || !cb->font) return PG_SALAH;
        {
                int bh = w->kotak.h;
                int th = pg_font_tinggi(cb->font);
                if (th <= 0) th = 8;
                int lh = th + 4;
                int pop_y0 = w->kotak.y + bh;
                int pop_y1 = pop_y0 + cb->n_items * lh;
                if (p.x >= w->kotak.x &&
                    p.x < w->kotak.x + w->kotak.w &&
                    p.y >= pop_y0 && p.y < pop_y1)
                        return PG_BENAR;
        }
        return PG_SALAH;
}

static const pg_widget_vtable_t pg_kg_vtable = {
        pg_kg_catat_v,
        pg_kg_peristiwa_v,
        NULL,
        pg_kg_hancur_v,
        pg_kg_catat_popup_v,
        pg_kg_berisi_v,
        pg_kg_bebas_v
};

pg_kotak_gabungan_t *pg_buat_kotak_gabungan(const char *awal,
                                              int pm, pg_font_t *font)
{
        pg_kotak_gabungan_t *cb;
        int cap;
        if (pm <= 0) pm = 64;
        cap = pm + 1;
        cb = (pg_kotak_gabungan_t *)calloc(1, sizeof(*cb));
        if (!cb) return NULL;
        pg_widget_init(&cb->base, PG_WIDGET_DASAR, &pg_kg_vtable);
        pg_widget_milik(&cb->base, PG_BENAR);
        cb->font = font;
        cb->cap = cap;
        cb->terpilih = -1;
        cb->hover_idx = -1;
        cb->base.latar = PG_TRANSPARAN;
        cb->base.radius = 0;  /* sudut tajam */
        cb->buf = (char *)calloc(cap, 1);
        if (!cb->buf) {
                free(cb);
                return NULL;
        }
        if (awal) {
                size_t n = strlen(awal);
                if (n > (size_t)pm) n = pm;
                memcpy(cb->buf, awal, n);
                cb->buf[n] = 0;
                cb->len = (int)n;
                cb->kursor = (int)n;
        }
        pg_widget_setel_ukuran_min(&cb->base, 80, 24);
        return cb;
}

void pg_kotak_gabungan_hancur(pg_kotak_gabungan_t *cb)
{
        if (!cb) return;
        pg_widget_hancur(&cb->base);
        free(cb);
}

const char *pg_kotak_gabungan_ambil_teks(pg_kotak_gabungan_t *cb)
{
        return cb ? (cb->buf ? cb->buf : "") : "";
}

void pg_kotak_gabungan_setel_teks(pg_kotak_gabungan_t *cb,
                                    const char *t)
{
        if (!cb) return;
        if (!t) {
                cb->buf[0] = 0;
                cb->len = 0;
                cb->kursor = 0;
                pg_widget_kotor(&cb->base);
                return;
        }
        {
                size_t n = strlen(t);
                if (n >= (size_t)cb->cap) n = cb->cap - 1;
                memcpy(cb->buf, t, n);
                cb->buf[n] = 0;
                cb->len = (int)n;
                cb->kursor = (int)n;
                pg_widget_kotor(&cb->base);
        }
}

void pg_kotak_gabungan_tambah_item(pg_kotak_gabungan_t *cb,
                                     const char *t)
{
        size_t n;
        if (!cb || !t || cb->n_items >= PG_KG_MAX_ITEMS) return;
        n = strlen(t) + 1;
        cb->items[cb->n_items] = (char *)malloc(n);
        if (!cb->items[cb->n_items]) return;
        memcpy(cb->items[cb->n_items], t, n);
        cb->n_items++;
}

int pg_kotak_gabungan_jumlah_item(pg_kotak_gabungan_t *cb)
{
        return cb ? cb->n_items : 0;
}

const char *pg_kotak_gabungan_item(pg_kotak_gabungan_t *cb, int idx)
{
        if (!cb || idx < 0 || idx >= cb->n_items) return NULL;
        return cb->items[idx];
}

int pg_kotak_gabungan_terpilih(pg_kotak_gabungan_t *cb)
{
        return cb ? cb->terpilih : -1;
}

void pg_kotak_gabungan_setel_terpilih(pg_kotak_gabungan_t *cb, int idx)
{
        if (!cb || idx < 0 || idx >= cb->n_items) return;
        cb->terpilih = idx;
        if (cb->items[idx]) {
                strncpy(cb->buf, cb->items[idx], cb->cap - 1);
                cb->buf[cb->cap - 1] = 0;
                cb->len = (int)strlen(cb->buf);
                cb->kursor = cb->len;
        }
        pg_widget_kotor(&cb->base);
}

pg_bool pg_kotak_gabungan_terbuka(pg_kotak_gabungan_t *cb)
{
        return cb ? cb->buka : PG_SALAH;
}

void pg_kotak_gabungan_buka(pg_kotak_gabungan_t *cb)
{
        if (!cb || cb->buka) return;
        cb->buka = PG_BENAR;
        cb->hover_idx = (cb->terpilih >= 0) ? cb->terpilih : 0;
        pg_widget_kotor(&cb->base);
}

void pg_kotak_gabungan_tutup(pg_kotak_gabungan_t *cb)
{
        if (!cb || !cb->buka) return;
        cb->buka = PG_SALAH;
        cb->hover_idx = -1;
        pg_widget_kotor(&cb->base);
}

void pg_kotak_gabungan_saatberubah(pg_kotak_gabungan_t *cb,
                                     pg_kotak_gabungan_cb fn, void *ctx)
{
        if (!cb) return;
        cb->cb = fn;
        cb->ctx = ctx;
}

void pg_kotak_gabungan_setel_aktif(pg_kotak_gabungan_t *cb,
                                     pg_bool aktif)
{
        if (!cb) return;
        if (aktif) {
                pg_widget_aktifkan(&cb->base);
        } else {
                pg_widget_nonaktifkan(&cb->base);
                if (cb->base.fokus) pg_widget_blur(&cb->base);
                cb->buka = PG_SALAH;
                cb->hover_idx = -1;
                cb->hover_btn = PG_SALAH;
                cb->tekan_btn = PG_SALAH;
                cb->hover_field = PG_SALAH;
                pg_widget_kotor(&cb->base);
        }
}

pg_bool pg_kotak_gabungan_aktif(pg_kotak_gabungan_t *cb)
{
        return cb ? cb->base.aktif : PG_SALAH;
}

void pg_kotak_gabungan_setel_papan_klip(pg_kotak_gabungan_t *cb,
                                          pg_papan_klip_t *klip)
{
        if (!cb) return;
        cb->klip = klip;
}

pg_widget_t *pg_kotak_gabungan_widget(pg_kotak_gabungan_t *cb)
{
        return cb ? &cb->base : NULL;
}
