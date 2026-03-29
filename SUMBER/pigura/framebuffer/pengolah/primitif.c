/*
 * PIGURA OS - PRIMITIF.C
 * =======================
 * Operasi primitif gabungan untuk pengolah grafis.
 *
 * Berkas ini menyediakan fungsi utilitas tingkat tinggi
 * yang menggabungkan operasi dasar:
 *   - Bersihkan: isi seluruh buffer dengan warna
 *   - Silang: gambar tanda silang di area
 *   - Bingkai: gambar bingkai tebal di sekeliling area
 *   - Grid: gambar kisi sel di area
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Seluruh nama dalam Bahasa Indonesia
 * - Mendukung: x86, x86_64, ARM, ARMv7, ARM64
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../../inti/kernel.h"
#include "pengolah.h"

/*
 * ===========================================================================
 * DEKLARASI FUNGSI BACKEND
 * ===========================================================================
 */

extern status_t cpu_garis(struct p_konteks *ktx,
                          tanda32_t x0, tanda32_t y0,
                          tanda32_t x1, tanda32_t y1,
                          tak_bertanda32_t warna);
extern status_t cpu_kotak_isi(struct p_konteks *ktx,
                              tanda32_t x, tanda32_t y,
                              tak_bertanda32_t lebar,
                              tak_bertanda32_t tinggi,
                              tak_bertanda32_t warna);

/*
 * ===========================================================================
 * FUNGSI PUBLIK - BERSIHKAN
 * ===========================================================================
 */

/*
 * pengolah_bersihkan
 * -----------------
 * Mengisi seluruh buffer dengan warna tertentu.
 * Operasi ini menggunakan memset32 untuk efisiensi.
 *
 * Parameter:
 *   ktx   - Pointer ke konteks pengolah
 *   warna - Nilai piksel 32-bit untuk pengisian
 *
 * Return:
 *   STATUS_BERHASIL atau kode error
 */
