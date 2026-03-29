/*
 * PIGURA OS - ISI.C
 * ==================
 * Operasi pengisian area (flood fill) untuk pengolah grafis.
 *
 * Mengimplementasikan algoritma flood fill berbasis tumpukan
 * untuk mengisi area tertutup dengan warna tertentu.
 * Berhenti pada batas warna yang ditentukan.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Seluruh nama dalam Bahasa Indonesia
 * - Mendukung: x86, x86_64, ARM, ARMv7, ARM64
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../../inti/kernel.h"
#include "pengolah.h"

extern status_t cpu_isi_area(struct p_konteks *ktx,
                             tanda32_t x, tanda32_t y,
                             tak_bertanda32_t warna_isi,
                             tak_bertanda32_t warna_batas);
extern status_t gpu_isi_area(struct p_konteks *ktx,
                             tanda32_t x, tanda32_t y,
                             tak_bertanda32_t warna_isi,
                             tak_bertanda32_t warna_batas);

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * pengolah_isi_area
 * ----------------
 * Mengisi area tertutup (flood fill) dimulai dari titik (x,y).
 * Pengisian berhenti saat menemukan piksel warna_batas.
 *
 * Parameter:
 *   ktx        - Pointer ke konteks pengolah
 *   x, y       - Titik awal pengisian
 *   warna_isi  - Warna untuk mengisi area
 *   warna_batas - Warna batas yang menghentikan fill
 *
 * Return:
 *   STATUS_BERHASIL atau kode error
 */
status_t pengolah_isi_area(struct p_konteks *ktx,
                           tanda32_t x, tanda32_t y,
                           tak_bertanda32_t warna_isi,
                           tak_bertanda32_t warna_batas)
{
    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (ktx->magic != PENGOLAH_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    if (ktx->buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (!p_dalam_buffer(ktx, x, y)) {
        return STATUS_PARAM_JARAK;
    }

    /* Flood fill selalu menggunakan CPU karena
     * memerlukan akses piksel individual */
    return cpu_isi_area(ktx, x, y, warna_isi, warna_batas);
}
