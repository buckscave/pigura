/*
 * PIGURA OS - PROBE/PROBE_PCI.C
 * ==============================
 * PCI bus probe untuk IC Detection.
 *
 * Berkas ini mengimplementasikan probing pada bus PCI/PCIe
 * untuk mendeteksi perangkat yang terhubung dengan dukungan
 * HAL abstraction untuk portabilitas antar arsitektur.
 *
 * Dukungan:
 * - x86/x86_64: I/O port (0xCF8/0xCFC)
 * - ARM/ARM64: MMIO (ECAM/PCI Express Enhanced Configuration)
 *
 * Versi: 2.0
 * Tanggal: 2026
 */

#include "../ic.h"

/*
 * ===========================================================================
 * KONSTANTA PCI
 * ===========================================================================
 */

/* PCI configuration space ports (x86 specific) */
#define PCI_CONFIG_ADDRESS_X86      0xCF8
#define PCI_CONFIG_DATA_X86         0xCFC

/* PCI Express Enhanced Configuration (MMIO) */
#define PCI_ECAM_BASE               0xE0000000ULL
#define PCI_ECAM_SIZE               0x10000000ULL
#define PCI_ECAM_BUS_SHIFT          20
#define PCI_ECAM_DEV_SHIFT          15
#define PCI_ECAM_FUNC_SHIFT         12

/* PCI configuration space offsets */
#define PCI_OFFSET_VENDOR           0x00
#define PCI_OFFSET_DEVICE           0x02
#define PCI_OFFSET_COMMAND          0x04
#define PCI_OFFSET_STATUS           0x06
#define PCI_OFFSET_REVISION         0x08
#define PCI_OFFSET_CLASS            0x0A
#define PCI_OFFSET_CACHELINE        0x0C
#define PCI_OFFSET_LATENCY          0x0D
#define PCI_OFFSET_HEADERTYPE       0x0E
#define PCI_OFFSET_BIST             0x0F
#define PCI_OFFSET_BAR0             0x10
#define PCI_OFFSET_BAR1             0x14
#define PCI_OFFSET_BAR2             0x18
#define PCI_OFFSET_BAR3             0x1C
#define PCI_OFFSET_BAR4             0x20
#define PCI_OFFSET_BAR5             0x24
#define PCI_OFFSET_CARDBUS          0x28
#define PCI_OFFSET_SUBSYSTEM_VENDOR 0x2C
#define PCI_OFFSET_SUBSYSTEM_ID     0x2E
#define PCI_OFFSET_EXPANSION        0x30
#define PCI_OFFSET_CAPABILITIES     0x34
#define PCI_OFFSET_IRQ              0x3C
#define PCI_OFFSET_IRQ_PIN          0x3D
#define PCI_OFFSET_MIN_GNT          0x3E
#define PCI_OFFSET_MAX_LAT          0x3F

/* PCI vendor ID special values */
#define PCI_VENDOR_INVALID          0xFFFF
#define PCI_VENDOR_NONE             0x0000

/* PCI command bits */
#define PCI_COMMAND_IO              0x01
#define PCI_COMMAND_MEM             0x02
#define PCI_COMMAND_MASTER          0x04
#define PCI_COMMAND_SPECIAL         0x08
#define PCI_COMMAND_INVALIDATE      0x10
#define PCI_COMMAND_VGA_PALETTE     0x20
#define PCI_COMMAND_PARITY          0x40
#define PCI_COMMAND_WAIT            0x80
#define PCI_COMMAND_SERR            0x100
#define PCI_COMMAND_FAST_BACK       0x200

/* PCI status bits */
#define PCI_STATUS_CAPABILITIES     0x10
#define PCI_STATUS_66MHZ            0x20
#define PCI_STATUS_FAST_BACK        0x80
#define PCI_STATUS_DEVSEL_MASK      0x600
#define PCI_STATUS_PARITY           0x800
#define PCI_STATUS_SERR             0x1000
#define PCI_STATUS_MASTER_ABORT     0x2000
#define PCI_STATUS_TARGET_ABORT     0x4000

