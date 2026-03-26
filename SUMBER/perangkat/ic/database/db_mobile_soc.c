/*
 * PIGURA OS - DATABASE/DB_MOBILE_SOC.C
 * =====================================
 * Database Mobile SoC IC dari berbagai vendor.
 *
 * Berkas ini berisi database IC SoC mobile dari:
 * - Qualcomm Snapdragon series
 * - MediaTek Dimensity dan Helio series
 * - Samsung Exynos series
 * - Apple Silicon (A-series, M-series)
 * - Huawei HiSilicon Kirin series
 * - UNISOC series
 *
 * Versi: 1.0
 * Tanggal: 2026
 */

#include "../ic.h"

/*
 * ===========================================================================
 * KONSTANTA VENDOR ID SOC
 * ===========================================================================
 */

/* Vendor ID untuk SoC (PCI-style atau custom) */
#define SOC_VENDOR_QUALCOMM     0x17CB
#define SOC_VENDOR_MEDIATEK    0x14C3
#define SOC_VENDOR_SAMSUNG     0x144D
#define SOC_VENDOR_APPLE       0x106B
#define SOC_VENDOR_HISILICON   0x19E5
#define SOC_VENDOR_UNISOC      0x1D9A
#define SOC_VENDOR_NVIDIA      0x10DE
#define SOC_VENDOR_INTEL       0x8086
#define SOC_VENDOR_AMD         0x1022
#define SOC_VENDOR_ROCKCHIP    0x1D17

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * ic_db_soc_tambah - Tambah entri SoC ke database
 */
