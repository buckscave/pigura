/*
 * PIGURA OS - MEMORI.H
 * =====================
 * Header untuk manajemen memori perangkat.
 *
 * Berkas ini mendefinisikan interface untuk pengatur memori,
 * cache controller, RAM controller, dan PCI controller.
 *
 * Kepatuhan standar:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Mendukung semua arsitektur
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#ifndef PERANGKAT_MEMORI_H
#define PERANGKAT_MEMORI_H

/*
 * ===========================================================================
 * INCLUDE
 * ===========================================================================
 */

#include "../../inti/types.h"

/*
 * ===========================================================================
 * KONSTANTA MEMORI
 * ===========================================================================
 */

/* Versi modul */
#define MEMORI_VERSI_MAJOR 1
#define MEMORI_VERSI_MINOR 0
#define MEMORI_VERSI_PATCH 0

/* Magic numbers */
#define MEMORI_MAGIC          0x4D454D52  /* "MEMR" */
#define RAM_CONTROLLER_MAGIC  0x52414D43  /* "RAMC" */
#define CACHE_CTRL_MAGIC      0x43434843  /* "CCHC" */
#define PCI_CTRL_MAGIC        0x50434943  /* "PCIC" */

/* Batas sistem */
#define MEMORI_REGION_MAKS    32
#define CACHE_LEVEL_MAKS      4
#define PCI_BUS_MAKS          256
#define PCI_DEVICE_MAKS       32
#define PCI_FUNCTION_MAKS     8
#define PCI_BAR_MAKS          6

/* Tipe memory region */
#define MEM_REGION_TYPE_TIDAK_ADA    0
#define MEM_REGION_TYPE_USABLE       1
#define MEM_REGION_TYPE_RESERVED     2
#define MEM_REGION_TYPE_ACPI         3
#define MEM_REGION_TYPE_NVS          4
#define MEM_REGION_TYPE_UNUSABLE     5
#define MEM_REGION_TYPE_DISABLED     6
#define MEM_REGION_TYPE_PMEM         7

/* Cache tipe */
#define CACHE_TYPE_TIDAK_ADA        0
#define CACHE_TYPE_DATA             1
#define CACHE_TYPE_INSTRUCTION      2
#define CACHE_TYPE_UNIFIED          3

/* Cache policy */
#define CACHE_POLICY_WRITE_BACK     0
#define CACHE_POLICY_WRITE_THROUGH  1
#define CACHE_POLICY_WRITE_PROTECT  2
#define CACHE_POLICY_WRITE_COMBINE  3

/* PCI command flags */
#define PCI_CMD_IO_SPACE            0x0001
#define PCI_CMD_MEMORY_SPACE        0x0002
#define PCI_CMD_BUS_MASTER          0x0004
#define PCI_CMD_SPECIAL_CYCLES      0x0008
#define PCI_CMD_MEM_WRITE_INVALID   0x0010
#define PCI_CMD_VGA_PALETTE_SNOOP   0x0020
#define PCI_CMD_PARITY_ERROR        0x0040
#define PCI_CMD_SERR                0x0100
#define PCI_CMD_FAST_BACK_TO_BACK   0x0200
#define PCI_CMD_INT_DISABLE         0x0400

/* PCI status flags */
#define PCI_STATUS_CAP_LIST         0x0010
#define PCI_STATUS_66MHZ           0x0020
#define PCI_STATUS_FAST_BACK        0x0080
#define PCI_STATUS_MASTER_PARITY    0x0100
#define PCI_STATUS_DEVSEL_MASK      0x0600
#define PCI_STATUS_SIGNALED_TABORT  0x0800
#define PCI_STATUS_RECEIVED_TABORT  0x1000
#define PCI_STATUS_RECEIVED_MABORT  0x2000
#define PCI_STATUS_SIGNALED_SERR    0x4000
#define PCI_STATUS_PARITY_ERROR     0x8000

