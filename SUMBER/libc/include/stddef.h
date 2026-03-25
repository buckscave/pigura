/*
 * PIGURA LIBC - STDDEF.H
 * =======================
 * Definisi tipe dasar standar C sesuai standar C89/C90.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#ifndef LIBC_STDDEF_H
#define LIBC_STDDEF_H

/* ============================================================
 * NULL POINTER
 * ============================================================
 * Definisi NULL yang kompatibel dengan C89. Menggunakan cast
 * ke void* untuk memastikan tipenya benar.
 */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* ============================================================
 * TIPE PTRDIFF_T
 * ============================================================
 * Tipe untuk menyimpan hasil pengurangan dua pointer.
 * Ukurannya sama dengan ukuran pointer pada arsitektur
 * tersebut.
 */
#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
    typedef long long ptrdiff_t;
#else
    typedef int ptrdiff_t;
#endif

/* ============================================================
 * TIPE SIZE_T
 * ============================================================
 * Tipe unsigned untuk menyimpan ukuran objek dalam memori.
 * Digunakan oleh sizeof operator dan berbagai fungsi string/
 * memori.
 */
#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
    typedef unsigned long long size_t;
#else
    typedef unsigned int size_t;
#endif

/* ============================================================
 * TIPE WCHAR_T
 * ============================================================
 * Tipe untuk karakter lebar (wide character). Ukurannya
 * implementasi-defined, di sini diasumsikan 32-bit untuk
 * mendukung Unicode penuh.
 */
typedef int wchar_t;

/* ============================================================
 * MAKRO OFFSETOF
 * ============================================================
 * Makro untuk mendapatkan offset anggota struct dalam byte.
 * Menggunakan teknik pointer null yang aman untuk C89.
 *
 * Contoh penggunaan:
 *   struct contoh { int a; char b; };
 *   size_t off = offsetof(struct contoh, b);
 */
#define offsetof(type, member) \
    ((size_t) &((type *)0)->member)

/* ============================================================
 * TIPE WINT_T (opsional, untuk <wchar.h>)
 * ============================================================
 * Tipe integer yang dapat menyimpan semua nilai wchar_t
 * plus WEOF.
 */
typedef int wint_t;

/* ============================================================
 * KONSTANTA WEOF
 * ============================================================
 * Wide character EOF untuk fungsi karakter lebar.
 */
#define WEOF ((wint_t)(-1))

/* ============================================================
 * TIPE UNTUK ALIGNMENT (C11, disertakan untuk kompatibilitas)
 * ============================================================
 * Tipe unsigned untuk alignment maksimum objek.
 */
typedef size_t max_align_t;

/* ============================================================
 * TIPE NULLPTR_T (C23, disertakan untuk kompatibilitas maju)
 * ============================================================
 * Didefinisikan sebagai tipe pointer null jika compiler
 * mendukung.
 */
#if defined(__cplusplus) && __cplusplus >= 201103L
    typedef decltype(nullptr) nullptr_t;
#endif

#endif /* LIBC_STDDEF_H */
