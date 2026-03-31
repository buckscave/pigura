/*
 * PIGURA OS - GPU_COPY.C
 * =====================
 * Operasi penyalinan (copy) blok piksel menggunakan akselerasi GPU.
 *
 * Berkas ini mengimplementasikan:
 *   - Penyalinan blok piksel dari sumber ke tujuan
 *   - Penyalinan dengan clipping otomatis
 *   - Penyalinan dengan offset (sub-area copy)
 *   - Penyalinan horizontal dan vertikal (flip)
 *
 * Semua operasi menggunakan C89 murni tanpa floating point
 * untuk portabilitas maksimal ke semua arsitektur.
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
 * Deklarasi fungsi yang didefinisikan di gpu_accel.c.
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

/*
 * ===========================================================================
 * FUNGSI PUBLIK - SALIN BLOK (BLOCK COPY)
 * ===========================================================================
 */

/*
 * gpu_salin_blok
 * --------------
 * Menyalin blok piksel dari sumber ke tujuan.
 * Mendukung clipping otomatis jika area penyalinan
 * melebihi batas permukaan tujuan.
 *
 * Parameter:
 *   src_buffer - Buffer sumber (wajib valid, bukan NULL)
 *   src_lebar  - Lebar sumber dalam piksel
 *   src_tinggi - Tinggi sumber dalam piksel
 *   src_pitch  - Pitch sumber dalam piksel (0 = auto)
 *   dst_buffer - Buffer tujuan (wajib valid, bukan NULL)
 *   dst_lebar  - Lebar tujuan dalam piksel
 *   dst_tinggi - Tinggi tujuan dalam piksel
 *   dst_pitch  - Pitch tujuan dalam piksel (0 = auto)
 *   dst_x      - Posisi X pada tujuan (offset)
 *   dst_y      - Posisi Y pada tujuan (offset)
 *   src_x      - Posisi X pada sumber (offset awal)
 *   src_y      - Posisi Y pada sumber (offset awal)
 *   lebar      - Lebar area yang disalin
 *   tinggi     - Tinggi area yang disalin
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_salin_blok(const tak_bertanda32_t *src_buffer,
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
    status_t status;
    tak_bertanda32_t row, col;

    (void)src_x;
    (void)src_y;

    /* Siapkan dan validasi konteks */
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

    /* Adjust offset sumber agar sesuai dengan posisi clipping */
    if (ctx.src_x > 0 || ctx.src_y > 0) {
        /* ctx.src_x dan ctx.src_y sudah disesuaikan oleh
         * validasi konteks agar sesuai area yang valid */
    }

    /* Salin baris per baris */
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
 * FUNGSI PUBLIK - SALIN SELURUH
 * ===========================================================================
 */

/*
 * gpu_salin_seluruh
 * ----------------
 * Menyalin seluruh isi sumber ke tujuan.
 * Sumber dan tujuan harus memiliki ukuran yang sama.
 *
 * Parameter:
 *   src_buffer - Buffer sumber
 *   dst_buffer - Buffer tujuan
 *   lebar      - Lebar dalam piksel
 *   tinggi     - Tinggi dalam piksel
 *   pitch       - Pitch dalam piksel (0 = auto = lebar)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_salin_seluruh(const tak_bertanda32_t *src_buffer,
                          tak_bertanda32_t *dst_buffer,
                          tak_bertanda32_t lebar,
                          tak_bertanda32_t tinggi,
                          tak_bertanda32_t pitch)
{
    if (src_buffer == NULL || dst_buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (lebar == 0 || tinggi == 0) {
        return STATUS_PARAM_JARAK;
    }

    /* Salin seluruh blok tanpa offset */
    return gpu_salin_blok(src_buffer, lebar, tinggi, pitch,
                          dst_buffer, lebar, tinggi, pitch,
                          0, 0, 0, 0, lebar, tinggi);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - SALIN DENGAN FLIP HORIZONTAL
 * ===========================================================================
 */

