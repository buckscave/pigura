/* -------------------------------------------------------------------------- *
 * pigura/widget/komponen/dock.c - Dock manager (multi-panel + splitter)
 * -------------------------------------------------------------------------- *
 * Dock = wrapper yang menampung multiple panel berurutan sesuai orientasi.
 *
 * Layout:
 *   - ATAS/BAWAH → panel disusun horizontal (kiri ke kanan)
 *   - KIRI/KANAN → panel disusun vertikal (atas ke bawah)
 *
 * Pembatas antar panel = splitter dragable.
 *
 * Header panel auto-set berdasarkan flag dock:
 *   - PROPERTIES → header THIN
 *   - TOOLBAR/RIBBON → header GRIP (orientasi = dock orientasi)
 *
 * Dock TIDAK create/free panel. App yang punya.
 * -------------------------------------------------------------------------- */
#include "pigura/dock.h"
#include "pigura/panel.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/peristiwa.h"
#include "pigura/widget.h"
#include "pigura/galat.h"
#include <stdlib.h>
#include <string.h>

/* ===== Konstanta ===== */

#define PG_DOCK_SPLITTER_TEBAL  6
#define PG_DOCK_SPLITTER_TARIK  4
#define PG_DOCK_MIN_PANEL       24

/* ===== Struct internal ===== */

struct pg_dock {
        pg_dock_flag_t      flag;
        pg_dock_posisi_t    posisi;
        pg_font_t          *font;

        /* Area dock di layar. */
        int x, y, w, h;

        /* Daftar panel. */
        pg_panel_t        **panels;
        int                 n_panels;
        int                 cap_panels;

        /* Ukuran tiap panel (bisa di-resize via splitter). Array
         * n_panels. Bila NULL, panel dibagi rata. */
        int                *ukuran_panels;
        int                 ukuran_cap;

        /* Splitter state. */
        int                 split_drag_idx;  /* -1 = tidak drag */
        int                 split_drag_start;
        int                 split_drag_offset; /* posisi splitter awal */
};

/* ===== Forward declaration internal ===== */

/* Dari panel.c — API internal panel. */
extern void pg_panel_setel_grip_vertikal(pg_panel_t *p, pg_bool vertikal);
extern void pg_panel_setel_mode(pg_panel_t *p, pg_panel_mode_t mode);

/* Internal helpers (di bawah). */
static pg_bool pg_dock_ukuran_grow(pg_dock_t *d, int cap_baru);
static void pg_dock_ukuran_reset(pg_dock_t *d);
static void pg_dock_ukuran_sesuaikan(pg_dock_t *d);
static void pg_dock_ukuran_resize(pg_dock_t *d, int old_avail, int new_avail);
static void pg_dock_hitung_ukuran(pg_dock_t *d,
                                    int *ukuran_per_panel,
                                    int *sisa);

/* ===== Helper ===== */

static pg_bool pg_dock_vertikal(const pg_dock_t *d)
{
        return (d->posisi == PG_DOCK_POS_KIRI ||
                d->posisi == PG_DOCK_POS_KANAN);
}

/* Auto-set header mode panel berdasarkan flag dock. */
static void pg_dock_panel_update_header(pg_dock_t *d, pg_panel_t *p)
{
        pg_panel_header_mode_t mode;
        pg_bool grip_vertikal;
        if (!d || !p) return;
        grip_vertikal = pg_dock_vertikal(d);
        switch (d->flag) {
        case PG_DOCK_FLAG_PROPERTIES:
                mode = PG_PANEL_HEADER_THIN;
                break;
        case PG_DOCK_FLAG_TOOLBAR:
        case PG_DOCK_FLAG_RIBBON:
                mode = PG_PANEL_HEADER_GRIP;
                break;
        default:
                mode = PG_PANEL_HEADER_THIN;
                break;
        }
        pg_panel_setel_header_mode(p, mode);
        pg_panel_setel_grip_vertikal(p, grip_vertikal);
        pg_panel_setel_mode(p, 1); /* DOCKED */
}

/* ===== Buat/hancur ===== */

