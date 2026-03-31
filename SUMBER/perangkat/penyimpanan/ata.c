/*
 * PIGURA OS - ATA.C
 * ==================
 * Implementasi driver ATA/IDE untuk perangkat penyimpanan.
 *
 * Berkas ini menyediakan fungsi-fungsi untuk mengakses perangkat
 * penyimpanan ATA melalui interface IDE (PATA).
 *
 * Kepatuhan standar:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Mendukung semua arsitektur: x86, x86_64, ARM, ARMv7, ARM64
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../cpu/cpu.h"
#include "../../inti/hal/hal.h"
#include "../../inti/kernel.h"

/*
 * ===========================================================================
 * KONSTANTA ATA
 * ===========================================================================
 */

#define ATA_VERSI_MAJOR 1
#define ATA_VERSI_MINOR 0

/* Magic number */
#define ATA_MAGIC            0x41544144  /* "ATAD" */
#define ATA_DEVICE_MAGIC     0x41544144  /* "ATAD" */

/* Port I/O IDE Primer */
#define ATA_PRIMARY_BASE     0x1F0
#define ATA_PRIMARY_CTRL     0x3F6

/* Port I/O IDE Sekunder */
#define ATA_SECONDARY_BASE   0x170
#define ATA_SECONDARY_CTRL   0x376

/* Register offset */
#define ATA_REG_DATA         0x00
#define ATA_REG_ERROR        0x01
#define ATA_REG_FEATURES     0x01
#define ATA_REG_SECTOR_COUNT 0x02
#define ATA_REG_LBA_LOW      0x03
#define ATA_REG_LBA_MID      0x04
#define ATA_REG_LBA_HIGH     0x05
#define ATA_REG_DRIVE        0x06
#define ATA_REG_STATUS       0x07
#define ATA_REG_COMMAND      0x07

/* Register kontrol */
#define ATA_CTRL_STATUS      0x00
#define ATA_CTRL_ALTSTATUS   0x00
#define ATA_CTRL_DEVICE_CTRL 0x00

/* Perintah ATA */
#define ATA_CMD_READ_SECTORS      0x20
#define ATA_CMD_READ_SECTORS_EXT  0x24
#define ATA_CMD_WRITE_SECTORS     0x30
#define ATA_CMD_WRITE_SECTORS_EXT 0x34
#define ATA_CMD_IDENTIFY          0xEC
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_FLUSH_CACHE       0xE7
#define ATA_CMD_FLUSH_CACHE_EXT   0xEA
#define ATA_CMD_PACKET            0xA0

/* Status bit */
#define ATA_STATUS_ERR      0x01
#define ATA_STATUS_DRQ      0x08
#define ATA_STATUS_SRV      0x10
#define ATA_STATUS_DF       0x20
#define ATA_STATUS_RDY      0x40
#define ATA_STATUS_BSY      0x80

/* Drive bit */
#define ATA_DRIVE_LBA       0x40
#define ATA_DRIVE_SLAVE     0x10

/* Tipe perangkat */
#define ATA_DEVICE_UNKNOWN   0
#define ATA_DEVICE_ATA       1
#define ATA_DEVICE_ATAPI     2
#define ATA_DEVICE_SATA      3

/* Jumlah maksimum perangkat */
#define ATA_MAX_DEVICES      4
#define ATA_MAX_CHANNELS     2

/* Ukuran sektor */
#define ATA_SECTOR_SIZE      512

/*
 * ===========================================================================
 * STRUKTUR DATA ATA
 * ===========================================================================
 */

