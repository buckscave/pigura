/*
 * PIGURA LIBC - ASSERT.H
 * =======================
 * Makro untuk verifikasi asersi (assertion) saat runtime.
 * Sesuai standar C89/C90.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#ifndef LIBC_ASSERT_H
#define LIBC_ASSERT_H

/* ============================================================
 * DEPENDENSI
 * ============================================================
 * assert memerlukan stdio.h untuk fprintf dan stderr
 */
#include <stdio.h>

/* ============================================================
 * IMPLEMENTASI ASSERT
 * ============================================================
 * Makro assert mengevaluasi ekspresi dan jika hasilnya 0
 * (false), maka program dihentikan dengan pesan error.
 *
 * Jika NDEBUG didefinisikan, assert tidak melakukan apa-apa.
 * Ini memungkinkan penonaktifan assertion di build release.
 *
 * Pesan error format:
 *   file:line: assertion "expression" failed
 */

#ifdef NDEBUG

/* Mode release: assert dinonaktifkan */
#define assert(condition) ((void)0)

#else

/* Mode debug: assert aktif */

/* Deklarasi fungsi internal untuk menangani assertion gagal */
extern void __assert_fail(const char *expr, const char *file,
                          int line, const char *func);

/* Makro assert yang memeriksa kondisi */
#define assert(condition) \
    do { \
        if (!(condition)) { \
            __assert_fail(#condition, __FILE__, __LINE__, \
                          __func__); \
        } \
    } while (0)

#endif /* NDEBUG */

/* ============================================================
 * MAKRO STATIC_ASSERT (C11, disertakan untuk kompatibilitas)
 * ============================================================
 * Static assertion dilakukan pada compile-time.
 * Jika kondisi false, kompilasi gagal dengan pesan error.
 */

#if defined(__cplusplus) && __cplusplus >= 201103L
    /* C++11 memiliki static_assert built-in */
    #define static_assert(cond, msg) static_assert(cond, msg)

#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    /* C11 memiliki _Static_assert built-in */
    #define static_assert(cond, msg) _Static_assert(cond, msg)

#elif defined(__GNUC__) || defined(__clang__)
    /* GCC/Clang: gunakan _Static_assert atau typedef trick */
    #define static_assert(cond, msg) \
        _Static_assert(cond, msg)

#else
    /* C89: gunakan teknik typedef dengan array */
    #define static_assert(cond, msg) \
        typedef char static_assert_##msg[(cond) ? 1 : -1]

#endif

/* ============================================================
 * MAKRO ASSERT TAMBAHAN (NON-STANDAR)
 * ============================================================
 * Makro tambahan yang berguna untuk debugging.
 */

#ifdef NDEBUG

#define debug_assert(condition) ((void)0)
#define assert_reachable() ((void)0)
#define assert_unreachable() ((void)0)

#else

/* Debug-only assertion */
#define debug_assert(condition) assert(condition)

/* Menandai kode yang seharusnya dapat dijangkau */
#define assert_reachable() \
    do { \
        /* No-op, hanya untuk dokumentasi */ \
    } while (0)

/* Menandai kode yang seharusnya tidak dapat dijangkau */
#define assert_unreachable() \
    do { \
        __assert_fail("unreachable code reached", __FILE__, \
                      __LINE__, __func__); \
    } while (0)

#endif /* NDEBUG */

/* ============================================================
 * MAKRO UNTUK VERIFIKASI POINTER
 * ============================================================
 */

#ifndef NDEBUG

/* Assert pointer tidak NULL */
#define assert_not_null(ptr) \
    assert((ptr) != NULL)

/* Assert pointer NULL */
#define assert_null(ptr) \
    assert((ptr) == NULL)

/* Assert pointer aligned */
#define assert_aligned(ptr, align) \
    assert(((uintptr_t)(ptr) & ((align) - 1)) == 0)

#else

#define assert_not_null(ptr)    ((void)0)
#define assert_null(ptr)        ((void)0)
#define assert_aligned(ptr, align) ((void)0)

#endif

/* ============================================================
 * MAKRO UNTUK VERIFIKASI INDEX DAN RANGE
 * ============================================================
 */

#ifndef NDEBUG

/* Assert index dalam batas array */
#define assert_in_bounds(idx, size) \
    assert((idx) >= 0 && (idx) < (size))

/* Assert range valid (min <= val <= max) */
#define assert_in_range(val, min, max) \
    assert((val) >= (min) && (val) <= (max))

#else

#define assert_in_bounds(idx, size) ((void)0)
#define assert_in_range(val, min, max) ((void)0)

#endif

/* ============================================================
 * MAKRO UNTUK VERIFIKASI STRING
 * ============================================================
 */

#ifndef NDEBUG

/* Assert string tidak NULL dan tidak kosong */
#define assert_valid_string(str) \
    assert((str) != NULL && (str)[0] != '\0')

#else

#define assert_valid_string(str) ((void)0)

#endif

/* ============================================================
 * MAKRO UNTUK VERIFIKASI MEMORI
 * ============================================================
 */

#ifndef NDEBUG

/* Assert alokasi memori berhasil */
#define assert_alloc(ptr) \
    do { \
        if ((ptr) == NULL) { \
            __assert_fail("allocation failed", __FILE__, \
                          __LINE__, __func__); \
        } \
    } while (0)

#else

#define assert_alloc(ptr) ((void)0)

#endif

#endif /* LIBC_ASSERT_H */
