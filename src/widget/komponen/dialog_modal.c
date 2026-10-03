/* ----------------------------------------------------------------------------------------------
 * pigura widget: dialog_modal.c - overlay modal dialog
 * ----------------------------------------------------------------------------------------------
 * Modal dialog memblokir input ke widget lain. Struktur:
 *
 *   +----- overlay gelap (50% hitam, seluruh layar) -----+
 *   |                                                     |
 *   |          +--------------------+                    |
 *   |          | judul           [X]|                    |
 *   |          +--------------------+                    |
 *   |          |                    |                    |
 *   |          |  child widget      |                    |
 *   |          |                    |                    |
 *   |          +--------------------+                    |
 *   |          | [OK]  [Cancel]     |                    |
 *   |          +--------------------+                    |
 *   |                                                     |
 *   +-----------------------------------------------------+
 *
 * Saat aktif, event mouse di luar dialog diabaikan. Hanya event ke
 * tombol/dialog yang diproses. ESC menutup dialog.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/dialog_modal.h"
#include "pigura/tombol.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/peristiwa.h"
#include "pigura/widget.h"

#include <stdlib.h>
#include <string.h>

#define PG_DM_TITLE_H  20
#define PG_DM_CLOSE_W  14
#define PG_DM_CLOSE_H  14
#define PG_DM_TOMBOL_H 26
#define PG_DM_TOMBOL_W 80
#define PG_DM_TOMBOL_PAD 8

typedef struct pg_dm_tombol {
        pg_tombol_t          *tombol;
        void                (*cb)(pg_dialog_modal_t*, void*);
        void                 *ctx;
        int                   id;
        struct pg_dm_tombol  *berikutnya;
} pg_dm_tombol_t;

struct pg_dialog_modal {
        pg_widget_t        base;
        char              *judul;
        pg_widget_t       *anak;
        pg_font_t         *font;
        pg_bool            aktif;
        int                 layar_w;
        int                 layar_h;
        pg_dm_tombol_t    *tombol_head;
        pg_dm_tombol_t    *tombol_tail; /* append: urutan catat = urutan tambah */
        int                 tombol_count;
        int                 tombol_total_w;
        pg_warna_t          judul_fg;
        pg_warna_t          judul_bg;
        pg_warna_t          batas;
        pg_warna_t          overlay;
        pg_bool             ditekan_close;
};

static char *pg_dm_dup(const char *s)
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

static pg_dialog_modal_t *pg_dm_dari(pg_widget_t *w)
{
        return (pg_dialog_modal_t *)w;
}

/* Hitung tinggi body bawah tombol: title_h + child_h + tombol_h. */
static int pg_dm_tombol_h(void)
{
        return PG_DM_TOMBOL_H + PG_DM_TOMBOL_PAD;
}

