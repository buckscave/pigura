/*
 * PIGURA OS - AHCI.C
 * ===================
 * Implementasi driver AHCI (Advanced Host Controller Interface)
 * untuk perangkat penyimpanan SATA.
 *
 * Berkas ini menyediakan fungsi-fungsi untuk mengakses perangkat
 * penyimpanan SATA melalui controller AHCI.
 *
 * Kepatuhan standar:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Mendukung semua arsitektur: x86, x86_64, ARM, ARMv7, ARM64
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../cpu/cpu.h"
#include "../../inti/kernel.h"

/*
 * ===========================================================================
 * KONSTANTA AHCI
 * ===========================================================================
 */

#define AHCI_VERSI_MAJOR 1
#define AHCI_VERSI_MINOR 0

/* Magic number */
#define AHCI_MAGIC           0x41484349  /* "AHCI" */
#define AHCI_PORT_MAGIC      0x41504F52  /* "APOR" */
#define AHCI_DEVICE_MAGIC    0x41444556  /* "ADEV" */

/* Register offset controller */
#define AHCI_REG_CAP         0x0000  /* HBA Capabilities */
#define AHCI_REG_GHC         0x0004  /* Global HBA Control */
#define AHCI_REG_IS          0x0008  /* Interrupt Status */
#define AHCI_REG_PI          0x000C  /* Ports Implemented */
#define AHCI_REG_VS          0x0010  /* AHCI Version */
#define AHCI_REG_CCC_CTL     0x0014  /* CCC Control */
#define AHCI_REG_CCC_PORTS   0x0018  /* CCC Ports */
#define AHCI_REG_EM_LOC      0x001C  /* Enclosure Mgmt Location */
#define AHCI_REG_EM_CTL      0x0020  /* Enclosure Mgmt Control */
#define AHCI_REG_CAP2        0x0024  /* HBA Capabilities Extended */

/* Register offset port (0x80 + 0x10 * port) */
#define AHCI_PORT_CLB        0x00    /* Command List Base */
#define AHCI_PORT_CLBU       0x04    /* Command List Base Upper */
#define AHCI_PORT_FB         0x08    /* FIS Base */
#define AHCI_PORT_FBU        0x0C    /* FIS Base Upper */
#define AHCI_PORT_IS         0x10    /* Interrupt Status */
#define AHCI_PORT_IE         0x14    /* Interrupt Enable */
#define AHCI_PORT_CMD        0x18    /* Command and Status */
#define AHCI_PORT_RESERVED   0x1C
#define AHCI_PORT_TFD        0x20    /* Task File Data */
#define AHCI_PORT_SIG        0x24    /* Signature */
#define AHCI_PORT_SSTS       0x28    /* SATA Status */
#define AHCI_PORT_SCTL       0x2C    /* SATA Control */
#define AHCI_PORT_SERR       0x30    /* SATA Error */
#define AHCI_PORT_SACT       0x34    /* SATA Active */
#define AHCI_PORT_CI         0x38    /* Command Issue */
#define AHCI_PORT_SNTF       0x3C    /* SATA Notification */
#define AHCI_PORT_FBS        0x40    /* FIS-based Switching Control */

/* GHC flags */
#define AHCI_GHC_HR          0x00000001  /* HBA Reset */
#define AHCI_GHC_IE          0x00000002  /* Interrupt Enable */
#define AHCI_GHC_MRSM        0x00000004  /* MSI Revert to Single Msg */
#define AHCI_GHC_AE          0x80000000  /* AHCI Enable */

/* CMD flags */
#define AHCI_CMD_ST          0x00000001  /* Start */
#define AHCI_CMD_SUD         0x00000002  /* Spin-Up Device */
#define AHCI_CMD_POD         0x00000004  /* Power On Device */
#define AHCI_CMD_CLO         0x00000008  /* Command List Override */
#define AHCI_CMD_FRE         0x00000010  /* FIS Receive Enable */
#define AHCI_CMD_CCS         0x00001F00  /* Current Command Slot */
#define AHCI_CMD_ISS         0x00002000  /* Interface Communication State */
#define AHCI_CMD_FR          0x00004000  /* FIS Receive Running */
#define AHCI_CMD_CR          0x00008000  /* Command List Running */

