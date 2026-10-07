/* ----------------------------------------------------------------------------------------------
 * pigura gambar: gambar.c - primitif gambar anti-aliasing
 * ----------------------------------------------------------------------------------------------
 * Implementasi primitif gambar AA dengan kualitas mendekati Cairo.
 * Garis (Wu) dan Bezier pakai float untuk presisi sub-piksel
 * (8-bit fractional fixed-point lama menyebabkan staircase pada
 * gradient=1.0). Fixed-point 16.16 hanya dipakai di API publik
 * untuk koordinat sub-piksel; lingkaran & poligon tetap pakai
 * integer/float sesuai kebutuhan.
 *
 * Algoritma:
 *   - Garis AA: Xiaolin Wu (coverage sub-piksel, float)
 *   - Lingkaran AA: midpoint + AA edge (filter tabung 2 piksel)
 *   - Poligon AA: scanline fill dengan sub-piksel coverage + AA edge
 *   - Bezier: de Casteljau recursive subdivision (float) sampai
 *     flatness < 1 piksel
 *
 * Coverage 0..255 → alpha blend manual (tidak butuh lib alpha).
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/gambar.h"
#include "pigura/permukaan.h"
#include "pigura/galat.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ===================================================================
 * Util blend coverage
 * =================================================================== */

pg_warna_t pg_blend_coverage(pg_warna_t bg, pg_warna_t fg, int alpha)
{
        int r, g, b, a;
        int fa;
        if (alpha <= 0)   return bg;
        fa = (alpha * PG_A(fg)) / 255;
        if (fa >= 255) return fg;
        if (fa <= 0)   return bg;
        {
                int ia = 255 - fa;
                r = (PG_R(fg) * fa + PG_R(bg) * ia) / 255;
                g = (PG_G(fg) * fa + PG_G(bg) * ia) / 255;
                b = (PG_B(fg) * fa + PG_B(bg) * ia) / 255;
                /* Composite alpha: bila bg transparan (alpha=0),
                 * result alpha = fa (semi-transparan). Bila bg opaque
                 * (alpha=255), result alpha = 255 (opaque).
                 * Ini membuat AA edge di permukaan transparan
                 * menjadi semi-transparan → smooth saat di-blit
                 * ke parent. */
                a = fa + (PG_A(bg) * ia) / 255;
        }
        return PG_RGBA(r, g, b, a);
}

/* Setel piksel dengan coverage 0..255 di permukaan (terpotong). */
static void pg_setel_piksel_aa(pg_permukaan_t *s, int x, int y,
                                int alpha, pg_warna_t c)
{
        int lebar, tinggi, langkah;
        pg_warna_t *p;
        if (!s) return;
        lebar  = pg_permukaan_lebar(s);
        tinggi = pg_permukaan_tinggi(s);
        langkah = pg_permukaan_langkah(s);
        if (x < 0 || y < 0 || x >= lebar || y >= tinggi) return;
        if (alpha <= 0) return;
        p = (pg_warna_t *)((char *)pg_permukaan_piksel_mut(s) +
                            (size_t)y * langkah);
        /* Jangan timpa langsung bila: fg semi-transparan, atau
         * bg masih transparan (alpha < 255). Kalau langsung
         * timpa, edge AA tidak ada gradasi di permukaan
         * transparan → jagged saat di-blit ke parent. */
        if (alpha >= 255 && PG_A(c) >= 255 && PG_A(p[x]) >= 255) {
                p[x] = c;
        } else {
                p[x] = pg_blend_coverage(p[x], c, alpha);
        }
        pg_permukaan_kotor(s, pg_buat_kotak(x, y, 1, 1));
}

/* ===================================================================
 * Garis AA — algoritma Xiaolin Wu (float)
 * ===================================================================
 */

/* Garis AA Xiaolin Wu.
 *
 * Konversi fixed 16.16 → float di awal, lalu seluruh perhitungan
 * (gradient, intery, alpha) di float. Float memberi presisi penuh
 * untuk semua gradient, termasuk diagonal 45° (gradient=1.0)
 * di mana fixed 8-bit fractional lama menghasilkan alpha 255/0
 * gantian (staircase) alih-alih 128/128 yang halus.
 *
 * Konvensi: pg_setel_piksel_aa(s, x, y, ...) menerima (x=kolom,
 * y=baris) di koordinat layar asli. Untuk garis curam (steep),
 * x dan y di-swap di awal supaya algoritma selalu iterasi ke
 * arah sumbu panjang. Saat plot, koordinat di-unswap balik
 * supaya piksel jatuh di posisi (kolom, baris) yang benar. */
