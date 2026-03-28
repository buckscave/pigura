/*
 * PIGURA OS - ETHERNET.C
 * =======================
 * Implementasi lapisan Ethernet.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi untuk menangani
 * frame Ethernet dalam Pigura OS.
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
 * KONSTANTA ETHERNET
 * ===========================================================================
 */

/* Ethernet type codes */
#define ETHERTYPE_IPV4    0x0800
#define ETHERTYPE_ARP     0x0806
#define ETHERTYPE_RARP    0x8035
#define ETHERTYPE_IPV6    0x86DD
#define ETHERTYPE_VLAN    0x8100
#define ETHERTYPE_LOOP    0x9000

/* Minimum dan maximum frame size */
#define ETHERNET_MIN_FRAME    64
#define ETHERNET_MAX_FRAME    1518
#define ETHERNET_HEADER_LEN   14
#define ETHERNET_FCS_LEN      4

/* Preamble dan SFD */
#define ETHERNET_PREAMBLE_LEN 7
#define ETHERNET_SFD_LEN      1

/* Broadcast MAC */
static const alamat_mac_t g_mac_broadcast = {
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
};

/* Multicast MAC prefix */
static const tak_bertanda8_t g_multicast_prefix = 0x01;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

/* Pool untuk paket statis */
#define ETHERNET_PAKET_POOL_MAKS 32
static paket_t g_ethernet_paket_pool[ETHERNET_PAKET_POOL_MAKS];
static bool_t g_ethernet_paket_terpakai[ETHERNET_PAKET_POOL_MAKS];

/* Buffer untuk frame */
#define ETHERNET_BUFFER_POOL_MAKS 32
static tak_bertanda8_t g_ethernet_buffer_pool[ETHERNET_BUFFER_POOL_MAKS]
    [ETHERNET_MAX_FRAME + ETHERNET_FCS_LEN];
static bool_t g_ethernet_buffer_terpakai[ETHERNET_BUFFER_POOL_MAKS];

/* Statistik */
static tak_bertanda64_t g_ethernet_rx_frames = 0;
static tak_bertanda64_t g_ethernet_tx_frames = 0;
static tak_bertanda64_t g_ethernet_rx_errors = 0;
static tak_bertanda64_t g_ethernet_tx_errors = 0;

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * ethernet_alokasi_paket - Alokasi struktur paket
 */
static paket_t *ethernet_alokasi_paket(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < ETHERNET_PAKET_POOL_MAKS; i++) {
        if (!g_ethernet_paket_terpakai[i]) {
            g_ethernet_paket_terpakai[i] = BENAR;
            return &g_ethernet_paket_pool[i];
        }
    }

    return NULL;
}

/*
 * ethernet_bebaskan_paket - Bebaskan struktur paket
 */
static void ethernet_bebaskan_paket(paket_t *paket)
{
    tak_bertanda32_t i;

    if (paket == NULL) {
        return;
    }

    for (i = 0; i < ETHERNET_PAKET_POOL_MAKS; i++) {
        if (&g_ethernet_paket_pool[i] == paket) {
            g_ethernet_paket_terpakai[i] = SALAH;
            return;
        }
    }
}

/*
 * ethernet_alokasi_buffer - Alokasi buffer frame
 */
static tak_bertanda8_t *ethernet_alokasi_buffer(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < ETHERNET_BUFFER_POOL_MAKS; i++) {
        if (!g_ethernet_buffer_terpakai[i]) {
            g_ethernet_buffer_terpakai[i] = BENAR;
            return g_ethernet_buffer_pool[i];
        }
    }

    return NULL;
}

/*
 * ethernet_bebaskan_buffer - Bebaskan buffer frame
 */
static void ethernet_bebaskan_buffer(tak_bertanda8_t *buffer)
{
    tak_bertanda32_t i;

    if (buffer == NULL) {
        return;
    }

    for (i = 0; i < ETHERNET_BUFFER_POOL_MAKS; i++) {
        if (g_ethernet_buffer_pool[i] == buffer) {
            g_ethernet_buffer_terpakai[i] = SALAH;
            return;
        }
    }
}

