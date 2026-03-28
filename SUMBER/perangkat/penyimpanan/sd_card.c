/*
 * PIGURA OS - SD_CARD.C
 * ======================
 * Implementasi driver SD Card (Secure Digital)
 * untuk perangkat penyimpanan SD/SDHC/SDXC.
 *
 * Berkas ini menyediakan fungsi-fungsi untuk mengakses kartu SD
 * melalui interface SDHCI.
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
 * KONSTANTA SD CARD
 * ===========================================================================
 */

#define SD_CARD_VERSI_MAJOR 1
#define SD_CARD_VERSI_MINOR 0

/* Magic number */
#define SD_MAGIC            0x53444352  /* "SDCR" */
#define SD_DEVICE_MAGIC     0x53444456  /* "SDDV" */
#define SD_HOST_MAGIC       0x53444853  /* "SDHS" */

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
#define SDHCI_CAPABILITIES      0x40
#define SDHCI_CAPABILITIES_1    0x44

/* Command flags */
#define SDHCI_CMD_RESP_NONE     0x00
#define SDHCI_CMD_RESP_LONG     0x01
#define SDHCI_CMD_RESP_SHORT    0x02
#define SDHCI_CMD_CRC           0x08
#define SDHCI_CMD_INDEX         0x10
#define SDHCI_CMD_DATA          0x20

/* SD commands */
#define SD_CMD_GO_IDLE_STATE        0
#define SD_CMD_ALL_SEND_CID         2
#define SD_CMD_SEND_RELATIVE_ADDR   3
#define SD_CMD_SET_DSR              4
#define SD_CMD_SELECT_CARD          7
#define SD_CMD_SEND_IF_COND         8
#define SD_CMD_SEND_CSD             9
#define SD_CMD_SEND_CID             10
#define SD_CMD_STOP_TRANSMISSION    12
#define SD_CMD_SEND_STATUS          13
#define SD_CMD_SET_BLOCKLEN         16
#define SD_CMD_READ_SINGLE_BLOCK    17
#define SD_CMD_READ_MULTIPLE_BLOCK  18
#define SD_CMD_WRITE_BLOCK          24
#define SD_CMD_WRITE_MULTIPLE_BLOCK 25
#define SD_CMD_PROGRAM_CSD          27
#define SD_CMD_ERASE_WR_BLK_START   32
#define SD_CMD_ERASE_WR_BLK_END     33
#define SD_CMD_ERASE                38
#define SD_CMD_APP_CMD              55

/* SD Application commands (ACMD) */
#define SD_ACMD_SET_BUS_WIDTH       6
#define SD_ACMD_SD_STATUS           13
#define SD_ACMD_SEND_NUM_WR_BLOCKS  22
#define SD_ACMD_SET_WR_BLK_ERASE_COUNT 23
#define SD_ACMD_SD_SEND_OP_COND     41
#define SD_ACMD_SET_CLR_CARD_DETECT 42
#define SD_ACMD_SEND_SCR            51

/* OCR (Operation Condition Register) bits */
#define SD_OCR_VDD_170_195      0x00000080
#define SD_OCR_VDD_20_21        0x00000100
#define SD_OCR_VDD_21_22        0x00000200
#define SD_OCR_VDD_22_23        0x00000400
#define SD_OCR_VDD_23_24        0x00000800
#define SD_OCR_VDD_24_25        0x00001000
#define SD_OCR_VDD_25_26        0x00002000
#define SD_OCR_VDD_26_27        0x00004000
#define SD_OCR_VDD_27_28        0x00008000
#define SD_OCR_VDD_28_29        0x00010000
#define SD_OCR_VDD_29_30        0x00020000
#define SD_OCR_VDD_30_31        0x00040000
#define SD_OCR_VDD_31_32        0x00080000
#define SD_OCR_VDD_32_33        0x00100000
#define SD_OCR_VDD_33_34        0x00200000
#define SD_OCR_VDD_34_35        0x00400000
#define SD_OCR_VDD_35_36        0x00800000
#define SD_OCR_S18R             0x01000000  /* Switch to 1.8V */
#define SD_OCR_XPC              0x02000000  /* SDXC Power Control */
#define SD_OCR_CCS              0x40000000  /* Card Capacity Status */
#define SD_OCR_BUSY             0x80000000  /* Card Power Up Status */

