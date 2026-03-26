/*
 * PIGURA OS - DATABASE/IC_DATABASE.C
 * ==================================
 * Core database untuk IC Detection.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi untuk mengelola
 * database IC yang berisi entri dari berbagai kategori.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../ic.h"

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

/* Head dari linked list database */
static ic_entri_t *g_db_head = NULL;
static tak_bertanda32_t g_db_jumlah = 0;

/* Flag inisialisasi */
static bool_t g_db_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * FUNGSI INISIALISASI DATABASE
 * ===========================================================================
 */

/*
 * ic_database_init - Inisialisasi database IC
 */
status_t ic_database_init(void)
{
    if (g_db_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }
    
    g_db_head = NULL;
    g_db_jumlah = 0;
    g_db_diinisialisasi = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * ic_database_tambah - Tambah entri ke database
 */
status_t ic_database_tambah(ic_entri_t *entri)
{
    if (entri == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (entri->magic != IC_ENTRI_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Tambah di awal linked list */
    entri->berikutnya = g_db_head;
    g_db_head = entri;
    g_db_jumlah++;
    
    /* Update konteks global */
    if (g_ic_konteks.magic == IC_MAGIC) {
        g_ic_konteks.database = g_db_head;
        g_ic_konteks.jumlah_entri = g_db_jumlah;
    }
    
    return STATUS_BERHASIL;
}

/*
 * ic_database_cari_id - Cari entri berdasarkan Vendor ID dan Device ID
 */
ic_entri_t *ic_database_cari_id(tak_bertanda16_t vendor_id,
                                tak_bertanda16_t device_id)
{
    ic_entri_t *entri;
    
    if (!g_db_diinisialisasi) {
        return NULL;
    }
    
    entri = g_db_head;
    while (entri != NULL) {
        if (entri->vendor_id == vendor_id &&
            entri->device_id == device_id) {
            return entri;
        }
        entri = entri->berikutnya;
    }
    
    return NULL;
}

/*
 * ic_database_cari_kategori - Cari entri pertama berdasarkan kategori
 */
ic_entri_t *ic_database_cari_kategori(ic_kategori_t kategori)
{
    ic_entri_t *entri;
    
    if (!g_db_diinisialisasi) {
        return NULL;
    }
    
    entri = g_db_head;
    while (entri != NULL) {
        if (entri->kategori == kategori) {
            return entri;
        }
        entri = entri->berikutnya;
    }
    
    return NULL;
}

/*
 * ic_database_cari_rentang - Cari entri berdasarkan rentang
 */
ic_entri_t *ic_database_cari_rentang(ic_rentang_t rentang)
{
    ic_entri_t *entri;
    
    if (!g_db_diinisialisasi) {
        return NULL;
    }
    
    entri = g_db_head;
    while (entri != NULL) {
        if (entri->rentang == rentang) {
            return entri;
        }
        entri = entri->berikutnya;
    }
    
    return NULL;
}

/*
 * ic_database_jumlah - Dapatkan jumlah entri database
 */
tak_bertanda32_t ic_database_jumlah(void)
{
    return g_db_jumlah;
}

/*
 * ic_database_head - Dapatkan pointer ke entri pertama
 */
ic_entri_t *ic_database_head(void)
{
    return g_db_head;
}

/*
 * ===========================================================================
 * FUNGSI HELPER UNTUK MEMBUAT ENTRI
 * ===========================================================================
 */

/*
 * ic_buat_entri_cpu - Buat entri CPU
 */
ic_entri_t *ic_buat_entri_cpu(tak_bertanda16_t vendor_id,
                               tak_bertanda16_t device_id,
                               const char *nama,
                               const char *vendor,
                               const char *seri,
                               ic_rentang_t rentang)
{
    ic_entri_t *entri;
    status_t hasil;
    
    entri = ic_entri_buat(nama, vendor, seri, IC_KATEGORI_CPU, rentang);
    if (entri == NULL) {
        return NULL;
    }
    
    entri->vendor_id = vendor_id;
    entri->device_id = device_id;
    entri->bus = IC_BUS_PCI;
    
    /* Inisialisasi parameter CPU */
    hasil = ic_parameter_inisialisasi_kategori(entri);
    if (hasil != STATUS_BERHASIL) {
        ic_pool_entri_bebaskan(entri);
        return NULL;
    }
    
    return entri;
}

/*
 * ic_buat_entri_gpu - Buat entri GPU
 */
ic_entri_t *ic_buat_entri_gpu(tak_bertanda16_t vendor_id,
                               tak_bertanda16_t device_id,
                               const char *nama,
                               const char *vendor,
                               const char *seri,
                               ic_rentang_t rentang)
{
    ic_entri_t *entri;
    status_t hasil;
    
    entri = ic_entri_buat(nama, vendor, seri, IC_KATEGORI_GPU, rentang);
    if (entri == NULL) {
        return NULL;
    }
    
    entri->vendor_id = vendor_id;
    entri->device_id = device_id;
    entri->bus = IC_BUS_PCI;
    
    /* Inisialisasi parameter GPU */
    hasil = ic_parameter_inisialisasi_kategori(entri);
    if (hasil != STATUS_BERHASIL) {
        ic_pool_entri_bebaskan(entri);
        return NULL;
    }
    
    return entri;
}

/*
 * ic_buat_entri_storage - Buat entri storage
 */
ic_entri_t *ic_buat_entri_storage(tak_bertanda16_t vendor_id,
                                   tak_bertanda16_t device_id,
                                   const char *nama,
                                   const char *vendor,
                                   const char *seri,
                                   ic_rentang_t rentang)
{
    ic_entri_t *entri;
    status_t hasil;
    
    entri = ic_entri_buat(nama, vendor, seri, IC_KATEGORI_STORAGE, rentang);
    if (entri == NULL) {
        return NULL;
    }
    
    entri->vendor_id = vendor_id;
    entri->device_id = device_id;
    entri->bus = IC_BUS_PCI;
    
    /* Inisialisasi parameter storage */
    hasil = ic_parameter_inisialisasi_kategori(entri);
    if (hasil != STATUS_BERHASIL) {
        ic_pool_entri_bebaskan(entri);
        return NULL;
    }
    
    return entri;
}

/*
 * ic_buat_entri_network - Buat entri network
 */
ic_entri_t *ic_buat_entri_network(tak_bertanda16_t vendor_id,
                                   tak_bertanda16_t device_id,
                                   const char *nama,
                                   const char *vendor,
                                   const char *seri,
                                   ic_rentang_t rentang)
{
    ic_entri_t *entri;
    status_t hasil;
    
    entri = ic_entri_buat(nama, vendor, seri, IC_KATEGORI_NETWORK, rentang);
    if (entri == NULL) {
        return NULL;
    }
    
    entri->vendor_id = vendor_id;
    entri->device_id = device_id;
    entri->bus = IC_BUS_PCI;
    
    /* Inisialisasi parameter network */
    hasil = ic_parameter_inisialisasi_kategori(entri);
    if (hasil != STATUS_BERHASIL) {
        ic_pool_entri_bebaskan(entri);
        return NULL;
    }
    
    return entri;
}

/*
 * ===========================================================================
 * FUNGSI MUAT DATABASE
 * ===========================================================================
 */

/* Forward declarations untuk loader kategori */
extern status_t ic_muat_cpu_database(void);
extern status_t ic_muat_gpu_database(void);
extern status_t ic_muat_network_database(void);
extern status_t ic_muat_storage_database(void);
extern status_t ic_muat_audio_database(void);
extern status_t ic_muat_display_database(void);
extern status_t ic_muat_input_database(void);
extern status_t ic_muat_usb_database(void);

/*
 * ic_database_muat - Muat database dari semua kategori
 */
status_t ic_database_muat(void)
{
    status_t hasil;
    
    /* Muat database CPU */
    hasil = ic_muat_cpu_database();
    if (hasil != STATUS_BERHASIL) {
        /* Lanjutkan meskipun ada yang gagal */
    }
    
    /* Muat database GPU */
    hasil = ic_muat_gpu_database();
    
    /* Muat database Network */
    hasil = ic_muat_network_database();
    
    /* Muat database Storage */
    hasil = ic_muat_storage_database();
    
    /* Muat database Audio */
    hasil = ic_muat_audio_database();
    
    /* Muat database Display */
    hasil = ic_muat_display_database();
    
    /* Muat database Input */
    hasil = ic_muat_input_database();
    
    /* Muat database USB */
    hasil = ic_muat_usb_database();
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI STATISTIK
 * ===========================================================================
 */

/*
 * ic_database_statistik - Dapatkan statistik database
 */
void ic_database_statistik(tak_bertanda32_t *total,
                            tak_bertanda32_t *per_kategori)
{
    ic_entri_t *entri;
    tak_bertanda32_t count[IC_MAKS_KATEGORI];
    tak_bertanda32_t i;
    
    /* Reset counter */
    for (i = 0; i < IC_MAKS_KATEGORI; i++) {
        count[i] = 0;
    }
    
    /* Hitung per kategori */
    entri = g_db_head;
    while (entri != NULL) {
        if (entri->kategori < IC_MAKS_KATEGORI) {
            count[entri->kategori]++;
        }
        entri = entri->berikutnya;
    }
    
    /* Return hasil */
    if (total != NULL) {
        *total = g_db_jumlah;
    }
    
    if (per_kategori != NULL) {
        for (i = 0; i < IC_MAKS_KATEGORI; i++) {
            per_kategori[i] = count[i];
        }
    }
}
