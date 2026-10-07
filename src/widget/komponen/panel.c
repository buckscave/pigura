/* -------------------------------------------------------------------------- *
 * pigura/widget/komponen/panel.c - Panel (entitas UI mandiri)
 * -------------------------------------------------------------------------- *
 * Implementasi pg_panel_t. Header 3 mode:
 *   - FULL  (floating): biru + judul + 3 tombol kanan
 *   - THIN  (docked properties): body color + judul + 3 tombol
 *   - GRIP  (docked toolbar/ribbon): box gelap, no judul, no tombol
 *
 * Grip orientasi:
 *   - vertikal (dock kiri/kanan): grip kotak di atas panel
 *   - horizontal (dock atas/bawah): grip kotak di kiri panel
 * -------------------------------------------------------------------------- */
#include "pigura/panel.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/aksi.h"
#include "pigura/widget.h"
#include "pigura/galat.h"

#include <stdlib.h>
#include <string.h>

/* ===== Konstanta ===== */

#define PG_PANEL_HEADER_H   20  /* tinggi header FULL/THIN */
#define PG_PANEL_GRIP_SIZE  12  /* ukuran grip box */
#define PG_PANEL_BTN_W      14  /* lebar tombol header */
#define PG_PANEL_BTN_H      14
#define PG_PANEL_BTN_GAP    2   /* jarak antar tombol */
#define PG_PANEL_BORDER     1
#define PG_PANEL_MIN_W      80  /* lebar minimum panel */
#define PG_PANEL_MIN_H      60  /* tinggi minimum panel */
#define PG_PANEL_RESIZE_GRIP 12 /* ukuran grip resize corner */

/* Tombol header (untuk mode FULL/THIN). */
typedef enum {
        PG_PANEL_BTN_NONE = -1,
        PG_PANEL_BTN_LIPAT = 0,
        PG_PANEL_BTN_SEMULA = 1,
        PG_PANEL_BTN_TUTUP = 2
} pg_panel_btn_t;

/* ===== Struct internal ===== */

struct pg_panel {
        pg_widget_t          base;
        char                *judul;
        pg_widget_t         *anak;
        pg_font_t           *font;
        pg_panel_flag_t      flag;
        pg_panel_mode_t      mode;       /* floating | docked */
        pg_panel_header_mode_t header_mode;
        pg_bool              collapsed;
        pg_bool              grip_vertikal; /* BENAR = grip di atas */

        /* Header button hit-test state. */
        pg_panel_btn_t       hover_btn;
        pg_panel_btn_t       ditekan_btn;

        /* Drag state. */
        pg_bool              menyeret;
        int                  seret_ofs_x;
        int                  seret_ofs_y;

        /* Resize state (floating only). */
        pg_bool              resize_menyeret;
        int                  resize_ofs_w;
        int                  resize_ofs_h;

        /* Warna. */
        pg_warna_t           header_bg_full;   /* floating */
        pg_warna_t           header_bg_thin;   /* docked properties */
        pg_warna_t           header_bg_grip;   /* docked toolbar */
        pg_warna_t           header_fg;
        pg_warna_t           body_bg;
        pg_warna_t           batas;

        /* Callbacks. */
        pg_panel_cb          cb_tutup;
        pg_panel_cb          cb_semula;
        pg_panel_lipat_cb    cb_lipat;
        pg_panel_drag_cb     cb_drag_selesai;
        pg_panel_drag_gerak_cb cb_drag_gerak;
        void                *cb_ctx;
};

/* ===== Helper ===== */

static pg_panel_t *pg_p_dari(pg_widget_t *w)
{
        return (pg_panel_t *)w;
}

static char *pg_p_dup(const char *s)
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

/* Tinggi header saat ini (tergantung mode). */
static int pg_p_header_h(const pg_panel_t *p)
{
        if (!p) return 0;
        if (p->collapsed) return PG_PANEL_HEADER_H;
        switch (p->header_mode) {
        case PG_PANEL_HEADER_FULL:
        case PG_PANEL_HEADER_THIN:
                return PG_PANEL_HEADER_H;
        case PG_PANEL_HEADER_GRIP:
                return p->grip_vertikal ? PG_PANEL_GRIP_SIZE : 0;
        default:
                return 0;
        }
}

