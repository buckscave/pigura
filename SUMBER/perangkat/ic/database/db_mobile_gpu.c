/*
 * PIGURA OS - DATABASE/DB_MOBILE_GPU.C
 * =====================================
 * Database Mobile GPU (Adreno, Mali, PowerVR, Apple GPU)
 *
 * Versi: 1.0
 */

#include "../ic.h"

static status_t ic_db_mobile_gpu_tambah(tak_bertanda16_t vendor_id,
                                         tak_bertanda16_t device_id,
                                         const char *nama,
                                         const char *vendor,
                                         const char *seri,
                                         ic_rentang_t rentang,
                                         tak_bertanda32_t shader_cores,
                                         tak_bertanda32_t freq_mhz)
{
    ic_entri_t *entri;
    status_t hasil;
    
    entri = ic_entri_buat(nama, vendor, seri, IC_KATEGORI_GPU, rentang);
    if (entri == NULL) return STATUS_MEMORI_HABIS;
    
    entri->vendor_id = vendor_id;
    entri->device_id = device_id;
    entri->bus = IC_BUS_MMIO;
    
    hasil = ic_entri_tambah_param(entri, "shader_cores", IC_PARAM_TIPE_JUMLAH, "units", 1, 4096);
    hasil = ic_entri_tambah_param(entri, "core_clock", IC_PARAM_TIPE_FREKUENSI, "MHz", 100, 2000);
    
    return ic_database_tambah(entri);
}

/* Vendor IDs */
#define VENDOR_QUALCOMM      0x17CB
#define VENDOR_ARM_MALI      0x13B5
#define VENDOR_IMGTEC        0x1063
#define VENDOR_APPLE         0x106B
#define VENDOR_VIVANTE       0x1D13
#define VENDOR_NVIDIA_TEGRA  0x10DE

