/*
 * PIGURA LIBC - STDINT.H
 * =======================
 * Definisi tipe integer dengan ukuran tetap sesuai standar C99.
 * Kompatibel dengan C89 melalui typedef eksplisit.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#ifndef LIBC_STDINT_H
#define LIBC_STDINT_H

/* Deteksi arsitektur untuk ukuran pointer */
#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
    #define _PIGURA_64BIT 1
#elif defined(__i386__) || defined(_M_IX86) || defined(__arm__)
    #define _PIGURA_32BIT 1
#endif

/* ============================================================
 * TIPE INTEGER TEPAT (EXACT-WIDTH INTEGER TYPES)
 * ============================================================ */

/* Tipe integer 8-bit */
typedef signed char int8_t;
typedef unsigned char uint8_t;

/* Tipe integer 16-bit */
typedef signed short int16_t;
typedef unsigned short uint16_t;

/* Tipe integer 32-bit */
typedef signed int int32_t;
typedef unsigned int uint32_t;

/* Tipe integer 64-bit */
typedef signed long long int64_t;
typedef unsigned long long uint64_t;

/* ============================================================
 * TIPE INTEGER MINIMUM (MINIMUM-WIDTH INTEGER TYPES)
 * ============================================================ */

typedef signed char int_least8_t;
typedef unsigned char uint_least8_t;

typedef signed short int_least16_t;
typedef unsigned short uint_least16_t;

typedef signed int int_least32_t;
typedef unsigned int uint_least32_t;

typedef signed long long int_least64_t;
typedef unsigned long long uint_least64_t;

/* ============================================================
 * TIPE INTEGER TERCEPAT (FASTEST MINIMUM-WIDTH INTEGER TYPES)
 * ============================================================ */

typedef signed char int_fast8_t;
typedef unsigned char uint_fast8_t;

typedef signed int int_fast16_t;
typedef unsigned int uint_fast16_t;

typedef signed int int_fast32_t;
typedef unsigned int uint_fast32_t;

typedef signed long long int_fast64_t;
typedef unsigned long long uint_fast64_t;

/* ============================================================
 * TIPE UNTUK POINTER (INTEGER TYPES CAPABLE OF HOLDING OBJECT
 * POINTERS)
 * ============================================================ */

#ifdef _PIGURA_64BIT
    typedef signed long long intptr_t;
    typedef unsigned long long uintptr_t;
#else
    typedef signed int intptr_t;
    typedef unsigned int uintptr_t;
#endif

/* ============================================================
 * TIPE INTEGER TERBESAR (GREATEST-WIDTH INTEGER TYPES)
 * ============================================================ */

typedef signed long long intmax_t;
typedef unsigned long long uintmax_t;

/* ============================================================
 * MAKRO NILAI MINIMUM DAN MAKSIMUM
 * ============================================================ */

/* Nilai untuk int8_t */
#define INT8_MIN    (-128)
#define INT8_MAX    (127)
#define UINT8_MAX   (255U)

/* Nilai untuk int16_t */
#define INT16_MIN   (-32767 - 1)
#define INT16_MAX   (32767)
#define UINT16_MAX  (65535U)

/* Nilai untuk int32_t */
#define INT32_MIN   (-2147483647 - 1)
#define INT32_MAX   (2147483647)
#define UINT32_MAX  (4294967295U)

/* Nilai untuk int64_t */
#define INT64_MIN   (-9223372036854775807LL - 1)
#define INT64_MAX   (9223372036854775807LL)
#define UINT64_MAX  (18446744073709551615ULL)

/* Nilai untuk int_least8_t */
#define INT_LEAST8_MIN   INT8_MIN
#define INT_LEAST8_MAX   INT8_MAX
#define UINT_LEAST8_MAX  UINT8_MAX

/* Nilai untuk int_least16_t */
#define INT_LEAST16_MIN  INT16_MIN
#define INT_LEAST16_MAX  INT16_MAX
#define UINT_LEAST16_MAX UINT16_MAX

/* Nilai untuk int_least32_t */
#define INT_LEAST32_MIN  INT32_MIN
#define INT_LEAST32_MAX  INT32_MAX
#define UINT_LEAST32_MAX UINT32_MAX

/* Nilai untuk int_least64_t */
#define INT_LEAST64_MIN  INT64_MIN
#define INT_LEAST64_MAX  INT64_MAX
#define UINT_LEAST64_MAX UINT64_MAX

