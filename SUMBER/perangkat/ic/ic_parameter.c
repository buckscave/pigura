/*
 * PIGURA OS - IC_PARAMETER.C
 * ==========================
 * Sistem parameter untuk IC Detection.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi untuk mengelola
 * parameter IC termasuk test dan perhitungan nilai default.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "ic.h"

/*
 * ===========================================================================
 * KONSTANTA PARAMETER
 * ===========================================================================
 */

/* ID parameter standar */
#define IC_PARAM_ID_FREKUENSI_CORE      1
#define IC_PARAM_ID_FREKUENSI_MAX       2
#define IC_PARAM_ID_JUMLAH_CORE         3
#define IC_PARAM_ID_JUMLAH_THREAD       4
#define IC_PARAM_ID_CACHE_L1            5
#define IC_PARAM_ID_CACHE_L2            6
#define IC_PARAM_ID_CACHE_L3            7
#define IC_PARAM_ID_TDP                 8
#define IC_PARAM_ID_TEGANGAN            9
#define IC_PARAM_ID_TEMPERATUR_MAX      10

#define IC_PARAM_ID_GPU_MEMORI          20
#define IC_PARAM_ID_GPU_CORE_CLOCK      21
#define IC_PARAM_ID_GPU_MEM_CLOCK       22
#define IC_PARAM_ID_GPU_SHADERS         23
#define IC_PARAM_ID_GPU_TMUS            24
#define IC_PARAM_ID_GPU_ROPS            25

#define IC_PARAM_ID_RAM_KAPASITAS       30
#define IC_PARAM_ID_RAM_FREKUENSI       31
#define IC_PARAM_ID_RAM_LATENSI         32

#define IC_PARAM_ID_STORAGE_KAPASITAS   40
#define IC_PARAM_ID_STORAGE_KECEPATAN   41
#define IC_PARAM_ID_STORAGE_LATENSI     42
#define IC_PARAM_ID_STORAGE_QUEUE       43

#define IC_PARAM_ID_NET_KECEPATAN       50
#define IC_PARAM_ID_NET_DUPLEX          51

#define IC_PARAM_ID_AUDIO_CHANNEL       60
#define IC_PARAM_ID_AUDIO_SAMPLE_RATE   61
#define IC_PARAM_ID_AUDIO_BIT_DEPTH     62

/* Waktu timeout untuk test (dalam milidetik) */
#define IC_TEST_TIMEOUT_DEFAULT  1000
#define IC_TEST_TIMEOUT_CPU      5000
#define IC_TEST_TIMEOUT_GPU      3000
#define IC_TEST_TIMEOUT_STORAGE  2000
#define IC_TEST_TIMEOUT_NETWORK  1000

/*
 * ===========================================================================
 * FUNGSI INISIALISASI PARAMETER
 * ===========================================================================
 */

/*
 * ic_param_cpu_inisialisasi - Inisialisasi parameter CPU
 */
