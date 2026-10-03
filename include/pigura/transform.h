/* ----------------------------------------------------------------------------------------------
 * pigura/transform.h - Transformasi 2D affine
 * ----------------------------------------------------------------------------------------------
 * pg_transform_t menyandang matriks 2D affine 3x3 (baris-terakhir
 * implisit [0 0 1]):
 *
 *     [ a  c  tx ]
 *     [ b  d  ty ]
 *     [ 0  0   1 ]
 *
 * Sehingga transformasi titik (x, y) menjadi:
 *
 *     x' = a*x + c*y + tx
 *     y' = b*x + d*y + ty
 *
 * Komposisi pg_transform_kali(t1, t2) menghasilkan t1*t2, artinya
 * t2 diterapkan DULU, kemudian t1. Sesuai konvensi matriks standar
 * (OpenCV, Cairo, CSS transform).
 *
 * pg_gambar_transform: render src ke dst dengan transformasi t.
 * Untuk setiap piksel di dst, koordinat src dihitung via inverse
 * transform, lalu bilinear sampling dari src. Piksel di luar
 * bounds src TIDAK di-render (dibiarkan apa adanya di dst).
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_TRANSFORM_H
#define PIGURA_TRANSFORM_H

#include "pigura/tipe.h"
#include "pigura/permukaan.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Matriks 2D affine. */
typedef struct pg_transform {
	float a, b, c, d, tx, ty;
} pg_transform_t;

/* Identitas: a=d=1, b=c=tx=ty=0. */
pg_transform_t pg_transform_identitas(void);

/* Komposisi: hasil = t1 * t2 (t2 diterapkan dulu). */
pg_transform_t pg_transform_kali(pg_transform_t t1,
                                  pg_transform_t t2);

/* Translasi murni: geser (dx, dy). */
pg_transform_t pg_transform_pindah(float dx, float dy);

/* Rotasi murni (radian, berlawanan arah jarum jam). */
pg_transform_t pg_transform_putar(float rad);

/* Skala murni: (sx, sy). */
pg_transform_t pg_transform_skala(float sx, float sy);

/* Transform titik diskrit: kembalikan p' = t(p). */
pg_titik_t pg_transform_titik(pg_transform_t t, pg_titik_t p);

/* Render src ke dst dengan transformasi t.
 *
 * Untuk setiap piksel (dx, dy) di dst, koordinat src dihitung
 * via inverse transform. Bila src (setelah dibulatkan ke 4 tetangga
 * bilinear) valid — minimal 1 tetangga di dalam bounds src — piksel
 * dst di-blend dari sampel bilinear. Bila seluruh tetangga di luar
 * bounds src, piksel dst dibiarkan apa adanya.
 *
 * AA: interpolasi bilinear dari 4 piksel terdekat di src. */
void pg_gambar_transform(pg_permukaan_t *dst,
                          const pg_permukaan_t *src,
                          pg_transform_t t);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_TRANSFORM_H */
