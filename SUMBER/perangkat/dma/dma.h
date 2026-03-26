/*
 * PIGURA OS - DMA.H
 * ==================
 * Header untuk DMA (Direct Memory Access) controller.
 *
 * Berkas ini mendefinisikan interface untuk DMA controller
 * dan transfer data tanpa CPU intervention.
 *
 * Kepatuhan standar:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Mendukung semua arsitektur
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#ifndef PERANGKAT_DMA_H
#define PERANGKAT_DMA_H

/*
 * ===========================================================================
 * INCLUDE
 * ===========================================================================
 */

#include "../../inti/types.h"

/*
 * ===========================================================================
 * KONSTANTA DMA
 * ===========================================================================
 */

/* Versi modul */
#define DMA_VERSI_MAJOR 1
#define DMA_VERSI_MINOR 0
#define DMA_VERSI_PATCH 0

/* Magic numbers */
#define DMA_MAGIC              0x444D4100  /* "DMA\0" */
#define DMA_CHANNEL_MAGIC      0x444D4348  /* "DMCH" */
#define DMA_TRANSFER_MAGIC     0x444D5452  /* "DMTR" */

/* Batas sistem */
#define DMA_CHANNEL_MAKS       16
#define DMA_TRANSFER_MAKS      64
#define DMA_BUFFER_MAKS        (64 * 1024)  /* 64 KB */
#define DMA_ALIGN              4             /* 4-byte alignment */

/* Tipe DMA controller */
#define DMA_TIPE_TIDAK_DIKETAHUI  0
#define DMA_TIPE_ISA              1
#define DMA_TIPE_PCI_BUS_MASTER   2
#define DMA_TIPE_INTEL_IOAT       3
#define DMA_TIPE_ARM_PL330        4

/* Mode transfer */
#define DMA_MODE_TIDAK_ADA        0
#define DMA_MODE_READ             1
#define DMA_MODE_WRITE            2
#define DMA_MODE_VERIFY           3

/* Mode operasi */
#define DMA_OP_MODE_DEMAND        0
#define DMA_OP_MODE_SINGLE        1
#define DMA_OP_MODE_BLOCK         2
#define DMA_OP_MODE_CASCADE       3

/* Transfer direction */
#define DMA_DIR_MEM_TO_MEM        0
#define DMA_DIR_MEM_TO_DEV        1
#define DMA_DIR_DEV_TO_MEM        2
#define DMA_DIR_DEV_TO_DEV        3

/* Channel status */
#define DMA_STATUS_BELEK          0
#define DMA_STATUS_SIAP           1
#define DMA_STATUS_AKTIF          2
#define DMA_STATUS_SELESAI        3
#define DMA_STATUS_ERROR          4

/* DMA flags */
#define DMA_FLAG_AUTOINIT         0x01
#define DMA_FLAG_INCREMENT        0x02
#define DMA_FLAG_DECREMENT        0x04
#define DMA_FLAG_FLYBY            0x08

/* ISA DMA channels */
#define DMA_ISA_CHANNEL_0         0
#define DMA_ISA_CHANNEL_1         1
#define DMA_ISA_CHANNEL_2         2
#define DMA_ISA_CHANNEL_3         3
#define DMA_ISA_CHANNEL_5         5
#define DMA_ISA_CHANNEL_6         6
#define DMA_ISA_CHANNEL_7         7

/* Register offsets (ISA DMA) */
#define DMA_REG_STATUS            0x08
#define DMA_REG_COMMAND           0x08
#define DMA_REG_REQUEST           0x09
#define DMA_REG_MASK              0x0A
#define DMA_REG_MODE              0x0B
#define DMA_REG_CLEAR_FF          0x0C
#define DMA_REG_MASTER_CLEAR      0x0D
#define DMA_REG_CLEAR_MASK        0x0E
#define DMA_REG_MASK_ALL          0x0F

/*
 * ===========================================================================
 * STRUKTUR DMA CHANNEL
 * ===========================================================================
 */

typedef struct dma_channel dma_channel_t;

/* Callback untuk transfer selesai */
typedef void (*dma_callback_t)(dma_channel_t *channel, tak_bertanda32_t status);

struct dma_channel {
    tak_bertanda32_t magic;         /* Magic number */
    tak_bertanda32_t id;            /* Channel ID */
    
    /* Konfigurasi */
    tak_bertanda32_t tipe;          /* Controller type */
    tak_bertanda32_t nomor;         /* Channel number */
    tak_bertanda32_t mode;          /* Transfer mode */
    tak_bertanda32_t direction;     /* Transfer direction */
    
    /* Alamat */
    alamat_fisik_t src_addr;        /* Source address */
    alamat_fisik_t dst_addr;        /* Destination address */
    ukuran_t transfer_size;         /* Transfer size */
    ukuran_t bytes_transferred;     /* Bytes transferred */
    
    /* Buffer */
    void *buffer;                   /* DMA buffer */
    ukuran_t buffer_size;           /* Buffer size */
    alamat_fisik_t buffer_phys;     /* Physical address */
    