static status_t ic_db_soc_tambah(tak_bertanda16_t vendor_id,
                                  tak_bertanda16_t device_id,
                                  const char *nama,
                                  const char *vendor,
                                  const char *seri,
                                  ic_rentang_t rentang,
                                  tak_bertanda32_t cpu_cores,
                                  tak_bertanda32_t gpu_cores,
                                  tak_bertanda32_t frekuensi_cpu_mhz,
                                  tak_bertanda32_t proses_nm)
{
    ic_entri_t *entri;
    ic_parameter_t *param;
    status_t hasil;
    
    entri = ic_entri_buat(nama, vendor, seri, IC_KATEGORI_SOC, rentang);
    if (entri == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    entri->vendor_id = vendor_id;
    entri->device_id = device_id;
    entri->bus = IC_BUS_MMIO;
    
    /* Tambah parameter SoC */
    
    /* Jumlah CPU cores */
    hasil = ic_entri_tambah_param(entri, "cpu_cores",
                                    IC_PARAM_TIPE_JUMLAH, "cores",
                                    1, 24);
    if (hasil != STATUS_BERHASIL) {
        ic_pool_entri_bebaskan(entri);
        return hasil;
    }
    param = ic_entri_cari_param(entri, "cpu_cores");
    if (param != NULL) {
        param->nilai_min = cpu_cores;
        param->nilai_max = cpu_cores;
        param->nilai_typical = cpu_cores;
        param->nilai_default = cpu_cores;
        param->ditest = BENAR;
    }
    
    /* Jumlah GPU cores */
    hasil = ic_entri_tambah_param(entri, "gpu_cores",
                                    IC_PARAM_TIPE_JUMLAH, "cores",
                                    1, 2048);
    if (hasil != STATUS_BERHASIL) {
        ic_pool_entri_bebaskan(entri);
        return hasil;
    }
    param = ic_entri_cari_param(entri, "gpu_cores");
    if (param != NULL) {
        param->nilai_min = gpu_cores;
        param->nilai_max = gpu_cores;
        param->nilai_typical = gpu_cores;
        param->nilai_default = gpu_cores;
        param->ditest = BENAR;
    }
    
    /* Frekuensi CPU max */
    hasil = ic_entri_tambah_param(entri, "frekuensi_cpu_max",
                                    IC_PARAM_TIPE_FREKUENSI, "MHz",
                                    100, 4000);
    if (hasil != STATUS_BERHASIL) {
        ic_pool_entri_bebaskan(entri);
        return hasil;
    }
    param = ic_entri_cari_param(entri, "frekuensi_cpu_max");
    if (param != NULL) {
        param->nilai_min = frekuensi_cpu_mhz;
        param->nilai_max = frekuensi_cpu_mhz;
        param->nilai_typical = frekuensi_cpu_mhz;
        param->nilai_default = ic_hitung_default(frekuensi_cpu_mhz, IC_TOLERANSI_DEFAULT_MAX);
        param->ditest = BENAR;
    }
    
    /* Proses fabrikasi (nm) */
    hasil = ic_entri_tambah_param(entri, "proses_fabrikasi",
                                    IC_PARAM_TIPE_JUMLAH, "nm",
                                    2, 28);
    if (hasil != STATUS_BERHASIL) {
        ic_pool_entri_bebaskan(entri);
        return hasil;
    }
    param = ic_entri_cari_param(entri, "proses_fabrikasi");
    if (param != NULL) {
        param->nilai_min = proses_nm;
        param->nilai_max = proses_nm;
        param->nilai_typical = proses_nm;
        param->nilai_default = proses_nm;
        param->ditest = BENAR;
    }
    
    /* Tambah ke database */
    hasil = ic_database_tambah(entri);
    
    return hasil;
}

/*
 * ===========================================================================
 * DATABASE QUALCOMM SNAPDRAGON
 * ===========================================================================
 */

static status_t ic_muat_qualcomm_soc(void)
{
    status_t hasil;
    
    /* Snapdragon 8-series (Flagship) */
    
    /* Snapdragon 8 Gen 3 (2023) */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0001,
                              "Snapdragon 8 Gen 3", "Qualcomm", "SM8650",
                              IC_RENTANG_ENTHUSIAST, 8, 3072, 3300, 4);
    
    /* Snapdragon 8 Gen 2 (2022) */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0002,
                              "Snapdragon 8 Gen 2", "Qualcomm", "SM8550",
                              IC_RENTANG_HIGH_END, 8, 2304, 3200, 4);
    
    /* Snapdragon 8 Gen 1 (2021) */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0003,
                              "Snapdragon 8 Gen 1", "Qualcomm", "SM8450",
                              IC_RENTANG_HIGH_END, 8, 1792, 3000, 4);
    
    /* Snapdragon 888+ */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0004,
                              "Snapdragon 888+", "Qualcomm", "SM8350-AC",
                              IC_RENTANG_HIGH_END, 8, 1440, 2995, 5);
    
    /* Snapdragon 888 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0005,
                              "Snapdragon 888", "Qualcomm", "SM8350",
                              IC_RENTANG_HIGH_END, 8, 1440, 2840, 5);
    
    /* Snapdragon 870 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0006,
                              "Snapdragon 870", "Qualcomm", "SM8250-AC",
                              IC_RENTANG_MID_HIGH, 8, 1024, 3200, 7);
    
    /* Snapdragon 865+ */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0007,
                              "Snapdragon 865+", "Qualcomm", "SM8250-AB",
                              IC_RENTANG_MID_HIGH, 8, 1024, 3100, 7);
    
    /* Snapdragon 865 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0008,
                              "Snapdragon 865", "Qualcomm", "SM8250",
                              IC_RENTANG_MID_HIGH, 8, 1024, 2840, 7);
    
    /* Snapdragon 855+ */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0009,
                              "Snapdragon 855+", "Qualcomm", "SM8150-AC",
                              IC_RENTANG_MID_HIGH, 8, 768, 2960, 7);
    
    /* Snapdragon 855 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x000A,
                              "Snapdragon 855", "Qualcomm", "SM8150",
                              IC_RENTANG_MID_HIGH, 8, 768, 2840, 7);
    
    /* Snapdragon 845 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x000B,
                              "Snapdragon 845", "Qualcomm", "SDM845",
                              IC_RENTANG_MID, 8, 768, 2800, 10);
    
    /* Snapdragon 835 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x000C,
                              "Snapdragon 835", "Qualcomm", "MSM8998",
                              IC_RENTANG_MID, 8, 576, 2450, 10);
    
    /* Snapdragon 821 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x000D,
                              "Snapdragon 821", "Qualcomm", "MSM8996 Pro",
                              IC_RENTANG_MID_LOW, 4, 512, 2350, 14);
    
    /* Snapdragon 820 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x000E,
                              "Snapdragon 820", "Qualcomm", "MSM8996",
                              IC_RENTANG_MID_LOW, 4, 512, 2200, 14);
    
    /* Snapdragon 810 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x000F,
                              "Snapdragon 810", "Qualcomm", "MSM8994",
                              IC_RENTANG_MID_LOW, 8, 448, 2000, 20);
    
    /* Snapdragon 7-series (Mid-range) */
    
    /* Snapdragon 7+ Gen 2 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0010,
                              "Snapdragon 7+ Gen 2", "Qualcomm", "SM7475",
                              IC_RENTANG_MID_HIGH, 8, 1280, 2900, 4);
    
    /* Snapdragon 7 Gen 1 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0011,
                              "Snapdragon 7 Gen 1", "Qualcomm", "SM7450",
                              IC_RENTANG_MID, 8, 768, 2400, 4);
    
    /* Snapdragon 778G+ */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0012,
                              "Snapdragon 778G+", "Qualcomm", "SM7325-AE",
                              IC_RENTANG_MID, 8, 768, 2500, 6);
    
    /* Snapdragon 778G */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0013,
                              "Snapdragon 778G", "Qualcomm", "SM7325",
                              IC_RENTANG_MID, 8, 768, 2400, 6);
    
    /* Snapdragon 768G */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0014,
                              "Snapdragon 768G", "Qualcomm", "SM7250-AC",
                              IC_RENTANG_MID, 8, 512, 2800, 7);
    
    /* Snapdragon 765G */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0015,
                              "Snapdragon 765G", "Qualcomm", "SM7250",
                              IC_RENTANG_MID, 8, 512, 2400, 7);
    
    /* Snapdragon 750G */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0016,
                              "Snapdragon 750G", "Qualcomm", "SM7225",
                              IC_RENTANG_MID_LOW, 8, 384, 2200, 8);
    
    /* Snapdragon 732G */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0017,
                              "Snapdragon 732G", "Qualcomm", "SM7150-AC",
                              IC_RENTANG_MID_LOW, 8, 384, 2300, 8);
    
    /* Snapdragon 730G */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0018,
                              "Snapdragon 730G", "Qualcomm", "SM7150",
                              IC_RENTANG_MID_LOW, 8, 384, 2200, 8);
    
    /* Snapdragon 6-series (Lower mid-range) */
    
    /* Snapdragon 695 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0019,
                              "Snapdragon 695", "Qualcomm", "SM6375",
                              IC_RENTANG_MID_LOW, 8, 256, 2200, 6);
    
    /* Snapdragon 690 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x001A,
                              "Snapdragon 690", "Qualcomm", "SM6350",
                              IC_RENTANG_MID_LOW, 8, 256, 2000, 8);
    
    /* Snapdragon 680 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x001B,
                              "Snapdragon 680", "Qualcomm", "SM6225",
                              IC_RENTANG_LOW_END, 8, 256, 2400, 6);
    
    /* Snapdragon 678 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x001C,
                              "Snapdragon 678", "Qualcomm", "SM6150-AC",
                              IC_RENTANG_MID_LOW, 8, 256, 2200, 11);
    
    /* Snapdragon 675 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x001D,
                              "Snapdragon 675", "Qualcomm", "SM6150",
                              IC_RENTANG_MID_LOW, 8, 256, 2000, 11);
    
    /* Snapdragon 670 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x001E,
                              "Snapdragon 670", "Qualcomm", "SDM670",
                              IC_RENTANG_MID_LOW, 8, 256, 2000, 10);
    
    /* Snapdragon 665 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x001F,
                              "Snapdragon 665", "Qualcomm", "SM6125",
                              IC_RENTANG_LOW_END, 8, 256, 2000, 11);
    
    /* Snapdragon 660 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0020,
                              "Snapdragon 660", "Qualcomm", "SDM660",
                              IC_RENTANG_MID_LOW, 8, 256, 2200, 14);
    
    /* Snapdragon 636 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0021,
                              "Snapdragon 636", "Qualcomm", "SDM636",
                              IC_RENTANG_LOW_END, 8, 192, 1800, 14);
    
    /* Snapdragon 632 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0022,
                              "Snapdragon 632", "Qualcomm", "SDM632",
                              IC_RENTANG_LOW_END, 8, 192, 1800, 14);
    
    /* Snapdragon 625 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0023,
                              "Snapdragon 625", "Qualcomm", "MSM8953",
                              IC_RENTANG_LOW_END, 8, 192, 2000, 14);
    
    /* Snapdragon 4-series (Entry level) */
    
    /* Snapdragon 4 Gen 2 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0024,
                              "Snapdragon 4 Gen 2", "Qualcomm", "SM4450",
                              IC_RENTANG_LOW_END, 8, 128, 2200, 4);
    
    /* Snapdragon 4 Gen 1 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0025,
                              "Snapdragon 4 Gen 1", "Qualcomm", "SM4375",
                              IC_RENTANG_LOW_END, 8, 128, 2000, 6);
    
    /* Snapdragon 480+ */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0026,
                              "Snapdragon 480+", "Qualcomm", "SM4350-AC",
                              IC_RENTANG_LOW_END, 8, 128, 2200, 8);
    
    /* Snapdragon 480 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0027,
                              "Snapdragon 480", "Qualcomm", "SM4350",
                              IC_RENTANG_LOW_END, 8, 128, 2000, 8);
    
    /* Snapdragon 460 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0028,
                              "Snapdragon 460", "Qualcomm", "SM4250",
                              IC_RENTANG_LOW_END, 8, 128, 1800, 11);
    
    /* Snapdragon 450 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x0029,
                              "Snapdragon 450", "Qualcomm", "SDM450",
                              IC_RENTANG_LOW_END, 8, 128, 1800, 14);
    
    /* Snapdragon 439 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x002A,
                              "Snapdragon 439", "Qualcomm", "SDM439",
                              IC_RENTANG_LOW_END, 8, 128, 1800, 14);
    
    /* Snapdragon 435 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x002B,
                              "Snapdragon 435", "Qualcomm", "MSM8940",
                              IC_RENTANG_LOW_END, 8, 128, 1400, 14);
    
    /* Snapdragon 430 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_QUALCOMM, 0x002C,
                              "Snapdragon 430", "Qualcomm", "MSM8937",
                              IC_RENTANG_LOW_END, 8, 128, 1400, 28);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * DATABASE MEDIATEK
 * ===========================================================================
 */

