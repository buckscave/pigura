/*
 * PIGURA OS - MMC.C
 * ==================
 * Implementasi driver MMC/eMMC (MultiMediaCard / embedded MMC)
 * untuk perangkat penyimpanan flash embedded.
 *
 * Berkas ini menyediakan fungsi-fungsi untuk mengakses perangkat
 * penyimpanan MMC/eMMC melalui interface SDHCI.
 *
 * Kepatuhan standar:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Mendukung semua arsitektur: x86, x86_64, ARM, ARMv7, ARM64
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../cpu/cpu.h"

/*
 * ===========================================================================
 * KONSTANTA MMC
 * ===========================================================================
 */

#define MMC_VERSI_MAJOR 1
#define MMC_VERSI_MINOR 0

/* Magic number */
#define MMC_MAGIC           0x4D4D4344  /* "MMCD" */
#define MMC_DEVICE_MAGIC    0x4D4D4456  /* "MMDV" */
#define MMC_HOST_MAGIC      0x4D4D4853  /* "MMHS" */

/* Register offset SDHCI */
#define SDHCI_DMA_ADDRESS       0x00
#define SDHCI_BLOCK_SIZE        0x04
#define SDHCI_BLOCK_COUNT       0x06
#define SDHCI_ARGUMENT          0x08
#define SDHCI_TRANSFER_MODE     0x0C
#define SDHCI_COMMAND           0x0E
#define SDHCI_RESPONSE          0x10
#define SDHCI_BUFFER            0x20
#define SDHCI_PRESENT_STATE     0x24
#define SDHCI_HOST_CONTROL      0x28
#define SDHCI_POWER_CONTROL     0x29
#define SDHCI_BLOCK_GAP_CONTROL 0x2A
#define SDHCI_WAKE_UP_CONTROL   0x2B
#define SDHCI_CLOCK_CONTROL     0x2C
#define SDHCI_TIMEOUT_CONTROL   0x2E
#define SDHCI_SOFTWARE_RESET    0x2F
#define SDHCI_INT_STATUS        0x30
#define SDHCI_INT_ENABLE        0x34
#define SDHCI_SIGNAL_ENABLE     0x38
#define SDHCI_ACMD12_ERR        0x3C
#define SDHCI_CAPABILITIES      0x40
#define SDHCI_CAPABILITIES_1    0x44
#define SDHCI_MAX_CURRENT       0x48
#define SDHCI_SLOT_INT_STATUS   0xFC
#define SDHCI_HOST_VERSION      0xFE

/* Transfer mode */
#define SDHCI_TRNS_DMA          0x01
#define SDHCI_TRNS_BLK_CNT_EN   0x02
#define SDHCI_TRNS_AUTO_CMD12   0x04
#define SDHCI_TRNS_READ         0x10
#define SDHCI_TRNS_MULTI        0x20

/* Present state */
#define SDHCI_CMD_INHIBIT       0x00000001
#define SDHCI_DATA_INHIBIT      0x00000002
#define SDHCI_DOING_WRITE       0x00000100
#define SDHCI_DOING_READ        0x00000200
#define SDHCI_SPACE_AVAILABLE   0x00000400
#define SDHCI_DATA_AVAILABLE    0x00000800
#define SDHCI_CARD_PRESENT      0x00010000
#define SDHCI_CARD_STATE_STABLE 0x00020000
#define SDHCI_CARD_DETECT_PIN   0x00040000
#define SDHCI_WRITE_PROTECT     0x00080000

/* Command flags */
#define SDHCI_CMD_RESP_MASK     0x03
#define SDHCI_CMD_CRC           0x08
#define SDHCI_CMD_INDEX         0x10
#define SDHCI_CMD_DATA          0x20
#define SDHCI_CMD_ABORTCMD      0xC0

/* Response tipe */
#define SDHCI_CMD_RESP_NONE     0x00
#define SDHCI_CMD_RESP_LONG     0x01
#define SDHCI_CMD_RESP_SHORT    0x02
#define SDHCI_CMD_RESP_SHORT_BUSY 0x03

