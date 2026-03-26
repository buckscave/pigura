/*
 * PIGURA OS - DATABASE/DB_STORAGE.C
 * =================================
 * Database Storage IC dari low-end hingga high-end.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../ic.h"

/*
 * ===========================================================================
 * VENDOR ID STORAGE
 * ===========================================================================
 */

#define VENDOR_ID_INTEL_STORAGE 0x8086
#define VENDOR_ID_AMD_STORAGE   0x1022
#define VENDOR_ID_SAMSUNG       0x144D
#define VENDOR_ID_SANDISK       0x15B7
#define VENDOR_ID_SKHYNIX       0x1C5F

/*
 * ===========================================================================
 * FUNGSI TAMBAH STORAGE
 * ===========================================================================
 */

static status_t ic_db_storage_tambah(tak_bertanda16_t vendor_id,
                                      tak_bertanda16_t device_id,
                                      const char *nama,
                                      const char *vendor,
                                      const char *seri,
                                      ic_rentang_t rentang,
                                      tak_bertanda32_t kapasitas_gb,
                                      tak_bertanda32_t kecepatan_mbs)
{
    ic_entri_t *entri;
    ic_parameter_t *param;
    status_t hasil;
    
    entri = ic_buat_entri_storage(vendor_id, device_id, nama, vendor, seri, rentang);
    if (entri == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    /* Set parameter kapasitas */
    param = ic_entri_cari_param(entri, "kapasitas");
    if (param != NULL) {
        param->nilai_min = (tak_bertanda64_t)kapasitas_gb * 1024;
        param->nilai_max = (tak_bertanda64_t)kapasitas_gb * 1024;
        param->nilai_typical = param->nilai_max;
        param->nilai_default = param->nilai_max;
    }
    
    /* Set parameter kecepatan */
    param = ic_entri_cari_param(entri, "kecepatan_baca");
    if (param != NULL) {
        param->nilai_min = kecepatan_mbs;
        param->nilai_max = kecepatan_mbs;
        param->nilai_typical = kecepatan_mbs;
        param->nilai_default = ic_hitung_default(kecepatan_mbs, 
                                                  IC_TOLERANSI_DEFAULT_MAX);
    }
    
    hasil = ic_database_tambah(entri);
    return hasil;
}

/*
 * ===========================================================================
 * DATABASE STORAGE CONTROLLERS
 * ===========================================================================
 */

static status_t ic_muat_storage_controllers(void)
{
    status_t hasil;
    
    /* Intel Storage Controllers */
    hasil = ic_db_storage_tambah(VENDOR_ID_INTEL_STORAGE, 0x24DB,
                                  "Intel ICH5", "Intel", "ICH5 IDE",
                                  IC_RENTANG_LOW_END, 0, 133);
    
    hasil = ic_db_storage_tambah(VENDOR_ID_INTEL_STORAGE, 0x2653,
                                  "Intel ICH6R", "Intel", "ICH6R AHCI",
                                  IC_RENTANG_MID_LOW, 0, 150);
    
    hasil = ic_db_storage_tambah(VENDOR_ID_INTEL_STORAGE, 0x27C1,
                                  "Intel ICH7", "Intel", "ICH7 AHCI",
                                  IC_RENTANG_MID, 0, 300);
    
    hasil = ic_db_storage_tambah(VENDOR_ID_INTEL_STORAGE, 0x2922,
                                  "Intel ICH9", "Intel", "ICH9 AHCI",
                                  IC_RENTANG_MID, 0, 300);
    
    hasil = ic_db_storage_tambah(VENDOR_ID_INTEL_STORAGE, 0x3A22,
                                  "Intel ICH10", "Intel", "ICH10 AHCI",
                                  IC_RENTANG_MID_HIGH, 0, 300);
    
    hasil = ic_db_storage_tambah(VENDOR_ID_INTEL_STORAGE, 0x1C02,
                                  "Intel C600", "Intel", "C600 AHCI",
                                  IC_RENTANG_MID_HIGH, 0, 600);
    
    /* Intel NVMe */
    hasil = ic_db_storage_tambah(VENDOR_ID_INTEL_STORAGE, 0x0953,
                                  "Intel NVMe", "Intel", "DC P3700",
                                  IC_RENTANG_HIGH_END, 2000, 2800);
    
    hasil = ic_db_storage_tambah(VENDOR_ID_INTEL_STORAGE, 0xF1A5,
                                  "Intel NVMe", "Intel", "600p",
                                  IC_RENTANG_MID_HIGH, 256, 1570);
    
    hasil = ic_db_storage_tambah(VENDOR_ID_INTEL_STORAGE, 0x00A0,
                                  "Intel NVMe", "Intel", "660p",
                                  IC_RENTANG_MID_HIGH, 512, 1500);
    
    hasil = ic_db_storage_tambah(VENDOR_ID_INTEL_STORAGE, 0x00A1,
                                  "Intel NVMe", "Intel", "665p",
                                  IC_RENTANG_MID_HIGH, 1024, 1700);
    
    /* Samsung NVMe */
    hasil = ic_db_storage_tambah(VENDOR_ID_SAMSUNG, 0xA800,
                                  "Samsung NVMe", "Samsung", "960 EVO",
                                  IC_RENTANG_HIGH_END, 250, 3200);
    
    hasil = ic_db_storage_tambah(VENDOR_ID_SAMSUNG, 0xA821,
                                  "Samsung NVMe", "Samsung", "960 PRO",
                                  IC_RENTANG_HIGH_END, 512, 3500);
    
    hasil = ic_db_storage_tambah(VENDOR_ID_SAMSUNG, 0xA822,
                                  "Samsung NVMe", "Samsung", "970 EVO",
                                  IC_RENTANG_HIGH_END, 250, 3500);
    
    hasil = ic_db_storage_tambah(VENDOR_ID_SAMSUNG, 0xA809,
                                  "Samsung NVMe", "Samsung", "970 EVO Plus",
                                  IC_RENTANG_HIGH_END, 250, 3500);
    
    hasil = ic_db_storage_tambah(VENDOR_ID_SAMSUNG, 0xA808,
                                  "Samsung NVMe", "Samsung", "970 PRO",
                                  IC_RENTANG_ENTHUSIAST, 512, 3500);
    
    hasil = ic_db_storage_tambah(VENDOR_ID_SAMSUNG, 0xA80D,
                                  "Samsung NVMe", "Samsung", "980 PRO",
                                  IC_RENTANG_ENTHUSIAST, 250, 7000);
    
    hasil = ic_db_storage_tambah(VENDOR_ID_SAMSUNG, 0xA80E,
                                  "Samsung NVMe", "Samsung", "990 PRO",
                                  IC_RENTANG_ENTHUSIAST, 1000, 7450);
    
    /* SK Hynix NVMe */
    hasil = ic_db_storage_tambah(VENDOR_ID_SKHYNIX, 0x174A,
                                  "SK Hynix NVMe", "SK Hynix", "PC401",
                                  IC_RENTANG_MID_HIGH, 256, 2600);
    
    hasil = ic_db_storage_tambah(VENDOR_ID_SKHYNIX, 0x13C3,
                                  "SK Hynix NVMe", "SK Hynix", "P31",
                                  IC_RENTANG_MID_HIGH, 512, 3500);
    
    hasil = ic_db_storage_tambah(VENDOR_ID_SKHYNIX, 0x14C3,
                                  "SK Hynix NVMe", "SK Hynix", "P41",
                                  IC_RENTANG_HIGH_END, 512, 7000);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI UTAMA
 * ===========================================================================
 */

status_t ic_muat_storage_database(void)
{
    ic_muat_storage_controllers();
    return STATUS_BERHASIL;
}
