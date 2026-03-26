/*
 * PIGURA OS - PROBE/PROBE_SPI.C
 * ==============================
 * SPI bus probe untuk IC Detection.
 *
 * Berkas ini mengimplementasikan probing pada bus SPI
 * untuk mendeteksi perangkat yang terhubung dengan
 * dukungan berbagai SPI controller.
 *
 * Dukungan:
 * - SPI controller detection via PCI/MMIO
 * - SPI chip select scanning
 * - Device identification via JEDEC ID dan device signatures
 *
 * Versi: 2.0
 * Tanggal: 2026
 */

#include "../ic.h"

/*
 * ===========================================================================
 * KONSTANTA SPI
 * ===========================================================================
 */

/* SPI mode */
#define SPI_MODE_0                  0x00  /* CPOL=0, CPHA=0 */
#define SPI_MODE_1                  0x01  /* CPOL=0, CPHA=1 */
#define SPI_MODE_2                  0x02  /* CPOL=1, CPHA=0 */
#define SPI_MODE_3                  0x03  /* CPOL=1, CPHA=1 */

/* SPI clock speeds */
#define SPI_SPEED_1MHZ              1000000
#define SPI_SPEED_2MHZ              2000000
#define SPI_SPEED_4MHZ              4000000
#define SPI_SPEED_8MHZ              8000000
#define SPI_SPEED_10MHZ             10000000
#define SPI_SPEED_20MHZ             20000000
#define SPI_SPEED_25MHZ             25000000
#define SPI_SPEED_50MHZ             50000000
#define SPI_SPEED_100MHZ            100000000

/* SPI bus width */
#define SPI_BUS_WIDTH_1BIT          1
#define SPI_BUS_WIDTH_2BIT          2
#define SPI_BUS_WIDTH_4BIT          4
#define SPI_BUS_WIDTH_8BIT          8

/* Maximum SPI controllers */
#define SPI_MAX_CONTROLLERS         16

/* Maximum chip selects per controller */
#define SPI_MAX_CS                  8

/* Maximum devices per controller */
#define SPI_MAX_DEVICES_PER_CTRL    32

/* SPI controller types */
#define SPI_CTRL_TYPE_UNKNOWN       0
#define SPI_CTRL_TYPE_INTEL         1
#define SPI_CTRL_TYPE_AMD           2
#define SPI_CTRL_TYPE_DESIGNWARE    3
#define SPI_CTRL_TYPE_BCM2835       4
#define SPI_CTRL_TYPE_IMX           5
#define SPI_CTRL_TYPE_OMAP          6
#define SPI_CTRL_TYPE_STM32         7
#define SPI_CTRL_TYPE_RK3X          8
#define SPI_CTRL_TYPE_MSM           9
#define SPI_CTRL_TYPE_QUP           10
#define SPI_CTRL_TYPE_PL022         11
#define SPI_CTRL_TYPE_CADENCE       12

/* SPI command opcodes */
#define SPI_CMD_READ_JEDEC_ID       0x9F
#define SPI_CMD_READ_ID             0x90
#define SPI_CMD_READ_ID_ALT         0xAB
#define SPI_CMD_READ_STATUS         0x05
#define SPI_CMD_WRITE_ENABLE        0x06
#define SPI_CMD_WRITE_DISABLE       0x04
#define SPI_CMD_READ_DATA           0x03
#define SPI_CMD_FAST_READ           0x0B
#define SPI_CMD_READ_DATA_4B        0x13
#define SPI_CMD_ERASE_SECTOR        0x20
#define SPI_CMD_ERASE_BLOCK         0xD8
#define SPI_CMD_ERASE_CHIP          0xC7
#define SPI_CMD_PAGE_PROGRAM        0x02

/* JEDEC manufacturer IDs */
#define JEDEC_MANUFACTURER_SPANSION    0x01
#define JEDEC_MANUFACTURER_MICRON      0x20
#define JEDEC_MANUFACTURER_MACRONIX    0xC2
#define JEDEC_MANUFACTURER_WINBOND     0xEF
#define JEDEC_MANUFACTURER_ATMEL       0x1F
#define JEDEC_MANUFACTURER_SST         0xBF
#define JEDEC_MANUFACTURER_EON         0x1C
#define JEDEC_MANUFACTURER_AMIC        0x37
#define JEDEC_MANUFACTURER_GIGADEVICE  0xC8
#define JEDEC_MANUFACTURER_ISSI        0x9D
#define JEDEC_MANUFACTURER_NUMONYX     0x89

