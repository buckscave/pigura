/*
 * PIGURA OS - PENGOLAH.C
 * =====================
 * Proses dan render utama subsistem penata.
 *
 * Modul ini mengorkestrasi pipeline render:
 *   1. Proses animasi aktif
 *   2. Urutkan lapisan berdasarkan z-order
 *   3. Render setiap lapisan ke permukaan target
 *   4. Terapkan efek dan klip
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
 * FUNGSI INTERNAL - RENDER SATU LAPISAN
 * ===========================================================================
 *
 * Render satu lapisan ke permukaan target.
 * Termasuk klip area, efek, dan alpha blending.
 */

static status_t render_lapisan(penata_konteks_t *ktx,
                               penata_lapisan_t *l)
{
    penata_rect_t area_target;
    tak_bertanda32_t tw, th;

    if (ktx == NULL || l == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (l->permukaan == NULL) {
        return STATUS_BERHASIL;
    }
    if (!(l->flags & PENATA_FLAG_VISIBLE)) {
        return STATUS_BERHASIL;
    }

    tw = permukaan_lebar(ktx->permukaan_target);
    th = permukaan_tinggi(ktx->permukaan_target);

    /* Bangun area target */
    area_target.x = l->x;
    area_target.y = l->y;
    area_target.lebar = l->lebar;
    area_target.tinggi = l->tinggi;

    /* Klip area target ke permukaan target */
    if (!penata_klip_kotak(
            (const penata_rect_t *)&area_target,
            &area_target)) {
        /* Area sepenuhnya di luar permukaan */
        return STATUS_BERHASIL;
    }

    /* Klip ke region klip global jika ada */
    if (ktx->region_klip_global.jumlah > 0) {
        tak_bertanda32_t ri;
        for (ri = 0;
             ri < ktx->region_klip_global.jumlah; ri++) {
            penata_rect_t irisan;
            if (penata_intersek(&area_target,
                &ktx->region_klip_global.rect[ri],
                &irisan)) {
                area_target = irisan;
                break;
            }
        }
    }

    /*
     * Terapkan efek sebelum blit jika ada.
     * Efek diterapkan ke permukaan konten lapisan.
     */
    if ((l->flags & PENATA_FLAG_EFEK) &&
        l->jumlah_efek > 0) {
        tak_bertanda32_t ei;
        penata_rect_t area_efek;
        area_efek.x = 0;
        area_efek.y = 0;
        area_efek.lebar =
            permukaan_lebar(l->permukaan);
        area_efek.tinggi =
            permukaan_tinggi(l->permukaan);

        for (ei = 0; ei < l->jumlah_efek; ei++) {
            penata_efek_terapkan(&l->efek[ei],
                l->permukaan, &area_efek);
        }
    }

    /*
     * Terapkan alpha transparansi jika diperlukan.
     */
    if ((l->flags & PENATA_FLAG_TRANSPARAN) &&
        l->alpha < 255) {
        penata_rect_t area_alpha;
        area_alpha.x = 0;
        area_alpha.y = 0;
        area_alpha.lebar =
            permukaan_lebar(l->permukaan);
        area_alpha.tinggi =
            permukaan_tinggi(l->permukaan);
        penata_campur_alpha(l->permukaan,
            &area_alpha, l->alpha);
    }

    /*
     * Blit permukaan konten lapisan ke permukaan target.
     * Menggunakan permukaan_blit_copy untuk operasi
     * copy area dari sumber ke tujuan.
     */
    permukaan_blit_copy(
        l->permukaan,
        ktx->permukaan_target,
        0, 0,
        (tak_bertanda32_t)area_target.x,
        (tak_bertanda32_t)area_target.y,
        area_target.lebar,
        area_target.tinggi);

    /* Update statistik */
    ktx->piksel_diproses +=
        (tak_bertanda64_t)area_target.lebar *
        (tak_bertanda64_t)area_target.tinggi;
    ktx->jumlah_klip++;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - PROSES
 * ===========================================================================
 *
 * Proses satu langkah (frame) penata.
 * Memproses animasi dan mempersiapkan state
 * untuk rendering.
 */

status_t penata_proses(penata_konteks_t *ktx)
{
    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (!penata_siap(ktx)) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Proses animasi (16ms per frame @ ~60fps) */
    penata_animasi_langkah(ktx, 16);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - RENDER
 * ===========================================================================
 *
 * Render semua lapisan ke permukaan target.
 * Lapisan digambar dari bawah ke atas sesuai z-order.
 */

status_t penata_render(penata_konteks_t *ktx)
{
    status_t st;
    tak_bertanda32_t i;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (!penata_siap(ktx)) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Urutkan lapisan berdasarkan z-order */
    st = penata_lapisan_urutkan(ktx);
    if (st != STATUS_BERHASIL) {
        return st;
    }

    /* Render lapisan dari z terendah ke tertinggi */
    for (i = 0; i < PENATA_LAPISAN_MAKS; i++) {
        penata_lapisan_t *l = &ktx->lapisan[i];

        if (l->magic != PENATA_MAGIC) continue;
        if (!(l->flags & PENATA_FLAG_VISIBLE)) continue;
        if (l->permukaan == NULL) continue;

        st = render_lapisan(ktx, l);
        if (st != STATUS_BERHASIL) {
            return st;
        }
    }

    ktx->jumlah_render++;
    return STATUS_BERHASIL;
}