/* Header types */
#define PCI_HEADER_TYPE_NORMAL      0x00
#define PCI_HEADER_TYPE_BRIDGE      0x01
#define PCI_HEADER_TYPE_CARDBUS     0x02
#define PCI_HEADER_TYPE_MULTIFUNC   0x80

/* Maximum bus/device/function */
#define PCI_MAX_BUS                 256
#define PCI_MAX_DEVICE              32
#define PCI_MAX_FUNCTION            8

/* Maximum BARs */
#define PCI_MAX_BAR                 6

/* PCI class codes */
#define PCI_CLASS_STORAGE_SCSI      0x00010000
#define PCI_CLASS_STORAGE_IDE       0x00010100
#define PCI_CLASS_STORAGE_FLOPPY    0x00010200
#define PCI_CLASS_STORAGE_RAID      0x00010400
#define PCI_CLASS_STORAGE_ATA       0x00010500
#define PCI_CLASS_STORAGE_SATA      0x00010600
#define PCI_CLASS_STORAGE_NVME      0x00010800

#define PCI_CLASS_NETWORK_ETHERNET  0x00020000
#define PCI_CLASS_NETWORK_TOKEN     0x00020100
#define PCI_CLASS_NETWORK_FDDI      0x00020200
#define PCI_CLASS_NETWORK_ATM       0x00020300
#define PCI_CLASS_NETWORK_ISDN      0x00020400
#define PCI_CLASS_NETWORK_WIRELESS  0x00028000

#define PCI_CLASS_DISPLAY_VGA       0x00030000
#define PCI_CLASS_DISPLAY_XGA       0x00030100
#define PCI_CLASS_DISPLAY_3D        0x00030200
#define PCI_CLASS_DISPLAY_OTHER     0x00038000

#define PCI_CLASS_MULTIMEDIA_AUDIO  0x00040100
#define PCI_CLASS_MULTIMEDIA_VIDEO  0x00040000
#define PCI_CLASS_MULTIMEDIA_HDA    0x00040300

#define PCI_CLASS_BRIDGE_HOST       0x00060000
#define PCI_CLASS_BRIDGE_ISA        0x00060100
#define PCI_CLASS_BRIDGE_EISA       0x00060200
#define PCI_CLASS_BRIDGE_PCI        0x00060400
#define PCI_CLASS_BRIDGE_PCMCIA     0x00060500
#define PCI_CLASS_BRIDGE_CARDBUS    0x00060700

#define PCI_CLASS_SERIAL_USB        0x000C0300
#define PCI_CLASS_SERIAL_FIREWIRE   0x000C0000
#define PCI_CLASS_SERIAL_SMBUS      0x000C0500

/*
 * ===========================================================================
 * TIPE HAL PCI
 * ===========================================================================
 */

typedef enum {
    PCI_HAL_TYPE_X86_PORT = 0,
    PCI_HAL_TYPE_MMIO = 1,
    PCI_HAL_TYPE_ECAME = 2
} pci_hal_type_t;

typedef struct {
    pci_hal_type_t type;
    alamat_fisik_t base_address;
    bool_t initialized;
} pci_hal_context_t;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static pci_hal_context_t g_pci_hal;
static bool_t g_pci_hal_initialized = SALAH;

/*
 * ===========================================================================
 * FUNGSI HAL ABSTRACTION LAYER
 * ===========================================================================
 */

/*
 * pci_hal_detect_method - Deteksi metode akses PCI yang tersedia
 */
static pci_hal_type_t pci_hal_detect_method(void)
{
    /* Deteksi arsitektur melalui compiler macros */
#if defined(__x86_64__) || defined(__i386__) || defined(__i686__)
    return PCI_HAL_TYPE_X86_PORT;
#elif defined(__arm__) || defined(__aarch64__) || defined(__arm64__)
    return PCI_HAL_TYPE_MMIO;
#else
    return PCI_HAL_TYPE_MMIO;
#endif
}

/*
 * pci_hal_outl - Tulis 32-bit ke I/O port (x86)
 */
static __inline__ void pci_hal_outl(tak_bertanda16_t port, tak_bertanda32_t nilai)
{
#if defined(__x86_64__) || defined(__i386__)
    __asm__ __volatile__ ("outl %0, %1" : : "a"(nilai), "Nd"(port));
#else
    (void)port;
    (void)nilai;
#endif
}

