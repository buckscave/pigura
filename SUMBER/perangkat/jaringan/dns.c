/*
 * PIGURA OS - DNS.C
 * ==================
 * Implementasi DNS resolver.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi untuk menangani
 * resolusi nama DNS (Domain Name System) dalam Pigura OS.
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
 * KONSTANTA DNS
 * ===========================================================================
 */

/* DNS port */
#define DNS_PORT            53

/* DNS query types */
#define DNS_TYPE_A          1
#define DNS_TYPE_AAAA       28
#define DNS_TYPE_CNAME      5
#define DNS_TYPE_MX         15
#define DNS_TYPE_NS         2
#define DNS_TYPE_PTR        12
#define DNS_TYPE_TXT        16
#define DNS_TYPE_SOA        6

/* DNS query classes */
#define DNS_CLASS_IN        1

/* DNS response codes */
#define DNS_RCODE_OK        0
#define DNS_RCODE_FORMAT    1
#define DNS_RCODE_SERVER    2
#define DNS_RCODE_NAME      3
#define DNS_RCODE_NOTIMPL   4
#define DNS_RCODE_REFUSED   5

/* DNS flags */
#define DNS_FLAG_QR         0x8000  /* Response flag */
#define DNS_FLAG_AA         0x0400  /* Authoritative answer */
#define DNS_FLAG_TC         0x0200  /* Truncated */
#define DNS_FLAG_RD         0x0100  /* Recursion desired */
#define DNS_FLAG_RA         0x0080  /* Recursion available */

/* DNS timeout dan cache */
#define DNS_TIMEOUT_MS      5000
#define DNS_CACHE_TTL       300     /* 5 minutes */
#define DNS_MAX_RETRIES     3

/* Maximum sizes */
#define DNS_MAX_LABEL       63
#define DNS_MAX_NAME        255
#define DNS_MAX_UDP_SIZE    512
#define DNS_HEADER_SIZE     12

/*
 * ===========================================================================
 * STRUKTUR DNS
 * ===========================================================================
 */

/* DNS header */
typedef struct {
    tak_bertanda16_t id;        /* Query ID */
    tak_bertanda16_t flags;     /* Flags */
    tak_bertanda16_t qdcount;   /* Question count */
    tak_bertanda16_t ancount;   /* Answer count */
    tak_bertanda16_t nscount;   /* Authority count */
    tak_bertanda16_t arcount;   /* Additional count */
} dns_header_t;

/* DNS cache entry */
typedef struct {
    char nama[DNS_NAMA_MAKS];
    alamat_ipv4_t ipv4;
    alamat_ipv6_t ipv6;
    tak_bertanda32_t ttl;
    tak_bertanda64_t expiry;
    tak_bertanda32_t type;
    bool_t valid;
} dns_cache_entry_t;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

/* DNS cache */
#define DNS_CACHE_MAKS      64
static dns_cache_entry_t g_dns_cache[DNS_CACHE_MAKS];
static tak_bertanda32_t g_dns_cache_count = 0;

/* DNS servers */
static alamat_ipv4_t g_dns_servers[2];

/* Query ID counter */
static tak_bertanda16_t g_dns_query_id = 1;

/* Statistics */
static tak_bertanda64_t g_dns_queries = 0;
static tak_bertanda64_t g_dns_responses = 0;
static tak_bertanda64_t g_dns_cache_hits = 0;
static tak_bertanda64_t g_dns_cache_misses = 0;
static tak_bertanda64_t g_dns_timeouts = 0;
static tak_bertanda64_t g_dns_errors = 0;

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * dns_generate_id - Generate query ID
 */
static tak_bertanda16_t dns_generate_id(void)
{
    return g_dns_query_id++;
}

/*
 * dns_encode_name - Encode hostname ke DNS format
 */
