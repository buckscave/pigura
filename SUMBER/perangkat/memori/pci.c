/*
 * PIGURA OS - PCI.C
 * =================
 * Implementasi PCI controller untuk Pigura OS.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi untuk mengakses
 * dan mengelola perangkat PCI (Peripheral Component Interconnect).
 *
 * Kepatuhan standar:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Mendukung semua arsitektur (x86, x86_64, ARM, ARMv7, ARM64)
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "memori.h"
#include "../../inti/kernel.h"

/*
 * ===========================================================================
 * KONSTANTA LOKAL
 * ===========================================================================
 */

/* PCI I/O ports */
#define PCI_CONFIG_ADDRESS      0xCF8
#define PCI_CONFIG_DATA         0xCFC

/* PCI configuration register offsets */
#define PCI_REG_VENDOR_ID       0x00
#define PCI_REG_DEVICE_ID       0x02
#define PCI_REG_COMMAND         0x04
#define PCI_REG_STATUS          0x06
#define PCI_REG_REVISION        0x08
#define PCI_REG_CLASS_CODE      0x09
#define PCI_REG_SUBCLASS        0x0A
#define PCI_REG_PROG_IF         0x09
#define PCI_REG_CACHE_LINE      0x0C
#define PCI_REG_LATENCY         0x0D
#define PCI_REG_HEADER_TYPE     0x0E
#define PCI_REG_BIST            0x0F
#define PCI_REG_BAR0            0x10
#define PCI_REG_BAR1            0x14
#define PCI_REG_BAR2            0x18
#define PCI_REG_BAR3            0x1C
#define PCI_REG_BAR4            0x20
#define PCI_REG_BAR5            0x24
#define PCI_REG_SUBSYSTEM_ID    0x2E
#define PCI_REG_SUBSYSTEM_VID   0x2C
#define PCI_REG_EXPROM_BASE     0x30
#define PCI_REG_CAP_PTR         0x34
#define PCI_REG_INT_LINE        0x3C
#define PCI_REG_INT_PIN         0x3D
#define PCI_REG_MIN_GNT         0x3E
#define PCI_REG_MAX_LAT         0x3F

/* Header types */
#define PCI_HEADER_TYPE_NORMAL  0x00
#define PCI_HEADER_TYPE_BRIDGE  0x01
#define PCI_HEADER_TYPE_CARDBUS 0x02
#define PCI_HEADER_TYPE_MULTI   0x80

/* Vendor ID invalid */
#define PCI_VENDOR_INVALID      0xFFFF

/* Capability IDs */
#define PCI_CAP_ID_PM           0x01
#define PCI_CAP_ID_AGP          0x02
#define PCI_CAP_ID_VPD          0x03
#define PCI_CAP_ID_SLOTID       0x04
#define PCI_CAP_ID_MSI          0x05
#define PCI_CAP_ID_CHSWP        0x06
#define PCI_CAP_ID_PCIX         0x07
#define PCI_CAP_ID_HT           0x08
#define PCI_CAP_ID_VNDR         0x09
#define PCI_CAP_ID_DBG          0x0A
#define PCI_CAP_ID_CCRC         0x0B
#define PCI_CAP_ID_SHPC         0x0C
#define PCI_CAP_ID_SSVID        0x0D
#define PCI_CAP_ID_AGP3         0x0E
#define PCI_CAP_ID_SECDEV       0x0F
#define PCI_CAP_ID_EXP          0x10
#define PCI_CAP_ID_MSIX         0x11

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

/* PCI controller global */
static pci_controller_t g_pci_controller;

/* Device pool untuk alokasi */
static pci_device_t g_pci_device_pool[PCI_BUS_MAKS * PCI_DEVICE_MAKS];

/* Status inisialisasi */
static bool_t g_pci_initialized = SALAH;

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * pci_make_address - Buat alamat konfigurasi PCI
 *
 * Parameter:
 *   bus - Nomor bus
 *   dev - Nomor device
 *   func - Nomor function
 *   reg - Register offset
 *
 * Return: Alamat konfigurasi
 */
