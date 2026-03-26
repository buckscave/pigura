/*
 * PIGURA OS - TCP.C
 * ==================
 * Implementasi protokol TCP.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi untuk menangani
 * protokol TCP (Transmission Control Protocol) dalam Pigura OS.
 *
 * Kepatuhan standar:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Mendukung semua arsitektur
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "jaringan.h"

/*
 * ===========================================================================
 * KONSTANTA TCP
 * ===========================================================================
 */

/* TCP header size */
#define TCP_HEADER_MIN_LEN   20
#define TCP_HEADER_MAX_LEN   60
#define TCP_OPTION_MSS       2
#define TCP_OPTION_WINDOW    3
#define TCP_OPTION_SACK      4
#define TCP_OPTION_TIMESTAMP 8

/* Default values */
#define TCP_DEFAULT_MSS      1460
#define TCP_DEFAULT_WINDOW   65535
#define TCP_DEFAULT_TTL      64

/* Timeout values (ms) */
#define TCP_TIMEOUT_SYN      3000
#define TCP_TIMEOUT_FIN      3000
#define TCP_TIMEOUT_KEEPALIVE 7200000

/* Sequence number range */
#define TCP_SEQ_WRAP         0xFFFFFFFF

/*
 * ===========================================================================
 * STRUKTUR TCP INTERNAL
 * ===========================================================================
 */

/* TCP connection block */
typedef struct {
    tak_bertanda32_t local_ip;
    tak_bertanda16_t local_port;
    tak_bertanda32_t remote_ip;
    tak_bertanda16_t remote_port;
    tak_bertanda32_t state;
    tak_bertanda32_t seq_num;
    tak_bertanda32_t ack_num;
    tak_bertanda16_t window;
    tak_bertanda32_t mss;
    bool_t active;
} tcp_connection_t;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

/* Connection table */
#define TCP_CONNECTION_MAKS 64
static tcp_connection_t g_tcp_connections[TCP_CONNECTION_MAKS];
static tak_bertanda32_t g_tcp_connection_count = 0;

/* Port allocation */
static bool_t g_tcp_port_used[PORT_MAKS];

/* Statistics */
static tak_bertanda64_t g_tcp_segments_sent = 0;
static tak_bertanda64_t g_tcp_segments_received = 0;
static tak_bertanda64_t g_tcp_retransmissions = 0;
static tak_bertanda64_t g_tcp_checksum_errors = 0;
static tak_bertanda64_t g_tcp_connections_active = 0;
static tak_bertanda64_t g_tcp_connections_passive = 0;

/* Initial sequence number */
static tak_bertanda32_t g_tcp_initial_seq = 0x12345678;

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * tcp_checksum - Hitung checksum TCP
 */
static tak_bertanda16_t tcp_checksum(const void *data, ukuran_t len,
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
    sum += PROTO_TCP;
    sum += len;

    /* TCP segment */
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
 * tcp_build_header - Bangun header TCP
 */
static void tcp_build_header(tak_bertanda8_t *buffer,
                             tak_bertanda16_t src_port,
                             tak_bertanda16_t dest_port,
                             tak_bertanda32_t seq_num,
                             tak_bertanda32_t ack_num,
                             tak_bertanda8_t flags,
                             tak_bertanda16_t window,
                             ukuran_t payload_len)
{
    tcp_header_t *header;

    if (buffer == NULL) {
        return;
    }

    header = (tcp_header_t *)buffer;

    header->src_port = src_port;
    header->dest_port = dest_port;
    header->seq_num = seq_num;
    header->ack_num = ack_num;
    header->data_offset = (TCP_HEADER_MIN_LEN / 4) << 4;
    header->flags = flags;
    header->window = window;
    header->checksum = 0;
    header->urgent_ptr = 0;

    /* Hitung checksum */
    /* Note: Untuk checksum yang benar, perlu pseudo-header IP */
    header->checksum = hitung_checksum(header, TCP_HEADER_MIN_LEN);
}

/*
 * tcp_find_connection - Cari connection berdasarkan alamat
 */
static tcp_connection_t *tcp_find_connection(tak_bertanda16_t local_port,
                                             tak_bertanda32_t remote_ip,
                                             tak_bertanda16_t remote_port)
{
    tak_bertanda32_t i;

    for (i = 0; i < TCP_CONNECTION_MAKS; i++) {
        if (g_tcp_connections[i].active &&
            g_tcp_connections[i].local_port == local_port &&
            g_tcp_connections[i].remote_ip == remote_ip &&
            g_tcp_connections[i].remote_port == remote_port) {
            return &g_tcp_connections[i];
        }
    }

    return NULL;
}

/*
 * tcp_allocate_connection - Alokasi connection baru
 */
static tcp_connection_t *tcp_allocate_connection(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < TCP_CONNECTION_MAKS; i++) {
        if (!g_tcp_connections[i].active) {
            g_tcp_connections[i].active = BENAR;
            g_tcp_connections[i].seq_num = g_tcp_initial_seq++;
            g_tcp_connections[i].ack_num = 0;
            g_tcp_connections[i].window = TCP_DEFAULT_WINDOW;
            g_tcp_connections[i].mss = TCP_DEFAULT_MSS;
            g_tcp_connections[i].state = TCP_STATE_CLOSED;
            g_tcp_connection_count++;
            return &g_tcp_connections[i];
        }
    }

    return NULL;
}