/*
 * pci_hal_inl - Baca 32-bit dari I/O port (x86)
 */
static __inline__ tak_bertanda32_t pci_hal_inl(tak_bertanda16_t port)
{
#if defined(__x86_64__) || defined(__i386__)
    tak_bertanda32_t nilai;
    __asm__ __volatile__ ("inl %1, %0" : "=a"(nilai) : "Nd"(port));
    return nilai;
#else
    (void)port;
    return 0xFFFFFFFF;
#endif
}

/*
 * pci_hal_mmio_read - Baca 32-bit dari alamat MMIO
 */
static __inline__ tak_bertanda32_t pci_hal_mmio_read(alamat_fisik_t alamat)
{
    volatile tak_bertanda32_t *ptr;
    ptr = (volatile tak_bertanda32_t *)(uintptr_t)(tak_bertanda32_t)alamat;
    return *ptr;
}

/*
 * pci_hal_mmio_write - Tulis 32-bit ke alamat MMIO
 */
static __inline__ void pci_hal_mmio_write(alamat_fisik_t alamat, tak_bertanda32_t nilai)
{
    volatile tak_bertanda32_t *ptr;
    ptr = (volatile tak_bertanda32_t *)(uintptr_t)(tak_bertanda32_t)alamat;
    *ptr = nilai;
}

/*
 * pci_hal_init - Inisialisasi HAL PCI
 */
