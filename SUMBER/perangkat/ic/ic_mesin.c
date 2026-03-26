/*
 * PIGURA OS - IC_MESIN.C
 * ======================
 * Core engine untuk sistem IC Detection.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi inti mesin IC
 * termasuk manajemen memori dan struktur data internal.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "ic.h"

/*
 * ===========================================================================
 * KONSTANTA LOKAL
 * ===========================================================================
 */

/* Ukuran pool memori untuk IC entries */
#define IC_POOL_ENTRI_MAKS 512
#define IC_POOL_PARAM_MAKS 4096

/*
 * ===========================================================================
 * STRUKTUR LOKAL
 * ===========================================================================
 */

/* Pool allocator untuk IC entries */
typedef struct {
    ic_entri_t entri[IC_POOL_ENTRI_MAKS];
    tak_bertanda32_t entri_terpakai;
    bool_t entri_terisi[IC_POOL_ENTRI_MAKS];
} ic_pool_entri_t;

/* Pool allocator untuk parameters */
typedef struct {
    ic_parameter_t param[IC_POOL_PARAM_MAKS];
    tak_bertanda32_t param_terpakai;
    bool_t param_terisi[IC_POOL_PARAM_MAKS];
} ic_pool_param_t;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static ic_pool_entri_t g_pool_entri;
static ic_pool_param_t g_pool_param;
static bool_t g_pool_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * FUNGSI POOL ALLOCATOR
 * ===========================================================================
 */

/*
 * ic_pool_init - Inisialisasi pool memori
 */