void pg_gambar_garis_aa(pg_permukaan_t *s,
                         pg_fixed_t x0f, pg_fixed_t y0f,
                         pg_fixed_t x1f, pg_fixed_t y1f,
                         pg_warna_t c)
{
        float x0, y0, x1, y1, dx, dy, gradient;
        float xend, yend, xgap, fpart, intery;
        int steep, xpxl1, ypxl1, xpxl2, ypxl2, x;

        if (!s) return;
        x0 = PG_FIXED_KE_FLOAT(x0f);
        y0 = PG_FIXED_KE_FLOAT(y0f);
        x1 = PG_FIXED_KE_FLOAT(x1f);
        y1 = PG_FIXED_KE_FLOAT(y1f);

        /* Steep: |dy| > |dx|. */
        steep = fabsf(y1 - y0) > fabsf(x1 - x0);

        /* Jika curam, swap x dan y supaya iterasi selalu ke
         * arah sumbu panjang (horizontal). */
        if (steep) {
                float t = x0; x0 = y0; y0 = t;
                t = x1; x1 = y1; y1 = t;
        }

        /* Urutkan supaya x0 <= x1. */
        if (x0 > x1) {
                float t = x0; x0 = x1; x1 = t;
                t = y0; y0 = y1; y1 = t;
        }

        dx = x1 - x0;
        dy = y1 - y0;
        gradient = (dx == 0.0f) ? 1.0f : dy / dx;

        /* ---- Endpoint pertama (x0) ---- */
        xend = roundf(x0);
        yend = y0 + gradient * (xend - x0);
        xgap = 1.0f - (x0 + 0.5f - floorf(x0 + 0.5f));
        xpxl1 = (int)xend;
        ypxl1 = (int)floorf(yend);
        fpart = yend - floorf(yend);
        {
                int a0 = (int)(sqrtf((1.0f - fpart) * xgap) * 255.0f);
                int a1 = (int)(sqrtf(fpart * xgap) * 255.0f);
                if (steep) {
                        /* Unswap: kolom=ypxl1, baris=xpxl1. */
                        pg_setel_piksel_aa(s, ypxl1,     xpxl1, a0, c);
                        pg_setel_piksel_aa(s, ypxl1 + 1, xpxl1, a1, c);
                } else {
                        pg_setel_piksel_aa(s, xpxl1, ypxl1,     a0, c);
                        pg_setel_piksel_aa(s, xpxl1, ypxl1 + 1, a1, c);
                }
        }
        intery = yend + gradient;

        /* ---- Endpoint kedua (x1) ---- */
        xend = roundf(x1);
        yend = y1 + gradient * (xend - x1);
        xgap = (x1 + 0.5f) - floorf(x1 + 0.5f);
        xpxl2 = (int)xend;
        ypxl2 = (int)floorf(yend);
        fpart = yend - floorf(yend);
        {
                int a0 = (int)(sqrtf((1.0f - fpart) * xgap) * 255.0f);
                int a1 = (int)(sqrtf(fpart * xgap) * 255.0f);
                if (steep) {
                        pg_setel_piksel_aa(s, ypxl2,     xpxl2, a0, c);
                        pg_setel_piksel_aa(s, ypxl2 + 1, xpxl2, a1, c);
                } else {
                        pg_setel_piksel_aa(s, xpxl2, ypxl2,     a0, c);
                        pg_setel_piksel_aa(s, xpxl2, ypxl2 + 1, a1, c);
                }
        }

        /* ---- Badan garis ---- */
        for (x = xpxl1 + 1; x < xpxl2; x++) {
                int yi = (int)floorf(intery);
                float fp = intery - floorf(intery);
                int a0 = (int)(sqrtf(1.0f - fp) * 255.0f);
                int a1 = (int)(sqrtf(fp) * 255.0f);
                if (steep) {
                        /* Unswap: kolom=yi, baris=x. */
                        pg_setel_piksel_aa(s, yi,     x, a0, c);
                        pg_setel_piksel_aa(s, yi + 1, x, a1, c);
                } else {
                        pg_setel_piksel_aa(s, x, yi,     a0, c);
                        pg_setel_piksel_aa(s, x, yi + 1, a1, c);
                }
                intery += gradient;
        }
}

/* Versi integer pembungkus — menerima koordinat biasa. */
static pg_fixed_t pg_to_fixed(int v)
{
        return (pg_fixed_t)v << 16;
}

/* ===================================================================
 * Garis AA tebal — approximasi polygon berisi stroke
 * =================================================================== */

void pg_gambar_garis_aa_tebal(pg_permukaan_t *s,
                               pg_fixed_t x0, pg_fixed_t y0,
                               pg_fixed_t x1, pg_fixed_t y1,
                               pg_fixed_t lebar, pg_warna_t c)
{
        pg_fixed_t dx, dy, len, nx, ny, hx, hy;
        pg_titik_fixed_t p[4];

        if (!s) return;
        if (lebar <= 0x10000) {
                pg_gambar_garis_aa(s, x0, y0, x1, y1, c);
                return;
        }

        dx = x1 - x0;
        dy = y1 - y0;
        /* Panjang vektor (fixed 16.16). */
        {
                pg_s64 dxs = (pg_s64)dx * dx + (pg_s64)dy * dy;
                /* sqrt approximasi: akar integer 32-bit. */
                pg_u32 v = (pg_u32)(dxs >> 16);
                pg_u32 r = 0;
                pg_u32 bit;
                for (bit = 1u << 30; bit; bit >>= 2) {
                        pg_u32 t = r + bit;
                        if (v >= t) { v -= t; r = (r >> 1) + bit; }
                        else r >>= 1;
                }
                len = (pg_fixed_t)r << 16;
                if (len == 0) {
                        pg_gambar_garis_aa(s, x0, y0, x1, y1, c);
                        return;
                }
        }

        /* Normal (perpendicular). */
        nx = (pg_fixed_t)(((pg_s64)-dy << 16) / (len >> 16));
        ny = (pg_fixed_t)(((pg_s64) dx << 16) / (len >> 16));
        hx = (nx * (lebar >> 1)) >> 16;
        hy = (ny * (lebar >> 1)) >> 16;

        p[0].x = x0 - hx; p[0].y = y0 - hy;
        p[1].x = x1 - hx; p[1].y = y1 - hy;
        p[2].x = x1 + hx; p[2].y = y1 + hy;
        p[3].x = x0 + hx; p[3].y = y0 + hy;

        /* Approximasi: isi polygon. */
        {
                pg_titik_t t[4];
                int i;
                for (i = 0; i < 4; i++) {
                        t[i].x = PG_KE_INT(p[i].x);
                        t[i].y = PG_KE_INT(p[i].y);
                }
                pg_gambar_poligon_isi_aa(s, t, 4, c);
        }
}

/* ===================================================================
 * Kotak AA — outline dengan tepi AA
 * =================================================================== */

