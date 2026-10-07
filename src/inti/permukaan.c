/* ----------------------------------------------------------------------------------------------
 * pigura inti: permukaan.c - permukaan gambar offscreen
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/permukaan.h"
#include "pigura/galat.h"
#include "pigura/catat.h"

#include <stdlib.h>
#include <string.h>

struct pg_permukaan {
        int        lebar, tinggi, langkah;
        pg_warna_t *piksel;
        pg_bool    punya;
        pg_kotak_t kotor;
};

pg_kotak_t pg_buat_kotak(int x, int y, int w, int h)
{
        pg_kotak_t r;
        r.x = x; r.y = y; r.w = w; r.h = h;
        return r;
}

pg_titik_t pg_buat_titik(int x, int y)
{
        pg_titik_t p;
        p.x = x; p.y = y;
        return p;
}

pg_permukaan_t *pg_buat_permukaan(int lebar, int tinggi)
{
        pg_permukaan_t *s;
        if (lebar <= 0 || tinggi <= 0) {
                pg_set_galat(PG_GALAT_ARGUMEN, "buat_permukaan: dimensi "
                             "buruk %dx%d", lebar, tinggi);
                return NULL;
        }
        s = (pg_permukaan_t *)calloc(1, sizeof(*s));
        if (!s) {
                pg_set_galat(PG_GALAT_MEMORI, "buat_permukaan: oom");
                return NULL;
        }
        s->lebar  = lebar;
        s->tinggi = tinggi;
        s->langkah = lebar * (int)sizeof(pg_warna_t);
        s->piksel = (pg_warna_t *)calloc((size_t)lebar * tinggi,
                                          sizeof(pg_warna_t));
        if (!s->piksel) {
                free(s);
                pg_set_galat(PG_GALAT_MEMORI, "buat_permukaan: oom");
                return NULL;
        }
        s->punya = PG_BENAR;
        s->kotor = pg_buat_kotak(0, 0, lebar, tinggi);
        return s;
}

pg_permukaan_t *pg_klon_permukaan(const pg_permukaan_t *src)
{
        pg_permukaan_t *s;
        if (!src) return NULL;
        s = pg_buat_permukaan(src->lebar, src->tinggi);
        if (!s) return NULL;
        memcpy(s->piksel, src->piksel,
               (size_t)src->langkah * src->tinggi);
        s->kotor = src->kotor;
        return s;
}

void pg_hancur_permukaan(pg_permukaan_t *s)
{
        if (!s) return;
        if (s->punya && s->piksel)
                free(s->piksel);
        free(s);
}

pg_permukaan_t *pg_bungkus_permukaan(void *piksel, int lebar, int tinggi,
                                      int langkah)
{
        pg_permukaan_t *s;
        if (!piksel || lebar <= 0 || tinggi <= 0 || langkah <= 0)
                return NULL;
        s = (pg_permukaan_t *)calloc(1, sizeof(*s));
        if (!s) return NULL;
        s->lebar  = lebar;
        s->tinggi = tinggi;
        s->langkah = langkah;
        s->piksel = (pg_warna_t *)piksel;
        s->punya = PG_SALAH;
        s->kotor = pg_buat_kotak(0, 0, lebar, tinggi);
        return s;
}

int pg_permukaan_lebar(const pg_permukaan_t *s)  { return s ? s->lebar  : 0; }
int pg_permukaan_tinggi(const pg_permukaan_t *s) { return s ? s->tinggi : 0; }
int pg_permukaan_langkah(const pg_permukaan_t *s)  { return s ? s->langkah : 0; }

const void *pg_permukaan_piksel(const pg_permukaan_t *s)
{
        return s ? s->piksel : NULL;
}

void *pg_permukaan_piksel_mut(pg_permukaan_t *s)
{
        return s ? s->piksel : NULL;
}

static pg_kotak_t pg_potong_ke_permukaan(const pg_permukaan_t *s,
                                          pg_kotak_t r)
{
        if (r.x < 0) { r.w += r.x; r.x = 0; }
        if (r.y < 0) { r.h += r.y; r.y = 0; }
        if (r.x + r.w > s->lebar)  r.w = s->lebar  - r.x;
        if (r.y + r.h > s->tinggi) r.h = s->tinggi - r.y;
        if (r.w < 0) r.w = 0;
        if (r.h < 0) r.h = 0;
        return r;
}

void pg_isi_permukaan(pg_permukaan_t *s, pg_warna_t c)
{
        int y;
        if (!s) return;
        /* Kalau c = transparan penuh (A==0):
         * - Owned permukaan (offscreen icon): clear ke 0 (memset)
         * - Wrapped permukaan (X11 backbuffer): skip, jangan timpa
         *   parent dengan 0. */
        if (PG_A(c) == 0) {
                if (s->punya) {
                        int y2;
                        for (y2 = 0; y2 < s->tinggi; y2++) {
                                pg_warna_t *row = (pg_warna_t *)
                                        ((char *)s->piksel +
                                         (size_t)y2 * s->langkah);
                                memset(row, 0,
                                       (size_t)s->lebar * sizeof(pg_warna_t));
                        }
                        s->kotor = pg_buat_kotak(0, 0, s->lebar, s->tinggi);
                }
                return;
        }
        for (y = 0; y < s->tinggi; y++) {
                pg_warna_t *row = (pg_warna_t *)((char *)s->piksel +
                                  (size_t)y * s->langkah);
                int x;
                if (PG_A(c) == 255) {
                        for (x = 0; x < s->lebar; x++)
                                row[x] = c;
                } else {
                        /* Semi-transparan: blend dengan existing pixel */
                        int ia = 255 - PG_A(c);
                        int r = PG_R(c), g = PG_G(c), b = PG_B(c), a = PG_A(c);
                        for (x = 0; x < s->lebar; x++) {
                                pg_warna_t bg = row[x];
                                int nr = (r*a + PG_R(bg)*ia)/255;
                                int ng = (g*a + PG_G(bg)*ia)/255;
                                int nb = (b*a + PG_B(bg)*ia)/255;
                                int na = a + (PG_A(bg)*ia)/255;
                                row[x] = PG_RGBA(nr,ng,nb,na);
                        }
                }
        }
        s->kotor = pg_buat_kotak(0, 0, s->lebar, s->tinggi);
}

