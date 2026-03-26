/*
 * PIGURA OS - PROBE/PROBE_MMIO.C
 * ===============================
 * Memory-Mapped I/O probe untuk IC Detection.
 *
 * Berkas ini mengimplementasikan probing pada MMIO space
 * untuk mendeteksi perangkat yang terhubung melalui
 * memory-mapped interface (umum pada ARM SoC).
 *
 * Dukungan:
 * - ARM SoC peripheral detection
 * - Device Tree based probing
 * - Simple peripheral register scanning
 * - SoC specific base address detection
 *
 * Versi: 2.0
 * Tanggal: 2026
 */

#include "../ic.h"

/*
 * ===========================================================================
 * KONSTANTA MMIO
 * ===========================================================================
 */

/* ARM SoC peripheral base addresses */
#define MMIO_BASE_RASPBERRY_PI      0x3F000000ULL
#define MMIO_BASE_RASPBERRY_PI4     0xFE000000ULL
#define MMIO_BASE_ALLWINNER_A64     0x01C00000ULL
#define MMIO_BASE_ALLWINNER_H3      0x01C00000ULL
#define MMIO_BASE_ROCKCHIP_RK3288   0xFF000000ULL
#define MMIO_BASE_ROCKCHIP_RK3399   0xFF000000ULL
#define MMIO_BASE_NVIDIA_TEGRA      0x70000000ULL
#define MMIO_BASE_TI_OMAP           0x48000000ULL
#define MMIO_BASE_AMLOGIC_MESON     0xC0000000ULL
#define MMIO_BASE_SNAPDRAGON        0x00000000ULL
#define MMIO_APPLE_M1               0x200000000ULL
#define MMIO_MTK_BASE               0x10000000ULL
#define MMIO_IMX_BASE               0x20000000ULL
#define MMIO_STM32_BASE             0x40000000ULL

/* Common peripheral offsets */
#define MMIO_OFFSET_GPIO            0x00000000
#define MMIO_OFFSET_UART            0x00100000
#define MMIO_OFFSET_SPI             0x00200000
#define MMIO_OFFSET_I2C             0x00300000
#define MMIO_OFFSET_USB             0x00400000
#define MMIO_OFFSET_ETHERNET        0x00500000
#define MMIO_OFFSET_SDHCI           0x00600000
#define MMIO_OFFSET_DISPLAY         0x00700000
#define MMIO_OFFSET_GPU             0x00800000
#define MMIO_OFFSET_VIDEO           0x00900000
#define MMIO_OFFSET_AUDIO           0x00A00000
#define MMIO_OFFSET_CAMERA          0x00B00000
#define MMIO_OFFSET_PCIE            0x00C00000
#define MMIO_OFFSET_SATA            0x00D00000
#define MMIO_OFFSET_TIMER           0x00E00000
#define MMIO_OFFSET_WATCHDOG        0x00F00000
#define MMIO_OFFSET_RTC             0x01000000
#define MMIO_OFFSET_PWM             0x01100000
#define MMIO_OFFSET_ADC             0x01200000
#define MMIO_OFFSET_DMA             0x01300000

/* Maximum MMIO regions */
#define MMIO_MAX_REGIONS            64

/* Maximum devices per region */
#define MMIO_MAX_DEVICES_PER_REGION 32

/* Magic values for device identification */
#define MMIO_MAGIC_UART_PL011       0x00000011
#define MMIO_MAGIC_UART_NS16550     0x00000000
#define MMIO_MAGIC_GPIO_GENERIC     0xFFFFFFFF
#define MMIO_MAGIC_DW_APB_UART      0x00003501
#define MMIO_MAGIC_DW_APB_I2C       0x00003130
#define MMIO_MAGIC_DW_APB_SPI       0x00003330
#define MMIO_MAGIC_DW_APB_GPIO      0x00003107
#define MMIO_MAGIC_DW_APB_TIM       0x00003106
#define MMIO_MAGIC_DW_DMA           0x00007813
#define MMIO_MAGIC_SNPS_DWC2_USB    0x00004F54
#define MMIO_MAGIC_SNPS_DWC3_USB    0x00005533
#define MMIO_MAGIC_SYNOPSYS_ETH     0x00001037
#define MMIO_MAGIC_CADENCE_UART     0x00001430
#define MMIO_MAGIC_CADENCE_SPI      0x0002080C
#define MMIO_MAGIC_CADENCE_I2C      0x00001C02
#define MMIO_MAGIC_ARM_PL330_DMA    0x00004130
#define MMIO_MAGIC_ARM_SMMU         0x00000003
#define MMIO_MAGIC_ARM_GIC          0x00000043
#define MMIO_MAGIC_ARM_GIC_400      0x00000400
#define MMIO_MAGIC_ARM_GIC_V3       0x00000003