status_t ic_muat_mobile_gpu_database(void)
{
    status_t hasil;
    
    /* ===== QUALCOMM ADRENO ===== */
    
    /* Adreno 8-series (New flagship) */
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x8001, "Adreno 830", "Qualcomm", "Adreno 830",
                                     IC_RENTANG_ENTHUSIAST, 3072, 1000);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x8002, "Adreno 750", "Qualcomm", "Adreno 750",
                                     IC_RENTANG_HIGH_END, 2304, 900);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x8003, "Adreno 740", "Qualcomm", "Adreno 740",
                                     IC_RENTANG_HIGH_END, 2048, 900);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x8004, "Adreno 730", "Qualcomm", "Adreno 730",
                                     IC_RENTANG_HIGH_END, 1792, 818);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x8005, "Adreno 710", "Qualcomm", "Adreno 710",
                                     IC_RENTANG_MID_HIGH, 1024, 750);
    
    /* Adreno 6-series */
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x8006, "Adreno 690", "Qualcomm", "Adreno 690",
                                     IC_RENTANG_HIGH_END, 1024, 850);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x8007, "Adreno 680", "Qualcomm", "Adreno 680",
                                     IC_RENTANG_MID_HIGH, 768, 850);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x8008, "Adreno 675", "Qualcomm", "Adreno 675",
                                     IC_RENTANG_MID, 512, 600);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x8009, "Adreno 670", "Qualcomm", "Adreno 670",
                                     IC_RENTANG_MID, 768, 750);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x800A, "Adreno 665", "Qualcomm", "Adreno 665",
                                     IC_RENTANG_MID, 512, 700);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x800B, "Adreno 660", "Qualcomm", "Adreno 660",
                                     IC_RENTANG_MID_HIGH, 1024, 840);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x800C, "Adreno 650", "Qualcomm", "Adreno 650",
                                     IC_RENTANG_MID, 1024, 670);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x800D, "Adreno 646", "Qualcomm", "Adreno 646",
                                     IC_RENTANG_MID_LOW, 384, 700);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x800E, "Adreno 644", "Qualcomm", "Adreno 644",
                                     IC_RENTANG_MID_LOW, 256, 700);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x800F, "Adreno 642L", "Qualcomm", "Adreno 642L",
                                     IC_RENTANG_MID_LOW, 384, 550);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x8010, "Adreno 642", "Qualcomm", "Adreno 642",
                                     IC_RENTANG_MID_LOW, 448, 550);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x8011, "Adreno 640", "Qualcomm", "Adreno 640",
                                     IC_RENTANG_MID, 768, 585);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x8012, "Adreno 630", "Qualcomm", "Adreno 630",
                                     IC_RENTANG_MID, 512, 710);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x8013, "Adreno 620", "Qualcomm", "Adreno 620",
                                     IC_RENTANG_MID_LOW, 384, 650);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x8014, "Adreno 619", "Qualcomm", "Adreno 619",
                                     IC_RENTANG_LOW_END, 256, 700);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x8015, "Adreno 618", "Qualcomm", "Adreno 618",
                                     IC_RENTANG_MID_LOW, 512, 700);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x8016, "Adreno 616", "Qualcomm", "Adreno 616",
                                     IC_RENTANG_MID_LOW, 256, 700);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x8017, "Adreno 615", "Qualcomm", "Adreno 615",
                                     IC_RENTANG_LOW_END, 192, 700);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x8018, "Adreno 612", "Qualcomm", "Adreno 612",
                                     IC_RENTANG_LOW_END, 256, 650);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x8019, "Adreno 610", "Qualcomm", "Adreno 610",
                                     IC_RENTANG_LOW_END, 128, 600);
    
    /* Adreno 5-series */
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x801A, "Adreno 540", "Qualcomm", "Adreno 540",
                                     IC_RENTANG_MID, 512, 710);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x801B, "Adreno 530", "Qualcomm", "Adreno 530",
                                     IC_RENTANG_MID_LOW, 512, 624);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x801C, "Adreno 510", "Qualcomm", "Adreno 510",
                                     IC_RENTANG_MID_LOW, 192, 600);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x801D, "Adreno 509", "Qualcomm", "Adreno 509",
                                     IC_RENTANG_LOW_END, 128, 600);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x801E, "Adreno 508", "Qualcomm", "Adreno 508",
                                     IC_RENTANG_LOW_END, 128, 600);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x801F, "Adreno 506", "Qualcomm", "Adreno 506",
                                     IC_RENTANG_LOW_END, 192, 650);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x8020, "Adreno 505", "Qualcomm", "Adreno 505",
                                     IC_RENTANG_LOW_END, 128, 450);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_QUALCOMM, 0x8021, "Adreno 504", "Qualcomm", "Adreno 504",
                                     IC_RENTANG_LOW_END, 96, 450);
    
    /* ===== ARM MALI ===== */
    
    /* Mali Immortalis (Flagship) */
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x9001, "Immortalis-G720", "ARM", "Immortalis-G720",
                                     IC_RENTANG_HIGH_END, 2048, 950);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x9002, "Immortalis-G715", "ARM", "Immortalis-G715",
                                     IC_RENTANG_HIGH_END, 1536, 950);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x9003, "Immortalis-G711", "ARM", "Immortalis-G711",
                                     IC_RENTANG_MID_HIGH, 1024, 850);
    
    /* Mali Valhall */
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x9004, "Mali-G715", "ARM", "Mali-G715",
                                     IC_RENTANG_HIGH_END, 1536, 900);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x9005, "Mali-G710", "ARM", "Mali-G710",
                                     IC_RENTANG_HIGH_END, 1536, 850);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x9006, "Mali-G710 MC7", "ARM", "Mali-G710 MC7",
                                     IC_RENTANG_MID_HIGH, 896, 750);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x9007, "Mali-G615", "ARM", "Mali-G615",
                                     IC_RENTANG_MID, 1024, 800);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x9008, "Mali-G610", "ARM", "Mali-G610",
                                     IC_RENTANG_MID, 1024, 750);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x9009, "Mali-G610 MC4", "ARM", "Mali-G610 MC4",
                                     IC_RENTANG_MID_LOW, 512, 700);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x900A, "Mali-G510", "ARM", "Mali-G510",
                                     IC_RENTANG_MID_LOW, 512, 700);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x900B, "Mali-G57", "ARM", "Mali-G57",
                                     IC_RENTANG_MID_LOW, 512, 850);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x900C, "Mali-G57 MC2", "ARM", "Mali-G57 MC2",
                                     IC_RENTANG_LOW_END, 256, 700);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x900D, "Mali-G57 MC1", "ARM", "Mali-G57 MC1",
                                     IC_RENTANG_LOW_END, 128, 650);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x900E, "Mali-G57 MC3", "ARM", "Mali-G57 MC3",
                                     IC_RENTANG_MID_LOW, 384, 750);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x900F, "Mali-G68", "ARM", "Mali-G68",
                                     IC_RENTANG_MID, 768, 800);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x9010, "Mali-G78", "ARM", "Mali-G78",
                                     IC_RENTANG_MID_HIGH, 1024, 800);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x9011, "Mali-G77", "ARM", "Mali-G77",
                                     IC_RENTANG_MID, 1024, 800);
    
    /* Mali Bifrost */
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x9012, "Mali-G76", "ARM", "Mali-G76",
                                     IC_RENTANG_MID, 768, 800);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x9013, "Mali-G72", "ARM", "Mali-G72",
                                     IC_RENTANG_MID_LOW, 512, 750);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x9014, "Mali-G71", "ARM", "Mali-G71",
                                     IC_RENTANG_MID_LOW, 512, 800);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x9015, "Mali-G52", "ARM", "Mali-G52",
                                     IC_RENTANG_MID_LOW, 256, 750);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x9016, "Mali-G52 MC2", "ARM", "Mali-G52 MC2",
                                     IC_RENTANG_LOW_END, 128, 700);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x9017, "Mali-G52 MC1", "ARM", "Mali-G52 MC1",
                                     IC_RENTANG_LOW_END, 64, 650);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x9018, "Mali-G51", "ARM", "Mali-G51",
                                     IC_RENTANG_LOW_END, 192, 700);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x9019, "Mali-G31", "ARM", "Mali-G31",
                                     IC_RENTANG_LOW_END, 64, 650);
    
    /* Mali Midgard */
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x901A, "Mali-T880", "ARM", "Mali-T880",
                                     IC_RENTANG_MID_LOW, 256, 800);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x901B, "Mali-T860", "ARM", "Mali-T860",
                                     IC_RENTANG_MID_LOW, 256, 700);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x901C, "Mali-T830", "ARM", "Mali-T830",
                                     IC_RENTANG_LOW_END, 128, 700);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x901D, "Mali-T820", "ARM", "Mali-T820",
                                     IC_RENTANG_LOW_END, 128, 650);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x901E, "Mali-T760", "ARM", "Mali-T760",
                                     IC_RENTANG_MID_LOW, 256, 700);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x901F, "Mali-T720", "ARM", "Mali-T720",
                                     IC_RENTANG_LOW_END, 128, 600);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x9020, "Mali-T628", "ARM", "Mali-T628",
                                     IC_RENTANG_LOW_END, 128, 600);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x9021, "Mali-T624", "ARM", "Mali-T624",
                                     IC_RENTANG_LOW_END, 64, 550);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_ARM_MALI, 0x9022, "Mali-T604", "ARM", "Mali-T604",
                                     IC_RENTANG_LOW_END, 64, 500);
    
    /* ===== POWERVR (IMGTEC) ===== */
    
    hasil = ic_db_mobile_gpu_tambah(VENDOR_IMGTEC, 0xA001, "PowerVR BXM-8-256", "Imagination", "BXM-8-256",
                                     IC_RENTANG_MID_LOW, 256, 800);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_IMGTEC, 0xA002, "PowerVR BXM-4-64", "Imagination", "BXM-4-64",
                                     IC_RENTANG_LOW_END, 64, 650);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_IMGTEC, 0xA003, "PowerVR BXS-4-64", "Imagination", "BXS-4-64",
                                     IC_RENTANG_LOW_END, 64, 600);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_IMGTEC, 0xA004, "PowerVR Rogue G6430", "Imagination", "G6430",
                                     IC_RENTANG_MID, 128, 480);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_IMGTEC, 0xA005, "PowerVR Rogue G6200", "Imagination", "G6200",
                                     IC_RENTANG_MID_LOW, 64, 500);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_IMGTEC, 0xA006, "PowerVR GE8320", "Imagination", "GE8320",
                                     IC_RENTANG_LOW_END, 64, 660);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_IMGTEC, 0xA007, "PowerVR GE8310", "Imagination", "GE8310",
                                     IC_RENTANG_LOW_END, 32, 600);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_IMGTEC, 0xA008, "PowerVR GE8300", "Imagination", "GE8300",
                                     IC_RENTANG_LOW_END, 32, 500);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_IMGTEC, 0xA009, "PowerVR SGX544", "Imagination", "SGX544",
                                     IC_RENTANG_LOW_END, 64, 384);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_IMGTEC, 0xA00A, "PowerVR SGX543", "Imagination", "SGX543",
                                     IC_RENTANG_LOW_END, 64, 304);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_IMGTEC, 0xA00B, "PowerVR SGX540", "Imagination", "SGX540",
                                     IC_RENTANG_LOW_END, 32, 200);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_IMGTEC, 0xA00C, "PowerVR SGX535", "Imagination", "SGX535",
                                     IC_RENTANG_LOW_END, 32, 200);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_IMGTEC, 0xA00D, "PowerVR SGX530", "Imagination", "SGX530",
                                     IC_RENTANG_LOW_END, 16, 200);
    
    /* ===== APPLE GPU ===== */
    
    hasil = ic_db_mobile_gpu_tambah(VENDOR_APPLE, 0xB001, "Apple GPU 15-core", "Apple", "M3 GPU",
                                     IC_RENTANG_HIGH_END, 1024, 1300);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_APPLE, 0xB002, "Apple GPU 10-core", "Apple", "M2 GPU",
                                     IC_RENTANG_MID_HIGH, 768, 1200);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_APPLE, 0xB003, "Apple GPU 8-core", "Apple", "M1 GPU",
                                     IC_RENTANG_MID, 512, 1000);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_APPLE, 0xB004, "Apple GPU 7-core", "Apple", "A17 Pro",
                                     IC_RENTANG_MID_HIGH, 768, 1200);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_APPLE, 0xB005, "Apple GPU 6-core", "Apple", "A16",
                                     IC_RENTANG_MID, 512, 1100);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_APPLE, 0xB006, "Apple GPU 5-core", "Apple", "A15",
                                     IC_RENTANG_MID, 384, 1000);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_APPLE, 0xB007, "Apple GPU 4-core", "Apple", "A14",
                                     IC_RENTANG_MID_LOW, 256, 900);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_APPLE, 0xB008, "Apple GPU 3-core", "Apple", "A13",
                                     IC_RENTANG_MID_LOW, 192, 800);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_APPLE, 0xB009, "Apple A12 GPU", "Apple", "A12 GPU",
                                     IC_RENTANG_MID_LOW, 128, 750);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_APPLE, 0xB00A, "Apple A11 GPU", "Apple", "A11 GPU",
                                     IC_RENTANG_LOW_END, 96, 700);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_APPLE, 0xB00B, "Apple A10 GPU", "Apple", "A10 GPU",
                                     IC_RENTANG_LOW_END, 64, 650);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_APPLE, 0xB00C, "Apple A9 GPU", "Apple", "A9 GPU",
                                     IC_RENTANG_LOW_END, 48, 600);
    
    /* ===== NVIDIA TEGRA ===== */
    
    hasil = ic_db_mobile_gpu_tambah(VENDOR_NVIDIA_TEGRA, 0xC001, "GeForce ULP 72-core", "NVIDIA", "Tegra X1",
                                     IC_RENTANG_MID, 256, 1000);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_NVIDIA_TEGRA, 0xC002, "GeForce ULP 192-core", "NVIDIA", "Tegra K1",
                                     IC_RENTANG_MID, 192, 950);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_NVIDIA_TEGRA, 0xC003, "GeForce ULP 16-core", "NVIDIA", "Tegra 4",
                                     IC_RENTANG_MID_LOW, 72, 720);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_NVIDIA_TEGRA, 0xC004, "GeForce ULP 12-core", "NVIDIA", "Tegra 3",
                                     IC_RENTANG_LOW_END, 48, 520);
    hasil = ic_db_mobile_gpu_tambah(VENDOR_NVIDIA_TEGRA, 0xC005, "GeForce ULP 8-core", "NVIDIA", "Tegra 2",
                                     IC_RENTANG_LOW_END, 32, 400);
    
    return STATUS_BERHASIL;
}