void pg_isi_permukaan_kotak(pg_permukaan_t *s, pg_kotak_t r, pg_warna_t c)
{
        int y;
        if (!s) return;
        if (PG_A(c) == 0) return;
        r = pg_potong_ke_permukaan(s, r);
        if (PG_KOTAK_KOSONG(r)) return;
        for (y = r.y; y < r.y + r.h; y++) {
                pg_warna_t *row = (pg_warna_t *)((char *)s->piksel +
                                  (size_t)y * s->langkah);
                int x;
                if (PG_A(c) == 255) {
                        for (x = r.x; x < r.x + r.w; x++)
                                row[x] = c;
                } else {
                        int ia = 255 - PG_A(c);
                        int r2 = PG_R(c), g2 = PG_G(c), b2 = PG_B(c), a2 = PG_A(c);
                        for (x = r.x; x < r.x + r.w; x++) {
                                pg_warna_t bg = row[x];
                                int nr = (r2*a2 + PG_R(bg)*ia)/255;
                                int ng = (g2*a2 + PG_G(bg)*ia)/255;
                                int nb = (b2*a2 + PG_B(bg)*ia)/255;
                                int na = a2 + (PG_A(bg)*ia)/255;
                                row[x] = PG_RGBA(nr,ng,nb,na);
                        }
                }
        }
        pg_permukaan_kotor(s, r);
}

void pg_setel_piksel_permukaan(pg_permukaan_t *s, int x, int y, pg_warna_t c)
{
        pg_warna_t *p;
        if (!s) return;
        if (x < 0 || y < 0 || x >= s->lebar || y >= s->tinggi) return;
        p = (pg_warna_t *)((char *)s->piksel + (size_t)y * s->langkah);
        p[x] = c;
        pg_permukaan_kotor(s, pg_buat_kotak(x, y, 1, 1));
}

pg_warna_t pg_ambil_piksel_permukaan(const pg_permukaan_t *s, int x, int y)
{
        const pg_warna_t *p;
        if (!s) return 0;
        if (x < 0 || y < 0 || x >= s->lebar || y >= s->tinggi) return 0;
        p = (const pg_warna_t *)((const char *)s->piksel +
             (size_t)y * s->langkah);
        return p[x];
}

/* Bersihkan permukaan ke transparan penuh (0x00000000).
 * Berbeda dari pg_isi_permukaan(PG_TRANSPARAN) yang skip
 * bila A==0 — fungsi ini selalu set pixel ke 0. */
void pg_bersihkan_permukaan(pg_permukaan_t *s)
{
        int y;
        if (!s || !s->piksel) return;
        for (y = 0; y < s->tinggi; y++) {
                pg_warna_t *row = (pg_warna_t *)((char *)s->piksel +
                                  (size_t)y * s->langkah);
                int x;
                for (x = 0; x < s->lebar; x++)
                        row[x] = 0;
        }
        s->kotor = pg_buat_kotak(0, 0, s->lebar, s->tinggi);
}

