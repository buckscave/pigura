/*
 * PIGURA OS - HANCURKAN.C
 * =======================
 Penghapusan lapisan dan pembersihan konteks penata.
 *
 * Modul ini menyediakan fungsi untuk menghancurkan
 * lapisan individual atau semua lapisan sekaligus,
 * serta mereset efek dan animasi yang terkait.
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
 * API PUBLIK - HANCURKAN SEMUA LAPISAN
 * ===========================================================================
 *
 * Hapus semua lapisan dalam konteks penata.
 * Membebaskan sumber daya yang terkait setiap lapisan
 * (efek, permukaan konten, animasi).
 */

void penata_hancurkan_semua(penata_konteks_t *ktx)
{
    tak_bertanda32_t i;

    if (ktx == NULL) return;

    for (i = 0; i < PENATA_LAPISAN_MAKS; i++) {
        penata_lapisan_t *l = &ktx->lapisan[i];

        if (l->magic != PENATA_MAGIC) continue;

        /* Reset magic (tandai slot kosong) */
        l->magic = 0;

        /* Bersihkan efek */
        l->jumlah_efek = 0;

        /* Lepaskan referensi permukaan */
        l->permukaan = NULL;
    }

    ktx->jumlah_lapisan = 0;
}

/*
 * ===========================================================================
 * API PUBLIK - HANCURKAN SATU LAPISAN
 * ===========================================================================
 *
 * Hancurkan satu lapisan berdasarkan ID.
 * Termasuk membersihkan efek dan melepaskan
 * referensi ke permukaan konten.
 */

status_t penata_hancurkan_lapisan(penata_konteks_t *ktx,
                                  tak_bertanda32_t id)
{
    penata_lapisan_t *l;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }

    l = penata_lapisan_cari(ktx, id);
    if (l == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Reset magic */
    l->magic = 0;

    /* Bersihkan efek */
    l->jumlah_efek = 0;

    /* Lepaskan referensi permukaan */
    l->permukaan = NULL;

    /* Update counter */
    if (ktx->jumlah_lapisan > 0) {
        ktx->jumlah_lapisan--;
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - BERSIHKAN EFEK SEMUA LAPISAN
 * ===========================================================================
 *
 * Hapus semua efek dari semua lapisan.
 */

void penata_hancurkan_efek_semua(penata_konteks_t *ktx)
{
    tak_bertanda32_t i;

    if (ktx == NULL) return;

    for (i = 0; i < PENATA_LAPISAN_MAKS; i++) {
        penata_lapisan_t *l = &ktx->lapisan[i];

        if (l->magic != PENATA_MAGIC) continue;

        l->jumlah_efek = 0;
        l->flags &= ~(tak_bertanda32_t)PENATA_FLAG_EFEK;
    }
}

/*
 * ===========================================================================
 * API PUBLIK - BERSIHKAN ANIMASI SEMUA
 * ===========================================================================
 *
 * Hentikan dan bersihkan semua animasi aktif.
 */

void penata_hancurkan_animasi_semua(penata_konteks_t *ktx)
{
    tak_bertanda32_t i;

    if (ktx == NULL) return;

    for (i = 0; i < PENATA_ANIMASI_FRAMA_MAKS; i++) {
        ktx->animasi[i].status = PENATA_ANIMASI_STOP;
        ktx->animasi[i].fungsi = NULL;
        ktx->animasi[i].data = NULL;
        ktx->animasi[i].waktu_sekarang = 0;
    }

    ktx->jumlah_animasi = 0;
}

/*
 * ===========================================================================
 * API PUBLIK - RESET STATISTIK
 * ===========================================================================
 *
 * Reset semua counter statistik konteks penata.
 */

void penata_reset_statistik(penata_konteks_t *ktx)
{
    if (ktx == NULL) return;

    ktx->jumlah_render = 0;
    ktx->jumlah_klip = 0;
    ktx->piksel_diproses = 0;
}
