/*
 * PIGURA OS - PENUNJUK.C
 * =======================
 * Manajemen penunjuk (pointer/cursor) subsistem peristiwa.
 *
 * Modul ini mengelola state kursor mouse pada layar:
 *   - Posisi kursor (koordinat X, Y absolut)
 *   - Kecepatan gerakan kursor
 *   - Visibilitas kursor (tampil/sembunyi)
 *   - Pembatasan area kursor (clipping ke jendela)
 *   - Pengambilan dan pelepasan kursor (grab/release)
 *   - State tombol kursor
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

#include "peristiwa.h"

/*
 * ===========================================================================
 * KONSTANTA INTERNAL
 * ===========================================================================
 */

/* Kecepatan kursor default (1:1 mapping) */
#define PENUNJUK_KECEPATAN_DEFAULT   1

/* Kecepatan kursor lambat */
#define PENUNJUK_KECEPATAN_LAMBAT    1

/* Kecepatan kursor cepat */
#define PENUNJUK_KECEPATAN_CEPAT     2

/* Nilai threshold akselerasi */
#define PENUNJUK_AKS_THRESHOLD       6

/* Faktor akselerasi kursor */
#define PENUNJUK_AKS_FAKTOR          2

/* Batas area clipping minimum */
#define PENUNJUK_KLIP_MIN            0

/* Jumlah maksimum penunjuk yang di-grab secara bersamaan */
#define PENUNJUK_GRAB_MAKS           4

/*
 * ===========================================================================
 * VARIABEL STATIK - STATE PENUNJUK
 * ===========================================================================
 */

/* Flag status modul penunjuk */
static bool_t g_penunjuk_aktif = SALAH;

/* Posisi kursor saat ini */
static tak_bertanda32_t g_pos_x = 0;
static tak_bertanda32_t g_pos_y = 0;

/* Ukuran layar (batas kursor) */
static tak_bertanda32_t g_layar_lebar = 0;
static tak_bertanda32_t g_layar_tinggi = 0;

/* Status tombol kursor saat ini */
static tak_bertanda32_t g_tombol = 0;

/* Status tombol pada frame sebelumnya */
static tak_bertanda32_t g_tombol_sebelumnya = 0;

/* Flag visibilitas kursor */
static bool_t g_tampil = BENAR;

/* Kecepatan kursor (pengali pergerakan) */
static tak_bertanda32_t g_kecepatan =
    PENUNJUK_KECEPATAN_DEFAULT;

/* Area clipping kursor (0 = tidak di-klip) */
static tak_bertanda32_t g_klip_x1 = 0;
static tak_bertanda32_t g_klip_y1 = 0;
static tak_bertanda32_t g_klip_x2 = 0;
static tak_bertanda32_t g_klip_y2 = 0;
static bool_t g_klip_aktif = SALAH;

/* State grab kursor */
static tak_bertanda32_t g_grab_jendela =
    PENUNJUK_GRAB_MAKS;
static bool_t g_ter_grab = SALAH;

/* Statistik */
static tak_bertanda64_t g_total_gerakan = 0;
static tak_bertanda64_t g_total_klik = 0;
static tak_bertanda64_t g_total_grab = 0;

/*
 * ===========================================================================
 * FUNGSI INTERNAL - BATASI POSISI KE LAYAR
 * ===========================================================================
 *
 * Pastikan posisi kursor berada dalam batas layar.
 * Jika clipping aktif, batasi juga ke area klip.
 */

