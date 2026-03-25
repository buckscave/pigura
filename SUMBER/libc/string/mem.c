/*
 * PIGURA LIBC - STRING/MEM.C
 * ==========================
 * Implementasi fungsi manipulasi memori.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#include <string.h>
#include <stddef.h>

/* ============================================================
 * MEMCPY
 * ============================================================
 * Salin blok memori dari src ke dest.
 * Area sumber dan tujuan tidak boleh overlap.
 */
void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d;
    const unsigned char *s;
    
    if (n == 0 || dest == src) {
        return dest;
    }
    
    d = (unsigned char *)dest;
    s = (const unsigned char *)src;
    
    /* Copy byte by byte */
    while (n--) {
        *d++ = *s++;
    }
    
    return dest;
}

/* ============================================================
 * MEMMOVE
 * ============================================================
 * Pindahkan blok memori. Area boleh overlap.
 */
void *memmove(void *dest, const void *src, size_t n) {
    unsigned char *d;
    const unsigned char *s;
    
    if (n == 0 || dest == src) {
        return dest;
    }
    
    d = (unsigned char *)dest;
    s = (const unsigned char *)src;
    
    /* Cek apakah overlap dan perlu copy mundur */
    if (d < s) {
        /* Copy forward (dari awal ke akhir) */
        while (n--) {
            *d++ = *s++;
        }
    } else {
        /* Copy backward (dari akhir ke awal) */
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    }
    
    return dest;
}

/* ============================================================
 * MEMSET
 * ============================================================
 * Isi blok memori dengan nilai tertentu.
 */
void *memset(void *s, int c, size_t n) {
    unsigned char *p;
    unsigned char value;
    
    if (n == 0) {
        return s;
    }
    
    p = (unsigned char *)s;
    value = (unsigned char)c;
    
    while (n--) {
        *p++ = value;
    }
    
    return s;
}

/* ============================================================
 * MEMCMP
 * ============================================================
 * Bandingkan dua blok memori.
 */
int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1;
    const unsigned char *p2;
    
    if (n == 0) {
        return 0;
    }
    
    p1 = (const unsigned char *)s1;
    p2 = (const unsigned char *)s2;
    
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    
    return 0;
}

/* ============================================================
 * MEMCHR
 * ============================================================
 * Cari karakter pertama dalam blok memori.
 */
void *memchr(const void *s, int c, size_t n) {
    const unsigned char *p;
    unsigned char value;
    
    if (n == 0) {
        return NULL;
    }
    
    p = (const unsigned char *)s;
    value = (unsigned char)c;
    
    while (n--) {
        if (*p == value) {
            return (void *)p;
        }
        p++;
    }
    
    return NULL;
}

/* ============================================================
 * MEMRCHR (GNU extension)
 * ============================================================
 * Cari karakter terakhir dalam blok memori.
 */
void *memrchr(const void *s, int c, size_t n) {
    const unsigned char *p;
    unsigned char value;
    
    if (n == 0) {
        return NULL;
    }
    
    p = (const unsigned char *)s + n - 1;
    value = (unsigned char)c;
    
    while (n--) {
        if (*p == value) {
            return (void *)p;
        }
        p--;
    }
    
    return NULL;
}

/* ============================================================
 * MEMMEM (GNU extension)
 * ============================================================
 * Cari substring dalam blok memori.
 */
void *memmem(const void *haystack, size_t haystacklen,
             const void *needle, size_t needlelen) {
    const unsigned char *h;
    const unsigned char *n;
    size_t i;
    
    if (needlelen == 0) {
        return (void *)haystack;
    }
    
    if (needlelen > haystacklen) {
        return NULL;
    }
    
    h = (const unsigned char *)haystack;
    n = (const unsigned char *)needle;
    
    /* Simple search */
    for (i = 0; i <= haystacklen - needlelen; i++) {
        if (h[i] == n[0]) {
            size_t j;
            for (j = 1; j < needlelen; j++) {
                if (h[i + j] != n[j]) {
                    break;
                }
            }
            if (j == needlelen) {
                return (void *)(h + i);
            }
        }
    }
    
    return NULL;
}

