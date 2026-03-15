/*******************************************************************************
 * KLIP.C - Implementasi Clipboard Manager
 *
 * Modul ini menangani penyimpanan dan pengambilan data clipboard.
 *
 * Copyright (c) 2024 Pigura OS Project
 ******************************************************************************/

#include "pigura.h"
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
 * FUNGSI IMPLEMENTASI
 ******************************************************************************/

/* Inisialisasi manager clipboard */
int klip_init(ManagerKlip *klip, size_t ukuran_default, size_t ukuran_maks) {
    int i;
    
    if (!klip) return 0;
    
    /* Validasi ukuran */
    if (ukuran_default < 256) ukuran_default = KLIP_UKURAN_DEFAULT;
    if (ukuran_maks < ukuran_default) ukuran_maks = ukuran_default * 2;
    if (ukuran_maks > KLIP_UKURAN_MAKS) ukuran_maks = KLIP_UKURAN_MAKS;
    
    /* Inisialisasi manager */
    klip->jumlah = 0;
    klip->saat_ini = -1;
    klip->id_berikutnya = 1;
    klip->ukuran_default = ukuran_default;
    klip->ukuran_maks = ukuran_maks;
    klip->auto_hapus = 1;
    
    /* Inisialisasi entri */
    for (i = 0; i < KLIP_ENTRI_MAKS; i++) {
        klip->entri[i].data = NULL;
        klip->entri[i].ukuran = 0;
        klip->entri[i].kapasitas = 0;
        klip->entri[i].tipe = 0;
        klip->entri[i].id = 0;
    }
    
    return 1;
}

/* Bebaskan manager clipboard */
void klip_bebas(ManagerKlip *klip) {
    int i;
    
    if (!klip) return;
    
    for (i = 0; i < klip->jumlah; i++) {
        if (klip->entri[i].data) {
            free(klip->entri[i].data);
            klip->entri[i].data = NULL;
        }
        klip->entri[i].ukuran = 0;
        klip->entri[i].kapasitas = 0;
        klip->entri[i].tipe = 0;
        klip->entri[i].id = 0;
    }
    
    klip->jumlah = 0;
    klip->saat_ini = -1;
}

/* Hapus entri tertentu */
static void klip_hapus_entri(ManagerKlip *klip, int indeks) {
    int i;
    
    if (indeks < 0 || indeks >= klip->jumlah) return;
    
    if (klip->entri[indeks].data) {
        free(klip->entri[indeks].data);
    }
    
    /* Geser entri */
    for (i = indeks; i < klip->jumlah - 1; i++) {
        klip->entri[i] = klip->entri[i + 1];
    }
    
    klip->entri[klip->jumlah - 1].data = NULL;
    klip->entri[klip->jumlah - 1].ukuran = 0;
    klip->entri[klip->jumlah - 1].kapasitas = 0;
    klip->entri[klip->jumlah - 1].tipe = 0;
    klip->entri[klip->jumlah - 1].id = 0;
    
    klip->jumlah--;
    if (klip->saat_ini >= klip->jumlah) {
        klip->saat_ini = klip->jumlah - 1;
    }
}

/* Salin data ke clipboard */
int klip_saline(ManagerKlip *klip, const char *data, size_t ukuran, int tipe) {
    int indeks;
    char *data_baru;
    
    if (!klip || !data || ukuran == 0) return 0;
    if (ukuran > klip->ukuran_maks) ukuran = klip->ukuran_maks;
    
    /* Auto hapus jika penuh */
    if (klip->auto_hapus && klip->jumlah >= KLIP_ENTRI_MAKS) {
        klip_hapus_entri(klip, 0);
    }
    
    if (klip->jumlah >= KLIP_ENTRI_MAKS) return 0;
    
    /* Alokasi memori */
    data_baru = (char *)malloc(ukuran + 1);
    if (!data_baru) return 0;
    
    memcpy(data_baru, data, ukuran);
    data_baru[ukuran] = '\0';
    
    /* Tambah entri baru */
    indeks = klip->jumlah;
    klip->entri[indeks].data = data_baru;
    klip->entri[indeks].ukuran = ukuran;
    klip->entri[indeks].kapasitas = ukuran + 1;
    klip->entri[indeks].tipe = tipe;
    klip->entri[indeks].id = klip->id_berikutnya++;
    
    klip->jumlah++;
    klip->saat_ini = indeks;
    
    return 1;
}

