/*
 * PIGURA OS - DHCP.C
 * ===================
 * Implementasi DHCP client.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi untuk menangani
 * protokol DHCP (Dynamic Host Configuration Protocol) dalam Pigura OS.
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

/* Forward declaration */
extern status_t udp_send(tak_bertanda16_t src_port, tak_bertanda32_t dest_ip,
                          tak_bertanda16_t dest_port, const void *data,
                          ukuran_t len);

/*
 * ===========================================================================
 * KONSTANTA DHCP
 * ===========================================================================
 */

/* DHCP ports */
#define DHCP_SERVER_PORT    67
#define DHCP_CLIENT_PORT    68

/* DHCP message types */
#define DHCP_MSG_DISCOVER   1
#define DHCP_MSG_OFFER      2
#define DHCP_MSG_REQUEST    3
#define DHCP_MSG_DECLINE    4
#define DHCP_MSG_ACK        5
#define DHCP_MSG_NAK        6
#define DHCP_MSG_RELEASE    7
#define DHCP_MSG_INFORM     8

/* DHCP options */
#define DHCP_OPT_PAD            0
#define DHCP_OPT_SUBNET_MASK    1
#define DHCP_OPT_ROUTER         3
#define DHCP_OPT_DNS_SERVER     6
#define DHCP_OPT_HOST_NAME      12
#define DHCP_OPT_DOMAIN_NAME    15
#define DHCP_OPT_REQUEST_IP     50
#define DHCP_OPT_LEASE_TIME     51
#define DHCP_OPT_MSG_TYPE       53
#define DHCP_OPT_SERVER_ID      54
#define DHCP_OPT_PARAM_REQ_LIST 55
#define DHCP_OPT_MAX_MSG_SIZE   57
#define DHCP_OPT_RENEWAL_TIME   58
#define DHCP_OPT_REBIND_TIME    59
#define DHCP_OPT_CLIENT_ID      61
#define DHCP_OPT_END            255

/* DHCP timeout (ms) */
#define DHCP_TIMEOUT_INIT       1000
#define DHCP_TIMEOUT_MAX        60000
#define DHCP_RETRIES            4

/* DHCP message size */
#define DHCP_MESSAGE_SIZE       548
#define DHCP_HEADER_SIZE        236
#define DHCP_MAGIC_COOKIE       0x63825363

/*
 * ===========================================================================
 * STRUKTUR DHCP
 * ===========================================================================
 */

/* DHCP message header */
typedef struct {
    tak_bertanda8_t op;         /* Message op code */
    tak_bertanda8_t htype;      /* Hardware address type */
    tak_bertanda8_t hlen;       /* Hardware address length */
    tak_bertanda8_t hops;       /* Relay hops */
    tak_bertanda32_t xid;       /* Transaction ID */
    tak_bertanda16_t secs;      /* Seconds since start */
    tak_bertanda16_t flags;     /* Flags */
    tak_bertanda32_t ciaddr;    /* Client IP */
    tak_bertanda32_t yiaddr;    /* Your IP */
    tak_bertanda32_t siaddr;    /* Server IP */
    tak_bertanda32_t giaddr;    /* Gateway IP */
    tak_bertanda8_t chaddr[16]; /* Client hardware address */
    tak_bertanda8_t sname[64];  /* Server name */
    tak_bertanda8_t file[128];  /* Boot file name */
    tak_bertanda32_t cookie;    /* Magic cookie */
    tak_bertanda8_t options[];  /* Options */
} dhcp_message_t;

/* DHCP client state */
typedef struct {
    tak_bertanda32_t xid;
    tak_bertanda32_t state;
    tak_bertanda32_t retries;
    tak_bertanda32_t timeout;
    tak_bertanda64_t start_time;
    alamat_ipv4_t server_ip;
    dhcp_config_t config;
    bool_t active;
} dhcp_client_t;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

/* Client state per interface */
#define DHCP_CLIENT_MAKS    16
static dhcp_client_t g_dhcp_clients[DHCP_CLIENT_MAKS];

/* Transaction ID counter */
static tak_bertanda32_t g_dhcp_xid_counter = 1;

