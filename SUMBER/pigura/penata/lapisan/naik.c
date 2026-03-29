/*
 * PIGURA OS - NAIK.C
 * ================
 * Naikkan lapisan ke posisi lebih tinggi (z-order).
 *
 * Modul ini mengimplementasikan operasi "naik" yang
 * memindahkan lapisan satu posisi ke atas dalam
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
 * API PUBLIK - NAIKKAN LAPISAN
 * ===========================================================================
 *
 * Naikkan lapisan satu posisi ke atas dalam z-order.
 * Jika lapisan sudah di posisi tertinggi, tidak
 * ada perubahan.
 *
 * Z-order yang lebih tinggi berarti lapisan digambar
 * di atas lapisan lain (lebih dekat ke pengguna).
 */

status_t penata_lapisan_naik(penata_konteks_t *ktx,
                             tak_bertanda32_t id)
{
    penata_lapisan_t *target;
    tak_bertanda32_t i;
    tanda32_t target_z;
    tanda32_t z_di_atas;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }

    target = penata_lapisan_cari(ktx, id);
    if (target == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    target_z = target->z_urutan;
    z_di_atas = target_z;

    /* Cari z_urutan yang tepat di atas target */
    for (i = 0; i < PENATA_LAPISAN_MAKS; i++) {
        penata_lapisan_t *l = &ktx->lapisan[i];
        if (l->magic != PENATA_MAGIC) continue;
        if (l == target) continue;

        if (l->z_urutan > target_z &&
            (z_di_atas == target_z ||
             l->z_urutan < z_di_atas)) {
            z_di_atas = l->z_urutan;
        }
    }

    /* Jika sudah di posisi tertinggi */
    if (z_di_atas == target_z) {
        return STATUS_BERHASIL;
    }

    /* Tukar z_urutan */
    for (i = 0; i < PENATA_LAPISAN_MAKS; i++) {
        penata_lapisan_t *l = &ktx->lapisan[i];
        if (l->magic != PENATA_MAGIC) continue;
        if (l == target) continue;

        if (l->z_urutan == z_di_atas) {
            l->z_urutan = target_z;
            target->z_urutan = z_di_atas;
            break;
        }
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - NAIKKAN KE PALING ATAS
 * ===========================================================================
 *
 * Naikkan lapisan ke posisi paling atas (z-order tertinggi).
 * Lapisan akan digambar di atas semua lapisan lainnya.
 */

status_t penata_lapisan_naik_ke_atas(penata_konteks_t *ktx,
                                    tak_bertanda32_t id)
{
    penata_lapisan_t *target;
    tak_bertanda32_t i;
    tanda32_t z_tertinggi;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }

    target = penata_lapisan_cari(ktx, id);
    if (target == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Cari z_urutan tertinggi saat ini */
    z_tertinggi = target->z_urutan;
    for (i = 0; i < PENATA_LAPISAN_MAKS; i++) {
        penata_lapisan_t *l = &ktx->lapisan[i];
        if (l->magic != PENATA_MAGIC) continue;
        if (l->z_urutan > z_tertinggi) {
            z_tertinggi = l->z_urutan;
        }
    }

    /* Jika sudah di paling atas */
    if (target->z_urutan == z_tertinggi) {
        return STATUS_BERHASIL;
    }

    /* Set ke z tertinggi + 1 */
    target->z_urutan = z_tertinggi + 1;
    return STATUS_BERHASIL;
}
