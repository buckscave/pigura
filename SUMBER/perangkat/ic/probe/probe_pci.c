/*
 * PIGURA OS - PROBE/PROBE_PCI.C
 * ==============================
 * PCI bus probe untuk IC Detection.
 *
 * Berkas ini mengimplementasikan probing pada bus PCI/PCIe
 * untuk mendeteksi perangkat yang terhubung.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../ic.h"

/*
 * ===========================================================================
 * KONSTANTA PCI
 * ===========================================================================
 */

/* PCI configuration space ports */
#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC

/* PCI configuration space offsets */
#define PCI_OFFSET_VENDOR   0x00
#define PCI_OFFSET_DEVICE   0x02
#define PCI_OFFSET_COMMAND  0x04
#define PCI_OFFSET_STATUS   0x06
#define PCI_OFFSET_REVISION 0x08
#define PCI_OFFSET_CLASS    0x0A
#define PCI_OFFSET_BAR0     0x10
#define PCI_OFFSET_BAR1     0x14
#define PCI_OFFSET_BAR2     0x18
#define PCI_OFFSET_BAR3     0x1C
#define PCI_OFFSET_BAR4     0x20
#define PCI_OFFSET_BAR5     0x24
#define PCI_OFFSET_IRQ      0x3C

/* PCI vendor ID special values */
#define PCI_VENDOR_INVALID  0xFFFF
#define PCI_VENDOR_NONE     0x0000

/* PCI command bits */
#define PCI_COMMAND_IO      0x01
#define PCI_COMMAND_MEM     0x02
#define PCI_COMMAND_MASTER  0x04

/* Maximum bus/device/function */
#define PCI_MAX_BUS         256
#define PCI_MAX_DEVICE      32
#define PCI_MAX_FUNCTION    8

/*
 * ===========================================================================
 * FUNGSI I/O PCI
 * ===========================================================================
 */

/*
 * pci_baca_konfigurasi32 - Baca 32-bit dari PCI config space
 */
static tak_bertanda32_t pci_baca_konfigurasi32(tak_bertanda8_t bus,
                                               tak_bertanda8_t dev,
                                               tak_bertanda8_t func,
                                               tak_bertanda8_t offset)
{
    tak_bertanda32_t alamat;
    tak_bertanda32_t nilai;
    
    /* Format alamat PCI */
    alamat = ((tak_bertanda32_t)bus << 16) |
             ((tak_bertanda32_t)dev << 11) |
             ((tak_bertanda32_t)func << 8) |
             ((tak_bertanda32_t)offset & 0xFC) |
             0x80000000UL;
    
    /* Tulis alamat ke port */
    /*outl(PCI_CONFIG_ADDRESS, alamat);*/
    
    /* Baca data dari port */
    /*nilai = inl(PCI_CONFIG_DATA);*/
    nilai = 0; /* Placeholder */
    
    return nilai;
}

/*
 * pci_baca_konfigurasi16 - Baca 16-bit dari PCI config space
 */
static tak_bertanda16_t pci_baca_konfigurasi16(tak_bertanda8_t bus,
                                               tak_bertanda8_t dev,
                                               tak_bertanda8_t func,
                                               tak_bertanda8_t offset)
{
    tak_bertanda32_t nilai;
    tak_bertanda16_t hasil;
    
    nilai = pci_baca_konfigurasi32(bus, dev, func, offset);
    
    /* Extract 16-bit value berdasarkan offset */
    if (offset & 0x02) {
        hasil = (tak_bertanda16_t)((nilai >> 16) & 0xFFFF);
    } else {
        hasil = (tak_bertanda16_t)(nilai & 0xFFFF);
    }
    
    return hasil;
}

/*
 * pci_baca_konfigurasi8 - Baca 8-bit dari PCI config space
 */
