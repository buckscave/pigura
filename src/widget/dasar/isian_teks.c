/* ----------------------------------------------------------------------------------------------
 * pigura widget: isian_teks.c - input teks satu baris
 * ----------------------------------------------------------------------------------------------
 * Rewrite penuh dari ubahiteks.c. Mendukung:
 *   - Visual style konsisten dengan tombol (latar panel, outline
 *     tipis, border fokus biru, sudut tumpul opsional via radius).
 *   - Placeholder abu-abu saat kosong dan tidak fokus.
 *   - Cursor blink 500 ms (on/off) saat fokus; reset saat user tekan
 *     key / klik.
 *   - Unicode UTF-8 penuh: input codepoint, cursor movement, dan
 *     selection bekerja pada level byte offset.
 *   - Clipboard asli via pg_papan_klip_t (Ctrl+C / V / X / A).
 *   - Undo/redo lokal per instance (Ctrl+Z / Y / Shift+Z).
 *   - Padding konfigurabel (default 6 px).
 *   - Auto-size berbasis tinggi font dan padding.
 *   - Gulir horizontal otomatis bila teks melebihi viewport.
 *
 * Catatan arsitektur:
 *   - byte offset dipakai untuk kursor + selection. Untuk gerakan
 *     kiri/kanan, kita advance per-codepoint (1-4 byte) dengan decode
 *     UTF-8 dari posisi saat ini.
 *   - Waktu blink diambil dari clock_gettime(CLOCK_MONOTONIC) supaya
 *     cursor bisa blink bahkan saat tidak ada event (render-only).
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/isian_teks.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/peristiwa.h"
#include "pigura/widget.h"
#include "pigura/utf8.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PG_ISIAN_TEKS_PAD_DEFAULT   6
#define PG_ISIAN_TEKS_UNDO_DEFAULT 50
#define PG_ISIAN_TEKS_BLINK_MS     500
#define PG_ISIAN_TEKS_CHAR_LEBAR   7   /* estimasi lebar char rata-rata untuk min_w */

/* ---- Snapshot undo/redo ---- */
typedef struct pg_isian_snapshot {
        char *buf;        /* null-terminated UTF-8 (malloc'd) */
        int   len;        /* bytes (not counting NUL) */
        int   kursor;     /* byte offset */
        int   sel_mulai;  /* -1 = no selection */
        int   sel_akhir;
} pg_isian_snapshot_t;

struct pg_isian_teks {
        pg_widget_t   base;
        char         *buf;
        int           cap;        /* capacity bytes including NUL */
        int           len;        /* bytes used (not counting NUL) */
        int           kursor;     /* byte offset 0..len */
        int           sel_mulai;  /* -1 = no selection */
        int           sel_akhir;
        pg_bool       menyeret;

        /* Placeholder (malloc'd) or NULL. */
        char         *placeholder;

        /* Clipboard (optional). */
        pg_papan_klip_t *klip;

        /* Undo/redo stacks (dynamic arrays of snapshots). */
        pg_isian_snapshot_t *undo;
        int                  n_undo;
        int                  cap_undo;
        int                  maks_undo;
        pg_isian_snapshot_t *redo;
        int                  n_redo;
        int                  cap_redo;
        pg_bool             beku_undo; /* suppress push during restore */

        /* Padding (px). */
        int           padding;

        /* Horizontal scroll offset (px) — posisi pixel pertama yang
         * terlihat relatif terhadap awal teks. */
        int           gulir_x;

        /* Blink state. */
        pg_u32        blink_acu_ms;  /* anchor time for blink cycle */
        pg_bool       blink_nyala;  /* cursor currently visible */

        /* Warna custom (PG_TRANSPARAN = pakai tema). */
        pg_warna_t    fg;
        pg_warna_t    latar;
        pg_warna_t    batas;

        pg_font_t    *font;
        pg_isian_teks_cb cb;
        void         *ctx;
};

/* ---- Helper waktu (clock_gettime monotonic) ---- */
static pg_u32 pg_isian_sekarang_ms(void)
{
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (pg_u32)((pg_u64)ts.tv_sec * 1000ULL +
                         (pg_u64)ts.tv_nsec / 1000000ULL);
}

/* ---- Helper warna tema ---- */
static pg_warna_t pg_isian_warna_fg(pg_isian_teks_t *it)
{
        if (it->fg != PG_TRANSPARAN) return it->fg;
        return PG_WARNA_TEKS_TOMBOL;
}
static pg_warna_t pg_isian_warna_latar(pg_isian_teks_t *it)
{
        if (it->latar != PG_TRANSPARAN) return it->latar;
        return PG_WARNA_PANEL;
}
static pg_warna_t pg_isian_warna_batas(pg_isian_teks_t *it)
{
        if (it->batas != PG_TRANSPARAN) return it->batas;
        return PG_WARNA_HOVER_OUTLINE;
}

/* ---- Casting ---- */
static pg_isian_teks_t *pg_isian_dari(pg_widget_t *w)
{
        return (pg_isian_teks_t *)w;
}

