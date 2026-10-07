/* -------------------------------------------------------------------------- *
 * demo/aplikasi.c - Demo aplikasi desktop lengkap (Pigura v0.14.0)
 * -------------------------------------------------------------------------- *
 * Simulasi aplikasi desktop production-grade dengan struktur dock/panel
 * baru v0.14.0. Menampilkan:
 *   - Menubar 4 menu (Berkas/Sunting/Tampilkan/Bantuan)
 *   - Ribbon dock atas (TOOLBAR, horizontal) 3 panel: Beranda/Sisip/Tampilan
 *   - Toolbox dock kiri (TOOLBAR, vertikal) 1 panel: 6 ikon tool
 *   - Properties dock kanan (PROPERTIES, vertikal) 3 panel: Layers/Colors/Props
 *   - Kanvas floating center (gulir 800x600 dengan 10 label dummy)
 *   - Palette floating (toggle dari menu Tampilkan)
 *   - About dialog modal
 *   - Context menu (klik kanan di kanvas)
 *   - Status bar 3 seksi
 *
 * Layout:
 *   +-----------------------------------------------+
 *   | Menubar (22px)                                |
 *   +-----------------------------------------------+
 *   | Ribbon dock (50px)                            |
 *   +----+----------------------------+-------------+
 *   |Tool|                            | Properties  |
 *   |box |     Kanvas (floating)      | dock kanan  |
 *   |    |                            | (Layers/    |
 *   |    |                            |  Colors/    |
 *   |    |                            |  Props)     |
 *   +----+----------------------------+-------------+
 *   | Status bar (22px)                             |
 *   +-----------------------------------------------+
 * -------------------------------------------------------------------------- */
#include "pigura/pigura.h"
#include "pigura/widget.h"
#include "pigura/kotak.h"
#include "pigura/label.h"
#include "pigura/tombol.h"
#include "pigura/toolbar.h"
#include "pigura/panel.h"
#include "pigura/dock.h"
#include "pigura/menubar.h"
#include "pigura/status_bar.h"
#include "pigura/gulir.h"
#include "pigura/tab.h"
#include "pigura/kolaps.h"
#include "pigura/spinbutton.h"
#include "pigura/colorpicker.h"
#include "pigura/tableview.h"
#include "pigura/dialog_modal.h"
#include "pigura/contextmenu.h"
#include "pigura/tooltip.h"
#include "pigura/permukaan.h"
#include "pigura/aksi.h"
#include "pigura/layar.h"
#include "pigura/masukan.h"
#include "pigura/perulangan.h"
#include "pigura/papan_klip.h"
#include "pigura/font.h"
#include "pigura/gambar.h"
#include "ikon_xpm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W_AWAL    1200
#define H_AWAL    700
#define MENUBAR_H 22
#define STATUS_H  22
#define RIBBON_H  50
#define TOOLBOX_W 32
#define PROPS_W   240

/* ===== Struktur aplikasi ===== */

typedef struct {
        pg_layar_t        *layar;
        pg_masukan_t      *masukan;
        pg_perulangan_t   *loop;
        pg_font_t         *font;

        /* Menubar + statusbar */
        pg_menubar_t      *mbar;
        pg_status_bar_t   *status;

        /* Docks */
        pg_dock_t         *dock_ribbon;   /* TOOLBAR atas, horizontal */
        pg_dock_t         *dock_toolbox;  /* TOOLBAR kiri, vertikal */
        pg_dock_t         *dock_props;    /* PROPERTIES kanan, vertikal */

        /* Ribbon panels (3) */
        pg_panel_t        *panel_beranda;
        pg_panel_t        *panel_sisip;
        pg_panel_t        *panel_tampilan;
        /* Toolbox panel (1) */
        pg_panel_t        *panel_toolbox;
        /* Properties panels (3) */
        pg_panel_t        *panel_layers;
        pg_panel_t        *panel_colors;
        pg_panel_t        *panel_props;
        /* Kanvas + Palette (2 floating) */
        pg_panel_t        *panel_kanvas;
        pg_panel_t        *panel_palette;

        /* Konten ribbon */
        pg_toolbar_t      *tb_beranda;
        pg_toolbar_t      *tb_sisip;
        pg_toolbar_t      *tb_tampilan;
        /* Konten toolbox */
        pg_toolbar_t      *tb_toolbox;
        /* Konten kanvas */
        pg_gulir_t        *kanvas;
        /* Konten properties */
        pg_tableview_t    *layers_tv;
        pg_colorpicker_t  *colors_cp;
        pg_kolaps_t       *props_kolaps;
        pg_spinbutton_t   *spin_w;
        pg_spinbutton_t   *spin_h;
        /* Konten palette */
        pg_kotak_widget_t *palette_kotak;

        /* Modal + context menu */
        pg_dialog_modal_t *dialog_about;
        pg_contextmenu_t  *cm_kanvas;

        /* Ikon permukaan */
        pg_permukaan_t    *ik_baru, *ik_buka, *ik_simpan;
        pg_permukaan_t    *ik_urung, *ik_ulang;
        pg_permukaan_t    *ik_perbesar, *ik_perkecil;
        pg_permukaan_t    *ik_tool[6];

        /* State drag untuk hint */
        pg_panel_t        *drag_panel_aktif;
        pg_panel_flag_t   drag_flag_aktif;
        int                drag_mx, drag_my;

        /* State aplikasi */
        int                zoom;
        pg_bool            palette_terlihat;
        pg_bool            grid_terlihat;
        int                tool_aktif;

        char               status_teks[128];
        int                mouse_x, mouse_y;
} app_t;

/* ===== Helper muat ikon XPM ===== */

static pg_permukaan_t *muat_xpm(const char *xpm[])
{
        int w, h, ch;
        pg_byte *px = pg_muat_xpm_dari_string(xpm, &w, &h, &ch);
        pg_permukaan_t *s;
        if (!px) return NULL;
        s = pg_buat_permukaan(w, h);
        if (!s) { free(px); return NULL; }
        {
                int i;
                pg_warna_t *dst = (pg_warna_t *)pg_permukaan_piksel_mut(s);
                for (i = 0; i < w * h; i++) dst[i] = ((pg_warna_t *)px)[i];
        }
        free(px);
        return s;
}

static void muat_ikon(app_t *a)
{
        a->ik_baru      = muat_xpm(ikon_baru);
        a->ik_buka      = muat_xpm(ikon_buka);
        a->ik_simpan    = muat_xpm(ikon_simpan);
        a->ik_urung     = muat_xpm(ikon_urungkan);
        a->ik_ulang     = muat_xpm(ikon_ulangi);
        a->ik_perbesar  = muat_xpm(ikon_perbesar);
        a->ik_perkecil  = muat_xpm(ikon_perkecil);
        a->ik_tool[0]   = muat_xpm(ikon_tool_select);
        a->ik_tool[1]   = muat_xpm(ikon_tool_rect);
        a->ik_tool[2]   = muat_xpm(ikon_tool_circle);
        a->ik_tool[3]   = muat_xpm(ikon_tool_text);
        a->ik_tool[4]   = muat_xpm(ikon_tool_zoom);
        a->ik_tool[5]   = muat_xpm(ikon_tool_hand);
}

/* ===== Helper status ===== */

static void set_status(app_t *a, const char *teks)
{
        snprintf(a->status_teks, sizeof(a->status_teks), "%s", teks);
        pg_sb_setel_teks(a->status, 0, a->status_teks);
}