/* Interrupt status */
#define SDHCI_INT_RESPONSE      0x00000001
#define SDHCI_INT_DATA_END      0x00000002
#define SDHCI_INT_DMA_END       0x00000008
#define SDHCI_INT_SPACE_AVAIL   0x00000010
#define SDHCI_INT_DATA_AVAIL    0x00000020
#define SDHCI_INT_CARD_INSERT   0x00000040
#define SDHCI_INT_CARD_REMOVE   0x00000080
#define SDHCI_INT_CARD_INT      0x00000100
#define SDHCI_INT_ERROR         0x00008000
#define SDHCI_INT_TIMEOUT       0x00010000
#define SDHCI_INT_CRC           0x00020000
#define SDHCI_INT_END_BIT       0x00040000
#define SDHCI_INT_INDEX         0x00080000
#define SDHCI_INT_DATA_TIMEOUT  0x00100000
#define SDHCI_INT_DATA_CRC      0x00200000
#define SDHCI_INT_DATA_END_BIT  0x00400000
#define SDHCI_INT_BUS_POWER     0x00800000
#define SDHCI_INT_ACMD12ERR     0x01000000
#define SDHCI_INT_ADMA_ERROR    0x02000000

/* MMC commands */
#define MMC_CMD_GO_IDLE_STATE       0
#define MMC_CMD_SEND_OP_COND        1
#define MMC_CMD_ALL_SEND_CID        2
#define MMC_CMD_SET_RELATIVE_ADDR   3
#define MMC_CMD_SET_DSR             4
#define MMC_CMD_SLEEP_AWAKE         5
#define MMC_CMD_SWITCH              6
#define MMC_CMD_SELECT_CARD         7
#define MMC_CMD_SEND_EXT_CSD        8
#define MMC_CMD_SEND_CSD            9
#define MMC_CMD_SEND_CID            10
#define MMC_CMD_STOP_TRANSMISSION   12
#define MMC_CMD_SEND_STATUS         13
#define MMC_CMD_SET_BLOCKLEN        16
#define MMC_CMD_READ_SINGLE_BLOCK   17
#define MMC_CMD_READ_MULTIPLE_BLOCK 18
#define MMC_CMD_WRITE_BLOCK         24
#define MMC_CMD_WRITE_MULTIPLE_BLOCK 25
#define MMC_CMD_ERASE_GROUP_START   35
#define MMC_CMD_ERASE_GROUP_END     36
#define MMC_CMD_ERASE               38
#define MMC_CMD_APP_CMD             55

/* EXT_CSD fields */
#define EXT_CSD_BUS_WIDTH       183
#define EXT_CSD_HS_TIMING       185
#define EXT_CSD_CARD_TYPE       196
#define EXT_CSD_REV             192
#define EXT_CSD_SEC_COUNT       212

/* Card type */
#define MMC_CARD_TYPE_MMC       0x01
#define MMC_CARD_TYPE_EMMC      0x02

/* Jumlah maksimum */
#define MMC_MAX_HOSTS           4
#define MMC_MAX_DEVICES         4
#define MMC_MAX_BLOCK_SIZE      512

/*
 * ===========================================================================
 * STRUKTUR DATA MMC
 * ===========================================================================
 */

/* CSD register (Card-Specific Data) */
typedef struct {
    tak_bertanda8_t data[16];
    tak_bertanda32_t csd_structure;
    tak_bertanda32_t spec_vers;
    tak_bertanda32_t taac;
    tak_bertanda32_t nsac;
    tak_bertanda32_t tran_speed;
    tak_bertanda32_t ccc;
    tak_bertanda32_t read_bl_len;
    tak_bertanda32_t write_bl_len;
    tak_bertanda64_t capacity;
} mmc_csd_t;

/* CID register (Card Identification) */
typedef struct {
    tak_bertanda8_t data[16];
    tak_bertanda32_t mid;           /* Manufacturer ID */
    char pnm[7];                    /* Product Name */
    tak_bertanda32_t prv;           /* Product Revision */
    tak_bertanda32_t psn;           /* Product Serial Number */
} mmc_cid_t;