/* PCI class codes */
#define PCI_CLASS_NOT_DEFINED       0x0000
#define PCI_CLASS_STORAGE           0x0100
#define PCI_CLASS_NETWORK           0x0200
#define PCI_CLASS_DISPLAY           0x0300
#define PCI_CLASS_MULTIMEDIA        0x0400
#define PCI_CLASS_MEMORY            0x0500
#define PCI_CLASS_BRIDGE            0x0600
#define PCI_CLASS_COMM              0x0700
#define PCI_CLASS_SYSTEM            0x0800
#define PCI_CLASS_INPUT             0x0900
#define PCI_CLASS_DOCKING           0x0A00
#define PCI_CLASS_PROCESSOR         0x0B00
#define PCI_CLASS_SERIAL            0x0C00
#define PCI_CLASS_WIRELESS          0x0D00
#define PCI_CLASS_SATELLITE         0x0E00
#define PCI_CLASS_CRYPTO            0x1000
#define PCI_CLASS_SIGNAL            0x1100

/* PCI subclass storage */
#define PCI_SUBCLASS_STORAGE_SCSI   0x0100
#define PCI_SUBCLASS_STORAGE_IDE    0x0101
#define PCI_SUBCLASS_STORAGE_FLOPPY 0x0102
#define PCI_SUBCLASS_STORAGE_IPI    0x0103
#define PCI_SUBCLASS_STORAGE_RAID   0x0104
#define PCI_SUBCLASS_STORAGE_ATA    0x0105
#define PCI_SUBCLASS_STORAGE_SATA   0x0106
#define PCI_SUBCLASS_STORAGE_NVME   0x0108

/* PCI subclass network */
#define PCI_SUBCLASS_NETWORK_ETHERNET  0x0200
#define PCI_SUBCLASS_NETWORK_TOKENRING 0x0201
#define PCI_SUBCLASS_NETWORK_FDDI      0x0202
#define PCI_SUBCLASS_NETWORK_ATM       0x0203
#define PCI_SUBCLASS_NETWORK_WIFI      0x0280

/* PCI subclass display */
#define PCI_SUBCLASS_DISPLAY_VGA    0x0300
#define PCI_SUBCLASS_DISPLAY_XGA    0x0301
#define PCI_SUBCLASS_DISPLAY_3D     0x0302
#define PCI_SUBCLASS_DISPLAY_OTHER  0x0380

/*
 * ===========================================================================
 * STRUKTUR MEMORY REGION
 * ===========================================================================
 */

typedef struct {
    tak_bertanda32_t magic;         /* Magic number */
    tak_bertanda32_t id;            /* Region ID */
    
    alamat_fisik_t mulai;           /* Alamat awal */
    alamat_fisik_t akhir;           /* Alamat akhir */
    ukuran_t ukuran;                /* Ukuran region */
    
    tak_bertanda32_t tipe;          /* Tipe region */
    tak_bertanda32_t flags;         /* Flags tambahan */
    
    bool_t tersedia;                /* Apakah tersedia */
    bool_t reserved;                /* Apakah reserved */
    
} mem_region_t;

/*
 * ===========================================================================
 * STRUKTUR CACHE CONTROLLER
 * ===========================================================================
 */

typedef struct {
    tak_bertanda32_t magic;         /* Magic number */
    
    /* Cache levels */
    tak_bertanda32_t level_count;
    
    /* Per-level info */
    ukuran_t ukuran[CACHE_LEVEL_MAKS];
    ukuran_t line_size[CACHE_LEVEL_MAKS];
    tak_bertanda32_t ways[CACHE_LEVEL_MAKS];
    tak_bertanda32_t tipe[CACHE_LEVEL_MAKS];
    tak_bertanda32_t policy[CACHE_LEVEL_MAKS];
    
    /* Control registers */
    tak_bertanda32_t ctrl_reg;
    tak_bertanda32_t status_reg;
    
    /* Status */
    bool_t enabled;
    bool_t write_back;
    
} cache_controller_t;

/*
 * ===========================================================================
 * STRUKTUR RAM CONTROLLER
 * ===========================================================================
 */

