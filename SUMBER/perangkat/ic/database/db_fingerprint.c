/*
 * PIGURA OS - DATABASE/DB_FINGERPRINT.C
 * ======================================
 * Database Fingerprint Sensors
 *
 * Versi: 1.0
 */

#include "../ic.h"

status_t ic_muat_fingerprint_database(void)
{
    ic_entri_t *entri;
    status_t hasil;
    
    /* Goodix Fingerprint Sensors */
    entri = ic_entri_buat("Goodix GF516", "Goodix", "GF516", IC_KATEGORI_FINGERPRINT, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x27C6; entri->device_id = 0x0516; entri->bus = IC_BUS_SPI; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Goodix GF5288", "Goodix", "GF5288", IC_KATEGORI_FINGERPRINT, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x27C6; entri->device_id = 0x5288; entri->bus = IC_BUS_SPI; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Goodix GF3208", "Goodix", "GF3208", IC_KATEGORI_FINGERPRINT, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x27C6; entri->device_id = 0x3208; entri->bus = IC_BUS_SPI; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Goodix GF368", "Goodix", "GF368", IC_KATEGORI_FINGERPRINT, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x27C6; entri->device_id = 0x0368; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    /* Synaptics Fingerprint Sensors */
    entri = ic_entri_buat("Synaptics FS7604", "Synaptics", "FS7604", IC_KATEGORI_FINGERPRINT, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x06CB; entri->device_id = 0x7604; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Synaptics FS7600", "Synaptics", "FS7600", IC_KATEGORI_FINGERPRINT, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x06CB; entri->device_id = 0x7600; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Synaptics Prometheus", "Synaptics", "Prometheus", IC_KATEGORI_FINGERPRINT, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x06CB; entri->device_id = 0x00BD; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    /* Elan Fingerprint Sensors */
    entri = ic_entri_buat("ELAN ELAN9020", "ELAN", "ELAN9020", IC_KATEGORI_FINGERPRINT, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x04F3; entri->device_id = 0x0C29; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("ELAN ELAN1010", "ELAN", "ELAN1010", IC_KATEGORI_FINGERPRINT, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x04F3; entri->device_id = 0x090F; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    /* Validity/Synaptics Sensors */
    entri = ic_entri_buat("Validity VFS7500", "Validity", "VFS7500", IC_KATEGORI_FINGERPRINT, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x138A; entri->device_id = 0x0018; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Validity VFS495", "Validity", "VFS495", IC_KATEGORI_FINGERPRINT, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x138A; entri->device_id = 0x0016; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    /* FPC (Fingerprint Cards) Sensors */
    entri = ic_entri_buat("FPC FPC1025", "FPC", "FPC1025", IC_KATEGORI_FINGERPRINT, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x10A5; entri->device_id = 0x9200; entri->bus = IC_BUS_SPI; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("FPC FPC1155", "FPC", "FPC1155", IC_KATEGORI_FINGERPRINT, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x10A5; entri->device_id = 0x1155; entri->bus = IC_BUS_SPI; hasil = ic_database_tambah(entri); }
    
    /* Egis Sensors */
    entri = ic_entri_buat("Egis ES603", "Egis", "ES603", IC_KATEGORI_FINGERPRINT, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x1C7A; entri->device_id = 0x0603; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Egis ES620", "Egis", "ES620", IC_KATEGORI_FINGERPRINT, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1C7A; entri->device_id = 0x0570; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    return STATUS_BERHASIL;
}