static status_t pci_hal_init(void)
{
    if (g_pci_hal_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
    g_pci_hal.type = pci_hal_detect_method();
    g_pci_hal.base_address = 0;
    g_pci_hal.initialized = SALAH;
    
    /* Untuk MMIO, set base address ECAM */
    if (g_pci_hal.type == PCI_HAL_TYPE_MMIO) {
        g_pci_hal.base_address = PCI_ECAM_BASE;
    }
    
    g_pci_hal.initialized = BENAR;
    g_pci_hal_initialized = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * pci_hal_baca_konfigurasi32 - Baca 32-bit dari PCI config space (portabel)
 */
static tak_bertanda32_t pci_hal_baca_konfigurasi32(tak_bertanda8_t bus,
                                                    tak_bertanda8_t dev,
                                                    tak_bertanda8_t func,
                                                    tak_bertanda8_t offset)
{
    tak_bertanda32_t nilai = 0xFFFFFFFF;
    
    if (!g_pci_hal_initialized) {
        return nilai;
    }
    
    switch (g_pci_hal.type) {
    case PCI_HAL_TYPE_X86_PORT:
        {
            tak_bertanda32_t alamat;
            
            /* Format alamat PCI Configuration Mechanism #1 */
            alamat = ((tak_bertanda32_t)bus << 16) |
                     ((tak_bertanda32_t)dev << 11) |
                     ((tak_bertanda32_t)func << 8) |
                     ((tak_bertanda32_t)offset & 0xFC) |
                     0x80000000UL;
            
            /* Tulis alamat ke CONFIG_ADDRESS */
            pci_hal_outl(PCI_CONFIG_ADDRESS_X86, alamat);
            
            /* Baca data dari CONFIG_DATA */
            nilai = pci_hal_inl(PCI_CONFIG_DATA_X86);
        }
        break;
        
    case PCI_HAL_TYPE_MMIO:
    case PCI_HAL_TYPE_ECAME:
        {
            /* PCI Express Enhanced Configuration Access Mechanism (ECAM) */
            alamat_fisik_t alamat;
            
            alamat = g_pci_hal.base_address |
                     ((tak_bertanda64_t)bus << PCI_ECAM_BUS_SHIFT) |
                     ((tak_bertanda64_t)dev << PCI_ECAM_DEV_SHIFT) |
                     ((tak_bertanda64_t)func << PCI_ECAM_FUNC_SHIFT) |
                     (offset & 0xFFC);
            
            nilai = pci_hal_mmio_read(alamat);
        }
        break;
        
    default:
        nilai = 0xFFFFFFFF;
        break;
    }
    
    return nilai;
}

/*
 * pci_hal_tulis_konfigurasi32 - Tulis 32-bit ke PCI config space (portabel)
 */
static void pci_hal_tulis_konfigurasi32(tak_bertanda8_t bus,
                                         tak_bertanda8_t dev,
                                         tak_bertanda8_t func,
                                         tak_bertanda8_t offset,
                                         tak_bertanda32_t nilai)
{
    if (!g_pci_hal_initialized) {
        return;
    }
    
    switch (g_pci_hal.type) {
    case PCI_HAL_TYPE_X86_PORT:
        {
            tak_bertanda32_t alamat;
            
            alamat = ((tak_bertanda32_t)bus << 16) |
                     ((tak_bertanda32_t)dev << 11) |
                     ((tak_bertanda32_t)func << 8) |
                     ((tak_bertanda32_t)offset & 0xFC) |
                     0x80000000UL;
            
            pci_hal_outl(PCI_CONFIG_ADDRESS_X86, alamat);
            pci_hal_outl(PCI_CONFIG_DATA_X86, nilai);
        }
        break;
        
    case PCI_HAL_TYPE_MMIO:
    case PCI_HAL_TYPE_ECAME:
        {
            alamat_fisik_t alamat;
            
            alamat = g_pci_hal.base_address |
                     ((tak_bertanda64_t)bus << PCI_ECAM_BUS_SHIFT) |
                     ((tak_bertanda64_t)dev << PCI_ECAM_DEV_SHIFT) |
                     ((tak_bertanda64_t)func << PCI_ECAM_FUNC_SHIFT) |
                     (offset & 0xFFC);
            
            pci_hal_mmio_write(alamat, nilai);
        }
        break;
        
    default:
        break;
    }
}

/*
 * ===========================================================================
 * FUNGSI I/O PCI HIGH-LEVEL
 * ===========================================================================
 */

/*
 * pci_baca_konfigurasi32 - Baca 32-bit dari PCI config space
 */
static tak_bertanda32_t pci_baca_konfigurasi32(tak_bertanda8_t bus,
                                               tak_bertanda8_t dev,
                                               tak_bertanda8_t func,
                                               tak_bertanda8_t offset)
{
    return pci_hal_baca_konfigurasi32(bus, dev, func, offset);
}

/*
 * pci_baca_konfigurasi16 - Baca 16-bit dari PCI config space
 */
static tak_bertanda16_t pci_baca_konfigurasi16(tak_bertanda8_t bus,
                                               tak_bertanda8_t dev,
                                               tak_bertanda8_t func,
                                               tak_bertanda8_t offset)
{
    tak_bertanda32_t nilai;
    tak_bertanda16_t hasil;
    
    nilai = pci_baca_konfigurasi32(bus, dev, func, offset);
    
    /* Extract 16-bit value berdasarkan offset */
    if (offset & 0x02) {
        hasil = (tak_bertanda16_t)((nilai >> 16) & 0xFFFF);
    } else {
        hasil = (tak_bertanda16_t)(nilai & 0xFFFF);
    }
    
    return hasil;
}

/*
 * pci_baca_konfigurasi8 - Baca 8-bit dari PCI config space
 */
static tak_bertanda8_t pci_baca_konfigurasi8(tak_bertanda8_t bus,
                                             tak_bertanda8_t dev,
                                             tak_bertanda8_t func,
                                             tak_bertanda8_t offset)
{
    tak_bertanda32_t nilai;
    tak_bertanda8_t hasil;
    tak_bertanda32_t shift;
    
    nilai = pci_baca_konfigurasi32(bus, dev, func, offset);
    shift = (tak_bertanda32_t)((offset & 0x03) * 8);
    hasil = (tak_bertanda8_t)((nilai >> shift) & 0xFF);
    
    return hasil;
}

/*
 * pci_tulis_konfigurasi32 - Tulis 32-bit ke PCI config space
 */
static void pci_tulis_konfigurasi32(tak_bertanda8_t bus,
                                     tak_bertanda8_t dev,
                                     tak_bertanda8_t func,
                                     tak_bertanda8_t offset,
                                     tak_bertanda32_t nilai)
{
    pci_hal_tulis_konfigurasi32(bus, dev, func, offset, nilai);
}

/*
 * pci_tulis_konfigurasi16 - Tulis 16-bit ke PCI config space
 * Fungsi ini disediakan untuk penggunaan masa depan
 */
__attribute__((unused))
static void pci_tulis_konfigurasi16(tak_bertanda8_t bus,
                                     tak_bertanda8_t dev,
                                     tak_bertanda8_t func,
                                     tak_bertanda8_t offset,
                                     tak_bertanda16_t nilai)
{
    tak_bertanda32_t nilai32;
    tak_bertanda32_t mask;
    
    nilai32 = pci_baca_konfigurasi32(bus, dev, func, offset);
    
    if (offset & 0x02) {
        mask = 0x0000FFFF;
        nilai32 = (nilai32 & mask) | ((tak_bertanda32_t)nilai << 16);
    } else {
        mask = 0xFFFF0000;
        nilai32 = (nilai32 & mask) | (tak_bertanda32_t)nilai;
    }
    
    pci_tulis_konfigurasi32(bus, dev, func, offset, nilai32);
}

/*
 * ===========================================================================
 * FUNGSI DETEKSI PCI
 * ===========================================================================
 */

/*
 * pci_cek_perangkat - Cek apakah perangkat ada
 */
static bool_t pci_cek_perangkat(tak_bertanda8_t bus,
                                 tak_bertanda8_t dev,
                                 tak_bertanda8_t func)
{
    tak_bertanda16_t vendor_id;
    
    vendor_id = pci_baca_konfigurasi16(bus, dev, func, PCI_OFFSET_VENDOR);
    
    if (vendor_id == PCI_VENDOR_INVALID || vendor_id == PCI_VENDOR_NONE) {
        return SALAH;
    }
    
    return BENAR;
}

/*
 * pci_baca_info_perangkat - Baca informasi lengkap perangkat
 */
static void pci_baca_info_perangkat(tak_bertanda8_t bus,
                                     tak_bertanda8_t dev,
                                     tak_bertanda8_t func,
                                     tak_bertanda16_t *vendor_id,
                                     tak_bertanda16_t *device_id,
                                     tak_bertanda16_t *subsystem_vendor,
                                     tak_bertanda16_t *subsystem_id,
                                     tak_bertanda32_t *class_code,
                                     tak_bertanda8_t *revision,
                                     tak_bertanda8_t *header_type,
                                     tak_bertanda8_t *irq_line,
                                     tak_bertanda8_t *irq_pin)
{
    tak_bertanda32_t nilai;
    
    /* Baca Vendor ID dan Device ID */
    *vendor_id = pci_baca_konfigurasi16(bus, dev, func, PCI_OFFSET_VENDOR);
    *device_id = pci_baca_konfigurasi16(bus, dev, func, PCI_OFFSET_DEVICE);
    
    /* Baca Class Code dan Revision */
    nilai = pci_baca_konfigurasi32(bus, dev, func, PCI_OFFSET_REVISION);
    *revision = (tak_bertanda8_t)(nilai & 0xFF);
    *class_code = (nilai >> 8) & 0xFFFFFF;
    
    /* Baca Header Type */
    *header_type = pci_baca_konfigurasi8(bus, dev, func, PCI_OFFSET_HEADERTYPE);
    
    /* Baca Subsystem IDs (untuk header type 0) */
    if ((*header_type & 0x7F) == PCI_HEADER_TYPE_NORMAL) {
        *subsystem_vendor = pci_baca_konfigurasi16(bus, dev, func, PCI_OFFSET_SUBSYSTEM_VENDOR);
        *subsystem_id = pci_baca_konfigurasi16(bus, dev, func, PCI_OFFSET_SUBSYSTEM_ID);
    } else {
        *subsystem_vendor = 0;
        *subsystem_id = 0;
    }
    
    /* Baca IRQ info */
    nilai = pci_baca_konfigurasi32(bus, dev, func, PCI_OFFSET_IRQ);
    *irq_line = (tak_bertanda8_t)(nilai & 0xFF);
    *irq_pin = (tak_bertanda8_t)((nilai >> 8) & 0xFF);
}

/*
 * pci_baca_bar - Baca Base Address Register
 */
static alamat_fisik_t pci_baca_bar(tak_bertanda8_t bus,
                                    tak_bertanda8_t dev,
                                    tak_bertanda8_t func,
                                    tak_bertanda8_t bar_index)
{
    tak_bertanda8_t offset;
    tak_bertanda32_t nilai;
    
    if (bar_index >= PCI_MAX_BAR) {
        return 0;
    }
    
    offset = PCI_OFFSET_BAR0 + (bar_index * 4);
    nilai = pci_baca_konfigurasi32(bus, dev, func, offset);
    
    /* Mask flag bits */
    if (nilai & 0x01) {
        /* I/O space - mask lower 2 bits */
        return (alamat_fisik_t)(nilai & 0xFFFFFFFC);
    } else {
        /* Memory space - mask lower 4 bits */
        return (alamat_fisik_t)(nilai & 0xFFFFFFF0);
    }
}

/*
 * pci_baca_bar_ukuran - Baca ukuran BAR
 */
static tak_bertanda32_t pci_baca_bar_ukuran(tak_bertanda8_t bus,
                                             tak_bertanda8_t dev,
                                             tak_bertanda8_t func,
                                             tak_bertanda8_t bar_index)
{
    tak_bertanda8_t offset;
    tak_bertanda32_t nilai_asli;
    tak_bertanda32_t nilai_ukuran;
    tak_bertanda32_t ukuran;
    
    if (bar_index >= PCI_MAX_BAR) {
        return 0;
    }
    
    offset = PCI_OFFSET_BAR0 + (bar_index * 4);
    
    /* Simpan nilai asli */
    nilai_asli = pci_baca_konfigurasi32(bus, dev, func, offset);
    
    /* Tulis 0xFFFFFFFF untuk mendapatkan ukuran */
    pci_tulis_konfigurasi32(bus, dev, func, offset, 0xFFFFFFFF);
    
    /* Baca kembali */
    nilai_ukuran = pci_baca_konfigurasi32(bus, dev, func, offset);
    
    /* Kembalikan nilai asli */
    pci_tulis_konfigurasi32(bus, dev, func, offset, nilai_asli);
    
    /* Hitung ukuran */
    if (nilai_ukuran == 0 || nilai_ukuran == 0xFFFFFFFF) {
        return 0;
    }
    
    /* Mask flag bits */
    if (nilai_asli & 0x01) {
        /* I/O space */
        nilai_ukuran &= 0xFFFFFFFC;
    } else {
        /* Memory space */
        nilai_ukuran &= 0xFFFFFFF0;
    }
    
    /* Hitung ukuran actual: find least significant set bit */
    ukuran = 1;
    while ((nilai_ukuran & 1) == 0) {
        ukuran <<= 1;
        nilai_ukuran >>= 1;
    }
    
    return ukuran;
}

/*
 * pci_baca_bar_flags - Baca flags BAR (type, prefetchable)
 * Fungsi ini disediakan untuk penggunaan masa depan
 */
__attribute__((unused))
static tak_bertanda32_t pci_baca_bar_flags(tak_bertanda8_t bus,
                                            tak_bertanda8_t dev,
                                            tak_bertanda8_t func,
                                            tak_bertanda8_t bar_index)
{
    tak_bertanda8_t offset;
    tak_bertanda32_t nilai;
    tak_bertanda32_t flags;
    
    if (bar_index >= PCI_MAX_BAR) {
        return 0;
    }
    
    offset = PCI_OFFSET_BAR0 + (bar_index * 4);
    nilai = pci_baca_konfigurasi32(bus, dev, func, offset);
    
    flags = 0;
    
    if (nilai & 0x01) {
        /* I/O space */
        flags |= 0x01;  /* IC_BAR_IO */
    } else {
        /* Memory space */
        flags |= 0x02;  /* IC_BAR_MEMORY */
        
        /* Check prefetchable */
        if (nilai & 0x08) {
            flags |= 0x04;  /* IC_BAR_PREFETCHABLE */
        }
        
        /* Memory type */
        flags |= ((nilai >> 1) & 0x03) << 4;  /* IC_BAR_TYPE_MASK */
    }
    
    return flags;
}

/*
 * pci_baca_capabilities - Baca capability list
 * Fungsi ini disediakan untuk penggunaan masa depan
 */
__attribute__((unused))
static status_t pci_baca_capabilities(tak_bertanda8_t bus,
                                       tak_bertanda8_t dev,
                                       tak_bertanda8_t func,
                                       tak_bertanda8_t *caps,
                                       tak_bertanda8_t *jumlah_caps)
{
    tak_bertanda8_t status;
    tak_bertanda8_t cap_ptr;
    tak_bertanda8_t cap_id;
    tak_bertanda8_t count;
    
    if (caps == NULL || jumlah_caps == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    *jumlah_caps = 0;
    count = 0;
    
    /* Cek apakah capabilities supported */
    status = pci_baca_konfigurasi8(bus, dev, func, PCI_OFFSET_STATUS + 1);
    if (!(status & (PCI_STATUS_CAPABILITIES >> 8))) {
        return STATUS_BERHASIL;
    }
    
    /* Baca capability pointer */
    cap_ptr = pci_baca_konfigurasi8(bus, dev, func, PCI_OFFSET_CAPABILITIES);
    
    /* Traverse capability list */
    while (cap_ptr != 0 && count < 16) {
        cap_id = pci_baca_konfigurasi8(bus, dev, func, cap_ptr);
        caps[count++] = cap_id;
        
        /* Get next capability pointer */
        cap_ptr = pci_baca_konfigurasi8(bus, dev, func, cap_ptr + 1);
    }
    
    *jumlah_caps = count;
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PROBE UTAMA
 * ===========================================================================
 */

/*
 * ic_probe_pci_fungsi - Probe satu fungsi PCI
 */
static status_t ic_probe_pci_fungsi(tak_bertanda8_t bus,
                                     tak_bertanda8_t dev,
                                     tak_bertanda8_t func,
                                     tanda32_t *jumlah_ditemukan)
{
    ic_perangkat_t *perangkat;
    tak_bertanda16_t vendor_id;
    tak_bertanda16_t device_id;
    tak_bertanda16_t subsystem_vendor;
    tak_bertanda16_t subsystem_id;
    tak_bertanda32_t class_code;
    tak_bertanda8_t revision;
    tak_bertanda8_t header_type;
    tak_bertanda8_t irq_line;
    tak_bertanda8_t irq_pin;
    tak_bertanda8_t i;
    alamat_fisik_t bar_alamat;
    tak_bertanda32_t bar_ukuran;
    
    /* Cek apakah perangkat ada */
    if (!pci_cek_perangkat(bus, dev, func)) {
        return STATUS_BERHASIL;
    }
    
    /* Baca informasi perangkat */
    pci_baca_info_perangkat(bus, dev, func, &vendor_id, &device_id,
                            &subsystem_vendor, &subsystem_id,
                            &class_code, &revision, &header_type,
                            &irq_line, &irq_pin);
    
    /* Buat entri perangkat baru */
    perangkat = ic_probe_tambah_perangkat(IC_BUS_PCI, bus, dev, func);
    if (perangkat == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    /* Set informasi perangkat */
    ic_probe_set_info(perangkat, vendor_id, device_id, class_code, revision);
    
    /* Baca BARs (hanya untuk header type 0 - normal device) */
    if ((header_type & 0x7F) == PCI_HEADER_TYPE_NORMAL) {
        for (i = 0; i < PCI_MAX_BAR; i++) {
            bar_alamat = pci_baca_bar(bus, dev, func, i);
            bar_ukuran = pci_baca_bar_ukuran(bus, dev, func, i);
            
            if (bar_alamat != 0 || bar_ukuran != 0) {
                ic_probe_set_bar(perangkat, i, bar_alamat, bar_ukuran);
            }
        }
    }
    
    /* Baca IRQ */
    if (irq_line != 0 && irq_line != 0xFF) {
        ic_probe_set_irq(perangkat, irq_line);
    }
    
    /* Tentukan bus tipe (PCI atau PCIe) berdasarkan class code */
    if ((class_code & 0xFFFF00) == PCI_CLASS_BRIDGE_PCI) {
        perangkat->bus = IC_BUS_PCIE;
    } else {
        perangkat->bus = IC_BUS_PCI;
    }
    
    /* Mark as detected */
    perangkat->terdeteksi = BENAR;
    perangkat->status = IC_STATUS_TERDETEKSI;
    
    (*jumlah_ditemukan)++;
    
    return STATUS_BERHASIL;
}

/*
 * ic_probe_pci_device - Probe satu device PCI
 */
static status_t ic_probe_pci_device(tak_bertanda8_t bus,
                                     tak_bertanda8_t dev,
                                     tanda32_t *jumlah_ditemukan)
{
    tak_bertanda8_t func;
    tak_bertanda8_t header_type;
    tak_bertanda16_t vendor_id;
    status_t hasil;
    
    /* Cek fungsi 0 */
    vendor_id = pci_baca_konfigurasi16(bus, dev, 0, PCI_OFFSET_VENDOR);
    if (vendor_id == PCI_VENDOR_INVALID || vendor_id == 0) {
        return STATUS_BERHASIL;
    }
    
    /* Probe fungsi 0 */
    hasil = ic_probe_pci_fungsi(bus, dev, 0, jumlah_ditemukan);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Cek jenis header untuk multi-fungsi */
    header_type = pci_baca_konfigurasi8(bus, dev, 0, PCI_OFFSET_HEADERTYPE);
    
    /* Jika multi-fungsi device, probe fungsi lainnya */
    if (header_type & PCI_HEADER_TYPE_MULTIFUNC) {
        for (func = 1; func < PCI_MAX_FUNCTION; func++) {
            hasil = ic_probe_pci_fungsi(bus, dev, func, jumlah_ditemukan);
            if (hasil != STATUS_BERHASIL) {
                /* Lanjutkan meskipun ada error */
            }
        }
    }
    
    return STATUS_BERHASIL;
}

/*
 * ic_probe_pci_bus - Probe satu bus PCI
 */
static status_t ic_probe_pci_bus(tak_bertanda8_t bus,
                                  tanda32_t *jumlah_ditemukan)
{
    tak_bertanda8_t dev;
    status_t hasil;
    
    for (dev = 0; dev < PCI_MAX_DEVICE; dev++) {
        hasil = ic_probe_pci_device(bus, dev, jumlah_ditemukan);
        if (hasil != STATUS_BERHASIL) {
            /* Lanjutkan meskipun ada error */
        }
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI UTAMA
 * ===========================================================================
 */

/*
 * ic_probe_pci - Scan seluruh bus PCI
 */
tanda32_t ic_probe_pci(void)
{
    tanda32_t jumlah_ditemukan = 0;
    tak_bertanda16_t bus;
    status_t hasil;
    
    if (!g_probe_diinisialisasi) {
        return -1;
    }
    
    /* Inisialisasi HAL */
    hasil = pci_hal_init();
    if (hasil != STATUS_BERHASIL) {
        return -1;
    }
    
    /* Scan semua bus (0-255) */
    for (bus = 0; bus < PCI_MAX_BUS; bus++) {
        hasil = ic_probe_pci_bus((tak_bertanda8_t)bus, &jumlah_ditemukan);
        if (hasil != STATUS_BERHASIL) {
            /* Lanjutkan meskipun ada error */
        }
    }
    
    return jumlah_ditemukan;
}

/*
 * pci_get_hal_type - Dapatkan tipe HAL yang digunakan
 */
pci_hal_type_t pci_get_hal_type(void)
{
    return g_pci_hal.type;
}

/*
 * pci_is_x86_port_mode - Cek apakah menggunakan x86 I/O port mode
 */
bool_t pci_is_x86_port_mode(void)
{
    return (g_pci_hal.type == PCI_HAL_TYPE_X86_PORT);
}

/*
 * pci_is_mmio_mode - Cek apakah menggunakan MMIO mode
 */
bool_t pci_is_mmio_mode(void)
{
    return (g_pci_hal.type == PCI_HAL_TYPE_MMIO || 
            g_pci_hal.type == PCI_HAL_TYPE_ECAME);
}
