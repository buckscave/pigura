/*
 * PIGURA OS - DATABASE/DB_CPU.C
 * =============================
 * Database CPU IC dari low-end hingga high-end.
 *
 * Berkas ini berisi database IC CPU Intel dan AMD dari:
 * - Intel: Pentium 1 hingga Core i7 terbaru
 * - AMD: K6 hingga Ryzen 9 terbaru
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../ic.h"

/*
 * ===========================================================================
 * KONSTANTA VENDOR ID
 * ===========================================================================
 */

#define VENDOR_ID_INTEL    0x8086
#define VENDOR_ID_AMD      0x1022
#define VENDOR_ID_VIA      0x1106
#define VENDOR_ID_HYGON    0x17D3

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * ic_db_cpu_tambah - Tambah entri CPU ke database
 */
static status_t ic_db_cpu_tambah(tak_bertanda16_t vendor_id,
                                  tak_bertanda16_t device_id,
                                  const char *nama,
                                  const char *vendor,
                                  const char *seri,
                                  ic_rentang_t rentang,
                                  tak_bertanda32_t frekuensi_min,
                                  tak_bertanda32_t frekuensi_max,
                                  tak_bertanda32_t core_min,
                                  tak_bertanda32_t core_max,
                                  tak_bertanda32_t cache_l2_kb)
{
    ic_entri_t *entri;
    ic_parameter_t *param;
    status_t hasil;
    
    entri = ic_buat_entri_cpu(vendor_id, device_id, nama, vendor, seri, rentang);
    if (entri == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    /* Set parameter frekuensi */
    param = ic_entri_cari_param(entri, "frekuensi_core");
    if (param != NULL) {
        param->nilai_min = frekuensi_min;
        param->nilai_max = frekuensi_max;
        param->nilai_typical = (frekuensi_min + frekuensi_max) / 2;
        param->nilai_default = ic_hitung_default(frekuensi_max, 
                                                  IC_TOLERANSI_DEFAULT_MAX);
    }
    
    /* Set parameter jumlah core */
    param = ic_entri_cari_param(entri, "jumlah_core");
    if (param != NULL) {
        param->nilai_min = core_min;
        param->nilai_max = core_max;
        param->nilai_typical = core_max;
        param->nilai_default = core_max;
    }
    
    /* Set parameter cache */
    param = ic_entri_cari_param(entri, "cache_l2");
    if (param != NULL) {
        param->nilai_min = cache_l2_kb;
        param->nilai_max = cache_l2_kb;
        param->nilai_typical = cache_l2_kb;
        param->nilai_default = cache_l2_kb;
    }
    
    /* Tambah ke database */
    hasil = ic_database_tambah(entri);
    
    return hasil;
}

/*
 * ===========================================================================
 * DATABASE INTEL CPU
 * ===========================================================================
 */

/*
 * ic_muat_intel_low_end - Muat database Intel low-end (Pentium, Celeron)
 */
static status_t ic_muat_intel_low_end(void)
{
    status_t hasil;
    
    /* Pentium 1 (P5) - 1993-1997 */
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x0500,
                              "Intel Pentium", "Intel", "Pentium 60",
                              IC_RENTANG_LOW_END, 60, 66, 1, 1, 0);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x0540,
                              "Intel Pentium", "Intel", "Pentium 75",
                              IC_RENTANG_LOW_END, 75, 90, 1, 1, 0);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x0570,
                              "Intel Pentium", "Intel", "Pentium 100",
                              IC_RENTANG_LOW_END, 100, 133, 1, 1, 0);
    
    /* Pentium MMX - 1997 */
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x0580,
                              "Intel Pentium MMX", "Intel", "Pentium MMX",
                              IC_RENTANG_LOW_END, 133, 233, 1, 1, 32);
    
    /* Pentium II - 1997-1999 */
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x0630,
                              "Intel Pentium II", "Intel", "Pentium II 233",
                              IC_RENTANG_LOW_END, 233, 300, 1, 1, 512);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x0633,
                              "Intel Pentium II", "Intel", "Pentium II 350",
                              IC_RENTANG_LOW_END, 350, 450, 1, 1, 512);
    
    /* Celeron - 1998+ */
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x0650,
                              "Intel Celeron", "Intel", "Celeron Mendocino",
                              IC_RENTANG_LOW_END, 300, 533, 1, 1, 128);
    
    /* Pentium III - 1999-2003 */
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x0670,
                              "Intel Pentium III", "Intel", "Pentium III 450",
                              IC_RENTANG_MID_LOW, 450, 600, 1, 1, 256);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x0680,
                              "Intel Pentium III", "Intel", "Pentium III 600",
                              IC_RENTANG_MID_LOW, 600, 866, 1, 1, 256);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x06A0,
                              "Intel Pentium III", "Intel", "Pentium III 800",
                              IC_RENTANG_MID_LOW, 800, 1133, 1, 1, 256);
    
    return STATUS_BERHASIL;
}

