/*
 * PIGURA OS - NETDEV.C
 * =====================
 * Implementasi manajemen perangkat jaringan.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi untuk manajemen
 * perangkat jaringan (network device) dalam Pigura OS.
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

/* Pool untuk device statis */
#define NETDEV_POOL_MAKS 16
static netdev_t g_netdev_pool[NETDEV_POOL_MAKS];
static bool_t g_netdev_pool_terpakai[NETDEV_POOL_MAKS];
static tak_bertanda32_t g_netdev_counter = 0;

/* Buffer pool untuk RX/TX */
#define NETDEV_BUFFER_POOL_MAKS 64
static tak_bertanda8_t g_netdev_buffer_pool[NETDEV_BUFFER_POOL_MAKS]
    [PAKET_BUFFER_MAKS];
static bool_t g_netdev_buffer_terpakai[NETDEV_BUFFER_POOL_MAKS];

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * netdev_alokasi - Alokasi struktur netdev dari pool
 *
 * Return: Pointer ke netdev atau NULL
 */
static netdev_t *netdev_alokasi(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < NETDEV_POOL_MAKS; i++) {
        if (!g_netdev_pool_terpakai[i]) {
            g_netdev_pool_terpakai[i] = BENAR;
            return &g_netdev_pool[i];
        }
    }

    return NULL;
}

/*
 * netdev_bebaskan - Bebaskan struktur netdev ke pool
 *
 * Parameter:
 *   dev - Pointer ke device
 */
static void netdev_bebaskan(netdev_t *dev)
{
    tak_bertanda32_t i;

    if (dev == NULL) {
        return;
    }

    for (i = 0; i < NETDEV_POOL_MAKS; i++) {
        if (&g_netdev_pool[i] == dev) {
            g_netdev_pool_terpakai[i] = SALAH;
            return;
        }
    }
}

/*
 * netdev_buffer_alokasi - Alokasi buffer dari pool
 *
 * Return: Pointer ke buffer atau NULL
 */
static tak_bertanda8_t *netdev_buffer_alokasi(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < NETDEV_BUFFER_POOL_MAKS; i++) {
        if (!g_netdev_buffer_terpakai[i]) {
            g_netdev_buffer_terpakai[i] = BENAR;
            return g_netdev_buffer_pool[i];
        }
    }

    return NULL;
}

/*
 * netdev_buffer_bebaskan - Bebaskan buffer ke pool
 *
 * Parameter:
 *   buffer - Pointer ke buffer
 */
static void netdev_buffer_bebaskan(tak_bertanda8_t *buffer)
{
    tak_bertanda32_t i;

    if (buffer == NULL) {
        return;
    }

    for (i = 0; i < NETDEV_BUFFER_POOL_MAKS; i++) {
        if (g_netdev_buffer_pool[i] == buffer) {
            g_netdev_buffer_terpakai[i] = SALAH;
            return;
        }
    }
}

/*
 * netdev_validasi - Validasi device
 *
 * Parameter:
 *   dev - Pointer ke device
 *
 * Return: BENAR jika valid
 */
