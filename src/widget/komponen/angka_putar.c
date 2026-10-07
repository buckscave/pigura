/* ----------------------------------------------------------------------------------------------
 * pigura widget: angka_putar.c - widget spinbox (angka putar) integer
 * ----------------------------------------------------------------------------------------------
 * Spinbox dengan tombol naik/turun dan field teks nilai. Konsisten
 * dengan tema Batch 1: latar PANEL, batas HOVER_OUTLINE, fokus FOKUS,
 * tekan TEKAN_ISI. Radius kotak default 4.
 *
 * Layout:
 *   [ field nilai (text)        ] [ ▲ ]
 *                                  [ ▼ ]
 *
 * Interaksi:
 *   - Klik ▲ / ▼ : naik/turun nilai sebesar langkah.
 *   - Klik field : fokus, lalu keyboard arrow up/down = naik/turun.
 *   - Roda mouse: gulir vertikal = naik/turun.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/angka_putar.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/font.h"
#include "pigura/aksi.h"
#include "pigura/widget.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PG_AP_BTN_W 20

struct pg_angka_putar {
        pg_widget_t base;
        int min, maks, nilai, langkah;
        pg_warna_t fg, latar, batas, btn_bg;
        pg_font_t *font;
        pg_angka_putar_cb cb;
        void *ctx;
        pg_bool naik_hover, turun_hover, naik_tekan, turun_tekan;
        pg_bool hover;          /* mouse di field nilai */
        /* Buffer input manual angka. Saat user ketik digit di field,
         * akumulasi ke sini. Enter = commit, Escape = batal. */
        char input_buf[16];
        int  input_len;
        pg_bool sedang_input;
};

static pg_angka_putar_t *pg_ap_dari(pg_widget_t *w)
{
        return (pg_angka_putar_t *)w;
}

static void pg_ap_kotor(pg_angka_putar_t *sb)
{
        pg_widget_kotor(&sb->base);
}

static void pg_ap_fire(pg_angka_putar_t *sb)
{
        if (sb->cb) sb->cb(sb, sb->nilai, sb->ctx);
}

static void pg_ap_naik(pg_angka_putar_t *sb)
{
        if (sb->nilai + sb->langkah <= sb->maks) {
                sb->nilai += sb->langkah;
                pg_ap_kotor(sb);
                pg_ap_fire(sb);
        }
}

static void pg_ap_turun(pg_angka_putar_t *sb)
{
        if (sb->nilai - sb->langkah >= sb->min) {
                sb->nilai -= sb->langkah;
                pg_ap_kotor(sb);
                pg_ap_fire(sb);
        }
}