void pg_garis_h_permukaan(pg_permukaan_t *s, int x0, int x1, int y,
                            pg_warna_t c)
{
        int x;
        if (!s) return;
        if (y < 0 || y >= s->tinggi) return;
        if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
        if (x0 < 0) x0 = 0;
        if (x1 >= s->lebar) x1 = s->lebar - 1;
        for (x = x0; x <= x1; x++)
                pg_setel_piksel_permukaan(s, x, y, c);
}

void pg_garis_v_permukaan(pg_permukaan_t *s, int x, int y0, int y1,
                            pg_warna_t c)
{
        int y;
        if (!s) return;
        if (x < 0 || x >= s->lebar) return;
        if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
        if (y0 < 0) y0 = 0;
        if (y1 >= s->tinggi) y1 = s->tinggi - 1;
        for (y = y0; y <= y1; y++)
                pg_setel_piksel_permukaan(s, x, y, c);
}

void pg_kotak_permukaan(pg_permukaan_t *s, pg_kotak_t r, pg_warna_t c)
{
        if (!s) return;
        pg_garis_h_permukaan(s, r.x, r.x + r.w - 1, r.y, c);
        pg_garis_h_permukaan(s, r.x, r.x + r.w - 1, r.y + r.h - 1, c);
        pg_garis_v_permukaan(s, r.x, r.y, r.y + r.h - 1, c);
        pg_garis_v_permukaan(s, r.x + r.w - 1, r.y, r.y + r.h - 1, c);
}

void pg_blit_permukaan(pg_permukaan_t *dst, int dx, int dy,
                        const pg_permukaan_t *src)
{
        if (!dst || !src) return;
        pg_blit_sub_permukaan(dst, dx, dy, src,
                               pg_buat_kotak(0, 0, src->lebar, src->tinggi));
}

void pg_blit_potong_permukaan(pg_permukaan_t *dst, int dx, int dy,
                                const pg_permukaan_t *src,
                                pg_kotak_t clip)
{
        pg_kotak_t dr;
        int sx_off, sy_off, y;
        if (!dst || !src) return;
        /* Hitung overlap antara dst area (dx,dy,src.w,src.h) dan clip. */
        {
                int x0 = PG_MAX(dx, clip.x);
                int y0 = PG_MAX(dy, clip.y);
                int x1 = PG_MIN(dx + src->lebar, clip.x + clip.w);
                int y1 = PG_MIN(dy + src->tinggi, clip.y + clip.h);
                if (x1 <= x0 || y1 <= y0) return;
                dr = pg_buat_kotak(x0, y0, x1 - x0, y1 - y0);
        }
        /* Potong ke bounds dst juga. */
        dr = pg_potong_ke_permukaan(dst, dr);
        if (PG_KOTAK_KOSONG(dr)) return;
        sx_off = dr.x - dx;
        sy_off = dr.y - dy;
        /* Alpha blend: bila src punya alpha < 255, blend dengan dst.
         * Bila alpha = 255, raw copy (cepat). */
        for (y = 0; y < dr.h; y++) {
                pg_warna_t *drow = (pg_warna_t *)((char *)dst->piksel +
                                  (size_t)(dr.y + y) * dst->langkah);
                const pg_warna_t *srow = (const pg_warna_t *)
                                  ((const char *)src->piksel +
                                  (size_t)(sy_off + y) * src->langkah);
                int x;
                for (x = 0; x < dr.w; x++) {
                        pg_warna_t sc = srow[sx_off + x];
                        int sa = (int)PG_A(sc);
                        if (sa == 0) continue;  /* transparan */
                        if (sa == 255) {
                                drow[dr.x + x] = sc;
                        } else {
                                pg_warna_t dc = drow[dr.x + x];
                                int ia = 255 - sa;
                                int r = (PG_R(sc) * sa + PG_R(dc) * ia) / 255;
                                int g = (PG_G(sc) * sa + PG_G(dc) * ia) / 255;
                                int b = (PG_B(sc) * sa + PG_B(dc) * ia) / 255;
                                int a = sa + (PG_A(dc) * ia) / 255;
                                drow[dr.x + x] = PG_RGBA(r, g, b, a);
                        }
                }
        }
        pg_permukaan_kotor(dst, dr);
}

