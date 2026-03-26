/*
 * PIGURA OS - IC_KLASIFIKASI.C
 * ============================
 * Sistem klasifikasi IC berdasarkan karakteristik hardware.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi untuk mengklasifikasikan
 * IC ke dalam kategori dan rentang kinerja yang sesuai.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "ic.h"

/*
 * ===========================================================================
 * KONSTANTA KLASIFIKASI
 * ===========================================================================
 */

/* Batas untuk menentukan rentang kinerja CPU */
#define CPU_FREKUENSI_LOW_END      800     /* MHz */
#define CPU_FREKUENSI_MID_LOW      1500    /* MHz */
#define CPU_FREKUENSI_MID          2500    /* MHz */
#define CPU_FREKUENSI_MID_HIGH     3500    /* MHz */
#define CPU_FREKUENSI_HIGH_END     4500    /* MHz */
#define CPU_FREKUENSI_ENTHUSIAST   5500    /* MHz */

/* Batas untuk GPU */
#define GPU_MEM_LOW_END            128     /* MB */
#define GPU_MEM_MID_LOW            512     /* MB */
#define GPU_MEM_MID                2048    /* MB */
#define GPU_MEM_MID_HIGH           4096    /* MB */
#define GPU_MEM_HIGH_END           8192    /* MB */
#define GPU_MEM_ENTHUSIAST         16384   /* MB */

/* Batas untuk RAM */
#define RAM_KAPASITAS_LOW_END      64      /* MB */
#define RAM_KAPASITAS_MID_LOW      256     /* MB */
#define RAM_KAPASITAS_MID          1024    /* MB */
#define RAM_KAPASITAS_MID_HIGH     4096    /* MB */
#define RAM_KAPASITAS_HIGH_END     16384   /* MB */
#define RAM_KAPASITAS_ENTHUSIAST   65536   /* MB */

/* Batas untuk Storage */
#define STORAGE_KAPASITAS_LOW_END  128     /* MB */
#define STORAGE_KAPASITAS_MID_LOW  1024    /* MB */
#define STORAGE_KAPASITAS_MID      32768   /* MB */
#define STORAGE_KAPASITAS_MID_HIGH 262144  /* MB (256 GB) */
#define STORAGE_KAPASITAS_HIGH_END 1048576 /* MB (1 TB) */

/* PCI Class codes */
#define PCI_CLASS_STORAGE_SCSI     0x0000
#define PCI_CLASS_STORAGE_IDE      0x0101
#define PCI_CLASS_STORAGE_FLOPPY   0x0102
#define PCI_CLASS_STORAGE_IPI      0x0103
#define PCI_CLASS_STORAGE_RAID     0x0104
#define PCI_CLASS_STORAGE_ATA      0x0105
#define PCI_CLASS_STORAGE_SATA     0x0106
#define PCI_CLASS_STORAGE_SAS      0x0107
#define PCI_CLASS_STORAGE_NVME     0x0108

#define PCI_CLASS_NETWORK_ETHERNET 0x0200
#define PCI_CLASS_NETWORK_TOKEN    0x0201
#define PCI_CLASS_NETWORK_FDDI     0x0202
#define PCI_CLASS_NETWORK_ATM      0x0203
#define PCI_CLASS_NETWORK_ISDN     0x0204
#define PCI_CLASS_NETWORK_WORLDFIP 0x0205
#define PCI_CLASS_NETWORK_PICMG    0x0206
#define PCI_CLASS_NETWORK_INFINIBAND 0x0207
#define PCI_CLASS_NETWORK_FABRIC   0x0208

#define PCI_CLASS_DISPLAY_VGA      0x0300
#define PCI_CLASS_DISPLAY_XGA      0x0301
#define PCI_CLASS_DISPLAY_3D       0x0302
#define PCI_CLASS_DISPLAY_OTHER    0x0380

#define PCI_CLASS_MULTIMEDIA_AUDIO 0x0401
#define PCI_CLASS_MULTIMEDIA_PHONE 0x0402
#define PCI_CLASS_MULTIMEDIA_HDA   0x0403

/*
 * ===========================================================================
 * FUNGSI KLASIFIKASI KATEGORI
 * ===========================================================================
 */

/*
 * ic_klasifikasi_class_code - Klasifikasi berdasarkan PCI class code
 */
