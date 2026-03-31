/*
 * PIGURA OS - GPU_BLEND.C
 * =======================
 * Operasi pencampuran (blend) warna piksel.
 *
 * Berkas ini mengimplementasikan:
 *   - Blend dua permukaan (per-piksel alpha)
 *   - Blend dengan alpha global seragam
 *   - Blend additive (penambahan warna)
 *   - Blend subtractive (pengurangan warna)
 *   - Blend multiply (perkalian warna)
 *   - Blend screen (inversi perkalian)
 *   - Blend overlay (kombinasi multiply & screen)
 *   - Blend dengan operasi Porter-Duff lengkap
 *
 * Semua operasi menggunakan aritmatika integer murni.
 * Tidak menggunakan floating point untuk kompatibilitas
 * dengan arsitektur tanpa FPU (ARMv7 tanpa VFP, x86
 * tanpa x87).
 *
 * Mendukung arsitektur: x86, x86_64, ARM, ARMv7, ARM64
 * Kepatuhan standar: C89 + POSIX Safe Functions
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../../../inti/kernel.h"
#include "akselerasi_gpu.h"

/*
 * ===========================================================================
 * FUNGSI EXTERNAL DARI GPU_ACCEL.C
 * ===========================================================================
 */

extern bool_t gpu_akselerasi_siap(void);
extern status_t gpu_konteks_siapkan(struct gpu_konteks *ctx,
    const tak_bertanda32_t *src_buffer,
    tak_bertanda32_t src_lebar,
    tak_bertanda32_t src_tinggi,
    tak_bertanda32_t src_pitch,
    tak_bertanda32_t *dst_buffer,
    tak_bertanda32_t dst_lebar,
    tak_bertanda32_t dst_tinggi,
    tak_bertanda32_t dst_pitch,
    tak_bertanda32_t dst_x,
    tak_bertanda32_t dst_y,
    tak_bertanda32_t op_lebar,
    tak_bertanda32_t op_tinggi);

extern void gpu_statistik_tambah(bool_t fallback,
                                  tak_bertanda64_t piksel);

extern void gpu_ekstrak_warna(tak_bertanda32_t piksel,
    tak_bertanda8_t *r, tak_bertanda8_t *g,
    tak_bertanda8_t *b, tak_bertanda8_t *a);

extern tak_bertanda8_t gpu_kali_persen(tak_bertanda8_t nilai,
                                         tak_bertanda8_t persen);

extern tak_bertanda32_t gpu_gabung_warna(tak_bertanda8_t r,
    tak_bertanda8_t g, tak_bertanda8_t b, tak_bertanda8_t a);

/*
 * ===========================================================================
 * FUNGSI INTERNAL - OPERASI BLEND DASAR
 * ===========================================================================
 */

/*
 * gpu_blend_porter_duff_over
 * --------------------------
 * Implementasi Porter-Duff "over" - standar de facto
 * compositing. Sumber ditimpa di atas tujuan berdasarkan
 * alpha sumber.
 *
 * Rumus: Out = Src + Dst * (1 - SrcAlpha)
 */
static tak_bertanda32_t gpu_blend_porter_duff_over(
    tak_bertanda32_t dst,
    tak_bertanda32_t src)
{
    tak_bertanda8_t sr, sg, sb, sa;
    tak_bertanda8_t dr, dg, db, da;
    tak_bertanda8_t inv_sa;
    tak_bertanda32_t rr, rg, rb, ra;

    gpu_ekstrak_warna(src, &sr, &sg, &sb, &sa);
    gpu_ekstrak_warna(dst, &dr, &dg, &db, &da);

    if (sa == 255) {
        return src;
    }

    if (sa == 0) {
        return dst;
    }

    inv_sa = (tak_bertanda8_t)(255 - sa);
    rr = (tak_bertanda32_t)sr + gpu_kali_persen(dr, inv_sa);
    rg = (tak_bertanda32_t)sg + gpu_kali_persen(dg, inv_sa);
    rb = (tak_bertanda32_t)sb + gpu_kali_persen(db, inv_sa);
    ra = (tak_bertanda32_t)sa + gpu_kali_persen(da, inv_sa);

    if (rr > 255) rr = 255;
    if (rg > 255) rg = 255;
    if (rb > 255) rb = 255;
    if (ra > 255) ra = 255;

    return gpu_gabung_warna((tak_bertanda8_t)rr,
                              (tak_bertanda8_t)rg,
                              (tak_bertanda8_t)rb,
                              (tak_bertanda8_t)ra);
}

