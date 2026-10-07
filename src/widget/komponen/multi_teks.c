/* ----------------------------------------------------------------------------------------------
 * pigura widget: multi_teks.c - widget multiline text editor (multi teks)
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/multi_teks.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/aksi.h"
#include "pigura/widget.h"
#include "pigura/papan_klip.h"
#include "pigura/utf8.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PG_MT_BLINK_MS  500

struct pg_multi_teks {
        pg_widget_t base;
        char *buf;
        int cap, len, kursor;
        int sel_mulai, sel_akhir;
        pg_bool menyeret;
        pg_bool hover;
        pg_bool bungkus;   /* word wrap enable */
        pg_warna_t fg, batas, sel_bg;
        pg_font_t *font;
        pg_multi_teks_cb cb;
        void *ctx;
        pg_papan_klip_t *klip;
        /* Cursor blink state. */
        pg_u32 blink_acu_ms;
        pg_bool blink_nyala;
};

static pg_multi_teks_t *pg_mt_dari(pg_widget_t *w)
{
        return (pg_multi_teks_t *)w;
}

static pg_u32 pg_mt_sekarang_ms(void)
{
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (pg_u32)((pg_u64)ts.tv_sec * 1000ULL +
                         (pg_u64)ts.tv_nsec / 1000000ULL);
}

static void pg_mt_blink_reset(pg_multi_teks_t *t)
{
        t->blink_acu_ms = pg_mt_sekarang_ms();
        t->blink_nyala = PG_BENAR;
}

static pg_bool pg_mt_blink_aktif(pg_multi_teks_t *t)
{
        pg_u32 now = pg_mt_sekarang_ms();
        pg_u32 elapsed = now - t->blink_acu_ms;
        if (elapsed >= PG_MT_BLINK_MS * 2) {
                /* Cycle baru. */
                t->blink_acu_ms = now;
                t->blink_nyala = PG_BENAR;
        } else if (elapsed >= PG_MT_BLINK_MS) {
                t->blink_nyala = PG_SALAH;
        }
        return t->blink_nyala;
}

/* Layout baris visual. Untuk word wrap, hitung berapa karakter
 * yang muat dalam lebar widget. Tanpa wrap, baris hanya pecah pada
 * newline (10).
 *
 * Output: array baris[i] = { start_offset, end_offset } di buf.
 * end_offset exclusive (points to char AFTER line, atau newline).
 *
 * Maks PG_MT_MAX_BARIS baris. */
#define PG_MT_MAX_BARIS 256
typedef struct { int start; int end; } pg_mt_baris_t;

static int pg_mt_hitung_layout(pg_multi_teks_t *t, int sw,
                                pg_mt_baris_t *baris, int maks_baris)
{
        int n = 0;
        int i = 0;
        int max_x = sw - 8;  /* padding 4 kiri + 4 kanan */
        if (!t->font || t->len == 0) {
                if (maks_baris > 0) {
                        baris[0].start = 0;
                        baris[0].end = 0;
                        return 1;
                }
                return 0;
        }
        while (i <= t->len && n < maks_baris) {
                int line_end = i;
                int j;
                /* Cari akhir baris natural (newline atau end). */
                for (j = i; j <= t->len; j++) {
                        if (j == t->len || t->buf[j] == 10) {
                                line_end = j;
                                break;
                        }
                }
                /* Bila word wrap aktif, mungkin perlu split baris. */
                if (t->bungkus && t->font) {
                        int last_fit = i;
                        int k;
                        for (k = i; k <= line_end; k++) {
                                char sv;
                                int w_k;
                                sv = t->buf[k];
                                t->buf[k] = 0;
                                w_k = 4 + pg_font_lebar_teks(
                                        t->font, t->buf + i);
                                t->buf[k] = sv;
                                if (w_k > max_x && k > i) {
                                        break;
                                }
                                last_fit = k;
                        }
                        if (last_fit < line_end) {
                                line_end = last_fit + 1;
                        }
                }
                if (n < maks_baris) {
                        baris[n].start = i;
                        baris[n].end = line_end;
                        n++;
                }
                if (line_end < t->len && t->buf[line_end] == 10) {
                        i = line_end + 1;
                } else {
                        i = line_end;
                        if (i >= t->len) break;
                }
        }
        return n;
}