/* Hitung rect tombol ke-i (kanan ke kiri). */
static pg_kotak_t pg_p_btn_kotak(const pg_panel_t *p, int btn_idx)
{
        int w = p->base.kotak.w;
        int x = w - 4 - PG_PANEL_BTN_W;
        int y = (PG_PANEL_HEADER_H - PG_PANEL_BTN_H) / 2;
        int i;
        for (i = 0; i < btn_idx; i++) {
                x -= (PG_PANEL_BTN_W + PG_PANEL_BTN_GAP);
        }
        return pg_buat_kotak(x, y, PG_PANEL_BTN_W, PG_PANEL_BTN_H);
}

/* Hit-test tombol mana yang di klik (-1 bila tidak). */
static pg_panel_btn_t pg_p_btn_hit(const pg_panel_t *p, int mx, int my)
{
        int i;
        if (!p || p->header_mode == PG_PANEL_HEADER_GRIP)
                return PG_PANEL_BTN_NONE;
        if (my < 0 || my >= PG_PANEL_HEADER_H) return PG_PANEL_BTN_NONE;
        for (i = 0; i < 3; i++) {
                pg_kotak_t r = pg_p_btn_kotak(p, i);
                if (mx >= r.x && mx < r.x + r.w &&
                    my >= r.y && my < r.y + r.h)
                        return (pg_panel_btn_t)i;
        }
        return PG_PANEL_BTN_NONE;
}

/* ===== Layout ===== */

static void pg_p_layout_anak(pg_panel_t *p)
{
        int hh, body_x, body_y, body_w, body_h;
        if (!p || !p->anak) return;
        hh = pg_p_header_h(p);
        if (p->header_mode == PG_PANEL_HEADER_GRIP && !p->grip_vertikal) {
                /* Grip di kiri, body di kanan. */
                body_x = PG_PANEL_GRIP_SIZE;
                body_y = 0;
                body_w = p->base.kotak.w - PG_PANEL_GRIP_SIZE;
                body_h = p->base.kotak.h;
        } else {
                /* Header di atas, body di bawah. */
                body_x = 0;
                body_y = hh;
                body_w = p->base.kotak.w;
                body_h = p->base.kotak.h - hh;
        }
        if (body_w < 0) body_w = 0;
        if (body_h < 0) body_h = 0;
        pg_widget_setel_kotak(p->anak,
                pg_buat_kotak(body_x, body_y, body_w, body_h));
}

/* ===== Vtable ===== */

static void pg_p_catat_v(pg_widget_t *w, pg_permukaan_t *s);
static pg_bool pg_p_peristiwa_v(pg_widget_t *w, const pg_aksi_t *e);
static void pg_p_ubah_ukuran_v(pg_widget_t *w, int w_, int h);
static void pg_p_hancur_v(pg_widget_t *w);
static void pg_p_bebas_v(pg_widget_t *w);

static const pg_widget_vtable_t pg_p_vtable = {
        pg_p_catat_v,
        pg_p_peristiwa_v,
        pg_p_ubah_ukuran_v,
        pg_p_hancur_v,
        NULL,
        NULL,
        pg_p_bebas_v
};

/* ===== Render ===== */

/* Gambar ikon tombol lipat (collapse): kotak dengan garis di tengah. */
static void pg_p_gambar_ikon_lipat(pg_permukaan_t *s, pg_kotak_t r,
                                     pg_warna_t fg)
{
        int cx = r.x + r.w / 2;
        int cy = r.y + r.h / 2;
        int hw = r.w / 2 - 3;
        int hh = r.h / 2 - 3;
        pg_gambar_kotak_aa(s, pg_buat_kotak(cx-hw, cy-hh, hw*2, hh*2), fg);
        /* Garis bawah = collapsed state. */
        pg_garis_h_permukaan(s, cx-hw, cy+hh-1, hw*2, fg);
}

