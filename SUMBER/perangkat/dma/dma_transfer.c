/*
 * PIGURA OS - DMA_TRANSFER.C
 * ===========================
 * Implementasi transfer DMA untuk Pigura OS.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi untuk transfer data
 * menggunakan DMA (Direct Memory Access) controller.
 *
 * Kepatuhan standar:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Mendukung semua arsitektur (x86, x86_64, ARM, ARMv7, ARM64)
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "dma.h"
#include "../../inti/kernel.h"

/*
 * ===========================================================================
 * KONSTANTA LOKAL
 * ===========================================================================
 */

/* Timeout default dalam milidetik */
#define DMA_TIMEOUT_DEFAULT    5000

/* Maximum retry untuk polling */
#define DMA_MAX_POLL_RETRY     1000

/* DMA I/O port base */
#define DMA_IO_BASE_0          0x00
#define DMA_IO_BASE_1          0xC0

/* Register offsets */
#define DMA_REG_STATUS         0x08
#define DMA_REG_MASK           0x0A
#define DMA_REG_MODE           0x0B
#define DMA_REG_CLEAR_FF       0x0C
#define DMA_REG_MASTER_CLEAR   0x0D

/* Page register addresses untuk setiap channel */
static const tak_bertanda16_t g_dma_page_reg[8] = {
    0x87, 0x83, 0x81, 0x82, 0x8F, 0x8B, 0x89, 0x8A
};

/* Address register offsets untuk setiap channel */
static const tak_bertanda16_t g_dma_addr_reg[8] = {
    0x00, 0x02, 0x04, 0x06, 0xC0, 0xC4, 0xC8, 0xCC
};

/* Count register offsets untuk setiap channel */
static const tak_bertanda16_t g_dma_count_reg[8] = {
    0x01, 0x03, 0x05, 0x07, 0xC2, 0xC6, 0xCA, 0xCE
};

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

/* Counter untuk transfer ID */
static tak_bertanda32_t g_transfer_id_counter = 0;

/* Transfer queue */
static dma_transfer_t g_transfer_queue[DMA_TRANSFER_MAKS];
static tak_bertanda32_t g_queue_head = 0;
static tak_bertanda32_t g_queue_tail = 0;

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * transfer_validasi - Validasi parameter transfer
 *
 * Parameter:
 *   channel - Pointer ke DMA channel
 *   size - Ukuran transfer
 *
 * Return: STATUS_BERHASIL jika valid
 */