/* SoC types */
#define MMIO_SOC_TYPE_UNKNOWN       0
#define MMIO_SOC_TYPE_RPI_BCM2835   1
#define MMIO_SOC_TYPE_RPI_BCM2836   2
#define MMIO_SOC_TYPE_RPI_BCM2837   3
#define MMIO_SOC_TYPE_RPI_BCM2711   4
#define MMIO_SOC_TYPE_ALLWINNER_A64 5
#define MMIO_SOC_TYPE_ALLWINNER_H3  6
#define MMIO_SOC_TYPE_ALLWINNER_H5  7
#define MMIO_SOC_TYPE_ROCKCHIP_RK3288 8
#define MMIO_SOC_TYPE_ROCKCHIP_RK3399 9
#define MMIO_SOC_TYPE_ROCKCHIP_RK3328 10
#define MMIO_SOC_TYPE_NVIDIA_TEGRA210 11
#define MMIO_SOC_TYPE_NVIDIA_TEGRAX1 12
#define MMIO_SOC_TYPE_TI_OMAP5      13
#define MMIO_SOC_TYPE_AMLOGIC_S905  14
#define MMIO_SOC_TYPE_AMLOGIC_S922X 15
#define MMIO_SOC_TYPE_QUALCOMM_SDM845 16
#define MMIO_SOC_TYPE_MEDIATEK_MT8173 17
#define MMIO_SOC_TYPE_NXP_IMX6      18
#define MMIO_SOC_TYPE_NXP_IMX8      19
#define MMIO_SOC_TYPE_STM32MP1      20
#define MMIO_SOC_TYPE_APPLE_M1      21
#define MMIO_SOC_TYPE_APPLE_M2      22
#define MMIO_SOC_TYPE_BCM2837P      23
#define MMIO_SOC_TYPE_SIFIVE_U74    24

/*
 * ===========================================================================
 * STRUKTUR MMIO
 * ===========================================================================
 */

/* MMIO region info */
typedef struct {
    tak_bertanda32_t id;
    alamat_fisik_t base_address;
    tak_bertanda64_t size;
    tak_bertanda8_t soc_type;
    const char *soc_name;
    bool_t initialized;
} mmio_region_t;

/* MMIO peripheral info */
typedef struct {
    tak_bertanda32_t id;
    tak_bertanda32_t region_id;
    alamat_fisik_t address;
    tak_bertanda32_t size;
    tak_bertanda32_t magic;
    ic_kategori_t kategori;
    char nama[IC_NAMA_PANJANG_MAKS];
    bool_t detected;
} mmio_peripheral_t;

/* MMIO probe context */
typedef struct {
    mmio_region_t regions[MMIO_MAX_REGIONS];
    tak_bertanda32_t region_count;
    mmio_peripheral_t peripherals[MMIO_MAX_REGIONS * MMIO_MAX_DEVICES_PER_REGION];
    tak_bertanda32_t peripheral_count;
    bool_t initialized;
} mmio_probe_context_t;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static mmio_probe_context_t g_mmio_ctx;
static bool_t g_mmio_initialized = SALAH;

/*
 * ===========================================================================
 * DATABASE PERIPHERAL MMIO YANG DIKENALI
 * ===========================================================================
 */

typedef struct {
    tak_bertanda64_t address_mask;
    tak_bertanda64_t address_value;
    tak_bertanda32_t magic;
    const char *nama;
    ic_kategori_t kategori;
    const char *vendor;
} mmio_known_peripheral_t;