static void update_zoom_status(app_t *a)
{
        char b[32];
        snprintf(b, sizeof(b), "Zoom: %d%%", a->zoom);
        pg_sb_setel_teks(a->status, 1, b);
}

/* ===== Aksi menu/ribbon ===== */

static void aksi_baru(app_t *a)        { set_status(a, "Dokumen baru dibuat"); }
static void aksi_buka(app_t *a)        { set_status(a, "Membuka file..."); }
static void aksi_simpan(app_t *a)      { set_status(a, "Menyimpan file..."); }
static void aksi_keluar(app_t *a)      { pg_hentikan_perulangan(a->loop); }
static void aksi_urungkan(app_t *a)    { set_status(a, "Urungkan aksi terakhir"); }
static void aksi_ulangi(app_t *a)      { set_status(a, "Ulangi aksi terakhir"); }
static void aksi_potong(app_t *a)      { set_status(a, "Potong ke papan klip"); }
static void aksi_salin(app_t *a)       { set_status(a, "Salin ke papan klip"); }
static void aksi_tempel(app_t *a)      { set_status(a, "Tempel dari papan klip"); }
static void aksi_pilih_semua(app_t *a) { set_status(a, "Pilih semua objek"); }
static void aksi_tentang(app_t *a)     { pg_dialog_modal_tampil(a->dialog_about); }

static void aksi_toggle_palette(app_t *a)
{
        a->palette_terlihat = !a->palette_terlihat;
        if (a->palette_terlihat) {
                pg_panel_tampil(a->panel_palette);
                set_status(a, "Palette ditampilkan");
        } else {
                pg_panel_sembunyi(a->panel_palette);
                set_status(a, "Palette disembunyikan");
        }
}

static void aksi_perbesar(app_t *a)
{
        a->zoom += 10;
        if (a->zoom > 500) a->zoom = 500;
        update_zoom_status(a);
        set_status(a, "Perbesar");
}

static void aksi_perkecil(app_t *a)
{
        a->zoom -= 10;
        if (a->zoom < 10) a->zoom = 10;
        update_zoom_status(a);
        set_status(a, "Perkecil");
}

static void aksi_reset_zoom(app_t *a)
{
        a->zoom = 100;
        update_zoom_status(a);
        set_status(a, "Reset zoom ke 100%");
}

static void aksi_toggle_grid(app_t *a)
{
        a->grid_terlihat = !a->grid_terlihat;
        set_status(a, a->grid_terlihat ? "Grid aktif" : "Grid nonaktif");
}

static void aksi_fit(app_t *a)
{
        a->zoom = 100;
        update_zoom_status(a);
        set_status(a, "Fit ke layar");
}

static void aksi_sisip(app_t *a, const char *jenis)
{
        char b[64];
        snprintf(b, sizeof(b), "Sisip: %s", jenis);
        set_status(a, b);
}

static void aksi_pilih_tool(app_t *a, int idx)
{
        static const char *nama_tool[6] = {
                "Select", "Rectangle", "Circle", "Text", "Zoom", "Hand"
        };
        if (idx < 0 || idx >= 6) return;
        a->tool_aktif = idx;
        {
                char b[64];
                snprintf(b, sizeof(b), "Tool: %s", nama_tool[idx]);
                set_status(a, b);
        }
}

/* ===== Callback menu ===== */

static void cb_menu_berkas(pg_menu_t *m, int idx, void *ctx)
{
        app_t *a = ctx;
        (void)m;
        /* 0=Baru, 1=Buka, 2=Simpan, 3=pemisah, 4=Keluar */
        switch (idx) {
        case 0: aksi_baru(a); break;
        case 1: aksi_buka(a); break;
        case 2: aksi_simpan(a); break;
        case 4: aksi_keluar(a); break;
        default: break;
        }
}

static void cb_menu_sunting(pg_menu_t *m, int idx, void *ctx)
{
        app_t *a = ctx;
        (void)m;
        /* 0=Urungkan, 1=Ulangi, 2=pemisah, 3=Potong, 4=Salin,
         * 5=Tempel, 6=pemisah, 7=Pilih Semua */
        switch (idx) {
        case 0: aksi_urungkan(a); break;
        case 1: aksi_ulangi(a); break;
        case 3: aksi_potong(a); break;
        case 4: aksi_salin(a); break;
        case 5: aksi_tempel(a); break;
        case 7: aksi_pilih_semua(a); break;
        default: break;
        }
}

static void cb_menu_tampilkan(pg_menu_t *m, int idx, void *ctx)
{
        app_t *a = ctx;
        (void)m;
        /* 0=Palette, 1=pemisah, 2=Perbesar, 3=Perkecil, 4=Reset Zoom,
         * 5=pemisah, 6=Grid */
        switch (idx) {
        case 0: aksi_toggle_palette(a); break;
        case 2: aksi_perbesar(a); break;
        case 3: aksi_perkecil(a); break;
        case 4: aksi_reset_zoom(a); break;
        case 6: aksi_toggle_grid(a); break;
        default: break;
        }
}

static void cb_menu_bantuan(pg_menu_t *m, int idx, void *ctx)
{
        app_t *a = ctx;
        (void)m;
        /* 0=Tentang */
        if (idx == 0) aksi_tentang(a);
}

/* ===== Callback toolbar ribbon ===== */

static void cb_tb_beranda(pg_toolbar_t *tb, int id, void *ctx)
{
        app_t *a = ctx;
        (void)tb;
        /* ids: 0=Baru, 1=Buka, 2=Simpan, 3=pemisah, 4=Urungkan,
         *      5=Ulangi, 6=pemisah, 7=Perbesar, 8=Perkecil */
        switch (id) {
        case 0: aksi_baru(a); break;
        case 1: aksi_buka(a); break;
        case 2: aksi_simpan(a); break;
        case 4: aksi_urungkan(a); break;
        case 5: aksi_ulangi(a); break;
        case 7: aksi_perbesar(a); break;
        case 8: aksi_perkecil(a); break;
        default: break;
        }
}

static void cb_tb_sisip(pg_toolbar_t *tb, int id, void *ctx)
{
        app_t *a = ctx;
        (void)tb;
        /* ids: 0=Gambar, 1=Bentuk, 2=Teks */
        switch (id) {
        case 0: aksi_sisip(a, "Gambar"); break;
        case 1: aksi_sisip(a, "Bentuk"); break;
        case 2: aksi_sisip(a, "Teks"); break;
        default: break;
        }
}

static void cb_tb_tampilan(pg_toolbar_t *tb, int id, void *ctx)
{
        app_t *a = ctx;
        (void)tb;
        /* ids: 0=Reset Zoom, 1=Fit, 2=Grid On/Off */
        switch (id) {
        case 0: aksi_reset_zoom(a); break;
        case 1: aksi_fit(a); break;
        case 2: aksi_toggle_grid(a); break;
        default: break;
        }
}

static void cb_tb_toolbox(pg_toolbar_t *tb, int id, void *ctx)
{
        app_t *a = ctx;
        (void)tb;
        /* ids: 0=Select, 1=Rect, 2=Circle, 3=Text, 4=pemisah,
         *      5=Zoom, 6=Hand */
        switch (id) {
        case 0: aksi_pilih_tool(a, 0); break;
        case 1: aksi_pilih_tool(a, 1); break;
        case 2: aksi_pilih_tool(a, 2); break;
        case 3: aksi_pilih_tool(a, 3); break;
        case 5: aksi_pilih_tool(a, 4); break;
        case 6: aksi_pilih_tool(a, 5); break;
        default: break;
        }
}