static void batasi_posisi(void)
{
    tak_bertanda32_t min_x = 0;
    tak_bertanda32_t min_y = 0;
    tak_bertanda32_t max_x = g_layar_lebar > 0
        ? g_layar_lebar - 1 : 0;
    tak_bertanda32_t max_y = g_layar_tinggi > 0
        ? g_layar_tinggi - 1 : 0;

    /* Terapkan area clipping jika aktif */
    if (g_klip_aktif) {
        if (g_klip_x1 > min_x) {
            min_x = g_klip_x1;
        }
        if (g_klip_y1 > min_y) {
            min_y = g_klip_y1;
        }
        if (g_klip_x2 > 0 &&
            g_klip_x2 - 1 < max_x) {
            max_x = g_klip_x2 - 1;
        }
        if (g_klip_y2 > 0 &&
            g_klip_y2 - 1 < max_y) {
            max_y = g_klip_y2 - 1;
        }
    }

    /* Batasi posisi */
    if (g_pos_x < min_x) {
        g_pos_x = min_x;
    }
    if (g_pos_x > max_x) {
        g_pos_x = max_x;
    }
    if (g_pos_y < min_y) {
        g_pos_y = min_y;
    }
    if (g_pos_y > max_y) {
        g_pos_y = max_y;
    }
}

/*
 * ===========================================================================
 * API PUBLIK - INISIALISASI PENUNJUK
 * ===========================================================================
 *
 * Menyiapkan state penunjuk. Set posisi ke tengah layar.
 */