/* SSTS values */
#define AHCI_SSTS_DET_NO_DEV     0x00
#define AHCI_SSTS_DET_PRESENT    0x01
#define AHCI_SSTS_DET_PHY_OFF    0x02
#define AHCI_SSTS_DET_PHY_ON     0x03

/* Port signature */
#define AHCI_SIG_ATA         0x00000101
#define AHCI_SIG_ATAPI       0xEB140101
#define AHCI_SIG_PM          0x96690101

/* FIS tipe */
#define AHCI_FIS_REG_H2D     0x27
#define AHCI_FIS_REG_D2H     0x34
#define AHCI_FIS_DMA_ACT     0x39
#define AHCI_FIS_DMA_SETUP   0x41
#define AHCI_FIS_DATA        0x46
#define AHCI_FIS_BIST        0x58
#define AHCI_FIS_PIO_SETUP   0x5F
#define AHCI_FIS_DEV_BITS    0xA1

/* Command flags */
#define AHCI_CMD_FLAG_WRITE  0x02
#define AHCI_CMD_FLAG_PREFETCH 0x04
#define AHCI_CMD_FLAG_RESET  0x08
#define AHCI_CMD_FLAG_BIST   0x10
#define AHCI_CMD_FLAG_CLEAR  0x20

/* Jumlah maksimum */
#define AHCI_MAX_PORTS       32
#define AHCI_MAX_COMMANDS    32
#define AHCI_MAX_SLOTS       32

/* Ukuran struktur */
#define AHCI_COMMAND_SIZE    32
#define AHCI_PRDT_SIZE       16
#define AHCI_FIS_SIZE        256
#define AHCI_COMMAND_TABLE_SIZE  (AHCI_FIS_SIZE + \
                                  (AHCI_PRDT_SIZE * 256))

/*
 * ===========================================================================
 * STRUKTUR DATA AHCI
 * ===========================================================================
 */

/* Command table entry */
typedef struct {
    tak_bertanda8_t fis[64];            /* FIS (64 byte) */
    tak_bertanda8_t reserved[16];       /* Reserved */
    /* PRDT mengikuti */
} ahci_command_table_t;

/* PRDT (Physical Region Descriptor Table) entry */
typedef struct {
    tak_bertanda64_t dba;               /* Data Base Address */
    tak_bertanda32_t reserved;
    tak_bertanda32_t dbc;               /* Data Byte Count (bit 0-21) */
} __attribute__((packed)) ahci_prdt_t;

/* Command header */
typedef struct {
    tak_bertanda16_t flags;             /* Flags dan PRDT length */
    tak_bertanda16_t prdtl;             /* PRDT table length */
    tak_bertanda32_t prdt_byte_count;   /* PRDT byte count */
    tak_bertanda64_t ctba;              /* Command Table Base Address */
    tak_bertanda32_t reserved[4];
} __attribute__((packed)) ahci_cmd_header_t;

/* FIS Register - Host to Device */
typedef struct {
    tak_bertanda8_t fis_type;
    tak_bertanda8_t pm_port;
    tak_bertanda8_t command;
    tak_bertanda8_t features;
    tak_bertanda8_t lba_low;
    tak_bertanda8_t lba_mid;
    tak_bertanda8_t lba_high;
    tak_bertanda8_t device;
    tak_bertanda8_t lba_low_exp;
    tak_bertanda8_t lba_mid_exp;
    tak_bertanda8_t lba_high_exp;
    tak_bertanda8_t features_exp;
    tak_bertanda8_t sector_count;
    tak_bertanda8_t sector_count_exp;
    tak_bertanda8_t reserved;
    tak_bertanda8_t control;
    tak_bertanda32_t reserved2;
} __attribute__((packed)) ahci_fis_h2d_t;

/* Perangkat AHCI */
typedef struct {
    tak_bertanda32_t magic;
    tak_bertanda32_t id;
    bool_t valid;
    bool_t present;
    tak_bertanda8_t port_num;
    tak_bertanda32_t signature;

    /* Kapasitas */
    tak_bertanda64_t total_sectors;
    tak_bertanda64_t size_bytes;
    ukuran_t sector_size;

    /* Identifikasi */
    char model[41];
    char serial[21];

    /* Statistik */
    tak_bertanda64_t read_ops;
    tak_bertanda64_t write_ops;
    tak_bertanda64_t errors;
} ahci_device_t;

