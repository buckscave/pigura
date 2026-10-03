/* ----------------------------------------------------------------------------------------------
 * pigura gambar: transform.c - transformasi 2D affine + blit bilinear
 * ----------------------------------------------------------------------------------------------
 * pg_transform_t menyandang matriks affine 2D:
 *
 *     [ a  c  tx ]
 *     [ b  d  ty ]
 *     [ 0  0   1 ]
 *
 * Titik (x, y) dipetakan ke:
 *
 *     x' = a*x + c*y + tx
 *     y' = b*x + d*y + ty
 *
 * pg_gambar_transform: untuk setiap piksel di dst, koordinat src
 * dihitung via inverse transform, lalu bilinear sample 4 tetangga.
 * Piksel di luar bounds src TIDAK ditulis ulang (dibiarkan apa
 * adanya).
 *
 * Seluruh perhitungan internal pakai float untuk presisi sub-piksel.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/transform.h"
#include "pigura/permukaan.h"
#include "pigura/galat.h"

#include <stddef.h>
#include <math.h>

/* ===================================================================
 * Konstruktor dasar
 * =================================================================== */

pg_transform_t pg_transform_identitas(void)
{
        pg_transform_t t;
        t.a = 1.0f;
        t.b = 0.0f;
        t.c = 0.0f;
        t.d = 1.0f;
        t.tx = 0.0f;
        t.ty = 0.0f;
        return t;
}

pg_transform_t pg_transform_pindah(float dx, float dy)
{
        pg_transform_t t;
        t.a = 1.0f;
        t.b = 0.0f;
        t.c = 0.0f;
        t.d = 1.0f;
        t.tx = dx;
        t.ty = dy;
        return t;
}

pg_transform_t pg_transform_putar(float rad)
{
        pg_transform_t t;
        float cs = cosf(rad);
        float sn = sinf(rad);
        /* [ cos -sin 0 ; sin cos 0 ; 0 0 1 ] */
        t.a = cs;
        t.b = sn;
        t.c = -sn;
        t.d = cs;
        t.tx = 0.0f;
        t.ty = 0.0f;
        return t;
}

pg_transform_t pg_transform_skala(float sx, float sy)
{
        pg_transform_t t;
        t.a = sx;
        t.b = 0.0f;
        t.c = 0.0f;
        t.d = sy;
        t.tx = 0.0f;
        t.ty = 0.0f;
        return t;
}

/* ===================================================================
 * Komposisi & inverse
 * =================================================================== */

/* Hasil = t1 * t2. Berarti t2 diterapkan dulu, lalu t1. */
pg_transform_t pg_transform_kali(pg_transform_t t1,
                                  pg_transform_t t2)
{
        pg_transform_t r;
        /* Komposisi matriks affine 3x3 (baris terakhir implisit):
         *   [a c tx]   [a c tx]   [a1*a2+c1*b2  a1*c2+c1*d2  ...]
         *   [b d ty] * [b d ty] = [b1*a2+d1*b2  b1*c2+d1*d2  ...]
         *   [0 0  1]   [0 0  1]   [0            0            1  ]
         * translasi: r.tx = a1*tx2 + c1*ty2 + tx1;
         *             r.ty = b1*tx2 + d1*ty2 + ty1. */
        r.a  = t1.a * t2.a + t1.c * t2.b;
        r.b  = t1.b * t2.a + t1.d * t2.b;
        r.c  = t1.a * t2.c + t1.c * t2.d;
        r.d  = t1.b * t2.c + t1.d * t2.d;
        r.tx = t1.a * t2.tx + t1.c * t2.ty + t1.tx;
        r.ty = t1.b * t2.tx + t1.d * t2.ty + t1.ty;
        return r;
}

/* Hitung inverse transform. Bila determinan ~0, kembalikan
 * identitas (caller harus cek determinan dulu bila perlu). */
static pg_transform_t pg_transform_inverse(pg_transform_t t)
{
        pg_transform_t inv;
        float det = t.a * t.d - t.b * t.c;
        float inv_det;
        if (det == 0.0f) return pg_transform_identitas();
        inv_det = 1.0f / det;
        inv.a  = t.d * inv_det;
        inv.b  = -t.b * inv_det;
        inv.c  = -t.c * inv_det;
        inv.d  = t.a * inv_det;
        /* inv.tx = (c*ty - d*tx) / det
         * inv.ty = (b*tx - a*ty) / det */
        inv.tx = (t.c * t.ty - t.d * t.tx) * inv_det;
        inv.ty = (t.b * t.tx - t.a * t.ty) * inv_det;
        return inv;
}

/* ===================================================================
 * Transform titik diskrit
 * =================================================================== */

pg_titik_t pg_transform_titik(pg_transform_t t, pg_titik_t p)
{
        pg_titik_t q;
        float fx = (float)p.x;
        float fy = (float)p.y;
        float qx = t.a * fx + t.c * fy + t.tx;
        float qy = t.b * fx + t.d * fy + t.ty;
        /* Pembulatan terdekat, bukan truncation, supaya titik diskrit
         * yang dipetakan tidak bergeser konsisten ke kiri-atas. */
        q.x = (int)floorf(qx + 0.5f);
        q.y = (int)floorf(qy + 0.5f);
        return q;
}