/* ===== Callback widget lain ===== */

static void cb_colorpicker(pg_colorpicker_t *cp, pg_warna_t w, void *ctx)
{
        app_t *a = ctx;
        char b[64];
        (void)cp;
        snprintf(b, sizeof(b), "Warna dipilih: R%u G%u B%u",
                (unsigned)PG_R(w), (unsigned)PG_G(w), (unsigned)PG_B(w));
        set_status(a, b);
}

static void cb_spin_w(pg_spinbutton_t *sb, int naik, void *ctx)
{
        app_t *a = ctx;
        char b[64];
        (void)sb;
        snprintf(b, sizeof(b), "Width: %s", naik ? "+" : "-");
        set_status(a, b);
}

static void cb_spin_h(pg_spinbutton_t *sb, int naik, void *ctx)
{
        app_t *a = ctx;
        char b[64];
        (void)sb;
        snprintf(b, sizeof(b), "Height: %s", naik ? "+" : "-");
        set_status(a, b);
}

/* ===== Callback context menu ===== */

static void cb_cm_item(pg_contextmenu_t *cm, int idx, void *ctx)
{
        app_t *a = ctx;
        (void)cm;
        /* 0=Salin, 1=Tempel, 2=Hapus, 3=Pilih Semua */
        switch (idx) {
        case 0: aksi_salin(a); break;
        case 1: aksi_tempel(a); break;
        case 2: set_status(a, "Hapus objek terpilih"); break;
        case 3: aksi_pilih_semua(a); break;
        default: break;
        }
}

/* ===== Callback dialog OK ===== */

static void cb_dialog_ok(pg_dialog_modal_t *dm, void *ctx)
{
        app_t *a = ctx;
        (void)a;
        pg_dialog_modal_tutup(dm);
}

/* ===== Callback klik kanan kanvas ===== */

static void cb_kanvas_klik_kanan(pg_widget_t *w, int x, int y, void *ctx)
{
        app_t *a = ctx;
        (void)w; (void)x; (void)y;
        /* Tampilkan context menu di posisi mouse terakhir (global). */
        pg_contextmenu_tampil(a->cm_kanvas, a->mouse_x, a->mouse_y);
}

/* ===== Panel callbacks (shared) ===== */

static pg_dock_t *cari_dock_untuk_panel(app_t *a, pg_panel_t *p,
                                         int mx, int my, int *out_idx)
{
        pg_panel_flag_t flag = pg_panel_flag(p);
        pg_dock_t *candidates[3];
        int n = 0;
        (void)flag;
        /* Panel KANVAS tidak bisa masuk dock manapun — selalu floating. */
        if (flag == PG_PANEL_FLAG_KANVAS) {
                if (out_idx) *out_idx = -1;
                return NULL;
        }
        if (pg_dock_flag(a->dock_ribbon) == (pg_dock_flag_t)flag)
                candidates[n++] = a->dock_ribbon;
        if (pg_dock_flag(a->dock_toolbox) == (pg_dock_flag_t)flag)
                candidates[n++] = a->dock_toolbox;
        if (pg_dock_flag(a->dock_props) == (pg_dock_flag_t)flag)
                candidates[n++] = a->dock_props;
        {
                int i;
                for (i = 0; i < n; i++) {
                        if (pg_dock_berisi(candidates[i], mx, my)) {
                                if (out_idx)
                                        *out_idx = pg_dock_indeks_insert(
                                                candidates[i], mx, my);
                                return candidates[i];
                        }
                }
        }
        if (out_idx) *out_idx = -1;
        return NULL;
}

static void lepas_dari_semua_dock(app_t *a, pg_panel_t *p)
{
        if (pg_dock_punya_panel(a->dock_ribbon, p))
                pg_dock_hapus_panel(a->dock_ribbon, p);
        if (pg_dock_punya_panel(a->dock_toolbox, p))
                pg_dock_hapus_panel(a->dock_toolbox, p);
        if (pg_dock_punya_panel(a->dock_props, p))
                pg_dock_hapus_panel(a->dock_props, p);
}

static int panel_di_dock(app_t *a, pg_panel_t *p)
{
        return (pg_dock_punya_panel(a->dock_ribbon, p) ||
                pg_dock_punya_panel(a->dock_toolbox, p) ||
                pg_dock_punya_panel(a->dock_props, p));
}

static void cb_panel_drag_selesai(pg_panel_t *p, int x, int y, void *ctx)
{
        app_t *a = ctx;
        pg_dock_t *target;
        int idx;
        target = cari_dock_untuk_panel(a, p, x, y, &idx);
        if (target) {
                lepas_dari_semua_dock(a, p);
                pg_dock_tambah_panel(target, p, idx);
                pg_dock_tata(target);
                set_status(a, "Panel di-snap ke dock");
        } else {
                lepas_dari_semua_dock(a, p);
                {
                        char b[64];
                        snprintf(b, sizeof(b),
                                "Panel di floating (%d, %d)", x, y);
                        set_status(a, b);
                }
        }
}

static void cb_panel_semula(pg_panel_t *p, void *ctx)
{
        app_t *a = ctx;
        if (panel_di_dock(a, p)) {
                lepas_dari_semua_dock(a, p);
                set_status(a, "Panel dilepas ke floating");
        } else {
                pg_dock_t *target = NULL;
                pg_panel_flag_t flag = pg_panel_flag(p);
                if (pg_dock_flag(a->dock_ribbon) == (pg_dock_flag_t)flag)
                        target = a->dock_ribbon;
                else if (pg_dock_flag(a->dock_toolbox) == (pg_dock_flag_t)flag)
                        target = a->dock_toolbox;
                else if (pg_dock_flag(a->dock_props) == (pg_dock_flag_t)flag)
                        target = a->dock_props;
                if (target) {
                        pg_dock_tambah_panel(target, p, -1);
                        pg_dock_tata(target);
                        set_status(a, "Panel di-dock kembali");
                }
        }
}

static void cb_panel_tutup(pg_panel_t *p, void *ctx)
{
        app_t *a = ctx;
        lepas_dari_semua_dock(a, p);
        pg_panel_sembunyi(p);
        /* Sinkronkan state palette bila panel_palette yang ditutup. */
        if (p == a->panel_palette) a->palette_terlihat = PG_SALAH;
        set_status(a, "Panel disembunyikan");
}

static void cb_panel_lipat(pg_panel_t *p, pg_bool collapsed, void *ctx)
{
        app_t *a = ctx;
        const char *judul = pg_panel_judul(p);
        char b[64];
        snprintf(b, sizeof(b), "Panel \"%s\" %s",
                judul ? judul : "?", collapsed ? "dilipat" : "dibuka");
        set_status(a, b);
        /* Re-layout dock tempat panel berada, supaya panel
         * lain shift menutup gap yang ditinggalkan. */
        if (pg_dock_punya_panel(a->dock_ribbon, p))
                pg_dock_tata(a->dock_ribbon);
        if (pg_dock_punya_panel(a->dock_toolbox, p))
                pg_dock_tata(a->dock_toolbox);
        if (pg_dock_punya_panel(a->dock_props, p))
                pg_dock_tata(a->dock_props);
}

static void cb_panel_drag_gerak(pg_panel_t *p, int x, int y, void *ctx)
{
        app_t *a = ctx;
        if (x < 0 || y < 0) {
                a->drag_panel_aktif = NULL;
                a->drag_flag_aktif = PG_PANEL_FLAG_TOOLBAR;
        } else {
                a->drag_panel_aktif = p;
                a->drag_flag_aktif = pg_panel_flag(p);
                a->drag_mx = x;
                a->drag_my = y;
        }
}