static const mmio_known_peripheral_t g_mmio_known_peripherals[] = {
    /* UART */
    { 0xFFFFF000ULL, 0x00000000ULL, MMIO_MAGIC_UART_PL011, "PL011 UART", IC_KATEGORI_LAINNYA, "ARM" },
    { 0xFFFFF000ULL, 0x00001000ULL, MMIO_MAGIC_DW_APB_UART, "DW APB UART", IC_KATEGORI_LAINNYA, "Synopsys" },
    { 0xFFFFF000ULL, 0x00002000ULL, MMIO_MAGIC_UART_NS16550, "NS16550 UART", IC_KATEGORI_LAINNYA, "NS" },
    { 0xFFFFF000ULL, 0x00003000ULL, MMIO_MAGIC_CADENCE_UART, "Cadence UART", IC_KATEGORI_LAINNYA, "Cadence" },
    
    /* GPIO */
    { 0xFFFFF000ULL, 0x00100000ULL, 0x00000000, "GPIO Controller", IC_KATEGORI_LAINNYA, "Generic" },
    { 0xFFFFF000ULL, 0x00101000ULL, MMIO_MAGIC_DW_APB_GPIO, "DW APB GPIO", IC_KATEGORI_LAINNYA, "Synopsys" },
    
    /* I2C */
    { 0xFFFFF000ULL, 0x00200000ULL, MMIO_MAGIC_DW_APB_I2C, "DW APB I2C", IC_KATEGORI_LAINNYA, "Synopsys" },
    { 0xFFFFF000ULL, 0x00201000ULL, MMIO_MAGIC_CADENCE_I2C, "Cadence I2C", IC_KATEGORI_LAINNYA, "Cadence" },
    
    /* SPI */
    { 0xFFFFF000ULL, 0x00300000ULL, MMIO_MAGIC_DW_APB_SPI, "DW APB SPI", IC_KATEGORI_LAINNYA, "Synopsys" },
    { 0xFFFFF000ULL, 0x00301000ULL, MMIO_MAGIC_CADENCE_SPI, "Cadence SPI", IC_KATEGORI_LAINNYA, "Cadence" },
    
    /* USB */
    { 0xFFFF0000ULL, 0x00400000ULL, MMIO_MAGIC_SNPS_DWC2_USB, "DWC2 USB OTG", IC_KATEGORI_USB, "Synopsys" },
    { 0xFFFF0000ULL, 0x00410000ULL, MMIO_MAGIC_SNPS_DWC3_USB, "DWC3 USB 3.0", IC_KATEGORI_USB, "Synopsys" },
    { 0xFFFF0000ULL, 0x00420000ULL, 0x00000000, "EHCI USB 2.0", IC_KATEGORI_USB, "Generic" },
    { 0xFFFF0000ULL, 0x00430000ULL, 0x00000000, "OHCI USB 1.1", IC_KATEGORI_USB, "Generic" },
    { 0xFFFF0000ULL, 0x00440000ULL, 0x00000000, "xHCI USB 3.0", IC_KATEGORI_USB, "Generic" },
    
    /* Ethernet */
    { 0xFFFF0000ULL, 0x00500000ULL, MMIO_MAGIC_SYNOPSYS_ETH, "DW Ethernet GMAC", IC_KATEGORI_NETWORK, "Synopsys" },
    { 0xFFFF0000ULL, 0x00510000ULL, 0x00000000, "DesignWare Ethernet", IC_KATEGORI_NETWORK, "Synopsys" },
    { 0xFFFF0000ULL, 0x00520000ULL, 0x00000000, "FEC Ethernet", IC_KATEGORI_NETWORK, "NXP" },
    { 0xFFFF0000ULL, 0x00530000ULL, 0x00000000, "RTL8169 Ethernet", IC_KATEGORI_NETWORK, "Realtek" },
    { 0xFFFF0000ULL, 0x00540000ULL, 0x00000000, "E1000 Ethernet", IC_KATEGORI_NETWORK, "Intel" },
    
    /* SDHCI / eMMC */
    { 0xFFFFF000ULL, 0x00600000ULL, 0x00000000, "SDHCI Controller", IC_KATEGORI_STORAGE, "Generic" },
    { 0xFFFFF000ULL, 0x00601000ULL, 0x00000000, "DW SDHCI", IC_KATEGORI_STORAGE, "Synopsys" },
    { 0xFFFFF000ULL, 0x00602000ULL, 0x00000000, "SDHCI eMMC", IC_KATEGORI_STORAGE, "Generic" },
    
    /* Display / GPU */
    { 0xFFFF0000ULL, 0x00700000ULL, 0x00000000, "Display Controller", IC_KATEGORI_DISPLAY, "Generic" },
    { 0xFFFF0000ULL, 0x00710000ULL, 0x00000000, "HDMI Controller", IC_KATEGORI_HDMI, "Generic" },
    { 0xFFFF0000ULL, 0x00720000ULL, 0x00000000, "DSI Controller", IC_KATEGORI_DISPLAY, "Generic" },
    { 0xFFFF0000ULL, 0x00800000ULL, 0x00000000, "GPU Mali", IC_KATEGORI_GPU, "ARM" },
    { 0xFFFF0000ULL, 0x00810000ULL, 0x00000000, "GPU Adreno", IC_KATEGORI_GPU, "Qualcomm" },
    { 0xFFFF0000ULL, 0x00820000ULL, 0x00000000, "GPU PowerVR", IC_KATEGORI_GPU, "Imagination" },
    { 0xFFFF0000ULL, 0x00830000ULL, 0x00000000, "GPU Vivante", IC_KATEGORI_GPU, "Vivante" },
    
    /* Video codecs */
    { 0xFFFF0000ULL, 0x00900000ULL, 0x00000000, "Video Decoder", IC_KATEGORI_LAINNYA, "Generic" },
    { 0xFFFF0000ULL, 0x00910000ULL, 0x00000000, "Video Encoder", IC_KATEGORI_LAINNYA, "Generic" },
    { 0xFFFF0000ULL, 0x00920000ULL, 0x00000000, "VPU", IC_KATEGORI_LAINNYA, "Generic" },
    
    /* Audio */
    { 0xFFFF0000ULL, 0x00A00000ULL, 0x00000000, "I2S Controller", IC_KATEGORI_AUDIO, "Generic" },
    { 0xFFFF0000ULL, 0x00A10000ULL, 0x00000000, "SPDIF Controller", IC_KATEGORI_AUDIO, "Generic" },
    { 0xFFFF0000ULL, 0x00A20000ULL, 0x00000000, "PDM Controller", IC_KATEGORI_AUDIO, "Generic" },
    { 0xFFFF0000ULL, 0x00A30000ULL, 0x00000000, "Audio DSP", IC_KATEGORI_AUDIO, "Generic" },
    
    /* Camera */
    { 0xFFFF0000ULL, 0x00B00000ULL, 0x00000000, "Camera CSI", IC_KATEGORI_CAMERA, "Generic" },
    { 0xFFFF0000ULL, 0x00B10000ULL, 0x00000000, "ISP", IC_KATEGORI_CAMERA, "Generic" },
    { 0xFFFF0000ULL, 0x00B20000ULL, 0x00000000, "MIPI CSI-2", IC_KATEGORI_CAMERA, "Generic" },
    
    /* PCIe */
    { 0xFFFF0000ULL, 0x00C00000ULL, 0x00000000, "PCIe Controller", IC_KATEGORI_BRIDGE, "Generic" },
    
    /* SATA */
    { 0xFFFF0000ULL, 0x00D00000ULL, 0x00000000, "AHCI SATA", IC_KATEGORI_STORAGE, "Generic" },
    
    /* Timer */
    { 0xFFFFF000ULL, 0x00E00000ULL, 0x00000000, "Timer", IC_KATEGORI_TIMER, "Generic" },
    { 0xFFFFF000ULL, 0x00E01000ULL, MMIO_MAGIC_DW_APB_TIM, "DW APB Timer", IC_KATEGORI_TIMER, "Synopsys" },
    
    /* Watchdog */
    { 0xFFFFF000ULL, 0x00F00000ULL, 0x00000000, "Watchdog", IC_KATEGORI_LAINNYA, "Generic" },
    
    /* RTC */
    { 0xFFFFF000ULL, 0x01000000ULL, 0x00000000, "RTC", IC_KATEGORI_TIMER, "Generic" },
    
    /* PWM */
    { 0xFFFFF000ULL, 0x01100000ULL, 0x00000000, "PWM Controller", IC_KATEGORI_LAINNYA, "Generic" },
    
    /* ADC */
    { 0xFFFFF000ULL, 0x01200000ULL, 0x00000000, "ADC", IC_KATEGORI_SENSOR, "Generic" },
    
    /* DMA */
    { 0xFFFF0000ULL, 0x01300000ULL, MMIO_MAGIC_DW_DMA, "DW DMA", IC_KATEGORI_DMA, "Synopsys" },
    { 0xFFFF0000ULL, 0x01310000ULL, MMIO_MAGIC_ARM_PL330_DMA, "PL330 DMA", IC_KATEGORI_DMA, "ARM" },
    
    /* GIC (Generic Interrupt Controller) */
    { 0xFFFF0000ULL, 0x02000000ULL, MMIO_MAGIC_ARM_GIC, "GIC-400", IC_KATEGORI_LAINNYA, "ARM" },
    { 0xFFFF0000ULL, 0x02010000ULL, MMIO_MAGIC_ARM_GIC_V3, "GIC-500/GIC-600", IC_KATEGORI_LAINNYA, "ARM" },
    
    /* SMMU */
    { 0xFFFF0000ULL, 0x02100000ULL, MMIO_MAGIC_ARM_SMMU, "SMMU", IC_KATEGORI_LAINNYA, "ARM" },
};

