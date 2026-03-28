/*
 * PIGURA OS - PENYIMPANAN.C
 * ==========================
 * Implementasi subsistem penyimpanan.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../cpu/cpu.h"
#include "../../inti/kernel.h"

/*
 * ===========================================================================
 * KONSTANTA PENYIMPANAN
 * ===========================================================================
 */

#define PENYIMPANAN_VERSI_MAJOR 1
#define PENYIMPANAN_VERSI_MINOR 0

#define PENYIMPANAN_MAGIC       0x50455354  /* "PEST" */
#define STORAGE_DEV_MAGIC       0x53544456  /* "STDV" */

#define STORAGE_DEV_MAKS        32
#define PARTISI_MAKS            16
#define SECTOR_SIZE             512

/* Tipe storage */
#define STORAGE_TIPE_TIDAK_ADA  0
#define STORAGE_TIPE_IDE        1
#define STORAGE_TIPE_SATA       2
#define STORAGE_TIPE_NVME       3
#define STORAGE_TIPE_SCSI       4
#define STORAGE_TIPE_USB        5
#define STORAGE_TIPE_SD         6
#define STORAGE_TIPE_MMC        7

/* Status storage */
#define STORAGE_STATUS_TIDAK_ADA  0
#define STORAGE_STATUS_TERDETEKSI 1
#define STORAGE_STATUS_SIAP       2
#define STORAGE_STATUS_ERROR      3

/*
 * ===========================================================================
 * STRUKTUR DATA
 * ===========================================================================
 */

typedef struct partisi partisi_t;
typedef struct storage_dev storage_dev_t;

/* Partisi entry */
struct partisi {
    tak_bertanda8_t status;
    tak_bertanda8_t type;
    tak_bertanda64_t start_lba;
    tak_bertanda64_t total_sectors;
    tak_bertanda32_t flags;
    storage_dev_t *parent;
    bool_t valid;
};

/* Storage device */
struct storage_dev {
    tak_bertanda32_t magic;
    tak_bertanda32_t id;
    char nama[32];
    tak_bertanda32_t tipe;
    tak_bertanda32_t status;
    
    /* Kapasitas */
    tak_bertanda64_t total_sectors;
    tak_bertanda64_t size_bytes;
    ukuran_t sector_size;
    
    /* Partisi */
    partisi_t partisi[PARTISI_MAKS];
    tak_bertanda32_t jumlah_partisi;
    
    /* Statistik */
    tak_bertanda64_t read_ops;
    tak_bertanda64_t write_ops;
    tak_bertanda64_t read_bytes;
    tak_bertanda64_t write_bytes;
    tak_bertanda64_t errors;
    
    /* Callbacks */
    status_t (*baca)(storage_dev_t *dev, tak_bertanda64_t lba,
                     void *buffer, ukuran_t count);
    status_t (*tulis)(storage_dev_t *dev, tak_bertanda64_t lba,
                      const void *buffer, ukuran_t count);
    status_t (*ioctl)(storage_dev_t *dev, tak_bertanda32_t cmd, void *arg);
    
    void *priv;
    storage_dev_t *berikutnya;
};

/* Konteks penyimpanan */
typedef struct {
    tak_bertanda32_t magic;
    storage_dev_t *device_list;
    tak_bertanda32_t device_count;
    bool_t diinisialisasi;
} penyimpanan_konteks_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

static penyimpanan_konteks_t g_penyimpanan_konteks;
static bool_t g_penyimpanan_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

status_t penyimpanan_init(void)
{
    status_t hasil;
    
    if (g_penyimpanan_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }
    
    kernel_memset(&g_penyimpanan_konteks, 0, sizeof(penyimpanan_konteks_t));
    
    /* Inisialisasi subsistem */
    hasil = ata_init();
    if (hasil != STATUS_BERHASIL) {
        /* Continue anyway */
    }
    
    hasil = ahci_init();
    if (hasil != STATUS_BERHASIL) {
        /* Continue anyway */
    }
    
    hasil = nvme_init();
    if (hasil != STATUS_BERHASIL) {
        /* Continue anyway */
    }
    
    hasil = sd_card_init();
    if (hasil != STATUS_BERHASIL) {
        /* Continue anyway */
    }
    
    g_penyimpanan_konteks.magic = PENYIMPANAN_MAGIC;
    g_penyimpanan_diinisialisasi = BENAR;
    
    return STATUS_BERHASIL;
}

status_t penyimpanan_shutdown(void)
{
    storage_dev_t *dev;
    storage_dev_t *next;
    
    if (!g_penyimpanan_diinisialisasi) {
        return STATUS_BERHASIL;
    }
    
    dev = g_penyimpanan_konteks.device_list;
    while (dev != NULL) {
        next = dev->berikutnya;
        dev->magic = 0;
        kfree(dev);
        dev = next;
    }
    
    g_penyimpanan_konteks.magic = 0;
    g_penyimpanan_diinisialisasi = SALAH;
    
    return STATUS_BERHASIL;
}

