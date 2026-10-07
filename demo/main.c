/* -------------------------------------------------------------------------- *
 * demo/main.c - Pigura Widget Gallery (no scroll, v0.22.1)
 * -------------------------------------------------------------------------- *
 * Demo widget tanpa scroll view. Konten langsung di vertikal kotak.
 * Bila overflow, terpotong — tapi widgetnya kelihatan.
 * -------------------------------------------------------------------------- */
#include "pigura/pigura.h"
#include "pigura/widget.h"
#include "pigura/kotak.h"
#include "pigura/label.h"
#include "pigura/tombol.h"
#include "pigura/gulir.h"
#include "pigura/isian_teks.h"
#include "pigura/kotak_penanda.h"
#include "pigura/tombol_radio.h"
#include "pigura/bilah_geser.h"
#include "pigura/angka_putar.h"
#include "pigura/kotak_gabungan.h"
#include "pigura/kotak_pencarian.h"
#include "pigura/multi_teks.h"
#include "pigura/papan_klip.h"
#include "pigura/font.h"
#include "pigura/permukaan.h"
#include "pigura/layar.h"
#include "pigura/masukan.h"
#include "pigura/perulangan.h"
#include "pigura/aksi.h"
#include "pigura/gambar.h"
#include "pigura/ikon.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W_AWAL 900
#define H_AWAL 700
#define HEADER_H 32
#define SECTION_GAP 12
#define ITEM_GAP 8

typedef struct {
        pg_layar_t      *layar;
        pg_masukan_t    *masukan;
        pg_perulangan_t *loop;
        pg_font_t       *font;
        pg_font_t       *font_ikon;
        pg_kotak_widget_t *root;
        pg_gulir_t       *scroll;
        pg_kotak_widget_t *content;
        pg_papan_klip_t  *klip;
} app_t;

/* ===== Helpers ===== */

static pg_kotak_widget_t *buat_seksi(const char *judul,
                                       const char *deskripsi,
                                       pg_font_t *font)
{
        pg_kotak_widget_t *s = pg_buat_kotak_widget(
                PG_KOTAK_VERTIKAL, 4);
        pg_kotak_milik(s, PG_BENAR);
        pg_kotak_setel_padding_kotak(s, 8);
        pg_kotak_setel_latar(s, PG_PUTIH);
        pg_kotak_setel_batas(s, PG_WARNA_HOVER_OUTLINE);
        pg_widget_setel_radius(pg_kotak_widget(s), 4);
        {
                pg_label_t *jl = pg_buat_label(judul,
                        PG_WARNA_TEKS_TOMBOL, font);
                pg_label_setel_padding(jl, 0);
                pg_kotak_tambah(s, pg_label_widget(jl), PG_SALAH);
        }
        if (deskripsi) {
                pg_label_t *dl = pg_buat_label(deskripsi,
                        PG_ABU_GELAP, font);
                pg_label_setel_padding(dl, 0);
                pg_kotak_tambah(s, pg_label_widget(dl), PG_SALAH);
        }
        return s;
}

static pg_kotak_widget_t *buat_baris(pg_font_t *font)
{
        pg_kotak_widget_t *r = pg_buat_kotak_widget(
                PG_KOTAK_HORIZONTAL, ITEM_GAP);
        pg_kotak_milik(r, PG_BENAR);
        pg_kotak_setel_padding_kotak(r, 4);
        (void)font;
        return r;
}

static pg_kotak_widget_t *buat_item(const char *caption,
                                      pg_widget_t *widget,
                                      pg_font_t *font)
{
        pg_kotak_widget_t *v = pg_buat_kotak_widget(
                PG_KOTAK_VERTIKAL, 2);
        pg_kotak_milik(v, PG_BENAR);
        pg_kotak_setel_padding_kotak(v, 4);
        {
                pg_label_t *c = pg_buat_label(caption,
                        PG_ABU_GELAP, font);
                pg_label_setel_padding(c, 0);
                pg_kotak_tambah(v, pg_label_widget(c), PG_SALAH);
        }
        pg_kotak_tambah(v, widget, PG_SALAH);
        return v;
}

/* ===== Section builders ===== */

static void cb_ikon_tombol(pg_tombol_t *b, void *ctx)
{
        const char *nama = (const char *)ctx;
        (void)b;
        fprintf(stderr, "[ikon tombol] %s ditekan\n", nama);
}