static void wire_panel_callbacks(app_t *a, pg_panel_t *p)
{
        pg_panel_saat_drag_selesai(p, cb_panel_drag_selesai, a);
        pg_panel_saat_drag_gerak(p, cb_panel_drag_gerak, a);
        pg_panel_saat_semula(p, cb_panel_semula, a);
        pg_panel_saat_tutup(p, cb_panel_tutup, a);
        pg_panel_saat_lipat(p, cb_panel_lipat, a);
}

/* ===== Build UI ===== */

static void build_menubar(app_t *a)
{
        pg_menu_t *m;
        a->mbar = pg_buat_menubar(a->font);
        pg_widget_setel_kotak(pg_menubar_widget(a->mbar),
                pg_buat_kotak(0, 0, W_AWAL, MENUBAR_H));

        /* Berkas: Baru, Buka, Simpan, ---, Keluar */
        m = pg_menubar_tambah_menu(a->mbar, "Berkas");
        pg_menu_tambah_item(m, "Baru",    cb_menu_berkas, a);
        pg_menu_tambah_item(m, "Buka",    cb_menu_berkas, a);
        pg_menu_tambah_item(m, "Simpan",  cb_menu_berkas, a);
        pg_menu_tambah_pemisah(m);
        pg_menu_tambah_item(m, "Keluar",  cb_menu_berkas, a);

        /* Sunting: Urungkan, Ulangi, ---, Potong, Salin, Tempel, ---,
         * Pilih Semua */
        m = pg_menubar_tambah_menu(a->mbar, "Sunting");
        pg_menu_tambah_item(m, "Urungkan",   cb_menu_sunting, a);
        pg_menu_tambah_item(m, "Ulangi",     cb_menu_sunting, a);
        pg_menu_tambah_pemisah(m);
        pg_menu_tambah_item(m, "Potong",     cb_menu_sunting, a);
        pg_menu_tambah_item(m, "Salin",      cb_menu_sunting, a);
        pg_menu_tambah_item(m, "Tempel",     cb_menu_sunting, a);
        pg_menu_tambah_pemisah(m);
        pg_menu_tambah_item(m, "Pilih Semua", cb_menu_sunting, a);

        /* Tampilkan: Palette, ---, Perbesar, Perkecil, Reset Zoom, ---, Grid */
        m = pg_menubar_tambah_menu(a->mbar, "Tampilkan");
        pg_menu_tambah_item(m, "Palette",    cb_menu_tampilkan, a);
        pg_menu_tambah_pemisah(m);
        pg_menu_tambah_item(m, "Perbesar",   cb_menu_tampilkan, a);
        pg_menu_tambah_item(m, "Perkecil",   cb_menu_tampilkan, a);
        pg_menu_tambah_item(m, "Reset Zoom", cb_menu_tampilkan, a);
        pg_menu_tambah_pemisah(m);
        pg_menu_tambah_item(m, "Grid",       cb_menu_tampilkan, a);

        /* Bantuan: Tentang */
        m = pg_menubar_tambah_menu(a->mbar, "Bantuan");
        pg_menu_tambah_item(m, "Tentang",    cb_menu_bantuan, a);
}

static void build_status_bar(app_t *a)
{
        a->status = pg_buat_status_bar(a->font);
        /* Seksi 0: status utama (expand), 1: zoom (80px), 2: mouse (100px) */
        pg_sb_tambah_seksi(a->status, 0, 1);
        pg_sb_tambah_seksi(a->status, 80, 0);
        pg_sb_tambah_seksi(a->status, 100, 0);
        pg_sb_setel_teks(a->status, 0, "Ready");
        pg_sb_setel_teks(a->status, 1, "Zoom: 100%");
        pg_sb_setel_teks(a->status, 2, "Mouse: 0,0");
}

/* Ribbon dock atas: 3 panel berurutan. */
static void build_ribbon(app_t *a)
{
        int id;

        /* Panel "Beranda" — toolbar horizontal ikon. */
        a->tb_beranda = pg_buat_toolbar(PG_SALAH);
        pg_tb_setel_font(a->tb_beranda, a->font);
        id = pg_tb_tambah_tombol_v(a->tb_beranda, "", cb_tb_beranda, a);
        pg_tb_setel_icon(a->tb_beranda, id, a->ik_baru);
        id = pg_tb_tambah_tombol_v(a->tb_beranda, "", cb_tb_beranda, a);
        pg_tb_setel_icon(a->tb_beranda, id, a->ik_buka);
        id = pg_tb_tambah_tombol_v(a->tb_beranda, "", cb_tb_beranda, a);
        pg_tb_setel_icon(a->tb_beranda, id, a->ik_simpan);
        pg_tb_tambah_pemisah(a->tb_beranda);
        id = pg_tb_tambah_tombol_v(a->tb_beranda, "", cb_tb_beranda, a);
        pg_tb_setel_icon(a->tb_beranda, id, a->ik_urung);
        id = pg_tb_tambah_tombol_v(a->tb_beranda, "", cb_tb_beranda, a);
        pg_tb_setel_icon(a->tb_beranda, id, a->ik_ulang);
        pg_tb_tambah_pemisah(a->tb_beranda);
        id = pg_tb_tambah_tombol_v(a->tb_beranda, "", cb_tb_beranda, a);
        pg_tb_setel_icon(a->tb_beranda, id, a->ik_perbesar);
        id = pg_tb_tambah_tombol_v(a->tb_beranda, "", cb_tb_beranda, a);
        pg_tb_setel_icon(a->tb_beranda, id, a->ik_perkecil);

        a->panel_beranda = pg_buat_panel("Beranda",
                PG_PANEL_FLAG_TOOLBAR, 360, RIBBON_H + 4, a->font);
        pg_panel_setel_anak(a->panel_beranda,
                pg_toolbar_widget(a->tb_beranda));

        /* Panel "Sisip" — toolbar horizontal tombol teks. */
        a->tb_sisip = pg_buat_toolbar(PG_SALAH);
        pg_tb_setel_font(a->tb_sisip, a->font);
        pg_tb_tambah_tombol_v(a->tb_sisip, "Gambar", cb_tb_sisip, a);
        pg_tb_tambah_tombol_v(a->tb_sisip, "Bentuk", cb_tb_sisip, a);
        pg_tb_tambah_tombol_v(a->tb_sisip, "Teks",   cb_tb_sisip, a);

        a->panel_sisip = pg_buat_panel("Sisip",
                PG_PANEL_FLAG_TOOLBAR, 220, RIBBON_H + 4, a->font);
        pg_panel_setel_anak(a->panel_sisip,
                pg_toolbar_widget(a->tb_sisip));

        /* Panel "Tampilan" — toolbar horizontal tombol teks. */
        a->tb_tampilan = pg_buat_toolbar(PG_SALAH);
        pg_tb_setel_font(a->tb_tampilan, a->font);
        pg_tb_tambah_tombol_v(a->tb_tampilan, "Reset Zoom", cb_tb_tampilan, a);
        pg_tb_tambah_tombol_v(a->tb_tampilan, "Fit",        cb_tb_tampilan, a);
        pg_tb_tambah_tombol_v(a->tb_tampilan, "Grid On/Off", cb_tb_tampilan, a);

        a->panel_tampilan = pg_buat_panel("Tampilan",
                PG_PANEL_FLAG_TOOLBAR, 280, RIBBON_H + 4, a->font);
        pg_panel_setel_anak(a->panel_tampilan,
                pg_toolbar_widget(a->tb_tampilan));
}

