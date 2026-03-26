/*
 * PIGURA OS - JARINGAN.H
 * =======================
 * Header untuk subsistem jaringan Pigura OS.
 *
 * Berkas ini mendefinisikan interface untuk stack jaringan
 * termasuk TCP/IP, WiFi, Ethernet, dan protokol terkait.
 *
 * Kepatuhan standar:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Mendukung semua arsitektur
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#ifndef PERANGKAT_JARINGAN_H
#define PERANGKAT_JARINGAN_H

/*
 * ===========================================================================
 * INCLUDE
 * ===========================================================================
 */

#include "../../inti/types.h"

/*
 * ===========================================================================
 * KONSTANTA JARINGAN
 * ===========================================================================
 */

/* Versi modul */
#define JARINGAN_VERSI_MAJOR 1
#define JARINGAN_VERSI_MINOR 0
#define JARINGAN_VERSI_PATCH 0

/* Magic numbers */
#define JARINGAN_MAGIC            0x4A524E47  /* "JRNG" */
#define NETDEV_MAGIC              0x4E455444  /* "NETD" */
#define SOCKET_MAGIC              0x534F434B  /* "SOCK" */
#define PAKET_MAGIC               0x50414B54  /* "PAKT" */

/* Batas sistem */
#define NETDEV_NAMA_PANJANG_MAKS  32
#define NETDEV_ALAMAT_MAC_LEN     6
#define NETDEV_IPV4_LEN           4
#define NETDEV_IPV6_LEN           16
#define NETDEV_MAKS               16
#define SOCKET_MAKS               256
#define PAKET_BUFFER_MAKS         65536
#define PAKET_MAKS                1024
#define PORT_MAKS                 65536
#define DNS_NAMA_MAKS             256
#define DNS_IP_MAKS               16

/* MTU */
#define MTU_ETHERNET              1500
#define MTU_JUMBO                 9000
#define MTU_LOOPBACK              65536

/* Tipe interface */
#define NETDEV_TIPE_TIDAK_DIKETAHUI  0
#define NETDEV_TIPE_LOOPBACK         1
#define NETDEV_TIPE_ETHERNET         2
#define NETDEV_TIPE_WIFI             3
#define NETDEV_TIPE_PPP              4
#define NETDEV_TIPE_TUN              5
#define NETDEV_TIPE_TAP              6
#define NETDEV_TIPE_BRIDGE           7
#define NETDEV_TIPE_VLAN             8
#define NETDEV_TIPE_BONDING          9

/* Status interface */
#define NETDEV_STATUS_TIDAK_ADA      0
#define NETDEV_STATUS_DOWN           1
#define NETDEV_STATUS_UP             2
#define NETDEV_STATUS_RUNNING        3
#define NETDEV_STATUS_ERROR          4

/* Flag interface */
#define NETDEV_FLAG_UP               0x0001
#define NETDEV_FLAG_RUNNING          0x0002
#define NETDEV_FLAG_PROMISC          0x0004
#define NETDEV_FLAG_MULTICAST        0x0008
#define NETDEV_FLAG_BROADCAST        0x0010
#define NETDEV_FLAG_LOOPBACK         0x0020
#define NETDEV_FLAG_POINTOPOINT      0x0040
#define NETDEV_FLAG_ARP              0x0080
#define NETDEV_FLAG_DYNAMIC          0x0100
#define NETDEV_FLAG_DHCP             0x0200
#define NETDEV_FLAG_WIFI             0x0400

/* Protocol */
#define PROTO_IP                     0
#define PROTO_ICMP                   1
#define PROTO_TCP                    6
#define PROTO_UDP                    17
#define PROTO_IPV6                   41
#define PROTO_ICMPV6                 58

/* Port standar */
#define PORT_FTP_DATA                20
#define PORT_FTP                     21
#define PORT_SSH                     22
#define PORT_TELNET                  23
#define PORT_SMTP                    25
#define PORT_DNS                     53
#define PORT_DHCP_SERVER             67
#define PORT_DHCP_CLIENT             68
#define PORT_TFTP                    69
#define PORT_HTTP                    80
#define PORT_POP3                    110
#define PORT_NTP                     123
#define PORT_IMAP                    143
#define PORT_SNMP                    161
#define PORT_HTTPS                   443
#define PORT_SMB                     445

/* Socket tipe */
#define SOCK_STREAM                  1
#define SOCK_DGRAM                   2
#define SOCK_RAW                     3
#define SOCK_SEQPACKET               4

/* Address family */
#define AF_UNSPEC                    0
#define AF_UNIX                      1
#define AF_INET                      2
#define AF_INET6                     10
#define AF_PACKET                    17

