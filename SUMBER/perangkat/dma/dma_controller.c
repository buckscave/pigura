/*
 * PIGURA OS - DMA_CONTROLLER.C
 * =============================
 * Implementasi DMA controller untuk Pigura OS.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi untuk mengelola
 * DMA controller, termasuk inisialisasi dan reset controller.
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

/* Base I/O port untuk DMA controller */
#define DMA_CTRL0_BASE        0x00    /* DMA controller 0 (channel 0-3) */
#define DMA_CTRL1_BASE        0xC0    /* DMA controller 1 (channel 4-7) */

/* Register offsets */
#define DMA_REG_CHAN0_ADDR    0x00
#define DMA_REG_CHAN0_COUNT   0x01
#define DMA_REG_CHAN1_ADDR    0x02
#define DMA_REG_CHAN1_COUNT   0x03
#define DMA_REG_CHAN2_ADDR    0x04
#define DMA_REG_CHAN2_COUNT   0x05
#define DMA_REG_CHAN3_ADDR    0x06
#define DMA_REG_CHAN3_COUNT   0x07

/* Extended registers untuk channel 5-7 (16-bit) */
#define DMA_REG_CHAN4_ADDR    0xC0
#define DMA_REG_CHAN4_COUNT   0xC2
#define DMA_REG_CHAN5_ADDR    0xC4
#define DMA_REG_CHAN5_COUNT   0xC6
#define DMA_REG_CHAN6_ADDR    0xC8
#define DMA_REG_CHAN6_COUNT   0xCA
#define DMA_REG_CHAN7_ADDR    0xCC
#define DMA_REG_CHAN7_COUNT   0xCE

/* Control registers */
#define DMA_REG_COMMAND0      0x08
#define DMA_REG_STATUS0       0x08
#define DMA_REG_REQUEST0      0x09
#define DMA_REG_MASK0         0x0A
#define DMA_REG_MODE0         0x0B
#define DMA_REG_FF0           0x0C
#define DMA_REG_CLEAR0        0x0D
#define DMA_REG_CLR_MASK0     0x0E
#define DMA_REG_MASK_ALL0     0x0F

#define DMA_REG_COMMAND1      0xD0
#define DMA_REG_STATUS1       0xD0
#define DMA_REG_REQUEST1      0xD2
#define DMA_REG_MASK1         0xD4
#define DMA_REG_MODE1         0xD6
#define DMA_REG_FF1           0xD8
#define DMA_REG_CLEAR1        0xDA
#define DMA_REG_CLR_MASK1     0xDC
#define DMA_REG_MASK_ALL1     0xDE

/* Page registers */
#define DMA_PAGE_CHAN0        0x87
#define DMA_PAGE_CHAN1        0x83
#define DMA_PAGE_CHAN2        0x81
#define DMA_PAGE_CHAN3        0x82
#define DMA_PAGE_CHAN5        0x8B
#define DMA_PAGE_CHAN6        0x89
#define DMA_PAGE_CHAN7        0x8A

/* Command flags */
#define DMA_CMD_MEM_TO_MEM    0x01
#define DMA_CMD_ADDR_HOLD     0x02
#define DMA_CMD_CONTROLLER    0x04
#define DMA_CMD_COMPRESSED    0x08
#define DMA_CMD_DREQ_ACTIVE   0x10
#define DMA_CMD_DACK_ACTIVE   0x20
#define DMA_CMD_RO_PRIORITY   0x40
#define DMA_CMD_ENABLE        0x00

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

