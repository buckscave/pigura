/*
 * PIGURA LIBC - STDLIB.H
 * =======================
 * Fungsi utilitas umum sesuai standar C89/C90.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#ifndef LIBC_STDLIB_H
#define LIBC_STDLIB_H

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
 * KONSTANTA
 * ============================================================
 */

/* Exit status */
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

/* Rentang nilai random */
#define RAND_MAX 32767

/* Pembagi untuk hasil div() */
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

/* Alignment default */
#define ALIGN_DEFAULT 8

/* ============================================================
 * TIPE DATA
 * ============================================================
 */

/* Tipe untuk hasil div() */
typedef struct {
    int quot;   /* Quotient (hasil bagi) */
    int rem;    /* Remainder (sisa) */
} div_t;

/* Tipe untuk hasil ldiv() */
typedef struct {
    long quot;  /* Quotient (hasil bagi) */
    long rem;   /* Remainder (sisa) */
} ldiv_t;

/* Tipe untuk hasil lldiv() (C99) */
typedef struct {
    long long quot;  /* Quotient */
    long long rem;   /* Remainder */
} lldiv_t;

/* Tipe untuk comparison function */
typedef int (*compar_fn_t)(const void *, const void *);

/* ============================================================
 * FUNGSI KONVERSI STRING KE ANGKA
 * ============================================================
 */

/*
 * atof - Konversi string ke double
 *
 * Parameter:
 *   nptr - String input
 *
 * Return: Nilai double hasil konversi
 */
extern double atof(const char *nptr);

/*
 * atoi - Konversi string ke int
 *
 * Parameter:
 *   nptr - String input
 *
 * Return: Nilai int hasil konversi
 */
extern int atoi(const char *nptr);

/*
 * atol - Konversi string ke long
 *
 * Parameter:
 *   nptr - String input
 *
 * Return: Nilai long hasil konversi
 */
extern long atol(const char *nptr);

/*
 * atoll - Konversi string ke long long (C99)
 *
 * Parameter:
 *   nptr - String input
 *
 * Return: Nilai long long hasil konversi
 */
extern long long atoll(const char *nptr);

/*
 * strtol - Konversi string ke long dengan error handling
 *
 * Parameter:
 *   nptr   - String input
 *   endptr - Pointer untuk menyimpan posisi akhir (boleh NULL)
 *   base   - Basis angka (0, 2-36)
 *
 * Return: Nilai long hasil konversi
 *
 * Catatan: Set errno ke ERANGE jika overflow.
 */
extern long strtol(const char *nptr, char **endptr, int base);

/*
 * strtoll - Konversi string ke long long (C99)
 *
 * Parameter:
 *   nptr   - String input
 *   endptr - Pointer untuk menyimpan posisi akhir
 *   base   - Basis angka (0, 2-36)
 *
 * Return: Nilai long long hasil konversi
 */
extern long long strtoll(const char *nptr, char **endptr, int base);

/*
 * strtoul - Konversi string ke unsigned long
 *
 * Parameter:
 *   nptr   - String input
 *   endptr - Pointer untuk menyimpan posisi akhir
 *   base   - Basis angka (0, 2-36)
 *
 * Return: Nilai unsigned long hasil konversi
 */
extern unsigned long strtoul(const char *nptr, char **endptr, int base);

/*
 * strtoull - Konversi string ke unsigned long long (C99)
 *
 * Parameter:
 *   nptr   - String input
 *   endptr - Pointer untuk menyimpan posisi akhir
 *   base   - Basis angka (0, 2-36)
 *
 * Return: Nilai unsigned long long hasil konversi
 */
extern unsigned long long strtoull(const char *nptr, char **endptr,
                                   int base);

/*
 * strtof - Konversi string ke float (C99)
 *
 * Parameter:
 *   nptr   - String input
 *   endptr - Pointer untuk menyimpan posisi akhir
 *
 * Return: Nilai float hasil konversi
 */
extern float strtof(const char *nptr, char **endptr);

/*
 * strtod - Konversi string ke double
 *
 * Parameter:
 *   nptr   - String input
 *   endptr - Pointer untuk menyimpan posisi akhir
 *
 * Return: Nilai double hasil konversi
 */
extern double strtod(const char *nptr, char **endptr);

/*
 * strtold - Konversi string ke long double (C99)
 *
 * Parameter:
 *   nptr   - String input
 *   endptr - Pointer untuk menyimpan posisi akhir
 *
 * Return: Nilai long double hasil konversi
 */
extern long double strtold(const char *nptr, char **endptr);