/*
 * ic_muat_intel_mid_range - Muat database Intel mid-range (Pentium 4, Core 2)
 */
static status_t ic_muat_intel_mid_range(void)
{
    status_t hasil;
    
    /* Pentium 4 - 2000-2006 */
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x0F00,
                              "Intel Pentium 4", "Intel", "Pentium 4 Willamette",
                              IC_RENTANG_MID_LOW, 1300, 2000, 1, 1, 256);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x0F20,
                              "Intel Pentium 4", "Intel", "Pentium 4 Northwood",
                              IC_RENTANG_MID, 1600, 3400, 1, 1, 512);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x0F40,
                              "Intel Pentium 4", "Intel", "Pentium 4 Prescott",
                              IC_RENTANG_MID, 2400, 3800, 1, 1, 1024);
    
    /* Pentium D (Dual Core) - 2005-2008 */
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x0F60,
                              "Intel Pentium D", "Intel", "Pentium D 800",
                              IC_RENTANG_MID, 2400, 3200, 2, 2, 2048);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x0F70,
                              "Intel Pentium D", "Intel", "Pentium D 900",
                              IC_RENTANG_MID, 2800, 3730, 2, 2, 4096);
    
    /* Core 2 Duo - 2006-2011 */
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x06E0,
                              "Intel Core 2 Duo", "Intel", "Core 2 Duo E4000",
                              IC_RENTANG_MID, 1600, 2000, 2, 2, 2048);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x06F0,
                              "Intel Core 2 Duo", "Intel", "Core 2 Duo E6000",
                              IC_RENTANG_MID_HIGH, 1860, 3000, 2, 2, 4096);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x0670,
                              "Intel Core 2 Quad", "Intel", "Core 2 Quad Q6000",
                              IC_RENTANG_MID_HIGH, 2400, 2800, 4, 4, 8192);
    
    return STATUS_BERHASIL;
}

/*
 * ic_muat_intel_high_end - Muat database Intel high-end (Core i3/i5/i7/i9)
 */