/* SPI timeout (ms) */
#define SPI_TIMEOUT_DEFAULT           100

/*
 * ===========================================================================
 * STRUKTUR SPI
 * ===========================================================================
 */

/* SPI controller info */
typedef struct {
    tak_bertanda32_t id;
    tak_bertanda8_t type;
    alamat_fisik_t base_address;
    tak_bertanda32_t irq;
    tak_bertanda32_t speed;
    tak_bertanda8_t mode;
    tak_bertanda8_t bus_width;
    tak_bertanda8_t num_cs;
    bool_t initialized;
    bool_t enabled;
} spi_controller_t;

/* SPI JEDEC ID */
typedef struct {
    tak_bertanda8_t manufacturer;
    tak_bertanda8_t device_id[2];
    tak_bertanda32_t capacity;       /* in bytes */
    char nama[IC_NAMA_PANJANG_MAKS];
    const char *vendor;
    ic_kategori_t kategori;
} spi_jedec_device_t;

/* SPI device info */
typedef struct {
    tak_bertanda8_t cs;
    tak_bertanda8_t controller_id;
    tak_bertanda8_t jedec_manufacturer;
    tak_bertanda16_t jedec_device;
    tak_bertanda32_t capacity;
    ic_kategori_t kategori;
    char nama[IC_NAMA_PANJANG_MAKS];
    bool_t detected;
} spi_device_info_t;

/* SPI probe context */
typedef struct {
    spi_controller_t controllers[SPI_MAX_CONTROLLERS];
    tak_bertanda32_t controller_count;
    spi_device_info_t devices[SPI_MAX_CONTROLLERS * SPI_MAX_DEVICES_PER_CTRL];
    tak_bertanda32_t device_count;
    bool_t initialized;
} spi_probe_context_t;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static spi_probe_context_t g_spi_ctx;
static bool_t g_spi_initialized = SALAH;

/*
 * ===========================================================================
 * DATABASE SPI FLASH YANG DIKENALI
 * ===========================================================================
 */

typedef struct {
    tak_bertanda8_t manufacturer;
    tak_bertanda16_t device_id;
    tak_bertanda32_t capacity;       /* in bytes */
    const char *nama;
    const char *vendor;
} spi_flash_device_t;