/* Catat modal ke permukaan sendiri (base.permukaan). */
static void pg_dm_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
        pg_dialog_modal_t *dm = pg_dm_dari(w);
        int                sw, sh, baseline;
        pg_kotak_t         title_r, close_r;
        pg_dm_tombol_t    *tb;
        int                tombol_y, x;
        sw = pg_permukaan_lebar(s);
        sh = pg_permukaan_tinggi(s);
        title_r = pg_buat_kotak(0, 0, sw, PG_DM_TITLE_H);
        close_r = pg_buat_kotak(sw - PG_DM_CLOSE_W - 3, 3,
                                 PG_DM_CLOSE_W, PG_DM_CLOSE_H);
        /* Title bar. */
        pg_isi_permukaan_kotak(s, title_r, dm->judul_bg);
        pg_isi_permukaan_kotak(s, close_r,
                dm->ditekan_close ? PG_RGB(0xb0, 0, 0) : PG_MERAH);
        pg_gambar_garis_aa(s,
                PG_KE_FIXED(close_r.x + 3),
                PG_KE_FIXED(close_r.y + 3),
                PG_KE_FIXED(close_r.x + close_r.w - 3),
                PG_KE_FIXED(close_r.y + close_r.h - 3),
                PG_PUTIH);
        pg_gambar_garis_aa(s,
                PG_KE_FIXED(close_r.x + close_r.w - 3),
                PG_KE_FIXED(close_r.y + 3),
                PG_KE_FIXED(close_r.x + 3),
                PG_KE_FIXED(close_r.y + close_r.h - 3),
                PG_PUTIH);
        if (dm->judul && dm->font) {
                baseline = pg_font_baseline_tengah(dm->font,
                        PG_DM_TITLE_H);
                pg_font_gambar_teks(dm->font, dm->judul, s, 4,
                        baseline, dm->judul_fg);
        }
        /* Body latar. */
        pg_isi_permukaan_kotak(s,
                pg_buat_kotak(0, PG_DM_TITLE_H, sw,
                        sh - PG_DM_TITLE_H), PG_ABU_TERANG);
        /* Border luar. */
        pg_kotak_permukaan(s, pg_buat_kotak(0, 0, sw, sh),
                dm->batas);
        /* Child widget. */
        if (dm->anak) {
                int sx, sy;
                sx = dm->anak->kotak.x;
                sy = dm->anak->kotak.y;
                dm->anak->kotak.x = 0;
                dm->anak->kotak.y = PG_DM_TITLE_H;
                pg_widget_catat(dm->anak, s);
                dm->anak->kotak.x = sx;
                dm->anak->kotak.y = sy;
        }
        /* Tombol: posisi di bawah child, center. */
        tombol_y = sh - pg_dm_tombol_h() - PG_DM_TOMBOL_PAD;
        x = (sw - dm->tombol_total_w) / 2;
        if (x < 0) x = 0;
        for (tb = dm->tombol_head; tb; tb = tb->berikutnya) {
                pg_widget_setel_kotak(
                        pg_tombol_widget(tb->tombol),
                        pg_buat_kotak(x, tombol_y,
                                PG_DM_TOMBOL_W, PG_DM_TOMBOL_H));
                pg_widget_catat(pg_tombol_widget(tb->tombol), s);
                x += PG_DM_TOMBOL_W + PG_DM_TOMBOL_PAD;
        }
}

/* Forward tombol ke child. Bypass pg_widget_tangani_peristiwa
 * sebab koordinat sudah lokal-dialog; pg_widget_tangani_peristiwa
 * akan cek berisi pakai kotak tombol (posisi lokal-dialog) — perlu
 * kirim koordinat lokal-dialog TANPA translasi manual. Vtable
 * tombol sendiri tidak pakai tetik_pos untuk state, jadi aman.
 * Mengembalikan BENAR bila tombol mengonsumsi event. */
static pg_bool pg_dm_ke_tombol(pg_dialog_modal_t *dm,
                                const pg_peristiwa_t *e)
{
        pg_dm_tombol_t *tb;
        for (tb = dm->tombol_head; tb; tb = tb->berikutnya) {
                pg_widget_t *bw = pg_tombol_widget(tb->tombol);
                if (pg_widget_berisi(bw, e->tetik_pos)) {
                        pg_peristiwa_t e2 = *e;
                        e2.tetik_pos.x -= bw->kotak.x;
                        e2.tetik_pos.y -= bw->kotak.y;
                        if (bw->vtable && bw->vtable->peristiwa &&
                            bw->vtable->peristiwa(bw, &e2))
                                return PG_BENAR;
                }
        }
        return PG_SALAH;
}

