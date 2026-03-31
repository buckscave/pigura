/*
 * PIGURA OS - WIFI.C
 * ===================
 * Implementasi manajemen WiFi.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi untuk menangani
 * konektivitas WiFi dalam Pigura OS.
 *
 * Kepatuhan standar:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Mendukung semua arsitektur
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "jaringan.h"
#include "../../inti/kernel.h"

/*
 * ===========================================================================
 * KONSTANTA WIFI
 * ===========================================================================
 */

/* WiFi channels */
#define WIFI_CHANNEL_MIN        1
#define WIFI_CHANNEL_MAX        14
#define WIFI_CHANNEL_5GHZ_MIN   36

/* WiFi frequency bands */
#define WIFI_BAND_2_4GHZ        1
#define WIFI_BAND_5GHZ          2

/* WiFi authentication */
#define WIFI_AUTH_OPEN          0
#define WIFI_AUTH_WEP           1
#define WIFI_AUTH_WPA_PSK       2
#define WIFI_AUTH_WPA2_PSK      3
#define WIFI_AUTH_WPA3_SAE      4

/* Scan timeout (ms) */
#define WIFI_SCAN_TIMEOUT       10000

/* Connection timeout (ms) */
#define WIFI_CONNECT_TIMEOUT    30000

/* Maximum SSID length */
#define WIFI_SSID_MAX           32

/* Maximum password length */
#define WIFI_PASSWORD_MAX       64

/* Maximum scan results */
#define WIFI_SCAN_MAX           64

/*
 * ===========================================================================
 * STRUKTUR WIFI INTERNAL
 * ===========================================================================
 */

/* WiFi interface state */
typedef struct {
    tak_bertanda32_t status;
    char ssid[WIFI_SSID_MAX + 1];
    alamat_mac_t bssid;
    tak_bertanda16_t channel;
    tak_bertanda16_t frequency;
    tanda32_t rssi;
    tak_bertanda32_t kecepatan;
    tak_bertanda32_t keamanan;
    tak_bertanda32_t auth_type;
    tak_bertanda32_t iface_id;
    bool_t connected;
    bool_t scanning;
} wifi_iface_t;

/* Scan result internal */
typedef struct {
    wifi_scan_result_t result;
    bool_t valid;
} wifi_scan_entry_t;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

/* WiFi interfaces */
#define WIFI_IFACE_MAKS     4
static wifi_iface_t g_wifi_ifaces[WIFI_IFACE_MAKS];

/* Scan results */
static wifi_scan_entry_t g_wifi_scan_results[WIFI_SCAN_MAX];
static tak_bertanda32_t g_wifi_scan_count = 0;

/* Statistics */
static tak_bertanda64_t g_wifi_scans = 0;
static tak_bertanda64_t g_wifi_connections = 0;
static tak_bertanda64_t g_wifi_disconnections = 0;
static tak_bertanda64_t g_wifi_tx_packets = 0;
static tak_bertanda64_t g_wifi_rx_packets = 0;
static tak_bertanda64_t g_wifi_errors = 0;

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * wifi_find_iface - Cari interface berdasarkan ID
 */
static wifi_iface_t *wifi_find_iface(tak_bertanda32_t iface_id)
{
    tak_bertanda32_t i;

    for (i = 0; i < WIFI_IFACE_MAKS; i++) {
        if (g_wifi_ifaces[i].iface_id == iface_id) {
            return &g_wifi_ifaces[i];
        }
    }

    return NULL;
}

/*
 * wifi_find_free_iface - Cari slot interface kosong
 */
static wifi_iface_t *wifi_find_free_iface(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < WIFI_IFACE_MAKS; i++) {
        if (g_wifi_ifaces[i].iface_id == 0) {
            return &g_wifi_ifaces[i];
        }
    }

    return NULL;
}

/*
 * wifi_channel_to_freq - Konversi channel ke frekuensi
 */
static tak_bertanda16_t wifi_channel_to_freq(tak_bertanda16_t channel)
{
    /* 2.4 GHz band */
    if (channel >= 1 && channel <= 14) {
        if (channel == 14) {
            return 2484;
        }
        return (tak_bertanda16_t)(2407 + channel * 5);
    }

    /* 5 GHz band (simplified) */
    if (channel >= 36 && channel <= 165) {
        return (tak_bertanda16_t)(5000 + channel * 5);
    }

    return 0;
}

/*
 * wifi_freq_to_channel - Konversi frekuensi ke channel
 */
