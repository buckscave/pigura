/*******************************************************************************
 * KONFIGURASI.C - Implementasi Config Manager
 *
 * Modul ini menangani pembacaan dan penulisan file konfigurasi INI.
 *
 * Copyright (c) 2024 Pigura OS Project
 ******************************************************************************/

#include "pigura.h"

/*******************************************************************************
 * FUNGSI INTERNAL
 ******************************************************************************/

/* Trim whitespace */
static char* konfig_trim(char *str) {
    char *start = str;
    char *end = str + strlen(str) - 1;
    
    while (start <= end && (*start == ' ' || *start == '\t' || 
           *start == '\r' || *start == '\n')) {
        start++;
    }
    
    while (end >= start && (*end == ' ' || *end == '\t' || 
           *end == '\r' || *end == '\n')) {
        end--;
    }
    
    *(end + 1) = '\0';
    return start;
}

/* Konversi string ke boolean */
static int konfig_string_ke_bool(const char *str) {
    if (!str) return 0;
    return (strcasecmp(str, "true") == 0 || strcasecmp(str, "yes") == 0 ||
            strcasecmp(str, "1") == 0 || strcasecmp(str, "on") == 0);
}

/* Konversi string ke warna */
static int konfig_string_ke_warna(const char *str, WarnaRGB *warna) {
    unsigned int r, g, b;
    
    if (!str || !warna) return 0;
    
    /* Format #RRGGBB */
    if (str[0] == '#' && strlen(str) == 7) {
        if (sscanf(str + 1, "%2x%2x%2x", &r, &g, &b) == 3) {
            warna->r = (unsigned char)r;
            warna->g = (unsigned char)g;
            warna->b = (unsigned char)b;
            return 1;
        }
    }
    
    /* Format rgb(R,G,B) */
    if (strncmp(str, "rgb(", 4) == 0) {
        if (sscanf(str + 4, "%u,%u,%u", &r, &g, &b) == 3) {
            warna->r = (unsigned char)r;
            warna->g = (unsigned char)g;
            warna->b = (unsigned char)b;
            return 1;
        }
    }
    
    /* Nama warna */
    if (strcasecmp(str, "black") == 0) {
        warna->r = 0; warna->g = 0; warna->b = 0; return 1;
    } else if (strcasecmp(str, "white") == 0) {
        warna->r = 255; warna->g = 255; warna->b = 255; return 1;
    } else if (strcasecmp(str, "red") == 0) {
        warna->r = 255; warna->g = 0; warna->b = 0; return 1;
    } else if (strcasecmp(str, "green") == 0) {
        warna->r = 0; warna->g = 255; warna->b = 0; return 1;
    } else if (strcasecmp(str, "blue") == 0) {
        warna->r = 0; warna->g = 0; warna->b = 255; return 1;
    }
    
    return 0;
}

/* Cari seksi */
static SeksiKonfig* konfig_cari_seksi(ManagerKonfig *km, const char *nama) {
    int i;
    for (i = 0; i < km->jumlah_seksi; i++) {
        if (strcmp(km->seksi[i].nama, nama) == 0) {
            return &km->seksi[i];
        }
    }
    return NULL;
}

/* Tambah seksi baru */
static SeksiKonfig* konfig_tambah_seksi(ManagerKonfig *km, const char *nama) {
    if (km->jumlah_seksi >= KONFIG_SEKSI_MAKS) return NULL;
    
    strncpy(km->seksi[km->jumlah_seksi].nama, nama, KONFIG_NILAI_MAKS - 1);
    km->seksi[km->jumlah_seksi].nama[KONFIG_NILAI_MAKS - 1] = '\0';
    km->seksi[km->jumlah_seksi].jumlah_kunci = 0;
    km->jumlah_seksi++;
    km->diubah = 1;
    
    return &km->seksi[km->jumlah_seksi - 1];
}

/* Cari kunci dalam seksi */
static NilaiKonfig* konfig_cari_kunci(SeksiKonfig *seksi, const char *kunci) {
    int i;
    for (i = 0; i < seksi->jumlah_kunci; i++) {
        if (strcmp(seksi->nilai[i].kunci, kunci) == 0) {
            return &seksi->nilai[i];
        }
    }
    return NULL;
}

/* Tambah kunci baru */
static NilaiKonfig* konfig_tambah_kunci(SeksiKonfig *seksi, const char *kunci,
    const char *nilai, TipeKonfig tipe) {
    if (seksi->jumlah_kunci >= KONFIG_KUNCI_MAKS) return NULL;
    
    strncpy(seksi->nilai[seksi->jumlah_kunci].kunci, kunci, KONFIG_NILAI_MAKS - 1);
    seksi->nilai[seksi->jumlah_kunci].kunci[KONFIG_NILAI_MAKS - 1] = '\0';
    strncpy(seksi->nilai[seksi->jumlah_kunci].nilai, nilai, KONFIG_NILAI_MAKS - 1);
    seksi->nilai[seksi->jumlah_kunci].nilai[KONFIG_NILAI_MAKS - 1] = '\0';
    seksi->nilai[seksi->jumlah_kunci].tipe = tipe;
    seksi->nilai[seksi->jumlah_kunci].diubah = 1;
    seksi->jumlah_kunci++;
    
    return &seksi->nilai[seksi->jumlah_kunci - 1];
}

