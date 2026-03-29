/*
 * PIGURA OS - INIT.C
 * ================
 * Inisialisasi dan shutdown subsistem penata.
 *
 * Modul ini menginisialisasi konteks penata yang sudah
 * dialokasikan, mempersiapkan state awal untuk
 * compositor.
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
 * API PUBLIK - INISIALISASI KONTEKS
 * ===========================================================================
 *
 * Inisialisasi konteks penata yang sudah dialokasikan.
 * Memvalidasi permukaan target dan menyiapkan state awal.
 */

status_t penata_init(penata_konteks_t *ktx,
                     permukaan_t *target)
{
    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (target == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Bersihkan seluruh konteks */
    kernel_memset(ktx, 0, sizeof(penata_konteks_t));

    /* Set metadata */
    ktx->magic = PENATA_MAGIC;
    ktx->versi = PENATA_VERSI;
    ktx->permukaan_target = target;

    /* State awal */
    ktx->jumlah_lapisan = 0;
    ktx->id_counter = 0;
    ktx->jumlah_animasi = 0;
    ktx->jumlah_render = 0;
    ktx->jumlah_klip = 0;
    ktx->piksel_diproses = 0;

    /* Inisialisasi region klip global */
    penata_region_buat(&ktx->region_klip_global);

    /* Set region klip global ke ukuran permukaan target */
    {
        penata_rect_t layar;
        layar.x = 0;
        layar.y = 0;
        layar.lebar = permukaan_lebar(target);
        layar.tinggi = permukaan_tinggi(target);
        penata_region_tambah(&ktx->region_klip_global,
                             &layar);
    }

    ktx->diinisialisasi = BENAR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - SHUTDOWN KONTEKS
 * ===========================================================================
 *
 * Shutdown konteks penata dengan aman.
 * Menghancurkan semua lapisan dan mereset state.
 */

void penata_shutdown(penata_konteks_t *ktx)
{
    if (ktx == NULL) return;

    /* Hancurkan semua lapisan */
    penata_hancurkan_semua(ktx);

    /* Bersihkan animasi */
    kernel_memset(ktx->animasi, 0, sizeof(ktx->animasi));
    ktx->jumlah_animasi = 0;

    /* Bersihkan region klip */
    penata_region_bersihkan(&ktx->region_klip_global);

    /* Reset statistik */
    ktx->jumlah_render = 0;
    ktx->jumlah_klip = 0;
    ktx->piksel_diproses = 0;

    /* Reset state */
    ktx->permukaan_target = NULL;
    ktx->jumlah_lapisan = 0;
    ktx->id_counter = 0;
    ktx->diinisialisasi = SALAH;
    ktx->magic = 0;
}