#define MMIO_KNOWN_PERIPHERALS_COUNT (sizeof(g_mmio_known_peripherals) / sizeof(g_mmio_known_peripherals[0]))

/*
 * ===========================================================================
 * DATABASE SOC YANG DIKENALI
 * ===========================================================================
 */

typedef struct {
    tak_bertanda8_t type;
    const char *nama;
    const char *vendor;
    alamat_fisik_t base_address;
    tak_bertanda64_t size;
} mmio_known_soc_t;

static const mmio_known_soc_t g_mmio_known_socs[] = {
    { MMIO_SOC_TYPE_RPI_BCM2835, "BCM2835", "Broadcom", MMIO_BASE_RASPBERRY_PI, 0x01000000ULL },
    { MMIO_SOC_TYPE_RPI_BCM2836, "BCM2836", "Broadcom", MMIO_BASE_RASPBERRY_PI, 0x01000000ULL },
    { MMIO_SOC_TYPE_RPI_BCM2837, "BCM2837", "Broadcom", MMIO_BASE_RASPBERRY_PI, 0x01000000ULL },
    { MMIO_SOC_TYPE_RPI_BCM2711, "BCM2711", "Broadcom", MMIO_BASE_RASPBERRY_PI4, 0x02000000ULL },
    { MMIO_SOC_TYPE_ALLWINNER_A64, "Allwinner A64", "Allwinner", MMIO_BASE_ALLWINNER_A64, 0x04000000ULL },
    { MMIO_SOC_TYPE_ALLWINNER_H3, "Allwinner H3", "Allwinner", MMIO_BASE_ALLWINNER_H3, 0x04000000ULL },
    { MMIO_SOC_TYPE_ROCKCHIP_RK3288, "RK3288", "Rockchip", MMIO_BASE_ROCKCHIP_RK3288, 0x01000000ULL },
    { MMIO_SOC_TYPE_ROCKCHIP_RK3399, "RK3399", "Rockchip", MMIO_BASE_ROCKCHIP_RK3399, 0x02000000ULL },
    { MMIO_SOC_TYPE_NVIDIA_TEGRA210, "Tegra210", "NVIDIA", MMIO_BASE_NVIDIA_TEGRA, 0x04000000ULL },
    { MMIO_SOC_TYPE_NVIDIA_TEGRAX1, "Tegra X1", "NVIDIA", MMIO_BASE_NVIDIA_TEGRA, 0x04000000ULL },
    { MMIO_SOC_TYPE_TI_OMAP5, "OMAP5", "TI", MMIO_BASE_TI_OMAP, 0x02000000ULL },
    { MMIO_SOC_TYPE_AMLOGIC_S905, "S905", "Amlogic", MMIO_BASE_AMLOGIC_MESON, 0x04000000ULL },
    { MMIO_SOC_TYPE_AMLOGIC_S922X, "S922X", "Amlogic", MMIO_BASE_AMLOGIC_MESON, 0x04000000ULL },
    { MMIO_SOC_TYPE_QUALCOMM_SDM845, "SDM845", "Qualcomm", MMIO_BASE_SNAPDRAGON, 0x10000000ULL },
    { MMIO_SOC_TYPE_MEDIATEK_MT8173, "MT8173", "MediaTek", MMIO_MTK_BASE, 0x04000000ULL },
    { MMIO_SOC_TYPE_NXP_IMX6, "i.MX6", "NXP", MMIO_IMX_BASE, 0x04000000ULL },
    { MMIO_SOC_TYPE_NXP_IMX8, "i.MX8", "NXP", MMIO_IMX_BASE, 0x08000000ULL },
    { MMIO_SOC_TYPE_STM32MP1, "STM32MP1", "ST", MMIO_STM32_BASE, 0x04000000ULL },
    { MMIO_SOC_TYPE_APPLE_M1, "Apple M1", "Apple", MMIO_APPLE_M1, 0x400000000ULL },
};