static pg_kotak_widget_t *seksi_ikon(app_t *a)
{
        pg_kotak_widget_t *s;
        pg_kotak_widget_t *r;
        if (!a->font_ikon) return NULL;

        s = pg_buat_kotak_widget(PG_KOTAK_VERTIKAL, 4);
        pg_kotak_milik(s, PG_BENAR);
        pg_kotak_setel_padding_kotak(s, 8);
        pg_kotak_setel_latar(s, PG_PUTIH);
        pg_kotak_setel_batas(s, PG_WARNA_HOVER_OUTLINE);
        pg_widget_setel_radius(pg_kotak_widget(s), 4);
        {
                pg_label_t *jl = pg_buat_label("Ikon Material Design",
                        PG_WARNA_TEKS_TOMBOL, a->font);
                pg_kotak_tambah(s, pg_label_widget(jl), PG_SALAH);
        }
        {
                pg_label_t *dl = pg_buat_label(
                        "Tombol dengan ikon Material Design subset (5.9KB, 65 ikon).",
                        PG_ABU_GELAP, a->font);
                pg_kotak_tambah(s, pg_label_widget(dl), PG_SALAH);
        }

        r = pg_buat_kotak_widget(PG_KOTAK_HORIZONTAL, ITEM_GAP);
        pg_kotak_milik(r, PG_BENAR);
        pg_kotak_setel_padding_kotak(r, 4);

        {
                struct { const char *ikon; const char *nama; } daftar[] = {
                        { PG_IKON_HOME,         "home" },
                        { PG_IKON_SEARCH,       "search" },
                        { PG_IKON_SETTINGS,     "settings" },
                        { PG_IKON_ADD,          "add" },
                        { PG_IKON_DELETE,       "delete" },
                        { PG_IKON_EDIT,         "edit" },
                        { PG_IKON_SAVE,         "save" },
                        { PG_IKON_DOWNLOAD,     "download" },
                        { PG_IKON_UPLOAD,       "upload" },
                        { PG_IKON_REFRESH,      "refresh" },
                        { PG_IKON_FOLDER,       "folder" },
                        { PG_IKON_PERSON,       "person" },
                        { PG_IKON_MENU,         "menu" },
                        { PG_IKON_MORE_VERT,    "more_vert" },
                        { PG_IKON_CLOSE,        "close" },
                        { PG_IKON_CHECK,        "check" },
                        { PG_IKON_INFO,         "info" },
                        { PG_IKON_WARNING,      "warning" },
                };
                int i;
                int n = (int)(sizeof(daftar) / sizeof(daftar[0]));
                for (i = 0; i < n; i++) {
                        pg_tombol_t *b = pg_buat_tombol(daftar[i].ikon,
                                                          a->font_ikon);
                        pg_widget_setel_tooltip(pg_tombol_widget(b),
                                                 daftar[i].nama);
                        pg_tombol_saatklik(b, cb_ikon_tombol,
                                            (void *)daftar[i].nama);
                        pg_kotak_tambah(r, pg_tombol_widget(b), PG_SALAH);
                }
        }
        pg_kotak_tambah(s, pg_kotak_widget(r), PG_SALAH);
        return s;
}

static pg_kotak_widget_t *seksi_tombol(pg_font_t *font)
{
        pg_kotak_widget_t *s = buat_seksi("Tombol",
                "State: idle / terpilih. Mode: teks / ikon / hibrida.",
                font);

        /* Row 1: button states. */
        pg_kotak_widget_t *r1 = buat_baris(font);
        {
                pg_tombol_t *b = pg_buat_tombol("Idle", font);
                pg_kotak_tambah(r1,
                        pg_kotak_widget(buat_item("idle",
                                pg_tombol_widget(b), font)), PG_SALAH);
        }
        {
                pg_tombol_t *b = pg_buat_tombol("Terpilih", font);
                pg_tombol_setel_terpilih(b, PG_BENAR);
                pg_kotak_tambah(r1,
                        pg_kotak_widget(buat_item("terpilih",
                                pg_tombol_widget(b), font)), PG_SALAH);
        }
        {
                pg_tombol_t *b = pg_buat_tombol("Radius 6", font);
                pg_widget_setel_radius(pg_tombol_widget(b), 6);
                pg_kotak_tambah(r1,
                        pg_kotak_widget(buat_item("radius 6",
                                pg_tombol_widget(b), font)), PG_SALAH);
        }
        pg_kotak_tambah(s, pg_kotak_widget(r1), PG_SALAH);

        /* Row 2: icon buttons. */
        pg_kotak_widget_t *r2 = buat_baris(font);
        {
                pg_permukaan_t *icon = pg_buat_permukaan(16, 16);
                if (icon) {
                        pg_isi_permukaan(icon, PG_TRANSPARAN);
                        pg_isi_permukaan_kotak(icon,
                                pg_buat_kotak(3, 3, 10, 10), PG_BIRU);
                }
                {
                        pg_tombol_t *b = pg_buat_tombol(NULL, font);
                        pg_tombol_setel_icon(b, icon);
                        pg_widget_setel_tooltip(pg_tombol_widget(b),
                                "Tombol ikon saja");
                        pg_kotak_tambah(r2,
                                pg_kotak_widget(buat_item("ikon saja",
                                        pg_tombol_widget(b), font)), PG_SALAH);
                }
                {
                        pg_tombol_t *b = pg_buat_tombol("Simpan", font);
                        pg_tombol_setel_icon(b, icon);
                        pg_kotak_tambah(r2,
                                pg_kotak_widget(buat_item("ikon+kiri",
                                        pg_tombol_widget(b), font)), PG_SALAH);
                }
                {
                        pg_tombol_t *b = pg_buat_tombol("Atas", font);
                        pg_tombol_setel_icon(b, icon);
                        pg_tombol_setel_posisi_icon(b,
                                PG_TOMBOL_ICON_ATAS);
                        pg_kotak_tambah(r2,
                                pg_kotak_widget(buat_item("ikon+atas",
                                        pg_tombol_widget(b), font)), PG_SALAH);
                }
        }
        pg_kotak_tambah(s, pg_kotak_widget(r2), PG_SALAH);
        return s;
}