/* Port AHCI */
typedef struct {
    tak_bertanda32_t magic;
    tak_bertanda32_t port_num;
    volatile tak_bertanda32_t *reg_base;

    /* Memory untuk command list dan FIS */
    void *cmd_list;
    void *fis_recv;
    void *cmd_table;

    /* Status */
    bool_t active;
    ahci_device_t device;
} ahci_port_t;

/* Konteks AHCI */
typedef struct {
    tak_bertanda32_t magic;
    bool_t diinisialisasi;
    volatile tak_bertanda32_t *mmio_base;
    tak_bertanda32_t cap;
    tak_bertanda32_t cap2;
    tak_bertanda32_t version;
    tak_bertanda32_t port_bitmap;
    ahci_port_t port[AHCI_MAX_PORTS];
    tak_bertanda32_t jumlah_perangkat;
} ahci_konteks_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

static ahci_konteks_t g_ahci_konteks;
static bool_t g_ahci_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * FUNGSI MMIO
 * ===========================================================================
 */

/*
 * ahci_mmio_read - Baca register MMIO
 */
static inline tak_bertanda32_t ahci_mmio_read(volatile void *addr)
{
    return *((volatile tak_bertanda32_t *)addr);
}

/*
 * ahci_mmio_write - Tulis register MMIO
 */