static status_t ic_muat_intel_high_end(void)
{
    status_t hasil;
    
    /* Core i3 - Generasi 1-4 */
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1A00,
                              "Intel Core i3", "Intel", "Core i3 500 series",
                              IC_RENTANG_MID, 2100, 3200, 2, 4, 3072);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1A10,
                              "Intel Core i3", "Intel", "Core i3 2000 series",
                              IC_RENTANG_MID, 2100, 3400, 2, 4, 3072);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1A20,
                              "Intel Core i3", "Intel", "Core i3 3000 series",
                              IC_RENTANG_MID_LOW, 2200, 3800, 2, 4, 3072);
    
    /* Core i5 - Generasi 1-4 */
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1A30,
                              "Intel Core i5", "Intel", "Core i5 600 series",
                              IC_RENTANG_MID_HIGH, 2400, 3400, 2, 4, 4096);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1A40,
                              "Intel Core i5", "Intel", "Core i5 2000 series",
                              IC_RENTANG_MID_HIGH, 2300, 3800, 4, 4, 6144);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1A50,
                              "Intel Core i5", "Intel", "Core i5 3000 series",
                              IC_RENTANG_MID_HIGH, 2500, 4000, 4, 4, 6144);
    
    /* Core i7 - Generasi 1-4 */
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1A60,
                              "Intel Core i7", "Intel", "Core i7 800 series",
                              IC_RENTANG_HIGH_END, 2600, 3400, 4, 8, 8192);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1A70,
                              "Intel Core i7", "Intel", "Core i7 2000 series",
                              IC_RENTANG_HIGH_END, 2600, 4200, 4, 8, 8192);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1A80,
                              "Intel Core i7", "Intel", "Core i7 3000 series",
                              IC_RENTANG_HIGH_END, 2700, 4600, 4, 8, 8192);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1A90,
                              "Intel Core i7", "Intel", "Core i7 4000 series",
                              IC_RENTANG_HIGH_END, 2800, 4500, 4, 8, 8192);
    
    /* Core i7 Generasi 6-10 */
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1B00,
                              "Intel Core i7", "Intel", "Core i7 6000 series",
                              IC_RENTANG_HIGH_END, 2800, 4200, 4, 8, 8192);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1B10,
                              "Intel Core i7", "Intel", "Core i7 7000 series",
                              IC_RENTANG_HIGH_END, 2900, 4500, 4, 8, 8192);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1B20,
                              "Intel Core i7", "Intel", "Core i7 8000 series",
                              IC_RENTANG_HIGH_END, 3000, 5000, 6, 12, 12288);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1B30,
                              "Intel Core i7", "Intel", "Core i7 9000 series",
                              IC_RENTANG_HIGH_END, 3100, 5200, 6, 12, 12288);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1B40,
                              "Intel Core i7", "Intel", "Core i7 10000 series",
                              IC_RENTANG_HIGH_END, 3200, 5300, 8, 16, 16384);
    
    /* Core i9 */
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1C00,
                              "Intel Core i9", "Intel", "Core i9 7000 series",
                              IC_RENTANG_ENTHUSIAST, 3000, 4500, 10, 20, 13824);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1C10,
                              "Intel Core i9", "Intel", "Core i9 9000 series",
                              IC_RENTANG_ENTHUSIAST, 3100, 5000, 8, 16, 16384);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1C20,
                              "Intel Core i9", "Intel", "Core i9 10000 series",
                              IC_RENTANG_ENTHUSIAST, 3200, 5300, 10, 20, 20480);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1C30,
                              "Intel Core i9", "Intel", "Core i9 11000 series",
                              IC_RENTANG_ENTHUSIAST, 3300, 5300, 8, 16, 16384);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1C40,
                              "Intel Core i9", "Intel", "Core i9 12000 series",
                              IC_RENTANG_ENTHUSIAST, 3400, 5800, 16, 24, 30720);
    
    /* Xeon Server */
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1D00,
                              "Intel Xeon", "Intel", "Xeon E3 series",
                              IC_RENTANG_SERVER, 1800, 4000, 4, 8, 8192);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1D10,
                              "Intel Xeon", "Intel", "Xeon E5 series",
                              IC_RENTANG_SERVER, 1600, 4500, 4, 22, 25600);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_INTEL, 0x1D20,
                              "Intel Xeon", "Intel", "Xeon E7 series",
                              IC_RENTANG_SERVER, 2000, 4800, 10, 28, 35840);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * DATABASE AMD CPU
 * ===========================================================================
 */

/*
 * ic_muat_amd_low_end - Muat database AMD low-end (K6, K7)
 */
static status_t ic_muat_amd_low_end(void)
{
    status_t hasil;
    
    /* AMD K6 - 1997-2000 */
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1560,
                              "AMD K6", "AMD", "K6 166",
                              IC_RENTANG_LOW_END, 166, 300, 1, 1, 64);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1570,
                              "AMD K6-2", "AMD", "K6-2 300",
                              IC_RENTANG_LOW_END, 266, 550, 1, 1, 64);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1580,
                              "AMD K6-III", "AMD", "K6-III 400",
                              IC_RENTANG_LOW_END, 400, 500, 1, 1, 256);
    
    /* AMD K7 (Athlon) - 1999-2005 */
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1600,
                              "AMD Athlon", "AMD", "Athlon Classic",
                              IC_RENTANG_MID_LOW, 500, 1400, 1, 1, 256);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1610,
                              "AMD Athlon XP", "AMD", "Athlon XP 1500+",
                              IC_RENTANG_MID_LOW, 1330, 2200, 1, 1, 256);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1620,
                              "AMD Athlon XP", "AMD", "Athlon XP 2800+",
                              IC_RENTANG_MID, 1800, 2333, 1, 1, 512);
    
    /* AMD Sempron - 2004-2014 */
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1630,
                              "AMD Sempron", "AMD", "Sempron 2800+",
                              IC_RENTANG_LOW_END, 1600, 2000, 1, 1, 128);
    
    return STATUS_BERHASIL;
}

