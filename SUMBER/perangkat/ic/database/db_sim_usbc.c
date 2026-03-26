/*
 * PIGURA OS - DATABASE/DB_SIM_USBC.C
 * ===================================
 * Database SIM Card Readers and USB Type-C Controllers
 *
 * Versi: 1.0
 */

#include "../ic.h"

status_t ic_muat_sim_database(void)
{
    ic_entri_t *entri;
    
    /* SIM Card Controller ICs */
    entri = ic_entri_buat("NXP PN544", "NXP", "PN544", IC_KATEGORI_SIM, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x04CC; entri->device_id = 0x0544; entri->bus = IC_BUS_I2C; (void)ic_database_tambah(entri); }
    
    entri = ic_entri_buat("NXP PN65N", "NXP", "PN65N", IC_KATEGORI_SIM, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x04CC; entri->device_id = 0x0650; entri->bus = IC_BUS_I2C; (void)ic_database_tambah(entri); }
    
    entri = ic_entri_buat("NXP PN80T", "NXP", "PN80T", IC_KATEGORI_SIM, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x04CC; entri->device_id = 0x0800; entri->bus = IC_BUS_I2C; (void)ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Infineon SLB9615", "Infineon", "SLB9615", IC_KATEGORI_SIM, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x0588; entri->device_id = 0x9615; entri->bus = IC_BUS_SPI; (void)ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Infineon SLM97", "Infineon", "SLM97", IC_KATEGORI_SIM, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x0588; entri->device_id = 0x0097; entri->bus = IC_BUS_SPI; (void)ic_database_tambah(entri); }
    
    entri = ic_entri_buat("ST ST33J2M0", "ST", "ST33J2M0", IC_KATEGORI_SIM, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0x3320; entri->bus = IC_BUS_SPI; (void)ic_database_tambah(entri); }
    
    entri = ic_entri_buat("ST ST54J", "ST", "ST54J", IC_KATEGORI_SIM, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0x0540; entri->bus = IC_BUS_I2C; (void)ic_database_tambah(entri); }
    
    /* eSIM ICs */
    entri = ic_entri_buat("ST ST33J2M0 eSIM", "ST", "ST33J2M0 eUICC", IC_KATEGORI_SIM, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0xE021; entri->bus = IC_BUS_SPI; (void)ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Gemalto eUICC", "Gemalto", "eUICC", IC_KATEGORI_SIM, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x058A; entri->device_id = 0x0001; entri->bus = IC_BUS_SPI; (void)ic_database_tambah(entri); }
    
    return STATUS_BERHASIL;
}

status_t ic_muat_usbc_database(void)
{
    ic_entri_t *entri;
    
    /* USB Type-C Controller ICs */
    
    /* TI USB-C Controllers */
    entri = ic_entri_buat("TI TPS65982", "TI", "TPS65982", IC_KATEGORI_USBC, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0x5982; entri->bus = IC_BUS_I2C; (void)ic_database_tambah(entri); }
    
    entri = ic_entri_buat("TI TPS65987", "TI", "TPS65987", IC_KATEGORI_USBC, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0x5987; entri->bus = IC_BUS_I2C; (void)ic_database_tambah(entri); }
    
    entri = ic_entri_buat("TI TPS65988", "TI", "TPS65988", IC_KATEGORI_USBC, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0x5988; entri->bus = IC_BUS_I2C; (void)ic_database_tambah(entri); }
    
    entri = ic_entri_buat("TI HD3SS460", "TI", "HD3SS460", IC_KATEGORI_USBC, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0x3460; entri->bus = IC_BUS_I2C; (void)ic_database_tambah(entri); }
    
    /* NXP USB-C Controllers */
    entri = ic_entri_buat("NXP PTN5110", "NXP", "PTN5110", IC_KATEGORI_USBC, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x1131; entri->device_id = 0x5110; entri->bus = IC_BUS_I2C; (void)ic_database_tambah(entri); }
    
    /* Cypress USB-C Controllers */
    entri = ic_entri_buat("Cypress CYPD3174", "Cypress", "CYPD3174", IC_KATEGORI_USBC, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x04B4; entri->device_id = 0x3174; entri->bus = IC_BUS_I2C; (void)ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Cypress CYPD4226", "Cypress", "CYPD4226", IC_KATEGORI_USBC, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x04B4; entri->device_id = 0x4226; entri->bus = IC_BUS_I2C; (void)ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Cypress CYPD5225", "Cypress", "CYPD5225", IC_KATEGORI_USBC, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x04B4; entri->device_id = 0x5225; entri->bus = IC_BUS_I2C; (void)ic_database_tambah(entri); }
    
    /* Microchip USB-C Controllers */
    entri = ic_entri_buat("Microchip UCS2112", "Microchip", "UCS2112", IC_KATEGORI_USBC, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1095; entri->device_id = 0x2112; entri->bus = IC_BUS_I2C; (void)ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Microchip UCS2114", "Microchip", "UCS2114", IC_KATEGORI_USBC, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x1095; entri->device_id = 0x2114; entri->bus = IC_BUS_I2C; (void)ic_database_tambah(entri); }
    
    /* Synaptics USB-C Controllers */
    entri = ic_entri_buat("Synaptics SY780", "Synaptics", "SY780", IC_KATEGORI_USBC, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x06CB; entri->device_id = 0x0780; entri->bus = IC_BUS_I2C; (void)ic_database_tambah(entri); }
    
    /* Parade Technologies USB-C */
    entri = ic_entri_buat("Parade PS8743", "Parade", "PS8743", IC_KATEGORI_USBC, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x1D9B; entri->device_id = 0x8743; entri->bus = IC_BUS_I2C; (void)ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Parade PS8755", "Parade", "PS8755", IC_KATEGORI_USBC, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x1D9B; entri->device_id = 0x8755; entri->bus = IC_BUS_I2C; (void)ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Parade PS8815", "Parade", "PS8815", IC_KATEGORI_USBC, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x1D9B; entri->device_id = 0x8815; entri->bus = IC_BUS_I2C; (void)ic_database_tambah(entri); }
    
    /* VIA USB-C Controllers */
    entri = ic_entri_buat("VIA VL103", "VIA", "VL103", IC_KATEGORI_USBC, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1106; entri->device_id = 0x3103; entri->bus = IC_BUS_I2C; (void)ic_database_tambah(entri); }
    
    /* Realtek USB-C Controllers */
    entri = ic_entri_buat("Realtek RTD2142", "Realtek", "RTD2142", IC_KATEGORI_USBC, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x0BDA; entri->device_id = 0x2142; entri->bus = IC_BUS_I2C; (void)ic_database_tambah(entri); }
    
    /* ITE USB-C Controllers */
    entri = ic_entri_buat("ITE IT5205", "ITE", "IT5205", IC_KATEGORI_USBC, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0x5205; entri->bus = IC_BUS_I2C; (void)ic_database_tambah(entri); }
    
    entri = ic_entri_buat("ITE IT8893", "ITE", "IT8893", IC_KATEGORI_USBC, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0x8893; entri->bus = IC_BUS_I2C; (void)ic_database_tambah(entri); }
    
    return STATUS_BERHASIL;
}
