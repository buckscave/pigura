/*
 * PIGURA OS - IC_DETEKSI.C
 * ========================
 * Engine deteksi utama untuk sistem IC Detection.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi deteksi IC utama
 * yang memindai bus sistem dan mengidentifikasi perangkat.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "ic.h"

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

ic_konteks_t g_ic_konteks;
bool_t g_ic_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

/* Buffer untuk pesan error */
static char pesan_error[256];

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * ic_set_error - Set pesan error
 */
static void ic_set_error(const char *pesan)
{
    ukuran_t panjang;
    
    if (pesan == NULL) {
        return;
    }
    
    panjang = 0;
    while (pesan[panjang] != '\0' && panjang < 255) {
        pesan_error[panjang] = pesan[panjang];
        panjang++;
    }
    pesan_error[panjang] = '\0';
}

/*
 * ic_validasi_konteks - Validasi konteks sistem IC
 */
static bool_t ic_validasi_konteks(void)
{
    if (!g_ic_diinisialisasi) {
        ic_set_error("Sistem IC belum diinisialisasi");
        return SALAH;
    }
    
    if (g_ic_konteks.magic != IC_MAGIC) {
        ic_set_error("Konteks IC tidak valid");
        return SALAH;
    }
    
    return BENAR;
}

/*
 * ic_inisialisasi_perangkat - Inisialisasi struktur perangkat
 */
static void ic_inisialisasi_perangkat(ic_perangkat_t *perangkat)
{
    tak_bertanda32_t i;
    
    if (perangkat == NULL) {
        return;
    }
    
    perangkat->magic = IC_ENTRI_MAGIC;
    perangkat->id = 0;
    perangkat->bus = IC_BUS_TIDAK_DIKETAHUI;
    perangkat->bus_num = 0;
    perangkat->dev_num = 0;
    perangkat->func_num = 0;
    perangkat->vendor_id = 0;
    perangkat->device_id = 0;
    perangkat->class_code = 0;
    perangkat->revision = 0;
    perangkat->entri = NULL;
    perangkat->param_hasil = NULL;
    perangkat->jumlah_param_hasil = 0;
    perangkat->status = IC_STATUS_TIDAK_ADA;
    perangkat->terdeteksi = SALAH;
    perangkat->teridentifikasi = SALAH;
    perangkat->ditest = SALAH;
    perangkat->irq = 0;
    
    for (i = 0; i < 6; i++) {
        perangkat->bar[i] = 0;
        perangkat->bar_ukuran[i] = 0;
    }
}

/*
 * ===========================================================================
 * FUNGSI INISIALISASI
 * ===========================================================================
 */

status_t ic_init(void)
{
    status_t hasil;
    
    if (g_ic_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }
    
    /* Inisialisasi konteks */
    g_ic_konteks.magic = 0;
    g_ic_konteks.status = 0;
    g_ic_konteks.database = NULL;
    g_ic_konteks.jumlah_entri = 0;
    g_ic_konteks.jumlah_perangkat = 0;
    g_ic_konteks.total_probe = 0;
    g_ic_konteks.total_teridentifikasi = 0;
    g_ic_konteks.total_tak_dikenal = 0;
    
    /* Inisialisasi database */
    hasil = ic_database_init();
    if (hasil != STATUS_BERHASIL) {
        ic_set_error("Gagal menginisialisasi database IC");
        return hasil;
    }
    
    /* Muat database dari semua kategori */
    hasil = ic_database_muat();
    if (hasil != STATUS_BERHASIL) {
        ic_set_error("Gagal memuat database IC");
        return hasil;
    }
    
    /* Inisialisasi sistem probe */
    hasil = ic_probe_init();
    if (hasil != STATUS_BERHASIL) {
        ic_set_error("Gagal menginisialisasi sistem probe");
        return hasil;
    }
    
    /* Set magic number dan status */
    g_ic_konteks.magic = IC_MAGIC;
    g_ic_konteks.status = IC_STATUS_SIAP;
    g_ic_diinisialisasi = BENAR;
    
    return STATUS_BERHASIL;
}