pg_dock_t *pg_buat_dock(pg_dock_flag_t flag, pg_dock_posisi_t posisi)
{
        pg_dock_t *d = (pg_dock_t *)calloc(1, sizeof(*d));
        if (!d) return NULL;
        d->flag = flag;
        d->posisi = posisi;
        d->font = NULL;
        d->x = d->y = d->w = d->h = 0;
        d->panels = NULL;
        d->n_panels = 0;
        d->cap_panels = 0;
        d->split_drag_idx = -1;
        return d;
}

void pg_dock_hancur(pg_dock_t *d)
{
        if (!d) return;
        /* Reset header panel ke FULL sebelum dock hilang. */
        {
                int i;
                for (i = 0; i < d->n_panels; i++) {
                        if (d->panels[i]) {
                                pg_panel_setel_header_mode(d->panels[i],
                                        PG_PANEL_HEADER_FULL);
                                pg_panel_setel_mode(d->panels[i], 0);
                        }
                }
        }
        if (d->panels) free(d->panels);
        if (d->ukuran_panels) free(d->ukuran_panels);
        free(d);
}

/* ===== Identitas ===== */

pg_dock_flag_t pg_dock_flag(const pg_dock_t *d)
{
        return d ? d->flag : PG_DOCK_FLAG_TOOLBAR;
}

pg_dock_posisi_t pg_dock_posisi(const pg_dock_t *d)
{
        return d ? d->posisi : PG_DOCK_POS_ATAS;
}

/* ===== Setel area ===== */

void pg_dock_setel_area(pg_dock_t *d, int x, int y, int w, int h)
{
        int old_avail, new_avail;
        if (!d) return;
        old_avail = (pg_dock_vertikal(d) ? d->h : d->w) -
                (d->n_panels > 1 ?
                        (d->n_panels - 1) * PG_DOCK_SPLITTER_TEBAL : 0);
        d->x = x;
        d->y = y;
        d->w = w;
        d->h = h;
        new_avail = (pg_dock_vertikal(d) ? d->h : d->w) -
                (d->n_panels > 1 ?
                        (d->n_panels - 1) * PG_DOCK_SPLITTER_TEBAL : 0);
        if (d->n_panels > 0 && old_avail > 0 && new_avail > 0 &&
            old_avail != new_avail) {
                pg_dock_ukuran_resize(d, old_avail, new_avail);
        }
}

pg_kotak_t pg_dock_area(const pg_dock_t *d)
{
        return d ? pg_buat_kotak(d->x, d->y, d->w, d->h) :
                pg_buat_kotak(0,0,0,0);
}

void pg_dock_setel_font(pg_dock_t *d, pg_font_t *font)
{
        if (!d) return;
        d->font = font;
}

/* ===== Panel management ===== */

pg_bool pg_dock_tambah_panel(pg_dock_t *d, pg_panel_t *p, int idx)
{
        pg_panel_t **baru;
        int cap_baru;
        int i;
        if (!d || !p) return PG_SALAH;
        /* Cek flag cocok. */
        if (pg_panel_flag(p) != (pg_panel_flag_t)d->flag) {
                pg_set_galat(PG_GALAT_ARGUMEN,
                        "dock: flag panel tidak cocok");
                return PG_SALAH;
        }
        /* Cek apakah sudah ada. */
        for (i = 0; i < d->n_panels; i++) {
                if (d->panels[i] == p) return PG_SALAH;
        }
        /* Grow array bila perlu. */
        if (d->n_panels >= d->cap_panels) {
                cap_baru = d->cap_panels ? d->cap_panels * 2 : 4;
                baru = (pg_panel_t **)realloc(d->panels,
                        (size_t)cap_baru * sizeof(*baru));
                if (!baru) return PG_SALAH;
                d->panels = baru;
                d->cap_panels = cap_baru;
        }
        /* Grow ukuran_panels array juga. */
        if (!d->ukuran_panels || d->ukuran_cap < d->n_panels + 1) {
                int cap_u = d->ukuran_cap ? d->ukuran_cap : 4;
                while (cap_u < d->n_panels + 1) cap_u *= 2;
                pg_dock_ukuran_grow(d, cap_u);
        }
        /* Insert di idx atau append. */
        if (idx < 0 || idx > d->n_panels) idx = d->n_panels;
        for (i = d->n_panels; i > idx; i--) {
                d->panels[i] = d->panels[i-1];
                if (d->ukuran_panels)
                        d->ukuran_panels[i] = d->ukuran_panels[i-1];
        }
        d->panels[idx] = p;
        if (d->ukuran_panels) {
                /* Pakai ukuran preferred panel (ukuran saat buat
                 * atau ukuran terakhir yang di-set user). */
                pg_kotak_t pk = pg_panel_kotak(p);
                int pref = pg_dock_vertikal(d) ? pk.h : pk.w;
                if (pref <= 0) pref = 100;
                d->ukuran_panels[idx] = pref;
        }
        d->n_panels++;
        /* Update header panel. */
        pg_dock_panel_update_header(d, p);
        /* Sesuaikan ukuran: panel baru dapat jatah dari yang ada,
         * bukan reset total. */
        if (d->n_panels == 1)
                pg_dock_ukuran_reset(d);
        else
                pg_dock_ukuran_sesuaikan(d);
        return PG_BENAR;
}