/* Statistics */
static tak_bertanda64_t g_dhcp_discoveries = 0;
static tak_bertanda64_t g_dhcp_offers = 0;
static tak_bertanda64_t g_dhcp_requests = 0;
static tak_bertanda64_t g_dhcp_acks = 0;
static tak_bertanda64_t g_dhcp_naks = 0;
static tak_bertanda64_t g_dhcp_releases = 0;
static tak_bertanda64_t g_dhcp_errors = 0;

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * dhcp_generate_xid - Generate transaction ID
 */
static tak_bertanda32_t dhcp_generate_xid(void)
{
    return g_dhcp_xid_counter++;
}

/*
 * dhcp_build_message - Bangun pesan DHCP
 */
static void dhcp_build_message(tak_bertanda8_t *buffer,
                               tak_bertanda8_t msg_type,
                               tak_bertanda32_t xid,
                               const alamat_mac_t *client_mac,
                               const alamat_ipv4_t *request_ip)
{
    dhcp_message_t *msg;
    tak_bertanda8_t *opt;

    if (buffer == NULL || client_mac == NULL) {
        return;
    }

    kernel_memset(buffer, 0, DHCP_MESSAGE_SIZE);

    msg = (dhcp_message_t *)buffer;

    /* Header */
    msg->op = 1;       /* Boot request */
    msg->htype = 1;    /* Ethernet */
    msg->hlen = 6;     /* MAC length */
    msg->hops = 0;
    msg->xid = xid;
    msg->secs = 0;
    msg->flags = 0;
    msg->ciaddr = 0;
    msg->yiaddr = 0;
    msg->siaddr = 0;
    msg->giaddr = 0;

    /* Copy MAC address */
    kernel_memcpy(msg->chaddr, client_mac->byte, 6);

    /* Magic cookie */
    msg->cookie = DHCP_MAGIC_COOKIE;

    /* Options */
    opt = msg->options;

    /* Message type */
    *opt++ = DHCP_OPT_MSG_TYPE;
    *opt++ = 1;
    *opt++ = msg_type;

    /* Client identifier */
    *opt++ = DHCP_OPT_CLIENT_ID;
    *opt++ = 7;
    *opt++ = 1;  /* Ethernet */
    kernel_memcpy(opt, client_mac->byte, 6);
    opt += 6;

    /* Request IP untuk REQUEST */
    if (request_ip != NULL && msg_type == DHCP_MSG_REQUEST) {
        *opt++ = DHCP_OPT_REQUEST_IP;
        *opt++ = 4;
        kernel_memcpy(opt, request_ip->byte, 4);
        opt += 4;
    }

    /* Parameter request list */
    *opt++ = DHCP_OPT_PARAM_REQ_LIST;
    *opt++ = 4;
    *opt++ = DHCP_OPT_SUBNET_MASK;
    *opt++ = DHCP_OPT_ROUTER;
    *opt++ = DHCP_OPT_DNS_SERVER;
    *opt++ = DHCP_OPT_DOMAIN_NAME;

    /* Max message size */
    *opt++ = DHCP_OPT_MAX_MSG_SIZE;
    *opt++ = 2;
    *opt++ = (DHCP_MESSAGE_SIZE >> 8) & 0xFF;
    *opt++ = DHCP_MESSAGE_SIZE & 0xFF;

    /* End */
    *opt++ = DHCP_OPT_END;
}

/*
 * dhcp_parse_options - Parse DHCP options
 */
static status_t dhcp_parse_options(const tak_bertanda8_t *options,
                                   ukuran_t len,
                                   dhcp_config_t *config)
{
    ukuran_t i;
    tak_bertanda8_t opt;
    tak_bertanda8_t opt_len;

    if (options == NULL || config == NULL) {
        return STATUS_PARAM_NULL;
    }

    i = 0;

    while (i < len) {
        opt = options[i];

        if (opt == DHCP_OPT_PAD) {
            i++;
            continue;
        }

        if (opt == DHCP_OPT_END) {
            break;
        }

        if (i + 1 >= len) {
            break;
        }

        opt_len = options[i + 1];

        if (i + 2 + opt_len > len) {
            break;
        }

        switch (opt) {
        case DHCP_OPT_SUBNET_MASK:
            if (opt_len >= 4) {
                kernel_memcpy(config->netmask.byte, &options[i + 2], 4);
            }
            break;

        case DHCP_OPT_ROUTER:
            if (opt_len >= 4) {
                kernel_memcpy(config->gateway.byte, &options[i + 2], 4);
            }
            break;

        case DHCP_OPT_DNS_SERVER:
            if (opt_len >= 4) {
                kernel_memcpy(config->dns1.byte, &options[i + 2], 4);
            }
            if (opt_len >= 8) {
                kernel_memcpy(config->dns2.byte, &options[i + 6], 4);
            }
            break;

        case DHCP_OPT_LEASE_TIME:
            if (opt_len >= 4) {
                tak_bertanda32_t lease;
                kernel_memcpy(&lease, &options[i + 2], 4);
                config->lease_time = lease;
            }
            break;

        case DHCP_OPT_SERVER_ID:
            if (opt_len >= 4) {
                kernel_memcpy(config->server.byte, &options[i + 2], 4);
            }
            break;
        }

        i += 2 + opt_len;
    }

    return STATUS_BERHASIL;
}