static tak_bertanda8_t pci_baca_konfigurasi8(tak_bertanda8_t bus,
                                             tak_bertanda8_t dev,
                                             tak_bertanda8_t func,
                                             tak_bertanda8_t offset)
{
    tak_bertanda32_t nilai;
    tak_bertanda8_t hasil;
    tak_bertanda32_t shift;
    
    nilai = pci_baca_konfigurasi32(bus, dev, func, offset);
    shift = (tak_bertanda32_t)((offset & 0x03) * 8);
    hasil = (tak_bertanda8_t)((nilai >> shift) & 0xFF);
    
    return hasil;
}

/*
 * pci_tulis_konfigurasi32 - Tulis 32-bit ke PCI config space
 */
static void pci_tulis_konfigurasi32(tak_bertanda8_t bus,
                                     tak_bertanda8_t dev,
                                     tak_bertanda8_t func,
                                     tak_bertanda8_t offset,
                                     tak_bertanda32_t nilai)
{
    tak_bertanda32_t alamat;
    
    /* Format alamat PCI */
    alamat = ((tak_bertanda32_t)bus << 16) |
             ((tak_bertanda32_t)dev << 11) |
             ((tak_bertanda32_t)func << 8) |
             ((tak_bertanda32_t)offset & 0xFC) |
             0x80000000UL;
    
    /* Tulis alamat ke port */
    /*outl(PCI_CONFIG_ADDRESS, alamat);*/
    
    /* Tulis data ke port */
    /*outl(PCI_CONFIG_DATA, nilai);*/
}

/*
 * ===========================================================================
 * FUNGSI DETEKSI PCI
 * ===========================================================================
 */

/*
 * pci_cek_perangkat - Cek apakah perangkat ada
 */
static bool_t pci_cek_perangkat(tak_bertanda8_t bus,
                                 tak_bertanda8_t dev,
                                 tak_bertanda8_t func)
{
    tak_bertanda16_t vendor_id;
    
    vendor_id = pci_baca_konfigurasi16(bus, dev, func, PCI_OFFSET_VENDOR);
    
    if (vendor_id == PCI_VENDOR_INVALID || vendor_id == PCI_VENDOR_NONE) {
        return SALAH;
    }
    
    return BENAR;
}

/*
 * pci_baca_info_perangkat - Baca informasi lengkap perangkat
 */
static void pci_baca_info_perangkat(tak_bertanda8_t bus,
                                     tak_bertanda8_t dev,
                                     tak_bertanda8_t func,
                                     tak_bertanda16_t *vendor_id,
                                     tak_bertanda16_t *device_id,
                                     tak_bertanda32_t *class_code,
                                     tak_bertanda8_t *revision)
{
    tak_bertanda32_t nilai;
    
    /* Baca Vendor ID dan Device ID */
    *vendor_id = pci_baca_konfigurasi16(bus, dev, func, PCI_OFFSET_VENDOR);
    *device_id = pci_baca_konfigurasi16(bus, dev, func, PCI_OFFSET_DEVICE);
    
    /* Baca Class Code dan Revision */
    nilai = pci_baca_konfigurasi32(bus, dev, func, PCI_OFFSET_REVISION);
    *revision = (tak_bertanda8_t)(nilai & 0xFF);
    *class_code = (nilai >> 8) & 0xFFFFFF;
}

/*
 * pci_baca_bar - Baca Base Address Register
 */
static alamat_fisik_t pci_baca_bar(tak_bertanda8_t bus,
                                    tak_bertanda8_t dev,
                                    tak_bertanda8_t func,
                                    tak_bertanda8_t bar_index)
{
    tak_bertanda8_t offset;
    tak_bertanda32_t nilai;
    
    if (bar_index >= 6) {
        return 0;
    }
    
    offset = PCI_OFFSET_BAR0 + (bar_index * 4);
    nilai = pci_baca_konfigurasi32(bus, dev, func, offset);
    
    /* Mask flag bits */
    if (nilai & 0x01) {
        /* I/O space */
        return (alamat_fisik_t)(nilai & 0xFFFFFFFC);
    } else {
        /* Memory space */
        return (alamat_fisik_t)(nilai & 0xFFFFFFF0);
    }
}

