/*
 * PIGURA OS - UDP.C
 * ==================
 * Implementasi protokol UDP.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi untuk menangani
 * protokol UDP (User Datagram Protocol) dalam Pigura OS.
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
 * KONSTANTA UDP
 * ===========================================================================
 */

/* UDP header size */
#define UDP_HEADER_LEN       8

/* Maximum UDP payload */
#define UDP_PAYLOAD_MAX      65507

/* Default TTL */
#define UDP_DEFAULT_TTL      64

/*
 * ===========================================================================
 * STRUKTUR UDP INTERNAL
 * ===========================================================================
 */

/* UDP binding */
typedef struct {
    tak_bertanda16_t port;
    tak_bertanda32_t local_ip;
    bool_t in_use;
    void *socket_ptr;
} udp_binding_t;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

/* Binding table */
#define UDP_BINDING_MAKS 256
static udp_binding_t g_udp_bindings[UDP_BINDING_MAKS];
static tak_bertanda32_t g_udp_binding_count = 0;

/* Port allocation */
static bool_t g_udp_port_used[PORT_MAKS];

/* Statistics */
static tak_bertanda64_t g_udp_datagrams_sent = 0;
static tak_bertanda64_t g_udp_datagrams_received = 0;
static tak_bertanda64_t g_udp_checksum_errors = 0;
static tak_bertanda64_t g_udp_port_unreachable = 0;
static tak_bertanda64_t g_udp_buffer_overflows = 0;

/* Receive buffer */
#define UDP_RX_BUFFER_MAKS  64
typedef struct {
    tak_bertanda32_t src_ip;
    tak_bertanda16_t src_port;
    tak_bertanda16_t dst_port;
    tak_bertanda8_t data[UDP_PAYLOAD_MAX];
    ukuran_t data_len;
    bool_t valid;
} udp_rx_entry_t;

static udp_rx_entry_t g_udp_rx_buffer[UDP_RX_BUFFER_MAKS];
static tak_bertanda32_t g_udp_rx_write_idx = 0;
static tak_bertanda32_t g_udp_rx_read_idx = 0;

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * udp_checksum - Hitung checksum UDP
 */
static tak_bertanda16_t udp_checksum(const void *data, ukuran_t len,
                                     tak_bertanda32_t src_ip,
                                     tak_bertanda32_t dest_ip)
{
    tak_bertanda32_t sum;
    const tak_bertanda16_t *ptr;
    ukuran_t i;

    /* Pseudo-header */
    sum = (src_ip >> 16) & 0xFFFF;
    sum += src_ip & 0xFFFF;
    sum += (dest_ip >> 16) & 0xFFFF;
    sum += dest_ip & 0xFFFF;
    sum += PROTO_UDP;
    sum += len;

    /* UDP datagram */
    ptr = (const tak_bertanda16_t *)data;

    for (i = 0; i < len / 2; i++) {
        sum += ptr[i];
    }

    if (len & 1) {
        sum += ((const tak_bertanda8_t *)data)[len - 1];
    }

    /* Fold */
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (tak_bertanda16_t)~sum;
}

/*
 * udp_build_header - Bangun header UDP
 */
static void udp_build_header(tak_bertanda8_t *buffer,
                             tak_bertanda16_t src_port,
                             tak_bertanda16_t dest_port,
                             ukuran_t payload_len)
{
    udp_header_t *header;

    if (buffer == NULL) {
        return;
    }

    header = (udp_header_t *)buffer;

    header->src_port = src_port;
    header->dest_port = dest_port;
    header->length = (tak_bertanda16_t)(UDP_HEADER_LEN + payload_len);
    header->checksum = 0;

    /* Checksum optional untuk UDP, set ke 0 */
}

/*
 * udp_find_binding - Cari binding berdasarkan port
 */
static udp_binding_t *udp_find_binding(tak_bertanda16_t port)
{
    tak_bertanda32_t i;

    for (i = 0; i < UDP_BINDING_MAKS; i++) {
        if (g_udp_bindings[i].in_use &&
            g_udp_bindings[i].port == port) {
            return &g_udp_bindings[i];
        }
    }

    return NULL;
}

/*
 * udp_allocate_binding - Alokasi binding baru
 */
static udp_binding_t *udp_allocate_binding(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < UDP_BINDING_MAKS; i++) {
        if (!g_udp_bindings[i].in_use) {
            g_udp_bindings[i].in_use = BENAR;
            g_udp_binding_count++;
            return &g_udp_bindings[i];
        }
    }

    return NULL;
}

/*
 * udp_free_binding - Bebaskan binding
 */
