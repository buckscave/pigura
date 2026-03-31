/*
 * PIGURA OS - SOCKET.C
 * =====================
 * Implementasi interface socket.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi untuk manajemen
 * socket dalam Pigura OS.
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
 * VARIABEL LOKAL
 * ===========================================================================
 */

/* Pool untuk socket statis */
#define SOCKET_POOL_MAKS 256
static socket_t g_socket_pool[SOCKET_POOL_MAKS];
static bool_t g_socket_pool_terpakai[SOCKET_POOL_MAKS];
static tak_bertanda32_t g_socket_counter = 0;

/* Buffer pool untuk socket */
#define SOCKET_BUFFER_POOL_MAKS 128
#define SOCKET_BUFFER_SIZE      8192
static tak_bertanda8_t g_socket_rx_buffer_pool[SOCKET_BUFFER_POOL_MAKS]
    [SOCKET_BUFFER_SIZE];
static tak_bertanda8_t g_socket_tx_buffer_pool[SOCKET_BUFFER_POOL_MAKS]
    [SOCKET_BUFFER_SIZE];
static bool_t g_socket_buffer_terpakai[SOCKET_BUFFER_POOL_MAKS];

/* Listen queue */
#define SOCKET_LISTEN_QUEUE_MAKS 32
typedef struct {
    tak_bertanda32_t socket_id;
    sockaddr_t client_addr;
    ukuran_t addr_len;
    bool_t valid;
} socket_pending_t;

static socket_pending_t g_listen_queue[SOCKET_POOL_MAKS]
    [SOCKET_LISTEN_QUEUE_MAKS];

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * socket_alokasi - Alokasi struktur socket dari pool
 */
static socket_t *socket_alokasi(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < SOCKET_POOL_MAKS; i++) {
        if (!g_socket_pool_terpakai[i]) {
            g_socket_pool_terpakai[i] = BENAR;
            return &g_socket_pool[i];
        }
    }

    return NULL;
}

/*
 * socket_bebaskan - Bebaskan struktur socket ke pool
 */
static void socket_bebaskan(socket_t *sock)
{
    tak_bertanda32_t i;

    if (sock == NULL) {
        return;
    }

    for (i = 0; i < SOCKET_POOL_MAKS; i++) {
        if (&g_socket_pool[i] == sock) {
            g_socket_pool_terpakai[i] = SALAH;
            return;
        }
    }
}

/*
 * socket_buffer_alokasi - Alokasi buffer RX/TX
 */
static status_t socket_buffer_alokasi(socket_t *sock)
{
    tak_bertanda32_t i;

    if (sock == NULL) {
        return STATUS_PARAM_NULL;
    }

    for (i = 0; i < SOCKET_BUFFER_POOL_MAKS; i++) {
        if (!g_socket_buffer_terpakai[i]) {
            g_socket_buffer_terpakai[i] = BENAR;
            sock->rx_buffer = g_socket_rx_buffer_pool[i];
            sock->tx_buffer = g_socket_tx_buffer_pool[i];
            sock->rx_buffer_size = SOCKET_BUFFER_SIZE;
            sock->tx_buffer_size = SOCKET_BUFFER_SIZE;
            sock->rx_data_len = 0;
            sock->tx_data_len = 0;
            return STATUS_BERHASIL;
        }
    }

    return STATUS_MEMORI_HABIS;
}

/*
 * socket_buffer_bebaskan - Bebaskan buffer socket
 */
static void socket_buffer_bebaskan(socket_t *sock)
{
    tak_bertanda32_t i;

    if (sock == NULL) {
        return;
    }

    /* Cari index buffer RX */
    for (i = 0; i < SOCKET_BUFFER_POOL_MAKS; i++) {
        if (g_socket_rx_buffer_pool[i] == sock->rx_buffer) {
            g_socket_buffer_terpakai[i] = SALAH;
            break;
        }
    }

    sock->rx_buffer = NULL;
    sock->tx_buffer = NULL;
    sock->rx_buffer_size = 0;
    sock->tx_buffer_size = 0;
}