/* Toolbox dock kiri: 1 panel vertikal dengan 6 ikon tool. */
static void build_toolbox(app_t *a)
{
        int id;
        a->tb_toolbox = pg_buat_toolbar(PG_BENAR);
        pg_tb_setel_font(a->tb_toolbox, a->font);
        id = pg_tb_tambah_tombol_v(a->tb_toolbox, "", cb_tb_toolbox, a);
        pg_tb_setel_icon(a->tb_toolbox, id, a->ik_tool[0]);
        id = pg_tb_tambah_tombol_v(a->tb_toolbox, "", cb_tb_toolbox, a);
        pg_tb_setel_icon(a->tb_toolbox, id, a->ik_tool[1]);
        id = pg_tb_tambah_tombol_v(a->tb_toolbox, "", cb_tb_toolbox, a);
        pg_tb_setel_icon(a->tb_toolbox, id, a->ik_tool[2]);
        id = pg_tb_tambah_tombol_v(a->tb_toolbox, "", cb_tb_toolbox, a);
        pg_tb_setel_icon(a->tb_toolbox, id, a->ik_tool[3]);
        pg_tb_tambah_pemisah(a->tb_toolbox);
        id = pg_tb_tambah_tombol_v(a->tb_toolbox, "", cb_tb_toolbox, a);
        pg_tb_setel_icon(a->tb_toolbox, id, a->ik_tool[4]);
        id = pg_tb_tambah_tombol_v(a->tb_toolbox, "", cb_tb_toolbox, a);
        pg_tb_setel_icon(a->tb_toolbox, id, a->ik_tool[5]);

        a->panel_toolbox = pg_buat_panel("Toolbox",
                PG_PANEL_FLAG_TOOLBAR, TOOLBOX_W + 16, 200, a->font);
        pg_panel_setel_anak(a->panel_toolbox,
                pg_toolbar_widget(a->tb_toolbox));
}

/* Properties dock kanan: 3 panel (Layers/Colors/Props). */
static void build_properties(app_t *a)
{
        /* Panel "Layers" — tableview dengan 5 row dummy. */
        {
                const char *judul_kolom[1] = { "Layer" };
                const char *baris[1];
                int i;
                a->layers_tv = pg_buat_tableview(a->font);
                pg_tableview_setel_kolom(a->layers_tv, 1, judul_kolom);
                for (i = 1; i <= 5; i++) {
                        char b[32];
                        snprintf(b, sizeof(b), "Layer %d", i);
                        baris[0] = b;
                        pg_tableview_tambah_baris(a->layers_tv, baris);
                }
                pg_tableview_saatpilih(a->layers_tv, NULL, NULL);
                pg_widget_setel_ukuran_min(
                        pg_tableview_widget(a->layers_tv), PROPS_W - 16, 120);

                a->panel_layers = pg_buat_panel("Layers",
                        PG_PANEL_FLAG_PROPERTIES, PROPS_W, 160, a->font);
                pg_panel_setel_anak(a->panel_layers,
                        pg_tableview_widget(a->layers_tv));
        }

        /* Panel "Colors" — colorpicker + label. */
        {
                pg_kotak_widget_t *body = pg_buat_kotak_widget(
                        PG_KOTAK_VERTIKAL, 4);
                pg_kotak_milik(body, PG_BENAR);
                pg_label_t *lbl = pg_buat_label("Pilih warna:",
                        PG_HITAM, a->font);
                pg_kotak_tambah_lengkap(body, pg_label_widget(lbl),
                        PG_SALAH, PG_SALAH, 0);
                a->colors_cp = pg_buat_colorpicker(a->font);
                pg_widget_setel_ukuran_min(
                        pg_colorpicker_widget(a->colors_cp), PROPS_W - 16, 80);
                pg_colorpicker_saatberubah(a->colors_cp, cb_colorpicker, a);
                pg_kotak_tambah_lengkap(body,
                        pg_colorpicker_widget(a->colors_cp),
                        PG_SALAH, PG_SALAH, 0);

                a->panel_colors = pg_buat_panel("Colors",
                        PG_PANEL_FLAG_PROPERTIES, PROPS_W, 160, a->font);
                pg_panel_setel_anak(a->panel_colors, pg_kotak_widget(body));
        }

        /* Panel "Props" — kolaps dengan 2 spinbutton (W, H) + label. */
        {
                pg_kotak_widget_t *body = pg_buat_kotak_widget(
                        PG_KOTAK_VERTIKAL, 4);
                pg_kotak_milik(body, PG_BENAR);
                pg_label_t *lbl_w = pg_buat_label("Width:", PG_HITAM, a->font);
                pg_kotak_tambah_lengkap(body, pg_label_widget(lbl_w),
                        PG_SALAH, PG_SALAH, 0);
                a->spin_w = pg_buat_spinbutton(a->font);
                pg_spinbutton_saatklik(a->spin_w, cb_spin_w, a);
                pg_widget_setel_ukuran_min(
                        pg_spinbutton_widget(a->spin_w), PROPS_W - 16, 24);
                pg_kotak_tambah_lengkap(body,
                        pg_spinbutton_widget(a->spin_w),
                        PG_SALAH, PG_SALAH, 0);
                pg_label_t *lbl_h = pg_buat_label("Height:", PG_HITAM, a->font);
                pg_kotak_tambah_lengkap(body, pg_label_widget(lbl_h),
                        PG_SALAH, PG_SALAH, 0);
                a->spin_h = pg_buat_spinbutton(a->font);
                pg_spinbutton_saatklik(a->spin_h, cb_spin_h, a);
                pg_widget_setel_ukuran_min(
                        pg_spinbutton_widget(a->spin_h), PROPS_W - 16, 24);
                pg_kotak_tambah_lengkap(body,
                        pg_spinbutton_widget(a->spin_h),
                        PG_SALAH, PG_SALAH, 0);

                a->props_kolaps = pg_buat_kolaps("Ukuran Objek", a->font);
                pg_kolaps_setel_anak(a->props_kolaps, pg_kotak_widget(body));

                a->panel_props = pg_buat_panel("Props",
                        PG_PANEL_FLAG_PROPERTIES, PROPS_W, 180, a->font);
                pg_panel_setel_anak(a->panel_props,
                        pg_kolaps_widget(a->props_kolaps));
        }
}

/* Kanvas floating center: gulir 800x600 dengan 10 label dummy. */
static void build_kanvas(app_t *a)
{
        pg_kotak_widget_t *isi;
        int i;
        a->kanvas = pg_buat_gulir();
        isi = pg_buat_kotak_widget(PG_KOTAK_VERTIKAL, 4);
        pg_kotak_milik(isi, PG_BENAR);
        pg_widget_setel_ukuran_min(pg_kotak_widget(isi), 800, 600);
        pg_widget_setel_latar(pg_kotak_widget(isi), PG_PUTIH);
        for (i = 1; i <= 10; i++) {
                char b[32];
                pg_label_t *l;
                snprintf(b, sizeof(b), "Baris kanvas %d", i);
                l = pg_buat_label(b, PG_HITAM, a->font);
                pg_kotak_tambah_lengkap(isi, pg_label_widget(l),
                        PG_SALAH, PG_SALAH, 0);
        }
        pg_gulir_setel_anak(a->kanvas, pg_kotak_widget(isi), 800, 600);

        /* Wire klik kanan untuk context menu. */
        pg_widget_saat_klik_kanan(pg_gulir_widget(a->kanvas),
                cb_kanvas_klik_kanan, a);

        a->panel_kanvas = pg_buat_panel("Kanvas",
                PG_PANEL_FLAG_KANVAS, 600, 400, a->font);
        pg_panel_setel_anak(a->panel_kanvas,
                pg_gulir_widget(a->kanvas));
        /* Header mode FULL (floating, biru) — default untuk floating. */
        pg_panel_setel_header_mode(a->panel_kanvas, PG_PANEL_HEADER_FULL);
}

