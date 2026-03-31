/*
 * PIGURA OS - GPU_FILL.C
 * =====================
 * Operasi pengisian (fill) area piksel menggunakan akselerasi GPU.
 *
 * Berkas ini mengimplementasikan:
 *   - Pengisian seluruh permukaan dengan satu warna solid
 *   - Pengisian area persegi panjang (rectangle fill)
 *   - Pengisian dengan pola (pattern fill)
 *   - Pengisian gradien horizontal dan vertikal
 *
 * Semua operasi menggunakan C89 murni tanpa floating point
 * untuk kompatibilitas arsitektur tertanpa FPU.
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
 * Menggunakan extern untuk menghindari duplikasi kode.
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

extern tak_bertanda32_t gpu_gabung_warna(tak_bertanda8_t r,
    tak_bertanda8_t g, tak_bertanda8_t b, tak_bertanda8_t a);

extern tak_bertanda8_t gpu_kali_persen(tak_bertanda8_t nilai,
                                         tak_bertanda8_t persen);

/*
 * ===========================================================================
 * FUNGSI INTERNAL - ISIAN BARIS
 * ===========================================================================
 */

/*
 * gpu_isi_baris32
 * ---------------
 * Mengisi satu baris piksel (32-bit per piksel) dengan warna.
 * Optimalisasi: menggunakan kernel_memset32 jika tersedia.
 *
 * Parameter:
 *   baris   - Pointer ke awal baris
 *   jumlah  - Jumlah piksel yang akan diisi
 *   warna   - Nilai warna 32-bit
 */
static void gpu_isi_baris32(tak_bertanda32_t *baris,
                             tak_bertanda32_t jumlah,
                             tak_bertanda32_t warna)
{
    tak_bertanda32_t i;
    for (i = 0; i < jumlah; i++) {
        baris[i] = warna;
    }
}

/*
 * gpu_isi_baris32_kliped
 * -------------------------
 * Mengisi baris piksel dengan clipping pada batas kiri dan kanan.
 *
 * Parameter:
 *   baris       - Pointer ke awal baris pada buffer tujuan
 *   lebar_total  - Lebar total baris (pitch dalam piksel)
 *   x_awal       - Posisi awal pengisian
 *   x_akhir      - Posisi akhir pengisian (eksklusif)
 *   warna        - Nilai warna 32-bit
 */