static pg_kotak_widget_t *seksi_label(pg_font_t *font)
{
        pg_kotak_widget_t *s = buat_seksi("Label",
                "Perataan: KIRI / TENGAH / KANAN. Latar, wrap.",
                font);

        pg_kotak_widget_t *r1 = buat_baris(font);
        {
                pg_label_t *l = pg_buat_label("Kiri",
                        PG_WARNA_TEKS_TOMBOL, font);
                pg_label_setel_latar(l, PG_PUTIH);
                pg_label_setel_padding(l, 8);
                pg_kotak_tambah(r1,
                        pg_kotak_widget(buat_item("KIRI",
                                pg_label_widget(l), font)), PG_SALAH);
        }
        {
                pg_label_t *l = pg_buat_label("Tengah",
                        PG_WARNA_TEKS_TOMBOL, font);
                pg_label_setel_perataan(l, PG_LABEL_PERATAAN_TENGAH);
                pg_label_setel_latar(l, PG_PUTIH);
                pg_label_setel_padding(l, 8);
                pg_kotak_tambah(r1,
                        pg_kotak_widget(buat_item("TENGAH",
                                pg_label_widget(l), font)), PG_SALAH);
        }
        {
                pg_label_t *l = pg_buat_label("Kanan",
                        PG_WARNA_TEKS_TOMBOL, font);
                pg_label_setel_perataan(l, PG_LABEL_PERATAAN_KANAN);
                pg_label_setel_latar(l, PG_PUTIH);
                pg_label_setel_padding(l, 8);
                pg_kotak_tambah(r1,
                        pg_kotak_widget(buat_item("KANAN",
                                pg_label_widget(l), font)), PG_SALAH);
        }
        pg_kotak_tambah(s, pg_kotak_widget(r1), PG_SALAH);

        pg_kotak_widget_t *r2 = buat_baris(font);
        {
                pg_label_t *l = pg_buat_label("Badge",
                        PG_PUTIH, font);
                pg_label_setel_latar(l, PG_BIRU);
                pg_label_setel_padding(l, 6);
                pg_kotak_tambah(r2,
                        pg_kotak_widget(buat_item("latar biru",
                                pg_label_widget(l), font)), PG_SALAH);
        }
        {
                pg_label_t *l = pg_buat_label("Peringatan!",
                        PG_PUTIH, font);
                pg_label_setel_latar(l, PG_MERAH);
                pg_label_setel_padding(l, 6);
                pg_kotak_tambah(r2,
                        pg_kotak_widget(buat_item("latar merah",
                                pg_label_widget(l), font)), PG_SALAH);
        }
        pg_kotak_tambah(s, pg_kotak_widget(r2), PG_SALAH);
        return s;
}

static pg_kotak_widget_t *seksi_kotak(pg_font_t *font)
{
        pg_kotak_widget_t *s = buat_seksi("Kotak",
                "Kontainer layout. Homogen, latar, batas, radius.",
                font);

        pg_kotak_widget_t *r1 = buat_baris(font);
        {
                pg_kotak_widget_t *h = pg_buat_kotak_widget(
                        PG_KOTAK_HORIZONTAL, 4);
                pg_kotak_milik(h, PG_BENAR);
                pg_kotak_setel_latar(h, PG_WARNA_PANEL);
                pg_kotak_setel_batas(h, PG_WARNA_HOVER_OUTLINE);
                pg_kotak_setel_padding_kotak(h, 6);
                {
                        int i;
                        for (i = 0; i < 3; i++) {
                                char b[8];
                                pg_tombol_t *t;
                                snprintf(b, sizeof(b), "B%d", i+1);
                                t = pg_buat_tombol(b, font);
                                pg_kotak_tambah(h,
                                        pg_tombol_widget(t), PG_SALAH);
                        }
                }
                pg_kotak_tambah(r1,
                        pg_kotak_widget(buat_item("horizontal",
                                pg_kotak_widget(h), font)), PG_SALAH);
        }
        {
                pg_kotak_widget_t *v = pg_buat_kotak_widget(
                        PG_KOTAK_VERTIKAL, 4);
                pg_kotak_milik(v, PG_BENAR);
                pg_kotak_setel_latar(v, PG_WARNA_PANEL);
                pg_kotak_setel_batas(v, PG_WARNA_HOVER_OUTLINE);
                pg_kotak_setel_padding_kotak(v, 6);
                {
                        int i;
                        for (i = 0; i < 3; i++) {
                                char b[8];
                                pg_tombol_t *t;
                                snprintf(b, sizeof(b), "B%d", i+1);
                                t = pg_buat_tombol(b, font);
                                pg_kotak_tambah(v,
                                        pg_tombol_widget(t), PG_SALAH);
                        }
                }
                pg_kotak_tambah(r1,
                        pg_kotak_widget(buat_item("vertikal",
                                pg_kotak_widget(v), font)), PG_SALAH);
        }
        pg_kotak_tambah(s, pg_kotak_widget(r1), PG_SALAH);

        pg_kotak_widget_t *r2 = buat_baris(font);
        {
                pg_kotak_widget_t *hg = pg_buat_kotak_widget(
                        PG_KOTAK_HORIZONTAL, 2);
                pg_kotak_milik(hg, PG_BENAR);
                pg_kotak_setel_latar(hg, PG_WARNA_PANEL);
                pg_kotak_setel_batas(hg, PG_WARNA_HOVER_OUTLINE);
                pg_kotak_setel_homogen(hg, PG_BENAR);
                pg_kotak_setel_padding_kotak(hg, 4);
                {
                        int i;
                        for (i = 0; i < 4; i++) {
                                char b[8];
                                pg_tombol_t *t;
                                snprintf(b, sizeof(b), "%d", i+1);
                                t = pg_buat_tombol(b, font);
                                pg_kotak_tambah(hg,
                                        pg_tombol_widget(t), PG_SALAH);
                        }
                }
                pg_kotak_tambah(r2,
                        pg_kotak_widget(buat_item("homogen",
                                pg_kotak_widget(hg), font)), PG_SALAH);
        }
        pg_kotak_tambah(s, pg_kotak_widget(r2), PG_SALAH);
        return s;
}

static pg_papan_klip_t *g_klip = NULL;