/* Palette floating: 8 kotak warna kecil. */
static void build_palette(app_t *a)
{
        static const pg_warna_t palet[8] = {
                PG_MERAH, PG_HIJAU, PG_BIRU, PG_KUNING,
                PG_CYAN, PG_MAGENTA, PG_ABU, PG_PUTIH
        };
        pg_kotak_widget_t *baris_atas, *baris_bawah;
        int i;
        a->palette_kotak = pg_buat_kotak_widget(PG_KOTAK_VERTIKAL, 2);
        pg_kotak_milik(a->palette_kotak, PG_BENAR);
        pg_kotak_setel_padding_kotak(a->palette_kotak, 4);

        baris_atas = pg_buat_kotak_widget(PG_KOTAK_HORIZONTAL, 2);
        pg_kotak_milik(baris_atas, PG_BENAR);
        for (i = 0; i < 4; i++) {
                pg_label_t *sw = pg_buat_label("", PG_PUTIH, a->font);
                pg_widget_setel_latar(pg_label_widget(sw), palet[i]);
                pg_widget_setel_ukuran_min(pg_label_widget(sw), 24, 24);
                pg_kotak_tambah_lengkap(baris_atas,
                        pg_label_widget(sw), PG_SALAH, PG_SALAH, 0);
        }
        pg_kotak_tambah_lengkap(a->palette_kotak,
                pg_kotak_widget(baris_atas), PG_SALAH, PG_SALAH, 0);

        baris_bawah = pg_buat_kotak_widget(PG_KOTAK_HORIZONTAL, 2);
        pg_kotak_milik(baris_bawah, PG_BENAR);
        for (i = 4; i < 8; i++) {
                pg_label_t *sw = pg_buat_label("", PG_PUTIH, a->font);
                pg_widget_setel_latar(pg_label_widget(sw), palet[i]);
                pg_widget_setel_ukuran_min(pg_label_widget(sw), 24, 24);
                pg_kotak_tambah_lengkap(baris_bawah,
                        pg_label_widget(sw), PG_SALAH, PG_SALAH, 0);
        }
        pg_kotak_tambah_lengkap(a->palette_kotak,
                pg_kotak_widget(baris_bawah), PG_SALAH, PG_SALAH, 0);

        a->panel_palette = pg_buat_panel("Palette",
                PG_PANEL_FLAG_TOOLBAR, 120, 80, a->font);
        pg_panel_setel_anak(a->panel_palette,
                pg_kotak_widget(a->palette_kotak));
        pg_panel_setel_header_mode(a->panel_palette, PG_PANEL_HEADER_FULL);
        /* Awalnya tersembunyi. */
        pg_panel_sembunyi(a->panel_palette);
}

/* About dialog modal. */
static void build_dialog_about(app_t *a)
{
        pg_label_t *l;
        a->dialog_about = pg_buat_dialog_modal("Tentang Pigura",
                360, 140, a->font);
        pg_dialog_modal_setel_layar(a->dialog_about, W_AWAL, H_AWAL);
        l = pg_buat_label("Pigura v0.14.0 -- Demo Aplikasi Desktop",
                PG_HITAM, a->font);
        pg_dialog_modal_setel_anak(a->dialog_about, pg_label_widget(l));
        pg_dialog_modal_tambah_tombol(a->dialog_about, "OK",
                cb_dialog_ok, a);
}

/* Context menu kanvas: Salin, Tempel, Hapus, Pilih Semua. */
static void build_context_menu(app_t *a)
{
        a->cm_kanvas = pg_buat_contextmenu(a->font);
        pg_contextmenu_tambah_item(a->cm_kanvas, "Salin",
                cb_cm_item, a);
        pg_contextmenu_tambah_item(a->cm_kanvas, "Tempel",
                cb_cm_item, a);
        pg_contextmenu_tambah_item(a->cm_kanvas, "Hapus",
                cb_cm_item, a);
        pg_contextmenu_tambah_item(a->cm_kanvas, "Pilih Semua",
                cb_cm_item, a);
        /* Awalnya tersembunyi (default). */
        pg_contextmenu_sembunyi(a->cm_kanvas);
}

/* ===== Build all ===== */

static void build_all(app_t *a)
{
        muat_ikon(a);
        build_menubar(a);
        build_status_bar(a);
        build_ribbon(a);
        build_toolbox(a);
        build_properties(a);
        build_kanvas(a);
        build_palette(a);
        build_dialog_about(a);
        build_context_menu(a);

        /* Wire callback untuk semua panel. */
        wire_panel_callbacks(a, a->panel_beranda);
        wire_panel_callbacks(a, a->panel_sisip);
        wire_panel_callbacks(a, a->panel_tampilan);
        wire_panel_callbacks(a, a->panel_toolbox);
        wire_panel_callbacks(a, a->panel_layers);
        wire_panel_callbacks(a, a->panel_colors);
        wire_panel_callbacks(a, a->panel_props);
        wire_panel_callbacks(a, a->panel_kanvas);
        wire_panel_callbacks(a, a->panel_palette);

        /* Dock TOOLBAR atas (horizontal): ribbon 3 panel. */
        a->dock_ribbon = pg_buat_dock(PG_DOCK_FLAG_TOOLBAR,
                PG_DOCK_POS_ATAS);
        pg_dock_tambah_panel(a->dock_ribbon, a->panel_beranda, -1);
        pg_dock_tambah_panel(a->dock_ribbon, a->panel_sisip, -1);
        pg_dock_tambah_panel(a->dock_ribbon, a->panel_tampilan, -1);

        /* Dock TOOLBAR kiri (vertikal): toolbox. */
        a->dock_toolbox = pg_buat_dock(PG_DOCK_FLAG_TOOLBAR,
                PG_DOCK_POS_KIRI);
        pg_dock_tambah_panel(a->dock_toolbox, a->panel_toolbox, -1);

        /* Dock PROPERTIES kanan (vertikal): layers + colors + props. */
        a->dock_props = pg_buat_dock(PG_DOCK_FLAG_PROPERTIES,
                PG_DOCK_POS_KANAN);
        pg_dock_tambah_panel(a->dock_props, a->panel_layers, -1);
        pg_dock_tambah_panel(a->dock_props, a->panel_colors, -1);
        pg_dock_tambah_panel(a->dock_props, a->panel_props, -1);
}

/* ===== Layout saat resize ===== */

