/*
 * PIGURA OS - GPU_ROTATE.C
 * ========================
 * Operasi rotasi permukaan piksel.
 *
 * Berkas ini mengimplementasikan:
 *   - Rotasi 90 derajat (searah dan berlawanan jarum jam)
 *   - Rotasi 180 derajat
 *   - Rotasi 270 derajat
 *   - Rotasi sudut bebas dengan interpolasi bilinear
 *   - Rotasi dengan background fill
 *
 * Semua operasi menggunakan aritmatika integer murni.
 * Rotasi sudut bebas menggunakan fixed-point 16.16
 * untuk akurasi trigonometri tanpa floating point.
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
 * Tabel sinus dan kosinus fixed-point 16.16 untuk sudut 0-359.
 * Dihasilkan dari: sin(angle * PI / 180) * 65536
 * Kosinus = sinus sudut (90 - angle)
 */
#define ROTASI_FP_SHIFT    16
#define ROTASI_FP_SATU     (1 << ROTASI_FP_SHIFT)

#define ROTASI_90_DERAJAT  1
#define ROTASI_180_DERAJAT 2
#define ROTASI_270_DERAJAT 3

/*
 * Ukuran tabel trigonometri (0 sampai 359 derajat).
 */
#define ROTASI_TABEL_SUDUT 360

/*
 * ===========================================================================
 * TABEL TRIGONOMETRI FIXED-POINT
 * ===========================================================================
 * Tabel sinus fixed-point 16.16 untuk 0-359 derajat.
 * Nilai = sin(sudut * PI / 180) * 65536.
 * Kosinus dapat dihitung: cos(x) = sin(90 - x)
 */

static const tanda32_t tabel_sin_fp16[ROTASI_TABEL_SUDUT] = {
        0,   1144,   2287,   3430,   4572,   5712,   6850,   7987,
     9121,  10252,  11380,  12505,  13626,  14742,  15855,  16962,
    18064,  19161,  20252,  21336,  22415,  23486,  24550,  25607,
    26656,  27697,  28729,  29753,  30767,  31772,  32768,  33754,
    34729,  35693,  36647,  37590,  38521,  39441,  40348,  41243,
    42126,  42995,  43852,  44695,  45525,  46341,  47143,  47930,
    48703,  49461,  50203,  50931,  51643,  52339,  53020,  53684,
    54332,  54963,  55578,  56175,  56756,  57319,  57865,  58393,
    58903,  59396,  59870,  60326,  60764,  61183,  61584,  61966,
    62328,  62672,  62997,  63303,  63589,  63856,  64104,  64332,
    64540,  64729,  64898,  65048,  65177,  65287,  65376,  65446,
    65496,  65526,  65536,  65526,  65496,  65446,  65376,  65287,
    65177,  65048,  64898,  64729,  64540,  64332,  64104,  63856,
    63589,  63303,  62997,  62672,  62328,  61966,  61584,  61183,
    60764,  60326,  59870,  59396,  58903,  58393,  57865,  57319,
    56756,  56175,  55578,  54963,  54332,  53684,  53020,  52339,
    51643,  50931,  50203,  49461,  48703,  47930,  47143,  46341,
    45525,  44695,  43852,  42995,  42126,  41243,  40348,  39441,
    38521,  37590,  36647,  35693,  34729,  33754,  32768,  31772,
    30767,  29753,  28729,  27697,  26656,  25607,  24550,  23486,
    22415,  21336,  20252,  19161,  18064,  16962,  15855,  14742,
    13626,  12505,  11380,  10252,   9121,   7987,   6850,   5712,
     4572,   3430,   2287,   1144,      0,  -1144,  -2287,  -3430,
    -4572,  -5712,  -6850,  -7987,  -9121, -10252, -11380, -12505,
   -13626, -14742, -15855, -16962, -18064, -19161, -20252, -21336,
   -22415, -23486, -24550, -25607, -26656, -27697, -28729, -29753,
   -30767, -31772, -32768, -33754, -34729, -35693, -36647, -37590,
   -38521, -39441, -40348, -41243, -42126, -42995, -43852, -44695,
   -45525, -46341, -47143, -47930, -48703, -49461, -50203, -50931,
   -51643, -52339, -53020, -53684, -54332, -54963, -55578, -56175,
   -56756, -57319, -57865, -58393, -58903, -59396, -59870, -60326,
   -60764, -61183, -61584, -61966, -62328, -62672, -62997, -63303,
   -63589, -63856, -64104, -64332, -64540, -64729, -64898, -65048,
   -65177, -65287, -65376, -65446, -65496, -65526, -65536, -65526,
   -65496, -65446, -65376, -65287, -65177, -65048, -64898, -64729,
   -64540, -64332, -64104, -63856, -63589, -63303, -62997, -62672,
   -62328, -61966, -61584, -61183, -60764, -60326, -59870, -59396,
   -58903, -58393, -57865, -57319, -56756, -56175, -55578, -54963,
   -54332, -53684, -53020, -52339, -51643, -50931, -50203, -49461,
   -48703, -47930, -47143, -46341, -45525, -44695, -43852, -42995,
   -42126, -41243, -40348, -39441, -38521, -37590, -36647, -35693,
   -34729, -33754, -32768, -31772, -30767, -29753, -28729, -27697,
   -26656, -25607, -24550, -23486, -22415, -21336, -20252, -19161,
   -18064, -16962, -15855, -14742, -13626, -12505, -11380, -10252,
    -9121,  -7987,  -6850,  -5712,  -4572,  -3430,  -2287,  -1144
};