void pg_blit_alpha_permukaan(pg_permukaan_t *dst, int dx, int dy,
                              const pg_permukaan_t *src)
{
        pg_kotak_t dr, sr;
        int y;
        if (!dst || !src) return;
        sr = pg_buat_kotak(0, 0, src->lebar, src->tinggi);
        dr = pg_buat_kotak(dx, dy, src->lebar, src->tinggi);
        dr = pg_potong_ke_permukaan(dst, dr);
        if (PG_KOTAK_KOSONG(dr)) return;
        sr.x += dr.x - dx;
        sr.y += dr.y - dy;
        sr.w = dr.w;
        sr.h = dr.h;

        for (y = 0; y < dr.h; y++) {
                pg_warna_t *drow = (pg_warna_t *)((char *)dst->piksel +
                                  (size_t)(dr.y + y) * dst->langkah);
                const pg_warna_t *srow = (const pg_warna_t *)
                                  ((const char *)src->piksel +
                                  (size_t)(sr.y + y) * src->langkah);
                int x;
                for (x = 0; x < dr.w; x++) {
                        pg_warna_t sc = srow[sr.x + x];
                        int sa = (int)((sc >> 24) & 0xff);
                        if (sa == 0) continue;
                        if (sa == 255) {
                                drow[dr.x + x] = sc;
                                continue;
                        }
                        {
                                pg_warna_t dc = drow[dr.x + x];
                                int ia = 255 - sa;
                                int r = (PG_R(sc) * sa + PG_R(dc) * ia) / 255;
                                int g = (PG_G(sc) * sa + PG_G(dc) * ia) / 255;
                                int b = (PG_B(sc) * sa + PG_B(dc) * ia) / 255;
                                int a = sa + (PG_A(dc) * ia) / 255;
                                drow[dr.x + x] = PG_RGBA(r, g, b, a);
                        }
                }
        }
        pg_permukaan_kotor(dst, dr);
}

void pg_blit_sub_permukaan(pg_permukaan_t *dst, int dx, int dy,
                            const pg_permukaan_t *src, pg_kotak_t sr)
{
        pg_kotak_t dr;
        int y;
        if (!dst || !src) return;
        if (sr.x < 0) { dx -= sr.x; sr.w += sr.x; sr.x = 0; }
        if (sr.y < 0) { dy -= sr.y; sr.h += sr.y; sr.y = 0; }
        if (sr.x + sr.w > src->lebar)  sr.w = src->lebar  - sr.x;
        if (sr.y + sr.h > src->tinggi) sr.h = src->tinggi - sr.y;
        if (sr.w <= 0 || sr.h <= 0) return;
        dr = pg_buat_kotak(dx, dy, sr.w, sr.h);
        dr = pg_potong_ke_permukaan(dst, dr);
        if (PG_KOTAK_KOSONG(dr)) return;

        for (y = 0; y < dr.h; y++) {
                pg_warna_t *drow = (pg_warna_t *)((char *)dst->piksel +
                                  (size_t)(dr.y + y) * dst->langkah);
                const pg_warna_t *srow = (const pg_warna_t *)
                                  ((const char *)src->piksel +
                                  (size_t)(sr.y + y) * src->langkah);
                int x;
                for (x = 0; x < dr.w; x++) {
                        pg_warna_t sc = srow[sr.x + x];
                        int sa = PG_A(sc);
                        if (sa == 0) continue;  /* transparan: skip */
                        if (sa == 255) {
                                drow[dr.x + x] = sc;
                        } else {
                                pg_warna_t dc = drow[dr.x + x];
                                int ia = 255 - sa;
                                int r = (PG_R(sc)*sa + PG_R(dc)*ia)/255;
                                int g = (PG_G(sc)*sa + PG_G(dc)*ia)/255;
                                int b = (PG_B(sc)*sa + PG_B(dc)*ia)/255;
                                int a = sa + (PG_A(dc)*ia)/255;
                                drow[dr.x + x] = PG_RGBA(r,g,b,a);
                        }
                }
        }
        pg_permukaan_kotor(dst, dr);
}

void pg_permukaan_kotor(pg_permukaan_t *s, pg_kotak_t r)
{
        pg_kotak_t u;
        if (!s) return;
        if (PG_KOTAK_KOSONG(r)) return;
        if (PG_KOTAK_KOSONG(s->kotor)) {
                s->kotor = r;
                return;
        }
        u.x = PG_MIN(s->kotor.x, r.x);
        u.y = PG_MIN(s->kotor.y, r.y);
        {
                int x1 = PG_MAX(s->kotor.x + s->kotor.w, r.x + r.w);
                int y1 = PG_MAX(s->kotor.y + s->kotor.h, r.y + r.h);
                u.w = x1 - u.x;
                u.h = y1 - u.y;
        }
        s->kotor = u;
}

pg_kotak_t pg_permukaan_ambil_kotor(const pg_permukaan_t *s)
{
        return s ? s->kotor : pg_buat_kotak(0, 0, 0, 0);
}

void pg_permukaan_bersih_kotor(pg_permukaan_t *s)
{
        if (!s) return;
        s->kotor = pg_buat_kotak(0, 0, 0, 0);
}
