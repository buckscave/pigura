/*
 * PIGURA OS - IC.H
 * =================
 * Header utama sistem deteksi IC (Integrated Circuit).
 *
 * Sistem ini mendeteksi dan mengidentifikasi perangkat keras berdasarkan
 * IC/Chip, bukan berdasarkan vendor. Database berisi informasi seri/tipe
 * IC dari low-end hingga high-end untuk identifikasi yang akurat.
 *
 * Filosofi:
 * - Deteksi IC berdasarkan fungsi dan spesifikasi teknis
 * - Test parameter secara nyata (bukan dari datasheet)
 * - Toleransi 5-10% untuk nilai default operasional yang aman
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#ifndef PERANGKAT_IC_H
#define PERANGKAT_IC_H

/*
 * ===========================================================================
 * INCLUDE
 * ===========================================================================
 */

#include "../../inti/types.h"

/*
 * ===========================================================================
 * KONSTANTA SISTEM IC
 * ===========================================================================
 */

/* Versi sistem IC */
#define IC_VERSI_MAJOR 1
#define IC_VERSI_MINOR 0
#define IC_VERSI_PATCH 0

/* Magic number untuk validasi */
#define IC_MAGIC 0x49435359  /* "ICSY" - IC System */
#define IC_ENTRI_MAGIC 0x4943454E  /* "ICEN" - IC Entry */
#define IC_PARAM_MAGIC 0x49435041  /* "ICPA" - IC Parameter */

/* Batas sistem */
#define IC_NAMA_PANJANG_MAKS 64
#define IC_VENDOR_PANJANG_MAKS 32
#define IC_SERI_PANJANG_MAKS 48
#define IC_DESKRIPSI_PANJANG_MAKS 128
#define IC_SATUAN_PANJANG_MAKS 16

/* Jumlah maksimum */
#define IC_MAKS_ENTRI 256
#define IC_MAKS_PARAM_PER_KATEGORI 32
#define IC_MAKS_KATEGORI 16
#define IC_MAKS_PERANGKAT 128

/* Toleransi default untuk test parameter */
#define IC_TOLERANSI_DEFAULT_MIN 5   /* 5% */
#define IC_TOLERANSI_DEFAULT_MAX 10  /* 10% */

/*
 * ===========================================================================
 * KATEGORI IC
 * ===========================================================================
 */

typedef enum {
    IC_KATEGORI_TIDAK_DIKETAHUI = 0,
    IC_KATEGORI_CPU = 1,
    IC_KATEGORI_GPU = 2,
    IC_KATEGORI_RAM = 3,
    IC_KATEGORI_ROM = 4,
    IC_KATEGORI_AUDIO = 5,
    IC_KATEGORI_NETWORK = 6,
    IC_KATEGORI_STORAGE = 7,
    IC_KATEGORI_DISPLAY = 8,
    IC_KATEGORI_INPUT = 9,
    IC_KATEGORI_USB = 10,
    IC_KATEGORI_BRIDGE = 11,
    IC_KATEGORI_POWER = 12,
    IC_KATEGORI_TIMER = 13,
    IC_KATEGORI_DMA = 14,
    IC_KATEGORI_LAINNYA = 15
} ic_kategori_t;

/*
 * ===========================================================================
 * RENTANG KINERJA (LOW-END SAMPAI HIGH-END)
 * ===========================================================================
 */

typedef enum {
    IC_RENTANG_TIDAK_DIKETAHUI = 0,
    IC_RENTANG_LOW_END = 1,      /* Entry level, legacy */
    IC_RENTANG_MID_LOW = 2,      /* Low mid-range */
    IC_RENTANG_MID = 3,          /* Mid-range */
    IC_RENTANG_MID_HIGH = 4,     /* High mid-range */
    IC_RENTANG_HIGH_END = 5,     /* High-end, enthusiast */
    IC_RENTANG_ENTHUSIAST = 6,   /* Enthusiast, workstation */
    IC_RENTANG_SERVER = 7        /* Server, datacenter */
} ic_rentang_t;

/*
 * ===========================================================================
 * STATUS IC
 * ===========================================================================
 */