/* Perangkat MMC */
typedef struct {
    tak_bertanda32_t magic;
    tak_bertanda32_t id;
    bool_t valid;
    bool_t present;
    bool_t initialized;
    tak_bertanda8_t card_type;
    tak_bertanda16_t rca;           /* Relative Card Address */

    /* Kapasitas */
    tak_bertanda64_t capacity;
    tak_bertanda64_t size_bytes;
    ukuran_t block_size;
    tak_bertanda32_t erase_group_size;

    /* Identifikasi */
    mmc_csd_t csd;
    mmc_cid_t cid;
    char product_name[8];
    tak_bertanda32_t manufacturer;
    tak_bertanda32_t oem_id;

    /* Speed */
    tak_bertanda32_t tran_speed;
    tak_bertanda32_t clock;

    /* Statistik */
    tak_bertanda64_t read_ops;
    tak_bertanda64_t write_ops;
    tak_bertanda64_t errors;
} mmc_device_t;

/* MMC Host Controller */
typedef struct {
    tak_bertanda32_t magic;
    tak_bertanda32_t id;
    volatile tak_bertanda32_t *io_base;
    tak_bertanda32_t irq;

    /* Capabilities */
    tak_bertanda64_t capabilities;
    tak_bertanda32_t max_clock;
    tak_bertanda32_t min_clock;
    tak_bertanda32_t voltage;

    /* Current settings */
    tak_bertanda32_t clock;
    tak_bertanda32_t bus_width;
    tak_bertanda8_t power_mode;

    /* Device */
    mmc_device_t device;

    /* Status */
    bool_t initialized;
    bool_t card_inserted;
} mmc_host_t;

/* Konteks MMC */
typedef struct {
    tak_bertanda32_t magic;
    bool_t diinisialisasi;
    mmc_host_t host[MMC_MAX_HOSTS];
    tak_bertanda32_t jumlah_host;
} mmc_konteks_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

static mmc_konteks_t g_mmc_konteks;
static bool_t g_mmc_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * FUNGSI I/O LOW-LEVEL
 * ===========================================================================
 */

/*
 * mmc_readl - Baca register 32-bit
 */
static inline tak_bertanda32_t mmc_readl(mmc_host_t *host,
                                         tak_bertanda32_t offset)
{
    volatile tak_bertanda32_t *addr;

    if (host == NULL || host->io_base == NULL) {
        return 0;
    }

    addr = (volatile tak_bertanda32_t *)
           ((tak_bertanda8_t *)host->io_base + offset);
    return *addr;
}

/*
 * mmc_writel - Tulis register 32-bit
 */
static inline void mmc_writel(mmc_host_t *host, tak_bertanda32_t offset,
                              tak_bertanda32_t nilai)
{
    volatile tak_bertanda32_t *addr;

    if (host == NULL || host->io_base == NULL) {
        return;
    }

    addr = (volatile tak_bertanda32_t *)
           ((tak_bertanda8_t *)host->io_base + offset);
    *addr = nilai;
}

/*
 * mmc_readw - Baca register 16-bit
 */
static inline tak_bertanda16_t mmc_readw(mmc_host_t *host,
                                         tak_bertanda32_t offset)
{
    return (tak_bertanda16_t)(mmc_readl(host, offset) & 0xFFFF);
}

/*
 * mmc_writew - Tulis register 16-bit
 */
static inline void mmc_writew(mmc_host_t *host, tak_bertanda32_t offset,
                              tak_bertanda16_t nilai)
{
    tak_bertanda32_t val = mmc_readl(host, offset);
    val = (val & 0xFFFF0000) | nilai;
    mmc_writel(host, offset, val);
}

/*
 * mmc_readb - Baca register 8-bit
 */
static inline tak_bertanda8_t mmc_readb(mmc_host_t *host,
                                        tak_bertanda32_t offset)
{
    return (tak_bertanda8_t)(mmc_readl(host, offset) & 0xFF);
}

/*
 * mmc_writeb - Tulis register 8-bit
 */