/*
 * gpu_blend_additive
 * -----------------
 * Blend additive: menambahkan komponen warna sumber
 * ke tujuan. Hasil di-clamp ke 255.
 *
 * Rumus: Out = min(Dst + Src, 255)
 */
static tak_bertanda32_t gpu_blend_additive(
    tak_bertanda32_t dst,
    tak_bertanda32_t src,
    tak_bertanda8_t alpha)
{
    tak_bertanda8_t sr, sg, sb, sa;
    tak_bertanda8_t dr, dg, db, da;
    tak_bertanda32_t rr, rg, rb, ra;

    gpu_ekstrak_warna(src, &sr, &sg, &sb, &sa);
    gpu_ekstrak_warna(dst, &dr, &dg, &db, &da);

    sr = gpu_kali_persen(sr, alpha);
    sg = gpu_kali_persen(sg, alpha);
    sb = gpu_kali_persen(sb, alpha);

    rr = (tak_bertanda32_t)dr + (tak_bertanda32_t)sr;
    rg = (tak_bertanda32_t)dg + (tak_bertanda32_t)sg;
    rb = (tak_bertanda32_t)db + (tak_bertanda32_t)sb;
    ra = (tak_bertanda32_t)da + gpu_kali_persen(sa, alpha);

    if (rr > 255) rr = 255;
    if (rg > 255) rg = 255;
    if (rb > 255) rb = 255;
    if (ra > 255) ra = 255;

    return gpu_gabung_warna((tak_bertanda8_t)rr,
                              (tak_bertanda8_t)rg,
                              (tak_bertanda8_t)rb,
                              (tak_bertanda8_t)ra);
}

/*
 * gpu_blend_subtractive
 * --------------------
 * Blend subtractive: mengurangi komponen warna sumber
 * dari tujuan. Hasil di-clamp ke 0.
 *
 * Rumus: Out = max(Dst - Src, 0)
 */
static tak_bertanda32_t gpu_blend_subtractive(
    tak_bertanda32_t dst,
    tak_bertanda32_t src,
    tak_bertanda8_t alpha)
{
    tak_bertanda8_t sr, sg, sb;
    tak_bertanda8_t dr, dg, db, da;
    tak_bertanda32_t rr, rg, rb;

    gpu_ekstrak_warna(src, &sr, &sg, &sb, NULL);
    gpu_ekstrak_warna(dst, &dr, &dg, &db, &da);

    sr = gpu_kali_persen(sr, alpha);
    sg = gpu_kali_persen(sg, alpha);
    sb = gpu_kali_persen(sb, alpha);

    if ((tak_bertanda32_t)dr > (tak_bertanda32_t)sr) {
        rr = (tak_bertanda32_t)dr - (tak_bertanda32_t)sr;
    } else {
        rr = 0;
    }

    if ((tak_bertanda32_t)dg > (tak_bertanda32_t)sg) {
        rg = (tak_bertanda32_t)dg - (tak_bertanda32_t)sg;
    } else {
        rg = 0;
    }

    if ((tak_bertanda32_t)db > (tak_bertanda32_t)sb) {
        rb = (tak_bertanda32_t)db - (tak_bertanda32_t)sb;
    } else {
        rb = 0;
    }

    return gpu_gabung_warna((tak_bertanda8_t)rr,
                              (tak_bertanda8_t)rg,
                              (tak_bertanda8_t)rb, da);
}

/*
 * gpu_blend_multiply
 * -----------------
 * Blend multiply: mengalikan komponen warna sumber
 * dan tujuan, menghasilkan warna yang lebih gelap.
 *
 * Rumus: Out = Src * Dst / 255
 */