ic_kategori_t ic_klasifikasi_class_code(tak_bertanda32_t class_code)
{
    tak_bertanda8_t class_base;
    tak_bertanda8_t class_sub;
    
    class_base = (tak_bertanda8_t)((class_code >> 16) & 0xFF);
    class_sub = (tak_bertanda8_t)((class_code >> 8) & 0xFF);
    
    switch (class_base) {
    case 0x01: /* Mass Storage Controller */
        return IC_KATEGORI_STORAGE;
        
    case 0x02: /* Network Controller */
        return IC_KATEGORI_NETWORK;
        
    case 0x03: /* Display Controller */
        return IC_KATEGORI_GPU;
        
    case 0x04: /* Multimedia Controller */
        if (class_sub == 0x01 || class_sub == 0x03) {
            return IC_KATEGORI_AUDIO;
        }
        return IC_KATEGORI_LAINNYA;
        
    case 0x05: /* Memory Controller */
        return IC_KATEGORI_RAM;
        
    case 0x06: /* Bridge Device */
        return IC_KATEGORI_BRIDGE;
        
    case 0x07: /* Communication Controller */
        return IC_KATEGORI_LAINNYA;
        
    case 0x08: /* Generic System Peripheral */
        return IC_KATEGORI_LAINNYA;
        
    case 0x09: /* Input Device Controller */
        return IC_KATEGORI_INPUT;
        
    case 0x0A: /* Docking Station */
        return IC_KATEGORI_LAINNYA;
        
    case 0x0B: /* Processor */
        return IC_KATEGORI_CPU;
        
    case 0x0C: /* Serial Bus Controller */
        if (class_sub == 0x03) {
            return IC_KATEGORI_USB;
        }
        return IC_KATEGORI_LAINNYA;
        
    case 0x0D: /* Wireless Controller */
        return IC_KATEGORI_NETWORK;
        
    case 0x0E: /* Intelligent I/O Controller */
        return IC_KATEGORI_LAINNYA;
        
    case 0x0F: /* Satellite Communication Controller */
        return IC_KATEGORI_NETWORK;
        
    case 0x10: /* Encryption/Decryption Controller */
        return IC_KATEGORI_LAINNYA;
        
    case 0x11: /* Data Acquisition and Signal Processing */
        return IC_KATEGORI_LAINNYA;
        
    case 0x12: /* Processing Accelerator */
        return IC_KATEGORI_LAINNYA;
        
    case 0x13: /* Non-Essential Instrumentation */
        return IC_KATEGORI_LAINNYA;
        
    default:
        return IC_KATEGORI_TIDAK_DIKETAHUI;
    }
}

/*
 * ic_klasifikasi_vendor_cpu - Klasifikasi vendor CPU
 */
ic_kategori_t ic_klasifikasi_vendor_cpu(tak_bertanda16_t vendor_id)
{
    /* Intel CPU */
    if (vendor_id == 0x8086) {
        return IC_KATEGORI_CPU;
    }
    
    /* AMD CPU */
    if (vendor_id == 0x1022) {
        return IC_KATEGORI_CPU;
    }
    
    /* VIA CPU */
    if (vendor_id == 0x1106) {
        return IC_KATEGORI_CPU;
    }
    
    return IC_KATEGORI_TIDAK_DIKETAHUI;
}

/*
 * ===========================================================================
 * FUNGSI KLASIFIKASI RENTANG
 * ===========================================================================
 */

/*
 * ic_klasifikasi_rentang_cpu - Klasifikasi rentang CPU
 */
ic_rentang_t ic_klasifikasi_rentang_cpu(tak_bertanda64_t frekuensi_mhz)
{
    if (frekuensi_mhz < CPU_FREKUENSI_LOW_END) {
        return IC_RENTANG_LOW_END;
    }
    if (frekuensi_mhz < CPU_FREKUENSI_MID_LOW) {
        return IC_RENTANG_LOW_END;
    }
    if (frekuensi_mhz < CPU_FREKUENSI_MID) {
        return IC_RENTANG_MID_LOW;
    }
    if (frekuensi_mhz < CPU_FREKUENSI_MID_HIGH) {
        return IC_RENTANG_MID;
    }
    if (frekuensi_mhz < CPU_FREKUENSI_HIGH_END) {
        return IC_RENTANG_MID_HIGH;
    }
    if (frekuensi_mhz < CPU_FREKUENSI_ENTHUSIAST) {
        return IC_RENTANG_HIGH_END;
    }
    
    return IC_RENTANG_ENTHUSIAST;
}

/*
 * ic_klasifikasi_rentang_gpu - Klasifikasi rentang GPU
 */