status_t ic_shutdown(void)
{
    tak_bertanda32_t i;
    
    if (!g_ic_diinisialisasi) {
        return STATUS_BERHASIL;
    }
    
    /* Bersihkan perangkat */
    for (i = 0; i < IC_MAKS_PERANGKAT; i++) {
        if (g_ic_konteks.perangkat[i].param_hasil != NULL) {
            /* Parameter akan dibebaskan oleh sistem memori */
            g_ic_konteks.perangkat[i].param_hasil = NULL;
        }
        g_ic_konteks.perangkat[i].magic = 0;
        g_ic_konteks.perangkat[i].status = IC_STATUS_TIDAK_ADA;
    }
    
    /* Reset konteks */
    g_ic_konteks.magic = 0;
    g_ic_konteks.status = IC_STATUS_TIDAK_ADA;
    g_ic_konteks.jumlah_perangkat = 0;
    g_ic_konteks.jumlah_entri = 0;
    
    g_ic_diinisialisasi = SALAH;
    
    return STATUS_BERHASIL;
}

ic_konteks_t *ic_konteks_dapatkan(void)
{
    if (!g_ic_diinisialisasi) {
        return NULL;
    }
    
    if (g_ic_konteks.magic != IC_MAGIC) {
        return NULL;
    }
    
    return &g_ic_konteks;
}

/*
 * ===========================================================================
 * FUNGSI DETEKSI
 * ===========================================================================
 */

status_t ic_deteksi_semua(void)
{
    tanda32_t jumlah;
    status_t hasil_total = STATUS_BERHASIL;
    
    if (!ic_validasi_konteks()) {
        return STATUS_GAGAL;
    }
    
    /* Reset statistik */
    g_ic_konteks.total_probe = 0;
    g_ic_konteks.total_teridentifikasi = 0;
    g_ic_konteks.total_tak_dikenal = 0;
    
    /* Scan bus PCI */
    jumlah = ic_probe_pci();
    if (jumlah > 0) {
        g_ic_konteks.total_probe += (tak_bertanda32_t)jumlah;
    } else if (jumlah < 0) {
        hasil_total = STATUS_GAGAL;
    }
    
    /* Scan bus USB */
    jumlah = ic_probe_usb();
    if (jumlah > 0) {
        g_ic_konteks.total_probe += (tak_bertanda32_t)jumlah;
    } else if (jumlah < 0) {
        /* USB failure tidak fatal */
        /* tetap lanjutkan */
    }
    
    /* Identifikasi semua perangkat terdeteksi */
    ic_identifikasi_semua();
    
    return hasil_total;
}

status_t ic_deteksi_bus(ic_bus_t bus)
{
    tanda32_t jumlah;
    
    if (!ic_validasi_konteks()) {
        return STATUS_GAGAL;
    }
    
    switch (bus) {
    case IC_BUS_PCI:
    case IC_BUS_PCIE:
        jumlah = ic_probe_pci();
        break;
    case IC_BUS_USB:
        jumlah = ic_probe_usb();
        break;
    default:
        ic_set_error("Bus tidak didukung");
        return STATUS_TIDAK_DUKUNG;
    }
    
    if (jumlah < 0) {
        return STATUS_GAGAL;
    }
    
    return STATUS_BERHASIL;
}

