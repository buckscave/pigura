/*
 * PIGURA OS - DATABASE/DB_GPU.C
 * =============================
 * Database GPU IC dari low-end hingga high-end.
 *
 * Berkas ini berisi database IC GPU Intel, AMD, dan NVIDIA.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../ic.h"

/*
 * ===========================================================================
 * KONSTANTA VENDOR ID GPU
 * ===========================================================================
 */

#define VENDOR_ID_NVIDIA  0x10DE
#define VENDOR_ID_AMD_GPU 0x1002
#define VENDOR_ID_INTEL_GPU 0x8086

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * ic_db_gpu_tambah - Tambah entri GPU ke database
 */
static status_t ic_db_gpu_tambah(tak_bertanda16_t vendor_id,
                                  tak_bertanda16_t device_id,
                                  const char *nama,
                                  const char *vendor,
                                  const char *seri,
                                  ic_rentang_t rentang,
                                  tak_bertanda32_t memori_mb,
                                  tak_bertanda32_t core_clock_mhz,
                                  tak_bertanda32_t shader_count)
{
    ic_entri_t *entri;
    ic_parameter_t *param;
    status_t hasil;
    
    entri = ic_buat_entri_gpu(vendor_id, device_id, nama, vendor, seri, rentang);
    if (entri == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    /* Set parameter memori */
    param = ic_entri_cari_param(entri, "memori_vram");
    if (param != NULL) {
        param->nilai_min = memori_mb;
        param->nilai_max = memori_mb;
        param->nilai_typical = memori_mb;
        param->nilai_default = memori_mb;
    }
    
    /* Set parameter core clock */
    param = ic_entri_cari_param(entri, "core_clock");
    if (param != NULL) {
        param->nilai_min = core_clock_mhz;
        param->nilai_max = (tak_bertanda64_t)(core_clock_mhz * 1.1);
        param->nilai_typical = core_clock_mhz;
        param->nilai_default = ic_hitung_default(core_clock_mhz,
                                                  IC_TOLERANSI_DEFAULT_MAX);
    }
    
    /* Set parameter shader */
    param = ic_entri_cari_param(entri, "jumlah_shader");
    if (param != NULL) {
        param->nilai_min = shader_count;
        param->nilai_max = shader_count;
        param->nilai_typical = shader_count;
        param->nilai_default = shader_count;
    }
    
    /* Tambah ke database */
    hasil = ic_database_tambah(entri);
    
    return hasil;
}

/*
 * ===========================================================================
 * DATABASE NVIDIA GPU
 * ===========================================================================
 */

static status_t ic_muat_nvidia_low_end(void)
{
    status_t hasil;
    
    /* GeForce 2 series - 2000 */
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x0150,
                              "NVIDIA GeForce2", "NVIDIA", "GeForce2 MX",
                              IC_RENTANG_LOW_END, 32, 175, 2);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x0151,
                              "NVIDIA GeForce2", "NVIDIA", "GeForce2 MX400",
                              IC_RENTANG_LOW_END, 32, 200, 2);
    
    /* GeForce 4 series - 2002 */
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x0170,
                              "NVIDIA GeForce4", "NVIDIA", "GeForce4 MX 440",
                              IC_RENTANG_LOW_END, 64, 270, 2);
    
    /* GeForce FX series - 2003-2004 */
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x0320,
                              "NVIDIA GeForce FX", "NVIDIA", "GeForce FX 5200",
                              IC_RENTANG_LOW_END, 128, 250, 4);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x0330,
                              "NVIDIA GeForce FX", "NVIDIA", "GeForce FX 5600",
                              IC_RENTANG_MID_LOW, 128, 325, 4);
    
    /* GeForce 6 series - 2004-2005 */
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x00C0,
                              "NVIDIA GeForce 6", "NVIDIA", "GeForce 6200",
                              IC_RENTANG_LOW_END, 128, 300, 3);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x00F0,
                              "NVIDIA GeForce 6", "NVIDIA", "GeForce 6800",
                              IC_RENTANG_MID_LOW, 128, 350, 12);
    
    /* GeForce 7 series - 2005-2006 */
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x01D0,
                              "NVIDIA GeForce 7", "NVIDIA", "GeForce 7300",
                              IC_RENTANG_LOW_END, 256, 350, 4);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x0090,
                              "NVIDIA GeForce 7", "NVIDIA", "GeForce 7800",
                              IC_RENTANG_MID, 256, 430, 16);
    
    return STATUS_BERHASIL;
}