static void update_layout(app_t *a, int lebar, int tinggi)
{
        int dock_h = tinggi - MENUBAR_H - STATUS_H;
        int kiri_w = TOOLBOX_W + 16;
        int kanan_w = PROPS_W;
        int atas_h = RIBBON_H;
        int center_x = kiri_w;
        int center_y = MENUBAR_H + atas_h;
        int center_w = lebar - kiri_w - kanan_w;
        int center_h = dock_h - atas_h;

        if (a->mbar)
                pg_widget_setel_kotak(pg_menubar_widget(a->mbar),
                        pg_buat_kotak(0, 0, lebar, MENUBAR_H));

        if (a->status)
                pg_widget_setel_kotak(pg_status_bar_widget(a->status),
                        pg_buat_kotak(0, tinggi - STATUS_H, lebar, STATUS_H));

        /* Dock ribbon: full lebar di bawah menubar. */
        pg_dock_setel_area(a->dock_ribbon, 0, MENUBAR_H, lebar, atas_h);
        pg_dock_tata(a->dock_ribbon);

        /* Dock toolbox: vertikal di kiri. */
        pg_dock_setel_area(a->dock_toolbox, 0, MENUBAR_H + atas_h,
                kiri_w, dock_h - atas_h);
        pg_dock_tata(a->dock_toolbox);

        /* Dock props: vertikal di kanan. */
        pg_dock_setel_area(a->dock_props, lebar - kanan_w,
                MENUBAR_H + atas_h, kanan_w, dock_h - atas_h);
        pg_dock_tata(a->dock_props);

        /* Kanvas floating di area tengah. */
        if (center_w > 100 && center_h > 100) {
                pg_panel_setel_ukuran(a->panel_kanvas,
                        center_w - 20, center_h - 20);
                pg_panel_pindah(a->panel_kanvas,
                        center_x + 10, center_y + 10);
        }

        /* Palette floating di pojok kanan atas kanvas (jika terlihat). */
        if (a->palette_terlihat) {
                int px = center_x + center_w - 140;
                int py = center_y + 10;
                if (px < center_x + 10) px = center_x + 10;
                pg_panel_pindah(a->panel_palette, px, py);
        }

        /* Update dialog modal screen size. */
        pg_dialog_modal_setel_layar(a->dialog_about, lebar, tinggi);
}

/* ===== Render ===== */

static void catat_all(app_t *a, pg_permukaan_t *s)
{
        /* Menubar. */
        if (a->mbar)
                pg_widget_catat(pg_menubar_widget(a->mbar), s);

        /* Dock latar + outline + splitter. */
        pg_dock_catat(a->dock_ribbon, s);
        pg_dock_catat(a->dock_toolbox, s);
        pg_dock_catat(a->dock_props, s);

        /* Hint outline biru untuk dock yang akan menerima panel
         * yang sedang di-drag. */
        if (a->drag_panel_aktif) {
                pg_dock_catat_hint(a->dock_ribbon, s,
                        a->drag_mx, a->drag_my, a->drag_flag_aktif);
                pg_dock_catat_hint(a->dock_toolbox, s,
                        a->drag_mx, a->drag_my, a->drag_flag_aktif);
                pg_dock_catat_hint(a->dock_props, s,
                        a->drag_mx, a->drag_my, a->drag_flag_aktif);
        }

        /* Render semua panel (panel punya catat sendiri). */
        pg_panel_catat(a->panel_beranda, s);
        pg_panel_catat(a->panel_sisip, s);
        pg_panel_catat(a->panel_tampilan, s);
        pg_panel_catat(a->panel_toolbox, s);
        pg_panel_catat(a->panel_layers, s);
        pg_panel_catat(a->panel_colors, s);
        pg_panel_catat(a->panel_props, s);
        pg_panel_catat(a->panel_kanvas, s);
        if (a->palette_terlihat)
                pg_panel_catat(a->panel_palette, s);

        /* Status bar. */
        if (a->status)
                pg_widget_catat(pg_status_bar_widget(a->status), s);

        /* Menubar popup (di atas semua). */
        if (a->mbar)
                pg_widget_catat_popup(pg_menubar_widget(a->mbar), s);

        /* Modal dialog (di atas semua widget biasa). */
        if (pg_dialog_modal_aktif(a->dialog_about))
                pg_dialog_modal_catat(a->dialog_about, s,
                        pg_permukaan_lebar(s), pg_permukaan_tinggi(s));

        /* Context menu (di atas modal). */
        if (pg_contextmenu_terlihat(a->cm_kanvas))
                pg_widget_catat(pg_contextmenu_widget(a->cm_kanvas), s);

        /* Tooltip (paling atas). */
        pg_tooltip_catat(s, a->mouse_x, a->mouse_y);
}

/* ===== Event handler ===== */

static void on_event(const pg_aksi_t *e, void *ctx)
{
        app_t *a = ctx;
        pg_bool consumed = PG_SALAH;

        /* Tombol close window (X) — X11 emit PG_AKSI_KELUAR. */
        if (e->tipe == PG_AKSI_KELUAR) {
                pg_hentikan_perulangan(a->loop);
                return;
        }
        /* Handle window events (resize, close). */
        if (e->tipe == PG_AKSI_JENDELA) {
                if (e->jendela_aksi == PG_JENDELA_UBAH_UKURAN) {
                        /* Window di-resize/maximize: query layar
                         * untuk ukuran baru, lalu update layout. */
                        pg_layar_info_t info;
                        if (pg_layar_kueri(a->layar, &info) == PG_OK) {
                                update_layout(a, info.lebar, info.tinggi);
                        }
                } else if (e->jendela_aksi == PG_JENDELA_TUTUP) {
                        /* Tombol close window (X) ditekan. */
                        pg_hentikan_perulangan(a->loop);
                }
                return;
        }

        /* Track mouse pos untuk status + context menu positioning. */
        if (e->tipe == PG_AKSI_TETIKUS_GERAK ||
            e->tipe == PG_AKSI_TETIKUS_TEKAN ||
            e->tipe == PG_AKSI_TETIKUS_LEPAS) {
                a->mouse_x = e->tetik_pos.x;
                a->mouse_y = e->tetik_pos.y;
                if (e->tipe == PG_AKSI_TETIKUS_GERAK) {
                        char b[64];
                        snprintf(b, sizeof(b), "Mouse: %d, %d",
                                a->mouse_x, a->mouse_y);
                        pg_sb_setel_teks(a->status, 2, b);
                }
        }

        /* ESC: tutup modal/context menu, atau keluar. */
        if (e->tipe == PG_AKSI_TOMBOL_TURUN &&
            e->tombol == PG_TOMBOL_ESCAPE) {
                if (pg_dialog_modal_aktif(a->dialog_about)) {
                        pg_dialog_modal_tutup(a->dialog_about);
                } else if (pg_contextmenu_terlihat(a->cm_kanvas)) {
                        pg_contextmenu_sembunyi(a->cm_kanvas);
                } else {
                        pg_hentikan_perulangan(a->loop);
                }
                return;
        }

        /* Tooltip tracking. */
        pg_tooltip_tangani(e);

        /* Modal dialog (jika aktif) memblokir semua input lain. */
        if (pg_dialog_modal_aktif(a->dialog_about)) {
                pg_dialog_modal_tangani(a->dialog_about, e);
                return;
        }

        /* Context menu (jika terlihat) — prioritas berikutnya. */
        if (pg_contextmenu_terlihat(a->cm_kanvas)) {
                consumed = pg_widget_tangani_aksi(
                        pg_contextmenu_widget(a->cm_kanvas), e);
                if (consumed) return;
                /* Klik di luar context menu: sembunyikan. */
                if (e->tipe == PG_AKSI_TETIKUS_TEKAN) {
                        pg_contextmenu_sembunyi(a->cm_kanvas);
                        /* Jangan consume — biarkan event ke widget lain. */
                }
        }

        /* Menubar. */
        if (!consumed && a->mbar)
                consumed = pg_widget_tangani_aksi(
                        pg_menubar_widget(a->mbar), e);

        /* Panel (z-order: palette > kanvas > props > layers > toolbox > ribbon). */
        if (!consumed && a->palette_terlihat)
                consumed = pg_panel_tangani(a->panel_palette, e);
        if (!consumed)
                consumed = pg_panel_tangani(a->panel_kanvas, e);
        if (!consumed)
                consumed = pg_panel_tangani(a->panel_props, e);
        if (!consumed)
                consumed = pg_panel_tangani(a->panel_colors, e);
        if (!consumed)
                consumed = pg_panel_tangani(a->panel_layers, e);
        if (!consumed)
                consumed = pg_panel_tangani(a->panel_toolbox, e);
        if (!consumed)
                consumed = pg_panel_tangani(a->panel_tampilan, e);
        if (!consumed)
                consumed = pg_panel_tangani(a->panel_sisip, e);
        if (!consumed)
                consumed = pg_panel_tangani(a->panel_beranda, e);

        /* Dock splitter. */
        if (!consumed && pg_dock_tangani(a->dock_ribbon, e))
                consumed = PG_BENAR;
        if (!consumed && pg_dock_tangani(a->dock_toolbox, e))
                consumed = PG_BENAR;
        if (!consumed && pg_dock_tangani(a->dock_props, e))
                consumed = PG_BENAR;

        /* Status bar. */
        if (!consumed && a->status)
                consumed = pg_widget_tangani_aksi(
                        pg_status_bar_widget(a->status), e);
}