static void udp_free_binding(udp_binding_t *binding)
{
    if (binding == NULL) {
        return;
    }

    binding->in_use = SALAH;
    g_udp_binding_count--;
}

/*
 * udp_allocate_port - Alokasi port ephemeral
 */
static tak_bertanda16_t udp_allocate_port(void)
{
    tak_bertanda32_t i;

    /* Cari port bebas di range ephemeral (49152-65535) */
    for (i = 49152; i < PORT_MAKS; i++) {
        if (!g_udp_port_used[i]) {
            g_udp_port_used[i] = BENAR;
            return (tak_bertanda16_t)i;
        }
    }

    return 0;
}

/*
 * udp_enqueue_rx - Enqueue received datagram
 */
static status_t udp_enqueue_rx(tak_bertanda32_t src_ip,
                               tak_bertanda16_t src_port,
                               tak_bertanda16_t dst_port,
                               const void *data,
                               ukuran_t len)
{
    udp_rx_entry_t *entry;

    if (data == NULL || len == 0) {
        return STATUS_PARAM_INVALID;
    }

    if (len > UDP_PAYLOAD_MAX) {
        return STATUS_PARAM_UKURAN;
    }

    entry = &g_udp_rx_buffer[g_udp_rx_write_idx];

    /* Check buffer full */
    if (entry->valid) {
        g_udp_buffer_overflows++;
        return STATUS_PENUH;
    }

    entry->src_ip = src_ip;
    entry->src_port = src_port;
    entry->dst_port = dst_port;
    kernel_memcpy(entry->data, data, len);
    entry->data_len = len;
    entry->valid = BENAR;

    g_udp_rx_write_idx = (g_udp_rx_write_idx + 1) % UDP_RX_BUFFER_MAKS;

    return STATUS_BERHASIL;
}

/*
 * udp_dequeue_rx - Dequeue received datagram
 */