/* Informasi identifikasi perangkat */
typedef struct {
    tak_bertanda16_t config;              /* Word 0 */
    tak_bertanda16_t cylinders;           /* Word 1 */
    tak_bertanda16_t heads;               /* Word 3 */
    tak_bertanda16_t sectors_per_track;   /* Word 6 */
    tak_bertanda16_t serial[10];          /* Word 10-19 */
    tak_bertanda16_t firmware_rev[4];     /* Word 23-26 */
    tak_bertanda16_t model[20];           /* Word 27-46 */
    tak_bertanda16_t capabilities;        /* Word 49 */
    tak_bertanda16_t valid_ext_data;      /* Word 53 */
    tak_bertanda16_t current_cylinders;   /* Word 54 */
    tak_bertanda16_t current_heads;       /* Word 55 */
    tak_bertanda16_t current_sectors;     /* Word 56 */
    tak_bertanda16_t current_capacity[2]; /* Word 57-58 */
    tak_bertanda16_t multi_word_dma;      /* Word 63 */
    tak_bertanda16_t pio_modes;           /* Word 64 */
    tak_bertanda16_t min_multi_word;      /* Word 65 */
    tak_bertanda16_t rec_multi_word;      /* Word 66 */
    tak_bertanda16_t min_pio_cycle;       /* Word 67 */
    tak_bertanda16_t min_pio_cycle_iordy; /* Word 68 */
    tak_bertanda16_t reserved[10];        /* Word 69-78 */
    tak_bertanda16_t major_version;       /* Word 80 */
    tak_bertanda16_t minor_version;       /* Word 81 */
    tak_bertanda16_t command_set[3];      /* Word 82-84 */
    tak_bertanda16_t command_set_enabled[3]; /* Word 85-87 */
    tak_bertanda16_t ultra_dma;           /* Word 88 */
    tak_bertanda16_t sectors_28[2];       /* Word 60-61 */
    tak_bertanda16_t reserved2[12];       /* Word 62-79 */
    tak_bertanda16_t sectors_48[4];       /* Word 100-103 */
} ata_identify_t;

/* Perangkat ATA */
typedef struct {
    tak_bertanda32_t magic;
    tak_bertanda32_t id;
    bool_t valid;
    bool_t present;
    tak_bertanda8_t tipe;
    tak_bertanda8_t channel;
    tak_bertanda8_t drive;

    /* Alamat I/O */
    tak_bertanda16_t io_base;
    tak_bertanda16_t ctrl_base;

    /* Kapasitas */
    tak_bertanda64_t total_sectors;
    tak_bertanda64_t size_bytes;
    ukuran_t sector_size;

    /* Identifikasi */
    char model[41];
    char serial[21];
    char firmware[9];

    /* Fitur */
    bool_t lba48_supported;
    bool_t lba_supported;
    bool_t dma_supported;

    /* Statistik */
    tak_bertanda64_t read_ops;
    tak_bertanda64_t write_ops;
    tak_bertanda64_t errors;
} ata_device_t;

/* Channel ATA */
typedef struct {
    tak_bertanda32_t magic;
    tak_bertanda16_t io_base;
    tak_bertanda16_t ctrl_base;
    tak_bertanda8_t irq;
    ata_device_t devices[2];
} ata_channel_t;

/* Konteks ATA */
typedef struct {
    tak_bertanda32_t magic;
    bool_t diinisialisasi;
    ata_channel_t channel[ATA_MAX_CHANNELS];
    tak_bertanda32_t jumlah_perangkat;
} ata_konteks_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

static ata_konteks_t g_ata_konteks;
static bool_t g_ata_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * FUNGSI I/O LOW-LEVEL (Multi-Arsitektur)
 * ===========================================================================
 * Menggunakan HAL untuk portabilitas antar arsitektur.
 * Pada x86/x86_64 menggunakan port I/O.
 * Pada ARM menggunakan memory-mapped I/O.
 */

/*
 * ata_inb - Baca byte dari port I/O
 */
static inline tak_bertanda8_t ata_inb(tak_bertanda16_t port)
{
    return hal_io_read_8(port);
}

/*
 * ata_outb - Tulis byte ke port I/O
 */
static inline void ata_outb(tak_bertanda16_t port,
                            tak_bertanda8_t nilai)
{
    hal_io_write_8(port, nilai);
}

/*
 * ata_insw - Baca word array dari port I/O
 */
static inline void ata_insw(tak_bertanda16_t port, void *buffer,
                            ukuran_t count)
{
    hal_io_read_string_8(port, buffer, count * 2);
}

/*
 * ata_outsw - Tulis word array ke port I/O
 */
