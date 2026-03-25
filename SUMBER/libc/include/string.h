/*
 * PIGURA LIBC - STRING.H
 * =======================
 * Fungsi manipulasi string dan memori sesuai standar C89/C90
 * dengan tambahan fungsi aman dari POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#ifndef LIBC_STRING_H
#define LIBC_STRING_H

/* ============================================================
 * DEPENDENSI
 * ============================================================
 */
#include <stddef.h>

/* ============================================================
 * NULL POINTER
 * ============================================================
 */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* ============================================================
 * FUNGSI MANIPULASI MEMORI
 * ============================================================
 */

/*
 * memcpy - Salin blok memori
 * Menyalin n byte dari src ke dest. Area tidak boleh overlap.
 *
 * Parameter:
 *   dest - Pointer ke blok tujuan
 *   src  - Pointer ke blok sumber
 *   n    - Jumlah byte yang disalin
 *
 * Return: Pointer ke dest
 *
 * Catatan: Jika area overlap, gunakan memmove().
 */
extern void *memcpy(void *dest, const void *src, size_t n);

/*
 * memmove - Pindahkan blok memori
 * Memindahkan n byte dari src ke dest. Area boleh overlap.
 *
 * Parameter:
 *   dest - Pointer ke blok tujuan
 *   src  - Pointer ke blok sumber
 *   n    - Jumlah byte yang dipindahkan
 *
 * Return: Pointer ke dest
 */
extern void *memmove(void *dest, const void *src, size_t n);

/*
 * memset - Isi blok memori
 * Mengisi n byte pertama blok memori dengan nilai c.
 *
 * Parameter:
 *   s - Pointer ke blok memori
 *   c - Nilai byte untuk pengisian
 *   n - Jumlah byte yang diisi
 *
 * Return: Pointer ke s
 */
extern void *memset(void *s, int c, size_t n);

/*
 * memcmp - Bandingkan dua blok memori
 * Membandingkan n byte pertama dari dua blok memori.
 *
 * Parameter:
 *   s1 - Pointer ke blok pertama
 *   s2 - Pointer ke blok kedua
 *   n  - Jumlah byte yang dibandingkan
 *
 * Return: 0 jika sama, <0 jika s1<s2, >0 jika s1>s2
 */
extern int memcmp(const void *s1, const void *s2, size_t n);

/*
 * memchr - Cari karakter dalam blok memori
 * Mencari kemunculan pertama c dalam n byte pertama s.
 *
 * Parameter:
 *   s - Pointer ke blok memori
 *   c - Karakter yang dicari
 *   n - Jumlah byte yang dicari
 *
 * Return: Pointer ke karakter, atau NULL jika tidak ditemukan
 */
extern void *memchr(const void *s, int c, size_t n);

/* ============================================================
 * FUNGSI STRING - PENGGABUNGAN
 * ============================================================
 */

/*
 * strcpy - Salin string
 * Menyalin string dari src ke dest, termasuk null terminator.
 *
 * Parameter:
 *   dest - Buffer tujuan
 *   src  - String sumber
 *
 * Return: Pointer ke dest
 *
 * Peringatan: Tidak aman jika buffer dest terlalu kecil.
 * Gunakan strncpy untuk keamanan.
 */
extern char *strcpy(char *dest, const char *src);

/*
 * strncpy - Salin string dengan batas
 * Menyalin maksimal n karakter dari src ke dest.
 *
 * Parameter:
 *   dest - Buffer tujuan
 *   src  - String sumber
 *   n    - Ukuran buffer dest
 *
 * Return: Pointer ke dest
 *
 * Catatan: Jika src lebih pendek dari n, dest diisi dengan
 * null hingga n karakter. Jika src >= n, dest tidak
 * null-terminated.
 */
extern char *strncpy(char *dest, const char *src, size_t n);