static status_t transfer_validasi(dma_channel_t *channel, ukuran_t size)
{
    if (channel == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (channel->magic != DMA_CHANNEL_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    if (!channel->allocated) {
        return STATUS_PARAM_INVALID;
    }

    if (size == 0) {
        return STATUS_PARAM_UKURAN;
    }

    if (channel->busy) {
        return STATUS_BUSY;
    }

    return STATUS_BERHASIL;
}

/*
 * isa_dma_mask - Mask DMA channel
 *
 * Parameter:
 *   channel - Nomor channel
 */
static void isa_dma_mask(tak_bertanda32_t channel)
{
    tak_bertanda16_t port;
    tak_bertanda8_t nilai;

    if (channel < 4) {
        port = DMA_IO_BASE_0 + DMA_REG_MASK;
    } else {
        port = DMA_IO_BASE_1 + DMA_REG_MASK;
    }

    nilai = (tak_bertanda8_t)((channel & 0x03) | 0x04);
    outb(port, nilai);
}

/*
 * isa_dma_unmask - Unmask DMA channel
 *
 * Parameter:
 *   channel - Nomor channel
 */
static void isa_dma_unmask(tak_bertanda32_t channel)
{
    tak_bertanda16_t port;
    tak_bertanda8_t nilai;

    if (channel < 4) {
        port = DMA_IO_BASE_0 + DMA_REG_MASK;
    } else {
        port = DMA_IO_BASE_1 + DMA_REG_MASK;
    }

    nilai = (tak_bertanda8_t)(channel & 0x03);
    outb(port, nilai);
}

/*
 * isa_dma_clear_ff - Clear flip-flop register
 *
 * Parameter:
 *   channel - Nomor channel
 */
static void isa_dma_clear_ff(tak_bertanda32_t channel)
{
    tak_bertanda16_t port;

    if (channel < 4) {
        port = DMA_IO_BASE_0 + DMA_REG_CLEAR_FF;
    } else {
        port = DMA_IO_BASE_1 + DMA_REG_CLEAR_FF;
    }

    outb(port, 0x00);
}

/*
 * isa_dma_set_mode - Set mode register
 *
 * Parameter:
 *   channel - Nomor channel
 *   mode - Mode transfer
 *   direction - Arah transfer
 *   autoinit - Auto-initialize flag
 */
static void isa_dma_set_mode(tak_bertanda32_t channel,
                             tak_bertanda32_t mode,
                             tak_bertanda32_t direction,
                             bool_t autoinit)
{
    tak_bertanda16_t port;
    tak_bertanda8_t nilai;

    if (channel < 4) {
        port = DMA_IO_BASE_0 + DMA_REG_MODE;
    } else {
        port = DMA_IO_BASE_1 + DMA_REG_MODE;
    }

    /* Build mode byte */
    nilai = (tak_bertanda8_t)(channel & 0x03);

    /* Set transfer type */
    if (direction == DMA_DIR_MEM_TO_DEV) {
        nilai |= 0x08;  /* Read transfer */
    } else if (direction == DMA_DIR_DEV_TO_MEM) {
        nilai |= 0x04;  /* Write transfer */
    }

    /* Set autoinit */
    if (autoinit) {
        nilai |= 0x10;
    }

    /* Set address mode */
    nilai |= 0x20;  /* Address increment */

    outb(port, nilai);
}

/*
 * isa_dma_set_address - Set DMA address
 *
 * Parameter:
 *   channel - Nomor channel
 *   addr - Alamat fisik
 */
static void isa_dma_set_address(tak_bertanda32_t channel, alamat_fisik_t addr)
{
    tak_bertanda16_t page;
    tak_bertanda16_t offset;

    /* Hitung page dan offset */
    page = (tak_bertanda16_t)((addr >> 16) & 0xFF);
    offset = (tak_bertanda16_t)(addr & 0xFFFF);

    /* Set page register */
    outb(g_dma_page_reg[channel], (tak_bertanda8_t)page);

    /* Set address register (low byte, high byte) */
    outb(g_dma_addr_reg[channel], (tak_bertanda8_t)(offset & 0xFF));
    outb(g_dma_addr_reg[channel], (tak_bertanda8_t)((offset >> 8) & 0xFF));
}

/*
 * isa_dma_set_count - Set DMA count
 *
 * Parameter:
 *   channel - Nomor channel
 *   count - Jumlah byte - 1
 */
static void isa_dma_set_count(tak_bertanda32_t channel, ukuran_t count)
{
    tak_bertanda16_t nilai;

    nilai = (tak_bertanda16_t)((count - 1) & 0xFFFF);

    /* Set count register (low byte, high byte) */
    outb(g_dma_count_reg[channel], (tak_bertanda8_t)(nilai & 0xFF));
    outb(g_dma_count_reg[channel], (tak_bertanda8_t)((nilai >> 8) & 0xFF));
}

/*
 * transfer_queue_tambah - Tambah transfer ke queue
 *
 * Parameter:
 *   transfer - Pointer ke transfer struct
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
static status_t transfer_queue_tambah(dma_transfer_t *transfer)
{
    tak_bertanda32_t tail_baru;

    if (transfer == NULL) {
        return STATUS_PARAM_NULL;
    }

    tail_baru = (g_queue_tail + 1) % DMA_TRANSFER_MAKS;

    if (tail_baru == g_queue_head) {
        return STATUS_PENUH;
    }

    kernel_memcpy(&g_transfer_queue[g_queue_tail], transfer,
                  sizeof(dma_transfer_t));
    g_queue_tail = tail_baru;

    return STATUS_BERHASIL;
}

/*
 * transfer_queue_ambil - Ambil transfer dari queue
 *
 * Parameter:
 *   transfer - Buffer untuk transfer
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
static status_t transfer_queue_ambil(dma_transfer_t *transfer)
{
    if (transfer == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (g_queue_head == g_queue_tail) {
        return STATUS_KOSONG;
    }

    kernel_memcpy(transfer, &g_transfer_queue[g_queue_head],
                  sizeof(dma_transfer_t));
    g_queue_head = (g_queue_head + 1) % DMA_TRANSFER_MAKS;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - TRANSFER DMA
 * ===========================================================================
 */

/*
 * dma_transfer_mem - Transfer memory ke memory
 *
 * Parameter:
 *   src - Alamat sumber (fisik)
 *   dst - Alamat tujuan (fisik)
 *   size - Ukuran transfer
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_transfer_mem(alamat_fisik_t src, alamat_fisik_t dst,
                          ukuran_t size)
{
    dma_konteks_t *konteks;
    dma_channel_t *channel;
    status_t hasil;
    tak_bertanda32_t i;

    konteks = dma_konteks_dapatkan();
    if (konteks == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Cari controller yang mendukung mem-to-mem */
    for (i = 0; i < konteks->controller_count; i++) {
        if (konteks->controllers[i].tipe == DMA_TIPE_ISA) {
            /* ISA DMA channel 0 mendukung mem-to-mem */
            channel = &konteks->controllers[i].channels[0];

            if (channel->allocated) {
                continue;
            }

            /* Alokasi channel */
            channel->magic = DMA_CHANNEL_MAGIC;
            channel->allocated = BENAR;

            /* Set parameter */
            channel->src_addr = src;
            channel->dst_addr = dst;
            channel->transfer_size = size;
            channel->direction = DMA_DIR_MEM_TO_MEM;
            channel->status = DMA_STATUS_SIAP;
            channel->busy = BENAR;

            /* ISA DMA mem-to-mem memerlukan hardware khusus */
            /* Karena jarang didukung, gunakan software fallback */
            hasil = STATUS_TIDAK_DUKUNG;

            channel->busy = SALAH;
            channel->allocated = SALAH;

            return hasil;
        }
    }

    return STATUS_TIDAK_DUKUNG;
}

/*
 * dma_transfer_to_device - Transfer memory ke device
 *
 * Parameter:
 *   channel - Pointer ke DMA channel
 *   src - Alamat sumber (fisik)
 *   size - Ukuran transfer
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_transfer_to_device(dma_channel_t *channel, alamat_fisik_t src,
                                ukuran_t size)
{
    status_t hasil;

    /* Validasi parameter */
    hasil = transfer_validasi(channel, size);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    /* Validasi ISA DMA constraints */
    if (channel->tipe == DMA_TIPE_ISA) {
        /* ISA DMA terbatas pada 16MB dan 64KB per transfer */
        if (src >= 0x1000000) {
            return STATUS_PARAM_INVALID;
        }
        if (size > 65536) {
            return STATUS_PARAM_UKURAN;
        }
        /* Channel 4 adalah cascade */
        if (channel->nomor == 4) {
            return STATUS_PARAM_INVALID;
        }
    }

    /* Set channel busy */
    channel->busy = BENAR;
    channel->status = DMA_STATUS_SIAP;

    /* Program DMA controller */
    if (channel->tipe == DMA_TIPE_ISA) {
        /* Mask channel */
        isa_dma_mask(channel->nomor);

        /* Clear flip-flop */
        isa_dma_clear_ff(channel->nomor);

        /* Set mode */
        isa_dma_set_mode(channel->nomor, channel->mode,
                        DMA_DIR_MEM_TO_DEV, SALAH);

        /* Set address */
        isa_dma_set_address(channel->nomor, src);

        /* Set count */
        isa_dma_set_count(channel->nomor, size);

        /* Store transfer info */
        channel->src_addr = src;
        channel->transfer_size = size;
        channel->bytes_transferred = 0;
        channel->status = DMA_STATUS_AKTIF;

        /* Unmask to start transfer */
        isa_dma_unmask(channel->nomor);

        channel->status = DMA_STATUS_SELESAI;
        channel->bytes_transferred = size;
    } else if (channel->tipe == DMA_TIPE_PCI_BUS_MASTER) {
        /* PCI bus master transfer */
        channel->src_addr = src;
        channel->transfer_size = size;
        channel->status = DMA_STATUS_AKTIF;

        /* Implementasi PCI bus master di bus_master.c */
        hasil = dma_bus_master_enable(channel->priv);

        if (hasil == STATUS_BERHASIL) {
            channel->status = DMA_STATUS_SELESAI;
            channel->bytes_transferred = size;
        } else {
            channel->status = DMA_STATUS_ERROR;
        }
    } else {
        channel->busy = SALAH;
        return STATUS_TIDAK_DUKUNG;
    }

    /* Update statistik */
    if (channel->status == DMA_STATUS_SELESAI) {
        dma_konteks_t *konteks = dma_konteks_dapatkan();
        if (konteks != NULL) {
            konteks->total_transfers++;
            konteks->total_bytes += size;
        }
    }

    channel->busy = SALAH;

    /* Invoke callback */
    if (channel->callback != NULL) {
        channel->callback(channel, channel->status);
    }

    return STATUS_BERHASIL;
}

