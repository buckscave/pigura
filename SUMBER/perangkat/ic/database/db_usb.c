/*
 * PIGURA OS - DATABASE/DB_USB.C
 * =============================
 * Database USB Controller IC dari low-end hingga high-end.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../ic.h"

/*
 * ===========================================================================
 * VENDOR ID USB
 * ===========================================================================
 */

#define VENDOR_ID_INTEL_USB   0x8086
#define VENDOR_ID_AMD_USB     0x1022
#define VENDOR_ID_NEC_USB     0x1033
#define VENDOR_ID_VIA_USB     0x1106
#define VENDOR_ID_RENESAS_USB 0x1912

/*
 * ===========================================================================
 * FUNGSI UTAMA
 * ===========================================================================
 */

status_t ic_muat_usb_database(void)
{
    ic_entri_t *entri;
    status_t hasil;
    
    /* Intel UHCI - USB 1.1 */
    entri = ic_entri_buat("Intel UHCI", "Intel", "UHCI USB 1.1",
                          IC_KATEGORI_USB, IC_RENTANG_LOW_END);
    if (entri != NULL) {
        entri->vendor_id = VENDOR_ID_INTEL_USB;
        entri->device_id = 0x24D2;
        entri->bus = IC_BUS_PCI;
        hasil = ic_database_tambah(entri);
    }
    
    /* Intel EHCI - USB 2.0 */
    entri = ic_entri_buat("Intel EHCI", "Intel", "EHCI USB 2.0",
                          IC_KATEGORI_USB, IC_RENTANG_MID);
    if (entri != NULL) {
        entri->vendor_id = VENDOR_ID_INTEL_USB;
        entri->device_id = 0x24DD;
        entri->bus = IC_BUS_PCI;
        hasil = ic_database_tambah(entri);
    }
    
    /* Intel xHCI - USB 3.0 */
    entri = ic_entri_buat("Intel xHCI", "Intel", "xHCI USB 3.0",
                          IC_KATEGORI_USB, IC_RENTANG_MID_HIGH);
    if (entri != NULL) {
        entri->vendor_id = VENDOR_ID_INTEL_USB;
        entri->device_id = 0x1E31;
        entri->bus = IC_BUS_PCI;
        hasil = ic_database_tambah(entri);
    }
    
    /* Intel xHCI - Series 7 */
    entri = ic_entri_buat("Intel xHCI", "Intel", "Series 7 xHCI",
                          IC_KATEGORI_USB, IC_RENTANG_MID_HIGH);
    if (entri != NULL) {
        entri->vendor_id = VENDOR_ID_INTEL_USB;
        entri->device_id = 0x8C31;
        entri->bus = IC_BUS_PCI;
        hasil = ic_database_tambah(entri);
    }
    
    /* Intel xHCI - Series 100 */
    entri = ic_entri_buat("Intel xHCI", "Intel", "Series 100 xHCI",
                          IC_KATEGORI_USB, IC_RENTANG_HIGH_END);
    if (entri != NULL) {
        entri->vendor_id = VENDOR_ID_INTEL_USB;
        entri->device_id = 0xA12F;
        entri->bus = IC_BUS_PCI;
        hasil = ic_database_tambah(entri);
    }
    
    /* Intel xHCI - Series 200 */
    entri = ic_entri_buat("Intel xHCI", "Intel", "Series 200 xHCI",
                          IC_KATEGORI_USB, IC_RENTANG_HIGH_END);
    if (entri != NULL) {
        entri->vendor_id = VENDOR_ID_INTEL_USB;
        entri->device_id = 0xA2AF;
        entri->bus = IC_BUS_PCI;
        hasil = ic_database_tambah(entri);
    }
    
    /* AMD xHCI */
    entri = ic_entri_buat("AMD xHCI", "AMD", "xHCI USB 3.0",
                          IC_KATEGORI_USB, IC_RENTANG_MID_HIGH);
    if (entri != NULL) {
        entri->vendor_id = VENDOR_ID_AMD_USB;
        entri->device_id = 0x145C;
        entri->bus = IC_BUS_PCI;
        hasil = ic_database_tambah(entri);
    }
    
    /* NEC xHCI */
    entri = ic_entri_buat("NEC xHCI", "NEC", "uPD720200",
                          IC_KATEGORI_USB, IC_RENTANG_MID);
    if (entri != NULL) {
        entri->vendor_id = VENDOR_ID_NEC_USB;
        entri->device_id = 0x0194;
        entri->bus = IC_BUS_PCI;
        hasil = ic_database_tambah(entri);
    }
    
    /* Renesas xHCI */
    entri = ic_entri_buat("Renesas xHCI", "Renesas", "uPD720201",
                          IC_KATEGORI_USB, IC_RENTANG_MID_HIGH);
    if (entri != NULL) {
        entri->vendor_id = VENDOR_ID_RENESAS_USB;
        entri->device_id = 0x0014;
        entri->bus = IC_BUS_PCI;
        hasil = ic_database_tambah(entri);
    }
    
    return STATUS_BERHASIL;
}
