/*
 * PIGURA OS - IC_VALIDASI.C
 * =========================
 * Sistem validasi parameter untuk IC Detection.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi untuk memvalidasi
 * parameter IC terhadap database dan menentukan nilai aman.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "ic.h"

/*
 * ===========================================================================
 * KONSTANTA VALIDASI
 * ===========================================================================
 */

/* Faktor pengali untuk validasi */
#define IC_VALIDASI_FAKTOR_MIN  0.5   /* 50% dari typical */
#define IC_VALIDASI_FAKTOR_MAX  1.5   /* 150% dari typical */

/* Status validasi */
#define IC_VALIDASI_OK          0
#define IC_VALIDASI_WARN_MIN    1
#define IC_VALIDASI_WARN_MAX    2
#define IC_VALIDASI_ERROR_MIN   3
#define IC_VALIDASI_ERROR_MAX   4

/*
 * ===========================================================================
 * STRUKTUR LOKAL
 * ===========================================================================
 */

/* Hasil validasi */
typedef struct {
    tak_bertanda32_t status;
    tak_bertanda64_t nilai_min_db;
    tak_bertanda64_t nilai_max_db;
    tak_bertanda64_t nilai_min_test;
    tak_bertanda64_t nilai_max_test;
} ic_validasi_hasil_t;

/*
 * ===========================================================================
 * FUNGSI VALIDASI INTERNAL
 * ===========================================================================
 */

/*
 * ic_validasi_rentang - Validasi nilai dalam rentang
 */
static bool_t ic_validasi_rentang(tak_bertanda64_t nilai,
                                   tak_bertanda64_t min,
                                   tak_bertanda64_t maks)
{
    if (nilai < min) {
        return SALAH;
    }
    if (nilai > maks) {
        return SALAH;
    }
    return BENAR;
}

/*
 * ic_validasi_dalam_toleransi - Cek apakah dalam toleransi
 */
static bool_t ic_validasi_dalam_toleransi(tak_bertanda64_t nilai,
                                           tak_bertanda64_t referensi,
                                           tak_bertanda32_t toleransi)
{
    tak_bertanda64_t batas_min;
    tak_bertanda64_t batas_maks;
    tak_bertanda64_t delta;
    tak_bertanda64_t toleransi_nilai;
    
    /* Hitung delta dengan proteksi overflow */
    if (referensi > nilai) {
        delta = referensi - nilai;
    } else {
        delta = nilai - referensi;
    }
    
    /* Hitung nilai toleransi dalam satuan absolut */
    toleransi_nilai = (referensi * toleransi) / 100;
    
    /* Hitung batas toleransi */
    batas_min = referensi - toleransi_nilai;
    if (batas_min > referensi) {
        /* Underflow, gunakan minimum */
        batas_min = 0;
    }
    
    batas_maks = referensi + toleransi_nilai;
    if (batas_maks < referensi) {
        /* Overflow, gunakan maksimum */
        batas_maks = TAK_BERTANDA64_MAX;
    }
    
    /* Cek apakah delta dalam batas toleransi */
    if (delta <= toleransi_nilai) {
        return BENAR;
    }
    
    return ic_validasi_rentang(nilai, batas_min, batas_maks);
}

/*
 * ===========================================================================
 * FUNGSI VALIDASI PARAMETER
 * ===========================================================================
 */

/*
 * ic_validasi_parameter - Validasi satu parameter
 */
