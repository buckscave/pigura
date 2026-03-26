/*
 * PIGURA OS - DATABASE/DB_ARM_CPU.C
 * ================================
 * Database ARM CPU Cores dari Cortex-A series.
 *
 * Versi: 1.0
 */

#include "../ic.h"

static status_t ic_db_arm_core_tambah(tak_bertanda16_t vendor_id,
                                       tak_bertanda16_t device_id,
                                       const char *nama,
                                       const char *vendor,
                                       const char *seri,
                                       ic_rentang_t rentang,
                                       tak_bertanda32_t cores,
                                       tak_bertanda32_t freq_mhz,
                                       tak_bertanda32_t cache_kb)
{
    ic_entri_t *entri;
    status_t hasil;
    
    entri = ic_entri_buat(nama, vendor, seri, IC_KATEGORI_CPU, rentang);
    if (entri == NULL) return STATUS_MEMORI_HABIS;
    
    entri->vendor_id = vendor_id;
    entri->device_id = device_id;
    entri->bus = IC_BUS_MMIO;
    
    hasil = ic_entri_tambah_param(entri, "frekuensi", IC_PARAM_TIPE_FREKUENSI, "MHz", 100, 5000);
    hasil = ic_entri_tambah_param(entri, "cores", IC_PARAM_TIPE_JUMLAH, "cores", 1, 256);
    
    return ic_database_tambah(entri);
}

status_t ic_muat_arm_cpu_database(void)
{
    status_t hasil;
    
    /* Cortex-A7xx series (Performance cores) */
    
    /* Cortex-A720 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x001, "Cortex-A720", "ARM", "A720",
                                   IC_RENTANG_HIGH_END, 1, 3500, 65536);
    
    /* Cortex-A715 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x002, "Cortex-A715", "ARM", "A715",
                                   IC_RENTANG_MID_HIGH, 1, 3200, 32768);
    
    /* Cortex-A710 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x003, "Cortex-A710", "ARM", "A710",
                                   IC_RENTANG_MID_HIGH, 1, 2900, 32768);
    
    /* Cortex-A78AE */
    hasil = ic_db_arm_core_tambah(0x41B, 0x004, "Cortex-A78AE", "ARM", "A78AE",
                                   IC_RENTANG_MID_HIGH, 1, 2800, 32768);
    
    /* Cortex-A78C */
    hasil = ic_db_arm_core_tambah(0x41B, 0x005, "Cortex-A78C", "ARM", "A78C",
                                   IC_RENTANG_MID_HIGH, 1, 3000, 32768);
    
    /* Cortex-A78 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x006, "Cortex-A78", "ARM", "A78",
                                   IC_RENTANG_MID_HIGH, 1, 3000, 32768);
    
    /* Cortex-A77 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x007, "Cortex-A77", "ARM", "A77",
                                   IC_RENTANG_MID, 1, 3000, 32768);
    
    /* Cortex-A76AE */
    hasil = ic_db_arm_core_tambah(0x41B, 0x008, "Cortex-A76AE", "ARM", "A76AE",
                                   IC_RENTANG_MID, 1, 2800, 24576);
    
    /* Cortex-A76 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x009, "Cortex-A76", "ARM", "A76",
                                   IC_RENTANG_MID, 1, 3000, 24576);
    
    /* Cortex-A75 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x00A, "Cortex-A75", "ARM", "A75",
                                   IC_RENTANG_MID, 1, 2800, 24576);
    
    /* Cortex-A73 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x00B, "Cortex-A73", "ARM", "A73",
                                   IC_RENTANG_MID_LOW, 1, 2600, 16384);
    
    /* Cortex-A72 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x00C, "Cortex-A72", "ARM", "A72",
                                   IC_RENTANG_MID_LOW, 1, 2400, 16384);
    
    /* Cortex-A71 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x00D, "Cortex-A71", "ARM", "A71",
                                   IC_RENTANG_MID_LOW, 1, 2200, 16384);
    
    /* Cortex-A57 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x00E, "Cortex-A57", "ARM", "A57",
                                   IC_RENTANG_MID_LOW, 1, 1900, 16384);
    
    /* Cortex-A53 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x00F, "Cortex-A53", "ARM", "A53",
                                   IC_RENTANG_LOW_END, 1, 1600, 8192);
    
    /* Cortex-A55 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x010, "Cortex-A55", "ARM", "A55",
                                   IC_RENTANG_LOW_END, 1, 1800, 8192);
    
    /* Cortex-A510 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x011, "Cortex-A510", "ARM", "A510",
                                   IC_RENTANG_MID_LOW, 1, 2000, 16384);
    
    /* Cortex-A520 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x012, "Cortex-A520", "ARM", "A520",
                                   IC_RENTANG_MID_LOW, 1, 2200, 16384);
    
    /* Cortex-A5 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x013, "Cortex-A5", "ARM", "A5",
                                   IC_RENTANG_LOW_END, 1, 600, 4096);
    
    /* Cortex-A7 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x014, "Cortex-A7", "ARM", "A7",
                                   IC_RENTANG_LOW_END, 1, 1500, 4096);
    
    /* Cortex-A8 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x015, "Cortex-A8", "ARM", "A8",
                                   IC_RENTANG_LOW_END, 1, 1000, 2048);
    
    /* Cortex-A9 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x016, "Cortex-A9", "ARM", "A9",
                                   IC_RENTANG_LOW_END, 1, 1400, 4096);
    
    /* Cortex-A12 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x017, "Cortex-A12", "ARM", "A12",
                                   IC_RENTANG_MID_LOW, 1, 1600, 8192);
    
    /* Cortex-A15 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x018, "Cortex-A15", "ARM", "A15",
                                   IC_RENTANG_MID, 1, 2000, 16384);
    
    /* Cortex-A17 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x019, "Cortex-A17", "ARM", "A17",
                                   IC_RENTANG_MID_LOW, 1, 1800, 8192);
    
    /* Cortex-A32 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x01A, "Cortex-A32", "ARM", "A32",
                                   IC_RENTANG_LOW_END, 1, 1200, 4096);
    
    /* Cortex-A35 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x01B, "Cortex-A35", "ARM", "A35",
                                   IC_RENTANG_LOW_END, 1, 1200, 4096);
    
    /* Neoverse series (Server) */
    
    /* Neoverse N2 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x020, "Neoverse N2", "ARM", "N2",
                                   IC_RENTANG_SERVER, 1, 3200, 65536);
    
    /* Neoverse V2 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x021, "Neoverse V2", "ARM", "V2",
                                   IC_RENTANG_SERVER, 1, 3500, 65536);
    
    /* Neoverse N1 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x022, "Neoverse N1", "ARM", "N1",
                                   IC_RENTANG_SERVER, 1, 3000, 32768);
    
    /* Neoverse E1 */
    hasil = ic_db_arm_core_tambah(0x41B, 0x023, "Neoverse E1", "ARM", "E1",
                                   IC_RENTANG_SERVER, 1, 2600, 16384);
    
    return STATUS_BERHASIL;
}
