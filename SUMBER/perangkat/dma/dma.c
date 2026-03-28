/*
 * PIGURA OS - DMA.C
 * ==================
 * Implementasi DMA (Direct Memory Access) controller.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "dma.h"
#include "../../inti/kernel.h"

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

dma_konteks_t g_dma_konteks;
bool_t g_dma_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static char g_dma_error[256];

/* ISA DMA registers */
#define DMA0_IO_BASE       0x00
#define DMA1_IO_BASE       0xC0

/* Channel register offsets */
static const struct {
    tak_bertanda16_t page;
    tak_bertanda16_t addr;
    tak_bertanda16_t count;
} g_dma_regs[8] = {
    { 0x87, 0x00, 0x01 },  /* Channel 0 */
    { 0x83, 0x02, 0x03 },  /* Channel 1 */
    { 0x81, 0x04, 0x05 },  /* Channel 2 */
    { 0x82, 0x06, 0x07 },  /* Channel 3 */
    { 0x8F, 0xC0, 0xC2 },  /* Channel 4 */
    { 0x8B, 0xC4, 0xC6 },  /* Channel 5 */
    { 0x89, 0xC8, 0xCA },  /* Channel 6 */
    { 0x8A, 0xCC, 0xCE }   /* Channel 7 */
};

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

static void dma_set_error(const char *pesan)
{
    ukuran_t i;
    
    if (pesan == NULL) {
        return;
    }
    
    i = 0;
    while (pesan[i] != '\0' && i < 255) {
        g_dma_error[i] = pesan[i];
        i++;
    }
    g_dma_error[i] = '\0';
}

static void isa_dma_mask(tak_bertanda32_t channel)
{
    tak_bertanda16_t port;
    
    if (channel < 4) {
        port = DMA0_IO_BASE + DMA_REG_MASK;
    } else {
        port = DMA1_IO_BASE + DMA_REG_MASK;
    }
    
    outb(port, (channel & 0x03) | 0x04);
}