void pg_dock_hapus_panel(pg_dock_t *d, pg_panel_t *p)
{
        int i, j;
        if (!d || !p) return;
        for (i = 0; i < d->n_panels; i++) {
                if (d->panels[i] == p) {
                        /* Reset header ke FULL. */
                        pg_panel_setel_header_mode(p, PG_PANEL_HEADER_FULL);
                        pg_panel_setel_mode(p, 0);
                        /* Shift kiri. */
                        for (j = i; j < d->n_panels - 1; j++) {
                                d->panels[j] = d->panels[j+1];
                                if (d->ukuran_panels)
                                        d->ukuran_panels[j] =
                                                d->ukuran_panels[j+1];
                        }
                        d->n_panels--;
                        if (d->ukuran_panels && d->n_panels > 0)
                                pg_dock_ukuran_sesuaikan(d);
                        return;
                }
        }
}

pg_bool pg_dock_punya_panel(pg_dock_t *d, pg_panel_t *p)
{
        int i;
        if (!d || !p) return PG_SALAH;
        for (i = 0; i < d->n_panels; i++)
                if (d->panels[i] == p) return PG_BENAR;
        return PG_SALAH;
}

int pg_dock_jumlah_panel(const pg_dock_t *d)
{
        return d ? d->n_panels : 0;
}

pg_panel_t *pg_dock_panel_di(const pg_dock_t *d, int idx)
{
        if (!d || idx < 0 || idx >= d->n_panels) return NULL;
        return d->panels[idx];
}

/* ===== Hit-test ===== */

pg_bool pg_dock_berisi(pg_dock_t *d, int mx, int my)
{
        if (!d) return PG_SALAH;
        return (mx >= d->x && mx < d->x + d->w &&
                my >= d->y && my < d->y + d->h) ?
                PG_BENAR : PG_SALAH;
}

pg_bool pg_dock_terima_flag(pg_dock_t *d, pg_panel_flag_t flag)
{
        if (!d) return PG_SALAH;
        return ((int)flag == (int)d->flag) ? PG_BENAR : PG_SALAH;
}

/* ===== Hit-test insert ===== */

int pg_dock_indeks_insert(pg_dock_t *d, int mx, int my)
{
        int ukuran_per, sisa;
        int i, pos;
        int n;
        if (!d) return 0;
        n = d->n_panels;
        if (n <= 0) return 0;
        pg_dock_hitung_ukuran(d, &ukuran_per, &sisa);
        /* Cek posisi mouse relatif ke panel-panel di dock. */
        pos = 0;
        for (i = 0; i < n; i++) {
                int sz = ukuran_per + (i == n-1 ? sisa : 0);
                int panel_start, panel_end;
                if (pg_dock_vertikal(d)) {
                        panel_start = d->y + pos;
                        panel_end = panel_start + sz;
                        if (my < panel_start) return i;
                        if (my < panel_start + sz/2) return i;
                        if (my < panel_end) return i+1;
                } else {
                        panel_start = d->x + pos;
                        panel_end = panel_start + sz;
                        if (mx < panel_start) return i;
                        if (mx < panel_start + sz/2) return i;
                        if (mx < panel_end) return i+1;
                }
                pos += sz + PG_DOCK_SPLITTER_TEBAL;
        }
        return n;
}