/*
 * strcat - Gabungkan string
 * Menambahkan src ke akhir dest.
 *
 * Parameter:
 *   dest - String tujuan (harus null-terminated)
 *   src  - String yang ditambahkan
 *
 * Return: Pointer ke dest
 *
 * Peringatan: Tidak aman jika buffer dest terlalu kecil.
 * Gunakan strncat untuk keamanan.
 */
extern char *strcat(char *dest, const char *src);

/*
 * strncat - Gabungkan string dengan batas
 * Menambahkan maksimal n karakter dari src ke dest.
 *
 * Parameter:
 *   dest - String tujuan (harus null-terminated)
 *   src  - String yang ditambahkan
 *   n    - Jumlah maksimal karakter dari src
 *
 * Return: Pointer ke dest
 *
 * Catatan: Selalu menambahkan null terminator.
 */
extern char *strncat(char *dest, const char *src, size_t n);

/* ============================================================
 * FUNGSI STRING - PENGUKURAN
 * ============================================================
 */

/*
 * strlen - Hitung panjang string
 *
 * Parameter:
 *   s - String yang diukur
 *
 * Return: Panjang string (tanpa null terminator)
 */
extern size_t strlen(const char *s);

/*
 * strnlen - Hitung panjang string dengan batas
 *
 * Parameter:
 *   s      - String yang diukur
 *   maxlen - Panjang maksimum
 *
 * Return: strlen(s) jika < maxlen, atau maxlen
 */
extern size_t strnlen(const char *s, size_t maxlen);

/* ============================================================
 * FUNGSI STRING - PERBANDINGAN
 * ============================================================
 */

/*
 * strcmp - Bandingkan dua string
 *
 * Parameter:
 *   s1 - String pertama
 *   s2 - String kedua
 *
 * Return: 0 jika sama, <0 jika s1<s2, >0 jika s1>s2
 */
extern int strcmp(const char *s1, const char *s2);

/*
 * strncmp - Bandingkan n karakter pertama dua string
 *
 * Parameter:
 *   s1 - String pertama
 *   s2 - String kedua
 *   n  - Jumlah karakter maksimum
 *
 * Return: 0 jika sama, <0 jika s1<s2, >0 jika s1>s2
 */
extern int strncmp(const char *s1, const char *s2, size_t n);

/*
 * strcasecmp - Bandingkan string case-insensitive (POSIX)
 *
 * Parameter:
 *   s1 - String pertama
 *   s2 - String kedua
 *
 * Return: 0 jika sama (ignoring case)
 */
extern int strcasecmp(const char *s1, const char *s2);

/*
 * strncasecmp - Bandingkan n karakter case-insensitive (POSIX)
 *
 * Parameter:
 *   s1 - String pertama
 *   s2 - String kedua
 *   n  - Jumlah karakter maksimum
 *
 * Return: 0 jika sama (ignoring case)
 */
extern int strncasecmp(const char *s1, const char *s2, size_t n);

/* ============================================================
 * FUNGSI STRING - PENCARIAN
 * ============================================================
 */

/*
 * strchr - Cari karakter pertama dalam string
 *
 * Parameter:
 *   s - String yang dicari
 *   c - Karakter yang dicari
 *
 * Return: Pointer ke karakter, atau NULL
 */
extern char *strchr(const char *s, int c);

/*
 * strrchr - Cari karakter terakhir dalam string
 *
 * Parameter:
 *   s - String yang dicari
 *   c - Karakter yang dicari
 *
 * Return: Pointer ke karakter terakhir, atau NULL
 */
extern char *strrchr(const char *s, int c);

/*
 * strstr - Cari substring dalam string
 *
 * Parameter:
 *   haystack - String yang dicari
 *   needle   - Substring yang dicari
 *
 * Return: Pointer ke substring, atau NULL
 */
extern char *strstr(const char *haystack, const char *needle);