static tak_bertanda32_t pci_make_address(tak_bertanda8_t bus,
                                         tak_bertanda8_t dev,
                                         tak_bertanda8_t func,
                                         tak_bertanda8_t reg)
{
    tak_bertanda32_t alamat;

    alamat = 0x80000000;
    alamat |= ((tak_bertanda32_t)bus << 16);
    alamat |= ((tak_bertanda32_t)dev << 11);
    alamat |= ((tak_bertanda32_t)func << 8);
    alamat |= ((tak_bertanda32_t)reg & 0xFC);

    return alamat;
}

/*
 * pci_config_read32 - Baca konfigurasi 32-bit
 *
 * Parameter:
 *   bus - Nomor bus
 *   dev - Nomor device
 *   func - Nomor function
 *   reg - Register offset
 *
 * Return: Nilai yang dibaca
 */
static tak_bertanda32_t pci_config_read32(tak_bertanda8_t bus,
                                          tak_bertanda8_t dev,
                                          tak_bertanda8_t func,
                                          tak_bertanda8_t reg)
{
    tak_bertanda32_t alamat;

    alamat = pci_make_address(bus, dev, func, reg);

    outl(PCI_CONFIG_ADDRESS, alamat);

    return inl(PCI_CONFIG_DATA);
}

/*
 * pci_config_write32 - Tulis konfigurasi 32-bit
 *
 * Parameter:
 *   bus - Nomor bus
 *   dev - Nomor device
 *   func - Nomor function
 *   reg - Register offset
 *   nilai - Nilai yang akan ditulis
 */
static void pci_config_write32(tak_bertanda8_t bus, tak_bertanda8_t dev,
                               tak_bertanda8_t func, tak_bertanda8_t reg,
                               tak_bertanda32_t nilai)
{
    tak_bertanda32_t alamat;

    alamat = pci_make_address(bus, dev, func, reg);

    outl(PCI_CONFIG_ADDRESS, alamat);
    outl(PCI_CONFIG_DATA, nilai);
}

/*
 * pci_device_pool_alokasi - Alokasi device dari pool
 *
 * Return: Pointer ke device atau NULL
 */
static pci_device_t *pci_device_pool_alokasi(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < (PCI_BUS_MAKS * PCI_DEVICE_MAKS); i++) {
        if (g_pci_device_pool[i].magic != PCI_CTRL_MAGIC) {
            g_pci_device_pool[i].magic = PCI_CTRL_MAGIC;
            return &g_pci_device_pool[i];
        }
    }

    return NULL;
}

/*
 * pci_device_pool_bebaskan - Bebaskan device ke pool
 *
 * Parameter:
 *   dev - Pointer ke device
 */
static void pci_device_pool_bebaskan(pci_device_t *dev)
{
    if (dev != NULL) {
        kernel_memset(dev, 0, sizeof(pci_device_t));
    }
}

/*
 * pci_scan_bus - Scan satu bus PCI
 *
 * Parameter:
 *   bus - Nomor bus
 *
 * Return: Jumlah device ditemukan
 */