    /* Status */
    tak_bertanda32_t status;        /* Current status */
    bool_t allocated;               /* Is allocated */
    bool_t enabled;                 /* Is enabled */
    bool_t busy;                    /* Is busy */
    
    /* Callback */
    dma_callback_t callback;        /* Completion callback */
    
    /* Private data */
    void *priv;
    
    /* Next channel */
    dma_channel_t *berikutnya;
};

/*
 * ===========================================================================
 * STRUKTUR DMA TRANSFER
 * ===========================================================================
 */

typedef struct {
    tak_bertanda32_t magic;         /* Magic number */
    tak_bertanda32_t id;            /* Transfer ID */
    
    /* Channel */
    dma_channel_t *channel;
    
    /* Addresses */
    alamat_fisik_t src;
    alamat_fisik_t dst;
    ukuran_t size;
    
    /* Status */
    tak_bertanda32_t status;
    ukuran_t bytes_done;
    tak_bertanda32_t error_code;
    
    /* Timing */
    tak_bertanda64_t start_time;
    tak_bertanda64_t end_time;
    
    /* Flags */
    tak_bertanda32_t flags;
    
} dma_transfer_t;

/*
 * ===========================================================================
 * STRUKTUR DMA CONTROLLER
 * ===========================================================================
 */

typedef struct {
    tak_bertanda32_t magic;         /* Magic number */
    tak_bertanda32_t id;            /* Controller ID */
    
    /* Tipe */
    tak_bertanda32_t tipe;          /* Controller type */
    
    /* Channels */
    dma_channel_t channels[DMA_CHANNEL_MAKS];
    tak_bertanda32_t channel_count;
    
    /* Base addresses */
    alamat_fisik_t base_addr;       /* Controller base address */
    alamat_fisik_t page_addr;       /* Page register address */
    
    /* Statistics */
    tak_bertanda64_t total_transfers;
    tak_bertanda64_t total_bytes;
    tak_bertanda64_t errors;
    
    /* Status */
    bool_t initialized;
    bool_t enabled;
    
} dma_controller_t;

/*
 * ===========================================================================
 * STRUKTUR KONTEKS DMA
 * ===========================================================================
 */

typedef struct {
    tak_bertanda32_t magic;         /* Magic number */
    
    /* Controllers */
    dma_controller_t controllers[4];
    tak_bertanda32_t controller_count;
    
    /* Channels */
    dma_channel_t *channel_list;
    tak_bertanda32_t channel_count;
    
    /* Transfer queue */
    dma_transfer_t transfers[DMA_TRANSFER_MAKS];
    tak_bertanda32_t transfer_head;
    tak_bertanda32_t transfer_tail;
    
    /* Buffer pool */
    void *buffer_pool;
    ukuran_t buffer_pool_size;
    alamat_fisik_t buffer_pool_phys;
    
    /* Statistics */
    tak_bertanda64_t total_transfers;
    tak_bertanda64_t total_bytes;
    
    /* Status */
    bool_t initialized;
    
} dma_konteks_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

extern dma_konteks_t g_dma_konteks;
extern bool_t g_dma_diinisialisasi;

/*
 * ===========================================================================
 * FUNGSI INISIALISASI
 * ===========================================================================
 */

/*
 * dma_init - Inisialisasi subsistem DMA
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_init(void);

/*
 * dma_shutdown - Matikan subsistem DMA
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_shutdown(void);

/*
 * dma_konteks_dapatkan - Dapatkan konteks DMA
 *
 * Return: Pointer ke konteks atau NULL
 */
dma_konteks_t *dma_konteks_dapatkan(void);

/*
 * ===========================================================================
 * FUNGSI CONTROLLER
 * ===========================================================================
 */

/*
 * dma_controller_init - Inisialisasi DMA controller
 *
 * Parameter:
 *   tipe - Tipe controller
 *   base_addr - Base address
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_controller_init(tak_bertanda32_t tipe,
                              alamat_fisik_t base_addr);

/*
 * dma_controller_reset - Reset DMA controller
 *
 * Parameter:
 *   ctrl - Pointer ke controller
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_controller_reset(dma_controller_t *ctrl);

/*
 * ===========================================================================
 * FUNGSI CHANNEL
 * ===========================================================================
 */

/*
 * dma_channel_alokasi - Alokasi DMA channel
 *
 * Parameter:
 *   tipe - Tipe controller
 *   nomor - Nomor channel (-1 untuk auto)
 *
 * Return: Pointer ke channel atau NULL
 */
dma_channel_t *dma_channel_alokasi(tak_bertanda32_t tipe,
                                    tanda32_t nomor);