#define MMIO_KNOWN_SOCS_COUNT (sizeof(g_mmio_known_socs) / sizeof(g_mmio_known_socs[0]))

/*
 * ===========================================================================
 * FUNGSI DETEKSI MMIO
 * ===========================================================================
 */

/*
 * mmio_read32 - Baca 32-bit dari alamat MMIO
 */
static __inline__ tak_bertanda32_t mmio_read32(alamat_fisik_t alamat)
{
    volatile tak_bertanda32_t *ptr;
    ptr = (volatile tak_bertanda32_t *)(uintptr_t)(tak_bertanda32_t)alamat;
    return *ptr;
}

/*
 * mmio_write32 - Tulis 32-bit ke alamat MMIO
 */
static __inline__ void mmio_write32(alamat_fisik_t alamat, tak_bertanda32_t nilai)
{
    volatile tak_bertanda32_t *ptr;
    ptr = (volatile tak_bertanda32_t *)(uintptr_t)(tak_bertanda32_t)alamat;
    *ptr = nilai;
}

/*
 * mmio_identifikasi_peripheral - Identifikasi peripheral berdasarkan alamat dan magic
 */
static const mmio_known_peripheral_t *mmio_identifikasi_peripheral(alamat_fisik_t alamat,
                                                                    tak_bertanda32_t magic)
{
    tak_bertanda32_t i;
    
    for (i = 0; i < MMIO_KNOWN_PERIPHERALS_COUNT; i++) {
        tak_bertanda64_t masked = alamat & g_mmio_known_peripherals[i].address_mask;
        if (masked == g_mmio_known_peripherals[i].address_value) {
            if (g_mmio_known_peripherals[i].magic == 0 ||
                g_mmio_known_peripherals[i].magic == magic) {
                return &g_mmio_known_peripherals[i];
            }
        }
    }
    
    return NULL;
}