int pg_dock_indeks_panel_di(pg_dock_t *d, int mx, int my)
{
        int ukuran_per, sisa;
        int i, pos;
        if (!d || d->n_panels <= 0) return -1;
        pg_dock_hitung_ukuran(d, &ukuran_per, &sisa);
        pos = 0;
        for (i = 0; i < d->n_panels; i++) {
                int sz = ukuran_per + (i == d->n_panels-1 ? sisa : 0);
                int panel_start, panel_end;
                if (pg_dock_vertikal(d)) {
                        panel_start = d->y + pos;
                        panel_end = panel_start + sz;
                        if (my >= panel_start && my < panel_end)
                                return i;
                } else {
                        panel_start = d->x + pos;
                        panel_end = panel_start + sz;
                        if (mx >= panel_start && mx < panel_end)
                                return i;
                }
                pos += sz + PG_DOCK_SPLITTER_TEBAL;
        }
        return -1;
}

/* ===== Layout ===== */

/* Hitung ukuran preferred setiap panel.
 * Pakai array ukuran_panels bila ada. Bila tidak, bagi rata. */
static void pg_dock_hitung_ukuran(pg_dock_t *d,
                                    int *ukuran_per_panel,
                                    int *sisa)
{
        int total, splitter_total, avail;
        int n = d->n_panels;
        if (n <= 0) {
                *ukuran_per_panel = 0;
                *sisa = 0;
                return;
        }
        /* Bila ukuran_panels sudah diset (dari resize), gunakan. */
        if (d->ukuran_panels && d->ukuran_panels[0] > 0) {
                /* Tidak dipakai dalam mode ini — caller pakai array
                 * langsung. Tapi untuk compat, hitung total. */
                *ukuran_per_panel = 0;
                *sisa = 0;
                return;
        }
        splitter_total = (n - 1) * PG_DOCK_SPLITTER_TEBAL;
        if (pg_dock_vertikal(d))
                avail = d->h;
        else
                avail = d->w;
        total = avail - splitter_total;
        if (total < 0) total = 0;
        *ukuran_per_panel = total / n;
        *sisa = total - (*ukuran_per_panel * n);
}

/* Hitung total ukuran semua panel. */
static int pg_dock_total_ukuran(const pg_dock_t *d)
{
        int i, total = 0;
        if (!d) return 0;
        if (!d->ukuran_panels) return 0;
        for (i = 0; i < d->n_panels; i++)
                total += d->ukuran_panels[i];
        return total;
}

/* Alokasi/resize array ukuran_panels. */
static pg_bool pg_dock_ukuran_grow(pg_dock_t *d, int cap_baru)
{
        int *baru;
        if (!d) return PG_SALAH;
        baru = (int *)realloc(d->ukuran_panels,
                (size_t)cap_baru * sizeof(int));
        if (!baru) return PG_SALAH;
        /* Init entry baru ke 0 (auto-distribute). */
        {
                int i;
                for (i = d->ukuran_cap; i < cap_baru; i++)
                        baru[i] = 0;
        }
        d->ukuran_panels = baru;
        d->ukuran_cap = cap_baru;
        return PG_BENAR;
}

/* Inisialisasi ukuran_panels: distribusi merata.
 * Dipakai saat dock pertama dibuat atau panel pertama masuk.
 * Setelah itu, ukuran panel dipertahankan — user bisa resize
 * manual via splitter. */
static void pg_dock_ukuran_reset(pg_dock_t *d)
{
        int avail, per, sisa, i;
        if (!d || d->n_panels <= 0) return;
        if (!d->ukuran_panels) {
                if (!pg_dock_ukuran_grow(d, d->n_panels)) return;
        }
        avail = (pg_dock_vertikal(d) ? d->h : d->w) -
                (d->n_panels - 1) * PG_DOCK_SPLITTER_TEBAL;
        if (avail < 0) avail = 0;
        per = avail / d->n_panels;
        sisa = avail - per * d->n_panels;
        for (i = 0; i < d->n_panels; i++) {
                d->ukuran_panels[i] = per + (i == d->n_panels - 1 ? sisa : 0);
        }
}