typedef enum {
    IC_STATUS_TIDAK_ADA = 0,
    IC_STATUS_TERDETEKSI = 1,
    IC_STATUS_DIIDENTIFIKASI = 2,
    IC_STATUS_DITEST = 3,
    IC_STATUS_SIAP = 4,
    IC_STATUS_ERROR = 5,
    IC_STATUS_TIDAK_DUKUNG = 6
} ic_status_t;

/*
 * ===========================================================================
 * TIPE BUS
 * ===========================================================================
 */

typedef enum {
    IC_BUS_TIDAK_DIKETAHUI = 0,
    IC_BUS_PCI = 1,
    IC_BUS_PCIE = 2,
    IC_BUS_USB = 3,
    IC_BUS_I2C = 4,
    IC_BUS_SPI = 5,
    IC_BUS_MMIO = 6,
    IC_BUS_ISA = 7,
    IC_BUS_APB = 8,     /* ARM Peripheral Bus */
    IC_BUS_AXI = 9      /* ARM Advanced eXtensible Interface */
} ic_bus_t;

/*
 * ===========================================================================
 * STRUKTUR PARAMETER IC
 * ===========================================================================
 */

/* Tipe parameter */
typedef enum {
    IC_PARAM_TIPE_TIDAK_DIKETAHUI = 0,
    IC_PARAM_TIPE_FREKUENSI = 1,     /* MHz, GHz */
    IC_PARAM_TIPE_KAPASITAS = 2,     /* Byte, KB, MB, GB */
    IC_PARAM_TIPE_KECEPATAN = 3,     /* Mbps, Gbps */
    IC_PARAM_TIPE_DAYA = 4,          /* mW, W */
    IC_PARAM_TIPE_TEGANGAN = 5,      /* mV, V */
    IC_PARAM_TIPE_ARUS = 6,          /* mA, A */
    IC_PARAM_TIPE_SUHU = 7,          /* Celsius */
    IC_PARAM_TIPE_LATENSI = 8,       /* ns, us, ms */
    IC_PARAM_TIPE_JUMLAH = 9,        /* cores, threads, units */
    IC_PARAM_TIPE_LEBAR_BIT = 10,    /* bit */
    IC_PARAM_TIPE_RESOLUSI = 11,     /* pixels */
    IC_PARAM_TIPE_REFRESH = 12,      /* Hz */
    IC_PARAM_TIPE_CHANNEL = 13       /* channels */
} ic_param_tipe_t;

/* Satuan parameter */
typedef enum {
    IC_SATUAN_TIDAK_ADA = 0,
    IC_SATUAN_HZ = 1,
    IC_SATUAN_KHZ = 2,
    IC_SATUAN_MHZ = 3,
    IC_SATUAN_GHZ = 4,
    IC_SATUAN_BYTE = 5,
    IC_SATUAN_KB = 6,
    IC_SATUAN_MB = 7,
    IC_SATUAN_GB = 8,
    IC_SATUAN_TB = 9,
    IC_SATUAN_BIT = 10,
    IC_SATUAN_MBPS = 11,
    IC_SATUAN_GBPS = 12,
    IC_SATUAN_MW = 13,
    IC_SATUAN_W = 14,
    IC_SATUAN_MV = 15,
    IC_SATUAN_V = 16,
    IC_SATUAN_MA = 17,
    IC_SATUAN_A = 18,
    IC_SATUAN_CELSIUS = 19,
    IC_SATUAN_NS = 20,
    IC_SATUAN_US = 21,
    IC_SATUAN_MS = 22,
    IC_SATUAN_S = 23,
    IC_SATUAN_CORES = 24,
    IC_SATUAN_THREADS = 25,
    IC_SATUAN_UNITS = 26,
    IC_SATUAN_PIXELS = 27,
    IC_SATUAN_CHANNELS = 28,
    IC_SATUAN_PERCENT = 29,
    IC_SATUAN_DBM = 30
} ic_satuan_t;