status_t ic_pool_init(void)
{
    tak_bertanda32_t i;
    
    if (g_pool_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }
    
    /* Reset pool entri */
    for (i = 0; i < IC_POOL_ENTRI_MAKS; i++) {
        g_pool_entri.entri[i].magic = 0;
        g_pool_entri.entri_terisi[i] = SALAH;
    }
    g_pool_entri.entri_terpakai = 0;
    
    /* Reset pool parameter */
    for (i = 0; i < IC_POOL_PARAM_MAKS; i++) {
        g_pool_param.param[i].id = 0;
        g_pool_param.param_terisi[i] = SALAH;
    }
    g_pool_param.param_terpakai = 0;
    
    g_pool_diinisialisasi = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * ic_pool_entri_alokasi - Alokasikan entri dari pool
 */
ic_entri_t *ic_pool_entri_alokasi(void)
{
    tak_bertanda32_t i;
    
    if (!g_pool_diinisialisasi) {
        return NULL;
    }
    
    /* Cari slot kosong */
    for (i = 0; i < IC_POOL_ENTRI_MAKS; i++) {
        if (!g_pool_entri.entri_terisi[i]) {
            g_pool_entri.entri_terisi[i] = BENAR;
            g_pool_entri.entri_terpakai++;
            
            /* Inisialisasi entri */
            g_pool_entri.entri[i].magic = IC_ENTRI_MAGIC;
            g_pool_entri.entri[i].vendor_id = 0;
            g_pool_entri.entri[i].device_id = 0;
            g_pool_entri.entri[i].ic_id = i;
            g_pool_entri.entri[i].nama[0] = '\0';
            g_pool_entri.entri[i].vendor[0] = '\0';
            g_pool_entri.entri[i].seri[0] = '\0';
            g_pool_entri.entri[i].deskripsi[0] = '\0';
            g_pool_entri.entri[i].kategori = IC_KATEGORI_TIDAK_DIKETAHUI;
            g_pool_entri.entri[i].rentang = IC_RENTANG_TIDAK_DIKETAHUI;
            g_pool_entri.entri[i].bus = IC_BUS_TIDAK_DIKETAHUI;
            g_pool_entri.entri[i].param = NULL;
            g_pool_entri.entri[i].jumlah_param = 0;
            g_pool_entri.entri[i].status = IC_STATUS_TIDAK_ADA;
            g_pool_entri.entri[i].teridentifikasi = SALAH;
            g_pool_entri.entri[i].berikutnya = NULL;
            
            return &g_pool_entri.entri[i];
        }
    }
    
    return NULL;
}

/*
 * ic_pool_entri_bebaskan - Bebaskan entri ke pool
 */
void ic_pool_entri_bebaskan(ic_entri_t *entri)
{
    tak_bertanda32_t indeks;
    
    if (entri == NULL || !g_pool_diinisialisasi) {
        return;
    }
    
    indeks = entri->ic_id;
    
    if (indeks >= IC_POOL_ENTRI_MAKS) {
        return;
    }
    
    if (g_pool_entri.entri_terisi[indeks]) {
        g_pool_entri.entri_terisi[indeks] = SALAH;
        g_pool_entri.entri_terpakai--;
        entri->magic = 0;
    }
}

/*
 * ic_pool_param_alokasi - Alokasikan parameter dari pool
 */
ic_parameter_t *ic_pool_param_alokasi(tak_bertanda32_t jumlah)
{
    tak_bertanda32_t i;
    tak_bertanda32_t mulai = 0;
    tak_bertanda32_t ditemukan = 0;
    
    if (!g_pool_diinisialisasi || jumlah == 0) {
        return NULL;
    }
    
    /* Cari blok kontigu yang cukup */
    for (i = 0; i < IC_POOL_PARAM_MAKS; i++) {
        if (!g_pool_param.param_terisi[i]) {
            if (ditemukan == 0) {
                mulai = i;
            }
            ditemukan++;
            
            if (ditemukan >= jumlah) {
                break;
            }
        } else {
            ditemukan = 0;
        }
    }
    
    if (ditemukan < jumlah) {
        return NULL;
    }
    
    /* Tandai slot sebagai terisi */
    for (i = 0; i < jumlah; i++) {
        g_pool_param.param_terisi[mulai + i] = BENAR;
        g_pool_param.param_terpakai++;
        
        /* Inisialisasi parameter */
        g_pool_param.param[mulai + i].id = i;
        g_pool_param.param[mulai + i].tipe = IC_PARAM_TIPE_TIDAK_DIKETAHUI;
        g_pool_param.param[mulai + i].nama[0] = '\0';
        g_pool_param.param[mulai + i].satuan[0] = '\0';
        g_pool_param.param[mulai + i].nilai_min = 0;
        g_pool_param.param[mulai + i].nilai_max = 0;
        g_pool_param.param[mulai + i].nilai_typical = 0;
        g_pool_param.param[mulai + i].nilai_default = 0;
        g_pool_param.param[mulai + i].toleransi_min = IC_TOLERANSI_DEFAULT_MIN;
        g_pool_param.param[mulai + i].toleransi_max = IC_TOLERANSI_DEFAULT_MAX;
        g_pool_param.param[mulai + i].didukung = SALAH;
        g_pool_param.param[mulai + i].ditest = SALAH;
        g_pool_param.param[mulai + i].flags = 0;
    }
    
    return &g_pool_param.param[mulai];
}

/*
 * ic_pool_param_bebaskan - Bebaskan parameter ke pool
 */
void ic_pool_param_bebaskan(ic_parameter_t *param, tak_bertanda32_t jumlah)
{
    tak_bertanda32_t i;
    tak_bertanda32_t indeks;
    
    if (param == NULL || !g_pool_diinisialisasi || jumlah == 0) {
        return;
    }
    
    indeks = (tak_bertanda32_t)(param - g_pool_param.param);
    
    if (indeks >= IC_POOL_PARAM_MAKS) {
        return;
    }
    
    for (i = 0; i < jumlah && (indeks + i) < IC_POOL_PARAM_MAKS; i++) {
        if (g_pool_param.param_terisi[indeks + i]) {
            g_pool_param.param_terisi[indeks + i] = SALAH;
            g_pool_param.param_terpakai--;
        }
    }
}

/*
 * ===========================================================================
 * FUNGSI UTILITAS STRING
 * ===========================================================================
 */

/*
 * ic_salinnama - Salin nama dengan aman
 */
void ic_salinnama(char *tujuan, const char *sumber, ukuran_t panjang_maks)
{
    ukuran_t i;
    
    if (tujuan == NULL || sumber == NULL || panjang_maks == 0) {
        return;
    }
    
    for (i = 0; i < panjang_maks - 1 && sumber[i] != '\0'; i++) {
        tujuan[i] = sumber[i];
    }
    tujuan[i] = '\0';
}

/*
 * ===========================================================================
 * FUNGSI MANAJEMEN ENTRI
 * ===========================================================================
 */

/*
 * ic_entri_buat - Buat entri IC baru
 */
ic_entri_t *ic_entri_buat(const char *nama, const char *vendor,
                           const char *seri, ic_kategori_t kategori,
                           ic_rentang_t rentang)
{
    ic_entri_t *entri;
    
    entri = ic_pool_entri_alokasi();
    if (entri == NULL) {
        return NULL;
    }
    
    /* Salin string */
    if (nama != NULL) {
        ic_salinnama(entri->nama, nama, IC_NAMA_PANJANG_MAKS);
    }
    if (vendor != NULL) {
        ic_salinnama(entri->vendor, vendor, IC_VENDOR_PANJANG_MAKS);
    }
    if (seri != NULL) {
        ic_salinnama(entri->seri, seri, IC_SERI_PANJANG_MAKS);
    }
    
    entri->kategori = kategori;
    entri->rentang = rentang;
    
    return entri;
}

/*
 * ic_entri_tambah_param - Tambah parameter ke entri
 */
status_t ic_entri_tambah_param(ic_entri_t *entri,
                                 const char *nama,
                                 ic_param_tipe_t tipe,
                                 const char *satuan,
                                 tak_bertanda64_t nilai_min,
                                 tak_bertanda64_t nilai_max)
{
    ic_parameter_t *param_baru;
    ic_parameter_t *param_lama;
    tak_bertanda32_t i;
    
    if (entri == NULL || entri->magic != IC_ENTRI_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Alokasi parameter baru (jumlah saat ini + 1) */
    param_baru = ic_pool_param_alokasi(entri->jumlah_param + 1);
    if (param_baru == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    /* Salin parameter lama jika ada */
    if (entri->param != NULL && entri->jumlah_param > 0) {
        for (i = 0; i < entri->jumlah_param; i++) {
            param_baru[i] = entri->param[i];
        }
        param_lama = entri->param;
    } else {
        param_lama = NULL;
    }
    
    /* Tambah parameter baru di akhir */
    i = entri->jumlah_param;
    param_baru[i].id = i;
    param_baru[i].tipe = tipe;
    if (nama != NULL) {
        ic_salinnama(param_baru[i].nama, nama, IC_NAMA_PANJANG_MAKS);
    }
    if (satuan != NULL) {
        ic_salinnama(param_baru[i].satuan, satuan, IC_SATUAN_PANJANG_MAKS);
    }
    param_baru[i].nilai_min = nilai_min;
    param_baru[i].nilai_max = nilai_max;
    param_baru[i].nilai_typical = nilai_max;
    param_baru[i].nilai_default = ic_hitung_default(nilai_max, 
                                                     IC_TOLERANSI_DEFAULT_MAX);
    param_baru[i].toleransi_min = IC_TOLERANSI_DEFAULT_MIN;
    param_baru[i].toleransi_max = IC_TOLERANSI_DEFAULT_MAX;
    param_baru[i].didukung = BENAR;
    param_baru[i].ditest = SALAH;
    param_baru[i].flags = 0;
    
    /* Update entri */
    entri->param = param_baru;
    entri->jumlah_param++;
    
    /* Bebaskan parameter lama */
    if (param_lama != NULL) {
        ic_pool_param_bebaskan(param_lama, entri->jumlah_param - 1);
    }
    
    return STATUS_BERHASIL;
}

/*
 * ic_entri_cari_param - Cari parameter berdasarkan nama
 */
ic_parameter_t *ic_entri_cari_param(ic_entri_t *entri, const char *nama)
{
    tak_bertanda32_t i;
    ukuran_t panjang;
    ukuran_t j;
    bool_t cocok;
    
    if (entri == NULL || nama == NULL) {
        return NULL;
    }
    
    if (entri->magic != IC_ENTRI_MAGIC) {
        return NULL;
    }
    
    /* Hitung panjang nama yang dicari */
    panjang = 0;
    while (nama[panjang] != '\0' && panjang < IC_NAMA_PANJANG_MAKS) {
        panjang++;
    }
    
    for (i = 0; i < entri->jumlah_param; i++) {
        cocok = BENAR;
        for (j = 0; j < panjang; j++) {
            if (entri->param[i].nama[j] != nama[j]) {
                cocok = SALAH;
                break;
            }
        }
        if (cocok && entri->param[i].nama[panjang] == '\0') {
            return &entri->param[i];
        }
    }
    
    return NULL;
}

/*
 * ic_entri_cari_param_id - Cari parameter berdasarkan ID
 */
ic_parameter_t *ic_entri_cari_param_id(ic_entri_t *entri,
                                        tak_bertanda32_t param_id)
{
    tak_bertanda32_t i;
    
    if (entri == NULL || entri->magic != IC_ENTRI_MAGIC) {
        return NULL;
    }
    
    for (i = 0; i < entri->jumlah_param; i++) {
        if (entri->param[i].id == param_id) {
            return &entri->param[i];
        }
    }
    
    return NULL;
}

/*
 * ===========================================================================
 * FUNGSI STATISTIK
 * ===========================================================================
 */

/*
 * ic_statistik_entri - Dapatkan statistik pool entri
 */
void ic_statistik_entri(tak_bertanda32_t *total, tak_bertanda32_t *terpakai)
{
    if (total != NULL) {
        *total = IC_POOL_ENTRI_MAKS;
    }
    if (terpakai != NULL) {
        *terpakai = g_pool_entri.entri_terpakai;
    }
}

/*
 * ic_statistik_param - Dapatkan statistik pool parameter
 */
void ic_statistik_param(tak_bertanda32_t *total, tak_bertanda32_t *terpakai)
{
    if (total != NULL) {
        *total = IC_POOL_PARAM_MAKS;
    }
    if (terpakai != NULL) {
        *terpakai = g_pool_param.param_terpakai;
    }
}

/*
 * ===========================================================================
 * FUNGSI INISIALISASI MESIN
 * ===========================================================================
 */

/*
 * ic_mesin_init - Inisialisasi mesin IC
 */
status_t ic_mesin_init(void)
{
    status_t hasil;
    
    hasil = ic_pool_init();
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    return STATUS_BERHASIL;
}

/*
 * ic_mesin_shutdown - Matikan mesin IC
 */
void ic_mesin_shutdown(void)
{
    tak_bertanda32_t i;
    
    /* Reset semua entri */
    for (i = 0; i < IC_POOL_ENTRI_MAKS; i++) {
        g_pool_entri.entri_terisi[i] = SALAH;
        g_pool_entri.entri[i].magic = 0;
    }
    g_pool_entri.entri_terpakai = 0;
    
    /* Reset semua parameter */
    for (i = 0; i < IC_POOL_PARAM_MAKS; i++) {
        g_pool_param.param_terisi[i] = SALAH;
    }
    g_pool_param.param_terpakai = 0;
    
    g_pool_diinisialisasi = SALAH;
}
