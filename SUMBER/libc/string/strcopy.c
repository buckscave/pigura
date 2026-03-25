/*
 * PIGURA LIBC - STRING/STRCOPY.C
 * ==============================
 * Implementasi fungsi copy dan concatenation string.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#include <string.h>
#include <stddef.h>
#include <stdlib.h>

/* ============================================================
 * STRCPY
 * ============================================================
 * Salin string dari src ke dest.
 */
char *strcpy(char *dest, const char *src) {
    char *d;
    
    if (dest == NULL) {
        return NULL;
    }
    
    if (src == NULL) {
        *dest = '\0';
        return dest;
    }
    
    d = dest;
    while ((*d++ = *src++) != '\0') {
        /* Copy sampai null terminator */
    }
    
    return dest;
}

/* ============================================================
 * STRNCPY
 * ============================================================
 * Salin maksimal n karakter dari src ke dest.
 */
char *strncpy(char *dest, const char *src, size_t n) {
    char *d;
    size_t i;
    
    if (dest == NULL || n == 0) {
        return dest;
    }
    
    d = dest;
    
    if (src != NULL) {
        /* Copy karakter dari src */
        for (i = 0; i < n && src[i] != '\0'; i++) {
            d[i] = src[i];
        }
        
        /* Pad dengan null jika perlu */
        for (; i < n; i++) {
            d[i] = '\0';
        }
    } else {
        /* src NULL, isi dengan null */
        for (i = 0; i < n; i++) {
            d[i] = '\0';
        }
    }
    
    return dest;
}

/* ============================================================
 * STRCAT
 * ============================================================
 * Tambahkan src ke akhir dest.
 */
char *strcat(char *dest, const char *src) {
    char *d;
    
    if (dest == NULL) {
        return NULL;
    }
    
    if (src == NULL || *src == '\0') {
        return dest;
    }
    
    /* Cari akhir dest */
    d = dest;
    while (*d != '\0') {
        d++;
    }
    
    /* Copy src ke akhir dest */
    while ((*d++ = *src++) != '\0') {
        /* Copy sampai null terminator */
    }
    
    return dest;
}

/* ============================================================
 * STRNCAT
 * ============================================================
 * Tambahkan maksimal n karakter dari src ke dest.
 */
char *strncat(char *dest, const char *src, size_t n) {
    char *d;
    size_t i;
    
    if (dest == NULL || n == 0) {
        return dest;
    }
    
    if (src == NULL || *src == '\0') {
        return dest;
    }
    
    /* Cari akhir dest */
    d = dest;
    while (*d != '\0') {
        d++;
    }
    
    /* Copy maksimal n karakter */
    for (i = 0; i < n && src[i] != '\0'; i++) {
        d[i] = src[i];
    }
    
    /* Selalu tambahkan null terminator */
    d[i] = '\0';
    
    return dest;
}

/* ============================================================
 * STRDUP
 * ============================================================
 * Duplikasi string. Memori harus di-free oleh pemanggil.
 */
char *strdup(const char *s) {
    size_t len;
    char *copy;
    
    if (s == NULL) {
        return NULL;
    }
    
    len = strlen(s) + 1;  /* +1 untuk null terminator */
    
    copy = (char *)malloc(len);
    if (copy == NULL) {
        return NULL;
    }
    
    memcpy(copy, s, len);
    return copy;
}

/* ============================================================
 * STRNDUP
 * ============================================================
 * Duplikasi maksimal n karakter.
 */
char *strndup(const char *s, size_t n) {
    size_t len;
    char *copy;
    
    if (s == NULL) {
        return NULL;
    }
    
    /* Hitung panjang maksimal */
    len = strnlen(s, n);
    
    copy = (char *)malloc(len + 1);
    if (copy == NULL) {
        return NULL;
    }
    
    memcpy(copy, s, len);
    copy[len] = '\0';
    
    return copy;
}

/* ============================================================
 * STRCPYN (internal)
 * ============================================================
 * Copy dengan panjang output maksimal.
 */
size_t strcpyn(char *dest, size_t destsize, const char *src) {
    size_t srclen;
    size_t copylen;
    
    if (dest == NULL || destsize == 0) {
        return 0;
    }
    
    if (src == NULL) {
        *dest = '\0';
        return 0;
    }
    
    srclen = strlen(src);
    
    if (srclen < destsize) {
        copylen = srclen;
    } else {
        copylen = destsize - 1;
    }
    
    memcpy(dest, src, copylen);
    dest[copylen] = '\0';
    
    return copylen;
}