/* Gambar ikon tombol semula (restore): kotak + arrow. */
static void pg_p_gambar_ikon_semula(pg_permukaan_t *s, pg_kotak_t r,
                                      pg_warna_t fg)
{
        int cx = r.x + r.w / 2;
        int cy = r.y + r.h / 2;
        int hw = r.w / 2 - 3;
        int hh = r.h / 2 - 3;
        /* Kotak luar. */
        pg_gambar_kotak_aa(s, pg_buat_kotak(cx-hw, cy-hh, hw*2, hh*2), fg);
        /* Panah ke kanan atas ( Restore ke floating). */
        pg_gambar_garis_aa(s,
                PG_KE_FIXED(cx-hw+2), PG_KE_FIXED(cy-hh+2),
                PG_KE_FIXED(cx+hw-2), PG_KE_FIXED(cy-hh+2), fg);
        pg_gambar_garis_aa(s,
                PG_KE_FIXED(cx+hw-2), PG_KE_FIXED(cy-hh+2),
                PG_KE_FIXED(cx+hw-2), PG_KE_FIXED(cy+hh-2), fg);
}

/* Gambar ikon tombol tutup (close): X. */
static void pg_p_gambar_ikon_tutup(pg_permukaan_t *s, pg_kotak_t r,
                                     pg_warna_t fg)
{
        int cx = r.x + r.w / 2;
        int cy = r.y + r.h / 2;
        int hw = r.w / 2 - 3;
        int hh = r.h / 2 - 3;
        pg_gambar_garis_aa(s,
                PG_KE_FIXED(cx-hw), PG_KE_FIXED(cy-hh),
                PG_KE_FIXED(cx+hw), PG_KE_FIXED(cy+hh), fg);
        pg_gambar_garis_aa(s,
                PG_KE_FIXED(cx+hw), PG_KE_FIXED(cy-hh),
                PG_KE_FIXED(cx-hw), PG_KE_FIXED(cy+hh), fg);
}

/* Gambar grip box (untuk GRIP mode). */
static void pg_p_gambar_grip(pg_panel_t *p, pg_permukaan_t *s)
{
        int w = p->base.kotak.w;
        int h = p->base.kotak.h;
        pg_warna_t grip_bg = p->header_bg_grip;
        pg_warna_t grip_dot = PG_RGB(0x60, 0x60, 0x60);
        if (p->grip_vertikal) {
                /* Grip di atas. */
                pg_kotak_t gr = pg_buat_kotak(0, 0, w, PG_PANEL_GRIP_SIZE);
                pg_isi_permukaan_kotak(s, gr, grip_bg);
                /* Dots pattern (2 baris titik). */
                {
                        int dx, dy;
                        int start_x = (w - 8) / 2;
                        int start_y = (PG_PANEL_GRIP_SIZE - 4) / 2;
                        for (dy = 0; dy < 2; dy++) {
                                for (dx = 0; dx < 4; dx++) {
                                        int px = start_x + dx*3;
                                        int py = start_y + dy*3;
                                        if (px < w-1 && py < PG_PANEL_GRIP_SIZE-1)
                                                pg_isi_permukaan_kotak(s,
                                                        pg_buat_kotak(px, py, 1, 1),
                                                        grip_dot);
                                }
                        }
                }
        } else {
                /* Grip di kiri. */
                pg_kotak_t gr = pg_buat_kotak(0, 0, PG_PANEL_GRIP_SIZE, h);
                pg_isi_permukaan_kotak(s, gr, grip_bg);
                {
                        int dx, dy;
                        int start_y = (h - 8) / 2;
                        int start_x = (PG_PANEL_GRIP_SIZE - 4) / 2;
                        for (dx = 0; dx < 2; dx++) {
                                for (dy = 0; dy < 4; dy++) {
                                        int px = start_x + dx*3;
                                        int py = start_y + dy*3;
                                        if (px < PG_PANEL_GRIP_SIZE-1 && py < h-1)
                                                pg_isi_permukaan_kotak(s,
                                                        pg_buat_kotak(px, py, 1, 1),
                                                        grip_dot);
                                }
                        }
                }
        }
}

