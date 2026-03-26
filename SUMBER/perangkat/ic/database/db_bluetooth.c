/*
 * PIGURA OS - DATABASE/DB_BLUETOOTH.C
 * ===================================
 * Database Bluetooth Controllers
 *
 * Versi: 1.0
 */

#include "../ic.h"

#define BT_VENDOR_INTEL      0x8087
#define BT_VENDOR_REALTEK    0x0BDA
#define BT_VENDOR_BROADCOM   0x0A5C
#define BT_VENDOR_QUALCOMM   0x0CF3
#define BT_VENDOR_MEDIATEK   0x13D3
#define BT_VENDOR_ATHEROS    0x0489
#define BT_VENDOR_NVIDIA     0x0955
#define BT_VENDOR_QCA        0x0CF3

status_t ic_muat_bluetooth_database(void)
{
    ic_entri_t *entri;
    status_t hasil;
    
    /* Intel Bluetooth */
    entri = ic_entri_buat("Intel AX211 Bluetooth", "Intel", "AX211", IC_KATEGORI_BLUETOOTH, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = BT_VENDOR_INTEL; entri->device_id = 0x0032; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Intel AX210 Bluetooth", "Intel", "AX210", IC_KATEGORI_BLUETOOTH, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = BT_VENDOR_INTEL; entri->device_id = 0x0032; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Intel AX201 Bluetooth", "Intel", "AX201", IC_KATEGORI_BLUETOOTH, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = BT_VENDOR_INTEL; entri->device_id = 0x0026; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Intel AX200 Bluetooth", "Intel", "AX200", IC_KATEGORI_BLUETOOTH, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = BT_VENDOR_INTEL; entri->device_id = 0x0029; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Intel AX101 Bluetooth", "Intel", "AX101", IC_KATEGORI_BLUETOOTH, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = BT_VENDOR_INTEL; entri->device_id = 0x0033; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Intel 9560 Bluetooth", "Intel", "9560", IC_KATEGORI_BLUETOOTH, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = BT_VENDOR_INTEL; entri->device_id = 0x0AAA; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Intel 9260 Bluetooth", "Intel", "9260", IC_KATEGORI_BLUETOOTH, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = BT_VENDOR_INTEL; entri->device_id = 0x0025; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Intel 8265 Bluetooth", "Intel", "8265", IC_KATEGORI_BLUETOOTH, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = BT_VENDOR_INTEL; entri->device_id = 0x0A2B; hasil = ic_database_tambah(entri); }
    
    /* Realtek Bluetooth */
    entri = ic_entri_buat("Realtek RTL8852AE Bluetooth", "Realtek", "RTL8852AE", IC_KATEGORI_BLUETOOTH, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = BT_VENDOR_REALTEK; entri->device_id = 0x8852; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Realtek RTL8822BE Bluetooth", "Realtek", "RTL8822BE", IC_KATEGORI_BLUETOOTH, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = BT_VENDOR_REALTEK; entri->device_id = 0xB82C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Realtek RTL8821A Bluetooth", "Realtek", "RTL8821A", IC_KATEGORI_BLUETOOTH, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = BT_VENDOR_REALTEK; entri->device_id = 0x8821; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Realtek RTL8761B Bluetooth", "Realtek", "RTL8761B", IC_KATEGORI_BLUETOOTH, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = BT_VENDOR_REALTEK; entri->device_id = 0x8771; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Realtek RTL8723D Bluetooth", "Realtek", "RTL8723D", IC_KATEGORI_BLUETOOTH, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = BT_VENDOR_REALTEK; entri->device_id = 0xD723; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Realtek RTL8723B Bluetooth", "Realtek", "RTL8723B", IC_KATEGORI_BLUETOOTH, IC_RENTANG_LOW_END);
    if (entri) { entri->vendor_id = BT_VENDOR_REALTEK; entri->device_id = 0xB72A; hasil = ic_database_tambah(entri); }
    
    /* Broadcom Bluetooth */
    entri = ic_entri_buat("Broadcom BCM4389 Bluetooth", "Broadcom", "BCM4389", IC_KATEGORI_BLUETOOTH, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = BT_VENDOR_BROADCOM; entri->device_id = 0x4389; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Broadcom BCM4377 Bluetooth", "Broadcom", "BCM4377", IC_KATEGORI_BLUETOOTH, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = BT_VENDOR_BROADCOM; entri->device_id = 0x4377; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Broadcom BCM4356 Bluetooth", "Broadcom", "BCM4356", IC_KATEGORI_BLUETOOTH, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = BT_VENDOR_BROADCOM; entri->device_id = 0x2404; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Broadcom BCM20702A0 Bluetooth", "Broadcom", "BCM20702A0", IC_KATEGORI_BLUETOOTH, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = BT_VENDOR_BROADCOM; entri->device_id = 0x21D3; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Broadcom BCM2045 Bluetooth", "Broadcom", "BCM2045", IC_KATEGORI_BLUETOOTH, IC_RENTANG_LOW_END);
    if (entri) { entri->vendor_id = BT_VENDOR_BROADCOM; entri->device_id = 0x2045; hasil = ic_database_tambah(entri); }
    
    /* Qualcomm/Atheros Bluetooth */
    entri = ic_entri_buat("Qualcomm QCA6390 Bluetooth", "Qualcomm", "QCA6390", IC_KATEGORI_BLUETOOTH, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = BT_VENDOR_QUALCOMM; entri->device_id = 0x6390; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Qualcomm QCA6174 Bluetooth", "Qualcomm", "QCA6174", IC_KATEGORI_BLUETOOTH, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = BT_VENDOR_QUALCOMM; entri->device_id = 0x003E; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Qualcomm WCN6855 Bluetooth", "Qualcomm", "WCN6855", IC_KATEGORI_BLUETOOTH, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = BT_VENDOR_QUALCOMM; entri->device_id = 0xE303; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Qualcomm WCN3990 Bluetooth", "Qualcomm", "WCN3990", IC_KATEGORI_BLUETOOTH, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = BT_VENDOR_QUALCOMM; entri->device_id = 0x0316; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Atheros AR9462 Bluetooth", "Qualcomm Atheros", "AR9462", IC_KATEGORI_BLUETOOTH, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = BT_VENDOR_ATHEROS; entri->device_id = 0xE042; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Atheros AR3012 Bluetooth", "Qualcomm Atheros", "AR3012", IC_KATEGORI_BLUETOOTH, IC_RENTANG_LOW_END);
    if (entri) { entri->vendor_id = BT_VENDOR_ATHEROS; entri->device_id = 0x3004; hasil = ic_database_tambah(entri); }
    
    /* MediaTek Bluetooth */
    entri = ic_entri_buat("MediaTek MT7922 Bluetooth", "MediaTek", "MT7922", IC_KATEGORI_BLUETOOTH, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = BT_VENDOR_MEDIATEK; entri->device_id = 0x7922; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("MediaTek MT7921 Bluetooth", "MediaTek", "MT7921", IC_KATEGORI_BLUETOOTH, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = BT_VENDOR_MEDIATEK; entri->device_id = 0x7921; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("MediaTek MT7663 Bluetooth", "MediaTek", "MT7663", IC_KATEGORI_BLUETOOTH, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = BT_VENDOR_MEDIATEK; entri->device_id = 0x7663; hasil = ic_database_tambah(entri); }
    
    /* Generic Bluetooth */
    entri = ic_entri_buat("Generic Bluetooth HCI", "Generic", "USB HCI", IC_KATEGORI_BLUETOOTH, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x0000; entri->device_id = 0x0001; hasil = ic_database_tambah(entri); }
    
    return STATUS_BERHASIL;
}
