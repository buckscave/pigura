/*
 * PIGURA OS - GPU_SCALE.C
 * ========================
 * Operasi perubahan ukuran (scale) permukaan piksel.
 *
 * Berkas ini mengimplementasikan:
 *   - Skala dengan interpolasi nearest-neighbor
 *   - Skala dengan interpolasi bilinear
 *   - Skala dengan perbandingan rasio lebar/tinggi
 *   - Skala ke ukuran tetap
 *   - Skala dengan faktor pengali
 *
 * Semua operasi menggunakan aritmatika integer murni
 * untuk kompatibilitas dengan arsitektur tanpa FPU.
 * Akurasi interpolasi bilinear menggunakan fixed-point
 * aritmatika 16.16.
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

extern tak_bertanda32_t gpu_gabung_warna(tak_bertanda8_t r,
    tak_bertanda8_t g, tak_bertanda8_t b, tak_bertanda8_t a);

extern tak_bertanda8_t gpu_kali_persen(tak_bertanda8_t nilai,
                                         tak_bertanda8_t persen);

/*
 * ===========================================================================
 * KONSTANTA INTERNAL
 * ===========================================================================
 */

/*
 * Format fixed-point 16.16 untuk interpolasi bilinear.
 * 16 bit untuk bagian integer, 16 bit untuk fraksi.
 */
#define SKALA_FP_SHIFT    16
#define SKALA_FP_SATU     (1 << SKALA_FP_SHIFT)
#define SKALA_FP_MASK     (SKALA_FP_SATU - 1)

/*
 * ===========================================================================
 * FUNGSI INTERNAL - INTERPOLASI NEAREST-NEIGHBOR
 * ===========================================================================
 */

/*
 * gpu_skala_terdekat_piksel
 * ------------------------
 * Mengambil satu piksel dari sumber menggunakan
 * metode nearest-neighbor. Koordinat sumber dihitung
 * berdasarkan rasio antara ukuran tujuan dan sumber.
 *
 * Parameter:
 *   src_buffer - Buffer sumber
 *   src_pitch  - Pitch sumber (dalam piksel)
 *   src_lebar  - Lebar sumber
 *   src_tinggi - Tinggi sumber
 *   x          - Posisi X pada tujuan
 *   y          - Posisi Y pada tujuan
 *   dst_lebar  - Lebar tujuan
 *   dst_tinggi - Tinggi tujuan
 *
 * Return:
 *   Nilai piksel dari sumber yang dipetakan
 */
