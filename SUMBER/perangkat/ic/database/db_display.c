/*
 * PIGURA OS - DATABASE/DB_DISPLAY.C
 * ==================================
 * Database Display IC dari low-end hingga high-end.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../ic.h"

/*
 * ===========================================================================
 * FUNGSI UTAMA
 * ===========================================================================
 */

status_t ic_muat_display_database(void)
{
    ic_entri_t *entri;
    status_t hasil;
    
    /* VGA Compatible Controllers terdeteksi sebagai GPU */
    /* Lihat db_gpu.c untuk database lengkap GPU */
    
    /* Standard VGA */
    entri = ic_entri_buat("Standard VGA", "Generic", "VGA Compatible",
                          IC_KATEGORI_DISPLAY, IC_RENTANG_LOW_END);
    if (entri != NULL) {
        entri->vendor_id = 0x0000;
        entri->device_id = 0x0000;
        entri->bus = IC_BUS_PCI;
        hasil = ic_database_tambah(entri);
    }
    
    return STATUS_BERHASIL;
}