/* Setelah panel tambah/lepas: JANGAN scale proporsional.
 * Panel tetap pakai ukuran preferred (saat buat) atau ukuran
 * yang sudah di-resize user. Hanya clamp bila total overflow
 * (total > area dock) — scale down ke area.
 *
 * Bila total < area dock: ruang kosong di akhir dock = OK,
 * tidak diisi (sesuai spec user). */
static void pg_dock_ukuran_sesuaikan(pg_dock_t *d)
{
        int avail, total, i;
        if (!d || !d->ukuran_panels || d->n_panels <= 0) return;
        avail = (pg_dock_vertikal(d) ? d->h : d->w) -
                (d->n_panels - 1) * PG_DOCK_SPLITTER_TEBAL;
        if (avail < 0) avail = 0;
        total = 0;
        for (i = 0; i < d->n_panels; i++)
                total += d->ukuran_panels[i];
        if (total <= 0) {
                pg_dock_ukuran_reset(d);
                return;
        }
        /* Hanya scale down bila overflow. Bila underflow, biarkan. */
        if (total > avail) {
                int new_total = 0;
                int extra;
                for (i = 0; i < d->n_panels; i++) {
                        int new_sz = (d->ukuran_panels[i] * avail) / total;
                        if (new_sz < PG_DOCK_MIN_PANEL)
                                new_sz = PG_DOCK_MIN_PANEL;
                        d->ukuran_panels[i] = new_sz;
                        new_total += new_sz;
                }
                extra = avail - new_total;
                if (extra != 0 && d->n_panels > 0)
                        d->ukuran_panels[d->n_panels - 1] += extra;
        }
}

/* Saat area dock berubah (window resize/maximize): scale semua
 * panel proporsional ke area baru. Preserve ratio ukuran. */
static void pg_dock_ukuran_resize(pg_dock_t *d, int old_avail, int new_avail)
{
        int i;
        if (!d || !d->ukuran_panels || d->n_panels <= 0) return;
        if (old_avail <= 0 || new_avail <= 0) {
                pg_dock_ukuran_reset(d);
                return;
        }
        {
                int new_total = 0;
                int extra;
                for (i = 0; i < d->n_panels; i++) {
                        int new_sz = (d->ukuran_panels[i] * new_avail) / old_avail;
                        if (new_sz < PG_DOCK_MIN_PANEL)
                                new_sz = PG_DOCK_MIN_PANEL;
                        d->ukuran_panels[i] = new_sz;
                        new_total += new_sz;
                }
                extra = new_avail - new_total;
                if (extra != 0 && d->n_panels > 0)
                        d->ukuran_panels[d->n_panels - 1] += extra;
        }
}

void pg_dock_tata(pg_dock_t *d)
{
        int i, pos;
        if (!d) return;
        /* Bila ukuran_panels belum di-set, inisialisasi merata. */
        if (!d->ukuran_panels || d->ukuran_cap < d->n_panels) {
                if (d->n_panels > 0) {
                        int cap_baru = d->ukuran_cap ? d->ukuran_cap : 4;
                        while (cap_baru < d->n_panels) cap_baru *= 2;
                        pg_dock_ukuran_grow(d, cap_baru);
                }
        }
        if (!d->ukuran_panels) return;
        /* Bila ada entry yang 0 (auto), reset semua. */
        {
                int perlu_reset = 0;
                for (i = 0; i < d->n_panels; i++)
                        if (d->ukuran_panels[i] <= 0) perlu_reset = 1;
                if (perlu_reset) pg_dock_ukuran_reset(d);
        }
        pos = 0;
        for (i = 0; i < d->n_panels; i++) {
                int sz = d->ukuran_panels[i];
                /* Bila panel collapsed, pakai ukuran header saja
                 * (tipis). Panel di bawahnya akan shift ke atas
                 * otomatis karena pos += sz_collapsed. */
                if (pg_panel_collapsed(d->panels[i])) {
                        pg_panel_header_mode_t hm =
                                pg_panel_header_mode(d->panels[i]);
                        if (hm == PG_PANEL_HEADER_GRIP) {
                                sz = 12; /* GRIP_SIZE */
                        } else {
                                sz = 20; /* HEADER_H */
                        }
                }
                if (sz < PG_DOCK_MIN_PANEL) sz = PG_DOCK_MIN_PANEL;
                if (pg_dock_vertikal(d))
                        pg_panel_pindah(d->panels[i], d->x, d->y + pos);
                else
                        pg_panel_pindah(d->panels[i], d->x + pos, d->y);
                /* Set ukuran panel = full dock cross + sz main. */
                if (pg_dock_vertikal(d)) {
                        int w = d->w;
                        int h = sz;
                        pg_panel_setel_ukuran(d->panels[i], w, h);
                } else {
                        int w = sz;
                        int h = d->h;
                        pg_panel_setel_ukuran(d->panels[i], w, h);
                }
                pos += sz + PG_DOCK_SPLITTER_TEBAL;
        }
}