ic_perangkat_t *ic_deteksi_perangkat(ic_bus_t bus, tak_bertanda8_t bus_num,
                                      tak_bertanda8_t dev_num,
                                      tak_bertanda8_t func_num)
{
    ic_perangkat_t *perangkat;
    tak_bertanda32_t indeks;
    
    if (!ic_validasi_konteks()) {
        return NULL;
    }
    
    /* Cari slot kosong atau perangkat yang sudah ada */
    for (indeks = 0; indeks < IC_MAKS_PERANGKAT; indeks++) {
        perangkat = &g_ic_konteks.perangkat[indeks];
        
        if (perangkat->magic != IC_ENTRI_MAGIC) {
            /* Slot kosong, inisialisasi */
            ic_inisialisasi_perangkat(perangkat);
            perangkat->id = indeks;
            perangkat->bus = bus;
            perangkat->bus_num = bus_num;
            perangkat->dev_num = dev_num;
            perangkat->func_num = func_num;
            
            if (g_ic_konteks.jumlah_perangkat <= indeks) {
                g_ic_konteks.jumlah_perangkat = indeks + 1;
            }
            
            return perangkat;
        }
        
        /* Cek apakah perangkat yang sama */
        if (perangkat->bus == bus &&
            perangkat->bus_num == bus_num &&
            perangkat->dev_num == dev_num &&
            perangkat->func_num == func_num) {
            return perangkat;
        }
    }
    
    ic_set_error("Tidak ada slot perangkat tersedia");
    return NULL;
}

/*
 * ===========================================================================
 * FUNGSI IDENTIFIKASI
 * ===========================================================================
 */