/* Tabel konfigurasi channel DMA */
static const struct {
    tak_bertanda16_t alamat_port;
    tak_bertanda16_t hitung_port;
    tak_bertanda16_t halaman_port;
    tak_bertanda16_t mode_port;
    tak_bertanda16_t mask_port;
    tak_bertanda16_t clear_ff_port;
} g_dma_chan_config[8] __attribute__((unused)) = {
    /* Channel 0 */
    { DMA_REG_CHAN0_ADDR, DMA_REG_CHAN0_COUNT, DMA_PAGE_CHAN0,
      DMA_REG_MODE0, DMA_REG_MASK0, DMA_REG_FF0 },
    /* Channel 1 */
    { DMA_REG_CHAN1_ADDR, DMA_REG_CHAN1_COUNT, DMA_PAGE_CHAN1,
      DMA_REG_MODE0, DMA_REG_MASK0, DMA_REG_FF0 },
    /* Channel 2 */
    { DMA_REG_CHAN2_ADDR, DMA_REG_CHAN2_COUNT, DMA_PAGE_CHAN2,
      DMA_REG_MODE0, DMA_REG_MASK0, DMA_REG_FF0 },
    /* Channel 3 */
    { DMA_REG_CHAN3_ADDR, DMA_REG_CHAN3_COUNT, DMA_PAGE_CHAN3,
      DMA_REG_MODE0, DMA_REG_MASK0, DMA_REG_FF0 },
    /* Channel 4 (cascade, tidak digunakan untuk transfer) */
    { DMA_REG_CHAN4_ADDR, DMA_REG_CHAN4_COUNT, 0x8F,
      DMA_REG_MODE1, DMA_REG_MASK1, DMA_REG_FF1 },
    /* Channel 5 */
    { DMA_REG_CHAN5_ADDR, DMA_REG_CHAN5_COUNT, DMA_PAGE_CHAN5,
      DMA_REG_MODE1, DMA_REG_MASK1, DMA_REG_FF1 },
    /* Channel 6 */
    { DMA_REG_CHAN6_ADDR, DMA_REG_CHAN6_COUNT, DMA_PAGE_CHAN6,
      DMA_REG_MODE1, DMA_REG_MASK1, DMA_REG_FF1 },
    /* Channel 7 */
    { DMA_REG_CHAN7_ADDR, DMA_REG_CHAN7_COUNT, DMA_PAGE_CHAN7,
      DMA_REG_MODE1, DMA_REG_MASK1, DMA_REG_FF1 }
};

/* Status controller saat ini */
static bool_t g_controller_initialized[4] = { SALAH, SALAH, SALAH, SALAH };

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * dma_controller_validasi - Validasi parameter controller
 *
 * Parameter:
 *   ctrl - Pointer ke controller
 *
 * Return: BENAR jika valid, SALAH jika tidak
 */
static bool_t dma_controller_validasi(dma_controller_t *ctrl)
{
    if (ctrl == NULL) {
        return SALAH;
    }

    if (ctrl->magic != DMA_MAGIC) {
        return SALAH;
    }

    if (ctrl->tipe > DMA_TIPE_ARM_PL330) {
        return SALAH;
    }

    return BENAR;
}

