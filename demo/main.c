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
#include "pigura/cek.h"
#include "pigura/radio.h"
#include "pigura/papan_klip.h"
#include "pigura/font.h"
#include "pigura/permukaan.h"
#include "pigura/layar.h"
#include "pigura/masukan.h"
#include "pigura/perulangan.h"
#include "pigura/peristiwa.h"
#include "pigura/gambar.h"

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
                pg_widget_setel_radius(pg_label_widget(l), 8);
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
                        pg_widget_setel_radius(
                                pg_isian_teks_widget(it), 6);
                        if (g_klip) pg_isian_teks_setel_papan_klip(it, g_klip);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("radius 6",
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

static pg_kotak_widget_t *seksi_cek(pg_font_t *font)
{
        pg_kotak_widget_t *s = buat_seksi("Cek",
                "Checkbox. Posisi, tri-state, hover, fokus, keyboard.",
                font);

        /* Row 1: basic + positions. */
        {
                pg_kotak_widget_t *r = buat_baris(font);
                {
                        pg_cek_t *c = pg_buat_cek("Aktifkan notifikasi", font);
                        pg_cek_setel_dicek(c, PG_BENAR);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("dicek",
                                        pg_cek_widget(c), font)), PG_SALAH);
                }
                {
                        pg_cek_t *c = pg_buat_cek("Tidak dicek", font);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("kosong",
                                        pg_cek_widget(c), font)), PG_SALAH);
                }
                {
                        pg_cek_t *c = pg_buat_cek("Indeterminate", font);
                        pg_cek_setel_tri(c, PG_TRI_INDETERMINATE);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("tri-state",
                                        pg_cek_widget(c), font)), PG_SALAH);
                }
                pg_kotak_tambah(s, pg_kotak_widget(r), PG_SALAH);
        }

        /* Row 2: posisi berbeda. */
        {
                pg_kotak_widget_t *r = buat_baris(font);
                {
                        pg_cek_t *c = pg_buat_cek("Kanan", font);
                        pg_cek_setel_posisi(c, PG_POSISI_KANAN);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("kotak kanan",
                                        pg_cek_widget(c), font)), PG_SALAH);
                }
                {
                        pg_cek_t *c = pg_buat_cek("Besar", font);
                        pg_cek_setel_ukuran_kotak(c, 20);
                        pg_cek_setel_dicek(c, PG_BENAR);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("ukuran 20",
                                        pg_cek_widget(c), font)), PG_SALAH);
                }
                {
                        pg_cek_t *c = pg_buat_cek("Radius 4", font);
                        pg_widget_setel_radius(pg_cek_widget(c), 4);
                        pg_cek_setel_dicek(c, PG_BENAR);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("radius 4",
                                        pg_cek_widget(c), font)), PG_SALAH);
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
                pg_radio_grup_t *g = pg_buat_radio_grup();
                {
                        pg_radio_t *r1 = pg_buat_radio(g, "Pilihan A", font);
                        pg_radio_pilih(r1);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("A (terpilih)",
                                        pg_radio_widget(r1), font)), PG_SALAH);
                }
                {
                        pg_radio_t *r2 = pg_buat_radio(g, "Pilihan B", font);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("B",
                                        pg_radio_widget(r2), font)), PG_SALAH);
                }
                {
                        pg_radio_t *r3 = pg_buat_radio(g, "Pilihan C", font);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("C",
                                        pg_radio_widget(r3), font)), PG_SALAH);
                }
                pg_kotak_tambah(s, pg_kotak_widget(r), PG_SALAH);
        }

        /* Row 2: posisi + ukuran. */
        {
                pg_kotak_widget_t *r = buat_baris(font);
                pg_radio_grup_t *g2 = pg_buat_radio_grup();
                {
                        pg_radio_t *r1 = pg_buat_radio(g2, "Kanan", font);
                        pg_radio_setel_posisi(r1, PG_POSISI_KANAN);
                        pg_radio_pilih(r1);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("posisi kanan",
                                        pg_radio_widget(r1), font)), PG_SALAH);
                }
                {
                        pg_radio_t *r2 = pg_buat_radio(g2, "Besar", font);
                        pg_radio_setel_ukuran(r2, 20);
                        pg_kotak_tambah(r,
                                pg_kotak_widget(buat_item("ukuran 20",
                                        pg_radio_widget(r2), font)), PG_SALAH);
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
        pg_kotak_tambah(a->content,
                pg_kotak_widget(seksi_tombol(a->font)), PG_SALAH);
        pg_kotak_tambah(a->content,
                pg_kotak_widget(seksi_label(a->font)), PG_SALAH);
        pg_kotak_tambah(a->content,
                pg_kotak_widget(seksi_kotak(a->font)), PG_SALAH);
        pg_kotak_tambah(a->content,
                pg_kotak_widget(seksi_isian_teks(a->font)), PG_SALAH);
        pg_kotak_tambah(a->content,
                pg_kotak_widget(seksi_cek(a->font)), PG_SALAH);
        pg_kotak_tambah(a->content,
                pg_kotak_widget(seksi_radio(a->font)), PG_SALAH);

        /* Set ukuran content. */
        pg_widget_setel_kotak(pg_kotak_widget(a->content),
                pg_buat_kotak(0, 0, W_AWAL, 1600));
        pg_kotak_tata(a->content);

        /* Set anak gulir. */
        pg_gulir_setel_anak(a->scroll,
                pg_kotak_widget(a->content), W_AWAL, 1600);

        a->root = root;
}

/* ===== Render ===== */

static void catat_all(app_t *a, pg_permukaan_t *s)
{
        pg_isi_permukaan(s, PG_WARNA_PANEL);
        pg_widget_catat(pg_kotak_widget(a->root), s);
}

/* ===== Event ===== */

static void on_event(const pg_peristiwa_t *e, void *ctx)
{
        app_t *a = ctx;
        if (e->tipe == PG_PERISTIWA_KELUAR) {
                pg_hentikan_perulangan(a->loop);
                return;
        }
        if (e->tipe == PG_PERISTIWA_JENDELA) {
                if (e->jendela_peristiwa == PG_JENDELA_TUTUP) {
                        pg_hentikan_perulangan(a->loop);
                } else if (e->jendela_peristiwa == PG_JENDELA_UBAH_UKURAN) {
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
        if (e->tipe == PG_PERISTIWA_TOMBOL_TURUN &&
            e->tombol == PG_TOMBOL_ESCAPE) {
                pg_hentikan_perulangan(a->loop);
                return;
        }
        pg_widget_tangani_peristiwa(pg_kotak_widget(a->root), e);
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

        memset(&lcfg, 0, sizeof(lcfg));
        lcfg.lebar = W_AWAL;
        lcfg.tinggi = H_AWAL;
        lcfg.judul = "Pigura Widget Gallery";
        err = pg_buka_layar(&a.layar, &lcfg);
        if (err != PG_OK) {
                fprintf(stderr, "pg_buka_layar: %s\n",
                        pg_galat_pesan(pg_galat_terakhir()));
                pg_hancur_font(a.font);
                pigura_selesai();
                return 1;
        }

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
                pg_hancur_font(a.font);
                pigura_selesai();
                return 1;
        }

        pg_jalankan_perulangan(a.loop, on_event, &a);

        pg_hancur_perulangan(a.loop);
        pg_tutup_masukan(a.masukan);
        if (a.klip) pg_papan_klip_tutup(a.klip);
        pg_tutup_layar(a.layar);
        pg_kotak_hancur(a.root);
        pg_hancur_font(a.font);
        pigura_selesai();
        return 0;
}
