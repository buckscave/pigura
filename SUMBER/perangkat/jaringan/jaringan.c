/*
 * PIGURA OS - JARINGAN.C
 * =======================
 * Implementasi subsistem jaringan.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "jaringan.h"
#include "../../inti/kernel.h"

/* Forward declaration - defined at line ~1005 */
extern status_t arp_proses(paket_t *paket);

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

jaringan_konteks_t g_jaringan_konteks;
bool_t g_jaringan_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static char g_jaringan_error[256];

/* Pool untuk DNS cache */
#define DNS_CACHE_MAKS 64
static dns_entry_t g_dns_cache[DNS_CACHE_MAKS];
static tak_bertanda32_t g_dns_cache_count = 0;

/* DNS servers */
static alamat_ipv4_t g_dns_servers[2];

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * jaringan_set_error - Set pesan error
 */
static void jaringan_set_error(const char *pesan)
{
    ukuran_t i;
    
    if (pesan == NULL) {
        return;
    }
    
    i = 0;
    while (pesan[i] != '\0' && i < 255) {
        g_jaringan_error[i] = pesan[i];
        i++;
    }
    g_jaringan_error[i] = '\0';
}

/*
 * jaringan_validasi - Validasi konteks
 */
static bool_t jaringan_validasi(void)
{
    if (!g_jaringan_diinisialisasi) {
        jaringan_set_error("Jaringan belum diinisialisasi");
        return SALAH;
    }
    
    if (g_jaringan_konteks.magic != JARINGAN_MAGIC) {
        jaringan_set_error("Konteks jaringan tidak valid");
        return SALAH;
    }
    
    return BENAR;
}

/*
 * inisialisasi_konteks - Inisialisasi konteks jaringan
 */