static inline void ata_outsw(tak_bertanda16_t port, const void *buffer,
                             ukuran_t count)
{
    hal_io_write_string_8(port, buffer, count * 2);
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * ata_delay - Delay singkat (400ns)
 */
static void ata_delay(tak_bertanda16_t ctrl_base)
{
    tak_bertanda8_t dummy;
    (void)dummy;

    /* Baca alternate status 4 kali (~400ns) */
    dummy = ata_inb(ctrl_base + ATA_CTRL_ALTSTATUS);
    dummy = ata_inb(ctrl_base + ATA_CTRL_ALTSTATUS);
    dummy = ata_inb(ctrl_base + ATA_CTRL_ALTSTATUS);
    dummy = ata_inb(ctrl_base + ATA_CTRL_ALTSTATUS);
}

/*
 * ata_pilih_drive - Pilih drive target
 */
static void ata_pilih_drive(tak_bertanda16_t io_base,
                            tak_bertanda8_t drive,
                            tak_bertanda8_t lba_high)
{
    tak_bertanda8_t nilai;

    nilai = ATA_DRIVE_LBA | (drive << 4) | (lba_high & 0x0F);
    ata_outb(io_base + ATA_REG_DRIVE, nilai);
    ata_delay(io_base + 0x206);
}

/*
 * ata_tunggu_rdy - Tunggu hingga drive ready
 */
static status_t ata_tunggu_rdy(tak_bertanda16_t io_base,
                               tak_bertanda32_t timeout_ms)
{
    tak_bertanda8_t status;
    tak_bertanda32_t i;

    for (i = 0; i < timeout_ms * 100; i++) {
        status = ata_inb(io_base + ATA_REG_STATUS);

        if ((status & ATA_STATUS_BSY) == 0) {
            if (status & ATA_STATUS_RDY) {
                return STATUS_BERHASIL;
            }
        }

        hal_cpu_delay_us(10);
    }

    return STATUS_TIMEOUT;
}

/*
 * ata_tunggu_drq - Tunggu hingga data ready
 */
static status_t ata_tunggu_drq(tak_bertanda16_t io_base,
                               tak_bertanda32_t timeout_ms)
{
    tak_bertanda8_t status;
    tak_bertanda32_t i;

    for (i = 0; i < timeout_ms * 100; i++) {
        status = ata_inb(io_base + ATA_REG_STATUS);

        if (status & ATA_STATUS_ERR) {
            return STATUS_IO_ERROR;
        }

        if ((status & ATA_STATUS_BSY) == 0) {
            if (status & ATA_STATUS_DRQ) {
                return STATUS_BERHASIL;
            }
        }

        hal_cpu_delay_us(10);
    }

    return STATUS_TIMEOUT;
}

/*
 * ata_identify - Identifikasi perangkat
 */
static status_t ata_identify(ata_device_t *dev)
{
    tak_bertanda16_t buffer[256];
    ata_identify_t *identify;
    status_t hasil;
    tak_bertanda8_t status;
    tak_bertanda32_t i;
    char *p;

    if (dev == NULL) {
        return STATUS_PARAM_NULL;
    }

    identify = (ata_identify_t *)buffer;

    /* Pilih drive */
    ata_pilih_drive(dev->io_base, dev->drive, 0);
    ata_delay(dev->ctrl_base);

    /* Kirim perintah IDENTIFY */
    ata_outb(dev->io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    /* Tunggu response */
    status = ata_inb(dev->io_base + ATA_REG_STATUS);
    if (status == 0) {
        dev->present = SALAH;
        return STATUS_TIDAK_DITEMUKAN;
    }

    hasil = ata_tunggu_drq(dev->io_base, 1000);
    if (hasil != STATUS_BERHASIL) {
        /* Mungkin ATAPI, coba IDENTIFY PACKET */
        ata_outb(dev->io_base + ATA_REG_COMMAND,
                 ATA_CMD_IDENTIFY_PACKET);
        hasil = ata_tunggu_drq(dev->io_base, 1000);
        if (hasil == STATUS_BERHASIL) {
            dev->tipe = ATA_DEVICE_ATAPI;
        } else {
            dev->present = SALAH;
            return STATUS_TIDAK_DITEMUKAN;
        }
    } else {
        dev->tipe = ATA_DEVICE_ATA;
    }

    /* Baca data identify */
    ata_insw(dev->io_base + ATA_REG_DATA, buffer, 256);

    /* Parse informasi */
    dev->present = BENAR;
    dev->lba_supported = (identify->capabilities & 0x0200) ? BENAR : SALAH;

    /* Cek LBA48 */
    if (identify->command_set[1] & 0x0400) {
        dev->lba48_supported = BENAR;
    }

    /* Kapasitas */
    if (dev->lba48_supported) {
        dev->total_sectors = ((tak_bertanda64_t)
                              identify->sectors_48[3] << 48) |
                             ((tak_bertanda64_t)
                              identify->sectors_48[2] << 32) |
                             ((tak_bertanda64_t)
                              identify->sectors_48[1] << 16) |
                             (tak_bertanda64_t)identify->sectors_48[0];
    } else {
        dev->total_sectors = ((tak_bertanda32_t)
                              identify->sectors_28[1] << 16) |
                             (tak_bertanda32_t)identify->sectors_28[0];
    }

    dev->size_bytes = dev->total_sectors * ATA_SECTOR_SIZE;
    dev->sector_size = ATA_SECTOR_SIZE;

    /* Model */
    p = dev->model;
    for (i = 0; i < 20; i++) {
        tak_bertanda16_t w = identify->model[i];
        *p++ = (char)(w >> 8);
        *p++ = (char)(w & 0xFF);
    }
    *p = '\0';

    /* Serial */
    p = dev->serial;
    for (i = 0; i < 10; i++) {
        tak_bertanda16_t w = identify->serial[i];
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
 * ata_init - Inisialisasi driver ATA
 */
status_t ata_init(void)
{
    status_t hasil;
    tak_bertanda32_t i;
    tak_bertanda32_t j;
    tak_bertanda32_t dev_id;

    if (g_ata_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }

    kernel_memset(&g_ata_konteks, 0, sizeof(ata_konteks_t));

    /* Setup channel primer */
    g_ata_konteks.channel[0].magic = ATA_MAGIC;
    g_ata_konteks.channel[0].io_base = ATA_PRIMARY_BASE;
    g_ata_konteks.channel[0].ctrl_base = ATA_PRIMARY_CTRL;
    g_ata_konteks.channel[0].irq = 14;

    /* Setup channel sekunder */
    g_ata_konteks.channel[1].magic = ATA_MAGIC;
    g_ata_konteks.channel[1].io_base = ATA_SECONDARY_BASE;
    g_ata_konteks.channel[1].ctrl_base = ATA_SECONDARY_CTRL;
    g_ata_konteks.channel[1].irq = 15;

    dev_id = 0;

    /* Identifikasi semua perangkat */
    for (i = 0; i < ATA_MAX_CHANNELS; i++) {
        ata_channel_t *ch = &g_ata_konteks.channel[i];

        for (j = 0; j < 2; j++) {
            ata_device_t *dev = &ch->devices[j];

            dev->magic = ATA_DEVICE_MAGIC;
            dev->id = dev_id++;
            dev->channel = (tak_bertanda8_t)i;
            dev->drive = (tak_bertanda8_t)j;
            dev->io_base = ch->io_base;
            dev->ctrl_base = ch->ctrl_base;

            hasil = ata_identify(dev);
            if (hasil == STATUS_BERHASIL && dev->present) {
                dev->valid = BENAR;
                g_ata_konteks.jumlah_perangkat++;
            }
        }
    }

    g_ata_konteks.magic = ATA_MAGIC;
    g_ata_konteks.diinisialisasi = BENAR;
    g_ata_diinisialisasi = BENAR;

    return STATUS_BERHASIL;
}

/*
 * ata_shutdown - Matikan driver ATA
 */
status_t ata_shutdown(void)
{
    if (!g_ata_diinisialisasi) {
        return STATUS_BERHASIL;
    }

    g_ata_konteks.magic = 0;
    g_ata_konteks.diinisialisasi = SALAH;
    g_ata_diinisialisasi = SALAH;

    return STATUS_BERHASIL;
}

/*
 * ata_baca - Baca sektor dari perangkat ATA
 *
 * Parameter:
 *   dev_id - ID perangkat
 *   lba - Alamat LBA
 *   buffer - Buffer output
 *   count - Jumlah sektor
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ata_baca(tak_bertanda32_t dev_id, tak_bertanda64_t lba,
                  void *buffer, ukuran_t count)
{
    ata_device_t *dev;
    ukuran_t i;

    if (!g_ata_diinisialisasi) {
        return STATUS_GAGAL;
    }

    if (buffer == NULL || count == 0) {
        return STATUS_PARAM_INVALID;
    }

    /* Cari perangkat */
    dev = NULL;
    for (i = 0; i < ATA_MAX_CHANNELS; i++) {
        if (g_ata_konteks.channel[i].devices[0].id == dev_id) {
            dev = &g_ata_konteks.channel[i].devices[0];
            break;
        }
        if (g_ata_konteks.channel[i].devices[1].id == dev_id) {
            dev = &g_ata_konteks.channel[i].devices[1];
            break;
        }
    }

    if (dev == NULL || !dev->valid) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Gunakan LBA48 jika didukung dan LBA > 28-bit */
    if (dev->lba48_supported && lba > 0x0FFFFFFF) {
        /* LBA48 read */
        ata_pilih_drive(dev->io_base, dev->drive,
                       (tak_bertanda8_t)(lba >> 24));

        ata_outb(dev->io_base + ATA_REG_SECTOR_COUNT,
                (tak_bertanda8_t)(count >> 8));
        ata_outb(dev->io_base + ATA_REG_LBA_LOW,
                (tak_bertanda8_t)(lba >> 24));
        ata_outb(dev->io_base + ATA_REG_LBA_MID,
                (tak_bertanda8_t)(lba >> 32));
        ata_outb(dev->io_base + ATA_REG_LBA_HIGH,
                (tak_bertanda8_t)(lba >> 40));

        ata_outb(dev->io_base + ATA_REG_SECTOR_COUNT,
                (tak_bertanda8_t)count);
        ata_outb(dev->io_base + ATA_REG_LBA_LOW,
                (tak_bertanda8_t)lba);
        ata_outb(dev->io_base + ATA_REG_LBA_MID,
                (tak_bertanda8_t)(lba >> 8));
        ata_outb(dev->io_base + ATA_REG_LBA_HIGH,
                (tak_bertanda8_t)(lba >> 16));

        ata_outb(dev->io_base + ATA_REG_COMMAND,
                ATA_CMD_READ_SECTORS_EXT);
    } else {
        /* LBA28 read */
        ata_pilih_drive(dev->io_base, dev->drive,
                       (tak_bertanda8_t)((lba >> 24) & 0x0F));

        ata_outb(dev->io_base + ATA_REG_SECTOR_COUNT,
                (tak_bertanda8_t)count);
        ata_outb(dev->io_base + ATA_REG_LBA_LOW,
                (tak_bertanda8_t)lba);
        ata_outb(dev->io_base + ATA_REG_LBA_MID,
                (tak_bertanda8_t)(lba >> 8));
        ata_outb(dev->io_base + ATA_REG_LBA_HIGH,
                (tak_bertanda8_t)(lba >> 16));

        ata_outb(dev->io_base + ATA_REG_COMMAND,
                ATA_CMD_READ_SECTORS);
    }

    /* Baca data */
    for (i = 0; i < count; i++) {
        status_t hasil = ata_tunggu_drq(dev->io_base, 5000);
        if (hasil != STATUS_BERHASIL) {
            dev->errors++;
            return hasil;
        }

        ata_insw(dev->io_base + ATA_REG_DATA,
                (tak_bertanda16_t *)buffer + (i * 256), 256);
    }

    dev->read_ops++;

    return STATUS_BERHASIL;
}

/*
 * ata_tulis - Tulis sektor ke perangkat ATA
 *
 * Parameter:
 *   dev_id - ID perangkat
 *   lba - Alamat LBA
 *   buffer - Buffer input
 *   count - Jumlah sektor
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ata_tulis(tak_bertanda32_t dev_id, tak_bertanda64_t lba,
                   const void *buffer, ukuran_t count)
{
    ata_device_t *dev;
    ukuran_t i;

    if (!g_ata_diinisialisasi) {
        return STATUS_GAGAL;
    }

    if (buffer == NULL || count == 0) {
        return STATUS_PARAM_INVALID;
    }

    /* Cari perangkat */
    dev = NULL;
    for (i = 0; i < ATA_MAX_CHANNELS; i++) {
        if (g_ata_konteks.channel[i].devices[0].id == dev_id) {
            dev = &g_ata_konteks.channel[i].devices[0];
            break;
        }
        if (g_ata_konteks.channel[i].devices[1].id == dev_id) {
            dev = &g_ata_konteks.channel[i].devices[1];
            break;
        }
    }

    if (dev == NULL || !dev->valid) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Gunakan LBA48 jika didukung */
    if (dev->lba48_supported && lba > 0x0FFFFFFF) {
        /* LBA48 write */
        ata_pilih_drive(dev->io_base, dev->drive,
                       (tak_bertanda8_t)(lba >> 24));

        ata_outb(dev->io_base + ATA_REG_SECTOR_COUNT,
                (tak_bertanda8_t)(count >> 8));
        ata_outb(dev->io_base + ATA_REG_LBA_LOW,
                (tak_bertanda8_t)(lba >> 24));
        ata_outb(dev->io_base + ATA_REG_LBA_MID,
                (tak_bertanda8_t)(lba >> 32));
        ata_outb(dev->io_base + ATA_REG_LBA_HIGH,
                (tak_bertanda8_t)(lba >> 40));

        ata_outb(dev->io_base + ATA_REG_SECTOR_COUNT,
                (tak_bertanda8_t)count);
        ata_outb(dev->io_base + ATA_REG_LBA_LOW,
                (tak_bertanda8_t)lba);
        ata_outb(dev->io_base + ATA_REG_LBA_MID,
                (tak_bertanda8_t)(lba >> 8));
        ata_outb(dev->io_base + ATA_REG_LBA_HIGH,
                (tak_bertanda8_t)(lba >> 16));

        ata_outb(dev->io_base + ATA_REG_COMMAND,
                ATA_CMD_WRITE_SECTORS_EXT);
    } else {
        /* LBA28 write */
        ata_pilih_drive(dev->io_base, dev->drive,
                       (tak_bertanda8_t)((lba >> 24) & 0x0F));

        ata_outb(dev->io_base + ATA_REG_SECTOR_COUNT,
                (tak_bertanda8_t)count);
        ata_outb(dev->io_base + ATA_REG_LBA_LOW,
                (tak_bertanda8_t)lba);
        ata_outb(dev->io_base + ATA_REG_LBA_MID,
                (tak_bertanda8_t)(lba >> 8));
        ata_outb(dev->io_base + ATA_REG_LBA_HIGH,
                (tak_bertanda8_t)(lba >> 16));

        ata_outb(dev->io_base + ATA_REG_COMMAND,
                ATA_CMD_WRITE_SECTORS);
    }

    /* Tulis data */
    for (i = 0; i < count; i++) {
        status_t hasil = ata_tunggu_drq(dev->io_base, 5000);
        if (hasil != STATUS_BERHASIL) {
            dev->errors++;
            return hasil;
        }

        ata_outsw(dev->io_base + ATA_REG_DATA,
                 (const tak_bertanda16_t *)buffer + (i * 256), 256);
    }

    /* Flush cache */
    if (dev->lba48_supported) {
        ata_outb(dev->io_base + ATA_REG_COMMAND,
                ATA_CMD_FLUSH_CACHE_EXT);
    } else {
        ata_outb(dev->io_base + ATA_REG_COMMAND,
                ATA_CMD_FLUSH_CACHE);
    }

    ata_tunggu_rdy(dev->io_base, 5000);

    dev->write_ops++;

    return STATUS_BERHASIL;
}

/*
 * ata_cetak_info - Cetak informasi perangkat ATA
 */
void ata_cetak_info(void)
{
    tak_bertanda32_t i;
    tak_bertanda32_t j;

    if (!g_ata_diinisialisasi) {
        return;
    }

    kernel_printf("\n=== Perangkat ATA/IDE ===\n\n");

    for (i = 0; i < ATA_MAX_CHANNELS; i++) {
        ata_channel_t *ch = &g_ata_konteks.channel[i];

        for (j = 0; j < 2; j++) {
            ata_device_t *dev = &ch->devices[j];

            if (dev->valid && dev->present) {
                kernel_printf("ata%d: ", dev->id);
                kernel_printf("%s\n", dev->model);
                kernel_printf("  Tipe: %s\n",
                             dev->tipe == ATA_DEVICE_ATA ? "ATA" :
                             "ATAPI");
                kernel_printf("  Kapasitas: %lu MB\n",
                             dev->size_bytes / (1024 * 1024));
                kernel_printf("  Sektor: %lu\n", dev->total_sectors);
                kernel_printf("  LBA48: %s\n",
                             dev->lba48_supported ? "Ya" : "Tidak");
                kernel_printf("\n");
            }
        }
    }
}