void pg_gambar_kotak_aa(pg_permukaan_t *s, pg_kotak_t r, pg_warna_t c)
{
        /* Gambar 4 garis solid 1px. Tidak pakai Wu line karena
         * Wu di titik integer kasih alpha ujung ~127, dan tiap
         * sudut digambar 2x (intersection) -> alpha dobel ->
         * kelihatan seperti bevel di pojok. Pakai garis solid
         * supaya outline utuh, tidak ada gradasi di sudut. */
        if (!s || PG_KOTAK_KOSONG(r)) return;
        pg_garis_h_permukaan(s, r.x, r.x+r.w, r.y, c);
        pg_garis_h_permukaan(s, r.x, r.x+r.w, r.y+r.h-1, c);
        pg_garis_v_permukaan(s, r.x, r.y, r.y+r.h, c);
        pg_garis_v_permukaan(s, r.x+r.w-1, r.y, r.y+r.h, c);
}

/* ===================================================================
 * Lingkaran & ellipse AA
 * =================================================================== */

void pg_gambar_lingkaran_aa(pg_permukaan_t *s,
                             int cx, int cy, int radius,
                             pg_warna_t c)
{
        pg_gambar_ellipse_aa(s, cx, cy, radius, radius, c);
}

void pg_gambar_lingkaran_isi_aa(pg_permukaan_t *s,
                                  int cx, int cy, int radius,
                                  pg_warna_t isi, pg_warna_t tepi)
{
        /* Fill dengan alpha halus di tepi. Jangan render outline
         * terpisah karena akan menyebabkan double-blend halo. */
        (void)tepi;
        pg_gambar_ellipse_isi_aa(s, cx, cy, radius, radius, isi);
}

/* Lingkaran/ellipse AA: supersampling adaptif + sqrt gamma.
 *
 * SS adaptif:
 *   - rx atau ry < 20 (radio kecil, contoh: dot di widget radio)
 *     pakai 8x8 = 64 sample → AA halus di arc sempit.
 *   - Besar pakai 4x4 = 16 sample (cukup, lebih cepat).
 *
 * Coverage per sub-piksel:
 *   - Hitung d = fx²·inv_rx² + fy²·inv_ry² (normalized squared
 *     distance ke pusat; d=1 = boundary).
 *   - dist = |d - 1| untuk outline (kedua sisi boundary),
 *     atau max(0, d-1) untuk fill (hanya sisi luar).
 *   - Local gradient |∇d| ≈ 2·sqrt((fx/rx²)² + (fy/ry²)²).
 *   - e = edge_band · |∇d| → konversi 1px band ke d-space.
 *   - cov = clamp(1 - dist/e, 0, 1).
 *
 * Gamma:
 *   - coverage = Σcov / (SS·SS) (linear, 0..1)
 *   - alpha = sqrt(coverage) · 255  → gamma sRGB supaya tidak bold
 *
 * Band: edge_band = 1.0 piksel (constant). Tidak pakai 0.2 (terlalu
 * tipis → staircase untuk r<10). Tidak pakai 1/rx+1/ry (itu ekuivalen
 * dengan edge_band=0.5 setengah-gradient — terlalu tipis juga).
 */
static void pg_gambar_ellipse_aa_int(pg_permukaan_t *s,
                                       int cx, int cy,
                                       int rx, int ry,
                                       pg_warna_t c,
                                       pg_bool isi)
{
        const int SS = (rx < 20 || ry < 20) ? 8 : 4;
        const float edge_band = 1.0f;
        int x, y;
        float inv_rx2, inv_ry2;
        if (!s || rx <= 0 || ry <= 0) return;
        inv_rx2 = 1.0f / ((float)rx * (float)rx);
        inv_ry2 = 1.0f / ((float)ry * (float)ry);

        for (y = -ry - 1; y <= ry + 1; y++) {
                for (x = -rx - 1; x <= rx + 1; x++) {
                        int total_cov = 0;
                        int sub_x, sub_y;
                        for (sub_y = 0; sub_y < SS; sub_y++) {
                                for (sub_x = 0; sub_x < SS; sub_x++) {
                                        float fx = (float)x +
                                                ((float)sub_x + 0.5f) /
                                                (float)SS;
                                        float fy = (float)y +
                                                ((float)sub_y + 0.5f) /
                                                (float)SS;
                                        float d = fx * fx * inv_rx2 +
                                                  fy * fy * inv_ry2;
                                        float dist;
                                        float gx, gy, grad, e, cov;
                                        if (isi) {
                                                /* FILL: hanya sisi luar
                                                 * yang fade. Dalam = cov 1. */
                                                dist = (d > 1.0f) ?
                                                        (d - 1.0f) : 0.0f;
                                        } else {
                                                /* OUTLINE: kedua sisi
                                                 * boundary fade. */
                                                dist = fabsf(d - 1.0f);
                                        }
                                        /* Local gradient |∇d| di pixel
                                         * space. ∂d/∂fx = 2·fx/rx²,
                                         * jadi |∇d| = 2·sqrt(gx²+gy²). */
                                        gx = fx * inv_rx2;
                                        gy = fy * inv_ry2;
                                        grad = 2.0f * sqrtf(gx * gx +
                                                            gy * gy);
                                        e = edge_band * grad;
                                        if (e < 0.001f) e = 0.001f;
                                        cov = 1.0f - dist / e;
                                        if (cov < 0.0f) cov = 0.0f;
                                        if (cov > 1.0f) cov = 1.0f;
                                        total_cov += (int)(cov * 256.0f);
                                }
                        }
                        if (total_cov > 0) {
                                float coverage = (float)total_cov /
                                        (float)(SS * SS * 256);
                                float alpha_f = sqrtf(coverage);
                                int alpha = (int)(alpha_f * 255.0f + 0.5f);
                                if (alpha > 255) alpha = 255;
                                if (alpha > 0) {
                                        pg_setel_piksel_aa(s, cx + x,
                                            cy + y, alpha, c);
                                }
                        }
                }
        }
}