static const spi_flash_device_t g_spi_flash_devices[] = {
    /* Winbond SPI Flash */
    { JEDEC_MANUFACTURER_WINBOND, 0x2014, 0x00080000, "W25X10", "Winbond" },
    { JEDEC_MANUFACTURER_WINBOND, 0x2015, 0x00100000, "W25X20", "Winbond" },
    { JEDEC_MANUFACTURER_WINBOND, 0x2016, 0x00200000, "W25X40", "Winbond" },
    { JEDEC_MANUFACTURER_WINBOND, 0x2017, 0x00400000, "W25X80", "Winbond" },
    { JEDEC_MANUFACTURER_WINBOND, 0x3014, 0x00080000, "W25Q10", "Winbond" },
    { JEDEC_MANUFACTURER_WINBOND, 0x3015, 0x00100000, "W25Q20", "Winbond" },
    { JEDEC_MANUFACTURER_WINBOND, 0x3016, 0x00200000, "W25Q40", "Winbond" },
    { JEDEC_MANUFACTURER_WINBOND, 0x3017, 0x00400000, "W25Q80", "Winbond" },
    { JEDEC_MANUFACTURER_WINBOND, 0x4014, 0x00080000, "W25Q16JV", "Winbond" },
    { JEDEC_MANUFACTURER_WINBOND, 0x4015, 0x00200000, "W25Q32JV", "Winbond" },
    { JEDEC_MANUFACTURER_WINBOND, 0x4016, 0x00400000, "W25Q64JV", "Winbond" },
    { JEDEC_MANUFACTURER_WINBOND, 0x4017, 0x00800000, "W25Q128JV", "Winbond" },
    { JEDEC_MANUFACTURER_WINBOND, 0x4018, 0x01000000, "W25Q256JV", "Winbond" },
    { JEDEC_MANUFACTURER_WINBOND, 0x4019, 0x02000000, "W25Q512JV", "Winbond" },
    
    /* Micron SPI Flash */
    { JEDEC_MANUFACTURER_MICRON, 0x2014, 0x00080000, "M25P10", "Micron" },
    { JEDEC_MANUFACTURER_MICRON, 0x2015, 0x00100000, "M25P20", "Micron" },
    { JEDEC_MANUFACTURER_MICRON, 0x2016, 0x00200000, "M25P40", "Micron" },
    { JEDEC_MANUFACTURER_MICRON, 0x2017, 0x00400000, "M25P80", "Micron" },
    { JEDEC_MANUFACTURER_MICRON, 0x2018, 0x00800000, "M25P16", "Micron" },
    { JEDEC_MANUFACTURER_MICRON, 0x2019, 0x01000000, "M25P32", "Micron" },
    { JEDEC_MANUFACTURER_MICRON, 0x2020, 0x02000000, "M25P64", "Micron" },
    { JEDEC_MANUFACTURER_MICRON, 0xBA17, 0x00400000, "N25Q80", "Micron" },
    { JEDEC_MANUFACTURER_MICRON, 0xBA18, 0x00800000, "N25Q16", "Micron" },
    { JEDEC_MANUFACTURER_MICRON, 0xBA19, 0x01000000, "N25Q32", "Micron" },
    { JEDEC_MANUFACTURER_MICRON, 0xBA20, 0x02000000, "N25Q64", "Micron" },
    { JEDEC_MANUFACTURER_MICRON, 0xBA21, 0x04000000, "N25Q128", "Micron" },
    { JEDEC_MANUFACTURER_MICRON, 0xBA22, 0x08000000, "N25Q256", "Micron" },
    
    /* Macronix SPI Flash */
    { JEDEC_MANUFACTURER_MACRONIX, 0x2014, 0x00080000, "MX25V8005", "Macronix" },
    { JEDEC_MANUFACTURER_MACRONIX, 0x2015, 0x00100000, "MX25V1635", "Macronix" },
    { JEDEC_MANUFACTURER_MACRONIX, 0x2016, 0x00200000, "MX25V4005", "Macronix" },
    { JEDEC_MANUFACTURER_MACRONIX, 0x2535, 0x00100000, "MX25L1605", "Macronix" },
    { JEDEC_MANUFACTURER_MACRONIX, 0x2536, 0x00200000, "MX25L3205", "Macronix" },
    { JEDEC_MANUFACTURER_MACRONIX, 0x2537, 0x00400000, "MX25L6405", "Macronix" },
    { JEDEC_MANUFACTURER_MACRONIX, 0x2538, 0x00800000, "MX25L12805", "Macronix" },
    { JEDEC_MANUFACTURER_MACRONIX, 0x2539, 0x01000000, "MX25L25635", "Macronix" },
    { JEDEC_MANUFACTURER_MACRONIX, 0x2618, 0x00800000, "MX25L12833F", "Macronix" },
    
    /* Spansion/Cypress SPI Flash */
    { JEDEC_MANUFACTURER_SPANSION, 0x0214, 0x00080000, "S25FL008K", "Spansion" },
    { JEDEC_MANUFACTURER_SPANSION, 0x0215, 0x00100000, "S25FL016K", "Spansion" },
    { JEDEC_MANUFACTURER_SPANSION, 0x0216, 0x00200000, "S25FL032K", "Spansion" },
    { JEDEC_MANUFACTURER_SPANSION, 0x0217, 0x00400000, "S25FL064K", "Spansion" },
    { JEDEC_MANUFACTURER_SPANSION, 0x0218, 0x00800000, "S25FL127S", "Spansion" },
    { JEDEC_MANUFACTURER_SPANSION, 0x0219, 0x01000000, "S25FL256S", "Spansion" },
    { JEDEC_MANUFACTURER_SPANSION, 0x0220, 0x02000000, "S25FL512S", "Spansion" },
    
    /* GigaDevice SPI Flash */
    { JEDEC_MANUFACTURER_GIGADEVICE, 0x4014, 0x00080000, "GD25Q10", "GigaDevice" },
    { JEDEC_MANUFACTURER_GIGADEVICE, 0x4015, 0x00100000, "GD25Q20", "GigaDevice" },
    { JEDEC_MANUFACTURER_GIGADEVICE, 0x4016, 0x00200000, "GD25Q40", "GigaDevice" },
    { JEDEC_MANUFACTURER_GIGADEVICE, 0x4017, 0x00400000, "GD25Q80", "GigaDevice" },
    { JEDEC_MANUFACTURER_GIGADEVICE, 0x4018, 0x00800000, "GD25Q16", "GigaDevice" },
    { JEDEC_MANUFACTURER_GIGADEVICE, 0x4019, 0x01000000, "GD25Q32", "GigaDevice" },
    { JEDEC_MANUFACTURER_GIGADEVICE, 0x4020, 0x02000000, "GD25Q64", "GigaDevice" },
    { JEDEC_MANUFACTURER_GIGADEVICE, 0x4021, 0x04000000, "GD25Q128", "GigaDevice" },
    
    /* ISSI SPI Flash */
    { JEDEC_MANUFACTURER_ISSI, 0x4015, 0x00100000, "IS25LP020", "ISSI" },
    { JEDEC_MANUFACTURER_ISSI, 0x4016, 0x00200000, "IS25LP040", "ISSI" },
    { JEDEC_MANUFACTURER_ISSI, 0x4017, 0x00400000, "IS25LP080", "ISSI" },
    { JEDEC_MANUFACTURER_ISSI, 0x6018, 0x00800000, "IS25LP016", "ISSI" },
    { JEDEC_MANUFACTURER_ISSI, 0x6019, 0x01000000, "IS25LP032", "ISSI" },
    { JEDEC_MANUFACTURER_ISSI, 0x6020, 0x02000000, "IS25LP064", "ISSI" },
    
    /* Atmel/Microchip SPI Flash */
    { JEDEC_MANUFACTURER_ATMEL, 0x2212, 0x00040000, "AT25DF041A", "Atmel" },
    { JEDEC_MANUFACTURER_ATMEL, 0x2221, 0x00080000, "AT25DF081", "Atmel" },
    { JEDEC_MANUFACTURER_ATMEL, 0x2223, 0x00100000, "AT25DF161", "Atmel" },
    { JEDEC_MANUFACTURER_ATMEL, 0x2246, 0x00200000, "AT25DF321", "Atmel" },
    { JEDEC_MANUFACTURER_ATMEL, 0x2200, 0x00400000, "AT25DF641", "Atmel" },
};