/* Socket option level */
#define SOL_SOCKET                   1
#define SOL_IP                       0
#define SOL_TCP                      6
#define SOL_UDP                      17

/* Socket options */
#define SO_DEBUG                     1
#define SO_REUSEADDR                 2
#define SO_KEEPALIVE                 3
#define SO_DONTROUTE                 4
#define SO_BROADCAST                 6
#define SO_SNDBUF                    7
#define SO_RCVBUF                    8
#define SO_RCVTIMEO                  20
#define SO_SNDTIMEO                  21

/* TCP flags */
#define TCP_FLAG_FIN                 0x01
#define TCP_FLAG_SYN                 0x02
#define TCP_FLAG_RST                 0x04
#define TCP_FLAG_PSH                 0x08
#define TCP_FLAG_ACK                 0x10
#define TCP_FLAG_URG                 0x20

/* TCP states */
#define TCP_STATE_CLOSED             0
#define TCP_STATE_LISTEN             1
#define TCP_STATE_SYN_SENT           2
#define TCP_STATE_SYN_RECEIVED       3
#define TCP_STATE_ESTABLISHED        4
#define TCP_STATE_FIN_WAIT_1         5
#define TCP_STATE_FIN_WAIT_2         6
#define TCP_STATE_CLOSE_WAIT         7
#define TCP_STATE_CLOSING            8
#define TCP_STATE_LAST_ACK           9
#define TCP_STATE_TIME_WAIT          10

/* WiFi tipe */
#define WIFI_TIPE_TIDAK_DIKETAHUI    0
#define WIFI_TIPE_ADHOC              1
#define WIFI_TIPE_INFRASTRUKTUR      2
#define WIFI_TIPE_AP                 3
#define WIFI_TIPE_MONITOR            4
#define WIFI_TIPE_MESH               5

/* WiFi keamanan */
#define WIFI_KEAMANAN_TIDAK_ADA      0
#define WIFI_KEAMANAN_WEP            1
#define WIFI_KEAMANAN_WPA            2
#define WIFI_KEAMANAN_WPA2           3
#define WIFI_KEAMANAN_WPA3           4

/* WiFi status */
#define WIFI_STATUS_DISCONNECTED     0
#define WIFI_STATUS_SCANNING         1
#define WIFI_STATUS_AUTHENTICATING   2
#define WIFI_STATUS_ASSOCIATING      3
#define WIFI_STATUS_CONNECTED        4

/* ARP opcodes */
#define ARP_OP_REQUEST               1
#define ARP_OP_REPLY                 2

/* DHCP message types */
#define DHCP_DISCOVER                1
#define DHCP_OFFER                   2
#define DHCP_REQUEST                 3
#define DHCP_DECLINE                 4
#define DHCP_ACK                     5
#define DHCP_NAK                     6
#define DHCP_RELEASE                 7
#define DHCP_INFORM                  8

/* Error codes */
#define NET_ERR_TIDAK_ADA            0
#define NET_ERR_SUCCESS              1
#define NET_ERR_UNKNOWN              -1
#define NET_ERR_IFACE_DOWN           -2
#define NET_ERR_NO_ROUTE             -3
#define NET_ERR_CONN_REFUSED         -4
#define NET_ERR_CONN_RESET           -5
#define NET_ERR_TIMEOUT              -6
#define NET_ERR_NO_MEMORY            -7
#define NET_ERR_INVALID_PARAM        -8
#define NET_ERR_SOCKET_CLOSED        -9

/*
 * ===========================================================================
 * STRUKTUR ALAMAT
 * ===========================================================================
 */

/* Alamat MAC */
typedef struct {
    tak_bertanda8_t byte[NETDEV_ALAMAT_MAC_LEN];
} alamat_mac_t;

/* Alamat IPv4 */
typedef struct {
    tak_bertanda8_t byte[NETDEV_IPV4_LEN];
} alamat_ipv4_t;

/* Alamat IPv6 */
typedef struct {
    tak_bertanda8_t byte[NETDEV_IPV6_LEN];
} alamat_ipv6_t;

/* Alamat socket IPv4 */
typedef struct {
    tak_bertanda16_t family;        /* AF_INET */
    tak_bertanda16_t port;          /* Port dalam network byte order */
    alamat_ipv4_t addr;             /* Alamat IPv4 */
    tak_bertanda8_t zero[8];        /* Padding */
} sockaddr_in_t;

/* Alamat socket IPv6 */
typedef struct {
    tak_bertanda16_t family;        /* AF_INET6 */
    tak_bertanda16_t port;          /* Port dalam network byte order */
    tak_bertanda32_t flowinfo;      /* Flow info */
    alamat_ipv6_t addr;             /* Alamat IPv6 */
    tak_bertanda32_t scope_id;      /* Scope ID */
} sockaddr_in6_t;