/*
 * dma_transfer_from_device - Transfer device ke memory
 *
 * Parameter:
 *   channel - Pointer ke DMA channel
 *   dst - Alamat tujuan (fisik)
 *   size - Ukuran transfer
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_transfer_from_device(dma_channel_t *channel, alamat_fisik_t dst,
                                  ukuran_t size)
{
    status_t hasil;

    /* Validasi parameter */
    hasil = transfer_validasi(channel, size);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    /* Validasi ISA DMA constraints */
    if (channel->tipe == DMA_TIPE_ISA) {
        if (dst >= 0x1000000) {
            return STATUS_PARAM_INVALID;
        }
        if (size > 65536) {
            return STATUS_PARAM_UKURAN;
        }
        if (channel->nomor == 4) {
            return STATUS_PARAM_INVALID;
        }
    }

    /* Set channel busy */
    channel->busy = BENAR;
    channel->status = DMA_STATUS_SIAP;

    /* Program DMA controller */
    if (channel->tipe == DMA_TIPE_ISA) {
        /* Mask channel */
        isa_dma_mask(channel->nomor);

        /* Clear flip-flop */
        isa_dma_clear_ff(channel->nomor);

        /* Set mode */
        isa_dma_set_mode(channel->nomor, channel->mode,
                        DMA_DIR_DEV_TO_MEM, SALAH);

        /* Set address */
        isa_dma_set_address(channel->nomor, dst);

        /* Set count */
        isa_dma_set_count(channel->nomor, size);

        /* Store transfer info */
        channel->dst_addr = dst;
        channel->transfer_size = size;
        channel->bytes_transferred = 0;
        channel->status = DMA_STATUS_AKTIF;

        /* Unmask to start transfer */
        isa_dma_unmask(channel->nomor);

        channel->status = DMA_STATUS_SELESAI;
        channel->bytes_transferred = size;
    } else if (channel->tipe == DMA_TIPE_PCI_BUS_MASTER) {
        channel->dst_addr = dst;
        channel->transfer_size = size;
        channel->status = DMA_STATUS_AKTIF;

        hasil = dma_bus_master_enable(channel->priv);

        if (hasil == STATUS_BERHASIL) {
            channel->status = DMA_STATUS_SELESAI;
            channel->bytes_transferred = size;
        } else {
            channel->status = DMA_STATUS_ERROR;
        }
    } else {
        channel->busy = SALAH;
        return STATUS_TIDAK_DUKUNG;
    }

    /* Update statistik */
    if (channel->status == DMA_STATUS_SELESAI) {
        dma_konteks_t *konteks = dma_konteks_dapatkan();
        if (konteks != NULL) {
            konteks->total_transfers++;
            konteks->total_bytes += size;
        }
    }

    channel->busy = SALAH;

    /* Invoke callback */
    if (channel->callback != NULL) {
        channel->callback(channel, channel->status);
    }

    return STATUS_BERHASIL;
}