static void pg_ap_catat_v(pg_widget_t *w, pg_permukaan_t *s)
{
        pg_angka_putar_t *sb = pg_ap_dari(w);
        int sw, sh, bw, bl, th, y, radius;
        pg_kotak_t r;
        char buf[32];
        pg_warna_t latar_warna, batas_warna, fg_warna;
        pg_warna_t naik_warna, turun_warna;

        sw = pg_permukaan_lebar(s);
        sh = pg_permukaan_tinggi(s);
        bw = PG_AP_BTN_W;
        bl = sw - bw;
        radius = w->radius;

        /* Tentukan warna state. */
        if (!w->aktif) {
                latar_warna  = PG_WARNA_NONAKTIF_ISI;
                batas_warna  = PG_ABU_TERANG;
                fg_warna     = PG_WARNA_NONAKTIF_TEKS;
                naik_warna   = PG_ABU_TERANG;
                turun_warna  = PG_ABU_TERANG;
        } else {
                latar_warna = sb->latar;
                fg_warna    = sb->fg;
                if (w->fokus) batas_warna = PG_WARNA_FOKUS;
                else if (sb->hover) batas_warna = PG_RGB(0x6E, 0x6E, 0x6E);
                else batas_warna = sb->batas;
                naik_warna = sb->naik_tekan ? PG_WARNA_TEKAN_ISI :
                              (sb->naik_hover ? PG_WARNA_HOVER_ISI :
                                                sb->btn_bg);
                turun_warna = sb->turun_tekan ? PG_WARNA_TEKAN_ISI :
                               (sb->turun_hover ? PG_WARNA_HOVER_ISI :
                                                  sb->btn_bg);
        }

        /* Latar cerdas: bila alpha < 255, clear transparan. */
        if (PG_A(w->latar) < 255) {
                pg_isi_permukaan(s, PG_TRANSPARAN);
        }

        /* Field nilai (latar + outline) dengan rounded rect AA.
         * Hanya bagian field (0..bl), bukan button. */
        r = pg_buat_kotak(0, 0, sw, sh);
        /* Render full outline sebagai satu kotak dulu, supaya
         * tidak ada gap antara field dan button. */
        pg_gambar_kotak_tumpul_isi_garis_aa(s, r, radius,
                                            latar_warna, batas_warna);

        /* Tombol ▲ dan ▼ di kanan. Pakai isi solid dengan state color.
         * Supaya outline tetap utuh, gambar fill saja (tanpa outline). */
        {
                pg_kotak_t r_naik = pg_buat_kotak(bl, 0, bw, sh / 2);
                pg_kotak_t r_turun = pg_buat_kotak(bl, sh / 2, bw,
                                                    sh - sh / 2);
                /* Untuk button kanan, kita gambar rounded rect penuh
                 * supaya sudut kanan-atas dan kanan-bawah rounded. Tapi
                 * karena button ada 2 (atas+bawah), kita bagi jadi 2
                 * rect dengan radius setengah lingkaran. Sederhana:
                 * gambar fill rect biasa untuk button area, lalu
                 * rounded rect outline di atas full widget (sudut
                 * rounded mengikuti outline luar). */
                pg_isi_permukaan_kotak(s, r_naik, naik_warna);
                pg_isi_permukaan_kotak(s, r_turun, turun_warna);
                /* Divider horizontal antara ▲ dan ▼. */
                pg_garis_h_permukaan(s, bl, sw, sh / 2, batas_warna);
                /* Divider vertikal antara field dan button. */
                pg_garis_v_permukaan(s, bl, 0, sh, batas_warna);
        }

        /* Panah ▲. */
        {
                int cx = bl + bw / 2, cy = sh / 4;
                pg_setel_piksel_permukaan(s, cx, cy - 2, fg_warna);
                pg_garis_h_permukaan(s, cx - 2, cx + 2, cy - 1,
                                      fg_warna);
                pg_garis_h_permukaan(s, cx - 3, cx + 3, cy, fg_warna);
        }
        /* Panah ▼. */
        {
                int cx = bl + bw / 2, cy = sh * 3 / 4;
                pg_garis_h_permukaan(s, cx - 3, cx + 3, cy, fg_warna);
                pg_garis_h_permukaan(s, cx - 2, cx + 2, cy + 1,
                                      fg_warna);
                pg_setel_piksel_permukaan(s, cx, cy + 2, fg_warna);
        }

        /* Teks nilai atau input buffer. */
        if (sb->font) {
                th = pg_font_tinggi(sb->font);
                if (th <= 0) th = 8;
                y = pg_font_baseline_tengah(sb->font, sh);
                if (sb->sedang_input && sb->input_len > 0) {
                        /* Tampilkan input buffer + cursor. */
                        char tmp[20];
                        snprintf(tmp, sizeof(tmp), "%s",
                                 sb->input_buf);
                        pg_font_gambar_teks(sb->font, tmp, s, 4, y,
                                             fg_warna);
                        /* Cursor blink garis vertikal di akhir. */
                        {
                                int cx = 4 + pg_font_lebar_teks(
                                        sb->font, tmp);
                                pg_garis_v_permukaan(s, cx,
                                        y - th + 2, y + 2,
                                        fg_warna);
                        }
                } else {
                        snprintf(buf, sizeof(buf), "%d", sb->nilai);
                        pg_font_gambar_teks(sb->font, buf, s, 4, y,
                                             fg_warna);
                }
        }
}