static ukuran_t dns_encode_name(const char *hostname, tak_bertanda8_t *buffer,
                                ukuran_t maxlen)
{
    ukuran_t hostname_len;
    ukuran_t pos;
    ukuran_t label_start;
    ukuran_t label_len;
    ukuran_t i;

    if (hostname == NULL || buffer == NULL || maxlen == 0) {
        return 0;
    }

    hostname_len = kernel_strlen(hostname);
    if (hostname_len >= DNS_MAX_NAME) {
        return 0;
    }

    pos = 0;
    label_start = 0;
    label_len = 0;

    for (i = 0; i <= hostname_len && pos < maxlen - 1; i++) {
        if (hostname[i] == '.' || hostname[i] == '\0') {
            /* Write label length */
            if (label_len > 0 && label_len <= DNS_MAX_LABEL) {
                buffer[pos++] = (tak_bertanda8_t)label_len;

                /* Write label characters */
                if (pos + label_len >= maxlen) {
                    break;
                }

                kernel_memcpy(buffer + pos, hostname + label_start, label_len);
                pos += label_len;
            }

            label_start = i + 1;
            label_len = 0;
        } else {
            label_len++;
        }
    }

    /* Null terminator */
    if (pos < maxlen) {
        buffer[pos++] = 0;
    }

    return pos;
}

/*
 * dns_decode_name - Decode DNS name ke string
 */
static ukuran_t dns_decode_name(const tak_bertanda8_t *buffer,
                                ukuran_t buflen,
                                ukuran_t offset,
                                char *name,
                                ukuran_t namelen)
{
    ukuran_t pos;
    ukuran_t name_pos;
    tak_bertanda8_t label_len;
    bool_t jumped;
    ukuran_t jump_offset;
    ukuran_t i;

    if (buffer == NULL || name == NULL || namelen == 0) {
        return 0;
    }

    pos = offset;
    name_pos = 0;
    jumped = SALAH;
    jump_offset = 0;

    while (pos < buflen) {
        label_len = buffer[pos];

        /* Check for pointer (compression) */
        if ((label_len & 0xC0) == 0xC0) {
            if (pos + 1 >= buflen) {
                break;
            }

            if (!jumped) {
                jump_offset = pos + 2;
            }

            pos = ((label_len & 0x3F) << 8) | buffer[pos + 1];
            jumped = BENAR;
            continue;
        }

        if (label_len == 0) {
            pos++;
            break;
        }

        /* Add dot separator */
        if (name_pos > 0 && name_pos < namelen - 1) {
            name[name_pos++] = '.';
        }

        pos++;

        /* Copy label */
        for (i = 0; i < label_len && pos < buflen &&
             name_pos < namelen - 1; i++) {
            name[name_pos++] = buffer[pos++];
        }
    }

    name[name_pos] = '\0';

    if (jumped) {
        return jump_offset;
    }

    return pos;
}

/*
 * dns_build_query - Bangun DNS query
 */
static ukuran_t dns_build_query(tak_bertanda8_t *buffer, ukuran_t maxlen,
                                const char *hostname, tak_bertanda16_t type,
                                tak_bertanda16_t *query_id)
{
    dns_header_t *header;
    ukuran_t name_len;
    ukuran_t pos;
    tak_bertanda8_t *qtype;
    tak_bertanda8_t *qclass;

    if (buffer == NULL || hostname == NULL || maxlen < DNS_HEADER_SIZE + 6) {
        return 0;
    }

    kernel_memset(buffer, 0, maxlen);

    header = (dns_header_t *)buffer;

    /* Set header */
    header->id = dns_generate_id();
    *query_id = header->id;
    header->flags = DNS_FLAG_RD;  /* Recursion desired */
    header->qdcount = 1;
    header->ancount = 0;
    header->nscount = 0;
    header->arcount = 0;

    /* Encode name */
    pos = DNS_HEADER_SIZE;
    name_len = dns_encode_name(hostname, buffer + pos, maxlen - pos - 4);

    if (name_len == 0) {
        return 0;
    }

    pos += name_len;

    /* Query type */
    buffer[pos++] = (type >> 8) & 0xFF;
    buffer[pos++] = type & 0xFF;

    /* Query class (IN) */
    buffer[pos++] = (DNS_CLASS_IN >> 8) & 0xFF;
    buffer[pos++] = DNS_CLASS_IN & 0xFF;

    g_dns_queries++;

    return pos;
}

/*
 * dns_find_cache - Cari entry di cache
 */
static tak_bertanda32_t dns_find_cache(const char *hostname)
{
    tak_bertanda32_t i;

    for (i = 0; i < DNS_CACHE_MAKS; i++) {
        if (g_dns_cache[i].valid &&
            kernel_strcmp(g_dns_cache[i].nama, hostname) == 0) {
            return i;
        }
    }

    return INDEX_INVALID;
}