/*
 * ethernet_validasi_mac - Validasi alamat MAC
 */
static bool_t ethernet_validasi_mac(const alamat_mac_t *mac)
{
    tak_bertanda32_t i;
    bool_t semua_nol = BENAR;
    bool_t semua_ff = BENAR;

    if (mac == NULL) {
        return SALAH;
    }

    /* Cek MAC tidak semua 0 atau FF */
    for (i = 0; i < NETDEV_ALAMAT_MAC_LEN; i++) {
        if (mac->byte[i] != 0x00) {
            semua_nol = SALAH;
        }
        if (mac->byte[i] != 0xFF) {
            semua_ff = SALAH;
        }
    }

    /* MAC semua 0 atau FF valid sebagai broadcast/undefined */
    return BENAR;
}

/*
 * ethernet_is_broadcast - Cek apakah MAC broadcast
 */
static bool_t ethernet_is_broadcast(const alamat_mac_t *mac)
{
    tak_bertanda32_t i;

    if (mac == NULL) {
        return SALAH;
    }

    for (i = 0; i < NETDEV_ALAMAT_MAC_LEN; i++) {
        if (mac->byte[i] != 0xFF) {
            return SALAH;
        }
    }

    return BENAR;
}

/*
 * ethernet_is_multicast - Cek apakah MAC multicast
 */
static bool_t ethernet_is_multicast(const alamat_mac_t *mac)
{
    if (mac == NULL) {
        return SALAH;
    }

    /* Multicast MAC: least significant bit of first byte is 1 */
    return (mac->byte[0] & 0x01) ? BENAR : SALAH;
}

/*
 * ethernet_is_unicast - Cek apakah MAC unicast
 */
static bool_t ethernet_is_unicast(const alamat_mac_t *mac)
{
    if (mac == NULL) {
        return SALAH;
    }

    return (!ethernet_is_broadcast(mac) &&
            !ethernet_is_multicast(mac)) ? BENAR : SALAH;
}

/*
 * ethernet_build_header - Bangun header Ethernet
 */
static void ethernet_build_header(eth_header_t *header,
                                  const alamat_mac_t *dest,
                                  const alamat_mac_t *src,
                                  tak_bertanda16_t tipe)
{
    if (header == NULL || dest == NULL || src == NULL) {
        return;
    }

    kernel_memcpy(&header->dest, dest, sizeof(alamat_mac_t));
    kernel_memcpy(&header->src, src, sizeof(alamat_mac_t));
    header->tipe = tipe;
}

/*
 * ethernet_calc_fcs - Hitung Frame Check Sequence
 */
static tak_bertanda32_t ethernet_calc_fcs(const tak_bertanda8_t *data,
                                          ukuran_t len)
{
    tak_bertanda32_t crc;
    tak_bertanda32_t i;
    tak_bertanda32_t j;
    tak_bertanda8_t bit;

    if (data == NULL || len == 0) {
        return 0;
    }

    /* CRC-32 calculation (simplified) */
    crc = 0xFFFFFFFF;

    for (i = 0; i < len; i++) {
        crc ^= data[i];

        for (j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc = crc >> 1;
            }
        }
    }

    return crc ^ 0xFFFFFFFF;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * ethernet_init - Inisialisasi lapisan Ethernet
 */
status_t ethernet_init(void)
{
    tak_bertanda32_t i;

    /* Reset pool paket */
    for (i = 0; i < ETHERNET_PAKET_POOL_MAKS; i++) {
        g_ethernet_paket_terpakai[i] = SALAH;
        kernel_memset(&g_ethernet_paket_pool[i], 0, sizeof(paket_t));
    }

    /* Reset pool buffer */
    for (i = 0; i < ETHERNET_BUFFER_POOL_MAKS; i++) {
        g_ethernet_buffer_terpakai[i] = SALAH;
    }

    /* Reset statistik */
    g_ethernet_rx_frames = 0;
    g_ethernet_tx_frames = 0;
    g_ethernet_rx_errors = 0;
    g_ethernet_tx_errors = 0;

    return STATUS_BERHASIL;
}