static void pg_p_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
        pg_panel_t *p = pg_p_dari(w);
        int sw = p->base.kotak.w;
        int sh = p->base.kotak.h;
        int hh;
        pg_warna_t header_bg;
        int baseline;
        int i;

        if (!p->base.terlihat) return;

        hh = pg_p_header_h(p);

        /* Body background. */
        pg_isi_permukaan_kotak(s, pg_buat_kotak(0, 0, sw, sh),
                p->body_bg);

        /* Header. */
        switch (p->header_mode) {
        case PG_PANEL_HEADER_FULL:
                header_bg = p->header_bg_full;
                break;
        case PG_PANEL_HEADER_THIN:
                header_bg = p->header_bg_thin;
                break;
        case PG_PANEL_HEADER_GRIP:
                pg_p_gambar_grip(p, s);
                goto render_body;
        default:
                header_bg = p->header_bg_full;
        }

        if (p->collapsed) {
                /* Hanya header terlihat. */
                pg_isi_permukaan_kotak(s,
                        pg_buat_kotak(0, 0, sw, PG_PANEL_HEADER_H),
                        header_bg);
        } else {
                pg_isi_permukaan_kotak(s,
                        pg_buat_kotak(0, 0, sw, hh), header_bg);
        }

        /* Judul. */
        if (p->judul && p->font) {
                baseline = pg_font_baseline_tengah(p->font, PG_PANEL_HEADER_H);
                pg_font_gambar_teks(p->font, p->judul, s, 4, baseline,
                        p->header_fg);
        }

        /* Tombol header (kanan ke kiri: tutup, semula, lipat). */
        for (i = 0; i < 3; i++) {
                pg_kotak_t r = pg_p_btn_kotak(p, i);
                pg_warna_t btn_bg;
                pg_warna_t btn_fg = p->header_fg;
                /* Background tombol: hover / ditekan. */
                if (p->ditekan_btn == i && p->hover_btn == i)
                        btn_bg = PG_RGB(0x60, 0x60, 0x60);
                else if (p->hover_btn == i)
                        btn_bg = PG_RGB(0x40, 0x40, 0x40);
                else
                        btn_bg = header_bg;
                pg_isi_permukaan_kotak(s, r, btn_bg);
                /* Ikon. */
                switch ((pg_panel_btn_t)i) {
                case PG_PANEL_BTN_LIPAT:
                        pg_p_gambar_ikon_lipat(s, r, btn_fg);
                        break;
                case PG_PANEL_BTN_SEMULA:
                        pg_p_gambar_ikon_semula(s, r, btn_fg);
                        break;
                case PG_PANEL_BTN_TUTUP:
                        pg_p_gambar_ikon_tutup(s, r, btn_fg);
                        break;
                default:
                        break;
                }
        }

render_body:
        /* Body: child widget. */
        if (!p->collapsed && p->anak) {
                pg_widget_catat(p->anak, s);
        }
        /* Border (untuk floating). */
        if (p->mode == PG_PANEL_FLOATING) {
                pg_gambar_kotak_aa(s,
                        pg_buat_kotak(0, 0, sw, sh), p->batas);
                /* Grip resize di corner kanan-bawah. */
                if (!p->collapsed) {
                        int gx = sw - PG_PANEL_RESIZE_GRIP;
                        int gy = sh - PG_PANEL_RESIZE_GRIP;
                        /* Kotak kecil abu dengan dot pattern. */
                        pg_warna_t grip_bg = PG_ABU;
                        pg_warna_t grip_dot = PG_ABU_GELAP;
                        pg_isi_permukaan_kotak(s,
                                pg_buat_kotak(gx, gy,
                                        PG_PANEL_RESIZE_GRIP,
                                        PG_PANEL_RESIZE_GRIP),
                                grip_bg);
                        {
                                int dx, dy;
                                for (dy = 0; dy < 3; dy++) {
                                        for (dx = 0; dx < 3; dx++) {
                                                int px = gx + 2 + dx*3;
                                                int py = gy + 2 + dy*3;
                                                if (px < sw-1 && py < sh-1)
                                                        pg_isi_permukaan_kotak(s,
                                                                pg_buat_kotak(px, py, 1, 1),
                                                                grip_dot);
                                        }
                                }
                        }
                }
        }
}