static pg_kotak_widget_t *seksi_isian_teks(pg_font_t *font)
{
        pg_kotak_widget_t *s = buat_seksi("Isian Teks",
                "Input teks satu baris. Placeholder, cursor blink, "
                "undo/redo, clipboard, Unicode UTF-8.", font);
        {
                pg_kotak_widget_t *r = buat_baris(font);
                {
                        pg_isian_teks_t *it;
                        it = pg_buat_isian_teks("Halo dunia", 64, font);
                        pg_isian_teks_setel_placeholder(it,
                                "Ketik sesuatu...");
                        if (g_klip) pg_isian_teks_setel_papan_klip(it, g_klip);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("default",
                                        pg_isian_teks_widget(it), font)),
                                PG_SALAH);
                }
                {
                        pg_isian_teks_t *it;
                        it = pg_buat_isian_teks(NULL, 64, font);
                        pg_isian_teks_setel_placeholder(it,
                                "Kosong...");
                        if (g_klip) pg_isian_teks_setel_papan_klip(it, g_klip);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("placeholder",
                                        pg_isian_teks_widget(it), font)),
                                PG_SALAH);
                }
                {
                        pg_isian_teks_t *it;
                        it = pg_buat_isian_teks("", 64, font);
                        pg_widget_setel_radius(pg_isian_teks_widget(it), 6);
                        if (g_klip) pg_isian_teks_setel_papan_klip(it, g_klip);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("radius 6",
                                        pg_isian_teks_widget(it), font)),
                                PG_SALAH);
                }
                {
                        pg_isian_teks_t *it;
                        it = pg_buat_isian_teks(
                                "Tidak bisa diubah", 64, font);
                        pg_isian_teks_setel_aktif(it, PG_SALAH);
                        if (g_klip) pg_isian_teks_setel_papan_klip(it, g_klip);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("nonaktif",
                                        pg_isian_teks_widget(it), font)),
                                PG_SALAH);
                }
                pg_kotak_tambah(s, pg_kotak_widget(r), PG_SALAH);
        }
        {
                pg_kotak_widget_t *r = buat_baris(font);
                {
                        pg_isian_teks_t *it;
                        it = pg_buat_isian_teks(
                                "Teks panjang yang melebihi lebar widget",
                                128, font);
                        pg_widget_setel_kotak(
                                pg_isian_teks_widget(it),
                                pg_buat_kotak(0, 0, 200, 30));
                        if (g_klip) pg_isian_teks_setel_papan_klip(it, g_klip);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("horizontal scroll",
                                        pg_isian_teks_widget(it), font)),
                                PG_SALAH);
                }
                pg_kotak_tambah(s, pg_kotak_widget(r), PG_SALAH);
        }
        return s;
}

static pg_kotak_widget_t *seksi_penanda(pg_font_t *font)
{
        pg_kotak_widget_t *s = buat_seksi("Cek",
                "Checkbox. Posisi, tri-state, hover, fokus, keyboard.",
                font);

        /* Row 1: basic + positions. */
        {
                pg_kotak_widget_t *r = buat_baris(font);
                {
                        pg_kotak_penanda_t *c = pg_buat_kotak_penanda("Aktifkan notifikasi", font);
                        pg_kotak_penanda_setel_dicek(c, PG_BENAR);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("dicek",
                                        pg_kotak_penanda_widget(c), font)), PG_SALAH);
                }
                {
                        pg_kotak_penanda_t *c = pg_buat_kotak_penanda("Tidak dicek", font);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("kosong",
                                        pg_kotak_penanda_widget(c), font)), PG_SALAH);
                }
                {
                        pg_kotak_penanda_t *c = pg_buat_kotak_penanda("Indeterminate", font);
                        pg_kotak_penanda_setel_tri(c, PG_TRI_INDETERMINATE);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("tri-state",
                                        pg_kotak_penanda_widget(c), font)), PG_SALAH);
                }
                pg_kotak_tambah(s, pg_kotak_widget(r), PG_SALAH);
        }

        /* Row 2: posisi berbeda. */
        {
                pg_kotak_widget_t *r = buat_baris(font);
                {
                        pg_kotak_penanda_t *c = pg_buat_kotak_penanda("Kanan", font);
                        pg_kotak_penanda_setel_posisi(c, PG_POSISI_KANAN);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("kotak kanan",
                                        pg_kotak_penanda_widget(c), font)), PG_SALAH);
                }
                {
                        pg_kotak_penanda_t *c = pg_buat_kotak_penanda("Besar", font);
                        pg_kotak_penanda_setel_ukuran_kotak(c, 20);
                        pg_kotak_penanda_setel_dicek(c, PG_BENAR);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("ukuran 20",
                                        pg_kotak_penanda_widget(c), font)), PG_SALAH);
                }
                {
                        pg_kotak_penanda_t *c = pg_buat_kotak_penanda("Radius 4", font);
                        pg_widget_setel_radius(pg_kotak_penanda_widget(c), 4);
                        pg_kotak_penanda_setel_dicek(c, PG_BENAR);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("radius 4",
                                        pg_kotak_penanda_widget(c), font)), PG_SALAH);
                }
                pg_kotak_tambah(s, pg_kotak_widget(r), PG_SALAH);
        }

        return s;
}

/* ===== Batch 2 widgets ===== */

/* Callback shared untuk demo. */
static void cb_slider(pg_bilah_geser_t *g, int nilai, void *ctx)
{
        char buf[32];
        pg_label_t *l = (pg_label_t *)ctx;
        (void)g;
        snprintf(buf, sizeof(buf), "Nilai: %d", nilai);
        pg_label_setel_teks(l, buf);
}

static void cb_spinbox(pg_angka_putar_t *sb, int nilai, void *ctx)
{
        char buf[32];
        pg_label_t *l = (pg_label_t *)ctx;
        (void)sb;
        snprintf(buf, sizeof(buf), "Nilai: %d", nilai);
        pg_label_setel_teks(l, buf);
}

static void cb_search(pg_kotak_pencarian_t *kp, const char *t, void *ctx)
{
        char buf[80];
        pg_label_t *l = (pg_label_t *)ctx;
        snprintf(buf, sizeof(buf), "Cari: %s", t);
        pg_label_setel_teks(l, buf);
}

static void cb_combo(pg_kotak_gabungan_t *kg, int idx, void *ctx)
{
        char buf[80];
        pg_label_t *l = (pg_label_t *)ctx;
        snprintf(buf, sizeof(buf), "Idx: %d", idx);
        pg_label_setel_teks(l, buf);
}