void pg_gambar_ellipse_aa(pg_permukaan_t *s,
                           int cx, int cy,
                           int rx, int ry,
                           pg_warna_t c)
{
        pg_gambar_ellipse_aa_int(s, cx, cy, rx, ry, c, PG_SALAH);
}

void pg_gambar_ellipse_isi_aa(pg_permukaan_t *s,
                                int cx, int cy,
                                int rx, int ry,
                                pg_warna_t isi)
{
        pg_gambar_ellipse_aa_int(s, cx, cy, rx, ry, isi, PG_BENAR);
}

/* ===================================================================
 * Busur (arc) AA — sebagian lingkaran
 * ===================================================================
 *
 * Strategi: untuk setiap piksel dalam bounding box arc, hitung
 * jarak ke lingkaran ideal (distance field). Bila dalam range
 * edge (|d-1| < edge/radius), hitung alpha. Lalu cek apakah
 * sudut piksel berada dalam range [sudut_mulai, sudut_akhir].
 *
 * Edge width ~1 piksel, gamma sqrt untuk AA halus (sama seperti
 * ellipse outline).
 */

/* Normalisasi sudut ke range [-PI, PI]. */
static float pg_norm_sudut(float a)
{
        while (a >  3.14159265f) a -= 6.28318530f;
        while (a < -3.14159265f) a += 6.28318530f;
        return a;
}

/* Cek apakah sudut 'a' berada dalam range [s0, s1] (searah jarum jam).
 * Range boleh lebih dari 2*PI (mis. -PI/4 .. 5*PI/4 = 1.5 putaran).
 * Algoritma: shift s0 ke 0, lalu cek a-shift dalam [0, s1-s0]. */
static int pg_sudut_dalam_range(float a, float s0, float s1)
{
        float span = s1 - s0;
        float rel;
        if (span <= 0.0f) return 0;
        if (span >= 6.28318530f) return 1; /* full circle */
        rel = a - s0;
        /* Wrap ke [0, 2*PI). */
        while (rel < 0.0f)        rel += 6.28318530f;
        while (rel >= 6.28318530f) rel -= 6.28318530f;
        return rel <= span;
}

void pg_gambar_busur_aa(pg_permukaan_t *s, int cx, int cy,
                        int radius,
                        float sudut_mulai, float sudut_akhir,
                        pg_warna_t c)
{
        int x, y;
        int r_int;
        float r, inv_r2, edge;
        if (!s || radius <= 0) return;

        /* Normalisasi range. */
        sudut_mulai  = pg_norm_sudut(sudut_mulai);
        sudut_akhir  = pg_norm_sudut(sudut_akhir);
        /* Bila sudut_akhir < sudut_mulai, berarti span melewati PI.
         * Tambah 2*PI supaya span positif. */
        if (sudut_akhir < sudut_mulai)
                sudut_akhir += 6.28318530f;

        r = (float)radius;
        inv_r2 = 1.0f / (r * r);
        r_int = radius;
        /* Edge width SAMA dengan ellipse outline: 1/rx + 1/ry = 2/r
         * (untuk circle, rx = ry = r). Sebelumnya 1/r, terlalu tipis. */
        edge = 2.0f / r;

        /* Bounding box: cx-r..cx+r, cy-r..cy+r. */
        for (y = -r_int - 1; y <= r_int + 1; y++) {
                for (x = -r_int - 1; x <= r_int + 1; x++) {
                        float fx, fy, d, dist, cov;
                        float sudut;
                        int alpha;
                        fx = (float)x + 0.5f;
                        fy = (float)y + 0.5f;
                        /* Distance field: SAMA dengan ellipse rx=ry=r. */
                        d = (fx * fx + fy * fy) * inv_r2;
                        dist = fabsf(d - 1.0f);
                        cov = 1.0f - dist / edge;
                        if (cov <= 0.0f) continue;
                        if (cov > 1.0f) cov = 1.0f;
                        /* Cek sudut piksel. */
                        sudut = atan2f(fy, fx); /* [-PI, PI] */
                        if (!pg_sudut_dalam_range(sudut, sudut_mulai,
                                                   sudut_akhir))
                                continue;
                        alpha = (int)(sqrtf(cov) * 255.0f + 0.5f);
                        if (alpha > 255) alpha = 255;
                        pg_setel_piksel_aa(s, cx + x, cy + y, alpha, c);
                }
        }
}

/* ===================================================================
 * Kotak rounded AA — distance field + supersampling
 * ===================================================================
 *
 * Strategi (KONSISTEN dengan ellipse AA):
 *   OUTLINE: distance field + gamma sqrt
 *     - Hitung signed distance ke boundary rounded rect
 *     - cov = 1 - |distance| / edge_width
 *     - alpha = sqrt(cov) * 255
 *
 *   FILL: supersampling 4x4 + gamma sqrt
 *     - 16 sub-piksel per piksel
 *     - total = jumlah sub-piksel yang inside (distance <= 0)
 *     - coverage = total / 16
 *     - alpha = sqrt(coverage) * 255
 *
 * Helper: pg_round_rect_distance() hitung signed distance ke
 * boundary rounded rect. Konvensi: <=0 = di dalam, >0 = di luar.
 */

/* Hitung signed distance dari titik (fx, fy) ke boundary rounded
 * rect. fx, fy dalam koordinat lokal (relatif ke x0,y0).
 * w, h = lebar/tinggi kotak. rad = radius pojok.
 * Return: <=0 = di dalam, >0 = di luar, 0 = di boundary. */