/*
 * isa_controller_init - Inisialisasi ISA DMA controller
 *
 * Parameter:
 *   ctrl - Pointer ke controller
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
static status_t isa_controller_init(dma_controller_t *ctrl)
{
    tak_bertanda32_t i;
    tak_bertanda8_t cmd;

    /* Reset controller 0 (channel 0-3) */
    outb(DMA_CTRL0_BASE + DMA_REG_CLEAR0, 0x00);

    /* Reset controller 1 (channel 4-7) */
    outb(DMA_CTRL1_BASE + DMA_REG_CLEAR1, 0x00);

    /* Set command register - enable controller */
    cmd = 0x00;  /* Enable, normal priority, DREQ active high */
    outb(DMA_CTRL0_BASE + DMA_REG_COMMAND0, cmd);
    outb(DMA_CTRL1_BASE + DMA_REG_COMMAND1, cmd);

    /* Mask semua channel dulu */
    outb(DMA_CTRL0_BASE + DMA_REG_MASK_ALL0, 0x0F);
    outb(DMA_CTRL1_BASE + DMA_REG_MASK_ALL1, 0x0F);

    /* Clear flip-flop */
    outb(DMA_CTRL0_BASE + DMA_REG_FF0, 0x00);
    outb(DMA_CTRL1_BASE + DMA_REG_FF1, 0x00);

    /* Inisialisasi struktur channel */
    for (i = 0; i < DMA_CHANNEL_MAKS; i++) {
        ctrl->channels[i].magic = DMA_CHANNEL_MAGIC;
        ctrl->channels[i].id = i;
        ctrl->channels[i].tipe = DMA_TIPE_ISA;
        ctrl->channels[i].nomor = i;
        ctrl->channels[i].mode = DMA_MODE_TIDAK_ADA;
        ctrl->channels[i].direction = DMA_DIR_MEM_TO_DEV;
        ctrl->channels[i].src_addr = 0;
        ctrl->channels[i].dst_addr = 0;
        ctrl->channels[i].transfer_size = 0;
        ctrl->channels[i].bytes_transferred = 0;
        ctrl->channels[i].buffer = NULL;
        ctrl->channels[i].buffer_size = 0;
        ctrl->channels[i].buffer_phys = 0;
        ctrl->channels[i].status = DMA_STATUS_BELEK;
        ctrl->channels[i].allocated = SALAH;
        ctrl->channels[i].enabled = SALAH;
        ctrl->channels[i].busy = SALAH;
        ctrl->channels[i].callback = NULL;
        ctrl->channels[i].priv = NULL;
        ctrl->channels[i].berikutnya = NULL;
    }

    /* Channel 4 adalah cascade, tidak tersedia untuk transfer */
    ctrl->channels[4].allocated = BENAR;

    return STATUS_BERHASIL;
}

/*
 * pci_bus_master_controller_init - Inisialisasi PCI bus master controller
 *
 * Parameter:
 *   ctrl - Pointer ke controller
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
static status_t pci_bus_master_controller_init(dma_controller_t *ctrl)
{
    tak_bertanda32_t i;

    /* Inisialisasi dasar */
    ctrl->tipe = DMA_TIPE_PCI_BUS_MASTER;
    ctrl->channel_count = DMA_CHANNEL_MAKS;

    for (i = 0; i < DMA_CHANNEL_MAKS; i++) {
        ctrl->channels[i].magic = DMA_CHANNEL_MAGIC;
        ctrl->channels[i].id = i;
        ctrl->channels[i].tipe = DMA_TIPE_PCI_BUS_MASTER;
        ctrl->channels[i].nomor = i;
        ctrl->channels[i].mode = DMA_MODE_TIDAK_ADA;
        ctrl->channels[i].direction = DMA_DIR_MEM_TO_MEM;
        ctrl->channels[i].status = DMA_STATUS_BELEK;
        ctrl->channels[i].allocated = SALAH;
        ctrl->channels[i].enabled = SALAH;
        ctrl->channels[i].busy = SALAH;
    }

    return STATUS_BERHASIL;
}