/* ============================================================
 * FUNGSI KONVERSI ANGKA KE STRING
 * ============================================================
 */

/*
 * itoa - Konversi int ke string (NON-STANDAR)
 *
 * Parameter:
 *   value - Nilai yang dikonversi
 *   str   - Buffer tujuan
 *   base  - Basis output (2-36)
 *
 * Return: Pointer ke str
 */
extern char *itoa(int value, char *str, int base);

/*
 * ltoa - Konversi long ke string (NON-STANDAR)
 *
 * Parameter:
 *   value - Nilai yang dikonversi
 *   str   - Buffer tujuan
 *   base  - Basis output (2-36)
 *
 * Return: Pointer ke str
 */
extern char *ltoa(long value, char *str, int base);

/*
 * lltoa - Konversi long long ke string (NON-STANDAR)
 *
 * Parameter:
 *   value - Nilai yang dikonversi
 *   str   - Buffer tujuan
 *   base  - Basis output (2-36)
 *
 * Return: Pointer ke str
 */
extern char *lltoa(long long value, char *str, int base);

/*
 * utoa - Konversi unsigned int ke string (NON-STANDAR)
 *
 * Parameter:
 *   value - Nilai yang dikonversi
 *   str   - Buffer tujuan
 *   base  - Basis output (2-36)
 *
 * Return: Pointer ke str
 */
extern char *utoa(unsigned int value, char *str, int base);

/*
 * ultoa - Konversi unsigned long ke string (NON-STANDAR)
 *
 * Parameter:
 *   value - Nilai yang dikonversi
 *   str   - Buffer tujuan
 *   base  - Basis output (2-36)
 *
 * Return: Pointer ke str
 */
extern char *ultoa(unsigned long value, char *str, int base);

/*
 * ulltoa - Konversi unsigned long long ke string (NON-STANDAR)
 *
 * Parameter:
 *   value - Nilai yang dikonversi
 *   str   - Buffer tujuan
 *   base  - Basis output (2-36)
 *
 * Return: Pointer ke str
 */
extern char *ulltoa(unsigned long long value, char *str, int base);

/* ============================================================
 * FUNGSI RANDOM NUMBER
 * ============================================================
 */

/*
 * rand - Generate random number
 *
 * Return: Integer random dalam rentang [0, RAND_MAX]
 */
extern int rand(void);

/*
 * srand - Set seed untuk random number generator
 *
 * Parameter:
 *   seed - Nilai seed
 */
extern void srand(unsigned int seed);

/* ============================================================
 * FUNGSI ALOKASI MEMORI
 * ============================================================
 */

/*
 * malloc - Alokasikan memori
 *
 * Parameter:
 *   size - Ukuran memori yang dialokasikan
 *
 * Return: Pointer ke memori, atau NULL jika gagal
 */
extern void *malloc(size_t size);

/*
 * calloc - Alokasikan dan inisialisasi memori
 *
 * Parameter:
 *   nmemb - Jumlah elemen
 *   size  - Ukuran setiap elemen
 *
 * Return: Pointer ke memori yang sudah di-zero, atau NULL
 */
extern void *calloc(size_t nmemb, size_t size);

/*
 * realloc - Realokasikan memori
 *
 * Parameter:
 *   ptr  - Pointer ke memori lama (boleh NULL)
 *   size - Ukuran baru
 *
 * Return: Pointer ke memori baru, atau NULL jika gagal
 *
 * Catatan: Jika gagal, memori lama tidak di-free.
 */
extern void *realloc(void *ptr, size_t size);

/*
 * free - Bebaskan memori
 *
 * Parameter:
 *   ptr - Pointer ke memori (boleh NULL)
 */
extern void free(void *ptr);

/*
 * aligned_alloc - Alokasikan memori dengan alignment (C11)
 *
 * Parameter:
 *   alignment - Alignment yang diminta
 *   size      - Ukuran memori
 *
 * Return: Pointer ke memori yang aligned, atau NULL
 */
extern void *aligned_alloc(size_t alignment, size_t size);

/*
 * memalign - Alokasikan memori dengan alignment (POSIX)
 *
 * Parameter:
 *   alignment - Alignment yang diminta
 *   size      - Ukuran memori
 *
 * Return: Pointer ke memori yang aligned, atau NULL
 */
extern void *memalign(size_t alignment, size_t size);

/*
 * posix_memalign - Alokasikan memori aligned (POSIX)
 *
 * Parameter:
 *   memptr    - Pointer untuk menyimpan hasil
 *   alignment - Alignment yang diminta
 *   size      - Ukuran memori
 *
 * Return: 0 jika berhasil, error code jika gagal
 */