static int smin(pg_multi_teks_t *t)
{
        return t->sel_mulai < 0 ? t->kursor :
                (t->sel_mulai < t->sel_akhir ?
                  t->sel_mulai : t->sel_akhir);
}
static int smax(pg_multi_teks_t *t)
{
        return t->sel_mulai < 0 ? t->kursor :
                (t->sel_mulai > t->sel_akhir ?
                  t->sel_mulai : t->sel_akhir);
}
static int hsel(pg_multi_teks_t *t)
{
        return t->sel_mulai >= 0 && t->sel_mulai != t->sel_akhir;
}
static void csel(pg_multi_teks_t *t)
{
        t->sel_mulai = -1;
        t->sel_akhir = -1;
        t->menyeret = PG_SALAH;
}
static void dsel(pg_multi_teks_t *t)
{
        int lo = smin(t), hi = smax(t), i;
        for (i = lo; i < t->len - (hi - lo); i++)
                t->buf[i] = t->buf[i + (hi - lo)];
        t->len -= (hi - lo);
        t->buf[t->len] = 0;
        t->kursor = lo;
        csel(t);
}
static void fire(pg_multi_teks_t *t)
{
        if (t->cb) t->cb(t, t->ctx);
}

static void pg_mt_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
        pg_multi_teks_t *t = pg_mt_dari(w);
        int sw, sh, th, lh, asc;
        int radius = w->radius;
        int y;
        pg_kotak_t r;
        pg_warna_t latar_warna, batas_warna, fg_warna, sel_warna;

        sw = pg_permukaan_lebar(s);
        sh = pg_permukaan_tinggi(s);

        if (!w->aktif) {
                latar_warna = PG_WARNA_NONAKTIF_ISI;
                batas_warna = PG_ABU_TERANG;
                fg_warna    = PG_WARNA_NONAKTIF_TEKS;
                sel_warna   = PG_ABU_TERANG;
        } else {
                latar_warna = PG_PUTIH;  /* text area pakai PUTIH */
                fg_warna    = PG_WARNA_TEKS_TOMBOL;
                sel_warna   = PG_WARNA_FOKUS;
                if (w->fokus) batas_warna = PG_WARNA_FOKUS;
                else if (t->hover) batas_warna = PG_RGB(0x6E, 0x6E, 0x6E);
                else batas_warna = PG_WARNA_HOVER_OUTLINE;
        }

        /* Latar cerdas. */
        if (PG_A(w->latar) < 255) {
                pg_isi_permukaan(s, PG_TRANSPARAN);
        }

        /* Latar + outline. */
        r = pg_buat_kotak(0, 0, sw, sh);
        pg_gambar_kotak_tumpul_isi_garis_aa(s, r, radius,
                                            latar_warna, batas_warna);

        if (!t->font) return;
        th = pg_font_tinggi(t->font);
        if (th <= 0) th = 8;
        lh = th + 2;
        asc = pg_font_ascent(t->font);
        (void)y;

        /* Hitung layout baris (dengan opsi word wrap). */
        {
                pg_mt_baris_t baris[PG_MT_MAX_BARIS];
                int n_baris = pg_mt_hitung_layout(t, sw, baris,
                                                   PG_MT_MAX_BARIS);
                int li;

                /* Selection highlight per visual line. */
                if (hsel(t)) {
                        int lo = smin(t), hi = smax(t);
                        for (li = 0; li < n_baris; li++) {
                                int sy = 4 + asc + li * lh;
                                int ls = baris[li].start;
                                int le = baris[li].end;
                                if (sy >= sh) break;
                                if (ls < hi && le > lo) {
                                        int s0 = (lo > ls) ? lo : ls;
                                        int e0 = (hi < le) ? hi : le;
                                        int x0 = 4, x1 = 4;
                                        if (s0 > ls) {
                                                char sv = t->buf[s0];
                                                t->buf[s0] = 0;
                                                x0 = 4 +
                                                  pg_font_lebar_teks(
                                                  t->font,
                                                  t->buf + ls);
                                                t->buf[s0] = sv;
                                        }
                                        if (e0 < le) {
                                                char sv = t->buf[e0];
                                                t->buf[e0] = 0;
                                                x1 = 4 +
                                                  pg_font_lebar_teks(
                                                  t->font,
                                                  t->buf + ls);
                                                t->buf[e0] = sv;
                                        } else {
                                                char sv = t->buf[e0];
                                                t->buf[e0] = 0;
                                                x1 = 4 +
                                                  pg_font_lebar_teks(
                                                  t->font,
                                                  t->buf + ls);
                                                t->buf[e0] = sv;
                                        }
                                        if (x1 > x0) {
                                                pg_isi_permukaan_kotak(
                                                  s,
                                                  pg_buat_kotak(
                                                  x0, sy - asc,
                                                  x1 - x0, th + 1),
                                                  sel_warna);
                                        }
                                }
                        }
                }

                /* Render teks per visual line. */
                for (li = 0; li < n_baris; li++) {
                        int sy = 4 + asc + li * lh;
                        int ls = baris[li].start;
                        int le = baris[li].end;
                        if (sy >= sh) break;
                        if (le > ls) {
                                char sv = t->buf[le];
                                t->buf[le] = 0;
                                pg_font_gambar_teks(t->font,
                                     t->buf + ls, s, 4,
                                     sy, fg_warna);
                                t->buf[le] = sv;
                        }
                }

                /* Cursor blink bila fokus — pakai layout untuk
                 * cari baris + x position. */
                if (w->fokus && t->kursor >= 0 &&
                    t->kursor <= t->len && pg_mt_blink_aktif(t)) {
                        int cx_px = 4, cy_px = 4 + asc;
                        /* Cari baris yang mengandung kursor. */
                        for (li = 0; li < n_baris; li++) {
                                int ls = baris[li].start;
                                int le = baris[li].end;
                                if (t->kursor >= ls &&
                                    t->kursor <= le) {
                                        char sv = t->buf[t->kursor];
                                        int line_x;
                                        t->buf[t->kursor] = 0;
                                        line_x = pg_font_lebar_teks(
                                                t->font, t->buf + ls);
                                        t->buf[t->kursor] = sv;
                                        cx_px = 4 + line_x;
                                        cy_px = 4 + asc + li * lh;
                                        break;
                                }
                        }
                        pg_garis_v_permukaan(s, cx_px,
                                cy_px - asc, cy_px + 2, fg_warna);
                }
        }
}

