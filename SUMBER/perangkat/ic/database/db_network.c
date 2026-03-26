/*
 * PIGURA OS - DATABASE/DB_NETWORK.C
 * =================================
 * Database Network IC dari low-end hingga high-end.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../ic.h"

/*
 * ===========================================================================
 * VENDOR ID NETWORK
 * ===========================================================================
 */

#define VENDOR_ID_REALTEK     0x10EC
#define VENDOR_ID_INTEL_NET   0x8086
#define VENDOR_ID_BROADCOM    0x14E4
#define VENDOR_ID_QUALCOMM    0x168C
#define VENDOR_ID_ATHEROS     0x1969

/*
 * ===========================================================================
 * FUNGSI TAMBAH NETWORK
 * ===========================================================================
 */

static status_t ic_db_net_tambah(tak_bertanda16_t vendor_id,
                                  tak_bertanda16_t device_id,
                                  const char *nama,
                                  const char *vendor,
                                  const char *seri,
                                  ic_rentang_t rentang,
                                  tak_bertanda32_t kecepatan_mbps)
{
    ic_entri_t *entri;
    ic_parameter_t *param;
    status_t hasil;
    
    entri = ic_buat_entri_network(vendor_id, device_id, nama, vendor, seri, rentang);
    if (entri == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    /* Set parameter kecepatan */
    param = ic_entri_cari_param(entri, "kecepatan");
    if (param != NULL) {
        param->nilai_min = kecepatan_mbps;
        param->nilai_max = kecepatan_mbps;
        param->nilai_typical = kecepatan_mbps;
        param->nilai_default = kecepatan_mbps;
    }
    
    hasil = ic_database_tambah(entri);
    return hasil;
}

/*
 * ===========================================================================
 * DATABASE REALTEK
 * ===========================================================================
 */

static status_t ic_muat_realtek_network(void)
{
    status_t hasil;
    
    /* Realtek Ethernet - Low-end */
    hasil = ic_db_net_tambah(VENDOR_ID_REALTEK, 0x8139,
                              "Realtek RTL8139", "Realtek", "RTL8139A",
                              IC_RENTANG_LOW_END, 100);
    
    hasil = ic_db_net_tambah(VENDOR_ID_REALTEK, 0x8138,
                              "Realtek RTL8139", "Realtek", "RTL8139C",
                              IC_RENTANG_LOW_END, 100);
    
    /* Realtek Ethernet - Mid-range */
    hasil = ic_db_net_tambah(VENDOR_ID_REALTEK, 0x8167,
                              "Realtek RTL8111", "Realtek", "RTL8111C",
                              IC_RENTANG_MID, 1000);
    
    hasil = ic_db_net_tambah(VENDOR_ID_REALTEK, 0x8168,
                              "Realtek RTL8111", "Realtek", "RTL8111E",
                              IC_RENTANG_MID, 1000);
    
    hasil = ic_db_net_tambah(VENDOR_ID_REALTEK, 0x8161,
                              "Realtek RTL8111", "Realtek", "RTL8111G",
                              IC_RENTANG_MID_HIGH, 1000);
    
    /* Realtek WiFi */
    hasil = ic_db_net_tambah(VENDOR_ID_REALTEK, 0x8179,
                              "Realtek RTL8188EE", "Realtek", "RTL8188EE",
                              IC_RENTANG_MID, 150);
    
    hasil = ic_db_net_tambah(VENDOR_ID_REALTEK, 0x8178,
                              "Realtek RTL8192CE", "Realtek", "RTL8192CE",
                              IC_RENTANG_MID, 300);
    
    hasil = ic_db_net_tambah(VENDOR_ID_REALTEK, 0x8812,
                              "Realtek RTL8812AU", "Realtek", "RTL8812AU",
                              IC_RENTANG_MID_HIGH, 1200);
    
    hasil = ic_db_net_tambah(VENDOR_ID_REALTEK, 0x8822,
                              "Realtek RTL8822BE", "Realtek", "RTL8822BE",
                              IC_RENTANG_MID_HIGH, 1733);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * DATABASE INTEL NETWORK
 * ===========================================================================
 */

static status_t ic_muat_intel_network(void)
{
    status_t hasil;
    
    /* Intel Ethernet - Low-end */
    hasil = ic_db_net_tambah(VENDOR_ID_INTEL_NET, 0x1229,
                              "Intel PRO/100", "Intel", "PRO/100 VE",
                              IC_RENTANG_LOW_END, 100);
    
    hasil = ic_db_net_tambah(VENDOR_ID_INTEL_NET, 0x103D,
                              "Intel PRO/1000", "Intel", "PRO/1000 GT",
                              IC_RENTANG_MID_LOW, 1000);
    
    /* Intel Ethernet - Mid-range */
    hasil = ic_db_net_tambah(VENDOR_ID_INTEL_NET, 0x10BD,
                              "Intel PRO/1000", "Intel", "PRO/1000 PT",
                              IC_RENTANG_MID, 1000);
    
    hasil = ic_db_net_tambah(VENDOR_ID_INTEL_NET, 0x10C9,
                              "Intel PRO/1000", "Intel", "PRO/1000 PT Dual",
                              IC_RENTANG_MID_HIGH, 1000);
    
    /* Intel WiFi */
    hasil = ic_db_net_tambah(VENDOR_ID_INTEL_NET, 0x0891,
                              "Intel WiFi Link 5100", "Intel", "WiFi Link 5100",
                              IC_RENTANG_MID, 300);
    
    hasil = ic_db_net_tambah(VENDOR_ID_INTEL_NET, 0x4236,
                              "Intel WiFi Link 5300", "Intel", "WiFi Link 5300",
                              IC_RENTANG_MID_HIGH, 450);
    
    hasil = ic_db_net_tambah(VENDOR_ID_INTEL_NET, 0x0886,
                              "Intel WiFi Link 6200", "Intel", "WiFi Link 6200",
                              IC_RENTANG_MID_HIGH, 300);
    
    hasil = ic_db_net_tambah(VENDOR_ID_INTEL_NET, 0x0892,
                              "Intel WiFi Link 6300", "Intel", "WiFi Link 6300",
                              IC_RENTANG_MID_HIGH, 450);
    
    hasil = ic_db_net_tambah(VENDOR_ID_INTEL_NET, 0x0894,
                              "Intel WiFi Link 6250", "Intel", "WiFi Link 6250",
                              IC_RENTANG_MID_HIGH, 300);
    
    hasil = ic_db_net_tambah(VENDOR_ID_INTEL_NET, 0x0091,
                              "Intel WiFi Link 5100AGN", "Intel", "5100AGN",
                              IC_RENTANG_MID, 300);
    
    /* Intel WiFi - High-end */
    hasil = ic_db_net_tambah(VENDOR_ID_INTEL_NET, 0x08B1,
                              "Intel WiFi Link 7260", "Intel", "WiFi Link 7260",
                              IC_RENTANG_HIGH_END, 867);
    
    hasil = ic_db_net_tambah(VENDOR_ID_INTEL_NET, 0x24F3,
                              "Intel WiFi Link 8260", "Intel", "WiFi Link 8260",
                              IC_RENTANG_HIGH_END, 867);
    
    hasil = ic_db_net_tambah(VENDOR_ID_INTEL_NET, 0x24FD,
                              "Intel WiFi Link 8265", "Intel", "WiFi Link 8265",
                              IC_RENTANG_HIGH_END, 867);
    
    hasil = ic_db_net_tambah(VENDOR_ID_INTEL_NET, 0x2526,
                              "Intel WiFi Link 9260", "Intel", "WiFi Link 9260",
                              IC_RENTANG_HIGH_END, 1733);
    
    hasil = ic_db_net_tambah(VENDOR_ID_INTEL_NET, 0x2725,
                              "Intel WiFi Link AX200", "Intel", "WiFi 6 AX200",
                              IC_RENTANG_HIGH_END, 2400);
    
    hasil = ic_db_net_tambah(VENDOR_ID_INTEL_NET, 0x2723,
                              "Intel WiFi Link AX201", "Intel", "WiFi 6 AX201",
                              IC_RENTANG_HIGH_END, 2400);
    
    hasil = ic_db_net_tambah(VENDOR_ID_INTEL_NET, 0x7F70,
                              "Intel WiFi Link AX210", "Intel", "WiFi 6E AX210",
                              IC_RENTANG_ENTHUSIAST, 2400);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * DATABASE BROADCOM
 * ===========================================================================
 */

static status_t ic_muat_broadcom_network(void)
{
    status_t hasil;
    
    hasil = ic_db_net_tambah(VENDOR_ID_BROADCOM, 0x1677,
                              "Broadcom BCM5751", "Broadcom", "BCM5751",
                              IC_RENTANG_MID, 1000);
    
    hasil = ic_db_net_tambah(VENDOR_ID_BROADCOM, 0x1659,
                              "Broadcom BCM5721", "Broadcom", "BCM5721",
                              IC_RENTANG_MID, 1000);
    
    hasil = ic_db_net_tambah(VENDOR_ID_BROADCOM, 0x16B4,
                              "Broadcom BCM57416", "Broadcom", "BCM57416",
                              IC_RENTANG_HIGH_END, 10000);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI UTAMA
 * ===========================================================================
 */

status_t ic_muat_network_database(void)
{
    ic_muat_realtek_network();
    ic_muat_intel_network();
    ic_muat_broadcom_network();
    
    return STATUS_BERHASIL;
}