/*
 * ic_muat_amd_mid_range - Muat database AMD mid-range (Athlon 64, Phenom)
 */
static status_t ic_muat_amd_mid_range(void)
{
    status_t hasil;
    
    /* AMD Athlon 64 - 2003-2010 */
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1680,
                              "AMD Athlon 64", "AMD", "Athlon 64 2800+",
                              IC_RENTANG_MID, 1800, 2400, 1, 1, 512);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1690,
                              "AMD Athlon 64", "AMD", "Athlon 64 3800+",
                              IC_RENTANG_MID, 2000, 2400, 1, 1, 512);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x16A0,
                              "AMD Athlon 64 X2", "AMD", "Athlon X2 3800+",
                              IC_RENTANG_MID, 2000, 2800, 2, 2, 1024);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x16B0,
                              "AMD Athlon 64 X2", "AMD", "Athlon X2 6000+",
                              IC_RENTANG_MID_HIGH, 3000, 3200, 2, 2, 2048);
    
    /* AMD Phenom - 2007-2012 */
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1700,
                              "AMD Phenom", "AMD", "Phenom X3",
                              IC_RENTANG_MID, 2100, 2500, 3, 3, 2048);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1710,
                              "AMD Phenom II", "AMD", "Phenom II X4",
                              IC_RENTANG_MID_HIGH, 2500, 3700, 4, 4, 6144);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1720,
                              "AMD Phenom II", "AMD", "Phenom II X6",
                              IC_RENTANG_MID_HIGH, 2300, 3300, 6, 6, 9216);
    
    /* AMD FX - 2011-2017 */
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1730,
                              "AMD FX", "AMD", "FX-4100",
                              IC_RENTANG_MID, 3600, 4200, 4, 4, 8192);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1740,
                              "AMD FX", "AMD", "FX-6300",
                              IC_RENTANG_MID_HIGH, 3500, 4300, 6, 6, 8192);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1750,
                              "AMD FX", "AMD", "FX-8350",
                              IC_RENTANG_MID_HIGH, 4000, 4700, 8, 8, 8192);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1760,
                              "AMD FX", "AMD", "FX-9590",
                              IC_RENTANG_HIGH_END, 4700, 5000, 8, 8, 8192);
    
    return STATUS_BERHASIL;
}

/*
 * ic_muat_amd_high_end - Muat database AMD high-end (Ryzen)
 */