static pg_kotak_widget_t *seksi_bilah_geser(pg_font_t *font)
{
        pg_kotak_widget_t *s = buat_seksi("Bilah Geser",
                "Slider integer dengan knob AA. Drag, klik track, atau "
                "panah kiri/kanan untuk ubah nilai.",
                font);
        {
                pg_kotak_widget_t *r = buat_baris(font);
                pg_label_t *lbl;
                pg_bilah_geser_t *g;

                /* Slider default + label nilai. */
                g = pg_buat_bilah_geser(0, 100, 50);
                pg_widget_setel_kotak(pg_bilah_geser_widget(g),
                        pg_buat_kotak(0, 0, 180, 24));
                lbl = pg_buat_label("Nilai: 50", PG_WARNA_TEKS_TOMBOL, font);
                pg_bilah_geser_saatberubah(g, cb_slider, lbl);
                pg_kotak_tambah(r, pg_bilah_geser_widget(g), PG_SALAH);
                pg_kotak_tambah(r, pg_label_widget(lbl), PG_SALAH);

                /* Slider dengan radius 4 (alternatif tema). */
                {
                        pg_bilah_geser_t *g2 = pg_buat_bilah_geser(0, 50, 25);
                        pg_widget_setel_kotak(pg_bilah_geser_widget(g2),
                                pg_buat_kotak(0, 0, 180, 24));
                        pg_widget_setel_radius(pg_bilah_geser_widget(g2), 4);
                        pg_kotak_tambah(r, pg_bilah_geser_widget(g2),
                                        PG_SALAH);
                }

                /* Slider disabled. */
                {
                        pg_bilah_geser_t *g3 = pg_buat_bilah_geser(0, 10, 5);
                        pg_widget_setel_kotak(pg_bilah_geser_widget(g3),
                                pg_buat_kotak(0, 0, 180, 24));
                        pg_bilah_geser_setel_aktif(g3, PG_SALAH);
                        pg_kotak_tambah(r, pg_bilah_geser_widget(g3),
                                        PG_SALAH);
                }
                pg_kotak_tambah(s, pg_kotak_widget(r), PG_SALAH);
        }
        return s;
}

static pg_kotak_widget_t *seksi_angka_putar(pg_font_t *font)
{
        pg_kotak_widget_t *s = buat_seksi("Angka Putar",
                "Spinbox dengan tombol ▲▼, roda mouse, dan keyboard "
                "arrow up/down. Home/End untuk min/maks.",
                font);
        {
                pg_kotak_widget_t *r = buat_baris(font);
                pg_label_t *lbl;
                pg_angka_putar_t *sb;

                sb = pg_buat_angka_putar(0, 100, 42, 1, font);
                pg_widget_setel_kotak(pg_angka_putar_widget(sb),
                        pg_buat_kotak(0, 0, 100, 28));
                lbl = pg_buat_label("Nilai: 42", PG_WARNA_TEKS_TOMBOL, font);
                pg_angka_putar_saatberubah(sb, cb_spinbox, lbl);
                pg_kotak_tambah(r, pg_angka_putar_widget(sb), PG_SALAH);
                pg_kotak_tambah(r, pg_label_widget(lbl), PG_SALAH);

                /* Spinbox dengan langkah besar. */
                {
                        pg_angka_putar_t *sb2 = pg_buat_angka_putar(
                                0, 1000, 500, 50, font);
                        pg_widget_setel_kotak(pg_angka_putar_widget(sb2),
                                pg_buat_kotak(0, 0, 100, 28));
                        pg_kotak_tambah(r, pg_angka_putar_widget(sb2),
                                        PG_SALAH);
                }

                /* Spinbox radius 4 (alternatif tema). */
                {
                        pg_angka_putar_t *sb4 = pg_buat_angka_putar(
                                0, 100, 30, 5, font);
                        pg_widget_setel_kotak(pg_angka_putar_widget(sb4),
                                pg_buat_kotak(0, 0, 100, 28));
                        pg_widget_setel_radius(pg_angka_putar_widget(sb4), 4);
                        pg_kotak_tambah(r, pg_angka_putar_widget(sb4),
                                        PG_SALAH);
                }

                /* Spinbox disabled. */
                {
                        pg_angka_putar_t *sb3 = pg_buat_angka_putar(
                                0, 10, 5, 1, font);
                        pg_widget_setel_kotak(pg_angka_putar_widget(sb3),
                                pg_buat_kotak(0, 0, 100, 28));
                        pg_angka_putar_setel_aktif(sb3, PG_SALAH);
                        pg_kotak_tambah(r, pg_angka_putar_widget(sb3),
                                        PG_SALAH);
                }
                pg_kotak_tambah(s, pg_kotak_widget(r), PG_SALAH);
        }
        return s;
}