static tak_bertanda32_t pci_scan_bus(tak_bertanda8_t bus)
{
    pci_device_t *dev;
    tak_bertanda32_t dev_count;
    tak_bertanda32_t nilai32;
    tak_bertanda16_t vendor;
    tak_bertanda16_t device;
    tak_bertanda8_t header_type;
    tak_bertanda8_t func;
    tak_bertanda8_t dev_num;

    dev_count = 0;

    for (dev_num = 0; dev_num < PCI_DEVICE_MAKS; dev_num++) {
        /* Cek vendor ID untuk device 0 */
        nilai32 = pci_config_read32(bus, dev_num, 0, PCI_REG_VENDOR_ID);
        vendor = (tak_bertanda16_t)(nilai32 & 0xFFFF);

        if (vendor == PCI_VENDOR_INVALID) {
            continue;
        }

        device = (tak_bertanda16_t)((nilai32 >> 16) & 0xFFFF);

        /* Cek header type untuk multi-function */
        nilai32 = pci_config_read32(bus, dev_num, 0, PCI_REG_HEADER_TYPE);
        header_type = (tak_bertanda8_t)((nilai32 >> 16) & 0xFF);

        /* Scan semua function */
        if (header_type & PCI_HEADER_TYPE_MULTI) {
            func = 0;
            while (func < PCI_FUNCTION_MAKS) {
                nilai32 = pci_config_read32(bus, dev_num, func,
                                           PCI_REG_VENDOR_ID);
                vendor = (tak_bertanda16_t)(nilai32 & 0xFFFF);

                if (vendor != PCI_VENDOR_INVALID) {
                    dev = pci_device_pool_alokasi();
                    if (dev != NULL) {
                        dev->bus = bus;
                        dev->device = dev_num;
                        dev->function = func;
                        dev->vendor_id = vendor;
                        dev->device_id =
                            (tak_bertanda16_t)((nilai32 >> 16) & 0xFFFF);
                        dev->berikutnya = g_pci_controller.device_list;
                        g_pci_controller.device_list = dev;
                        dev_count++;
                    }
                }
                func++;
            }
        } else {
            /* Single function device */
            dev = pci_device_pool_alokasi();
            if (dev != NULL) {
                dev->bus = bus;
                dev->device = dev_num;
                dev->function = 0;
                dev->vendor_id = vendor;
                dev->device_id = device;
                dev->berikutnya = g_pci_controller.device_list;
                g_pci_controller.device_list = dev;
                dev_count++;
            }
        }
    }

    return dev_count;
}

/*
 * pci_read_bars - Baca semua BAR device
 *
 * Parameter:
 *   dev - Pointer ke device
 */