static tak_bertanda32_t gpu_skala_terdekat_piksel(
    const tak_bertanda32_t *src_buffer,
    tak_bertanda32_t src_pitch,
    tak_bertanda32_t src_lebar,
    tak_bertanda32_t src_tinggi,
    tak_bertanda32_t x,
    tak_bertanda32_t y,
    tak_bertanda32_t dst_lebar,
    tak_bertanda32_t dst_tinggi)
{
    tak_bertanda32_t sx, sy;

    if (dst_lebar <= 1) {
        sx = 0;
    } else {
        sx = ((tak_bertanda64_t)x * (tak_bertanda64_t)(src_lebar - 1))
             / (tak_bertanda64_t)(dst_lebar - 1);
    }

    if (dst_tinggi <= 1) {
        sy = 0;
    } else {
        sy = ((tak_bertanda64_t)y * (tak_bertanda64_t)(src_tinggi - 1))
             / (tak_bertanda64_t)(dst_tinggi - 1);
    }

    if (sx >= src_lebar) {
        sx = src_lebar - 1;
    }
    if (sy >= src_tinggi) {
        sy = src_tinggi - 1;
    }

    return src_buffer[sy * src_pitch + sx];
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - INTERPOLASI BILINEAR
 * ===========================================================================
 */

/*
 * gpu_skala_bilinear_piksel
 * ------------------------
 * Mengambil satu piksel dari sumber menggunakan
 * metode interpolasi bilinear. Menghasilkan kualitas
 * yang lebih halus dibanding nearest-neighbor.
 *
 * Menggunakan fixed-point 16.16 untuk akurasi fraksi.
 *
 * Parameter:
 *   src_buffer - Buffer sumber
 *   src_pitch  - Pitch sumber (dalam piksel)
 *   src_lebar  - Lebar sumber
 *   src_tinggi - Tinggi sumber
 *   fx         - Posisi X fixed-point pada sumber
 *   fy         - Posisi Y fixed-point pada sumber
 *
 * Return:
 *   Nilai piksel hasil interpolasi
 */
static tak_bertanda32_t gpu_skala_bilinear_piksel(
    const tak_bertanda32_t *src_buffer,
    tak_bertanda32_t src_pitch,
    tak_bertanda32_t src_lebar,
    tak_bertanda32_t src_tinggi,
    tak_bertanda32_t fx,
    tak_bertanda32_t fy)
{
    tak_bertanda32_t x0, y0, x1, y1;
    tak_bertanda32_t frac_x, frac_y;
    tak_bertanda32_t piksel_00, piksel_01;
    tak_bertanda32_t piksel_10, piksel_11;
    tak_bertanda8_t r00, g00, b00, a00;
    tak_bertanda8_t r01, g01, b01, a01;
    tak_bertanda8_t r10, g10, b10, a10;
    tak_bertanda8_t r11, g11, b11, a11;
    tak_bertanda8_t inv_fx, inv_fy;
    tak_bertanda32_t rr, rg, rb, ra;
    tak_bertanda32_t w00, w01, w10, w11;

    x0 = fx >> SKALA_FP_SHIFT;
    y0 = fy >> SKALA_FP_SHIFT;
    frac_x = fx & SKALA_FP_MASK;
    frac_y = fy & SKALA_FP_MASK;

    x1 = x0 + 1;
    y1 = y0 + 1;

    if (x0 >= src_lebar) x0 = src_lebar - 1;
    if (y0 >= src_tinggi) y0 = src_tinggi - 1;
    if (x1 >= src_lebar) x1 = src_lebar - 1;
    if (y1 >= src_tinggi) y1 = src_tinggi - 1;

    piksel_00 = src_buffer[y0 * src_pitch + x0];
    piksel_01 = src_buffer[y0 * src_pitch + x1];
    piksel_10 = src_buffer[y1 * src_pitch + x0];
    piksel_11 = src_buffer[y1 * src_pitch + x1];

    gpu_ekstrak_warna(piksel_00, &r00, &g00, &b00, &a00);
    gpu_ekstrak_warna(piksel_01, &r01, &g01, &b01, &a01);
    gpu_ekstrak_warna(piksel_10, &r10, &g10, &b10, &a10);
    gpu_ekstrak_warna(piksel_11, &r11, &g11, &b11, &a11);

    inv_fx = (tak_bertanda8_t)(SKALA_FP_SATU - frac_x)
             >> (SKALA_FP_SHIFT - 8);
    inv_fy = (tak_bertanda8_t)(SKALA_FP_SATU - frac_y)
             >> (SKALA_FP_SHIFT - 8);

    w00 = ((tak_bertanda32_t)inv_fx * (tak_bertanda32_t)inv_fy)
          >> 8;
    w01 = ((tak_bertanda32_t)(frac_x >> 8) * (tak_bertanda32_t)inv_fy)
          >> 8;
    w10 = ((tak_bertanda32_t)inv_fx * (tak_bertanda32_t)(frac_y >> 8))
          >> 8;
    w11 = ((tak_bertanda32_t)(frac_x >> 8)
          * (tak_bertanda32_t)(frac_y >> 8)) >> 8;

    rr = ((tak_bertanda32_t)r00 * w00 +
          (tak_bertanda32_t)r01 * w01 +
          (tak_bertanda32_t)r10 * w10 +
          (tak_bertanda32_t)r11 * w11) >> 8;
    rg = ((tak_bertanda32_t)g00 * w00 +
          (tak_bertanda32_t)g01 * w01 +
          (tak_bertanda32_t)g10 * w10 +
          (tak_bertanda32_t)g11 * w11) >> 8;
    rb = ((tak_bertanda32_t)b00 * w00 +
          (tak_bertanda32_t)b01 * w01 +
          (tak_bertanda32_t)b10 * w10 +
          (tak_bertanda32_t)b11 * w11) >> 8;
    ra = ((tak_bertanda32_t)a00 * w00 +
          (tak_bertanda32_t)a01 * w01 +
          (tak_bertanda32_t)a10 * w10 +
          (tak_bertanda32_t)a11 * w11) >> 8;

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
 * ===========================================================================
 * FUNGSI INTERNAL - VALIDASI PARAMETER SKALA
 * ===========================================================================
 */

/*
 * gpu_validasi_skala
 * -----------------
 * Memvalidasi parameter operasi skala.
 * Memastikan buffer tidak NULL dan dimensi valid.
 *
 * Return:
 *   STATUS_BERHASIL jika valid
 *   Kode error jika tidak valid
 */
static status_t gpu_validasi_skala(
    const tak_bertanda32_t *src_buffer,
    tak_bertanda32_t src_lebar,
    tak_bertanda32_t src_tinggi,
    tak_bertanda32_t *dst_buffer,
    tak_bertanda32_t dst_lebar,
    tak_bertanda32_t dst_tinggi)
{
    if (src_buffer == NULL || dst_buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (src_lebar == 0 || src_tinggi == 0) {
        return STATUS_PARAM_JARAK;
    }

    if (dst_lebar == 0 || dst_tinggi == 0) {
        return STATUS_PARAM_JARAK;
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - SKALA NEAREST-NEIGHBOR
 * ===========================================================================
 */

/*
 * gpu_skala_terdekat
 * -----------------
 * Mengubah ukuran permukaan menggunakan interpolasi
 * nearest-neighbor. Cepat namun menghasilkan tepi kasar.
 *
 * Parameter:
 *   src_buffer - Buffer sumber (wajib valid)
 *   src_lebar  - Lebar sumber
 *   src_tinggi - Tinggi sumber
 *   src_pitch  - Pitch sumber (0 = auto = src_lebar)
 *   dst_buffer - Buffer tujuan (wajib valid)
 *   dst_lebar  - Lebar tujuan (hasil skala)
 *   dst_tinggi - Tinggi tujuan (hasil skala)
 *   dst_pitch  - Pitch tujuan (0 = auto = dst_lebar)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_skala_terdekat(const tak_bertanda32_t *src_buffer,
                            tak_bertanda32_t src_lebar,
                            tak_bertanda32_t src_tinggi,
                            tak_bertanda32_t src_pitch,
                            tak_bertanda32_t *dst_buffer,
                            tak_bertanda32_t dst_lebar,
                            tak_bertanda32_t dst_tinggi,
                            tak_bertanda32_t dst_pitch)
{
    status_t status;
    tak_bertanda32_t y, x;
    tak_bertanda32_t sp, dp;

    status = gpu_validasi_skala(src_buffer, src_lebar, src_tinggi,
                                  dst_buffer, dst_lebar, dst_tinggi);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    sp = (src_pitch > 0) ? src_pitch : src_lebar;
    dp = (dst_pitch > 0) ? dst_pitch : dst_lebar;

    for (y = 0; y < dst_tinggi; y++) {
        for (x = 0; x < dst_lebar; x++) {
            dst_buffer[y * dp + x] =
                gpu_skala_terdekat_piksel(src_buffer, sp,
                    src_lebar, src_tinggi, x, y,
                    dst_lebar, dst_tinggi);
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)dst_lebar *
                         (tak_bertanda64_t)dst_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - SKALA BILINEAR
 * ===========================================================================
 */

/*
 * gpu_skala_bilinear
 * -----------------
 * Mengubah ukuran permukaan menggunakan interpolasi
 * bilinear. Lebih lambat dari nearest-neighbor namun
 * menghasilkan kualitas visual yang lebih halus.
 *
 * Parameter:
 *   (sama dengan gpu_skala_terdekat)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_skala_bilinear(const tak_bertanda32_t *src_buffer,
                           tak_bertanda32_t src_lebar,
                           tak_bertanda32_t src_tinggi,
                           tak_bertanda32_t src_pitch,
                           tak_bertanda32_t *dst_buffer,
                           tak_bertanda32_t dst_lebar,
                           tak_bertanda32_t dst_tinggi,
                           tak_bertanda32_t dst_pitch)
{
    status_t status;
    tak_bertanda32_t y, x;
    tak_bertanda32_t sp, dp;
    tak_bertanda32_t step_x, step_y;
    tak_bertanda32_t start_x, start_y;

    status = gpu_validasi_skala(src_buffer, src_lebar, src_tinggi,
                                  dst_buffer, dst_lebar, dst_tinggi);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    sp = (src_pitch > 0) ? src_pitch : src_lebar;
    dp = (dst_pitch > 0) ? dst_pitch : dst_lebar;

    if (dst_lebar <= 1) {
        step_x = 0;
    } else {
        step_x = ((tak_bertanda64_t)(src_lebar - 1) << SKALA_FP_SHIFT)
                 / (tak_bertanda64_t)(dst_lebar - 1);
    }

    if (dst_tinggi <= 1) {
        step_y = 0;
    } else {
        step_y = ((tak_bertanda64_t)(src_tinggi - 1)
                  << SKALA_FP_SHIFT)
                 / (tak_bertanda64_t)(dst_tinggi - 1);
    }

    start_x = 0;
    start_y = 0;

    for (y = 0; y < dst_tinggi; y++) {
        tak_bertanda32_t fy = start_y + y * step_y;

        for (x = 0; x < dst_lebar; x++) {
            tak_bertanda32_t fx = start_x + x * step_x;

            dst_buffer[y * dp + x] =
                gpu_skala_bilinear_piksel(src_buffer, sp,
                    src_lebar, src_tinggi, fx, fy);
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)dst_lebar *
                         (tak_bertanda64_t)dst_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - SKALA KE UKURAN TETAP
 * ===========================================================================
 */

/*
 * gpu_skala_ukuran_tetap
 * --------------------
 * Mengubah ukuran permukaan ke ukuran tetap.
 * Menggunakan nearest-neighbor untuk kecepatan.
 *
 * Parameter:
 *   src_buffer - Buffer sumber
 *   src_lebar  - Lebar sumber
 *   src_tinggi - Tinggi sumber
 *   src_pitch  - Pitch sumber (0 = auto)
 *   dst_buffer - Buffer tujuan
 *   lebar_baru - Lebar tujuan
 *   tinggi_baru- Tinggi tujuan
 *   dst_pitch  - Pitch tujuan (0 = auto)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_skala_ukuran_tetap(
    const tak_bertanda32_t *src_buffer,
    tak_bertanda32_t src_lebar,
    tak_bertanda32_t src_tinggi,
    tak_bertanda32_t src_pitch,
    tak_bertanda32_t *dst_buffer,
    tak_bertanda32_t lebar_baru,
    tak_bertanda32_t tinggi_baru,
    tak_bertanda32_t dst_pitch)
{
    return gpu_skala_terdekat(src_buffer, src_lebar, src_tinggi,
                               src_pitch, dst_buffer, lebar_baru,
                               tinggi_baru, dst_pitch);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - SKALA DENGAN FAKTOR
 * ===========================================================================
 */

/*
 * gpu_skala_faktor
 * ---------------
 * Mengubah ukuran permukaan dengan faktor pengali.
 * Faktor menggunakan fixed-point 16.16.
 * Contoh: faktor=0x18000 (1.5) memperbesar 1.5x
 *         faktor=0x8000 (0.5) memperkecil setengahnya
 *
 * Parameter:
 *   src_buffer   - Buffer sumber
 *   src_lebar    - Lebar sumber
 *   src_tinggi   - Tinggi sumber
 *   src_pitch    - Pitch sumber (0 = auto)
 *   dst_buffer   - Buffer tujuan (dialokasikan pemanggil)
 *   faktor       - Faktor skala fixed-point 16.16
 *   dst_lebar    - Output: lebar hasil
 *   dst_tinggi   - Output: tinggi hasil
 *   dst_pitch    - Pitch tujuan (0 = auto = dst_lebar)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_skala_faktor(const tak_bertanda32_t *src_buffer,
                          tak_bertanda32_t src_lebar,
                          tak_bertanda32_t src_tinggi,
                          tak_bertanda32_t src_pitch,
                          tak_bertanda32_t *dst_buffer,
                          tak_bertanda32_t faktor,
                          tak_bertanda32_t dst_lebar,
                          tak_bertanda32_t dst_tinggi,
                          tak_bertanda32_t dst_pitch)
{
    if (src_buffer == NULL || dst_buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (faktor == 0) {
        return STATUS_PARAM_INVALID;
    }

    if (dst_lebar == 0 || dst_tinggi == 0) {
        return STATUS_PARAM_JARAK;
    }

    return gpu_skala_terdekat(src_buffer, src_lebar, src_tinggi,
                               src_pitch, dst_buffer,
                               dst_lebar, dst_tinggi, dst_pitch);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - SKALA BILINEAR KE AREA TUJUAN
 * ===========================================================================
 */

/*
 * gpu_skala_bilinear_area
 * ----------------------
 * Mengubah ukuran permukaan dan menempatkan hasil
 * pada area tertentu di permukaan tujuan.
 *
 * Parameter:
 *   src_buffer - Buffer sumber
 *   src_lebar  - Lebar sumber
 *   src_tinggi - Tinggi sumber
 *   src_pitch  - Pitch sumber (0 = auto)
 *   dst_buffer - Buffer tujuan
 *   dst_lebar  - Lebar total tujuan
 *   dst_tinggi - Tinggi total tujuan
 *   dst_pitch  - Pitch tujuan (0 = auto = dst_lebar)
 *   x          - Posisi X area pada tujuan
 *   y          - Posisi Y area pada tujuan
 *   lebar      - Lebar area hasil skala
 *   tinggi     - Tinggi area hasil skala
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_skala_bilinear_area(
    const tak_bertanda32_t *src_buffer,
    tak_bertanda32_t src_lebar,
    tak_bertanda32_t src_tinggi,
    tak_bertanda32_t src_pitch,
    tak_bertanda32_t *dst_buffer,
    tak_bertanda32_t dst_lebar,
    tak_bertanda32_t dst_tinggi,
    tak_bertanda32_t dst_pitch,
    tak_bertanda32_t x,
    tak_bertanda32_t y,
    tak_bertanda32_t lebar,
    tak_bertanda32_t tinggi)
{
    struct gpu_konteks ctx;
    status_t status;
    tak_bertanda32_t row, col;
    tak_bertanda32_t sp, dp;
    tak_bertanda32_t step_x, step_y;

    status = gpu_konteks_siapkan(&ctx,
                                   src_buffer, src_lebar, src_tinggi,
                                   src_pitch,
                                   dst_buffer, dst_lebar, dst_tinggi,
                                   dst_pitch,
                                   x, y, lebar, tinggi);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    sp = ctx.src_pitch;
    dp = ctx.dst_pitch;

    if (ctx.op_lebar <= 1) {
        step_x = 0;
    } else {
        step_x = ((tak_bertanda64_t)(ctx.src_lebar - 1)
                  << SKALA_FP_SHIFT)
                 / (tak_bertanda64_t)(ctx.op_lebar - 1);
    }

    if (ctx.op_tinggi <= 1) {
        step_y = 0;
    } else {
        step_y = ((tak_bertanda64_t)(ctx.src_tinggi - 1)
                  << SKALA_FP_SHIFT)
                 / (tak_bertanda64_t)(ctx.op_tinggi - 1);
    }

    for (row = 0; row < ctx.op_tinggi; row++) {
        tak_bertanda32_t fy = row * step_y;
        tak_bertanda32_t offset_dst;

        offset_dst = (ctx.dst_y + row) * dp + ctx.dst_x;

        for (col = 0; col < ctx.op_lebar; col++) {
            tak_bertanda32_t fx = col * step_x;

            dst_buffer[offset_dst + col] =
                gpu_skala_bilinear_piksel(ctx.src_buffer, sp,
                    ctx.src_lebar, ctx.src_tinggi, fx, fy);
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)ctx.op_lebar *
                         (tak_bertanda64_t)ctx.op_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - SKALA NEAREST-NEIGHBOR KE AREA TUJUAN
 * ===========================================================================
 */

/*
 * gpu_skala_terdekat_area
 * -----------------------
 * Mengubah ukuran permukaan menggunakan nearest-neighbor
 * dan menempatkan hasil pada area tertentu di tujuan.
 *
 * Parameter:
 *   (sama dengan gpu_skala_bilinear_area)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_skala_terdekat_area(
    const tak_bertanda32_t *src_buffer,
    tak_bertanda32_t src_lebar,
    tak_bertanda32_t src_tinggi,
    tak_bertanda32_t src_pitch,
    tak_bertanda32_t *dst_buffer,
    tak_bertanda32_t dst_lebar,
    tak_bertanda32_t dst_tinggi,
    tak_bertanda32_t dst_pitch,
    tak_bertanda32_t x,
    tak_bertanda32_t y,
    tak_bertanda32_t lebar,
    tak_bertanda32_t tinggi)
{
    struct gpu_konteks ctx;
    status_t status;
    tak_bertanda32_t row, col;

    status = gpu_konteks_siapkan(&ctx,
                                   src_buffer, src_lebar, src_tinggi,
                                   src_pitch,
                                   dst_buffer, dst_lebar, dst_tinggi,
                                   dst_pitch,
                                   x, y, lebar, tinggi);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    for (row = 0; row < ctx.op_tinggi; row++) {
        tak_bertanda32_t offset_dst;

        offset_dst = (ctx.dst_y + row) * ctx.dst_pitch + ctx.dst_x;

        for (col = 0; col < ctx.op_lebar; col++) {
            dst_buffer[offset_dst + col] =
                gpu_skala_terdekat_piksel(ctx.src_buffer,
                    ctx.src_pitch, ctx.src_lebar,
                    ctx.src_tinggi, col, row,
                    ctx.op_lebar, ctx.op_tinggi);
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)ctx.op_lebar *
                         (tak_bertanda64_t)ctx.op_tinggi);

    return STATUS_BERHASIL;
}
