/*
 * PIGURA LIBC - STRING/STR.C
 * ==========================
 * Implementasi fungsi string dasar.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#include <string.h>
#include <stddef.h>

/* ============================================================
 * STRLEN
 * ============================================================
 * Hitung panjang string.
 */
size_t strlen(const char *s) {
    const char *p;
    
    if (s == NULL) {
        return 0;
    }
    
    p = s;
    while (*p != '\0') {
        p++;
    }
    
    return (size_t)(p - s);
}

/* ============================================================
 * STRNLEN
 * ============================================================
 * Hitung panjang string dengan batas maksimum.
 */
size_t strnlen(const char *s, size_t maxlen) {
    size_t len;
    
    if (s == NULL || maxlen == 0) {
        return 0;
    }
    
    len = 0;
    while (len < maxlen && s[len] != '\0') {
        len++;
    }
    
    return len;
}

/* ============================================================
 * STRCMP
 * ============================================================
 * Bandingkan dua string.
 */
int strcmp(const char *s1, const char *s2) {
    if (s1 == NULL || s2 == NULL) {
        if (s1 == s2) {
            return 0;
        }
        return (s1 == NULL) ? -1 : 1;
    }
    
    while (*s1 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
    }
    
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

/* ============================================================
 * STRNCMP
 * ============================================================
 * Bandingkan n karakter pertama dari dua string.
 */
int strncmp(const char *s1, const char *s2, size_t n) {
    if (n == 0) {
        return 0;
    }
    
    if (s1 == NULL || s2 == NULL) {
        if (s1 == s2) {
            return 0;
        }
        return (s1 == NULL) ? -1 : 1;
    }
    
    while (n > 0 && *s1 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
        n--;
    }
    
    if (n == 0) {
        return 0;
    }
    
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

/* ============================================================
 * STRCASECMP
 * ============================================================
 * Bandingkan string case-insensitive.
 */
int strcasecmp(const char *s1, const char *s2) {
    unsigned char c1, c2;
    
    if (s1 == NULL || s2 == NULL) {
        if (s1 == s2) {
            return 0;
        }
        return (s1 == NULL) ? -1 : 1;
    }
    
    do {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;
        
        /* Konversi ke lowercase */
        if (c1 >= 'A' && c1 <= 'Z') {
            c1 += 32;
        }
        if (c2 >= 'A' && c2 <= 'Z') {
            c2 += 32;
        }
        
        if (c1 != c2) {
            return c1 - c2;
        }
    } while (c1 != '\0');
    
    return 0;
}

/* ============================================================
 * STRNCASECMP
 * ============================================================
 * Bandingkan n karakter case-insensitive.
 */
int strncasecmp(const char *s1, const char *s2, size_t n) {
    unsigned char c1, c2;
    
    if (n == 0) {
        return 0;
    }
    
    if (s1 == NULL || s2 == NULL) {
        if (s1 == s2) {
            return 0;
        }
        return (s1 == NULL) ? -1 : 1;
    }
    
    do {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;
        
        /* Konversi ke lowercase */
        if (c1 >= 'A' && c1 <= 'Z') {
            c1 += 32;
        }
        if (c2 >= 'A' && c2 <= 'Z') {
            c2 += 32;
        }
        
        if (c1 != c2) {
            return c1 - c2;
        }
        
        n--;
    } while (c1 != '\0' && n > 0);
    
    return 0;
}

/* ============================================================
 * STRCOLL
 * ============================================================
 * Bandingkan string menggunakan locale.
 * Untuk sekarang, sama seperti strcmp.
 */
int strcoll(const char *s1, const char *s2) {
    return strcmp(s1, s2);
}

/* ============================================================
 * STRXFRM
 * ============================================================
 * Transformasi string untuk collation.
 */
size_t strxfrm(char *dest, const char *src, size_t n) {
    size_t len;
    
    if (src == NULL) {
        return 0;
    }
    
    len = strlen(src);
    
    if (dest != NULL && n > 0) {
        size_t copy_len = (len < n - 1) ? len : n - 1;
        memcpy(dest, src, copy_len);
        dest[copy_len] = '\0';
    }
    
    return len;
}

/* ============================================================
 * STRSTR
 * ============================================================
 * Cari substring dalam string.
 */
char *strstr(const char *haystack, const char *needle) {
    size_t needle_len;
    
    if (haystack == NULL || needle == NULL) {
        return NULL;
    }
    
    if (*needle == '\0') {
        return (char *)haystack;
    }
    
    needle_len = strlen(needle);
    
    while (*haystack != '\0') {
        if (*haystack == *needle) {
            if (strncmp(haystack, needle, needle_len) == 0) {
                return (char *)haystack;
            }
        }
        haystack++;
    }
    
    return NULL;
}

/* ============================================================
 * STRCASESTR (BSD extension)
 * ============================================================
 * Cari substring case-insensitive.
 */
char *strcasestr(const char *haystack, const char *needle) {
    if (haystack == NULL || needle == NULL) {
        return NULL;
    }
    
    if (*needle == '\0') {
        return (char *)haystack;
    }
    
    while (*haystack != '\0') {
        if (strncasecmp(haystack, needle, strlen(needle)) == 0) {
            return (char *)haystack;
        }
        haystack++;
    }
    
    return NULL;
}

/* ============================================================
 * STRCHR
 * ============================================================
 * Cari karakter pertama dalam string.
 */
char *strchr(const char *s, int c) {
    unsigned char ch;
    
    if (s == NULL) {
        return NULL;
    }
    
    ch = (unsigned char)c;
    
    do {
        if ((unsigned char)*s == ch) {
            return (char *)s;
        }
    } while (*s++ != '\0');
    
    return NULL;
}

/* ============================================================
 * STRRCHR
 * ============================================================
 * Cari karakter terakhir dalam string.
 */
char *strrchr(const char *s, int c) {
    const char *last;
    unsigned char ch;
    
    if (s == NULL) {
        return NULL;
    }
    
    last = NULL;
    ch = (unsigned char)c;
    
    do {
        if ((unsigned char)*s == ch) {
            last = s;
        }
    } while (*s++ != '\0');
    
    return (char *)last;
}

/* ============================================================
 * STRCHRNUL (GNU extension)
 * ============================================================
 * Cari karakter, return pointer ke karakter atau terminator.
 */
char *strchrnul(const char *s, int c) {
    unsigned char ch;
    
    if (s == NULL) {
        return NULL;
    }
    
    ch = (unsigned char)c;
    
    while (*s != '\0' && (unsigned char)*s != ch) {
        s++;
    }
    
    return (char *)s;
}

/* ============================================================
 * STRCOUNT (internal)
 * ============================================================
 * Hitung jumlah kemunculan karakter.
 */
size_t strcount(const char *s, int c) {
    size_t count;
    unsigned char ch;
    
    if (s == NULL) {
        return 0;
    }
    
    count = 0;
    ch = (unsigned char)c;
    
    while (*s != '\0') {
        if ((unsigned char)*s == ch) {
            count++;
        }
        s++;
    }
    
    return count;
}

/* ============================================================
 * STRENDSSWITH (internal)
 * ============================================================
 * Cek apakah string berakhir dengan suffix.
 */
int strendsswith(const char *s, const char *suffix) {
    size_t s_len;
    size_t suffix_len;
    
    if (s == NULL || suffix == NULL) {
        return 0;
    }
    
    s_len = strlen(s);
    suffix_len = strlen(suffix);
    
    if (suffix_len > s_len) {
        return 0;
    }
    
    return strcmp(s + s_len - suffix_len, suffix) == 0;
}

/* ============================================================
 * STRSTARTSWITH (internal)
 * ============================================================
 * Cek apakah string dimulai dengan prefix.
 */
int strstartswith(const char *s, const char *prefix) {
    if (s == NULL || prefix == NULL) {
        return 0;
    }
    
    while (*prefix != '\0') {
        if (*s != *prefix) {
            return 0;
        }
        s++;
        prefix++;
    }
    
    return 1;
}