/* Card type */
#define SD_CARD_TYPE_SDSC       0x01
#define SD_CARD_TYPE_SDHC       0x02
#define SD_CARD_TYPE_SDXC       0x03

/* Transfer speed */
#define SD_SPEED_DS            0x00  /* Default Speed: 12.5 MB/s */
#define SD_SPEED_HS            0x01  /* High Speed: 25 MB/s */
#define SD_SPEED_SDR12         0x02  /* SDR12: 12.5 MB/s */
#define SD_SPEED_SDR25         0x03  /* SDR25: 25 MB/s */
#define SD_SPEED_SDR50         0x04  /* SDR50: 50 MB/s */
#define SD_SPEED_SDR104        0x05  /* SDR104: 104 MB/s */
#define SD_SPEED_DDR50         0x06  /* DDR50: 50 MB/s */

/* Jumlah maksimum */
#define SD_MAX_HOSTS           4
#define SD_MAX_BLOCK_SIZE      512

/*
 * ===========================================================================
 * STRUKTUR DATA SD CARD
 * ===========================================================================
 */

/* CSD register */
typedef struct {
    tak_bertanda8_t data[16];
    tak_bertanda8_t csd_structure;
    tak_bertanda8_t taac;
    tak_bertanda8_t nsac;
    tak_bertanda8_t tran_speed;
    tak_bertanda16_t ccc;
    tak_bertanda8_t read_bl_len;
    tak_bertanda8_t write_bl_len;
    tak_bertanda64_t capacity;
} sd_csd_t;

/* CID register */
typedef struct {
    tak_bertanda8_t data[16];
    tak_bertanda8_t mid;            /* Manufacturer ID */
    char oid[3];                    /* OEM/Application ID */
    char pnm[6];                    /* Product Name */
    tak_bertanda8_t prv;            /* Product Revision */
    tak_bertanda32_t psn;           /* Product Serial Number */
    tak_bertanda16_t mdt;           /* Manufacturing Date */
} sd_cid_t;

/* SCR (SD Configuration Register) */
typedef struct {
    tak_bertanda8_t data[8];
    tak_bertanda8_t scr_structure;
    tak_bertanda8_t sd_spec;
    tak_bertanda8_t bus_width;
    tak_bertanda8_t sd_spec3;
    tak_bertanda8_t sd_spec4;
    tak_bertanda8_t sd_specx;
} sd_scr_t;

/* Perangkat SD Card */
typedef struct {
    tak_bertanda32_t magic;
    tak_bertanda32_t id;
    bool_t valid;
    bool_t present;
    bool_t initialized;
    tak_bertanda8_t card_type;
    tak_bertanda16_t rca;

    /* Kapasitas */
    tak_bertanda64_t capacity;
    tak_bertanda64_t size_bytes;
    ukuran_t block_size;

    /* Registers */
    sd_csd_t csd;
    sd_cid_t cid;
    sd_scr_t scr;
    tak_bertanda32_t ocr;

    /* Identifikasi */
    char product_name[7];
    char oem_id[3];
    tak_bertanda8_t manufacturer;
    tak_bertanda8_t revision;

    /* Speed */
    tak_bertanda32_t tran_speed;
    tak_bertanda32_t clock;
    tak_bertanda8_t speed_mode;

    /* Statistik */
    tak_bertanda64_t read_ops;
    tak_bertanda64_t write_ops;
    tak_bertanda64_t errors;
} sd_device_t;

/* SD Host Controller */
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
    sd_device_t device;

    /* Status */
    bool_t initialized;
    bool_t card_inserted;
} sd_host_t;

/* Konteks SD Card */
typedef struct {
    tak_bertanda32_t magic;
    bool_t diinisialisasi;
    sd_host_t host[SD_MAX_HOSTS];
    tak_bertanda32_t jumlah_host;
} sd_konteks_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

static sd_konteks_t g_sd_konteks;
static bool_t g_sd_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * FUNGSI I/O LOW-LEVEL
 * ===========================================================================
 */

/*
 * sd_readl - Baca register 32-bit
 */