static pg_bool pg_dm_peristiwa_v(pg_widget_t *w,
                                  const pg_peristiwa_t *e)
{
        pg_dialog_modal_t *dm = pg_dm_dari(w);
        int                sw, close_x, close_y;
        sw = w->kotak.w;
        close_x = sw - PG_DM_CLOSE_W - 3;
        close_y = 3;
        /* ESC: tutup. */
        if (e->tipe == PG_PERISTIWA_TOMBOL_TURUN &&
            e->tombol == PG_TOMBOL_ESCAPE) {
                pg_dialog_modal_tutup(dm);
                return PG_BENAR;
        }
        if (e->tipe == PG_PERISTIWA_TETIK_TURUN &&
            e->tetik_tombol == PG_TETIK_KIRI) {
                int x = e->tetik_pos.x, y = e->tetik_pos.y;
                /* Tombol close? */
                if (x >= close_x && x < close_x + PG_DM_CLOSE_W &&
                    y >= close_y && y < close_y + PG_DM_CLOSE_H) {
                        dm->ditekan_close = PG_BENAR;
                        pg_widget_kotor(w);
                        return PG_BENAR;
                }
                /* Tombol dialog? */
                if (pg_dm_ke_tombol(dm, e))
                        return PG_BENAR;
                /* Title bar? abaikan (tidak draggable). */
                if (y < PG_DM_TITLE_H) return PG_BENAR;
                /* Body: teruskan ke anak. */
                if (dm->anak) {
                        pg_peristiwa_t e2 = *e;
                        e2.tetik_pos.y -= PG_DM_TITLE_H;
                        return pg_widget_tangani_peristiwa(
                                dm->anak, &e2);
                }
                return PG_SALAH;
        }
        if (e->tipe == PG_PERISTIWA_TETIK_NAIK &&
            e->tetik_tombol == PG_TETIK_KIRI) {
                int x = e->tetik_pos.x, y = e->tetik_pos.y;
                if (dm->ditekan_close) {
                        dm->ditekan_close = PG_SALAH;
                        if (x >= close_x && x < close_x + PG_DM_CLOSE_W &&
                            y >= close_y && y < close_y + PG_DM_CLOSE_H) {
                                pg_dialog_modal_tutup(dm);
                        }
                        pg_widget_kotor(w);
                        return PG_BENAR;
                }
                /* Tombol dialog? */
                if (pg_dm_ke_tombol(dm, e))
                        return PG_BENAR;
                if (dm->anak) {
                        pg_peristiwa_t e2 = *e;
                        e2.tetik_pos.y -= PG_DM_TITLE_H;
                        return pg_widget_tangani_peristiwa(
                                dm->anak, &e2);
                }
                return PG_SALAH;
        }
        /* Peristiwa lain (GERAK, RODA, keyboard): coba teruskan ke
         * tombol child atau anak body. */
        if (pg_dm_ke_tombol(dm, e))
                return PG_BENAR;
        if (dm->anak) {
                pg_peristiwa_t e2 = *e;
                e2.tetik_pos.y -= PG_DM_TITLE_H;
                return pg_widget_tangani_peristiwa(dm->anak, &e2);
        }
        return PG_SALAH;
}

static void pg_dm_ubah_ukuran_v(pg_widget_t *w, int w_, int h)
{
        pg_dialog_modal_t *dm = pg_dm_dari(w);
        if (dm->anak && h > PG_DM_TITLE_H + pg_dm_tombol_h() + 4) {
                int body_h = h - PG_DM_TITLE_H - pg_dm_tombol_h() - 4;
                pg_widget_setel_kotak(dm->anak,
                        pg_buat_kotak(0, PG_DM_TITLE_H, w_, body_h));
        }
}

static void pg_dm_hancur_v(pg_widget_t *w)
{
        pg_dialog_modal_t *dm = pg_dm_dari(w);
        pg_dm_tombol_t    *tb = dm->tombol_head;
        /* JANGAN free tombol di sini. Tombol sudah ada di universal
         * anak list lewat pg_widget_tambah_anak(). pg_widget_hancur
         * akan hancurkan bila milik=BENAR. */
        while (tb) {
                pg_dm_tombol_t *n = tb->berikutnya;
                /* Hanya NULL-kan pointer; struktur di-free lewat
                 * universal anak list. */
                tb->tombol = NULL;
                free(tb);
                tb = n;
        }
        dm->tombol_head = NULL;
        dm->tombol_tail = NULL;
        dm->tombol_count = 0;
        if (dm->judul) {
                free(dm->judul);
                dm->judul = NULL;
        }
}

/* Deallocator struktur turunan — dipanggil oleh
 * pg_widget_hancur_penuh() setelah hancur_v. */
static void pg_dm_bebas_v(pg_widget_t *w)
{
        free(w);
}

static const pg_widget_vtable_t pg_dm_vtable = {
        pg_dm_catat_v,
        pg_dm_peristiwa_v,
        pg_dm_ubah_ukuran_v,
        pg_dm_hancur_v,
        NULL,
        NULL,
        pg_dm_bebas_v
};

/* Wrapper callback tombol -> panggil cb dm + tutup otomatis bila
 * cb non-NULL. */
static void pg_dm_tombol_cb(pg_tombol_t *t, void *ctx)
{
        pg_dm_tombol_t *tb = (pg_dm_tombol_t *)ctx;
        (void)t;
        if (!tb) return;
        if (tb->cb) {
                /* Cari dm dari base widget tombol -> induk. */
                pg_widget_t *bw = pg_tombol_widget(tb->tombol);
                if (bw && bw->induk)
                        tb->cb((pg_dialog_modal_t *)bw->induk,
                                tb->ctx);
        }
}