/* Generic socket address */
typedef struct {
    tak_bertanda16_t family;
    char data[14];
} sockaddr_t;

/*
 * ===========================================================================
 * STRUKTUR PAKET
 * ===========================================================================
 */

/* Header Ethernet */
typedef struct {
    alamat_mac_t dest;              /* Alamat tujuan */
    alamat_mac_t src;               /* Alamat sumber */
    tak_bertanda16_t tipe;          /* Tipe protocol */
} __attribute__((packed)) eth_header_t;

/* Header IP */
typedef struct {
    tak_bertanda8_t version_ihl;    /* Version (4) + IHL */
    tak_bertanda8_t tos;            /* Type of Service */
    tak_bertanda16_t total_length;  /* Total length */
    tak_bertanda16_t id;            /* Identification */
    tak_bertanda16_t flags_offset;  /* Flags + Fragment offset */
    tak_bertanda8_t ttl;            /* Time to Live */
    tak_bertanda8_t protocol;       /* Protocol */
    tak_bertanda16_t checksum;      /* Header checksum */
    alamat_ipv4_t src;              /* Source address */
    alamat_ipv4_t dest;             /* Destination address */
} __attribute__((packed)) ip_header_t;

/* Header TCP */
typedef struct {
    tak_bertanda16_t src_port;      /* Source port */
    tak_bertanda16_t dest_port;     /* Destination port */
    tak_bertanda32_t seq_num;       /* Sequence number */
    tak_bertanda32_t ack_num;       /* Acknowledgement number */
    tak_bertanda8_t data_offset;    /* Data offset */
    tak_bertanda8_t flags;          /* TCP flags */
    tak_bertanda16_t window;        /* Window size */
    tak_bertanda16_t checksum;      /* Checksum */
    tak_bertanda16_t urgent_ptr;    /* Urgent pointer */
} __attribute__((packed)) tcp_header_t;

/* Header UDP */
typedef struct {
    tak_bertanda16_t src_port;      /* Source port */
    tak_bertanda16_t dest_port;     /* Destination port */
    tak_bertanda16_t length;        /* Length */
    tak_bertanda16_t checksum;      /* Checksum */
} __attribute__((packed)) udp_header_t;

/* Header ARP */
typedef struct {
    tak_bertanda16_t hardware_type; /* Hardware type */
    tak_bertanda16_t protocol_type; /* Protocol type */
    tak_bertanda8_t hardware_len;   /* Hardware address length */
    tak_bertanda8_t protocol_len;   /* Protocol address length */
    tak_bertanda16_t opcode;        /* Operation code */
    alamat_mac_t sender_mac;        /* Sender MAC */
    alamat_ipv4_t sender_ip;        /* Sender IP */
    alamat_mac_t target_mac;        /* Target MAC */
    alamat_ipv4_t target_ip;        /* Target IP */
} __attribute__((packed)) arp_header_t;

/* Header ICMP */
typedef struct {
    tak_bertanda8_t tipe;           /* Type */
    tak_bertanda8_t code;           /* Code */
    tak_bertanda16_t checksum;      /* Checksum */
    tak_bertanda16_t id;            /* Identifier */
    tak_bertanda16_t sequence;      /* Sequence number */
} __attribute__((packed)) icmp_header_t;

/* Paket jaringan */
typedef struct {
    tak_bertanda32_t magic;         /* Magic number */
    tak_bertanda32_t id;            /* ID paket */
    
    /* Buffer */
    tak_bertanda8_t *data;          /* Pointer ke data */
    ukuran_t panjang;               /* Panjang total */
    ukuran_t kapasitas;             /* Kapasitas buffer */
    
    /* Header pointers */
    eth_header_t *eth;              /* Header Ethernet */
    ip_header_t *ip;                /* Header IP */
    tcp_header_t *tcp;              /* Header TCP */
    udp_header_t *udp;              /* Header UDP */
    tak_bertanda8_t *payload;       /* Payload */
    ukuran_t payload_len;           /* Panjang payload */
    
    /* Metadata */
    tak_bertanda32_t iface_id;      /* Interface ID */
    tak_bertanda64_t timestamp;     /* Timestamp */
    tak_bertanda32_t flags;         /* Flags */
    
} paket_t;

/*
 * ===========================================================================
 * STRUKTUR NETWORK DEVICE
 * ===========================================================================
 */

typedef struct netdev netdev_t;

/* Callback functions */
typedef status_t (*netdev_send_fn)(netdev_t *dev, const void *data, 
                                    ukuran_t len);