static status_t ic_muat_nvidia_mid_range(void)
{
    status_t hasil;
    
    /* GeForce 8 series - 2006-2008 */
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x0400,
                              "NVIDIA GeForce 8", "NVIDIA", "GeForce 8600 GT",
                              IC_RENTANG_MID_LOW, 256, 540, 32);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x0190,
                              "NVIDIA GeForce 8", "NVIDIA", "GeForce 8800 GT",
                              IC_RENTANG_MID, 512, 600, 112);
    
    /* GeForce 9 series - 2008-2009 */
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x0600,
                              "NVIDIA GeForce 9", "NVIDIA", "GeForce 9600 GT",
                              IC_RENTANG_MID_LOW, 512, 650, 64);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x0610,
                              "NVIDIA GeForce 9", "NVIDIA", "GeForce 9800 GTX",
                              IC_RENTANG_MID, 512, 675, 128);
    
    /* GeForce 200 series - 2008-2010 */
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x05E0,
                              "NVIDIA GeForce GT200", "NVIDIA", "GeForce GTS 250",
                              IC_RENTANG_MID, 1024, 738, 128);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x05A0,
                              "NVIDIA GeForce GT200", "NVIDIA", "GeForce GTX 260",
                              IC_RENTANG_MID_HIGH, 896, 576, 192);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x0580,
                              "NVIDIA GeForce GT200", "NVIDIA", "GeForce GTX 285",
                              IC_RENTANG_MID_HIGH, 2048, 648, 240);
    
    /* GeForce 400 series - 2010 */
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x0DC0,
                              "NVIDIA GeForce 400", "NVIDIA", "GeForce GT 430",
                              IC_RENTANG_MID_LOW, 1024, 700, 96);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x06C0,
                              "NVIDIA GeForce 400", "NVIDIA", "GeForce GTX 460",
                              IC_RENTANG_MID, 1024, 675, 336);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x06D0,
                              "NVIDIA GeForce 400", "NVIDIA", "GeForce GTX 480",
                              IC_RENTANG_MID_HIGH, 1536, 700, 480);
    
    return STATUS_BERHASIL;
}