static float pg_round_rect_distance(float fx, float fy,
                                     int w, int h, int rad)
{
        float dx_l, dx_r, dy_t, dy_b;
        float d_edge;
        /* Jarak ke 4 tepi kotak (positif bila di luar).
         * Pakai w/h bukan w-1/h-1 supaya box tidak kekecilan
         * 1px di kanan-bawah, dan arc tidak geser. */
        dx_l = -fx;
        dx_r = fx - (float)w;
        dy_t = -fy;
        dy_b = fy - (float)h;
        /* Cek region pojok (4 region). Bila di region pojok, hitung
         * jarak ke arc pojok. */
        if (fx < rad && fy < rad) {
                /* TL: pusat arc di (rad, rad) */
                float dx = fx - (float)rad;
                float dy = fy - (float)rad;
                return sqrtf(dx * dx + dy * dy) - (float)rad;
        }
        if (fx >= (float)(w - rad) && fy < rad) {
                /* TR: pusat arc di (w-rad, rad) */
                float dx = fx - (float)(w - rad);
                float dy = fy - (float)rad;
                return sqrtf(dx * dx + dy * dy) - (float)rad;
        }
        if (fx >= (float)(w - rad) && fy >= (float)(h - rad)) {
                /* BR: pusat arc di (w-rad, h-rad) */
                float dx = fx - (float)(w - rad);
                float dy = fy - (float)(h - rad);
                return sqrtf(dx * dx + dy * dy) - (float)rad;
        }
        if (fx < rad && fy >= (float)(h - rad)) {
                /* BL: pusat arc di (rad, h-rad) */
                float dx = fx - (float)rad;
                float dy = fy - (float)(h - rad);
                return sqrtf(dx * dx + dy * dy) - (float)rad;
        }
        /* Tengah (di luar region pojok): signed distance = max dari
         * 4 jarak tepi. Bila semua negatif, artinya di dalam kotak
         * (jarak ke tepi terdekat). */
        d_edge = dx_l;
        if (dx_r > d_edge) d_edge = dx_r;
        if (dy_t > d_edge) d_edge = dy_t;
        if (dy_b > d_edge) d_edge = dy_b;
        return d_edge;
}

void pg_gambar_kotak_tumpul_aa(pg_permukaan_t *s, pg_kotak_t r,
                               int radius, pg_warna_t c)
{
        int x0, y0, x1, y1;
        int w, h, rad;
        int x, y;
        float edge;
        if (!s) return;
        x0 = r.x;  y0 = r.y;
        x1 = r.x + r.w;  y1 = r.y + r.h;
        if (x1 <= x0 || y1 <= y0) return;
        w = x1 - x0;  h = y1 - y0;
        rad = radius;
        if (rad < 0) rad = 0;
        if (rad > w / 2) rad = w / 2;
        if (rad > h / 2) rad = h / 2;
        /* Bila radius 0, fallback ke kotak tajam. */
        if (rad == 0) {
                pg_gambar_kotak_aa(s, r, c);
                return;
        }
        /* Edge width 0.5 supaya outline 1px tajam utuh.
         * edge=1.0 + sqrtf bikin outline 2px + gamma terlalu lembut.
         * edge=0.5 + linear (tanpa sqrtf) = 1px tajam. */
        edge = 0.5f;
        for (y = -1; y <= h; y++) {
                for (x = -1; x <= w; x++) {
                        float fx, fy, d, dist, cov;
                        int alpha;
                        int lx, ly;
                        fx = (float)x + 0.5f;
                        fy = (float)y + 0.5f;
                        d = pg_round_rect_distance(fx, fy, w, h, rad);
                        dist = fabsf(d);
                        cov = 1.0f - dist / edge;
                        if (cov <= 0.0f) continue;
                        if (cov > 1.0f) cov = 1.0f;
                        alpha = (int)(cov * 255.0f + 0.5f);
                        if (alpha > 255) alpha = 255;
                        lx = x0 + x;
                        ly = y0 + y;
                        pg_setel_piksel_aa(s, lx, ly, alpha, c);
                }
        }
}

void pg_gambar_kotak_tumpul_isi_aa(pg_permukaan_t *s, pg_kotak_t r,
                                    int radius, pg_warna_t isi)
{
        const int SS = 4;
        int x0, y0, x1, y1;
        int w, h, rad;
        int x, y;
        if (!s) return;
        x0 = r.x;  y0 = r.y;
        x1 = r.x + r.w;  y1 = r.y + r.h;
        if (x1 <= x0 || y1 <= y0) return;
        w = x1 - x0;  h = y1 - y0;
        rad = radius;
        if (rad < 0) rad = 0;
        if (rad > w / 2) rad = w / 2;
        if (rad > h / 2) rad = h / 2;
        /* Bila radius 0, fallback ke kotak tajam isi. */
        if (rad == 0) {
                pg_isi_permukaan_kotak(s, r, isi);
                return;
        }
        /* Supersampling 4x4 dengan gamma sqrt (sama seperti ellipse
         * fill). Untuk tiap piksel, hitung berapa sub-piksel yang
         * berada di dalam rounded rect (distance <= 0). */
        for (y = -1; y <= h; y++) {
                for (x = -1; x <= w; x++) {
                        int total = 0;
                        int sub_x, sub_y;
                        int lx, ly;
                        for (sub_y = 0; sub_y < SS; sub_y++) {
                                for (sub_x = 0; sub_x < SS; sub_x++) {
                                        float fx = (float)x +
                                                (sub_x + 0.5f) /
                                                (float)SS;
                                        float fy = (float)y +
                                                (sub_y + 0.5f) /
                                                (float)SS;
                                        float d = pg_round_rect_distance(
                                                fx, fy, w, h, rad);
                                        if (d <= 0.0f) total++;
                                }
                        }
                        if (total > 0) {
                                float coverage = (float)total /
                                        (float)(SS * SS);
                                int alpha;
                                if (coverage > 1.0f) coverage = 1.0f;
                                alpha = (int)(sqrtf(coverage) *
                                               255.0f + 0.5f);
                                if (alpha > 255) alpha = 255;
                                lx = x0 + x;
                                ly = y0 + y;
                                pg_setel_piksel_aa(s, lx, ly,
                                                    alpha, isi);
                        }
                }
        }
}