static tak_bertanda16_t wifi_freq_to_channel(tak_bertanda16_t freq)
{
    /* 2.4 GHz band */
    if (freq >= 2412 && freq <= 2484) {
        if (freq == 2484) {
            return 14;
        }
        return (tak_bertanda16_t)((freq - 2407) / 5);
    }

    /* 5 GHz band */
    if (freq >= 5180 && freq <= 5825) {
        return (tak_bertanda16_t)((freq - 5000) / 5);
    }

    return 0;
}

/*
 * wifi_rssi_to_quality - Konversi RSSI ke kualitas sinyal
 */
static tak_bertanda32_t wifi_rssi_to_quality(tanda32_t rssi)
{
    tak_bertanda32_t quality;

    /* RSSI typically ranges from -100 (weak) to -30 (strong) */
    if (rssi >= -50) {
        quality = 100;
    } else if (rssi >= -100) {
        quality = (tak_bertanda32_t)(rssi + 100) * 2;
    } else {
        quality = 0;
    }

    return quality;
}

/*
 * wifi_add_scan_result - Tambah hasil scan
 */
static status_t wifi_add_scan_result(const wifi_scan_result_t *result)
{
    tak_bertanda32_t i;

    if (result == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Check if already exists */
    for (i = 0; i < WIFI_SCAN_MAX; i++) {
        if (g_wifi_scan_results[i].valid &&
            kernel_strcmp(g_wifi_scan_results[i].result.ssid,
                          result->ssid) == 0) {
            /* Update existing */
            kernel_memcpy(&g_wifi_scan_results[i].result, result,
                          sizeof(wifi_scan_result_t));
            return STATUS_BERHASIL;
        }
    }

    /* Find free slot */
    for (i = 0; i < WIFI_SCAN_MAX; i++) {
        if (!g_wifi_scan_results[i].valid) {
            kernel_memcpy(&g_wifi_scan_results[i].result, result,
                          sizeof(wifi_scan_result_t));
            g_wifi_scan_results[i].valid = BENAR;
            g_wifi_scan_count++;
            return STATUS_BERHASIL;
        }
    }

    return STATUS_PENUH;
}

/*
 * wifi_clear_scan_results - Hapus semua hasil scan
 */
static void wifi_clear_scan_results(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < WIFI_SCAN_MAX; i++) {
        g_wifi_scan_results[i].valid = SALAH;
    }

    g_wifi_scan_count = 0;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * wifi_init - Inisialisasi WiFi
 */
status_t wifi_init(void)
{
    tak_bertanda32_t i;

    /* Reset interfaces */
    for (i = 0; i < WIFI_IFACE_MAKS; i++) {
        g_wifi_ifaces[i].iface_id = 0;
        g_wifi_ifaces[i].status = WIFI_STATUS_DISCONNECTED;
        g_wifi_ifaces[i].ssid[0] = '\0';
        g_wifi_ifaces[i].connected = SALAH;
        g_wifi_ifaces[i].scanning = SALAH;
    }

    /* Clear scan results */
    wifi_clear_scan_results();

    /* Reset statistik */
    g_wifi_scans = 0;
    g_wifi_connections = 0;
    g_wifi_disconnections = 0;
    g_wifi_tx_packets = 0;
    g_wifi_rx_packets = 0;
    g_wifi_errors = 0;

    return STATUS_BERHASIL;
}

/*
 * wifi_scan - Scan jaringan WiFi
 */
tanda32_t wifi_scan(netdev_t *dev, wifi_scan_result_t *results,
                    tak_bertanda32_t max_results)
{
    (void)dev;
    tak_bertanda32_t i;
    tak_bertanda32_t count;

    if (results == NULL || max_results == 0) {
        return -1;
    }

    /* Clear previous results */
    wifi_clear_scan_results();

    g_wifi_scans++;

    /* Simulated scan results for demonstration */
    /* In real implementation, would trigger hardware scan */

    /* Add some dummy results */
    {
        wifi_scan_result_t dummy;

        kernel_memset(&dummy, 0, sizeof(dummy));

        /* Network 1 */
        kernel_strncpy(dummy.ssid, "Network1", 8);
        dummy.bssid.byte[0] = 0x00;
        dummy.bssid.byte[1] = 0x11;
        dummy.bssid.byte[2] = 0x22;
        dummy.bssid.byte[3] = 0x33;
        dummy.bssid.byte[4] = 0x44;
        dummy.bssid.byte[5] = 0x55;
        dummy.channel = 6;
        dummy.frekuensi = 2437;
        dummy.keamanan = WIFI_KEAMANAN_WPA2;
        dummy.rssi = -55;
        dummy.tipe = WIFI_TIPE_INFRASTRUKTUR;
        dummy.terenkripsi = BENAR;
        wifi_add_scan_result(&dummy);

        /* Network 2 */
        kernel_strncpy(dummy.ssid, "Network2", 8);
        dummy.bssid.byte[5] = 0x66;
        dummy.channel = 11;
        dummy.frekuensi = 2462;
        dummy.keamanan = WIFI_KEAMANAN_WPA;
        dummy.rssi = -70;
        wifi_add_scan_result(&dummy);

        /* Network 3 */
        kernel_strncpy(dummy.ssid, "OpenNetwork", 11);
        dummy.bssid.byte[5] = 0x77;
        dummy.channel = 1;
        dummy.frekuensi = 2412;
        dummy.keamanan = WIFI_KEAMANAN_TIDAK_ADA;
        dummy.rssi = -80;
        dummy.terenkripsi = SALAH;
        wifi_add_scan_result(&dummy);
    }

    /* Copy results to output buffer */
    count = 0;

    for (i = 0; i < WIFI_SCAN_MAX && count < max_results; i++) {
        if (g_wifi_scan_results[i].valid) {
            kernel_memcpy(&results[count], &g_wifi_scan_results[i].result,
                          sizeof(wifi_scan_result_t));
            count++;
        }
    }

    return (tanda32_t)count;
}

/*
 * wifi_connect - Connect ke jaringan WiFi
 */
status_t wifi_connect(netdev_t *dev, wifi_config_t *config)
{
    wifi_iface_t *iface;

    if (dev == NULL || config == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Find or create interface state */
    iface = wifi_find_iface(dev->id);

    if (iface == NULL) {
        iface = wifi_find_free_iface();
        if (iface == NULL) {
            return STATUS_PENUH;
        }
        iface->iface_id = dev->id;
    }

    /* Check if already connected */
    if (iface->connected) {
        /* Disconnect first */
        wifi_disconnect(dev);
    }

    /* Set connecting state */
    iface->status = WIFI_STATUS_AUTHENTICATING;
    iface->scanning = SALAH;

    /* Copy SSID */
    {
        ukuran_t ssid_len = kernel_strlen(config->ssid);
        if (ssid_len >= WIFI_SSID_MAX) {
            ssid_len = WIFI_SSID_MAX - 1;
        }
        kernel_strncpy(iface->ssid, config->ssid, ssid_len);
        iface->ssid[ssid_len] = '\0';
    }

    /* Set security type */
    iface->keamanan = config->keamanan;
    iface->auth_type = config->keamanan;

    /* Find network in scan results */
    {
        tak_bertanda32_t i;
        bool_t found = SALAH;

        for (i = 0; i < WIFI_SCAN_MAX; i++) {
            if (g_wifi_scan_results[i].valid &&
                kernel_strcmp(g_wifi_scan_results[i].result.ssid,
                              config->ssid) == 0) {
                found = BENAR;

                /* Copy BSSID */
                kernel_memcpy(&iface->bssid,
                              &g_wifi_scan_results[i].result.bssid,
                              sizeof(alamat_mac_t));

                /* Set channel and frequency */
                iface->channel = g_wifi_scan_results[i].result.channel;
                iface->frequency = g_wifi_scan_results[i].result.frekuensi;
                iface->rssi = g_wifi_scan_results[i].result.rssi;
                break;
            }
        }

        if (!found && !config->hidden) {
            iface->status = WIFI_STATUS_DISCONNECTED;
            return STATUS_TIDAK_DITEMUKAN;
        }
    }

    /* Authentication process (simplified) */
    /* In real implementation, would handle 802.11 authentication */

    iface->status = WIFI_STATUS_ASSOCIATING;

    /* Complete connection */
    iface->status = WIFI_STATUS_CONNECTED;
    iface->connected = BENAR;
    iface->kecepatan = 54;  /* Default 54 Mbps */

    /* Set device flags */
    dev->flags |= NETDEV_FLAG_UP | NETDEV_FLAG_RUNNING | NETDEV_FLAG_WIFI;
    dev->status = NETDEV_STATUS_RUNNING;

    g_wifi_connections++;

    return STATUS_BERHASIL;
}

/*
 * wifi_disconnect - Disconnect dari WiFi
 */
status_t wifi_disconnect(netdev_t *dev)
{
    wifi_iface_t *iface;

    if (dev == NULL) {
        return STATUS_PARAM_NULL;
    }

    iface = wifi_find_iface(dev->id);

    if (iface == NULL || !iface->connected) {
        return STATUS_BERHASIL;
    }

    /* Clear state */
    iface->status = WIFI_STATUS_DISCONNECTED;
    iface->connected = SALAH;
    iface->ssid[0] = '\0';
    kernel_memset(&iface->bssid, 0, sizeof(alamat_mac_t));
    iface->channel = 0;
    iface->frequency = 0;
    iface->rssi = 0;
    iface->kecepatan = 0;

    /* Clear device flags */
    dev->flags &= ~(NETDEV_FLAG_UP | NETDEV_FLAG_RUNNING);
    dev->status = NETDEV_STATUS_DOWN;

    g_wifi_disconnections++;

    return STATUS_BERHASIL;
}

/*
 * wifi_status - Dapatkan status WiFi
 */
status_t wifi_status(netdev_t *dev, wifi_status_t *status)
{
    wifi_iface_t *iface;

    if (dev == NULL || status == NULL) {
        return STATUS_PARAM_NULL;
    }

    iface = wifi_find_iface(dev->id);

    if (iface == NULL) {
        status->status = WIFI_STATUS_DISCONNECTED;
        kernel_memset(status, 0, sizeof(wifi_status_t));
        return STATUS_BERHASIL;
    }

    /* Fill status */
    status->status = iface->status;
    kernel_strncpy(status->ssid, iface->ssid, 31);
    status->ssid[31] = '\0';
    kernel_memcpy(&status->bssid, &iface->bssid, sizeof(alamat_mac_t));
    status->channel = iface->channel;
    status->rssi = iface->rssi;
    status->kecepatan = iface->kecepatan;

    /* Get IP from device */
    kernel_memcpy(&status->ipv4, &dev->ipv4, sizeof(alamat_ipv4_t));

    /* Statistics */
    status->tx_packets = (tak_bertanda32_t)g_wifi_tx_packets;
    status->rx_packets = (tak_bertanda32_t)g_wifi_rx_packets;

    return STATUS_BERHASIL;
}

/*
 * wifi_get_rssi - Dapatkan kekuatan sinyal
 */
tanda32_t wifi_get_rssi(netdev_t *dev)
{
    wifi_iface_t *iface;

    if (dev == NULL) {
        return -100;
    }

    iface = wifi_find_iface(dev->id);

    if (iface == NULL || !iface->connected) {
        return -100;
    }

    return iface->rssi;
}

/*
 * wifi_get_signal_quality - Dapatkan kualitas sinyal (0-100)
 */
tak_bertanda32_t wifi_get_signal_quality(netdev_t *dev)
{
    wifi_iface_t *iface;

    if (dev == NULL) {
        return 0;
    }

    iface = wifi_find_iface(dev->id);

    if (iface == NULL || !iface->connected) {
        return 0;
    }

    return wifi_rssi_to_quality(iface->rssi);
}

/*
 * wifi_get_channel - Dapatkan channel
 */
tak_bertanda16_t wifi_get_channel(netdev_t *dev)
{
    wifi_iface_t *iface;

    if (dev == NULL) {
        return 0;
    }

    iface = wifi_find_iface(dev->id);

    if (iface == NULL) {
        return 0;
    }

    return iface->channel;
}

/*
 * wifi_get_frequency - Dapatkan frekuensi
 */
tak_bertanda16_t wifi_get_frequency(netdev_t *dev)
{
    wifi_iface_t *iface;

    if (dev == NULL) {
        return 0;
    }

    iface = wifi_find_iface(dev->id);

    if (iface == NULL) {
        return 0;
    }

    return iface->frequency;
}

/*
 * wifi_is_connected - Cek apakah terkoneksi
 */
bool_t wifi_is_connected(netdev_t *dev)
{
    wifi_iface_t *iface;

    if (dev == NULL) {
        return SALAH;
    }

    iface = wifi_find_iface(dev->id);

    if (iface == NULL) {
        return SALAH;
    }

    return iface->connected;
}

/*
 * wifi_get_ssid - Dapatkan SSID
 */
const char *wifi_get_ssid(netdev_t *dev)
{
    wifi_iface_t *iface;

    if (dev == NULL) {
        return NULL;
    }

    iface = wifi_find_iface(dev->id);

    if (iface == NULL || !iface->connected) {
        return NULL;
    }

    return iface->ssid;
}

/*
 * wifi_get_bssid - Dapatkan BSSID
 */
status_t wifi_get_bssid(netdev_t *dev, alamat_mac_t *bssid)
{
    wifi_iface_t *iface;

    if (dev == NULL || bssid == NULL) {
        return STATUS_PARAM_NULL;
    }

    iface = wifi_find_iface(dev->id);

    if (iface == NULL || !iface->connected) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    kernel_memcpy(bssid, &iface->bssid, sizeof(alamat_mac_t));

    return STATUS_BERHASIL;
}

/*
 * wifi_get_stats - Dapatkan statistik WiFi
 */
status_t wifi_get_stats(tak_bertanda64_t *scans,
                        tak_bertanda64_t *connections,
                        tak_bertanda64_t *disconnections,
                        tak_bertanda64_t *errors)
{
    if (scans != NULL) {
        *scans = g_wifi_scans;
    }

    if (connections != NULL) {
        *connections = g_wifi_connections;
    }

    if (disconnections != NULL) {
        *disconnections = g_wifi_disconnections;
    }

    if (errors != NULL) {
        *errors = g_wifi_errors;
    }

    return STATUS_BERHASIL;
}

/*
 * wifi_reset_stats - Reset statistik WiFi
 */
void wifi_reset_stats(void)
{
    g_wifi_scans = 0;
    g_wifi_connections = 0;
    g_wifi_disconnections = 0;
    g_wifi_tx_packets = 0;
    g_wifi_rx_packets = 0;
    g_wifi_errors = 0;
}

/*
 * wifi_print_info - Cetak informasi WiFi
 */
void wifi_print_info(void)
{
    tak_bertanda32_t i;
    char mac_str[18];

    kernel_printf("\n=== WiFi Interfaces ===\n\n");

    for (i = 0; i < WIFI_IFACE_MAKS; i++) {
        if (g_wifi_ifaces[i].iface_id != 0) {
            kernel_printf("Interface %u:\n", g_wifi_ifaces[i].iface_id);

            if (g_wifi_ifaces[i].connected) {
                kernel_printf("  SSID: %s\n", g_wifi_ifaces[i].ssid);

                mac_ke_string(&g_wifi_ifaces[i].bssid, mac_str,
                              sizeof(mac_str));
                kernel_printf("  BSSID: %s\n", mac_str);

                kernel_printf("  Channel: %u\n", g_wifi_ifaces[i].channel);
                kernel_printf("  Frequency: %u MHz\n",
                              g_wifi_ifaces[i].frequency);
                kernel_printf("  RSSI: %d dBm\n", g_wifi_ifaces[i].rssi);
                kernel_printf("  Speed: %u Mbps\n",
                              g_wifi_ifaces[i].kecepatan);
                kernel_printf("  Signal Quality: %u%%\n",
                              wifi_rssi_to_quality(g_wifi_ifaces[i].rssi));
            } else {
                kernel_printf("  Status: Disconnected\n");
            }

            kernel_printf("\n");
        }
    }

    kernel_printf("Statistics:\n");
    kernel_printf("  Scans: %lu\n", g_wifi_scans);
    kernel_printf("  Connections: %lu\n", g_wifi_connections);
    kernel_printf("  Disconnections: %lu\n", g_wifi_disconnections);
    kernel_printf("  Errors: %lu\n", g_wifi_errors);
}

/*
 * wifi_set_channel - Set channel manual
 */
status_t wifi_set_channel(netdev_t *dev, tak_bertanda16_t channel)
{
    wifi_iface_t *iface;

    if (dev == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (channel < WIFI_CHANNEL_MIN || channel > WIFI_CHANNEL_MAX) {
        return STATUS_PARAM_INVALID;
    }

    iface = wifi_find_iface(dev->id);

    if (iface == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    iface->channel = channel;
    iface->frequency = wifi_channel_to_freq(channel);

    return STATUS_BERHASIL;
}

/*
 * wifi_get_security_name - Dapatkan nama keamanan
 */
const char *wifi_get_security_name(tak_bertanda32_t keamanan)
{
    switch (keamanan) {
    case WIFI_KEAMANAN_TIDAK_ADA:
        return "Open";
    case WIFI_KEAMANAN_WEP:
        return "WEP";
    case WIFI_KEAMANAN_WPA:
        return "WPA";
    case WIFI_KEAMANAN_WPA2:
        return "WPA2";
    case WIFI_KEAMANAN_WPA3:
        return "WPA3";
    default:
        return "Unknown";
    }
}

/*
 * wifi_get_connection_count - Dapatkan jumlah koneksi aktif
 */
tak_bertanda32_t wifi_get_connection_count(void)
{
    tak_bertanda32_t count = 0;
    tak_bertanda32_t i;

    for (i = 0; i < WIFI_IFACE_MAKS; i++) {
        if (g_wifi_ifaces[i].connected) {
            count++;
        }
    }

    return count;
}