/* Salin string ke clipboard */
int klip_saline_string(ManagerKlip *klip, const char *str) {
    if (!str) return 0;
    return klip_saline(klip, str, strlen(str), 0);
}

/* Salin binary data ke clipboard */
int klip_saline_binary(ManagerKlip *klip, const char *data, size_t ukuran) {
    return klip_saline(klip, data, ukuran, 1);
}

/* Tempel data dari clipboard */
const char* klip_tempel(ManagerKlip *klip, size_t *ukuran, int *tipe) {
    if (!klip || klip->saat_ini < 0 || klip->saat_ini >= klip->jumlah) {
        if (ukuran) *ukuran = 0;
        if (tipe) *tipe = 0;
        return NULL;
    }
    
    if (ukuran) *ukuran = klip->entri[klip->saat_ini].ukuran;
    if (tipe) *tipe = klip->entri[klip->saat_ini].tipe;
    
    return klip->entri[klip->saat_ini].data;
}

/* Tempel string dari clipboard */
const char* klip_tempel_string(ManagerKlip *klip) {
    size_t ukuran;
    int tipe;
    const char *data;
    
    data = klip_tempel(klip, &ukuran, &tipe);
    if (data && tipe == 0) return data;
    return NULL;
}

/* Tempel binary data dari clipboard */
const char* klip_tempel_binary(ManagerKlip *klip, size_t *ukuran) {
    int tipe;
    const char *data;
    
    data = klip_tempel(klip, ukuran, &tipe);
    if (data && tipe == 1) return data;
    if (ukuran) *ukuran = 0;
    return NULL;
}

/* Dapatkan ukuran clipboard saat ini */
size_t klip_ukuran(ManagerKlip *klip) {
    if (!klip || klip->saat_ini < 0 || klip->saat_ini >= klip->jumlah) return 0;
    return klip->entri[klip->saat_ini].ukuran;
}

/* Dapatkan tipe clipboard saat ini */
int klip_tipe(ManagerKlip *klip) {
    if (!klip || klip->saat_ini < 0 || klip->saat_ini >= klip->jumlah) return -1;
    return klip->entri[klip->saat_ini].tipe;
}

/* Dapatkan ID clipboard saat ini */
unsigned int klip_id(ManagerKlip *klip) {
    if (!klip || klip->saat_ini < 0 || klip->saat_ini >= klip->jumlah) return 0;
    return klip->entri[klip->saat_ini].id;
}

/* Pilih entri clipboard */
int klip_pilih(ManagerKlip *klip, int indeks) {
    if (!klip || indeks < 0 || indeks >= klip->jumlah) return 0;
    klip->saat_ini = indeks;
    return 1;
}

/* Pilih entri berdasarkan ID */
int klip_pilih_id(ManagerKlip *klip, unsigned int id) {
    int i;
    for (i = 0; i < klip->jumlah; i++) {
        if (klip->entri[i].id == id) {
            klip->saat_ini = i;
            return 1;
        }
    }
    return 0;
}

/* Dapatkan jumlah entri */
int klip_jumlah(ManagerKlip *klip) {
    return klip ? klip->jumlah : 0;
}

/* Hapus entri saat ini */
int klip_hapus_saat_ini(ManagerKlip *klip) {
    if (!klip || klip->saat_ini < 0) return 0;
    klip_hapus_entri(klip, klip->saat_ini);
    return 1;
}

/* Hapus semua entri */
void klip_hapus_semua(ManagerKlip *klip) {
    klip_bebas(klip);
}