ic_rentang_t ic_klasifikasi_rentang_gpu(tak_bertanda64_t memori_mb)
{
    if (memori_mb < GPU_MEM_LOW_END) {
        return IC_RENTANG_LOW_END;
    }
    if (memori_mb < GPU_MEM_MID_LOW) {
        return IC_RENTANG_LOW_END;
    }
    if (memori_mb < GPU_MEM_MID) {
        return IC_RENTANG_MID_LOW;
    }
    if (memori_mb < GPU_MEM_MID_HIGH) {
        return IC_RENTANG_MID;
    }
    if (memori_mb < GPU_MEM_HIGH_END) {
        return IC_RENTANG_MID_HIGH;
    }
    if (memori_mb < GPU_MEM_ENTHUSIAST) {
        return IC_RENTANG_HIGH_END;
    }
    
    return IC_RENTANG_ENTHUSIAST;
}

/*
 * ic_klasifikasi_rentang_ram - Klasifikasi rentang RAM
 */
ic_rentang_t ic_klasifikasi_rentang_ram(tak_bertanda64_t kapasitas_mb)
{
    if (kapasitas_mb < RAM_KAPASITAS_LOW_END) {
        return IC_RENTANG_LOW_END;
    }
    if (kapasitas_mb < RAM_KAPASITAS_MID_LOW) {
        return IC_RENTANG_LOW_END;
    }
    if (kapasitas_mb < RAM_KAPASITAS_MID) {
        return IC_RENTANG_MID_LOW;
    }
    if (kapasitas_mb < RAM_KAPASITAS_MID_HIGH) {
        return IC_RENTANG_MID;
    }
    if (kapasitas_mb < RAM_KAPASITAS_HIGH_END) {
        return IC_RENTANG_MID_HIGH;
    }
    if (kapasitas_mb < RAM_KAPASITAS_ENTHUSIAST) {
        return IC_RENTANG_HIGH_END;
    }
    
    return IC_RENTANG_SERVER;
}

/*
 * ic_klasifikasi_rentang_storage - Klasifikasi rentang storage
 */
ic_rentang_t ic_klasifikasi_rentang_storage(tak_bertanda64_t kapasitas_mb)
{
    if (kapasitas_mb < STORAGE_KAPASITAS_LOW_END) {
        return IC_RENTANG_LOW_END;
    }
    if (kapasitas_mb < STORAGE_KAPASITAS_MID_LOW) {
        return IC_RENTANG_LOW_END;
    }
    if (kapasitas_mb < STORAGE_KAPASITAS_MID) {
        return IC_RENTANG_MID_LOW;
    }
    if (kapasitas_mb < STORAGE_KAPASITAS_MID_HIGH) {
        return IC_RENTANG_MID;
    }
    if (kapasitas_mb < STORAGE_KAPASITAS_HIGH_END) {
        return IC_RENTANG_MID_HIGH;
    }
    
    return IC_RENTANG_HIGH_END;
}

/*
 * ===========================================================================
 * FUNGSI IDENTIFIKASI VENDOR
 * ===========================================================================
 */

/* Vendor ID database */
typedef struct {
    tak_bertanda16_t vendor_id;
    const char *nama;
} vendor_entri_t;

/* Daftar vendor yang dikenali */
static const vendor_entri_t g_vendor_db[] = {
    /* CPU Vendors */
    { 0x8086, "Intel" },
    { 0x1022, "AMD" },
    { 0x1106, "VIA" },
    { 0x106B, "Apple" },
    { 0x17D3, "Hygon" },
    
    /* GPU Vendors */
    { 0x10DE, "NVIDIA" },
    { 0x1002, "AMD" },
    { 0x8086, "Intel" },
    { 0x1A03, "ASPEED" },
    { 0x15AD, "VMware" },
    { 0x80EE, "Oracle" },
    
    /* Network Vendors */
    { 0x8086, "Intel" },
    { 0x10EC, "Realtek" },
    { 0x14E4, "Broadcom" },
    { 0x1969, "Qualcomm Atheros" },
    { 0x168C, "Qualcomm Atheros" },
    { 0x10B7, "3Com" },
    { 0x11AB, "Marvell" },
    { 0x8087, "Intel" },
    { 0x15B3, "Mellanox" },
    { 0x19EE, "Solarflare" },
    
    /* Storage Vendors */
    { 0x8086, "Intel" },
    { 0x1022, "AMD" },
    { 0x144D, "Samsung" },
    { 0x1B4B, "Marvell" },
    { 0x1C58, "HGST" },
    { 0x1C5F, "SK Hynix" },
    { 0x1D95, "Phison" },
    { 0x1E4B, "SK Hynix" },
    { 0x126F, "Silicon Motion" },
    { 0x1179, "Toshiba" },
    { 0x1B21, "ASMedia" },
    
    /* Audio Vendors */
    { 0x8086, "Intel" },
    { 0x10DE, "NVIDIA" },
    { 0x1002, "AMD" },
    { 0x1102, "Creative" },
    { 0x1412, "VIA" },
    { 0x1FA8, "Conexant" },
    
    /* USB Controllers */
    { 0x8086, "Intel" },
    { 0x1022, "AMD" },
    { 0x1033, "NEC" },
    { 0x1912, "Renesas" },
    { 0x1106, "VIA" },
    { 0x1D6B, "Linux Foundation" },
    
    /* Input Devices */
    { 0x046D, "Logitech" },
    { 0x045E, "Microsoft" },
    { 0x046A, "Cherry" },
    { 0x04B3, "IBM" },
    
    /* Bridge/Chipset */
    { 0x8086, "Intel" },
    { 0x1022, "AMD" },
    { 0x1106, "VIA" },
    { 0x10B9, "ALi" },
    { 0x1166, "Broadcom" },
    { 0x1039, "SiS" },
    { 0x10B9, "ALi" }
};

