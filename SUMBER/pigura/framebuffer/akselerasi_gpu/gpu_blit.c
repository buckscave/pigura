/*
 * PIGURA OS - GPU_BLIT.C
 * ====================
 * Operasi blitting (blit) piksel dengan dukungan clipping,
 * transparansi, dan warna kunci (color key).
 *
 * Berkas ini mengimplementasikan:
 *   - Blit standar dengan clipping
 *   - Blit dengan alpha channel per-piksel
 *   - Blit dengan warna kunci (color key transparency)
 *   - Blit dengan alpha global (transparansi seragam)
 *
 * Semua operasi menggunakan C89 murni tanpa floating point
 * untuk portabilitas ke semua arsitektur.
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
 * FUNGSI INTERNAL - PERBANDINGAN WARNA
 * ===========================================================================
 */

/*
 * gpu_sama_persis
 * ---------------
 * Memeriksa apakah dua warna identik.
 *
 * Parameter:
 *   a - Warna pertama
 *   b - Warna kedua
 *
 * Return:
 *   BENAR jika identik, SALAH jika berbeda
 */
static bool_t gpu_sama_persis(tak_bertanda32_t a,
                               tak_bertanda32_t b)
{
    return (a == b) ? BENAR : SALAH;
}

/*
 * gpu_sama_warna_kunci
 * ---------------------
 * Memeriksa apakah warna cocok dengan warna kunci (color key).
 * Pemeriksaan menggunakan mask sehingga hanya komponen
 * warna yang diaktifkan dalam mask yang diperiksa.
 *
 * Parameter:
 *   piksel      - Warna piksel yang akan diperiksa
 *   warna_kunci - Warna kunci (warna yang harus diabaikan)
 *   mask_alpha  - Jika BENAR, komponen alpha diperiksa juga
 *
 * Return:
 *   BENAR jika cocok (harus diabaikan)
 *   SALAH jika berbeda (harus disalin)
 */