/*
 * mmio_detect_soc - Deteksi jenis SoC
 */
static tak_bertanda8_t mmio_detect_soc(void)
{
    /* Deteksi SoC berdasarkan board info atau register tertentu */
    /* Untuk sekarang, kembalikan default */
    return MMIO_SOC_TYPE_RPI_BCM2837;
}

/*
 * mmio_detect_regions - Deteksi MMIO regions
 */
static status_t mmio_detect_regions(void)
{
    tak_bertanda32_t i;
    tak_bertanda8_t soc_type;
    const mmio_known_soc_t *soc_info;
    
    g_mmio_ctx.region_count = 0;
    
    /* Deteksi SoC */
    soc_type = mmio_detect_soc();
    
    /* Cari info SoC */
    soc_info = NULL;
    for (i = 0; i < MMIO_KNOWN_SOCS_COUNT; i++) {
        if (g_mmio_known_socs[i].type == soc_type) {
            soc_info = &g_mmio_known_socs[i];
            break;
        }
    }
    
    /* Tambahkan region untuk SoC yang terdeteksi */
    if (soc_info != NULL) {
        g_mmio_ctx.regions[0].id = 0;
        g_mmio_ctx.regions[0].base_address = soc_info->base_address;
        g_mmio_ctx.regions[0].size = soc_info->size;
        g_mmio_ctx.regions[0].soc_type = soc_type;
        g_mmio_ctx.regions[0].soc_name = soc_info->nama;
        g_mmio_ctx.regions[0].initialized = BENAR;
        g_mmio_ctx.region_count = 1;
    }
    
    /* Tambahkan region generik */
    if (g_mmio_ctx.region_count < MMIO_MAX_REGIONS) {
        g_mmio_ctx.regions[g_mmio_ctx.region_count].id = g_mmio_ctx.region_count;
        g_mmio_ctx.regions[g_mmio_ctx.region_count].base_address = 0xFF000000ULL;
        g_mmio_ctx.regions[g_mmio_ctx.region_count].size = 0x01000000ULL;
        g_mmio_ctx.regions[g_mmio_ctx.region_count].soc_type = MMIO_SOC_TYPE_UNKNOWN;
        g_mmio_ctx.regions[g_mmio_ctx.region_count].soc_name = "Generic MMIO";
        g_mmio_ctx.regions[g_mmio_ctx.region_count].initialized = BENAR;
        g_mmio_ctx.region_count++;
    }
    
    return STATUS_BERHASIL;
}