static status_t ic_muat_nvidia_high_end(void)
{
    status_t hasil;
    
    /* GeForce 500 series - 2010-2012 */
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x0FC0,
                              "NVIDIA GeForce 500", "NVIDIA", "GeForce GTX 550 Ti",
                              IC_RENTANG_MID, 1024, 900, 192);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x1080,
                              "NVIDIA GeForce 500", "NVIDIA", "GeForce GTX 570",
                              IC_RENTANG_MID_HIGH, 1280, 732, 480);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x1086,
                              "NVIDIA GeForce 500", "NVIDIA", "GeForce GTX 580",
                              IC_RENTANG_MID_HIGH, 1536, 772, 512);
    
    /* GeForce 600 series - 2012-2013 */
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x0FC6,
                              "NVIDIA GeForce 600", "NVIDIA", "GeForce GTX 650",
                              IC_RENTANG_MID, 1024, 1058, 384);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x0FC8,
                              "NVIDIA GeForce 600", "NVIDIA", "GeForce GTX 660",
                              IC_RENTANG_MID_HIGH, 2048, 980, 960);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x11C0,
                              "NVIDIA GeForce 600", "NVIDIA", "GeForce GTX 670",
                              IC_RENTANG_MID_HIGH, 2048, 915, 1344);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x11BF,
                              "NVIDIA GeForce 600", "NVIDIA", "GeForce GTX 680",
                              IC_RENTANG_MID_HIGH, 2048, 1006, 1536);
    
    /* GeForce 700 series - 2013-2015 */
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x0FCB,
                              "NVIDIA GeForce 700", "NVIDIA", "GeForce GTX 750 Ti",
                              IC_RENTANG_MID, 2048, 1020, 640);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x1004,
                              "NVIDIA GeForce 700", "NVIDIA", "GeForce GTX 770",
                              IC_RENTANG_MID_HIGH, 2048, 1046, 1536);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x100A,
                              "NVIDIA GeForce 700", "NVIDIA", "GeForce GTX 780",
                              IC_RENTANG_HIGH_END, 3072, 863, 2304);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x100D,
                              "NVIDIA GeForce 700", "NVIDIA", "GeForce GTX 780 Ti",
                              IC_RENTANG_HIGH_END, 3072, 875, 2880);
    
    /* GeForce 900 series - 2014-2016 */
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x1401,
                              "NVIDIA GeForce 900", "NVIDIA", "GeForce GTX 950",
                              IC_RENTANG_MID, 2048, 1024, 768);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x1402,
                              "NVIDIA GeForce 900", "NVIDIA", "GeForce GTX 960",
                              IC_RENTANG_MID_HIGH, 2048, 1127, 1024);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x1617,
                              "NVIDIA GeForce 900", "NVIDIA", "GeForce GTX 970",
                              IC_RENTANG_MID_HIGH, 4096, 1050, 1664);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x1618,
                              "NVIDIA GeForce 900", "NVIDIA", "GeForce GTX 980",
                              IC_RENTANG_HIGH_END, 4096, 1126, 2048);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x17C2,
                              "NVIDIA GeForce 900", "NVIDIA", "GeForce GTX 980 Ti",
                              IC_RENTANG_HIGH_END, 6144, 1000, 2816);
    
    /* GeForce 10 series - 2016-2018 */
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x1C01,
                              "NVIDIA GeForce 10", "NVIDIA", "GeForce GT 1030",
                              IC_RENTANG_MID_LOW, 2048, 1227, 384);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x1C03,
                              "NVIDIA GeForce 10", "NVIDIA", "GeForce GTX 1050 Ti",
                              IC_RENTANG_MID, 4096, 1290, 768);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x1B80,
                              "NVIDIA GeForce 10", "NVIDIA", "GeForce GTX 1070",
                              IC_RENTANG_MID_HIGH, 8192, 1506, 1920);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x1B81,
                              "NVIDIA GeForce 10", "NVIDIA", "GeForce GTX 1080",
                              IC_RENTANG_HIGH_END, 8192, 1607, 2560);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x1B82,
                              "NVIDIA GeForce 10", "NVIDIA", "GeForce GTX 1080 Ti",
                              IC_RENTANG_HIGH_END, 11264, 1480, 3584);
    
    /* GeForce RTX 20 series - 2018-2020 */
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x1F02,
                              "NVIDIA GeForce RTX 20", "NVIDIA", "GeForce RTX 2060",
                              IC_RENTANG_MID_HIGH, 6144, 1365, 1920);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x1E84,
                              "NVIDIA GeForce RTX 20", "NVIDIA", "GeForce RTX 2070",
                              IC_RENTANG_HIGH_END, 8192, 1410, 2304);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x1E87,
                              "NVIDIA GeForce RTX 20", "NVIDIA", "GeForce RTX 2080",
                              IC_RENTANG_HIGH_END, 8192, 1515, 2944);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x1E07,
                              "NVIDIA GeForce RTX 20", "NVIDIA", "GeForce RTX 2080 Ti",
                              IC_RENTANG_ENTHUSIAST, 11264, 1350, 4352);
    
    /* GeForce RTX 30 series - 2020-2022 */
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x2503,
                              "NVIDIA GeForce RTX 30", "NVIDIA", "GeForce RTX 3050",
                              IC_RENTANG_MID, 8192, 1552, 2560);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x2507,
                              "NVIDIA GeForce RTX 30", "NVIDIA", "GeForce RTX 3060",
                              IC_RENTANG_MID_HIGH, 12288, 1320, 3584);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x2484,
                              "NVIDIA GeForce RTX 30", "NVIDIA", "GeForce RTX 3070",
                              IC_RENTANG_HIGH_END, 8192, 1500, 5888);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x2206,
                              "NVIDIA GeForce RTX 30", "NVIDIA", "GeForce RTX 3080",
                              IC_RENTANG_HIGH_END, 10240, 1440, 8704);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x2204,
                              "NVIDIA GeForce RTX 30", "NVIDIA", "GeForce RTX 3090",
                              IC_RENTANG_ENTHUSIAST, 24576, 1395, 10496);
    
    /* GeForce RTX 40 series - 2022+ */
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x2684,
                              "NVIDIA GeForce RTX 40", "NVIDIA", "GeForce RTX 4060",
                              IC_RENTANG_MID_HIGH, 8192, 1830, 3072);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x2782,
                              "NVIDIA GeForce RTX 40", "NVIDIA", "GeForce RTX 4070",
                              IC_RENTANG_HIGH_END, 12288, 1920, 5888);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x2704,
                              "NVIDIA GeForce RTX 40", "NVIDIA", "GeForce RTX 4080",
                              IC_RENTANG_HIGH_END, 16384, 2205, 9728);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_NVIDIA, 0x2684,
                              "NVIDIA GeForce RTX 40", "NVIDIA", "GeForce RTX 4090",
                              IC_RENTANG_ENTHUSIAST, 24576, 2235, 16384);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * DATABASE AMD GPU
 * ===========================================================================
 */