status_t penunjuk_init(tak_bertanda32_t layar_lebar,
                        tak_bertanda32_t layar_tinggi)
{
    if (g_penunjuk_aktif) {
        return STATUS_SUDAH_ADA;
    }

    g_layar_lebar = layar_lebar;
    g_layar_tinggi = layar_tinggi;

    /* Posisi awal: tengah layar */
    g_pos_x = layar_lebar / 2;
    g_pos_y = layar_tinggi / 2;

    g_tombol = MOUSE_BUTTON_NONE;
    g_tombol_sebelumnya = MOUSE_BUTTON_NONE;
    g_tampil = BENAR;
    g_kecepatan = PENUNJUK_KECEPATAN_DEFAULT;
    g_klip_aktif = SALAH;
    g_klip_x1 = 0;
    g_klip_y1 = 0;
    g_klip_x2 = 0;
    g_klip_y2 = 0;
    g_ter_grab = SALAH;
    g_grab_jendela = PENUNJUK_GRAB_MAKS;

    g_total_gerakan = 0;
    g_total_klik = 0;
    g_total_grab = 0;

    g_penunjuk_aktif = BENAR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - SHUTDOWN PENUNJUK
 * ===========================================================================
 */

void penunjuk_shutdown(void)
{
    if (!g_penunjuk_aktif) {
        return;
    }

    g_penunjuk_aktif = SALAH;
    g_ter_grab = SALAH;
    g_klip_aktif = SALAH;
}

/*
 * ===========================================================================
 * API PUBLIK - GERAKKAN PENUNJUK (RELATIF)
 * ===========================================================================
 *
 * Menggerakkan kursor berdasarkan pergerakan relatif
 * dari perangkat mouse.
 *
 * Mendukung akselerasi: gerakan cepat (melebihi threshold)
 * akan dikalikan dengan faktor akselerasi untuk
 * pergerakan yang lebih responsif.
 *
 * Parameter:
 *   dx - Pergerakan relatif sumbu X
 *   dy - Pergerakan relatif sumbu Y
 */

void penunjuk_gerak(tanda32_t dx, tanda32_t dy)
{
    tanda32_t abs_dx, abs_dy;

    if (!g_penunjuk_aktif) {
        return;
    }

    /* Hitung nilai absolut untuk deteksi akselerasi */
    abs_dx = dx < 0 ? -dx : dx;
    abs_dy = dy < 0 ? -dy : dy;

    /*
     * Terapkan akselerasi.
     * Jika pergerakan melebihi threshold, kalikan
     * dengan faktor akselerasi.
     */
    if (abs_dx > PENUNJUK_AKS_THRESHOLD) {
        dx = dx * PENUNJUK_AKS_FAKTOR;
    }
    if (abs_dy > PENUNJUK_AKS_THRESHOLD) {
        dy = dy * PENUNJUK_AKS_FAKTOR;
    }

    /* Terapkan kecepatan */
    dx = (tanda32_t)((tak_bertanda32_t)dx * g_kecepatan);
    dy = (tanda32_t)((tak_bertanda32_t)dy * g_kecepatan);

    /* Update posisi */
    if (dx > 0) {
        g_pos_x += (tak_bertanda32_t)dx;
    } else if (dx < 0) {
        tak_bertanda32_t kurang = (tak_bertanda32_t)(-dx);
        if (g_pos_x >= kurang) {
            g_pos_x -= kurang;
        } else {
            g_pos_x = 0;
        }
    }

    if (dy > 0) {
        g_pos_y += (tak_bertanda32_t)dy;
    } else if (dy < 0) {
        tak_bertanda32_t kurang = (tak_bertanda32_t)(-dy);
        if (g_pos_y >= kurang) {
            g_pos_y -= kurang;
        } else {
            g_pos_y = 0;
        }
    }

    /* Batasi posisi ke area layar/klip */
    batasi_posisi();

    g_total_gerakan++;
}

/*
 * ===========================================================================
 * API PUBLIK - SET POSISI LANGSUNG (ABSOLUT)
 * ===========================================================================
 */

void penunjuk_set_posisi(tak_bertanda32_t x,
                          tak_bertanda32_t y)
{
    if (!g_penunjuk_aktif) {
        return;
    }

    g_pos_x = x;
    g_pos_y = y;
    batasi_posisi();
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT POSISI PENUNJUK
 * ===========================================================================
 */

void penunjuk_dapat_posisi(tak_bertanda32_t *x,
                             tak_bertanda32_t *y)
{
    if (x != NULL) {
        *x = g_pos_x;
    }
    if (y != NULL) {
        *y = g_pos_y;
    }
}

/*
 * ===========================================================================
 * API PUBLIK - UPDATE STATUS TOMBOL
 * ===========================================================================
 *
 * Update state tombol kursor dan deteksi klik baru
 * (transisi dari tidak ditekan ke ditekan).
 */

void penunjuk_set_tombol(tak_bertanda32_t tombol)
{
    tak_bertanda32_t perubahan;

    if (!g_penunjuk_aktif) {
        return;
    }

    g_tombol_sebelumnya = g_tombol;
    g_tombol = tombol;

    /* Deteksi klik baru: tombol baru saja ditekan */
    perubahan = g_tombol & ~g_tombol_sebelumnya;
    if (perubahan != 0) {
        g_total_klik++;
    }
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT STATUS TOMBOL
 * ===========================================================================
 */

tak_bertanda32_t penunjuk_dapat_tombol(void)
{
    return g_tombol;
}

/*
 * ===========================================================================
 * API PUBLIK - CEK APAKAH TOMBOL DITEKAN
 * ===========================================================================
 */

bool_t penunjuk_tombol_ditekan(tak_bertanda32_t tombol_mask)
{
    return (g_tombol & tombol_mask) != 0 ? BENAR : SALAH;
}

/*
 * ===========================================================================
 * API PUBLIK - SET VISIBILITAS PENUNJUK
 * ===========================================================================
 */

void penunjuk_set_tampil(bool_t tampil)
{
    g_tampil = tampil;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT VISIBILITAS PENUNJUK
 * ===========================================================================
 */

bool_t penunjuk_dapat_tampil(void)
{
    return g_tampil;
}

/*
 * ===========================================================================
 * API PUBLIK - SET KECEPATAN PENUNJUK
 * ===========================================================================
 *
 * Set faktor kecepatan kursor. Nilai 1 = normal,
 * nilai lebih tinggi = lebih cepat.
 */

status_t penunjuk_set_kecepatan(tak_bertanda32_t kecepatan)
{
    if (kecepatan == 0) {
        return STATUS_PARAM_INVALID;
    }

    g_kecepatan = kecepatan;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - SET AREA KLIP PENUNJUK
 * ===========================================================================
 *
 * Batasi pergerakan kursor ke area persegi panjang tertentu.
 * Digunakan saat jendela meng-grab kursor (contoh:
 * resize handle, fullscreen game).
 *
 * Set x1=y1=x2=y2=0 dan aktifkan untuk melepas klip.
 */

void penunjuk_set_klip(tak_bertanda32_t x1,
                        tak_bertanda32_t y1,
                        tak_bertanda32_t x2,
                        tak_bertanda32_t y2,
                        bool_t aktif)
{
    g_klip_x1 = x1;
    g_klip_y1 = y1;
    g_klip_x2 = x2;
    g_klip_y2 = y2;
    g_klip_aktif = aktif;

    if (aktif) {
        batasi_posisi();
    }
}

/*
 * ===========================================================================
 * API PUBLIK - GRAB PENUNJUK (AMBIL ALIH KURSOR)
 * ===========================================================================
 *
 * Ambil alih kursor. Kursor akan terkunci ke jendela
 * yang melakukan grab. Hanya jendela pemilik grab yang
 * menerima event pergerakan kursor.
 *
 * Parameter:
 *   id_jendela - ID jendela yang meng-grab
 *
 * Return: STATUS_BERHASIL jika berhasil grab,
 *         STATUS_PARAM_NULL jika tidak ada grab aktif
 *         sebelumnya,
 *         STATUS_BUSY jika kursor sudah di-grab.
 */

status_t penunjuk_grab(tak_bertanda32_t id_jendela)
{
    if (!g_penunjuk_aktif) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Cek apakah sudah di-grab */
    if (g_ter_grab && g_grab_jendela != id_jendela) {
        return STATUS_BUSY;
    }

    g_ter_grab = BENAR;
    g_grab_jendela = id_jendela;
    g_total_grab++;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - RELEASE PENUNJUK (LEPAS KURSOR)
 * ===========================================================================
 *
 * Lepas kursor dari grab jendela.
 * Hanya jendela pemilik grab yang bisa melepas.
 *
 * Parameter:
 *   id_jendela - ID jendela yang melepas grab
 *
 * Return: STATUS_BERHASIL jika berhasil,
 *         STATUS_PARAM_INVALID jika bukan pemilik grab.
 */

status_t penunjuk_release(tak_bertanda32_t id_jendela)
{
    if (!g_penunjuk_aktif) {
        return STATUS_TIDAK_DUKUNG;
    }

    if (!g_ter_grab) {
        return STATUS_BERHASIL;
    }

    if (g_grab_jendela != id_jendela) {
        return STATUS_PARAM_INVALID;
    }

    g_ter_grab = SALAH;
    g_grab_jendela = PENUNJUK_GRAB_MAKS;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT PEMILIK GRAB
 * ===========================================================================
 *
 * Mengembalikan ID jendela yang sedang meng-grab kursor,
 * atau 0 jika tidak ada grab aktif.
 */

tak_bertanda32_t penunjuk_dapat_grab(void)
{
    if (!g_ter_grab) {
        return 0;
    }
    return g_grab_jendela;
}

/*
 * ===========================================================================
 * API PUBLIK - CEK APAKAH GRAB AKTIF
 * ===========================================================================
 */

bool_t penunjuk_grab_aktif(void)
{
    return g_ter_grab;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT STATISTIK PENUNJUK
 * ===========================================================================
 */

void penunjuk_dapat_statistik(tak_bertanda64_t *gerakan,
                               tak_bertanda64_t *klik,
                               tak_bertanda64_t *grab)
{
    if (gerakan != NULL) {
        *gerakan = g_total_gerakan;
    }
    if (klik != NULL) {
        *klik = g_total_klik;
    }
    if (grab != NULL) {
        *grab = g_total_grab;
    }
}
