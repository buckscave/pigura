/* ----------------------------------------------------------------------------------------------
 * pigura widget: kotak_pencarian.c - widget searchbox (kotak pencarian)
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/kotak_pencarian.h"
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

#define PG_KP_BTN_W 22
#define PG_KP_BLINK_MS 500

struct pg_kotak_pencarian {
        pg_widget_t base;
        char *buf; int cap, len, kursor;
        char *placeholder;
        pg_font_t *font;
        pg_kotak_pencarian_cb cb;
        void *ctx;
        pg_papan_klip_t *klip;
        pg_bool clear_hover, clear_tekan;
        pg_bool hover;
        pg_u32 blink_acu_ms;
        pg_bool blink_nyala;
};

static pg_u32 pg_kp_sekarang_ms(void)
{
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (pg_u32)((pg_u64)ts.tv_sec * 1000ULL +
                         (pg_u64)ts.tv_nsec / 1000000ULL);
}

static void pg_kp_blink_reset(pg_kotak_pencarian_t *sb)
{
        sb->blink_acu_ms = pg_kp_sekarang_ms();
        sb->blink_nyala = PG_BENAR;
}

static pg_bool pg_kp_blink_aktif(pg_kotak_pencarian_t *sb)
{
        pg_u32 now = pg_kp_sekarang_ms();
        pg_u32 elapsed = now - sb->blink_acu_ms;
        if (elapsed >= PG_KP_BLINK_MS * 2) {
                sb->blink_acu_ms = now;
                sb->blink_nyala = PG_BENAR;
        } else if (elapsed >= PG_KP_BLINK_MS) {
                sb->blink_nyala = PG_SALAH;
        }
        return sb->blink_nyala;
}

static pg_kotak_pencarian_t *pg_kp_dari(pg_widget_t *w)
{
        return (pg_kotak_pencarian_t *)w;
}

/* Hitung posisi kursor (byte offset) dari klik di posisi x.
 * Bandingkan x dengan lebar teks per-karakter untuk cari posisi
 * terdekat. Offset +20 karena search icon di kiri. */
static int pg_kp_kursor_dari_x(pg_kotak_pencarian_t *sb, int x)
{
        int best = 0;
        int best_dist = 999999;
        int i;
        int teks_x = 20;  /* offset karena search icon */
        if (!sb->font || sb->len == 0) return 0;
        for (i = 0; i <= sb->len; i++) {
                char sv = sb->buf[i];
                int w_i;
                sb->buf[i] = 0;
                w_i = teks_x + pg_font_lebar_teks(sb->font, sb->buf);
                sb->buf[i] = sv;
                {
                        int dist = x - w_i;
                        if (dist < 0) dist = -dist;
                        if (dist < best_dist) {
                                best_dist = dist;
                                best = i;
                        }
                }
        }
        return best;
}

static void pg_kp_fire(pg_kotak_pencarian_t *sb)
{
        if (sb->cb) sb->cb(sb, sb->buf ? sb->buf : "", sb->ctx);
}