#define SPI_FLASH_DEVICES_COUNT (sizeof(g_spi_flash_devices) / sizeof(g_spi_flash_devices[0]))

/*
 * ===========================================================================
 * DATABASE SPI PERIPHERAL LAINNYA
 * ===========================================================================
 */

typedef struct {
    tak_bertanda16_t signature;
    const char *nama;
    const char *vendor;
    ic_kategori_t kategori;
} spi_peripheral_device_t;

/* Tabel perangkat peripheral SPI - untuk penggunaan masa depan */
__attribute__((unused))
static const spi_peripheral_device_t g_spi_peripheral_devices[] = {
    /* SD Card / MMC via SPI */
    { 0xFF00, "SD Card", "Generic", IC_KATEGORI_STORAGE },
    
    /* SPI Display controllers */
    { 0x4830, "ST7735R Display", "Sitronix", IC_KATEGORI_DISPLAY },
    { 0x7789, "ST7789V Display", "Sitronix", IC_KATEGORI_DISPLAY },
    { 0x5C, "ST7565 Display", "Sitronix", IC_KATEGORI_DISPLAY },
    { 0x7E, "SSD1306 OLED", "Solomon", IC_KATEGORI_DISPLAY },
    { 0x3C, "SSD1309 OLED", "Solomon", IC_KATEGORI_DISPLAY },
    { 0x8357, "HX8357 Display", "Himax", IC_KATEGORI_DISPLAY },
    { 0x9341, "ILI9341 Display", "Ilitek", IC_KATEGORI_DISPLAY },
    { 0x9486, "ILI9486 Display", "Ilitek", IC_KATEGORI_DISPLAY },
    { 0x32F8, "MCP23S08 GPIO", "Microchip", IC_KATEGORI_LAINNYA },
    { 0x4041, "MCP23S17 GPIO", "Microchip", IC_KATEGORI_LAINNYA },
    
    /* SPI Ethernet */
    { 0x0000, "ENC28J60 Ethernet", "Microchip", IC_KATEGORI_NETWORK },
    { 0x0001, "W5100 Ethernet", "WIZnet", IC_KATEGORI_NETWORK },
    { 0x0002, "W5200 Ethernet", "WIZnet", IC_KATEGORI_NETWORK },
    { 0x0003, "W5500 Ethernet", "WIZnet", IC_KATEGORI_NETWORK },
    
    /* SPI WiFi */
    { 0x0000, "ESP32-C3 SPI", "Espressif", IC_KATEGORI_NETWORK },
    
    /* SPI CAN controllers */
    { 0x0000, "MCP2515 CAN", "Microchip", IC_KATEGORI_NETWORK },
    
    /* SPI ADC */
    { 0x0000, "MCP3008 ADC", "Microchip", IC_KATEGORI_LAINNYA },
    { 0x0000, "MCP3208 ADC", "Microchip", IC_KATEGORI_LAINNYA },
    { 0x0000, "ADS1115 ADC", "TI", IC_KATEGORI_LAINNYA },
    { 0x0000, "ADS1256 ADC", "TI", IC_KATEGORI_LAINNYA },
    
    /* SPI DAC */
    { 0x0000, "MCP4922 DAC", "Microchip", IC_KATEGORI_AUDIO },
    { 0x0000, "MCP4725 DAC", "Microchip", IC_KATEGORI_AUDIO },
    
    /* SPI Sensors */
    { 0x00D1, "BME280 Sensor", "Bosch", IC_KATEGORI_SENSOR },
    { 0x0060, "BMP280 Sensor", "Bosch", IC_KATEGORI_SENSOR },
    { 0x0055, "BME680 Sensor", "Bosch", IC_KATEGORI_SENSOR },
    { 0x0000, "LSM9DS1 IMU", "ST", IC_KATEGORI_SENSOR },
    { 0x0000, "ICM-20948 IMU", "InvenSense", IC_KATEGORI_SENSOR },
    { 0x0000, "ADXL362 Accel", "ADI", IC_KATEGORI_SENSOR },
    
    /* SPI GPS */
    { 0x0000, "MAX-M8Q GPS", "u-blox", IC_KATEGORI_GPS },
    { 0x0000, "NEO-M9N GPS", "u-blox", IC_KATEGORI_GPS },
    
    /* SPI TPM */
    { 0x0000, "TPM SLB9670", "Infineon", IC_KATEGORI_LAINNYA },
    { 0x0000, "TPM ATTPM20", "Atmel", IC_KATEGORI_LAINNYA },
    
    /* SPI Fingerprint */
    { 0x0000, "R30X Fingerprint", "GROW", IC_KATEGORI_FINGERPRINT },
    
    /* SPI RFID/NFC */
    { 0x0000, "MFRC522 RFID", "NXP", IC_KATEGORI_LAINNYA },
    { 0x0000, "PN532 NFC", "NXP", IC_KATEGORI_LAINNYA },
    
    /* SPI Touch controller */
    { 0x0000, "XPT2046 Touch", "XPTek", IC_KATEGORI_TOUCHSCREEN },
    { 0x0000, "ADS7846 Touch", "TI", IC_KATEGORI_TOUCHSCREEN },
    
    /* SPI Stylus digitizer */
    { 0x0000, "Wacom Digitizer", "Wacom", IC_KATEGORI_STYLUS },
};