/* ============================================================
 * STRLCAT (BSD extension)
 * ============================================================
 * Concatenate dengan batas buffer yang aman.
 */
size_t strlcat(char *dest, const char *src, size_t destsize) {
    size_t destlen;
    size_t srclen;
    size_t space;
    size_t copylen;
    
    if (dest == NULL || destsize == 0) {
        return 0;
    }
    
    /* Cari panjang dest yang sudah ada */
    destlen = 0;
    while (destlen < destsize && dest[destlen] != '\0') {
        destlen++;
    }
    
    if (destlen >= destsize) {
        /* dest tidak null-terminated */
        return destlen + strlen(src);
    }
    
    if (src == NULL) {
        return destlen;
    }
    
    srclen = strlen(src);
    space = destsize - destlen - 1;  /* -1 untuk null terminator */
    
    if (srclen <= space) {
        copylen = srclen;
    } else {
        copylen = space;
    }
    
    memcpy(dest + destlen, src, copylen);
    dest[destlen + copylen] = '\0';
    
    return destlen + srclen;
}

/* ============================================================
 * STRLCPY (BSD extension)
 * ============================================================
 * Copy dengan batas buffer yang aman.
 */
size_t strlcpy(char *dest, const char *src, size_t destsize) {
    size_t srclen;
    size_t copylen;
    
    if (dest == NULL || destsize == 0) {
        return (src == NULL) ? 0 : strlen(src);
    }
    
    if (src == NULL) {
        *dest = '\0';
        return 0;
    }
    
    srclen = strlen(src);
    
    if (srclen < destsize) {
        copylen = srclen;
    } else {
        copylen = destsize - 1;
    }
    
    memcpy(dest, src, copylen);
    dest[copylen] = '\0';
    
    return srclen;
}

/* ============================================================
 * STRREV (non-standard)
 * ============================================================
 * Balik string in-place.
 */
char *strrev(char *s) {
    char *start;
    char *end;
    char temp;
    
    if (s == NULL || *s == '\0') {
        return s;
    }
    
    start = s;
    end = s + strlen(s) - 1;
    
    while (start < end) {
        temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
    
    return s;
}

/* ============================================================
 * STRTRIM (non-standard)
 * ============================================================
 * Hapus whitespace dari kedua ujung string in-place.
 */
char *strtrim(char *s) {
    char *start;
    char *end;
    char *write;
    
    if (s == NULL) {
        return NULL;
    }
    
    /* Skip leading whitespace */
    start = s;
    while (*start == ' ' || *start == '\t' || *start == '\n' ||
           *start == '\r' || *start == '\v' || *start == '\f') {
        start++;
    }
    
    /* Jika string semua whitespace */
    if (*start == '\0') {
        *s = '\0';
        return s;
    }
    
    /* Cari trailing whitespace */
    end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t' ||
           *end == '\n' || *end == '\r' || *end == '\v' ||
           *end == '\f')) {
        end--;
    }
    
    /* Pindahkan ke awal buffer */
    if (start != s) {
        write = s;
        while (start <= end) {
            *write++ = *start++;
        }
        *write = '\0';
    } else {
        *(end + 1) = '\0';
    }
    
    return s;
}

/* ============================================================
 * STRTRIM_LEFT (non-standard)
 * ============================================================
 * Hapus whitespace dari awal string.
 */
char *strtrim_left(char *s) {
    char *read;
    char *write;
    
    if (s == NULL) {
        return NULL;
    }
    
    /* Skip leading whitespace */
    read = s;
    while (*read == ' ' || *read == '\t' || *read == '\n' ||
           *read == '\r' || *read == '\v' || *read == '\f') {
        read++;
    }
    
    /* Pindahkan ke awal buffer jika perlu */
    if (read != s) {
        write = s;
        while ((*write++ = *read++) != '\0') {
            /* Copy */
        }
    }
    
    return s;
}

/* ============================================================
 * STRTRIM_RIGHT (non-standard)
 * ============================================================
 * Hapus whitespace dari akhir string.
 */
char *strtrim_right(char *s) {
    size_t len;
    
    if (s == NULL) {
        return NULL;
    }
    
    len = strlen(s);
    
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' ||
           s[len - 1] == '\n' || s[len - 1] == '\r' ||
           s[len - 1] == '\v' || s[len - 1] == '\f')) {
        len--;
    }
    
    s[len] = '\0';
    
    return s;
}

/* Stub untuk malloc jika belum ada */
extern void *malloc(size_t size);
void *malloc(size_t size) {
    (void)size;
    return (void *)0;  /* Stub */
}