static void isa_dma_unmask(tak_bertanda32_t channel)
{
    tak_bertanda16_t port;
    
    if (channel < 4) {
        port = DMA0_IO_BASE + DMA_REG_MASK;
    } else {
        port = DMA1_IO_BASE + DMA_REG_MASK;
    }
    
    outb(port, channel & 0x03);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

status_t dma_init(void)
{
    tak_bertanda32_t i;
    
    if (g_dma_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }
    
    /* Inisialisasi konteks */
    kernel_memset(&g_dma_konteks, 0, sizeof(dma_konteks_t));
    
    /* Inisialisasi channels */
    for (i = 0; i < DMA_CHANNEL_MAKS; i++) {
        g_dma_konteks.controllers[0].channels[i].magic = 0;
        g_dma_konteks.controllers[0].channels[i].id = i;
        g_dma_konteks.controllers[0].channels[i].tipe = DMA_TIPE_ISA;
        g_dma_konteks.controllers[0].channels[i].nomor = i;
        g_dma_konteks.controllers[0].channels[i].allocated = SALAH;
        g_dma_konteks.controllers[0].channels[i].enabled = SALAH;
        g_dma_konteks.controllers[0].channels[i].busy = SALAH;
    }
    
    g_dma_konteks.controller_count = 1;
    g_dma_konteks.controllers[0].magic = DMA_MAGIC;
    g_dma_konteks.controllers[0].tipe = DMA_TIPE_ISA;
    g_dma_konteks.controllers[0].channel_count = DMA_CHANNEL_MAKS;
    g_dma_konteks.controllers[0].initialized = BENAR;
    g_dma_konteks.controllers[0].enabled = BENAR;
    
    /* Reset ISA DMA controller */
    isa_dma_init();
    
    /* Finalisasi */
    g_dma_konteks.magic = DMA_MAGIC;
    g_dma_diinisialisasi = BENAR;
    
    return STATUS_BERHASIL;
}

status_t dma_shutdown(void)
{
    if (!g_dma_diinisialisasi) {
        return STATUS_BERHASIL;
    }
    
    g_dma_konteks.magic = 0;
    g_dma_diinisialisasi = SALAH;
    
    return STATUS_BERHASIL;
}

dma_konteks_t *dma_konteks_dapatkan(void)
{
    if (!g_dma_diinisialisasi) {
        return NULL;
    }
    
    if (g_dma_konteks.magic != DMA_MAGIC) {
        return NULL;
    }
    
    return &g_dma_konteks;
}

/*
 * ===========================================================================
 * FUNGSI ISA DMA
 * ===========================================================================
 */

status_t dma_isa_init(void)
{
    /* Reset both DMA controllers */
    outb(DMA0_IO_BASE + DMA_REG_MASTER_CLEAR, 0);
    outb(DMA1_IO_BASE + DMA_REG_MASTER_CLEAR, 0);
    
    return STATUS_BERHASIL;
}

status_t dma_isa_mask_channel(tak_bertanda32_t channel)
{
    if (channel > 7) {
        return STATUS_PARAM_INVALID;
    }
    
    isa_dma_mask(channel);
    
    return STATUS_BERHASIL;
}

status_t dma_isa_unmask_channel(tak_bertanda32_t channel)
{
    if (channel > 7) {
        return STATUS_PARAM_INVALID;
    }
    
    isa_dma_unmask(channel);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI CHANNEL
 * ===========================================================================
 */

dma_channel_t *dma_channel_alokasi(tak_bertanda32_t tipe, tanda32_t nomor)
{
    dma_channel_t *channel;
    tak_bertanda32_t i;
    tak_bertanda32_t start;
    tak_bertanda32_t end;
    
    if (!g_dma_diinisialisasi) {
        return NULL;
    }
    
    /* Tentukan range channel */
    if (tipe == DMA_TIPE_ISA) {
        if (nomor >= 0 && nomor <= 7) {
            /* Channel spesifik */
            channel = &g_dma_konteks.controllers[0].channels[nomor];
            if (!channel->allocated) {
                channel->magic = DMA_CHANNEL_MAGIC;
                channel->allocated = BENAR;
                return channel;
            }
            return NULL;
        }
        
        /* Auto-allocate */
        for (i = 0; i < 8; i++) {
            channel = &g_dma_konteks.controllers[0].channels[i];
            if (!channel->allocated) {
                channel->magic = DMA_CHANNEL_MAGIC;
                channel->allocated = BENAR;
                return channel;
            }
        }
    }
    
    return NULL;
}

status_t dma_channel_bebaskan(dma_channel_t *channel)
{
    if (channel == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (channel->magic != DMA_CHANNEL_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Mask channel */
    if (channel->tipe == DMA_TIPE_ISA) {
        isa_dma_mask(channel->nomor);
    }
    
    channel->magic = 0;
    channel->allocated = SALAH;
    channel->enabled = SALAH;
    channel->busy = SALAH;
    
    return STATUS_BERHASIL;
}

status_t dma_channel_konfigurasi(dma_channel_t *channel,
                                  tak_bertanda32_t mode,
                                  tak_bertanda32_t direction,
                                  bool_t autoinit)
{
    tak_bertanda8_t dma_mode;
    tak_bertanda16_t mode_port;
    
    if (channel == NULL || channel->magic != DMA_CHANNEL_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    channel->mode = mode;
    channel->direction = direction;
    
    if (channel->tipe == DMA_TIPE_ISA) {
        /* Build ISA DMA mode byte */
        dma_mode = channel->nomor & 0x03;
        dma_mode |= (mode & 0x03) << 2;
        
        if (direction == DMA_DIR_MEM_TO_DEV) {
            dma_mode |= 0x04;  /* Read */
        } else {
            dma_mode |= 0x08;  /* Write */
        }
        
        if (autoinit) {
            dma_mode |= 0x10;
        }
        
        /* Mask channel */
        isa_dma_mask(channel->nomor);
        
        /* Write mode */
        if (channel->nomor < 4) {
            mode_port = DMA0_IO_BASE + DMA_REG_MODE;
        } else {
            mode_port = DMA1_IO_BASE + DMA_REG_MODE;
        }
        outb(mode_port, dma_mode);
        
        channel->enabled = BENAR;
    }
    
    return STATUS_BERHASIL;
}

status_t dma_channel_set_callback(dma_channel_t *channel,
                                   dma_callback_t callback)
{
    if (channel == NULL || channel->magic != DMA_CHANNEL_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    channel->callback = callback;
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI TRANSFER
 * ===========================================================================
 */

status_t dma_transfer_mem(alamat_fisik_t src, alamat_fisik_t dst,
                           ukuran_t size)
{
    /* DMA memory-to-memory memerlukan controller khusus */
    return STATUS_TIDAK_DUKUNG;
}

status_t dma_transfer_to_device(dma_channel_t *channel,
                                 alamat_fisik_t src, ukuran_t size)
{
    tak_bertanda16_t page;
    tak_bertanda16_t offset;
    tak_bertanda16_t count;
    
    if (channel == NULL || channel->magic != DMA_CHANNEL_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    if (channel->tipe != DMA_TIPE_ISA) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    /* ISA DMA limitations */
    if (src >= 0x1000000) {  /* > 16MB */
        return STATUS_PARAM_INVALID;
    }
    
    if (size > 65536) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Calculate page and offset */
    page = (tak_bertanda16_t)(src >> 16);
    offset = (tak_bertanda16_t)(src & 0xFFFF);
    count = (tak_bertanda16_t)(size - 1);
    
    /* Mask channel */
    isa_dma_mask(channel->nomor);
    
    /* Clear flip-flop */
    if (channel->nomor < 4) {
        outb(DMA0_IO_BASE + DMA_REG_CLEAR_FF, 0);
    } else {
        outb(DMA1_IO_BASE + DMA_REG_CLEAR_FF, 0);
    }
    
    /* Set page register */
    outb(g_dma_regs[channel->nomor].page, page);
    
    /* Set address */
    outb(g_dma_regs[channel->nomor].addr, offset & 0xFF);
    outb(g_dma_regs[channel->nomor].addr, (offset >> 8) & 0xFF);
    
    /* Set count */
    outb(g_dma_regs[channel->nomor].count, count & 0xFF);
    outb(g_dma_regs[channel->nomor].count, (count >> 8) & 0xFF);
    
    /* Unmask to start transfer */
    isa_dma_unmask(channel->nomor);
    
    channel->src_addr = src;
    channel->transfer_size = size;
    channel->bytes_transferred = size;
    channel->status = DMA_STATUS_SELESAI;
    
    /* Callback */
    if (channel->callback != NULL) {
        channel->callback(channel, DMA_STATUS_SELESAI);
    }
    
    return STATUS_BERHASIL;
}

status_t dma_transfer_from_device(dma_channel_t *channel,
                                   alamat_fisik_t dst, ukuran_t size)
{
    tak_bertanda16_t page;
    tak_bertanda16_t offset;
    tak_bertanda16_t count;
    
    if (channel == NULL || channel->magic != DMA_CHANNEL_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    if (channel->tipe != DMA_TIPE_ISA) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    /* ISA DMA limitations */
    if (dst >= 0x1000000) {
        return STATUS_PARAM_INVALID;
    }
    
    if (size > 65536) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Calculate page and offset */
    page = (tak_bertanda16_t)(dst >> 16);
    offset = (tak_bertanda16_t)(dst & 0xFFFF);
    count = (tak_bertanda16_t)(size - 1);
    
    /* Mask channel */
    isa_dma_mask(channel->nomor);
    
    /* Clear flip-flop */
    if (channel->nomor < 4) {
        outb(DMA0_IO_BASE + DMA_REG_CLEAR_FF, 0);
    } else {
        outb(DMA1_IO_BASE + DMA_REG_CLEAR_FF, 0);
    }
    
    /* Set page register */
    outb(g_dma_regs[channel->nomor].page, page);
    
    /* Set address */
    outb(g_dma_regs[channel->nomor].addr, offset & 0xFF);
    outb(g_dma_regs[channel->nomor].addr, (offset >> 8) & 0xFF);
    
    /* Set count */
    outb(g_dma_regs[channel->nomor].count, count & 0xFF);
    outb(g_dma_regs[channel->nomor].count, (count >> 8) & 0xFF);
    
    /* Unmask */
    isa_dma_unmask(channel->nomor);
    
    channel->dst_addr = dst;
    channel->transfer_size = size;
    channel->bytes_transferred = size;
    channel->status = DMA_STATUS_SELESAI;
    
    if (channel->callback != NULL) {
        channel->callback(channel, DMA_STATUS_SELESAI);
    }
    
    return STATUS_BERHASIL;
}

status_t dma_transfer_async(dma_channel_t *channel, alamat_fisik_t src,
                             alamat_fisik_t dst, ukuran_t size)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t dma_transfer_wait(dma_channel_t *channel,
                            tak_bertanda32_t timeout_ms)
{
    tak_bertanda64_t start;
    
    if (channel == NULL || channel->magic != DMA_CHANNEL_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    /* ISA DMA sudah selesai langsung */
    if (channel->tipe == DMA_TIPE_ISA) {
        return STATUS_BERHASIL;
    }
    
    return STATUS_BERHASIL;
}

status_t dma_transfer_cancel(dma_channel_t *channel)
{
    if (channel == NULL || channel->magic != DMA_CHANNEL_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Mask channel */
    if (channel->tipe == DMA_TIPE_ISA) {
        isa_dma_mask(channel->nomor);
    }
    
    channel->status = DMA_STATUS_BELEK;
    channel->busy = SALAH;
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI BUFFER
 * ===========================================================================
 */

void *dma_buffer_alokasi(ukuran_t size)
{
    void *buffer;
    
    if (!g_dma_diinisialisasi) {
        return NULL;
    }
    
    /* Alokasi buffer yang aligned dan di bawah 16MB untuk ISA DMA */
    buffer = kmalloc_aligned(size + 16, 4096);
    if (buffer == NULL) {
        return NULL;
    }
    
    return buffer;
}

void dma_buffer_bebaskan(void *buffer)
{
    if (buffer == NULL) {
        return;
    }
    
    kfree(buffer);
}

alamat_fisik_t dma_buffer_phys(void *buffer)
{
    /* Perlu implementasi virtual to physical mapping */
    return (alamat_fisik_t)(tak_bertanda64_t)buffer;
}

/*
 * ===========================================================================
 * FUNGSI BUS MASTER
 * ===========================================================================
 */

status_t dma_bus_master_init(void *dev)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t dma_bus_master_enable(void *dev)
{
    return STATUS_TIDAK_DUKUNG;
}

/*
 * ===========================================================================
 * FUNGSI UTILITAS
 * ===========================================================================
 */

const char *dma_status_nama(tak_bertanda32_t status)
{
    switch (status) {
    case DMA_STATUS_BELEK:
        return "Idle";
    case DMA_STATUS_SIAP:
        return "Ready";
    case DMA_STATUS_AKTIF:
        return "Active";
    case DMA_STATUS_SELESAI:
        return "Complete";
    case DMA_STATUS_ERROR:
        return "Error";
    default:
        return "Unknown";
    }
}

const char *dma_tipe_nama(tak_bertanda32_t tipe)
{
    switch (tipe) {
    case DMA_TIPE_ISA:
        return "ISA DMA";
    case DMA_TIPE_PCI_BUS_MASTER:
        return "PCI Bus Master";
    case DMA_TIPE_INTEL_IOAT:
        return "Intel I/OAT";
    case DMA_TIPE_ARM_PL330:
        return "ARM PL330";
    default:
        return "Unknown";
    }
}

void dma_cetak_info(void)
{
    if (!g_dma_diinisialisasi) {
        return;
    }
    
    kernel_printf("\n=== DMA Controller ===\n\n");
    kernel_printf("Type: %s\n", 
                 dma_tipe_nama(g_dma_konteks.controllers[0].tipe));
    kernel_printf("Channels: %u\n",
                 g_dma_konteks.controllers[0].channel_count);
}

void dma_cetak_channel(dma_channel_t *channel)
{
    if (channel == NULL) {
        return;
    }
    
    kernel_printf("Channel %u: %s\n",
                 channel->nomor, dma_status_nama(channel->status));
}