/* ============================================================
 * MEMPCPY (GNU extension)
 * ============================================================
 * Seperti memcpy tapi return pointer ke byte setelah
 * terakhir yang ditulis.
 */
void *mempcpy(void *dest, const void *src, size_t n) {
    memcpy(dest, src, n);
    return (unsigned char *)dest + n;
}

/* ============================================================
 * MEMSET32 (internal)
 * ============================================================
 * Isi memori dengan nilai 32-bit.
 */
void *memset32(void *s, unsigned int val, size_t n) {
    unsigned int *p = (unsigned int *)s;
    
    while (n--) {
        *p++ = val;
    }
    
    return s;
}

/* ============================================================
 * MEMSET64 (internal)
 * ============================================================
 * Isi memori dengan nilai 64-bit.
 */
void *memset64(void *s, unsigned long long val, size_t n) {
    unsigned long long *p = (unsigned long long *)s;
    
    while (n--) {
        *p++ = val;
    }
    
    return s;
}

/* ============================================================
 * MEMCCPY (POSIX)
 * ============================================================
 * Copy sampai menemukan karakter c.
 */
void *memccpy(void *dest, const void *src, int c, size_t n) {
    unsigned char *d;
    const unsigned char *s;
    unsigned char value;
    
    if (n == 0) {
        return NULL;
    }
    
    d = (unsigned char *)dest;
    s = (const unsigned char *)src;
    value = (unsigned char)c;
    
    while (n--) {
        *d = *s;
        if (*s == value) {
            return d + 1;
        }
        d++;
        s++;
    }
    
    return NULL;
}

/* ============================================================
 * MEMCMP_CONSTTIME (internal)
 * ============================================================
 * Perbandingan memori dengan waktu konstan (untuk keamanan).
 */
int memcmp_consttime(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1;
    const unsigned char *p2;
    unsigned char result;
    size_t i;
    
    p1 = (const unsigned char *)s1;
    p2 = (const unsigned char *)s2;
    result = 0;
    
    for (i = 0; i < n; i++) {
        result |= p1[i] ^ p2[i];
    }
    
    return result;
}

/* ============================================================
 * MEMSWAP (internal)
 * ============================================================
 * Tukar isi dua blok memori.
 */
void memswap(void *a, void *b, size_t n) {
    unsigned char *pa;
    unsigned char *pb;
    unsigned char temp;
    
    if (n == 0 || a == b) {
        return;
    }
    
    pa = (unsigned char *)a;
    pb = (unsigned char *)b;
    
    while (n--) {
        temp = *pa;
        *pa++ = *pb;
        *pb++ = temp;
    }
}

/* ============================================================
 * MEMALIGN (internal)
 * ============================================================
 * Cek apakah pointer aligned.
 */
int mem_is_aligned(const void *ptr, size_t alignment) {
    return ((size_t)ptr & (alignment - 1)) == 0;
}

/* ============================================================
 * MEMCPY_ALIGNED (internal)
 * ============================================================
 * Copy dengan asumsi pointer sudah aligned.
 */
void *memcpy_aligned(void *dest, const void *src, size_t n) {
    unsigned long *d;
    const unsigned long *s;
    size_t word_size;
    size_t words;
    size_t bytes;
    
    word_size = sizeof(unsigned long);
    words = n / word_size;
    bytes = n % word_size;
    
    /* Copy words */
    d = (unsigned long *)dest;
    s = (const unsigned long *)src;
    
    while (words--) {
        *d++ = *s++;
    }
    
    /* Copy remaining bytes */
    {
        unsigned char *db = (unsigned char *)d;
        const unsigned char *sb = (const unsigned char *)s;
        
        while (bytes--) {
            *db++ = *sb++;
        }
    }
    
    return dest;
}