/*
 * arm_pl330_controller_init - Inisialisasi ARM PL330 DMA controller
 *
 * Parameter:
 *   ctrl - Pointer ke controller
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
static status_t arm_pl330_controller_init(dma_controller_t *ctrl)
{
    tak_bertanda32_t i;

    /* Inisialisasi dasar */
    ctrl->tipe = DMA_TIPE_ARM_PL330;
    ctrl->channel_count = 8;  /* PL330 memiliki 8 channel */

    for (i = 0; i < 8; i++) {
        ctrl->channels[i].magic = DMA_CHANNEL_MAGIC;
        ctrl->channels[i].id = i;
        ctrl->channels[i].tipe = DMA_TIPE_ARM_PL330;
        ctrl->channels[i].nomor = i;
        ctrl->channels[i].mode = DMA_MODE_TIDAK_ADA;
        ctrl->channels[i].direction = DMA_DIR_MEM_TO_MEM;
        ctrl->channels[i].status = DMA_STATUS_BELEK;
        ctrl->channels[i].allocated = SALAH;
        ctrl->channels[i].enabled = SALAH;
        ctrl->channels[i].busy = SALAH;
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * dma_controller_init - Inisialisasi DMA controller
 *
 * Parameter:
 *   tipe - Tipe controller (ISA, PCI, IOAT, ARM PL330)
 *   base_addr - Base address controller
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_controller_init(tak_bertanda32_t tipe, alamat_fisik_t base_addr)
{
    dma_konteks_t *konteks;
    dma_controller_t *ctrl;
    tak_bertanda32_t ctrl_idx;
    status_t hasil;

    /* Dapatkan konteks global */
    konteks = dma_konteks_dapatkan();
    if (konteks == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Cari slot controller yang tersedia */
    ctrl_idx = konteks->controller_count;
    if (ctrl_idx >= 4) {
        return STATUS_PENUH;
    }

    ctrl = &konteks->controllers[ctrl_idx];

    /* Reset struktur controller */
    kernel_memset(ctrl, 0, sizeof(dma_controller_t));

    /* Set magic dan ID */
    ctrl->magic = DMA_MAGIC;
    ctrl->id = ctrl_idx;
    ctrl->tipe = tipe;
    ctrl->base_addr = base_addr;

    /* Inisialisasi berdasarkan tipe */
    switch (tipe) {
    case DMA_TIPE_ISA:
        hasil = isa_controller_init(ctrl);
        break;

    case DMA_TIPE_PCI_BUS_MASTER:
        hasil = pci_bus_master_controller_init(ctrl);
        break;

    case DMA_TIPE_INTEL_IOAT:
        /* Intel I/OAT memerlukan deteksi PCI */
        hasil = STATUS_TIDAK_DUKUNG;
        break;

    case DMA_TIPE_ARM_PL330:
        hasil = arm_pl330_controller_init(ctrl);
        break;

    default:
        hasil = STATUS_PARAM_INVALID;
        break;
    }

    if (hasil != STATUS_BERHASIL) {
        ctrl->magic = 0;
        return hasil;
    }

    /* Set status */
    ctrl->initialized = BENAR;
    ctrl->enabled = BENAR;
    ctrl->channel_count = DMA_CHANNEL_MAKS;

    /* Update konteks */
    konteks->controller_count++;
    g_controller_initialized[ctrl_idx] = BENAR;

    return STATUS_BERHASIL;
}

/*
 * dma_controller_reset - Reset DMA controller
 *
 * Parameter:
 *   ctrl - Pointer ke controller
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_controller_reset(dma_controller_t *ctrl)
{
    tak_bertanda32_t i;

    /* Validasi parameter */
    if (!dma_controller_validasi(ctrl)) {
        return STATUS_PARAM_INVALID;
    }

    /* Reset berdasarkan tipe */
    if (ctrl->tipe == DMA_TIPE_ISA) {
        /* Reset controller 0 */
        outb(DMA_CTRL0_BASE + DMA_REG_CLEAR0, 0x00);

        /* Reset controller 1 */
        outb(DMA_CTRL1_BASE + DMA_REG_CLEAR1, 0x00);

        /* Mask semua channel */
        outb(DMA_CTRL0_BASE + DMA_REG_MASK_ALL0, 0x0F);
        outb(DMA_CTRL1_BASE + DMA_REG_MASK_ALL1, 0x0F);

        /* Clear flip-flop */
        outb(DMA_CTRL0_BASE + DMA_REG_FF0, 0x00);
        outb(DMA_CTRL1_BASE + DMA_REG_FF1, 0x00);
    }

    /* Reset semua channel */
    for (i = 0; i < DMA_CHANNEL_MAKS; i++) {
        if (ctrl->channels[i].allocated) {
            /* Bebaskan buffer jika ada */
            if (ctrl->channels[i].buffer != NULL) {
                dma_buffer_bebaskan(ctrl->channels[i].buffer);
            }
        }

        /* Reset status channel */
        ctrl->channels[i].src_addr = 0;
        ctrl->channels[i].dst_addr = 0;
        ctrl->channels[i].transfer_size = 0;
        ctrl->channels[i].bytes_transferred = 0;
        ctrl->channels[i].buffer = NULL;
        ctrl->channels[i].buffer_size = 0;
        ctrl->channels[i].buffer_phys = 0;
        ctrl->channels[i].status = DMA_STATUS_BELEK;
        ctrl->channels[i].busy = SALAH;
    }

    /* Reset statistik */
    ctrl->total_transfers = 0;
    ctrl->total_bytes = 0;
    ctrl->errors = 0;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI TAMBAHAN
 * ===========================================================================
 */

/*
 * dma_controller_dapatkan - Dapatkan controller berdasarkan indeks
 *
 * Parameter:
 *   indeks - Indeks controller
 *
 * Return: Pointer ke controller atau NULL
 */
dma_controller_t *dma_controller_dapatkan(tak_bertanda32_t indeks)
{
    dma_konteks_t *konteks;

    konteks = dma_konteks_dapatkan();
    if (konteks == NULL) {
        return NULL;
    }

    if (indeks >= konteks->controller_count) {
        return NULL;
    }

    if (konteks->controllers[indeks].magic != DMA_MAGIC) {
        return NULL;
    }

    return &konteks->controllers[indeks];
}

/*
 * dma_controller_enable - Aktifkan controller
 *
 * Parameter:
 *   ctrl - Pointer ke controller
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_controller_enable(dma_controller_t *ctrl)
{
    if (!dma_controller_validasi(ctrl)) {
        return STATUS_PARAM_INVALID;
    }

    if (ctrl->tipe == DMA_TIPE_ISA) {
        /* Enable ISA DMA controller */
        outb(DMA_CTRL0_BASE + DMA_REG_COMMAND0, 0x00);
        outb(DMA_CTRL1_BASE + DMA_REG_COMMAND1, 0x00);
    }

    ctrl->enabled = BENAR;

    return STATUS_BERHASIL;
}