extern int posix_memalign(void **memptr, size_t alignment, size_t size);

/*
 * valloc - Alokasikan memori page-aligned (BSD)
 *
 * Parameter:
 *   size - Ukuran memori
 *
 * Return: Pointer ke memori yang page-aligned, atau NULL
 */
extern void *valloc(size_t size);

/* ============================================================
 * FUNGSI KONTROL PROSES
 * ============================================================
 */

/*
 * abort - Hentikan program secara abnormal
 *
 * Menghasilkan sinyal SIGABRT dan menghentikan program.
 */
extern void abort(void) __attribute__((noreturn));

/*
 * exit - Hentikan program secara normal
 *
 * Parameter:
 *   status - Exit status
 *
 * Memanggil fungsi terdaftar dengan atexit() dan flush
 * semua stream sebelum menghentikan program.
 */
extern void exit(int status) __attribute__((noreturn));

/*
 * _exit - Hentikan program tanpa cleanup
 *
 * Parameter:
 *   status - Exit status
 *
 * Menghentikan program tanpa memanggil atexit handlers.
 */
extern void _exit(int status) __attribute__((noreturn));

/*
 * _Exit - Hentikan program tanpa cleanup (C99)
 *
 * Parameter:
 *   status - Exit status
 */
extern void _Exit(int status) __attribute__((noreturn));

/*
 * atexit - Daftarkan fungsi untuk dipanggil saat exit
 *
 * Parameter:
 *   func - Fungsi yang didaftarkan
 *
 * Return: 0 jika berhasil, non-zero jika gagal
 */
extern int atexit(void (*func)(void));

/*
 * at_quick_exit - Daftarkan fungsi untuk quick_exit (C11)
 *
 * Parameter:
 *   func - Fungsi yang didaftarkan
 *
 * Return: 0 jika berhasil, non-zero jika gagal
 */
extern int at_quick_exit(void (*func)(void));

/*
 * quick_exit - Hentikan program dengan quick handlers (C11)
 *
 * Parameter:
 *   status - Exit status
 */
extern void quick_exit(int status) __attribute__((noreturn));

/*
 * getenv - Dapatkan nilai environment variable
 *
 * Parameter:
 *   name - Nama environment variable
 *
 * Return: Nilai variable, atau NULL jika tidak ada
 */
extern char *getenv(const char *name);

/*
 * setenv - Set nilai environment variable (POSIX)
 *
 * Parameter:
 *   name      - Nama variable
 *   value     - Nilai variable
 *   overwrite - Overwrite jika sudah ada
 *
 * Return: 0 jika berhasil, -1 jika gagal
 */
extern int setenv(const char *name, const char *value, int overwrite);

/*
 * unsetenv - Hapus environment variable (POSIX)
 *
 * Parameter:
 *   name - Nama variable yang dihapus
 *
 * Return: 0 jika berhasil, -1 jika gagal
 */
extern int unsetenv(const char *name);

/*
 * putenv - Set environment variable (POSIX)
 *
 * Parameter:
 *   string - String "name=value"
 *
 * Return: 0 jika berhasil, non-zero jika gagal
 */
extern int putenv(char *string);

/*
 * clearenv - Hapus semua environment variables (BSD)
 *
 * Return: 0 jika berhasil, non-zero jika gagal
 */
extern int clearenv(void);

/*
 * system - Jalankan perintah shell
 *
 * Parameter:
 *   command - Perintah yang dijalankan
 *
 * Return: Status perintah, atau -1 jika shell tidak tersedia
 */
extern int system(const char *command);

/* ============================================================
 * FUNGSI SORTING DAN SEARCHING
 * ============================================================
 */

/*
 * bsearch - Binary search
 *
 * Parameter:
 *   key    - Elemen yang dicari
 *   base   - Array awal
 *   nmemb  - Jumlah elemen
 *   size   - Ukuran setiap elemen
 *   compar - Fungsi perbandingan
 *
 * Return: Pointer ke elemen yang ditemukan, atau NULL
 */
extern void *bsearch(const void *key, const void *base, size_t nmemb,
                     size_t size, compar_fn_t compar);

/*
 * qsort - Quick sort
 *
 * Parameter:
 *   base   - Array yang diurutkan
 *   nmemb  - Jumlah elemen
 *   size   - Ukuran setiap elemen
 *   compar - Fungsi perbandingan
 */
extern void qsort(void *base, size_t nmemb, size_t size,
                  compar_fn_t compar);