typedef struct {
    tak_bertanda32_t magic;         /* Magic number */
    tak_bertanda32_t id;            /* Controller ID */
    
    /* Kapasitas */
    ukuran_t total_size;            /* Total RAM size */
    ukuran_t available_size;        /* Available RAM */
    ukuran_t used_size;             /* Used RAM */
    
    /* Konfigurasi */
    tak_bertanda32_t bus_width;     /* Bus width (bits) */
    tak_bertanda32_t speed_mhz;     /* Speed in MHz */
    tak_bertanda32_t cas_latency;   /* CAS latency */
    tak_bertanda32_t ranks;         /* Number of ranks */
    
    /* Timing */
    tak_bertanda32_t tCL;           /* CAS latency */
    tak_bertanda32_t tRCD;          /* RAS to CAS delay */
    tak_bertanda32_t tRP;           /* Row precharge */
    tak_bertanda32_t tRAS;          /* Row active time */
    
    /* Regions */
    mem_region_t regions[MEMORI_REGION_MAKS];
    tak_bertanda32_t region_count;
    
    /* Status */
    bool_t initialized;
    bool_t ecc_enabled;
    bool_t interleaved;
    
} ram_controller_t;

/*
 * ===========================================================================
 * STRUKTUR PCI DEVICE
 * ===========================================================================
 */

typedef struct pci_device pci_device_t;

struct pci_device {
    tak_bertanda32_t magic;         /* Magic number */
    tak_bertanda32_t id;            /* Device ID internal */
    
    /* Bus address */
    tak_bertanda8_t bus;            /* Bus number */
    tak_bertanda8_t device;         /* Device number */
    tak_bertanda8_t function;       /* Function number */
    
    /* Identification */
    tak_bertanda16_t vendor_id;     /* Vendor ID */
    tak_bertanda16_t device_id;     /* Device ID */
    tak_bertanda16_t subsystem_vendor;
    tak_bertanda16_t subsystem_id;
    tak_bertanda16_t revision;      /* Revision ID */
    
    /* Classification */
    tak_bertanda8_t base_class;     /* Base class code */
    tak_bertanda8_t sub_class;      /* Subclass code */
    tak_bertanda8_t interface;      /* Programming interface */
    tak_bertanda16_t class_code;    /* Full class code */
    
    /* BARs */
    alamat_fisik_t bar[PCI_BAR_MAKS];
    ukuran_t bar_size[PCI_BAR_MAKS];
    tak_bertanda32_t bar_type[PCI_BAR_MAKS];
    
    /* Interrupt */
    tak_bertanda8_t irq_pin;        /* IRQ pin (INTA-INTD) */
    tak_bertanda8_t irq_line;       /* IRQ line */
    
    /* Command/Status */
    tak_bertanda16_t command;
    tak_bertanda16_t status;
    
    /* Capabilities */
    tak_bertanda8_t cap_pointer;
    bool_t has_msi;
    bool_t has_msix;
    bool_t has_pcie;
    
    /* Private data */
    void *driver_data;
    void *priv;
    
    /* Next device */
    pci_device_t *berikutnya;
};

/*
 * ===========================================================================
 * STRUKTUR PCI CONTROLLER
 * ===========================================================================
 */

typedef struct {
    tak_bertanda32_t magic;         /* Magic number */
    
    /* Configuration */
    alamat_fisik_t config_base;     /* Configuration space base */
    tak_bertanda32_t segment;       /* PCI segment */
    tak_bertanda8_t bus_start;      /* Start bus */
    tak_bertanda8_t bus_end;        /* End bus */
    
    /* Device list */
    pci_device_t *device_list;
    tak_bertanda32_t device_count;
    
    /* Statistics */
    tak_bertanda32_t devices_found;
    tak_bertanda32_t bridges_found;
    
    /* Status */
    bool_t initialized;
    bool_t is_pcie;
    
} pci_controller_t;

/*
 * ===========================================================================
 * STRUKTUR KONTEKS MEMORI
 * ===========================================================================
 */

