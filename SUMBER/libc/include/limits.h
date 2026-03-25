/*
 * PIGURA LIBC - LIMITS.H
 * =======================
 * Definisi batas numerik untuk tipe integer sesuai standar
 * C89/C90.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#ifndef LIBC_LIMITS_H
#define LIBC_LIMITS_H

/* ============================================================
 * KONSTANTA UKURAN BIT
 * ============================================================
 * Jumlah bit dalam satu byte. Standar C menjamin minimal 8.
 */
#define CHAR_BIT    8

/* ============================================================
 * BATAS UNTUK CHAR
 * ============================================================
 * Nilai minimum dan maksimum untuk tipe char. Perhatikan
 * bahwa char bisa signed atau unsigned tergantung implementasi.
 */
#ifdef __CHAR_UNSIGNED__
    /* Char adalah unsigned */
    #define CHAR_MIN    0
    #define CHAR_MAX    UCHAR_MAX
#else
    /* Char adalah signed */
    #define CHAR_MIN    SCHAR_MIN
    #define CHAR_MAX    SCHAR_MAX
#endif

/* Batas untuk signed char */
#define SCHAR_MIN   (-128)
#define SCHAR_MAX   127

/* Batas untuk unsigned char */
#define UCHAR_MAX   255

/* ============================================================
 * BATAS UNTUK SHORT
 * ============================================================
 * Nilai minimum dan maksimum untuk tipe short int.
 */
#define SHRT_MIN    (-32767 - 1)
#define SHRT_MAX    32767

/* Batas untuk unsigned short */
#define USHRT_MAX   65535

/* ============================================================
 * BATAS UNTUK INT
 * ============================================================
 * Nilai minimum dan maksimum untuk tipe int.
 */
#define INT_MIN     (-2147483647 - 1)
#define INT_MAX     2147483647

/* Batas untuk unsigned int */
#define UINT_MAX    4294967295U

/* ============================================================
 * BATAS UNTUK LONG
 * ============================================================
 * Nilai minimum dan maksimum untuk tipe long int.
 * Ukuran long berbeda antara 32-bit dan 64-bit system.
 */
#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
    /* 64-bit system: long adalah 64-bit */
    #define LONG_MIN    (-9223372036854775807L - 1L)
    #define LONG_MAX    9223372036854775807L
    #define ULONG_MAX   18446744073709551615UL
#else
    /* 32-bit system: long adalah 32-bit */
    #define LONG_MIN    (-2147483647L - 1L)
    #define LONG_MAX    2147483647L
    #define ULONG_MAX   4294967295UL
#endif

/* ============================================================
 * BATAS UNTUK LONG LONG (C99, disertakan untuk kompatibilitas)
 * ============================================================
 * Nilai minimum dan maksimum untuk tipe long long int.
 */
#define LLONG_MIN   (-9223372036854775807LL - 1LL)
#define LLONG_MAX   9223372036854775807LL

/* Batas untuk unsigned long long */
#define ULLONG_MAX  18446744073709551615ULL

/* ============================================================
 * BATAS UNTUK MB_LEN_MAX
 * ============================================================
 * Jumlah byte maksimum dalam karakter multibyte.
 * Diset ke 4 untuk mendukung UTF-8 yang menggunakan hingga
 * 4 byte per karakter.
 */
#define MB_LEN_MAX  4

/* ============================================================
 * MAKRO TAMBAHAN UNTUK KOMPATIBILITAS
 * ============================================================
 */

/* Alias untuk long long limits (C99) */
#define LONG_LONG_MIN   LLONG_MIN
#define LONG_LONG_MAX   LLONG_MAX
#define ULONG_LONG_MAX  ULLONG_MAX

/* Batas untuk size_t (implementasi-defined) */
#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
    #define SIZE_MAX    ULLONG_MAX
#else
    #define SIZE_MAX    UINT_MAX
#endif

/* Batas untuk ptrdiff_t */
#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
    #define PTRDIFF_MIN LLONG_MIN
    #define PTRDIFF_MAX LLONG_MAX
#else
    #define PTRDIFF_MIN INT_MIN
    #define PTRDIFF_MAX INT_MAX
#endif

/* Batas untuk int8_t */
#define INT8_MIN    (-128)
#define INT8_MAX    127
#define UINT8_MAX   255U

/* Batas untuk int16_t */
#define INT16_MIN   (-32767 - 1)
#define INT16_MAX   32767
#define UINT16_MAX  65535U

/* Batas untuk int32_t */
#define INT32_MIN   (-2147483647 - 1)
#define INT32_MAX   2147483647
#define UINT32_MAX  4294967295U

/* Batas untuk int64_t */
#define INT64_MIN   (-9223372036854775807LL - 1LL)
#define INT64_MAX   9223372036854775807LL
#define UINT64_MAX  18446744073709551615ULL

/* ============================================================
 * MAKRO UNTUK LEBAH BIT INTEGER (C23, opsional)
 * ============================================================
 */
#define CHAR_WIDTH    CHAR_BIT
#define SCHAR_WIDTH   CHAR_BIT
#define UCHAR_WIDTH   CHAR_BIT
#define SHRT_WIDTH    16
#define USHRT_WIDTH   16
#define INT_WIDTH     32
#define UINT_WIDTH    32

#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
    #define LONG_WIDTH   64
    #define ULONG_WIDTH  64
#else
    #define LONG_WIDTH   32
    #define ULONG_WIDTH  32
#endif

#define LLONG_WIDTH   64
#define ULLONG_WIDTH  64

#endif /* LIBC_LIMITS_H */
