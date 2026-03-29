/*
 * PIGURA OS - Z_ORDER.C
 * ===================
 * Pengaturan z-order lapisan subsistem penata.
 *
 * Modul ini mengatur urutan kedalaman (z-order) lapisan
 * untuk menentukan lapisan mana yang digambar di atas
 * lapisan lainnya saat rendering.
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
 * API PUBLIK - SET Z URUTAN
 * ===========================================================================
 *
 * Set z_urutan lapisan secara langsung.
 */

status_t penata_lapisan_set_z(penata_konteks_t *ktx,
                              tak_bertanda32_t id,
                              tanda32_t urutan)
{
    penata_lapisan_t *l;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }

    l = penata_lapisan_cari(ktx, id);
    if (l == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    l->z_urutan = urutan;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - URUTKAN LAPISAN
 * ===========================================================================
 *
 * Urutkan semua lapisan aktif berdasarkan z_urutan.
 * Setelah pemanggilan, lapisan dengan z_urutan lebih
 * rendah akan memiliki z_urutan yang lebih rendah
 * secara konsisten (0, 1, 2, ...).
 *
 * Ini memastikan urutan z-order yang rapi tanpa
 * celah atau duplikasi.
 */

status_t penata_lapisan_urutkan(penata_konteks_t *ktx)
{
    tak_bertanda32_t i, j;
    tak_bertanda32_t aktif[PENATA_LAPISAN_MAKS];
    tak_bertanda32_t count;
    tanda32_t tmp_id;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Kumpulkan indeks lapisan aktif */
    count = 0;
    for (i = 0; i < PENATA_LAPISAN_MAKS; i++) {
        if (ktx->lapisan[i].magic == PENATA_MAGIC) {
            aktif[count] = i;
            count++;
        }
    }

    if (count <= 1) {
        return STATUS_BERHASIL;
    }

    /*
     * Bubble sort berdasarkan z_urutan (ascending).
     * Bubble sort cukup efisien untuk jumlah kecil
     * (maks 32 lapisan).
     */
    for (i = 0; i < count - 1; i++) {
        for (j = 0; j < count - 1 - i; j++) {
            tak_bertanda32_t idx_a = aktif[j];
            tak_bertanda32_t idx_b = aktif[j + 1];
            penata_lapisan_t *la = &ktx->lapisan[idx_a];
            penata_lapisan_t *lb = &ktx->lapisan[idx_b];

            if (la->z_urutan > lb->z_urutan) {
                /* Tukar z_urutan */
                tmp_id = la->z_urutan;
                la->z_urutan = lb->z_urutan;
                lb->z_urutan = tmp_id;
            }
        }
    }

    /*
     * Normalisasi z_urutan menjadi 0, 1, 2, ...
     * untuk menghilangkan celah.
     */
    for (i = 0; i < count; i++) {
        ktx->lapisan[aktif[i]].z_urutan = (tanda32_t)i;
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT URUTAN Z
 * ===========================================================================
 *
 * Dapatkan z_urutan suatu lapisan berdasarkan ID.
 * Mengembalikan -1 jika lapisan tidak ditemukan.
 */

tanda32_t penata_lapisan_dapat_z(penata_konteks_t *ktx,
                                tak_bertanda32_t id)
{
    penata_lapisan_t *l;

    if (ktx == NULL) {
        return -1;
    }

    l = penata_lapisan_cari(ktx, id);
    if (l == NULL) {
        return -1;
    }

    return l->z_urutan;
}

/*
 * ===========================================================================
 * FUNGSI - DAPAT LAPISAN BERDASARKAN Z
 * ===========================================================================
 *
 * Dapatkan lapisan pada posisi z tertentu.
 * idx 0 = lapisan paling bawah.
 * Mengembalikan NULL jika tidak ada.
 */

penata_lapisan_t *penata_lapisan_dari_z(
    penata_konteks_t *ktx, tak_bertanda32_t idx)
{
    tak_bertanda32_t i;

    if (ktx == NULL) return NULL;

    for (i = 0; i < PENATA_LAPISAN_MAKS; i++) {
        penata_lapisan_t *l = &ktx->lapisan[i];
        if (l->magic == PENATA_MAGIC &&
            (tak_bertanda32_t)l->z_urutan == idx) {
            return l;
        }
    }

    return NULL;
}