typedef struct {
    tak_bertanda32_t magic;         /* Magic number */
    
    /* Memory regions */
    mem_region_t regions[MEMORI_REGION_MAKS];
    tak_bertanda32_t region_count;
    
    /* Controllers */
    ram_controller_t ram_ctrl;
    cache_controller_t cache_ctrl;
    pci_controller_t pci_ctrl;
    
    /* Statistics */
    ukuran_t total_memory;
    ukuran_t available_memory;
    ukuran_t used_memory;
    
    /* Status */
    bool_t initialized;
    
} memori_konteks_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

extern memori_konteks_t g_memori_konteks;
extern bool_t g_memori_diinisialisasi;

/*
 * ===========================================================================
 * FUNGSI INISIALISASI
 * ===========================================================================
 */

/*
 * memori_init - Inisialisasi subsistem memori
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t memori_init(void);

/*
 * memori_shutdown - Matikan subsistem memori
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t memori_shutdown(void);

/*
 * memori_konteks_dapatkan - Dapatkan konteks memori
 *
 * Return: Pointer ke konteks atau NULL
 */
memori_konteks_t *memori_konteks_dapatkan(void);

/*
 * ===========================================================================
 * FUNGSI MEMORY REGION
 * ===========================================================================
 */

/*
 * mem_region_tambah - Tambah memory region
 *
 * Parameter:
 *   mulai - Alamat awal
 *   akhir - Alamat akhir
 *   tipe - Tipe region
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t mem_region_tambah(alamat_fisik_t mulai, alamat_fisik_t akhir,
                            tak_bertanda32_t tipe);

/*
 * mem_region_cari - Cari region berdasarkan alamat
 *
 * Parameter:
 *   addr - Alamat yang dicari
 *
 * Return: Pointer ke region atau NULL
 */
mem_region_t *mem_region_cari(alamat_fisik_t addr);

/*
 * mem_region_total - Dapatkan total memori
 *
 * Return: Total memori dalam byte
 */
ukuran_t mem_region_total(void);

/*
 * mem_region_tersedia - Dapatkan memori tersedia
 *
 * Return: Memori tersedia dalam byte
 */
ukuran_t mem_region_tersedia(void);

/*
 * ===========================================================================
 * FUNGSI RAM CONTROLLER
 * ===========================================================================
 */

/*
 * ram_init - Inisialisasi RAM controller
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ram_init(void);

/*
 * ram_deteksi - Deteksi RAM
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ram_deteksi(void);

/*
 * ram_ukuran - Dapatkan ukuran RAM
 *
 * Return: Ukuran RAM dalam byte
 */
ukuran_t ram_ukuran(void);

/*
 * ram_info - Dapatkan info RAM
 *
 * Parameter:
 *   ctrl - Buffer untuk controller info
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ram_info(ram_controller_t *ctrl);

/*
 * ram_cetak_info - Cetak informasi RAM
 */
void ram_cetak_info(void);

/*
 * ===========================================================================
 * FUNGSI CACHE CONTROLLER
 * ===========================================================================
 */

/*
 * cache_init - Inisialisasi cache controller
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cache_init(void);

/*
 * cache_enable - Aktifkan cache
 *
 * Parameter:
 *   level - Level cache (1-3)
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cache_enable(tak_bertanda32_t level);

/*
 * cache_disable - Nonaktifkan cache
 *
 * Parameter:
 *   level - Level cache
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cache_disable(tak_bertanda32_t level);

/*
 * cache_flush - Flush cache
 *
 * Parameter:
 *   level - Level cache
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cache_flush(tak_bertanda32_t level);

/*
 * cache_invalidate - Invalidate cache
 *
 * Parameter:
 *   level - Level cache
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cache_invalidate(tak_bertanda32_t level);

/*
 * cache_info - Dapatkan info cache
 *
 * Parameter:
 *   level - Level cache
 *   ctrl - Buffer untuk info
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cache_info(tak_bertanda32_t level, cache_controller_t *ctrl);

/*
 * ===========================================================================
 * FUNGSI PCI
 * ===========================================================================
 */