/* ------------------------------------------------------------------
 * Single-pass fill + outline dengan 4x4 supersampling + sqrt gamma
 * ------------------------------------------------------------------
 * Strategi (mirip Cairo):
 *
 *   Untuk setiap piksel output, sample 16 sub-posisi (4x4 grid).
 *   Untuk tiap sub:
 *     - Hitung d_out (jarak ke outer rounded rect).
 *     - Bila d_out > 0 → sub di luar silhouette → skip (alpha=0).
 *     - Bila d_out <= 0 → sub di dalam:
 *         - Hitung d_in (jarak ke inner rounded rect, offset 1,1).
 *         - Pilih warna: isi (d_in <= 0) atau garis (d_in > 0).
 *         - Akumulasi RGB ke sum_r/g/b.
 *
 *   Setelah loop 16 sub:
 *     - coverage = hitung / 16  (0..1)
 *     - alpha = sqrtf(coverage)  → gamma sRGB
 *     - rgb = sum_rgb / hitung   → rata-rata warna "inside" sub
 *     - Blend ke piksel existing dengan alpha tersebut.
 *
 * Keuntungan:
 *   - Edge silhouette halus: 16 level coverage + gamma.
 *   - Inner edge (isi→garis) ikut halus karena sub di border arc
 *     dapat campuran isi+garis tergantung jumlah sub yang masuk
 *     masing-masing region.
 *   - Tidak double-AA: 1 pass, 1 warna per piksel.
 *
 * Performa: O(w*h*16). Untuk widget 50x50 = 40k SDF eval; 200x200 =
 * 640k. Masih cepat untuk widget UI.
 * ------------------------------------------------------------------ */

/* Helper SDF inner (rounded rect di offset (1,1), ukuran (w-2,h-2),
 * radius (rad-1)). Sama algoritma dengan pg_round_rect_distance
 * tapi dengan parameter inner. */
static float pg_round_rect_inner_sdf(float fx, float fy,
                                      int w, int h, int rad)
{
        float ifx = fx - 1.0f;
        float ify = fy - 1.0f;
        int iw = w - 2;
        int ih = h - 2;
        int irad = rad - 1;
        float dx_l, dx_r, dy_t, dy_b;
        float d_edge;
        if (iw <= 0 || ih <= 0) return -1e9f;  /* seluruh dalam */
        if (irad < 0) irad = 0;
        if (irad > iw / 2) irad = iw / 2;
        if (irad > ih / 2) irad = ih / 2;
        dx_l = -ifx;
        dx_r = ifx - (float)iw;
        dy_t = -ify;
        dy_b = ify - (float)ih;
        if (ifx < irad && ify < irad) {
                float dx = ifx - (float)irad;
                float dy = ify - (float)irad;
                return sqrtf(dx * dx + dy * dy) - (float)irad;
        }
        if (ifx >= (float)(iw - irad) && ify < irad) {
                float dx = ifx - (float)(iw - irad);
                float dy = ify - (float)irad;
                return sqrtf(dx * dx + dy * dy) - (float)irad;
        }
        if (ifx >= (float)(iw - irad) && ify >= (float)(ih - irad)) {
                float dx = ifx - (float)(iw - irad);
                float dy = ify - (float)(ih - irad);
                return sqrtf(dx * dx + dy * dy) - (float)irad;
        }
        if (ifx < irad && ify >= (float)(ih - irad)) {
                float dx = ifx - (float)irad;
                float dy = ify - (float)(ih - irad);
                return sqrtf(dx * dx + dy * dy) - (float)irad;
        }
        d_edge = dx_l;
        if (dx_r > d_edge) d_edge = dx_r;
        if (dy_t > d_edge) d_edge = dy_t;
        if (dy_b > d_edge) d_edge = dy_b;
        return d_edge;
}

