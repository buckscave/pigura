/*
 * PIGURA OS - RENDAH.C
 * ===================
 * Turunkan lapisan ke posisi lebih rendah (z-order).
 *
 * Modul ini mengimplementasikan operasi "turun" yang
 * memindahkan lapisan satu posisi ke bawah dalam
 * urutan z-order.
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
 * API PUBLIK - TURUNKAN LAPISAN
 * ===========================================================================
 *
 * Turunkan lapisan satu posisi ke bawah dalam z-order.
 * Jika lapisan sudah di posisi terendah, tidak
 * ada perubahan.
 */

status_t penata_lapisan_turun(penata_konteks_t *ktx,
                              tak_bertanda32_t id)
{
    penata_lapisan_t *target;
    tak_bertanda32_t i;
    tanda32_t target_z;
    tanda32_t z_di_bawah;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }

    target = penata_lapisan_cari(ktx, id);
    if (target == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    target_z = target->z_urutan;
    z_di_bawah = target_z;

    /* Cari z_urutan yang tepat di bawah target */
    for (i = 0; i < PENATA_LAPISAN_MAKS; i++) {
        penata_lapisan_t *l = &ktx->lapisan[i];
        if (l->magic != PENATA_MAGIC) continue;
        if (l == target) continue;

        if (l->z_urutan < target_z &&
            (z_di_bawah == target_z ||
             l->z_urutan > z_di_bawah)) {
            z_di_bawah = l->z_urutan;
        }
    }

    /* Jika sudah di posisi terendah */
    if (z_di_bawah == target_z) {
        return STATUS_BERHASIL;
    }

    /* Tukar z_urutan */
    for (i = 0; i < PENATA_LAPISAN_MAKS; i++) {
        penata_lapisan_t *l = &ktx->lapisan[i];
        if (l->magic != PENATA_MAGIC) continue;
        if (l == target) continue;

        if (l->z_urutan == z_di_bawah) {
            l->z_urutan = target_z;
            target->z_urutan = z_di_bawah;
            break;
        }
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - TURUNKAN KE PALING BAWAH
 * ===========================================================================
 *
 * Turunkan lapisan ke posisi paling bawah (z-order terendah).
 * Lapisan akan digambar di bawah semua lapisan lainnya.
 */

status_t penata_lapisan_turun_ke_bawah(penata_konteks_t *ktx,
                                      tak_bertanda32_t id)
{
    penata_lapisan_t *target;
    tak_bertanda32_t i;
    tanda32_t z_terendah;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }

    target = penata_lapisan_cari(ktx, id);
    if (target == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Cari z_urutan terendah saat ini */
    z_terendah = target->z_urutan;
    for (i = 0; i < PENATA_LAPISAN_MAKS; i++) {
        penata_lapisan_t *l = &ktx->lapisan[i];
        if (l->magic != PENATA_MAGIC) continue;
        if (l->z_urutan < z_terendah) {
            z_terendah = l->z_urutan;
        }
    }

    /* Jika sudah di paling bawah */
    if (target->z_urutan == z_terendah) {
        return STATUS_BERHASIL;
    }

    /* Set ke z terendah - 1 */
    target->z_urutan = z_terendah - 1;
    return STATUS_BERHASIL;
}