static void inisialisasi_konteks(jaringan_konteks_t *ctx)
{
    tak_bertanda32_t i;
    
    if (ctx == NULL) {
        return;
    }
    
    ctx->magic = 0;
    ctx->netdev_list = NULL;
    ctx->netdev_count = 0;
    ctx->netdev_default = NULL;
    ctx->socket_list = NULL;
    ctx->socket_count = 0;
    ctx->dns_cache = g_dns_cache;
    ctx->dns_cache_size = 0;
    ctx->total_rx = 0;
    ctx->total_tx = 0;
    ctx->diinisialisasi = SALAH;
    
    /* Inisialisasi DNS servers (Google DNS) */
    g_dns_servers[0].byte[0] = 8;
    g_dns_servers[0].byte[1] = 8;
    g_dns_servers[0].byte[2] = 8;
    g_dns_servers[0].byte[3] = 8;
    
    g_dns_servers[1].byte[0] = 8;
    g_dns_servers[1].byte[1] = 8;
    g_dns_servers[1].byte[2] = 4;
    g_dns_servers[1].byte[3] = 4;
    
    /* Clear DNS cache */
    for (i = 0; i < DNS_CACHE_MAKS; i++) {
        g_dns_cache[i].nama[0] = '\0';
        g_dns_cache[i].ttl = 0;
        g_dns_cache[i].expiry = 0;
    }
    g_dns_cache_count = 0;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - INISIALISASI
 * ===========================================================================
 */

status_t jaringan_init(void)
{
    status_t hasil;
    
    if (g_jaringan_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }
    
    /* Inisialisasi konteks */
    inisialisasi_konteks(&g_jaringan_konteks);
    
    /* Inisialisasi subsistem */
    hasil = ethernet_init();
    if (hasil != STATUS_BERHASIL) {
        /* Ethernet init gagal, lanjutkan */
    }
    
    hasil = arp_init();
    if (hasil != STATUS_BERHASIL) {
        /* ARP init gagal, lanjutkan */
    }
    
    hasil = tcp_init();
    if (hasil != STATUS_BERHASIL) {
        /* TCP init gagal, lanjutkan */
    }
    
    hasil = udp_init();
    if (hasil != STATUS_BERHASIL) {
        /* UDP init gagal, lanjutkan */
    }
    
    hasil = dns_init();
    if (hasil != STATUS_BERHASIL) {
        /* DNS init gagal, lanjutkan */
    }
    
    hasil = dhcp_init();
    if (hasil != STATUS_BERHASIL) {
        /* DHCP init gagal, lanjutkan */
    }
    
    hasil = wifi_init();
    if (hasil != STATUS_BERHASIL) {
        /* WiFi init gagal, lanjutkan */
    }
    
    /* Inisialisasi loopback */
    hasil = loopback_init();
    if (hasil != STATUS_BERHASIL) {
        jaringan_set_error("Gagal inisialisasi loopback");
        return hasil;
    }
    
    /* Finalisasi */
    g_jaringan_konteks.magic = JARINGAN_MAGIC;
    g_jaringan_konteks.diinisialisasi = BENAR;
    g_jaringan_diinisialisasi = BENAR;
    
    return STATUS_BERHASIL;
}

status_t jaringan_shutdown(void)
{
    netdev_t *dev;
    netdev_t *next;
    
    if (!g_jaringan_diinisialisasi) {
        return STATUS_BERHASIL;
    }
    
    /* Tutup semua device */
    dev = g_jaringan_konteks.netdev_list;
    while (dev != NULL) {
        next = dev->berikutnya;
        if (dev->tutup != NULL) {
            dev->tutup(dev);
        }
        dev = next;
    }
    
    /* Reset konteks */
    g_jaringan_konteks.magic = 0;
    g_jaringan_konteks.netdev_list = NULL;
    g_jaringan_konteks.netdev_count = 0;
    g_jaringan_konteks.diinisialisasi = SALAH;
    
    g_jaringan_diinisialisasi = SALAH;
    
    return STATUS_BERHASIL;
}

jaringan_konteks_t *jaringan_konteks_dapatkan(void)
{
    if (!g_jaringan_diinisialisasi) {
        return NULL;
    }
    
    if (g_jaringan_konteks.magic != JARINGAN_MAGIC) {
        return NULL;
    }
    
    return &g_jaringan_konteks;
}

/*
 * ===========================================================================
 * FUNGSI NETWORK DEVICE
 * ===========================================================================
 */

netdev_t *netdev_register(const char *nama, tak_bertanda32_t tipe)
{
    netdev_t *dev;
    
    if (!jaringan_validasi()) {
        return NULL;
    }
    
    if (nama == NULL) {
        return NULL;
    }
    
    /* Alokasi device */
    dev = (netdev_t *)kmalloc(sizeof(netdev_t));
    if (dev == NULL) {
        jaringan_set_error("Gagal alokasi memori untuk device");
        return NULL;
    }
    
    /* Inisialisasi */
    kernel_memset(dev, 0, sizeof(netdev_t));
    
    dev->magic = NETDEV_MAGIC;
    dev->id = g_jaringan_konteks.netdev_count;
    
    /* Salin nama */
    {
        ukuran_t i = 0;
        while (nama[i] != '\0' && i < NETDEV_NAMA_PANJANG_MAKS - 1) {
            dev->nama[i] = nama[i];
            i++;
        }
        dev->nama[i] = '\0';
    }
    
    dev->tipe = tipe;
    dev->status = NETDEV_STATUS_DOWN;
    dev->flags = 0;
    dev->mtu = MTU_ETHERNET;
    dev->tx_queue_len = 1000;
    dev->berikutnya = NULL;
    dev->priv = NULL;
    
    /* Default MAC (00:00:00:00:00:00) */
    kernel_memset(&dev->mac, 0, sizeof(alamat_mac_t));
    
    /* Tambah ke list */
    if (g_jaringan_konteks.netdev_list == NULL) {
        g_jaringan_konteks.netdev_list = dev;
    } else {
        netdev_t *last = g_jaringan_konteks.netdev_list;
        while (last->berikutnya != NULL) {
            last = last->berikutnya;
        }
        last->berikutnya = dev;
    }
    
    g_jaringan_konteks.netdev_count++;
    
    /* Set sebagai default jika pertama */
    if (g_jaringan_konteks.netdev_default == NULL) {
        g_jaringan_konteks.netdev_default = dev;
    }
    
    return dev;
}

status_t netdev_unregister(netdev_t *dev)
{
    netdev_t *prev;
    netdev_t *curr;
    
    if (!jaringan_validasi()) {
        return STATUS_GAGAL;
    }
    
    if (dev == NULL || dev->magic != NETDEV_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Cari di list */
    prev = NULL;
    curr = g_jaringan_konteks.netdev_list;
    
    while (curr != NULL) {
        if (curr == dev) {
            /* Ditemukan, hapus dari list */
            if (prev == NULL) {
                g_jaringan_konteks.netdev_list = curr->berikutnya;
            } else {
                prev->berikutnya = curr->berikutnya;
            }
            
            /* Update default */
            if (g_jaringan_konteks.netdev_default == dev) {
                g_jaringan_konteks.netdev_default = 
                    g_jaringan_konteks.netdev_list;
            }
            
            /* Bebaskan */
            dev->magic = 0;
            kfree(dev);
            
            g_jaringan_konteks.netdev_count--;
            
            return STATUS_BERHASIL;
        }
        
        prev = curr;
        curr = curr->berikutnya;
    }
    
    return STATUS_TIDAK_DITEMUKAN;
}

netdev_t *netdev_cari(const char *nama)
{
    netdev_t *dev;
    
    if (!jaringan_validasi()) {
        return NULL;
    }
    
    if (nama == NULL) {
        return NULL;
    }
    
    dev = g_jaringan_konteks.netdev_list;
    
    while (dev != NULL) {
        ukuran_t i = 0;
        bool_t sama = BENAR;
        
        while (nama[i] != '\0' && dev->nama[i] != '\0') {
            if (nama[i] != dev->nama[i]) {
                sama = SALAH;
                break;
            }
            i++;
        }
        
        if (sama && nama[i] == '\0' && dev->nama[i] == '\0') {
            return dev;
        }
        
        dev = dev->berikutnya;
    }
    
    return NULL;
}

netdev_t *netdev_cari_id(tak_bertanda32_t id)
{
    netdev_t *dev;
    
    if (!jaringan_validasi()) {
        return NULL;
    }
    
    dev = g_jaringan_konteks.netdev_list;
    
    while (dev != NULL) {
        if (dev->id == id) {
            return dev;
        }
        dev = dev->berikutnya;
    }
    
    return NULL;
}

status_t netdev_up(netdev_t *dev)
{
    if (dev == NULL || dev->magic != NETDEV_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    dev->status = NETDEV_STATUS_UP;
    dev->flags |= NETDEV_FLAG_UP;
    
    /* Panggil callback buka jika ada */
    if (dev->buka != NULL) {
        return dev->buka(dev);
    }
    
    return STATUS_BERHASIL;
}

status_t netdev_down(netdev_t *dev)
{
    if (dev == NULL || dev->magic != NETDEV_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    dev->status = NETDEV_STATUS_DOWN;
    dev->flags &= ~NETDEV_FLAG_UP;
    
    return STATUS_BERHASIL;
}

status_t netdev_set_ipv4(netdev_t *dev, alamat_ipv4_t *addr,
                          alamat_ipv4_t *netmask)
{
    if (dev == NULL || dev->magic != NETDEV_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    if (addr != NULL) {
        kernel_memcpy(&dev->ipv4, addr, sizeof(alamat_ipv4_t));
    }
    
    if (netmask != NULL) {
        kernel_memcpy(&dev->ipv4_netmask, netmask, sizeof(alamat_ipv4_t));
        
        /* Hitung broadcast address */
        dev->ipv4_broadcast.byte[0] = addr->byte[0] | ~netmask->byte[0];
        dev->ipv4_broadcast.byte[1] = addr->byte[1] | ~netmask->byte[1];
        dev->ipv4_broadcast.byte[2] = addr->byte[2] | ~netmask->byte[2];
        dev->ipv4_broadcast.byte[3] = addr->byte[3] | ~netmask->byte[3];
    }
    
    return STATUS_BERHASIL;
}

status_t netdev_set_gateway(netdev_t *dev, alamat_ipv4_t *gateway)
{
    if (dev == NULL || dev->magic != NETDEV_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    if (gateway != NULL) {
        kernel_memcpy(&dev->ipv4_gateway, gateway, sizeof(alamat_ipv4_t));
    }
    
    return STATUS_BERHASIL;
}

status_t netdev_kirim(netdev_t *dev, const void *data, ukuran_t len)
{
    status_t hasil;
    
    if (dev == NULL || dev->magic != NETDEV_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    if (data == NULL || len == 0) {
        return STATUS_PARAM_INVALID;
    }
    
    if (!(dev->flags & NETDEV_FLAG_UP)) {
        return NET_ERR_IFACE_DOWN;
    }
    
    if (dev->kirim != NULL) {
        hasil = dev->kirim(dev, data, len);
        
        if (hasil == STATUS_BERHASIL) {
            dev->tx_packets++;
            dev->tx_bytes += len;
            g_jaringan_konteks.total_tx += len;
        } else {
            dev->tx_errors++;
        }
        
        return hasil;
    }
    
    return NET_ERR_UNKNOWN;
}

status_t netdev_terima(netdev_t *dev, void *data, ukuran_t *len)
{
    status_t hasil;
    
    if (dev == NULL || dev->magic != NETDEV_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    if (data == NULL || len == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    if (!(dev->flags & NETDEV_FLAG_UP)) {
        return NET_ERR_IFACE_DOWN;
    }
    
    if (dev->terima != NULL) {
        hasil = dev->terima(dev, data, len);
        
        if (hasil == STATUS_BERHASIL) {
            dev->rx_packets++;
            dev->rx_bytes += *len;
            g_jaringan_konteks.total_rx += *len;
        } else {
            dev->rx_errors++;
        }
        
        return hasil;
    }
    
    return NET_ERR_UNKNOWN;
}

/*
 * ===========================================================================
 * FUNGSI UTILITAS
 * ===========================================================================
 */

tak_bertanda32_t ipv4_ke_string(alamat_ipv4_t *addr, char *buffer,
                                  ukuran_t len)
{
    char temp[4];
    ukuran_t pos;
    tak_bertanda32_t i;
    
    if (addr == NULL || buffer == NULL || len < 16) {
        return 0;
    }
    
    pos = 0;
    
    for (i = 0; i < 4; i++) {
        tak_bertanda32_t num = addr->byte[i];
        tak_bertanda32_t j = 0;
        
        /* Konversi ke string */
        if (num == 0) {
            temp[0] = '0';
            j = 1;
        } else {
            while (num > 0 && j < 3) {
                temp[j] = '0' + (num % 10);
                num /= 10;
                j++;
            }
        }
        
        /* Reverse dan copy */
        while (j > 0 && pos < len - 1) {
            j--;
            buffer[pos++] = temp[j];
        }
        
        /* Tambahkan dot */
        if (i < 3 && pos < len - 1) {
            buffer[pos++] = '.';
        }
    }
    
    buffer[pos] = '\0';
    
    return (tak_bertanda32_t)pos;
}

status_t string_ke_ipv4(const char *str, alamat_ipv4_t *addr)
{
    tak_bertanda32_t i;
    tak_bertanda32_t octet;
    tak_bertanda32_t byte_index;
    
    if (str == NULL || addr == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    byte_index = 0;
    octet = 0;
    i = 0;
    
    while (str[i] != '\0' && byte_index < 4) {
        if (str[i] >= '0' && str[i] <= '9') {
            octet = octet * 10 + (str[i] - '0');
            
            if (octet > 255) {
                return STATUS_PARAM_INVALID;
            }
        } else if (str[i] == '.') {
            addr->byte[byte_index++] = (tak_bertanda8_t)octet;
            octet = 0;
        } else {
            return STATUS_PARAM_INVALID;
        }
        
        i++;
    }
    
    if (byte_index == 3) {
        addr->byte[byte_index] = (tak_bertanda8_t)octet;
        return STATUS_BERHASIL;
    }
    
    return STATUS_PARAM_INVALID;
}

tak_bertanda32_t mac_ke_string(alamat_mac_t *mac, char *buffer,
                                 ukuran_t len)
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

status_t string_ke_mac(const char *str, alamat_mac_t *mac)
{
    tak_bertanda32_t i;
    tak_bertanda32_t byte_index;
    tak_bertanda32_t nibble_count;
    tak_bertanda8_t val;
    
    if (str == NULL || mac == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    byte_index = 0;
    nibble_count = 0;
    val = 0;
    i = 0;
    
    while (str[i] != '\0' && byte_index < 6) {
        if (str[i] >= '0' && str[i] <= '9') {
            val = (val << 4) | (str[i] - '0');
            nibble_count++;
        } else if (str[i] >= 'A' && str[i] <= 'F') {
            val = (val << 4) | (str[i] - 'A' + 10);
            nibble_count++;
        } else if (str[i] >= 'a' && str[i] <= 'f') {
            val = (val << 4) | (str[i] - 'a' + 10);
            nibble_count++;
        } else if (str[i] == ':' || str[i] == '-') {
            if (nibble_count == 2) {
                mac->byte[byte_index++] = val;
                val = 0;
                nibble_count = 0;
            }
        }
        
        i++;
    }
    
    if (nibble_count == 2 && byte_index == 5) {
        mac->byte[byte_index] = val;
        return STATUS_BERHASIL;
    }
    
    return STATUS_PARAM_INVALID;
}

tak_bertanda16_t hitung_checksum(const void *data, ukuran_t len)
{
    tak_bertanda32_t sum;
    const tak_bertanda16_t *ptr;
    ukuran_t i;
    
    if (data == NULL || len == 0) {
        return 0;
    }
    
    sum = 0;
    ptr = (const tak_bertanda16_t *)data;
    
    /* Sum semua 16-bit words */
    for (i = 0; i < len / 2; i++) {
        sum += ptr[i];
    }
    
    /* Tambahkan byte tersisa jika ada */
    if (len & 1) {
        sum += ((const tak_bertanda8_t *)data)[len - 1];
    }
    
    /* Fold 32-bit sum ke 16-bit */
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return (tak_bertanda16_t)~sum;
}

void jaringan_cetak_info(void)
{
    netdev_t *dev;
    char buffer[32];
    
    if (!g_jaringan_diinisialisasi) {
        return;
    }
    
    /* Print header */
    kernel_printf("\n=== Network Devices ===\n\n");
    
    /* Iterasi device */
    dev = g_jaringan_konteks.netdev_list;
    
    while (dev != NULL) {
        /* Nama device */
        kernel_printf("%s: ", dev->nama);
        
        /* Status */
        if (dev->flags & NETDEV_FLAG_UP) {
            kernel_printf("UP");
        } else {
            kernel_printf("DOWN");
        }
        
        /* MAC address */
        mac_ke_string(&dev->mac, buffer, sizeof(buffer));
        kernel_printf(" <%s>\n", buffer);
        
        /* IP address */
        ipv4_ke_string(&dev->ipv4, buffer, sizeof(buffer));
        kernel_printf("    inet %s", buffer);
        
        ipv4_ke_string(&dev->ipv4_netmask, buffer, sizeof(buffer));
        kernel_printf(" netmask %s\n", buffer);
        
        /* Statistics */
        kernel_printf("    RX: %lu packets, %lu bytes\n",
                     dev->rx_packets, dev->rx_bytes);
        kernel_printf("    TX: %lu packets, %lu bytes\n",
                     dev->tx_packets, dev->tx_bytes);
        
        dev = dev->berikutnya;
    }
    
    kernel_printf("\n");
}

/*
 * ===========================================================================
 * FUNGSI LOOPBACK
 * ===========================================================================
 */

static netdev_t *g_loopback_dev = NULL;

static status_t loopback_kirim(netdev_t *dev, const void *data, ukuran_t len)
{
    (void)dev; (void)data; (void)len;
    /* Loopback: kirim ke diri sendiri */
    /* Untuk implementasi lengkap, perlu queue RX */
    return STATUS_BERHASIL;
}

status_t loopback_init(void)
{
    netdev_t *dev;
    alamat_ipv4_t addr;
    
    dev = netdev_register("lo", NETDEV_TIPE_LOOPBACK);
    if (dev == NULL) {
        return STATUS_GAGAL;
    }
    
    /* Set callbacks */
    dev->kirim = loopback_kirim;
    
    /* Set IP 127.0.0.1 */
    addr.byte[0] = 127;
    addr.byte[1] = 0;
    addr.byte[2] = 0;
    addr.byte[3] = 1;
    
    netdev_set_ipv4(dev, &addr, NULL);
    
    /* Set flags */
    dev->flags = NETDEV_FLAG_UP | NETDEV_FLAG_LOOPBACK;
    dev->mtu = MTU_LOOPBACK;
    dev->status = NETDEV_STATUS_RUNNING;
    
    g_loopback_dev = dev;
    
    return netdev_up(dev);
}

/*
 * ===========================================================================
 * FUNGSI ETHERNET
 * ===========================================================================
 */

status_t ethernet_init(void)
{
    return STATUS_BERHASIL;
}

status_t ethernet_proses(paket_t *paket)
{
    if (paket == NULL || paket->magic != PAKET_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Parse header ethernet */
    paket->eth = (eth_header_t *)paket->data;
    
    /* Cek tipe protocol */
    switch (paket->eth->tipe) {
    case 0x0800:  /* IPv4 */
        /* Proses IP */
        break;
    case 0x0806:  /* ARP */
        /* Proses ARP */
        arp_proses(paket);
        break;
    case 0x86DD:  /* IPv6 */
        /* Proses IPv6 */
        break;
    default:
        /* Unknown protocol */
        break;
    }
    
    return STATUS_BERHASIL;
}

status_t ethernet_kirim(netdev_t *dev, alamat_mac_t *dest,
                         tak_bertanda16_t tipe, const void *data,
                         ukuran_t len)
{
    eth_header_t *eth;
    tak_bertanda8_t *buffer;
    ukuran_t total_len;
    
    if (dev == NULL || dest == NULL || data == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (len > dev->mtu) {
        return STATUS_PARAM_UKURAN;
    }
    
    total_len = sizeof(eth_header_t) + len;
    
    buffer = (tak_bertanda8_t *)kmalloc(total_len);
    if (buffer == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    /* Build header */
    eth = (eth_header_t *)buffer;
    kernel_memcpy(&eth->dest, dest, sizeof(alamat_mac_t));
    kernel_memcpy(&eth->src, &dev->mac, sizeof(alamat_mac_t));
    eth->tipe = tipe;
    
    /* Copy payload */
    kernel_memcpy(buffer + sizeof(eth_header_t), data, len);
    
    /* Kirim */
    return netdev_kirim(dev, buffer, total_len);
}

/*
 * ===========================================================================
 * FUNGSI ARP
 * ===========================================================================
 */

#define ARP_CACHE_MAKS 64

static struct {
    alamat_ipv4_t ip;
    alamat_mac_t mac;
    tak_bertanda64_t timestamp;
    bool_t valid;
} g_arp_cache[ARP_CACHE_MAKS];

static tak_bertanda32_t g_arp_cache_index = 0;

status_t arp_init(void)
{
    tak_bertanda32_t i;
    
    for (i = 0; i < ARP_CACHE_MAKS; i++) {
        g_arp_cache[i].valid = SALAH;
    }
    
    g_arp_cache_index = 0;
    
    return STATUS_BERHASIL;
}

status_t arp_cari(netdev_t *dev, alamat_ipv4_t *ip, alamat_mac_t *mac)
{
    (void)dev;
    tak_bertanda32_t i;
    
    if (ip == NULL || mac == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Cari di cache */
    for (i = 0; i < ARP_CACHE_MAKS; i++) {
        if (g_arp_cache[i].valid) {
            if (kernel_memcmp(&g_arp_cache[i].ip, ip, 
                             sizeof(alamat_ipv4_t)) == 0) {
                kernel_memcpy(mac, &g_arp_cache[i].mac, 
                             sizeof(alamat_mac_t));
                return STATUS_BERHASIL;
            }
        }
    }
    
    return STATUS_TIDAK_DITEMUKAN;
}

status_t arp_tambah(netdev_t *dev, alamat_ipv4_t *ip, alamat_mac_t *mac)
{
    (void)dev;
    tak_bertanda32_t index;
    
    if (ip == NULL || mac == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Gunakan round-robin */
    index = g_arp_cache_index;
    g_arp_cache_index = (g_arp_cache_index + 1) % ARP_CACHE_MAKS;
    
    /* Simpan entry */
    kernel_memcpy(&g_arp_cache[index].ip, ip, sizeof(alamat_ipv4_t));
    kernel_memcpy(&g_arp_cache[index].mac, mac, sizeof(alamat_mac_t));
    g_arp_cache[index].timestamp = 0;  /* TODO: get timestamp */
    g_arp_cache[index].valid = BENAR;
    
    return STATUS_BERHASIL;
}

status_t arp_hapus(netdev_t *dev, alamat_ipv4_t *ip)
{
    (void)dev;
    tak_bertanda32_t i;
    
    if (ip == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    for (i = 0; i < ARP_CACHE_MAKS; i++) {
        if (g_arp_cache[i].valid) {
            if (kernel_memcmp(&g_arp_cache[i].ip, ip,
                             sizeof(alamat_ipv4_t)) == 0) {
                g_arp_cache[i].valid = SALAH;
                return STATUS_BERHASIL;
            }
        }
    }
    
    return STATUS_TIDAK_DITEMUKAN;
}

status_t arp_proses(paket_t *paket)
{
    arp_header_t *arp;
    
    if (paket == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    arp = (arp_header_t *)(paket->data + sizeof(eth_header_t));
    
    /* Hanya proses ARP request/reply untuk IPv4 */
    if (arp->hardware_type != 0x0001 || arp->protocol_type != 0x0800) {
        return STATUS_BERHASIL;  /* Ignore */
    }
    
    if (arp->opcode == ARP_OP_REPLY) {
        /* Tambah ke cache */
        arp_tambah(NULL, &arp->sender_ip, &arp->sender_mac);
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI TCP, UDP, DNS, DHCP, WIFI (STUB)
 * ===========================================================================
 */

status_t tcp_init(void) { return STATUS_BERHASIL; }
status_t tcp_proses(paket_t *paket) { (void)paket; return STATUS_BERHASIL; }
status_t tcp_kirim_syn(socket_t *sock) { (void)sock; return STATUS_BERHASIL; }
status_t tcp_kirim_ack(socket_t *sock) { (void)sock; return STATUS_BERHASIL; }
status_t tcp_kirim_fin(socket_t *sock) { (void)sock; return STATUS_BERHASIL; }
status_t tcp_kirim_rst(socket_t *sock) { (void)sock; return STATUS_BERHASIL; }

status_t udp_init(void) { return STATUS_BERHASIL; }
status_t udp_proses(paket_t *paket) { (void)paket; return STATUS_BERHASIL; }

status_t dns_init(void) { return STATUS_BERHASIL; }
status_t dns_resolve(const char *hostname, alamat_ipv4_t *ipv4)
{
    if (hostname == NULL || ipv4 == NULL) {
        return STATUS_PARAM_NULL;
    }
    return STATUS_TIDAK_DUKUNG;
}
status_t dns_set_server(tak_bertanda32_t index, alamat_ipv4_t *server)
{
    if (server == NULL || index > 1) {
        return STATUS_PARAM_INVALID;
    }
    kernel_memcpy(&g_dns_servers[index], server, sizeof(alamat_ipv4_t));
    return STATUS_BERHASIL;
}

status_t dhcp_init(void) { return STATUS_BERHASIL; }
status_t dhcp_request(netdev_t *dev, dhcp_config_t *config)
{
    (void)dev; (void)config;
    return STATUS_TIDAK_DUKUNG;
}
status_t dhcp_release(netdev_t *dev) { (void)dev; return STATUS_BERHASIL; }
status_t dhcp_renew(netdev_t *dev, dhcp_config_t *config)
{
    (void)dev; (void)config;
    return STATUS_TIDAK_DUKUNG;
}

status_t wifi_init(void) { return STATUS_BERHASIL; }
tanda32_t wifi_scan(netdev_t *dev, wifi_scan_result_t *results,
                     tak_bertanda32_t max_results)
{
    (void)dev; (void)results; (void)max_results;
    return 0;
}
status_t wifi_connect(netdev_t *dev, wifi_config_t *config)
{
    (void)dev; (void)config;
    return STATUS_TIDAK_DUKUNG;
}
status_t wifi_disconnect(netdev_t *dev) { (void)dev; return STATUS_BERHASIL; }
status_t wifi_status(netdev_t *dev, wifi_status_t *status)
{
    (void)dev; (void)status;
    return STATUS_TIDAK_DUKUNG;
}

/*
 * ===========================================================================
 * FUNGSI SOCKET (STUB)
 * ===========================================================================
 */

tanda32_t socket_buat(tak_bertanda32_t domain, tak_bertanda32_t tipe,
                       tak_bertanda32_t protocol)
{
    (void)domain; (void)tipe; (void)protocol;
    return -1;
}

status_t socket_tutup(tak_bertanda32_t sockfd)
{
    (void)sockfd;
    return STATUS_TIDAK_DUKUNG;
}

status_t socket_ikat(tak_bertanda32_t sockfd, const sockaddr_t *addr,
                      ukuran_t addrlen)
{
    (void)sockfd; (void)addr; (void)addrlen;
    return STATUS_TIDAK_DUKUNG;
}

status_t socket_dengar(tak_bertanda32_t sockfd, tak_bertanda32_t backlog)
{
    (void)sockfd; (void)backlog;
    return STATUS_TIDAK_DUKUNG;
}

tanda32_t socket_terima(tak_bertanda32_t sockfd, sockaddr_t *addr,
                         ukuran_t *addrlen)
{
    (void)sockfd; (void)addr; (void)addrlen;
    return -1;
}

status_t socket_hubung(tak_bertanda32_t sockfd, const sockaddr_t *addr,
                        ukuran_t addrlen)
{
    (void)sockfd; (void)addr; (void)addrlen;
    return STATUS_TIDAK_DUKUNG;
}

tanda64_t socket_kirim(tak_bertanda32_t sockfd, const void *data,
                        ukuran_t len, tak_bertanda32_t flags)
{
    (void)sockfd; (void)data; (void)len; (void)flags;
    return -1;
}

tanda64_t socket_terima_data(tak_bertanda32_t sockfd, void *data,
                              ukuran_t len, tak_bertanda32_t flags)
{
    (void)sockfd; (void)data; (void)len; (void)flags;
    return -1;
}