/*******************************************************************************
 * FUNGSI PUBLIK
 ******************************************************************************/

/* Inisialisasi manager konfigurasi */
int konfig_init(ManagerKonfig *km, const char *namafile) {
    int i, j;
    
    if (!km) return 0;
    
    km->jumlah_seksi = 0;
    km->diubah = 0;
    km->auto_simpan = 1;
    km->buat_backup = 1;
    
    if (namafile) {
        strncpy(km->namafile, namafile, KONFIG_JALUR_MAKS - 1);
        km->namafile[KONFIG_JALUR_MAKS - 1] = '\0';
    } else {
        strcpy(km->namafile, KONFIG_FILE_DEFAULT);
    }
    
    /* Inisialisasi seksi */
    for (i = 0; i < KONFIG_SEKSI_MAKS; i++) {
        strcpy(km->seksi[i].nama, "");
        km->seksi[i].jumlah_kunci = 0;
        for (j = 0; j < KONFIG_KUNCI_MAKS; j++) {
            strcpy(km->seksi[i].nilai[j].kunci, "");
            strcpy(km->seksi[i].nilai[j].nilai, "");
            km->seksi[i].nilai[j].tipe = KONFIG_TIPE_TIDAK_DIKETAHUI;
            km->seksi[i].nilai[j].diubah = 0;
        }
    }
    
    return 1;
}

/* Muat konfigurasi dari file */
int konfig_muat(ManagerKonfig *km, const char *namafile) {
    FILE *file;
    char baris[KONFIG_BARIS_MAKS];
    SeksiKonfig *seksi_saat_ini = NULL;
    
    if (!km) return 0;
    
    if (namafile) {
        strncpy(km->namafile, namafile, KONFIG_JALUR_MAKS - 1);
        km->namafile[KONFIG_JALUR_MAKS - 1] = '\0';
    }
    
    file = fopen(km->namafile, "r");
    if (!file) return 0;
    
    /* Clear existing */
    km->jumlah_seksi = 0;
    
    while (fgets(baris, sizeof(baris), file)) {
        char *trimmed = konfig_trim(baris);
        char *equal;
        
        /* Skip kosong dan komentar */
        if (trimmed[0] == '\0' || trimmed[0] == '#') continue;
        
        /* Parse seksi */
        if (trimmed[0] == '[' && trimmed[strlen(trimmed) - 1] == ']') {
            trimmed[strlen(trimmed) - 1] = '\0';
            seksi_saat_ini = konfig_cari_seksi(km, trimmed + 1);
            if (!seksi_saat_ini) {
                seksi_saat_ini = konfig_tambah_seksi(km, trimmed + 1);
            }
            continue;
        }
        
        /* Parse key=value */
        equal = strchr(trimmed, '=');
        if (equal && seksi_saat_ini) {
            *equal = '\0';
            char *kunci = konfig_trim(trimmed);
            char *nilai = konfig_trim(equal + 1);
            
            if (strlen(kunci) > 0) {
                NilaiKonfig *nv = konfig_cari_kunci(seksi_saat_ini, kunci);
                if (!nv) {
                    konfig_tambah_kunci(seksi_saat_ini, kunci, nilai, 
                        KONFIG_TIPE_STRING);
                } else {
                    strcpy(nv->nilai, nilai);
                    nv->diubah = 0;
                }
            }
        }
    }
    
    fclose(file);
    km->diubah = 0;
    return 1;
}

/* Simpan konfigurasi ke file */
int konfig_simpan(ManagerKonfig *km, const char *namafile) {
    FILE *file;
    int i, j;
    char backup[KONFIG_JALUR_MAKS];
    
    if (!km) return 0;
    
    if (namafile) {
        strncpy(km->namafile, namafile, KONFIG_JALUR_MAKS - 1);
        km->namafile[KONFIG_JALUR_MAKS - 1] = '\0';
    }
    
    /* Create backup */
    if (km->buat_backup) {
        strcpy(backup, km->namafile);
        strcat(backup, KONFIG_BACKUP_EXT);
        rename(km->namafile, backup);
    }
    
    file = fopen(km->namafile, "w");
    if (!file) return 0;
    
    for (i = 0; i < km->jumlah_seksi; i++) {
        fprintf(file, "[%s]\n", km->seksi[i].nama);
        for (j = 0; j < km->seksi[i].jumlah_kunci; j++) {
            fprintf(file, "%s = %s\n", km->seksi[i].nilai[j].kunci,
                km->seksi[i].nilai[j].nilai);
        }
        if (i < km->jumlah_seksi - 1) fprintf(file, "\n");
    }
    
    fclose(file);
    km->diubah = 0;
    return 1;
}