static void gpu_isi_baris32_kliped(tak_bertanda32_t *baris,
                                   tak_bertanda32_t lebar_total,
                                   tak_bertanda32_t x_awal,
                                   tak_bertanda32_t x_akhir,
                                   tak_bertanda32_t warna)
{
    tak_bertanda32_t i;

    (void)lebar_total;

    for (i = x_awal; i < x_akhir; i++) {
        baris[i] = warna;
    }
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - GRADIEN
 * ===========================================================================
 */

/*
 * gpu_hitung_warna_gradien_h
 * ---------------------------
 * Menghitung warna untuk gradien horizontal pada posisi x.
 * Interpolasi linear dari warna_a (kiri) ke warna_b (kanan).
 *
 * Parameter:
 *   warna_a - Warna di tepi kiri
 *   warna_b - Warna di tepi kanan
 *   x       - Posisi saat ini (0 sampai lebar-1)
 *   lebar   - Total lebar area
 *
 * Return:
 *   Warna hasil interpolasi
 */
static tak_bertanda32_t gpu_hitung_warna_gradien_h(
    tak_bertanda32_t warna_a,
    tak_bertanda32_t warna_b,
    tak_bertanda32_t x,
    tak_bertanda32_t lebar)
{
    tak_bertanda8_t ra, ga, ba, aa;
    tak_bertanda8_t rb, gb, bb, ab;
    tak_bertanda8_t rr, gr, br, ar;
    tak_bertanda32_t rasio;

    /* Ekstrak komponen warna_a */
    ra = (tak_bertanda8_t)((warna_a >> 16) & 0xFF);
    ga = (tak_bertanda8_t)((warna_a >> 8)  & 0xFF);
    ba = (tak_bertanda8_t)(warna_a         & 0xFF);
    aa = (tak_bertanda8_t)((warna_a >> 24) & 0xFF);

    /* Ekstrak komponen warna_b */
    rb = (tak_bertanda8_t)((warna_b >> 16) & 0xFF);
    gb = (tak_bertanda8_t)((warna_b >> 8)  & 0xFF);
    bb = (tak_bertanda8_t)(warna_b         & 0xFF);
    ab = (tak_bertanda8_t)((warna_b >> 24) & 0xFF);

    /* Hitung rasio interpolasi */
    if (lebar <= 1) {
        rasio = 0;
    } else {
        rasio = ((tak_bertanda32_t)x * 255) / (lebar - 1);
    }

    /* Interpolasi setiap komponen */
    rr = gpu_kali_persen(ra, (tak_bertanda8_t)(255 - rasio))
       + gpu_kali_persen(rb, (tak_bertanda8_t)rasio);
    gr = gpu_kali_persen(ga, (tak_bertanda8_t)(255 - rasio))
       + gpu_kali_persen(gb, (tak_bertanda8_t)rasio);
    br = gpu_kali_persen(ba, (tak_bertanda8_t)(255 - rasio))
       + gpu_kali_persen(bb, (tak_bertanda8_t)rasio);
    ar = gpu_kali_persen(aa, (tak_bertanda8_t)(255 - rasio))
       + gpu_kali_persen(ab, (tak_bertanda8_t)rasio);

    return gpu_gabung_warna(rr, gr, br, ar);
}

/*
 * gpu_hitung_warna_gradien_v
 * ---------------------------
 * Menghitung warna untuk gradien vertikal pada posisi y.
 *
 * Parameter:
 *   warna_a - Warna di tepi atas
 *   warna_b - Warna di tepi bawah
 *   y       - Posisi saat ini (0 sampai tinggi-1)
 *   tinggi  - Total tinggi area
 *
 * Return:
 *   Warna hasil interpolasi
 */
static tak_bertanda32_t gpu_hitung_warna_gradien_v(
    tak_bertanda32_t warna_a,
    tak_bertanda32_t warna_b,
    tak_bertanda32_t y,
    tak_bertanda32_t tinggi)
{
    /* Gradien vertikal menggunakan logika yang sama */
    return gpu_hitung_warna_gradien_h(warna_a, warna_b, y, tinggi);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - ISI SELURUH PERMUKAAN
 * ===========================================================================
 */

/*
 * gpu_isi_permukaan
 * -----------------
 * Mengisi seluruh permukaan tujuan dengan satu warna solid.
 *
 * Parameter:
 *   dst_buffer - Buffer tujuan (wajib valid)
 *   dst_lebar  - Lebar permukaan tujuan
 *   dst_tinggi - Tinggi permukaan tujuan
 *   dst_pitch  - Pitch tujuan dalam piksel (0 = auto)
 *   warna      - Warna pengisi (32-bit ARGB/XRGB)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   STATUS_PARAM_NULL jika buffer NULL
 *   STATUS_PARAM_JARAK jika dimensi tidak valid
 */
status_t gpu_isi_permukaan(tak_bertanda32_t *dst_buffer,
                          tak_bertanda32_t dst_lebar,
                          tak_bertanda32_t dst_tinggi,
                          tak_bertanda32_t dst_pitch,
                          tak_bertanda32_t warna)
{
    struct gpu_konteks ctx;
    status_t status;
    tak_bertanda32_t y;

    /* Siapkan dan validasi konteks */
    status = gpu_konteks_siapkan(&ctx, NULL, 0, 0, 0,
                                   dst_buffer, dst_lebar, dst_tinggi,
                                   dst_pitch, 0, 0,
                                   dst_lebar, dst_tinggi);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    /* Hitung area yang valid (clipping sudah dilakukan konteks) */
    ctx.warna = warna;

    /* Isi setiap baris */
    for (y = 0; y < ctx.op_tinggi; y++) {
        tak_bertanda32_t *baris;
        tak_bertanda32_t offset;

        offset = (ctx.dst_y + y) * ctx.dst_pitch + ctx.dst_x;
        baris = ctx.dst_buffer + offset;

        gpu_isi_baris32(baris, ctx.op_lebar, warna);
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)ctx.op_lebar *
                         (tak_bertanda64_t)ctx.op_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - ISI KOTAK (RECTANGLE FILL)
 * ===========================================================================
 */

/*
 * gpu_isi_kotak
 * -------------
 * Mengisi area persegi panjang pada permukaan tujuan
 * dengan satu warna solid. Mendukung clipping otomatis
 * terhadap batas permukaan.
 *
 * Parameter:
 *   dst_buffer - Buffer tujuan
 *   dst_lebar  - Lebar permukaan tujuan
 *   dst_tinggi - Tinggi permukaan tujuan
 *   dst_pitch  - Pitch tujuan dalam piksel (0 = auto)
 *   x          - Posisi X awal
 *   y          - Posisi Y awal
 *   lebar      - Lebar area yang akan diisi
 *   tinggi     - Tinggi area yang akan diisi
 *   warna      - Warna pengisi (32-bit)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_isi_kotak(tak_bertanda32_t *dst_buffer,
                       tak_bertanda32_t dst_lebar,
                       tak_bertanda32_t dst_tinggi,
                       tak_bertanda32_t dst_pitch,
                       tak_bertanda32_t x,
                       tak_bertanda32_t y,
                       tak_bertanda32_t lebar,
                       tak_bertanda32_t tinggi,
                       tak_bertanda32_t warna)
{
    struct gpu_konteks ctx;
    status_t status;
    tak_bertanda32_t row;
    tak_bertanda32_t x_klip;

    /* Siapkan konteks dengan area operasi */
    status = gpu_konteks_siapkan(&ctx, NULL, 0, 0, 0,
                                   dst_buffer, dst_lebar, dst_tinggi,
                                   dst_pitch, x, y, lebar, tinggi);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    x_klip = ctx.dst_x;

    /* Isi area baris per baris */
    for (row = 0; row < ctx.op_tinggi; row++) {
        tak_bertanda32_t *baris;
        tak_bertanda32_t offset;

        offset = (ctx.dst_y + row) * ctx.dst_pitch + x_klip;
        baris = ctx.dst_buffer + offset;

        gpu_isi_baris32(baris, ctx.op_lebar, warna);
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)ctx.op_lebar *
                         (tak_bertanda64_t)ctx.op_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - ISI DENGAN POLA (PATTERN FILL)
 * ===========================================================================
 */

/*
 * gpu_isi_pola
 * ------------
 * Mengisi area dengan pola yang diulang dari buffer kecil.
 * Pola di-tile (diulang) untuk mengisi seluruh area tujuan.
 *
 * Parameter:
 *   dst_buffer - Buffer tujuan
 *   dst_lebar  - Lebar permukaan tujuan
 *   dst_tinggi - Tinggi permukaan tujuan
 *   dst_pitch  - Pitch tujuan dalam piksel (0 = auto)
 *   pola       - Buffer pola (wajib valid)
 *   pola_lebar - Lebar pola dalam piksel
 *   pola_tinggi- Tinggi pola dalam piksel
 *   x          - Posisi X awal pada tujuan
 *   y          - Posisi Y awal pada tujuan
 *   lebar      - Lebar area yang akan diisi
 *   tinggi     - Tinggi area yang akan diisi
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_isi_pola(tak_bertanda32_t *dst_buffer,
                      tak_bertanda32_t dst_lebar,
                      tak_bertanda32_t dst_tinggi,
                      tak_bertanda32_t dst_pitch,
                      const tak_bertanda32_t *pola,
                      tak_bertanda32_t pola_lebar,
                      tak_bertanda32_t pola_tinggi,
                      tak_bertanda32_t x,
                      tak_bertanda32_t y,
                      tak_bertanda32_t lebar,
                      tak_bertanda32_t tinggi)
{
    struct gpu_konteks ctx;
    status_t status;
    tak_bertanda32_t row, col;
    tak_bertanda32_t src_col, src_row;

    if (pola == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (pola_lebar == 0 || pola_tinggi == 0) {
        return STATUS_PARAM_JARAK;
    }

    /* Validasi konteks */
    status = gpu_konteks_siapkan(&ctx, NULL, 0, 0, 0,
                                   dst_buffer, dst_lebar, dst_tinggi,
                                   dst_pitch, x, y, lebar, tinggi);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    /* Tile pola ke area tujuan */
    for (row = 0; row < ctx.op_tinggi; row++) {
        src_row = row % pola_tinggi;

        for (col = 0; col < ctx.op_lebar; col++) {
            src_col = col % pola_lebar;
            ctx.dst_buffer[(ctx.dst_y + row) * ctx.dst_pitch +
                            (ctx.dst_x + col)] =
                pola[src_row * pola_lebar + src_col];
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)ctx.op_lebar *
                         (tak_bertanda64_t)ctx.op_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - GRADIEN HORIZONTAL
 * ===========================================================================
 */

/*
 * gpu_isi_gradien_h
 * -------------------
 * Mengisi area dengan gradien horizontal (kiri ke kanan).
 * Interpolasi linear antara warna_a (kiri) dan warna_b (kanan).
 *
 * Parameter:
 *   dst_buffer - Buffer tujuan
 *   dst_lebar  - Lebar permukaan tujuan
 *   dst_tinggi - Tinggi permukaan tujuan
 *   dst_pitch  - Pitch tujuan dalam piksel (0 = auto)
 *   x          - Posisi X awal
 *   y          - Posisi Y awal
 *   lebar      - Lebar area gradien
 *   tinggi     - Tinggi area gradien
 *   warna_a    - Warna di tepi kiri
 *   warna_b    - Warna di tepi kanan
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_isi_gradien_h(tak_bertanda32_t *dst_buffer,
                          tak_bertanda32_t dst_lebar,
                          tak_bertanda32_t dst_tinggi,
                          tak_bertanda32_t dst_pitch,
                          tak_bertanda32_t x,
                          tak_bertanda32_t y,
                          tak_bertanda32_t lebar,
                          tak_bertanda32_t tinggi,
                          tak_bertanda32_t warna_a,
                          tak_bertanda32_t warna_b)
{
    struct gpu_konteks ctx;
    status_t status;
    tak_bertanda32_t row, col;

    status = gpu_konteks_siapkan(&ctx, NULL, 0, 0, 0,
                                   dst_buffer, dst_lebar, dst_tinggi,
                                   dst_pitch, x, y, lebar, tinggi);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    /* Isi gradien horizontal per baris */
    for (row = 0; row < ctx.op_tinggi; row++) {
        tak_bertanda32_t offset_dst;

        offset_dst = (ctx.dst_y + row) * ctx.dst_pitch + ctx.dst_x;

        for (col = 0; col < ctx.op_lebar; col++) {
            ctx.dst_buffer[offset_dst + col] =
                gpu_hitung_warna_gradien_h(warna_a, warna_b,
                                             col, ctx.op_lebar);
        }
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)ctx.op_lebar *
                         (tak_bertanda64_t)ctx.op_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - GRADIEN VERTIKAL
 * ===========================================================================
 */

/*
 * gpu_isi_gradien_v
 * -------------------
 * Mengisi area dengan gradien vertikal (atas ke bawah).
 * Interpolasi linear antara warna_a (atas) dan warna_b (bawah).
 *
 * Parameter:
 *   dst_buffer - Buffer tujuan
 *   dst_lebar  - Lebar permukaan tujuan
 *   dst_tinggi - Tinggi permukaan tujuan
 *   dst_pitch  - Pitch tujuan dalam piksel (0 = auto)
 *   x          - Posisi X awal
 *   y          - Posisi Y awal
 *   lebar      - Lebar area gradien
 *   tinggi     - Tinggi area gradien
 *   warna_a    - Warna di tepi atas
 *   warna_b    - Warna di tepi bawah
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_isi_gradien_v(tak_bertanda32_t *dst_buffer,
                          tak_bertanda32_t dst_lebar,
                          tak_bertanda32_t dst_tinggi,
                          tak_bertanda32_t dst_pitch,
                          tak_bertanda32_t x,
                          tak_bertanda32_t y,
                          tak_bertanda32_t lebar,
                          tak_bertanda32_t tinggi,
                          tak_bertanda32_t warna_a,
                          tak_bertanda32_t warna_b)
{
    struct gpu_konteks ctx;
    status_t status;
    tak_bertanda32_t row;
    tak_bertanda32_t warna_baris;

    status = gpu_konteks_siapkan(&ctx, NULL, 0, 0, 0,
                                   dst_buffer, dst_lebar, dst_tinggi,
                                   dst_pitch, x, y, lebar, tinggi);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    /* Hitung warna untuk setiap baris (gradien vertikal) */
    for (row = 0; row < ctx.op_tinggi; row++) {
        tak_bertanda32_t offset_dst;

        warna_baris = gpu_hitung_warna_gradien_v(
            warna_a, warna_b, row, ctx.op_tinggi);

        offset_dst = (ctx.dst_y + row) * ctx.dst_pitch + ctx.dst_x;

        gpu_isi_baris32(ctx.dst_buffer + offset_dst,
                         ctx.op_lebar, warna_baris);
    }

    gpu_statistik_tambah(SALAH,
                         (tak_bertanda64_t)ctx.op_lebar *
                         (tak_bertanda64_t)ctx.op_tinggi);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - ISI PERSEGI BERGARIS (OUTLINE RECTANGLE)
 * ===========================================================================
 */

/*
 * gpu_isi_garis_kotak
 * --------------------
 * Mengisi garis tepi (outline) dari persegi panjang.
 * Berguna untuk menggambar border, bingkai, atau kotak kosong.
 *
 * Parameter:
 *   dst_buffer - Buffer tujuan
 *   dst_lebar  - Lebar permukaan tujuan
 *   dst_tinggi - Tinggi permukaan tujuan
 *   dst_pitch  - Pitch tujuan dalam piksel (0 = auto)
 *   x          - Posisi X kotak
 *   y          - Posisi Y kotak
 *   lebar      - Lebar kotak
 *   tinggi     - Tinggi kotak
 *   warna      - Warna garis (32-bit)
 *   tebal      - Ketebalan garis dalam piksel
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t gpu_isi_garis_kotak(tak_bertanda32_t *dst_buffer,
                             tak_bertanda32_t dst_lebar,
                             tak_bertanda32_t dst_tinggi,
                             tak_bertanda32_t dst_pitch,
                             tak_bertanda32_t x,
                             tak_bertanda32_t y,
                             tak_bertanda32_t lebar,
                             tak_bertanda32_t tinggi,
                             tak_bertanda32_t warna,
                             tak_bertanda32_t tebal)
{
    status_t status;

    if (tebal == 0) {
        return STATUS_PARAM_JARAK;
    }

    /* Garis atas */
    status = gpu_isi_kotak(dst_buffer, dst_lebar, dst_tinggi,
                            dst_pitch, x, y, lebar, tebal, warna);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    /* Garis bawah */
    if (tinggi > tebal) {
        status = gpu_isi_kotak(dst_buffer, dst_lebar, dst_tinggi,
                                dst_pitch, x, y + tinggi - tebal,
                                lebar, tebal, warna);
        if (status != STATUS_BERHASIL) {
            return status;
        }
    }

    /* Garis kiri */
    if (lebar > tebal) {
        status = gpu_isi_kotak(dst_buffer, dst_lebar, dst_tinggi,
                                dst_pitch, x, y + tebal, tebal,
                                tinggi - (2 * tebal), warna);
        if (status != STATUS_BERHASIL) {
            return status;
        }
    }

    /* Garis kanan */
    if (lebar > (2 * tebal)) {
        status = gpu_isi_kotak(dst_buffer, dst_lebar, dst_tinggi,
                                dst_pitch, x + lebar - tebal,
                                y + tebal, tebal,
                                tinggi - (2 * tebal), warna);
    }

    return status;
}