typedef status_t (*netdev_recv_fn)(netdev_t *dev, void *data, 
                                    ukuran_t *len);
typedef status_t (*netdev_ioctl_fn)(netdev_t *dev, tak_bertanda32_t cmd,
                                     void *arg);
typedef status_t (*netdev_open_fn)(netdev_t *dev);
typedef void (*netdev_close_fn)(netdev_t *dev);

struct netdev {
    tak_bertanda32_t magic;         /* Magic number */
    tak_bertanda32_t id;            /* Device ID */
    char nama[NETDEV_NAMA_PANJANG_MAKS];
    
    /* Tipe dan status */
    tak_bertanda32_t tipe;          /* Tipe interface */
    tak_bertanda32_t status;        /* Status */
    tak_bertanda32_t flags;         /* Flags */
    
    /* Alamat */
    alamat_mac_t mac;               /* Alamat MAC */
    alamat_ipv4_t ipv4;             /* Alamat IPv4 */
    alamat_ipv4_t ipv4_netmask;     /* Netmask */
    alamat_ipv4_t ipv4_broadcast;   /* Broadcast */
    alamat_ipv4_t ipv4_gateway;     /* Gateway default */
    alamat_ipv6_t ipv6;             /* Alamat IPv6 */
    
    /* Statistik */
    tak_bertanda64_t rx_packets;    /* Paket diterima */
    tak_bertanda64_t tx_packets;    /* Paket dikirim */
    tak_bertanda64_t rx_bytes;      /* Bytes diterima */
    tak_bertanda64_t tx_bytes;      /* Bytes dikirim */
    tak_bertanda64_t rx_errors;     /* Error RX */
    tak_bertanda64_t tx_errors;     /* Error TX */
    tak_bertanda64_t rx_dropped;    /* Dropped RX */
    tak_bertanda64_t tx_dropped;    /* Dropped TX */
    
    /* MTU dan kapasitas */
    ukuran_t mtu;                   /* Maximum Transmission Unit */
    ukuran_t tx_queue_len;          /* TX queue length */
    
    /* Callbacks */
    netdev_send_fn kirim;
    netdev_recv_fn terima;
    netdev_ioctl_fn ioctl;
    netdev_open_fn buka;
    netdev_close_fn tutup;
    
    /* Private data */
    void *priv;
    
    /* Next device */
    netdev_t *berikutnya;
};

/*
 * ===========================================================================
 * STRUKTUR SOCKET
 * ===========================================================================
 */

typedef struct {
    tak_bertanda32_t magic;         /* Magic number */
    tak_bertanda32_t id;            /* Socket ID */
    
    /* Tipe dan status */
    tak_bertanda32_t domain;        /* Address family */
    tak_bertanda32_t tipe;          /* Socket type */
    tak_bertanda32_t protocol;      /* Protocol */
    tak_bertanda32_t state;         /* State (TCP) */
    
    /* Alamat */
    sockaddr_t alamat_lokal;        /* Alamat lokal */
    sockaddr_t alamat_remote;       /* Alamat remote */
    
    /* Buffer */
    tak_bertanda8_t *rx_buffer;     /* RX buffer */
    ukuran_t rx_buffer_size;        /* RX buffer size */
    ukuran_t rx_data_len;           /* Data in RX buffer */
    tak_bertanda8_t *tx_buffer;     /* TX buffer */
    ukuran_t tx_buffer_size;        /* TX buffer size */
    ukuran_t tx_data_len;           /* Data in TX buffer */
    
    /* Options */
    tak_bertanda32_t flags;         /* Flags */
    tak_bertanda32_t recv_timeout;  /* Receive timeout (ms) */
    tak_bertanda32_t send_timeout;  /* Send timeout (ms) */
    
    /* TCP specific */
    tak_bertanda32_t tcp_state;     /* TCP state */
    tak_bertanda32_t seq_num;       /* Sequence number */
    tak_bertanda32_t ack_num;       /* Ack number */
    tak_bertanda16_t window;        /* Window size */
    
    /* Associated interface */
    netdev_t *netdev;
    
    /* Next socket */
    struct socket *berikutnya;
    
} socket_t;

/*
 * ===========================================================================
 * STRUKTUR WIFI
 * ===========================================================================
 */

/* Scan result */
typedef struct {
    char ssid[32];                  /* SSID */
    alamat_mac_t bssid;             /* BSSID */
    tak_bertanda16_t channel;       /* Channel */
    tak_bertanda16_t frekuensi;     /* Frekuensi (MHz) */
    tak_bertanda32_t keamanan;      /* Tipe keamanan */
    tanda32_t rssi;                 /* Signal strength (dBm) */
    tak_bertanda8_t tipe;           /* Network type */
    bool_t terenkripsi;             /* Encrypted? */
} wifi_scan_result_t;

