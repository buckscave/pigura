/*
 * PIGURA OS - KLIP.C
 * ==================
 * Operasi klip titik dan garis subsistem penata.
 *
 * Modul ini menyediakan fungsi dasar klip:
 *   - Klip titik ke area persegi panjang
 *   - Klip garis ke area persegi panjang (Cohen-Sutherland)
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
#include "../../framebuffer/framebuffer.h"
#include "../penata.h"

/*
 * ===========================================================================
 * API PUBLIK - KLIP TITIK
 * ===========================================================================
 *
 * Cek apakah titik (x, y) berada di dalam area klip.
 * Mengembalikan BENAR jika titik di dalam area.
 */

bool_t penata_klip_titik(const penata_rect_t *klip,
                         tanda32_t x, tanda32_t y)
{
    tak_bertanda32_t kx2, ky2;

    if (klip == NULL) return SALAH;

    kx2 = (tak_bertanda32_t)klip->x + klip->lebar;
    ky2 = (tak_bertanda32_t)klip->y + klip->tinggi;

    /* Cek batas */
    if (x < klip->x || y < klip->y) return SALAH;
    if ((tak_bertanda32_t)x >= kx2) return SALAH;
    if ((tak_bertanda32_t)y >= ky2) return SALAH;

    return BENAR;
}

/*
 * ===========================================================================
 * ALGORITMA COHEN-SUTHERLAND - KODE REGION
 * ===========================================================================
 */

/* Kode region untuk Cohen-Sutherland */
#define CS_DI      0x0001  /* Bawah (y > y_max) */
#define CS_DD      0x0002  /* Atas (y < y_min)   */
#define CS_DK      0x0004  /* Kanan (x > x_max) */
#define CS_DKK     0x0008  /* Kiri (x < x_min)   */

/*
 * Hitung kode region untuk titik (x, y).
 */
static tak_bertanda32_t cs_kode(const penata_rect_t *klip,
                                tanda32_t x, tanda32_t y)
{
    tak_bertanda32_t kode = 0;
    tanda32_t x_min, y_min, x_max, y_max;

    if (klip == NULL) return 0;

    x_min = klip->x;
    y_min = klip->y;
    x_max = klip->x + (tanda32_t)klip->lebar - 1;
    y_max = klip->y + (tanda32_t)klip->tinggi - 1;

    if (x < x_min) kode |= CS_DKK;
    else if (x > x_max) kode |= CS_DK;

    if (y < y_min) kode |= CS_DD;
    else if (y > y_max) kode |= CS_DI;

    return kode;
}

/*
 * ===========================================================================
 * API PUBLIK - KLIP GARIS (COHEN-SUTHERLAND)
 * ===========================================================================
 *
 * Klip garis dari (x0,y0) ke (x1,y1) terhadap area klip.
 * Mengembalikan BENAR jika sebagian garis terlihat.
 * Titik akhir garis diperbarui ke koordinat terklip.
 */

status_t penata_klip_garis(const penata_rect_t *klip,
                           tanda32_t *x0, tanda32_t *y0,
                           tanda32_t *x1, tanda32_t *y1)
{
    tak_bertanda32_t kode0, kode1, kode_out;
    tanda32_t lx, ly, dx, dy;
    tanda32_t x_min, y_min, x_max, y_max;
    bool_t terima = SALAH;
    tak_bertanda32_t iterasi = 0;
    tak_bertanda32_t iterasi_maks = 32;

    if (klip == NULL || x0 == NULL || y0 == NULL ||
        x1 == NULL || y1 == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (klip->lebar == 0 || klip->tinggi == 0) {
        return STATUS_PARAM_UKURAN;
    }

    x_min = klip->x;
    y_min = klip->y;
    x_max = klip->x + (tanda32_t)klip->lebar - 1;
    y_max = klip->y + (tanda32_t)klip->tinggi - 1;

    kode0 = cs_kode(klip, *x0, *y0);
    kode1 = cs_kode(klip, *x1, *y1);

    while (iterasi < iterasi_maks) {
        iterasi++;

        if ((kode0 | kode1) == 0) {
            /* Kedua titik di dalam: terima */
            terima = BENAR;
            break;
        }

        if ((kode0 & kode1) != 0) {
            /* Kedua titik di sisi yang sama: tolak */
            break;
        }

        /* Pilih titik yang di luar */
        kode_out = kode0 ? kode0 : kode1;

        if (kode_out & CS_DI) {
            /* Bagian bawah */
            dx = *x1 - *x0;
            dy = *y1 - *y0;
            if (dy != 0) {
                lx = *x0 + dx * (y_max - *y0) / dy;
            } else {
                lx = *x0;
            }
            ly = y_max;
        } else if (kode_out & CS_DD) {
            /* Bagian atas */
            dx = *x1 - *x0;
            dy = *y1 - *y0;
            if (dy != 0) {
                lx = *x0 + dx * (y_min - *y0) / dy;
            } else {
                lx = *x0;
            }
            ly = y_min;
        } else if (kode_out & CS_DK) {
            /* Bagian kanan */
            dx = *x1 - *x0;
            dy = *y1 - *y0;
            if (dx != 0) {
                ly = *y0 + dy * (x_max - *x0) / dx;
            } else {
                ly = *y0;
            }
            lx = x_max;
        } else {
            /* Bagian kiri */
            dx = *x1 - *x0;
            dy = *y1 - *y0;
            if (dx != 0) {
                ly = *y0 + dy * (x_min - *x0) / dx;
            } else {
                ly = *y0;
            }
            lx = x_min;
        }

        /* Perbarui titik luar */
        if (kode_out == kode0) {
            *x0 = lx;
            *y0 = ly;
            kode0 = cs_kode(klip, lx, ly);
        } else {
            *x1 = lx;
            *y1 = ly;
            kode1 = cs_kode(klip, lx, ly);
        }
    }

    return terima ? STATUS_BERHASIL : STATUS_GAGAL;
}