/*
 * strpbrk - Cari karakter dari set dalam string
 *
 * Parameter:
 *   s   - String yang dicari
 *   accept - Set karakter yang dicari
 *
 * Return: Pointer ke karakter pertama dari accept, atau NULL
 */
extern char *strpbrk(const char *s, const char *accept);

/*
 * strspn - Hitung panjang awalan yang mengandung karakter
 *
 * Parameter:
 *   s      - String yang diperiksa
 *   accept - Set karakter yang diizinkan
 *
 * Return: Panjang awalan yang hanya mengandung karakter accept
 */
extern size_t strspn(const char *s, const char *accept);

/*
 * strcspn - Hitung panjang awalan tanpa karakter
 *
 * Parameter:
 *   s       - String yang diperiksa
 *   reject  - Set karakter yang ditolak
 *
 * Return: Panjang awalan tanpa karakter reject
 */
extern size_t strcspn(const char *s, const char *reject);

/*
 * strtok - Tokenize string (NON-REENTRANT)
 *
 * Parameter:
 *   str    - String yang di-tokenize (NULL untuk lanjutan)
 *   delim  - Set delimiter
 *
 * Return: Pointer ke token berikutnya, atau NULL
 *
 * Peringatan: Tidak thread-safe. Gunakan strtok_r.
 */
extern char *strtok(char *str, const char *delim);

/*
 * strtok_r - Tokenize string (REENTRANT, POSIX)
 *
 * Parameter:
 *   str    - String yang di-tokenize (NULL untuk lanjutan)
 *   delim  - Set delimiter
 *   saveptr - Pointer untuk menyimpan state
 *
 * Return: Pointer ke token berikutnya, atau NULL
 */
extern char *strtok_r(char *str, const char *delim, char **saveptr);

/* ============================================================
 * FUNGSI STRING - LAINNYA
 * ============================================================
 */

/*
 * strerror - Dapatkan pesan error
 *
 * Parameter:
 *   errnum - Kode error
 *
 * Return: String pesan error
 */
extern char *strerror(int errnum);

/*
 * strdup - Duplikasi string (POSIX)
 *
 * Parameter:
 *   s - String yang diduplikasi
 *
 * Return: Pointer ke string baru, atau NULL jika gagal
 *
 * Catatan: Memori harus di-free oleh pemanggil.
 */
extern char *strdup(const char *s);

/*
 * strndup - Duplikasi string dengan batas (POSIX)
 *
 * Parameter:
 *   s - String yang diduplikasi
 *   n - Panjang maksimum
 *
 * Return: Pointer ke string baru, atau NULL jika gagal
 */
extern char *strndup(const char *s, size_t n);

/*
 * strcoll - Bandingkan string menggunakan locale
 *
 * Parameter:
 *   s1 - String pertama
 *   s2 - String kedua
 *
 * Return: 0 jika sama, <0 jika s1<s2, >0 jika s1>s2
 */
extern int strcoll(const char *s1, const char *s2);

/*
 * strxfrm - Transformasi string untuk collation
 *
 * Parameter:
 *   dest - Buffer tujuan
 *   src  - String sumber
 *   n    - Ukuran buffer dest
 *
 * Return: Panjang string yang diperlukan
 */
extern size_t strxfrm(char *dest, const char *src, size_t n);

/*
 * strrev - Balik string (NON-STANDAR)
 *
 * Parameter:
 *   s - String yang dibalik (in-place)
 *
 * Return: Pointer ke s
 */
extern char *strrev(char *s);

/*
 * strtrim - Hapus whitespace dari ujung string (NON-STANDAR)
 *
 * Parameter:
 *   s - String yang di-trim (in-place)
 *
 * Return: Pointer ke s
 */
extern char *strtrim(char *s);

/* ============================================================
 * KONSTANTA ERROR
 * ============================================================
 */

/* Pesan error string */
#define STRERROR_MAX_LEN 64

#endif /* LIBC_STRING_H */
