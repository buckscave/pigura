/*
 * PIGURA LIBC - STRING/STRFIND.C
 * ===============================
 * Implementasi fungsi pencarian string.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Fungsi yang diimplementasikan:
 *   - strchr()     : Cari karakter pertama dalam string
 *   - strrchr()    : Cari karakter terakhir dalam string
 *   - strchrnul()  : Cari karakter atau return pointer ke NUL
 *   - strstr()     : Cari substring dalam string
 *   - strcasestr() : Cari substring (case-insensitive)
 *   - strpbrk()    : Cari karakter dari set
 *   - strspn()     : Hitung prefix dari karakter set
 *   - strcspn()    : Hitung prefix bukan dari karakter set
 *   - strtok()     : Tokenize string
 *   - strtok_r()   : Tokenize string (reentrant)
 */

#include <string.h>
#include <ctype.h>

/* ============================================================
 * FUNGSI PENCARIAN KARAKTER
 * ============================================================
 */

/*
 * strchr - Cari kemunculan pertama karakter dalam string
 *
 * Parameter:
 *   s - String yang dicari
 *   c - Karakter yang dicari (dikonversi ke unsigned char)
 *
 * Return: Pointer ke karakter pertama, atau NULL jika tidak ada
 */
char *strchr(const char *s, int c) {
    unsigned char ch;

    if (s == NULL) {
        return NULL;
    }

    ch = (unsigned char)c;

    while (*s != '\0') {
        if ((unsigned char)*s == ch) {
            return (char *)s;
        }
        s++;
    }

    /* Karakter NUL dianggap sebagai bagian dari string */
    if (ch == '\0') {
        return (char *)s;
    }

    return NULL;
}

/*
 * strrchr - Cari kemunculan terakhir karakter dalam string
 *
 * Parameter:
 *   s - String yang dicari
 *   c - Karakter yang dicari (dikonversi ke unsigned char)
 *
 * Return: Pointer ke karakter terakhir, atau NULL jika tidak ada
 */
char *strrchr(const char *s, int c) {
    const char *last;
    unsigned char ch;

    if (s == NULL) {
        return NULL;
    }

    ch = (unsigned char)c;
    last = NULL;

    while (*s != '\0') {
        if ((unsigned char)*s == ch) {
            last = s;
        }
        s++;
    }

    /* Karakter NUL dianggap sebagai bagian dari string */
    if (ch == '\0') {
        return (char *)s;
    }

    return (char *)last;
}

/*
 * strchrnul - Cari karakter atau return pointer ke NUL
 *
 * Parameter:
 *   s - String yang dicari
 *   c - Karakter yang dicari (dikonversi ke unsigned char)
 *
 * Return: Pointer ke karakter atau ke terminator NUL
 *
 * Catatan: Fungsi GNU extension, juga tersedia di POSIX.
 */
char *strchrnul(const char *s, int c) {
    unsigned char ch;

    if (s == NULL) {
        return NULL;
    }

    ch = (unsigned char)c;

    while (*s != '\0') {
        if ((unsigned char)*s == ch) {
            return (char *)s;
        }
        s++;
    }

    /* Return pointer ke NUL terminator */
    return (char *)s;
}

/* ============================================================
 * FUNGSI PENCARIAN SUBSTRING
 * ============================================================
 */

/*
 * strstr - Cari substring pertama dalam string
 *
 * Parameter:
 *   haystack - String yang dicari
 *   needle   - Substring yang dicari
 *
 * Return: Pointer ke awal substring, atau NULL jika tidak ada
 */
char *strstr(const char *haystack, const char *needle) {
    const char *h;
    const char *n;

    if (haystack == NULL) {
        return NULL;
    }

    if (needle == NULL) {
        return NULL;
    }

    /* Kasus khusus: needle kosong */
    if (*needle == '\0') {
        return (char *)haystack;
    }

    while (*haystack != '\0') {
        /* Cek apakah needle cocok mulai dari posisi ini */
        h = haystack;
        n = needle;

        while (*h != '\0' && *n != '\0' && *h == *n) {
            h++;
            n++;
        }

        /* Jika needle habis, berarti ditemukan */
        if (*n == '\0') {
            return (char *)haystack;
        }

        haystack++;
    }

    return NULL;
}

/*
 * strcasestr - Cari substring (case-insensitive)
 *
 * Parameter:
 *   haystack - String yang dicari
 *   needle   - Substring yang dicari
 *
 * Return: Pointer ke awal substring, atau NULL jika tidak ada
 *
 * Catatan: Fungsi non-standar, tersedia di BSD dan GNU.
 */