void pg_gambar_kotak_tumpul_isi_garis_aa(pg_permukaan_t *s,
                                           pg_kotak_t r,
                                           int radius,
                                           pg_warna_t isi,
                                           pg_warna_t garis)
{
        /* SS adaptif: radius kecil (<20) pakai 8x8 = 64 sub-piksel.
         * Untuk radius 4-6, arc cuma ~25-37px → 4x4 (16 sample)
         * hanya tangkap 4 level coverage → kasar. 8x8 → 16 level → halus.
         * Untuk radius besar, 4x4 sudah cukup dan lebih cepat.
         * Sama seperti ellipse: r<20 → SS=8, else SS=4. */
        const int SS = (radius < 20) ? 8 : 4;
        int x0, y0, x1, y1;
        int w, h, rad;
        int x, y;
        if (!s) return;
        x0 = r.x;  y0 = r.y;
        x1 = r.x + r.w;  y1 = r.y + r.h;
        if (x1 <= x0 || y1 <= y0) return;
        w = x1 - x0;  h = y1 - y0;
        rad = radius;
        if (rad < 0) rad = 0;
        if (rad > w / 2) rad = w / 2;
        if (rad > h / 2) rad = h / 2;

        /* Radius 0: fallback ke solid fill + 4 garis solid.
         * Pakai path ini karena SS tidak menambah kualitas untuk
         * rect tajam (silhouette = straight line, 1px AA sudah
         * cukup dari supersampling natural di piksel boundary).
         * Tapi karena solid fill + solid lines = stabil dan utuh,
         * kita pakai untuk radius 0. */
        if (rad == 0) {
                pg_isi_permukaan_kotak(s, r, isi);
                pg_garis_h_permukaan(s, r.x, r.x + w, r.y, garis);
                pg_garis_h_permukaan(s, r.x, r.x + w, r.y + h - 1, garis);
                pg_garis_v_permukaan(s, r.x, r.y, r.y + h, garis);
                pg_garis_v_permukaan(s, r.x + w - 1, r.y, r.y + h, garis);
                return;
        }

        /* Single pass 4x4 SS untuk tiap piksel.
         * Loop dari -1..w+1 dan -1..h+1 untuk capture edge AA yang
         * mungkin sedikit di luar kotak (untuk smooth blend). */
        for (y = -1; y <= h; y++) {
                for (x = -1; x <= w; x++) {
                        int sx, sy;
                        int hitung = 0;
                        int sum_r = 0, sum_g = 0, sum_b = 0;
                        int lx, ly;
                        for (sy = 0; sy < SS; sy++) {
                                for (sx = 0; sx < SS; sx++) {
                                        float fx = (float)x +
                                                ((float)sx + 0.5f) /
                                                (float)SS;
                                        float fy = (float)y +
                                                ((float)sy + 0.5f) /
                                                (float)SS;
                                        float d_out = pg_round_rect_distance(
                                                fx, fy, w, h, rad);
                                        if (d_out > 0.0f)
                                                continue;  /* outside */
                                        /* Inside outer */
                                        hitung++;
                                        {
                                                float d_in =
                                                        pg_round_rect_inner_sdf(
                                                                fx, fy,
                                                                w, h, rad);
                                                if (d_in <= 0.0f) {
                                                        sum_r += PG_R(isi);
                                                        sum_g += PG_G(isi);
                                                        sum_b += PG_B(isi);
                                                } else {
                                                        sum_r += PG_R(garis);
                                                        sum_g += PG_G(garis);
                                                        sum_b += PG_B(garis);
                                                }
                                        }
                                }
                        }
                        if (hitung == 0)
                                continue;  /* semua sub di luar */
                        {
                                float coverage = (float)hitung /
                                        (float)(SS * SS);
                                float alpha_f = sqrtf(coverage);
                                int alpha;
                                if (alpha_f > 1.0f) alpha_f = 1.0f;
                                alpha = (int)(alpha_f * 255.0f + 0.5f);
                                if (alpha > 255) alpha = 255;
                                if (alpha <= 0) continue;
                                {
                                        int rr = sum_r / hitung;
                                        int gg = sum_g / hitung;
                                        int bb = sum_b / hitung;
                                        lx = x0 + x;
                                        ly = y0 + y;
                                        pg_setel_piksel_aa(s, lx, ly,
                                                            alpha,
                                                            PG_RGB(rr, gg, bb));
                                }
                        }
                }
        }
}

/* ===================================================================
 * Poligon AA — scanline fill
 * =================================================================== */

void pg_gambar_poligon_isi_aa(pg_permukaan_t *s,
                                const pg_titik_t *titik,
                                int jumlah,
                                pg_warna_t c)
{
        int ymin, ymax, y;
        int lebar, tinggi;
        if (!s || !titik || jumlah < 3) return;

        lebar  = pg_permukaan_lebar(s);
        tinggi = pg_permukaan_tinggi(s);

        /* Cari range y. */
        ymin = ymax = titik[0].y;
        for (y = 1; y < jumlah; y++) {
                if (titik[y].y < ymin) ymin = titik[y].y;
                if (titik[y].y > ymax) ymax = titik[y].y;
        }
        if (ymin < 0) ymin = 0;
        if (ymax >= tinggi) ymax = tinggi - 1;

        /* Untuk tiap scanline, cari semua intersection dengan edge. */
        for (y = ymin; y <= ymax; y++) {
                int *xint;
                int count = 0;
                int i, j;
                xint = (int *)malloc(sizeof(int) * jumlah);
                if (!xint) return;
                j = jumlah - 1;
                for (i = 0; i < jumlah; i++) {
                        int yi = titik[i].y;
                        int yj = titik[j].y;
                        if ((yi <= y && yj > y) ||
                            (yj <= y && yi > y)) {
                                int xi = titik[i].x;
                                int xj = titik[j].x;
                                int dx = xj - xi;
                                int dy = yj - yi;
                                int xv;
                                if (dy == 0) dy = 1;
                                xv = xi + (y - yi) * dx / dy;
                                xint[count++] = xv;
                        }
                        j = i;
                }
                /* Sort intersection x ascending (insertion sort). */
                {
                        int a, b;
                        for (a = 1; a < count; a++) {
                                int key = xint[a];
                                b = a - 1;
                                while (b >= 0 && xint[b] > key) {
                                        xint[b + 1] = xint[b];
                                        b--;
                                }
                                xint[b + 1] = key;
                        }
                }
                /* Isi pasangan. */
                for (i = 0; i + 1 < count; i += 2) {
                        int x0 = xint[i];
                        int x1 = xint[i + 1];
                        pg_warna_t *p;
                        int x;
                        if (x0 < 0) x0 = 0;
                        if (x1 >= lebar) x1 = lebar - 1;
                        p = (pg_warna_t *)((char *)pg_permukaan_piksel_mut(s)
                                            + (size_t)y *
                                            pg_permukaan_langkah(s));
                        for (x = x0; x <= x1; x++) {
                                if (x < 0 || x >= lebar) continue;
                                p[x] = c;
                        }
                }
                free(xint);
        }
        pg_permukaan_kotor(s, pg_buat_kotak(0, ymin, lebar,
                                             ymax - ymin + 1));

        /* Garis tepi AA. */
        pg_gambar_poligon_garis_aa(s, titik, jumlah, c);
}