/*
 * dhcp_send_discover - Kirim DHCP DISCOVER
 */
static status_t dhcp_send_discover(netdev_t *dev, dhcp_client_t *client)
{
    tak_bertanda8_t buffer[DHCP_MESSAGE_SIZE];

    if (dev == NULL || client == NULL) {
        return STATUS_PARAM_NULL;
    }

    client->xid = dhcp_generate_xid();

    dhcp_build_message(buffer, DHCP_MSG_DISCOVER, client->xid,
                       &dev->mac, NULL);

    /* Send via UDP broadcast */
    udp_send(DHCP_CLIENT_PORT, 0xFFFFFFFF, DHCP_SERVER_PORT,
             buffer, DHCP_MESSAGE_SIZE);

    g_dhcp_discoveries++;

    return STATUS_BERHASIL;
}

/*
 * dhcp_send_request - Kirim DHCP REQUEST
 */
static status_t dhcp_send_request(netdev_t *dev, dhcp_client_t *client,
                                  const alamat_ipv4_t *request_ip)
{
    tak_bertanda8_t buffer[DHCP_MESSAGE_SIZE];

    if (dev == NULL || client == NULL) {
        return STATUS_PARAM_NULL;
    }

    dhcp_build_message(buffer, DHCP_MSG_REQUEST, client->xid,
                       &dev->mac, request_ip);

    /* Send via UDP broadcast */
    udp_send(DHCP_CLIENT_PORT, 0xFFFFFFFF, DHCP_SERVER_PORT,
             buffer, DHCP_MESSAGE_SIZE);

    g_dhcp_requests++;

    return STATUS_BERHASIL;
}

/*
 * dhcp_send_release - Kirim DHCP RELEASE
 */
static status_t dhcp_send_release(netdev_t *dev, dhcp_client_t *client)
{
    tak_bertanda8_t buffer[DHCP_MESSAGE_SIZE];

    if (dev == NULL || client == NULL) {
        return STATUS_PARAM_NULL;
    }

    dhcp_build_message(buffer, DHCP_MSG_RELEASE, client->xid,
                       &dev->mac, &client->config.alamat);

    udp_send(DHCP_CLIENT_PORT,
             ((client->config.server.byte[0] << 24) |
              (client->config.server.byte[1] << 16) |
              (client->config.server.byte[2] << 8) |
              client->config.server.byte[3]),
             DHCP_SERVER_PORT,
             buffer, DHCP_MESSAGE_SIZE);

    g_dhcp_releases++;

    return STATUS_BERHASIL;
}

/*
 * dhcp_find_client - Cari client berdasarkan interface
 */
static dhcp_client_t *dhcp_find_client(tak_bertanda32_t iface_id)
{
    (void)iface_id;
    tak_bertanda32_t i;

    for (i = 0; i < DHCP_CLIENT_MAKS; i++) {
        if (g_dhcp_clients[i].active) {
            return &g_dhcp_clients[i];
        }
    }

    return NULL;
}

/*
 * dhcp_allocate_client - Alokasi client baru
 */
static dhcp_client_t *dhcp_allocate_client(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < DHCP_CLIENT_MAKS; i++) {
        if (!g_dhcp_clients[i].active) {
            g_dhcp_clients[i].active = BENAR;
            return &g_dhcp_clients[i];
        }
    }

    return NULL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * dhcp_init - Inisialisasi DHCP client
 */
