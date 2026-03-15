/*******************************************************************************
 * UTILITAS.C - Fungsi Utilitas Umum
 *
 * Modul ini berisi fungsi-fungsi utilitas yang digunakan oleh modul lain.
 *
 * Copyright (c) 2024 Pigura OS Project
 ******************************************************************************/

#include "pigura.h"

/*******************************************************************************
 * FUNGSI UTILITAS STRING
 ******************************************************************************/

/* Duplikasi string dengan aman */
char* pigura_strdup(const char *str) {
    char *copy;
    size_t len;
    
    if (!str) {
        return NULL;
    }
    
    len = strlen(str);
    copy = (char *)malloc(len + 1);
    if (!copy) {
        return NULL;
    }
    
    memcpy(copy, str, len + 1);
    return copy;
}

/* Bandingkan string case-insensitive */
int pigura_strcasecmp(const char *s1, const char *s2) {
    unsigned char c1, c2;
    
    if (!s1 && !s2) {
        return 0;
    }
    if (!s1) {
        return -1;
    }
    if (!s2) {
        return 1;
    }
    
    while (*s1 && *s2) {
        c1 = (unsigned char)*s1;
        c2 = (unsigned char)*s2;
        
        if (c1 >= 'A' && c1 <= 'Z') {
            c1 += 32;
        }
        if (c2 >= 'A' && c2 <= 'Z') {
            c2 += 32;
        }
        
        if (c1 != c2) {
            return (int)c1 - (int)c2;
        }
        
        s1++;
        s2++;
    }
    
    return (int)(unsigned char)*s1 - (int)(unsigned char)*s2;
}

/* Konversi string ke boolean */
int pigura_string_ke_bool(const char *str) {
    if (!str) {
        return 0;
    }
    
    if (pigura_strcasecmp(str, "true") == 0) {
        return 1;
    }
    if (pigura_strcasecmp(str, "yes") == 0) {
        return 1;
    }
    if (pigura_strcasecmp(str, "1") == 0) {
        return 1;
    }
    if (pigura_strcasecmp(str, "on") == 0) {
        return 1;
    }
    
    return 0;
}

/* Konversi string ke warna */
int pigura_string_ke_warna(const char *str, WarnaRGB *warna) {
    unsigned int r, g, b;
    
    if (!str || !warna) {
        return 0;
    }
    
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
    
    /* Nama warna standar */
    if (pigura_strcasecmp(str, "black") == 0) {
        warna->r = 0;
        warna->g = 0;
        warna->b = 0;
        return 1;
    }
    if (pigura_strcasecmp(str, "white") == 0) {
        warna->r = 255;
        warna->g = 255;
        warna->b = 255;
        return 1;
    }
    if (pigura_strcasecmp(str, "red") == 0) {
        warna->r = 255;
        warna->g = 0;
        warna->b = 0;
        return 1;
    }
    if (pigura_strcasecmp(str, "green") == 0) {
        warna->r = 0;
        warna->g = 255;
        warna->b = 0;
        return 1;
    }
    if (pigura_strcasecmp(str, "blue") == 0) {
        warna->r = 0;
        warna->g = 0;
        warna->b = 255;
        return 1;
    }
    if (pigura_strcasecmp(str, "yellow") == 0) {
        warna->r = 255;
        warna->g = 255;
        warna->b = 0;
        return 1;
    }
    if (pigura_strcasecmp(str, "cyan") == 0) {
        warna->r = 0;
        warna->g = 255;
        warna->b = 255;
        return 1;
    }
    if (pigura_strcasecmp(str, "magenta") == 0) {
        warna->r = 255;
        warna->g = 0;
        warna->b = 255;
        return 1;
    }
    if (pigura_strcasecmp(str, "orange") == 0) {
        warna->r = 255;
        warna->g = 165;
        warna->b = 0;
        return 1;
    }
    if (pigura_strcasecmp(str, "gray") == 0 || 
        pigura_strcasecmp(str, "grey") == 0) {
        warna->r = 128;
        warna->g = 128;
        warna->b = 128;
        return 1;
    }
    if (pigura_strcasecmp(str, "lightgray") == 0 || 
        pigura_strcasecmp(str, "lightgrey") == 0) {
        warna->r = 192;
        warna->g = 192;
        warna->b = 192;
        return 1;
    }
    if (pigura_strcasecmp(str, "darkgray") == 0 || 
        pigura_strcasecmp(str, "darkgrey") == 0) {
        warna->r = 64;
        warna->g = 64;
        warna->b = 64;
        return 1;
    }
    
    return 0;
}