/* Dapatkan nilai string */
const char* konfig_string(ManagerKonfig *km, const char *seksi, const char *kunci,
    const char *nilai_default) {
    SeksiKonfig *sk;
    NilaiKonfig *nv;
    
    if (!km || !seksi || !kunci) return nilai_default;
    
    sk = konfig_cari_seksi(km, seksi);
    if (!sk) return nilai_default;
    
    nv = konfig_cari_kunci(sk, kunci);
    if (!nv) return nilai_default;
    
    return nv->nilai;
}

/* Dapatkan nilai integer */
int konfig_int(ManagerKonfig *km, const char *seksi, const char *kunci, int nilai_default) {
    const char *str = konfig_string(km, seksi, kunci, NULL);
    if (!str) return nilai_default;
    return atoi(str);
}

/* Dapatkan nilai boolean */
int konfig_bool(ManagerKonfig *km, const char *seksi, const char *kunci, int nilai_default) {
    const char *str = konfig_string(km, seksi, kunci, NULL);
    if (!str) return nilai_default;
    return konfig_string_ke_bool(str);
}

/* Dapatkan nilai float */
float konfig_float(ManagerKonfig *km, const char *seksi, const char *kunci, float nilai_default) {
    const char *str = konfig_string(km, seksi, kunci, NULL);
    if (!str) return nilai_default;
    return (float)atof(str);
}

/* Dapatkan nilai warna */
int konfig_warna(ManagerKonfig *km, const char *seksi, const char *kunci, WarnaRGB *warna) {
    const char *str = konfig_string(km, seksi, kunci, NULL);
    if (!str || !warna) return 0;
    return konfig_string_ke_warna(str, warna);
}

/* Set nilai string */
int konfig_set_string(ManagerKonfig *km, const char *seksi, const char *kunci, const char *nilai) {
    SeksiKonfig *sk;
    NilaiKonfig *nv;
    
    if (!km || !seksi || !kunci || !nilai) return 0;
    
    sk = konfig_cari_seksi(km, seksi);
    if (!sk) sk = konfig_tambah_seksi(km, seksi);
    if (!sk) return 0;
    
    nv = konfig_cari_kunci(sk, kunci);
    if (!nv) {
        nv = konfig_tambah_kunci(sk, kunci, nilai, KONFIG_TIPE_STRING);
    } else {
        strncpy(nv->nilai, nilai, KONFIG_NILAI_MAKS - 1);
        nv->nilai[KONFIG_NILAI_MAKS - 1] = '\0';
        nv->tipe = KONFIG_TIPE_STRING;
        nv->diubah = 1;
    }
    
    km->diubah = 1;
    if (km->auto_simpan) konfig_simpan(km, NULL);
    
    return 1;
}

/* Set nilai integer */
int konfig_set_int(ManagerKonfig *km, const char *seksi, const char *kunci, int nilai) {
    char str[32];
    sprintf(str, "%d", nilai);
    return konfig_set_string(km, seksi, kunci, str);
}

/* Set nilai boolean */
int konfig_set_bool(ManagerKonfig *km, const char *seksi, const char *kunci, int nilai) {
    return konfig_set_string(km, seksi, kunci, nilai ? "true" : "false");
}

/* Set nilai float */
int konfig_set_float(ManagerKonfig *km, const char *seksi, const char *kunci, float nilai) {
    char str[32];
    sprintf(str, "%f", nilai);
    return konfig_set_string(km, seksi, kunci, str);
}

/* Set nilai warna */
int konfig_set_warna(ManagerKonfig *km, const char *seksi, const char *kunci, WarnaRGB *warna) {
    char str[16];
    if (!warna) return 0;
    sprintf(str, "#%02X%02X%02X", warna->r, warna->g, warna->b);
    return konfig_set_string(km, seksi, kunci, str);
}

/* Set auto-save */
void konfig_set_auto_simpan(ManagerKonfig *km, int auto_simpan) {
    if (km) km->auto_simpan = auto_simpan;
}

/* Set buat backup */
void konfig_set_buat_backup(ManagerKonfig *km, int buat_backup) {
    if (km) km->buat_backup = buat_backup;
}

/* Cek apakah konfigurasi diubah */
int konfig_diubah(ManagerKonfig *km) {
    return km ? km->diubah : 0;
}

/* Dapatkan nama file */
const char* konfig_namafile(ManagerKonfig *km) {
    return km ? km->namafile : NULL;
}

/* Bebaskan manager konfigurasi */
void konfig_bebas(ManagerKonfig *km) {
    if (km) {
        km->jumlah_seksi = 0;
        km->diubah = 0;
    }
}