/* ---- Selection helpers ---- */
static int sel_min(pg_isian_teks_t *it)
{
        if (it->sel_mulai < 0) return it->kursor;
        return it->sel_mulai < it->sel_akhir ?
                it->sel_mulai : it->sel_akhir;
}
static int sel_max(pg_isian_teks_t *it)
{
        if (it->sel_mulai < 0) return it->kursor;
        return it->sel_mulai > it->sel_akhir ?
                it->sel_mulai : it->sel_akhir;
}
static int has_sel(pg_isian_teks_t *it)
{
        return it->sel_mulai >= 0 && it->sel_mulai != it->sel_akhir;
}
static void clear_sel(pg_isian_teks_t *it)
{
        it->sel_mulai = -1;
        it->sel_akhir = -1;
}

/* ---- UTF-8 helpers ----
 * Cursor movement bekerja pada byte offset. Untuk maju 1 codepoint,
 * kita decode sequence pada posisi saat ini dan advance.
 * Untuk mundur, kita scan backward sampai menemui byte bukan-
 * continuation (< 0x80 atau >= 0xC0). */

/* Hitung byte length sequence UTF-8 dimulai pada byte b. Return 1..4
 * atau 1 untuk byte invalid (continuation byte standalone). */
static int utf8_panjang_seq(pg_u8 b)
{
        if (b < 0x80)        return 1;
        if ((b & 0xE0) == 0xC0) return 2;
        if ((b & 0xF0) == 0xE0) return 3;
        if ((b & 0xF8) == 0xF0) return 4;
        return 1; /* invalid / continuation → treat as 1 */
}

/* Mundur 1 codepoint dari posisi kursor. Tidak melewati 0. */
static int utf8_mundur(const char *buf, int pos)
{
        if (pos <= 0) return 0;
        pos--;
        while (pos > 0 && ((pg_u8)buf[pos] & 0xC0) == 0x80) pos--;
        return pos;
}

/* Maju 1 codepoint dari posisi kursor. Tidak melewati len. */
static int utf8_maju(const char *buf, int pos, int len)
{
        int n;
        if (pos >= len) return len;
        n = utf8_panjang_seq((pg_u8)buf[pos]);
        if (pos + n > len) n = len - pos;
        return pos + n;
}

/* ---- Buffer edit primitives ---- */
static void delete_sel(pg_isian_teks_t *it)
{
        int lo, hi, i, span;
        if (!has_sel(it)) return;
        lo = sel_min(it);
        hi = sel_max(it);
        span = hi - lo;
        for (i = lo; i + span < it->len; i++)
                it->buf[i] = it->buf[i + span];
        it->len -= span;
        it->buf[it->len] = 0;
        it->kursor = lo;
        clear_sel(it);
}

/* Sisip string panjang n (byte) di posisi kursor. Asumsi: selection
 * sudah dibersihkan (caller delete_sel bila perlu). Tidak push undo
 * — pemanggil bertanggung jawab. */
static void insert_bytes(pg_isian_teks_t *it, const char *s, int n)
{
        int i, avail;
        if (!s || n <= 0) return;
        avail = it->cap - 1 - it->len;
        if (n > avail) n = avail;
        if (n <= 0) return;
        for (i = it->len; i >= it->kursor; i--)
                it->buf[i + n] = it->buf[i];
        memcpy(it->buf + it->kursor, s, n);
        it->len += n;
        it->kursor += n;
        it->buf[it->len] = 0;
}

static void fire_cb(pg_isian_teks_t *it)
{
        if (it->cb) it->cb(it, it->ctx);
}

/* ---- Undo/redo ---- */
static void snapshot_bebas(pg_isian_snapshot_t *s)
{
        if (s->buf) {
                free(s->buf);
                s->buf = NULL;
        }
}

/* Buat snapshot dari state saat ini. */
static int snapshot_buat(pg_isian_teks_t *it, pg_isian_snapshot_t *out)
{
        out->buf = (char *)malloc((size_t)it->len + 1);
        if (!out->buf) return 0;
        if (it->len > 0)
                memcpy(out->buf, it->buf, (size_t)it->len);
        out->buf[it->len] = 0;
        out->len = it->len;
        out->kursor = it->kursor;
        out->sel_mulai = it->sel_mulai;
        out->sel_akhir = it->sel_akhir;
        return 1;
}

/* Apply snapshot ke state saat ini. */
static void snapshot_pasang(pg_isian_teks_t *it,
                              const pg_isian_snapshot_t *s)
{
        int n;
        n = s->len;
        if (n > it->cap - 1) n = it->cap - 1;
        if (n < 0) n = 0;
        if (n > 0)
                memcpy(it->buf, s->buf, (size_t)n);
        it->buf[n] = 0;
        it->len = n;
        it->kursor = (s->kursor > n) ? n : s->kursor;
        it->sel_mulai = (s->sel_mulai > n) ? n : s->sel_mulai;
        it->sel_akhir = (s->sel_akhir > n) ? n : s->sel_akhir;
}