void pg_gambar_poligon_garis_aa(pg_permukaan_t *s,
                                  const pg_titik_t *titik,
                                  int jumlah,
                                  pg_warna_t c)
{
        int i;
        if (!s || !titik || jumlah < 2) return;
        for (i = 0; i < jumlah; i++) {
                int j = (i + 1) % jumlah;
                pg_gambar_garis_aa(s,
                                    pg_to_fixed(titik[i].x),
                                    pg_to_fixed(titik[i].y),
                                    pg_to_fixed(titik[j].x),
                                    pg_to_fixed(titik[j].y),
                                    c);
        }
}

/* ===================================================================
 * Bezier AA — de Casteljau recursive subdivision (float)
 * ===================================================================
 */

/* Sub-bagi rekursif kurva Bezier kubik sampai cukup datar.
 *
 * Pakai float untuk flatness test dan de Casteljau midpoint
 * subdivision. Konversi ke fixed-point hanya saat memanggil
 * pg_gambar_garis_aa (yang sendiri konversi balik ke float
 * internal - roundtrip presisi penuh, loss < 1/65536 px).
 *
 * Flatness: jarak p1 dan p2 dari garis p0-p3, diukur via cross
 * product. Threshold 1.0 = cukup datar; untuk kurva panjang
 * subdivisi lebih dalam otomatis karena cross product besar. */
static void pg_bezier_kubik_sub(pg_permukaan_t *s,
                                 float x0, float y0,
                                 float x1, float y1,
                                 float x2, float y2,
                                 float x3, float y3,
                                 pg_warna_t c,
                                 int kedalaman)
{
        float d1, d2;

        if (kedalaman > 18) return; /* batas rekursi */

        /* Cek flatness: cross product (p3-p0) x (p1-p0) dan
         * (p3-p0) x (p2-p0) - sebanding dengan jarak p1 dan p2
         * dari garis p0-p3. */
        d1 = fabsf((x3 - x0) * (y1 - y0) -
                   (x1 - x0) * (y3 - y0));
        d2 = fabsf((x3 - x0) * (y2 - y0) -
                   (x2 - x0) * (y3 - y0));
        if (d1 + d2 < 1.0f) {
                pg_gambar_garis_aa(s, PG_KE_FIXED(x0), PG_KE_FIXED(y0),
                                    PG_KE_FIXED(x3), PG_KE_FIXED(y3),
                                    c);
                return;
        }

        /* de Casteljau midpoint subdivision. */
        {
                float mx0  = (x0 + x1) * 0.5f;
                float my0  = (y0 + y1) * 0.5f;
                float mx1  = (x1 + x2) * 0.5f;
                float my1  = (y1 + y2) * 0.5f;
                float mx2  = (x2 + x3) * 0.5f;
                float my2  = (y2 + y3) * 0.5f;
                float m01  = (mx0 + mx1) * 0.5f;
                float m01y = (my0 + my1) * 0.5f;
                float m12  = (mx1 + mx2) * 0.5f;
                float m12y = (my1 + my2) * 0.5f;
                float m    = (m01 + m12) * 0.5f;
                float my   = (m01y + m12y) * 0.5f;

                pg_bezier_kubik_sub(s, x0, y0, mx0, my0,
                                     m01, m01y, m, my,
                                     c, kedalaman + 1);
                pg_bezier_kubik_sub(s, m, my, m12, m12y,
                                     mx2, my2, x3, y3,
                                     c, kedalaman + 1);
        }
}

void pg_gambar_bezier_kubik_aa(pg_permukaan_t *s,
                                pg_titik_fixed_t p0,
                                pg_titik_fixed_t p1,
                                pg_titik_fixed_t p2,
                                pg_titik_fixed_t p3,
                                pg_warna_t c)
{
        if (!s) return;
        pg_bezier_kubik_sub(s,
                             PG_FIXED_KE_FLOAT(p0.x),
                             PG_FIXED_KE_FLOAT(p0.y),
                             PG_FIXED_KE_FLOAT(p1.x),
                             PG_FIXED_KE_FLOAT(p1.y),
                             PG_FIXED_KE_FLOAT(p2.x),
                             PG_FIXED_KE_FLOAT(p2.y),
                             PG_FIXED_KE_FLOAT(p3.x),
                             PG_FIXED_KE_FLOAT(p3.y),
                             c, 0);
}

void pg_gambar_bezier_kuadratik_aa(pg_permukaan_t *s,
                                    pg_titik_fixed_t p0,
                                    pg_titik_fixed_t p1,
                                    pg_titik_fixed_t p2,
                                    pg_warna_t c)
{
        /* Konversi kuadratik ke kubik (degree elevation), lalu
         * panggil subdivisi kubik.
         *
         * Bezier kuadratik B(t) = (1-t)^2 P0 + 2(1-t)t P1 +
         * t^2 P2 setara dengan Bezier kubik dengan control:
         *   C0 = P0
         *   C1 = (2 P1 + P0) / 3
         *   C2 = (2 P1 + P2) / 3
         *   C3 = P2 */
        float x0, y0, x1, y1, x2, y2;
        float c1x, c1y, c2x, c2y;

        if (!s) return;
        x0 = PG_FIXED_KE_FLOAT(p0.x);
        y0 = PG_FIXED_KE_FLOAT(p0.y);
        x1 = PG_FIXED_KE_FLOAT(p1.x);
        y1 = PG_FIXED_KE_FLOAT(p1.y);
        x2 = PG_FIXED_KE_FLOAT(p2.x);
        y2 = PG_FIXED_KE_FLOAT(p2.y);

        c1x = (2.0f * x1 + x0) / 3.0f;
        c1y = (2.0f * y1 + y0) / 3.0f;
        c2x = (2.0f * x1 + x2) / 3.0f;
        c2y = (2.0f * y1 + y2) / 3.0f;

        pg_bezier_kubik_sub(s, x0, y0, c1x, c1y, c2x, c2y,
                             x2, y2, c, 0);
}