/* ===== Render ===== */

/* Hit-test splitter mana yang di klik. Return -1 bila tidak. */
static int pg_dock_splitter_hit(pg_dock_t *d, int mx, int my)
{
        int i, pos;
        int t = PG_DOCK_SPLITTER_TARIK;
        if (!d || d->n_panels <= 1 || !d->ukuran_panels) return -1;
        pos = 0;
        for (i = 0; i < d->n_panels - 1; i++) {
                int sz = d->ukuran_panels[i];
                int split_pos;
                pos += sz;
                split_pos = pos + PG_DOCK_SPLITTER_TEBAL/2;
                if (pg_dock_vertikal(d)) {
                        int sy = d->y + split_pos;
                        if (mx >= d->x && mx < d->x + d->w &&
                            my >= sy - t && my < sy + t)
                                return i;
                } else {
                        int sx = d->x + split_pos;
                        if (my >= d->y && my < d->y + d->h &&
                            mx >= sx - t && mx < sx + t)
                                return i;
                }
                pos += PG_DOCK_SPLITTER_TEBAL;
        }
        return -1;
}

void pg_dock_catat(pg_dock_t *d, pg_permukaan_t *dest)
{
        int i;
        if (!d || !dest) return;
        /* Bila dock kosong, TIDAK render apa-apa (invisible). */
        if (d->n_panels <= 0) return;

        /* Latar dock. */
        pg_isi_permukaan_kotak(dest, pg_buat_kotak(d->x, d->y, d->w, d->h),
                PG_ABU_TERANG);

        /* Garis pembatas antar panel (splitter). */
        if (d->ukuran_panels) {
                for (i = 0; i < d->n_panels - 1; i++) {
                        int pos = d->ukuran_panels[i] + PG_DOCK_SPLITTER_TEBAL*i;
                        pg_warna_t gc = (d->split_drag_idx == i) ?
                                PG_ABU_TERANG : PG_ABU;
                        pg_kotak_t gr;
                        if (pg_dock_vertikal(d))
                                gr = pg_buat_kotak(d->x,
                                        d->y + pos,
                                        d->w, PG_DOCK_SPLITTER_TEBAL);
                        else
                                gr = pg_buat_kotak(d->x + pos,
                                        d->y,
                                        PG_DOCK_SPLITTER_TEBAL, d->h);
                        pg_isi_permukaan_kotak(dest, gr, gc);
                        /* Grip dot di tengah. */
                        {
                                int cx = gr.x + gr.w/2 - 6;
                                int cy = gr.y + gr.h/2 - 6;
                                int dx, dy;
                                pg_warna_t dot = PG_ABU_GELAP;
                                for (dy = 0; dy < 4; dy++)
                                        for (dx = 0; dx < 4; dx++)
                                                pg_isi_permukaan_kotak(dest,
                                                        pg_buat_kotak(
                                                                cx+dx*3,
                                                                cy+dy*3,
                                                                1, 1), dot);
                        }
                }
        }

        /* Outline dock (sisi yang menghadap center). */
        {
                pg_warna_t ol = PG_ABU_GELAP;
                switch (d->posisi) {
                case PG_DOCK_POS_ATAS:
                        pg_garis_h_permukaan(dest, d->x,
                                d->y + d->h - 1, d->w, ol);
                        break;
                case PG_DOCK_POS_BAWAH:
                        pg_garis_h_permukaan(dest, d->x,
                                d->y, d->w, ol);
                        break;
                case PG_DOCK_POS_KIRI:
                        pg_garis_v_permukaan(dest, d->x + d->w - 1,
                                d->y, d->h, ol);
                        break;
                case PG_DOCK_POS_KANAN:
                        pg_garis_v_permukaan(dest, d->x,
                                d->y, d->h, ol);
                        break;
                default:
                        break;
                }
        }
}