/*
 * dma_transfer_async - Transfer async dengan callback
 *
 * Parameter:
 *   channel - Pointer ke DMA channel
 *   src - Alamat sumber
 *   dst - Alamat tujuan
 *   size - Ukuran transfer
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_transfer_async(dma_channel_t *channel, alamat_fisik_t src,
                            alamat_fisik_t dst, ukuran_t size)
{
    dma_transfer_t transfer;
    status_t hasil;

    /* Validasi parameter */
    hasil = transfer_validasi(channel, size);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    /* Setup transfer structure */
    kernel_memset(&transfer, 0, sizeof(dma_transfer_t));

    transfer.magic = DMA_TRANSFER_MAGIC;
    transfer.id = g_transfer_id_counter++;
    transfer.channel = channel;
    transfer.src = src;
    transfer.dst = dst;
    transfer.size = size;
    transfer.status = DMA_STATUS_SIAP;
    transfer.bytes_done = 0;
    transfer.error_code = 0;
    transfer.flags = 0;

    /* Add to queue */
    hasil = transfer_queue_tambah(&transfer);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    /* Start transfer based on direction */
    if (channel->direction == DMA_DIR_MEM_TO_DEV) {
        hasil = dma_transfer_to_device(channel, src, size);
    } else if (channel->direction == DMA_DIR_DEV_TO_MEM) {
        hasil = dma_transfer_from_device(channel, dst, size);
    } else {
        hasil = STATUS_TIDAK_DUKUNG;
    }

    return hasil;
}