/* Hitung cursor offset dari klik (x, y) — pakai layout yang sama
 * dengan render supaya konsisten dengan word wrap. */
static int kursor_dari_klik(pg_multi_teks_t *t, int x, int y, int sw)
{
        int th, lh, asc, line;
        pg_mt_baris_t baris[PG_MT_MAX_BARIS];
        int n_baris;
        if (!t->font) return 0;
        th = pg_font_tinggi(t->font);
        if (th <= 0) th = 8;
        lh = th + 2;
        asc = pg_font_ascent(t->font);
        /* Cari baris visual yang diklik. */
        if (y < 4 + asc) line = 0;
        else line = (y - 4 - asc) / lh;
        if (line < 0) line = 0;
        n_baris = pg_mt_hitung_layout(t, sw, baris, PG_MT_MAX_BARIS);
        if (n_baris == 0) return 0;
        if (line >= n_baris) line = n_baris - 1;
        /* Di baris 'line', cari offset karakter terdekat dengan x. */
        {
                int ls = baris[line].start;
                int le = baris[line].end;
                int best = ls;
                int best_dist = 999999;
                int k;
                for (k = ls; k <= le; k++) {
                        char sv = t->buf[k];
                        int w_k;
                        t->buf[k] = 0;
                        w_k = 4 + pg_font_lebar_teks(t->font,
                                                       t->buf + ls);
                        t->buf[k] = sv;
                        {
                                int dist = x - w_k;
                                if (dist < 0) dist = -dist;
                                if (dist < best_dist) {
                                        best_dist = dist;
                                        best = k;
                                }
                        }
                }
                return best;
        }
}