/*
 * pci_init - Inisialisasi PCI controller
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t pci_init(void);

/*
 * pci_scan - Scan bus PCI
 *
 * Return: Jumlah device ditemukan atau nilai negatif
 */
tanda32_t pci_scan(void);

/*
 * pci_device_cari - Cari device berdasarkan ID
 *
 * Parameter:
 *   vendor_id - Vendor ID
 *   device_id - Device ID
 *
 * Return: Pointer ke device atau NULL
 */
pci_device_t *pci_device_cari(tak_bertanda16_t vendor_id,
                               tak_bertanda16_t device_id);

/*
 * pci_device_cari_class - Cari device berdasarkan class
 *
 * Parameter:
 *   class_code - Class code
 *   index - Index (untuk multiple devices)
 *
 * Return: Pointer ke device atau NULL
 */
pci_device_t *pci_device_cari_class(tak_bertanda16_t class_code,
                                     tak_bertanda32_t index);

/*
 * pci_read_config - Baca configuration space
 *
 * Parameter:
 *   bus - Bus number
 *   dev - Device number
 *   func - Function number
 *   reg - Register offset
 *   size - Size (1, 2, atau 4 bytes)
 *
 * Return: Nilai yang dibaca
 */
tak_bertanda32_t pci_read_config(tak_bertanda8_t bus, tak_bertanda8_t dev,
                                  tak_bertanda8_t func, tak_bertanda8_t reg,
                                  tak_bertanda32_t size);

/*
 * pci_write_config - Tulis configuration space
 *
 * Parameter:
 *   bus - Bus number
 *   dev - Device number
 *   func - Function number
 *   reg - Register offset
 *   value - Nilai yang akan ditulis
 *   size - Size (1, 2, atau 4 bytes)
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t pci_write_config(tak_bertanda8_t bus, tak_bertanda8_t dev,
                           tak_bertanda8_t func, tak_bertanda8_t reg,
                           tak_bertanda32_t value, tak_bertanda32_t size);

/*
 * pci_enable_device - Enable PCI device
 *
 * Parameter:
 *   dev - Pointer ke device
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t pci_enable_device(pci_device_t *dev);

/*
 * pci_disable_device - Disable PCI device
 *
 * Parameter:
 *   dev - Pointer ke device
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t pci_disable_device(pci_device_t *dev);

/*
 * pci_set_bus_master - Set bus master
 *
 * Parameter:
 *   dev - Pointer ke device
 *   enable - Enable/disable
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t pci_set_bus_master(pci_device_t *dev, bool_t enable);

/*
 * pci_get_bar - Dapatkan BAR
 *
 * Parameter:
 *   dev - Pointer ke device
 *   index - BAR index
 *   addr - Pointer untuk alamat
 *   size - Pointer untuk ukuran
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t pci_get_bar(pci_device_t *dev, tak_bertanda32_t index,
                      alamat_fisik_t *addr, ukuran_t *size);

/*
 * pci_enable_irq - Enable IRQ untuk device
 *
 * Parameter:
 *   dev - Pointer ke device
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t pci_enable_irq(pci_device_t *dev);

/*
 * ===========================================================================
 * FUNGSI UTILITAS
 * ===========================================================================
 */

/*
 * memori_nama_tipe - Dapatkan nama tipe region
 *
 * Parameter:
 *   tipe - Tipe region
 *
 * Return: String nama tipe
 */
const char *memori_nama_tipe(tak_bertanda32_t tipe);

/*
 * pci_nama_class - Dapatkan nama PCI class
 *
 * Parameter:
 *   class_code - Class code
 *
 * Return: String nama class
 */
const char *pci_nama_class(tak_bertanda16_t class_code);

/*
 * memori_cetak_info - Cetak informasi memori
 */
void memori_cetak_info(void);

/*
 * pci_cetak_info - Cetak informasi PCI
 */
void pci_cetak_info(void);

/*
 * pci_cetak_device - Cetak informasi device
 *
 * Parameter:
 *   dev - Pointer ke device
 */
void pci_cetak_device(pci_device_t *dev);

#endif /* PERANGKAT_MEMORI_H */