/*
 * dns_find_free_cache - Cari slot kosong di cache
 */
static tak_bertanda32_t dns_find_free_cache(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < DNS_CACHE_MAKS; i++) {
        if (!g_dns_cache[i].valid) {
            return i;
        }
    }

    return INDEX_INVALID;
}

/*
 * dns_find_oldest_cache - Cari entry tertua
 */
static tak_bertanda32_t dns_find_oldest_cache(void)
{
    tak_bertanda32_t i;
    tak_bertanda32_t oldest = 0;
    tak_bertanda64_t oldest_expiry = g_dns_cache[0].expiry;

    for (i = 1; i < DNS_CACHE_MAKS; i++) {
        if (g_dns_cache[i].valid &&
            g_dns_cache[i].expiry < oldest_expiry) {
            oldest_expiry = g_dns_cache[i].expiry;
            oldest = i;
        }
    }

    return oldest;
}

/*
 * dns_add_cache - Tambah entry ke cache
 */
static status_t dns_add_cache(const char *hostname, alamat_ipv4_t *ipv4,
                              tak_bertanda32_t ttl)
{
    tak_bertanda32_t idx;
    ukuran_t len;

    if (hostname == NULL || ipv4 == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Cari existing entry */
    idx = dns_find_cache(hostname);

    if (idx == INDEX_INVALID) {
        idx = dns_find_free_cache();

        if (idx == INDEX_INVALID) {
            idx = dns_find_oldest_cache();
        }
    }

    if (idx == INDEX_INVALID) {
        return STATUS_PENUH;
    }

    /* Set entry */
    len = kernel_strlen(hostname);
    if (len >= DNS_NAMA_MAKS) {
        len = DNS_NAMA_MAKS - 1;
    }
    kernel_strncpy(g_dns_cache[idx].nama, hostname, len);
    g_dns_cache[idx].nama[len] = '\0';

    kernel_memcpy(&g_dns_cache[idx].ipv4, ipv4, sizeof(alamat_ipv4_t));
    g_dns_cache[idx].ttl = ttl;
    g_dns_cache[idx].expiry = kernel_get_uptime() + ttl;
    g_dns_cache[idx].type = DNS_TYPE_A;
    g_dns_cache[idx].valid = BENAR;

    if (!g_dns_cache[idx].valid) {
        g_dns_cache_count++;
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * dns_init - Inisialisasi DNS resolver
 */
status_t dns_init(void)
{
    tak_bertanda32_t i;

    /* Reset cache */
    for (i = 0; i < DNS_CACHE_MAKS; i++) {
        g_dns_cache[i].valid = SALAH;
        g_dns_cache[i].nama[0] = '\0';
    }
    g_dns_cache_count = 0;

    /* Set default DNS servers (Google DNS) */
    g_dns_servers[0].byte[0] = 8;
    g_dns_servers[0].byte[1] = 8;
    g_dns_servers[0].byte[2] = 8;
    g_dns_servers[0].byte[3] = 8;

    g_dns_servers[1].byte[0] = 8;
    g_dns_servers[1].byte[1] = 8;
    g_dns_servers[1].byte[2] = 4;
    g_dns_servers[1].byte[3] = 4;

    /* Reset statistik */
    g_dns_queries = 0;
    g_dns_responses = 0;
    g_dns_cache_hits = 0;
    g_dns_cache_misses = 0;
    g_dns_timeouts = 0;
    g_dns_errors = 0;

    return STATUS_BERHASIL;
}

/*
 * dns_resolve - Resolve hostname ke IP
 */
status_t dns_resolve(const char *hostname, alamat_ipv4_t *ipv4)
{
    tak_bertanda32_t idx;
    tak_bertanda8_t buffer[DNS_MAX_UDP_SIZE];
    ukuran_t query_len;
    tak_bertanda16_t query_id;

    if (hostname == NULL || ipv4 == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Cek cache dulu */
    idx = dns_find_cache(hostname);

    if (idx != INDEX_INVALID && g_dns_cache[idx].valid) {
        /* Check TTL */
        if (kernel_get_uptime() < g_dns_cache[idx].expiry) {
            kernel_memcpy(ipv4, &g_dns_cache[idx].ipv4,
                          sizeof(alamat_ipv4_t));
            g_dns_cache_hits++;
            return STATUS_BERHASIL;
        }

        /* Entry expired */
        g_dns_cache[idx].valid = SALAH;
    }

    g_dns_cache_misses++;

    /* Build query */
    query_len = dns_build_query(buffer, sizeof(buffer), hostname,
                                DNS_TYPE_A, &query_id);

    if (query_len == 0) {
        g_dns_errors++;
        return STATUS_GAGAL;
    }

    /* Send query via UDP */
    /* Gunakan DNS server pertama */
    {
        tak_bertanda32_t dns_ip;

        dns_ip = (g_dns_servers[0].byte[0] << 24) |
                 (g_dns_servers[0].byte[1] << 16) |
                 (g_dns_servers[0].byte[2] << 8) |
                 g_dns_servers[0].byte[3];

        udp_send(DNS_PORT, dns_ip, DNS_PORT, buffer, query_len);
    }

    /* In real implementation, would wait for response */
    /* For now, return not found */
    return STATUS_TIMEOUT;
}

/*
 * dns_set_server - Set DNS server
 */
status_t dns_set_server(tak_bertanda32_t index, alamat_ipv4_t *server)
{
    if (server == NULL || index > 1) {
        return STATUS_PARAM_INVALID;
    }

    kernel_memcpy(&g_dns_servers[index], server, sizeof(alamat_ipv4_t));

    return STATUS_BERHASIL;
}

/*
 * dns_get_server - Dapatkan DNS server
 */
status_t dns_get_server(tak_bertanda32_t index, alamat_ipv4_t *server)
{
    if (server == NULL || index > 1) {
        return STATUS_PARAM_INVALID;
    }

    kernel_memcpy(server, &g_dns_servers[index], sizeof(alamat_ipv4_t));

    return STATUS_BERHASIL;
}

/*
 * dns_parse_response - Parse respons DNS
 */
status_t dns_parse_response(const void *data, ukuran_t len,
                            char *hostname, alamat_ipv4_t *ipv4)
{
    const dns_header_t *header;
    const tak_bertanda8_t *ptr;
    ukuran_t pos;
    tak_bertanda16_t ancount;
    tak_bertanda16_t rcode;
    char name[DNS_NAMA_MAKS];
    tak_bertanda16_t rr_type;
    tak_bertanda16_t rr_class;
    tak_bertanda32_t rr_ttl;
    tak_bertanda16_t rr_len;
    ukuran_t i;

    if (data == NULL || len < DNS_HEADER_SIZE) {
        g_dns_errors++;
        return STATUS_PARAM_INVALID;
    }

    header = (const dns_header_t *)data;

    /* Check response flag */
    if (!(header->flags & DNS_FLAG_QR)) {
        g_dns_errors++;
        return STATUS_PARAM_INVALID;
    }

    /* Check response code */
    rcode = header->flags & 0x000F;

    if (rcode != DNS_RCODE_OK) {
        g_dns_errors++;
        return STATUS_TIDAK_DITEMUKAN;
    }

    ancount = header->ancount;
    g_dns_responses++;

    /* Skip question section */
    pos = DNS_HEADER_SIZE;

    /* Skip one question */
    {
        ptr = (const tak_bertanda8_t *)data;

        /* Skip QNAME */
        while (pos < len && ptr[pos] != 0) {
            if ((ptr[pos] & 0xC0) == 0xC0) {
                pos += 2;
                break;
            }
            pos += ptr[pos] + 1;
        }

        if (pos < len && ptr[pos] == 0) {
            pos++;
        }

        /* Skip QTYPE dan QCLASS */
        pos += 4;
    }

    /* Parse answer section */
    for (i = 0; i < ancount && pos < len; i++) {
        /* Decode name */
        pos = dns_decode_name((const tak_bertanda8_t *)data, len, pos,
                              name, sizeof(name));

        if (pos + 10 > len) {
            break;
        }

        ptr = (const tak_bertanda8_t *)data;

        /* Type */
        rr_type = (ptr[pos] << 8) | ptr[pos + 1];
        pos += 2;

        /* Class */
        rr_class = (ptr[pos] << 8) | ptr[pos + 1];
        pos += 2;

        /* TTL */
        rr_ttl = (ptr[pos] << 24) | (ptr[pos + 1] << 16) |
                 (ptr[pos + 2] << 8) | ptr[pos + 3];
        pos += 4;

        /* Data length */
        rr_len = (ptr[pos] << 8) | ptr[pos + 1];
        pos += 2;

        if (pos + rr_len > len) {
            break;
        }

        /* Process A record */
        if (rr_type == DNS_TYPE_A && rr_class == DNS_CLASS_IN &&
            rr_len == 4) {
            kernel_memcpy(ipv4->byte, ptr + pos, 4);

            if (hostname != NULL) {
                kernel_strncpy(hostname, name, DNS_NAMA_MAKS - 1);
                hostname[DNS_NAMA_MAKS - 1] = '\0';
            }

            /* Add to cache */
            dns_add_cache(name, ipv4, rr_ttl);

            return STATUS_BERHASIL;
        }

        pos += rr_len;
    }

    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * dns_flush_cache - Flush DNS cache
 */
void dns_flush_cache(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < DNS_CACHE_MAKS; i++) {
        g_dns_cache[i].valid = SALAH;
    }

    g_dns_cache_count = 0;
}

/*
 * dns_get_cache_count - Dapatkan jumlah entry cache
 */
tak_bertanda32_t dns_get_cache_count(void)
{
    return g_dns_cache_count;
}

/*
 * dns_get_stats - Dapatkan statistik DNS
 */
status_t dns_get_stats(tak_bertanda64_t *queries,
                       tak_bertanda64_t *responses,
                       tak_bertanda64_t *cache_hits,
                       tak_bertanda64_t *cache_misses)
{
    if (queries != NULL) {
        *queries = g_dns_queries;
    }

    if (responses != NULL) {
        *responses = g_dns_responses;
    }

    if (cache_hits != NULL) {
        *cache_hits = g_dns_cache_hits;
    }

    if (cache_misses != NULL) {
        *cache_misses = g_dns_cache_misses;
    }

    return STATUS_BERHASIL;
}

/*
 * dns_reset_stats - Reset statistik DNS
 */
void dns_reset_stats(void)
{
    g_dns_queries = 0;
    g_dns_responses = 0;
    g_dns_cache_hits = 0;
    g_dns_cache_misses = 0;
    g_dns_timeouts = 0;
    g_dns_errors = 0;
}

/*
 * dns_print_cache - Cetak DNS cache
 */
void dns_print_cache(void)
{
    tak_bertanda32_t i;
    char ip_str[16];

    kernel_printf("\n=== DNS Cache ===\n\n");

    for (i = 0; i < DNS_CACHE_MAKS; i++) {
        if (g_dns_cache[i].valid) {
            ipv4_ke_string(&g_dns_cache[i].ipv4, ip_str, sizeof(ip_str));
            kernel_printf("%s -> %s (TTL: %u)\n",
                          g_dns_cache[i].nama, ip_str,
                          g_dns_cache[i].ttl);
        }
    }

    kernel_printf("\nTotal entries: %u\n", g_dns_cache_count);
    kernel_printf("Cache hits: %lu\n", g_dns_cache_hits);
    kernel_printf("Cache misses: %lu\n", g_dns_cache_misses);
}

/*
 * dns_is_cached - Cek apakah hostname sudah di-cache
 */
bool_t dns_is_cached(const char *hostname)
{
    tak_bertanda32_t idx;

    if (hostname == NULL) {
        return SALAH;
    }

    idx = dns_find_cache(hostname);

    if (idx == INDEX_INVALID) {
        return SALAH;
    }

    return g_dns_cache[idx].valid;
}

/*
 * dns_remove_cache - Hapus entry dari cache
 */
status_t dns_remove_cache(const char *hostname)
{
    tak_bertanda32_t idx;

    if (hostname == NULL) {
        return STATUS_PARAM_NULL;
    }

    idx = dns_find_cache(hostname);

    if (idx == INDEX_INVALID) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    g_dns_cache[idx].valid = SALAH;
    g_dns_cache_count--;

    return STATUS_BERHASIL;
}