/*
 * ===========================================================================
 * FUNGSI INTERNAL - TRIGONOMETRI
 * ===========================================================================
 */

/*
 * gpu_sin_fp16
 * -----------
 * Mendapatkan nilai sinus fixed-point 16.16
 * untuk sudut dalam derajat.
 *
 * Parameter:
 *   sudut - Sudut dalam derajat (0-359)
 *
 * Return:
 *   Nilai sinus fixed-point 16.16
 */
static tanda32_t gpu_sin_fp16(tak_bertanda32_t sudut)
{
    sudut = sudut % 360;
    return tabel_sin_fp16[sudut] << 16;
}

/*
 * gpu_cos_fp16
 * -----------
 * Mendapatkan nilai kosinus fixed-point 16.16.
 * cos(x) = sin(90 - x).
 *
 * Parameter:
 *   sudut - Sudut dalam derajat (0-359)
 *
 * Return:
 *   Nilai kosinus fixed-point 16.16
 */
static tanda32_t gpu_cos_fp16(tak_bertanda32_t sudut)
{
    tak_bertanda32_t sudut_cos;

    if (sudut >= 90) {
        sudut_cos = sudut - 90;
    } else {
        sudut_cos = 90 - sudut;
    }
    sudut_cos = sudut_cos % 360;

    return tabel_sin_fp16[sudut_cos] << 16;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - ROTASI SUDUT BEBAS
 * ===========================================================================
 */

/*
 * gpu_putar_piksel_bilinear
 * ------------------------
 * Mengambil piksel dari sumber pada koordinat
 * floating-point menggunakan interpolasi bilinear.
 *
 * Parameter:
 *   src_buffer - Buffer sumber
 *   src_pitch  - Pitch sumber
 *   src_lebar  - Lebar sumber
 *   src_tinggi - Tinggi sumber
 *   fx         - Koordinat X fixed-point 16.16
 *   fy         - Koordinat Y fixed-point 16.16
 *   warna_bg   - Warna background untuk area luar
 *
 * Return:
 *   Nilai piksel hasil interpolasi
 */
static tak_bertanda32_t gpu_putar_piksel_bilinear(
    const tak_bertanda32_t *src_buffer,
    tak_bertanda32_t src_pitch,
    tak_bertanda32_t src_lebar,
    tak_bertanda32_t src_tinggi,
    tak_bertanda32_t fx,
    tak_bertanda32_t fy,
    tak_bertanda32_t warna_bg)
{
    tak_bertanda8_t r00, g00, b00, a00;
    tak_bertanda8_t r01, g01, b01, a01;
    tak_bertanda8_t r10, g10, b10, a10;
    tak_bertanda8_t r11, g11, b11, a11;
    tak_bertanda8_t br, bg_, bb, ba;
    tak_bertanda32_t x0, y0, x1, y1;
    tak_bertanda32_t frac_x, frac_y;
    tak_bertanda32_t w00, w01, w10, w11;
    tak_bertanda32_t rr, rg, rb, ra;
    tak_bertanda8_t inv_fx8, inv_fy8;
    bool_t valid_00, valid_01, valid_10, valid_11;

    gpu_ekstrak_warna(warna_bg, &br, &bg_, &bb, &ba);

    x0 = fx >> ROTASI_FP_SHIFT;
    y0 = fy >> ROTASI_FP_SHIFT;

    frac_x = fx & ((1 << ROTASI_FP_SHIFT) - 1);
    frac_y = fy & ((1 << ROTASI_FP_SHIFT) - 1);

    x1 = x0 + 1;
    y1 = y0 + 1;

    valid_00 = (x0 < src_lebar && y0 < src_tinggi) ? BENAR : SALAH;
    valid_01 = (x1 < src_lebar && y0 < src_tinggi) ? BENAR : SALAH;
    valid_10 = (x0 < src_lebar && y1 < src_tinggi) ? BENAR : SALAH;
    valid_11 = (x1 < src_lebar && y1 < src_tinggi) ? BENAR : SALAH;

    if (valid_00) {
        gpu_ekstrak_warna(src_buffer[y0 * src_pitch + x0],
                           &r00, &g00, &b00, &a00);
    } else {
        r00 = br; g00 = bg_; b00 = bb; a00 = ba;
    }

    if (valid_01) {
        gpu_ekstrak_warna(src_buffer[y0 * src_pitch + x1],
                           &r01, &g01, &b01, &a01);
    } else {
        r01 = br; g01 = bg_; b01 = bb; a01 = ba;
    }

    if (valid_10) {
        gpu_ekstrak_warna(src_buffer[y1 * src_pitch + x0],
                           &r10, &g10, &b10, &a10);
    } else {
        r10 = br; g10 = bg_; b10 = bb; a10 = ba;
    }

    if (valid_11) {
        gpu_ekstrak_warna(src_buffer[y1 * src_pitch + x1],
                           &r11, &g11, &b11, &a11);
    } else {
        r11 = br; g11 = bg_; b11 = bb; a11 = ba;
    }

    inv_fx8 = (tak_bertanda8_t)((1 << ROTASI_FP_SHIFT) - frac_x)
              >> (ROTASI_FP_SHIFT - 8);
    inv_fy8 = (tak_bertanda8_t)((1 << ROTASI_FP_SHIFT) - frac_y)
              >> (ROTASI_FP_SHIFT - 8);

    w00 = ((tak_bertanda32_t)inv_fx8 * (tak_bertanda32_t)inv_fy8)
          >> 8;
    w01 = ((tak_bertanda32_t)(frac_x >> (ROTASI_FP_SHIFT - 8))
          * (tak_bertanda32_t)inv_fy8) >> 8;
    w10 = ((tak_bertanda32_t)inv_fx8
          * (tak_bertanda32_t)(frac_y >> (ROTASI_FP_SHIFT - 8)))
          >> 8;
    w11 = ((tak_bertanda32_t)(frac_x >> (ROTASI_FP_SHIFT - 8))
          * (tak_bertanda32_t)(frac_y >> (ROTASI_FP_SHIFT - 8)))
          >> 8;

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
 * FUNGSI PUBLIK - ROTASI 90 DERAJAT (SEARAH JARUM JAM)
 * ===========================================================================
 */

/*
 * gpu_putar_90
 * -----------
 * Memutar permukaan 90 derajat searah jarum jam.
 * Lebar sumber menjadi tinggi tujuan, dan
 * tinggi sumber menjadi lebar tujuan.
 *
 * Parameter:
 *   src_buffer - Buffer sumber
 *   src_lebar  - Lebar sumber
 *   src_tinggi - Tinggi sumber
 *   src_pitch  - Pitch sumber (0 = auto)
 *   dst_buffer - Buffer tujuan
 *   dst_pitch  - Pitch tujuan (0 = auto = src_tinggi)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_putar_90(const tak_bertanda32_t *src_buffer,
                      tak_bertanda32_t src_lebar,
                      tak_bertanda32_t src_tinggi,
                      tak_bertanda32_t src_pitch,
                      tak_bertanda32_t *dst_buffer,
                      tak_bertanda32_t dst_pitch)
{
    tak_bertanda32_t y, x;
    tak_bertanda32_t sp, dp;

    if (src_buffer == NULL || dst_buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (src_lebar == 0 || src_tinggi == 0) {
        return STATUS_PARAM_JARAK;
    }

    sp = (src_pitch > 0) ? src_pitch : src_lebar;
    dp = (dst_pitch > 0) ? dst_pitch : src_tinggi;

    for (y = 0; y < src_tinggi; y++) {
        for (x = 0; x < src_lebar; x++) {
            dst_buffer[x * dp + (src_tinggi - 1 - y)] =
                src_buffer[y * sp + x];
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)src_lebar *
                         (tak_bertanda64_t)src_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - ROTASI 180 DERAJAT
 * ===========================================================================
 */

/*
 * gpu_putar_180
 * ------------
 * Memutar permukaan 180 derajat.
 * Ukuran tujuan sama dengan ukuran sumber.
 *
 * Parameter:
 *   src_buffer - Buffer sumber
 *   src_lebar  - Lebar sumber
 *   src_tinggi - Tinggi sumber
 *   src_pitch  - Pitch sumber (0 = auto)
 *   dst_buffer - Buffer tujuan
 *   dst_pitch  - Pitch tujuan (0 = auto = src_lebar)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_putar_180(const tak_bertanda32_t *src_buffer,
                       tak_bertanda32_t src_lebar,
                       tak_bertanda32_t src_tinggi,
                       tak_bertanda32_t src_pitch,
                       tak_bertanda32_t *dst_buffer,
                       tak_bertanda32_t dst_pitch)
{
    tak_bertanda32_t y, x;
    tak_bertanda32_t sp, dp;
    tak_bertanda32_t dst_y_maks, dst_x_maks;

    if (src_buffer == NULL || dst_buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (src_lebar == 0 || src_tinggi == 0) {
        return STATUS_PARAM_JARAK;
    }

    sp = (src_pitch > 0) ? src_pitch : src_lebar;
    dp = (dst_pitch > 0) ? dst_pitch : src_lebar;

    dst_y_maks = src_tinggi - 1;
    dst_x_maks = src_lebar - 1;

    for (y = 0; y < src_tinggi; y++) {
        for (x = 0; x < src_lebar; x++) {
            dst_buffer[(dst_y_maks - y) * dp +
                       (dst_x_maks - x)] =
                src_buffer[y * sp + x];
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)src_lebar *
                         (tak_bertanda64_t)src_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - ROTASI 270 DERAJAT
 * ===========================================================================
 */

/*
 * gpu_putar_270
 * ------------
 * Memutar permukaan 270 derajat (90 derajat
 * berlawanan jarum jam).
 *
 * Parameter:
 *   (sama dengan gpu_putar_90)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_putar_270(const tak_bertanda32_t *src_buffer,
                       tak_bertanda32_t src_lebar,
                       tak_bertanda32_t src_tinggi,
                       tak_bertanda32_t src_pitch,
                       tak_bertanda32_t *dst_buffer,
                       tak_bertanda32_t dst_pitch)
{
    tak_bertanda32_t y, x;
    tak_bertanda32_t sp, dp;

    if (src_buffer == NULL || dst_buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (src_lebar == 0 || src_tinggi == 0) {
        return STATUS_PARAM_JARAK;
    }

    sp = (src_pitch > 0) ? src_pitch : src_lebar;
    dp = (dst_pitch > 0) ? dst_pitch : src_tinggi;

    for (y = 0; y < src_tinggi; y++) {
        for (x = 0; x < src_lebar; x++) {
            dst_buffer[(src_lebar - 1 - x) * dp + y] =
                src_buffer[y * sp + x];
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)src_lebar *
                         (tak_bertanda64_t)src_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - ROTASI SUDUT BEBAS
 * ===========================================================================
 */

/*
 * gpu_putar_sudut
 * --------------
 * Memutar permukaan dengan sudut bebas (0-359 derajat).
 * Menggunakan interpolasi bilinear dan tabel
 * trigonometri fixed-point.
 *
 * Area yang tidak tertutup sumber akan diisi
 * dengan warna background.
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
 *   sudut      - Sudut rotasi dalam derajat (0-359)
 *   warna_bg   - Warna background (0xFF000000 = hitam opaque)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_putar_sudut(const tak_bertanda32_t *src_buffer,
                         tak_bertanda32_t src_lebar,
                         tak_bertanda32_t src_tinggi,
                         tak_bertanda32_t src_pitch,
                         tak_bertanda32_t *dst_buffer,
                         tak_bertanda32_t dst_lebar,
                         tak_bertanda32_t dst_tinggi,
                         tak_bertanda32_t dst_pitch,
                         tak_bertanda32_t sudut,
                         tak_bertanda32_t warna_bg)
{
    tak_bertanda32_t y, x;
    tak_bertanda32_t sp, dp;
    tanda32_t sin_a, cos_a;
    tak_bertanda32_t cx_s, cy_s;
    tak_bertanda32_t cx_d, cy_d;

    if (src_buffer == NULL || dst_buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (src_lebar == 0 || src_tinggi == 0 ||
        dst_lebar == 0 || dst_tinggi == 0) {
        return STATUS_PARAM_JARAK;
    }

    sp = (src_pitch > 0) ? src_pitch : src_lebar;
    dp = (dst_pitch > 0) ? dst_pitch : dst_lebar;

    sudut = sudut % 360;

    sin_a = gpu_sin_fp16(sudut);
    cos_a = gpu_cos_fp16(sudut);

    cx_s = (src_lebar - 1) << ROTASI_FP_SHIFT;
    cy_s = (src_tinggi - 1) << ROTASI_FP_SHIFT;
    cx_d = (dst_lebar - 1) << ROTASI_FP_SHIFT;
    cy_d = (dst_tinggi - 1) << ROTASI_FP_SHIFT;

    for (y = 0; y < dst_tinggi; y++) {
        tak_bertanda32_t fy_d = y << ROTASI_FP_SHIFT;
        tak_bertanda32_t dy = fy_d - cy_d;

        for (x = 0; x < dst_lebar; x++) {
            tak_bertanda32_t fx_d = x << ROTASI_FP_SHIFT;
            tak_bertanda32_t dx = fx_d - cx_d;
            tak_bertanda32_t fx_s, fy_s;
            tanda64_t sx_64, sy_64;

            sx_64 = ((tanda64_t)dx * (tanda64_t)cos_a +
                     (tanda64_t)dy * (tanda64_t)sin_a);
            sy_64 = ((tanda64_t)(tanda32_t)(-dx) *
                     (tanda64_t)sin_a +
                     (tanda64_t)dy * (tanda64_t)cos_a);

            fx_s = (tak_bertanda32_t)(sx_64 >> 16) + cx_s;
            fy_s = (tak_bertanda32_t)(sy_64 >> 16) + cy_s;

            dst_buffer[y * dp + x] =
                gpu_putar_piksel_bilinear(src_buffer, sp,
                    src_lebar, src_tinggi,
                    fx_s, fy_s, warna_bg);
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)dst_lebar *
                         (tak_bertanda64_t)dst_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - ROTASI KE AREA TUJUAN
 * ===========================================================================
 */

/*
 * gpu_putar_sudut_area
 * -------------------
 * Memutar permukaan dengan sudut bebas dan menempatkan
 * hasil pada area tertentu di permukaan tujuan.
 *
 * Parameter:
 *   src_buffer - Buffer sumber
 *   src_lebar  - Lebar sumber
 *   src_tinggi - Tinggi sumber
 *   src_pitch  - Pitch sumber (0 = auto)
 *   dst_buffer - Buffer tujuan
 *   dst_lebar  - Lebar total tujuan
 *   dst_tinggi - Tinggi total tujuan
 *   dst_pitch  - Pitch tujuan (0 = auto)
 *   dst_x      - Posisi X area pada tujuan
 *   dst_y      - Posisi Y area pada tujuan
 *   area_lebar - Lebar area hasil
 *   area_tinggi- Tinggi area hasil
 *   sudut      - Sudut rotasi (0-359)
 *   warna_bg   - Warna background
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_putar_sudut_area(const tak_bertanda32_t *src_buffer,
                              tak_bertanda32_t src_lebar,
                              tak_bertanda32_t src_tinggi,
                              tak_bertanda32_t src_pitch,
                              tak_bertanda32_t *dst_buffer,
                              tak_bertanda32_t dst_lebar,
                              tak_bertanda32_t dst_tinggi,
                              tak_bertanda32_t dst_pitch,
                              tak_bertanda32_t dst_x,
                              tak_bertanda32_t dst_y,
                              tak_bertanda32_t area_lebar,
                              tak_bertanda32_t area_tinggi,
                              tak_bertanda32_t sudut,
                              tak_bertanda32_t warna_bg)
{
    tak_bertanda32_t y, x;
    tak_bertanda32_t sp, dp;
    tanda32_t sin_a, cos_a;
    tak_bertanda32_t cx_s, cy_s;
    tak_bertanda32_t cx_a, cy_a;

    if (src_buffer == NULL || dst_buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (src_lebar == 0 || src_tinggi == 0 ||
        area_lebar == 0 || area_tinggi == 0) {
        return STATUS_PARAM_JARAK;
    }

    sp = (src_pitch > 0) ? src_pitch : src_lebar;
    dp = (dst_pitch > 0) ? dst_pitch : dst_lebar;

    sudut = sudut % 360;

    sin_a = gpu_sin_fp16(sudut);
    cos_a = gpu_cos_fp16(sudut);

    cx_s = (src_lebar - 1) << ROTASI_FP_SHIFT;
    cy_s = (src_tinggi - 1) << ROTASI_FP_SHIFT;
    cx_a = (area_lebar - 1) << ROTASI_FP_SHIFT;
    cy_a = (area_tinggi - 1) << ROTASI_FP_SHIFT;

    for (y = 0; y < area_tinggi; y++) {
        tak_bertanda32_t fy_a = y << ROTASI_FP_SHIFT;
        tak_bertanda32_t dy = fy_a - cy_a;

        for (x = 0; x < area_lebar; x++) {
            tak_bertanda32_t fx_a = x << ROTASI_FP_SHIFT;
            tak_bertanda32_t dx = fx_a - cx_a;
            tak_bertanda32_t fx_s, fy_s;
            tanda64_t sx_64, sy_64;
            tak_bertanda32_t py = dst_y + y;
            tak_bertanda32_t px = dst_x + x;

            if (py >= dst_tinggi || px >= dst_lebar) {
                continue;
            }

            sx_64 = ((tanda64_t)dx * (tanda64_t)cos_a +
                     (tanda64_t)dy * (tanda64_t)sin_a);
            sy_64 = ((tanda64_t)(tanda32_t)(-dx) *
                     (tanda64_t)sin_a +
                     (tanda64_t)dy * (tanda64_t)cos_a);

            fx_s = (tak_bertanda32_t)(sx_64 >> 16) + cx_s;
            fy_s = (tak_bertanda32_t)(sy_64 >> 16) + cy_s;

            dst_buffer[py * dp + px] =
                gpu_putar_piksel_bilinear(src_buffer, sp,
                    src_lebar, src_tinggi,
                    fx_s, fy_s, warna_bg);
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)area_lebar *
                         (tak_bertanda64_t)area_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - ROTASI DENGAN PILIHAN SUDUT KUNCI
 * ===========================================================================
 */

/*
 * gpu_putar
 * ---------
 * Memutar permukaan berdasarkan pengali sudut kunci.
 * 0 = tidak rotasi, 1 = 90 derajat CW,
 * 2 = 180 derajat, 3 = 270 derajat CW.
 *
 * Parameter:
 *   src_buffer - Buffer sumber
 *   src_lebar  - Lebar sumber
 *   src_tinggi - Tinggi sumber
 *   src_pitch  - Pitch sumber (0 = auto)
 *   dst_buffer - Buffer tujuan
 *   dst_pitch  - Pitch tujuan (0 = auto)
 *   sudut      - Pengali sudut (0, 1, 2, 3)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   STATUS_PARAM_INVALID jika sudut > 3
 *   Kode error lain jika parameter tidak valid
 */
status_t gpu_putar(const tak_bertanda32_t *src_buffer,
                   tak_bertanda32_t src_lebar,
                   tak_bertanda32_t src_tinggi,
                   tak_bertanda32_t src_pitch,
                   tak_bertanda32_t *dst_buffer,
                   tak_bertanda32_t dst_pitch,
                   tak_bertanda32_t sudut)
{
    switch (sudut) {
    case 0:
        return gpu_putar_180(src_buffer, src_lebar,
                              src_tinggi, src_pitch,
                              dst_buffer, dst_pitch);
    case ROTASI_90_DERAJAT:
        return gpu_putar_90(src_buffer, src_lebar,
                             src_tinggi, src_pitch,
                             dst_buffer, dst_pitch);
    case ROTASI_180_DERAJAT:
        return gpu_putar_180(src_buffer, src_lebar,
                              src_tinggi, src_pitch,
                              dst_buffer, dst_pitch);
    case ROTASI_270_DERAJAT:
        return gpu_putar_270(src_buffer, src_lebar,
                              src_tinggi, src_pitch,
                              dst_buffer, dst_pitch);
    default:
        return STATUS_PARAM_INVALID;
    }
}