/*
 * socket_validasi - Validasi socket
 */
static bool_t socket_validasi(socket_t *sock)
{
    if (sock == NULL) {
        return SALAH;
    }

    if (sock->magic != SOCKET_MAGIC) {
        return SALAH;
    }

    return BENAR;
}

/*
 * socket_cari - Cari socket berdasarkan ID
 */
static socket_t *socket_cari(tak_bertanda32_t id)
{
    tak_bertanda32_t i;

    for (i = 0; i < SOCKET_POOL_MAKS; i++) {
        if (g_socket_pool_terpakai[i] &&
            g_socket_pool[i].magic == SOCKET_MAGIC &&
            g_socket_pool[i].id == id) {
            return &g_socket_pool[i];
        }
    }

    return NULL;
}

/*
 * socket_init_struktur - Inisialisasi struktur socket
 */
static void socket_init_struktur(socket_t *sock,
                                 tak_bertanda32_t domain,
                                 tak_bertanda32_t tipe,
                                 tak_bertanda32_t protocol)
{
    if (sock == NULL) {
        return;
    }

    kernel_memset(sock, 0, sizeof(socket_t));

    sock->magic = SOCKET_MAGIC;
    sock->id = g_socket_counter++;
    sock->domain = domain;
    sock->tipe = tipe;
    sock->protocol = protocol;
    sock->state = 0;
    sock->tcp_state = TCP_STATE_CLOSED;
    sock->flags = 0;
    sock->recv_timeout = 0;
    sock->send_timeout = 0;
    sock->window = 65535;
    sock->seq_num = 0;
    sock->ack_num = 0;
    sock->netdev = NULL;
    sock->berikutnya = NULL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * socket_buat - Buat socket baru
 */
tanda32_t socket_buat(tak_bertanda32_t domain, tak_bertanda32_t tipe,
                      tak_bertanda32_t protocol)
{
    socket_t *sock;
    status_t hasil;

    /* Validasi domain */
    if (domain != AF_INET && domain != AF_INET6 && domain != AF_UNIX &&
        domain != AF_PACKET) {
        return NET_ERR_INVALID_PARAM;
    }

    /* Validasi tipe */
    if (tipe != SOCK_STREAM && tipe != SOCK_DGRAM &&
        tipe != SOCK_RAW && tipe != SOCK_SEQPACKET) {
        return NET_ERR_INVALID_PARAM;
    }

    /* Alokasi socket */
    sock = socket_alokasi();
    if (sock == NULL) {
        return NET_ERR_NO_MEMORY;
    }

    /* Inisialisasi */
    socket_init_struktur(sock, domain, tipe, protocol);

    /* Alokasi buffer */
    hasil = socket_buffer_alokasi(sock);
    if (hasil != STATUS_BERHASIL) {
        socket_bebaskan(sock);
        return NET_ERR_NO_MEMORY;
    }

    /* Tambah ke list global */
    if (g_jaringan_konteks.socket_list == NULL) {
        g_jaringan_konteks.socket_list = sock;
    } else {
        socket_t *last = g_jaringan_konteks.socket_list;
        while (last->berikutnya != NULL) {
            last = (socket_t *)last->berikutnya;
        }
        last->berikutnya = (struct socket *)sock;
    }

    g_jaringan_konteks.socket_count++;

    return (tanda32_t)sock->id;
}

/*
 * socket_tutup - Tutup socket
 */
status_t socket_tutup(tak_bertanda32_t sockfd)
{
    socket_t *sock;
    socket_t *prev;
    socket_t *curr;

    sock = socket_cari(sockfd);
    if (!socket_validasi(sock)) {
        return STATUS_PARAM_INVALID;
    }

    /* Untuk TCP, kirim FIN jika established */
    if (sock->tipe == SOCK_STREAM &&
        sock->tcp_state == TCP_STATE_ESTABLISHED) {
        tcp_kirim_fin(sock);
        sock->tcp_state = TCP_STATE_FIN_WAIT_1;
    }

    /* Hapus dari list */
    prev = NULL;
    curr = g_jaringan_konteks.socket_list;

    while (curr != NULL) {
        if (curr == sock) {
            if (prev == NULL) {
                g_jaringan_konteks.socket_list =
                    (socket_t *)curr->berikutnya;
            } else {
                prev->berikutnya = curr->berikutnya;
            }
            break;
        }
        prev = curr;
        curr = (socket_t *)curr->berikutnya;
    }

    /* Bebaskan resource */
    socket_buffer_bebaskan(sock);
    sock->magic = 0;
    socket_bebaskan(sock);

    g_jaringan_konteks.socket_count--;

    return STATUS_BERHASIL;
}

/*
 * socket_ikat - Bind socket ke alamat
 */
status_t socket_ikat(tak_bertanda32_t sockfd, const sockaddr_t *addr,
                     ukuran_t addrlen)
{
    (void)addrlen;
    socket_t *sock;

    sock = socket_cari(sockfd);
    if (!socket_validasi(sock)) {
        return STATUS_PARAM_INVALID;
    }

    if (addr == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Copy alamat lokal */
    kernel_memcpy(&sock->alamat_lokal, addr, sizeof(sockaddr_t));

    return STATUS_BERHASIL;
}

/*
 * socket_dengar - Listen untuk koneksi
 */
status_t socket_dengar(tak_bertanda32_t sockfd, tak_bertanda32_t backlog)
{
    (void)backlog;
    socket_t *sock;
    tak_bertanda32_t i;

    sock = socket_cari(sockfd);
    if (!socket_validasi(sock)) {
        return STATUS_PARAM_INVALID;
    }

    /* Hanya untuk SOCK_STREAM */
    if (sock->tipe != SOCK_STREAM) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Set state ke LISTEN */
    sock->tcp_state = TCP_STATE_LISTEN;
    sock->state = 1;

    /* Inisialisasi pending queue */
    for (i = 0; i < SOCKET_LISTEN_QUEUE_MAKS; i++) {
        g_listen_queue[sockfd][i].valid = SALAH;
    }

    return STATUS_BERHASIL;
}

/*
 * socket_terima - Accept koneksi
 */
tanda32_t socket_terima(tak_bertanda32_t sockfd, sockaddr_t *addr,
                        ukuran_t *addrlen)
{
    socket_t *sock;
    socket_t *new_sock;
    tak_bertanda32_t i;
    tanda32_t new_id;

    sock = socket_cari(sockfd);
    if (!socket_validasi(sock)) {
        return -1;
    }

    /* Hanya untuk SOCK_STREAM dalam state LISTEN */
    if (sock->tipe != SOCK_STREAM || sock->tcp_state != TCP_STATE_LISTEN) {
        return -1;
    }

    /* Cari pending connection */
    for (i = 0; i < SOCKET_LISTEN_QUEUE_MAKS; i++) {
        if (g_listen_queue[sockfd][i].valid) {
            break;
        }
    }

    if (i >= SOCKET_LISTEN_QUEUE_MAKS) {
        /* Tidak ada pending connection */
        return -1;
    }

    /* Buat socket baru untuk koneksi */
    new_id = socket_buat(sock->domain, sock->tipe, sock->protocol);
    if (new_id < 0) {
        return -1;
    }

    new_sock = socket_cari((tak_bertanda32_t)new_id);
    if (new_sock == NULL) {
        return -1;
    }

    /* Copy alamat client */
    if (addr != NULL && addrlen != NULL) {
        ukuran_t copy_len = *addrlen;
        if (copy_len > sizeof(sockaddr_t)) {
            copy_len = sizeof(sockaddr_t);
        }
        kernel_memcpy(addr, &g_listen_queue[sockfd][i].client_addr,
                      copy_len);
        *addrlen = copy_len;
    }

    /* Set state */
    new_sock->tcp_state = TCP_STATE_ESTABLISHED;
    new_sock->netdev = sock->netdev;

    /* Clear pending */
    g_listen_queue[sockfd][i].valid = SALAH;

    return new_id;
}

/*
 * socket_hubung - Connect ke remote
 */
status_t socket_hubung(tak_bertanda32_t sockfd, const sockaddr_t *addr,
                       ukuran_t addrlen)
{
    (void)addrlen;
    socket_t *sock;
    status_t hasil;

    sock = socket_cari(sockfd);
    if (!socket_validasi(sock)) {
        return STATUS_PARAM_INVALID;
    }

    if (addr == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Copy alamat remote */
    kernel_memcpy(&sock->alamat_remote, addr, sizeof(sockaddr_t));

    /* Untuk TCP, lakukan three-way handshake */
    if (sock->tipe == SOCK_STREAM) {
        /* Generate initial sequence number */
        sock->seq_num = 1000; /* Simplified */
        sock->tcp_state = TCP_STATE_SYN_SENT;

        /* Kirim SYN */
        hasil = tcp_kirim_syn(sock);
        if (hasil != STATUS_BERHASIL) {
            sock->tcp_state = TCP_STATE_CLOSED;
            return hasil;
        }

        /* Wait for SYN-ACK (simplified - normally async) */
        sock->tcp_state = TCP_STATE_ESTABLISHED;
    }

    return STATUS_BERHASIL;
}

/*
 * socket_kirim - Kirim data
 */
tanda64_t socket_kirim(tak_bertanda32_t sockfd, const void *data,
                       ukuran_t len, tak_bertanda32_t flags)
{
    (void)flags;
    socket_t *sock;
    ukuran_t bytes_to_send;

    sock = socket_cari(sockfd);
    if (!socket_validasi(sock)) {
        return -1;
    }

    if (data == NULL || len == 0) {
        return -1;
    }

    /* Cek state untuk TCP */
    if (sock->tipe == SOCK_STREAM &&
        sock->tcp_state != TCP_STATE_ESTABLISHED) {
        return -1;
    }

    /* Cek buffer space */
    bytes_to_send = len;
    if (bytes_to_send > sock->tx_buffer_size - sock->tx_data_len) {
        bytes_to_send = sock->tx_buffer_size - sock->tx_data_len;
    }

    if (bytes_to_send == 0) {
        return 0;
    }

    /* Copy ke TX buffer */
    kernel_memcpy(sock->tx_buffer + sock->tx_data_len, data, bytes_to_send);
    sock->tx_data_len += bytes_to_send;

    /* Untuk stream socket, update sequence */
    if (sock->tipe == SOCK_STREAM) {
        sock->seq_num += bytes_to_send;
    }

    return (tanda64_t)bytes_to_send;
}

/*
 * socket_terima_data - Terima data
 */
tanda64_t socket_terima_data(tak_bertanda32_t sockfd, void *data,
                             ukuran_t len, tak_bertanda32_t flags)
{
    (void)flags;
    socket_t *sock;
    ukuran_t bytes_to_read;

    sock = socket_cari(sockfd);
    if (!socket_validasi(sock)) {
        return -1;
    }

    if (data == NULL || len == 0) {
        return -1;
    }

    /* Cek state untuk TCP */
    if (sock->tipe == SOCK_STREAM &&
        sock->tcp_state != TCP_STATE_ESTABLISHED) {
        return -1;
    }

    /* Cek data available */
    if (sock->rx_data_len == 0) {
        return 0;
    }

    /* Baca dari RX buffer */
    bytes_to_read = len;
    if (bytes_to_read > sock->rx_data_len) {
        bytes_to_read = sock->rx_data_len;
    }

    kernel_memcpy(data, sock->rx_buffer, bytes_to_read);

    /* Shift buffer */
    if (bytes_to_read < sock->rx_data_len) {
        ukuran_t remaining = sock->rx_data_len - bytes_to_read;
        kernel_memmove(sock->rx_buffer, sock->rx_buffer + bytes_to_read,
                       remaining);
    }

    sock->rx_data_len -= bytes_to_read;

    return (tanda64_t)bytes_to_read;
}

/*
 * socket_kirim_ke - Kirim data ke alamat tertentu
 */
tanda64_t socket_kirim_ke(tak_bertanda32_t sockfd, const void *data,
                          ukuran_t len, tak_bertanda32_t flags,
                          const sockaddr_t *dest_addr, ukuran_t addrlen)
{
    (void)dest_addr;
    (void)addrlen;
    socket_t *sock;

    sock = socket_cari(sockfd);
    if (!socket_validasi(sock)) {
        return -1;
    }

    /* Hanya untuk datagram socket */
    if (sock->tipe != SOCK_DGRAM) {
        return -1;
    }

    /* Simpan alamat tujuan sementara */
    /* Implementasi lengkap memerlukan UDP layer */

    return socket_kirim(sockfd, data, len, flags);
}

/*
 * socket_terima_dari - Terima data dari alamat
 */
tanda64_t socket_terima_dari(tak_bertanda32_t sockfd, void *data,
                             ukuran_t len, tak_bertanda32_t flags,
                             sockaddr_t *src_addr, ukuran_t *addrlen)
{
    socket_t *sock;
    tanda64_t result;

    sock = socket_cari(sockfd);
    if (!socket_validasi(sock)) {
        return -1;
    }

    /* Hanya untuk datagram socket */
    if (sock->tipe != SOCK_DGRAM) {
        return -1;
    }

    result = socket_terima_data(sockfd, data, len, flags);

    /* Copy source address */
    if (src_addr != NULL && addrlen != NULL) {
        ukuran_t copy_len = *addrlen;
        if (copy_len > sizeof(sockaddr_t)) {
            copy_len = sizeof(sockaddr_t);
        }
        kernel_memcpy(src_addr, &sock->alamat_remote, copy_len);
        *addrlen = copy_len;
    }

    return result;
}

/*
 * socket_set_opt - Set socket option
 */
status_t socket_set_opt(tak_bertanda32_t sockfd, tak_bertanda32_t level,
                        tak_bertanda32_t optname, const void *optval,
                        ukuran_t optlen)
{
    socket_t *sock;

    sock = socket_cari(sockfd);
    if (!socket_validasi(sock)) {
        return STATUS_PARAM_INVALID;
    }

    if (optval == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Handle berdasarkan level */
    switch (level) {
    case SOL_SOCKET:
        switch (optname) {
        case SO_RCVBUF:
            if (optlen >= sizeof(tak_bertanda32_t)) {
                tak_bertanda32_t size = *(tak_bertanda32_t *)optval;
                sock->rx_buffer_size = size;
            }
            break;

        case SO_SNDBUF:
            if (optlen >= sizeof(tak_bertanda32_t)) {
                tak_bertanda32_t size = *(tak_bertanda32_t *)optval;
                sock->tx_buffer_size = size;
            }
            break;

        case SO_RCVTIMEO:
            if (optlen >= sizeof(tak_bertanda32_t)) {
                sock->recv_timeout = *(tak_bertanda32_t *)optval;
            }
            break;

        case SO_SNDTIMEO:
            if (optlen >= sizeof(tak_bertanda32_t)) {
                sock->send_timeout = *(tak_bertanda32_t *)optval;
            }
            break;

        case SO_REUSEADDR:
            if (*(tak_bertanda32_t *)optval) {
                sock->flags |= SO_REUSEADDR;
            } else {
                sock->flags &= ~SO_REUSEADDR;
            }
            break;

        case SO_KEEPALIVE:
            if (*(tak_bertanda32_t *)optval) {
                sock->flags |= SO_KEEPALIVE;
            } else {
                sock->flags &= ~SO_KEEPALIVE;
            }
            break;

        default:
            return STATUS_TIDAK_DUKUNG;
        }
        break;

    case SOL_TCP:
        /* TCP specific options */
        break;

    case SOL_UDP:
        /* UDP specific options */
        break;

    default:
        return STATUS_TIDAK_DUKUNG;
    }

    return STATUS_BERHASIL;
}

/*
 * socket_get_opt - Get socket option
 */
status_t socket_get_opt(tak_bertanda32_t sockfd, tak_bertanda32_t level,
                        tak_bertanda32_t optname, void *optval,
                        ukuran_t *optlen)
{
    socket_t *sock;

    sock = socket_cari(sockfd);
    if (!socket_validasi(sock)) {
        return STATUS_PARAM_INVALID;
    }

    if (optval == NULL || optlen == NULL) {
        return STATUS_PARAM_NULL;
    }

    switch (level) {
    case SOL_SOCKET:
        switch (optname) {
        case SO_RCVBUF:
            *(tak_bertanda32_t *)optval = sock->rx_buffer_size;
            *optlen = sizeof(tak_bertanda32_t);
            break;

        case SO_SNDBUF:
            *(tak_bertanda32_t *)optval = sock->tx_buffer_size;
            *optlen = sizeof(tak_bertanda32_t);
            break;

        case SO_RCVTIMEO:
            *(tak_bertanda32_t *)optval = sock->recv_timeout;
            *optlen = sizeof(tak_bertanda32_t);
            break;

        case SO_SNDTIMEO:
            *(tak_bertanda32_t *)optval = sock->send_timeout;
            *optlen = sizeof(tak_bertanda32_t);
            break;

        default:
            return STATUS_TIDAK_DUKUNG;
        }
        break;

    default:
        return STATUS_TIDAK_DUKUNG;
    }

    return STATUS_BERHASIL;
}

/*
 * socket_get_state - Dapatkan state socket
 */
tak_bertanda32_t socket_get_state(tak_bertanda32_t sockfd)
{
    socket_t *sock;

    sock = socket_cari(sockfd);
    if (!socket_validasi(sock)) {
        return 0;
    }

    return sock->tcp_state;
}

/*
 * socket_is_connected - Cek apakah socket terkoneksi
 */
bool_t socket_is_connected(tak_bertanda32_t sockfd)
{
    socket_t *sock;

    sock = socket_cari(sockfd);
    if (!socket_validasi(sock)) {
        return SALAH;
    }

    if (sock->tipe == SOCK_STREAM) {
        return (sock->tcp_state == TCP_STATE_ESTABLISHED) ? BENAR : SALAH;
    }

    return BENAR;
}

/*
 * socket_flush - Flush TX buffer
 */
status_t socket_flush(tak_bertanda32_t sockfd)
{
    socket_t *sock;

    sock = socket_cari(sockfd);
    if (!socket_validasi(sock)) {
        return STATUS_PARAM_INVALID;
    }

    /* Implementasi sederhana - reset buffer */
    sock->tx_data_len = 0;

    return STATUS_BERHASIL;
}

/*
 * socket_count - Hitung jumlah socket
 */
tak_bertanda32_t socket_count(void)
{
    return g_jaringan_konteks.socket_count;
}

/*
 * socket_get_info - Dapatkan info socket
 */
status_t socket_get_info(tak_bertanda32_t sockfd, socket_t *info)
{
    socket_t *sock;

    sock = socket_cari(sockfd);
    if (!socket_validasi(sock)) {
        return STATUS_PARAM_INVALID;
    }

    if (info == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memcpy(info, sock, sizeof(socket_t));

    return STATUS_BERHASIL;
}