/*
 * pci_baca_bar_ukuran - Baca ukuran BAR
 */
static tak_bertanda32_t pci_baca_bar_ukuran(tak_bertanda8_t bus,
                                             tak_bertanda8_t dev,
                                             tak_bertanda8_t func,
                                             tak_bertanda8_t bar_index)
{
    tak_bertanda8_t offset;
    tak_bertanda32_t nilai_asli;
    tak_bertanda32_t nilai_ukuran;
    tak_bertanda32_t ukuran;
    
    if (bar_index >= 6) {
        return 0;
    }
    
    offset = PCI_OFFSET_BAR0 + (bar_index * 4);
    
    /* Simpan nilai asli */
    nilai_asli = pci_baca_konfigurasi32(bus, dev, func, offset);
    
    /* Tulis 0xFFFFFFFF untuk mendapatkan ukuran */
    pci_tulis_konfigurasi32(bus, dev, func, offset, 0xFFFFFFFF);
    
    /* Baca kembali */
    nilai_ukuran = pci_baca_konfigurasi32(bus, dev, func, offset);
    
    /* Kembalikan nilai asli */
    pci_tulis_konfigurasi32(bus, dev, func, offset, nilai_asli);
    
    /* Hitung ukuran */
    if (nilai_ukuran == 0) {
        return 0;
    }
    
    /* Mask flag bits dan hitung ukuran */
    nilai_ukuran &= 0xFFFFFFF0;
    ukuran = (~(nilai_ukuran - 1)) & nilai_ukuran;
    
    return ukuran;
}

/*
 * pci_baca_irq - Baca IRQ line
 */
static tak_bertanda32_t pci_baca_irq(tak_bertanda8_t bus,
                                      tak_bertanda8_t dev,
                                      tak_bertanda8_t func)
{
    tak_bertanda32_t nilai;
    
    nilai = pci_baca_konfigurasi32(bus, dev, func, PCI_OFFSET_IRQ);
    
    return nilai & 0xFF;
}

/*
 * pci_jenis_header - Dapatkan jenis header
 */
static tak_bertanda8_t pci_jenis_header(tak_bertanda8_t bus,
                                         tak_bertanda8_t dev,
                                         tak_bertanda8_t func)
{
    tak_bertanda32_t nilai;
    
    nilai = pci_baca_konfigurasi32(bus, dev, func, 0x0C);
    
    return (tak_bertanda8_t)((nilai >> 16) & 0x7F);
}

/*
 * ===========================================================================
 * FUNGSI UTAMA PROBE PCI
 * ===========================================================================
 */

/*
 * ic_probe_pci_fungsi - Probe satu fungsi PCI
 */