static void pci_read_bars(pci_device_t *dev)
{
    tak_bertanda32_t i;
    tak_bertanda32_t bar_asli;
    tak_bertanda32_t ukuran;

    for (i = 0; i < PCI_BAR_MAKS; i++) {
        /* Baca BAR asli */
        bar_asli = pci_config_read32(dev->bus, dev->device, dev->function,
                                     (tak_bertanda8_t)(PCI_REG_BAR0 + (i * 4)));

        /* Tulis semua 1 untuk mendapatkan ukuran */
        pci_config_write32(dev->bus, dev->device, dev->function,
                          (tak_bertanda8_t)(PCI_REG_BAR0 + (i * 4)), 0xFFFFFFFF);

        /* Baca ukuran */
        ukuran = pci_config_read32(dev->bus, dev->device, dev->function,
                                  (tak_bertanda8_t)(PCI_REG_BAR0 + (i * 4)));

        /* Kembalikan nilai asli */
        pci_config_write32(dev->bus, dev->device, dev->function,
                          (tak_bertanda8_t)(PCI_REG_BAR0 + (i * 4)), bar_asli);

        /* Parse BAR */
        if (bar_asli & 0x01) {
            /* I/O space BAR */
            dev->bar[i] = bar_asli & 0xFFFFFFFC;
            dev->bar_type[i] = 1;  /* I/O */
            ukuran &= 0xFFFFFFFC;
        } else {
            /* Memory space BAR */
            dev->bar[i] = bar_asli & 0xFFFFFFF0;
            dev->bar_type[i] = 0;  /* Memory */
            ukuran &= 0xFFFFFFF0;
        }

        /* Hitung ukuran */
        if (ukuran != 0) {
            ukuran = (~ukuran) + 1;
            dev->bar_size[i] = ukuran;
        } else {
            dev->bar_size[i] = 0;
        }
    }
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * pci_init - Inisialisasi PCI controller
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t pci_init(void)
{
    if (g_pci_initialized) {
        return STATUS_SUDAH_ADA;
    }

    /* Reset controller */
    kernel_memset(&g_pci_controller, 0, sizeof(pci_controller_t));
    kernel_memset(g_pci_device_pool, 0, sizeof(g_pci_device_pool));

    /* Set magic */
    g_pci_controller.magic = PCI_CTRL_MAGIC;

    /* Set konfigurasi default */
    g_pci_controller.segment = 0;
    g_pci_controller.bus_start = 0;
    g_pci_controller.bus_end = 255;
    g_pci_controller.config_base = PCI_CONFIG_ADDRESS;

    g_pci_initialized = BENAR;
    g_pci_controller.initialized = BENAR;

    return STATUS_BERHASIL;
}

/*
 * pci_scan - Scan bus PCI
 *
 * Return: Jumlah device ditemukan atau nilai negatif
 */
tanda32_t pci_scan(void)
{
    tak_bertanda32_t total_devices;
    tak_bertanda32_t bus;
    tak_bertanda32_t found;

    if (!g_pci_initialized) {
        return STATUS_PARAM_INVALID;
    }

    total_devices = 0;

    /* Scan semua bus */
    for (bus = g_pci_controller.bus_start; bus <= g_pci_controller.bus_end;
         bus++) {
        found = pci_scan_bus((tak_bertanda8_t)bus);
        total_devices += found;
    }

    g_pci_controller.device_count = total_devices;
    g_pci_controller.devices_found = total_devices;

    return (tanda32_t)total_devices;
}

/*
 * pci_read_config - Baca configuration space
 *
 * Parameter:
 *   bus - Nomor bus
 *   dev - Nomor device
 *   func - Nomor function
 *   reg - Register offset
 *   size - Ukuran (1, 2, atau 4 bytes)
 *
 * Return: Nilai yang dibaca
 */
tak_bertanda32_t pci_read_config(tak_bertanda8_t bus, tak_bertanda8_t dev,
                                 tak_bertanda8_t func, tak_bertanda8_t reg,
                                 tak_bertanda32_t size)
{
    tak_bertanda32_t nilai;

    nilai = pci_config_read32(bus, dev, func, reg);

    switch (size) {
    case 1:
        nilai = (nilai >> ((reg & 0x03) * 8)) & 0xFF;
        break;
    case 2:
        nilai = (nilai >> ((reg & 0x02) * 8)) & 0xFFFF;
        break;
    case 4:
    default:
        break;
    }

    return nilai;
}

/*
 * pci_write_config - Tulis configuration space
 *
 * Parameter:
 *   bus - Nomor bus
 *   dev - Nomor device
 *   func - Nomor function
 *   reg - Register offset
 *   value - Nilai yang akan ditulis
 *   size - Ukuran (1, 2, atau 4 bytes)
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t pci_write_config(tak_bertanda8_t bus, tak_bertanda8_t dev,
                          tak_bertanda8_t func, tak_bertanda8_t reg,
                          tak_bertanda32_t value, tak_bertanda32_t size)
{
    tak_bertanda32_t nilai_asli;
    tak_bertanda32_t mask;
    tak_bertanda32_t shift;

    if (size == 4) {
        pci_config_write32(bus, dev, func, reg, value);
        return STATUS_BERHASIL;
    }

    /* Untuk ukuran < 4, baca-modify-tulis */
    nilai_asli = pci_config_read32(bus, dev, func, reg);

    shift = (reg & 0x03) * 8;

    if (size == 1) {
        mask = 0xFF;
    } else {
        mask = 0xFFFF;
        shift = (reg & 0x02) * 8;
    }

    nilai_asli &= ~(mask << shift);
    nilai_asli |= (value & mask) << shift;

    pci_config_write32(bus, dev, func, reg, nilai_asli);

    return STATUS_BERHASIL;
}

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
                              tak_bertanda16_t device_id)
{
    pci_device_t *dev;

    if (!g_pci_initialized) {
        return NULL;
    }

    dev = g_pci_controller.device_list;

    while (dev != NULL) {
        if (dev->vendor_id == vendor_id && dev->device_id == device_id) {
            return dev;
        }
        dev = dev->berikutnya;
    }

    return NULL;
}

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
                                    tak_bertanda32_t index)
{
    pci_device_t *dev;
    tak_bertanda32_t count;

    if (!g_pci_initialized) {
        return NULL;
    }

    dev = g_pci_controller.device_list;
    count = 0;

    while (dev != NULL) {
        if ((dev->class_code & 0xFF00) == class_code) {
            if (count == index) {
                return dev;
            }
            count++;
        }
        dev = dev->berikutnya;
    }

    return NULL;
}