/* Struktur parameter IC */
typedef struct {
    tak_bertanda32_t id;              /* ID parameter */
    tak_bertanda32_t tipe;            /* Tipe parameter */
    char nama[IC_NAMA_PANJANG_MAKS];  /* Nama parameter */
    char satuan[IC_SATUAN_PANJANG_MAKS]; /* Satuan */
    tak_bertanda64_t nilai_min;       /* Nilai minimum dari test */
    tak_bertanda64_t nilai_max;       /* Nilai maksimum dari test */
    tak_bertanda64_t nilai_typical;   /* Nilai typical dari test */
    tak_bertanda64_t nilai_default;   /* Nilai default dengan toleransi */
    tak_bertanda32_t toleransi_min;   /* Toleransi minimum (%) */
    tak_bertanda32_t toleransi_max;   /* Toleransi maksimum (%) */
    bool_t didukung;                  /* Apakah parameter didukung */
    bool_t ditest;                    /* Apakah sudah ditest */
    tak_bertanda32_t flags;           /* Flags tambahan */
} ic_parameter_t;

/*
 * ===========================================================================
 * STRUKTUR ENTRI DATABASE IC
 * ===========================================================================
 */

typedef struct ic_entri ic_entri_t;

struct ic_entri {
    tak_bertanda32_t magic;            /* Magic number validasi */
    tak_bertanda16_t vendor_id;        /* PCI/USB Vendor ID */
    tak_bertanda16_t device_id;        /* PCI/USB Device ID */
    tak_bertanda32_t ic_id;            /* ID unik IC dalam sistem */
    
    char nama[IC_NAMA_PANJANG_MAKS];   /* Nama IC */
    char vendor[IC_VENDOR_PANJANG_MAKS]; /* Nama vendor */
    char seri[IC_SERI_PANJANG_MAKS];   /* Seri/tipe IC */
    char deskripsi[IC_DESKRIPSI_PANJANG_MAKS]; /* Deskripsi singkat */
    
    ic_kategori_t kategori;            /* Kategori IC */
    ic_rentang_t rentang;              /* Rentang kinerja */
    ic_bus_t bus;                      /* Bus yang digunakan */
    
    ic_parameter_t *param;             /* Array parameter */
    tak_bertanda32_t jumlah_param;     /* Jumlah parameter */
    
    ic_status_t status;                /* Status saat ini */
    bool_t teridentifikasi;            /* Apakah berhasil diidentifikasi */
    
    ic_entri_t *berikutnya;            /* Pointer ke entri berikutnya */
};

/*
 * ===========================================================================
 * STRUKTUR PERANGKAT TERDETEKSI
 * ===========================================================================
 */

typedef struct {
    tak_bertanda32_t magic;            /* Magic number validasi */
    tak_bertanda32_t id;               /* ID perangkat dalam sistem */
    
    /* Identifikasi bus */
    ic_bus_t bus;                      /* Bus tempat terdeteksi */
    tak_bertanda8_t bus_num;           /* Nomor bus */
    tak_bertanda8_t dev_num;           /* Nomor device */
    tak_bertanda8_t func_num;          /* Nomor fungsi */
    
    /* ID dari hardware */
    tak_bertanda16_t vendor_id;        /* Vendor ID dari hardware */
    tak_bertanda16_t device_id;        /* Device ID dari hardware */
    tak_bertanda32_t class_code;       /* Class code (PCI) */
    tak_bertanda8_t revision;          /* Revision ID */
    
    /* Referensi ke database */
    ic_entri_t *entri;                 /* Pointer ke entri database */
    
    /* Parameter hasil test */
    ic_parameter_t *param_hasil;       /* Parameter hasil test */
    tak_bertanda32_t jumlah_param_hasil;
    
    /* Status */
    ic_status_t status;
    bool_t terdeteksi;
    bool_t teridentifikasi;
    bool_t ditest;
    
    /* Alamat resources */
    alamat_fisik_t bar[6];             /* Base Address Registers */
    tak_bertanda32_t bar_ukuran[6];
    tak_bertanda32_t irq;              /* IRQ number */
    
} ic_perangkat_t;

/*
 * ===========================================================================
 * STRUKTUR KONTEKS SISTEM IC
 * ===========================================================================
 */

