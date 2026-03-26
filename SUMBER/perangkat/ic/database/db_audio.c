/*
 * PIGURA OS - DATABASE/DB_AUDIO.C
 * ===============================
 * Database Audio IC dari low-end hingga high-end.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../ic.h"

/*
 * ===========================================================================
 * VENDOR ID AUDIO
 * ===========================================================================
 */

#define VENDOR_ID_INTEL_AUDIO  0x8086
#define VENDOR_ID_NVIDIA_AUDIO 0x10DE
#define VENDOR_ID_AMD_AUDIO    0x1002
#define VENDOR_ID_CREATIVE     0x1102
#define VENDOR_ID_REALTEK_AUDIO 0x10EC

/*
 * ===========================================================================
 * FUNGSI UTAMA
 * ===========================================================================
 */

status_t ic_muat_audio_database(void)
{
    ic_entri_t *entri;
    ic_parameter_t *param;
    status_t hasil;
    
    /* Intel HDA Controllers */
    entri = ic_entri_buat("Intel HDA", "Intel", "ICH6 HDA",
                          IC_KATEGORI_AUDIO, IC_RENTANG_MID_LOW);
    if (entri != NULL) {
        entri->vendor_id = VENDOR_ID_INTEL_AUDIO;
        entri->device_id = 0x2668;
        entri->bus = IC_BUS_PCI;
        
        hasil = ic_parameter_inisialisasi_kategori(entri);
        if (hasil == STATUS_BERHASIL) {
            param = ic_entri_cari_param(entri, "jumlah_channel");
            if (param != NULL) {
                param->nilai_min = 2;
                param->nilai_max = 8;
                param->nilai_typical = 6;
                param->nilai_default = 6;
            }
            ic_database_tambah(entri);
        }
    }
    
    /* Intel HDA - Series 7 */
    entri = ic_entri_buat("Intel HDA", "Intel", "Series 7 HDA",
                          IC_KATEGORI_AUDIO, IC_RENTANG_MID);
    if (entri != NULL) {
        entri->vendor_id = VENDOR_ID_INTEL_AUDIO;
        entri->device_id = 0x8C20;
        entri->bus = IC_BUS_PCI;
        hasil = ic_parameter_inisialisasi_kategori(entri);
        if (hasil == STATUS_BERHASIL) {
            ic_database_tambah(entri);
        }
    }
    
    /* Intel HDA - Series 200 */
    entri = ic_entri_buat("Intel HDA", "Intel", "Series 200 HDA",
                          IC_KATEGORI_AUDIO, IC_RENTANG_MID_HIGH);
    if (entri != NULL) {
        entri->vendor_id = VENDOR_ID_INTEL_AUDIO;
        entri->device_id = 0xA170;
        entri->bus = IC_BUS_PCI;
        hasil = ic_parameter_inisialisasi_kategori(entri);
        if (hasil == STATUS_BERHASIL) {
            ic_database_tambah(entri);
        }
    }
    
    /* NVIDIA HDA */
    entri = ic_entri_buat("NVIDIA HDA", "NVIDIA", "GK104 HDA",
                          IC_KATEGORI_AUDIO, IC_RENTANG_MID_HIGH);
    if (entri != NULL) {
        entri->vendor_id = VENDOR_ID_NVIDIA_AUDIO;
        entri->device_id = 0x0E0B;
        entri->bus = IC_BUS_PCI;
        hasil = ic_parameter_inisialisasi_kategori(entri);
        if (hasil == STATUS_BERHASIL) {
            ic_database_tambah(entri);
        }
    }
    
    /* AMD HDA */
    entri = ic_entri_buat("AMD HDA", "AMD", "RV770 HDA",
                          IC_KATEGORI_AUDIO, IC_RENTANG_MID);
    if (entri != NULL) {
        entri->vendor_id = VENDOR_ID_AMD_AUDIO;
        entri->device_id = 0xAA00;
        entri->bus = IC_BUS_PCI;
        hasil = ic_parameter_inisialisasi_kategori(entri);
        if (hasil == STATUS_BERHASIL) {
            ic_database_tambah(entri);
    }
    }
    
    /* AMD HDA - Modern */
    entri = ic_entri_buat("AMD HDA", "AMD", "Renoir HDA",
                          IC_KATEGORI_AUDIO, IC_RENTANG_HIGH_END);
    if (entri != NULL) {
        entri->vendor_id = VENDOR_ID_AMD_AUDIO;
        entri->device_id = 0x15E3;
        entri->bus = IC_BUS_PCI;
        hasil = ic_parameter_inisialisasi_kategori(entri);
        if (hasil == STATUS_BERHASIL) {
            ic_database_tambah(entri);
        }
    }
    
    return STATUS_BERHASIL;
}