status_t ic_param_cpu_inisialisasi(ic_entri_t *entri)
{
    status_t hasil;
    
    if (entri == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Frekuensi Core (MHz) */
    hasil = ic_entri_tambah_param(entri, "frekuensi_core",
                                    IC_PARAM_TIPE_FREKUENSI, "MHz",
                                    100, 5000);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Frekuensi Maksimum (MHz) */
    hasil = ic_entri_tambah_param(entri, "frekuensi_max",
                                    IC_PARAM_TIPE_FREKUENSI, "MHz",
                                    100, 6000);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Jumlah Core */
    hasil = ic_entri_tambah_param(entri, "jumlah_core",
                                    IC_PARAM_TIPE_JUMLAH, "cores",
                                    1, 128);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Jumlah Thread */
    hasil = ic_entri_tambah_param(entri, "jumlah_thread",
                                    IC_PARAM_TIPE_JUMLAH, "threads",
                                    1, 256);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Cache L1 (KB) */
    hasil = ic_entri_tambah_param(entri, "cache_l1",
                                    IC_PARAM_TIPE_KAPASITAS, "KB",
                                    8, 512);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Cache L2 (KB) */
    hasil = ic_entri_tambah_param(entri, "cache_l2",
                                    IC_PARAM_TIPE_KAPASITAS, "KB",
                                    64, 65536);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Cache L3 (KB) */
    hasil = ic_entri_tambah_param(entri, "cache_l3",
                                    IC_PARAM_TIPE_KAPASITAS, "KB",
                                    0, 524288);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* TDP (Watt) */
    hasil = ic_entri_tambah_param(entri, "tdp",
                                    IC_PARAM_TIPE_DAYA, "W",
                                    5, 350);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Temperatur Maksimum (Celsius) */
    hasil = ic_entri_tambah_param(entri, "temperatur_max",
                                    IC_PARAM_TIPE_SUHU, "C",
                                    60, 110);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    return STATUS_BERHASIL;
}

/*
 * ic_param_gpu_inisialisasi - Inisialisasi parameter GPU
 */
status_t ic_param_gpu_inisialisasi(ic_entri_t *entri)
{
    status_t hasil;
    
    if (entri == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Memori VRAM (MB) */
    hasil = ic_entri_tambah_param(entri, "memori_vram",
                                    IC_PARAM_TIPE_KAPASITAS, "MB",
                                    64, 32768);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Core Clock (MHz) */
    hasil = ic_entri_tambah_param(entri, "core_clock",
                                    IC_PARAM_TIPE_FREKUENSI, "MHz",
                                    300, 3000);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Memory Clock (MHz) */
    hasil = ic_entri_tambah_param(entri, "memory_clock",
                                    IC_PARAM_TIPE_FREKUENSI, "MHz",
                                    500, 24000);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Jumlah Shaders */
    hasil = ic_entri_tambah_param(entri, "jumlah_shader",
                                    IC_PARAM_TIPE_JUMLAH, "units",
                                    64, 16384);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Jumlah TMUs */
    hasil = ic_entri_tambah_param(entri, "jumlah_tmu",
                                    IC_PARAM_TIPE_JUMLAH, "units",
                                    8, 512);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Jumlah ROPs */
    hasil = ic_entri_tambah_param(entri, "jumlah_rop",
                                    IC_PARAM_TIPE_JUMLAH, "units",
                                    4, 256);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* TDP (Watt) */
    hasil = ic_entri_tambah_param(entri, "tdp",
                                    IC_PARAM_TIPE_DAYA, "W",
                                    10, 600);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    return STATUS_BERHASIL;
}

/*
 * ic_param_ram_inisialisasi - Inisialisasi parameter RAM
 */
status_t ic_param_ram_inisialisasi(ic_entri_t *entri)
{
    status_t hasil;
    
    if (entri == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Kapasitas (MB) */
    hasil = ic_entri_tambah_param(entri, "kapasitas",
                                    IC_PARAM_TIPE_KAPASITAS, "MB",
                                    16, 1048576);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Frekuensi (MHz) */
    hasil = ic_entri_tambah_param(entri, "frekuensi",
                                    IC_PARAM_TIPE_FREKUENSI, "MHz",
                                    100, 6400);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Latency CAS */
    hasil = ic_entri_tambah_param(entri, "latency_cas",
                                    IC_PARAM_TIPE_LATENSI, "cycles",
                                    4, 40);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Lebar Bit */
    hasil = ic_entri_tambah_param(entri, "lebar_bit",
                                    IC_PARAM_TIPE_LEBAR_BIT, "bit",
                                    8, 256);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    return STATUS_BERHASIL;
}

/*
 * ic_param_storage_inisialisasi - Inisialisasi parameter storage
 */
status_t ic_param_storage_inisialisasi(ic_entri_t *entri)
{
    status_t hasil;
    
    if (entri == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Kapasitas (MB) */
    hasil = ic_entri_tambah_param(entri, "kapasitas",
                                    IC_PARAM_TIPE_KAPASITAS, "MB",
                                    8, 32768000);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Kecepatan Baca Sequential (MB/s) */
    hasil = ic_entri_tambah_param(entri, "kecepatan_baca",
                                    IC_PARAM_TIPE_KECEPATAN, "MB/s",
                                    1, 14000);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Kecepatan Tulis Sequential (MB/s) */
    hasil = ic_entri_tambah_param(entri, "kecepatan_tulis",
                                    IC_PARAM_TIPE_KECEPATAN, "MB/s",
                                    1, 12000);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Latency (us) */
    hasil = ic_entri_tambah_param(entri, "latency",
                                    IC_PARAM_TIPE_LATENSI, "us",
                                    1, 20000);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Queue Depth */
    hasil = ic_entri_tambah_param(entri, "queue_depth",
                                    IC_PARAM_TIPE_JUMLAH, "entries",
                                    1, 65536);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* IOPS */
    hasil = ic_entri_tambah_param(entri, "iops",
                                    IC_PARAM_TIPE_JUMLAH, "ops",
                                    50, 2000000);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    return STATUS_BERHASIL;
}

/*
 * ic_param_network_inisialisasi - Inisialisasi parameter network
 */
status_t ic_param_network_inisialisasi(ic_entri_t *entri)
{
    status_t hasil;
    
    if (entri == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Kecepatan (Mbps) */
    hasil = ic_entri_tambah_param(entri, "kecepatan",
                                    IC_PARAM_TIPE_KECEPATAN, "Mbps",
                                    1, 100000);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Duplex */
    hasil = ic_entri_tambah_param(entri, "duplex",
                                    IC_PARAM_TIPE_JUMLAH, "mode",
                                    1, 2);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* TX Power (dBm) - untuk WiFi */
    hasil = ic_entri_tambah_param(entri, "tx_power",
                                    IC_PARAM_TIPE_TEGANGAN, "dBm",
                                    0, 30);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    return STATUS_BERHASIL;
}

/*
 * ic_param_audio_inisialisasi - Inisialisasi parameter audio
 */
status_t ic_param_audio_inisialisasi(ic_entri_t *entri)
{
    status_t hasil;
    
    if (entri == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Jumlah Channel */
    hasil = ic_entri_tambah_param(entri, "jumlah_channel",
                                    IC_PARAM_TIPE_JUMLAH, "channels",
                                    1, 16);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Sample Rate (Hz) */
    hasil = ic_entri_tambah_param(entri, "sample_rate",
                                    IC_PARAM_TIPE_FREKUENSI, "Hz",
                                    8000, 192000);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Bit Depth */
    hasil = ic_entri_tambah_param(entri, "bit_depth",
                                    IC_PARAM_TIPE_LEBAR_BIT, "bit",
                                    8, 32);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI TEST PARAMETER
 * ===========================================================================
 */

/*
 * ic_test_cpu - Test parameter CPU
 */
status_t ic_test_cpu(ic_perangkat_t *perangkat)
{
    ic_parameter_t *param;
    tak_bertanda64_t nilai;
    
    if (perangkat == NULL || perangkat->entri == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Test frekuensi core */
    param = ic_entri_cari_param(perangkat->entri, "frekuensi_core");
    if (param != NULL) {
        nilai = 2400; /* Nilai test default */
        param->nilai_min = nilai;
        param->nilai_max = nilai;
        param->nilai_typical = nilai;
        param->nilai_default = ic_hitung_default(nilai, IC_TOLERANSI_DEFAULT_MAX);
        param->ditest = BENAR;
    }
    
    /* Test jumlah core */
    param = ic_entri_cari_param(perangkat->entri, "jumlah_core");
    if (param != NULL) {
        nilai = 4; /* Nilai test default */
        param->nilai_min = nilai;
        param->nilai_max = nilai;
        param->nilai_typical = nilai;
        param->nilai_default = nilai;
        param->ditest = BENAR;
    }
    
    perangkat->ditest = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * ic_test_gpu - Test parameter GPU
 */
status_t ic_test_gpu(ic_perangkat_t *perangkat)
{
    ic_parameter_t *param;
    tak_bertanda64_t nilai;
    
    if (perangkat == NULL || perangkat->entri == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Test memori VRAM */
    param = ic_entri_cari_param(perangkat->entri, "memori_vram");
    if (param != NULL) {
        nilai = 8192; /* Nilai test default */
        param->nilai_min = nilai;
        param->nilai_max = nilai;
        param->nilai_typical = nilai;
        param->nilai_default = ic_hitung_default(nilai, IC_TOLERANSI_DEFAULT_MAX);
        param->ditest = BENAR;
    }
    
    /* Test core clock */
    param = ic_entri_cari_param(perangkat->entri, "core_clock");
    if (param != NULL) {
        nilai = 1500; /* Nilai test default */
        param->nilai_min = nilai;
        param->nilai_max = nilai;
        param->nilai_typical = nilai;
        param->nilai_default = ic_hitung_default(nilai, IC_TOLERANSI_DEFAULT_MAX);
        param->ditest = BENAR;
    }
    
    perangkat->ditest = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * ic_test_storage - Test parameter storage
 */
status_t ic_test_storage(ic_perangkat_t *perangkat)
{
    ic_parameter_t *param;
    tak_bertanda64_t nilai;
    
    if (perangkat == NULL || perangkat->entri == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Test kapasitas */
    param = ic_entri_cari_param(perangkat->entri, "kapasitas");
    if (param != NULL) {
        nilai = 512000; /* 500 GB test default */
        param->nilai_min = nilai;
        param->nilai_max = nilai;
        param->nilai_typical = nilai;
        param->nilai_default = nilai;
        param->ditest = BENAR;
    }
    
    /* Test kecepatan baca */
    param = ic_entri_cari_param(perangkat->entri, "kecepatan_baca");
    if (param != NULL) {
        nilai = 3500; /* MB/s test default */
        param->nilai_min = nilai;
        param->nilai_max = nilai;
        param->nilai_typical = nilai;
        param->nilai_default = ic_hitung_default(nilai, IC_TOLERANSI_DEFAULT_MAX);
        param->ditest = BENAR;
    }
    
    perangkat->ditest = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI UTAMA
 * ===========================================================================
 */

/*
 * ic_parameter_inisialisasi_kategori - Inisialisasi parameter per kategori
 */
status_t ic_parameter_inisialisasi_kategori(ic_entri_t *entri)
{
    if (entri == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    switch (entri->kategori) {
    case IC_KATEGORI_CPU:
        return ic_param_cpu_inisialisasi(entri);
        
    case IC_KATEGORI_GPU:
        return ic_param_gpu_inisialisasi(entri);
        
    case IC_KATEGORI_RAM:
        return ic_param_ram_inisialisasi(entri);
        
    case IC_KATEGORI_STORAGE:
        return ic_param_storage_inisialisasi(entri);
        
    case IC_KATEGORI_NETWORK:
        return ic_param_network_inisialisasi(entri);
        
    case IC_KATEGORI_AUDIO:
        return ic_param_audio_inisialisasi(entri);
        
    default:
        return STATUS_BERHASIL;
    }
}

/*
 * ic_parameter_test_perangkat - Test parameter berdasarkan kategori
 */
status_t ic_parameter_test_perangkat(ic_perangkat_t *perangkat)
{
    if (perangkat == NULL || perangkat->entri == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    switch (perangkat->entri->kategori) {
    case IC_KATEGORI_CPU:
        return ic_test_cpu(perangkat);
        
    case IC_KATEGORI_GPU:
        return ic_test_gpu(perangkat);
        
    case IC_KATEGORI_STORAGE:
        return ic_test_storage(perangkat);
        
    default:
        perangkat->ditest = BENAR;
        return STATUS_BERHASIL;
    }
}

/*
 * ic_parameter_dapatkan_nilai - Dapatkan nilai parameter
 */
tak_bertanda64_t ic_parameter_dapatkan_nilai(ic_perangkat_t *perangkat,
                                             const char *nama_param)
{
    ic_parameter_t *param;
    
    if (perangkat == NULL || perangkat->entri == NULL || nama_param == NULL) {
        return 0;
    }
    
    param = ic_entri_cari_param(perangkat->entri, nama_param);
    if (param == NULL) {
        return 0;
    }
    
    return param->nilai_default;
}

/*
 * ic_parameter_set_nilai - Set nilai parameter
 */
status_t ic_parameter_set_nilai(ic_perangkat_t *perangkat,
                                 const char *nama_param,
                                 tak_bertanda64_t nilai)
{
    ic_parameter_t *param;
    
    if (perangkat == NULL || perangkat->entri == NULL || nama_param == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    param = ic_entri_cari_param(perangkat->entri, nama_param);
    if (param == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Validasi nilai dalam rentang */
    if (nilai < param->nilai_min || nilai > param->nilai_max) {
        return STATUS_PARAM_INVALID;
    }
    
    param->nilai_default = nilai;
    
    return STATUS_BERHASIL;
}