static pg_kotak_widget_t *seksi_kotak_pencarian(pg_font_t *font)
{
        pg_kotak_widget_t *s = buat_seksi("Kotak Pencarian",
                "Searchbox dengan placeholder + clear button (✕). "
                "Klik ✕ untuk kosongkan.",
                font);
        {
                pg_kotak_widget_t *r = buat_baris(font);
                pg_label_t *lbl;
                pg_kotak_pencarian_t *kp;

                kp = pg_buat_kotak_pencarian("Cari sesuatu...", 64, font); if (g_klip) pg_kotak_pencarian_setel_papan_klip(kp, g_klip);
                pg_widget_setel_kotak(pg_kotak_pencarian_widget(kp),
                        pg_buat_kotak(0, 0, 240, 28));
                lbl = pg_buat_label("Cari: ", PG_WARNA_TEKS_TOMBOL, font);
                pg_kotak_pencarian_saatberubah(kp, cb_search, lbl);
                pg_kotak_tambah(r, pg_kotak_pencarian_widget(kp), PG_SALAH);
                pg_kotak_tambah(r, pg_label_widget(lbl), PG_SALAH);

                /* Searchbox dengan teks awal + radius 4 (alternatif tema). */
                {
                        pg_kotak_pencarian_t *kp2 = pg_buat_kotak_pencarian(
                                "Cari...", 64, font);
                        pg_widget_setel_kotak(pg_kotak_pencarian_widget(kp2),
                                pg_buat_kotak(0, 0, 240, 28));
                        pg_widget_setel_radius(
                                pg_kotak_pencarian_widget(kp2), 4);
                        if (g_klip) pg_kotak_pencarian_setel_papan_klip(kp2, g_klip);
                        pg_kotak_pencarian_setel_teks(kp2, "halo dunia");
                        pg_kotak_tambah(r, pg_kotak_pencarian_widget(kp2),
                                        PG_SALAH);
                }

                /* Searchbox disabled. */
                {
                        pg_kotak_pencarian_t *kp3 = pg_buat_kotak_pencarian(
                                "Nonaktif", 64, font);
                        pg_widget_setel_kotak(pg_kotak_pencarian_widget(kp3),
                                pg_buat_kotak(0, 0, 240, 28));
                        pg_kotak_pencarian_setel_aktif(kp3, PG_SALAH);
                        pg_kotak_tambah(r, pg_kotak_pencarian_widget(kp3),
                                        PG_SALAH);
                }
                pg_kotak_tambah(s, pg_kotak_widget(r), PG_SALAH);
        }
        return s;
}

static pg_kotak_widget_t *seksi_kotak_gabungan(pg_font_t *font)
{
        pg_kotak_widget_t *s = buat_seksi("Kotak Gabungan",
                "Combobox editable dengan popup dropdown. Klik ▼ atau "
                "panah bawah untuk buka popup.",
                font);
        {
                pg_kotak_widget_t *r = buat_baris(font);
                pg_label_t *lbl;
                pg_kotak_gabungan_t *kg;

                kg = pg_buat_kotak_gabungan(NULL, 64, font);
                pg_widget_setel_kotak(pg_kotak_gabungan_widget(kg),
                        pg_buat_kotak(0, 0, 200, 28));
                pg_kotak_gabungan_tambah_item(kg, "Pilihan A");
                pg_kotak_gabungan_tambah_item(kg, "Pilihan B");
                pg_kotak_gabungan_tambah_item(kg, "Pilihan C");
                pg_kotak_gabungan_tambah_item(kg, "Pilihan D");
                if (g_klip) pg_kotak_gabungan_setel_papan_klip(kg, g_klip);
                pg_kotak_gabungan_setel_terpilih(kg, 0);
                lbl = pg_buat_label("Idx: 0", PG_WARNA_TEKS_TOMBOL, font);
                pg_kotak_gabungan_saatberubah(kg, cb_combo, lbl);
                pg_kotak_tambah(r, pg_kotak_gabungan_widget(kg), PG_SALAH);
                pg_kotak_tambah(r, pg_label_widget(lbl), PG_SALAH);

                /* Combobox dengan radius 6 (alternatif tema). */
                {
                        pg_kotak_gabungan_t *kg2 = pg_buat_kotak_gabungan(
                                NULL, 64, font);
                        pg_widget_setel_kotak(pg_kotak_gabungan_widget(kg2),
                                pg_buat_kotak(0, 0, 200, 28));
                        pg_widget_setel_radius(
                                pg_kotak_gabungan_widget(kg2), 6);
                        pg_kotak_gabungan_tambah_item(kg2, "Merah");
                        pg_kotak_gabungan_tambah_item(kg2, "Hijau");
                        pg_kotak_gabungan_tambah_item(kg2, "Biru");
                        if (g_klip) pg_kotak_gabungan_setel_papan_klip(kg2, g_klip);
                        pg_kotak_gabungan_setel_terpilih(kg2, 0);
                        pg_kotak_tambah(r, pg_kotak_gabungan_widget(kg2),
                                        PG_SALAH);
                }

                /* Combobox disabled. */
                {
                        pg_kotak_gabungan_t *kg3 = pg_buat_kotak_gabungan(
                                NULL, 64, font);
                        pg_widget_setel_kotak(pg_kotak_gabungan_widget(kg3),
                                pg_buat_kotak(0, 0, 200, 28));
                        pg_kotak_gabungan_tambah_item(kg3, "Disabled");
                        pg_kotak_gabungan_setel_aktif(kg3, PG_SALAH);
                        pg_kotak_tambah(r, pg_kotak_gabungan_widget(kg3),
                                        PG_SALAH);
                }
                pg_kotak_tambah(s, pg_kotak_widget(r), PG_SALAH);
        }
        return s;
}

static pg_kotak_widget_t *seksi_multi_teks(pg_font_t *font)
{
        pg_kotak_widget_t *s = buat_seksi("Multi Teks",
                "Multiline text editor. Enter untuk baris baru, "
                "Shift+klik untuk seleksi, Backspace/Delete.",
                font);
        {
                pg_kotak_widget_t *r = buat_baris(font);
                pg_multi_teks_t *mt;

                mt = pg_buat_multi_teks(
                        "Baris pertama\nBaris kedua\nBaris ketiga",
                        512, font);
                pg_widget_setel_kotak(pg_multi_teks_widget(mt),
                        pg_buat_kotak(0, 0, 300, 100));
                pg_multi_teks_setel_bungkus(mt, PG_BENAR);
                if (g_klip) pg_multi_teks_setel_papan_klip(mt, g_klip);
                pg_kotak_tambah(r, pg_multi_teks_widget(mt), PG_SALAH);

                /* Multi_teks dengan radius 4 (alternatif tema). */
                {
                        pg_multi_teks_t *mt_r = pg_buat_multi_teks(
                                "Multi_teks rounded\nBorder halus\nAA konsisten",
                                128, font);
                        pg_widget_setel_kotak(pg_multi_teks_widget(mt_r),
                                pg_buat_kotak(0, 0, 200, 100));
                        if (g_klip) pg_multi_teks_setel_papan_klip(mt_r, g_klip);
                        pg_widget_setel_radius(pg_multi_teks_widget(mt_r), 4);
                        pg_kotak_tambah(r, pg_multi_teks_widget(mt_r),
                                        PG_SALAH);
                }

                /* Multi_teks disabled. */
                {
                        pg_multi_teks_t *mt2 = pg_buat_multi_teks(
                                "Tidak bisa diubah\nkarena disabled",
                                128, font);
                        pg_widget_setel_kotak(pg_multi_teks_widget(mt2),
                                pg_buat_kotak(0, 0, 200, 100));
                        pg_multi_teks_setel_aktif(mt2, PG_SALAH);
                        pg_kotak_tambah(r, pg_multi_teks_widget(mt2),
                                        PG_SALAH);
                }
                pg_kotak_tambah(s, pg_kotak_widget(r), PG_SALAH);
        }
        return s;
}