static bool_t gpu_sama_warna_kunci(tak_bertanda32_t piksel,
                                    tak_bertanda32_t warna_kunci,
                                    bool_t mask_alpha)
{
    tak_bertanda8_t pr, pg, pb, pa;
    tak_bertanda8_t kr, kg, kb, ka;

    gpu_ekstrak_warna(piksel, &pr, &pg, &pb, &pa);
    gpu_ekstrak_warna(warna_kunci, &kr, &kg, &kb, &ka);

    if (pr != kr || pg != kg || pb != kb) {
        return SALAH;
    }

    if (mask_alpha && (pa != ka)) {
        return SALAH;
    }

    return BENAR;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - BLENDING PER-PIKSEL
 * ===========================================================================
 */

/*
 * gpu_campur_alpha_piksel
 * -------------------------
 * Mencampur piksel sumber dengan piksel tujuan menggunakan
 * alpha per-piksel (Porter-Duff over).
 *
 * Parameter:
 *   piksel_dst - Piksel tujuan (background)
 *   piksel_src - Piksel sumber (foreground)
 *
 * Return:
 *   Hasil pencampuran
 */
static tak_bertanda32_t gpu_campur_alpha_piksel(
    tak_bertanda32_t piksel_dst,
    tak_bertanda32_t piksel_src)
{
    tak_bertanda8_t sr, sg, sb, sa;
    tak_bertanda8_t dr, dg, db, da;
    tak_bertanda8_t inv_sa;
    tak_bertanda32_t hasil_r, hasil_g, hasil_b;
    tak_bertanda32_t hasil_a;

    gpu_ekstrak_warna(piksel_src, &sr, &sg, &sb, &sa);
    gpu_ekstrak_warna(piksel_dst, &dr, &dg, &db, &da);

    if (sa == 0) {
        return piksel_dst;
    }

    if (sa == 255) {
        return piksel_src;
    }

    inv_sa = (tak_bertanda8_t)(255 - sa);

    hasil_r = (tak_bertanda32_t)sr +
              gpu_kali_persen(dr, inv_sa);
    hasil_g = (tak_bertanda32_t)sg +
              gpu_kali_persen(dg, inv_sa);
    hasil_b = (tak_bertanda32_t)sb +
              gpu_kali_persen(db, inv_sa);
    hasil_a = (tak_bertanda32_t)sa +
              gpu_kali_persen(da, inv_sa);

    if (hasil_r > 255) hasil_r = 255;
    if (hasil_g > 255) hasil_g = 255;
    if (hasil_b > 255) hasil_b = 255;
    if (hasil_a > 255) hasil_a = 255;

    return gpu_gabung_warna((tak_bertanda8_t)hasil_r,
                              (tak_bertanda8_t)hasil_g,
                              (tak_bertanda8_t)hasil_b,
                              (tak_bertanda8_t)hasil_a);
}

/*
 * gpu_campur_alpha_global
 * -----------------------
 * Mencampur piksel sumber dengan piksel tujuan menggunakan
 * alpha global (transparansi seragam untuk seluruh area).
 *
 * Parameter:
 *   piksel_dst - Piksel tujuan (background)
 *   piksel_src - Piksel sumber (foreground)
 *   alpha      - Nilai alpha global (0-255)
 *
 * Return:
 *   Hasil pencampuran
 */
static tak_bertanda32_t gpu_campur_alpha_global(
    tak_bertanda32_t piksel_dst,
    tak_bertanda32_t piksel_src,
    tak_bertanda8_t alpha)
{
    tak_bertanda8_t sr, sg, sb, sa;
    tak_bertanda8_t dr, dg, db, da;
    tak_bertanda8_t inv_alpha;
    tak_bertanda32_t hasil_r, hasil_g, hasil_b;
    tak_bertanda32_t hasil_a;

    gpu_ekstrak_warna(piksel_src, &sr, &sg, &sb, &sa);
    gpu_ekstrak_warna(piksel_dst, &dr, &dg, &db, &da);

    if (alpha == 0) {
        return piksel_dst;
    }

    if (alpha == 255) {
        return piksel_src;
    }

    inv_alpha = (tak_bertanda8_t)(255 - alpha);

    hasil_r = (tak_bertanda32_t)sr +
              gpu_kali_persen(dr, inv_alpha);
    hasil_g = (tak_bertanda32_t)sg +
              gpu_kali_persen(dg, inv_alpha);
    hasil_b = (tak_bertanda32_t)sb +
              gpu_kali_persen(db, inv_alpha);
    hasil_a = (tak_bertanda32_t)alpha +
              gpu_kali_persen(da, inv_alpha);

    if (hasil_r > 255) hasil_r = 255;
    if (hasil_g > 255) hasil_g = 255;
    if (hasil_b > 255) hasil_b = 255;
    if (hasil_a > 255) hasil_a = 255;

    return gpu_gabung_warna((tak_bertanda8_t)hasil_r,
                              (tak_bertanda8_t)hasil_g,
                              (tak_bertanda8_t)hasil_b,
                              (tak_bertanda8_t)hasil_a);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - BLIT STANDAR
 * ===========================================================================
 */

/*
 * gpu_blit
 * --------
 * Menyalin blok piksel dari sumber ke tujuan dengan
 * clipping otomatis terhadap batas permukaan tujuan.
 *
 * Parameter:
 *   src_buffer - Buffer sumber (wajib valid)
 *   src_lebar  - Lebar sumber
 *   src_tinggi - Tinggi sumber
 *   src_pitch  - Pitch sumber (0 = auto = src_lebar)
 *   dst_buffer - Buffer tujuan (wajib valid)
 *   dst_lebar  - Lebar tujuan
 *   dst_tinggi - Tinggi tujuan
 *   dst_pitch  - Pitch tujuan (0 = auto = dst_lebar)
 *   dst_x      - Offset X pada tujuan
 *   dst_y      - Offset Y pada tujuan
 *   src_x      - Offset X pada sumber
 *   src_y      - Offset Y pada sumber
 *   lebar      - Lebar area blit
 *   tinggi     - Tinggi area blit
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_blit(const tak_bertanda32_t *src_buffer,
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
            ctx.dst_buffer[offset_dst + col] =
                ctx.src_buffer[offset_src + col];
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)ctx.op_lebar *
                         (tak_bertanda64_t)ctx.op_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - BLIT DENGAN ALPHA PER-PIKSEL
 * ===========================================================================
 */

/*
 * gpu_blit_alpha
 * --------------
 * Menyalin blok piksel dengan dukungan alpha channel per-piksel.
 * Setiap piksel sumber dicampur dengan piksel tujuan menggunakan
 * formula Porter-Duff (over).
 *
 * Piksel dengan alpha=0 (transparan penuh) akan diabaikan.
 * Piksel dengan alpha=255 (opaque penuh) akan menimpa tujuan.
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
 *   lebar      - Lebar area blit
 *   tinggi     - Tinggi area blit
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_blit_alpha(const tak_bertanda32_t *src_buffer,
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
            tak_bertanda32_t piksel_s;
            tak_bertanda32_t piksel_d;

            piksel_s = ctx.src_buffer[offset_src + col];
            piksel_d = ctx.dst_buffer[offset_dst + col];

            ctx.dst_buffer[offset_dst + col] =
                gpu_campur_alpha_piksel(piksel_d, piksel_s);
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)ctx.op_lebar *
                         (tak_bertanda64_t)ctx.op_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - BLIT DENGAN ALPHA GLOBAL
 * ===========================================================================
 */

/*
 * gpu_blit_alpha_global
 * --------------------
 * Menyalin blok piksel dengan alpha global (seragam).
 * Seluruh area menggunakan satu nilai alpha yang sama.
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
 *   lebar      - Lebar area blit
 *   tinggi     - Tinggi area blit
 *   alpha      - Alpha global (0-255)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_blit_alpha_global(const tak_bertanda32_t *src_buffer,
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
            tak_bertanda32_t piksel_s;
            tak_bertanda32_t piksel_d;

            piksel_s = ctx.src_buffer[offset_src + col];
            piksel_d = ctx.dst_buffer[offset_dst + col];

            ctx.dst_buffer[offset_dst + col] =
                gpu_campur_alpha_global(piksel_d, piksel_s, alpha);
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)ctx.op_lebar *
                         (tak_bertanda64_t)ctx.op_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - BLIT DENGAN WARNA KUNCI (COLOR KEY)
 * ===========================================================================
 */

/*
 * gpu_blit_warna_kunci
 * ---------------------
 * Menyalin blok piksel dengan dukungan warna kunci (color key).
 * Piksel pada sumber yang cocok dengan warna kunci akan
 * diabaikan (tidak disalin ke tujuan).
 *
 * Parameter:
 *   src_buffer  - Buffer sumber
 *   src_lebar   - Lebar sumber
 *   src_tinggi  - Tinggi sumber
 *   src_pitch   - Pitch sumber (0 = auto)
 *   dst_buffer  - Buffer tujuan
 *   dst_lebar   - Lebar tujuan
 *   dst_tinggi  - Tinggi tujuan
 *   dst_pitch   - Pitch tujuan (0 = auto)
 *   dst_x       - Offset X pada tujuan
 *   dst_y       - Offset Y pada tujuan
 *   src_x       - Offset X pada sumber
 *   src_y       - Offset Y pada sumber
 *   lebar       - Lebar area blit
 *   tinggi      - Tinggi area blit
 *   warna_kunci - Warna kunci (piksel yang diabaikan)
 *   mask_alpha  - Jika BENAR, cek alpha juga
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_blit_warna_kunci(const tak_bertanda32_t *src_buffer,
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
                            tak_bertanda32_t warna_kunci,
                            bool_t mask_alpha)
{
    struct gpu_konteks ctx;
    status_t status;
    (void)src_x; (void)src_y;
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
            tak_bertanda32_t piksel_s;

            piksel_s = ctx.src_buffer[offset_src + col];

            if (!gpu_sama_warna_kunci(piksel_s, warna_kunci,
                                       mask_alpha)) {
                ctx.dst_buffer[offset_dst + col] = piksel_s;
            }
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)ctx.op_lebar *
                         (tak_bertanda64_t)ctx.op_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - BLIT DENGAN ALPHA DAN WARNA KUNCI
 * ===========================================================================
 */

/*
 * gpu_blit_alpha_warna_kunci
 * ---------------------------
 * Menyalin blok piksel dengan dukungan alpha per-piksel
 * DAN warna kunci. Piksel yang cocok dengan warna kunci
 * akan diabaikan sama sekali (tidak di-blend).
 *
 * Parameter:
 *   src_buffer  - Buffer sumber
 *   src_lebar   - Lebar sumber
 *   src_tinggi  - Tinggi sumber
 *   src_pitch   - Pitch sumber (0 = auto)
 *   dst_buffer  - Buffer tujuan
 *   dst_lebar   - Lebar tujuan
 *   dst_tinggi  - Tinggi tujuan
 *   dst_pitch   - Pitch tujuan (0 = auto)
 *   dst_x       - Offset X pada tujuan
 *   dst_y       - Offset Y pada tujuan
 *   src_x       - Offset X pada sumber
 *   src_y       - Offset Y pada sumber
 *   lebar       - Lebar area blit
 *   tinggi      - Tinggi area blit
 *   warna_kunci - Warna kunci
 *   mask_alpha  - Jika BENAR, cek alpha pada warna kunci
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_blit_alpha_warna_kunci(
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
    tak_bertanda32_t src_x,
    tak_bertanda32_t src_y,
    tak_bertanda32_t lebar,
    tak_bertanda32_t tinggi,
    tak_bertanda32_t warna_kunci,
    bool_t mask_alpha)
{
    struct gpu_konteks ctx;
    status_t status;
    (void)src_x; (void)src_y;
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
            tak_bertanda32_t piksel_s;
            tak_bertanda32_t piksel_d;

            piksel_s = ctx.src_buffer[offset_src + col];

            if (gpu_sama_warna_kunci(piksel_s, warna_kunci,
                                      mask_alpha)) {
                continue;
            }

            piksel_d = ctx.dst_buffer[offset_dst + col];
            ctx.dst_buffer[offset_dst + col] =
                gpu_campur_alpha_piksel(piksel_d, piksel_s);
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)ctx.op_lebar *
                         (tak_bertanda64_t)ctx.op_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - BLIT DENGAN TINT WARNA
 * ===========================================================================
 */

/*
 * gpu_blit_tint
 * -------------
 * Menyalin blok piksel dengan efek tint (pewarnaan seragam).
 * Setiap piksel sumber dicampur dengan warna tint menggunakan
 * alpha per-piksel dari sumber.
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
 *   lebar      - Lebar area blit
 *   tinggi     - Tinggi area blit
 *   warna_tint - Warna tint (ARGB8888)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_blit_tint(const tak_bertanda32_t *src_buffer,
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
                       tak_bertanda32_t warna_tint)
{
    struct gpu_konteks ctx;
    status_t status;
    (void)src_x; (void)src_y;
    tak_bertanda32_t row, col;
    tak_bertanda8_t tr, tg, tb, ta;

    gpu_ekstrak_warna(warna_tint, &tr, &tg, &tb, &ta);

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
            tak_bertanda32_t piksel_s;
            tak_bertanda32_t piksel_d;
            tak_bertanda8_t sr, sg, sb, sa;
            tak_bertanda8_t rr, rg, rb;
            tak_bertanda8_t inv_sa;

            piksel_s = ctx.src_buffer[offset_src + col];
            piksel_d = ctx.dst_buffer[offset_dst + col];

            gpu_ekstrak_warna(piksel_s, &sr, &sg, &sb, &sa);

            if (sa == 0) {
                continue;
            }

            inv_sa = (tak_bertanda8_t)(255 - sa);

            rr = gpu_kali_persen(sr, ta) +
                 gpu_kali_persen(tr, (tak_bertanda8_t)(255 - ta));
            rg = gpu_kali_persen(sg, ta) +
                 gpu_kali_persen(tg, (tak_bertanda8_t)(255 - ta));
            rb = gpu_kali_persen(sb, ta) +
                 gpu_kali_persen(tb, (tak_bertanda8_t)(255 - ta));

            ctx.dst_buffer[offset_dst + col] =
                gpu_gabung_warna(rr, rg, rb,
                    (tak_bertanda8_t)((tak_bertanda32_t)sa +
                    gpu_kali_persen(
                        (tak_bertanda8_t)(piksel_d >> 24),
                        inv_sa)));
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)ctx.op_lebar *
                         (tak_bertanda64_t)ctx.op_tinggi);

    return STATUS_BERHASIL;
}