/*
 * ethernet_proses - Proses frame Ethernet yang diterima
 */
status_t ethernet_proses(paket_t *paket)
{
    eth_header_t *header;
    tak_bertanda16_t tipe;
    ukuran_t payload_len;

    if (paket == NULL || paket->data == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (paket->panjang < ETHERNET_HEADER_LEN) {
        g_ethernet_rx_errors++;
        return STATUS_PARAM_UKURAN;
    }

    /* Parse header Ethernet */
    header = (eth_header_t *)paket->data;
    paket->eth = header;

    /* Convert type dari network byte order */
    tipe = header->tipe;

    /* Hitung payload length */
    payload_len = paket->panjang - ETHERNET_HEADER_LEN;

    /* Cek minimum frame size */
    if (paket->panjang < ETHERNET_MIN_FRAME - ETHERNET_FCS_LEN) {
        g_ethernet_rx_errors++;
        return STATUS_PARAM_UKURAN;
    }

    /* Update statistik */
    g_ethernet_rx_frames++;

    /* Proses berdasarkan tipe protocol */
    switch (tipe) {
    case ETHERTYPE_IPV4:
        /* Proses IPv4 */
        paket->ip = (ip_header_t *)(paket->data + ETHERNET_HEADER_LEN);
        paket->payload = paket->data + ETHERNET_HEADER_LEN;
        paket->payload_len = payload_len;
        break;

    case ETHERTYPE_IPV6:
        /* Proses IPv6 */
        paket->payload = paket->data + ETHERNET_HEADER_LEN;
        paket->payload_len = payload_len;
        break;

    case ETHERTYPE_ARP:
        /* Proses ARP */
        paket->payload = paket->data + ETHERNET_HEADER_LEN;
        paket->payload_len = payload_len;
        arp_proses(paket);
        break;

    case ETHERTYPE_VLAN:
        /* Proses VLAN (802.1Q) */
        paket->payload = paket->data + ETHERNET_HEADER_LEN + 4;
        paket->payload_len = payload_len - 4;
        break;

    default:
        /* Unknown protocol - ignore */
        break;
    }

    return STATUS_BERHASIL;
}

/*
 * ethernet_kirim - Kirim frame Ethernet
 */
status_t ethernet_kirim(netdev_t *dev, alamat_mac_t *dest,
                        tak_bertanda16_t tipe, const void *data,
                        ukuran_t len)
{
    tak_bertanda8_t *buffer;
    eth_header_t *header;
    ukuran_t total_len;
    status_t hasil;

    /* Validasi parameter */
    if (dev == NULL || dest == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (data == NULL && len > 0) {
        return STATUS_PARAM_NULL;
    }

    /* Cek device aktif */
    if (!(dev->flags & NETDEV_FLAG_UP)) {
        g_ethernet_tx_errors++;
        return NET_ERR_IFACE_DOWN;
    }

    /* Hitung total length */
    total_len = ETHERNET_HEADER_LEN + len;

    /* Cek MTU */
    if (total_len > dev->mtu) {
        g_ethernet_tx_errors++;
        return STATUS_PARAM_UKURAN;
    }

    /* Cek minimum frame size */
    if (total_len < ETHERNET_MIN_FRAME - ETHERNET_FCS_LEN) {
        total_len = ETHERNET_MIN_FRAME - ETHERNET_FCS_LEN;
    }

    /* Alokasi buffer */
    buffer = ethernet_alokasi_buffer();
    if (buffer == NULL) {
        g_ethernet_tx_errors++;
        return STATUS_MEMORI_HABIS;
    }

    /* Bangun header */
    header = (eth_header_t *)buffer;
    ethernet_build_header(header, dest, &dev->mac, tipe);

    /* Copy payload */
    if (len > 0 && data != NULL) {
        kernel_memcpy(buffer + ETHERNET_HEADER_LEN, data, len);
    }

    /* Padding jika perlu */
    if (len < ETHERNET_MIN_FRAME - ETHERNET_FCS_LEN - ETHERNET_HEADER_LEN) {
        ukuran_t padding_len;
        padding_len = ETHERNET_MIN_FRAME - ETHERNET_FCS_LEN -
                      ETHERNET_HEADER_LEN - len;
        kernel_memset(buffer + ETHERNET_HEADER_LEN + len, 0, padding_len);
    }

    /* Kirim melalui device */
    hasil = netdev_kirim(dev, buffer, total_len);

    /* Bebaskan buffer */
    ethernet_bebaskan_buffer(buffer);

    if (hasil == STATUS_BERHASIL) {
        g_ethernet_tx_frames++;
    } else {
        g_ethernet_tx_errors++;
    }

    return hasil;
}

/*
 * ethernet_kirim_raw - Kirim raw Ethernet frame
 */
status_t ethernet_kirim_raw(netdev_t *dev, const void *frame, ukuran_t len)
{
    status_t hasil;

    if (dev == NULL || frame == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (len < ETHERNET_MIN_FRAME - ETHERNET_FCS_LEN) {
        return STATUS_PARAM_UKURAN;
    }

    if (len > ETHERNET_MAX_FRAME) {
        return STATUS_PARAM_UKURAN;
    }

    hasil = netdev_kirim(dev, frame, len);

    if (hasil == STATUS_BERHASIL) {
        g_ethernet_tx_frames++;
    } else {
        g_ethernet_tx_errors++;
    }

    return hasil;
}

/*
 * ethernet_broadcast - Kirim frame broadcast
 */
status_t ethernet_broadcast(netdev_t *dev, tak_bertanda16_t tipe,
                            const void *data, ukuran_t len)
{
    return ethernet_kirim(dev, (alamat_mac_t *)&g_mac_broadcast,
                          tipe, data, len);
}

/*
 * ethernet_mac_to_string - Konversi MAC ke string
 */
tak_bertanda32_t ethernet_mac_to_string(const alamat_mac_t *mac,
                                        char *buffer, ukuran_t len)
{
    static const char hex[] = "0123456789ABCDEF";
    ukuran_t pos;
    tak_bertanda32_t i;

    if (mac == NULL || buffer == NULL || len < 18) {
        return 0;
    }

    pos = 0;

    for (i = 0; i < 6; i++) {
        buffer[pos++] = hex[(mac->byte[i] >> 4) & 0x0F];
        buffer[pos++] = hex[mac->byte[i] & 0x0F];

        if (i < 5) {
            buffer[pos++] = ':';
        }
    }

    buffer[pos] = '\0';

    return (tak_bertanda32_t)pos;
}

/*
 * ethernet_string_to_mac - Konversi string ke MAC
 */
status_t ethernet_string_to_mac(const char *str, alamat_mac_t *mac)
{
    tak_bertanda32_t i;
    tak_bertanda32_t byte_index;
    tak_bertanda8_t val;

    if (str == NULL || mac == NULL) {
        return STATUS_PARAM_NULL;
    }

    byte_index = 0;
    val = 0;

    for (i = 0; str[i] != '\0' && byte_index < 6; i++) {
        if (str[i] >= '0' && str[i] <= '9') {
            val = (val << 4) | (str[i] - '0');
        } else if (str[i] >= 'A' && str[i] <= 'F') {
            val = (val << 4) | (str[i] - 'A' + 10);
        } else if (str[i] >= 'a' && str[i] <= 'f') {
            val = (val << 4) | (str[i] - 'a' + 10);
        } else if (str[i] == ':' || str[i] == '-') {
            mac->byte[byte_index++] = val;
            val = 0;
        }
    }

    /* Byte terakhir */
    if (byte_index == 5) {
        mac->byte[byte_index] = val;
    }

    return STATUS_BERHASIL;
}

/*
 * ethernet_get_broadcast - Dapatkan alamat broadcast
 */
status_t ethernet_get_broadcast(alamat_mac_t *mac)
{
    tak_bertanda32_t i;

    if (mac == NULL) {
        return STATUS_PARAM_NULL;
    }

    for (i = 0; i < NETDEV_ALAMAT_MAC_LEN; i++) {
        mac->byte[i] = 0xFF;
    }

    return STATUS_BERHASIL;
}

/*
 * ethernet_compare_mac - Bandingkan dua alamat MAC
 */
tanda32_t ethernet_compare_mac(const alamat_mac_t *mac1,
                               const alamat_mac_t *mac2)
{
    if (mac1 == NULL || mac2 == NULL) {
        return -1;
    }

    return kernel_memcmp(mac1->byte, mac2->byte, NETDEV_ALAMAT_MAC_LEN);
}

/*
 * ethernet_copy_mac - Copy alamat MAC
 */
status_t ethernet_copy_mac(alamat_mac_t *dest, const alamat_mac_t *src)
{
    if (dest == NULL || src == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memcpy(dest, src, sizeof(alamat_mac_t));

    return STATUS_BERHASIL;
}

/*
 * ethernet_is_valid_mac - Cek validitas MAC
 */
bool_t ethernet_is_valid_mac(const alamat_mac_t *mac)
{
    return ethernet_validasi_mac(mac);
}

/*
 * ethernet_get_type_name - Dapatkan nama tipe Ethernet
 */
const char *ethernet_get_type_name(tak_bertanda16_t tipe)
{
    switch (tipe) {
    case ETHERTYPE_IPV4:
        return "IPv4";
    case ETHERTYPE_IPV6:
        return "IPv6";
    case ETHERTYPE_ARP:
        return "ARP";
    case ETHERTYPE_RARP:
        return "RARP";
    case ETHERTYPE_VLAN:
        return "VLAN";
    case ETHERTYPE_LOOP:
        return "Loop";
    default:
        return "Unknown";
    }
}

/*
 * ethernet_get_stats - Dapatkan statistik Ethernet
 */
status_t ethernet_get_stats(tak_bertanda64_t *rx_frames,
                            tak_bertanda64_t *tx_frames,
                            tak_bertanda64_t *rx_errors,
                            tak_bertanda64_t *tx_errors)
{
    if (rx_frames != NULL) {
        *rx_frames = g_ethernet_rx_frames;
    }

    if (tx_frames != NULL) {
        *tx_frames = g_ethernet_tx_frames;
    }

    if (rx_errors != NULL) {
        *rx_errors = g_ethernet_rx_errors;
    }

    if (tx_errors != NULL) {
        *tx_errors = g_ethernet_tx_errors;
    }

    return STATUS_BERHASIL;
}

/*
 * ethernet_reset_stats - Reset statistik Ethernet
 */
void ethernet_reset_stats(void)
{
    g_ethernet_rx_frames = 0;
    g_ethernet_tx_frames = 0;
    g_ethernet_rx_errors = 0;
    g_ethernet_tx_errors = 0;
}

/*
 * ethernet_print_frame - Cetak informasi frame
 */
void ethernet_print_frame(const paket_t *paket)
{
    char mac_str[18];

    if (paket == NULL || paket->eth == NULL) {
        return;
    }

    kernel_printf("Ethernet Frame:\n");

    /* Destination MAC */
    ethernet_mac_to_string(&paket->eth->dest, mac_str, sizeof(mac_str));
    kernel_printf("  Dest: %s", mac_str);

    if (ethernet_is_broadcast(&paket->eth->dest)) {
        kernel_printf(" (Broadcast)");
    } else if (ethernet_is_multicast(&paket->eth->dest)) {
        kernel_printf(" (Multicast)");
    }
    kernel_printf("\n");

    /* Source MAC */
    ethernet_mac_to_string(&paket->eth->src, mac_str, sizeof(mac_str));
    kernel_printf("  Src:  %s\n", mac_str);

    /* Type */
    kernel_printf("  Type: 0x%04X (%s)\n", paket->eth->tipe,
                  ethernet_get_type_name(paket->eth->tipe));

    /* Length */
    kernel_printf("  Length: %lu bytes\n", paket->panjang);
}