/* WiFi configuration */
typedef struct {
    char ssid[32];                  /* SSID */
    char password[64];              /* Password */
    tak_bertanda32_t keamanan;      /* Security type */
    tak_bertanda32_t tipe;          /* Network type */
    bool_t auto_connect;            /* Auto-connect */
    bool_t hidden;                  /* Hidden SSID */
} wifi_config_t;

/* WiFi status */
typedef struct {
    tak_bertanda32_t status;        /* Connection status */
    char ssid[32];                  /* Current SSID */
    alamat_mac_t bssid;             /* Current BSSID */
    tak_bertanda16_t channel;       /* Current channel */
    tanda32_t rssi;                 /* Signal strength */
    tak_bertanda32_t kecepatan;     /* Link speed (Mbps) */
    alamat_ipv4_t ipv4;             /* IP address */
    tak_bertanda32_t tx_packets;    /* TX packets */
    tak_bertanda32_t rx_packets;    /* RX packets */
} wifi_status_t;

/*
 * ===========================================================================
 * STRUKTUR DNS
 * ===========================================================================
 */

/* DNS entry */
typedef struct {
    char nama[DNS_NAMA_MAKS];       /* Hostname */
    alamat_ipv4_t ipv4;             /* IPv4 address */
    alamat_ipv6_t ipv6;             /* IPv6 address */
    tak_bertanda32_t ttl;           /* Time to live */
    tak_bertanda64_t expiry;        /* Expiry timestamp */
} dns_entry_t;

/*
 * ===========================================================================
 * STRUKTUR DHCP
 * ===========================================================================
 */

/* DHCP config */
typedef struct {
    alamat_ipv4_t alamat;           /* Assigned IP */
    alamat_ipv4_t netmask;          /* Netmask */
    alamat_ipv4_t gateway;          /* Gateway */
    alamat_ipv4_t dns1;             /* DNS server 1 */
    alamat_ipv4_t dns2;             /* DNS server 2 */
    tak_bertanda32_t lease_time;    /* Lease time */
    tak_bertanda64_t lease_expiry;  /* Lease expiry */
    alamat_ipv4_t server;           /* DHCP server */
} dhcp_config_t;

/*
 * ===========================================================================
 * STRUKTUR KONTEKS JARINGAN
 * ===========================================================================
 */

typedef struct {
    tak_bertanda32_t magic;         /* Magic number */
    
    /* Network devices */
    netdev_t *netdev_list;          /* Device list */
    tak_bertanda32_t netdev_count;  /* Device count */
    netdev_t *netdev_default;       /* Default interface */
    
    /* Sockets */
    socket_t *socket_list;          /* Socket list */
    tak_bertanda32_t socket_count;  /* Socket count */
    
    /* DNS cache */
    dns_entry_t *dns_cache;         /* DNS cache */
    tak_bertanda32_t dns_cache_size;
    
    /* Statistics */
    tak_bertanda64_t total_rx;
    tak_bertanda64_t total_tx;
    
    /* Status */
    bool_t diinisialisasi;
    
} jaringan_konteks_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

extern jaringan_konteks_t g_jaringan_konteks;
extern bool_t g_jaringan_diinisialisasi;

/*
 * ===========================================================================
 * FUNGSI INISIALISASI
 * ===========================================================================
 */

/*
 * jaringan_init - Inisialisasi subsistem jaringan
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t jaringan_init(void);

/*
 * jaringan_shutdown - Matikan subsistem jaringan
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t jaringan_shutdown(void);

/*
 * jaringan_konteks_dapatkan - Dapatkan konteks jaringan
 *
 * Return: Pointer ke konteks atau NULL
 */
jaringan_konteks_t *jaringan_konteks_dapatkan(void);

/*
 * ===========================================================================
 * FUNGSI NETWORK DEVICE
 * ===========================================================================
 */

/*
 * netdev_register - Registrasi network device
 *
 * Parameter:
 *   nama - Nama device
 *   tipe - Tipe device
 *
 * Return: Pointer ke device atau NULL
 */
netdev_t *netdev_register(const char *nama, tak_bertanda32_t tipe);

/*
 * netdev_unregister - Unregister network device
 *
 * Parameter:
 *   dev - Pointer ke device
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t netdev_unregister(netdev_t *dev);

/*
 * netdev_cari - Cari device berdasarkan nama
 *
 * Parameter:
 *   nama - Nama device
 *
 * Return: Pointer ke device atau NULL
 */
