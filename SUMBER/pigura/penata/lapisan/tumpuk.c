/*
 * PIGURA OS - TUMPUK.C
 * ===================
 * Operasi tumpukan (stacking) lapisan penata.
 *
 * Modul ini menyediakan operasi untuk mengatur
 * urutan tumpukan lapisan, termasuk:
 *   - Tukar posisi dua lapisan
 *   - Balik urutan semua lapisan
 *   - Geser semua lapisan
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
 * API PUBLIK - TUKAR DUA LAPISAN
 * ===========================================================================
 *
 * Tukar z-order dua lapisan berdasarkan ID.
 * Lapisan dengan ID1 dan ID2 akan bertukar posisi
 * dalam urutan z-order.
 */

status_t penata_lapisan_tukar(penata_konteks_t *ktx,
                              tak_bertanda32_t id1,
                              tak_bertanda32_t id2)
{
    penata_lapisan_t *l1, *l2;
    tanda32_t z_temp;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (id1 == id2) {
        return STATUS_BERHASIL;
    }

    l1 = penata_lapisan_cari(ktx, id1);
    l2 = penata_lapisan_cari(ktx, id2);

    if (l1 == NULL || l2 == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Tukar z_urutan */
    z_temp = l1->z_urutan;
    l1->z_urutan = l2->z_urutan;
    l2->z_urutan = z_temp;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - BALIK URUTAN
 * ===========================================================================
 *
 * Balik urutan z-order semua lapisan yang aktif.
 * Lapisan yang paling atas menjadi paling bawah,
 * dan sebaliknya.
 */

status_t penata_lapisan_balik(penata_konteks_t *ktx)
{
    tak_bertanda32_t i;
    tak_bertanda32_t aktif[PENATA_LAPISAN_MAKS];
    tanda32_t z_values[PENATA_LAPISAN_MAKS];
    tak_bertanda32_t count;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Kumpulkan semua lapisan aktif beserta z-nya */
    count = 0;
    for (i = 0; i < PENATA_LAPISAN_MAKS; i++) {
        if (ktx->lapisan[i].magic == PENATA_MAGIC &&
            (ktx->lapisan[i].flags &
             PENATA_FLAG_VISIBLE)) {
            aktif[count] = i;
            z_values[count] = ktx->lapisan[i].z_urutan;
            count++;
        }
    }

    if (count <= 1) {
        return STATUS_BERHASIL;
    }

    /* Balik array z_values */
    {
        tak_bertanda32_t k;
        tak_bertanda32_t n = count;
        tanda32_t tmp;

        for (k = 0; k < n / 2; k++) {
            tmp = z_values[k];
            z_values[k] = z_values[n - 1 - k];
            z_values[n - 1 - k] = tmp;
        }
    }

    /* Terapkan z_urutan yang sudah dibalik */
    {
        tak_bertanda32_t j;
        for (j = 0; j < count; j++) {
            ktx->lapisan[aktif[j]].z_urutan =
                z_values[j];
        }
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - GESER SEMUA KE ATAS
 * ===========================================================================
 *
 * Geser semua lapisan satu posisi ke atas (ke z-order
 * lebih tinggi). Lapisan di atas tetap di atas.
 */

status_t penata_lapisan_geser_atas(penata_konteks_t *ktx)
{
    tak_bertanda32_t i;
    tak_bertanda32_t aktif[PENATA_LAPISAN_MAKS];
    tanda32_t z_values[PENATA_LAPISAN_MAKS];
    tak_bertanda32_t count;
    tak_bertanda32_t j;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Kumpulkan lapisan aktif */
    count = 0;
    for (i = 0; i < PENATA_LAPISAN_MAKS; i++) {
        if (ktx->lapisan[i].magic == PENATA_MAGIC &&
            (ktx->lapisan[i].flags &
             PENATA_FLAG_VISIBLE)) {
            aktif[count] = i;
            z_values[count] = ktx->lapisan[i].z_urutan;
            count++;
        }
    }

    if (count <= 1) {
        return STATUS_BERHASIL;
    }

    /* Geser semua z_urutan naik satu, yang teratas tetap */
    for (j = 0; j < count - 1; j++) {
        ktx->lapisan[aktif[j]].z_urutan = z_values[j + 1];
    }
    /* Lapisan paling atas mendapat z terendah */
    ktx->lapisan[aktif[count - 1]].z_urutan =
        z_values[0];

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - GESER SEMUA KE BAWAH
 * ===========================================================================
 *
 * Geser semua lapisan satu posisi ke bawah (ke z-order
 * lebih rendah). Lapisan di bawah tetap di bawah.
 */

status_t penata_lapisan_geser_bawah(penata_konteks_t *ktx)
{
    tak_bertanda32_t i;
    tak_bertanda32_t aktif[PENATA_LAPISAN_MAKS];
    tanda32_t z_values[PENATA_LAPISAN_MAKS];
    tak_bertanda32_t count;
    tak_bertanda32_t j;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Kumpulkan lapisan aktif */
    count = 0;
    for (i = 0; i < PENATA_LAPISAN_MAKS; i++) {
        if (ktx->lapisan[i].magic == PENATA_MAGIC &&
            (ktx->lapisan[i].flags &
             PENATA_FLAG_VISIBLE)) {
            aktif[count] = i;
            z_values[count] = ktx->lapisan[i].z_urutan;
            count++;
        }
    }

    if (count <= 1) {
        return STATUS_BERHASIL;
    }

    /* Geser semua z_urutan turun satu, yang terbawah tetap */
    for (j = 1; j < count; j++) {
        ktx->lapisan[aktif[j]].z_urutan = z_values[j - 1];
    }
    /* Lapisan paling bawah mendapat z tertinggi */
    ktx->lapisan[aktif[0]].z_urutan =
        z_values[count - 1];

    return STATUS_BERHASIL;
}