/*
 * pci_enable_device - Enable PCI device
 *
 * Parameter:
 *   dev - Pointer ke device
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t pci_enable_device(pci_device_t *dev)
{
    tak_bertanda16_t command;

    if (dev == NULL || dev->magic != PCI_CTRL_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Baca command register */
    command = (tak_bertanda16_t)pci_read_config(dev->bus, dev->device,
                                                dev->function,
                                                PCI_REG_COMMAND, 2);

    /* Enable I/O, memory, dan bus master */
    command |= PCI_CMD_IO_SPACE | PCI_CMD_MEMORY_SPACE | PCI_CMD_BUS_MASTER;

    /* Tulis kembali */
    pci_write_config(dev->bus, dev->device, dev->function, PCI_REG_COMMAND,
                    command, 2);

    dev->command = command;

    /* Baca informasi device */
    dev->revision = (tak_bertanda16_t)pci_read_config(dev->bus, dev->device,
                                                      dev->function,
                                                      PCI_REG_REVISION, 1);

    dev->class_code = (tak_bertanda16_t)pci_read_config(dev->bus, dev->device,
                                                        dev->function,
                                                        PCI_REG_CLASS_CODE, 2);

    dev->irq_pin = (tak_bertanda8_t)pci_read_config(dev->bus, dev->device,
                                                    dev->function,
                                                    PCI_REG_INT_PIN, 1);

    dev->irq_line = (tak_bertanda8_t)pci_read_config(dev->bus, dev->device,
                                                     dev->function,
                                                     PCI_REG_INT_LINE, 1);

    dev->subsystem_vendor =
        (tak_bertanda16_t)pci_read_config(dev->bus, dev->device,
                                          dev->function,
                                          PCI_REG_SUBSYSTEM_VID, 2);
    dev->subsystem_id =
        (tak_bertanda16_t)pci_read_config(dev->bus, dev->device,
                                          dev->function,
                                          PCI_REG_SUBSYSTEM_ID, 2);

    /* Baca BARs */
    pci_read_bars(dev);

    return STATUS_BERHASIL;
}

/*
 * pci_disable_device - Disable PCI device
 *
 * Parameter:
 *   dev - Pointer ke device
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t pci_disable_device(pci_device_t *dev)
{
    tak_bertanda16_t command;

    if (dev == NULL || dev->magic != PCI_CTRL_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Baca command register */
    command = (tak_bertanda16_t)pci_read_config(dev->bus, dev->device,
                                                dev->function,
                                                PCI_REG_COMMAND, 2);

    /* Disable I/O, memory, dan bus master */
    command &= ~(PCI_CMD_IO_SPACE | PCI_CMD_MEMORY_SPACE | PCI_CMD_BUS_MASTER);

    /* Tulis kembali */
    pci_write_config(dev->bus, dev->device, dev->function, PCI_REG_COMMAND,
                    command, 2);

    dev->command = command;

    return STATUS_BERHASIL;
}