status_t dhcp_init(void)
{
    tak_bertanda32_t i;

    /* Reset clients */
    for (i = 0; i < DHCP_CLIENT_MAKS; i++) {
        g_dhcp_clients[i].active = SALAH;
        g_dhcp_clients[i].state = 0;
        g_dhcp_clients[i].xid = 0;
        g_dhcp_clients[i].retries = 0;
        g_dhcp_clients[i].timeout = DHCP_TIMEOUT_INIT;
        kernel_memset(&g_dhcp_clients[i].config, 0,
                      sizeof(dhcp_config_t));
    }

    /* Reset statistik */
    g_dhcp_discoveries = 0;
    g_dhcp_offers = 0;
    g_dhcp_requests = 0;
    g_dhcp_acks = 0;
    g_dhcp_naks = 0;
    g_dhcp_releases = 0;
    g_dhcp_errors = 0;

    return STATUS_BERHASIL;
}

/*
 * dhcp_request - Request DHCP configuration
 */
status_t dhcp_request(netdev_t *dev, dhcp_config_t *config)
{
    dhcp_client_t *client;
    status_t hasil;

    if (dev == NULL || config == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Allocate client */
    client = dhcp_allocate_client();
    if (client == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    /* Initialize client */
    client->state = DHCP_DISCOVER;
    client->retries = 0;
    client->timeout = DHCP_TIMEOUT_INIT;
    client->start_time = kernel_get_ticks();
    kernel_memset(&client->config, 0, sizeof(dhcp_config_t));

    /* Send DISCOVER */
    hasil = dhcp_send_discover(dev, client);
    if (hasil != STATUS_BERHASIL) {
        client->active = SALAH;
        return hasil;
    }

    /* In real implementation, would wait for OFFER/ACK */
    /* For now, return timeout */
    client->active = SALAH;

    return STATUS_TIMEOUT;
}

/*
 * dhcp_release - Release DHCP lease
 */
status_t dhcp_release(netdev_t *dev)
{
    dhcp_client_t *client;
    tak_bertanda32_t i;

    if (dev == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Find client for this interface */
    for (i = 0; i < DHCP_CLIENT_MAKS; i++) {
        if (g_dhcp_clients[i].active) {
            client = &g_dhcp_clients[i];
            dhcp_send_release(dev, client);

            /* Clear config */
            kernel_memset(&client->config, 0, sizeof(dhcp_config_t));
            client->active = SALAH;

            return STATUS_BERHASIL;
        }
    }

    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * dhcp_renew - Renew DHCP lease
 */
status_t dhcp_renew(netdev_t *dev, dhcp_config_t *config)
{
    dhcp_client_t *client;

    if (dev == NULL || config == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Find existing client */
    client = dhcp_find_client(dev->id);
    if (client == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Send REQUEST for renewal */
    client->xid = dhcp_generate_xid();
    dhcp_send_request(dev, client, &client->config.alamat);

    return STATUS_BERHASIL;
}

/*
 * dhcp_proses_message - Proses pesan DHCP yang diterima
 */
status_t dhcp_proses_message(const void *data, ukuran_t len,
                             dhcp_config_t *config)
{
    const dhcp_message_t *msg;
    const tak_bertanda8_t *options;
    ukuran_t options_len;
    tak_bertanda8_t msg_type;
    ukuran_t i;

    if (data == NULL || config == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (len < DHCP_HEADER_SIZE) {
        g_dhcp_errors++;
        return STATUS_PARAM_UKURAN;
    }

    msg = (const dhcp_message_t *)data;

    /* Verify magic cookie */
    if (msg->cookie != DHCP_MAGIC_COOKIE) {
        g_dhcp_errors++;
        return STATUS_PARAM_INVALID;
    }

    /* Parse options to get message type */
    options = msg->options;
    options_len = len - DHCP_HEADER_SIZE;

    msg_type = 0;

    for (i = 0; i < options_len;) {
        if (options[i] == DHCP_OPT_PAD) {
            i++;
            continue;
        }

        if (options[i] == DHCP_OPT_END) {
            break;
        }

        if (i + 1 >= options_len) {
            break;
        }

        if (options[i] == DHCP_OPT_MSG_TYPE) {
            if (i + 2 < options_len) {
                msg_type = options[i + 2];
            }
            break;
        }

        i += 2 + options[i + 1];
    }

    /* Process based on message type */
    switch (msg_type) {
    case DHCP_MSG_OFFER:
        g_dhcp_offers++;

        /* Copy offered IP */
        kernel_memcpy(config->alamat.byte, &msg->yiaddr, 4);

        /* Parse other options */
        dhcp_parse_options(options, options_len, config);

        break;

    case DHCP_MSG_ACK:
        g_dhcp_acks++;

        /* Copy assigned IP */
        kernel_memcpy(config->alamat.byte, &msg->yiaddr, 4);

        /* Parse options */
        dhcp_parse_options(options, options_len, config);

        break;

    case DHCP_MSG_NAK:
        g_dhcp_naks++;

        /* Clear config */
        kernel_memset(config, 0, sizeof(dhcp_config_t));

        return NET_ERR_CONN_REFUSED;

    default:
        break;
    }

    return STATUS_BERHASIL;
}

/*
 * dhcp_get_config - Dapatkan konfigurasi DHCP
 */
status_t dhcp_get_config(netdev_t *dev, dhcp_config_t *config)
{
    dhcp_client_t *client;

    if (dev == NULL || config == NULL) {
        return STATUS_PARAM_NULL;
    }

    client = dhcp_find_client(dev->id);
    if (client == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    kernel_memcpy(config, &client->config, sizeof(dhcp_config_t));

    return STATUS_BERHASIL;
}

/*
 * dhcp_get_stats - Dapatkan statistik DHCP
 */
status_t dhcp_get_stats(tak_bertanda64_t *discoveries,
                        tak_bertanda64_t *offers,
                        tak_bertanda64_t *requests,
                        tak_bertanda64_t *acks)
{
    if (discoveries != NULL) {
        *discoveries = g_dhcp_discoveries;
    }

    if (offers != NULL) {
        *offers = g_dhcp_offers;
    }

    if (requests != NULL) {
        *requests = g_dhcp_requests;
    }

    if (acks != NULL) {
        *acks = g_dhcp_acks;
    }

    return STATUS_BERHASIL;
}

/*
 * dhcp_reset_stats - Reset statistik DHCP
 */
void dhcp_reset_stats(void)
{
    g_dhcp_discoveries = 0;
    g_dhcp_offers = 0;
    g_dhcp_requests = 0;
    g_dhcp_acks = 0;
    g_dhcp_naks = 0;
    g_dhcp_releases = 0;
    g_dhcp_errors = 0;
}

/*
 * dhcp_print_info - Cetak informasi DHCP
 */
void dhcp_print_info(void)
{
    tak_bertanda32_t i;
    char ip_str[16];

    kernel_printf("\n=== DHCP Clients ===\n\n");

    for (i = 0; i < DHCP_CLIENT_MAKS; i++) {
        if (g_dhcp_clients[i].active) {
            kernel_printf("Client %u:\n", i);

            ipv4_ke_string(&g_dhcp_clients[i].config.alamat,
                           ip_str, sizeof(ip_str));
            kernel_printf("  IP: %s\n", ip_str);

            ipv4_ke_string(&g_dhcp_clients[i].config.gateway,
                           ip_str, sizeof(ip_str));
            kernel_printf("  Gateway: %s\n", ip_str);

            ipv4_ke_string(&g_dhcp_clients[i].config.netmask,
                           ip_str, sizeof(ip_str));
            kernel_printf("  Netmask: %s\n", ip_str);

            kernel_printf("  Lease time: %u seconds\n",
                          g_dhcp_clients[i].config.lease_time);
        }
    }

    kernel_printf("\nStatistics:\n");
    kernel_printf("  Discoveries: %lu\n", g_dhcp_discoveries);
    kernel_printf("  Offers: %lu\n", g_dhcp_offers);
    kernel_printf("  Requests: %lu\n", g_dhcp_requests);
    kernel_printf("  ACKs: %lu\n", g_dhcp_acks);
    kernel_printf("  NAKs: %lu\n", g_dhcp_naks);
    kernel_printf("  Releases: %lu\n", g_dhcp_releases);
    kernel_printf("  Errors: %lu\n", g_dhcp_errors);
}

/*
 * dhcp_is_configured - Cek apakah interface sudah dikonfigurasi
 */
bool_t dhcp_is_configured(netdev_t *dev)
{
    dhcp_client_t *client;

    if (dev == NULL) {
        return SALAH;
    }

    client = dhcp_find_client(dev->id);

    if (client == NULL) {
        return SALAH;
    }

    /* Check if has valid IP */
    tak_bertanda32_t i;
    for (i = 0; i < 4; i++) {
        if (client->config.alamat.byte[i] != 0) {
            return BENAR;
        }
    }

    return SALAH;
}