#define VENDOR_DB_JUMLAH (sizeof(g_vendor_db) / sizeof(g_vendor_db[0]))

/*
 * ic_vendor_nama - Dapatkan nama vendor dari ID
 */
const char *ic_vendor_nama(tak_bertanda16_t vendor_id)
{
    tak_bertanda32_t i;
    
    for (i = 0; i < VENDOR_DB_JUMLAH; i++) {
        if (g_vendor_db[i].vendor_id == vendor_id) {
            return g_vendor_db[i].nama;
        }
    }
    
    return "Tidak Diketahui";
}

/*
 * ===========================================================================
 * FUNGSI KLASIFIKASI UTAMA
 * ===========================================================================
 */

/*
 * ic_klasifikasi_perangkat - Klasifikasi perangkat lengkap
 */
status_t ic_klasifikasi_perangkat(ic_perangkat_t *perangkat)
{
    ic_kategori_t kategori;
    const char *vendor_nama;
    
    if (perangkat == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (perangkat->magic != IC_ENTRI_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Klasifikasi berdasarkan PCI class code */
    kategori = ic_klasifikasi_class_code(perangkat->class_code);
    
    /* Dapatkan nama vendor */
    vendor_nama = ic_vendor_nama(perangkat->vendor_id);
    
    /* Update perangkat */
    if (perangkat->entri != NULL) {
        perangkat->entri->kategori = kategori;
        ic_salinnama(perangkat->entri->vendor, vendor_nama, 
                     IC_VENDOR_PANJANG_MAKS);
    }
    
    return STATUS_BERHASIL;
}

/*
 * ic_klasifikasi_perangkat_perbarui_rentang - Perbarui rentang setelah test
 */
status_t ic_klasifikasi_perangkat_perbarui_rentang(ic_perangkat_t *perangkat)
{
    ic_parameter_t *param;
    ic_rentang_t rentang;
    tak_bertanda64_t nilai;
    tak_bertanda32_t i;
    
    if (perangkat == NULL || perangkat->entri == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Cari parameter utama untuk menentukan rentang */
    for (i = 0; i < perangkat->entri->jumlah_param; i++) {
        param = &perangkat->entri->param[i];
        
        /* Skip parameter yang belum ditest */
        if (!param->ditest) {
            continue;
        }
        
        nilai = param->nilai_max;
        
        switch (perangkat->entri->kategori) {
        case IC_KATEGORI_CPU:
            if (param->tipe == IC_PARAM_TIPE_FREKUENSI) {
                rentang = ic_klasifikasi_rentang_cpu(nilai);
                perangkat->entri->rentang = rentang;
                return STATUS_BERHASIL;
            }
            break;
            
        case IC_KATEGORI_GPU:
            if (param->tipe == IC_PARAM_TIPE_KAPASITAS) {
                rentang = ic_klasifikasi_rentang_gpu(nilai);
                perangkat->entri->rentang = rentang;
                return STATUS_BERHASIL;
            }
            break;
            
        case IC_KATEGORI_RAM:
            if (param->tipe == IC_PARAM_TIPE_KAPASITAS) {
                rentang = ic_klasifikasi_rentang_ram(nilai);
                perangkat->entri->rentang = rentang;
                return STATUS_BERHASIL;
            }
            break;
            
        case IC_KATEGORI_STORAGE:
            if (param->tipe == IC_PARAM_TIPE_KAPASITAS) {
                rentang = ic_klasifikasi_rentang_storage(nilai);
                perangkat->entri->rentang = rentang;
                return STATUS_BERHASIL;
            }
            break;
            
        default:
            break;
        }
    }
    
    return STATUS_BERHASIL;
}