/*
 * pci_set_bus_master - Set bus master
 *
 * Parameter:
 *   dev - Pointer ke device
 *   enable - Enable/disable
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t pci_set_bus_master(pci_device_t *dev, bool_t enable)
{
    tak_bertanda16_t command;

    if (dev == NULL || dev->magic != PCI_CTRL_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Baca command register */
    command = (tak_bertanda16_t)pci_read_config(dev->bus, dev->device,
                                                dev->function,
                                                PCI_REG_COMMAND, 2);

    if (enable) {
        command |= PCI_CMD_BUS_MASTER;
    } else {
        command &= ~PCI_CMD_BUS_MASTER;
    }

    /* Tulis kembali */
    pci_write_config(dev->bus, dev->device, dev->function, PCI_REG_COMMAND,
                    command, 2);

    dev->command = command;

    return STATUS_BERHASIL;
}

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
                     alamat_fisik_t *addr, ukuran_t *size)
{
    if (dev == NULL || dev->magic != PCI_CTRL_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    if (index >= PCI_BAR_MAKS) {
        return STATUS_PARAM_INVALID;
    }

    if (addr != NULL) {
        *addr = dev->bar[index];
    }

    if (size != NULL) {
        *size = dev->bar_size[index];
    }

    return STATUS_BERHASIL;
}

/*
 * pci_enable_irq - Enable IRQ untuk device
 *
 * Parameter:
 *   dev - Pointer ke device
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t pci_enable_irq(pci_device_t *dev)
{
    tak_bertanda16_t command;

    if (dev == NULL || dev->magic != PCI_CTRL_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Baca command register */
    command = (tak_bertanda16_t)pci_read_config(dev->bus, dev->device,
                                                dev->function,
                                                PCI_REG_COMMAND, 2);

    /* Clear INT_DISABLE flag */
    command &= ~PCI_CMD_INT_DISABLE;

    /* Tulis kembali */
    pci_write_config(dev->bus, dev->device, dev->function, PCI_REG_COMMAND,
                    command, 2);

    dev->command = command;

    return STATUS_BERHASIL;
}

/*
 * pci_nama_class - Dapatkan nama PCI class
 *
 * Parameter:
 *   class_code - Class code
 *
 * Return: String nama class
 */
const char *pci_nama_class(tak_bertanda16_t class_code)
{
    switch (class_code & 0xFF00) {
    case PCI_CLASS_STORAGE:
        return "Storage";
    case PCI_CLASS_NETWORK:
        return "Network";
    case PCI_CLASS_DISPLAY:
        return "Display";
    case PCI_CLASS_MULTIMEDIA:
        return "Multimedia";
    case PCI_CLASS_MEMORY:
        return "Memory";
    case PCI_CLASS_BRIDGE:
        return "Bridge";
    case PCI_CLASS_COMM:
        return "Communication";
    case PCI_CLASS_SYSTEM:
        return "System";
    case PCI_CLASS_INPUT:
        return "Input";
    case PCI_CLASS_DOCKING:
        return "Docking";
    case PCI_CLASS_PROCESSOR:
        return "Processor";
    case PCI_CLASS_SERIAL:
        return "Serial Bus";
    case PCI_CLASS_WIRELESS:
        return "Wireless";
    case PCI_CLASS_SATELLITE:
        return "Satellite";
    case PCI_CLASS_CRYPTO:
        return "Encryption";
    case PCI_CLASS_SIGNAL:
        return "Signal Processing";
    default:
        return "Unknown";
    }
}

/*
 * pci_cetak_info - Cetak informasi PCI
 */
void pci_cetak_info(void)
{
    if (!g_pci_initialized) {
        kernel_printf("PCI: Belum diinisialisasi\n");
        return;
    }

    kernel_printf("\n=== PCI Controller ===\n\n");
    kernel_printf("Segment: %u\n", g_pci_controller.segment);
    kernel_printf("Bus range: %u-%u\n", g_pci_controller.bus_start,
                 g_pci_controller.bus_end);
    kernel_printf("Devices found: %u\n", g_pci_controller.devices_found);
}

/*
 * pci_cetak_device - Cetak informasi device
 *
 * Parameter:
 *   dev - Pointer ke device
 */
void pci_cetak_device(pci_device_t *dev)
{
    if (dev == NULL) {
        return;
    }

    kernel_printf("\nPCI Device %02X:%02X.%X\n",
                 dev->bus, dev->device, dev->function);
    kernel_printf("  Vendor ID: %04X  Device ID: %04X\n",
                 dev->vendor_id, dev->device_id);
    kernel_printf("  Class: %s (%04X)\n",
                 pci_nama_class(dev->class_code), dev->class_code);
    kernel_printf("  IRQ: %u (Pin %c)\n",
                 dev->irq_line, 'A' + dev->irq_pin - 1);
}