/*
 * dma_channel_bebaskan - Bebaskan DMA channel
 *
 * Parameter:
 *   channel - Pointer ke channel
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_channel_bebaskan(dma_channel_t *channel);

/*
 * dma_channel_konfigurasi - Konfigurasi DMA channel
 *
 * Parameter:
 *   channel - Pointer ke channel
 *   mode - Transfer mode
 *   direction - Transfer direction
 *   autoinit - Auto-initialize flag
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_channel_konfigurasi(dma_channel_t *channel,
                                  tak_bertanda32_t mode,
                                  tak_bertanda32_t direction,
                                  bool_t autoinit);

/*
 * dma_channel_set_callback - Set callback untuk channel
 *
 * Parameter:
 *   channel - Pointer ke channel
 *   callback - Fungsi callback
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_channel_set_callback(dma_channel_t *channel,
                                   dma_callback_t callback);

/*
 * ===========================================================================
 * FUNGSI TRANSFER
 * ===========================================================================
 */

/*
 * dma_transfer_mem - Transfer memory ke memory
 *
 * Parameter:
 *   src - Alamat sumber (physical)
 *   dst - Alamat tujuan (physical)
 *   size - Ukuran transfer
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_transfer_mem(alamat_fisik_t src, alamat_fisik_t dst,
                           ukuran_t size);

/*
 * dma_transfer_to_device - Transfer memory ke device
 *
 * Parameter:
 *   channel - Pointer ke channel
 *   src - Alamat sumber (physical)
 *   size - Ukuran transfer
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_transfer_to_device(dma_channel_t *channel,
                                 alamat_fisik_t src, ukuran_t size);

/*
 * dma_transfer_from_device - Transfer device ke memory
 *
 * Parameter:
 *   channel - Pointer ke channel
 *   dst - Alamat tujuan (physical)
 *   size - Ukuran transfer
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_transfer_from_device(dma_channel_t *channel,
                                   alamat_fisik_t dst, ukuran_t size);

/*
 * dma_transfer_async - Transfer async dengan callback
 *
 * Parameter:
 *   channel - Pointer ke channel
 *   src - Alamat sumber
 *   dst - Alamat tujuan
 *   size - Ukuran transfer
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_transfer_async(dma_channel_t *channel, alamat_fisik_t src,
                             alamat_fisik_t dst, ukuran_t size);

/*
 * dma_transfer_wait - Tunggu transfer selesai
 *
 * Parameter:
 *   channel - Pointer ke channel
 *   timeout_ms - Timeout dalam ms
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_transfer_wait(dma_channel_t *channel,
                            tak_bertanda32_t timeout_ms);

/*
 * dma_transfer_cancel - Batalkan transfer
 *
 * Parameter:
 *   channel - Pointer ke channel
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_transfer_cancel(dma_channel_t *channel);

/*
 * ===========================================================================
 * FUNGSI BUFFER
 * ===========================================================================
 */

/*
 * dma_buffer_alokasi - Alokasi DMA buffer
 *
 * Parameter:
 *   size - Ukuran buffer
 *
 * Return: Pointer ke buffer atau NULL
 */
void *dma_buffer_alokasi(ukuran_t size);

/*
 * dma_buffer_bebaskan - Bebaskan DMA buffer
 *
 * Parameter:
 *   buffer - Pointer ke buffer
 */
void dma_buffer_bebaskan(void *buffer);

/*
 * dma_buffer_phys - Dapatkan alamat fisik buffer
 *
 * Parameter:
 *   buffer - Pointer ke buffer
 *
 * Return: Alamat fisik
 */
alamat_fisik_t dma_buffer_phys(void *buffer);

/*
 * ===========================================================================
 * FUNGSI ISA DMA
 * ===========================================================================
 */

/*
 * dma_isa_init - Inisialisasi ISA DMA
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_isa_init(void);

/*
 * dma_isa_mask_channel - Mask ISA DMA channel
 *
 * Parameter:
 *   channel - Nomor channel
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_isa_mask_channel(tak_bertanda32_t channel);

/*
 * dma_isa_unmask_channel - Unmask ISA DMA channel
 *
 * Parameter:
 *   channel - Nomor channel
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_isa_unmask_channel(tak_bertanda32_t channel);

/*
 * ===========================================================================
 * FUNGSI BUS MASTER DMA
 * ===========================================================================
 */

/*
 * dma_bus_master_init - Inisialisasi bus master DMA
 *
 * Parameter:
 *   dev - PCI device
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_bus_master_init(void *dev);

/*
 * dma_bus_master_enable - Enable bus master
 *
 * Parameter:
 *   dev - PCI device
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_bus_master_enable(void *dev);

/*
 * ===========================================================================
 * FUNGSI UTILITAS
 * ===========================================================================
 */

/*
 * dma_status_nama - Dapatkan nama status
 *
 * Parameter:
 *   status - Status code
 *
 * Return: String nama status
 */
const char *dma_status_nama(tak_bertanda32_t status);

/*
 * dma_tipe_nama - Dapatkan nama tipe
 *
 * Parameter:
 *   tipe - Tipe controller
 *
 * Return: String nama tipe
 */
const char *dma_tipe_nama(tak_bertanda32_t tipe);

/*
 * dma_cetak_info - Cetak informasi DMA
 */
void dma_cetak_info(void);

/*
 * dma_cetak_channel - Cetak informasi channel
 *
 * Parameter:
 *   channel - Pointer ke channel
 */
void dma_cetak_channel(dma_channel_t *channel);

#endif /* PERANGKAT_DMA_H */