static pg_bool pg_ap_peristiwa_v(pg_widget_t *w, const pg_aksi_t *e)
{
        pg_angka_putar_t *sb = pg_ap_dari(w);
        int sw = w->kotak.w, sh = w->kotak.h;
        int bw = PG_AP_BTN_W, bl = sw - bw;

        if (!w->aktif) return PG_SALAH;

        /* Roda mouse: naik/turun. */
        if (e->tipe == PG_AKSI_TETIKUS_GULIR) {
                if (e->roda_dy > 0) pg_ap_naik(sb);
                else if (e->roda_dy < 0) pg_ap_turun(sb);
                return PG_BENAR;
        }

        if (e->tipe == PG_AKSI_TETIKUS_TEKAN &&
            e->tetik_tombol == PG_TETIKUS_KIRI) {
                int x = e->tetik_pos.x, y = e->tetik_pos.y;
                pg_widget_fokus(w);
                if (x >= bl) {
                        if (y < sh / 2) {
                                sb->naik_tekan = PG_BENAR;
                                pg_ap_naik(sb);
                        } else {
                                sb->turun_tekan = PG_BENAR;
                                pg_ap_turun(sb);
                        }
                        pg_ap_kotor(sb);
                        return PG_BENAR;
                }
                /* Klik field: fokus saja. */
                return PG_BENAR;
        }

        if (e->tipe == PG_AKSI_TETIKUS_LEPAS &&
            e->tetik_tombol == PG_TETIKUS_KIRI) {
                sb->naik_tekan = PG_SALAH;
                sb->turun_tekan = PG_SALAH;
                pg_ap_kotor(sb);
                return PG_BENAR;
        }

        if (e->tipe == PG_AKSI_TETIKUS_GERAK) {
                int x = e->tetik_pos.x, y = e->tetik_pos.y;
                pg_bool nh = PG_SALAH, th = PG_SALAH;
                pg_bool new_hover = PG_SALAH;
                /* Boundary check: hanya update hover bila mouse di widget. */
                if (x >= 0 && x < sw && y >= 0 && y < sh) {
                        if (x >= bl) {
                                if (y < sh / 2) nh = PG_BENAR;
                                else th = PG_BENAR;
                        } else {
                                new_hover = PG_BENAR;
                        }
                }
                if (nh != sb->naik_hover ||
                    th != sb->turun_hover ||
                    new_hover != sb->hover) {
                        sb->naik_hover = nh;
                        sb->turun_hover = th;
                        sb->hover = new_hover;
                        pg_ap_kotor(sb);
                }
                return PG_SALAH;
        }

        /* Keyboard. */
        if (e->tipe == PG_AKSI_TOMBOL_TURUN &&
            pg_widget_punya_fokus(w)) {
                if (e->tombol == PG_TOMBOL_ATAS) {
                        if (sb->sedang_input) {
                                sb->sedang_input = PG_SALAH;
                                sb->input_len = 0;
                        }
                        pg_ap_naik(sb);
                        return PG_BENAR;
                }
                if (e->tombol == PG_TOMBOL_BAWAH) {
                        if (sb->sedang_input) {
                                sb->sedang_input = PG_SALAH;
                                sb->input_len = 0;
                        }
                        pg_ap_turun(sb);
                        return PG_BENAR;
                }
                if (e->tombol == PG_TOMBOL_HOME) {
                        sb->sedang_input = PG_SALAH;
                        sb->input_len = 0;
                        if (sb->min != sb->nilai) {
                                sb->nilai = sb->min;
                                pg_ap_kotor(sb);
                                pg_ap_fire(sb);
                        }
                        return PG_BENAR;
                }
                if (e->tombol == PG_TOMBOL_END) {
                        sb->sedang_input = PG_SALAH;
                        sb->input_len = 0;
                        if (sb->maks != sb->nilai) {
                                sb->nilai = sb->maks;
                                pg_ap_kotor(sb);
                                pg_ap_fire(sb);
                        }
                        return PG_BENAR;
                }
                /* Enter: commit input buffer ke nilai. */
                if (e->tombol == PG_TOMBOL_ENTER) {
                        if (sb->sedang_input && sb->input_len > 0) {
                                int v = atoi(sb->input_buf);
                                if (v < sb->min) v = sb->min;
                                if (v > sb->maks) v = sb->maks;
                                if (v != sb->nilai) {
                                        sb->nilai = v;
                                        pg_ap_kotor(sb);
                                        pg_ap_fire(sb);
                                }
                                sb->sedang_input = PG_SALAH;
                                sb->input_len = 0;
                        }
                        return PG_BENAR;
                }
                /* Escape: batal input. */
                if (e->tombol == PG_TOMBOL_ESCAPE) {
                        if (sb->sedang_input) {
                                sb->sedang_input = PG_SALAH;
                                sb->input_len = 0;
                                pg_ap_kotor(sb);
                        }
                        return PG_BENAR;
                }
                /* Backspace: hapus digit terakhir dari input buffer. */
                if (e->tombol == PG_TOMBOL_BACKSPACE) {
                        if (sb->sedang_input && sb->input_len > 0) {
                                sb->input_len--;
                                sb->input_buf[sb->input_len] = 0;
                                pg_ap_kotor(sb);
                        }
                        return PG_BENAR;
                }
                /* Digit 0-9 atau minus: akumulasi ke input buffer. */
                {
                        pg_u32 cp = e->unicode ? e->unicode :
                                (pg_u32)e->tombol;
                        if ((cp >= '0' && cp <= '9') ||
                            (cp == '-' && sb->input_len == 0)) {
                                if (!sb->sedang_input) {
                                        sb->sedang_input = PG_BENAR;
                                        sb->input_len = 0;
                                        sb->input_buf[0] = 0;
                                }
                                if (sb->input_len < 15) {
                                        sb->input_buf[sb->input_len++] =
                                                (char)cp;
                                        sb->input_buf[sb->input_len] = 0;
                                        pg_ap_kotor(sb);
                                }
                                return PG_BENAR;
                        }
                }
        }

        return PG_SALAH;
}

