/*
 * PIGURA OS - ARP.C
 * ==================
 * Implementasi protokol ARP.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi untuk menangani
 * protokol ARP (Address Resolution Protocol) dalam Pigura OS.
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
 * KONSTANTA ARP
 * ===========================================================================
 */

/* ARP hardware types */
#define ARP_HW_ETHERNET     1
#define ARP_HW_TOKEN_RING   6
#define ARP_HW_FDDI         17

/* ARP protocol types */
#define ARP_PROTO_IPV4      0x0800
#define ARP_PROTO_IPV6      0x86DD

/* ARP cache timeout (seconds) */
#define ARP_CACHE_TIMEOUT   300
#define ARP_CACHE_MAX_AGE   600

/* ARP retry */
#define ARP_MAX_RETRIES     3
#define ARP_RETRY_INTERVAL  1000  /* ms */

/*
 * ===========================================================================
 * STRUKTUR ARP INTERNAL
 * ===========================================================================
 */

/* ARP cache entry */
typedef struct {
    alamat_ipv4_t ip;
    alamat_mac_t mac;
    tak_bertanda32_t iface_id;
    tak_bertanda64_t timestamp;
    tak_bertanda64_t expiry;
    tak_bertanda32_t state;
    tak_bertanda16_t retries;
    bool_t valid;
    bool_t permanent;
} arp_entry_t;

/* ARP pending request */
typedef struct {
    alamat_ipv4_t ip;
    tak_bertanda32_t iface_id;
    tak_bertanda64_t timestamp;
    tak_bertanda32_t retries;
    bool_t active;
} arp_pending_t;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

/* ARP cache */
#define ARP_CACHE_MAKS      64
static arp_entry_t g_arp_cache[ARP_CACHE_MAKS];
static tak_bertanda32_t g_arp_cache_count = 0;

/* Pending requests */
#define ARP_PENDING_MAKS    16
static arp_pending_t g_arp_pending[ARP_PENDING_MAKS];

/* Statistics */
static tak_bertanda64_t g_arp_requests_sent = 0;
static tak_bertanda64_t g_arp_requests_received = 0;
static tak_bertanda64_t g_arp_replies_sent = 0;
static tak_bertanda64_t g_arp_replies_received = 0;
static tak_bertanda64_t g_arp_cache_hits = 0;
static tak_bertanda64_t g_arp_cache_misses = 0;

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * arp_compare_ip - Bandingkan dua alamat IP
 */
static bool_t arp_compare_ip(const alamat_ipv4_t *ip1,
                             const alamat_ipv4_t *ip2)
{
    tak_bertanda32_t i;

    if (ip1 == NULL || ip2 == NULL) {
        return SALAH;
    }

    for (i = 0; i < NETDEV_IPV4_LEN; i++) {
        if (ip1->byte[i] != ip2->byte[i]) {
            return SALAH;
        }
    }

    return BENAR;
}

/*
 * arp_compare_mac - Bandingkan dua alamat MAC
 */
static bool_t arp_compare_mac(const alamat_mac_t *mac1,
                              const alamat_mac_t *mac2)
{
    tak_bertanda32_t i;

    if (mac1 == NULL || mac2 == NULL) {
        return SALAH;
    }

    for (i = 0; i < NETDEV_ALAMAT_MAC_LEN; i++) {
        if (mac1->byte[i] != mac2->byte[i]) {
            return SALAH;
        }
    }

    return BENAR;
}

/*
 * arp_copy_ip - Copy alamat IP
 */
static void arp_copy_ip(alamat_ipv4_t *dest, const alamat_ipv4_t *src)
{
    tak_bertanda32_t i;

    if (dest == NULL || src == NULL) {
        return;
    }

    for (i = 0; i < NETDEV_IPV4_LEN; i++) {
        dest->byte[i] = src->byte[i];
    }
}

/*
 * arp_copy_mac - Copy alamat MAC
 */