netdev_t *netdev_cari(const char *nama);

/*
 * netdev_cari_id - Cari device berdasarkan ID
 *
 * Parameter:
 *   id - ID device
 *
 * Return: Pointer ke device atau NULL
 */
netdev_t *netdev_cari_id(tak_bertanda32_t id);

/*
 * netdev_up - Aktifkan network device
 *
 * Parameter:
 *   dev - Pointer ke device
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t netdev_up(netdev_t *dev);

/*
 * netdev_down - Nonaktifkan network device
 *
 * Parameter:
 *   dev - Pointer ke device
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t netdev_down(netdev_t *dev);

/*
 * netdev_set_ipv4 - Set alamat IPv4
 *
 * Parameter:
 *   dev - Pointer ke device
 *   addr - Alamat IPv4
 *   netmask - Netmask
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t netdev_set_ipv4(netdev_t *dev, alamat_ipv4_t *addr,
                          alamat_ipv4_t *netmask);

/*
 * netdev_set_gateway - Set gateway default
 *
 * Parameter:
 *   dev - Pointer ke device
 *   gateway - Alamat gateway
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t netdev_set_gateway(netdev_t *dev, alamat_ipv4_t *gateway);

/*
 * netdev_kirim - Kirim paket
 *
 * Parameter:
 *   dev - Pointer ke device
 *   data - Data yang akan dikirim
 *   len - Panjang data
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t netdev_kirim(netdev_t *dev, const void *data, ukuran_t len);

/*
 * netdev_terima - Terima paket
 *
 * Parameter:
 *   dev - Pointer ke device
 *   data - Buffer untuk data
 *   len - Pointer ke panjang
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t netdev_terima(netdev_t *dev, void *data, ukuran_t *len);

/*
 * ===========================================================================
 * FUNGSI SOCKET
 * ===========================================================================
 */

/*
 * socket_buat - Buat socket baru
 *
 * Parameter:
 *   domain - Address family
 *   tipe - Socket type
 *   protocol - Protocol
 *
 * Return: Socket ID atau nilai negatif jika error
 */
tanda32_t socket_buat(tak_bertanda32_t domain, tak_bertanda32_t tipe,
                       tak_bertanda32_t protocol);

/*
 * socket_tutup - Tutup socket
 *
 * Parameter:
 *   sockfd - Socket file descriptor
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t socket_tutup(tak_bertanda32_t sockfd);

/*
 * socket_ikat - Bind socket ke alamat
 *
 * Parameter:
 *   sockfd - Socket file descriptor
 *   addr - Alamat
 *   addrlen - Panjang alamat
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t socket_ikat(tak_bertanda32_t sockfd, const sockaddr_t *addr,
                      ukuran_t addrlen);

/*
 * socket_dengar - Listen untuk koneksi
 *
 * Parameter:
 *   sockfd - Socket file descriptor
 *   backlog - Maximum pending connections
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t socket_dengar(tak_bertanda32_t sockfd, tak_bertanda32_t backlog);

/*
 * socket_terima - Accept koneksi
 *
 * Parameter:
 *   sockfd - Socket file descriptor
 *   addr - Buffer untuk alamat client
 *   addrlen - Pointer ke panjang alamat
 *
 * Return: Socket ID baru atau nilai negatif
 */
tanda32_t socket_terima(tak_bertanda32_t sockfd, sockaddr_t *addr,
                         ukuran_t *addrlen);

/*
 * socket_hubung - Connect ke remote
 *
 * Parameter:
 *   sockfd - Socket file descriptor
 *   addr - Alamat remote
 *   addrlen - Panjang alamat
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t socket_hubung(tak_bertanda32_t sockfd, const sockaddr_t *addr,
                        ukuran_t addrlen);

/*
 * socket_kirim - Kirim data
 *
 * Parameter:
 *   sockfd - Socket file descriptor
 *   data - Data yang akan dikirim
 *   len - Panjang data
 *   flags - Flags
 *
 * Return: Jumlah bytes terkirim atau nilai negatif
 */
tanda64_t socket_kirim(tak_bertanda32_t sockfd, const void *data,
                        ukuran_t len, tak_bertanda32_t flags);

/*
 * socket_terima_data - Terima data
 *
 * Parameter:
 *   sockfd - Socket file descriptor
 *   data - Buffer untuk data
 *   len - Panjang buffer
 *   flags - Flags
 *
 * Return: Jumlah bytes diterima atau nilai negatif
 */
tanda64_t socket_terima_data(tak_bertanda32_t sockfd, void *data,
                              ukuran_t len, tak_bertanda32_t flags);