static status_t ic_muat_mediatek_soc(void)
{
    status_t hasil;
    
    /* Dimensity flagship series */
    
    /* Dimensity 9300 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1000,
                              "Dimensity 9300", "MediaTek", "MT6989",
                              IC_RENTANG_ENTHUSIAST, 8, 2048, 3200, 4);
    
    /* Dimensity 9200+ */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1001,
                              "Dimensity 9200+", "MediaTek", "MT6985",
                              IC_RENTANG_HIGH_END, 8, 1536, 3350, 4);
    
    /* Dimensity 9200 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1002,
                              "Dimensity 9200", "MediaTek", "MT6985",
                              IC_RENTANG_HIGH_END, 8, 1536, 3050, 4);
    
    /* Dimensity 9000+ */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1003,
                              "Dimensity 9000+", "MediaTek", "MT6983",
                              IC_RENTANG_HIGH_END, 8, 1280, 3200, 4);
    
    /* Dimensity 9000 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1004,
                              "Dimensity 9000", "MediaTek", "MT6983",
                              IC_RENTANG_HIGH_END, 8, 1280, 3050, 4);
    
    /* Dimensity 8-series */
    
    /* Dimensity 8300 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1005,
                              "Dimensity 8300", "MediaTek", "MT6897",
                              IC_RENTANG_MID_HIGH, 8, 1024, 3200, 4);
    
    /* Dimensity 8200 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1006,
                              "Dimensity 8200", "MediaTek", "MT6896",
                              IC_RENTANG_MID_HIGH, 8, 1024, 3100, 5);
    
    /* Dimensity 8100 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1007,
                              "Dimensity 8100", "MediaTek", "MT6895",
                              IC_RENTANG_MID_HIGH, 8, 1024, 2850, 5);
    
    /* Dimensity 8000 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1008,
                              "Dimensity 8000", "MediaTek", "MT6895",
                              IC_RENTANG_MID, 8, 768, 2750, 5);
    
    /* Dimensity 7-series */
    
    /* Dimensity 7300 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1009,
                              "Dimensity 7300", "MediaTek", "MT6878",
                              IC_RENTANG_MID, 8, 512, 2800, 4);
    
    /* Dimensity 7200 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x100A,
                              "Dimensity 7200", "MediaTek", "MT6886",
                              IC_RENTANG_MID, 8, 512, 2800, 4);
    
    /* Dimensity 7050 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x100B,
                              "Dimensity 7050", "MediaTek", "MT6877",
                              IC_RENTANG_MID_LOW, 8, 384, 2600, 6);
    
    /* Dimensity 6-series */
    
    /* Dimensity 6100+ */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x100C,
                              "Dimensity 6100+", "MediaTek", "MT6835",
                              IC_RENTANG_MID_LOW, 8, 256, 2200, 6);
    
    /* Dimensity 6080 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x100D,
                              "Dimensity 6080", "MediaTek", "MT6833V",
                              IC_RENTANG_MID_LOW, 8, 256, 2200, 7);
    
    /* Dimensity 6020 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x100E,
                              "Dimensity 6020", "MediaTek", "MT6833",
                              IC_RENTANG_LOW_END, 8, 256, 2000, 7);
    
    /* Dimensity 700 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x100F,
                              "Dimensity 700", "MediaTek", "MT6833",
                              IC_RENTANG_LOW_END, 8, 256, 2200, 7);
    
    /* Dimensity 720 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1010,
                              "Dimensity 720", "MediaTek", "MT6853",
                              IC_RENTANG_MID_LOW, 8, 256, 2000, 7);
    
    /* Dimensity 800U */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1011,
                              "Dimensity 800U", "MediaTek", "MT6853V",
                              IC_RENTANG_MID, 8, 384, 2400, 7);
    
    /* Dimensity 810 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1012,
                              "Dimensity 810", "MediaTek", "MT6833P",
                              IC_RENTANG_MID_LOW, 8, 256, 2400, 6);
    
    /* Dimensity 900 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1013,
                              "Dimensity 900", "MediaTek", "MT6877",
                              IC_RENTANG_MID, 8, 384, 2400, 6);
    
    /* Dimensity 920 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1014,
                              "Dimensity 920", "MediaTek", "MT6877T",
                              IC_RENTANG_MID, 8, 384, 2500, 6);
    
    /* Dimensity 1000 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1015,
                              "Dimensity 1000", "MediaTek", "MT6889",
                              IC_RENTANG_MID_HIGH, 8, 512, 2200, 7);
    
    /* Dimensity 1000+ */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1016,
                              "Dimensity 1000+", "MediaTek", "MT6889Z",
                              IC_RENTANG_MID_HIGH, 8, 512, 2300, 7);
    
    /* Dimensity 1100 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1017,
                              "Dimensity 1100", "MediaTek", "MT6891",
                              IC_RENTANG_MID_HIGH, 8, 768, 2600, 6);
    
    /* Dimensity 1200 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1018,
                              "Dimensity 1200", "MediaTek", "MT6893",
                              IC_RENTANG_MID_HIGH, 8, 768, 3000, 6);
    
    /* Helio series */
    
    /* Helio G99 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1019,
                              "Helio G99", "MediaTek", "MT6789",
                              IC_RENTANG_MID_LOW, 8, 256, 2200, 6);
    
    /* Helio G96 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x101A,
                              "Helio G96", "MediaTek", "MT6781",
                              IC_RENTANG_MID_LOW, 8, 256, 2050, 12);
    
    /* Helio G95 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x101B,
                              "Helio G95", "MediaTek", "MT6785T",
                              IC_RENTANG_MID, 8, 256, 2050, 12);
    
    /* Helio G90T */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x101C,
                              "Helio G90T", "MediaTek", "MT6785",
                              IC_RENTANG_MID, 8, 256, 2050, 12);
    
    /* Helio G88 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x101D,
                              "Helio G88", "MediaTek", "MT6769W",
                              IC_RENTANG_LOW_END, 8, 192, 2000, 12);
    
    /* Helio G85 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x101E,
                              "Helio G85", "MediaTek", "MT6769Z",
                              IC_RENTANG_LOW_END, 8, 192, 2000, 12);
    
    /* Helio G80 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x101F,
                              "Helio G80", "MediaTek", "MT6769",
                              IC_RENTANG_LOW_END, 8, 192, 2000, 12);
    
    /* Helio G70 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1020,
                              "Helio G70", "MediaTek", "MT6769V",
                              IC_RENTANG_LOW_END, 8, 128, 2000, 12);
    
    /* Helio G35 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1021,
                              "Helio G35", "MediaTek", "MT6765V",
                              IC_RENTANG_LOW_END, 8, 128, 2300, 12);
    
    /* Helio G25 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1022,
                              "Helio G25", "MediaTek", "MT6762G",
                              IC_RENTANG_LOW_END, 8, 128, 2000, 12);
    
    /* Helio P series (older) */
    
    /* Helio P95 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1023,
                              "Helio P95", "MediaTek", "MT6779V",
                              IC_RENTANG_MID, 8, 256, 2200, 12);
    
    /* Helio P90 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1024,
                              "Helio P90", "MediaTek", "MT6779",
                              IC_RENTANG_MID, 8, 256, 2200, 12);
    
    /* Helio P70 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1025,
                              "Helio P70", "MediaTek", "MT6771T",
                              IC_RENTANG_MID_LOW, 8, 192, 2100, 12);
    
    /* Helio P65 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1026,
                              "Helio P65", "MediaTek", "MT6768",
                              IC_RENTANG_MID_LOW, 8, 192, 2000, 12);
    
    /* Helio P60 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1027,
                              "Helio P60", "MediaTek", "MT6771",
                              IC_RENTANG_MID_LOW, 8, 192, 2000, 12);
    
    /* Helio P35 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1028,
                              "Helio P35", "MediaTek", "MT6765",
                              IC_RENTANG_LOW_END, 8, 128, 2300, 12);
    
    /* Helio P22 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_MEDIATEK, 0x1029,
                              "Helio P22", "MediaTek", "MT6762",
                              IC_RENTANG_LOW_END, 8, 128, 2000, 12);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * DATABASE SAMSUNG EXYNOS
 * ===========================================================================
 */