static status_t ic_muat_amd_high_end(void)
{
    status_t hasil;
    
    /* AMD Ryzen 3 - 1000-5000 series */
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1800,
                              "AMD Ryzen 3", "AMD", "Ryzen 3 1200",
                              IC_RENTANG_MID, 3100, 3400, 4, 4, 8192);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1810,
                              "AMD Ryzen 3", "AMD", "Ryzen 3 2200G",
                              IC_RENTANG_MID, 3500, 3700, 4, 4, 8192);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1820,
                              "AMD Ryzen 3", "AMD", "Ryzen 3 3100",
                              IC_RENTANG_MID, 3600, 3900, 4, 8, 16384);
    
    /* AMD Ryzen 5 - 1000-7000 series */
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1900,
                              "AMD Ryzen 5", "AMD", "Ryzen 5 1600",
                              IC_RENTANG_MID_HIGH, 3200, 3600, 6, 12, 16384);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1910,
                              "AMD Ryzen 5", "AMD", "Ryzen 5 2600",
                              IC_RENTANG_MID_HIGH, 3400, 3900, 6, 12, 16384);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1920,
                              "AMD Ryzen 5", "AMD", "Ryzen 5 3600",
                              IC_RENTANG_MID_HIGH, 3600, 4200, 6, 12, 32768);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1930,
                              "AMD Ryzen 5", "AMD", "Ryzen 5 5600",
                              IC_RENTANG_MID_HIGH, 3700, 4500, 6, 12, 32768);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1940,
                              "AMD Ryzen 5", "AMD", "Ryzen 5 7600",
                              IC_RENTANG_HIGH_END, 3800, 5100, 6, 12, 32768);
    
    /* AMD Ryzen 7 - 1000-7000 series */
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1A00,
                              "AMD Ryzen 7", "AMD", "Ryzen 7 1700",
                              IC_RENTANG_HIGH_END, 3000, 3750, 8, 16, 16384);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1A10,
                              "AMD Ryzen 7", "AMD", "Ryzen 7 2700",
                              IC_RENTANG_HIGH_END, 3200, 4100, 8, 16, 16384);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1A20,
                              "AMD Ryzen 7", "AMD", "Ryzen 7 3700",
                              IC_RENTANG_HIGH_END, 3600, 4400, 8, 16, 32768);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1A30,
                              "AMD Ryzen 7", "AMD", "Ryzen 7 5800",
                              IC_RENTANG_HIGH_END, 3800, 4700, 8, 16, 32768);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1A40,
                              "AMD Ryzen 7", "AMD", "Ryzen 7 7700",
                              IC_RENTANG_HIGH_END, 3800, 5350, 8, 16, 32768);
    
    /* AMD Ryzen 9 - 3000-7000 series */
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1B00,
                              "AMD Ryzen 9", "AMD", "Ryzen 9 3900X",
                              IC_RENTANG_ENTHUSIAST, 3800, 4500, 12, 24, 65536);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1B10,
                              "AMD Ryzen 9", "AMD", "Ryzen 9 5900X",
                              IC_RENTANG_ENTHUSIAST, 3700, 4800, 12, 24, 65536);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1B20,
                              "AMD Ryzen 9", "AMD", "Ryzen 9 5950X",
                              IC_RENTANG_ENTHUSIAST, 3400, 4900, 16, 32, 65536);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1B30,
                              "AMD Ryzen 9", "AMD", "Ryzen 9 7900X",
                              IC_RENTANG_ENTHUSIAST, 4000, 5600, 12, 24, 65536);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1B40,
                              "AMD Ryzen 9", "AMD", "Ryzen 9 7950X",
                              IC_RENTANG_ENTHUSIAST, 4500, 5700, 16, 32, 65536);
    
    /* AMD Threadripper */
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1C00,
                              "AMD Threadripper", "AMD", "Threadripper 1950X",
                              IC_RENTANG_SERVER, 3200, 4000, 16, 32, 32768);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1C10,
                              "AMD Threadripper", "AMD", "Threadripper 3970X",
                              IC_RENTANG_SERVER, 3700, 4500, 32, 64, 131072);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1C20,
                              "AMD Threadripper", "AMD", "Threadripper PRO 5995WX",
                              IC_RENTANG_SERVER, 2700, 4500, 64, 128, 262144);
    
    /* AMD EPYC Server */
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1D00,
                              "AMD EPYC", "AMD", "EPYC 7001 series",
                              IC_RENTANG_SERVER, 2000, 3200, 8, 32, 65536);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1D10,
                              "AMD EPYC", "AMD", "EPYC 7002 series",
                              IC_RENTANG_SERVER, 2300, 3400, 8, 64, 131072);
    
    hasil = ic_db_cpu_tambah(VENDOR_ID_AMD, 0x1D20,
                              "AMD EPYC", "AMD", "EPYC 7003 series",
                              IC_RENTANG_SERVER, 2500, 3800, 8, 64, 262144);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI UTAMA
 * ===========================================================================
 */

/*
 * ic_muat_cpu_database - Muat seluruh database CPU
 */
status_t ic_muat_cpu_database(void)
{
    /* Intel CPU */
    ic_muat_intel_low_end();
    ic_muat_intel_mid_range();
    ic_muat_intel_high_end();
    
    /* AMD CPU */
    ic_muat_amd_low_end();
    ic_muat_amd_mid_range();
    ic_muat_amd_high_end();
    
    return STATUS_BERHASIL;
}