/*
 * socket_kirim_ke - Kirim data ke alamat tertentu
 *
 * Parameter:
 *   sockfd - Socket file descriptor
 *   data - Data
 *   len - Panjang data
 *   flags - Flags
 *   dest_addr - Alamat tujuan
 *   addrlen - Panjang alamat
 *
 * Return: Jumlah bytes terkirim
 */
tanda64_t socket_kirim_ke(tak_bertanda32_t sockfd, const void *data,
                           ukuran_t len, tak_bertanda32_t flags,
                           const sockaddr_t *dest_addr, ukuran_t addrlen);

/*
 * socket_terima_dari - Terima data dari alamat
 *
 * Parameter:
 *   sockfd - Socket file descriptor
 *   data - Buffer
 *   len - Panjang buffer
 *   flags - Flags
 *   src_addr - Buffer untuk alamat sumber
 *   addrlen - Pointer ke panjang alamat
 *
 * Return: Jumlah bytes diterima
 */
tanda64_t socket_terima_dari(tak_bertanda32_t sockfd, void *data,
                              ukuran_t len, tak_bertanda32_t flags,
                              sockaddr_t *src_addr, ukuran_t *addrlen);

/*
 * socket_set_opt - Set socket option
 *
 * Parameter:
 *   sockfd - Socket file descriptor
 *   level - Option level
 *   optname - Option name
 *   optval - Option value
 *   optlen - Option length
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t socket_set_opt(tak_bertanda32_t sockfd, tak_bertanda32_t level,
                         tak_bertanda32_t optname, const void *optval,
                         ukuran_t optlen);

/*
 * ===========================================================================
 * FUNGSI TCP
 * ===========================================================================
 */

/*
 * tcp_init - Inisialisasi TCP stack
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t tcp_init(void);

/*
 * tcp_proses - Proses paket TCP
 *
 * Parameter:
 *   paket - Pointer ke paket
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t tcp_proses(paket_t *paket);

/*
 * tcp_kirim_syn - Kirim SYN packet
 *
 * Parameter:
 *   sock - Socket
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t tcp_kirim_syn(socket_t *sock);

/*
 * tcp_kirim_ack - Kirim ACK packet
 *
 * Parameter:
 *   sock - Socket
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t tcp_kirim_ack(socket_t *sock);

/*
 * tcp_kirim_fin - Kirim FIN packet
 *
 * Parameter:
 *   sock - Socket
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t tcp_kirim_fin(socket_t *sock);

/*
 * tcp_kirim_rst - Kirim RST packet
 *
 * Parameter:
 *   sock - Socket
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t tcp_kirim_rst(socket_t *sock);

/*
 * ===========================================================================
 * FUNGSI UDP
 * ===========================================================================
 */

/*
 * udp_init - Inisialisasi UDP stack
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t udp_init(void);

/*
 * udp_proses - Proses paket UDP
 *
 * Parameter:
 *   paket - Pointer ke paket
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t udp_proses(paket_t *paket);

/*
 * ===========================================================================
 * FUNGSI ARP
 * ===========================================================================
 */

/*
 * arp_init - Inisialisasi ARP
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t arp_init(void);

/*
 * arp_cari - Cari MAC address untuk IP
 *
 * Parameter:
 *   dev - Network device
 *   ip - Alamat IP
 *   mac - Buffer untuk MAC address
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t arp_cari(netdev_t *dev, alamat_ipv4_t *ip, alamat_mac_t *mac);

/*
 * arp_tambah - Tambah entry ARP
 *
 * Parameter:
 *   dev - Network device
 *   ip - Alamat IP
 *   mac - MAC address
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t arp_tambah(netdev_t *dev, alamat_ipv4_t *ip, alamat_mac_t *mac);

/*
 * arp_hapus - Hapus entry ARP
 *
 * Parameter:
 *   dev - Network device
 *   ip - Alamat IP
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t arp_hapus(netdev_t *dev, alamat_ipv4_t *ip);

/*
 * ===========================================================================
 * FUNGSI DHCP
 * ===========================================================================
 */

/*
 * dhcp_init - Inisialisasi DHCP client
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dhcp_init(void);

/*
 * dhcp_request - Request DHCP configuration
 *
 * Parameter:
 *   dev - Network device
 *   config - Buffer untuk konfigurasi
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dhcp_request(netdev_t *dev, dhcp_config_t *config);

/*
 * dhcp_release - Release DHCP lease
 *
 * Parameter:
 *   dev - Network device
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dhcp_release(netdev_t *dev);

/*
 * dhcp_renew - Renew DHCP lease
 *
 * Parameter:
 *   dev - Network device
 *   config - Buffer untuk konfigurasi baru
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dhcp_renew(netdev_t *dev, dhcp_config_t *config);

/*
 * ===========================================================================
 * FUNGSI DNS
 * ===========================================================================
 */

