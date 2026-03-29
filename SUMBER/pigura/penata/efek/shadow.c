/*
 * PIGURA OS - SHADOW.C
 * ====================
 * Efek bayangan (drop shadow) subsistem penata Pigura OS.
 *
 * Menggambar bayangan di belakang area konten dengan
 * offset, radius blur, dan warna yang dapat dikonfigurasi.
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
 * FUNGSI HELPER - EKSTRAK KOMPONEN RGBA
 * ===========================================================================
 */

static tak_bertanda8_t ambil_r(tak_bertanda32_t piksel)
{
    return (tak_bertanda8_t)((piksel >> 16) & 0xFFU);
}

static tak_bertanda8_t ambil_g(tak_bertanda32_t piksel)
{
    return (tak_bertanda8_t)((piksel >> 8) & 0xFFU);
}

static tak_bertanda8_t ambil_b(tak_bertanda32_t piksel)
{
    return (tak_bertanda8_t)(piksel & 0xFFU);
}

static tak_bertanda8_t ambil_a(tak_bertanda32_t piksel)
{
    return (tak_bertanda8_t)((piksel >> 24) & 0xFFU);
}

static tak_bertanda32_t buat_piksel(tak_bertanda8_t r,
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
 * ===========================================================================
 * FUNGSI HELPER - ALPHA BLEND DUA PIKSEL
 * ===========================================================================
 */

static tak_bertanda32_t campur_alpha(tak_bertanda32_t bg,
                                     tak_bertanda32_t fg)
{
    tak_bertanda8_t bg_r, bg_g, bg_b, bg_a;
    tak_bertanda8_t fg_r, fg_g, fg_b, fg_a;
    tak_bertanda32_t inv_alpha;
    tak_bertanda32_t out_r, out_g, out_b, out_a;

    bg_r = ambil_r(bg);
    bg_g = ambil_g(bg);
    bg_b = ambil_b(bg);
    bg_a = ambil_a(bg);

    fg_r = ambil_r(fg);
    fg_g = ambil_g(fg);
    fg_b = ambil_b(fg);
    fg_a = ambil_a(fg);

    if (fg_a == 0) return bg;
    if (fg_a == 255) return fg;

    inv_alpha = 255 - fg_a;

    out_r = (bg_r * inv_alpha + fg_r * fg_a) / 255;
    out_g = (bg_g * inv_alpha + fg_g * fg_a) / 255;
    out_b = (bg_b * inv_alpha + fg_b * fg_a) / 255;
    out_a = bg_a + ((255 - bg_a) * fg_a) / 255;

    return buat_piksel(
        (tak_bertanda8_t)out_r,
        (tak_bertanda8_t)out_g,
        (tak_bertanda8_t)out_b,
        (tak_bertanda8_t)out_a);
}

/*
 * ===========================================================================
 * API PUBLIK - BAYANGAN (DROP SHADOW)
 * ===========================================================================
 *
 * Menggambar bayangan di belakang area konten.
 * Langkah:
 * 1. Simpan konten asli dalam area bayangan.
 * 2. Isi area bayangan dengan warna bayangan.
 * 3. Blur area bayangan.
 * 4. Alpha-blend konten asli di atas bayangan.
 */

status_t penata_bayangan(permukaan_t *target,
                         const penata_rect_t *area,
                         tak_bertanda32_t offset_x,
                         tak_bertanda32_t offset_y,
                         tak_bertanda32_t radius,
                         tak_bertanda32_t warna)
{
    tak_bertanda32_t sw_lebar, sw_tinggi;
    tak_bertanda32_t bx, by, bw, bh;
    tak_bertanda32_t x, y;
    tak_bertanda8_t warna_a;
    tak_bertanda32_t bg_piksel, shadow_piksel, hasil;
    penata_rect_t area_blur;

    if (target == NULL || area == NULL) {
        return STATUS_PARAM_NULL;
    }

    sw_lebar = permukaan_lebar(target);
    sw_tinggi = permukaan_tinggi(target);

    /* Hitung posisi dan ukuran area bayangan */
    bx = ((tak_bertanda32_t)area->x + offset_x < sw_lebar)
         ? (tak_bertanda32_t)area->x + offset_x
         : sw_lebar;
    by = ((tak_bertanda32_t)area->y + offset_y < sw_tinggi)
         ? (tak_bertanda32_t)area->y + offset_y
         : sw_tinggi;
    bw = area->lebar + radius * 2;
    bh = area->tinggi + radius * 2;

    /* Batasi agar tidak keluar permukaan */
    if (bx + bw > sw_lebar) bw = sw_lebar - bx;
    if (by + bh > sw_tinggi) bh = sw_tinggi - by;

    if (bw == 0 || bh == 0) {
        return STATUS_BERHASIL;
    }

    /* Ekstrak alpha dari warna bayangan */
    warna_a = (tak_bertanda8_t)((warna >> 24) & 0xFFU);

    /*
     * Langkah 1: Gambar warna bayangan di area shadow.
     * Alpha warna bayangan menentukan intensitas.
     */
    for (y = by; y < by + bh; y++) {
        for (x = bx; x < bx + bw; x++) {
            /* Hitung jarak dari tepi konten untuk fade */
            tanda32_t dx = 0, dy = 0;
            tak_bertanda32_t jarak;
            tak_bertanda8_t alpha_lokal;

            if (x < (tak_bertanda32_t)area->x)
                dx = (tanda32_t)area->x - (tanda32_t)x;
            else if (x >=
                (tak_bertanda32_t)area->x + area->lebar)
                dx = (tanda32_t)x -
                    ((tanda32_t)area->x +
                    (tanda32_t)area->lebar);

            if (y < (tak_bertanda32_t)area->y)
                dy = (tanda32_t)area->y - (tanda32_t)y;
            else if (y >=
                (tak_bertanda32_t)area->y + area->tinggi)
                dy = (tanda32_t)y -
                    ((tanda32_t)area->y +
                    (tanda32_t)area->tinggi);

            jarak = (tak_bertanda32_t)(dx * dx + dy * dy);

            if (jarak >= radius * radius) {
                continue;
            }

            /* Fade berdasarkan jarak */
            alpha_lokal = (tak_bertanda8_t)(
                (warna_a *
                (radius * radius - jarak)) /
                (radius * radius));

            shadow_piksel = buat_piksel(
                (tak_bertanda8_t)((warna >> 16) & 0xFFU),
                (tak_bertanda8_t)((warna >> 8) & 0xFFU),
                (tak_bertanda8_t)(warna & 0xFFU),
                alpha_lokal);

            bg_piksel = permukaan_baca_piksel(target, x, y);
            hasil = campur_alpha(bg_piksel, shadow_piksel);
            permukaan_tulis_piksel(target, x, y, hasil);
        }
    }

    /*
     * Langkah 2: Blur area bayangan untuk efek halus.
     * Hanya jika radius > 1 dan area cukup besar.
     */
    if (radius > 1 && bw >= 4 && bh >= 4) {
        area_blur.x = (tanda32_t)bx;
        area_blur.y = (tanda32_t)by;
        area_blur.lebar = bw;
        area_blur.tinggi = bh;
        /* Gunakan 1 iterasi blur untuk efisiensi */
        penata_blur_kotak(target, &area_blur,
                          PENATA_BLUR_MIN, 1);
    }

    return STATUS_BERHASIL;
}