static bool_t netdev_validasi(netdev_t *dev)
{
    if (dev == NULL) {
        return SALAH;
    }

    if (dev->magic != NETDEV_MAGIC) {
        return SALAH;
    }

    return BENAR;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * netdev_register - Registrasi network device baru
 */
netdev_t *netdev_register(const char *nama, tak_bertanda32_t tipe)
{
    netdev_t *dev;
    ukuran_t nama_len;
    ukuran_t i;

    if (nama == NULL) {
        return NULL;
    }

    /* Alokasi device */
    dev = netdev_alokasi();
    if (dev == NULL) {
        return NULL;
    }

    /* Inisialisasi struktur */
    kernel_memset(dev, 0, sizeof(netdev_t));

    dev->magic = NETDEV_MAGIC;
    dev->id = g_netdev_counter++;

    /* Salin nama dengan aman */
    nama_len = kernel_strlen(nama);
    if (nama_len >= NETDEV_NAMA_PANJANG_MAKS) {
        nama_len = NETDEV_NAMA_PANJANG_MAKS - 1;
    }
    kernel_strncpy(dev->nama, nama, nama_len);
    dev->nama[nama_len] = '\0';

    /* Set tipe dan status */
    dev->tipe = tipe;
    dev->status = NETDEV_STATUS_DOWN;
    dev->flags = 0;

    /* Set MTU default berdasarkan tipe */
    switch (tipe) {
    case NETDEV_TIPE_LOOPBACK:
        dev->mtu = MTU_LOOPBACK;
        break;
    case NETDEV_TIPE_ETHERNET:
    case NETDEV_TIPE_WIFI:
        dev->mtu = MTU_ETHERNET;
        break;
    default:
        dev->mtu = MTU_ETHERNET;
        break;
    }

    /* Inisialisasi statistik */
    dev->rx_packets = 0;
    dev->tx_packets = 0;
    dev->rx_bytes = 0;
    dev->tx_bytes = 0;
    dev->rx_errors = 0;
    dev->tx_errors = 0;
    dev->rx_dropped = 0;
    dev->tx_dropped = 0;

    /* Queue length */
    dev->tx_queue_len = 1000;

    /* Callbacks */
    dev->kirim = NULL;
    dev->terima = NULL;
    dev->ioctl = NULL;
    dev->buka = NULL;
    dev->tutup = NULL;

    /* Private data */
    dev->priv = NULL;
    dev->berikutnya = NULL;

    /* Tambah ke list global */
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

/*
 * netdev_unregister - Hapus registrasi network device
 */
status_t netdev_unregister(netdev_t *dev)
{
    netdev_t *prev;
    netdev_t *curr;

    if (!netdev_validasi(dev)) {
        return STATUS_PARAM_INVALID;
    }

    /* Cari di list */
    prev = NULL;
    curr = g_jaringan_konteks.netdev_list;

    while (curr != NULL) {
        if (curr == dev) {
            /* Panggil callback tutup jika ada */
            if (dev->tutup != NULL) {
                dev->tutup(dev);
            }

            /* Hapus dari list */
            if (prev == NULL) {
                g_jaringan_konteks.netdev_list = curr->berikutnya;
            } else {
                prev->berikutnya = curr->berikutnya;
            }

            /* Update default jika perlu */
            if (g_jaringan_konteks.netdev_default == dev) {
                g_jaringan_konteks.netdev_default =
                    g_jaringan_konteks.netdev_list;
            }

            /* Reset magic dan bebaskan */
            dev->magic = 0;
            netdev_bebaskan(dev);

            g_jaringan_konteks.netdev_count--;

            return STATUS_BERHASIL;
        }

        prev = curr;
        curr = curr->berikutnya;
    }

    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * netdev_cari - Cari device berdasarkan nama
 */
netdev_t *netdev_cari(const char *nama)
{
    netdev_t *dev;

    if (nama == NULL) {
        return NULL;
    }

    dev = g_jaringan_konteks.netdev_list;

    while (dev != NULL) {
        if (kernel_strcmp(dev->nama, nama) == 0) {
            return dev;
        }
        dev = dev->berikutnya;
    }

    return NULL;
}

/*
 * netdev_cari_id - Cari device berdasarkan ID
 */
netdev_t *netdev_cari_id(tak_bertanda32_t id)
{
    netdev_t *dev;

    dev = g_jaringan_konteks.netdev_list;

    while (dev != NULL) {
        if (dev->id == id) {
            return dev;
        }
        dev = dev->berikutnya;
    }

    return NULL;
}

/*
 * netdev_up - Aktifkan network device
 */
status_t netdev_up(netdev_t *dev)
{
    status_t hasil;

    if (!netdev_validasi(dev)) {
        return STATUS_PARAM_INVALID;
    }

    /* Set status UP */
    dev->status = NETDEV_STATUS_UP;
    dev->flags |= NETDEV_FLAG_UP;

    /* Panggil callback buka jika ada */
    if (dev->buka != NULL) {
        hasil = dev->buka(dev);
        if (hasil != STATUS_BERHASIL) {
            dev->status = NETDEV_STATUS_ERROR;
            return hasil;
        }
    }

    /* Set status running jika berhasil */
    dev->status = NETDEV_STATUS_RUNNING;
    dev->flags |= NETDEV_FLAG_RUNNING;

    return STATUS_BERHASIL;
}

/*
 * netdev_down - Nonaktifkan network device
 */
status_t netdev_down(netdev_t *dev)
{
    if (!netdev_validasi(dev)) {
        return STATUS_PARAM_INVALID;
    }

    /* Clear flags */
    dev->status = NETDEV_STATUS_DOWN;
    dev->flags &= ~(NETDEV_FLAG_UP | NETDEV_FLAG_RUNNING);

    return STATUS_BERHASIL;
}

/*
 * netdev_set_ipv4 - Set alamat IPv4
 */
status_t netdev_set_ipv4(netdev_t *dev, alamat_ipv4_t *addr,
                         alamat_ipv4_t *netmask)
{
    tak_bertanda32_t i;

    if (!netdev_validasi(dev)) {
        return STATUS_PARAM_INVALID;
    }

    if (addr != NULL) {
        kernel_memcpy(&dev->ipv4, addr, sizeof(alamat_ipv4_t));
    }

    if (netmask != NULL) {
        kernel_memcpy(&dev->ipv4_netmask, netmask, sizeof(alamat_ipv4_t));

        /* Hitung broadcast address */
        if (addr != NULL) {
            for (i = 0; i < NETDEV_IPV4_LEN; i++) {
                dev->ipv4_broadcast.byte[i] =
                    addr->byte[i] | (~netmask->byte[i]);
            }
        }
    }

    return STATUS_BERHASIL;
}

/*
 * netdev_set_gateway - Set gateway default
 */
status_t netdev_set_gateway(netdev_t *dev, alamat_ipv4_t *gateway)
{
    if (!netdev_validasi(dev)) {
        return STATUS_PARAM_INVALID;
    }

    if (gateway != NULL) {
        kernel_memcpy(&dev->ipv4_gateway, gateway, sizeof(alamat_ipv4_t));
    }

    return STATUS_BERHASIL;
}

/*
 * netdev_set_mac - Set alamat MAC
 */
status_t netdev_set_mac(netdev_t *dev, alamat_mac_t *mac)
{
    if (!netdev_validasi(dev)) {
        return STATUS_PARAM_INVALID;
    }

    if (mac != NULL) {
        kernel_memcpy(&dev->mac, mac, sizeof(alamat_mac_t));
    }

    return STATUS_BERHASIL;
}

/*
 * netdev_kirim - Kirim paket melalui device
 */
status_t netdev_kirim(netdev_t *dev, const void *data, ukuran_t len)
{
    status_t hasil;

    if (!netdev_validasi(dev)) {
        return STATUS_PARAM_INVALID;
    }

    if (data == NULL || len == 0) {
        return STATUS_PARAM_INVALID;
    }

    /* Cek apakah device aktif */
    if (!(dev->flags & NETDEV_FLAG_UP)) {
        dev->tx_errors++;
        return NET_ERR_IFACE_DOWN;
    }

    /* Cek MTU */
    if (len > dev->mtu) {
        dev->tx_dropped++;
        return STATUS_PARAM_UKURAN;
    }

    /* Panggil callback kirim */
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

    dev->tx_errors++;
    return NET_ERR_UNKNOWN;
}

/*
 * netdev_terima - Terima paket dari device
 */
status_t netdev_terima(netdev_t *dev, void *data, ukuran_t *len)
{
    status_t hasil;

    if (!netdev_validasi(dev)) {
        return STATUS_PARAM_INVALID;
    }

    if (data == NULL || len == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Cek apakah device aktif */
    if (!(dev->flags & NETDEV_FLAG_UP)) {
        dev->rx_errors++;
        return NET_ERR_IFACE_DOWN;
    }

    /* Panggil callback terima */
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

    dev->rx_errors++;
    return NET_ERR_UNKNOWN;
}

/*
 * netdev_ioctl - IOCTL untuk device
 */
status_t netdev_ioctl(netdev_t *dev, tak_bertanda32_t cmd, void *arg)
{
    if (!netdev_validasi(dev)) {
        return STATUS_PARAM_INVALID;
    }

    if (dev->ioctl != NULL) {
        return dev->ioctl(dev, cmd, arg);
    }

    return STATUS_TIDAK_DUKUNG;
}

/*
 * netdev_get_default - Dapatkan device default
 */
netdev_t *netdev_get_default(void)
{
    return g_jaringan_konteks.netdev_default;
}

/*
 * netdev_set_default - Set device default
 */
status_t netdev_set_default(netdev_t *dev)
{
    if (!netdev_validasi(dev)) {
        return STATUS_PARAM_INVALID;
    }

    g_jaringan_konteks.netdev_default = dev;

    return STATUS_BERHASIL;
}

/*
 * netdev_get_stats - Dapatkan statistik device
 */
status_t netdev_get_stats(netdev_t *dev,
                          tak_bertanda64_t *rx_packets,
                          tak_bertanda64_t *tx_packets,
                          tak_bertanda64_t *rx_bytes,
                          tak_bertanda64_t *tx_bytes)
{
    if (!netdev_validasi(dev)) {
        return STATUS_PARAM_INVALID;
    }

    if (rx_packets != NULL) {
        *rx_packets = dev->rx_packets;
    }

    if (tx_packets != NULL) {
        *tx_packets = dev->tx_packets;
    }

    if (rx_bytes != NULL) {
        *rx_bytes = dev->rx_bytes;
    }

    if (tx_bytes != NULL) {
        *tx_bytes = dev->tx_bytes;
    }

    return STATUS_BERHASIL;
}

/*
 * netdev_reset_stats - Reset statistik device
 */
status_t netdev_reset_stats(netdev_t *dev)
{
    if (!netdev_validasi(dev)) {
        return STATUS_PARAM_INVALID;
    }

    dev->rx_packets = 0;
    dev->tx_packets = 0;
    dev->rx_bytes = 0;
    dev->tx_bytes = 0;
    dev->rx_errors = 0;
    dev->tx_errors = 0;
    dev->rx_dropped = 0;
    dev->tx_dropped = 0;

    return STATUS_BERHASIL;
}

/*
 * netdev_generate_mac - Generate MAC address acak
 */
status_t netdev_generate_mac(alamat_mac_t *mac)
{
    static tak_bertanda32_t counter = 0;

    if (mac == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Generate MAC dengan format lokal */
    mac->byte[0] = 0x02; /* Locally administered */
    mac->byte[1] = 0x00;
    mac->byte[2] = 0x00;

    /* Gunakan counter untuk uniqueness */
    mac->byte[3] = (tak_bertanda8_t)((counter >> 16) & 0xFF);
    mac->byte[4] = (tak_bertanda8_t)((counter >> 8) & 0xFF);
    mac->byte[5] = (tak_bertanda8_t)(counter & 0xFF);

    counter++;

    return STATUS_BERHASIL;
}

/*
 * netdev_is_loopback - Cek apakah device adalah loopback
 */
bool_t netdev_is_loopback(netdev_t *dev)
{
    if (!netdev_validasi(dev)) {
        return SALAH;
    }

    return (dev->tipe == NETDEV_TIPE_LOOPBACK) ? BENAR : SALAH;
}

/*
 * netdev_is_wireless - Cek apakah device adalah wireless
 */
bool_t netdev_is_wireless(netdev_t *dev)
{
    if (!netdev_validasi(dev)) {
        return SALAH;
    }

    return (dev->tipe == NETDEV_TIPE_WIFI) ? BENAR : SALAH;
}

/*
 * netdev_is_up - Cek apakah device aktif
 */
bool_t netdev_is_up(netdev_t *dev)
{
    if (!netdev_validasi(dev)) {
        return SALAH;
    }

    return (dev->flags & NETDEV_FLAG_UP) ? BENAR : SALAH;
}

/*
 * netdev_is_running - Cek apakah device sedang berjalan
 */
bool_t netdev_is_running(netdev_t *dev)
{
    if (!netdev_validasi(dev)) {
        return SALAH;
    }

    return (dev->flags & NETDEV_FLAG_RUNNING) ? BENAR : SALAH;
}

/*
 * netdev_iterate - Iterasi semua device
 */
netdev_t *netdev_iterate(netdev_t *prev)
{
    if (prev == NULL) {
        return g_jaringan_konteks.netdev_list;
    }

    if (!netdev_validasi(prev)) {
        return NULL;
    }

    return prev->berikutnya;
}

/*
 * netdev_count - Hitung jumlah device
 */
tak_bertanda32_t netdev_count(void)
{
    return g_jaringan_konteks.netdev_count;
}

/*
 * netdev_flush_tx - Flush TX queue
 */
status_t netdev_flush_tx(netdev_t *dev)
{
    if (!netdev_validasi(dev)) {
        return STATUS_PARAM_INVALID;
    }

    /* Implementasi sederhana, hanya reset counters */
    /* Implementasi lengkap memerlukan queue management */

    return STATUS_BERHASIL;
}

/*
 * netdev_set_promisc - Set mode promiscuous
 */
status_t netdev_set_promisc(netdev_t *dev, bool_t enable)
{
    if (!netdev_validasi(dev)) {
        return STATUS_PARAM_INVALID;
    }

    if (enable) {
        dev->flags |= NETDEV_FLAG_PROMISC;
    } else {
        dev->flags &= ~NETDEV_FLAG_PROMISC;
    }

    return STATUS_BERHASIL;
}

/*
 * netdev_set_multicast - Set mode multicast
 */
status_t netdev_set_multicast(netdev_t *dev, bool_t enable)
{
    if (!netdev_validasi(dev)) {
        return STATUS_PARAM_INVALID;
    }

    if (enable) {
        dev->flags |= NETDEV_FLAG_MULTICAST;
    } else {
        dev->flags &= ~NETDEV_FLAG_MULTICAST;
    }

    return STATUS_BERHASIL;
}