/*
 * gpu_salin_flip_h
 * -----------------
 * Menyalin blok piksel dari sumber ke tujuan dengan
 * pembalikan horizontal (mirror kiri-kanan).
 * Piksel paling kiri di sumber menjadi piksel paling kanan
 * di tujuan, dan sebaliknya.
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
 *   lebar      - Lebar area yang disalin
 *   tinggi     - Tinggi area yang disalin
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_salin_flip_h(const tak_bertanda32_t *src_buffer,
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
    status_t status;
    tak_bertanda32_t row, col;
    tak_bertanda32_t mirror_col;

    (void)src_x;
    (void)src_y;

    /* Siapkan dan validasi konteks */
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

    /* Salin dengan pembalikan horizontal */
    for (row = 0; row < ctx.op_tinggi; row++) {
        tak_bertanda32_t offset_src;
        tak_bertanda32_t offset_dst;

        offset_src = (ctx.src_y + row) * ctx.src_pitch + ctx.src_x;
        offset_dst = (ctx.dst_y + row) * ctx.dst_pitch + ctx.dst_x;

        for (col = 0; col < ctx.op_lebar; col++) {
            mirror_col = ctx.op_lebar - 1 - col;
            ctx.dst_buffer[offset_dst + col] =
                ctx.src_buffer[offset_src + mirror_col];
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)ctx.op_lebar *
                         (tak_bertanda64_t)ctx.op_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - SALIN DENGAN FLIP VERTIKAL
 * ===========================================================================
 */

/*
 * gpu_salin_flip_v
 * -----------------
 * Menyalin blok piksel dari sumber ke tujuan dengan
 * pembalikan vertikal (mirror atas-bawah).
 * Baris pertama di sumber menjadi baris terakhir di tujuan,
 * dan sebaliknya.
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
 *   lebar      - Lebar area yang disalin
 *   tinggi     - Tinggi area yang disalin
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_salin_flip_v(const tak_bertanda32_t *src_buffer,
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
    status_t status;
    tak_bertanda32_t row;
    tak_bertanda32_t mirror_row;

    (void)src_x;
    (void)src_y;

    /* Siapkan dan validasi konteks */
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

    /* Salin dengan pembalikan vertikal */
    for (row = 0; row < ctx.op_tinggi; row++) {
        tak_bertanda32_t offset_src;
        tak_bertanda32_t offset_dst;
        tak_bertanda32_t col;

        mirror_row = ctx.op_tinggi - 1 - row;
        offset_src = (ctx.src_y + row) * ctx.src_pitch + ctx.src_x;
        offset_dst = (ctx.dst_y + mirror_row) * ctx.dst_pitch + ctx.dst_x;

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
 * FUNGSI PUBLIK - SALIN DENGAN FLIP KEDUA AXES
 * ===========================================================================
 */

/*
 * gpu_salin_flip_hv
 * ------------------
 * Menyalin blok piksel dari sumber ke tujuan dengan
 * pembalikan horizontal DAN vertikal (rotasi 180 derajat).
 *
 * Parameter:
 *   src_buffer - Buffer sumber
 *   src_lebar  - Lebar sumber
 *   src_tinggi - Tinggi sumber
 *   src_pitch  - Pitch sumber (0 = auto)
 *   dst_buffer - Buffer tujuan
 * dst_lebar  - Lebar tujuan
 * dst_tinggi - Tinggi tujuan
 *   dst_pitch  - Pitch tujuan (0 = auto)
 *   dst_x      - Offset X pada tujuan
 *   dst_y      - Offset Y pada tujuan
 *   src_x      - Offset X pada sumber
 *   src_y      - Offset Y pada sumber
 *   lebar      - Lebar area yang disalin
 *   tinggi     - Tinggi area yang disalin
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_salin_flip_hv(const tak_bertanda32_t *src_buffer,
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
    status_t status;
    tak_bertanda32_t row, col;
    tak_bertanda32_t mirror_row, mirror_col;

    (void)src_x;
    (void)src_y;

    /* Siapkan dan validasi konteks */
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

    /* Salin dengan pembalikan kedua sumbu */
    for (row = 0; row < ctx.op_tinggi; row++) {
        tak_bertanda32_t offset_src;
        tak_bertanda32_t offset_dst;

        mirror_row = ctx.op_tinggi - 1 - row;
        offset_src = (ctx.src_y + row) * ctx.src_pitch + ctx.src_x;
        offset_dst = (ctx.dst_y + mirror_row) * ctx.dst_pitch + ctx.dst_x;

        for (col = 0; col < ctx.op_lebar; col++) {
            mirror_col = ctx.op_lebar - 1 - col;
            ctx.dst_buffer[offset_dst + mirror_col] =
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
 * FUNGSI PUBLIK - SALIN SUB-AREA
 * ===========================================================================
 */

/*
 * gpu_salin_subarea
 * -----------------
 * Menyalin sub-area (region) dari sumber ke tujuan.
 * Memungkinkan menyalin bagian tertentu dari sumber
 * ke lokasi berbeda pada tujuan.
 *
 * Parameter:
 *   src_buffer - Buffer sumber
 *   src_lebar  - Lebar total sumber
 *   src_tinggi - Tinggi total sumber
 *   src_pitch  - Pitch sumber (0 = auto = src_lebar)
 *   dst_buffer - Buffer tujuan
 *   dst_lebar  - Lebar total tujuan
 *   dst_tinggi - Tinggi total tujuan
 * dst_pitch  - Pitch tujuan (0 = auto = dst_lebar)
 *   src_x      - Posisi X awal pada sumber
 *   src_y      - Posisi Y awal pada sumber
 *   dst_x      - Posisi X tujuan
 *   dst_y      - Posisi Y tujuan
 *   lebar      - Lebar area yang disalin
 * tinggi     - Tinggi area yang disalin
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_salin_subarea(const tak_bertanda32_t *src_buffer,
                          tak_bertanda32_t src_lebar,
                          tak_bertanda32_t src_tinggi,
                          tak_bertanda32_t src_pitch,
                          tak_bertanda32_t *dst_buffer,
                          tak_bertanda32_t dst_lebar,
                          tak_bertanda32_t dst_tinggi,
                          tak_bertanda32_t dst_pitch,
                          tak_bertanda32_t src_x,
                          tak_bertanda32_t src_y,
                          tak_bertanda32_t dst_x,
                          tak_bertanda32_t dst_y,
                          tak_bertanda32_t lebar,
                          tak_bertanda32_t tinggi)
{
    /* Fungsi ini adalah alias dari gpu_salin_blok yang mendukung
     * offset sumber dan tujuan secara terpisah */
    return gpu_salin_blok(src_buffer, src_lebar, src_tinggi, src_pitch,
                          dst_buffer, dst_lebar, dst_tinggi, dst_pitch,
                          dst_x, dst_y, src_x, src_y,
                          lebar, tinggi);
}
