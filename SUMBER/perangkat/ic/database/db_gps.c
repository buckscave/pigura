/*
 * PIGURA OS - DATABASE/DB_GPS.C
 * ==============================
 * Database GPS Modules
 *
 * Versi: 1.0
 */

#include "../ic.h"

status_t ic_muat_gps_database(void)
{
    ic_entri_t *entri;
    status_t hasil;
    
    /* u-blox GPS Modules */
    entri = ic_entri_buat("u-blox NEO-M9N", "u-blox", "NEO-M9N", IC_KATEGORI_GPS, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x1546; entri->device_id = 0x01A9; entri->bus = IC_BUS_UART; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("u-blox NEO-M8U", "u-blox", "NEO-M8U", IC_KATEGORI_GPS, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1546; entri->device_id = 0x01A8; entri->bus = IC_BUS_UART; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("u-blox NEO-M8T", "u-blox", "NEO-M8T", IC_KATEGORI_GPS, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1546; entri->device_id = 0x01A7; entri->bus = IC_BUS_UART; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("u-blox NEO-M8N", "u-blox", "NEO-M8N", IC_KATEGORI_GPS, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1546; entri->device_id = 0x01A2; entri->bus = IC_BUS_UART; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("u-blox MAX-M10S", "u-blox", "MAX-M10S", IC_KATEGORI_GPS, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x1546; entri->device_id = 0x01B2; entri->bus = IC_BUS_UART; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("u-blox ZED-F9P", "u-blox", "ZED-F9P", IC_KATEGORI_GPS, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x1546; entri->device_id = 0x01B8; entri->bus = IC_BUS_UART; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("u-blox ZED-F9R", "u-blox", "ZED-F9R", IC_KATEGORI_GPS, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x1546; entri->device_id = 0x01B9; entri->bus = IC_BUS_UART; hasil = ic_database_tambah(entri); }
    
    /* Quectel GPS Modules */
    entri = ic_entri_buat("Quectel L86", "Quectel", "L86-MT", IC_KATEGORI_GPS, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x2C7C; entri->device_id = 0x0086; entri->bus = IC_BUS_UART; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Quectel L89", "Quectel", "L89", IC_KATEGORI_GPS, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x2C7C; entri->device_id = 0x0089; entri->bus = IC_BUS_UART; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Quectel LC29H", "Quectel", "LC29H", IC_KATEGORI_GPS, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x2C7C; entri->device_id = 0x2900; entri->bus = IC_BUS_UART; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Quectel LG290P", "Quectel", "LG290P", IC_KATEGORI_GPS, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x2C7C; entri->device_id = 0x2901; entri->bus = IC_BUS_UART; hasil = ic_database_tambah(entri); }
    
    /* SiRF/CSR GPS */
    entri = ic_entri_buat("SiRFstar IV", "CSR", "SiRFstar IV GSD4t", IC_KATEGORI_GPS, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x0E0E; entri->device_id = 0x9200; entri->bus = IC_BUS_UART; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("SiRFstar V", "CSR", "SiRFstar V 5e", IC_KATEGORI_GPS, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x0E0E; entri->device_id = 0x9300; entri->bus = IC_BUS_UART; hasil = ic_database_tambah(entri); }
    
    /* MediaTek GPS */
    entri = ic_entri_buat("MediaTek MT3339", "MediaTek", "MT3339", IC_KATEGORI_GPS, IC_RENTANG_LOW_END);
    if (entri) { entri->vendor_id = 0x14C3; entri->device_id = 0x3339; entri->bus = IC_BUS_UART; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("MediaTek MT3333", "MediaTek", "MT3333", IC_KATEGORI_GPS, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x14C3; entri->device_id = 0x3333; entri->bus = IC_BUS_UART; hasil = ic_database_tambah(entri); }
    
    /* ST GPS */
    entri = ic_entri_buat("ST Teseo III", "ST", "STA8088FG", IC_KATEGORI_GPS, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0x8088; entri->bus = IC_BUS_UART; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("ST Teseo V", "ST", "TESEO V", IC_KATEGORI_GPS, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0x8090; entri->bus = IC_BUS_UART; hasil = ic_database_tambah(entri); }
    
    /* Sony GPS */
    entri = ic_entri_buat("Sony CXD5603GF", "Sony", "CXD5603GF", IC_KATEGORI_GPS, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x054C; entri->device_id = 0x5603; entri->bus = IC_BUS_SPI; hasil = ic_database_tambah(entri); }
    
    /* Generic USB GPS */
    entri = ic_entri_buat("Generic USB GPS", "Generic", "USB GPS NMEA", IC_KATEGORI_GPS, IC_RENTANG_LOW_END);
    if (entri) { entri->vendor_id = 0x0000; entri->device_id = 0x0000; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    return STATUS_BERHASIL;
}