/* ===================================================================
 * pg_gambar_transform - render src ke dst dengan affine + bilinear
 * =================================================================== */

/* Bilinear sample warna src pada (sx, sy) di bounds src.
 * Piksel di luar bounds di-clamp ke tepi (edge clamp). */
static pg_warna_t pg_transform_bilinear(const pg_warna_t *spik,
                                        int sw, int sh,
                                        int slangkah,
                                        float sx, float sy)
{
        int x0, y0, x1, y1;
        float fx, fy;
        float w00, w10, w01, w11;
        const pg_warna_t *p00, *p10, *p01, *p11;
        float r, g, b;
        pg_u8 cr, cg, cb;

        x0 = (int)floorf(sx);
        y0 = (int)floorf(sy);
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (x0 >= sw) x0 = sw - 1;
        if (y0 >= sh) y0 = sh - 1;
        x1 = x0 + 1;
        y1 = y0 + 1;
        if (x1 >= sw) x1 = sw - 1;
        if (y1 >= sh) y1 = sh - 1;

        fx = sx - (float)x0;
        fy = sy - (float)y0;
        if (fx < 0.0f) fx = 0.0f;
        if (fy < 0.0f) fy = 0.0f;

        w00 = (1.0f - fx) * (1.0f - fy);
        w10 = fx * (1.0f - fy);
        w01 = (1.0f - fx) * fy;
        w11 = fx * fy;

        p00 = (const pg_warna_t *)((const char *)spik +
                (size_t)y0 * slangkah) + x0;
        p10 = (const pg_warna_t *)((const char *)spik +
                (size_t)y0 * slangkah) + x1;
        p01 = (const pg_warna_t *)((const char *)spik +
                (size_t)y1 * slangkah) + x0;
        p11 = (const pg_warna_t *)((const char *)spik +
                (size_t)y1 * slangkah) + x1;

        r = (float)PG_R(*p00) * w00 + (float)PG_R(*p10) * w10 +
            (float)PG_R(*p01) * w01 + (float)PG_R(*p11) * w11;
        g = (float)PG_G(*p00) * w00 + (float)PG_G(*p10) * w10 +
            (float)PG_G(*p01) * w01 + (float)PG_G(*p11) * w11;
        b = (float)PG_B(*p00) * w00 + (float)PG_B(*p10) * w10 +
            (float)PG_B(*p01) * w01 + (float)PG_B(*p11) * w11;

        if (r < 0.0f) r = 0.0f;
        if (g < 0.0f) g = 0.0f;
        if (b < 0.0f) b = 0.0f;
        if (r > 255.0f) r = 255.0f;
        if (g > 255.0f) g = 255.0f;
        if (b > 255.0f) b = 255.0f;

        cr = (pg_u8)(r + 0.5f);
        cg = (pg_u8)(g + 0.5f);
        cb = (pg_u8)(b + 0.5f);
        return PG_RGB(cr, cg, cb);
}

void pg_gambar_transform(pg_permukaan_t *dst,
                          const pg_permukaan_t *src,
                          pg_transform_t t)
{
        int dw, dh, sw, sh, dlangkah, slangkah;
        int dx, dy;
        pg_warna_t *dpik;
        const pg_warna_t *spik;
        pg_transform_t inv;
        float det;

        if (!dst || !src) return;
        dw = pg_permukaan_lebar(dst);
        dh = pg_permukaan_tinggi(dst);
        sw = pg_permukaan_lebar(src);
        sh = pg_permukaan_tinggi(src);
        if (dw <= 0 || dh <= 0 || sw <= 0 || sh <= 0) return;

        dlangkah = pg_permukaan_langkah(dst);
        slangkah = pg_permukaan_langkah(src);
        dpik = (pg_warna_t *)pg_permukaan_piksel_mut(dst);
        spik = (const pg_warna_t *)pg_permukaan_piksel(src);

        /* Bila transform singular, tidak ada yang bisa di-sample. */
        det = t.a * t.d - t.b * t.c;
        if (det == 0.0f) {
                pg_set_galat(PG_GALAT_ARGUMEN,
                        "gambar_transform: transform singular");
                return;
        }
        inv = pg_transform_inverse(t);

        /* Iterasi dst. Untuk piksel di luar src bounds, lewati. */
        for (dy = 0; dy < dh; dy++) {
                pg_warna_t *drow = (pg_warna_t *)
                        ((char *)dpik + (size_t)dy * dlangkah);
                float fy = (float)dy;
                for (dx = 0; dx < dw; dx++) {
                        float fx = (float)dx;
                        float sx = inv.a * fx + inv.c * fy + inv.tx;
                        float sy = inv.b * fx + inv.d * fy + inv.ty;
                        /* Range valid bilinear: [0, sw-1] x [0, sh-1].
                         * Di luar itu, semua 4 tetangga di-clamp ke tepi
                         * yang sama — hasilnya solid edge, bukan AA. Jadi
                         * skip agar tidak menulis ulang dst. */
                        if (sx < 0.0f || sx > (float)(sw - 1) ||
                            sy < 0.0f || sy > (float)(sh - 1))
                                continue;
                        drow[dx] = pg_transform_bilinear(spik, sw, sh,
                                slangkah, sx, sy);
                }
        }

        pg_permukaan_kotor(dst, pg_buat_kotak(0, 0, dw, dh));
}