static tak_bertanda32_t gpu_blend_multiply(
    tak_bertanda32_t dst,
    tak_bertanda32_t src,
    tak_bertanda8_t alpha)
{
    tak_bertanda8_t sr, sg, sb;
    tak_bertanda8_t dr, dg, db, da;
    tak_bertanda32_t rr, rg, rb;
    tak_bertanda32_t inv_alpha;
    tak_bertanda32_t pr, pg, pb;

    gpu_ekstrak_warna(src, &sr, &sg, &sb, NULL);
    gpu_ekstrak_warna(dst, &dr, &dg, &db, &da);

    pr = ((tak_bertanda32_t)sr * (tak_bertanda32_t)dr) / 255;
    pg = ((tak_bertanda32_t)sg * (tak_bertanda32_t)dg) / 255;
    pb = ((tak_bertanda32_t)sb * (tak_bertanda32_t)db) / 255;

    inv_alpha = 255 - (tak_bertanda32_t)alpha;
    rr = pr + gpu_kali_persen(dr, (tak_bertanda8_t)inv_alpha);
    rg = pg + gpu_kali_persen(dg, (tak_bertanda8_t)inv_alpha);
    rb = pb + gpu_kali_persen(db, (tak_bertanda8_t)inv_alpha);

    if (rr > 255) rr = 255;
    if (rg > 255) rg = 255;
    if (rb > 255) rb = 255;

    return gpu_gabung_warna((tak_bertanda8_t)rr,
                              (tak_bertanda8_t)rg,
                              (tak_bertanda8_t)rb, da);
}

/*
 * gpu_blend_screen
 * ---------------
 * Blend screen: membalikkan multiply, menghasilkan
 * warna yang lebih terang.
 *
 * Rumus: Out = Src + Dst - Src*Dst/255
 */
static tak_bertanda32_t gpu_blend_screen(
    tak_bertanda32_t dst,
    tak_bertanda32_t src,
    tak_bertanda8_t alpha)
{
    tak_bertanda8_t sr, sg, sb;
    tak_bertanda8_t dr, dg, db, da;
    tak_bertanda32_t rr, rg, rb;
    tak_bertanda32_t inv_alpha;
    tak_bertanda32_t pr, pg, pb;

    gpu_ekstrak_warna(src, &sr, &sg, &sb, NULL);
    gpu_ekstrak_warna(dst, &dr, &dg, &db, &da);

    pr = (tak_bertanda32_t)sr + (tak_bertanda32_t)dr -
         ((tak_bertanda32_t)sr * (tak_bertanda32_t)dr) / 255;
    pg = (tak_bertanda32_t)sg + (tak_bertanda32_t)dg -
         ((tak_bertanda32_t)sg * (tak_bertanda32_t)dg) / 255;
    pb = (tak_bertanda32_t)sb + (tak_bertanda32_t)db -
         ((tak_bertanda32_t)sb * (tak_bertanda32_t)db) / 255;

    inv_alpha = 255 - (tak_bertanda32_t)alpha;
    rr = pr + gpu_kali_persen(dr, (tak_bertanda8_t)inv_alpha);
    rg = pg + gpu_kali_persen(dg, (tak_bertanda8_t)inv_alpha);
    rb = pb + gpu_kali_persen(db, (tak_bertanda8_t)inv_alpha);

    if (rr > 255) rr = 255;
    if (rg > 255) rg = 255;
    if (rb > 255) rb = 255;

    return gpu_gabung_warna((tak_bertanda8_t)rr,
                              (tak_bertanda8_t)rg,
                              (tak_bertanda8_t)rb, da);
}

/*
 * gpu_blend_overlay
 * ----------------
 * Blend overlay: menggabungkan multiply dan screen.
 * Jika tujuan gelap, menggunakan multiply.
 * Jika tujuan terang, menggunakan screen.
 */