status_t ic_identifikasi(ic_perangkat_t *perangkat)
{
    ic_entri_t *entri;
    
    if (perangkat == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (perangkat->magic != IC_ENTRI_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Cari di database */
    entri = ic_cari_database(perangkat->vendor_id, perangkat->device_id);
    
    if (entri != NULL) {
        /* Ditemukan di database */
        perangkat->entri = entri;
        perangkat->teridentifikasi = BENAR;
        perangkat->status = IC_STATUS_DIIDENTIFIKASI;
        
        /* Salin informasi dasar */
        /* dari entri database */
        
        g_ic_konteks.total_teridentifikasi++;
        
        return STATUS_BERHASIL;
    }
    
    /* Tidak ditemukan - tandai sebagai unknown */
    perangkat->entri = NULL;
    perangkat->teridentifikasi = SALAH;
    perangkat->status = IC_STATUS_TERDETEKSI;
    
    g_ic_konteks.total_tak_dikenal++;
    
    /* Perangkat tidak dikenali tapi tetap usable */
    return STATUS_TIDAK_DITEMUKAN;
}

status_t ic_identifikasi_semua(void)
{
    tak_bertanda32_t i;
    ic_perangkat_t *perangkat;
    
    if (!ic_validasi_konteks()) {
        return STATUS_GAGAL;
    }
    
    for (i = 0; i < g_ic_konteks.jumlah_perangkat; i++) {
        perangkat = &g_ic_konteks.perangkat[i];
        
        if (perangkat->magic == IC_ENTRI_MAGIC && perangkat->terdeteksi) {
            ic_identifikasi(perangkat);
        }
    }
    
    return STATUS_BERHASIL;
}

ic_entri_t *ic_cari_database(tak_bertanda16_t vendor_id,
                              tak_bertanda16_t device_id)
{
    ic_entri_t *entri;
    
    if (!ic_validasi_konteks()) {
        return NULL;
    }
    
    entri = g_ic_konteks.database;
    
    while (entri != NULL) {
        if (entri->vendor_id == vendor_id &&
            entri->device_id == device_id) {
            return entri;
        }
        entri = entri->berikutnya;
    }
    
    return NULL;
}

ic_perangkat_t *ic_cari_kategori(ic_kategori_t kategori,
                                  tak_bertanda32_t indeks)
{
    tak_bertanda32_t i;
    tak_bertanda32_t count = 0;
    ic_perangkat_t *perangkat;
    
    if (!ic_validasi_konteks()) {
        return NULL;
    }
    
    for (i = 0; i < g_ic_konteks.jumlah_perangkat; i++) {
        perangkat = &g_ic_konteks.perangkat[i];
        
        if (perangkat->magic == IC_ENTRI_MAGIC &&
            perangkat->entri != NULL &&
            perangkat->entri->kategori == kategori) {
            
            if (count == indeks) {
                return perangkat;
            }
            count++;
        }
    }
    
    return NULL;
}

/*
 * ===========================================================================
 * FUNGSI TEST PARAMETER
 * ===========================================================================
 */

status_t ic_test_parameter(ic_perangkat_t *perangkat)
{
    if (perangkat == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (perangkat->magic != IC_ENTRI_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    if (perangkat->entri == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Test akan dilakukan berdasarkan kategori */
    /* Implementasi detail di ic_parameter.c */
    
    perangkat->ditest = BENAR;
    perangkat->status = IC_STATUS_DITEST;
    
    return STATUS_BERHASIL;
}

tak_bertanda64_t ic_hitung_default(tak_bertanda64_t nilai_max,
                                    tak_bertanda32_t toleransi)
{
    tak_bertanda64_t hasil;
    tak_bertanda64_t pengurang;
    
    if (nilai_max == 0) {
        return 0;
    }
    
    /* Batasi toleransi antara 5% dan 10% */
    if (toleransi < IC_TOLERANSI_DEFAULT_MIN) {
        toleransi = IC_TOLERANSI_DEFAULT_MIN;
    }
    if (toleransi > IC_TOLERANSI_DEFAULT_MAX) {
        toleransi = IC_TOLERANSI_DEFAULT_MAX;
    }
    
    /* Hitung pengurang: nilai_max * toleransi / 100 */
    pengurang = (nilai_max * toleransi) / 100;
    
    /* Nilai default = nilai_max - pengurang */
    if (pengurang > nilai_max) {
        hasil = 0;
    } else {
        hasil = nilai_max - pengurang;
    }
    
    return hasil;
}

/*
 * ===========================================================================
 * FUNGSI UTILITAS
 * ===========================================================================
 */

const char *ic_kategori_nama(ic_kategori_t kategori)
{
    switch (kategori) {
    case IC_KATEGORI_CPU:
        return "CPU";
    case IC_KATEGORI_GPU:
        return "GPU";
    case IC_KATEGORI_RAM:
        return "RAM";
    case IC_KATEGORI_ROM:
        return "ROM";
    case IC_KATEGORI_AUDIO:
        return "Audio";
    case IC_KATEGORI_NETWORK:
        return "Network";
    case IC_KATEGORI_STORAGE:
        return "Storage";
    case IC_KATEGORI_DISPLAY:
        return "Display";
    case IC_KATEGORI_INPUT:
        return "Input";
    case IC_KATEGORI_USB:
        return "USB";
    case IC_KATEGORI_BRIDGE:
        return "Bridge";
    case IC_KATEGORI_POWER:
        return "Power";
    case IC_KATEGORI_TIMER:
        return "Timer";
    case IC_KATEGORI_DMA:
        return "DMA";
    case IC_KATEGORI_LAINNYA:
        return "Lainnya";
    default:
        return "Tidak Diketahui";
    }
}

const char *ic_rentang_nama(ic_rentang_t rentang)
{
    switch (rentang) {
    case IC_RENTANG_LOW_END:
        return "Low-End";
    case IC_RENTANG_MID_LOW:
        return "Mid-Low";
    case IC_RENTANG_MID:
        return "Mid-Range";
    case IC_RENTANG_MID_HIGH:
        return "Mid-High";
    case IC_RENTANG_HIGH_END:
        return "High-End";
    case IC_RENTANG_ENTHUSIAST:
        return "Enthusiast";
    case IC_RENTANG_SERVER:
        return "Server";
    default:
        return "Tidak Diketahui";
    }
}

const char *ic_bus_nama(ic_bus_t bus)
{
    switch (bus) {
    case IC_BUS_PCI:
        return "PCI";
    case IC_BUS_PCIE:
        return "PCIe";
    case IC_BUS_USB:
        return "USB";
    case IC_BUS_I2C:
        return "I2C";
    case IC_BUS_SPI:
        return "SPI";
    case IC_BUS_MMIO:
        return "MMIO";
    case IC_BUS_ISA:
        return "ISA";
    case IC_BUS_APB:
        return "APB";
    case IC_BUS_AXI:
        return "AXI";
    default:
        return "Tidak Diketahui";
    }
}

const char *ic_satuan_nama(ic_satuan_t satuan)
{
    switch (satuan) {
    case IC_SATUAN_HZ:
        return "Hz";
    case IC_SATUAN_KHZ:
        return "KHz";
    case IC_SATUAN_MHZ:
        return "MHz";
    case IC_SATUAN_GHZ:
        return "GHz";
    case IC_SATUAN_BYTE:
        return "B";
    case IC_SATUAN_KB:
        return "KB";
    case IC_SATUAN_MB:
        return "MB";
    case IC_SATUAN_GB:
        return "GB";
    case IC_SATUAN_TB:
        return "TB";
    case IC_SATUAN_BIT:
        return "bit";
    case IC_SATUAN_MBPS:
        return "Mbps";
    case IC_SATUAN_GBPS:
        return "Gbps";
    case IC_SATUAN_MW:
        return "mW";
    case IC_SATUAN_W:
        return "W";
    case IC_SATUAN_MV:
        return "mV";
    case IC_SATUAN_V:
        return "V";
    case IC_SATUAN_MA:
        return "mA";
    case IC_SATUAN_A:
        return "A";
    case IC_SATUAN_CELSIUS:
        return "C";
    case IC_SATUAN_NS:
        return "ns";
    case IC_SATUAN_US:
        return "us";
    case IC_SATUAN_MS:
        return "ms";
    case IC_SATUAN_S:
        return "s";
    case IC_SATUAN_CORES:
        return "cores";
    case IC_SATUAN_THREADS:
        return "threads";
    case IC_SATUAN_PIXELS:
        return "pixels";
    case IC_SATUAN_CHANNELS:
        return "channels";
    case IC_SATUAN_PERCENT:
        return "%";
    case IC_SATUAN_DBM:
        return "dBm";
    default:
        return "";
    }
}

const char *ic_status_nama(ic_status_t status)
{
    switch (status) {
    case IC_STATUS_TIDAK_ADA:
        return "Tidak Ada";
    case IC_STATUS_TERDETEKSI:
        return "Terdeteksi";
    case IC_STATUS_DIIDENTIFIKASI:
        return "Teridentifikasi";
    case IC_STATUS_DITEST:
        return "Dites";
    case IC_STATUS_SIAP:
        return "Siap";
    case IC_STATUS_ERROR:
        return "Error";
    case IC_STATUS_TIDAK_DUKUNG:
        return "Tidak Didukung";
    default:
        return "Tidak Diketahui";
    }
}

tak_bertanda32_t ic_jumlah_perangkat(void)
{
    if (!ic_validasi_konteks()) {
        return 0;
    }
    
    return g_ic_konteks.jumlah_perangkat;
}

ic_perangkat_t *ic_perangkat_dapatkan(tak_bertanda32_t indeks)
{
    if (!ic_validasi_konteks()) {
        return NULL;
    }
    
    if (indeks >= g_ic_konteks.jumlah_perangkat) {
        return NULL;
    }
    
    return &g_ic_konteks.perangkat[indeks];
}

void ic_cetak_info(ic_perangkat_t *perangkat)
{
    if (perangkat == NULL) {
        return;
    }
    
    /* Header */
    /* Nama dan Vendor */
    if (perangkat->entri != NULL) {
        /* Format: Nama Vendor Seri */
    } else {
        /* Format: Perangkat [VendorID:DeviceID] */
    }
    
    /* Kategori dan Rentang */
    /* Bus dan Alamat */
    /* Status */
}

void ic_cetak_semua(void)
{
    tak_bertanda32_t i;
    
    if (!ic_validasi_konteks()) {
        return;
    }
    
    /* Header */
    
    for (i = 0; i < g_ic_konteks.jumlah_perangkat; i++) {
        ic_cetak_info(&g_ic_konteks.perangkat[i]);
    }
    
    /* Footer dengan statistik */
}