/* ===== Idle render ===== */

static void idle(void *ctx)
{
        app_t *a = ctx;
        pg_layar_info_t info;
        void *px;
        int langkah;
        pg_permukaan_t *s;
        if (!a->layar) return;
        if (pg_layar_kueri(a->layar, &info) != PG_OK) return;
        if (pg_layar_kunci(a->layar, &px, &langkah) != PG_OK) return;
        s = pg_bungkus_permukaan(px, info.lebar, info.tinggi, langkah);
        if (!s) { pg_layar_buka_kunci(a->layar); return; }
        /* Background. */
        pg_isi_permukaan_kotak(s,
                pg_buat_kotak(0, 0, info.lebar, info.tinggi), PG_ABU_GELAP);
        catat_all(a, s);
        pg_layar_buka_kunci(a->layar);
        pg_layar_presentasi(a->layar);
}

/* ===== Main ===== */

int main(int argc, char **argv)
{
        app_t a;
        pg_layar_config_t lcfg;
        pg_masukan_config_t mcfg;
        pg_perulangan_config_t pcfg;
        pg_galat err;
        (void)argc;
        (void)argv;

        memset(&a, 0, sizeof(a));
        snprintf(a.status_teks, sizeof(a.status_teks), "Ready");
        a.zoom = 100;
        a.palette_terlihat = PG_SALAH;
        a.grid_terlihat = PG_SALAH;
        a.tool_aktif = 0;

        pigura_init();

        /* Inisialisasi sistem tooltip global. */
        pg_tooltip_init(NULL);

        /* Load font. */
        a.font = pg_buat_font_ttf(
                "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 13);
        if (!a.font) {
                a.font = pg_buat_font_bitmap();
                if (!a.font) {
                        fprintf(stderr, "Gagal load font\n");
                        pigura_selesai();
                        return 1;
                }
        }
        /* Setel font tooltip. */
        pg_tooltip_setel_font(a.font);

        /* Buka layar. */
        memset(&lcfg, 0, sizeof(lcfg));
        lcfg.lebar = W_AWAL;
        lcfg.tinggi = H_AWAL;
        lcfg.judul = "Pigura v0.14.0 - Demo Aplikasi Desktop";
        err = pg_buka_layar(&a.layar, &lcfg);
        if (err != PG_OK) {
                fprintf(stderr, "pg_buka_layar: %s\n",
                        pg_galat_pesan(pg_galat_terakhir()));
                pg_hancur_font(a.font);
                pigura_selesai();
                return 1;
        }

        /* Buka masukan. */
        memset(&mcfg, 0, sizeof(mcfg));
        err = pg_buka_masukan(&a.masukan, &mcfg, a.layar);
        if (err != PG_OK) {
                fprintf(stderr, "pg_buka_masukan: %s\n",
                        pg_galat_pesan(pg_galat_terakhir()));
                pg_tutup_layar(a.layar);
                pg_hancur_font(a.font);
                pigura_selesai();
                return 1;
        }

        /* Build UI. */
        build_all(&a);
        update_layout(&a, W_AWAL, H_AWAL);

        /* Loop. */
        memset(&pcfg, 0, sizeof(pcfg));
        pcfg.layar = a.layar;
        pcfg.masukan = a.masukan;
        pcfg.idle = idle;
        pcfg.idle_ctx = &a;
        pcfg.pompa_masukan = PG_BENAR;
        err = pg_buat_perulangan(&a.loop, &pcfg);
        if (err != PG_OK) {
                fprintf(stderr, "pg_buat_perulangan: %s\n",
                        pg_galat_pesan(pg_galat_terakhir()));
                pg_tutup_masukan(a.masukan);
                pg_tutup_layar(a.layar);
                pg_hancur_font(a.font);
                pigura_selesai();
                return 1;
        }

        pg_jalankan_perulangan(a.loop, on_event, &a);

        /* ===== Cleanup ===== */
        pg_hancur_perulangan(a.loop);
        pg_tutup_masukan(a.masukan);
        pg_tutup_layar(a.layar);

        /* Hancurkan dock DULU — pg_dock_hancur akan reset header
         * panel (akses panels[]). Bila panel dihancurkan dulu,
         * dock akan use-after-free saat akses panels[]. */
        pg_dock_hancur(a.dock_ribbon);
        pg_dock_hancur(a.dock_toolbox);
        pg_dock_hancur(a.dock_props);

        /* Hancurkan semua panel (akan hancurkan child via milik=BENAR). */
        pg_panel_hancur(a.panel_beranda);
        pg_panel_hancur(a.panel_sisip);
        pg_panel_hancur(a.panel_tampilan);
        pg_panel_hancur(a.panel_toolbox);
        pg_panel_hancur(a.panel_layers);
        pg_panel_hancur(a.panel_colors);
        pg_panel_hancur(a.panel_props);
        pg_panel_hancur(a.panel_kanvas);
        pg_panel_hancur(a.panel_palette);

        /* Menubar + status bar. */
        if (a.mbar) pg_menubar_hancur(a.mbar);
        if (a.status) pg_status_bar_hancur(a.status);

        /* Dialog + context menu. */
        if (a.dialog_about) pg_dialog_modal_hancur(a.dialog_about);
        if (a.cm_kanvas) pg_contextmenu_hancur(a.cm_kanvas);

        /* Ikon permukaan. */
        if (a.ik_baru)     pg_hancur_permukaan(a.ik_baru);
        if (a.ik_buka)     pg_hancur_permukaan(a.ik_buka);
        if (a.ik_simpan)   pg_hancur_permukaan(a.ik_simpan);
        if (a.ik_urung)    pg_hancur_permukaan(a.ik_urung);
        if (a.ik_ulang)    pg_hancur_permukaan(a.ik_ulang);
        if (a.ik_perbesar) pg_hancur_permukaan(a.ik_perbesar);
        if (a.ik_perkecil) pg_hancur_permukaan(a.ik_perkecil);
        {
                int i;
                for (i = 0; i < 6; i++)
                        if (a.ik_tool[i]) pg_hancur_permukaan(a.ik_tool[i]);
        }

        /* Tooltip + font. */
        pg_tooltip_selesai();
        pg_hancur_font(a.font);
        pigura_selesai();
        return 0;
}