static status_t ic_probe_pci_fungsi(tak_bertanda8_t bus,
                                     tak_bertanda8_t dev,
                                     tak_bertanda8_t func,
                                     tanda32_t *jumlah_ditemukan)
{
    ic_perangkat_t *perangkat;
    tak_bertanda16_t vendor_id;
    tak_bertanda16_t device_id;
    tak_bertanda32_t class_code;
    tak_bertanda8_t revision;
    tak_bertanda8_t i;
    alamat_fisik_t bar_alamat;
    tak_bertanda32_t bar_ukuran;
    tak_bertanda32_t irq;
    
    /* Cek apakah perangkat ada */
    if (!pci_cek_perangkat(bus, dev, func)) {
        return STATUS_BERHASIL;
    }
    
    /* Baca informasi perangkat */
    pci_baca_info_perangkat(bus, dev, func, &vendor_id, &device_id,
                            &class_code, &revision);
    
    /* Buat entri perangkat baru */
    perangkat = ic_probe_tambah_perangkat(IC_BUS_PCI, bus, dev, func);
    if (perangkat == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    /* Set informasi perangkat */
    ic_probe_set_info(perangkat, vendor_id, device_id, class_code, revision);
    
    /* Baca BARs */
    for (i = 0; i < 6; i++) {
        bar_alamat = pci_baca_bar(bus, dev, func, i);
        bar_ukuran = pci_baca_bar_ukuran(bus, dev, func, i);
        
        if (bar_alamat != 0 || bar_ukuran != 0) {
            ic_probe_set_bar(perangkat, i, bar_alamat, bar_ukuran);
        }
    }
    
    /* Baca IRQ */
    irq = pci_baca_irq(bus, dev, func);
    if (irq != 0 && irq != 0xFF) {
        ic_probe_set_irq(perangkat, irq);
    }
    
    /* Tentukan bus tipe (PCI atau PCIe) */
    if (class_code == 0x060400) {
        /* PCI-to-PCI Bridge */
        perangkat->bus = IC_BUS_PCIE;
    } else {
        perangkat->bus = IC_BUS_PCI;
    }
    
    (*jumlah_ditemukan)++;
    
    return STATUS_BERHASIL;
}

/*
 * ic_probe_pci_device - Probe satu device PCI
 */
static status_t ic_probe_pci_device(tak_bertanda8_t bus,
                                     tak_bertanda8_t dev,
                                     tanda32_t *jumlah_ditemukan)
{
    tak_bertanda8_t func;
    tak_bertanda8_t jenis_header;
    tak_bertanda16_t vendor_id;
    status_t hasil;
    
    /* Cek fungsi 0 */
    vendor_id = pci_baca_konfigurasi16(bus, dev, 0, PCI_OFFSET_VENDOR);
    if (vendor_id == PCI_VENDOR_INVALID || vendor_id == 0) {
        return STATUS_BERHASIL;
    }
    
    /* Probe fungsi 0 */
    hasil = ic_probe_pci_fungsi(bus, dev, 0, jumlah_ditemukan);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Cek jenis header untuk multi-fungsi */
    jenis_header = pci_jenis_header(bus, dev, 0);
    
    /* Jika multi-fungsi, probe fungsi lainnya */
    if (jenis_header == 0 && (vendor_id & 0x0080)) {
        /* Multi-fungsi device */
        for (func = 1; func < PCI_MAX_FUNCTION; func++) {
            hasil = ic_probe_pci_fungsi(bus, dev, func, jumlah_ditemukan);
            if (hasil != STATUS_BERHASIL) {
                /* Lanjutkan meskipun ada error */
            }
        }
    }
    
    return STATUS_BERHASIL;
}

/*
 * ic_probe_pci_bus - Probe satu bus PCI
 */
static status_t ic_probe_pci_bus(tak_bertanda8_t bus,
                                  tanda32_t *jumlah_ditemukan)
{
    tak_bertanda8_t dev;
    status_t hasil;
    
    for (dev = 0; dev < PCI_MAX_DEVICE; dev++) {
        hasil = ic_probe_pci_device(bus, dev, jumlah_ditemukan);
        if (hasil != STATUS_BERHASIL) {
            /* Lanjutkan meskipun ada error */
        }
    }
    
    return STATUS_BERHASIL;
}

/*
 * ic_probe_pci - Scan seluruh bus PCI
 */
tanda32_t ic_probe_pci(void)
{
    tanda32_t jumlah_ditemukan = 0;
    tak_bertanda16_t bus;
    status_t hasil;
    
    if (!g_probe_diinisialisasi) {
        return -1;
    }
    
    /* Scan semua bus */
    for (bus = 0; bus < PCI_MAX_BUS; bus++) {
        hasil = ic_probe_pci_bus((tak_bertanda8_t)bus, &jumlah_ditemukan);
        if (hasil != STATUS_BERHASIL) {
            /* Lanjutkan meskipun ada error */
        }
    }
    
    return jumlah_ditemukan;
}