status_t ic_validasi_parameter(ic_parameter_t *param,
                                ic_parameter_t *param_db)
{
    ic_validasi_hasil_t hasil;
    
    if (param == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Inisialisasi hasil */
    hasil.status = IC_VALIDASI_OK;
    hasil.nilai_min_db = 0;
    hasil.nilai_max_db = 0;
    hasil.nilai_min_test = param->nilai_min;
    hasil.nilai_max_test = param->nilai_max;
    
    /* Jika ada parameter database, bandingkan */
    if (param_db != NULL) {
        hasil.nilai_min_db = param_db->nilai_min;
        hasil.nilai_max_db = param_db->nilai_max;
        
        /* Validasi nilai minimum */
        if (hasil.nilai_min_test < hasil.nilai_min_db) {
            hasil.status = IC_VALIDASI_WARN_MIN;
        }
        
        /* Validasi nilai maksimum */
        if (hasil.nilai_max_test > hasil.nilai_max_db) {
            hasil.status = IC_VALIDASI_WARN_MAX;
        }
        
        /* Jika di luar toleransi, tandai error */
        if (!ic_validasi_dalam_toleransi(hasil.nilai_min_test,
                                          hasil.nilai_min_db, 50)) {
            hasil.status = IC_VALIDASI_ERROR_MIN;
        }
        
        if (!ic_validasi_dalam_toleransi(hasil.nilai_max_test,
                                          hasil.nilai_max_db, 50)) {
            hasil.status = IC_VALIDASI_ERROR_MAX;
        }
    }
    
    /* Update status parameter */
    switch (hasil.status) {
    case IC_VALIDASI_OK:
        param->flags = 0;
        break;
    case IC_VALIDASI_WARN_MIN:
    case IC_VALIDASI_WARN_MAX:
        param->flags = 1; /* Warning */
        break;
    case IC_VALIDASI_ERROR_MIN:
    case IC_VALIDASI_ERROR_MAX:
        param->flags = 2; /* Error */
        break;
    }
    
    return STATUS_BERHASIL;
}

/*
 * ic_validasi_semua_parameter - Validasi semua parameter perangkat
 */
status_t ic_validasi_semua_parameter(ic_perangkat_t *perangkat)
{
    tak_bertanda32_t i;
    ic_parameter_t *param;
    ic_parameter_t *param_db;
    status_t hasil;
    tak_bertanda32_t jumlah_error = 0;
    tak_bertanda32_t jumlah_warn = 0;
    
    if (perangkat == NULL || perangkat->entri == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    for (i = 0; i < perangkat->entri->jumlah_param; i++) {
        param = &perangkat->entri->param[i];
        
        /* Cari parameter di database */
        param_db = NULL;
        if (perangkat->entri != NULL) {
            param_db = ic_entri_cari_param_id(perangkat->entri, param->id);
        }
        
        hasil = ic_validasi_parameter(param, param_db);
        if (hasil != STATUS_BERHASIL) {
            jumlah_error++;
        }
        
        if (param->flags == 1) {
            jumlah_warn++;
        } else if (param->flags == 2) {
            jumlah_error++;
        }
    }
    
    /* Update status perangkat */
    if (jumlah_error > 0) {
        perangkat->status = IC_STATUS_ERROR;
        return STATUS_GAGAL;
    }
    
    perangkat->status = IC_STATUS_SIAP;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PERHITUNGAN NILAI AMAN
 * ===========================================================================
 */

/*
 * ic_hitung_nilai_aman - Hitung nilai aman dari hasil test
 */
tak_bertanda64_t ic_hitung_nilai_aman(tak_bertanda64_t nilai_test,
                                       tak_bertanda64_t nilai_db_min,
                                       tak_bertanda64_t nilai_db_max,
                                       tak_bertanda32_t toleransi)
{
    tak_bertanda64_t nilai_aman;
    tak_bertanda64_t batas_aman;
    
    /* Hitung nilai aman dengan toleransi */
    nilai_aman = ic_hitung_default(nilai_test, toleransi);
    
    /* Pastikan dalam batas database */
    batas_aman = ic_hitung_default(nilai_db_max, toleransi);
    
    /* Gunakan nilai yang lebih kecil */
    if (nilai_aman > batas_aman) {
        nilai_aman = batas_aman;
    }
    
    /* Pastikan tidak di bawah minimum */
    if (nilai_aman < nilai_db_min) {
        nilai_aman = nilai_db_min;
    }
    
    return nilai_aman;
}

/*
 * ic_perbarui_nilai_aman - Perbarui semua parameter dengan nilai aman
 */
status_t ic_perbarui_nilai_aman(ic_perangkat_t *perangkat)
{
    tak_bertanda32_t i;
    ic_parameter_t *param;
    tak_bertanda64_t nilai_aman;
    
    if (perangkat == NULL || perangkat->entri == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    for (i = 0; i < perangkat->entri->jumlah_param; i++) {
        param = &perangkat->entri->param[i];
        
        /* Hitung nilai aman */
        nilai_aman = ic_hitung_nilai_aman(param->nilai_max,
                                           param->nilai_min,
                                           param->nilai_max,
                                           param->toleransi_max);
        
        /* Update nilai default */
        param->nilai_default = nilai_aman;
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI LAPORAN VALIDASI
 * ===========================================================================
 */

/*
 * ic_validasi_laporan - Buat laporan validasi perangkat
 */
status_t ic_validasi_laporan(ic_perangkat_t *perangkat, char *buffer,
                              ukuran_t ukuran_buffer)
{
    tak_bertanda32_t i;
    ic_parameter_t *param;
    ukuran_t posisi = 0;
    ukuran_t ditulis;
    
    if (perangkat == NULL || buffer == NULL || ukuran_buffer == 0) {
        return STATUS_PARAM_NULL;
    }
    
    /* Header laporan */
    ditulis = 0;
    while (ditulis < ukuran_buffer - 1 && 
           "Laporan Validasi IC\n"[ditulis] != '\0') {
        buffer[posisi++] = "Laporan Validasi IC\n"[ditulis++];
    }
    
    /* Info perangkat */
    if (perangkat->entri != NULL) {
        /* Nama dan vendor */
    }
    
    /* Status validasi */
    for (i = 0; i < perangkat->entri->jumlah_param; i++) {
        param = &perangkat->entri->param[i];
        
        if (posisi >= ukuran_buffer - 64) {
            break;
        }
        
        /* Nama parameter */
        ditulis = 0;
        while (param->nama[ditulis] != '\0' && posisi < ukuran_buffer - 1) {
            buffer[posisi++] = param->nama[ditulis++];
        }
        buffer[posisi++] = ':';
        buffer[posisi++] = ' ';
        
        /* Status */
        if (param->flags == 0) {
            buffer[posisi++] = 'O';
            buffer[posisi++] = 'K';
        } else if (param->flags == 1) {
            buffer[posisi++] = 'W';
            buffer[posisi++] = 'A';
            buffer[posisi++] = 'R';
            buffer[posisi++] = 'N';
        } else {
            buffer[posisi++] = 'E';
            buffer[posisi++] = 'R';
            buffer[posisi++] = 'R';
            buffer[posisi++] = 'O';
            buffer[posisi++] = 'R';
        }
        buffer[posisi++] = '\n';
    }
    
    buffer[posisi] = '\0';
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PENGECEKAN KEAMANAN
 * ===========================================================================
 */

/*
 * ic_cek_keamanan_param - Cek apakah parameter aman digunakan
 */
bool_t ic_cek_keamanan_param(ic_parameter_t *param)
{
    if (param == NULL) {
        return SALAH;
    }
    
    /* Parameter dengan flag error tidak aman */
    if (param->flags == 2) {
        return SALAH;
    }
    
    /* Nilai default harus dalam rentang */
    if (!ic_validasi_rentang(param->nilai_default,
                              param->nilai_min,
                              param->nilai_max)) {
        return SALAH;
    }
    
    return BENAR;
}

/*
 * ic_cek_keamanan_perangkat - Cek apakah perangkat aman digunakan
 */
bool_t ic_cek_keamanan_perangkat(ic_perangkat_t *perangkat)
{
    tak_bertanda32_t i;
    
    if (perangkat == NULL || perangkat->entri == NULL) {
        return SALAH;
    }
    
    /* Status harus SIAP */
    if (perangkat->status != IC_STATUS_SIAP) {
        return SALAH;
    }
    
    /* Semua parameter harus aman */
    for (i = 0; i < perangkat->entri->jumlah_param; i++) {
        if (!ic_cek_keamanan_param(&perangkat->entri->param[i])) {
            return SALAH;
        }
    }
    
    return BENAR;
}

/*
 * ===========================================================================
 * FUNGSI UTILITASI
 * ===========================================================================
 */

/*
 * ic_validasi_param_nama - Validasi nama parameter
 */
bool_t ic_validasi_param_nama(const char *nama)
{
    ukuran_t panjang;
    
    if (nama == NULL) {
        return SALAH;
    }
    
    panjang = 0;
    while (nama[panjang] != '\0') {
        if (panjang >= IC_NAMA_PANJANG_MAKS) {
            return SALAH;
        }
        panjang++;
    }
    
    if (panjang == 0) {
        return SALAH;
    }
    
    return BENAR;
}

/*
 * ic_validasi_satuan - Validasi satuan parameter
 */
bool_t ic_validasi_satuan(const char *satuan)
{
    ukuran_t panjang;
    
    if (satuan == NULL) {
        return SALAH; /* Satuan boleh kosong */
    }
    
    panjang = 0;
    while (satuan[panjang] != '\0') {
        if (panjang >= IC_SATUAN_PANJANG_MAKS) {
            return SALAH;
        }
        panjang++;
    }
    
    return BENAR;
}