/*
 * tcp_free_connection - Bebaskan connection
 */
static void tcp_free_connection(tcp_connection_t *conn)
{
    if (conn == NULL) {
        return;
    }

    conn->active = SALAH;
    g_tcp_connection_count--;
}

/*
 * tcp_allocate_port - Alokasi port ephemeral
 */
static tak_bertanda16_t tcp_allocate_port(void)
{
    tak_bertanda32_t i;

    /* Cari port bebas di range ephemeral (49152-65535) */
    for (i = 49152; i < PORT_MAKS; i++) {
        if (!g_tcp_port_used[i]) {
            g_tcp_port_used[i] = BENAR;
            return (tak_bertanda16_t)i;
        }
    }

    return 0;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * tcp_init - Inisialisasi TCP stack
 */
status_t tcp_init(void)
{
    tak_bertanda32_t i;

    /* Reset connection table */
    for (i = 0; i < TCP_CONNECTION_MAKS; i++) {
        g_tcp_connections[i].active = SALAH;
        g_tcp_connections[i].local_port = 0;
        g_tcp_connections[i].remote_ip = 0;
        g_tcp_connections[i].remote_port = 0;
        g_tcp_connections[i].state = TCP_STATE_CLOSED;
    }

    /* Reset port allocation */
    for (i = 0; i < PORT_MAKS; i++) {
        g_tcp_port_used[i] = SALAH;
    }

    /* Mark well-known ports */
    g_tcp_port_used[PORT_FTP] = BENAR;
    g_tcp_port_used[PORT_SSH] = BENAR;
    g_tcp_port_used[PORT_TELNET] = BENAR;
    g_tcp_port_used[PORT_SMTP] = BENAR;
    g_tcp_port_used[PORT_HTTP] = BENAR;
    g_tcp_port_used[PORT_POP3] = BENAR;
    g_tcp_port_used[PORT_IMAP] = BENAR;
    g_tcp_port_used[PORT_HTTPS] = BENAR;

    /* Reset statistik */
    g_tcp_segments_sent = 0;
    g_tcp_segments_received = 0;
    g_tcp_retransmissions = 0;
    g_tcp_checksum_errors = 0;
    g_tcp_connections_active = 0;
    g_tcp_connections_passive = 0;
    g_tcp_connection_count = 0;

    return STATUS_BERHASIL;
}

/*
 * tcp_proses - Proses segment TCP yang diterima
 */
status_t tcp_proses(paket_t *paket)
{
    tcp_header_t *header;
    tak_bertanda32_t header_len;
    tak_bertanda16_t checksum_calc;
    tcp_connection_t *conn;

    if (paket == NULL || paket->data == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Parse header */
    header = (tcp_header_t *)paket->payload;
    if (header == NULL) {
        return STATUS_PARAM_INVALID;
    }

    header_len = (header->data_offset >> 4) * 4;
    if (header_len < TCP_HEADER_MIN_LEN) {
        g_tcp_checksum_errors++;
        return STATUS_PARAM_INVALID;
    }

    /* Verifikasi checksum */
    /* Simplified - seharusnya dengan pseudo-header IP */
    checksum_calc = hitung_checksum(header, header_len);
    if (checksum_calc != 0 && header->checksum != 0) {
        g_tcp_checksum_errors++;
        return STATUS_PARAM_INVALID;
    }

    g_tcp_segments_received++;

    /* Update socket */
    paket->tcp = header;

    /* Cari connection */
    conn = tcp_find_connection(header->dest_port,
                               0, /* remote IP dari IP header */
                               header->src_port);

    if (conn == NULL) {
        /* Tidak ada connection, mungkin SYN baru */
        if (header->flags & TCP_FLAG_SYN) {
            /* Handle new connection attempt */
        }
        return STATUS_BERHASIL;
    }

    /* Process berdasarkan state */
    switch (conn->state) {
    case TCP_STATE_LISTEN:
        /* Expect SYN */
        if (header->flags & TCP_FLAG_SYN) {
            conn->ack_num = header->seq_num + 1;
            conn->remote_port = header->src_port;
            conn->state = TCP_STATE_SYN_RECEIVED;
        }
        break;

    case TCP_STATE_SYN_SENT:
        /* Expect SYN-ACK */
        if ((header->flags & (TCP_FLAG_SYN | TCP_FLAG_ACK)) ==
            (TCP_FLAG_SYN | TCP_FLAG_ACK)) {
            conn->ack_num = header->seq_num + 1;
            conn->state = TCP_STATE_ESTABLISHED;
        }
        break;

    case TCP_STATE_SYN_RECEIVED:
        /* Expect ACK */
        if (header->flags & TCP_FLAG_ACK) {
            conn->state = TCP_STATE_ESTABLISHED;
            g_tcp_connections_active++;
        }
        break;

    case TCP_STATE_ESTABLISHED:
        /* Normal data transfer */
        if (header->flags & TCP_FLAG_FIN) {
            conn->state = TCP_STATE_CLOSE_WAIT;
        }
        break;

    case TCP_STATE_FIN_WAIT_1:
        if (header->flags & TCP_FLAG_ACK) {
            conn->state = TCP_STATE_FIN_WAIT_2;
        }
        break;

    case TCP_STATE_FIN_WAIT_2:
        if (header->flags & TCP_FLAG_FIN) {
            conn->state = TCP_STATE_TIME_WAIT;
        }
        break;

    case TCP_STATE_CLOSE_WAIT:
        /* Waiting for application to close */
        break;

    case TCP_STATE_CLOSING:
        if (header->flags & TCP_FLAG_ACK) {
            conn->state = TCP_STATE_TIME_WAIT;
        }
        break;

    case TCP_STATE_LAST_ACK:
        if (header->flags & TCP_FLAG_ACK) {
            conn->state = TCP_STATE_CLOSED;
            tcp_free_connection(conn);
        }
        break;

    case TCP_STATE_TIME_WAIT:
        /* Wait 2MSL then close */
        conn->state = TCP_STATE_CLOSED;
        tcp_free_connection(conn);
        break;

    default:
        break;
    }

    /* Update sequence numbers */
    if (header->flags & TCP_FLAG_SYN) {
        conn->ack_num++;
    }
    if (header->flags & TCP_FLAG_FIN) {
        conn->ack_num++;
    }
    /* Add payload length to ack */

    return STATUS_BERHASIL;
}

/*
 * tcp_kirim_syn - Kirim SYN packet
 */
status_t tcp_kirim_syn(socket_t *sock)
{
    tak_bertanda8_t buffer[TCP_HEADER_MAX_LEN];
    tcp_connection_t *conn;
    netdev_t *dev;

    if (sock == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Get default device */
    dev = g_jaringan_konteks.netdev_default;
    if (dev == NULL) {
        return NET_ERR_TIDAK_ADA;
    }

    /* Allocate connection */
    conn = tcp_allocate_connection();
    if (conn == NULL) {
        return NET_ERR_NO_MEMORY;
    }

    /* Setup connection */
    conn->local_port = tcp_allocate_port();
    conn->remote_ip = 0; /* From socket address */
    conn->remote_port = 0; /* From socket address */
    conn->state = TCP_STATE_SYN_SENT;

    /* Build SYN header */
    tcp_build_header(buffer, conn->local_port, conn->remote_port,
                     conn->seq_num, 0, TCP_FLAG_SYN, conn->window, 0);

    /* Update socket */
    sock->tcp_state = TCP_STATE_SYN_SENT;
    sock->seq_num = conn->seq_num;
    sock->window = conn->window;

    g_tcp_segments_sent++;

    return STATUS_BERHASIL;
}

/*
 * tcp_kirim_ack - Kirim ACK packet
 */
status_t tcp_kirim_ack(socket_t *sock)
{
    tak_bertanda8_t buffer[TCP_HEADER_MAX_LEN];
    netdev_t *dev;

    if (sock == NULL) {
        return STATUS_PARAM_NULL;
    }

    dev = g_jaringan_konteks.netdev_default;
    if (dev == NULL) {
        return NET_ERR_TIDAK_ADA;
    }

    /* Build ACK header */
    tcp_build_header(buffer, 0, 0, sock->seq_num, sock->ack_num,
                     TCP_FLAG_ACK, sock->window, 0);

    g_tcp_segments_sent++;

    return STATUS_BERHASIL;
}

/*
 * tcp_kirim_fin - Kirim FIN packet
 */
status_t tcp_kirim_fin(socket_t *sock)
{
    tak_bertanda8_t buffer[TCP_HEADER_MAX_LEN];
    netdev_t *dev;

    if (sock == NULL) {
        return STATUS_PARAM_NULL;
    }

    dev = g_jaringan_konteks.netdev_default;
    if (dev == NULL) {
        return NET_ERR_TIDAK_ADA;
    }

    /* Build FIN header */
    tcp_build_header(buffer, 0, 0, sock->seq_num, sock->ack_num,
                     TCP_FLAG_FIN | TCP_FLAG_ACK, sock->window, 0);

    sock->seq_num++;
    sock->tcp_state = TCP_STATE_FIN_WAIT_1;

    g_tcp_segments_sent++;

    return STATUS_BERHASIL;
}

/*
 * tcp_kirim_rst - Kirim RST packet
 */
status_t tcp_kirim_rst(socket_t *sock)
{
    tak_bertanda8_t buffer[TCP_HEADER_MAX_LEN];
    netdev_t *dev;

    if (sock == NULL) {
        return STATUS_PARAM_NULL;
    }

    dev = g_jaringan_konteks.netdev_default;
    if (dev == NULL) {
        return NET_ERR_TIDAK_ADA;
    }

    /* Build RST header */
    tcp_build_header(buffer, 0, 0, 0, 0, TCP_FLAG_RST, 0, 0);

    sock->tcp_state = TCP_STATE_CLOSED;

    g_tcp_segments_sent++;

    return STATUS_BERHASIL;
}

/*
 * tcp_send_data - Kirim data TCP
 */
status_t tcp_send_data(socket_t *sock, const void *data, ukuran_t len,
                       ukuran_t *bytes_sent)
{
    tak_bertanda8_t buffer[PAKET_BUFFER_MAKS];
    ukuran_t header_len;
    ukuran_t payload_len;
    netdev_t *dev;

    if (sock == NULL || data == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (sock->tcp_state != TCP_STATE_ESTABLISHED) {
        return NET_ERR_CONN_REFUSED;
    }

    dev = g_jaringan_konteks.netdev_default;
    if (dev == NULL) {
        return NET_ERR_TIDAK_ADA;
    }

    header_len = TCP_HEADER_MIN_LEN;
    payload_len = len;
    if (payload_len > TCP_DEFAULT_MSS) {
        payload_len = TCP_DEFAULT_MSS;
    }

    /* Build TCP header */
    tcp_build_header(buffer, 0, 0, sock->seq_num, sock->ack_num,
                     TCP_FLAG_ACK | TCP_FLAG_PSH, sock->window, payload_len);

    /* Copy payload */
    if (payload_len > 0) {
        kernel_memcpy(buffer + header_len, data, payload_len);
    }

    /* Update sequence */
    sock->seq_num += payload_len;

    if (bytes_sent != NULL) {
        *bytes_sent = payload_len;
    }

    g_tcp_segments_sent++;

    return STATUS_BERHASIL;
}

/*
 * tcp_receive_data - Terima data TCP
 */
status_t tcp_receive_data(socket_t *sock, void *data, ukuran_t len,
                          ukuran_t *bytes_received)
{
    if (sock == NULL || data == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (sock->tcp_state != TCP_STATE_ESTABLISHED) {
        return NET_ERR_CONN_RESET;
    }

    /* Simplified - read from socket buffer */
    if (bytes_received != NULL) {
        *bytes_received = 0;
    }

    return STATUS_BERHASIL;
}

/*
 * tcp_get_stats - Dapatkan statistik TCP
 */
status_t tcp_get_stats(tak_bertanda64_t *segments_sent,
                       tak_bertanda64_t *segments_received,
                       tak_bertanda64_t *retransmissions,
                       tak_bertanda64_t *checksum_errors)
{
    if (segments_sent != NULL) {
        *segments_sent = g_tcp_segments_sent;
    }

    if (segments_received != NULL) {
        *segments_received = g_tcp_segments_received;
    }

    if (retransmissions != NULL) {
        *retransmissions = g_tcp_retransmissions;
    }

    if (checksum_errors != NULL) {
        *checksum_errors = g_tcp_checksum_errors;
    }

    return STATUS_BERHASIL;
}

/*
 * tcp_get_connection_count - Dapatkan jumlah connection
 */
tak_bertanda32_t tcp_get_connection_count(void)
{
    return g_tcp_connection_count;
}

/*
 * tcp_reset_stats - Reset statistik TCP
 */
void tcp_reset_stats(void)
{
    g_tcp_segments_sent = 0;
    g_tcp_segments_received = 0;
    g_tcp_retransmissions = 0;
    g_tcp_checksum_errors = 0;
}

/*
 * tcp_print_info - Cetak informasi TCP
 */
void tcp_print_info(void)
{
    tak_bertanda32_t i;

    kernel_printf("\n=== TCP Connections ===\n\n");
    kernel_printf("Active connections: %u\n\n", g_tcp_connection_count);

    for (i = 0; i < TCP_CONNECTION_MAKS; i++) {
        if (g_tcp_connections[i].active) {
            kernel_printf("Connection %u:\n", i);
            kernel_printf("  State: %u\n", g_tcp_connections[i].state);
            kernel_printf("  Local port: %u\n",
                          g_tcp_connections[i].local_port);
            kernel_printf("  Remote port: %u\n",
                          g_tcp_connections[i].remote_port);
            kernel_printf("\n");
        }
    }

    kernel_printf("Statistics:\n");
    kernel_printf("  Segments sent: %lu\n", g_tcp_segments_sent);
    kernel_printf("  Segments received: %lu\n", g_tcp_segments_received);
    kernel_printf("  Retransmissions: %lu\n", g_tcp_retransmissions);
    kernel_printf("  Checksum errors: %lu\n", g_tcp_checksum_errors);
}

/*
 * tcp_is_port_available - Cek apakah port tersedia
 */
bool_t tcp_is_port_available(tak_bertanda16_t port)
{
    if (port >= PORT_MAKS) {
        return SALAH;
    }

    return g_tcp_port_used[port] ? SALAH : BENAR;
}

/*
 * tcp_reserve_port - Reservasi port
 */
status_t tcp_reserve_port(tak_bertanda16_t port)
{
    if (port >= PORT_MAKS) {
        return STATUS_PARAM_INVALID;
    }

    if (g_tcp_port_used[port]) {
        return STATUS_SUDAH_ADA;
    }

    g_tcp_port_used[port] = BENAR;

    return STATUS_BERHASIL;
}

/*
 * tcp_release_port - Lepas port
 */
status_t tcp_release_port(tak_bertanda16_t port)
{
    if (port >= PORT_MAKS) {
        return STATUS_PARAM_INVALID;
    }

    g_tcp_port_used[port] = SALAH;

    return STATUS_BERHASIL;
}
