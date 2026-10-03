/* ----------------------------------------------------------------------------------------------
 * pigura/gambar.h - Primitif gambar dengan anti-aliasing
 * ----------------------------------------------------------------------------------------------
 * Fungsi gambar dengan anti-aliasing coverage-based. Kualitas setara
 * Cairo/GTK untuk garis, kotak, lingkaran, poligon, dan kurva Bezier.
 *
 * Algoritma:
 *   - Garis: Xiaolin Wu (coverage sub-piksel)
 *   - Poligon isi: scanline fill dengan sub-piksel coverage
 *   - Lingkaran/ellipse: midpoint + AA edge
 *   - Bezier: recursive subdivision sampai flatness < 0.5 piksel
 *
 * Semua koordinat pakai fixed-point 16.16 internal untuk presisi
 * sub-piksel tanpa floating-point (memenuhi syarat embedded tanpa FPU).
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_GAMBAR_H
#define PIGURA_GAMBAR_H

#include "pigura/tipe.h"
#include "pigura/permukaan.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Tipe titik fixed-point 16.16. */
typedef pg_s32 pg_fixed_t;
#define PG_KE_FIXED(f)  ((pg_fixed_t)((f) * 65536.0 + 0.5))
#define PG_KE_INT(f)    ((int)((f) >> 16))
#define PG_FIXED_KE_FLOAT(f) ((float)(f) / 65536.0f)

typedef struct pg_titik_fixed {
        pg_fixed_t x, y;
} pg_titik_fixed_t;

/* ===== Garis anti-alias ===== */

/* Garis Wu dari (x0,y0) ke (x1,y1), lebar 1 piksel, AA. */
void pg_gambar_garis_aa(pg_permukaan_t *s,
                         pg_fixed_t x0, pg_fixed_t y0,
                         pg_fixed_t x1, pg_fixed_t y1,
                         pg_warna_t c);

/* Garis tebal AA, lebar arbitrer. */
void pg_gambar_garis_aa_tebal(pg_permukaan_t *s,
                               pg_fixed_t x0, pg_fixed_t y0,
                               pg_fixed_t x1, pg_fixed_t y1,
                               pg_fixed_t lebar, pg_warna_t c);

/* ===== Kotak anti-alias ===== */

/* Outline kotak dengan AA di sudut. */
void pg_gambar_kotak_aa(pg_permukaan_t *s, pg_kotak_t r, pg_warna_t c);

/* ===== Lingkaran & ellipse anti-alias ===== */

/* Lingkaran AA penuh. */
void pg_gambar_lingkaran_aa(pg_permukaan_t *s,
                             int cx, int cy, int radius,
                             pg_warna_t c);

/* Lingkaran AA diisi. */
void pg_gambar_lingkaran_isi_aa(pg_permukaan_t *s,
                                  int cx, int cy, int radius,
                                  pg_warna_t isi, pg_warna_t tepi);

/* Ellipse AA outline. */
void pg_gambar_ellipse_aa(pg_permukaan_t *s,
                           int cx, int cy,
                           int rx, int ry,
                           pg_warna_t c);

/* Ellipse AA diisi. */
void pg_gambar_ellipse_isi_aa(pg_permukaan_t *s,
                                int cx, int cy,
                                int rx, int ry,
                                pg_warna_t isi);

/* ===== Poligon anti-alias ===== */

/* Poligon isi (scanline fill + AA edge). */
void pg_gambar_poligon_isi_aa(pg_permukaan_t *s,
                                const pg_titik_t *titik,
                                int jumlah,
                                pg_warna_t c);

/* Outline poligon AA. */
void pg_gambar_poligon_garis_aa(pg_permukaan_t *s,
                                  const pg_titik_t *titik,
                                  int jumlah,
                                  pg_warna_t c);

/* ===== Bezier ===== */

/* Kurva Bezier kubik AA. */
void pg_gambar_bezier_kubik_aa(pg_permukaan_t *s,
                                pg_titik_fixed_t p0,
                                pg_titik_fixed_t p1,
                                pg_titik_fixed_t p2,
                                pg_titik_fixed_t p3,
                                pg_warna_t c);

/* Kurva Bezier kuadratik AA. */
void pg_gambar_bezier_kuadratik_aa(pg_permukaan_t *s,
                                    pg_titik_fixed_t p0,
                                    pg_titik_fixed_t p1,
                                    pg_titik_fixed_t p2,
                                    pg_warna_t c);

/* Busur (arc) AA — sebagian lingkaran dari sudut_mulai ke sudut_akhir
 * (radian). Berguna untuk gambar pie chart, round corner, dsb.
 * cx,cy = pusat; radius = jari-jari.
 * sudut_mulai..sudut_akhir berputar searah jarum jam dari sumbu X+.
 * Contoh: sudut 0 = kanan, PI/2 = bawah, PI = kiri, -PI/2 = atas. */
void pg_gambar_busur_aa(pg_permukaan_t *s, int cx, int cy,
                        int radius,
                        float sudut_mulai, float sudut_akhir,
                        pg_warna_t c);

/* Kotak rounded AA (outline). Radius seragam di 4 pojok.
 * Bila radius <= 0, fallback ke kotak tajam biasa.
 * Bila radius > min(w,h)/2, di-clamp otomatis. */
void pg_gambar_kotak_tumpul_aa(pg_permukaan_t *s, pg_kotak_t r,
                               int radius, pg_warna_t c);

/* Kotak rounded AA (fill). Sama seperti di atas tapi diisi penuh. */
void pg_gambar_kotak_tumpul_isi_aa(pg_permukaan_t *s, pg_kotak_t r,
                                    int radius, pg_warna_t isi);

/* ===== Util ===== */

/* BlendCoverage: campur warna foreground ke background dengan alpha. */
pg_warna_t pg_blend_coverage(pg_warna_t bg, pg_warna_t fg, int alpha);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_GAMBAR_H */