char *strcasestr(const char *haystack, const char *needle) {
    const char *h;
    const char *n;

    if (haystack == NULL) {
        return NULL;
    }

    if (needle == NULL) {
        return NULL;
    }

    /* Kasus khusus: needle kosong */
    if (*needle == '\0') {
        return (char *)haystack;
    }

    while (*haystack != '\0') {
        /* Cek apakah needle cocok mulai dari posisi ini */
        h = haystack;
        n = needle;

        while (*h != '\0' && *n != '\0' &&
               tolower((unsigned char)*h) ==
               tolower((unsigned char)*n)) {
            h++;
            n++;
        }

        /* Jika needle habis, berarti ditemukan */
        if (*n == '\0') {
            return (char *)haystack;
        }

        haystack++;
    }

    return NULL;
}

/* ============================================================
 * FUNGSI PENCARIAN KARAKTER DARI SET
 * ============================================================
 */

/*
 * strpbrk - Cari karakter pertama yang ada dalam accept
 *
 * Parameter:
 *   s      - String yang dicari
 *   accept - Set karakter yang diterima
 *
 * Return: Pointer ke karakter pertama yang ada di accept,
 *         atau NULL jika tidak ada
 */
char *strpbrk(const char *s, const char *accept) {
    const char *a;

    if (s == NULL || accept == NULL) {
        return NULL;
    }

    while (*s != '\0') {
        /* Cek apakah karakter ini ada di accept */
        for (a = accept; *a != '\0'; a++) {
            if (*s == *a) {
                return (char *)s;
            }
        }
        s++;
    }

    return NULL;
}

/*
 * strspn - Hitung panjang prefix dari karakter dalam accept
 *
 * Parameter:
 *   s      - String yang dihitung
 *   accept - Set karakter yang diterima
 *
 * Return: Panjang prefix yang terdiri dari karakter di accept
 */
size_t strspn(const char *s, const char *accept) {
    const char *a;
    size_t count;

    if (s == NULL || accept == NULL) {
        return 0;
    }

    count = 0;

    while (*s != '\0') {
        /* Cek apakah karakter ini ada di accept */
        for (a = accept; *a != '\0'; a++) {
            if (*s == *a) {
                break;
            }
        }

        /* Jika tidak ditemukan, berhenti */
        if (*a == '\0') {
            break;
        }

        count++;
        s++;
    }

    return count;
}

/*
 * strcspn - Hitung panjang prefix dari karakter BUKAN di reject
 *
 * Parameter:
 *   s      - String yang dihitung
 *   reject - Set karakter yang ditolak
 *
 * Return: Panjang prefix yang TIDAK terdiri dari karakter di reject
 */
size_t strcspn(const char *s, const char *reject) {
    const char *r;
    size_t count;

    if (s == NULL) {
        return 0;
    }

    if (reject == NULL) {
        /* Jika reject NULL, treat sebagai string kosong */
        return strlen(s);
    }

    count = 0;

    while (*s != '\0') {
        /* Cek apakah karakter ini ada di reject */
        for (r = reject; *r != '\0'; r++) {
            if (*s == *r) {
                return count;
            }
        }

        count++;
        s++;
    }

    return count;
}

/* ============================================================
 * FUNGSI TOKENISASI
 * ============================================================
 */

/* Variabel statis untuk strtok() (non-reentrant) */
static char *strtok_last = NULL;

/*
 * strtok - Tokenize string dengan delimiter
 *
 * Parameter:
 *   str  - String yang ditokenize (NULL untuk lanjut)
 *   delim - Set karakter delimiter
 *
 * Return: Pointer ke token berikutnya, atau NULL jika habis
 *
 * Peringatan: Fungsi ini TIDAK reentrant. Gunakan strtok_r()
 *             untuk aplikasi multi-threaded.
 */
char *strtok(char *str, const char *delim) {
    return strtok_r(str, delim, &strtok_last);
}

/*
 * strtok_r - Tokenize string dengan delimiter (reentrant)
 *
 * Parameter:
 *   str    - String yang ditokenize (NULL untuk lanjut)
 *   delim  - Set karakter delimiter
 *   saveptr - Pointer untuk menyimpan state antar panggilan
 *
 * Return: Pointer ke token berikutnya, atau NULL jika habis
 *
 * Catatan: Fungsi POSIX reentrant yang aman untuk multi-thread.
 */
char *strtok_r(char *str, const char *delim, char **saveptr) {
    char *token;
    char *end;

    /* Validasi parameter */
    if (delim == NULL || saveptr == NULL) {
        return NULL;
    }

    /* Gunakan saveptr jika str NULL */
    if (str == NULL) {
        str = *saveptr;
        if (str == NULL) {
            return NULL;
        }
    }

    /* Lewati leading delimiter */
    str += strspn(str, delim);
    if (*str == '\0') {
        *saveptr = NULL;
        return NULL;
    }

    /* Token dimulai dari sini */
    token = str;

    /* Cari akhir token */
    end = str + strcspn(str, delim);
    if (*end == '\0') {
        /* Token sampai akhir string */
        *saveptr = NULL;
    } else {
        /* Null-terminate token dan simpan posisi berikutnya */
        *end = '\0';
        *saveptr = end + 1;
    }

    return token;
}