/* ===== Event handler ===== */

static pg_bool pg_p_peristiwa_v(pg_widget_t *w, const pg_aksi_t *e)
{
        pg_panel_t *p = pg_p_dari(w);
        int mx, my;  /* posisi mouse LOKAL (relatif ke panel, 0..w, 0..h).
                      pg_widget_tangani_aksi sudah konversi dari
                      absolute ke lokal sebelum panggil vtable ini. */
        int panel_x, panel_y;  /* posisi panel di layar (absolute). */
        if (!p || !p->base.terlihat) return PG_SALAH;
        mx = e->tetik_pos.x;
        my = e->tetik_pos.y;
        panel_x = p->base.kotak.x;
        panel_y = p->base.kotak.y;

        /* 1. Drag sedang aktif. */
        if (p->menyeret) {
                if (e->tipe == PG_AKSI_TETIKUS_GERAK) {
                        /* Konversi mx,my (lokal) ke absolute: panel_x +
                         * mx. Posisi panel baru = mouse_absolute - offset
                         * drag. Offset drag disimpan saat klik awal
                         * (lihat step 3). */
                        int abs_mx = panel_x + mx;
                        int abs_my = panel_y + my;
                        pg_widget_pindah(&p->base,
                                abs_mx - p->seret_ofs_x,
                                abs_my - p->seret_ofs_y);
                        if (p->cb_drag_gerak)
                                p->cb_drag_gerak(p, abs_mx, abs_my,
                                        p->cb_ctx);
                        return PG_BENAR;
                }
                if (e->tipe == PG_AKSI_TETIKUS_LEPAS) {
                        int abs_mx = panel_x + mx;
                        int abs_my = panel_y + my;
                        p->menyeret = PG_SALAH;
                        if (p->cb_drag_selesai)
                                p->cb_drag_selesai(p, abs_mx, abs_my,
                                        p->cb_ctx);
                        if (p->cb_drag_gerak)
                                p->cb_drag_gerak(p, -1, -1, p->cb_ctx);
                        return PG_BENAR;
                }
                return PG_BENAR;
        }

        /* 1b. Resize sedang aktif (jika ada). */
        if (p->resize_menyeret) {
                if (e->tipe == PG_AKSI_TETIKUS_GERAK) {
                        int abs_mx = panel_x + mx;
                        int abs_my = panel_y + my;
                        int new_w = abs_mx - panel_x + p->resize_ofs_w;
                        int new_h = abs_my - panel_y + p->resize_ofs_h;
                        if (new_w < PG_PANEL_MIN_W) new_w = PG_PANEL_MIN_W;
                        if (new_h < PG_PANEL_MIN_H) new_h = PG_PANEL_MIN_H;
                        pg_panel_setel_ukuran(p, new_w, new_h);
                        return PG_BENAR;
                }
                if (e->tipe == PG_AKSI_TETIKUS_LEPAS) {
                        p->resize_menyeret = PG_SALAH;
                        return PG_BENAR;
                }
                return PG_BENAR;
        }

        /* 2. Tombol header (FULL/THIN mode). Pakai koordinat lokal. */
        if (e->tipe == PG_AKSI_TETIKUS_TEKAN &&
            e->tetik_tombol == PG_TETIKUS_KIRI) {
                pg_panel_btn_t btn = pg_p_btn_hit(p, mx, my);
                if (btn != PG_PANEL_BTN_NONE) {
                        p->ditekan_btn = btn;
                        p->hover_btn = btn;
                        return PG_BENAR;
                }
        }

        if (e->tipe == PG_AKSI_TETIKUS_LEPAS &&
            e->tetik_tombol == PG_TETIKUS_KIRI) {
                pg_panel_btn_t btn = pg_p_btn_hit(p, mx, my);
                if (p->ditekan_btn != PG_PANEL_BTN_NONE &&
                    btn == p->ditekan_btn) {
                        p->ditekan_btn = PG_PANEL_BTN_NONE;
                        switch (btn) {
                        case PG_PANEL_BTN_LIPAT:
                                p->collapsed = !p->collapsed;
                                pg_p_layout_anak(p);
                                if (p->cb_lipat)
                                        p->cb_lipat(p, p->collapsed, p->cb_ctx);
                                return PG_BENAR;
                        case PG_PANEL_BTN_SEMULA:
                                if (p->cb_semula)
                                        p->cb_semula(p, p->cb_ctx);
                                return PG_BENAR;
                        case PG_PANEL_BTN_TUTUP:
                                if (p->cb_tutup)
                                        p->cb_tutup(p, p->cb_ctx);
                                return PG_BENAR;
                        default:
                                break;
                        }
                }
                p->ditekan_btn = PG_PANEL_BTN_NONE;
        }

        /* Hover tracking tombol. */
        if (e->tipe == PG_AKSI_TETIKUS_GERAK &&
            p->header_mode != PG_PANEL_HEADER_GRIP) {
                p->hover_btn = pg_p_btn_hit(p, mx, my);
        }

        /* 3. Mulai drag via header. Pakai koordinat lokal untuk
         *    hit-test, simpan offset (lokal, relatif ke panel). */
        if (e->tipe == PG_AKSI_TETIKUS_TEKAN &&
            e->tetik_tombol == PG_TETIKUS_KIRI) {
                int hh = pg_p_header_h(p);
                int in_header = 0;
                if (p->header_mode == PG_PANEL_HEADER_GRIP) {
                        if (p->grip_vertikal)
                                in_header = (my >= 0 && my < PG_PANEL_GRIP_SIZE);
                        else
                                in_header = (mx >= 0 && mx < PG_PANEL_GRIP_SIZE);
                } else {
                        in_header = (my >= 0 && my < hh);
                        if (pg_p_btn_hit(p, mx, my) != PG_PANEL_BTN_NONE)
                                in_header = 0;
                }
                if (in_header) {
                        p->menyeret = PG_BENAR;
                        /* Offset drag (lokal): mx, my saat klik.
                         * Saat GERAK: posisi panel baru = absolute_mouse
                         * (panel_x + mx_gerak) - offset. */
                        p->seret_ofs_x = mx;
                        p->seret_ofs_y = my;
                        return PG_BENAR;
                }

                /* 3b. Cek grip resize di corner kanan-bawah (floating). */
                if (p->mode == PG_PANEL_FLOATING && !p->collapsed) {
                        int pw = p->base.kotak.w;
                        int ph = p->base.kotak.h;
                        int grip = PG_PANEL_RESIZE_GRIP;
                        if (mx >= pw - grip && mx < pw &&
                            my >= ph - grip && my < ph) {
                                p->resize_menyeret = PG_BENAR;
                                /* Offset resize (lokal). */
                                p->resize_ofs_w = pw - mx;
                                p->resize_ofs_h = ph - my;
                                return PG_BENAR;
                        }
                }
        }

        /* 4. Dispatch ke child widget (body). Child punya posisi
         *    lokal di permukaan panel. e->tetik_pos sudah lokal
         *    panel, perlu konversi ke lokal child: subtract
         *    anak->kotak.x. */
        if (!p->collapsed && p->anak) {
                pg_aksi_t e2 = *e;
                e2.tetik_pos.x = mx - p->anak->kotak.x;
                e2.tetik_pos.y = my - p->anak->kotak.y;
                if (pg_widget_tangani_aksi(p->anak, &e2))
                        return PG_BENAR;
        }

        return PG_SALAH;
}