static status_t ic_muat_amd_gpu_database(void)
{
    status_t hasil;
    
    /* AMD Radeon HD series - Low-end */
    hasil = ic_db_gpu_tambah(VENDOR_ID_AMD_GPU, 0x6779,
                              "AMD Radeon HD", "AMD", "Radeon HD 6450",
                              IC_RENTANG_LOW_END, 512, 625, 160);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_AMD_GPU, 0x6770,
                              "AMD Radeon HD", "AMD", "Radeon HD 6770",
                              IC_RENTANG_MID_LOW, 1024, 850, 800);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_AMD_GPU, 0x6898,
                              "AMD Radeon HD", "AMD", "Radeon HD 6870",
                              IC_RENTANG_MID, 1024, 900, 1120);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_AMD_GPU, 0x6718,
                              "AMD Radeon HD", "AMD", "Radeon HD 6970",
                              IC_RENTANG_MID, 2048, 880, 1536);
    
    /* AMD Radeon RX 400/500 series */
    hasil = ic_db_gpu_tambah(VENDOR_ID_AMD_GPU, 0x67EF,
                              "AMD Radeon RX", "AMD", "Radeon RX 460",
                              IC_RENTANG_MID_LOW, 2048, 1090, 896);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_AMD_GPU, 0x67DF,
                              "AMD Radeon RX", "AMD", "Radeon RX 480",
                              IC_RENTANG_MID_HIGH, 8192, 1120, 2304);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_AMD_GPU, 0x67DF,
                              "AMD Radeon RX", "AMD", "Radeon RX 580",
                              IC_RENTANG_MID_HIGH, 8192, 1257, 2304);
    
    /* AMD Radeon RX Vega series */
    hasil = ic_db_gpu_tambah(VENDOR_ID_AMD_GPU, 0x687F,
                              "AMD Radeon RX", "AMD", "Radeon RX Vega 56",
                              IC_RENTANG_HIGH_END, 8192, 1156, 3584);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_AMD_GPU, 0x687F,
                              "AMD Radeon RX", "AMD", "Radeon RX Vega 64",
                              IC_RENTANG_HIGH_END, 8192, 1247, 4096);
    
    /* AMD Radeon RX 5000 series */
    hasil = ic_db_gpu_tambah(VENDOR_ID_AMD_GPU, 0x731F,
                              "AMD Radeon RX", "AMD", "Radeon RX 5500 XT",
                              IC_RENTANG_MID_HIGH, 8192, 1670, 1408);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_AMD_GPU, 0x731F,
                              "AMD Radeon RX", "AMD", "Radeon RX 5700 XT",
                              IC_RENTANG_HIGH_END, 8192, 1605, 2560);
    
    /* AMD Radeon RX 6000 series */
    hasil = ic_db_gpu_tambah(VENDOR_ID_AMD_GPU, 0x73FF,
                              "AMD Radeon RX", "AMD", "Radeon RX 6600 XT",
                              IC_RENTANG_MID_HIGH, 8192, 1968, 2048);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_AMD_GPU, 0x73BF,
                              "AMD Radeon RX", "AMD", "Radeon RX 6700 XT",
                              IC_RENTANG_HIGH_END, 12288, 2321, 2560);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_AMD_GPU, 0x73BF,
                              "AMD Radeon RX", "AMD", "Radeon RX 6800 XT",
                              IC_RENTANG_HIGH_END, 16384, 1825, 4608);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_AMD_GPU, 0x73BF,
                              "AMD Radeon RX", "AMD", "Radeon RX 6900 XT",
                              IC_RENTANG_ENTHUSIAST, 16384, 2015, 5120);
    
    /* AMD Radeon RX 7000 series */
    hasil = ic_db_gpu_tambah(VENDOR_ID_AMD_GPU, 0x7480,
                              "AMD Radeon RX", "AMD", "Radeon RX 7600",
                              IC_RENTANG_MID_HIGH, 8192, 2250, 2048);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_AMD_GPU, 0x7448,
                              "AMD Radeon RX", "AMD", "Radeon RX 7800 XT",
                              IC_RENTANG_HIGH_END, 16384, 2124, 3840);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_AMD_GPU, 0x7448,
                              "AMD Radeon RX", "AMD", "Radeon RX 7900 XT",
                              IC_RENTANG_ENTHUSIAST, 20480, 1930, 5376);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_AMD_GPU, 0x744C,
                              "AMD Radeon RX", "AMD", "Radeon RX 7900 XTX",
                              IC_RENTANG_ENTHUSIAST, 24576, 1900, 6144);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * DATABASE INTEL GPU (Integrated)
 * ===========================================================================
 */

