/* ----------------------------------------------------------------------------------------------
 * pigura inti: tema.c - implementasi theme system
 * ----------------------------------------------------------------------------------------------
 * Theme system pigura. pg_tema_buat() memberi tema light mirip GTK
 * Adwaita; pg_tema_buat_gelap() memberi tema dark.
 *
 * pg_tema_terapkan_widget() mengubah pg_widget->latar ke warna
 * latar_widget tema. pg_tema_terapkan_kotak() rekursif menerapkan
 * ke box sendiri dan semua anaknya.
 *
 * Per-widget override (mis. tombol punya 3 warna latar_biasa/hover/
 * tekan) butuh casting ke struct impl masing-masing, yang akan
 * ditangani v0.3+ bersama theme engine yang lebih lengkap.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/tema.h"
#include "pigura/widget.h"
#include "pigura/kotak.h"
#include "pigura/galat.h"
#include "pigura/catat.h"

#include <stdlib.h>
#include <string.h>

pg_tema_t *pg_tema_buat(void)
{
        pg_tema_t *t;
        t = (pg_tema_t *)calloc(1, sizeof(*t));
        if (!t) {
                pg_set_galat(PG_GALAT_MEMORI, "pg_tema_buat: oom");
                return NULL;
        }
        /* Light theme mirip GTK Adwaita. */
        t->latar          = PG_ABU_TERANG;     /* 0xc0c0c0 */
        t->latar_widget    = PG_ABU_TERANG;
        t->latar_input     = PG_PUTIH;
        t->tepi           = PG_ABU_GELAP;
        t->teks            = PG_HITAM;
        t->teks_terpilih   = PG_PUTIH;
        t->latar_terpilih  = PG_BIRU;
        t->latar_hover     = PG_RGB(0xd0, 0xd0, 0xd0);
        t->aksen           = PG_BIRU;
        t->judul           = PG_BIRU;
        t->judul_teks      = PG_PUTIH;
        t->font            = NULL;
        t->ukuran_font     = 13;
        t->padding         = 4;
        t->spasi           = 4;
        t->radius_sudut    = 0;
        return t;
}

pg_tema_t *pg_tema_buat_gelap(void)
{
        pg_tema_t *t;
        t = (pg_tema_t *)calloc(1, sizeof(*t));
        if (!t) {
                pg_set_galat(PG_GALAT_MEMORI,
                             "pg_tema_buat_gelap: oom");
                return NULL;
        }
        /* Dark theme. */
        t->latar          = PG_ABU_GELAP;       /* 0x303030 */
        t->latar_widget    = PG_RGB(0x40, 0x40, 0x40);
        t->latar_input     = PG_RGB(0x20, 0x20, 0x20);
        t->tepi           = PG_RGB(0x60, 0x60, 0x60);
        t->teks            = PG_PUTIH;
        t->teks_terpilih   = PG_HITAM;
        t->latar_terpilih  = PG_CYAN;
        t->latar_hover     = PG_RGB(0x50, 0x50, 0x50);
        t->aksen           = PG_CYAN;
        t->judul           = PG_RGB(0x1a, 0x1a, 0x2a);
        t->judul_teks      = PG_PUTIH;
        t->font            = NULL;
        t->ukuran_font     = 13;
        t->padding         = 4;
        t->spasi           = 4;
        t->radius_sudut    = 0;
        return t;
}

void pg_tema_terapkan_widget(pg_widget_t *w, const pg_tema_t *t)
{
        if (!w || !t) return;
        /* v0.1: set latar widget dasar. Per-widget override (tombol,
         * cek, isian_teks) butuh casting ke struct impl masing-masing,
         * yang akan ditangani v0.3+. */
        pg_widget_setel_latar(w, t->latar_widget);
        pg_widget_kotor(w);
}

void pg_tema_terapkan_kotak(pg_kotak_widget_t *box,
                             const pg_tema_t *t)
{
        pg_widget_t *base;
        int n, i;
        if (!box || !t) return;
        base = pg_kotak_widget(box);
        if (base) pg_tema_terapkan_widget(base, t);
        /* Telusuri semua anak secara rekursif. */
        n = pg_kotak_jumlah_anak(box);
        for (i = 0; i < n; i++) {
                pg_widget_t *anak = pg_kotak_anak(box, i);
                if (!anak) continue;
                if (anak->tipe == PG_WIDGET_KOTAK) {
                        /* Cast aman: pg_kotak_widget_t punya base
                         * pg_widget_t sebagai field pertama. Pointer
                         * cast tidak butuh struct lengkap. */
                        pg_tema_terapkan_kotak(
                                (pg_kotak_widget_t *)anak, t);
                } else {
                        pg_tema_terapkan_widget(anak, t);
                }
        }
}

void pg_tema_hancur(pg_tema_t *t)
{
        if (!t) return;
        /* Font TIDAK dihancurkan di sini — pemilik tema mungkin pakai
         * font yang sama untuk banyak hal. */
        free(t);
}