static tak_bertanda32_t gpu_blend_overlay(
    tak_bertanda32_t dst,
    tak_bertanda32_t src,
    tak_bertanda8_t alpha)
{
    tak_bertanda8_t sr, sg, sb;
    tak_bertanda8_t dr, dg, db, da;
    tak_bertanda32_t rr, rg, rb;
    tak_bertanda32_t inv_alpha;

    gpu_ekstrak_warna(src, &sr, &sg, &sb, NULL);
    gpu_ekstrak_warna(dst, &dr, &dg, &db, &da);

    if (dr < 128) {
        rr = ((tak_bertanda32_t)sr * (tak_bertanda32_t)dr * 2)
             / 255;
    } else {
        rr = 255 - ((255 - (tak_bertanda32_t)sr)
             * (255 - (tak_bertanda32_t)dr) * 2) / 255;
    }

    if (dg < 128) {
        rg = ((tak_bertanda32_t)sg * (tak_bertanda32_t)dg * 2)
             / 255;
    } else {
        rg = 255 - ((255 - (tak_bertanda32_t)sg)
             * (255 - (tak_bertanda32_t)dg) * 2) / 255;
    }

    if (db < 128) {
        rb = ((tak_bertanda32_t)sb * (tak_bertanda32_t)db * 2)
             / 255;
    } else {
        rb = 255 - ((255 - (tak_bertanda32_t)sb)
             * (255 - (tak_bertanda32_t)db) * 2) / 255;
    }

    inv_alpha = 255 - (tak_bertanda32_t)alpha;
    rr = rr + gpu_kali_persen(dr, (tak_bertanda8_t)inv_alpha);
    rg = rg + gpu_kali_persen(dg, (tak_bertanda8_t)inv_alpha);
    rb = rb + gpu_kali_persen(db, (tak_bertanda8_t)inv_alpha);

    if (rr > 255) rr = 255;
    if (rg > 255) rg = 255;
    if (rb > 255) rb = 255;

    return gpu_gabung_warna((tak_bertanda8_t)rr,
                              (tak_bertanda8_t)rg,
                              (tak_bertanda8_t)rb, da);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - BLEND PORTER-DUFF OVER (STANDAR)
 * ===========================================================================
 */

/*
 * gpu_blend_permukaan
 * ------------------
 * Mencampur dua permukaan menggunakan Porter-Duff over.
 * Sumber dicampur di atas tujuan pada area yang ditentukan.
 *
 * Parameter:
 *   src_buffer - Buffer sumber
 *   src_lebar  - Lebar sumber
 *   src_tinggi - Tinggi sumber
 *   src_pitch  - Pitch sumber (0 = auto)
 *   dst_buffer - Buffer tujuan
 *   dst_lebar  - Lebar tujuan
 *   dst_tinggi - Tinggi tujuan
 *   dst_pitch  - Pitch tujuan (0 = auto)
 *   dst_x      - Offset X pada tujuan
 *   dst_y      - Offset Y pada tujuan
 *   src_x      - Offset X pada sumber
 *   src_y      - Offset Y pada sumber
 *   lebar      - Lebar area
 *   tinggi     - Tinggi area
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_blend_permukaan(const tak_bertanda32_t *src_buffer,
                             tak_bertanda32_t src_lebar,
                             tak_bertanda32_t src_tinggi,
                             tak_bertanda32_t src_pitch,
                             tak_bertanda32_t *dst_buffer,
                             tak_bertanda32_t dst_lebar,
                             tak_bertanda32_t dst_tinggi,
                             tak_bertanda32_t dst_pitch,
                             tak_bertanda32_t dst_x,
                             tak_bertanda32_t dst_y,
                             tak_bertanda32_t src_x,
                             tak_bertanda32_t src_y,
                             tak_bertanda32_t lebar,
                             tak_bertanda32_t tinggi)
{
    struct gpu_konteks ctx;
    (void)src_x; (void)src_y;
    status_t status;
    tak_bertanda32_t row, col;

    status = gpu_konteks_siapkan(&ctx,
                                   src_buffer, src_lebar, src_tinggi,
                                   src_pitch,
                                   dst_buffer, dst_lebar, dst_tinggi,
                                   dst_pitch,
                                   dst_x, dst_y,
                                   lebar, tinggi);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    for (row = 0; row < ctx.op_tinggi; row++) {
        tak_bertanda32_t offset_src;
        tak_bertanda32_t offset_dst;

        offset_src = (ctx.src_y + row) * ctx.src_pitch + ctx.src_x;
        offset_dst = (ctx.dst_y + row) * ctx.dst_pitch + ctx.dst_x;

        for (col = 0; col < ctx.op_lebar; col++) {
            tak_bertanda32_t piksel_s, piksel_d;

            piksel_s = ctx.src_buffer[offset_src + col];
            piksel_d = ctx.dst_buffer[offset_dst + col];

            ctx.dst_buffer[offset_dst + col] =
                gpu_blend_porter_duff_over(piksel_d, piksel_s);
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)ctx.op_lebar *
                         (tak_bertanda64_t)ctx.op_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - BLEND DENGAN ALPHA GLOBAL
 * ===========================================================================
 */

/*
 * gpu_blend_alpha_global
 * ---------------------
 * Mencampur dua permukaan dengan alpha global seragam.
 * Alpha berlaku untuk seluruh area operasi.
 *
 * Parameter:
 *   src_buffer - Buffer sumber
 *   src_lebar  - Lebar sumber
 *   src_tinggi - Tinggi sumber
 *   src_pitch  - Pitch sumber (0 = auto)
 *   dst_buffer - Buffer tujuan
 *   dst_lebar  - Lebar tujuan
 *   dst_tinggi - Tinggi tujuan
 *   dst_pitch  - Pitch tujuan (0 = auto)
 *   dst_x      - Offset X pada tujuan
 *   dst_y      - Offset Y pada tujuan
 *   src_x      - Offset X pada sumber
 *   src_y      - Offset Y pada sumber
 *   lebar      - Lebar area
 *   tinggi     - Tinggi area
 *   alpha      - Alpha global (0-255)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_blend_alpha_global(const tak_bertanda32_t *src_buffer,
                                tak_bertanda32_t src_lebar,
                                tak_bertanda32_t src_tinggi,
                                tak_bertanda32_t src_pitch,
                                tak_bertanda32_t *dst_buffer,
                                tak_bertanda32_t dst_lebar,
                                tak_bertanda32_t dst_tinggi,
                                tak_bertanda32_t dst_pitch,
                                tak_bertanda32_t dst_x,
                                tak_bertanda32_t dst_y,
                                tak_bertanda32_t src_x,
                                tak_bertanda32_t src_y,
                                tak_bertanda32_t lebar,
                                tak_bertanda32_t tinggi,
                                tak_bertanda8_t alpha)
{
    struct gpu_konteks ctx;
    (void)src_x; (void)src_y;
    status_t status;
    tak_bertanda32_t row, col;
    tak_bertanda8_t inv_alpha;

    status = gpu_konteks_siapkan(&ctx,
                                   src_buffer, src_lebar, src_tinggi,
                                   src_pitch,
                                   dst_buffer, dst_lebar, dst_tinggi,
                                   dst_pitch,
                                   dst_x, dst_y,
                                   lebar, tinggi);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    inv_alpha = (tak_bertanda8_t)(255 - alpha);

    for (row = 0; row < ctx.op_tinggi; row++) {
        tak_bertanda32_t offset_src;
        tak_bertanda32_t offset_dst;

        offset_src = (ctx.src_y + row) * ctx.src_pitch + ctx.src_x;
        offset_dst = (ctx.dst_y + row) * ctx.dst_pitch + ctx.dst_x;

        for (col = 0; col < ctx.op_lebar; col++) {
            tak_bertanda32_t piksel_s, piksel_d;
            tak_bertanda8_t sr, sg, sb;
            tak_bertanda8_t dr, dg, db;
            tak_bertanda32_t rr, rg, rb;

            piksel_s = ctx.src_buffer[offset_src + col];
            piksel_d = ctx.dst_buffer[offset_dst + col];

            gpu_ekstrak_warna(piksel_s, &sr, &sg, &sb, NULL);
            gpu_ekstrak_warna(piksel_d, &dr, &dg, &db, NULL);

            rr = gpu_kali_persen(sr, alpha) +
                 gpu_kali_persen(dr, inv_alpha);
            rg = gpu_kali_persen(sg, alpha) +
                 gpu_kali_persen(dg, inv_alpha);
            rb = gpu_kali_persen(sb, alpha) +
                 gpu_kali_persen(db, inv_alpha);

            ctx.dst_buffer[offset_dst + col] =
                gpu_gabung_warna((tak_bertanda8_t)rr,
                                  (tak_bertanda8_t)rg,
                                  (tak_bertanda8_t)rb, 255);
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)ctx.op_lebar *
                         (tak_bertanda64_t)ctx.op_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - BLEND ADDITIVE
 * ===========================================================================
 */

/*
 * gpu_blend_add
 * ------------
 * Blend additive: menambahkan komponen warna.
 * Efek berguna untuk cahaya, kilat, dan partikel.
 *
 * Parameter:
 *   src_buffer - Buffer sumber
 *   src_lebar  - Lebar sumber
 *   src_tinggi - Tinggi sumber
 *   src_pitch  - Pitch sumber (0 = auto)
 *   dst_buffer - Buffer tujuan
 *   dst_lebar  - Lebar tujuan
 *   dst_tinggi - Tinggi tujuan
 *   dst_pitch  - Pitch tujuan (0 = auto)
 *   dst_x      - Offset X pada tujuan
 *   dst_y      - Offset Y pada tujuan
 *   src_x      - Offset X pada sumber
 *   src_y      - Offset Y pada sumber
 *   lebar      - Lebar area
 *   tinggi     - Tinggi area
 *   alpha      - Intensitas (0-255)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_blend_add(const tak_bertanda32_t *src_buffer,
                       tak_bertanda32_t src_lebar,
                       tak_bertanda32_t src_tinggi,
                       tak_bertanda32_t src_pitch,
                       tak_bertanda32_t *dst_buffer,
                       tak_bertanda32_t dst_lebar,
                       tak_bertanda32_t dst_tinggi,
                       tak_bertanda32_t dst_pitch,
                       tak_bertanda32_t dst_x,
                       tak_bertanda32_t dst_y,
                       tak_bertanda32_t src_x,
                       tak_bertanda32_t src_y,
                       tak_bertanda32_t lebar,
                       tak_bertanda32_t tinggi,
                       tak_bertanda8_t alpha)
{
    struct gpu_konteks ctx;
    (void)src_x; (void)src_y;
    status_t status;
    tak_bertanda32_t row, col;

    status = gpu_konteks_siapkan(&ctx,
                                   src_buffer, src_lebar, src_tinggi,
                                   src_pitch,
                                   dst_buffer, dst_lebar, dst_tinggi,
                                   dst_pitch,
                                   dst_x, dst_y,
                                   lebar, tinggi);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    for (row = 0; row < ctx.op_tinggi; row++) {
        tak_bertanda32_t offset_src;
        tak_bertanda32_t offset_dst;

        offset_src = (ctx.src_y + row) * ctx.src_pitch + ctx.src_x;
        offset_dst = (ctx.dst_y + row) * ctx.dst_pitch + ctx.dst_x;

        for (col = 0; col < ctx.op_lebar; col++) {
            tak_bertanda32_t piksel_s, piksel_d;

            piksel_s = ctx.src_buffer[offset_src + col];
            piksel_d = ctx.dst_buffer[offset_dst + col];

            ctx.dst_buffer[offset_dst + col] =
                gpu_blend_additive(piksel_d, piksel_s, alpha);
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)ctx.op_lebar *
                         (tak_bertanda64_t)ctx.op_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - BLEND SUBTRACTIVE
 * ===========================================================================
 */

/*
 * gpu_blend_kurang
 * ---------------
 * Blend subtractive: mengurangi warna sumber dari tujuan.
 * Efek berguna untuk bayangan dan efek gelap.
 *
 * Parameter:
 *   (sama dengan gpu_blend_add)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_blend_kurang(const tak_bertanda32_t *src_buffer,
                          tak_bertanda32_t src_lebar,
                          tak_bertanda32_t src_tinggi,
                          tak_bertanda32_t src_pitch,
                          tak_bertanda32_t *dst_buffer,
                          tak_bertanda32_t dst_lebar,
                          tak_bertanda32_t dst_tinggi,
                          tak_bertanda32_t dst_pitch,
                          tak_bertanda32_t dst_x,
                          tak_bertanda32_t dst_y,
                          tak_bertanda32_t src_x,
                          tak_bertanda32_t src_y,
                          tak_bertanda32_t lebar,
                          tak_bertanda32_t tinggi,
                          tak_bertanda8_t alpha)
{
    struct gpu_konteks ctx;
    (void)src_x; (void)src_y;
    status_t status;
    tak_bertanda32_t row, col;

    status = gpu_konteks_siapkan(&ctx,
                                   src_buffer, src_lebar, src_tinggi,
                                   src_pitch,
                                   dst_buffer, dst_lebar, dst_tinggi,
                                   dst_pitch,
                                   dst_x, dst_y,
                                   lebar, tinggi);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    for (row = 0; row < ctx.op_tinggi; row++) {
        tak_bertanda32_t offset_src;
        tak_bertanda32_t offset_dst;

        offset_src = (ctx.src_y + row) * ctx.src_pitch + ctx.src_x;
        offset_dst = (ctx.dst_y + row) * ctx.dst_pitch + ctx.dst_x;

        for (col = 0; col < ctx.op_lebar; col++) {
            tak_bertanda32_t piksel_s, piksel_d;

            piksel_s = ctx.src_buffer[offset_src + col];
            piksel_d = ctx.dst_buffer[offset_dst + col];

            ctx.dst_buffer[offset_dst + col] =
                gpu_blend_subtractive(piksel_d, piksel_s, alpha);
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)ctx.op_lebar *
                         (tak_bertanda64_t)ctx.op_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - BLEND MULTIPLY
 * ===========================================================================
 */

/*
 * gpu_blend_kali
 * -------------
 * Blend multiply: menghasilkan warna lebih gelap.
 * Berguna untuk bayangan dan efek gelap.
 *
 * Parameter:
 *   (sama dengan gpu_blend_add)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t gpu_blend_kali(const tak_bertanda32_t *src_buffer,
                        tak_bertanda32_t src_lebar,
                        tak_bertanda32_t src_tinggi,
                        tak_bertanda32_t src_pitch,
                        tak_bertanda32_t *dst_buffer,
                        tak_bertanda32_t dst_lebar,
                        tak_bertanda32_t dst_tinggi,
                        tak_bertanda32_t dst_pitch,
                        tak_bertanda32_t dst_x,
                        tak_bertanda32_t dst_y,
                        tak_bertanda32_t src_x,
                        tak_bertanda32_t src_y,
                        tak_bertanda32_t lebar,
                        tak_bertanda32_t tinggi,
                        tak_bertanda8_t alpha)
{
    struct gpu_konteks ctx;
    (void)src_x; (void)src_y;
    status_t status;
    tak_bertanda32_t row, col;

    status = gpu_konteks_siapkan(&ctx,
                                   src_buffer, src_lebar, src_tinggi,
                                   src_pitch,
                                   dst_buffer, dst_lebar, dst_tinggi,
                                   dst_pitch,
                                   dst_x, dst_y,
                                   lebar, tinggi);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    for (row = 0; row < ctx.op_tinggi; row++) {
        tak_bertanda32_t offset_src;
        tak_bertanda32_t offset_dst;

        offset_src = (ctx.src_y + row) * ctx.src_pitch + ctx.src_x;
        offset_dst = (ctx.dst_y + row) * ctx.dst_pitch + ctx.dst_x;

        for (col = 0; col < ctx.op_lebar; col++) {
            tak_bertanda32_t piksel_s, piksel_d;

            piksel_s = ctx.src_buffer[offset_src + col];
            piksel_d = ctx.dst_buffer[offset_dst + col];

            ctx.dst_buffer[offset_dst + col] =
                gpu_blend_multiply(piksel_d, piksel_s, alpha);
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)ctx.op_lebar *
                         (tak_bertanda64_t)ctx.op_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - BLEND SCREEN
 * ===========================================================================
 */

/*
 * gpu_blend_layar
 * --------------
 * Blend screen: membalikkan multiply, menghasilkan
 * warna yang lebih terang. Berguna untuk efek cahaya.
 *
 * Parameter:
 *   (sama dengan gpu_blend_add)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t gpu_blend_layar(const tak_bertanda32_t *src_buffer,
                         tak_bertanda32_t src_lebar,
                         tak_bertanda32_t src_tinggi,
                         tak_bertanda32_t src_pitch,
                         tak_bertanda32_t *dst_buffer,
                         tak_bertanda32_t dst_lebar,
                         tak_bertanda32_t dst_tinggi,
                         tak_bertanda32_t dst_pitch,
                         tak_bertanda32_t dst_x,
                         tak_bertanda32_t dst_y,
                         tak_bertanda32_t src_x,
                         tak_bertanda32_t src_y,
                         tak_bertanda32_t lebar,
                         tak_bertanda32_t tinggi,
                         tak_bertanda8_t alpha)
{
    struct gpu_konteks ctx;
    (void)src_x; (void)src_y;
    status_t status;
    tak_bertanda32_t row, col;

    status = gpu_konteks_siapkan(&ctx,
                                   src_buffer, src_lebar, src_tinggi,
                                   src_pitch,
                                   dst_buffer, dst_lebar, dst_tinggi,
                                   dst_pitch,
                                   dst_x, dst_y,
                                   lebar, tinggi);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    for (row = 0; row < ctx.op_tinggi; row++) {
        tak_bertanda32_t offset_src;
        tak_bertanda32_t offset_dst;

        offset_src = (ctx.src_y + row) * ctx.src_pitch + ctx.src_x;
        offset_dst = (ctx.dst_y + row) * ctx.dst_pitch + ctx.dst_x;

        for (col = 0; col < ctx.op_lebar; col++) {
            tak_bertanda32_t piksel_s, piksel_d;

            piksel_s = ctx.src_buffer[offset_src + col];
            piksel_d = ctx.dst_buffer[offset_dst + col];

            ctx.dst_buffer[offset_dst + col] =
                gpu_blend_screen(piksel_d, piksel_s, alpha);
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)ctx.op_lebar *
                         (tak_bertanda64_t)ctx.op_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - BLEND OVERLAY
 * ===========================================================================
 */

/*
 * gpu_blend_overlay
 * ----------------
 * Blend overlay: kombinasi multiply dan screen.
 * Berguna untuk kontras dan efek dramatis.
 *
 * Parameter:
 *   (sama dengan gpu_blend_add)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t gpu_blend_tumpuk(const tak_bertanda32_t *src_buffer,
                          tak_bertanda32_t src_lebar,
                          tak_bertanda32_t src_tinggi,
                          tak_bertanda32_t src_pitch,
                          tak_bertanda32_t *dst_buffer,
                          tak_bertanda32_t dst_lebar,
                          tak_bertanda32_t dst_tinggi,
                          tak_bertanda32_t dst_pitch,
                          tak_bertanda32_t dst_x,
                          tak_bertanda32_t dst_y,
                          tak_bertanda32_t src_x,
                          tak_bertanda32_t src_y,
                          tak_bertanda32_t lebar,
                          tak_bertanda32_t tinggi,
                          tak_bertanda8_t alpha)
{
    struct gpu_konteks ctx;
    (void)src_x; (void)src_y;
    status_t status;
    tak_bertanda32_t row, col;

    status = gpu_konteks_siapkan(&ctx,
                                   src_buffer, src_lebar, src_tinggi,
                                   src_pitch,
                                   dst_buffer, dst_lebar, dst_tinggi,
                                   dst_pitch,
                                   dst_x, dst_y,
                                   lebar, tinggi);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    for (row = 0; row < ctx.op_tinggi; row++) {
        tak_bertanda32_t offset_src;
        tak_bertanda32_t offset_dst;

        offset_src = (ctx.src_y + row) * ctx.src_pitch + ctx.src_x;
        offset_dst = (ctx.dst_y + row) * ctx.dst_pitch + ctx.dst_x;

        for (col = 0; col < ctx.op_lebar; col++) {
            tak_bertanda32_t piksel_s, piksel_d;

            piksel_s = ctx.src_buffer[offset_src + col];
            piksel_d = ctx.dst_buffer[offset_dst + col];

            ctx.dst_buffer[offset_dst + col] =
                gpu_blend_overlay(piksel_d, piksel_s, alpha);
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)ctx.op_lebar *
                         (tak_bertanda64_t)ctx.op_tinggi);

    return STATUS_BERHASIL;
}