static inline void mmc_writeb(mmc_host_t *host, tak_bertanda32_t offset,
                              tak_bertanda8_t nilai)
{
    tak_bertanda32_t val = mmc_readl(host, offset);
    val = (val & 0xFFFFFF00) | nilai;
    mmc_writel(host, offset, val);
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * mmc_wait_cmd_complete - Tunggu command selesai
 */
static status_t mmc_wait_cmd_complete(mmc_host_t *host)
{
    tak_bertanda32_t status;
    tak_bertanda32_t timeout;

    if (host == NULL) {
        return STATUS_PARAM_NULL;
    }

    timeout = 1000;
    while (timeout > 0) {
        status = mmc_readl(host, SDHCI_INT_STATUS);

        if (status & SDHCI_INT_RESPONSE) {
            mmc_writel(host, SDHCI_INT_STATUS, SDHCI_INT_RESPONSE);
            return STATUS_BERHASIL;
        }

        if (status & SDHCI_INT_ERROR) {
            mmc_writel(host, SDHCI_INT_STATUS, SDHCI_INT_ERROR);
            return STATUS_IO_ERROR;
        }

        cpu_delay_ms(1);
        timeout--;
    }

    return STATUS_TIMEOUT;
}

/*
 * mmc_send_cmd - Kirim perintah MMC
 */
static status_t mmc_send_cmd(mmc_host_t *host, tak_bertanda16_t cmd,
                             tak_bertanda32_t arg, tak_bertanda16_t flags,
                             tak_bertanda32_t *response)
{
    tak_bertanda32_t mask;
    tak_bertanda32_t timeout;
    status_t hasil;

    if (host == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Tunggu command inhibit clear */
    timeout = 1000;
    while (timeout > 0) {
        mask = mmc_readl(host, SDHCI_PRESENT_STATE);
        if ((mask & SDHCI_CMD_INHIBIT) == 0) {
            break;
        }
        cpu_delay_ms(1);
        timeout--;
    }

    if (timeout == 0) {
        return STATUS_TIMEOUT;
    }

    /* Clear interrupt status */
    mmc_writel(host, SDHCI_INT_STATUS, 0xFFFFFFFF);

    /* Set argument */
    mmc_writel(host, SDHCI_ARGUMENT, arg);

    /* Set transfer mode */
    mmc_writew(host, SDHCI_TRANSFER_MODE, 0);

    /* Set command */
    mmc_writew(host, SDHCI_COMMAND,
               ((cmd << 8) & 0xFF00) | (flags & 0xFF));

    /* Tunggu response */
    hasil = mmc_wait_cmd_complete(host);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    /* Baca response */
    if (response != NULL && (flags & SDHCI_CMD_RESP_MASK)) {
        *response = mmc_readl(host, SDHCI_RESPONSE);
    }

    return STATUS_BERHASIL;
}

/*
 * mmc_go_idle - Reset card ke idle state
 */
static status_t mmc_go_idle(mmc_host_t *host)
{
    return mmc_send_cmd(host, MMC_CMD_GO_IDLE_STATE, 0,
                        SDHCI_CMD_RESP_NONE, NULL);
}

/*
 * mmc_send_op_cond - Kirim OP_COND
 */
static status_t mmc_send_op_cond(mmc_host_t *host, bool_t is_mmc)
{
    tak_bertanda32_t arg;
    tak_bertanda32_t response;
    tak_bertanda32_t timeout;
    status_t hasil;

    if (host == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Argumen untuk MMC: voltages + capacity */
    arg = 0x40FF8000;  /* 3.2V - 3.4V, high capacity */

    timeout = 100;
    while (timeout > 0) {
        hasil = mmc_send_cmd(host, MMC_CMD_SEND_OP_COND, arg,
                            SDHCI_CMD_RESP_SHORT, &response);
        if (hasil != STATUS_BERHASIL) {
            return hasil;
        }

        /* Cek apakah card ready */
        if (response & 0x80000000) {
            host->device.card_type = MMC_CARD_TYPE_MMC;
            return STATUS_BERHASIL;
        }

        cpu_delay_ms(10);
        timeout--;
    }

    return STATUS_TIMEOUT;
}

/*
 * mmc_all_send_cid - Dapatkan CID
 */
static status_t mmc_all_send_cid(mmc_host_t *host)
{
    tak_bertanda32_t response[4];
    tak_bertanda32_t i;
    status_t hasil;

    if (host == NULL) {
        return STATUS_PARAM_NULL;
    }

    hasil = mmc_send_cmd(host, MMC_CMD_ALL_SEND_CID, 0,
                        SDHCI_CMD_RESP_LONG | SDHCI_CMD_CRC,
                        &response[0]);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    /* Baca seluruh response (128-bit) */
    for (i = 0; i < 4; i++) {
        response[i] = mmc_readl(host, SDHCI_RESPONSE + i * 4);
    }

    /* Parse CID */
    host->device.cid.mid = (response[0] >> 24) & 0xFF;
    host->device.cid.psn = response[2];
    for (i = 0; i < 6; i++) {
        host->device.cid.pnm[i] = (char)((response[1] >> (24 - i * 8)) & 0xFF);
    }
    host->device.cid.pnm[6] = '\0';

    return STATUS_BERHASIL;
}

/*
 * mmc_set_relative_addr - Set relative card address
 */
static status_t mmc_set_relative_addr(mmc_host_t *host)
{
    tak_bertanda32_t response;
    status_t hasil;

    if (host == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Untuk MMC, RCA adalah 0x0001 */
    host->device.rca = 0x0001;

    hasil = mmc_send_cmd(host, MMC_CMD_SET_RELATIVE_ADDR,
                        host->device.rca << 16,
                        SDHCI_CMD_RESP_SHORT, &response);
    return hasil;
}

/*
 * mmc_send_csd - Dapatkan CSD
 */
static status_t mmc_send_csd(mmc_host_t *host)
{
    tak_bertanda32_t response[4];
    tak_bertanda32_t i;
    status_t hasil;

    if (host == NULL) {
        return STATUS_PARAM_NULL;
    }

    hasil = mmc_send_cmd(host, MMC_CMD_SEND_CSD,
                        host->device.rca << 16,
                        SDHCI_CMD_RESP_LONG | SDHCI_CMD_CRC,
                        &response[0]);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    /* Baca seluruh response */
    for (i = 0; i < 4; i++) {
        response[i] = mmc_readl(host, SDHCI_RESPONSE + i * 4);
    }

    /* Parse CSD (simplified) */
    /* CSD structure version */
    host->device.csd.csd_structure = (response[0] >> 30) & 0x3;

    /* Untuk eMMC, baca EXT_CSD untuk kapasitas yang benar */
    host->device.capacity = 0;

    return STATUS_BERHASIL;
}

/*
 * mmc_select_card - Pilih card
 */
static status_t mmc_select_card(mmc_host_t *host)
{
    tak_bertanda32_t response;
    status_t hasil;

    if (host == NULL) {
        return STATUS_PARAM_NULL;
    }

    hasil = mmc_send_cmd(host, MMC_CMD_SELECT_CARD,
                        host->device.rca << 16,
                        SDHCI_CMD_RESP_SHORT_BUSY, &response);
    return hasil;
}

/*
 * mmc_set_blocklen - Set block length
 */
static status_t mmc_set_blocklen(mmc_host_t *host, ukuran_t blocklen)
{
    tak_bertanda32_t response;
    status_t hasil;

    if (host == NULL) {
        return STATUS_PARAM_NULL;
    }

    hasil = mmc_send_cmd(host, MMC_CMD_SET_BLOCKLEN, blocklen,
                        SDHCI_CMD_RESP_SHORT, &response);
    return hasil;
}

/*
 * mmc_read_single_block - Baca single block
 */
static status_t mmc_read_single_block(mmc_host_t *host,
                                      tak_bertanda32_t block,
                                      void *buffer)
{
    tak_bertanda32_t status;
    tak_bertanda32_t timeout;
    tak_bertanda32_t i;
    tak_bertanda32_t *p;
    status_t hasil;

    if (host == NULL || buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Kirim perintah read */
    hasil = mmc_send_cmd(host, MMC_CMD_READ_SINGLE_BLOCK, block,
                        SDHCI_CMD_RESP_SHORT | SDHCI_CMD_DATA, NULL);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    /* Tunggu data available */
    timeout = 1000;
    while (timeout > 0) {
        status = mmc_readl(host, SDHCI_PRESENT_STATE);
        if (status & SDHCI_DATA_AVAILABLE) {
            break;
        }
        cpu_delay_ms(1);
        timeout--;
    }

    if (timeout == 0) {
        return STATUS_TIMEOUT;
    }

    /* Baca data dari buffer */
    p = (tak_bertanda32_t *)buffer;
    for (i = 0; i < 128; i++) {
        p[i] = mmc_readl(host, SDHCI_BUFFER);
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * mmc_init - Inisialisasi driver MMC
 */
status_t mmc_init(void)
{
    mmc_host_t *host;
    tak_bertanda32_t i;
    status_t hasil;

    if (g_mmc_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }

    kernel_memset(&g_mmc_konteks, 0, sizeof(mmc_konteks_t));

    /* Setup host controller */
    /* TODO: Deteksi SDHCI controller dari PCI/Device Tree */
    /* Untuk sekarang, setup satu host default */

    host = &g_mmc_konteks.host[0];
    host->magic = MMC_HOST_MAGIC;
    host->id = 0;

    /* TODO: Dapatkan IO base dari PCI/DT */
    /* host->io_base = ...; */
    /* host->irq = ...; */

    /* Read capabilities */
    if (host->io_base != NULL) {
        host->capabilities = ((tak_bertanda64_t)
                             mmc_readl(host, SDHCI_CAPABILITIES_1) << 32) |
                             mmc_readl(host, SDHCI_CAPABILITIES);

        /* Reset controller */
        mmc_writeb(host, SDHCI_SOFTWARE_RESET, 0x03);
        cpu_delay_ms(10);

        /* Enable interrupts */
        mmc_writel(host, SDHCI_INT_ENABLE, 0xFFFFFFFF);
        mmc_writel(host, SDHCI_SIGNAL_ENABLE, 0xFFFFFFFF);

        /* Set power */
        mmc_writeb(host, SDHCI_POWER_CONTROL, 0x0F);

        /* Set clock */
        mmc_writew(host, SDHCI_CLOCK_CONTROL, 0x0001);
        mmc_writew(host, SDHCI_CLOCK_CONTROL, 0x0003);
        host->clock = 400000;

        host->initialized = BENAR;
    }

    /* Inisialisasi card */
    host->device.magic = MMC_DEVICE_MAGIC;
    host->device.id = 0;
    host->device.block_size = MMC_MAX_BLOCK_SIZE;

    if (host->io_base != NULL) {
        hasil = mmc_go_idle(host);
        if (hasil == STATUS_BERHASIL) {
            hasil = mmc_send_op_cond(host, BENAR);
        }

        if (hasil == STATUS_BERHASIL) {
            hasil = mmc_all_send_cid(host);
        }

        if (hasil == STATUS_BERHASIL) {
            hasil = mmc_set_relative_addr(host);
        }

        if (hasil == STATUS_BERHASIL) {
            hasil = mmc_send_csd(host);
        }

        if (hasil == STATUS_BERHASIL) {
            hasil = mmc_select_card(host);
        }

        if (hasil == STATUS_BERHASIL) {
            hasil = mmc_set_blocklen(host, host->device.block_size);
        }

        if (hasil == STATUS_BERHASIL) {
            host->device.present = BENAR;
            host->device.valid = BENAR;
            host->device.initialized = BENAR;
            host->card_inserted = BENAR;

            /* Copy product name */
            snprintf(host->device.product_name,
                    sizeof(host->device.product_name),
                    "eMMC %s", host->device.cid.pnm);
        }
    }

    g_mmc_konteks.magic = MMC_MAGIC;
    g_mmc_konteks.jumlah_host = 1;
    g_mmc_konteks.diinisialisasi = BENAR;
    g_mmc_diinisialisasi = BENAR;

    return STATUS_BERHASIL;
}

/*
 * mmc_shutdown - Matikan driver MMC
 */
status_t mmc_shutdown(void)
{
    tak_bertanda32_t i;

    if (!g_mmc_diinisialisasi) {
        return STATUS_BERHASIL;
    }

    for (i = 0; i < MMC_MAX_HOSTS; i++) {
        mmc_host_t *host = &g_mmc_konteks.host[i];

        if (host->io_base != NULL) {
            /* Deselect card */
            mmc_send_cmd(host, MMC_CMD_SELECT_CARD, 0,
                        SDHCI_CMD_RESP_NONE, NULL);

            /* Reset controller */
            mmc_writeb(host, SDHCI_SOFTWARE_RESET, 0x03);
        }
    }

    g_mmc_konteks.magic = 0;
    g_mmc_konteks.diinisialisasi = SALAH;
    g_mmc_diinisialisasi = SALAH;

    return STATUS_BERHASIL;
}

/*
 * mmc_baca - Baca blok dari perangkat MMC
 *
 * Parameter:
 *   dev_id - ID perangkat
 *   block - Alamat blok
 *   buffer - Buffer output
 *   count - Jumlah blok
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t mmc_baca(tak_bertanda32_t dev_id, tak_bertanda64_t block,
                  void *buffer, ukuran_t count)
{
    mmc_host_t *host;
    ukuran_t i;
    status_t hasil;

    if (!g_mmc_diinisialisasi) {
        return STATUS_GAGAL;
    }

    if (buffer == NULL || count == 0) {
        return STATUS_PARAM_INVALID;
    }

    /* Cari host dengan device ID */
    host = NULL;
    for (i = 0; i < MMC_MAX_HOSTS; i++) {
        if (g_mmc_konteks.host[i].device.id == dev_id &&
            g_mmc_konteks.host[i].device.valid) {
            host = &g_mmc_konteks.host[i];
            break;
        }
    }

    if (host == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Baca blok per blok */
    for (i = 0; i < count; i++) {
        hasil = mmc_read_single_block(host,
                                     (tak_bertanda32_t)(block + i),
                                     (tak_bertanda8_t *)buffer +
                                     i * host->device.block_size);
        if (hasil != STATUS_BERHASIL) {
            host->device.errors++;
            return hasil;
        }
    }

    host->device.read_ops++;

    return STATUS_BERHASIL;
}

/*
 * mmc_tulis - Tulis blok ke perangkat MMC
 *
 * Parameter:
 *   dev_id - ID perangkat
 *   block - Alamat blok
 *   buffer - Buffer input
 *   count - Jumlah blok
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t mmc_tulis(tak_bertanda32_t dev_id, tak_bertanda64_t block,
                   const void *buffer, ukuran_t count)
{
    mmc_host_t *host;
    tak_bertanda32_t response;
    ukuran_t i;
    status_t hasil;

    if (!g_mmc_diinisialisasi) {
        return STATUS_GAGAL;
    }

    if (buffer == NULL || count == 0) {
        return STATUS_PARAM_INVALID;
    }

    /* Cari host */
    host = NULL;
    for (i = 0; i < MMC_MAX_HOSTS; i++) {
        if (g_mmc_konteks.host[i].device.id == dev_id &&
            g_mmc_konteks.host[i].device.valid) {
            host = &g_mmc_konteks.host[i];
            break;
        }
    }

    if (host == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Tulis blok per blok */
    for (i = 0; i < count; i++) {
        /* TODO: Implement write */
        hasil = mmc_send_cmd(host, MMC_CMD_WRITE_BLOCK,
                            (tak_bertanda32_t)(block + i),
                            SDHCI_CMD_RESP_SHORT | SDHCI_CMD_DATA,
                            &response);
        if (hasil != STATUS_BERHASIL) {
            host->device.errors++;
            return hasil;
        }
    }

    host->device.write_ops++;

    return STATUS_BERHASIL;
}

/*
 * mmc_cetak_info - Cetak informasi perangkat MMC
 */
void mmc_cetak_info(void)
{
    tak_bertanda32_t i;

    if (!g_mmc_diinisialisasi) {
        return;
    }

    kernel_printf("\n=== Perangkat MMC/eMMC ===\n\n");

    for (i = 0; i < MMC_MAX_HOSTS; i++) {
        mmc_host_t *host = &g_mmc_konteks.host[i];
        mmc_device_t *dev = &host->device;

        if (dev->valid && dev->present) {
            kernel_printf("mmc%d: ", dev->id);
            kernel_printf("%s\n", dev->product_name);
            kernel_printf("  Kapasitas: %lu MB\n",
                         dev->capacity / (1024 * 1024));
            kernel_printf("  Block Size: %u byte\n", dev->block_size);
            kernel_printf("  Manufacturer ID: %u\n", dev->cid.mid);
            kernel_printf("\n");
        }
    }
}