static pg_kotak_widget_t *seksi_radio(pg_font_t *font)
{
        pg_kotak_widget_t *s = buat_seksi("Radio",
                "Radio button bergrup. Pilihan eksklusif, arrow navigation.",
                font);

        /* Row 1: group dengan 3 pilihan. */
        {
                pg_kotak_widget_t *r = buat_baris(font);
                pg_tombol_radio_grup_t *g = pg_buat_tombol_radio_grup();
                {
                        pg_tombol_radio_t *r1 = pg_buat_tombol_radio(g, "Pilihan A", font);
                        pg_tombol_radio_pilih(r1);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("A (terpilih)",
                                        pg_tombol_radio_widget(r1), font)), PG_SALAH);
                }
                {
                        pg_tombol_radio_t *r2 = pg_buat_tombol_radio(g, "Pilihan B", font);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("B",
                                        pg_tombol_radio_widget(r2), font)), PG_SALAH);
                }
                {
                        pg_tombol_radio_t *r3 = pg_buat_tombol_radio(g, "Pilihan C", font);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("C",
                                        pg_tombol_radio_widget(r3), font)), PG_SALAH);
                }
                pg_kotak_tambah(s, pg_kotak_widget(r), PG_SALAH);
        }

        /* Row 2: posisi + ukuran. */
        {
                pg_kotak_widget_t *r = buat_baris(font);
                pg_tombol_radio_grup_t *g2 = pg_buat_tombol_radio_grup();
                {
                        pg_tombol_radio_t *r1 = pg_buat_tombol_radio(g2, "Kanan", font);
                        pg_tombol_radio_setel_posisi(r1, PG_POSISI_KANAN);
                        pg_tombol_radio_pilih(r1);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("posisi kanan",
                                        pg_tombol_radio_widget(r1), font)), PG_SALAH);
                }
                {
                        pg_tombol_radio_t *r2 = pg_buat_tombol_radio(g2, "Besar", font);
                        pg_tombol_radio_setel_ukuran(r2, 20);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("ukuran 20",
                                        pg_tombol_radio_widget(r2), font)), PG_SALAH);
                }
                pg_kotak_tambah(s, pg_kotak_widget(r), PG_SALAH);
        }

        return s;
}

static pg_kotak_widget_t *seksi_placeholder(const char *nama,
                                              const char *catatan,
                                              pg_font_t *font)
{
        pg_kotak_widget_t *s = buat_seksi(nama, catatan, font);
        {
                pg_label_t *l = pg_buat_label(
                        "[ belum di-polish — coming soon ]",
                        PG_ABU_GELAP, font);
                pg_label_setel_perataan(l, PG_LABEL_PERATAAN_TENGAH);
                pg_label_setel_padding(l, 12);
                pg_kotak_tambah(s, pg_label_widget(l), PG_BENAR);
        }
        return s;
}

/* ===== Build ===== */

static void build(app_t *a)
{
        pg_kotak_widget_t *root = pg_buat_kotak_widget(
                PG_KOTAK_VERTIKAL, SECTION_GAP);
        pg_kotak_milik(root, PG_BENAR);
        pg_kotak_setel_padding_kotak(root, 12);

        /* Set ukuran root = ukuran window. */
        pg_widget_setel_kotak(pg_kotak_widget(root),
                pg_buat_kotak(0, 0, W_AWAL, H_AWAL));

        /* Scroll area berisi semua section. */
        a->scroll = pg_buat_gulir();
        pg_widget_setel_kotak(pg_gulir_widget(a->scroll),
                pg_buat_kotak(0, 0, W_AWAL, H_AWAL - HEADER_H));
        pg_kotak_tambah(root, pg_gulir_widget(a->scroll), PG_BENAR);

        /* Content kotak vertikal berisi semua section. */
        a->content = pg_buat_kotak_widget(
                PG_KOTAK_VERTIKAL, SECTION_GAP);
        pg_kotak_milik(a->content, PG_BENAR);
        pg_kotak_setel_padding_kotak(a->content, 12);

        /* Sections. */
        {
                pg_kotak_widget_t *s_ikon = seksi_ikon(a);
                if (s_ikon)
                        pg_kotak_tambah(a->content,
                                pg_kotak_widget(s_ikon), PG_SALAH);
        }
        pg_kotak_tambah(a->content,
                pg_kotak_widget(seksi_tombol(a->font)), PG_SALAH);
        pg_kotak_tambah(a->content,
                pg_kotak_widget(seksi_label(a->font)), PG_SALAH);
        pg_kotak_tambah(a->content,
                pg_kotak_widget(seksi_kotak(a->font)), PG_SALAH);
        pg_kotak_tambah(a->content,
                pg_kotak_widget(seksi_isian_teks(a->font)), PG_SALAH);
        pg_kotak_tambah(a->content,
                pg_kotak_widget(seksi_penanda(a->font)), PG_SALAH);
        pg_kotak_tambah(a->content,
                pg_kotak_widget(seksi_radio(a->font)), PG_SALAH);
        pg_kotak_tambah(a->content,
                pg_kotak_widget(seksi_bilah_geser(a->font)), PG_SALAH);
        pg_kotak_tambah(a->content,
                pg_kotak_widget(seksi_angka_putar(a->font)), PG_SALAH);
        pg_kotak_tambah(a->content,
                pg_kotak_widget(seksi_kotak_pencarian(a->font)), PG_SALAH);
        pg_kotak_tambah(a->content,
                pg_kotak_widget(seksi_kotak_gabungan(a->font)), PG_SALAH);
        pg_kotak_tambah(a->content,
                pg_kotak_widget(seksi_multi_teks(a->font)), PG_SALAH);

        /* Set ukuran content. */
        pg_widget_setel_kotak(pg_kotak_widget(a->content),
                pg_buat_kotak(0, 0, W_AWAL, 2600));
        pg_kotak_tata(a->content);

        /* Set anak gulir. */
        pg_gulir_setel_anak(a->scroll,
                pg_kotak_widget(a->content), W_AWAL, 2600);

        a->root = root;
}