/* Push state saat ini ke undo stack. Trim stack bila melebihi maks. */
static void undo_push(pg_isian_teks_t *it)
{
        pg_isian_snapshot_t snap;
        int i;

        if (it->beku_undo) return;

        /* Snapshot tidak dibuat bila alokasi gagal. */
        if (!snapshot_buat(it, &snap)) return;

        /* Bila stack penuh, drop entry terlama. Karena ring buffer
         * sederhana, kita free entry [0] dan geser sisanya turun. */
        if (it->n_undo >= it->maks_undo) {
                int drop = it->n_undo - it->maks_undo + 1;
                if (drop < 1) drop = 1;
                for (i = 0; i < drop; i++)
                        snapshot_bebas(&it->undo[i]);
                for (i = 0; i < it->n_undo - drop; i++)
                        it->undo[i] = it->undo[i + drop];
                it->n_undo -= drop;
        }

        /* Grow buffer bila perlu. */
        if (it->n_undo >= it->cap_undo) {
                int baru_cap = it->cap_undo * 2;
                pg_isian_snapshot_t *baru;
                if (baru_cap < 8) baru_cap = 8;
                baru = (pg_isian_snapshot_t *)realloc(it->undo,
                        (size_t)baru_cap * sizeof(*baru));
                if (!baru) {
                        snapshot_bebas(&snap);
                        return;
                }
                it->undo = baru;
                it->cap_undo = baru_cap;
        }

        it->undo[it->n_undo++] = snap;

        /* Push ke undo stack mengosongkan redo stack (commit baru). */
        for (i = 0; i < it->n_redo; i++)
                snapshot_bebas(&it->redo[i]);
        it->n_redo = 0;
}

/* ---- Auto-size ---- */
static void pg_isian_update_min(pg_isian_teks_t *it)
{
        int th, mh, mw;
        if (it->font) {
                th = pg_font_tinggi(it->font);
                if (th <= 0) th = 8;
        } else {
                th = 8;
        }
        mh = th + 2 * it->padding;
        mw = 10 * PG_ISIAN_TEKS_CHAR_LEBAR + 2 * it->padding;
        pg_widget_setel_ukuran_min(&it->base, mw, mh);
}

/* ---- Blink ---- */
static void blink_reset(pg_isian_teks_t *it)
{
        it->blink_acu_ms = pg_isian_sekarang_ms();
        it->blink_nyala = PG_BENAR;
}

/* Return PG_BENAR bila cursor harus terlihat frame ini. */
static pg_bool blink_harus_terlihat(pg_isian_teks_t *it)
{
        pg_u32 now, elapsed, phase;
        now = pg_isian_sekarang_ms();
        elapsed = now - it->blink_acu_ms;
        phase = elapsed / (pg_u32)PG_ISIAN_TEKS_BLINK_MS;
        /* even phase = visible, odd phase = hidden. */
        return (phase & 1u) ? PG_SALAH : PG_BENAR;
}

/* ---- Layout/render helpers ---- */

/* Hitung pixel x dari byte offset di teks (relatif terhadap awal
 * teks, belum dikurangi gulir_x). */
static int teks_x_offset(pg_isian_teks_t *it, int byte_pos)
{
        char simpan;
        int x;
        if (!it->font || !it->buf || byte_pos <= 0) return 0;
        if (byte_pos > it->len) byte_pos = it->len;
        simpan = it->buf[byte_pos];
        it->buf[byte_pos] = 0;
        x = pg_font_lebar_teks_utf8(it->font, it->buf);
        it->buf[byte_pos] = simpan;
        return x;
}

/* Hitung byte offset terdekat dengan klik mouse (pixel x relatif ke
 * area teks, sudah dikurangi padding). */
static int kursor_dari_klik(pg_isian_teks_t *it, int x_dalam)
{
        int best = 0, best_dist = 0x7FFFFFFF;
        int i;
        if (!it->font || !it->buf || it->len <= 0) return 0;
        /* Tambahkan offset gulir untuk dapat koordinat teks absolut. */
        x_dalam += it->gulir_x;
        if (x_dalam <= 0) return 0;
        for (i = 0; i <= it->len; i = utf8_maju(it->buf, i, it->len)) {
                int w = teks_x_offset(it, i);
                int dist = x_dalam - w;
                if (dist < 0) dist = -dist;
                if (dist < best_dist) {
                        best_dist = dist;
                        best = i;
                }
                if (i == it->len) break;
        }
        return best;
}

/* Sesuaikan gulir_x supaya cursor terlihat di viewport. */
static void sesuaikan_gulir(pg_isian_teks_t *it, int sw)
{
        int cx_text, cx_view, pad, vp_lebar;
        int th;
        (void)th;
        if (!it->font) return;
        pad = it->padding;
        vp_lebar = sw - pad * 2;
        if (vp_lebar < 8) vp_lebar = 8;
        cx_text = teks_x_offset(it, it->kursor);
        cx_view = cx_text - it->gulir_x;
        /* Bila cursor di kiri viewport, scroll ke kiri. */
        if (cx_view < 0) {
                it->gulir_x = cx_text;
        } else if (cx_view > vp_lebar) {
                /* Bila cursor di kanan viewport, scroll ke kanan. */
                it->gulir_x = cx_text - vp_lebar;
                if (it->gulir_x < 0) it->gulir_x = 0;
        }
}

/* Gambar outline (border) kotak dengan radius opsional. */
static void gambar_outline(pg_permukaan_t *s, pg_kotak_t r,
                            int radius, pg_warna_t c)
{
        if (radius > 0)
                pg_gambar_kotak_tumpul_aa(s, r, radius, c);
        else
                pg_gambar_kotak_aa(s, r, c);
}

/* Gambar isi (fill) kotak dengan radius opsional. */
static void gambar_isi(pg_permukaan_t *s, pg_kotak_t r,
                        int radius, pg_warna_t c)
{
        if (radius > 0)
                pg_gambar_kotak_tumpul_isi_aa(s, r, radius, c);
        else
                pg_isi_permukaan(s, c);
}

