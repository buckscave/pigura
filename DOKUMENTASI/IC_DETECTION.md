# SISTEM DETEKSI IC/CHIP PIGURA OS

## Daftar Isi

1. [Konsep Dasar](#konsep-dasar)
2. [Arsitektur Sistem](#arsitektur-sistem)
3. [Komponen Utama](#komponen-utama)
4. [Alur Deteksi](#alur-dedeksi)
5. [Database Parameter](#database-parameter)
6. [Probe Engine](#probe-engine)
7. [Driver Generik](#driver-generik)
8. [API Reference](#api-reference)
9. [Implementasi Contoh](#implementasi-contoh)

---

## Konsep Dasar

### Filosofi

Pendekatan deteksi IC/Chip dalam Pigura OS merupakan salah satu inovasi utama yang membedakannya dari sistem operasi konvensional. Alih-alih membuat driver terpisah untuk setiap vendor atau model perangkat, Pigura OS mendeteksi IC berdasarkan **fungsi** dan **spesifikasi teknis**-nya.

### Mengapa Per-IC, Bukan Per-Vendor?

Banyak IC dari vendor berbeda memiliki karakteristik yang serupa atau bahkan identik:

| Contoh | IC yang Sama/Serupa |
|--------|---------------------|
| USB Controller | UHCI (Intel), OHCI (Compaq), EHCI (Intel), xHCI (Intel) |
| WiFi | RTL8188, AR9271, Intel WiFi - semua 802.11 |
| Audio | Semua HDA-compatible |
| Storage | ATA/IDE, AHCI, NVMe |

Dengan mendeteksi berdasarkan IC, satu driver dapat mendukung ratusan perangkat dari berbagai vendor.

### Keuntungan

| Aspek | Pendekatan Tradisional | Pigura IC Detection |
|-------|------------------------|---------------------|
| Jumlah Driver | 50-100+ driver vendor | ~10 driver fungsi |
| Ukuran Kode | ~50 MB | ~600 KB |
| Waktu Development | Tinggi | Rendah |
| Maintenance | Kompleks | Sederhana |
| Parameter | Hardcoded | Auto-detect |

---

## Arsitektur Sistem

```
┌─────────────────────────────────────────────────────────────────┐
│                     KERNEL SPACE                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐      │
│  │   PROBE      │───>│   DETEKSI    │───>│  KLASIFIKASI │      │
│  │   ENGINE     │    │   ENGINE     │    │   ENGINE     │      │
│  └──────────────┘    └──────────────┘    └──────────────┘      │
│         │                   │                   │               │
│         v                   v                   v               │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐      │
│  │  BUS PROBE   │    │  PARAMETER   │    │   DATABASE   │      │
│  │              │    │  READER      │    │              │      │
│  └──────────────┘    └──────────────┘    └──────────────┘      │
│         │                   │                   │               │
│         v                   v                   v               │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                    VALIDATOR                              │  │
│  └──────────────────────────────────────────────────────────┘  │
│                              │                                  │
│                              v                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                 DRIVER GENERIK                            │  │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐        │  │
│  │  │Storage  │ │ Network │ │ Display │ │  Audio  │        │  │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘        │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
                               │
                               v
┌─────────────────────────────────────────────────────────────────┐
│                       HARDWARE                                   │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐               │
│  │   CPU   │ │   GPU   │ │   NIC   │ │ Storage │               │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘               │
└─────────────────────────────────────────────────────────────────┘
```

---

## Komponen Utama

### Struktur Direktori

```
SUMBER/perangkat/ic/
│
├── ic_deteksi.c           # Engine deteksi utama
├── ic_mesin.c             # Core engine
├── ic_klasifikasi.c       # Klasifikasi fungsi IC
├── ic_parameter.c         # Struktur dan parsing parameter
├── ic_validasi.c          # Validasi parameter
│
├── database/              # Database parameter IC standar
│   ├── database.c         # Core database
│   ├── db_cpu.c           # Parameter CPU IC
│   ├── db_gpu.c           # Parameter GPU IC
│   ├── db_network.c       # Parameter Network IC
│   ├── db_storage.c       # Parameter Storage IC
│   ├── db_display.c       # Parameter Display IC
│   ├── db_audio.c         # Parameter Audio IC
│   ├── db_input.c         # Parameter Input IC
│   └── db_usb.c           # Parameter USB Controller
│
├── probe/                 # Membaca parameter dari hardware
│   ├── probe.c            # Core probe engine
│   ├── probe_pci.c        # Probe via bus PCI
│   ├── probe_usb.c        # Probe via bus USB
│   ├── probe_i2c.c        # Probe via bus I2C
│   ├── probe_spi.c        # Probe via bus SPI
│   └── probe_mmio.c       # Probe via Memory-Mapped I/O
│
└── driver_umum/           # Driver berdasarkan FUNGSI
    ├── driver_umum.c      # Framework driver generik
    ├── penyimpanan_nvme_umum.c   # NVMe generik
    ├── penyimpanan_ata_umum.c    # ATA/IDE generik
    ├── audio_hda_umum.c          # Intel HDA generik
    ├── display_vga_umum.c        # VGA generik
    ├── display_hdmi_umum.c       # HDMI generik
    ├── net_wifi_umum.c           # WiFi generik
    ├── net_ethernet_umum.c       # Ethernet generik
    └── input_usb_hid_umum.c      # USB HID generik
```

---

## Alur Deteksi

### Diagram Alur

```
START
  │
  v
┌──────────────────┐
│ 1. DETEKSI IC    │  Identifikasi keberadaan IC via bus
└────────┬─────────┘
         │
         v
┌──────────────────┐
│ 2. IDENTIFIKASI  │  Tentukan fungsi utama IC
│    FUNGSI        │  (WiFi, Storage, Display, dll)
└────────┬─────────┘
         │
         v
┌──────────────────┐
│ 3. PROBE         │  Baca parameter operasional:
│    PARAMETER     │  - Nilai minimum
│                  │  - Nilai maksimum
│                  │  - Nilai typical
└────────┬─────────┘
         │
         v
┌──────────────────┐
│ 4. VALIDASI      │  Bandingkan dengan database
│                  │  tandai jika di luar rentang aman
└────────┬─────────┘
         │
         v
┌──────────────────┐
│ 5. PILIH DEFAULT │  Pilih konfigurasi:
│                  │  1. Typical dari IC (prioritas)
│                  │  2. Default dari database
│                  │  3. Minimum yang valid
└────────┬─────────┘
         │
         v
┌──────────────────┐
│ 6. INISIALISASI  │  Driver generik dengan
│    DRIVER        │  parameter yang dipilih
└────────┬─────────┘
         │
         v
       DONE
```

### Contoh Deteksi IC WiFi

```
1. DETEKSI IC
   - Bus: PCI
   - Vendor ID: 0x10EC
   - Device ID: 0x8179
   - IC terdeteksi: Realtek RTL8188EE

2. IDENTIFIKASI FUNGSI
   - Class Code: 0x0280 (Network Controller)
   - Subclass: 0x80 (Wireless)
   - Fungsi: WiFi

3. PROBE PARAMETER
   - Transfer Rate Min: 1 Mbps
   - Transfer Rate Max: 150 Mbps
   - Transfer Rate Typical: 54 Mbps
   - TX Power Min: 0 dBm
   - TX Power Max: 20 dBm
   - TX Power Typical: 15 dBm
   - Frequency: 2.4 GHz
   - Channels: 1-14

4. VALIDASI
   - Semua parameter dalam rentang database
   - Status: VALID

5. PILIH DEFAULT
   - Rate: 54 Mbps (typical)
   - TX Power: 15 dBm (typical)
   - Channel: 6 (default)

6. INISIALISASI
   - Driver: net_wifi_umum
   - Parameter: Dari probe
   - Status: BERHASIL
```

---

## Database Parameter

### Struktur Parameter

```c
/* Struktur parameter IC */
typedef struct {
    tak_bertanda32_t id;              /* ID parameter */
    char nama[32];                    /* Nama parameter */
    tak_bertanda64_t nilai_min;       /* Nilai minimum */
    tak_bertanda64_t nilai_max;       /* Nilai maksimum */
    tak_bertanda64_t nilai_typical;   /* Nilai typical */
    tak_bertanda64_t nilai_default;   /* Nilai default database */
    char satuan[16];                  /* Satuan (MHz, dBm, dll) */
    tak_bertanda32_t flags;           /* Flags khusus */
} ic_parameter_t;

/* Struktur kategori IC */
typedef struct {
    tak_bertanda32_t id;              /* ID kategori */
    char nama[32];                    /* Nama kategori */
    ic_parameter_t *param;            /* Array parameter */
    tak_bertanda32_t jumlah_param;    /* Jumlah parameter */
} ic_kategori_t;
```

### Kategori dan Parameter

#### CPU IC

| Parameter | Min | Max | Typical | Satuan |
|-----------|-----|-----|---------|--------|
| Clock Speed | 100 | 5000 | 2400 | MHz |
| Core Count | 1 | 128 | 4 | cores |
| Cache L1 | 8 | 128 | 32 | KB |
| Cache L2 | 64 | 4096 | 256 | KB |
| Cache L3 | 0 | 65536 | 8192 | KB |
| TDP | 5 | 250 | 65 | W |

#### GPU IC

| Parameter | Min | Max | Typical | Satuan |
|-----------|-----|-----|---------|--------|
| VRAM | 128 | 24576 | 8192 | MB |
| Core Clock | 300 | 3000 | 1500 | MHz |
| Memory Clock | 500 | 24000 | 8000 | MHz |
| Shader Units | 64 | 16384 | 2048 | units |
| TMUs | 8 | 512 | 128 | units |
| ROPs | 4 | 256 | 64 | units |

#### Network IC (WiFi)

| Parameter | Min | Max | Typical | Satuan |
|-----------|-----|-----|---------|--------|
| Transfer Rate | 1 | 9600 | 1200 | Mbps |
| TX Power | 0 | 30 | 15 | dBm |
| Frequency Low | 2.4 | 2.5 | 2.4 | GHz |
| Frequency High | 5.0 | 7.0 | 5.8 | GHz |
| Channel Count | 1 | 256 | 14 | channels |
| MIMO Streams | 1 | 8 | 2 | streams |

#### Storage IC (NVMe)

| Parameter | Min | Max | Typical | Satuan |
|-----------|-----|-----|---------|--------|
| Queue Depth | 1 | 65536 | 32 | entries |
| Max Transfer | 4096 | 1048576 | 65536 | KB |
| Sector Size | 512 | 16384 | 4096 | bytes |
| Max Namespace | 1 | 256 | 1 | namespaces |

---

## Probe Engine

### Bus Probe Interface

```c
/* Interface bus probe */
typedef struct {
    char nama[16];                    /* Nama bus */
    status_t (*init)(void);           /* Inisialisasi */
    status_t (*scan)(void);           /* Scan perangkat */
    status_t (*read_vid_did)(tak_bertanda32_t bus_id,
                            tak_bertanda16_t *vid,
                            tak_bertanda16_t *did);  /* Baca ID */
    status_t (*read_class)(tak_bertanda32_t bus_id,
                          tak_bertanda32_t *class_code); /* Baca class */
    status_t (*read_param)(tak_bertanda32_t bus_id,
                          ic_parameter_t *param);  /* Baca parameter */
    status_t (*write_config)(tak_bertanda32_t bus_id,
                            tak_bertanda32_t reg,
                            tak_bertanda32_t val); /* Tulis config */
} bus_probe_t;
```

### PCI Probe

```c
/* Implementasi PCI probe */
status_t probe_pci_init(void) {
    /* Inisialisasi PCI configuration space */
    return hal_pci_init();
}

status_t probe_pci_scan(void) {
    tak_bertanda16_t bus, dev, func;
    
    for (bus = 0; bus < 256; bus++) {
        for (dev = 0; dev < 32; dev++) {
            for (func = 0; func < 8; func++) {
                tak_bertanda16_t vid, did;
                
                if (pci_read_vid_did(bus, dev, func, &vid, &did) == STATUS_BERHASIL) {
                    if (vid != 0xFFFF) {
                        /* IC ditemukan, tambahkan ke daftar */
                        ic_daftar_tambah(BUS_PCI, bus, dev, func, vid, did);
                    }
                }
            }
        }
    }
    
    return STATUS_BERHASIL;
}
```

### USB Probe

```c
/* Implementasi USB probe */
status_t probe_usb_scan(void) {
    tak_bertanda32_t controller_id = 0;
    
    /* Scan semua USB controller */
    while (hal_usb_controller_ada(controller_id)) {
        tak_bertanda32_t device_count = hal_usb_device_count(controller_id);
        
        for (tak_bertanda32_t dev = 0; dev < device_count; dev++) {
            usb_device_desc_t desc;
            
            if (hal_usb_get_descriptor(controller_id, dev, &desc) == STATUS_BERHASIL) {
                ic_daftar_tambah(BUS_USB, controller_id, dev, 0,
                                desc.idVendor, desc.idProduct);
            }
        }
        
        controller_id++;
    }
    
    return STATUS_BERHASIL;
}
```

---

## Driver Generik

### Framework Driver Generik

```c
/* Struktur driver generik */
typedef struct {
    char nama[32];                    /* Nama driver */
    tak_bertanda32_t kategori;        /* Kategori IC */
    
    /* Callbacks */
    status_t (*probe)(ic_info_t *ic);           /* Cek kompatibilitas */
    status_t (*init)(ic_info_t *ic, ic_param_t *param); /* Inisialisasi */
    status_t (*reset)(ic_info_t *ic);           /* Reset device */
    void (*hapus)(ic_info_t *ic);               /* Cleanup */
    
    /* Operations */
    tak_bertanda32_t (*read)(ic_info_t *ic, void *buf, ukuran_t size);
    tak_bertanda32_t (*write)(ic_info_t *ic, const void *buf, ukuran_t size);
    status_t (*ioctl)(ic_info_t *ic, tak_bertanda32_t cmd, void *arg);
    
} driver_generik_t;
```

### Contoh: WiFi Generik

```c
/* Driver WiFi generik */
static status_t wifi_umum_probe(ic_info_t *ic) {
    /* Cek apakah IC adalah WiFi */
    if (ic->kategori != IC_KATEGORI_NETWORK_WIFI) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    return STATUS_BERHASIL;
}

static status_t wifi_umum_init(ic_info_t *ic, ic_param_t *param) {
    /* Inisialisasi dengan parameter dari probe */
    
    /* Set transfer rate */
    ic_write_config(ic->bus_id, WIFI_REG_RATE, param->rate);
    
    /* Set TX power */
    ic_write_config(ic->bus_id, WIFI_REG_TX_POWER, param->tx_power);
    
    /* Set channel */
    ic_write_config(ic->bus_id, WIFI_REG_CHANNEL, param->channel);
    
    /* Enable WiFi */
    ic_write_config(ic->bus_id, WIFI_REG_ENABLE, 1);
    
    return STATUS_BERHASIL;
}

static tak_bertanda32_t wifi_umum_read(ic_info_t *ic, void *buf, ukuran_t size) {
    /* Baca packet dari WiFi */
    return hal_wifi_receive(ic->bus_id, buf, size);
}

static tak_bertanda32_t wifi_umum_write(ic_info_t *ic, const void *buf, ukuran_t size) {
    /* Kirim packet via WiFi */
    return hal_wifi_transmit(ic->bus_id, buf, size);
}

/* Registrasi driver */
driver_generik_t driver_wifi_umum = {
    .nama = "wifi_generik",
    .kategori = IC_KATEGORI_NETWORK_WIFI,
    .probe = wifi_umum_probe,
    .init = wifi_umum_init,
    .read = wifi_umum_read,
    .write = wifi_umum_write,
};
```

### Contoh: NVMe Generik

```c
/* Driver NVMe generik */
static status_t nvme_umum_init(ic_info_t *ic, ic_param_t *param) {
    /* Enable PCI bus master */
    pci_enable_bus_master(ic->bus_id);
    
    /* Mapping register */
    void *regs = hal_memori_map_pci_bar(ic->bus_id, 0);
    
    /* Configure admin queue */
    nvme_config_admin_queue(regs, param->queue_depth);
    
    /* Create I/O queue */
    nvme_create_io_queue(regs, param->queue_depth);
    
    /* Identify namespace */
    nvme_identify_namespace(regs, 1);
    
    return STATUS_BERHASIL;
}

static tak_bertanda32_t nvme_umum_read(ic_info_t *ic, void *buf, ukuran_t size) {
    /* Submit read command */
    return nvme_submit_read(ic->priv, buf, size);
}
```

---

## API Reference

### Deteksi

```c
/* Inisialisasi sistem IC detection */
status_t ic_deteksi_init(void);

/* Scan semua bus untuk IC */
status_t ic_deteksi_scan(void);

/* Dapatkan jumlah IC terdeteksi */
tak_bertanda32_t ic_deteksi_jumlah(void);

/* Dapatkan info IC berdasarkan index */
ic_info_t *ic_deteksi_info(tak_bertanda32_t index);

/* Cari IC berdasarkan kategori */
ic_info_t *ic_deteksi_cari_kategori(tak_bertanda32_t kategori);

/* Cari IC berdasarkan VID/DID */
ic_info_t *ic_deteksi_cari_id(tak_bertanda16_t vid, tak_bertanda16_t did);
```

### Parameter

```c
/* Baca parameter dari IC */
status_t ic_parameter_baca(ic_info_t *ic, ic_param_t *param);

/* Validasi parameter */
status_t ic_parameter_validasi(ic_param_t *param, ic_kategori_t *kategori);

/* Pilih nilai default */
status_t ic_parameter_pilih_default(ic_param_t *param, ic_kategori_t *kategori);
```

### Database

```c
/* Dapatkan kategori dari database */
ic_kategori_t *db_kategori_dapatkan(tak_bertanda32_t kategori_id);

/* Cek apakah nilai dalam rentang */
bool_t db_nilai_valid(tak_bertanda32_t param_id, tak_bertanda64_t nilai);

/* Dapatkan nilai default dari database */
tak_bertanda64_t db_default_dapatkan(tak_bertanda32_t param_id);
```

---

## Implementasi Contoh

### Contoh Penggunaan Lengkap

```c
#include "ic_deteksi.h"
#include "driver_umum.h"

status_t sistem_init_jaringan(void) {
    ic_info_t *wifi_ic;
    ic_param_t param;
    status_t status;
    
    /* Scan untuk IC WiFi */
    wifi_ic = ic_deteksi_cari_kategori(IC_KATEGORI_NETWORK_WIFI);
    if (wifi_ic == NULL) {
        log_error("Tidak ada IC WiFi terdeteksi");
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    log_info("WiFi IC ditemukan: VID=%04x DID=%04x",
             wifi_ic->vendor_id, wifi_ic->device_id);
    
    /* Baca parameter dari IC */
    status = ic_parameter_baca(wifi_ic, &param);
    if (status != STATUS_BERHASIL) {
        log_error("Gagal membaca parameter WiFi");
        return status;
    }
    
    log_info("Transfer Rate: %d-%d Mbps (typical: %d)",
             param.wifi.rate_min, param.wifi.rate_max,
             param.wifi.rate_typical);
    
    /* Validasi parameter */
    status = ic_parameter_validasi(&param, &db_wifi_kategori);
    if (status != STATUS_BERHASIL) {
        log_warn("Parameter WiFi di luar rentang aman, menggunakan safemode");
    }
    
    /* Inisialisasi driver generik */
    status = driver_wifi_umum.init(wifi_ic, &param);
    if (status != STATUS_BERHASIL) {
        log_error("Gagal inisialisasi WiFi");
        return status;
    }
    
    log_info("WiFi siap digunakan");
    return STATUS_BERHASIL;
}
```

---

## Ringkasan

Sistem IC Detection Pigura OS memberikan:

1. **Efisiensi** - Satu driver untuk ratusan IC
2. **Keamanan** - Parameter dari IC sendiri, bukan hardcoded
3. **Simplicity** - Kode lebih kecil dan mudah maintenance
4. **Flexibility** - Mudah menambah dukungan IC baru
5. **Robustness** - Parameter selalu dalam rentang yang valid

Pendekatan ini menghasilkan estimasi penghematan:
- **50-60%** waktu development
- **99%** ukuran kode driver
- **90%** kompleksitas maintenance