/* ===== Render ===== */

static void catat_all(app_t *a, pg_permukaan_t *s)
{
        pg_isi_permukaan(s, PG_WARNA_PANEL);
        pg_widget_catat(pg_kotak_widget(a->root), s);
}

/* ===== Event ===== */

static void on_event(const pg_aksi_t *e, void *ctx)
{
        app_t *a = ctx;
        if (e->tipe == PG_AKSI_KELUAR) {
                pg_hentikan_perulangan(a->loop);
                return;
        }
        if (e->tipe == PG_AKSI_JENDELA) {
                if (e->jendela_aksi == PG_JENDELA_TUTUP) {
                        pg_hentikan_perulangan(a->loop);
                } else if (e->jendela_aksi == PG_JENDELA_UBAH_UKURAN) {
                        /* Window di-resize/maximize: update layout. */
                        pg_layar_info_t info;
                        if (pg_layar_kueri(a->layar, &info) == PG_OK) {
                                pg_widget_setel_kotak(pg_kotak_widget(a->root),
                                        pg_buat_kotak(0, 0, info.lebar, info.tinggi));
                                pg_widget_setel_kotak(pg_gulir_widget(a->scroll),
                                        pg_buat_kotak(0, 0, info.lebar, info.tinggi - HEADER_H));
                                pg_kotak_tata(a->root);
                        }
                }
                return;
        }
        if (e->tipe == PG_AKSI_TOMBOL_TURUN &&
            e->tombol == PG_TOMBOL_ESCAPE) {
                pg_hentikan_perulangan(a->loop);
                return;
        }
        pg_widget_tangani_aksi(pg_kotak_widget(a->root), e);
}

/* ===== Idle ===== */

static void idle(void *ctx)
{
        app_t *a = ctx;
        pg_layar_info_t info;
        void *px;
        int langkah;
        pg_permukaan_t *s;
        if (pg_layar_kueri(a->layar, &info) != PG_OK) return;
        if (pg_layar_kunci(a->layar, &px, &langkah) != PG_OK) return;
        s = pg_bungkus_permukaan(px, info.lebar, info.tinggi, langkah);
        if (!s) { pg_layar_buka_kunci(a->layar); return; }
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
        pigura_init();

        a.font = pg_buat_font_ttf(
                "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14);
        if (!a.font) {
                fprintf(stderr, "Gagal load font\n");
                pigura_selesai();
                return 1;
        }

        /* Font ikon Material Design subset (5.9KB, 65 ikon). */
        a.font_ikon = pg_buat_font_ttf(
                "data/font/MaterialIcons-Subset.ttf", 18);
        if (!a.font_ikon) {
                fprintf(stderr, "Gagal load font ikon (lanjut tanpa ikon)\n");
        }

        memset(&lcfg, 0, sizeof(lcfg));
        lcfg.lebar = W_AWAL;
        lcfg.tinggi = H_AWAL;
        lcfg.judul = "Pigura Widget Gallery";
        err = pg_buka_layar(&a.layar, &lcfg);
        if (err != PG_OK) {
                fprintf(stderr, "pg_buka_layar: %s\n",
                        pg_galat_pesan(pg_galat_terakhir()));
                pg_hancur_font(a.font); if (a.font_ikon) pg_hancur_font(a.font_ikon);
                pigura_selesai();
                return 1;
        }

        memset(&mcfg, 0, sizeof(mcfg));
        err = pg_buka_masukan(&a.masukan, &mcfg, a.layar);
        if (err != PG_OK) {
                fprintf(stderr, "pg_buka_masukan: %s\n",
                        pg_galat_pesan(pg_galat_terakhir()));
                pg_tutup_layar(a.layar);
                pg_hancur_font(a.font); if (a.font_ikon) pg_hancur_font(a.font_ikon);
                pigura_selesai();
                return 1;
        }

        a.klip = pg_papan_klip_buka(a.layar);
        g_klip = a.klip;

        build(&a);

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
                pg_hancur_font(a.font); if (a.font_ikon) pg_hancur_font(a.font_ikon);
                pigura_selesai();
                return 1;
        }

        pg_jalankan_perulangan(a.loop, on_event, &a);

        pg_hancur_perulangan(a.loop);
        pg_tutup_masukan(a.masukan);
        if (a.klip) pg_papan_klip_tutup(a.klip);
        pg_tutup_layar(a.layar);
        pg_kotak_hancur(a.root);
        pg_hancur_font(a.font); if (a.font_ikon) pg_hancur_font(a.font_ikon);
        pigura_selesai();
        return 0;
}