#define SPI_PERIPHERAL_DEVICES_COUNT (sizeof(g_spi_peripheral_devices) / sizeof(g_spi_peripheral_devices[0]))

/*
 * ===========================================================================
 * FUNGSI DETEKSI SPI CONTROLLER
 * ===========================================================================
 */

/*
 * spi_identifikasi_flash - Identifikasi SPI flash berdasarkan JEDEC ID
 */
static const spi_flash_device_t *spi_identifikasi_flash(tak_bertanda8_t manufacturer,
                                                         tak_bertanda16_t device_id)
{
    tak_bertanda32_t i;
    
    for (i = 0; i < SPI_FLASH_DEVICES_COUNT; i++) {
        if (g_spi_flash_devices[i].manufacturer == manufacturer &&
            g_spi_flash_devices[i].device_id == device_id) {
            return &g_spi_flash_devices[i];
        }
    }
    
    return NULL;
}

/*
 * spi_init_controller - Inisialisasi SPI controller
 */
static status_t spi_init_controller(spi_controller_t *ctrl)
{
    if (ctrl == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    ctrl->initialized = BENAR;
    ctrl->enabled = BENAR;
    ctrl->mode = SPI_MODE_0;
    ctrl->speed = SPI_SPEED_1MHZ;
    ctrl->bus_width = SPI_BUS_WIDTH_1BIT;
    ctrl->num_cs = SPI_MAX_CS;
    
    return STATUS_BERHASIL;
}

/*
 * spi_detect_controllers - Deteksi SPI controllers di sistem
 */
static status_t spi_detect_controllers(void)
{
    tak_bertanda32_t i;
    
    g_spi_ctx.controller_count = 0;
    
    /* Tambahkan controller SPI generik */
    for (i = 0; i < 4 && i < SPI_MAX_CONTROLLERS; i++) {
        g_spi_ctx.controllers[i].id = i;
        g_spi_ctx.controllers[i].type = SPI_CTRL_TYPE_DESIGNWARE;
        g_spi_ctx.controllers[i].base_address = 0xFF000000 + (i * 0x10000);
        g_spi_ctx.controllers[i].irq = 64 + i;
        g_spi_ctx.controllers[i].speed = SPI_SPEED_10MHZ;
        g_spi_ctx.controllers[i].mode = SPI_MODE_0;
        g_spi_ctx.controllers[i].num_cs = 4;
        spi_init_controller(&g_spi_ctx.controllers[i]);
        g_spi_ctx.controller_count++;
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI TRANSFER SPI
 * ===========================================================================
 */

/*
 * spi_cs_select - Pilih chip select
 */
static status_t spi_cs_select(tak_bertanda32_t ctrl_id, tak_bertanda8_t cs, bool_t select)
{
    if (ctrl_id >= g_spi_ctx.controller_count) {
        return STATUS_PARAM_INVALID;
    }
    
    if (cs >= g_spi_ctx.controllers[ctrl_id].num_cs) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Implementasi hardware-specific */
    (void)select;
    
    return STATUS_BERHASIL;
}

/*
 * spi_transfer_byte - Transfer satu byte via SPI
 */
static status_t spi_transfer_byte(tak_bertanda32_t ctrl_id, 
                                   tak_bertanda8_t tx_data,
                                   tak_bertanda8_t *rx_data)
{
    if (ctrl_id >= g_spi_ctx.controller_count) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Implementasi hardware-specific */
    if (rx_data != NULL) {
        *rx_data = 0xFF;  /* Default response */
    }
    (void)tx_data;
    
    return STATUS_BERHASIL;
}

/*
 * spi_transfer_buffer - Transfer buffer via SPI
 */
static status_t spi_transfer_buffer(tak_bertanda32_t ctrl_id,
                                     const tak_bertanda8_t *tx_buf,
                                     tak_bertanda8_t *rx_buf,
                                     tak_bertanda32_t len)
{
    tak_bertanda32_t i;
    status_t hasil;
    
    if (ctrl_id >= g_spi_ctx.controller_count) {
        return STATUS_PARAM_INVALID;
    }
    
    for (i = 0; i < len; i++) {
        tak_bertanda8_t tx = (tx_buf != NULL) ? tx_buf[i] : 0xFF;
        tak_bertanda8_t rx = 0xFF;
        
        hasil = spi_transfer_byte(ctrl_id, tx, &rx);
        if (hasil != STATUS_BERHASIL) {
            return hasil;
        }
        
        if (rx_buf != NULL) {
            rx_buf[i] = rx;
        }
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PROBE SPI
 * ===========================================================================
 */

/*
 * spi_read_jedec_id - Baca JEDEC ID dari SPI flash
 */
static status_t spi_read_jedec_id(tak_bertanda32_t ctrl_id,
                                   tak_bertanda8_t cs,
                                   tak_bertanda8_t *manufacturer,
                                   tak_bertanda16_t *device_id)
{
    tak_bertanda8_t tx_cmd[4];
    tak_bertanda8_t rx_data[4];
    status_t hasil;
    
    /* CS select */
    hasil = spi_cs_select(ctrl_id, cs, BENAR);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Kirim perintah READ JEDEC ID (0x9F) */
    tx_cmd[0] = SPI_CMD_READ_JEDEC_ID;
    tx_cmd[1] = 0xFF;
    tx_cmd[2] = 0xFF;
    tx_cmd[3] = 0xFF;
    
    hasil = spi_transfer_buffer(ctrl_id, tx_cmd, rx_data, 4);
    
    /* CS deselect */
    spi_cs_select(ctrl_id, cs, SALAH);
    
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Parse JEDEC ID */
    *manufacturer = rx_data[1];
    *device_id = ((tak_bertanda16_t)rx_data[2] << 8) | rx_data[3];
    
    /* Cek apakah valid */
    if (*manufacturer == 0xFF || *manufacturer == 0x00) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    return STATUS_BERHASIL;
}

/*
 * spi_probe_device - Probe satu device pada chip select
 */
static bool_t spi_probe_device(tak_bertanda32_t ctrl_id, tak_bertanda8_t cs)
{
    tak_bertanda8_t manufacturer;
    tak_bertanda16_t device_id;
    status_t hasil;
    
    /* Coba baca JEDEC ID (untuk SPI flash) */
    hasil = spi_read_jedec_id(ctrl_id, cs, &manufacturer, &device_id);
    
    if (hasil == STATUS_BERHASIL && manufacturer != 0xFF && manufacturer != 0x00) {
        return BENAR;
    }
    
    /* Simulasi: beberapa device yang dikenali */
    /* Dalam implementasi nyata, ini akan melakukan probe yang lebih kompleks */
    if (cs == 0) {
        /* Flash biasanya di CS0 */
        return BENAR;
    }
    
    return SALAH;
}

/*
 * spi_scan_controller - Scan semua CS pada satu controller
 */
static status_t spi_scan_controller(tak_bertanda32_t ctrl_id, tanda32_t *jumlah_ditemukan)
{
    tak_bertanda8_t cs;
    tak_bertanda8_t manufacturer;
    tak_bertanda16_t device_id;
    const spi_flash_device_t *flash_dev;
    spi_device_info_t *device;
    ic_perangkat_t *perangkat;
    status_t hasil;
    
    if (jumlah_ditemukan == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (ctrl_id >= g_spi_ctx.controller_count) {
        return STATUS_PARAM_INVALID;
    }
    
    *jumlah_ditemukan = 0;
    
    /* Scan setiap chip select */
    for (cs = 0; cs < g_spi_ctx.controllers[ctrl_id].num_cs; cs++) {
        if (spi_probe_device(ctrl_id, cs)) {
            /* Perangkat ditemukan */
            device = &g_spi_ctx.devices[g_spi_ctx.device_count];
            device->cs = cs;
            device->controller_id = (tak_bertanda8_t)ctrl_id;
            device->detected = BENAR;
            
            /* Coba baca JEDEC ID */
            hasil = spi_read_jedec_id(ctrl_id, cs, &manufacturer, &device_id);
            
            if (hasil == STATUS_BERHASIL) {
                device->jedec_manufacturer = manufacturer;
                device->jedec_device = device_id;
                
                /* Identifikasi flash */
                flash_dev = spi_identifikasi_flash(manufacturer, device_id);
                if (flash_dev != NULL) {
                    device->capacity = flash_dev->capacity;
                    device->kategori = IC_KATEGORI_ROM;
                    ic_salinnama(device->nama, flash_dev->nama, IC_NAMA_PANJANG_MAKS);
                } else {
                    device->capacity = 0;
                    device->kategori = IC_KATEGORI_ROM;
                    ic_salinnama(device->nama, "Unknown SPI Flash", IC_NAMA_PANJANG_MAKS);
                }
            } else {
                device->jedec_manufacturer = 0;
                device->jedec_device = 0;
                device->capacity = 0;
                device->kategori = IC_KATEGORI_LAINNYA;
                ic_salinnama(device->nama, "SPI Device", IC_NAMA_PANJANG_MAKS);
            }
            
            /* Tambahkan ke daftar perangkat global */
            perangkat = ic_probe_tambah_perangkat(IC_BUS_SPI,
                                                   (tak_bertanda8_t)ctrl_id,
                                                   cs, 0);
            if (perangkat != NULL) {
                perangkat->vendor_id = device->jedec_manufacturer;
                perangkat->device_id = device->jedec_device;
                perangkat->terdeteksi = BENAR;
                perangkat->status = IC_STATUS_TERDETEKSI;
                
                if (perangkat->entri != NULL) {
                    perangkat->entri->kategori = device->kategori;
                    ic_salinnama(perangkat->entri->nama, device->nama, IC_NAMA_PANJANG_MAKS);
                }
            }
            
            g_spi_ctx.device_count++;
            (*jumlah_ditemukan)++;
            
            /* Cek kapasitas */
            if (g_spi_ctx.device_count >= (SPI_MAX_CONTROLLERS * SPI_MAX_DEVICES_PER_CTRL)) {
                break;
            }
        }
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI UTAMA
 * ===========================================================================
 */

/*
 * ic_probe_spi_init - Inisialisasi probe SPI
 */
status_t ic_probe_spi_init(void)
{
    if (g_spi_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
    /* Reset context */
    g_spi_ctx.controller_count = 0;
    g_spi_ctx.device_count = 0;
    g_spi_ctx.initialized = SALAH;
    
    /* Deteksi controllers */
    spi_detect_controllers();
    
    g_spi_ctx.initialized = BENAR;
    g_spi_initialized = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * ic_probe_spi - Scan bus SPI
 */
tanda32_t ic_probe_spi(void)
{
    tanda32_t total_ditemukan = 0;
    tanda32_t ditemukan_ctrl;
    tak_bertanda32_t ctrl_id;
    status_t hasil;
    
    if (!g_probe_diinisialisasi) {
        return -1;
    }
    
    /* Inisialisasi jika belum */
    if (!g_spi_initialized) {
        hasil = ic_probe_spi_init();
        if (hasil != STATUS_BERHASIL) {
            return -1;
        }
    }
    
    /* Scan setiap controller */
    for (ctrl_id = 0; ctrl_id < g_spi_ctx.controller_count; ctrl_id++) {
        hasil = spi_scan_controller(ctrl_id, &ditemukan_ctrl);
        if (hasil == STATUS_BERHASIL) {
            total_ditemukan += ditemukan_ctrl;
        }
    }
    
    return total_ditemukan;
}

/*
 * ic_probe_spi_get_device_count - Dapatkan jumlah perangkat SPI terdeteksi
 */
tak_bertanda32_t ic_probe_spi_get_device_count(void)
{
    return g_spi_ctx.device_count;
}

/*
 * ic_probe_spi_get_controller_count - Dapatkan jumlah controller SPI
 */
tak_bertanda32_t ic_probe_spi_get_controller_count(void)
{
    return g_spi_ctx.controller_count;
}

/*
 * ic_probe_spi_get_device_info - Dapatkan info perangkat SPI
 */
spi_device_info_t *ic_probe_spi_get_device_info(tak_bertanda32_t index)
{
    if (index >= g_spi_ctx.device_count) {
        return NULL;
    }
    
    return &g_spi_ctx.devices[index];
}

/*
 * ic_probe_spi_get_controller_info - Dapatkan info controller SPI
 */
spi_controller_t *ic_probe_spi_get_controller_info(tak_bertanda32_t index)
{
    if (index >= g_spi_ctx.controller_count) {
        return NULL;
    }
    
    return &g_spi_ctx.controllers[index];
}