static status_t ic_muat_intel_gpu_database(void)
{
    status_t hasil;
    
    /* Intel HD Graphics - Sandy Bridge */
    hasil = ic_db_gpu_tambah(VENDOR_ID_INTEL_GPU, 0x0102,
                              "Intel HD Graphics", "Intel", "HD Graphics 2000",
                              IC_RENTANG_LOW_END, 0, 850, 96);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_INTEL_GPU, 0x0116,
                              "Intel HD Graphics", "Intel", "HD Graphics 3000",
                              IC_RENTANG_LOW_END, 0, 1150, 144);
    
    /* Intel HD Graphics - Ivy Bridge */
    hasil = ic_db_gpu_tambah(VENDOR_ID_INTEL_GPU, 0x0166,
                              "Intel HD Graphics", "Intel", "HD Graphics 4000",
                              IC_RENTANG_MID_LOW, 0, 1150, 192);
    
    /* Intel HD Graphics - Haswell */
    hasil = ic_db_gpu_tambah(VENDOR_ID_INTEL_GPU, 0x0416,
                              "Intel HD Graphics", "Intel", "HD Graphics 4600",
                              IC_RENTANG_MID_LOW, 0, 1150, 288);
    
    /* Intel HD Graphics - Broadwell */
    hasil = ic_db_gpu_tambah(VENDOR_ID_INTEL_GPU, 0x1626,
                              "Intel HD Graphics", "Intel", "HD Graphics 5500",
                              IC_RENTANG_MID_LOW, 0, 850, 384);
    
    /* Intel HD Graphics - Skylake */
    hasil = ic_db_gpu_tambah(VENDOR_ID_INTEL_GPU, 0x1916,
                              "Intel HD Graphics", "Intel", "HD Graphics 520",
                              IC_RENTANG_MID_LOW, 0, 950, 384);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_INTEL_GPU, 0x191E,
                              "Intel HD Graphics", "Intel", "HD Graphics 530",
                              IC_RENTANG_MID_LOW, 0, 1050, 576);
    
    /* Intel UHD Graphics - Kaby Lake */
    hasil = ic_db_gpu_tambah(VENDOR_ID_INTEL_GPU, 0x5916,
                              "Intel UHD Graphics", "Intel", "UHD Graphics 620",
                              IC_RENTANG_MID_LOW, 0, 1050, 384);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_INTEL_GPU, 0x5912,
                              "Intel UHD Graphics", "Intel", "UHD Graphics 630",
                              IC_RENTANG_MID_LOW, 0, 1050, 576);
    
    /* Intel Iris Xe Graphics */
    hasil = ic_db_gpu_tambah(VENDOR_ID_INTEL_GPU, 0x9A49,
                              "Intel Iris Xe", "Intel", "Iris Xe Graphics",
                              IC_RENTANG_MID, 0, 1150, 768);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_INTEL_GPU, 0x4680,
                              "Intel Arc", "Intel", "Arc A380",
                              IC_RENTANG_MID, 6144, 2000, 1024);
    
    hasil = ic_db_gpu_tambah(VENDOR_ID_INTEL_GPU, 0x56A0,
                              "Intel Arc", "Intel", "Arc A770",
                              IC_RENTANG_MID_HIGH, 8192, 2100, 4096);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI UTAMA
 * ===========================================================================
 */

/*
 * ic_muat_gpu_database - Muat seluruh database GPU
 */
status_t ic_muat_gpu_database(void)
{
    /* NVIDIA GPU */
    ic_muat_nvidia_low_end();
    ic_muat_nvidia_mid_range();
    ic_muat_nvidia_high_end();
    
    /* AMD GPU */
    ic_muat_amd_gpu_database();
    
    /* Intel GPU */
    ic_muat_intel_gpu_database();
    
    return STATUS_BERHASIL;
}