/* ---------------------------------------------------------------- API */

pg_dialog_modal_t *pg_buat_dialog_modal(const char *judul,
    int w, int h, pg_font_t *font)
{
        pg_dialog_modal_t *dm;
        if (w <= 0 || h <= 0) return NULL;
        dm = (pg_dialog_modal_t *)calloc(1, sizeof(*dm));
        if (!dm) return NULL;
        pg_widget_init(&dm->base, PG_WIDGET_JENDELA,
                       &pg_dm_vtable);
        /* Set milik=BENAR agar tombol + child dihancurkan otomatis
         * via universal anak list. */
        pg_widget_milik(&dm->base, PG_BENAR);
        dm->font = font;
        dm->aktif = PG_SALAH;
        dm->ditekan_close = PG_SALAH;
        dm->layar_w = 800;
        dm->layar_h = 600;
        dm->judul_fg = PG_PUTIH;
        dm->judul_bg = PG_BIRU;
        dm->batas = PG_ABU_GELAP;
        dm->overlay = PG_RGB(0x00, 0x00, 0x00); /* hitam */
        if (judul) {
                dm->judul = pg_dm_dup(judul);
                if (!dm->judul) {
                        free(dm);
                        return NULL;
                }
        }
        pg_widget_setel_kotak(&dm->base, pg_buat_kotak(0, 0, w, h));
        pg_widget_setel_latar(&dm->base, PG_ABU_TERANG);
        pg_widget_setel_ukuran_min(&dm->base, w, h);
        /* Posisi default: tengah layar. */
        pg_widget_pindah(&dm->base,
                (dm->layar_w - w) / 2, (dm->layar_h - h) / 2);
        return dm;
}

void pg_dialog_modal_hancur(pg_dialog_modal_t *dm)
{
        if (!dm) return;
        pg_widget_hancur(&dm->base);
        free(dm);
}

void pg_dialog_modal_setel_anak(pg_dialog_modal_t *dm,
    pg_widget_t *child)
{
        int bw, bh;
        if (!dm) return;
        dm->anak = child;
        bw = dm->base.kotak.w;
        bh = dm->base.kotak.h - PG_DM_TITLE_H - pg_dm_tombol_h() - 4;
        if (bh < 0) bh = 0;
        if (child) {
                pg_widget_setel_kotak(child,
                        pg_buat_kotak(0, PG_DM_TITLE_H, bw, bh));
                /* Sinkronkan universal anak list. */
                pg_widget_tambah_anak(&dm->base, child);
        }
        pg_widget_kotor(&dm->base);
}

int pg_dialog_modal_tambah_tombol(pg_dialog_modal_t *dm,
    const char *label,
    void (*cb)(pg_dialog_modal_t*, void*), void *ctx)
{
        pg_dm_tombol_t *tb;
        int id;
        if (!dm) return -1;
        tb = (pg_dm_tombol_t *)calloc(1, sizeof(*tb));
        if (!tb) return -1;
        tb->tombol = pg_buat_tombol(label, dm->font);
        if (!tb->tombol) {
                free(tb);
                return -1;
        }
        tb->cb = cb;
        tb->ctx = ctx;
        id = dm->tombol_count + 1;
        tb->id = id;
        dm->tombol_count++;
        dm->tombol_total_w += PG_DM_TOMBOL_W + PG_DM_TOMBOL_PAD;
        /* Append ke tail supaya urutan catat (head→tail) = urutan
         * tambah tombol. */
        tb->berikutnya = NULL;
        if (!dm->tombol_head) {
                dm->tombol_head = tb;
                dm->tombol_tail = tb;
        } else {
                dm->tombol_tail->berikutnya = tb;
                dm->tombol_tail = tb;
        }
        pg_widget_tambah_anak(&dm->base,
                pg_tombol_widget(tb->tombol));
        pg_tombol_saatklik(tb->tombol, pg_dm_tombol_cb, tb);
        pg_widget_kotor(&dm->base);
        return id;
}