/* ---- vtable: catat ---- */
static void pg_isian_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
        pg_isian_teks_t *it = pg_isian_dari(w);
        pg_kotak_t r;
        int sw, sh, radius, pad;
        int th, y, ascent, top_y;
        pg_warna_t warna_latar, warna_batas, warna_fg;

        sw = pg_permukaan_lebar(s);
        sh = pg_permukaan_tinggi(s);
        r = pg_buat_kotak(0, 0, sw, sh);
        radius = w->radius;
        pad = it->padding;

        warna_latar = pg_isian_warna_latar(it);
        warna_batas = pg_isian_warna_batas(it);
        warna_fg    = pg_isian_warna_fg(it);

        /* Smart latar: bila alpha < 255, isi transparan (lalu gambar
         * fill rounded); bila opaque, isi langsung dengan latar. */
        if (PG_A(w->latar) < 255) {
                pg_isi_permukaan(s, PG_TRANSPARAN);
                gambar_isi(s, r, radius, warna_latar);
        } else {
                /* widget.c sudah isi dengan w->latar. Kita gambar ulang
                 * supaya rounded corners konsisten saat radius > 0. */
                if (radius > 0)
                        gambar_isi(s, r, radius, warna_latar);
        }

        /* Outline border (tipis). Fokus override ke biru. */
        if (pg_widget_punya_fokus(w))
                gambar_outline(s, r, radius, PG_WARNA_FOKUS);
        else
                gambar_outline(s, r, radius, warna_batas);

        if (!it->font) return;

        /* Hitung layout teks. */
        th = pg_font_tinggi(it->font);
        if (th <= 0) th = 8;
        ascent = pg_font_ascent(it->font);
        y = pg_font_baseline_tengah(it->font, sh);
        if (y < 0) y = 0;
        top_y = y - ascent;
        if (top_y < 1) top_y = 1;

        /* Sesuaikan gulir supaya cursor terlihat. */
        sesuaikan_gulir(it, sw);

        /* Placeholder saat kosong + tidak fokus. */
        if (it->len == 0 && !pg_widget_punya_fokus(w) && it->placeholder) {
                pg_font_gambar_teks_utf8(it->font, it->placeholder,
                                           s, pad, y, PG_ABU);
                return;
        }

        /* Bila kosong + fokus: tidak ada teks, hanya cursor blink. */
        if (it->len == 0) {
                if (pg_widget_punya_fokus(w) && blink_harus_terlihat(it)) {
                        pg_garis_v_permukaan(s, pad, top_y, y + 2,
                                               warna_fg);
                }
                return;
        }

        /* Selection highlight: gambar latar fokus untuk teks terpilih. */
        if (has_sel(it)) {
                int lo = sel_min(it);
                int hi = sel_max(it);
                int sel_x0 = teks_x_offset(it, lo) - it->gulir_x + pad;
                int sel_x1 = teks_x_offset(it, hi) - it->gulir_x + pad;
                int sx0, sw_sel;
                /* Clip ke viewport. */
                if (sel_x0 < pad) sel_x0 = pad;
                if (sel_x1 > sw - pad) sel_x1 = sw - pad;
                sx0 = sel_x0;
                sw_sel = sel_x1 - sel_x0;
                if (sw_sel > 0)
                        pg_isi_permukaan_kotak(s,
                                pg_buat_kotak(sx0, top_y, sw_sel, th + 2),
                                PG_WARNA_FOKUS);
        }

        /* Render teks (UTF-8). Untuk selection, teks di dalam range
         * di-render dengan warna putih supaya kontras dengan latar
         * fokus biru. Kita render per-segmen: sebelum sel, di sel,
         * setelah sel. */
        {
                int tx = pad - it->gulir_x;
                if (has_sel(it)) {
                        int lo = sel_min(it);
                        int hi = sel_max(it);
                        int w_pre, w_sel;
                        char simpan;
                        /* Pre-selection. */
                        if (lo > 0) {
                                simpan = it->buf[lo];
                                it->buf[lo] = 0;
                                w_pre = pg_font_lebar_teks_utf8(it->font, it->buf);
                                it->buf[lo] = simpan;
                                pg_font_gambar_teks_utf8(it->font, it->buf,
                                                           s, tx, y, warna_fg);
                                tx += w_pre;
                        }
                        /* Selection. */
                        {
                                simpan = it->buf[hi];
                                it->buf[hi] = 0;
                                w_sel = pg_font_lebar_teks_utf8(it->font,
                                                                  it->buf + lo);
                                it->buf[hi] = simpan;
                                pg_font_gambar_teks_utf8(it->font, it->buf + lo,
                                                           s, tx, y, PG_PUTIH);
                                tx += w_sel;
                        }
                        /* Post-selection. */
                        if (hi < it->len) {
                                pg_font_gambar_teks_utf8(it->font,
                                                           it->buf + hi,
                                                           s, tx, y,
                                                           warna_fg);
                        }
                } else {
                        pg_font_gambar_teks_utf8(it->font, it->buf, s,
                                                   tx, y, warna_fg);
                }
        }

        /* Cursor vertikal blink bila fokus. */
        if (pg_widget_punya_fokus(w) && blink_harus_terlihat(it) &&
            it->kursor >= 0 && it->kursor <= it->len) {
                int cx = teks_x_offset(it, it->kursor) - it->gulir_x + pad;
                /* Clip cursor ke area teks. */
                if (cx < pad) cx = pad;
                if (cx > sw - pad) cx = sw - pad;
                pg_garis_v_permukaan(s, cx, top_y, y + 2, warna_fg);
        }
}

