/*
 * PIGURA OS - DATABASE/DB_HDMI.C
 * ===============================
 * Database HDMI Controllers
 *
 * Versi: 1.0
 */

#include "../ic.h"

status_t ic_muat_hdmi_database(void)
{
    ic_entri_t *entri;
    status_t hasil;
    
    /* NXP HDMI Transmitters */
    entri = ic_entri_buat("NXP TDA9989", "NXP", "TDA9989", IC_KATEGORI_HDMI, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x1131; entri->device_id = 0x9989; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("NXP TDA9950", "NXP", "TDA9950", IC_KATEGORI_HDMI, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1131; entri->device_id = 0x9950; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("NXP TDA19971", "NXP", "TDA19971", IC_KATEGORI_HDMI, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x1131; entri->device_id = 0x9971; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    /* Silicon Image HDMI Transmitters */
    entri = ic_entri_buat("Silicon Image SiI9022A", "Silicon Image", "SiI9022A", IC_KATEGORI_HDMI, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1095; entri->device_id = 0x9022; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Silicon Image SiI9233", "Silicon Image", "SiI9233A", IC_KATEGORI_HDMI, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x1095; entri->device_id = 0x9233; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Silicon Image SiI9133", "Silicon Image", "SiI9133", IC_KATEGORI_HDMI, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x1095; entri->device_id = 0x9133; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    /* Analog Devices HDMI */
    entri = ic_entri_buat("ADI ADV7511", "ADI", "ADV7511", IC_KATEGORI_HDMI, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x00AD; entri->device_id = 0x7511; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("ADI ADV7513", "ADI", "ADV7513", IC_KATEGORI_HDMI, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x00AD; entri->device_id = 0x7513; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("ADI ADV7611", "ADI", "ADV7611", IC_KATEGORI_HDMI, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x00AD; entri->device_id = 0x7611; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("ADI ADV7612", "ADI", "ADV7612", IC_KATEGORI_HDMI, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x00AD; entri->device_id = 0x7612; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("ADI ADV7842", "ADI", "ADV7842", IC_KATEGORI_HDMI, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x00AD; entri->device_id = 0x7842; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    /* Texas Instruments HDMI */
    entri = ic_entri_buat("TI TFP410", "TI", "TFP410", IC_KATEGORI_HDMI, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0x4100; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("TI TFP501", "TI", "TFP501", IC_KATEGORI_HDMI, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0x501; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("TI SN65DSI86", "TI", "SN65DSI86", IC_KATEGORI_HDMI, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0x8686; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("TI TMDS351", "TI", "TMDS351", IC_KATEGORI_HDMI, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0x351; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    /* ITE HDMI Controllers */
    entri = ic_entri_buat("ITE IT66121", "ITE", "IT66121", IC_KATEGORI_HDMI, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0x6121; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    /* Parade Technologies HDMI */
    entri = ic_entri_buat("Parade PS8625", "Parade", "PS8625", IC_KATEGORI_HDMI, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1D9B; entri->device_id = 0x8625; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Parade PS8640", "Parade", "PS8640", IC_KATEGORI_HDMI, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x1D9B; entri->device_id = 0x8640; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    /* Microchip HDMI */
    entri = ic_entri_buat("Microchip TC358743", "Microchip", "TC358743XBG", IC_KATEGORI_HDMI, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x1095; entri->device_id = 0x8743; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Microchip TC358840", "Microchip", "TC358840XBG", IC_KATEGORI_HDMI, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x1095; entri->device_id = 0x8840; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    /* Intel HDMI */
    entri = ic_entri_buat("Intel HDMI Audio", "Intel", "HDMI Audio", IC_KATEGORI_HDMI, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x8086; entri->device_id = 0x004F; entri->bus = IC_BUS_PCI; hasil = ic_database_tambah(entri); }
    
    /* AMD HDMI */
    entri = ic_entri_buat("AMD HDMI Audio", "AMD", "HDMI Audio", IC_KATEGORI_HDMI, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1002; entri->device_id = 0xAA01; entri->bus = IC_BUS_PCI; hasil = ic_database_tambah(entri); }
    
    return STATUS_BERHASIL;
}