/*
 * dns_init - Inisialisasi DNS resolver
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dns_init(void);

/*
 * dns_resolve - Resolve hostname ke IP
 *
 * Parameter:
 *   hostname - Nama host
 *   ipv4 - Buffer untuk IPv4
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dns_resolve(const char *hostname, alamat_ipv4_t *ipv4);

/*
 * dns_set_server - Set DNS server
 *
 * Parameter:
 *   index - Index server (0-1)
 *   server - Alamat DNS server
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t dns_set_server(tak_bertanda32_t index, alamat_ipv4_t *server);

/*
 * ===========================================================================
 * FUNGSI WIFI
 * ===========================================================================
 */

/*
 * wifi_init - Inisialisasi WiFi
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t wifi_init(void);

/*
 * wifi_scan - Scan jaringan WiFi
 *
 * Parameter:
 *   dev - Network device
 *   results - Buffer untuk hasil
 *   max_results - Maximum hasil
 *
 * Return: Jumlah hasil atau nilai negatif
 */
tanda32_t wifi_scan(netdev_t *dev, wifi_scan_result_t *results,
                     tak_bertanda32_t max_results);

/*
 * wifi_connect - Connect ke jaringan WiFi
 *
 * Parameter:
 *   dev - Network device
 *   config - Konfigurasi WiFi
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t wifi_connect(netdev_t *dev, wifi_config_t *config);

/*
 * wifi_disconnect - Disconnect dari WiFi
 *
 * Parameter:
 *   dev - Network device
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t wifi_disconnect(netdev_t *dev);

/*
 * wifi_status - Dapatkan status WiFi
 *
 * Parameter:
 *   dev - Network device
 *   status - Buffer untuk status
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t wifi_status(netdev_t *dev, wifi_status_t *status);

/*
 * ===========================================================================
 * FUNGSI UTILITAS
 * ===========================================================================
 */

/*
 * ipv4_ke_string - Konversi IPv4 ke string
 *
 * Parameter:
 *   addr - Alamat IPv4
 *   buffer - Buffer output
 *   len - Panjang buffer
 *
 * Return: Panjang string
 */
tak_bertanda32_t ipv4_ke_string(alamat_ipv4_t *addr, char *buffer,
                                  ukuran_t len);

/*
 * string_ke_ipv4 - Konversi string ke IPv4
 *
 * Parameter:
 *   str - String
 *   addr - Buffer untuk alamat
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t string_ke_ipv4(const char *str, alamat_ipv4_t *addr);

/*
 * mac_ke_string - Konversi MAC ke string
 *
 * Parameter:
 *   mac - Alamat MAC
 *   buffer - Buffer output
 *   len - Panjang buffer
 *
 * Return: Panjang string
 */
tak_bertanda32_t mac_ke_string(alamat_mac_t *mac, char *buffer,
                                 ukuran_t len);

/*
 * string_ke_mac - Konversi string ke MAC
 *
 * Parameter:
 *   str - String
 *   mac - Buffer untuk MAC
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t string_ke_mac(const char *str, alamat_mac_t *mac);

/*
 * hitung_checksum - Hitung checksum IP
 *
 * Parameter:
 *   data - Data
 *   len - Panjang data
 *
 * Return: Nilai checksum
 */
tak_bertanda16_t hitung_checksum(const void *data, ukuran_t len);

/*
 * jaringan_cetak_info - Cetak informasi jaringan
 */
void jaringan_cetak_info(void);

/*
 * ===========================================================================
 * FUNGSI LOOPBACK
 * ===========================================================================
 */

/*
 * loopback_init - Inisialisasi loopback device
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t loopback_init(void);

/*
 * ===========================================================================
 * FUNGSI ETHERNET
 * ===========================================================================
 */

/*
 * ethernet_init - Inisialisasi ethernet layer
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ethernet_init(void);

/*
 * ethernet_proses - Proses paket ethernet
 *
 * Parameter:
 *   paket - Pointer ke paket
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ethernet_proses(paket_t *paket);

/*
 * ethernet_kirim - Kirim paket ethernet
 *
 * Parameter:
 *   dev - Network device
 *   dest - Alamat tujuan
 *   tipe - Protocol type
 *   data - Payload
 *   len - Panjang payload
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ethernet_kirim(netdev_t *dev, alamat_mac_t *dest,
                         tak_bertanda16_t tipe, const void *data,
                         ukuran_t len);

#endif /* PERANGKAT_JARINGAN_H */