typedef struct {
    tak_bertanda32_t magic;            /* Magic number validasi */
    tak_bertanda32_t status;           /* Status sistem */
    
    /* Database */
    ic_entri_t *database;              /* Pointer ke database */
    tak_bertanda32_t jumlah_entri;     /* Jumlah entri database */
    
    /* Perangkat terdeteksi */
    ic_perangkat_t perangkat[IC_MAKS_PERANGKAT];
    tak_bertanda32_t jumlah_perangkat;
    
    /* Statistik */
    tak_bertanda32_t total_probe;
    tak_bertanda32_t total_teridentifikasi;
    tak_bertanda32_t total_tak_dikenal;
    
} ic_konteks_t;

/*
 * ===========================================================================
 * FUNGSI INISIALISASI
 * ===========================================================================
 */

/*
 * ic_init - Inisialisasi sistem IC Detection
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ic_init(void);

/*
 * ic_shutdown - Matikan sistem IC Detection
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ic_shutdown(void);

/*
 * ic_konteks_dapatkan - Dapatkan konteks sistem IC
 *
 * Return: Pointer ke konteks atau NULL jika belum diinisialisasi
 */
ic_konteks_t *ic_konteks_dapatkan(void);

/*
 * ===========================================================================
 * FUNGSI DETEKSI
 * ===========================================================================
 */

/*
 * ic_deteksi_semua - Deteksi semua IC di sistem
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ic_deteksi_semua(void);

/*
 * ic_deteksi_bus - Deteksi IC di bus tertentu
 *
 * Parameter:
 *   bus - Tipe bus yang akan di-scan
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ic_deteksi_bus(ic_bus_t bus);

/*
 * ic_deteksi_perangkat - Deteksi satu perangkat
 *
 * Parameter:
 *   bus      - Tipe bus
 *   bus_num  - Nomor bus
 *   dev_num  - Nomor device
 *   func_num - Nomor fungsi
 *
 * Return: Pointer ke perangkat atau NULL jika gagal
 */
ic_perangkat_t *ic_deteksi_perangkat(ic_bus_t bus, tak_bertanda8_t bus_num,
                                      tak_bertanda8_t dev_num,
                                      tak_bertanda8_t func_num);

/*
 * ===========================================================================
 * FUNGSI IDENTIFIKASI
 * ===========================================================================
 */

/*
 * ic_identifikasi - Identifikasi perangkat dengan database
 *
 * Parameter:
 *   perangkat - Pointer ke perangkat yang akan diidentifikasi
 *
 * Return: STATUS_BERHASIL jika berhasil diidentifikasi
 */
status_t ic_identifikasi(ic_perangkat_t *perangkat);

/*
 * ic_cari_database - Cari entri di database berdasarkan ID
 *
 * Parameter:
 *   vendor_id - Vendor ID
 *   device_id - Device ID
 *
 * Return: Pointer ke entri atau NULL jika tidak ditemukan
 */
ic_entri_t *ic_cari_database(tak_bertanda16_t vendor_id,
                              tak_bertanda16_t device_id);

/*
 * ic_cari_kategori - Cari perangkat berdasarkan kategori
 *
 * Parameter:
 *   kategori - Kategori yang dicari
 *   indeks  - Indeks perangkat (untuk multiple perangkat)
 *
 * Return: Pointer ke perangkat atau NULL
 */
ic_perangkat_t *ic_cari_kategori(ic_kategori_t kategori,
                                  tak_bertanda32_t indeks);

/*
 * ===========================================================================
 * FUNGSI TEST PARAMETER
 * ===========================================================================
 */

