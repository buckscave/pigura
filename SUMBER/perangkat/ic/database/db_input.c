/*
 * PIGURA OS - DATABASE/DB_INPUT.C
 * ===============================
 * Database Input IC dari low-end hingga high-end.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../ic.h"

/*
 * ===========================================================================
 * VENDOR ID INPUT
 * ===========================================================================
 */

#define VENDOR_ID_LOGITECH    0x046D
#define VENDOR_ID_MICROSOFT   0x045E
#define VENDOR_ID_CHERRY      0x046A
#define VENDOR_ID_IBM         0x04B3

/*
 * ===========================================================================
 * FUNGSI UTAMA
 * ===========================================================================
 */

status_t ic_muat_input_database(void)
{
    ic_entri_t *entri;
    status_t hasil;
    
    /* USB HID Controllers - Generic */
    entri = ic_entri_buat("USB HID", "Generic", "USB HID Keyboard",
                          IC_KATEGORI_INPUT, IC_RENTANG_MID_LOW);
    if (entri != NULL) {
        entri->vendor_id = 0x0000;
        entri->device_id = 0x0000;
        entri->bus = IC_BUS_USB;
        hasil = ic_database_tambah(entri);
    }
    
    entri = ic_entri_buat("USB HID", "Generic", "USB HID Mouse",
                          IC_KATEGORI_INPUT, IC_RENTANG_MID_LOW);
    if (entri != NULL) {
        entri->vendor_id = 0x0000;
        entri->device_id = 0x0000;
        entri->bus = IC_BUS_USB;
        hasil = ic_database_tambah(entri);
    }
    
    /* Logitech Input Devices */
    entri = ic_entri_buat("Logitech Keyboard", "Logitech", "K120",
                          IC_KATEGORI_INPUT, IC_RENTANG_MID);
    if (entri != NULL) {
        entri->vendor_id = VENDOR_ID_LOGITECH;
        entri->device_id = 0xC31C;
        entri->bus = IC_BUS_USB;
        hasil = ic_database_tambah(entri);
    }
    
    entri = ic_entri_buat("Logitech Mouse", "Logitech", "B100",
                          IC_KATEGORI_INPUT, IC_RENTANG_MID);
    if (entri != NULL) {
        entri->vendor_id = VENDOR_ID_LOGITECH;
        entri->device_id = 0xC050;
        entri->bus = IC_BUS_USB;
        hasil = ic_database_tambah(entri);
    }
    
    /* Microsoft Input Devices */
    entri = ic_entri_buat("Microsoft Keyboard", "Microsoft", "Basic Keyboard",
                          IC_KATEGORI_INPUT, IC_RENTANG_MID);
    if (entri != NULL) {
        entri->vendor_id = VENDOR_ID_MICROSOFT;
        entri->device_id = 0x0750;
        entri->bus = IC_BUS_USB;
        hasil = ic_database_tambah(entri);
    }
    
    entri = ic_entri_buat("Microsoft Mouse", "Microsoft", "Basic Mouse",
                          IC_KATEGORI_INPUT, IC_RENTANG_MID);
    if (entri != NULL) {
        entri->vendor_id = VENDOR_ID_MICROSOFT;
        entri->device_id = 0x0040;
        entri->bus = IC_BUS_USB;
        hasil = ic_database_tambah(entri);
    }
    
    return STATUS_BERHASIL;
}