/*
 * dma_controller_disable - Nonaktifkan controller
 *
 * Parameter:
 *   ctrl - Pointer ke controller
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dma_controller_disable(dma_controller_t *ctrl)
{
    if (!dma_controller_validasi(ctrl)) {
        return STATUS_PARAM_INVALID;
    }

    if (ctrl->tipe == DMA_TIPE_ISA) {
        /* Disable ISA DMA controller */
        outb(DMA_CTRL0_BASE + DMA_REG_COMMAND0, 0x04);
        outb(DMA_CTRL1_BASE + DMA_REG_COMMAND1, 0x04);
    }

    ctrl->enabled = SALAH;

    return STATUS_BERHASIL;
}

/*
 * dma_controller_status - Baca status controller
 *
 * Parameter:
 *   ctrl - Pointer ke controller
 *
 * Return: Status register value
 */
tak_bertanda8_t dma_controller_status(dma_controller_t *ctrl)
{
    tak_bertanda8_t status;

    if (ctrl == NULL || ctrl->magic != DMA_MAGIC) {
        return 0xFF;
    }

    if (ctrl->tipe == DMA_TIPE_ISA) {
        /* Baca status register */
        status = inb(DMA_CTRL0_BASE + DMA_REG_STATUS0);
        return status;
    }

    return 0x00;
}

/*
 * dma_controller_statistik - Update statistik controller
 *
 * Parameter:
 *   ctrl - Pointer ke controller
 *   bytes - Jumlah byte yang ditransfer
 *   error - Apakah terjadi error
 */
void dma_controller_statistik(dma_controller_t *ctrl, ukuran_t bytes,
                              bool_t error)
{
    if (ctrl == NULL || ctrl->magic != DMA_MAGIC) {
        return;
    }

    ctrl->total_transfers++;
    ctrl->total_bytes += bytes;

    if (error) {
        ctrl->errors++;
    }
}