static void pg_ap_hancur_v(pg_widget_t *w)
{
        (void)w;
}

static void pg_ap_bebas_v(pg_widget_t *w)
{
        free(w);
}

static const pg_widget_vtable_t pg_ap_vtable = {
        pg_ap_catat_v,
        pg_ap_peristiwa_v,
        NULL,
        pg_ap_hancur_v,
        NULL,
        NULL,
        pg_ap_bebas_v
};

pg_angka_putar_t *pg_buat_angka_putar(int min, int maks,
                                        int nilai, int langkah,
                                        pg_font_t *font)
{
        pg_angka_putar_t *sb;
        sb = (pg_angka_putar_t *)calloc(1, sizeof(*sb));
        if (!sb) return NULL;
        pg_widget_init(&sb->base, PG_WIDGET_DASAR, &pg_ap_vtable);
        pg_widget_milik(&sb->base, PG_BENAR);
        sb->font = font;
        sb->min = min;
        sb->maks = maks;
        sb->nilai = nilai;
        sb->langkah = langkah;
        if (sb->maks < sb->min) sb->maks = sb->min;
        if (sb->nilai < sb->min) sb->nilai = sb->min;
        if (sb->nilai > sb->maks) sb->nilai = sb->maks;
        if (sb->langkah < 1) sb->langkah = 1;
        sb->fg = PG_WARNA_TEKS_TOMBOL;
        sb->latar = PG_WARNA_PANEL;
        sb->batas = PG_WARNA_HOVER_OUTLINE;
        sb->btn_bg = PG_WARNA_PANEL;
        sb->base.latar = PG_TRANSPARAN;
        sb->base.radius = 0;  /* sudut tajam */
        pg_widget_setel_ukuran_min(&sb->base, 60, 24);
        return sb;
}

void pg_angka_putar_hancur(pg_angka_putar_t *sb)
{
        if (!sb) return;
        pg_widget_hancur(&sb->base);
        free(sb);
}

int pg_angka_putar_nilai(pg_angka_putar_t *sb)
{
        return sb ? sb->nilai : 0;
}

void pg_angka_putar_setel_nilai(pg_angka_putar_t *sb, int v)
{
        if (!sb) return;
        if (v < sb->min) v = sb->min;
        if (v > sb->maks) v = sb->maks;
        if (v == sb->nilai) return;
        sb->nilai = v;
        pg_ap_kotor(sb);
        pg_ap_fire(sb);
}

void pg_angka_putar_setel_rentang(pg_angka_putar_t *sb,
                                    int min, int maks, int langkah)
{
        if (!sb) return;
        sb->min = min;
        sb->maks = maks;
        sb->langkah = langkah;
        if (sb->maks < sb->min) sb->maks = sb->min;
        if (sb->langkah < 1) sb->langkah = 1;
        if (sb->nilai < sb->min) sb->nilai = sb->min;
        if (sb->nilai > sb->maks) sb->nilai = sb->maks;
        pg_ap_kotor(sb);
}

void pg_angka_putar_saatberubah(pg_angka_putar_t *sb,
                                  pg_angka_putar_cb cb, void *ctx)
{
        if (!sb) return;
        sb->cb = cb;
        sb->ctx = ctx;
}

void pg_angka_putar_setel_aktif(pg_angka_putar_t *sb, pg_bool aktif)
{
        if (!sb) return;
        if (aktif) {
                pg_widget_aktifkan(&sb->base);
        } else {
                pg_widget_nonaktifkan(&sb->base);
                if (sb->base.fokus) pg_widget_blur(&sb->base);
                sb->naik_hover = PG_SALAH;
                sb->turun_hover = PG_SALAH;
                sb->naik_tekan = PG_SALAH;
                sb->turun_tekan = PG_SALAH;
                sb->hover = PG_SALAH;
        }
        pg_ap_kotor(sb);
}

pg_bool pg_angka_putar_aktif(pg_angka_putar_t *sb)
{
        return sb ? sb->base.aktif : PG_SALAH;
}

pg_widget_t *pg_angka_putar_widget(pg_angka_putar_t *sb)
{
        return sb ? &sb->base : NULL;
}