static storage_dev_t *storage_register(const char *nama,
                                        tak_bertanda32_t tipe)
{
    storage_dev_t *dev;
    
    if (nama == NULL) {
        return NULL;
    }
    
    dev = (storage_dev_t *)kmalloc(sizeof(storage_dev_t));
    if (dev == NULL) {
        return NULL;
    }
    
    kernel_memset(dev, 0, sizeof(storage_dev_t));
    
    dev->magic = STORAGE_DEV_MAGIC;
    dev->id = g_penyimpanan_konteks.device_count;
    dev->tipe = tipe;
    dev->status = STORAGE_STATUS_TERDETEKSI;
    dev->sector_size = SECTOR_SIZE;
    
    /* Salin nama */
    {
        ukuran_t i = 0;
        while (nama[i] != '\0' && i < 31) {
            dev->nama[i] = nama[i];
            i++;
        }
        dev->nama[i] = '\0';
    }
    
    /* Tambah ke list */
    if (g_penyimpanan_konteks.device_list == NULL) {
        g_penyimpanan_konteks.device_list = dev;
    } else {
        storage_dev_t *last = g_penyimpanan_konteks.device_list;
        while (last->berikutnya != NULL) {
            last = last->berikutnya;
        }
        last->berikutnya = dev;
    }
    
    g_penyimpanan_konteks.device_count++;
    
    return dev;
}

status_t storage_baca(tak_bertanda32_t dev_id, tak_bertanda64_t lba,
                       void *buffer, ukuran_t count)
{
    storage_dev_t *dev;
    status_t hasil;
    
    if (!g_penyimpanan_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    if (buffer == NULL || count == 0) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Cari device */
    dev = g_penyimpanan_konteks.device_list;
    while (dev != NULL) {
        if (dev->id == dev_id) {
            break;
        }
        dev = dev->berikutnya;
    }
    
    if (dev == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    if (dev->baca == NULL) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    hasil = dev->baca(dev, lba, buffer, count);
    
    if (hasil == STATUS_BERHASIL) {
        dev->read_ops++;
        dev->read_bytes += count * dev->sector_size;
    } else {
        dev->errors++;
    }
    
    return hasil;
}

status_t storage_tulis(tak_bertanda32_t dev_id, tak_bertanda64_t lba,
                        const void *buffer, ukuran_t count)
{
    storage_dev_t *dev;
    status_t hasil;
    
    if (!g_penyimpanan_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    if (buffer == NULL || count == 0) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Cari device */
    dev = g_penyimpanan_konteks.device_list;
    while (dev != NULL) {
        if (dev->id == dev_id) {
            break;
        }
        dev = dev->berikutnya;
    }
    
    if (dev == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    if (dev->tulis == NULL) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    hasil = dev->tulis(dev, lba, buffer, count);
    
    if (hasil == STATUS_BERHASIL) {
        dev->write_ops++;
        dev->write_bytes += count * dev->sector_size;
    } else {
        dev->errors++;
    }
    
    return hasil;
}

void penyimpanan_cetak_info(void)
{
    storage_dev_t *dev;
    char buffer[32];
    ukuran_t i;
    
    if (!g_penyimpanan_diinisialisasi) {
        return;
    }
    
    kernel_printf("\n=== Storage Devices ===\n\n");
    
    dev = g_penyimpanan_konteks.device_list;
    
    while (dev != NULL) {
        /* Nama dan tipe */
        kernel_printf("%s: ", dev->nama);
        
        /* Kapasitas */
        {
            ukuran_t gb = (ukuran_t)(dev->size_bytes / (1024 * 1024 * 1024));
            ukuran_t mb = (ukuran_t)((dev->size_bytes / (1024 * 1024)) % 1024);
            
            if (gb > 0) {
                kernel_printf("%lu GB", gb);
            } else {
                kernel_printf("%lu MB", mb);
            }
        }
        
        kernel_printf(" (%lu sectors)\n", dev->total_sectors);
        
        /* Partisi */
        if (dev->jumlah_partisi > 0) {
            kernel_printf("  Partitions: %u\n", dev->jumlah_partisi);
            
            for (i = 0; i < dev->jumlah_partisi; i++) {
                if (dev->partisi[i].valid) {
                    kernel_printf("    p%u: LBA %lu, %lu sectors\n",
                                 i + 1, dev->partisi[i].start_lba,
                                 dev->partisi[i].total_sectors);
                }
            }
        }
        
        dev = dev->berikutnya;
    }
    
    kernel_printf("\n");
}

/*
 * ===========================================================================
 * STUB IMPLEMENTATIONS
 * ===========================================================================
 */

status_t ata_init(void) { return STATUS_BERHASIL; }
status_t ahci_init(void) { return STATUS_BERHASIL; }
status_t nvme_init(void) { return STATUS_BERHASIL; }
status_t sd_card_init(void) { return STATUS_BERHASIL; }
status_t mmc_init(void) { return STATUS_BERHASIL; }
status_t partisi_init(void) { return STATUS_BERHASIL; }
status_t mbr_init(void) { return STATUS_BERHASIL; }
status_t gpt_init(void) { return STATUS_BERHASIL; }
status_t penyimpanan_usb_init(void) { return STATUS_BERHASIL; }