static inline tak_bertanda32_t sd_readl(sd_host_t *host,
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
 * sd_writel - Tulis register 32-bit
 */
static inline void sd_writel(sd_host_t *host, tak_bertanda32_t offset,
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
 * sd_readw - Baca register 16-bit
 */
static inline tak_bertanda16_t sd_readw(sd_host_t *host,
                                        tak_bertanda32_t offset)
{
    return (tak_bertanda16_t)(sd_readl(host, offset) & 0xFFFF);
}

/*
 * sd_writew - Tulis register 16-bit
 */
static inline void sd_writew(sd_host_t *host, tak_bertanda32_t offset,
                             tak_bertanda16_t nilai)
{
    tak_bertanda32_t val = sd_readl(host, offset);
    val = (val & 0xFFFF0000) | nilai;
    sd_writel(host, offset, val);
}

/*
 * sd_readb - Baca register 8-bit
 */
static inline tak_bertanda8_t sd_readb(sd_host_t *host,
                                       tak_bertanda32_t offset)
{
    return (tak_bertanda8_t)(sd_readl(host, offset) & 0xFF);
}

/*
 * sd_writeb - Tulis register 8-bit
 */
static inline void sd_writeb(sd_host_t *host, tak_bertanda32_t offset,
                             tak_bertanda8_t nilai)
{
    tak_bertanda32_t val = sd_readl(host, offset);
    val = (val & 0xFFFFFF00) | nilai;
    sd_writel(host, offset, val);
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * sd_wait_cmd_complete - Tunggu command selesai
 */
static status_t sd_wait_cmd_complete(sd_host_t *host)
{
    tak_bertanda32_t status;
    tak_bertanda32_t timeout;

    if (host == NULL) {
        return STATUS_PARAM_NULL;
    }

    timeout = 1000;
    while (timeout > 0) {
        status = sd_readl(host, SDHCI_INT_STATUS);

        if (status & 0x00000001) {  /* Response */
            sd_writel(host, SDHCI_INT_STATUS, 0x00000001);
            return STATUS_BERHASIL;
        }

        if (status & 0x00008000) {  /* Error */
            sd_writel(host, SDHCI_INT_STATUS, 0x00008000);
            return STATUS_IO_ERROR;
        }

        cpu_delay_ms(1);
        timeout--;
    }

    return STATUS_TIMEOUT;
}

/*
 * sd_send_cmd - Kirim perintah SD
 */
static status_t sd_send_cmd(sd_host_t *host, tak_bertanda16_t cmd,
                            tak_bertanda32_t arg, tak_bertanda16_t flags,
                            tak_bertanda32_t *response)
{
    tak_bertanda32_t mask;
    tak_bertanda32_t timeout;
    status_t hasil;

    if (host == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Tunggu command inhibit */
    timeout = 1000;
    while (timeout > 0) {
        mask = sd_readl(host, SDHCI_PRESENT_STATE);
        if ((mask & 0x00000001) == 0) {
            break;
        }
        cpu_delay_ms(1);
        timeout--;
    }

    if (timeout == 0) {
        return STATUS_TIMEOUT;
    }

    /* Clear interrupt status */
    sd_writel(host, SDHCI_INT_STATUS, 0xFFFFFFFF);

    /* Set argument */
    sd_writel(host, SDHCI_ARGUMENT, arg);

    /* Set transfer mode */
    sd_writew(host, SDHCI_TRANSFER_MODE, 0);

    /* Set command */
    sd_writew(host, SDHCI_COMMAND,
             ((cmd << 8) & 0xFF00) | (flags & 0xFF));

    /* Tunggu response */
    hasil = sd_wait_cmd_complete(host);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    /* Baca response */
    if (response != NULL && (flags & 0x03)) {
        *response = sd_readl(host, SDHCI_RESPONSE);
    }

    return STATUS_BERHASIL;
}

/*
 * sd_send_app_cmd - Kirim application command
 */
static status_t sd_send_app_cmd(sd_host_t *host, tak_bertanda16_t cmd,
                                tak_bertanda32_t arg, tak_bertanda16_t flags,
                                tak_bertanda32_t *response)
{
    status_t hasil;
    tak_bertanda32_t rca;

    if (host == NULL) {
        return STATUS_PARAM_NULL;
    }

    rca = host->device.rca << 16;

    /* Kirim CMD55 (APP_CMD) */
    hasil = sd_send_cmd(host, SD_CMD_APP_CMD, rca,
                        SDHCI_CMD_RESP_SHORT, NULL);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    /* Kirim ACMD */
    return sd_send_cmd(host, cmd, arg, flags, response);
}

/*
 * sd_go_idle - Reset card ke idle state
 */
static status_t sd_go_idle(sd_host_t *host)
{
    return sd_send_cmd(host, SD_CMD_GO_IDLE_STATE, 0,
                       SDHCI_CMD_RESP_NONE, NULL);
}

/*
 * sd_send_if_cond - Kirim interface condition (CMD8)
 */
static status_t sd_send_if_cond(sd_host_t *host)
{
    tak_bertanda32_t response;
    status_t hasil;

    /* VHS = 0x01 (2.7-3.6V), check pattern = 0xAA */
    hasil = sd_send_cmd(host, SD_CMD_SEND_IF_COND, 0x000001AA,
                       SDHCI_CMD_RESP_SHORT, &response);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    /* Verifikasi response */
    if ((response & 0x00000FFF) != 0x000001AA) {
        return STATUS_IO_ERROR;
    }

    return STATUS_BERHASIL;
}

/*
 * sd_send_op_cond - Kirim SD_SEND_OP_COND (ACMD41)
 */
static status_t sd_send_op_cond(sd_host_t *host)
{
    tak_bertanda32_t response;
    tak_bertanda32_t arg;
    tak_bertanda32_t timeout;
    status_t hasil;

    if (host == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Argumen: voltage window + CCS + S18R */
    arg = SD_OCR_VDD_27_28 | SD_OCR_VDD_28_29 | SD_OCR_VDD_29_30 |
          SD_OCR_VDD_30_31 | SD_OCR_VDD_31_32 | SD_OCR_VDD_32_33 |
          SD_OCR_VDD_33_34 | SD_OCR_CCS;

    timeout = 100;
    while (timeout > 0) {
        hasil = sd_send_app_cmd(host, SD_ACMD_SD_SEND_OP_COND, arg,
                               SDHCI_CMD_RESP_SHORT, &response);
        if (hasil != STATUS_BERHASIL) {
            /* Mungkin bukan SD card */
            return hasil;
        }

        /* Cek apakah card ready */
        if (response & SD_OCR_BUSY) {
            /* Tentukan tipe card */
            if (response & SD_OCR_CCS) {
                host->device.card_type = SD_CARD_TYPE_SDHC;
            } else {
                host->device.card_type = SD_CARD_TYPE_SDSC;
            }
            host->device.ocr = response;
            return STATUS_BERHASIL;
        }

        cpu_delay_ms(10);
        timeout--;
    }

    return STATUS_TIMEOUT;
}

/*
 * sd_all_send_cid - Dapatkan CID
 */
static status_t sd_all_send_cid(sd_host_t *host)
{
    tak_bertanda32_t response[4];
    tak_bertanda32_t i;
    status_t hasil;

    if (host == NULL) {
        return STATUS_PARAM_NULL;
    }

    hasil = sd_send_cmd(host, SD_CMD_ALL_SEND_CID, 0,
                       SDHCI_CMD_RESP_LONG | SDHCI_CMD_CRC,
                       &response[0]);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    /* Baca seluruh response */
    for (i = 0; i < 4; i++) {
        response[i] = sd_readl(host, SDHCI_RESPONSE + i * 4);
    }

    /* Parse CID */
    host->device.cid.mid = (response[0] >> 24) & 0xFF;
    host->device.oem_id[0] = (response[0] >> 16) & 0xFF;
    host->device.oem_id[1] = (response[0] >> 8) & 0xFF;
    host->device.oem_id[2] = '\0';
    host->device.cid.psn = response[2];

    for (i = 0; i < 5; i++) {
        host->device.cid.pnm[i] =
            (char)((response[1] >> (24 - i * 8)) & 0xFF);
    }
    host->device.cid.pnm[5] = (char)((response[2] >> 24) & 0xFF);
    host->device.cid.pnm[6] = '\0';

    return STATUS_BERHASIL;
}

/*
 * sd_send_rca - Dapatkan relative card address
 */
static status_t sd_send_rca(sd_host_t *host)
{
    tak_bertanda32_t response;
    status_t hasil;

    if (host == NULL) {
        return STATUS_PARAM_NULL;
    }

    hasil = sd_send_cmd(host, SD_CMD_SEND_RELATIVE_ADDR, 0,
                       SDHCI_CMD_RESP_SHORT, &response);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    host->device.rca = (tak_bertanda16_t)((response >> 16) & 0xFFFF);

    return STATUS_BERHASIL;
}

/*
 * sd_send_csd - Dapatkan CSD
 */
static status_t sd_send_csd(sd_host_t *host)
{
    tak_bertanda32_t response[4];
    tak_bertanda32_t i;
    status_t hasil;
    tak_bertanda32_t c_size;
    tak_bertanda32_t c_size_mult;

    if (host == NULL) {
        return STATUS_PARAM_NULL;
    }

    hasil = sd_send_cmd(host, SD_CMD_SEND_CSD,
                       host->device.rca << 16,
                       SDHCI_CMD_RESP_LONG | SDHCI_CMD_CRC,
                       &response[0]);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    /* Baca seluruh response */
    for (i = 0; i < 4; i++) {
        response[i] = sd_readl(host, SDHCI_RESPONSE + i * 4);
    }

    /* Parse CSD */
    host->device.csd.csd_structure = (response[0] >> 30) & 0x3;

    if (host->device.csd.csd_structure == 0) {
        /* SDSC: CSD version 1.0 */
        c_size = ((response[1] >> 30) & 0x3) |
                 ((response[2] & 0x3FF) << 2);
        c_size_mult = (response[2] >> 15) & 0x7;
        host->device.csd.read_bl_len = (response[1] >> 16) & 0xF;

        host->device.capacity = (tak_bertanda64_t)(c_size + 1) *
                               (1 << (c_size_mult + 2)) *
                               (1 << host->device.csd.read_bl_len);
    } else {
        /* SDHC/SDXC: CSD version 2.0 */
        c_size = ((response[1] >> 30) & 0x3) |
                 ((response[2] & 0xFF) << 2) |
                 ((response[2] >> 8) & 0x3FFFFF) << 8;

        host->device.capacity = (tak_bertanda64_t)(c_size + 1) * 512 * 1024;
    }

    host->device.size_bytes = host->device.capacity;

    return STATUS_BERHASIL;
}

/*
 * sd_select_card - Pilih card
 */
static status_t sd_select_card(sd_host_t *host)
{
    tak_bertanda32_t response;
    status_t hasil;

    if (host == NULL) {
        return STATUS_PARAM_NULL;
    }

    hasil = sd_send_cmd(host, SD_CMD_SELECT_CARD,
                       host->device.rca << 16,
                       SDHCI_CMD_RESP_SHORT, &response);
    return hasil;
}

/*
 * sd_set_blocklen - Set block length
 */
static status_t sd_set_blocklen(sd_host_t *host, ukuran_t blocklen)
{
    tak_bertanda32_t response;
    status_t hasil;

    if (host == NULL) {
        return STATUS_PARAM_NULL;
    }

    hasil = sd_send_cmd(host, SD_CMD_SET_BLOCKLEN, blocklen,
                       SDHCI_CMD_RESP_SHORT, &response);
    return hasil;
}

/*
 * sd_set_bus_width - Set bus width
 */
static status_t sd_set_bus_width(sd_host_t *host, tak_bertanda32_t width)
{
    tak_bertanda32_t response;
    status_t hasil;
    tak_bertanda32_t arg;

    if (host == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Argumen: 0 = 1-bit, 2 = 4-bit */
    arg = (width == 4) ? 0x02 : 0x00;

    hasil = sd_send_app_cmd(host, SD_ACMD_SET_BUS_WIDTH, arg,
                           SDHCI_CMD_RESP_SHORT, &response);
    if (hasil == STATUS_BERHASIL) {
        host->bus_width = width;
    }

    return hasil;
}

/*
 * sd_read_single_block - Baca single block
 */
static status_t sd_read_single_block(sd_host_t *host,
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
    hasil = sd_send_cmd(host, SD_CMD_READ_SINGLE_BLOCK, block,
                       SDHCI_CMD_RESP_SHORT | SDHCI_CMD_DATA, NULL);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    /* Tunggu data available */
    timeout = 1000;
    while (timeout > 0) {
        status = sd_readl(host, SDHCI_PRESENT_STATE);
        if (status & 0x00000800) {  /* Data Available */
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
        p[i] = sd_readl(host, SDHCI_BUFFER);
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * sd_card_init - Inisialisasi driver SD Card
 */
status_t sd_card_init(void)
{
    sd_host_t *host;
    tak_bertanda32_t i;
    status_t hasil;

    if (g_sd_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }

    kernel_memset(&g_sd_konteks, 0, sizeof(sd_konteks_t));

    /* Setup host controller */
    /* TODO: Deteksi SDHCI controller dari PCI/Device Tree */

    host = &g_sd_konteks.host[0];
    host->magic = SD_HOST_MAGIC;
    host->id = 0;

    /* TODO: Dapatkan IO base dari PCI/DT */
    /* host->io_base = ...; */

    /* Initialize controller */
    if (host->io_base != NULL) {
        /* Read capabilities */
        host->capabilities = ((tak_bertanda64_t)
                             sd_readl(host, SDHCI_CAPABILITIES_1) << 32) |
                             sd_readl(host, SDHCI_CAPABILITIES);

        /* Reset controller */
        sd_writeb(host, SDHCI_SOFTWARE_RESET, 0x03);
        cpu_delay_ms(10);

        /* Enable interrupts */
        sd_writel(host, SDHCI_INT_ENABLE, 0xFFFFFFFF);
        sd_writel(host, SDHCI_SIGNAL_ENABLE, 0xFFFFFFFF);

        /* Set power */
        sd_writeb(host, SDHCI_POWER_CONTROL, 0x0F);

        /* Set initial clock (400kHz) */
        sd_writew(host, SDHCI_CLOCK_CONTROL, 0x0001);
        sd_writew(host, SDHCI_CLOCK_CONTROL, 0x0003);
        host->clock = 400000;

        host->initialized = BENAR;
    }

    /* Initialize card */
    host->device.magic = SD_DEVICE_MAGIC;
    host->device.id = 0;
    host->device.block_size = SD_MAX_BLOCK_SIZE;

    if (host->io_base != NULL) {
        hasil = sd_go_idle(host);
        if (hasil == STATUS_BERHASIL) {
            /* Cek SD card dengan CMD8 */
            hasil = sd_send_if_cond(host);
        }

        if (hasil == STATUS_BERHASIL) {
            hasil = sd_send_op_cond(host);
        }

        if (hasil == STATUS_BERHASIL) {
            hasil = sd_all_send_cid(host);
        }

        if (hasil == STATUS_BERHASIL) {
            hasil = sd_send_rca(host);
        }

        if (hasil == STATUS_BERHASIL) {
            hasil = sd_send_csd(host);
        }

        if (hasil == STATUS_BERHASIL) {
            hasil = sd_select_card(host);
        }

        if (hasil == STATUS_BERHASIL) {
            hasil = sd_set_blocklen(host, host->device.block_size);
        }

        if (hasil == STATUS_BERHASIL) {
            hasil = sd_set_bus_width(host, 4);
        }

        if (hasil == STATUS_BERHASIL) {
            host->device.present = BENAR;
            host->device.valid = BENAR;
            host->device.initialized = BENAR;
            host->card_inserted = BENAR;

            /* Copy product name */
            snprintf(host->device.product_name,
                    sizeof(host->device.product_name),
                    "%s", host->device.cid.pnm);
        }
    }

    g_sd_konteks.magic = SD_MAGIC;
    g_sd_konteks.jumlah_host = 1;
    g_sd_konteks.diinisialisasi = BENAR;
    g_sd_diinisialisasi = BENAR;

    return STATUS_BERHASIL;
}

/*
 * sd_card_shutdown - Matikan driver SD Card
 */
status_t sd_card_shutdown(void)
{
    tak_bertanda32_t i;

    if (!g_sd_diinisialisasi) {
        return STATUS_BERHASIL;
    }

    for (i = 0; i < SD_MAX_HOSTS; i++) {
        sd_host_t *host = &g_sd_konteks.host[i];

        if (host->io_base != NULL) {
            /* Deselect card */
            sd_send_cmd(host, SD_CMD_SELECT_CARD, 0,
                       SDHCI_CMD_RESP_NONE, NULL);

            /* Reset controller */
            sd_writeb(host, SDHCI_SOFTWARE_RESET, 0x03);
        }
    }

    g_sd_konteks.magic = 0;
    g_sd_konteks.diinisialisasi = SALAH;
    g_sd_diinisialisasi = SALAH;

    return STATUS_BERHASIL;
}

/*
 * sd_card_baca - Baca blok dari SD Card
 *
 * Parameter:
 *   dev_id - ID perangkat
 *   block - Alamat blok
 *   buffer - Buffer output
 *   count - Jumlah blok
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t sd_card_baca(tak_bertanda32_t dev_id, tak_bertanda64_t block,
                      void *buffer, ukuran_t count)
{
    sd_host_t *host;
    ukuran_t i;
    status_t hasil;
    tak_bertanda32_t block_addr;

    if (!g_sd_diinisialisasi) {
        return STATUS_GAGAL;
    }

    if (buffer == NULL || count == 0) {
        return STATUS_PARAM_INVALID;
    }

    /* Cari host dengan device ID */
    host = NULL;
    for (i = 0; i < SD_MAX_HOSTS; i++) {
        if (g_sd_konteks.host[i].device.id == dev_id &&
            g_sd_konteks.host[i].device.valid) {
            host = &g_sd_konteks.host[i];
            break;
        }
    }

    if (host == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Untuk SDSC, gunakan byte address */
    if (host->device.card_type == SD_CARD_TYPE_SDSC) {
        block_addr = (tak_bertanda32_t)(block * host->device.block_size);
    } else {
        block_addr = (tak_bertanda32_t)block;
    }

    /* Baca blok per blok */
    for (i = 0; i < count; i++) {
        hasil = sd_read_single_block(host,
                                    block_addr +
                                    (host->device.card_type ==
                                     SD_CARD_TYPE_SDSC ?
                                     i * host->device.block_size : i),
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
 * sd_card_tulis - Tulis blok ke SD Card
 *
 * Parameter:
 *   dev_id - ID perangkat
 *   block - Alamat blok
 *   buffer - Buffer input
 *   count - Jumlah blok
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t sd_card_tulis(tak_bertanda32_t dev_id, tak_bertanda64_t block,
                       const void *buffer, ukuran_t count)
{
    sd_host_t *host;
    tak_bertanda32_t response;
    tak_bertanda32_t block_addr;
    ukuran_t i;
    status_t hasil;

    if (!g_sd_diinisialisasi) {
        return STATUS_GAGAL;
    }

    if (buffer == NULL || count == 0) {
        return STATUS_PARAM_INVALID;
    }

    /* Cari host */
    host = NULL;
    for (i = 0; i < SD_MAX_HOSTS; i++) {
        if (g_sd_konteks.host[i].device.id == dev_id &&
            g_sd_konteks.host[i].device.valid) {
            host = &g_sd_konteks.host[i];
            break;
        }
    }

    if (host == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Untuk SDSC, gunakan byte address */
    if (host->device.card_type == SD_CARD_TYPE_SDSC) {
        block_addr = (tak_bertanda32_t)(block * host->device.block_size);
    } else {
        block_addr = (tak_bertanda32_t)block;
    }

    /* Tulis blok per blok */
    for (i = 0; i < count; i++) {
        hasil = sd_send_cmd(host, SD_CMD_WRITE_BLOCK,
                           block_addr +
                           (host->device.card_type ==
                            SD_CARD_TYPE_SDSC ?
                            i * host->device.block_size : i),
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
 * sd_card_cetak_info - Cetak informasi SD Card
 */
void sd_card_cetak_info(void)
{
    tak_bertanda32_t i;

    if (!g_sd_diinisialisasi) {
        return;
    }

    kernel_printf("\n=== Perangkat SD Card ===\n\n");

    for (i = 0; i < SD_MAX_HOSTS; i++) {
        sd_host_t *host = &g_sd_konteks.host[i];
        sd_device_t *dev = &host->device;

        if (dev->valid && dev->present) {
            kernel_printf("sd%d: ", dev->id);

            /* Tipe card */
            if (dev->card_type == SD_CARD_TYPE_SDHC) {
                kernel_printf("SDHC ");
            } else if (dev->card_type == SD_CARD_TYPE_SDXC) {
                kernel_printf("SDXC ");
            } else {
                kernel_printf("SDSC ");
            }

            kernel_printf("%s\n", dev->product_name);
            kernel_printf("  Kapasitas: %lu MB\n",
                         dev->capacity / (1024 * 1024));
            kernel_printf("  Block Size: %u byte\n", dev->block_size);
            kernel_printf("  Manufacturer ID: %u\n", dev->cid.mid);
            kernel_printf("  OEM ID: %s\n", dev->oem_id);
            kernel_printf("\n");
        }
    }
}
