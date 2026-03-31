/*
 * PIGURA OS - PENGOLAH.C
 * ======================
 * Inti subsistem pengolah grafis Pigura OS.
 *
 * Berkas ini menyediakan fungsi inisialisasi, konfigurasi,
 * utilitas warna, dan routing ke backend CPU/GPU/Hybrid.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
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
 * FUNGSI INTERNAL - VALIDASI
 * ===========================================================================
 */

/*
 * pengolah_validasi
 * ----------------
 * Memvalidasi konteks pengolah.
 * Memeriksa magic, pointer buffer, dan dimensi.
 *
 * Return:
 *   STATUS_BERHASIL jika valid
 *   Kode error jika tidak valid
 */
static status_t pengolah_validasi(const struct p_konteks *ktx)
{
    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (ktx->buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->lebar == 0 || ktx->tinggi == 0) {
        return STATUS_PARAM_JARAK;
    }
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - INISIALISASI
 * ===========================================================================
 */

/*
 * pengolah_init
 * -------------
 * Menginisialisasi konteks pengolah dengan buffer target.
 * Mengatur mode default ke CPU, warna putih, tanpa klip.
 *
 * Parameter:
 *   ktx    - Pointer ke konteks pengolah
 *   buffer - Pointer ke buffer piksel 32-bit
 *   lebar  - Lebar buffer dalam piksel
 *   tinggi - Tinggi buffer dalam piksel
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika parameter tidak valid
 */
status_t pengolah_init(struct p_konteks *ktx,
                       tak_bertanda32_t *buffer,
                       tak_bertanda32_t lebar,
                       tak_bertanda32_t tinggi)
{
    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (lebar == 0 || tinggi == 0) {
        return STATUS_PARAM_JARAK;
    }

    kernel_memset(ktx, 0, sizeof(struct p_konteks));

    ktx->magic = PENGOLAH_MAGIC;
    ktx->versi = PENGOLAH_VERSI;
    ktx->mode = PENGOLAH_MODE_HYBRID;

    ktx->buffer = buffer;
    ktx->lebar = lebar;
    ktx->tinggi = tinggi;
    ktx->pitch = lebar * 4;  /* 32-bit per piksel */

    /* Warna default: putih penuh */
    ktx->konfig.warna.r = 255;
    ktx->konfig.warna.g = 255;
    ktx->konfig.warna.b = 255;
    ktx->konfig.warna.a = 255;

    /* Warna background default: hitam */
    ktx->konfig.warna_bg.r = 0;
    ktx->konfig.warna_bg.g = 0;
    ktx->konfig.warna_bg.b = 0;
    ktx->konfig.warna_bg.a = 255;

    ktx->konfig.ketebalan = 1;
    ktx->konfig.klip_aktif = SALAH;
    ktx->konfig.klip.x = 0;
    ktx->konfig.klip.y = 0;
    ktx->konfig.klip.lebar = lebar;
    ktx->konfig.klip.tinggi = tinggi;

    ktx->aktif = BENAR;
    ktx->kode_error = 0;

    return STATUS_BERHASIL;
}

/*
 * pengolah_shutdown
 * ----------------
 * Mematikan konteks pengolah.
 * Membersihkan state dan membatalkan referensi buffer.
 *
 * Parameter:
 *   ktx - Pointer ke konteks pengolah
 */
void pengolah_shutdown(struct p_konteks *ktx)
{
    if (ktx == NULL) {
        return;
    }

    kernel_memset(&ktx->statistik, 0, sizeof(struct p_statistik));
    kernel_memset(&ktx->konfig, 0, sizeof(struct p_konfig));

    ktx->buffer = NULL;
    ktx->lebar = 0;
    ktx->tinggi = 0;
    ktx->pitch = 0;
    ktx->aktif = SALAH;
    ktx->mode = 0;
    ktx->kode_error = 0;
    ktx->magic = 0;
}

/*
 * pengolah_siap
 * ------------
 * Memeriksa apakah konteks pengolah siap digunakan.
 *
 * Parameter:
 *   ktx - Pointer ke konteks pengolah
 *
 * Return:
 *   BENAR jika siap
 *   SALAH jika belum diinisialisasi
 */
bool_t pengolah_siap(const struct p_konteks *ktx)
{
    if (ktx == NULL || ktx->magic != PENGOLAH_MAGIC) {
        return SALAH;
    }
    return ktx->aktif;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - KONFIGURASI MODE
 * ===========================================================================
 */

/*
 * pengolah_set_mode
 * ----------------
 * Mengatur mode backend pengolah.
 *
 * Parameter:
 *   ktx  - Pointer ke konteks pengolah
 *   mode - PENGOLAH_MODE_CPU, GPU, atau HYBRID
 *
 * Return:
 *   STATUS_BERHASIL atau kode error
 */
status_t pengolah_set_mode(struct p_konteks *ktx,
                           tak_bertanda32_t mode)
{
    status_t st;

    st = pengolah_validasi(ktx);
    if (st != STATUS_BERHASIL) {
        return st;
    }

    if (mode != PENGOLAH_MODE_CPU &&
        mode != PENGOLAH_MODE_GPU &&
        mode != PENGOLAH_MODE_HYBRID) {
        return STATUS_PARAM_INVALID;
    }

    ktx->mode = mode;
    return STATUS_BERHASIL;
}

/*
 * pengolah_dapat_mode
 * ------------------
 * Mendapatkan mode backend aktif.
 *
 * Return:
 *   Mode saat ini, atau 0 jika konteks tidak valid
 */
tak_bertanda32_t pengolah_dapat_mode(const struct p_konteks *ktx)
{
    if (ktx == NULL || ktx->magic != PENGOLAH_MAGIC) {
        return 0;
    }
    return ktx->mode;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - KONFIGURASI WARNA
 * ===========================================================================
 */

/*
 * pengolah_set_warna
 * -----------------
 * Mengatur warna gambar aktif.
 *
 * Parameter:
 *   ktx - Pointer ke konteks pengolah
 *   r, g, b, a - Komponen warna (0-255)
 *
 * Return:
 *   STATUS_BERHASIL atau kode error
 */
status_t pengolah_set_warna(struct p_konteks *ktx,
                            tak_bertanda8_t r,
                            tak_bertanda8_t g,
                            tak_bertanda8_t b,
                            tak_bertanda8_t a)
{
    status_t st;

    st = pengolah_validasi(ktx);
    if (st != STATUS_BERHASIL) {
        return st;
    }

    ktx->konfig.warna.r = r;
    ktx->konfig.warna.g = g;
    ktx->konfig.warna.b = b;
    ktx->konfig.warna.a = a;

    return STATUS_BERHASIL;
}

/*
 * pengolah_set_ketebalan
 * ----------------------
 * Mengatur ketebalan garis untuk operasi garis dan busur.
 *
 * Parameter:
 *   ktx       - Pointer ke konteks pengolah
 *   ketebalan - Ketebalan dalam piksel (1-255)
 *
 * Return:
 *   STATUS_BERHASIL atau kode error
 */
status_t pengolah_set_ketebalan(struct p_konteks *ktx,
                                tak_bertanda8_t ketebalan)
{
    status_t st;

    st = pengolah_validasi(ktx);
    if (st != STATUS_BERHASIL) {
        return st;
    }

    if (ketebalan == 0) {
        return STATUS_PARAM_INVALID;
    }

    ktx->konfig.ketebalan = ketebalan;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - KLIP
 * ===========================================================================
 */

/*
 * pengolah_set_klip
 * ----------------
 * Mengatur area klip. Semua operasi menggambar akan
 * dibatasi ke area ini.
 *
 * Parameter:
 *   ktx    - Pointer ke konteks pengolah
 *   x, y   - Posisi sudut kiri atas klip
 *   lebar  - Lebar area klip
 *   tinggi - Tinggi area klip
 *
 * Return:
 *   STATUS_BERHASIL atau kode error
 */
status_t pengolah_set_klip(struct p_konteks *ktx,
                           tanda32_t x, tanda32_t y,
                           tak_bertanda32_t lebar,
                           tak_bertanda32_t tinggi)
{
    tanda32_t x_akhir, y_akhir;
    status_t st;

    st = pengolah_validasi(ktx);
    if (st != STATUS_BERHASIL) {
        return st;
    }

    if (lebar == 0 || tinggi == 0) {
        return STATUS_PARAM_JARAK;
    }

    /* Batasi klip ke batas buffer */
    x_akhir = (tanda32_t)MIN(
        (tak_bertanda32_t)(x + (tanda32_t)lebar),
        ktx->lebar);
    y_akhir = (tanda32_t)MIN(
        (tak_bertanda32_t)(y + (tanda32_t)tinggi),
        ktx->tinggi);

    if (x < 0) { x = 0; }
    if (y < 0) { y = 0; }

    ktx->konfig.klip_aktif = BENAR;
    ktx->konfig.klip.x = x;
    ktx->konfig.klip.y = y;
    ktx->konfig.klip.lebar =
        (tak_bertanda32_t)(x_akhir - x);
    ktx->konfig.klip.tinggi =
        (tak_bertanda32_t)(y_akhir - y);

    return STATUS_BERHASIL;
}

/*
 * pengolah_hapus_klip
 * ------------------
 * Menonaktifkan area klip.
 * Seluruh buffer menjadi area gambar aktif.
 *
 * Parameter:
 *   ktx - Pointer ke konteks pengolah
 */
void pengolah_hapus_klip(struct p_konteks *ktx)
{
    if (ktx == NULL || ktx->magic != PENGOLAH_MAGIC) {
        return;
    }

    ktx->konfig.klip_aktif = SALAH;
    ktx->konfig.klip.x = 0;
    ktx->konfig.klip.y = 0;
    ktx->konfig.klip.lebar = ktx->lebar;
    ktx->konfig.klip.tinggi = ktx->tinggi;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - STATISTIK
 * ===========================================================================
 */

/*
 * pengolah_dapat_statistik
 * ------------------------
 * Mengambil salinan statistik operasi.
 *
 * Parameter:
 *   ktx  - Pointer ke konteks pengolah
 *   stat - Pointer ke buffer output statistik
 *
 * Return:
 *   STATUS_BERHASIL atau kode error
 */
status_t pengolah_dapat_statistik(const struct p_konteks *ktx,
                                  struct p_statistik *stat)
{
    status_t st;

    st = pengolah_validasi(ktx);
    if (st != STATUS_BERHASIL) {
        return st;
    }
    if (stat == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memcpy(stat, &ktx->statistik, sizeof(struct p_statistik));
    return STATUS_BERHASIL;
}

/*
 * pengolah_reset_statistik
 * ------------------------
 * Mereset semua counter statistik ke nol.
 *
 * Parameter:
 *   ktx - Pointer ke konteks pengolah
 */
void pengolah_reset_statistik(struct p_konteks *ktx)
{
    if (ktx == NULL || ktx->magic != PENGOLAH_MAGIC) {
        return;
    }

    kernel_memset(&ktx->statistik, 0, sizeof(struct p_statistik));
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - KONVERSI WARNA
 * ===========================================================================
 */

/*
 * pengolah_rgba_ke_piksel
 * -----------------------
 * Mengkonversi komponen RGBA menjadi nilai piksel 32-bit.
 * Format: ARGB8888 (alpha di byte tertinggi).
 *
 * Return:
 *   Nilai piksel 32-bit
 */
tak_bertanda32_t pengolah_rgba_ke_piksel(tak_bertanda8_t r,
                                         tak_bertanda8_t g,
                                         tak_bertanda8_t b,
                                         tak_bertanda8_t a)
{
    return ((tak_bertanda32_t)a << 24) |
           ((tak_bertanda32_t)r << 16) |
           ((tak_bertanda32_t)g << 8)  |
           (tak_bertanda32_t)b;
}

/*
 * pengolah_piksel_ke_rgba
 * -----------------------
 * Mengekstrak komponen RGBA dari nilai piksel 32-bit.
 * Format: ARGB8888.
 */
void pengolah_piksel_ke_rgba(tak_bertanda32_t piksel,
                             tak_bertanda8_t *r,
                             tak_bertanda8_t *g,
                             tak_bertanda8_t *b,
                             tak_bertanda8_t *a)
{
    if (r != NULL) {
        *r = (tak_bertanda8_t)((piksel >> 16) & 0xFF);
    }
    if (g != NULL) {
        *g = (tak_bertanda8_t)((piksel >> 8) & 0xFF);
    }
    if (b != NULL) {
        *b = (tak_bertanda8_t)(piksel & 0xFF);
    }
    if (a != NULL) {
        *a = (tak_bertanda8_t)((piksel >> 24) & 0xFF);
    }
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - UTILITAS
 * ===========================================================================
 */

/*
 * p_dalam_buffer
 * --------------
 * Memeriksa apakah koordinat berada di dalam buffer.
 *
 * Return:
 *   BENAR jika di dalam batas buffer
 */
bool_t p_dalam_buffer(const struct p_konteks *ktx,
                      tanda32_t x, tanda32_t y)
{
    if (ktx == NULL) {
        return SALAH;
    }
    if (x < 0 || y < 0) {
        return SALAH;
    }
    if ((tak_bertanda32_t)x >= ktx->lebar) {
        return SALAH;
    }
    if ((tak_bertanda32_t)y >= ktx->tinggi) {
        return SALAH;
    }
    return BENAR;
}

/*
 * p_klip_titik
 * ------------
 * Memeriksa apakah titik berada di dalam area klip.
 * Jika klip tidak aktif, selalu mengembalikan BENAR
 * selama titik berada di dalam buffer.
 *
 * Return:
 *   BENAR jika titik boleh digambar
 */
bool_t p_klip_titik(const struct p_konteks *ktx,
                    tanda32_t x, tanda32_t y)
{
    if (ktx == NULL) {
        return SALAH;
    }

    if (x < 0 || y < 0) {
        return SALAH;
    }
    if ((tak_bertanda32_t)x >= ktx->lebar ||
        (tak_bertanda32_t)y >= ktx->tinggi) {
        return SALAH;
    }

    if (!ktx->konfig.klip_aktif) {
        return BENAR;
    }

    if ((tak_bertanda32_t)x < (tak_bertanda32_t)ktx->konfig.klip.x) {
        return SALAH;
    }
    if ((tak_bertanda32_t)y < (tak_bertanda32_t)ktx->konfig.klip.y) {
        return SALAH;
    }
    if ((tak_bertanda32_t)x >=
        ktx->konfig.klip.x + ktx->konfig.klip.lebar) {
        return SALAH;
    }
    if ((tak_bertanda32_t)y >=
        ktx->konfig.klip.y + ktx->konfig.klip.tinggi) {
        return SALAH;
    }

    return BENAR;
}

/*
 * p_tulis_piksel
 * --------------
 * Menulis satu piksel ke buffer setelah melewati
 * pemeriksaan klip.
 *
 * Parameter:
 *   ktx   - Pointer ke konteks pengolah (const)
 *   x, y  - Koordinat piksel
 *   warna - Nilai piksel 32-bit
 */
void p_tulis_piksel(const struct p_konteks *ktx,
                    tanda32_t x, tanda32_t y,
                    tak_bertanda32_t warna)
{
    tak_bertanda32_t offset;

    if (ktx == NULL || ktx->buffer == NULL) {
        return;
    }

    if (!p_klip_titik(ktx, x, y)) {
        return;
    }

    offset = (tak_bertanda32_t)y * (ktx->pitch / 4) +
             (tak_bertanda32_t)x;
    ktx->buffer[offset] = warna;
}

/*
 * p_baca_piksel
 * -------------
 * Membaca satu piksel dari buffer dengan
 * pemeriksaan batas aman.
 *
 * Return:
 *   Nilai piksel, atau 0 jika di luar batas
 */
tak_bertanda32_t p_baca_piksel(const struct p_konteks *ktx,
                               tanda32_t x, tanda32_t y)
{
    tak_bertanda32_t offset;

    if (ktx == NULL || ktx->buffer == NULL) {
        return 0;
    }
    if (!p_dalam_buffer(ktx, x, y)) {
        return 0;
    }

    offset = (tak_bertanda32_t)y * (ktx->pitch / 4) +
             (tak_bertanda32_t)x;
    return ktx->buffer[offset];
}
