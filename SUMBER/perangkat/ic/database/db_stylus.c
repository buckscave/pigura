/*
 * PIGURA OS - DATABASE/DB_STYLUS.C
 * ================================
 * Database Stylus/Pen Tablet Controllers
 *
 * Versi: 1.0
 */

#include "../ic.h"

status_t ic_muat_stylus_database(void)
{
    ic_entri_t *entri;
    status_t hasil;
    
    /* Wacom Tablets */
    entri = ic_entri_buat("Wacom Intuos Pro Large", "Wacom", "PTH-860", IC_KATEGORI_STYLUS, IC_RENTANG_ENTHUSIAST);
    if (entri) { entri->vendor_id = 0x056A; entri->device_id = 0x0367; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Wacom Intuos Pro Medium", "Wacom", "PTH-660", IC_KATEGORI_STYLUS, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x056A; entri->device_id = 0x0357; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Wacom Intuos Pro Small", "Wacom", "PTH-460", IC_KATEGORI_STYLUS, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x056A; entri->device_id = 0x0358; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Wacom Intuos S", "Wacom", "CTL-4100", IC_KATEGORI_STYLUS, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x056A; entri->device_id = 0x0374; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Wacom Intuos S BT", "Wacom", "CTL-4100WL", IC_KATEGORI_STYLUS, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x056A; entri->device_id = 0x0376; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Wacom One", "Wacom", "DTC133", IC_KATEGORI_STYLUS, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x056A; entri->device_id = 0x03A2; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Wacom Cintiq Pro 32", "Wacom", "DTH-3220", IC_KATEGORI_STYLUS, IC_RENTANG_SERVER);
    if (entri) { entri->vendor_id = 0x056A; entri->device_id = 0x0359; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Wacom Cintiq Pro 24", "Wacom", "DTH-2420", IC_KATEGORI_STYLUS, IC_RENTANG_ENTHUSIAST);
    if (entri) { entri->vendor_id = 0x056A; entri->device_id = 0x035A; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Wacom Cintiq 16", "Wacom", "DTK-1660", IC_KATEGORI_STYLUS, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x056A; entri->device_id = 0x0364; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Wacom Bamboo Slate", "Wacom", "CDP-1060", IC_KATEGORI_STYLUS, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x056A; entri->device_id = 0x0391; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    /* Huion Tablets */
    entri = ic_entri_buat("Huion Kamvas Pro 24", "Huion", "GT191", IC_KATEGORI_STYLUS, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x256C; entri->device_id = 0x006E; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Huion Kamvas 22", "Huion", "GT-221", IC_KATEGORI_STYLUS, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x256C; entri->device_id = 0x006D; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Huion Kamvas 16", "Huion", "GT-156", IC_KATEGORI_STYLUS, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x256C; entri->device_id = 0x006D; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Huion Kamvas 13", "Huion", "GT-133", IC_KATEGORI_STYLUS, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x256C; entri->device_id = 0x006D; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Huion H640P", "Huion", "H640P", IC_KATEGORI_STYLUS, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x256C; entri->device_id = 0x006D; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Huion H430P", "Huion", "H430P", IC_KATEGORI_STYLUS, IC_RENTANG_LOW_END);
    if (entri) { entri->vendor_id = 0x256C; entri->device_id = 0x006D; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Huion Inspiroy Q11K", "Huion", "Q11K", IC_KATEGORI_STYLUS, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x256C; entri->device_id = 0x0054; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    /* XP-Pen Tablets */
    entri = ic_entri_buat("XP-Pen Artist 24 Pro", "XP-Pen", "Artist 24 Pro", IC_KATEGORI_STYLUS, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x28BD; entri->device_id = 0x093A; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("XP-Pen Artist 22R Pro", "XP-Pen", "Artist 22R Pro", IC_KATEGORI_STYLUS, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x28BD; entri->device_id = 0x0928; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("XP-Pen Artist 15.6 Pro", "XP-Pen", "Artist 15.6 Pro", IC_KATEGORI_STYLUS, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x28BD; entri->device_id = 0x0909; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("XP-Pen Artist 12 Pro", "XP-Pen", "Artist 12 Pro", IC_KATEGORI_STYLUS, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x28BD; entri->device_id = 0x091A; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("XP-Pen Deco Pro Medium", "XP-Pen", "Deco Pro MW", IC_KATEGORI_STYLUS, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x28BD; entri->device_id = 0x0929; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("XP-Pen Deco 01 V2", "XP-Pen", "Deco 01 V2", IC_KATEGORI_STYLUS, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x28BD; entri->device_id = 0x0913; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("XP-Pen Star 06", "XP-Pen", "Star 06", IC_KATEGORI_STYLUS, IC_RENTANG_LOW_END);
    if (entri) { entri->vendor_id = 0x28BD; entri->device_id = 0x0054; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    /* Gaomon Tablets */
    entri = ic_entri_buat("Gaomon PD1561", "Gaomon", "PD1561", IC_KATEGORI_STYLUS, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x256C; entri->device_id = 0x0067; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Gaomon S620", "Gaomon", "S620", IC_KATEGORI_STYLUS, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x256C; entri->device_id = 0x0064; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Gaomon M10K Pro", "Gaomon", "M10K Pro", IC_KATEGORI_STYLUS, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x256C; entri->device_id = 0x0066; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    /* Apple Pencil */
    entri = ic_entri_buat("Apple Pencil 2nd Gen", "Apple", "MU8F2", IC_KATEGORI_STYLUS, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x106B; entri->device_id = 0x0001; entri->bus = IC_BUS_BLUETOOTH; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Apple Pencil 1st Gen", "Apple", "MK0C2", IC_KATEGORI_STYLUS, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x106B; entri->device_id = 0x0000; entri->bus = IC_BUS_BLUETOOTH; hasil = ic_database_tambah(entri); }
    
    /* Samsung S Pen */
    entri = ic_entri_buat("Samsung S Pen Pro", "Samsung", "S Pen Pro", IC_KATEGORI_STYLUS, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x04E8; entri->device_id = 0x0001; entri->bus = IC_BUS_BLUETOOTH; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Samsung S Pen", "Samsung", "S Pen", IC_KATEGORI_STYLUS, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x04E8; entri->device_id = 0x0000; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    return STATUS_BERHASIL;
}