/*
 * mmio_probe_address - Probe satu alamat MMIO
 */
static bool_t mmio_probe_address(alamat_fisik_t alamat, tak_bertanda32_t *magic)
{
    tak_bertanda32_t nilai;
    
    /* Baca nilai dari alamat MMIO */
    nilai = mmio_read32(alamat);
    
    /* Cek apakah valid */
    if (nilai != 0x00000000 && nilai != 0xFFFFFFFF) {
        *magic = nilai;
        return BENAR;
    }
    
    *magic = nilai;
    return SALAH;
}

/*
 * mmio_scan_region - Scan satu MMIO region
 */
static status_t mmio_scan_region(tak_bertanda32_t region_id, tanda32_t *jumlah_ditemukan)
{
    alamat_fisik_t alamat;
    alamat_fisik_t alamat_akhir;
    tak_bertanda32_t magic;
    const mmio_known_peripheral_t *periph_info;
    mmio_peripheral_t *peripheral;
    ic_perangkat_t *perangkat;
    
    if (jumlah_ditemukan == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (region_id >= g_mmio_ctx.region_count) {
        return STATUS_PARAM_INVALID;
    }
    
    *jumlah_ditemukan = 0;
    
    alamat = g_mmio_ctx.regions[region_id].base_address;
    alamat_akhir = alamat + g_mmio_ctx.regions[region_id].size;
    
    /* Scan setiap offset peripheral yang dikenali */
    while (alamat < alamat_akhir) {
        if (mmio_probe_address(alamat, &magic)) {
            /* Perangkat ditemukan */
            peripheral = &g_mmio_ctx.peripherals[g_mmio_ctx.peripheral_count];
            peripheral->id = g_mmio_ctx.peripheral_count;
            peripheral->region_id = region_id;
            peripheral->address = alamat;
            peripheral->magic = magic;
            peripheral->detected = BENAR;
            
            /* Identifikasi peripheral */
            periph_info = mmio_identifikasi_peripheral(alamat, magic);
            if (periph_info != NULL) {
                peripheral->kategori = periph_info->kategori;
                ic_salinnama(peripheral->nama, periph_info->nama, IC_NAMA_PANJANG_MAKS);
            } else {
                peripheral->kategori = IC_KATEGORI_LAINNYA;
                ic_salinnama(peripheral->nama, "Unknown MMIO Peripheral", IC_NAMA_PANJANG_MAKS);
            }
            
            /* Tambahkan ke daftar perangkat global */
            perangkat = ic_probe_tambah_perangkat(IC_BUS_MMIO,
                                                   (tak_bertanda8_t)region_id,
                                                   (tak_bertanda8_t)((alamat >> 12) & 0xFF),
                                                   (tak_bertanda8_t)((alamat >> 20) & 0xFF));
            if (perangkat != NULL) {
                perangkat->vendor_id = (tak_bertanda16_t)((magic >> 16) & 0xFFFF);
                perangkat->device_id = (tak_bertanda16_t)(magic & 0xFFFF);
                perangkat->bar[0] = alamat;
                perangkat->terdeteksi = BENAR;
                perangkat->status = IC_STATUS_TERDETEKSI;
                
                if (perangkat->entri != NULL) {
                    perangkat->entri->kategori = peripheral->kategori;
                    ic_salinnama(perangkat->entri->nama, peripheral->nama, IC_NAMA_PANJANG_MAKS);
                }
            }
            
            g_mmio_ctx.peripheral_count++;
            (*jumlah_ditemukan)++;
            
            /* Cek kapasitas */
            if (g_mmio_ctx.peripheral_count >= (MMIO_MAX_REGIONS * MMIO_MAX_DEVICES_PER_REGION)) {
                break;
            }
        }
        
        /* Skip ke offset berikutnya (4KB aligned) */
        alamat += 0x1000ULL;
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI UTAMA
 * ===========================================================================
 */

/*
 * ic_probe_mmio_init - Inisialisasi probe MMIO
 */
status_t ic_probe_mmio_init(void)
{
    if (g_mmio_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
    /* Reset context */
    g_mmio_ctx.region_count = 0;
    g_mmio_ctx.peripheral_count = 0;
    g_mmio_ctx.initialized = SALAH;
    
    /* Deteksi MMIO regions */
    mmio_detect_regions();
    
    g_mmio_ctx.initialized = BENAR;
    g_mmio_initialized = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * ic_probe_mmio - Scan MMIO space
 */
tanda32_t ic_probe_mmio(void)
{
    tanda32_t total_ditemukan = 0;
    tanda32_t ditemukan_region;
    tak_bertanda32_t region_id;
    status_t hasil;
    
    if (!g_probe_diinisialisasi) {
        return -1;
    }
    
    /* Inisialisasi jika belum */
    if (!g_mmio_initialized) {
        hasil = ic_probe_mmio_init();
        if (hasil != STATUS_BERHASIL) {
            return -1;
        }
    }
    
    /* Scan setiap region */
    for (region_id = 0; region_id < g_mmio_ctx.region_count; region_id++) {
        hasil = mmio_scan_region(region_id, &ditemukan_region);
        if (hasil == STATUS_BERHASIL) {
            total_ditemukan += ditemukan_region;
        }
    }
    
    return total_ditemukan;
}

/*
 * ic_probe_mmio_get_peripheral_count - Dapatkan jumlah peripheral MMIO terdeteksi
 */
tak_bertanda32_t ic_probe_mmio_get_peripheral_count(void)
{
    return g_mmio_ctx.peripheral_count;
}

/*
 * ic_probe_mmio_get_region_count - Dapatkan jumlah MMIO region
 */
tak_bertanda32_t ic_probe_mmio_get_region_count(void)
{
    return g_mmio_ctx.region_count;
}

/*
 * ic_probe_mmio_get_peripheral_info - Dapatkan info peripheral MMIO
 */
mmio_peripheral_t *ic_probe_mmio_get_peripheral_info(tak_bertanda32_t index)
{
    if (index >= g_mmio_ctx.peripheral_count) {
        return NULL;
    }
    
    return &g_mmio_ctx.peripherals[index];
}

/*
 * ic_probe_mmio_get_region_info - Dapatkan info MMIO region
 */
mmio_region_t *ic_probe_mmio_get_region_info(tak_bertanda32_t index)
{
    if (index >= g_mmio_ctx.region_count) {
        return NULL;
    }
    
    return &g_mmio_ctx.regions[index];
}