static void arp_copy_mac(alamat_mac_t *dest, const alamat_mac_t *src)
{
    tak_bertanda32_t i;

    if (dest == NULL || src == NULL) {
        return;
    }

    for (i = 0; i < NETDEV_ALAMAT_MAC_LEN; i++) {
        dest->byte[i] = src->byte[i];
    }
}

/*
 * arp_find_entry - Cari entry di cache
 */
static tak_bertanda32_t arp_find_entry(const alamat_ipv4_t *ip)
{
    tak_bertanda32_t i;

    for (i = 0; i < ARP_CACHE_MAKS; i++) {
        if (g_arp_cache[i].valid &&
            arp_compare_ip(&g_arp_cache[i].ip, ip)) {
            return i;
        }
    }

    return INDEX_INVALID;
}

/*
 * arp_find_free_entry - Cari entry kosong
 */
static tak_bertanda32_t arp_find_free_entry(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < ARP_CACHE_MAKS; i++) {
        if (!g_arp_cache[i].valid) {
            return i;
        }
    }

    return INDEX_INVALID;
}

/*
 * arp_find_oldest_entry - Cari entry tertua
 */
static tak_bertanda32_t arp_find_oldest_entry(void)
{
    tak_bertanda32_t i;
    tak_bertanda32_t oldest = 0;
    tak_bertanda64_t oldest_time = g_arp_cache[0].timestamp;

    for (i = 1; i < ARP_CACHE_MAKS; i++) {
        if (g_arp_cache[i].valid &&
            !g_arp_cache[i].permanent &&
            g_arp_cache[i].timestamp < oldest_time) {
            oldest_time = g_arp_cache[i].timestamp;
            oldest = i;
        }
    }

    return oldest;
}

/*
 * arp_build_request - Bangun ARP request
 */
static void arp_build_request(tak_bertanda8_t *buffer,
                              const alamat_mac_t *src_mac,
                              const alamat_ipv4_t *src_ip,
                              const alamat_ipv4_t *target_ip)
{
    arp_header_t *arp;

    if (buffer == NULL || src_mac == NULL ||
        src_ip == NULL || target_ip == NULL) {
        return;
    }

    arp = (arp_header_t *)buffer;

    arp->hardware_type = ARP_HW_ETHERNET;
    arp->protocol_type = ARP_PROTO_IPV4;
    arp->hardware_len = NETDEV_ALAMAT_MAC_LEN;
    arp->protocol_len = NETDEV_IPV4_LEN;
    arp->opcode = ARP_OP_REQUEST;

    arp_copy_mac(&arp->sender_mac, src_mac);
    arp_copy_ip(&arp->sender_ip, src_ip);

    /* Target MAC = 0 untuk request */
    kernel_memset(&arp->target_mac, 0, sizeof(alamat_mac_t));
    arp_copy_ip(&arp->target_ip, target_ip);
}

/*
 * arp_build_reply - Bangun ARP reply
 */
static void arp_build_reply(tak_bertanda8_t *buffer,
                            const alamat_mac_t *src_mac,
                            const alamat_ipv4_t *src_ip,
                            const alamat_mac_t *target_mac,
                            const alamat_ipv4_t *target_ip)
{
    arp_header_t *arp;

    if (buffer == NULL || src_mac == NULL || src_ip == NULL ||
        target_mac == NULL || target_ip == NULL) {
        return;
    }

    arp = (arp_header_t *)buffer;

    arp->hardware_type = ARP_HW_ETHERNET;
    arp->protocol_type = ARP_PROTO_IPV4;
    arp->hardware_len = NETDEV_ALAMAT_MAC_LEN;
    arp->protocol_len = NETDEV_IPV4_LEN;
    arp->opcode = ARP_OP_REPLY;

    arp_copy_mac(&arp->sender_mac, src_mac);
    arp_copy_ip(&arp->sender_ip, src_ip);
    arp_copy_mac(&arp->target_mac, target_mac);
    arp_copy_ip(&arp->target_ip, target_ip);
}

/*
 * arp_send_request - Kirim ARP request
 */