/*
 * ic_test_parameter - Test parameter perangkat
 *
 * Parameter:
 *   perangkat - Pointer ke perangkat
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ic_test_parameter(ic_perangkat_t *perangkat);

/*
 * ic_test_parameter_tunggal - Test satu parameter
 *
 * Parameter:
 *   perangkat - Pointer ke perangkat
 *   param_id  - ID parameter
 *   nilai_min - Pointer untuk menyimpan nilai minimum
 *   nilai_max - Pointer untuk menyimpan nilai maksimum
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ic_test_parameter_tunggal(ic_perangkat_t *perangkat,
                                    tak_bertanda32_t param_id,
                                    tak_bertanda64_t *nilai_min,
                                    tak_bertanda64_t *nilai_max);

/*
 * ic_hitung_default - Hitung nilai default dengan toleransi
 *
 * Parameter:
 *   nilai_max   - Nilai maksimum dari test
 *   toleransi   - Persentase toleransi (5-10%)
 *
 * Return: Nilai default yang aman
 */
tak_bertanda64_t ic_hitung_default(tak_bertanda64_t nilai_max,
                                    tak_bertanda32_t toleransi);

/*
 * ===========================================================================
 * FUNGSI UTILITAS
 * ===========================================================================
 */

/*
 * ic_kategori_nama - Dapatkan nama kategori
 *
 * Parameter:
 *   kategori - Kategori IC
 *
 * Return: String nama kategori
 */
const char *ic_kategori_nama(ic_kategori_t kategori);

/*
 * ic_rentang_nama - Dapatkan nama rentang
 *
 * Parameter:
 *   rentang - Rentang kinerja
 *
 * Return: String nama rentang
 */
const char *ic_rentang_nama(ic_rentang_t rentang);

/*
 * ic_bus_nama - Dapatkan nama bus
 *
 * Parameter:
 *   bus - Tipe bus
 *
 * Return: String nama bus
 */
const char *ic_bus_nama(ic_bus_t bus);

/*
 * ic_satuan_nama - Dapatkan nama satuan
 *
 * Parameter:
 *   satuan - Tipe satuan
 *
 * Return: String nama satuan
 */
const char *ic_satuan_nama(ic_satuan_t satuan);

/*
 * ic_status_nama - Dapatkan nama status
 *
 * Parameter:
 *   status - Status IC
 *
 * Return: String nama status
 */
const char *ic_status_nama(ic_status_t status);

/*
 * ic_jumlah_perangkat - Dapatkan jumlah perangkat terdeteksi
 *
 * Return: Jumlah perangkat
 */
tak_bertanda32_t ic_jumlah_perangkat(void);

/*
 * ic_perangkat_dapatkan - Dapatkan perangkat berdasarkan indeks
 *
 * Parameter:
 *   indeks - Indeks perangkat
 *
 * Return: Pointer ke perangkat atau NULL
 */
ic_perangkat_t *ic_perangkat_dapatkan(tak_bertanda32_t indeks);

/*
 * ic_cetak_info - Cetak informasi perangkat
 *
 * Parameter:
 *   perangkat - Pointer ke perangkat
 */
void ic_cetak_info(ic_perangkat_t *perangkat);

/*
 * ic_cetak_semua - Cetak informasi semua perangkat
 */
void ic_cetak_semua(void);

/*
 * ===========================================================================
 * FUNGSI DATABASE
 * ===========================================================================
 */

/*
 * ic_database_init - Inisialisasi database IC
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ic_database_init(void);

/*
 * ic_database_tambah - Tambah entri ke database
 *
 * Parameter:
 *   entri - Pointer ke entri yang akan ditambahkan
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ic_database_tambah(ic_entri_t *entri);

/*
 * ic_database_muat - Muat database dari semua kategori
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ic_database_muat(void);

/*
 * ===========================================================================
 * FUNGSI PROBE
 * ===========================================================================
 */

/*
 * ic_probe_init - Inisialisasi sistem probe
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t ic_probe_init(void);

/*
 * ic_probe_pci - Scan bus PCI
 *
 * Return: Jumlah perangkat ditemukan, atau nilai negatif jika error
 */
tanda32_t ic_probe_pci(void);

/*
 * ic_probe_usb - Scan bus USB
 *
 * Return: Jumlah perangkat ditemukan, atau nilai negatif jika error
 */
tanda32_t ic_probe_usb(void);

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

/* Konteks global sistem IC */
extern ic_konteks_t g_ic_konteks;

/* Flag inisialisasi */
extern bool_t g_ic_diinisialisasi;

#endif /* PERANGKAT_IC_H */