/* ---- vtable: event handler ---- */

static pg_bool pg_isian_peristiwa_v(pg_widget_t *w,
                                      const pg_peristiwa_t *e)
{
        pg_isian_teks_t *it = pg_isian_dari(w);

        /* TETIK_TURUN: fokus + klik untuk posisi kursor. */
        if (e->tipe == PG_PERISTIWA_TETIK_TURUN &&
            e->tetik_tombol == PG_TETIK_KIRI) {
                int pos;
                if (!pg_widget_punya_fokus(w))
                        pg_widget_fokus(w);
                blink_reset(it);
                pos = kursor_dari_klik(it, e->tetik_pos.x - it->padding);
                if (e->modifier & PG_MOD_SHIFT) {
                        if (it->sel_mulai < 0)
                                it->sel_mulai = it->kursor;
                        it->sel_akhir = pos;
                } else {
                        it->sel_mulai = pos;
                        it->sel_akhir = pos;
                        it->menyeret = PG_BENAR;
                }
                it->kursor = pos;
                pg_widget_kotor(w);
                return PG_BENAR;
        }

        /* TETIK_NAIK: end drag selection. */
        if (e->tipe == PG_PERISTIWA_TETIK_NAIK &&
            e->tetik_tombol == PG_TETIK_KIRI) {
                if (!pg_widget_punya_fokus(w) && !it->menyeret)
                        return PG_SALAH;
                it->menyeret = PG_SALAH;
                if (it->sel_mulai == it->sel_akhir)
                        clear_sel(it);
                pg_widget_kotor(w);
                return PG_BENAR;
        }

        /* GERAK: drag selection — extend selection saat mouse held. */
        if (e->tipe == PG_PERISTIWA_TETIK_GERAK &&
            it->menyeret && it->sel_mulai >= 0) {
                int pos = kursor_dari_klik(it, e->tetik_pos.x - it->padding);
                it->sel_akhir = pos;
                it->kursor = pos;
                pg_widget_kotor(w);
                return PG_BENAR;
        }

        if (e->tipe != PG_PERISTIWA_TOMBOL_TURUN) return PG_SALAH;
        if (!pg_widget_punya_fokus(w)) return PG_SALAH;

        /* Setiap keypress me-reset blink supaya cursor langsung
         * terlihat. */
        blink_reset(it);

        /* Ctrl+A: select all. */
        if ((e->tombol == 'a' || e->tombol == 'A') &&
            (e->modifier & PG_MOD_CTRL)) {
                it->sel_mulai = 0;
                it->sel_akhir = it->len;
                it->kursor = it->len;
                pg_widget_kotor(w);
                return PG_BENAR;
        }

        /* Ctrl+Z: undo. */
        if ((e->tombol == 'z' || e->tombol == 'Z') &&
            (e->modifier & PG_MOD_CTRL)) {
                if (e->modifier & PG_MOD_SHIFT)
                        pg_isian_teks_ulangi(it);
                else
                        pg_isian_teks_urungkan(it);
                pg_widget_kotor(w);
                return PG_BENAR;
        }

        /* Ctrl+Y: redo. */
        if ((e->tombol == 'y' || e->tombol == 'Y') &&
            (e->modifier & PG_MOD_CTRL)) {
                pg_isian_teks_ulangi(it);
                pg_widget_kotor(w);
                return PG_BENAR;
        }

        /* Ctrl+C: copy selection ke clipboard (asli). */
        if ((e->tombol == 'c' || e->tombol == 'C') &&
            (e->modifier & PG_MOD_CTRL)) {
                if (it->klip && has_sel(it)) {
                        int lo = sel_min(it);
                        int hi = sel_max(it);
                        int n = hi - lo;
                        char *tmp;
                        if (n > 0) {
                                tmp = (char *)malloc((size_t)n + 1);
                                if (tmp) {
                                        memcpy(tmp, it->buf + lo,
                                                (size_t)n);
                                        tmp[n] = 0;
                                        pg_papan_klip_tulis_teks(it->klip,
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
                if (has_sel(it)) {
                        int lo = sel_min(it);
                        int hi = sel_max(it);
                        int n = hi - lo;
                        if (it->klip && n > 0) {
                                char *tmp = (char *)malloc((size_t)n + 1);
                                if (tmp) {
                                        memcpy(tmp, it->buf + lo,
                                                (size_t)n);
                                        tmp[n] = 0;
                                        pg_papan_klip_tulis_teks(it->klip,
                                                                   tmp);
                                        free(tmp);
                                }
                        }
                        undo_push(it);
                        delete_sel(it);
                        pg_widget_kotor(w);
                        fire_cb(it);
                }
                return PG_BENAR;
        }

        /* Ctrl+V: paste dari clipboard. */
        if ((e->tombol == 'v' || e->tombol == 'V') &&
            (e->modifier & PG_MOD_CTRL)) {
                if (it->klip) {
                        char *clip = pg_papan_klip_baca_teks(it->klip);
                        if (clip) {
                                undo_push(it);
                                if (has_sel(it)) delete_sel(it);
                                insert_bytes(it, clip, (int)strlen(clip));
                                free(clip);
                                pg_widget_kotor(w);
                                fire_cb(it);
                        }
                }
                return PG_BENAR;
        }

        /* BACKSPACE: hapus selection atau codepoint sebelum kursor. */
        if (e->tombol == PG_TOMBOL_BACKSPACE) {
                if (it->len > 0) {
                        undo_push(it);
                        if (has_sel(it)) {
                                delete_sel(it);
                        } else if (it->kursor > 0) {
                                int baru = utf8_mundur(it->buf, it->kursor);
                                int span = it->kursor - baru;
                                int i;
                                for (i = baru; i + span < it->len; i++)
                                        it->buf[i] = it->buf[i + span];
                                it->len -= span;
                                it->kursor = baru;
                                it->buf[it->len] = 0;
                        }
                        pg_widget_kotor(w);
                        fire_cb(it);
                }
                return PG_BENAR;
        }

        /* DELETE: hapus codepoint setelah kursor. */
        if (e->tombol == PG_TOMBOL_DELETE) {
                if (it->len > 0) {
                        undo_push(it);
                        if (has_sel(it)) {
                                delete_sel(it);
                        } else if (it->kursor < it->len) {
                                int n = utf8_panjang_seq((pg_u8)it->buf[it->kursor]);
                                int i;
                                if (it->kursor + n > it->len)
                                        n = it->len - it->kursor;
                                for (i = it->kursor; i + n < it->len; i++)
                                        it->buf[i] = it->buf[i + n];
                                it->len -= n;
                                it->buf[it->len] = 0;
                        }
                        pg_widget_kotor(w);
                        fire_cb(it);
                }
                return PG_BENAR;
        }

        /* HOME: kursor ke awal. */
        if (e->tombol == PG_TOMBOL_HOME) {
                if (e->modifier & PG_MOD_SHIFT) {
                        if (it->sel_mulai < 0)
                                it->sel_mulai = it->kursor;
                        it->sel_akhir = 0;
                } else clear_sel(it);
                it->kursor = 0;
                pg_widget_kotor(w);
                return PG_BENAR;
        }

        /* END: kursor ke akhir. */
        if (e->tombol == PG_TOMBOL_END) {
                if (e->modifier & PG_MOD_SHIFT) {
                        if (it->sel_mulai < 0)
                                it->sel_mulai = it->kursor;
                        it->sel_akhir = it->len;
                } else clear_sel(it);
                it->kursor = it->len;
                pg_widget_kotor(w);
                return PG_BENAR;
        }

        /* Panah kiri. */
        if (e->tombol == PG_TOMBOL_KIRI) {
                if (e->modifier & PG_MOD_SHIFT) {
                        if (it->sel_mulai < 0)
                                it->sel_mulai = it->kursor;
                        it->kursor = utf8_mundur(it->buf, it->kursor);
                        it->sel_akhir = it->kursor;
                } else {
                        if (has_sel(it)) {
                                it->kursor = sel_min(it);
                                clear_sel(it);
                        } else {
                                it->kursor = utf8_mundur(it->buf, it->kursor);
                        }
                }
                pg_widget_kotor(w);
                return PG_BENAR;
        }

        /* Panah kanan. */
        if (e->tombol == PG_TOMBOL_KANAN) {
                if (e->modifier & PG_MOD_SHIFT) {
                        if (it->sel_mulai < 0)
                                it->sel_mulai = it->kursor;
                        it->kursor = utf8_maju(it->buf, it->kursor, it->len);
                        it->sel_akhir = it->kursor;
                } else {
                        if (has_sel(it)) {
                                it->kursor = sel_max(it);
                                clear_sel(it);
                        } else {
                                it->kursor = utf8_maju(it->buf, it->kursor,
                                                          it->len);
                        }
                }
                pg_widget_kotor(w);
                return PG_BENAR;
        }

        if (e->tombol == PG_TOMBOL_ENTER) {
                return PG_BENAR;
        }

        /* Printable Unicode codepoint: encode UTF-8 lalu insert. */
        if (e->unicode >= 32) {
                char tmp[4];
                int n;
                n = pg_utf8_encode(e->unicode, tmp);
                if (n > 0) {
                        undo_push(it);
                        if (has_sel(it)) delete_sel(it);
                        insert_bytes(it, tmp, n);
                        pg_widget_kotor(w);
                        fire_cb(it);
                }
                return PG_BENAR;
        }

        /* Fallback printable ASCII (bila unicode = 0). */
        if (e->tombol >= 32 && e->tombol <= 126) {
                char c = (char)e->tombol;
                undo_push(it);
                if (has_sel(it)) delete_sel(it);
                insert_bytes(it, &c, 1);
                pg_widget_kotor(w);
                fire_cb(it);
                return PG_BENAR;
        }

        return PG_SALAH;
}

/* ---- vtable: hancur + bebas ---- */
static void pg_isian_hancur_v(pg_widget_t *w)
{
        pg_isian_teks_t *it = pg_isian_dari(w);
        int i;
        if (it->buf) {
                free(it->buf);
                it->buf = NULL;
        }
        if (it->placeholder) {
                free(it->placeholder);
                it->placeholder = NULL;
        }
        for (i = 0; i < it->n_undo; i++)
                snapshot_bebas(&it->undo[i]);
        free(it->undo);
        it->undo = NULL;
        it->n_undo = 0;
        it->cap_undo = 0;
        for (i = 0; i < it->n_redo; i++)
                snapshot_bebas(&it->redo[i]);
        free(it->redo);
        it->redo = NULL;
        it->n_redo = 0;
        it->cap_redo = 0;
}

static void pg_isian_bebas_v(pg_widget_t *w)
{
        free(w);
}

static const pg_widget_vtable_t pg_isian_vtable = {
        pg_isian_catat_v,
        pg_isian_peristiwa_v,
        NULL,
        pg_isian_hancur_v,
        NULL,
        NULL,
        pg_isian_bebas_v
};

/* ---- API publik ---- */

pg_isian_teks_t *pg_buat_isian_teks(const char *awal,
                                       int panjang_maks,
                                       pg_font_t *font)
{
        pg_isian_teks_t *it;
        int cap;
        if (panjang_maks <= 0) panjang_maks = 64;
        cap = panjang_maks + 1;
        it = (pg_isian_teks_t *)calloc(1, sizeof(*it));
        if (!it) return NULL;
        pg_widget_init(&it->base, PG_WIDGET_ISIAN_TEK,
                       &pg_isian_vtable);
        pg_widget_milik(&it->base, PG_BENAR);
        it->font = font;
        it->fg = PG_TRANSPARAN;       /* pakai tema */
        it->latar = PG_TRANSPARAN;    /* pakai tema */
        it->batas = PG_TRANSPARAN;    /* pakai tema */
        it->cap = cap;
        it->sel_mulai = -1;
        it->sel_akhir = -1;
        it->padding = PG_ISIAN_TEKS_PAD_DEFAULT;
        it->maks_undo = PG_ISIAN_TEKS_UNDO_DEFAULT;
        it->buf = (char *)calloc((size_t)cap, 1);
        if (!it->buf) {
                free(it);
                return NULL;
        }
        if (awal) {
                size_t n = strlen(awal);
                if (n > (size_t)panjang_maks) n = (size_t)panjang_maks;
                memcpy(it->buf, awal, n);
                it->buf[n] = 0;
                it->len = (int)n;
                it->kursor = it->len;
        }
        blink_reset(it);
        pg_isian_update_min(it);
        return it;
}

void pg_isian_teks_hancur(pg_isian_teks_t *it)
{
        if (!it) return;
        pg_widget_hancur(&it->base);
        free(it);
}

void pg_isian_teks_setel_teks(pg_isian_teks_t *it, const char *teks)
{
        size_t n;
        if (!it) return;
        undo_push(it);
        if (!teks) {
                it->buf[0] = 0;
                it->len = 0;
                it->kursor = 0;
                clear_sel(it);
                pg_widget_kotor(&it->base);
                return;
        }
        n = strlen(teks);
        if (n >= (size_t)it->cap) n = (size_t)it->cap - 1;
        memcpy(it->buf, teks, n);
        it->buf[n] = 0;
        it->len = (int)n;
        it->kursor = it->len;
        clear_sel(it);
        pg_widget_kotor(&it->base);
        fire_cb(it);
}

const char *pg_isian_teks_ambil_teks(pg_isian_teks_t *it)
{
        return it ? (it->buf ? it->buf : "") : "";
}

void pg_isian_teks_saatberubah(pg_isian_teks_t *it,
                                 pg_isian_teks_cb cb, void *ctx)
{
        if (!it) return;
        it->cb = cb;
        it->ctx = ctx;
}

void pg_isian_teks_sisip(pg_isian_teks_t *it, const char *teks)
{
        if (!it || !teks) return;
        undo_push(it);
        if (has_sel(it)) delete_sel(it);
        insert_bytes(it, teks, (int)strlen(teks));
        pg_widget_kotor(&it->base);
        fire_cb(it);
}

pg_widget_t *pg_isian_teks_widget(pg_isian_teks_t *it)
{
        return it ? &it->base : NULL;
}

/* ---- Placeholder ---- */
void pg_isian_teks_setel_placeholder(pg_isian_teks_t *it,
                                        const char *teks)
{
        if (!it) return;
        if (it->placeholder) {
                free(it->placeholder);
                it->placeholder = NULL;
        }
        if (teks) {
                size_t n = strlen(teks) + 1;
                it->placeholder = (char *)malloc(n);
                if (it->placeholder)
                        memcpy(it->placeholder, teks, n);
        }
        pg_widget_kotor(&it->base);
}

/* ---- Selection ---- */
char *pg_isian_teks_ambil_pilihan(pg_isian_teks_t *it)
{
        int lo, hi, n;
        char *out;
        if (!it || !has_sel(it)) return NULL;
        lo = sel_min(it);
        hi = sel_max(it);
        n = hi - lo;
        if (n <= 0) return NULL;
        out = (char *)malloc((size_t)n + 1);
        if (!out) return NULL;
        memcpy(out, it->buf + lo, (size_t)n);
        out[n] = 0;
        return out;
}

void pg_isian_teks_setel_pilihan(pg_isian_teks_t *it,
                                    int mulai, int akhir)
{
        if (!it) return;
        if (mulai < 0 || akhir < 0 || mulai == akhir) {
                clear_sel(it);
        } else {
                if (mulai > it->len) mulai = it->len;
                if (akhir > it->len) akhir = it->len;
                it->sel_mulai = mulai;
                it->sel_akhir = akhir;
        }
        pg_widget_kotor(&it->base);
}

void pg_isian_teks_pilih_semua(pg_isian_teks_t *it)
{
        if (!it) return;
        it->sel_mulai = 0;
        it->sel_akhir = it->len;
        it->kursor = it->len;
        pg_widget_kotor(&it->base);
}

void pg_isian_teks_bersih_pilihan(pg_isian_teks_t *it)
{
        if (!it) return;
        clear_sel(it);
        pg_widget_kotor(&it->base);
}

/* ---- Clipboard ---- */
void pg_isian_teks_setel_papan_klip(pg_isian_teks_t *it,
                                      pg_papan_klip_t *klip)
{
        if (!it) return;
        it->klip = klip;
}

/* ---- Undo/redo ---- */
void pg_isian_teks_setel_batas_undo(pg_isian_teks_t *it, int maks)
{
        int i;
        if (!it) return;
        if (maks < 1) maks = 1;
        it->maks_undo = maks;
        /* Trim undo stack bila perlu. */
        while (it->n_undo > maks) {
                snapshot_bebas(&it->undo[0]);
                for (i = 0; i < it->n_undo - 1; i++)
                        it->undo[i] = it->undo[i + 1];
                it->n_undo--;
        }
        /* Trim redo stack bila perlu (redo tidak di-restrict sama
         * ketat — tapi tetap dibatasi maks_undo agar konsisten). */
        while (it->n_redo > maks) {
                snapshot_bebas(&it->redo[0]);
                for (i = 0; i < it->n_redo - 1; i++)
                        it->redo[i] = it->redo[i + 1];
                it->n_redo--;
        }
}

void pg_isian_teks_urungkan(pg_isian_teks_t *it)
{
        pg_isian_snapshot_t snap;
        if (!it || it->n_undo <= 0) return;
        /* Pop dari undo, push current ke redo. */
        snap = it->undo[--it->n_undo];
        /* Push current state ke redo stack. */
        if (it->n_redo >= it->cap_redo) {
                int baru_cap = it->cap_redo * 2;
                pg_isian_snapshot_t *baru;
                if (baru_cap < 8) baru_cap = 8;
                baru = (pg_isian_snapshot_t *)realloc(it->redo,
                        (size_t)baru_cap * sizeof(*baru));
                if (!baru) {
                        /* Bila realloc gagal, kita lepas snapshot
                         * supaya tidak leak. */
                        snapshot_bebas(&snap);
                        return;
                }
                it->redo = baru;
                it->cap_redo = baru_cap;
        }
        {
                pg_isian_snapshot_t cur;
                if (snapshot_buat(it, &cur)) {
                        it->redo[it->n_redo++] = cur;
                }
        }
        /* Apply snapshot. */
        it->beku_undo = PG_BENAR;
        snapshot_pasang(it, &snap);
        it->beku_undo = PG_SALAH;
        snapshot_bebas(&snap);
        pg_widget_kotor(&it->base);
        fire_cb(it);
}

void pg_isian_teks_ulangi(pg_isian_teks_t *it)
{
        pg_isian_snapshot_t snap;
        if (!it || it->n_redo <= 0) return;
        /* Pop dari redo, push current ke undo. */
        snap = it->redo[--it->n_redo];
        /* Push current state ke undo stack. */
        if (it->n_undo >= it->cap_undo) {
                int baru_cap = it->cap_undo * 2;
                pg_isian_snapshot_t *baru;
                if (baru_cap < 8) baru_cap = 8;
                baru = (pg_isian_snapshot_t *)realloc(it->undo,
                        (size_t)baru_cap * sizeof(*baru));
                if (!baru) {
                        snapshot_bebas(&snap);
                        return;
                }
                it->undo = baru;
                it->cap_undo = baru_cap;
        }
        {
                pg_isian_snapshot_t cur;
                if (snapshot_buat(it, &cur)) {
                        it->undo[it->n_undo++] = cur;
                }
        }
        it->beku_undo = PG_BENAR;
        snapshot_pasang(it, &snap);
        it->beku_undo = PG_SALAH;
        snapshot_bebas(&snap);
        pg_widget_kotor(&it->base);
        fire_cb(it);
}

/* ---- Padding ---- */
void pg_isian_teks_setel_padding(pg_isian_teks_t *it, int padding)
{
        if (!it) return;
        if (padding < 0) padding = 0;
        it->padding = padding;
        pg_isian_update_min(it);
        pg_widget_kotor(&it->base);
}

/* ---- Warna custom ---- */
void pg_isian_teks_setel_warna(pg_isian_teks_t *it,
                                 pg_warna_t fg,
                                 pg_warna_t latar,
                                 pg_warna_t batas)
{
        if (!it) return;
        it->fg = fg;
        it->latar = latar;
        it->batas = batas;
        /* Sinkronkan base.latar supaya widget.c smart-fill konsisten. */
        if (latar != PG_TRANSPARAN)
                pg_widget_setel_latar(&it->base, latar);
        else
                pg_widget_setel_latar(&it->base, PG_WARNA_PANEL);
        pg_widget_kotor(&it->base);
}