static void pg_p_ubah_ukuran_v(pg_widget_t *w, int w_, int h)
{
        (void)w_;
        (void)h;
        pg_p_layout_anak(pg_p_dari(w));
}

static void pg_p_hancur_v(pg_widget_t *w)
{
        pg_panel_t *p = pg_p_dari(w);
        if (p->judul) {
                free(p->judul);
                p->judul = NULL;
        }
        /* Catatan: anak TIDAK dihancurkan — app yang punya. */
}

static void pg_p_bebas_v(pg_widget_t *w)
{
        free(w);
}

/* ===== API publik ===== */

pg_panel_t *pg_buat_panel(const char *judul, pg_panel_flag_t flag,
                           int w, int h, pg_font_t *font)
{
        pg_panel_t *p;
        if (w <= 0) w = 200;
        if (h <= 0) h = 100;
        p = (pg_panel_t *)calloc(1, sizeof(*p));
        if (!p) return NULL;
        pg_widget_init(&p->base, PG_WIDGET_JENDELA, &pg_p_vtable);
        pg_widget_milik(&p->base, PG_BENAR);
        p->flag = flag;
        p->mode = PG_PANEL_FLOATING;
        p->header_mode = PG_PANEL_HEADER_FULL;
        p->collapsed = PG_SALAH;
        p->grip_vertikal = PG_SALAH;
        p->menyeret = PG_SALAH;
        p->ditekan_btn = PG_PANEL_BTN_NONE;
        p->hover_btn = PG_PANEL_BTN_NONE;
        p->font = font;
        p->header_bg_full = PG_BIRU;
        p->header_bg_thin = PG_ABU_TERANG;
        p->header_bg_grip = PG_ABU;
        p->header_fg = PG_PUTIH;
        p->body_bg = PG_ABU_TERANG;
        p->batas = PG_ABU_GELAP;
        if (judul) {
                p->judul = pg_p_dup(judul);
        }
        pg_widget_setel_kotak(&p->base, pg_buat_kotak(0, 0, w, h));
        return p;
}