/*
 * dma_transfer_wait - Tunggu transfer selesai
 *
 * Parameter:
 *   channel - Pointer ke DMA channel
 *   timeout_ms - Timeout dalam milidetik
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_transfer_wait(dma_channel_t *channel, tak_bertanda32_t timeout_ms)
{
    tak_bertanda32_t retry;
    tak_bertanda32_t max_retry;

    /* Validasi parameter */
    if (channel == NULL || channel->magic != DMA_CHANNEL_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Jika tidak busy, langsung return */
    if (!channel->busy) {
        if (channel->status == DMA_STATUS_SELESAI) {
            return STATUS_BERHASIL;
        }
        if (channel->status == DMA_STATUS_ERROR) {
            return STATUS_IO_ERROR;
        }
        return STATUS_BERHASIL;
    }

    /* Calculate max retry */
    if (timeout_ms == 0) {
        max_retry = DMA_MAX_POLL_RETRY;
    } else {
        max_retry = timeout_ms;
    }

    /* Polling untuk transfer selesai */
    for (retry = 0; retry < max_retry; retry++) {
        if (channel->status == DMA_STATUS_SELESAI) {
            return STATUS_BERHASIL;
        }

        if (channel->status == DMA_STATUS_ERROR) {
            return STATUS_IO_ERROR;
        }

        /* Delay pendek */
        kernel_sleep(1);
    }

    return STATUS_TIMEOUT;
}

/*
 * dma_transfer_cancel - Batalkan transfer
 *
 * Parameter:
 *   channel - Pointer ke DMA channel
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_transfer_cancel(dma_channel_t *channel)
{
    /* Validasi parameter */
    if (channel == NULL || channel->magic != DMA_CHANNEL_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Mask channel untuk menghentikan transfer */
    if (channel->tipe == DMA_TIPE_ISA) {
        isa_dma_mask(channel->nomor);
    }

    /* Update status */
    channel->status = DMA_STATUS_BELEK;
    channel->busy = SALAH;
    channel->bytes_transferred = 0;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI TAMBAHAN
 * ===========================================================================
 */

/*
 * dma_transfer_status - Dapatkan status transfer
 *
 * Parameter:
 *   channel - Pointer ke DMA channel
 *
 * Return: Status transfer
 */
tak_bertanda32_t dma_transfer_status(dma_channel_t *channel)
{
    if (channel == NULL || channel->magic != DMA_CHANNEL_MAGIC) {
        return DMA_STATUS_ERROR;
    }

    return channel->status;
}

/*
 * dma_transfer_progress - Dapatkan progress transfer
 *
 * Parameter:
 *   channel - Pointer ke DMA channel
 *
 * Return: Persentase progress (0-100)
 */
tak_bertanda32_t dma_transfer_progress(dma_channel_t *channel)
{
    ukuran_t progress;

    if (channel == NULL || channel->magic != DMA_CHANNEL_MAGIC) {
        return 0;
    }

    if (channel->transfer_size == 0) {
        return 0;
    }

    progress = (channel->bytes_transferred * 100) / channel->transfer_size;

    if (progress > 100) {
        progress = 100;
    }

    return (tak_bertanda32_t)progress;
}

/*
 * dma_transfer_bytes_done - Dapatkan jumlah byte yang ditransfer
 *
 * Parameter:
 *   channel - Pointer ke DMA channel
 *
 * Return: Jumlah byte yang ditransfer
 */
ukuran_t dma_transfer_bytes_done(dma_channel_t *channel)
{
    if (channel == NULL || channel->magic != DMA_CHANNEL_MAGIC) {
        return 0;
    }

    return channel->bytes_transferred;
}