void pg_dialog_modal_tampil(pg_dialog_modal_t *dm)
{
        if (!dm) return;
        dm->aktif = PG_BENAR;
        /* Posisi tengah layar. */
        pg_widget_pindah(&dm->base,
                (dm->layar_w - dm->base.kotak.w) / 2,
                (dm->layar_h - dm->base.kotak.h) / 2);
        pg_widget_kotor(&dm->base);
}

void pg_dialog_modal_tutup(pg_dialog_modal_t *dm)
{
        if (!dm) return;
        dm->aktif = PG_SALAH;
        dm->ditekan_close = PG_SALAH;
}

pg_bool pg_dialog_modal_aktif(const pg_dialog_modal_t *dm)
{
        return dm ? dm->aktif : PG_SALAH;
}

void pg_dialog_modal_catat(pg_dialog_modal_t *dm,
    pg_permukaan_t *dest, int screen_w, int screen_h)
{
        int sw, sh;
        if (!dm || !dest) return;
        if (!dm->aktif) return;
        /* Overlay gelap: isi kotak abu gelap transparan di area
         * dialog (bukan modify seluruh buffer — itu berbahaya
         * kalau ukuran buffer berubah). */
        sw = pg_permukaan_lebar(dest);
        sh = pg_permukaan_tinggi(dest);
        {
                int i;
                pg_warna_t *p = (pg_warna_t *)
                        pg_permukaan_piksel_mut(dest);
                if (p) {
                        int n = sw * sh;
                        for (i = 0; i < n; i++) {
                                pg_warna_t c = p[i];
                                p[i] = PG_RGB(
                                        (pg_u8)(PG_R(c) >> 1),
                                        (pg_u8)(PG_G(c) >> 1),
                                        (pg_u8)(PG_B(c) >> 1));
                        }
                }
        }
        /* Dialog box di atas overlay. */
        pg_widget_catat(&dm->base, dest);
}

pg_bool pg_dialog_modal_tangani(pg_dialog_modal_t *dm,
    const pg_peristiwa_t *e)
{
        pg_peristiwa_t te;
        if (!dm || !e) return PG_SALAH;
        if (!dm->aktif) return PG_SALAH;
        /* Block input: bila event mouse di luar dialog box, consume
         * tanpa teruskan (return BENAR untuk menelan). */
        if (e->tipe == PG_PERISTIWA_TETIK_TURUN ||
            e->tipe == PG_PERISTIWA_TETIK_NAIK ||
            e->tipe == PG_PERISTIWA_TETIK_GERAK ||
            e->tipe == PG_PERISTIWA_TETIK_RODA) {
                if (!pg_widget_berisi(&dm->base, e->tetik_pos)) {
                        /* Di luar dialog: block input. */
                        return PG_BENAR;
                }
        }
        /* Dispatch langsung ke vtable dialog, BYPASS capture logic
         * pg_widget_tangani_peristiwa. Sebab: pg_widget_tangani_peristiwa
         * memakai g_capture statik untuk TETIK_NAIK — capture-nya
         * ter-set ke tombol child saat TETIK_TURUN, sehingga NAIK di
         * level dialog base ditolak. Dengan dispatch langsung ke
         * vtable, pg_dm_peristiwa_v yang iterasi tombol child sendiri
         * yang memanggil pg_widget_tangani_peristiwa(bw, e) — di
         * level button, g_capture cocok dengan bw, klik callback
         * terpicu. Modal aktif selalu consume event. */
        te = *e;
        te.tetik_pos.x -= dm->base.kotak.x;
        te.tetik_pos.y -= dm->base.kotak.y;
        if (dm->base.vtable && dm->base.vtable->peristiwa)
                dm->base.vtable->peristiwa(&dm->base, &te);
        return PG_BENAR;
}

void pg_dialog_modal_pindah(pg_dialog_modal_t *dm, int x, int y)
{
        if (!dm) return;
        pg_widget_pindah(&dm->base, x, y);
}

void pg_dialog_modal_posisi(const pg_dialog_modal_t *dm,
    int *x, int *y)
{
        if (!dm) return;
        if (x) *x = dm->base.kotak.x;
        if (y) *y = dm->base.kotak.y;
}

void pg_dialog_modal_setel_layar(pg_dialog_modal_t *dm,
    int w, int h)
{
        if (!dm) return;
        dm->layar_w = w;
        dm->layar_h = h;
}