/* ============================================================
 * FUNGSI ARITMETIKA INTEGER
 * ============================================================
 */

/*
 * abs - Nilai absolut int
 *
 * Parameter:
 *   n - Nilai input
 *
 * Return: Nilai absolut
 */
extern int abs(int n);

/*
 * labs - Nilai absolut long
 *
 * Parameter:
 *   n - Nilai input
 *
 * Return: Nilai absolut
 */
extern long labs(long n);

/*
 * llabs - Nilai absolut long long (C99)
 *
 * Parameter:
 *   n - Nilai input
 *
 * Return: Nilai absolut
 */
extern long long llabs(long long n);

/*
 * div - Pembagian int
 *
 * Parameter:
 *   numer - Pembilang
 *   denom - Penyebut
 *
 * Return: Struktur dengan quot dan rem
 */
extern div_t div(int numer, int denom);

/*
 * ldiv - Pembagian long
 *
 * Parameter:
 *   numer - Pembilang
 *   denom - Penyebut
 *
 * Return: Struktur dengan quot dan rem
 */
extern ldiv_t ldiv(long numer, long denom);

/*
 * lldiv - Pembagian long long (C99)
 *
 * Parameter:
 *   numer - Pembilang
 *   denom - Penyebut
 *
 * Return: Struktur dengan quot dan rem
 */
extern lldiv_t lldiv(long long numer, long long denom);

/* ============================================================
 * FUNGSI KONVERSI KARAKTER MULTIBYTE
 * ============================================================
 */

/*
 * mblen - Panjang karakter multibyte
 *
 * Parameter:
 *   s   - String multibyte (boleh NULL)
 *   n   - Jumlah byte maksimum
 *
 * Return: Panjang karakter, atau -1 jika error
 */
extern int mblen(const char *s, size_t n);

/*
 * mbtowc - Konversi multibyte ke wide character
 *
 * Parameter:
 *   pwc - Pointer ke wide char output
 *   s   - String multibyte
 *   n   - Jumlah byte maksimum
 *
 * Return: Jumlah byte yang dikonsumsi, atau -1 jika error
 */
extern int mbtowc(wchar_t *pwc, const char *s, size_t n);

/*
 * wctomb - Konversi wide character ke multibyte
 *
 * Parameter:
 *   s  - Buffer output (boleh NULL)
 *   wc - Wide character
 *
 * Return: Jumlah byte yang ditulis, atau -1 jika error
 */
extern int wctomb(char *s, wchar_t wc);

/*
 * mbstowcs - Konversi string multibyte ke wide string
 *
 * Parameter:
 *   pwcs - Buffer wide string output
 *   s    - String multibyte input
 *   n    - Jumlah karakter maksimum
 *
 * Return: Jumlah karakter yang dikonversi, atau -1 jika error
 */
extern size_t mbstowcs(wchar_t *pwcs, const char *s, size_t n);

/*
 * wcstombs - Konversi wide string ke string multibyte
 *
 * Parameter:
 *   s    - Buffer output
 *   pwcs - Wide string input
 *   n    - Jumlah byte maksimum
 *
 * Return: Jumlah byte yang ditulis, atau -1 jika error
 */
extern size_t wcstombs(char *s, const wchar_t *pwcs, size_t n);

/* ============================================================
 * FUNGSI UTILITAS TAMBAHAN
 * ============================================================
 */

/*
 * min - Nilai minimum (macro)
 */
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

/*
 * max - Nilai maksimum (macro)
 */
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

/*
 * clamp - Batasi nilai dalam rentang
 */
#ifndef clamp
#define clamp(val, lo, hi) min(max(val, lo), hi)
#endif

/*
 * swap - Tukar dua nilai
 */
#define swap(a, b, type) \
    do { \
        type _tmp = (a); \
        (a) = (b); \
        (b) = _tmp; \
    } while (0)

/*
 * ARRAY_SIZE - Dapatkan jumlah elemen array
 */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/*
 * ALIGN_UP - Ratakan ke atas ke alignment
 */
#define ALIGN_UP(val, align) \
    (((val) + ((align) - 1)) & ~((align) - 1))

/*
 * ALIGN_DOWN - Ratakan ke bawah ke alignment
 */
#define ALIGN_DOWN(val, align) \
    ((val) & ~((align) - 1))

/*
 * IS_ALIGNED - Cek apakah sudah aligned
 */
#define IS_ALIGNED(val, align) \
    (((val) & ((align) - 1)) == 0)

#endif /* LIBC_STDLIB_H */