void pg_panel_hancur(pg_panel_t *p)
{
        if (!p) return;
        pg_widget_hancur(&p->base);
        free(p);
}

void pg_panel_setel_anak(pg_panel_t *p, pg_widget_t *child)
{
        if (!p) return;
        /* Lepas anak lama bila ada. */
        if (p->anak)
                pg_widget_hapus_anak(&p->base, p->anak);
        p->anak = child;
        if (child)
                pg_widget_tambah_anak(&p->base, child);
        pg_p_layout_anak(p);
        pg_widget_kotor(&p->base);
}

pg_widget_t *pg_panel_anak(pg_panel_t *p)
{
        return p ? p->anak : NULL;
}

pg_panel_flag_t pg_panel_flag(const pg_panel_t *p)
{
        return p ? p->flag : PG_PANEL_FLAG_TOOLBAR;
}

const char *pg_panel_judul(const pg_panel_t *p)
{
        return p ? p->judul : NULL;
}

void pg_panel_setel_judul(pg_panel_t *p, const char *judul)
{
        if (!p) return;
        if (p->judul) free(p->judul);
        p->judul = judul ? pg_p_dup(judul) : NULL;
        pg_widget_kotor(&p->base);
}

pg_panel_mode_t pg_panel_mode(const pg_panel_t *p)
{
        return p ? p->mode : PG_PANEL_FLOATING;
}

pg_bool pg_panel_collapsed(const pg_panel_t *p)
{
        return p ? p->collapsed : PG_SALAH;
}

void pg_panel_setel_collapsed(pg_panel_t *p, pg_bool collapsed)
{
        if (!p) return;
        p->collapsed = collapsed;
        pg_p_layout_anak(p);
        pg_widget_kotor(&p->base);
}

pg_panel_header_mode_t pg_panel_header_mode(const pg_panel_t *p)
{
        return p ? p->header_mode : PG_PANEL_HEADER_FULL;
}