/* Nilai untuk int_fast8_t */
#define INT_FAST8_MIN   INT8_MIN
#define INT_FAST8_MAX   INT8_MAX
#define UINT_FAST8_MAX  UINT8_MAX

/* Nilai untuk int_fast16_t */
#define INT_FAST16_MIN  INT32_MIN
#define INT_FAST16_MAX  INT32_MAX
#define UINT_FAST16_MAX UINT32_MAX

/* Nilai untuk int_fast32_t */
#define INT_FAST32_MIN  INT32_MIN
#define INT_FAST32_MAX  INT32_MAX
#define UINT_FAST32_MAX UINT32_MAX

/* Nilai untuk int_fast64_t */
#define INT_FAST64_MIN  INT64_MIN
#define INT_FAST64_MAX  INT64_MAX
#define UINT_FAST64_MAX UINT64_MAX

/* Nilai untuk intptr_t */
#ifdef _PIGURA_64BIT
    #define INTPTR_MIN  INT64_MIN
    #define INTPTR_MAX  INT64_MAX
    #define UINTPTR_MAX UINT64_MAX
#else
    #define INTPTR_MIN  INT32_MIN
    #define INTPTR_MAX  INT32_MAX
    #define UINTPTR_MAX UINT32_MAX
#endif

/* Nilai untuk intmax_t */
#define INTMAX_MIN   INT64_MIN
#define INTMAX_MAX   INT64_MAX
#define UINTMAX_MAX  UINT64_MAX

/* ============================================================
 * MAKRO UNTUK KONSTANTA INTEGER
 * ============================================================ */

/* Makro untuk konstanta 64-bit */
#define INT64_C(x)   (x ## LL)
#define UINT64_C(x)  (x ## ULL)

/* Makro untuk konstanta 32-bit */
#define INT32_C(x)   (x)
#define UINT32_C(x)  (x ## U)

/* Makro untuk konstanta 16-bit */
#define INT16_C(x)   (x)
#define UINT16_C(x)  (x ## U)

/* Makro untuk konstanta 8-bit */
#define INT8_C(x)    (x)
#define UINT8_C(x)   (x ## U)

/* Makro untuk konstanta intmax */
#define INTMAX_C(x)  INT64_C(x)
#define UINTMAX_C(x) UINT64_C(x)

/* ============================================================
 * MAKRO UNTUK LEBAR BIT
 * ============================================================ */

#define INT8_WIDTH   8
#define UINT8_WIDTH  8
#define INT16_WIDTH  16
#define UINT16_WIDTH 16
#define INT32_WIDTH  32
#define UINT32_WIDTH 32
#define INT64_WIDTH  64
#define UINT64_WIDTH 64

/* Makro PTRDIFF_WIDTH - lebar untuk ptrdiff_t */
#ifdef _PIGURA_64BIT
    #define PTRDIFF_WIDTH 64
#else
    #define PTRDIFF_WIDTH 32
#endif

/* Makro SIZE_WIDTH - lebar untuk size_t */
#ifdef _PIGURA_64BIT
    #define SIZE_WIDTH 64
#else
    #define SIZE_WIDTH 32
#endif

/* Makro SIG_ATOMIC_WIDTH */
#define SIG_ATOMIC_WIDTH 32

/* Makro WCHAR_WIDTH (asumsikan 32-bit wchar_t) */
#define WCHAR_WIDTH 32

/* Makro WINT_WIDTH */
#define WINT_WIDTH 32

/* Makro INTPTR_WIDTH */
#ifdef _PIGURA_64BIT
    #define INTPTR_WIDTH 64
    #define UINTPTR_WIDTH 64
#else
    #define INTPTR_WIDTH 32
    #define UINTPTR_WIDTH 32
#endif

/* Makro INTMAX_WIDTH */
#define INTMAX_WIDTH  64
#define UINTMAX_WIDTH 64

/* ============================================================
 * MAKRO NILAI MINIMUM/MAXIMUM TAMBAHAN
 * ============================================================ */

#define PTRDIFF_MIN   INTPTR_MIN
#define PTRDIFF_MAX   INTPTR_MAX
#define SIZE_MAX      UINTPTR_MAX

/* sig_atomic_t biasanya int */
#define SIG_ATOMIC_MIN INT32_MIN
#define SIG_ATOMIC_MAX INT32_MAX

/* wchar_t dan wint_t */
#define WCHAR_MIN INT32_MIN
#define WCHAR_MAX INT32_MAX
#define WINT_MIN  INT32_MIN
#define WINT_MAX  INT32_MAX

#endif /* LIBC_STDINT_H */
