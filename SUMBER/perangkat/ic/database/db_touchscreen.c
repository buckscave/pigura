/*
 * PIGURA OS - DATABASE/DB_TOUCHSCREEN.C
 * =====================================
 * Database Touchscreen Panel Controllers
 *
 * Versi: 1.0
 */

#include "../ic.h"

#define VENDOR_GOODIX       0x27C6
#define VENDOR_FOCALTECH    0x2808
#define VENDOR_SYNAPTICS    0x06CB
#define VENDOR_ELAN         0x04F3
#define VENDOR_CYPRESS      0x04B4
#define VENDOR_ILITEK       0x2575
#define VENDOR_NOVATEK      0x2058
#define VENDOR_WEIDA        0x2575
#define VENDOR_HIMAX        0x2274

status_t ic_muat_touchscreen_database(void)
{
    ic_entri_t *entri;
    status_t hasil;
    
    /* Goodix Touch Controllers */
    entri = ic_entri_buat("Goodix GT911", "Goodix", "GT911", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = VENDOR_GOODIX; entri->device_id = 0x1011; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Goodix GT915", "Goodix", "GT915", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = VENDOR_GOODIX; entri->device_id = 0x1015; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Goodix GT9271", "Goodix", "GT9271", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = VENDOR_GOODIX; entri->device_id = 0x9271; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Goodix GT928", "Goodix", "GT928", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = VENDOR_GOODIX; entri->device_id = 0x0928; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Goodix GT1151", "Goodix", "GT1151", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = VENDOR_GOODIX; entri->device_id = 0x1151; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Goodix GT7375", "Goodix", "GT7375", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = VENDOR_GOODIX; entri->device_id = 0x7375; hasil = ic_database_tambah(entri); }
    
    /* FocalTech Touch Controllers */
    entri = ic_entri_buat("FocalTech FT5316", "FocalTech", "FT5316", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = VENDOR_FOCALTECH; entri->device_id = 0x5316; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("FocalTech FT5406", "FocalTech", "FT5406", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = VENDOR_FOCALTECH; entri->device_id = 0x5406; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("FocalTech FT6236", "FocalTech", "FT6236", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = VENDOR_FOCALTECH; entri->device_id = 0x6236; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("FocalTech FT5452", "FocalTech", "FT5452", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = VENDOR_FOCALTECH; entri->device_id = 0x5452; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("FocalTech FT8719", "FocalTech", "FT8719", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = VENDOR_FOCALTECH; entri->device_id = 0x8719; hasil = ic_database_tambah(entri); }
    
    /* Synaptics Touch Controllers */
    entri = ic_entri_buat("Synaptics RMI4", "Synaptics", "RMI4", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = VENDOR_SYNAPTICS; entri->device_id = 0x1001; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Synaptics S3320", "Synaptics", "S3320", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = VENDOR_SYNAPTICS; entri->device_id = 0x3320; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Synaptics DS4", "Synaptics", "DS4", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = VENDOR_SYNAPTICS; entri->device_id = 0x0044; hasil = ic_database_tambah(entri); }
    
    /* ELAN Touch Controllers */
    entri = ic_entri_buat("ELAN eKTH3000", "ELAN", "eKTH3000", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = VENDOR_ELAN; entri->device_id = 0x3000; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("ELAN eKTH6312C", "ELAN", "eKTH6312C", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = VENDOR_ELAN; entri->device_id = 0x6312; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("ELAN eKTF3624", "ELAN", "eKTF3624", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = VENDOR_ELAN; entri->device_id = 0x3624; hasil = ic_database_tambah(entri); }
    
    /* Cypress Touch Controllers */
    entri = ic_entri_buat("Cypress TrueTouch", "Cypress", "TrueTouch", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = VENDOR_CYPRESS; entri->device_id = 0x1000; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Cypress CYTTSP5", "Cypress", "CYTTSP5", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = VENDOR_CYPRESS; entri->device_id = 0x5000; hasil = ic_database_tambah(entri); }
    
    /* ILI Touch Controllers */
    entri = ic_entri_buat("Ilitek ILI2132", "Ilitek", "ILI2132", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = VENDOR_ILITEK; entri->device_id = 0x2132; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Ilitek ILI2117", "Ilitek", "ILI2117", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = VENDOR_ILITEK; entri->device_id = 0x2117; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Ilitek ILI2316", "Ilitek", "ILI2316", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = VENDOR_ILITEK; entri->device_id = 0x2316; hasil = ic_database_tambah(entri); }
    
    /* Novatek Touch Controllers */
    entri = ic_entri_buat("Novatek NT36672A", "Novatek", "NT36672A", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = VENDOR_NOVATEK; entri->device_id = 0x6672; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Novatek NT36675", "Novatek", "NT36675", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = VENDOR_NOVATEK; entri->device_id = 0x6675; hasil = ic_database_tambah(entri); }
    
    /* Himax Touch Controllers */
    entri = ic_entri_buat("Himax HX83112A", "Himax", "HX83112A", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = VENDOR_HIMAX; entri->device_id = 0x8312; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Himax HX8527", "Himax", "HX8527", IC_KATEGORI_TOUCHSCREEN, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = VENDOR_HIMAX; entri->device_id = 0x8527; hasil = ic_database_tambah(entri); }
    
    return STATUS_BERHASIL;
}