static status_t ic_muat_samsung_soc(void)
{
    status_t hasil;
    
    /* Exynos flagship series */
    
    /* Exynos 2400 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_SAMSUNG, 0x2000,
                              "Exynos 2400", "Samsung", "S5E9945",
                              IC_RENTANG_HIGH_END, 10, 1536, 3200, 4);
    
    /* Exynos 2200 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_SAMSUNG, 0x2001,
                              "Exynos 2200", "Samsung", "S5E9925",
                              IC_RENTANG_HIGH_END, 8, 1024, 2900, 4);
    
    /* Exynos 2100 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_SAMSUNG, 0x2002,
                              "Exynos 2100", "Samsung", "S5E9915",
                              IC_RENTANG_HIGH_END, 8, 1024, 2900, 5);
    
    /* Exynos 990 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_SAMSUNG, 0x2003,
                              "Exynos 990", "Samsung", "S5E9830",
                              IC_RENTANG_MID_HIGH, 8, 768, 2700, 7);
    
    /* Exynos 9825 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_SAMSUNG, 0x2004,
                              "Exynos 9825", "Samsung", "S5E9825",
                              IC_RENTANG_MID_HIGH, 8, 512, 2700, 7);
    
    /* Exynos 9820 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_SAMSUNG, 0x2005,
                              "Exynos 9820", "Samsung", "S5E9820",
                              IC_RENTANG_MID_HIGH, 8, 512, 2700, 8);
    
    /* Exynos 9810 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_SAMSUNG, 0x2006,
                              "Exynos 9810", "Samsung", "S5E9810",
                              IC_RENTANG_MID, 8, 384, 2700, 10);
    
    /* Exynos 8895 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_SAMSUNG, 0x2007,
                              "Exynos 8895", "Samsung", "S5E8895",
                              IC_RENTANG_MID, 8, 320, 2300, 10);
    
    /* Exynos 8890 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_SAMSUNG, 0x2008,
                              "Exynos 8890", "Samsung", "S5E8890",
                              IC_RENTANG_MID_LOW, 8, 320, 2300, 14);
    
    /* Exynos 7-series */
    
    /* Exynos 1480 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_SAMSUNG, 0x2009,
                              "Exynos 1480", "Samsung", "S5E8835",
                              IC_RENTANG_MID, 8, 768, 2700, 4);
    
    /* Exynos 1380 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_SAMSUNG, 0x200A,
                              "Exynos 1380", "Samsung", "S5E8835",
                              IC_RENTANG_MID, 8, 512, 2400, 5);
    
    /* Exynos 1330 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_SAMSUNG, 0x200B,
                              "Exynos 1330", "Samsung", "S5E8825",
                              IC_RENTANG_MID_LOW, 8, 384, 2400, 5);
    
    /* Exynos 1280 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_SAMSUNG, 0x200C,
                              "Exynos 1280", "Samsung", "S5E8825",
                              IC_RENTANG_MID_LOW, 8, 384, 2400, 5);
    
    /* Exynos 1080 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_SAMSUNG, 0x200D,
                              "Exynos 1080", "Samsung", "S5E8810",
                              IC_RENTANG_MID, 8, 512, 2600, 5);
    
    /* Exynos 980 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_SAMSUNG, 0x200E,
                              "Exynos 980", "Samsung", "S5E9800",
                              IC_RENTANG_MID, 8, 384, 2200, 8);
    
    /* Exynos 850 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_SAMSUNG, 0x200F,
                              "Exynos 850", "Samsung", "S5E3830",
                              IC_RENTANG_LOW_END, 8, 128, 2000, 8);
    
    /* Exynos 7904 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_SAMSUNG, 0x2010,
                              "Exynos 7904", "Samsung", "S5E7904",
                              IC_RENTANG_LOW_END, 8, 128, 1800, 14);
    
    /* Exynos 7885 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_SAMSUNG, 0x2011,
                              "Exynos 7885", "Samsung", "S5E7885",
                              IC_RENTANG_LOW_END, 8, 128, 2000, 14);
    
    /* Exynos 7884 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_SAMSUNG, 0x2012,
                              "Exynos 7884", "Samsung", "S5E7884",
                              IC_RENTANG_LOW_END, 8, 128, 1800, 14);
    
    /* Exynos 7880 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_SAMSUNG, 0x2013,
                              "Exynos 7880", "Samsung", "S5E7880",
                              IC_RENTANG_LOW_END, 8, 128, 1900, 14);
    
    /* Exynos 7870 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_SAMSUNG, 0x2014,
                              "Exynos 7870", "Samsung", "S5E7870",
                              IC_RENTANG_LOW_END, 8, 128, 1600, 14);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * DATABASE APPLE SILICON
 * ===========================================================================
 */

static status_t ic_muat_apple_soc(void)
{
    status_t hasil;
    
    /* Apple M-series (Desktop/Laptop) */
    
    /* Apple M3 Ultra */
    hasil = ic_db_soc_tambah(SOC_VENDOR_APPLE, 0x3000,
                              "Apple M3 Ultra", "Apple", "M3 Ultra",
                              IC_RENTANG_SERVER, 32, 4096, 4000, 3);
    
    /* Apple M3 Max */
    hasil = ic_db_soc_tambah(SOC_VENDOR_APPLE, 0x3001,
                              "Apple M3 Max", "Apple", "M3 Max",
                              IC_RENTANG_ENTHUSIAST, 16, 2048, 4000, 3);
    
    /* Apple M3 Pro */
    hasil = ic_db_soc_tambah(SOC_VENDOR_APPLE, 0x3002,
                              "Apple M3 Pro", "Apple", "M3 Pro",
                              IC_RENTANG_HIGH_END, 12, 1536, 4000, 3);
    
    /* Apple M3 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_APPLE, 0x3003,
                              "Apple M3", "Apple", "M3",
                              IC_RENTANG_HIGH_END, 8, 1024, 4000, 3);
    
    /* Apple M2 Ultra */
    hasil = ic_db_soc_tambah(SOC_VENDOR_APPLE, 0x3004,
                              "Apple M2 Ultra", "Apple", "M2 Ultra",
                              IC_RENTANG_SERVER, 24, 3072, 3400, 5);
    
    /* Apple M2 Max */
    hasil = ic_db_soc_tambah(SOC_VENDOR_APPLE, 0x3005,
                              "Apple M2 Max", "Apple", "M2 Max",
                              IC_RENTANG_ENTHUSIAST, 12, 1536, 3400, 5);
    
    /* Apple M2 Pro */
    hasil = ic_db_soc_tambah(SOC_VENDOR_APPLE, 0x3006,
                              "Apple M2 Pro", "Apple", "M2 Pro",
                              IC_RENTANG_HIGH_END, 10, 1024, 3400, 5);
    
    /* Apple M2 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_APPLE, 0x3007,
                              "Apple M2", "Apple", "M2",
                              IC_RENTANG_MID_HIGH, 8, 768, 3200, 5);
    
    /* Apple M1 Ultra */
    hasil = ic_db_soc_tambah(SOC_VENDOR_APPLE, 0x3008,
                              "Apple M1 Ultra", "Apple", "M1 Ultra",
                              IC_RENTANG_SERVER, 20, 2048, 3200, 5);
    
    /* Apple M1 Max */
    hasil = ic_db_soc_tambah(SOC_VENDOR_APPLE, 0x3009,
                              "Apple M1 Max", "Apple", "M1 Max",
                              IC_RENTANG_ENTHUSIAST, 10, 1024, 3200, 5);
    
    /* Apple M1 Pro */
    hasil = ic_db_soc_tambah(SOC_VENDOR_APPLE, 0x300A,
                              "Apple M1 Pro", "Apple", "M1 Pro",
                              IC_RENTANG_HIGH_END, 8, 512, 3200, 5);
    
    /* Apple M1 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_APPLE, 0x300B,
                              "Apple M1", "Apple", "M1",
                              IC_RENTANG_MID_HIGH, 8, 256, 3200, 5);
    
    /* Apple A-series (iPhone/iPad) */
    
    /* Apple A17 Pro */
    hasil = ic_db_soc_tambah(SOC_VENDOR_APPLE, 0x300C,
                              "Apple A17 Pro", "Apple", "A17 Pro",
                              IC_RENTANG_ENTHUSIAST, 6, 768, 3780, 3);
    
    /* Apple A16 Bionic */
    hasil = ic_db_soc_tambah(SOC_VENDOR_APPLE, 0x300D,
                              "Apple A16 Bionic", "Apple", "A16",
                              IC_RENTANG_HIGH_END, 6, 512, 3460, 4);
    
    /* Apple A15 Bionic */
    hasil = ic_db_soc_tambah(SOC_VENDOR_APPLE, 0x300E,
                              "Apple A15 Bionic", "Apple", "A15",
                              IC_RENTANG_HIGH_END, 6, 384, 3230, 5);
    
    /* Apple A14 Bionic */
    hasil = ic_db_soc_tambah(SOC_VENDOR_APPLE, 0x300F,
                              "Apple A14 Bionic", "Apple", "A14",
                              IC_RENTANG_MID_HIGH, 6, 256, 3100, 5);
    
    /* Apple A13 Bionic */
    hasil = ic_db_soc_tambah(SOC_VENDOR_APPLE, 0x3010,
                              "Apple A13 Bionic", "Apple", "A13",
                              IC_RENTANG_MID, 6, 192, 2660, 7);
    
    /* Apple A12 Bionic */
    hasil = ic_db_soc_tambah(SOC_VENDOR_APPLE, 0x3011,
                              "Apple A12 Bionic", "Apple", "A12",
                              IC_RENTANG_MID, 6, 128, 2500, 7);
    
    /* Apple A11 Bionic */
    hasil = ic_db_soc_tambah(SOC_VENDOR_APPLE, 0x3012,
                              "Apple A11 Bionic", "Apple", "A11",
                              IC_RENTANG_MID_LOW, 6, 96, 2390, 10);
    
    /* Apple A10 Fusion */
    hasil = ic_db_soc_tambah(SOC_VENDOR_APPLE, 0x3013,
                              "Apple A10 Fusion", "Apple", "A10",
                              IC_RENTANG_LOW_END, 4, 64, 2340, 16);
    
    /* Apple A9 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_APPLE, 0x3014,
                              "Apple A9", "Apple", "A9",
                              IC_RENTANG_LOW_END, 2, 48, 1850, 14);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * DATABASE HUAWEI KIRIN
 * ===========================================================================
 */

static status_t ic_muat_huawei_soc(void)
{
    status_t hasil;
    
    /* Kirin flagship series */
    
    /* Kirin 9000S */
    hasil = ic_db_soc_tambah(SOC_VENDOR_HISILICON, 0x4000,
                              "Kirin 9000S", "HiSilicon", "Kirin 9000S",
                              IC_RENTANG_HIGH_END, 8, 768, 2500, 7);
    
    /* Kirin 9000 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_HISILICON, 0x4001,
                              "Kirin 9000", "HiSilicon", "Kirin 9000",
                              IC_RENTANG_HIGH_END, 8, 768, 3130, 5);
    
    /* Kirin 990 5G */
    hasil = ic_db_soc_tambah(SOC_VENDOR_HISILICON, 0x4002,
                              "Kirin 990 5G", "HiSilicon", "Kirin 990",
                              IC_RENTANG_MID_HIGH, 8, 512, 2860, 7);
    
    /* Kirin 990 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_HISILICON, 0x4003,
                              "Kirin 990", "HiSilicon", "Kirin 990",
                              IC_RENTANG_MID_HIGH, 8, 512, 2630, 7);
    
    /* Kirin 985 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_HISILICON, 0x4004,
                              "Kirin 985", "HiSilicon", "Kirin 985",
                              IC_RENTANG_MID, 8, 384, 2560, 7);
    
    /* Kirin 980 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_HISILICON, 0x4005,
                              "Kirin 980", "HiSilicon", "Kirin 980",
                              IC_RENTANG_MID, 8, 256, 2600, 7);
    
    /* Kirin 970 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_HISILICON, 0x4006,
                              "Kirin 970", "HiSilicon", "Kirin 970",
                              IC_RENTANG_MID_LOW, 8, 192, 2360, 10);
    
    /* Kirin 960 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_HISILICON, 0x4007,
                              "Kirin 960", "HiSilicon", "Kirin 960",
                              IC_RENTANG_MID_LOW, 8, 128, 2360, 16);
    
    /* Kirin 7-series */
    
    /* Kirin 820 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_HISILICON, 0x4008,
                              "Kirin 820", "HiSilicon", "Kirin 820",
                              IC_RENTANG_MID, 8, 256, 2360, 7);
    
    /* Kirin 810 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_HISILICON, 0x4009,
                              "Kirin 810", "HiSilicon", "Kirin 810",
                              IC_RENTANG_MID_LOW, 8, 192, 2270, 7);
    
    /* Kirin 710F */
    hasil = ic_db_soc_tambah(SOC_VENDOR_HISILICON, 0x400A,
                              "Kirin 710F", "HiSilicon", "Kirin 710F",
                              IC_RENTANG_LOW_END, 8, 128, 2000, 14);
    
    /* Kirin 710 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_HISILICON, 0x400B,
                              "Kirin 710", "HiSilicon", "Kirin 710",
                              IC_RENTANG_LOW_END, 8, 128, 2000, 12);
    
    /* Kirin 659 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_HISILICON, 0x400C,
                              "Kirin 659", "HiSilicon", "Kirin 659",
                              IC_RENTANG_LOW_END, 8, 64, 2360, 16);
    
    /* Kirin 658 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_HISILICON, 0x400D,
                              "Kirin 658", "HiSilicon", "Kirin 658",
                              IC_RENTANG_LOW_END, 8, 64, 2000, 16);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * DATABASE ROCKCHIP
 * ===========================================================================
 */

static status_t ic_muat_rockchip_soc(void)
{
    status_t hasil;
    
    /* RK flagship/high-end */
    
    /* RK3588 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_ROCKCHIP, 0x5000,
                              "RK3588", "Rockchip", "RK3588",
                              IC_RENTANG_MID_HIGH, 8, 512, 2400, 8);
    
    /* RK3588S */
    hasil = ic_db_soc_tambah(SOC_VENDOR_ROCKCHIP, 0x5001,
                              "RK3588S", "Rockchip", "RK3588S",
                              IC_RENTANG_MID, 8, 384, 2400, 8);
    
    /* RK3576 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_ROCKCHIP, 0x5002,
                              "RK3576", "Rockchip", "RK3576",
                              IC_RENTANG_MID, 8, 256, 2200, 8);
    
    /* RK3568 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_ROCKCHIP, 0x5003,
                              "RK3568", "Rockchip", "RK3568",
                              IC_RENTANG_MID_LOW, 4, 128, 2000, 8);
    
    /* RK3566 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_ROCKCHIP, 0x5004,
                              "RK3566", "Rockchip", "RK3566",
                              IC_RENTANG_MID_LOW, 4, 128, 1800, 8);
    
    /* RK3399 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_ROCKCHIP, 0x5005,
                              "RK3399", "Rockchip", "RK3399",
                              IC_RENTANG_MID, 6, 128, 2000, 28);
    
    /* RK3399Pro */
    hasil = ic_db_soc_tambah(SOC_VENDOR_ROCKCHIP, 0x5006,
                              "RK3399Pro", "Rockchip", "RK3399Pro",
                              IC_RENTANG_MID, 6, 128, 2000, 28);
    
    /* RK3368 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_ROCKCHIP, 0x5007,
                              "RK3368", "Rockchip", "RK3368",
                              IC_RENTANG_MID_LOW, 8, 64, 1500, 28);
    
    /* RK3328 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_ROCKCHIP, 0x5008,
                              "RK3328", "Rockchip", "RK3328",
                              IC_RENTANG_LOW_END, 4, 64, 1500, 28);
    
    /* RK3288 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_ROCKCHIP, 0x5009,
                              "RK3288", "Rockchip", "RK3288",
                              IC_RENTANG_MID_LOW, 4, 64, 1800, 28);
    
    /* RK3128 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_ROCKCHIP, 0x500A,
                              "RK3128", "Rockchip", "RK3128",
                              IC_RENTANG_LOW_END, 4, 32, 1200, 40);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * DATABASE UNISOC
 * ===========================================================================
 */

static status_t ic_muat_unisoc_soc(void)
{
    status_t hasil;
    
    /* Tanggula series */
    
    /* T820 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_UNISOC, 0x6000,
                              "Unisoc T820", "UNISOC", "T820",
                              IC_RENTANG_MID, 8, 384, 2500, 6);
    
    /* T770 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_UNISOC, 0x6001,
                              "Unisoc T770", "UNISOC", "T770",
                              IC_RENTANG_MID_LOW, 8, 256, 2300, 6);
    
    /* T760 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_UNISOC, 0x6002,
                              "Unisoc T760", "UNISOC", "T760",
                              IC_RENTANG_MID_LOW, 8, 256, 2200, 6);
    
    /* T740 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_UNISOC, 0x6003,
                              "Unisoc T740", "UNISOC", "T740",
                              IC_RENTANG_MID_LOW, 8, 192, 2000, 12);
    
    /* T710 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_UNISOC, 0x6004,
                              "Unisoc T710", "UNISOC", "T710",
                              IC_RENTANG_MID_LOW, 8, 192, 2000, 12);
    
    /* T700 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_UNISOC, 0x6005,
                              "Unisoc T700", "UNISOC", "T700",
                              IC_RENTANG_MID_LOW, 8, 192, 1800, 12);
    
    /* T618 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_UNISOC, 0x6006,
                              "Unisoc T618", "UNISOC", "T618",
                              IC_RENTANG_MID_LOW, 8, 128, 2000, 12);
    
    /* T616 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_UNISOC, 0x6007,
                              "Unisoc T616", "UNISOC", "T616",
                              IC_RENTANG_LOW_END, 8, 128, 2000, 12);
    
    /* T612 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_UNISOC, 0x6008,
                              "Unisoc T612", "UNISOC", "T612",
                              IC_RENTANG_LOW_END, 8, 128, 1800, 12);
    
    /* T606 */
    hasil = ic_db_soc_tambah(SOC_VENDOR_UNISOC, 0x6009,
                              "Unisoc T606", "UNISOC", "T606",
                              IC_RENTANG_LOW_END, 8, 64, 1600, 12);
    
    /* Tiger series */
    
    /* SC9863A */
    hasil = ic_db_soc_tambah(SOC_VENDOR_UNISOC, 0x600A,
                              "Unisoc SC9863A", "UNISOC", "SC9863A",
                              IC_RENTANG_LOW_END, 8, 64, 1600, 28);
    
    /* SC9832E */
    hasil = ic_db_soc_tambah(SOC_VENDOR_UNISOC, 0x600B,
                              "Unisoc SC9832E", "UNISOC", "SC9832E",
                              IC_RENTANG_LOW_END, 4, 64, 1400, 28);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI UTAMA
 * ===========================================================================
 */

/*
 * ic_muat_mobile_soc_database - Muat seluruh database Mobile SoC
 */
status_t ic_muat_mobile_soc_database(void)
{
    /* Qualcomm Snapdragon */
    ic_muat_qualcomm_soc();
    
    /* MediaTek */
    ic_muat_mediatek_soc();
    
    /* Samsung Exynos */
    ic_muat_samsung_soc();
    
    /* Apple Silicon */
    ic_muat_apple_soc();
    
    /* Huawei HiSilicon */
    ic_muat_huawei_soc();
    
    /* Rockchip */
    ic_muat_rockchip_soc();
    
    /* UNISOC */
    ic_muat_unisoc_soc();
    
    return STATUS_BERHASIL;
}