void pg_dock_catat_hint(pg_dock_t *d, pg_permukaan_t *dest,
                          int mx, int my, pg_panel_flag_t flag_drag)
{
        pg_kotak_t hint;
        pg_warna_t hint_fill;
        pg_warna_t hint_outline;
        if (!d || !dest) return;
        /* Cek: flag cocok + mouse di dalam area dock. */
        if ((int)flag_drag != (int)d->flag) return;
        if (!pg_dock_berisi(d, mx, my)) return;
        /* Hint = outline area dock (seluruh area). */
        hint = pg_buat_kotak(d->x, d->y, d->w, d->h);
        /* Fill semi-transparan (simulasi alpha). */
        hint_fill = PG_RGB(100, 150, 255);
        hint_outline = PG_BIRU;
        pg_isi_permukaan_kotak(dest, hint, hint_fill);
        pg_gambar_kotak_aa(dest, hint, hint_outline);
}

/* ===== Event handler ===== */

pg_bool pg_dock_tangani(pg_dock_t *d, const pg_peristiwa_t *e)
{
        if (!d) return PG_SALAH;

        /* 1. Splitter drag aktif. */
        if (d->split_drag_idx >= 0 && d->ukuran_panels) {
                if (e->tipe == PG_PERISTIWA_TETIK_GERAK) {
                        int cur = pg_dock_vertikal(d) ?
                                e->tetik_pos.y : e->tetik_pos.x;
                        int delta = cur - d->split_drag_start;
                        int idx = d->split_drag_idx;
                        int avail, total_used;
                        /* Hitung total avail (selain splitter). */
                        avail = (pg_dock_vertikal(d) ? d->h : d->w) -
                                (d->n_panels - 1) * PG_DOCK_SPLITTER_TEBAL;
                        if (avail < 0) avail = 0;
                        total_used = 0;
                        {
                                int i;
                                for (i = 0; i < d->n_panels; i++)
                                        total_used += d->ukuran_panels[i];
                        }
                        /* Aplikasikan delta ke panel idx dan idx+1. */
                        {
                                int a = d->ukuran_panels[idx];
                                int b = d->ukuran_panels[idx+1];
                                int new_a = a + delta;
                                int new_b = b - delta;
                                /* Clamp: min 24. */
                                if (new_a < PG_DOCK_MIN_PANEL) {
                                        new_a = PG_DOCK_MIN_PANEL;
                                        new_b = a + b - new_a;
                                }
                                if (new_b < PG_DOCK_MIN_PANEL) {
                                        new_b = PG_DOCK_MIN_PANEL;
                                        new_a = a + b - new_b;
                                }
                                d->ukuran_panels[idx] = new_a;
                                d->ukuran_panels[idx+1] = new_b;
                        }
                        d->split_drag_start = cur;
                        pg_dock_tata(d);
                        return PG_BENAR;
                }
                if (e->tipe == PG_PERISTIWA_TETIK_NAIK) {
                        d->split_drag_idx = -1;
                        return PG_BENAR;
                }
                return PG_BENAR;
        }

        /* 2. Klik di splitter. */
        if (e->tipe == PG_PERISTIWA_TETIK_TURUN &&
            e->tetik_tombol == PG_TETIK_KIRI) {
                int idx = pg_dock_splitter_hit(d,
                        e->tetik_pos.x, e->tetik_pos.y);
                if (idx >= 0) {
                        d->split_drag_idx = idx;
                        d->split_drag_start = pg_dock_vertikal(d) ?
                                e->tetik_pos.y : e->tetik_pos.x;
                        return PG_BENAR;
                }
        }

        return PG_SALAH;
}