static void pg_kp_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
        pg_kotak_pencarian_t *sb = pg_kp_dari(w);
        int sw, sh, bw, bl, th, y, radius;
        pg_kotak_t r;
        pg_warna_t latar_warna, batas_warna, fg_warna;
        pg_warna_t ph_warna, btn_warna;

        sw = pg_permukaan_lebar(s);
        sh = pg_permukaan_tinggi(s);
        bw = PG_KP_BTN_W;
        bl = sw - bw;
        radius = w->radius;

        if (!w->aktif) {
                latar_warna = PG_WARNA_NONAKTIF_ISI;
                batas_warna = PG_ABU_TERANG;
                fg_warna    = PG_WARNA_NONAKTIF_TEKS;
                ph_warna    = PG_ABU_TERANG;
                btn_warna   = PG_ABU_TERANG;
        } else {
                latar_warna = PG_WARNA_PANEL;
                fg_warna    = PG_WARNA_TEKS_TOMBOL;
                ph_warna    = PG_ABU;
                if (w->fokus) batas_warna = PG_WARNA_FOKUS;
                else if (sb->hover) batas_warna = PG_RGB(0x6E, 0x6E, 0x6E);
                else batas_warna = PG_WARNA_HOVER_OUTLINE;
                btn_warna = sb->clear_tekan ? PG_WARNA_TEKAN_ISI :
                            (sb->clear_hover ? PG_WARNA_HOVER_ISI :
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

        /* Search icon (kaca pembesar) di kiri.
         * Circle radius 3 di (10, sh/2 - 1) + handle diagonal.
         * Pakai ellipse AA + Wu line untuk konsistensi visual. */
        if (w->aktif) {
                int ic_cx = 10;
                int ic_cy = sh / 2 - 1;
                int ic_r = 3;
                pg_gambar_lingkaran_aa(s, ic_cx, ic_cy, ic_r,
                                        ph_warna);
                /* Handle: garis dari (ic_cx+2, ic_cy+2) ke (ic_cx+5, ic_cy+5). */
                pg_gambar_garis_aa(s,
                        PG_KE_FIXED(ic_cx + 2),
                        PG_KE_FIXED(ic_cy + 2),
                        PG_KE_FIXED(ic_cx + 5),
                        PG_KE_FIXED(ic_cy + 5),
                        ph_warna);
        }

        /* Tombol clear (✕) di kanan bila ada teks. */
        if (sb->len > 0 && w->aktif) {
                pg_kotak_t rb = pg_buat_kotak(bl, 0, bw, sh);
                pg_isi_permukaan_kotak(s, rb, btn_warna);
                pg_garis_v_permukaan(s, bl, 0, sh, batas_warna);
                /* X mark with AA lines. */
                {
                        int cx = bl + bw / 2, cy = sh / 2;
                        pg_gambar_garis_aa(s,
                                PG_KE_FIXED(cx - 4),
                                PG_KE_FIXED(cy - 4),
                                PG_KE_FIXED(cx + 4),
                                PG_KE_FIXED(cy + 4),
                                fg_warna);
                        pg_gambar_garis_aa(s,
                                PG_KE_FIXED(cx + 4),
                                PG_KE_FIXED(cy - 4),
                                PG_KE_FIXED(cx - 4),
                                PG_KE_FIXED(cy + 4),
                                fg_warna);
                }
        }

        /* Teks atau placeholder. Offset +20px ke kanan supaya tidak
         * overlap dengan search icon di kiri. */
        if (sb->font) {
                th = pg_font_tinggi(sb->font);
                if (th <= 0) th = 8;
                y = pg_font_baseline_tengah(sb->font, sh);
                if (sb->len > 0) {
                        pg_font_gambar_teks(sb->font, sb->buf, s, 20,
                                             y, fg_warna);
                } else if (sb->placeholder) {
                        pg_font_gambar_teks(sb->font, sb->placeholder,
                                             s, 20, y, ph_warna);
                }
                /* Cursor blink bila fokus dan ada teks. */
                if (w->fokus && sb->kursor >= 0 &&
                    sb->kursor <= sb->len && pg_kp_blink_aktif(sb)) {
                        char sv;
                        int cx;
                        sv = sb->buf[sb->kursor];
                        sb->buf[sb->kursor] = 0;
                        cx = 20 + pg_font_lebar_teks(sb->font,
                                                      sb->buf);
                        sb->buf[sb->kursor] = sv;
                        pg_garis_v_permukaan(s, cx,
                                y - pg_font_ascent(sb->font),
                                y + 2, fg_warna);
                }
        }
}

static pg_bool pg_kp_peristiwa_v(pg_widget_t *w, const pg_aksi_t *e)
{
        pg_kotak_pencarian_t *sb = pg_kp_dari(w);
        int sw = w->kotak.w, sh = w->kotak.h;
        int bw = PG_KP_BTN_W, bl = sw - bw;

        if (!w->aktif) return PG_SALAH;

        if (e->tipe == PG_AKSI_TETIKUS_TEKAN &&
            e->tetik_tombol == PG_TETIKUS_KIRI) {
                int x = e->tetik_pos.x;
                pg_widget_fokus(w);
                pg_kp_blink_reset(sb);
                if (x >= bl && sb->len > 0) {
                        /* Klik di tombol clear. */
                        sb->clear_tekan = PG_BENAR;
                        pg_widget_kotor(w);
                } else if (x < bl) {
                        /* Klik di field teks: set kursor ke posisi
                         * klik (click-to-position). Skip icon area
                         * (x < 20) — hanya fokus, tidak pindah kursor. */
                        if (x >= 20) {
                                int new_pos = pg_kp_kursor_dari_x(sb, x);
                                if (new_pos != sb->kursor) {
                                        sb->kursor = new_pos;
                                        pg_widget_kotor(w);
                                }
                        }
                }
                return PG_BENAR;
        }

        if (e->tipe == PG_AKSI_TETIKUS_LEPAS &&
            e->tetik_tombol == PG_TETIKUS_KIRI) {
                if (!pg_widget_punya_fokus(w) && !sb->clear_tekan)
                        return PG_SALAH;
                if (sb->clear_tekan) {
                        sb->clear_tekan = PG_SALAH;
                        if (e->tetik_pos.x >= bl && sb->len > 0) {
                                sb->len = 0;
                                sb->buf[0] = 0;
                                sb->kursor = 0;
                                pg_kp_fire(sb);
                        }
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }

        if (e->tipe == PG_AKSI_TETIKUS_GERAK) {
                int mx = e->tetik_pos.x;
                int my = e->tetik_pos.y;
                pg_bool ch = (mx >= bl && mx < sw && sb->len > 0) ?
                        PG_BENAR : PG_SALAH;
                /* Boundary check: hover hanya bila mouse di widget. */
                pg_bool new_hover = (mx >= 0 && mx < sw &&
                                      my >= 0 && my < sh) ?
                        PG_BENAR : PG_SALAH;
                if (ch != sb->clear_hover || new_hover != sb->hover) {
                        sb->clear_hover = ch;
                        sb->hover = new_hover;
                        pg_widget_kotor(w);
                }
                return PG_SALAH;
        }

        if (e->tipe != PG_AKSI_TOMBOL_TURUN) return PG_SALAH;
        if (!pg_widget_punya_fokus(w)) return PG_SALAH;

        /* Setiap keypress reset blink. */
        pg_kp_blink_reset(sb);

        /* Ctrl+A: kursor ke akhir (select all). */
        if ((e->tombol == 'a' || e->tombol == 'A') &&
            (e->modifier & PG_MOD_CTRL)) {
                sb->kursor = sb->len;
                pg_widget_kotor(w);
                return PG_BENAR;
        }

        /* Ctrl+C: copy teks ke clipboard. */
        if ((e->tombol == 'c' || e->tombol == 'C') &&
            (e->modifier & PG_MOD_CTRL)) {
                if (sb->klip && sb->buf) {
                        pg_papan_klip_tulis_teks(sb->klip, sb->buf);
                }
                return PG_BENAR;
        }

        /* Ctrl+V: paste dari clipboard. */
        if ((e->tombol == 'v' || e->tombol == 'V') &&
            (e->modifier & PG_MOD_CTRL)) {
                if (sb->klip) {
                        char *clip = pg_papan_klip_baca_teks(sb->klip);
                        if (clip) {
                                int n = (int)strlen(clip);
                                int i, j;
                                if (sb->len + n < sb->cap) {
                                        for (i = sb->len;
                                             i >= sb->kursor; i--)
                                                sb->buf[i + n] =
                                                        sb->buf[i];
                                        for (j = 0; j < n; j++)
                                                sb->buf[sb->kursor + j] =
                                                        clip[j];
                                        sb->len += n;
                                        sb->kursor += n;
                                        sb->buf[sb->len] = 0;
                                        pg_widget_kotor(w);
                                        pg_kp_fire(sb);
                                }
                                free(clip);
                        }
                }
                return PG_BENAR;
        }

        /* Ctrl+X: cut teks ke clipboard. */
        if ((e->tombol == 'x' || e->tombol == 'X') &&
            (e->modifier & PG_MOD_CTRL)) {
                if (sb->klip && sb->buf) {
                        pg_papan_klip_tulis_teks(sb->klip, sb->buf);
                }
                sb->buf[0] = 0;
                sb->len = 0;
                sb->kursor = 0;
                pg_widget_kotor(w);
                pg_kp_fire(sb);
                return PG_BENAR;
        }

        if (e->tombol == PG_TOMBOL_BACKSPACE) {
                if (sb->kursor > 0 && sb->len > 0) {
                        int i;
                        for (i = sb->kursor - 1;
                             i < sb->len - 1; i++)
                                sb->buf[i] = sb->buf[i + 1];
                        sb->len--;
                        sb->kursor--;
                        sb->buf[sb->len] = 0;
                        pg_widget_kotor(w);
                        pg_kp_fire(sb);
                }
                return PG_BENAR;
        }

        /* Insert char Unicode (UTF-8) atau ASCII fallback. */
        {
                pg_u32 cp = e->unicode ? e->unicode : (pg_u32)e->tombol;
                if (cp >= 32) {
                        char tmp[4];
                        int n;
                        n = pg_utf8_encode(cp, tmp);
                        if (n > 0 && sb->len + n < sb->cap) {
                                int i, j;
                                for (i = sb->len;
                                     i >= sb->kursor; i--)
                                        sb->buf[i + n] = sb->buf[i];
                                for (j = 0; j < n; j++)
                                        sb->buf[sb->kursor + j] = tmp[j];
                                sb->len += n;
                                sb->kursor += n;
                                sb->buf[sb->len] = 0;
                                pg_widget_kotor(w);
                                pg_kp_fire(sb);
                        }
                        return PG_BENAR;
                }
        }
        if (e->tombol == PG_TOMBOL_KIRI) {
                if (sb->kursor > 0) {
                        sb->kursor--;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }
        if (e->tombol == PG_TOMBOL_KANAN) {
                if (sb->kursor < sb->len) {
                        sb->kursor++;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }
        if (e->tombol == PG_TOMBOL_HOME) {
                if (sb->kursor != 0) {
                        sb->kursor = 0;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }
        if (e->tombol == PG_TOMBOL_END) {
                if (sb->kursor != sb->len) {
                        sb->kursor = sb->len;
                        pg_widget_kotor(w);
                }
                return PG_BENAR;
        }
        return PG_SALAH;
}

static void pg_kp_hancur_v(pg_widget_t *w)
{
        pg_kotak_pencarian_t *sb = pg_kp_dari(w);
        if (sb->buf) free(sb->buf);
        if (sb->placeholder) free(sb->placeholder);
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

pg_kotak_pencarian_t *pg_buat_kotak_pencarian(const char *placeholder,
                                                 int pm,
                                                 pg_font_t *font)
{
        pg_kotak_pencarian_t *sb;
        int cap;
        if (pm <= 0) pm = 64;
        cap = pm + 1;
        sb = (pg_kotak_pencarian_t *)calloc(1, sizeof(*sb));
        if (!sb) return NULL;
        pg_widget_init(&sb->base, PG_WIDGET_DASAR, &pg_kp_vtable);
        pg_widget_milik(&sb->base, PG_BENAR);
        sb->font = font;
        sb->cap = cap;
        sb->buf = (char *)calloc(cap, 1);
        if (!sb->buf) {
                free(sb);
                return NULL;
        }
        if (placeholder) {
                size_t n = strlen(placeholder) + 1;
                sb->placeholder = (char *)malloc(n);
                if (sb->placeholder) {
                        memcpy(sb->placeholder, placeholder, n);
                }
        }
        sb->base.latar = PG_TRANSPARAN;
        sb->base.radius = 0;  /* sudut tajam */
        pg_widget_setel_ukuran_min(&sb->base, 80, 24);
        return sb;
}

void pg_kotak_pencarian_hancur(pg_kotak_pencarian_t *sb)
{
        if (!sb) return;
        pg_widget_hancur(&sb->base);
        free(sb);
}

const char *pg_kotak_pencarian_ambil_teks(pg_kotak_pencarian_t *sb)
{
        return sb ? (sb->buf ? sb->buf : "") : "";
}

void pg_kotak_pencarian_setel_teks(pg_kotak_pencarian_t *sb,
                                     const char *t)
{
        if (!sb) return;
        if (!t) {
                sb->buf[0] = 0;
                sb->len = 0;
                sb->kursor = 0;
                pg_widget_kotor(&sb->base);
                return;
        }
        {
                size_t n = strlen(t);
                if (n >= (size_t)sb->cap) n = sb->cap - 1;
                memcpy(sb->buf, t, n);
                sb->buf[n] = 0;
                sb->len = (int)n;
                sb->kursor = (int)n;
                pg_widget_kotor(&sb->base);
        }
}

void pg_kotak_pencarian_saatberubah(pg_kotak_pencarian_t *sb,
                                       pg_kotak_pencarian_cb cb,
                                       void *ctx)
{
        if (!sb) return;
        sb->cb = cb;
        sb->ctx = ctx;
}

void pg_kotak_pencarian_setel_papan_klip(pg_kotak_pencarian_t *sb,
                                            pg_papan_klip_t *klip)
{
        if (!sb) return;
        sb->klip = klip;
}

void pg_kotak_pencarian_setel_aktif(pg_kotak_pencarian_t *sb,
                                       pg_bool aktif)
{
        if (!sb) return;
        if (aktif) {
                pg_widget_aktifkan(&sb->base);
        } else {
                pg_widget_nonaktifkan(&sb->base);
                if (sb->base.fokus) pg_widget_blur(&sb->base);
                sb->clear_hover = PG_SALAH;
                sb->clear_tekan = PG_SALAH;
                sb->hover = PG_SALAH;
                pg_widget_kotor(&sb->base);
        }
}

pg_bool pg_kotak_pencarian_aktif(pg_kotak_pencarian_t *sb)
{
        return sb ? sb->base.aktif : PG_SALAH;
}

pg_widget_t *pg_kotak_pencarian_widget(pg_kotak_pencarian_t *sb)
{
        return sb ? &sb->base : NULL;
}