status_t pengolah_bersihkan(struct p_konteks *ktx,
                            tak_bertanda32_t warna)
{
    tak_bertanda32_t total_piksel;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (ktx->buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    total_piksel = ktx->lebar * ktx->tinggi;
    kernel_memset32(ktx->buffer, warna, total_piksel);

    ktx->statistik.jumlah_operasi++;
    ktx->statistik.jumlah_cpu++;
    ktx->statistik.piksel_diproses += total_piksel;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - SILANG
 * ===========================================================================
 */

/*
 * pengolah_silang
 * ---------------
 * Menggambar tanda silang (X) pada area yang ditentukan.
 * Digunakan untuk menandai area atau sebagai ikon hapus.
 *
 * Parameter:
 *   ktx    - Pointer ke konteks pengolah
 *   x, y   - Posisi sudut kiri atas
 *   lebar  - Lebar area
 *   tinggi - Tinggi area
 *
 * Return:
 *   STATUS_BERHASIL atau kode error
 */
status_t pengolah_silang(struct p_konteks *ktx,
                         tanda32_t x, tanda32_t y,
                         tak_bertanda32_t lebar,
                         tak_bertanda32_t tinggi)
{
    tak_bertanda32_t warna;
    status_t st;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (lebar < 2 || tinggi < 2) {
        return STATUS_PARAM_JARAK;
    }

    warna = pengolah_rgba_ke_piksel(
        ktx->konfig.warna.r, ktx->konfig.warna.g,
        ktx->konfig.warna.b, ktx->konfig.warna.a);

    /* Garis diagonal kiri-atas ke kanan-bawah */
    st = cpu_garis(ktx, x, y,
                   x + (tanda32_t)lebar - 1,
                   y + (tanda32_t)tinggi - 1, warna);
    if (st != STATUS_BERHASIL) {
        return st;
    }

    /* Garis diagonal kanan-atas ke kiri-bawah */
    st = cpu_garis(ktx, x + (tanda32_t)lebar - 1, y,
                   x, y + (tanda32_t)tinggi - 1, warna);

    ktx->statistik.jumlah_operasi++;
    return st;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - BINGKAI
 * ===========================================================================
 */

/*
 * pengolah_bingkai
 * ----------------
 * Menggambar bingkai (frame) tebal di sekeliling area.
 * Ketebalan bingkai ditentukan oleh parameter tebal.
 * Area dalam bingkai tidak terisi.
 *
 * Parameter:
 *   ktx    - Pointer ke konteks pengolah
 *   x, y   - Posisi sudut kiri atas area luar
 *   lebar  - Lebar area luar
 *   tinggi - Tinggi area luar
 *   tebal  - Ketebalan bingkai dalam piksel
 *
 * Return:
 *   STATUS_BERHASIL atau kode error
 */
status_t pengolah_bingkai(struct p_konteks *ktx,
                          tanda32_t x, tanda32_t y,
                          tak_bertanda32_t lebar,
                          tak_bertanda32_t tinggi,
                          tak_bertanda8_t tebal)
{
    tak_bertanda32_t warna;
    tak_bertanda32_t i;
    status_t st;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (lebar < tebal * 2 || tinggi < tebal * 2) {
        return STATUS_PARAM_JARAK;
    }

    warna = pengolah_rgba_ke_piksel(
        ktx->konfig.warna.r, ktx->konfig.warna.g,
        ktx->konfig.warna.b, ktx->konfig.warna.a);

    /* Gambar kotak isi berlapis dari luar ke dalam */
    for (i = 0; i < tebal; i++) {
        st = cpu_kotak_isi(ktx,
                           x + (tanda32_t)i,
                           y + (tanda32_t)i,
                           lebar - 2 * i,
                           tinggi - 2 * i,
                           warna);
        if (st != STATUS_BERHASIL) {
            return st;
        }
    }

    /* Isi area dalam bingkai dengan warna background */
    if (tebal < lebar / 2 && tebal < tinggi / 2) {
        tak_bertanda32_t warna_bg;

        warna_bg = pengolah_rgba_ke_piksel(
            ktx->konfig.warna_bg.r,
            ktx->konfig.warna_bg.g,
            ktx->konfig.warna_bg.b,
            ktx->konfig.warna_bg.a);

        st = cpu_kotak_isi(ktx,
                           x + (tanda32_t)tebal,
                           y + (tanda32_t)tebal,
                           lebar - 2 * tebal,
                           tinggi - 2 * tebal,
                           warna_bg);
    }

    ktx->statistik.jumlah_operasi++;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - GRID
 * ===========================================================================
 */

/*
 * pengolah_grid
 * -------------
 * Menggambar kisi (grid) pada area yang ditentukan.
 * Jumlah sel ditentukan oleh sel_jml_x dan sel_jml_y.
 *
 * Parameter:
 *   ktx        - Pointer ke konteks pengolah
 *   x, y       - Posisi sudut kiri atas
 *   lebar      - Lebar area grid
 *   tinggi     - Tinggi area grid
 *   sel_jml_x  - Jumlah kolom sel
 *   sel_jml_y  - Jumlah baris sel
 *
 * Return:
 *   STATUS_BERHASIL atau kode error
 */
status_t pengolah_grid(struct p_konteks *ktx,
                       tanda32_t x, tanda32_t y,
                       tak_bertanda32_t lebar,
                       tak_bertanda32_t tinggi,
                       tak_bertanda32_t sel_jml_x,
                       tak_bertanda32_t sel_jml_y)
{
    tak_bertanda32_t warna;
    tak_bertanda32_t i;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (sel_jml_x == 0 || sel_jml_y == 0) {
        return STATUS_PARAM_JARAK;
    }

    warna = pengolah_rgba_ke_piksel(
        ktx->konfig.warna.r, ktx->konfig.warna.g,
        ktx->konfig.warna.b, ktx->konfig.warna.a);

    /* Garis vertikal */
    for (i = 0; i <= sel_jml_x; i++) {
        tanda32_t gx = x + (tanda32_t)(
            (tak_bertanda64_t)i * (tak_bertanda64_t)lebar /
            (tak_bertanda64_t)sel_jml_x);

        cpu_garis(ktx, gx, y, gx,
                  y + (tanda32_t)tinggi, warna);
    }

    /* Garis horizontal */
    for (i = 0; i <= sel_jml_y; i++) {
        tanda32_t gy = y + (tanda32_t)(
            (tak_bertanda64_t)i * (tak_bertanda64_t)tinggi /
            (tak_bertanda64_t)sel_jml_y);

        cpu_garis(ktx, x, gy,
                  x + (tanda32_t)lebar, gy, warna);
    }

    ktx->statistik.jumlah_operasi++;
    ktx->statistik.piksel_diproses +=
        (tak_bertanda64_t)(sel_jml_x + sel_jml_y + 2) *
        (tak_bertanda64_t)(lebar + tinggi);

    return STATUS_BERHASIL;
}