static status_t arp_send_request(netdev_t *dev, const alamat_ipv4_t *ip)
{
    tak_bertanda8_t buffer[sizeof(arp_header_t)];
    alamat_mac_t broadcast;

    if (dev == NULL || ip == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Set broadcast MAC */
    kernel_memset(&broadcast, 0xFF, sizeof(alamat_mac_t));

    /* Build request */
    arp_build_request(buffer, &dev->mac, &dev->ipv4, ip);

    /* Send via Ethernet */
    ethernet_kirim(dev, &broadcast, 0x0806, buffer, sizeof(arp_header_t));

    g_arp_requests_sent++;

    return STATUS_BERHASIL;
}

/*
 * arp_add_pending - Tambah pending request
 */
static status_t arp_add_pending(const alamat_ipv4_t *ip,
                                tak_bertanda32_t iface_id)
{
    tak_bertanda32_t i;

    for (i = 0; i < ARP_PENDING_MAKS; i++) {
        if (!g_arp_pending[i].active) {
            arp_copy_ip(&g_arp_pending[i].ip, ip);
            g_arp_pending[i].iface_id = iface_id;
            g_arp_pending[i].timestamp = 0;
            g_arp_pending[i].retries = 0;
            g_arp_pending[i].active = BENAR;
            return STATUS_BERHASIL;
        }
    }

    return STATUS_PENUH;
}

/*
 * arp_remove_pending - Hapus pending request
 */
static void arp_remove_pending(const alamat_ipv4_t *ip)
{
    tak_bertanda32_t i;

    for (i = 0; i < ARP_PENDING_MAKS; i++) {
        if (g_arp_pending[i].active &&
            arp_compare_ip(&g_arp_pending[i].ip, ip)) {
            g_arp_pending[i].active = SALAH;
            return;
        }
    }
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * arp_init - Inisialisasi ARP
 */
status_t arp_init(void)
{
    tak_bertanda32_t i;

    /* Reset cache */
    for (i = 0; i < ARP_CACHE_MAKS; i++) {
        g_arp_cache[i].valid = SALAH;
        g_arp_cache[i].permanent = SALAH;
        kernel_memset(&g_arp_cache[i].ip, 0, sizeof(alamat_ipv4_t));
        kernel_memset(&g_arp_cache[i].mac, 0, sizeof(alamat_mac_t));
    }
    g_arp_cache_count = 0;

    /* Reset pending */
    for (i = 0; i < ARP_PENDING_MAKS; i++) {
        g_arp_pending[i].active = SALAH;
    }

    /* Reset statistik */
    g_arp_requests_sent = 0;
    g_arp_requests_received = 0;
    g_arp_replies_sent = 0;
    g_arp_replies_received = 0;
    g_arp_cache_hits = 0;
    g_arp_cache_misses = 0;

    return STATUS_BERHASIL;
}

/*
 * arp_proses - Proses paket ARP
 */
status_t arp_proses(paket_t *paket)
{
    arp_header_t *arp;
    tak_bertanda32_t entry_idx;
    netdev_t *dev;

    if (paket == NULL || paket->data == NULL) {
        return STATUS_PARAM_NULL;
    }

    arp = (arp_header_t *)paket->payload;
    if (arp == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Hanya proses Ethernet/IPv4 ARP */
    if (arp->hardware_type != ARP_HW_ETHERNET ||
        arp->protocol_type != ARP_PROTO_IPV4) {
        return STATUS_BERHASIL;
    }

    /* Get interface */
    dev = netdev_cari_id(paket->iface_id);

    switch (arp->opcode) {
    case ARP_OP_REQUEST:
        g_arp_requests_received++;

        /* Cek apakah request untuk kita */
        if (dev != NULL && arp_compare_ip(&arp->target_ip, &dev->ipv4)) {
            tak_bertanda8_t reply[sizeof(arp_header_t)];

            /* Kirim reply */
            arp_build_reply(reply, &dev->mac, &dev->ipv4,
                            &arp->sender_mac, &arp->sender_ip);

            ethernet_kirim(dev, &arp->sender_mac, 0x0806,
                           reply, sizeof(arp_header_t));

            g_arp_replies_sent++;
        }

        /* Tambah sender ke cache */
        entry_idx = arp_find_entry(&arp->sender_ip);
        if (entry_idx == INDEX_INVALID) {
            entry_idx = arp_find_free_entry();
            if (entry_idx == INDEX_INVALID) {
                entry_idx = arp_find_oldest_entry();
            }
        }

        if (entry_idx != INDEX_INVALID) {
            arp_copy_ip(&g_arp_cache[entry_idx].ip, &arp->sender_ip);
            arp_copy_mac(&g_arp_cache[entry_idx].mac, &arp->sender_mac);
            g_arp_cache[entry_idx].timestamp = 0;
            g_arp_cache[entry_idx].valid = BENAR;
            g_arp_cache[entry_idx].permanent = SALAH;
            g_arp_cache_count++;
        }
        break;

    case ARP_OP_REPLY:
        g_arp_replies_received++;

        /* Tambah ke cache */
        entry_idx = arp_find_entry(&arp->sender_ip);
        if (entry_idx == INDEX_INVALID) {
            entry_idx = arp_find_free_entry();
            if (entry_idx == INDEX_INVALID) {
                entry_idx = arp_find_oldest_entry();
            }
        }

        if (entry_idx != INDEX_INVALID) {
            arp_copy_ip(&g_arp_cache[entry_idx].ip, &arp->sender_ip);
            arp_copy_mac(&g_arp_cache[entry_idx].mac, &arp->sender_mac);
            g_arp_cache[entry_idx].timestamp = 0;
            g_arp_cache[entry_idx].valid = BENAR;
            g_arp_cache[entry_idx].permanent = SALAH;
            g_arp_cache_count++;
        }

        /* Remove pending */
        arp_remove_pending(&arp->sender_ip);
        break;

    default:
        break;
    }

    return STATUS_BERHASIL;
}

/*
 * arp_cari - Cari MAC address untuk IP
 */
status_t arp_cari(netdev_t *dev, alamat_ipv4_t *ip, alamat_mac_t *mac)
{
    tak_bertanda32_t entry_idx;

    if (ip == NULL || mac == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Cari di cache */
    entry_idx = arp_find_entry(ip);

    if (entry_idx != INDEX_INVALID && g_arp_cache[entry_idx].valid) {
        arp_copy_mac(mac, &g_arp_cache[entry_idx].mac);
        g_arp_cache_hits++;
        return STATUS_BERHASIL;
    }

    g_arp_cache_misses++;

    /* Kirim request */
    if (dev != NULL) {
        arp_send_request(dev, ip);
    }

    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * arp_tambah - Tambah entry ARP
 */
status_t arp_tambah(netdev_t *dev, alamat_ipv4_t *ip, alamat_mac_t *mac)
{
    (void)dev;
    tak_bertanda32_t entry_idx;

    if (ip == NULL || mac == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Cari existing entry */
    entry_idx = arp_find_entry(ip);

    if (entry_idx == INDEX_INVALID) {
        /* Cari free slot */
        entry_idx = arp_find_free_entry();
        if (entry_idx == INDEX_INVALID) {
            entry_idx = arp_find_oldest_entry();
        }
    }

    if (entry_idx == INDEX_INVALID) {
        return STATUS_PENUH;
    }

    /* Set entry */
    arp_copy_ip(&g_arp_cache[entry_idx].ip, ip);
    arp_copy_mac(&g_arp_cache[entry_idx].mac, mac);
    g_arp_cache[entry_idx].timestamp = 0;
    g_arp_cache[entry_idx].expiry = ARP_CACHE_TIMEOUT;
    g_arp_cache[entry_idx].valid = BENAR;
    g_arp_cache[entry_idx].permanent = SALAH;

    if (!g_arp_cache[entry_idx].valid) {
        g_arp_cache_count++;
    }

    return STATUS_BERHASIL;
}

/*
 * arp_hapus - Hapus entry ARP
 */
status_t arp_hapus(netdev_t *dev, alamat_ipv4_t *ip)
{
    (void)dev;
    tak_bertanda32_t entry_idx;

    if (ip == NULL) {
        return STATUS_PARAM_NULL;
    }

    entry_idx = arp_find_entry(ip);

    if (entry_idx == INDEX_INVALID) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    g_arp_cache[entry_idx].valid = SALAH;
    g_arp_cache_count--;

    return STATUS_BERHASIL;
}

/*
 * arp_add_permanent - Tambah entry permanent
 */
status_t arp_add_permanent(alamat_ipv4_t *ip, alamat_mac_t *mac)
{
    status_t hasil;

    hasil = arp_tambah(NULL, ip, mac);
    if (hasil == STATUS_BERHASIL) {
        tak_bertanda32_t idx = arp_find_entry(ip);
        if (idx != INDEX_INVALID) {
            g_arp_cache[idx].permanent = BENAR;
        }
    }

    return hasil;
}

/*
 * arp_flush - Flush ARP cache
 */
void arp_flush(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < ARP_CACHE_MAKS; i++) {
        if (!g_arp_cache[i].permanent) {
            g_arp_cache[i].valid = SALAH;
        }
    }
}

/*
 * arp_flush_all - Flush semua entry
 */
void arp_flush_all(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < ARP_CACHE_MAKS; i++) {
        g_arp_cache[i].valid = SALAH;
        g_arp_cache[i].permanent = SALAH;
    }

    g_arp_cache_count = 0;
}

/*
 * arp_get_entry_count - Dapatkan jumlah entry
 */
tak_bertanda32_t arp_get_entry_count(void)
{
    return g_arp_cache_count;
}

/*
 * arp_get_stats - Dapatkan statistik ARP
 */
status_t arp_get_stats(tak_bertanda64_t *requests_sent,
                       tak_bertanda64_t *requests_received,
                       tak_bertanda64_t *replies_sent,
                       tak_bertanda64_t *replies_received)
{
    if (requests_sent != NULL) {
        *requests_sent = g_arp_requests_sent;
    }

    if (requests_received != NULL) {
        *requests_received = g_arp_requests_received;
    }

    if (replies_sent != NULL) {
        *replies_sent = g_arp_replies_sent;
    }

    if (replies_received != NULL) {
        *replies_received = g_arp_replies_received;
    }

    return STATUS_BERHASIL;
}

/*
 * arp_reset_stats - Reset statistik ARP
 */
void arp_reset_stats(void)
{
    g_arp_requests_sent = 0;
    g_arp_requests_received = 0;
    g_arp_replies_sent = 0;
    g_arp_replies_received = 0;
    g_arp_cache_hits = 0;
    g_arp_cache_misses = 0;
}

/*
 * arp_print_cache - Cetak ARP cache
 */
void arp_print_cache(void)
{
    tak_bertanda32_t i;
    char ip_str[16];
    char mac_str[18];

    kernel_printf("\n=== ARP Cache ===\n\n");

    for (i = 0; i < ARP_CACHE_MAKS; i++) {
        if (g_arp_cache[i].valid) {
            ipv4_ke_string(&g_arp_cache[i].ip, ip_str, sizeof(ip_str));
            mac_ke_string(&g_arp_cache[i].mac, mac_str, sizeof(mac_str));

            kernel_printf("%s -> %s", ip_str, mac_str);

            if (g_arp_cache[i].permanent) {
                kernel_printf(" [Permanent]");
            }

            kernel_printf("\n");
        }
    }

    kernel_printf("\nTotal entries: %u\n", g_arp_cache_count);
    kernel_printf("Cache hits: %lu\n", g_arp_cache_hits);
    kernel_printf("Cache misses: %lu\n", g_arp_cache_misses);
}

/*
 * arp_is_valid - Cek validitas entry
 */
bool_t arp_is_valid(const alamat_ipv4_t *ip)
{
    tak_bertanda32_t idx;

    if (ip == NULL) {
        return SALAH;
    }

    idx = arp_find_entry(ip);

    if (idx == INDEX_INVALID) {
        return SALAH;
    }

    return g_arp_cache[idx].valid;
}