/* Konversi warna ke string */
void pigura_warna_ke_string(WarnaRGB *warna, char *str, int panjang_maks) {
    if (!warna || !str || panjang_maks < 8) {
        return;
    }
    snprintf(str, (size_t)panjang_maks, "#%02X%02X%02X", 
             warna->r, warna->g, warna->b);
}

/*******************************************************************************
 * FUNGSI UTILITAS MATEMATIKA
 ******************************************************************************/

/* Hitung nilai absolut */
int pigura_abs(int nilai) {
    return (nilai < 0) ? -nilai : nilai;
}

/* Hitung nilai minimum */
int pigura_min(int a, int b) {
    return (a < b) ? a : b;
}

/* Hitung nilai maksimum */
int pigura_maks(int a, int b) {
    return (a > b) ? a : b;
}

/* Batasi nilai dalam rentang */
int pigura_batas(int nilai, int min, int maks) {
    if (nilai < min) {
        return min;
    }
    if (nilai > maks) {
        return maks;
    }
    return nilai;
}

/*******************************************************************************
 * FUNGSI UTILITAS MEMORI
 ******************************************************************************/

/* Alokasi memori dengan inisialisasi nol */
void* pigura_malloc(size_t ukuran) {
    void *ptr;
    
    if (ukuran == 0) {
        return NULL;
    }
    
    ptr = malloc(ukuran);
    if (ptr) {
        memset(ptr, 0, ukuran);
    }
    
    return ptr;
}

/* Realokasi memori dengan aman */
void* pigura_realloc(void *ptr, size_t ukuran_lama, size_t ukuran_baru) {
    void *ptr_baru;
    
    if (ukuran_baru == 0) {
        if (ptr) {
            free(ptr);
        }
        return NULL;
    }
    
    ptr_baru = realloc(ptr, ukuran_baru);
    if (ptr_baru && ukuran_baru > ukuran_lama) {
        /* Inisialisasi memori baru dengan nol */
        memset((char *)ptr_baru + ukuran_lama, 0, ukuran_baru - ukuran_lama);
    }
    
    return ptr_baru;
}

/* Bebaskan memori dengan aman */
void pigura_free(void *ptr) {
    if (ptr) {
        free(ptr);
    }
}

/*******************************************************************************
 * FUNGSI UTILITAS WAKTU
 ******************************************************************************/

/* Dapatkan waktu saat ini dalam milidetik */
unsigned long pigura_waktu_ms(void) {
    struct timeval tv;
    
    if (gettimeofday(&tv, NULL) != 0) {
        return 0;
    }
    
    return (unsigned long)(tv.tv_sec * 1000) + 
           (unsigned long)(tv.tv_usec / 1000);
}

/* Tidur selama beberapa milidetik */
void pigura_tidur_ms(unsigned int milidetik) {
    struct timespec ts;
    
    ts.tv_sec = (time_t)(milidetik / 1000);
    ts.tv_nsec = (long)((milidetik % 1000) * 1000000);
    
    nanosleep(&ts, NULL);
}

/*******************************************************************************
 * FUNGSI UTILITAS PATH FILE
 ******************************************************************************/

/* Periksa apakah file ada */
int pigura_file_ada(const char *jalur) {
    struct stat st;
    
    if (!jalur) {
        return 0;
    }
    
    return (stat(jalur, &st) == 0);
}

/* Dapatkan direktori dari path */
char* pigura_direktori_dari_path(const char *jalur, char *buffer, 
                                  size_t ukuran_buffer) {
    const char *p;
    size_t len;
    
    if (!jalur || !buffer || ukuran_buffer == 0) {
        return NULL;
    }
    
    /* Cari karakter '/' terakhir */
    p = strrchr(jalur, '/');
    if (!p) {
        /* Tidak ada direktori */
        buffer[0] = '.';
        buffer[1] = '\0';
        return buffer;
    }
    
    len = (size_t)(p - jalur);
    if (len >= ukuran_buffer) {
        len = ukuran_buffer - 1;
    }
    
    memcpy(buffer, jalur, len);
    buffer[len] = '\0';
    
    return buffer;
}

/* Dapatkan nama file dari path */
char* pigura_namafile_dari_path(const char *jalur, char *buffer,
                                 size_t ukuran_buffer) {
    const char *p;
    size_t len;
    
    if (!jalur || !buffer || ukuran_buffer == 0) {
        return NULL;
    }
    
    /* Cari karakter '/' terakhir */
    p = strrchr(jalur, '/');
    if (!p) {
        p = jalur;
    } else {
        p++;
    }
    
    len = strlen(p);
    if (len >= ukuran_buffer) {
        len = ukuran_buffer - 1;
    }
    
    memcpy(buffer, p, len);
    buffer[len] = '\0';
    
    return buffer;
}