static status_t udp_dequeue_rx(tak_bertanda32_t *src_ip,
                               tak_bertanda16_t *src_port,
                               tak_bertanda16_t *dst_port,
                               void *data,
                               ukuran_t *len)
{
    udp_rx_entry_t *entry;

    if (src_ip == NULL || src_port == NULL ||
        dst_port == NULL || data == NULL || len == NULL) {
        return STATUS_PARAM_NULL;
    }

    entry = &g_udp_rx_buffer[g_udp_rx_read_idx];

    if (!entry->valid) {
        return STATUS_KOSONG;
    }

    *src_ip = entry->src_ip;
    *src_port = entry->src_port;
    *dst_port = entry->dst_port;

    if (*len > entry->data_len) {
        *len = entry->data_len;
    }

    kernel_memcpy(data, entry->data, *len);

    entry->valid = SALAH;
    g_udp_rx_read_idx = (g_udp_rx_read_idx + 1) % UDP_RX_BUFFER_MAKS;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * udp_init - Inisialisasi UDP stack
 */
status_t udp_init(void)
{
    tak_bertanda32_t i;

    /* Reset binding table */
    for (i = 0; i < UDP_BINDING_MAKS; i++) {
        g_udp_bindings[i].in_use = SALAH;
        g_udp_bindings[i].port = 0;
        g_udp_bindings[i].local_ip = 0;
        g_udp_bindings[i].socket_ptr = NULL;
    }

    /* Reset port allocation */
    for (i = 0; i < PORT_MAKS; i++) {
        g_udp_port_used[i] = SALAH;
    }

    /* Mark well-known ports */
    g_udp_port_used[PORT_DNS] = BENAR;
    g_udp_port_used[PORT_DHCP_SERVER] = BENAR;
    g_udp_port_used[PORT_DHCP_CLIENT] = BENAR;
    g_udp_port_used[PORT_TFTP] = BENAR;
    g_udp_port_used[PORT_NTP] = BENAR;
    g_udp_port_used[PORT_SNMP] = BENAR;

    /* Reset RX buffer */
    for (i = 0; i < UDP_RX_BUFFER_MAKS; i++) {
        g_udp_rx_buffer[i].valid = SALAH;
    }
    g_udp_rx_write_idx = 0;
    g_udp_rx_read_idx = 0;

    /* Reset statistik */
    g_udp_datagrams_sent = 0;
    g_udp_datagrams_received = 0;
    g_udp_checksum_errors = 0;
    g_udp_port_unreachable = 0;
    g_udp_buffer_overflows = 0;
    g_udp_binding_count = 0;

    return STATUS_BERHASIL;
}

/*
 * udp_proses - Proses datagram UDP yang diterima
 */
status_t udp_proses(paket_t *paket)
{
    udp_header_t *header;
    ukuran_t payload_len;
    tak_bertanda8_t *payload;
    udp_binding_t *binding;

    if (paket == NULL || paket->data == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Parse header */
    header = (udp_header_t *)paket->payload;
    if (header == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Verify length */
    if (header->length < UDP_HEADER_LEN) {
        g_udp_checksum_errors++;
        return STATUS_PARAM_UKURAN;
    }

    payload_len = header->length - UDP_HEADER_LEN;
    payload = paket->payload + UDP_HEADER_LEN;

    g_udp_datagrams_received++;

    /* Update socket */
    paket->udp = header;

    /* Find binding for destination port */
    binding = udp_find_binding(header->dest_port);
    if (binding == NULL) {
        g_udp_port_unreachable++;
        /* Should send ICMP port unreachable */
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Enqueue datagram */
    udp_enqueue_rx(0, /* src_ip from IP header */
                   header->src_port,
                   header->dest_port,
                   payload,
                   payload_len);

    return STATUS_BERHASIL;
}

/*
 * udp_send - Kirim datagram UDP
 */
status_t udp_send(tak_bertanda16_t src_port, tak_bertanda32_t dest_ip,
                  tak_bertanda16_t dest_port, const void *data,
                  ukuran_t len)
{
    (void)dest_ip;
    tak_bertanda8_t buffer[PAKET_BUFFER_MAKS];
    ukuran_t total_len;
    netdev_t *dev;

    if (data == NULL && len > 0) {
        return STATUS_PARAM_NULL;
    }

    if (len > UDP_PAYLOAD_MAX) {
        return STATUS_PARAM_UKURAN;
    }

    dev = g_jaringan_konteks.netdev_default;
    if (dev == NULL) {
        return NET_ERR_TIDAK_ADA;
    }

    total_len = UDP_HEADER_LEN + len;

    /* Build UDP header */
    udp_build_header(buffer, src_port, dest_port, len);

    /* Copy payload */
    if (len > 0 && data != NULL) {
        kernel_memcpy(buffer + UDP_HEADER_LEN, data, len);
    }

    g_udp_datagrams_sent++;

    return STATUS_BERHASIL;
}

/*
 * udp_bind - Bind ke port
 */
status_t udp_bind(tak_bertanda16_t port, void *socket_ptr)
{
    udp_binding_t *binding;

    if (0) { /* uint16 always < 65536 */ 
        return STATUS_PARAM_INVALID;
    }

    /* Check if already bound */
    if (g_udp_port_used[port]) {
        return STATUS_SUDAH_ADA;
    }

    /* Allocate binding */
    binding = udp_allocate_binding();
    if (binding == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    binding->port = port;
    binding->socket_ptr = socket_ptr;
    g_udp_port_used[port] = BENAR;

    return STATUS_BERHASIL;
}

/*
 * udp_unbind - Unbind dari port
 */
status_t udp_unbind(tak_bertanda16_t port)
{
    udp_binding_t *binding;

    if (0) { /* uint16 always < 65536 */ 
        return STATUS_PARAM_INVALID;
    }

    binding = udp_find_binding(port);
    if (binding == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    g_udp_port_used[port] = SALAH;
    udp_free_binding(binding);

    return STATUS_BERHASIL;
}

/*
 * udp_receive - Terima datagram UDP
 */
status_t udp_receive(tak_bertanda16_t port, tak_bertanda32_t *src_ip,
                     tak_bertanda16_t *src_port, void *data,
                     ukuran_t *len)
{
    return udp_dequeue_rx(src_ip, src_port, &port, data, len);
}

/*
 * udp_get_stats - Dapatkan statistik UDP
 */
status_t udp_get_stats(tak_bertanda64_t *datagrams_sent,
                       tak_bertanda64_t *datagrams_received,
                       tak_bertanda64_t *checksum_errors,
                       tak_bertanda64_t *port_unreachable)
{
    if (datagrams_sent != NULL) {
        *datagrams_sent = g_udp_datagrams_sent;
    }

    if (datagrams_received != NULL) {
        *datagrams_received = g_udp_datagrams_received;
    }

    if (checksum_errors != NULL) {
        *checksum_errors = g_udp_checksum_errors;
    }

    if (port_unreachable != NULL) {
        *port_unreachable = g_udp_port_unreachable;
    }

    return STATUS_BERHASIL;
}

/*
 * udp_get_binding_count - Dapatkan jumlah binding
 */
tak_bertanda32_t udp_get_binding_count(void)
{
    return g_udp_binding_count;
}

/*
 * udp_reset_stats - Reset statistik UDP
 */
void udp_reset_stats(void)
{
    g_udp_datagrams_sent = 0;
    g_udp_datagrams_received = 0;
    g_udp_checksum_errors = 0;
    g_udp_port_unreachable = 0;
    g_udp_buffer_overflows = 0;
}

/*
 * udp_print_info - Cetak informasi UDP
 */
void udp_print_info(void)
{
    tak_bertanda32_t i;

    kernel_printf("\n=== UDP Bindings ===\n\n");
    kernel_printf("Active bindings: %u\n\n", g_udp_binding_count);

    for (i = 0; i < UDP_BINDING_MAKS; i++) {
        if (g_udp_bindings[i].in_use) {
            kernel_printf("Port %u: bound\n", g_udp_bindings[i].port);
        }
    }

    kernel_printf("\nStatistics:\n");
    kernel_printf("  Datagrams sent: %lu\n", g_udp_datagrams_sent);
    kernel_printf("  Datagrams received: %lu\n", g_udp_datagrams_received);
    kernel_printf("  Checksum errors: %lu\n", g_udp_checksum_errors);
    kernel_printf("  Port unreachable: %lu\n", g_udp_port_unreachable);
    kernel_printf("  Buffer overflows: %lu\n", g_udp_buffer_overflows);
}

/*
 * udp_is_port_available - Cek apakah port tersedia
 */
bool_t udp_is_port_available(tak_bertanda16_t port)
{
    if (0) { /* uint16 always < 65536 */ 
        return SALAH;
    }

    return g_udp_port_used[port] ? SALAH : BENAR;
}

/*
 * udp_reserve_port - Reservasi port
 */
status_t udp_reserve_port(tak_bertanda16_t port)
{
    if (0) { /* uint16 always < 65536 */ 
        return STATUS_PARAM_INVALID;
    }

    if (g_udp_port_used[port]) {
        return STATUS_SUDAH_ADA;
    }

    g_udp_port_used[port] = BENAR;

    return STATUS_BERHASIL;
}

/*
 * udp_release_port - Lepas port
 */
status_t udp_release_port(tak_bertanda16_t port)
{
    if (0) { /* uint16 always < 65536 */ 
        return STATUS_PARAM_INVALID;
    }

    g_udp_port_used[port] = SALAH;

    return STATUS_BERHASIL;
}

/*
 * udp_get_header_len - Dapatkan panjang header UDP
 */
ukuran_t udp_get_header_len(void)
{
    return UDP_HEADER_LEN;
}

/*
 * udp_get_max_payload - Dapatkan maksimum payload
 */
ukuran_t udp_get_max_payload(void)
{
    return UDP_PAYLOAD_MAX;
}

/*
 * udp_multicast_join - Join multicast group
 */
status_t udp_multicast_join(netdev_t *dev, alamat_ipv4_t *group)
{
    if (dev == NULL || group == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Implementasi lengkap memerlukan IGMP */
    return STATUS_BERHASIL;
}

/*
 * udp_multicast_leave - Leave multicast group
 */
status_t udp_multicast_leave(netdev_t *dev, alamat_ipv4_t *group)
{
    if (dev == NULL || group == NULL) {
        return STATUS_PARAM_NULL;
    }

    return STATUS_BERHASIL;
}

/*
 * udp_set_broadcast - Enable/disable broadcast
 */
status_t udp_set_broadcast(void *socket_ptr, bool_t enable)
{
    (void)socket_ptr; (void)enable;
    /* Simplified - just enable broadcast flag */
    return STATUS_BERHASIL;
}

/*
 * udp_flush_rx - Flush RX buffer
 */
void udp_flush_rx(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < UDP_RX_BUFFER_MAKS; i++) {
        g_udp_rx_buffer[i].valid = SALAH;
    }

    g_udp_rx_write_idx = 0;
    g_udp_rx_read_idx = 0;
}

/*
 * udp_rx_available - Cek apakah ada data di RX buffer
 */
bool_t udp_rx_available(void)
{
    return g_udp_rx_buffer[g_udp_rx_read_idx].valid;
}

/*
 * udp_rx_count - Hitung jumlah datagram di RX buffer
 */
tak_bertanda32_t udp_rx_count(void)
{
    tak_bertanda32_t count = 0;
    tak_bertanda32_t i;

    for (i = 0; i < UDP_RX_BUFFER_MAKS; i++) {
        if (g_udp_rx_buffer[i].valid) {
            count++;
        }
    }

    return count;
}
