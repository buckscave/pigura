/*
 * PIGURA OS - TRANSPARAN.C
 * ========================
 * Efek transparansi (alpha blending) subsistem penata.
 *
 * Modul ini menyediakan operasi alpha blending piksel
 * dan area pada permukaan. Alpha 0 = transparan penuh,
 * alpha 255 = opak penuh.
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
 * FUNGSI HELPER - ALPHA BLEND PIKSEL
 * ===========================================================================
 */

/*
 * Campurkan piksel foreground ke piksel background
 * menggunakan alpha compositing standar Porter-Duff.
 * Format piksel: 0xAARRGGBB.
 */
static tak_bertanda32_t campur_piksel(tak_bertanda32_t bg,
                                      tak_bertanda32_t fg)
{
    tak_bertanda32_t fg_a, inv_a;
    tak_bertanda32_t bg_r, bg_g, bg_b;
    tak_bertanda32_t fg_r, fg_g, fg_b;
    tak_bertanda32_t out_r, out_g, out_b, out_a;

    fg_a = (fg >> 24) & 0xFFU;

    /* Jika foreground opak penuh, langsung kembalikan */
    if (fg_a >= 255) return fg;
    /* Jika foreground transparan penuh, kembalikan bg */
    if (fg_a == 0) return bg;

    inv_a = 255 - fg_a;

    bg_r = (bg >> 16) & 0xFFU;
    bg_g = (bg >> 8) & 0xFFU;
    bg_b = bg & 0xFFU;

    fg_r = (fg >> 16) & 0xFFU;
    fg_g = (fg >> 8) & 0xFFU;
    fg_b = fg & 0xFFU;

    out_r = (bg_r * inv_a + fg_r * fg_a + 127) / 255;
    out_g = (bg_g * inv_a + fg_g * fg_a + 127) / 255;
    out_b = (bg_b * inv_a + fg_b * fg_a + 127) / 255;
    out_a = 255; /* Hasil selalu opak setelah blend */

    /* Clamp ke 0-255 */
    if (out_r > 255) out_r = 255;
    if (out_g > 255) out_g = 255;
    if (out_b > 255) out_b = 255;

    return (out_a << 24) | (out_r << 16) | (out_g << 8) | out_b;
}

/*
 * Modifikasi alpha piksel. Mengubah alpha piksel
 * menjadi alpha baru. Warna RGB tetap sama.
 */
static tak_bertanda32_t set_alpha(tak_bertanda32_t piksel,
                                   tak_bertanda8_t alpha)
{
    return ((tak_bertanda32_t)alpha << 24) | (piksel & 0x00FFFFFFU);
}

/*
 * Pre-multiply alpha ke komponen RGB.
 * Digunakan untuk alpha blending yang lebih akurat.
 */
static tak_bertanda32_t premultiply(tak_bertanda32_t piksel)
{
    tak_bertanda32_t a, r, g, b;

    a = (piksel >> 24) & 0xFFU;
    r = (piksel >> 16) & 0xFFU;
    g = (piksel >> 8) & 0xFFU;
    b = piksel & 0xFFU;

    /* Pre-multiply: channel = channel * alpha / 255 */
    r = (r * a + 127) / 255;
    g = (g * a + 127) / 255;
    b = (b * a + 127) / 255;

    return (a << 24) | (r << 16) | (g << 8) | b;
}

/*
 * ===========================================================================
 * API PUBLIK - CAMPUR ALPHA PADA AREA
 * ===========================================================================
 *
 * Terapkan alpha blending ke seluruh area pada permukaan.
 * Setiap piksel dalam area akan dibuat semi-transparan
 * dengan alpha yang ditentukan.
 */

status_t penata_campur_alpha(permukaan_t *target,
                             const penata_rect_t *area,
                             tak_bertanda8_t alpha)
{
    tak_bertanda32_t x, y;
    tak_bertanda32_t ax, ay, aw, ah;
    tak_bertanda32_t sw_lebar, sw_tinggi;
    tak_bertanda32_t piksel_bg, piksel_fg, hasil;

    if (target == NULL || area == NULL) {
        return STATUS_PARAM_NULL;
    }

    sw_lebar = permukaan_lebar(target);
    sw_tinggi = permukaan_tinggi(target);

    /* Hitung area yang valid */
    ax = (area->x < 0) ? 0 : (tak_bertanda32_t)area->x;
    ay = (area->y < 0) ? 0 : (tak_bertanda32_t)area->y;
    aw = area->lebar;
    ah = area->tinggi;

    /* Batasi area agar tidak keluar permukaan */
    if (ax + aw > sw_lebar) aw = sw_lebar - ax;
    if (ay + ah > sw_tinggi) ah = sw_tinggi - ay;

    if (aw == 0 || ah == 0) {
        return STATUS_BERHASIL;
    }

    /*
     * Jika alpha = 255 (opak penuh), tidak perlu blend.
     * Jika alpha = 0 (transparan penuh), isi dengan 0.
     */
    if (alpha == 255) {
        return STATUS_BERHASIL;
    }

    if (alpha == 0) {
        for (y = ay; y < ay + ah; y++) {
            for (x = ax; x < ax + aw; x++) {
                permukaan_tulis_piksel(target, x, y, 0);
            }
        }
        return STATUS_BERHASIL;
    }

    /*
     * Proses setiap piksel: buat foreground dengan alpha baru,
     * lalu blend dengan background (piksel saat ini).
     */
    for (y = ay; y < ay + ah; y++) {
        for (x = ax; x < ax + aw; x++) {
            piksel_bg = permukaan_baca_piksel(target, x, y);

            /* Foreground = piksel saat ini dengan alpha baru */
            piksel_fg = set_alpha(piksel_bg, alpha);

            /* Blend: bg dengan fg yang telah di-premultiply */
            piksel_fg = premultiply(piksel_fg);
            hasil = campur_piksel(piksel_bg, piksel_fg);

            permukaan_tulis_piksel(target, x, y, hasil);
        }
    }

    return STATUS_BERHASIL;
}