void pg_panel_setel_header_mode(pg_panel_t *p, pg_panel_header_mode_t mode)
{
        if (!p) return;
        p->header_mode = mode;
        pg_p_layout_anak(p);
        pg_widget_kotor(&p->base);
}

void pg_panel_pindah(pg_panel_t *p, int x, int y)
{
        if (!p) return;
        pg_widget_pindah(&p->base, x, y);
}

void pg_panel_setel_ukuran(pg_panel_t *p, int w, int h)
{
        if (!p) return;
        /* Clamp ke minimum HANYA bila floating. Saat docked,
         * panel harus ikut ukuran dock — bila tidak, panel
         * dengan MIN_H=60 di dock area 50px akan overflow
         * menimpa dock lain di atas/bawah. */
        if (p->mode == PG_PANEL_FLOATING) {
                if (w < PG_PANEL_MIN_W) w = PG_PANEL_MIN_W;
                if (h < PG_PANEL_MIN_H) h = PG_PANEL_MIN_H;
        }
        pg_widget_setel_kotak(&p->base,
                pg_buat_kotak(p->base.kotak.x, p->base.kotak.y, w, h));
}

pg_kotak_t pg_panel_kotak(const pg_panel_t *p)
{
        return p ? p->base.kotak : pg_buat_kotak(0,0,0,0);
}

void pg_panel_tampil(pg_panel_t *p)
{
        if (!p) return;
        pg_widget_tampil(&p->base);
}

void pg_panel_sembunyi(pg_panel_t *p)
{
        if (!p) return;
        pg_widget_sembunyi(&p->base);
}

pg_bool pg_panel_terlihat(const pg_panel_t *p)
{
        return p ? p->base.terlihat : PG_SALAH;
}

void pg_panel_saat_tutup(pg_panel_t *p, pg_panel_cb cb, void *ctx)
{
        if (!p) return;
        p->cb_tutup = cb;
        p->cb_ctx = ctx;
}

void pg_panel_saat_semula(pg_panel_t *p, pg_panel_cb cb, void *ctx)
{
        if (!p) return;
        p->cb_semula = cb;
        p->cb_ctx = ctx;
}

void pg_panel_saat_lipat(pg_panel_t *p, pg_panel_lipat_cb cb, void *ctx)
{
        if (!p) return;
        p->cb_lipat = cb;
        p->cb_ctx = ctx;
}

void pg_panel_saat_drag_selesai(pg_panel_t *p, pg_panel_drag_cb cb, void *ctx)
{
        if (!p) return;
        p->cb_drag_selesai = cb;
        p->cb_ctx = ctx;
}

void pg_panel_saat_drag_gerak(pg_panel_t *p, pg_panel_drag_gerak_cb cb,
                                void *ctx)
{
        if (!p) return;
        p->cb_drag_gerak = cb;
        p->cb_ctx = ctx;
}

void pg_panel_catat(pg_panel_t *p, pg_permukaan_t *dest)
{
        if (!p || !dest) return;
        pg_widget_catat(&p->base, dest);
}

pg_bool pg_panel_tangani(pg_panel_t *p, const pg_aksi_t *e)
{
        if (!p || !e) return PG_SALAH;
        return pg_widget_tangani_aksi(&p->base, e);
}

pg_widget_t *pg_panel_widget(pg_panel_t *p)
{
        return p ? &p->base : NULL;
}

pg_bool pg_panel_berisi(const pg_panel_t *p, pg_titik_t pt)
{
        if (!p) return PG_SALAH;
        return pg_widget_berisi(&p->base, pt);
}

/* Internal: set grip orientasi (dipanggil oleh dock). */
void pg_panel_setel_grip_vertikal(pg_panel_t *p, pg_bool vertikal)
{
        if (!p) return;
        p->grip_vertikal = vertikal;
        pg_p_layout_anak(p);
        pg_widget_kotor(&p->base);
}

/* Internal: set mode docked (dipanggil oleh dock). */
void pg_panel_setel_mode(pg_panel_t *p, pg_panel_mode_t mode)
{
        if (!p) return;
        p->mode = mode;
}
