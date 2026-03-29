/*
 * PIGURA OS - PENATA.C
 * ===================
 * Modul utama compositor penata Pigura OS.
 *
 * Menyediakan fungsi pembuatan dan penghapusan konteks
 * penata, serta konfigurasi permukaan target.
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
 * API PUBLIK - BUAT KONTEKS PENATA
 * ===========================================================================
 *
 * Alokasi dan inisialisasi konteks penata baru.
 * Mengembalikan pointer ke konteks yang dibuat,
 * atau NULL jika gagal.
 *
 * Konteks penata harus dihapus dengan penata_hapus()
 * setelah selesai digunakan.
 */

penata_konteks_t *penata_buat(permukaan_t *target)
{
    penata_konteks_t *ktx;

    if (target == NULL) {
        return NULL;
    }

    /* Gunakan alokasi memori kernel */
    ktx = (penata_konteks_t *)kmalloc(
        sizeof(penata_konteks_t));
    if (ktx == NULL) {
        return NULL;
    }

    kernel_memset(ktx, 0, sizeof(penata_konteks_t));

    ktx->magic = PENATA_MAGIC;
    ktx->versi = PENATA_VERSI;
    ktx->permukaan_target = target;
    ktx->jumlah_lapisan = 0;
    ktx->id_counter = 0;
    ktx->jumlah_animasi = 0;
    ktx->jumlah_render = 0;
    ktx->jumlah_klip = 0;
    ktx->piksel_diproses = 0;
    ktx->diinisialisasi = BENAR;

    /* Inisialisasi region klip global ke area kosong */
    penata_region_buat(&ktx->region_klip_global);

    return ktx;
}

/*
 * ===========================================================================
 * API PUBLIK - HAPUS KONTEKS PENATA
 * ===========================================================================
 *
 * Hapus konteks penata dan bebaskan memori.
 * Semua lapisan akan dihancurkan terlebih dahulu.
 */

void penata_hapus(penata_konteks_t *ktx)
{
    if (ktx == NULL) return;

    /* Hancurkan semua lapisan */
    penata_hancurkan_semua(ktx);

    /* Bersihkan state */
    ktx->permukaan_target = NULL;
    ktx->jumlah_lapisan = 0;
    ktx->diinisialisasi = SALAH;
    ktx->magic = 0;

    kfree(ktx);
}

/*
 * ===========================================================================
 * API PUBLIK - SET PERMUKAAN TARGET
 * ===========================================================================
 *
 * Ganti permukaan target untuk rendering.
 */

status_t penata_set_target(penata_konteks_t *ktx,
                           permukaan_t *target)
{
    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (target == NULL) {
        return STATUS_PARAM_NULL;
    }

    ktx->permukaan_target = target;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - CEK KESIAPAN
 * ===========================================================================
 */

bool_t penata_siap(const penata_konteks_t *ktx)
{
    if (ktx == NULL) return SALAH;
    if (ktx->magic != PENATA_MAGIC) return SALAH;
    if (!ktx->diinisialisasi) return SALAH;
    if (ktx->permukaan_target == NULL) return SALAH;
    return BENAR;
}