static pg_bool pg_mt_peristiwa_v(pg_widget_t *w, const pg_aksi_t *e)
{
        pg_multi_teks_t *t = pg_mt_dari(w);
        int sw, sh;

        if (!w->aktif) return PG_SALAH;

        sw = w->kotak.w;
        sh = w->kotak.h;

        /* GERAK: update hover + drag selection. */
        if (e->tipe == PG_AKSI_TETIKUS_GERAK) {
                int mx = e->tetik_pos.x;
                int my = e->tetik_pos.y;
                /* Boundary check: hover hanya bila mouse di widget. */
                pg_bool new_hover = (mx >= 0 && mx < sw &&
                                      my >= 0 && my < sh) ?
                        PG_BENAR : PG_SALAH;
                if (!t->menyeret && new_hover != t->hover) {
                        t->hover = new_hover;
                        pg_widget_kotor(w);
                }
                if (t->menyeret) {
                        int pos = kursor_dari_klik(t,
                                e->tetik_pos.x, e->tetik_pos.y, sw);
                        if (pos != t->sel_akhir) {
                                t->sel_akhir = pos;
                                t->kursor = pos;
                                pg_widget_kotor(w);
                        }
                        return PG_BENAR;
                }
                return PG_SALAH;
        }

        if (e->tipe == PG_AKSI_TETIKUS_TEKAN &&
            e->tetik_tombol == PG_TETIKUS_KIRI) {
                int pos = kursor_dari_klik(t, e->tetik_pos.x,
                                             e->tetik_pos.y, sw);
                pg_widget_fokus(w);
                if (e->modifier & PG_MOD_SHIFT) {
                        if (t->sel_mulai < 0)
                                t->sel_mulai = t->kursor;
                        t->sel_akhir = pos;
                } else {
                        t->sel_mulai = pos;
                        t->sel_akhir = pos;
                        t->menyeret = PG_BENAR;
                }
                t->kursor = pos;
                pg_widget_kotor(w);
                return PG_BENAR;
        }

        if (e->tipe == PG_AKSI_TETIKUS_LEPAS &&
            e->tetik_tombol == PG_TETIKUS_KIRI) {
                if (t->menyeret) {
                        t->menyeret = PG_SALAH;
                        if (t->sel_mulai == t->sel_akhir) csel(t);
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }

        if (e->tipe != PG_AKSI_TOMBOL_TURUN) return PG_SALAH;
        if (!pg_widget_punya_fokus(w)) return PG_SALAH;

        /* Setiap keypress reset blink supaya cursor langsung terlihat. */
        pg_mt_blink_reset(t);

        /* Ctrl+A: select all. */
        if ((e->tombol == 'a' || e->tombol == 'A') &&
            (e->modifier & PG_MOD_CTRL)) {
                t->sel_mulai = 0;
                t->sel_akhir = t->len;
                t->kursor = t->len;
                pg_widget_kotor(w);
                return PG_BENAR;
        }

        /* Ctrl+C: copy selection. */
        if ((e->tombol == 'c' || e->tombol == 'C') &&
            (e->modifier & PG_MOD_CTRL)) {
                if (t->klip && hsel(t)) {
                        int lo = smin(t), hi = smax(t);
                        int n = hi - lo;
                        if (n > 0) {
                                char *tmp = (char *)malloc((size_t)n + 1);
                                if (tmp) {
                                        memcpy(tmp, t->buf + lo,
                                                (size_t)n);
                                        tmp[n] = 0;
                                        pg_papan_klip_tulis_teks(t->klip,
                                                                   tmp);
                                        free(tmp);
                                }
                        }
                }
                return PG_BENAR;
        }

        /* Ctrl+X: cut selection. */
        if ((e->tombol == 'x' || e->tombol == 'X') &&
            (e->modifier & PG_MOD_CTRL)) {
                if (hsel(t)) {
                        int lo = smin(t), hi = smax(t);
                        int n = hi - lo;
                        if (t->klip && n > 0) {
                                char *tmp = (char *)malloc((size_t)n + 1);
                                if (tmp) {
                                        memcpy(tmp, t->buf + lo,
                                                (size_t)n);
                                        tmp[n] = 0;
                                        pg_papan_klip_tulis_teks(t->klip,
                                                                   tmp);
                                        free(tmp);
                                }
                        }
                        dsel(t);
                        pg_widget_kotor(w);
                        fire(t);
                }
                return PG_BENAR;
        }

        /* Ctrl+V: paste dari clipboard. */
        if ((e->tombol == 'v' || e->tombol == 'V') &&
            (e->modifier & PG_MOD_CTRL)) {
                if (t->klip) {
                        char *clip = pg_papan_klip_baca_teks(t->klip);
                        if (clip) {
                                int n = (int)strlen(clip);
                                if (hsel(t)) dsel(t);
                                if (t->len + n < t->cap) {
                                        int i, j;
                                        for (i = t->len;
                                             i >= t->kursor; i--)
                                                t->buf[i + n] =
                                                        t->buf[i];
                                        for (j = 0; j < n; j++)
                                                t->buf[t->kursor + j] =
                                                        clip[j];
                                        t->len += n;
                                        t->kursor += n;
                                        t->buf[t->len] = 0;
                                        pg_widget_kotor(w);
                                        fire(t);
                                }
                                free(clip);
                        }
                }
                return PG_BENAR;
        }

        /* Backspace: hapus karakter sebelum cursor (atau selection). */
        if (e->tombol == PG_TOMBOL_BACKSPACE) {
                if (hsel(t)) {
                        dsel(t);
                } else if (t->kursor > 0 && t->len > 0) {
                        int i;
                        for (i = t->kursor - 1;
                             i < t->len - 1; i++)
                                t->buf[i] = t->buf[i + 1];
                        t->len--;
                        t->kursor--;
                        t->buf[t->len] = 0;
                }
                pg_widget_kotor(w);
                fire(t);
                return PG_BENAR;
        }

        /* Delete: hapus karakter setelah cursor (atau selection). */
        if (e->tombol == PG_TOMBOL_DELETE) {
                if (hsel(t)) {
                        dsel(t);
                } else if (t->kursor < t->len) {
                        int i;
                        for (i = t->kursor;
                             i < t->len - 1; i++)
                                t->buf[i] = t->buf[i + 1];
                        t->len--;
                        t->buf[t->len] = 0;
                }
                pg_widget_kotor(w);
                fire(t);
                return PG_BENAR;
        }

        /* Tab: insert 4 spasi (indent). Shift+Tab: outdent (hapus
         * leading whitespace di baris saat ini).
         * Override base widget.c Tab handler yang biasanya pindah
         * focus — di multi_teks, Tab dipakai untuk indent teks. */
        if (e->tombol == PG_TOMBOL_TAB) {
                if (e->modifier & PG_MOD_SHIFT) {
                        /* Outdent: hapus leading whitespace di baris
                         * saat ini (sampai 4 spasi atau 1 tab). */
                        int line_start = t->kursor;
                        int i, n_hapus = 0;
                        for (i = t->kursor - 1; i >= 0; i--) {
                                if (t->buf[i] == 10) {
                                        line_start = i + 1;
                                        break;
                                }
                                if (i == 0) { line_start = 0; break; }
                        }
                        for (i = line_start;
                             i < t->len && n_hapus < 4; i++) {
                                if (t->buf[i] == ' ') n_hapus++;
                                else if (t->buf[i] == '\t') {
                                        n_hapus++; break;
                                } else break;
                        }
                        if (n_hapus > 0) {
                                int j;
                                for (j = line_start;
                                     j + n_hapus <= t->len; j++)
                                        t->buf[j] = t->buf[j + n_hapus];
                                t->len -= n_hapus;
                                t->kursor -= n_hapus;
                                if (t->kursor < line_start)
                                        t->kursor = line_start;
                                t->buf[t->len] = 0;
                                pg_widget_kotor(w);
                                fire(t);
                        }
                        return PG_BENAR;
                }
                /* Indent: insert 4 spasi di posisi kursor. */
                if (t->len + 4 < t->cap) {
                        int i, j;
                        if (hsel(t)) dsel(t);
                        for (i = t->len;
                             i >= t->kursor; i--)
                                t->buf[i + 4] = t->buf[i];
                        for (j = 0; j < 4; j++)
                                t->buf[t->kursor + j] = ' ';
                        t->len += 4;
                        t->kursor += 4;
                        t->buf[t->len] = 0;
                        pg_widget_kotor(w);
                        fire(t);
                }
                return PG_BENAR;
        }

        /* Enter: insert newline + auto-indent (salin whitespace
         * dari awal baris saat ini). */
        if (e->tombol == PG_TOMBOL_ENTER) {
                int line_start = t->kursor;
                int indent_n = 0;
                int i, j;
                /* Cari awal baris saat ini. */
                for (i = t->kursor - 1; i >= 0; i--) {
                        if (t->buf[i] == 10) {
                                line_start = i + 1;
                                break;
                        }
                        if (i == 0) { line_start = 0; break; }
                }
                /* Hitung whitespace di awal baris (auto-indent). */
                for (i = line_start; i < t->kursor; i++) {
                        if (t->buf[i] == ' ' || t->buf[i] == '\t') {
                                indent_n++;
                        } else break;
                }
                if (t->len + 1 + indent_n < t->cap) {
                        if (hsel(t)) dsel(t);
                        /* Shift kanan untuk newline + indent. */
                        for (i = t->len;
                             i >= t->kursor; i--)
                                t->buf[i + 1 + indent_n] = t->buf[i];
                        /* Insert newline. */
                        t->buf[t->kursor] = 10;
                        /* Insert indent (copy whitespace dari baris
                         * sebelumnya). */
                        for (j = 0; j < indent_n; j++)
                                t->buf[t->kursor + 1 + j] =
                                        t->buf[line_start + j];
                        t->len += 1 + indent_n;
                        t->kursor += 1 + indent_n;
                        t->buf[t->len] = 0;
                        pg_widget_kotor(w);
                        fire(t);
                }
                return PG_BENAR;
        }

        /* Panah kiri/kanan. */
        if (e->tombol == PG_TOMBOL_KIRI) {
                if (t->kursor > 0) {
                        t->kursor--;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }
        if (e->tombol == PG_TOMBOL_KANAN) {
                if (t->kursor < t->len) {
                        t->kursor++;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }

        /* Home/End. */
        if (e->tombol == PG_TOMBOL_HOME) {
                /* Pindah ke awal baris saat ini. */
                int i;
                for (i = t->kursor - 1; i >= 0; i--) {
                        if (t->buf[i] == 10) {
                                t->kursor = i + 1;
                                pg_widget_kotor(w);
                                return PG_BENAR;
                        }
                }
                t->kursor = 0;
                pg_widget_kotor(w);
                return PG_BENAR;
        }
        if (e->tombol == PG_TOMBOL_END) {
                int i;
                for (i = t->kursor; i < t->len; i++) {
                        if (t->buf[i] == 10) {
                                t->kursor = i;
                                pg_widget_kotor(w);
                                return PG_BENAR;
                        }
                }
                t->kursor = t->len;
                pg_widget_kotor(w);
                return PG_BENAR;
        }

        /* Panah ATAS: pindah ke baris sebelumnya (kalau ada). */
        if (e->tombol == PG_TOMBOL_ATAS) {
                int i, line_start = t->kursor;
                /* Cari awal baris saat ini. */
                for (i = t->kursor - 1; i >= 0; i--) {
                        if (t->buf[i] == 10) {
                                line_start = i + 1;
                                break;
                        }
                        if (i == 0) { line_start = 0; break; }
                }
                if (t->kursor > 0 && line_start > 0) {
                        /* Ada baris sebelumnya. Cari posisi x relatif. */
                        int x_offset = t->kursor - line_start;
                        int prev_line_end = line_start - 1;
                        int prev_line_start = 0;
                        int j;
                        for (j = prev_line_end - 1; j >= 0; j--) {
                                if (t->buf[j] == 10) {
                                        prev_line_start = j + 1;
                                        break;
                                }
                                if (j == 0) { prev_line_start = 0; break; }
                        }
                        {
                                int prev_len = prev_line_end -
                                        prev_line_start;
                                int new_pos = prev_line_start +
                                        (x_offset < prev_len ?
                                          x_offset : prev_len);
                                /* Shift+Up: extend selection. */
                                if (e->modifier & PG_MOD_SHIFT) {
                                        if (t->sel_mulai < 0)
                                                t->sel_mulai = t->kursor;
                                        t->sel_akhir = new_pos;
                                } else {
                                        csel(t);
                                }
                                t->kursor = new_pos;
                                pg_widget_kotor(w);
                        }
                }
                return PG_BENAR;
        }

        /* Panah BAWAH: pindah ke baris setelahnya (kalau ada). */
        if (e->tombol == PG_TOMBOL_BAWAH) {
                int i, line_start = 0;
                int next_line_start = -1;
                /* Cari awal baris saat ini. */
                for (i = t->kursor - 1; i >= 0; i--) {
                        if (t->buf[i] == 10) {
                                line_start = i + 1;
                                break;
                        }
                        if (i == 0) { line_start = 0; break; }
                }
                /* Cari baris berikutnya. */
                for (i = t->kursor; i < t->len; i++) {
                        if (t->buf[i] == 10) {
                                next_line_start = i + 1;
                                break;
                        }
                }
                if (next_line_start >= 0 && next_line_start <= t->len) {
                        int x_offset = t->kursor - line_start;
                        int next_line_end = t->len;
                        int j, new_pos;
                        for (j = next_line_start; j < t->len; j++) {
                                if (t->buf[j] == 10) {
                                        next_line_end = j;
                                        break;
                                }
                        }
                        {
                                int next_len = next_line_end -
                                        next_line_start;
                                new_pos = next_line_start +
                                        (x_offset < next_len ?
                                          x_offset : next_len);
                                if (e->modifier & PG_MOD_SHIFT) {
                                        if (t->sel_mulai < 0)
                                                t->sel_mulai = t->kursor;
                                        t->sel_akhir = new_pos;
                                } else {
                                        csel(t);
                                }
                                t->kursor = new_pos;
                                pg_widget_kotor(w);
                        }
                }
                return PG_BENAR;
        }

        /* Page Up: pindah 10 baris ke atas (atau ke awal). */
        if (e->tombol == PG_TOMBOL_PAGEUP) {
                int i, count = 0;
                for (i = t->kursor - 1; i >= 0 && count < 10; i--) {
                        if (t->buf[i] == 10) count++;
                }
                t->kursor = (i < 0) ? 0 : i + 1;
                if (!(e->modifier & PG_MOD_SHIFT)) csel(t);
                pg_widget_kotor(w);
                return PG_BENAR;
        }

        /* Page Down: pindah 10 baris ke bawah (atau ke akhir). */
        if (e->tombol == PG_TOMBOL_PAGEDOWN) {
                int i, count = 0;
                for (i = t->kursor; i < t->len && count < 10; i++) {
                        if (t->buf[i] == 10) count++;
                }
                t->kursor = i;
                if (t->kursor > t->len) t->kursor = t->len;
                if (!(e->modifier & PG_MOD_SHIFT)) csel(t);
                pg_widget_kotor(w);
                return PG_BENAR;
        }

        /* Insert char Unicode (UTF-8 encoded) atau ASCII fallback. */
        {
                pg_u32 cp = e->unicode ? e->unicode : (pg_u32)e->tombol;
                if (cp >= 32) {
                        char tmp[4];
                        int n;
                        n = pg_utf8_encode(cp, tmp);
                        if (n > 0 && t->len + n < t->cap) {
                                int i, j;
                                if (hsel(t)) dsel(t);
                                for (i = t->len;
                                     i >= t->kursor; i--)
                                        t->buf[i + n] = t->buf[i];
                                for (j = 0; j < n; j++)
                                        t->buf[t->kursor + j] = tmp[j];
                                t->len += n;
                                t->kursor += n;
                                t->buf[t->len] = 0;
                                pg_widget_kotor(w);
                                fire(t);
                        }
                        return PG_BENAR;
                }
        }
        return PG_SALAH;
}

static void pg_mt_hancur_v(pg_widget_t *w)
{
        pg_multi_teks_t *t = pg_mt_dari(w);
        if (t->buf) free(t->buf);
}

static void pg_mt_bebas_v(pg_widget_t *w)
{
        free(w);
}

static const pg_widget_vtable_t pg_mt_vtable = {
        pg_mt_catat_v,
        pg_mt_peristiwa_v,
        NULL,
        pg_mt_hancur_v,
        NULL,
        NULL,
        pg_mt_bebas_v
};

pg_multi_teks_t *pg_buat_multi_teks(const char *awal, int pm,
                                      pg_font_t *font)
{
        pg_multi_teks_t *t;
        int cap;
        if (pm <= 0) pm = 1024;
        cap = pm + 1;
        t = (pg_multi_teks_t *)calloc(1, sizeof(*t));
        if (!t) return NULL;
        pg_widget_init(&t->base, PG_WIDGET_DASAR, &pg_mt_vtable);
        pg_widget_milik(&t->base, PG_BENAR);
        t->font = font;
        t->cap = cap;
        t->sel_mulai = -1;
        t->sel_akhir = -1;
        t->base.latar = PG_TRANSPARAN;
        t->base.radius = 0;  /* sudut tajam */
        t->buf = (char *)calloc(cap, 1);
        if (!t->buf) {
                free(t);
                return NULL;
        }
        if (awal) {
                size_t n = strlen(awal);
                if (n > (size_t)pm) n = pm;
                memcpy(t->buf, awal, n);
                t->buf[n] = 0;
                t->len = (int)n;
                t->kursor = (int)n;
        }
        pg_widget_setel_ukuran_min(&t->base, 80, 60);
        return t;
}

void pg_multi_teks_hancur(pg_multi_teks_t *t)
{
        if (!t) return;
        pg_widget_hancur(&t->base);
        free(t);
}

const char *pg_multi_teks_ambil_teks(pg_multi_teks_t *t)
{
        return t ? (t->buf ? t->buf : "") : "";
}

void pg_multi_teks_setel_teks(pg_multi_teks_t *t, const char *t_)
{
        if (!t) return;
        if (!t_) {
                t->buf[0] = 0;
                t->len = 0;
                t->kursor = 0;
                csel(t);
                pg_widget_kotor(&t->base);
                return;
        }
        {
                size_t n = strlen(t_);
                if (n >= (size_t)t->cap) n = t->cap - 1;
                memcpy(t->buf, t_, n);
                t->buf[n] = 0;
                t->len = (int)n;
                t->kursor = (int)n;
                csel(t);
                pg_widget_kotor(&t->base);
        }
}

void pg_multi_teks_saatberubah(pg_multi_teks_t *t,
                                  pg_multi_teks_cb cb, void *ctx)
{
        if (!t) return;
        t->cb = cb;
        t->ctx = ctx;
}

void pg_multi_teks_setel_papan_klip(pg_multi_teks_t *t,
                                       pg_papan_klip_t *klip)
{
        if (!t) return;
        t->klip = klip;
}

void pg_multi_teks_setel_aktif(pg_multi_teks_t *t, pg_bool aktif)
{
        if (!t) return;
        if (aktif) {
                pg_widget_aktifkan(&t->base);
        } else {
                pg_widget_nonaktifkan(&t->base);
                if (t->base.fokus) pg_widget_blur(&t->base);
                t->menyeret = PG_SALAH;
                t->hover = PG_SALAH;
                csel(t);
                pg_widget_kotor(&t->base);
        }
}

pg_bool pg_multi_teks_aktif(pg_multi_teks_t *t)
{
        return t ? t->base.aktif : PG_SALAH;
}

void pg_multi_teks_setel_bungkus(pg_multi_teks_t *t, pg_bool bungkus)
{
        if (!t) return;
        t->bungkus = bungkus;
        pg_widget_kotor(&t->base);
}

pg_bool pg_multi_teks_bungkus(pg_multi_teks_t *t)
{
        return t ? t->bungkus : PG_SALAH;
}

pg_widget_t *pg_multi_teks_widget(pg_multi_teks_t *t)
{
        return t ? &t->base : NULL;
}