static inline void ahci_mmio_write(volatile void *addr,
                                   tak_bertanda32_t nilai)
{
    *((volatile tak_bertanda32_t *)addr) = nilai;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * ahci_port_read - Baca register port
 */
static inline tak_bertanda32_t ahci_port_read(ahci_port_t *port,
                                              tak_bertanda32_t offset)
{
    if (port == NULL || port->reg_base == NULL) {
        return 0;
    }

    return ahci_mmio_read((volatile void *)
                          ((tak_bertanda8_t *)port->reg_base + offset));
}

/*
 * ahci_port_write - Tulis register port
 */
static inline void ahci_port_write(ahci_port_t *port,
                                   tak_bertanda32_t offset,
                                   tak_bertanda32_t nilai)
{
    if (port == NULL || port->reg_base == NULL) {
        return;
    }

    ahci_mmio_write((volatile void *)
                    ((tak_bertanda8_t *)port->reg_base + offset), nilai);
}

/*
 * ahci_port_stop - Hentikan port engine
 */
static status_t ahci_port_stop(ahci_port_t *port)
{
    tak_bertanda32_t cmd;
    tak_bertanda32_t timeout;

    if (port == NULL) {
        return STATUS_PARAM_NULL;
    }

    cmd = ahci_port_read(port, AHCI_PORT_CMD);

    /* Clear ST (Start) */
    cmd &= ~AHCI_CMD_ST;
    ahci_port_write(port, AHCI_PORT_CMD, cmd);

    /* Tunggu CR (Command Running) clear */
    timeout = 500;
    while (timeout > 0) {
        cmd = ahci_port_read(port, AHCI_PORT_CMD);
        if ((cmd & AHCI_CMD_CR) == 0) {
            break;
        }
        cpu_delay_ms(1);
        timeout--;
    }

    /* Clear FRE (FIS Receive Enable) */
    cmd = ahci_port_read(port, AHCI_PORT_CMD);
    cmd &= ~AHCI_CMD_FRE;
    ahci_port_write(port, AHCI_PORT_CMD, cmd);

    /* Tunggu FR (FIS Running) clear */
    timeout = 500;
    while (timeout > 0) {
        cmd = ahci_port_read(port, AHCI_PORT_CMD);
        if ((cmd & AHCI_CMD_FR) == 0) {
            break;
        }
        cpu_delay_ms(1);
        timeout--;
    }

    return STATUS_BERHASIL;
}

/*
 * ahci_port_start - Mulai port engine
 */
static status_t ahci_port_start(ahci_port_t *port)
{
    tak_bertanda32_t cmd;

    if (port == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Set FRE (FIS Receive Enable) */
    cmd = ahci_port_read(port, AHCI_PORT_CMD);
    cmd |= AHCI_CMD_FRE;
    ahci_port_write(port, AHCI_PORT_CMD, cmd);

    /* Set ST (Start) */
    cmd = ahci_port_read(port, AHCI_PORT_CMD);
    cmd |= AHCI_CMD_ST;
    ahci_port_write(port, AHCI_PORT_CMD, cmd);

    port->active = BENAR;

    return STATUS_BERHASIL;
}

/*
 * ahci_find_free_slot - Cari slot command kosong
 */
static tak_bertanda32_t ahci_find_free_slot(ahci_port_t *port)
{
    tak_bertanda32_t ci;
    tak_bertanda32_t i;
    tak_bertanda32_t slot_count;

    if (port == NULL) {
        return INDEX_INVALID;
    }

    ci = ahci_port_read(port, AHCI_PORT_CI);

    /* Dapatkan jumlah slot dari CAP */
    slot_count = (g_ahci_konteks.cap >> 8) & 0x1F;
    if (slot_count == 0) {
        slot_count = AHCI_MAX_SLOTS;
    }

    for (i = 0; i < slot_count; i++) {
        if ((ci & (1UL << i)) == 0) {
            return i;
        }
    }

    return INDEX_INVALID;
}

/*
 * ahci_identify - Identifikasi perangkat
 */
static status_t ahci_identify(ahci_port_t *port)
{
    ahci_device_t *dev;
    ahci_cmd_header_t *cmd_header;
    ahci_fis_h2d_t *fis;
    tak_bertanda16_t *identify_data;
    tak_bertanda32_t slot;
    tak_bertanda32_t i;
    char *p;

    if (port == NULL) {
        return STATUS_PARAM_NULL;
    }

    dev = &port->device;

    /* Cari slot kosong */
    slot = ahci_find_free_slot(port);
    if (slot == INDEX_INVALID) {
        return STATUS_BUSY;
    }

    /* Setup command header */
    cmd_header = (ahci_cmd_header_t *)port->cmd_list + slot;
    kernel_memset(cmd_header, 0, sizeof(ahci_cmd_header_t));

    cmd_header->flags = 5;  /* 5 PRDT entries */
    cmd_header->prdtl = 1;
    cmd_header->ctba = (tak_bertanda64_t)(uintptr_t)port->cmd_table;

    /* Setup FIS */
    fis = (ahci_fis_h2d_t *)port->cmd_table;
    kernel_memset(fis, 0, sizeof(ahci_fis_h2d_t));

    fis->fis_type = AHCI_FIS_REG_H2D;
    fis->command = 0xEC;  /* IDENTIFY DEVICE */
    fis->device = 0;
    fis->pm_port = 0x80;  /* Command bit */

    /* Setup PRDT */
    identify_data = (tak_bertanda16_t *)
                    ((tak_bertanda8_t *)port->cmd_table + 128);
    {
        ahci_prdt_t *prdt = (ahci_prdt_t *)
                            ((tak_bertanda8_t *)port->cmd_table + 128);
        prdt->dba = (tak_bertanda64_t)(uintptr_t)identify_data;
        prdt->dbc = 512 - 1;
    }

    /* Issue command */
    ahci_port_write(port, AHCI_PORT_CI, 1UL << slot);

    /* Tunggu completion */
    for (i = 0; i < 5000; i++) {
        tak_bertanda32_t ci = ahci_port_read(port, AHCI_PORT_CI);
        if ((ci & (1UL << slot)) == 0) {
            break;
        }
        cpu_delay_ms(1);
    }

    /* Parse identify data */
    dev->present = BENAR;
    dev->total_sectors = identify_data[60] |
                        ((tak_bertanda64_t)identify_data[61] << 16);
    dev->size_bytes = dev->total_sectors * 512;
    dev->sector_size = 512;

    /* Model */
    p = dev->model;
    for (i = 0; i < 20; i++) {
        tak_bertanda16_t w = identify_data[27 + i];
        *p++ = (char)(w >> 8);
        *p++ = (char)(w & 0xFF);
    }
    *p = '\0';

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * ahci_init - Inisialisasi driver AHCI
 */
status_t ahci_init(void)
{
    tak_bertanda32_t i;
    tak_bertanda32_t pi;
    tak_bertanda32_t dev_count;

    if (g_ahci_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }

    kernel_memset(&g_ahci_konteks, 0, sizeof(ahci_konteks_t));

    /* TODO: Dapatkan MMIO base dari PCI */
    /* Untuk sekarang, gunakan nilai default */
    /* g_ahci_konteks.mmio_base = pci_get_bar(...); */

    /* Baca capability */
    if (g_ahci_konteks.mmio_base != NULL) {
        g_ahci_konteks.cap = ahci_mmio_read(g_ahci_konteks.mmio_base +
                                           AHCI_REG_CAP);
        g_ahci_konteks.cap2 = ahci_mmio_read(g_ahci_konteks.mmio_base +
                                            AHCI_REG_CAP2);
        g_ahci_konteks.version = ahci_mmio_read(g_ahci_konteks.mmio_base +
                                               AHCI_REG_VS);
        g_ahci_konteks.port_bitmap = ahci_mmio_read(g_ahci_konteks.mmio_base +
                                                   AHCI_REG_PI);
    }

    /* Setup port */
    dev_count = 0;
    pi = g_ahci_konteks.port_bitmap;

    for (i = 0; i < AHCI_MAX_PORTS; i++) {
        ahci_port_t *port;
        tak_bertanda32_t ssts;

        if ((pi & (1UL << i)) == 0) {
            continue;
        }

        port = &g_ahci_konteks.port[i];
        port->magic = AHCI_PORT_MAGIC;
        port->port_num = i;

        /* Setup register base */
        if (g_ahci_konteks.mmio_base != NULL) {
            port->reg_base = (volatile tak_bertanda32_t *)
                            ((tak_bertanda8_t *)g_ahci_konteks.mmio_base +
                             0x100 + (i * 0x80));
        }

        /* Cek status port */
        ssts = ahci_port_read(port, AHCI_PORT_SSTS);
        if ((ssts & 0x0F) != AHCI_SSTS_DET_PHY_ON) {
            continue;
        }

        /* Setup device */
        port->device.magic = AHCI_DEVICE_MAGIC;
        port->device.id = dev_count;
        port->device.port_num = (tak_bertanda8_t)i;
        port->device.signature = ahci_port_read(port, AHCI_PORT_SIG);

        /* Alloc command list dan FIS (jika MMIO tersedia) */
        if (g_ahci_konteks.mmio_base != NULL) {
            /* TODO: Alokasi memori DMA-safe */
            /* port->cmd_list = kmalloc_dma(...); */
            /* port->fis_recv = kmalloc_dma(...); */
            /* port->cmd_table = kmalloc_dma(...); */

            ahci_port_stop(port);
            ahci_port_start(port);
            ahci_identify(port);
        }

        if (port->device.present) {
            port->device.valid = BENAR;
            dev_count++;
        }
    }

    g_ahci_konteks.magic = AHCI_MAGIC;
    g_ahci_konteks.jumlah_perangkat = dev_count;
    g_ahci_konteks.diinisialisasi = BENAR;
    g_ahci_diinisialisasi = BENAR;

    return STATUS_BERHASIL;
}

/*
 * ahci_shutdown - Matikan driver AHCI
 */
status_t ahci_shutdown(void)
{
    tak_bertanda32_t i;

    if (!g_ahci_diinisialisasi) {
        return STATUS_BERHASIL;
    }

    /* Stop semua port */
    for (i = 0; i < AHCI_MAX_PORTS; i++) {
        ahci_port_t *port = &g_ahci_konteks.port[i];

        if (port->active) {
            ahci_port_stop(port);
        }

        /* Free DMA memory */
        /* if (port->cmd_list) kfree_dma(port->cmd_list); */
    }

    g_ahci_konteks.magic = 0;
    g_ahci_konteks.diinisialisasi = SALAH;
    g_ahci_diinisialisasi = SALAH;

    return STATUS_BERHASIL;
}

/*
 * ahci_baca - Baca sektor dari perangkat AHCI
 *
 * Parameter:
 *   dev_id - ID perangkat
 *   lba - Alamat LBA
 *   buffer - Buffer output
 *   count - Jumlah sektor
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ahci_baca(tak_bertanda32_t dev_id, tak_bertanda64_t lba,
                   void *buffer, ukuran_t count)
{
    ahci_port_t *port;
    ahci_device_t *dev;
    ahci_cmd_header_t *cmd_header;
    ahci_fis_h2d_t *fis;
    ahci_prdt_t *prdt;
    tak_bertanda32_t slot;
    tak_bertanda32_t i;

    if (!g_ahci_diinisialisasi) {
        return STATUS_GAGAL;
    }

    if (buffer == NULL || count == 0) {
        return STATUS_PARAM_INVALID;
    }

    /* Cari perangkat */
    dev = NULL;
    port = NULL;
    for (i = 0; i < AHCI_MAX_PORTS; i++) {
        if (g_ahci_konteks.port[i].device.id == dev_id &&
            g_ahci_konteks.port[i].device.valid) {
            port = &g_ahci_konteks.port[i];
            dev = &port->device;
            break;
        }
    }

    if (dev == NULL || port == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Cari slot kosong */
    slot = ahci_find_free_slot(port);
    if (slot == INDEX_INVALID) {
        return STATUS_BUSY;
    }

    /* Setup command header */
    cmd_header = (ahci_cmd_header_t *)port->cmd_list + slot;
    kernel_memset(cmd_header, 0, sizeof(ahci_cmd_header_t));

    cmd_header->flags = (tak_bertanda16_t)count;
    cmd_header->prdtl = 1;
    cmd_header->ctba = (tak_bertanda64_t)(uintptr_t)port->cmd_table;

    /* Setup FIS */
    fis = (ahci_fis_h2d_t *)port->cmd_table;
    kernel_memset(fis, 0, sizeof(ahci_fis_h2d_t));

    fis->fis_type = AHCI_FIS_REG_H2D;
    fis->command = 0x24;  /* READ DMA EXT */
    fis->device = 0xE0;
    fis->lba_low = (tak_bertanda8_t)(lba & 0xFF);
    fis->lba_mid = (tak_bertanda8_t)((lba >> 8) & 0xFF);
    fis->lba_high = (tak_bertanda8_t)((lba >> 16) & 0xFF);
    fis->lba_low_exp = (tak_bertanda8_t)((lba >> 24) & 0xFF);
    fis->lba_mid_exp = (tak_bertanda8_t)((lba >> 32) & 0xFF);
    fis->lba_high_exp = (tak_bertanda8_t)((lba >> 40) & 0xFF);
    fis->sector_count = (tak_bertanda8_t)(count & 0xFF);
    fis->sector_count_exp = (tak_bertanda8_t)((count >> 8) & 0xFF);
    fis->pm_port = 0x80;

    /* Setup PRDT */
    prdt = (ahci_prdt_t *)((tak_bertanda8_t *)port->cmd_table + 128);
    prdt->dba = (tak_bertanda64_t)(uintptr_t)buffer;
    prdt->dbc = (tak_bertanda32_t)(count * 512 - 1);

    /* Issue command */
    ahci_port_write(port, AHCI_PORT_CI, 1UL << slot);

    /* Tunggu completion */
    for (i = 0; i < 5000; i++) {
        tak_bertanda32_t ci = ahci_port_read(port, AHCI_PORT_CI);
        if ((ci & (1UL << slot)) == 0) {
            break;
        }
        cpu_delay_ms(1);
    }

    dev->read_ops++;

    return STATUS_BERHASIL;
}

/*
 * ahci_tulis - Tulis sektor ke perangkat AHCI
 *
 * Parameter:
 *   dev_id - ID perangkat
 *   lba - Alamat LBA
 *   buffer - Buffer input
 *   count - Jumlah sektor
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ahci_tulis(tak_bertanda32_t dev_id, tak_bertanda64_t lba,
                    const void *buffer, ukuran_t count)
{
    ahci_port_t *port;
    ahci_device_t *dev;
    ahci_cmd_header_t *cmd_header;
    ahci_fis_h2d_t *fis;
    ahci_prdt_t *prdt;
    tak_bertanda32_t slot;
    tak_bertanda32_t i;

    if (!g_ahci_diinisialisasi) {
        return STATUS_GAGAL;
    }

    if (buffer == NULL || count == 0) {
        return STATUS_PARAM_INVALID;
    }

    /* Cari perangkat */
    dev = NULL;
    port = NULL;
    for (i = 0; i < AHCI_MAX_PORTS; i++) {
        if (g_ahci_konteks.port[i].device.id == dev_id &&
            g_ahci_konteks.port[i].device.valid) {
            port = &g_ahci_konteks.port[i];
            dev = &port->device;
            break;
        }
    }

    if (dev == NULL || port == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Cari slot kosong */
    slot = ahci_find_free_slot(port);
    if (slot == INDEX_INVALID) {
        return STATUS_BUSY;
    }

    /* Setup command header */
    cmd_header = (ahci_cmd_header_t *)port->cmd_list + slot;
    kernel_memset(cmd_header, 0, sizeof(ahci_cmd_header_t));

    cmd_header->flags = (tak_bertanda16_t)count | AHCI_CMD_FLAG_WRITE;
    cmd_header->prdtl = 1;
    cmd_header->ctba = (tak_bertanda64_t)(uintptr_t)port->cmd_table;

    /* Setup FIS */
    fis = (ahci_fis_h2d_t *)port->cmd_table;
    kernel_memset(fis, 0, sizeof(ahci_fis_h2d_t));

    fis->fis_type = AHCI_FIS_REG_H2D;
    fis->command = 0x34;  /* WRITE DMA EXT */
    fis->device = 0xE0;
    fis->lba_low = (tak_bertanda8_t)(lba & 0xFF);
    fis->lba_mid = (tak_bertanda8_t)((lba >> 8) & 0xFF);
    fis->lba_high = (tak_bertanda8_t)((lba >> 16) & 0xFF);
    fis->lba_low_exp = (tak_bertanda8_t)((lba >> 24) & 0xFF);
    fis->lba_mid_exp = (tak_bertanda8_t)((lba >> 32) & 0xFF);
    fis->lba_high_exp = (tak_bertanda8_t)((lba >> 40) & 0xFF);
    fis->sector_count = (tak_bertanda8_t)(count & 0xFF);
    fis->sector_count_exp = (tak_bertanda8_t)((count >> 8) & 0xFF);
    fis->pm_port = 0x80;

    /* Setup PRDT */
    prdt = (ahci_prdt_t *)((tak_bertanda8_t *)port->cmd_table + 128);
    prdt->dba = (tak_bertanda64_t)(uintptr_t)buffer;
    prdt->dbc = (tak_bertanda32_t)(count * 512 - 1);

    /* Issue command */
    ahci_port_write(port, AHCI_PORT_CI, 1UL << slot);

    /* Tunggu completion */
    for (i = 0; i < 5000; i++) {
        tak_bertanda32_t ci = ahci_port_read(port, AHCI_PORT_CI);
        if ((ci & (1UL << slot)) == 0) {
            break;
        }
        cpu_delay_ms(1);
    }

    dev->write_ops++;

    return STATUS_BERHASIL;
}

/*
 * ahci_cetak_info - Cetak informasi perangkat AHCI
 */
void ahci_cetak_info(void)
{
    tak_bertanda32_t i;

    if (!g_ahci_diinisialisasi) {
        return;
    }

    kernel_printf("\n=== Perangkat AHCI/SATA ===\n\n");

    kernel_printf("AHCI Versi: %u.%u\n",
                 (g_ahci_konteks.version >> 16) & 0xFF,
                 (g_ahci_konteks.version >> 8) & 0xFF);

    kernel_printf("Port Aktif: 0x%08X\n", g_ahci_konteks.port_bitmap);
    kernel_printf("Jumlah Perangkat: %u\n\n",
                 g_ahci_konteks.jumlah_perangkat);

    for (i = 0; i < AHCI_MAX_PORTS; i++) {
        ahci_port_t *port = &g_ahci_konteks.port[i];
        ahci_device_t *dev = &port->device;

        if (dev->valid && dev->present) {
            kernel_printf("ahci%u (port %u): ", dev->id, i);
            kernel_printf("%s\n", dev->model);
            kernel_printf("  Kapasitas: %lu MB\n",
                         dev->size_bytes / (1024 * 1024));
            kernel_printf("  Sektor: %lu\n", dev->total_sectors);
            kernel_printf("\n");
        }
    }
}
