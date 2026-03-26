/*
 * PIGURA LIBC - STDARG.H
 * =======================
 * Definisi untuk fungsi dengan jumlah argumen variabel (variadic
 * functions) sesuai standar C89/C90.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.1 - Perbaikan untuk kompatibilitas GCC/Clang
 */

#ifndef LIBC_STDARG_H
#define LIBC_STDARG_H

/* ============================================================
 * TIPE VA_LIST
 * ============================================================
 * Untuk kompatibilitas dengan GCC/Clang, kita menggunakan
 * __builtin_va_list yang disediakan oleh compiler.
 * Ini memastikan kompatibilitas penuh dengan ABI platform.
 */

#if defined(__GNUC__) || defined(__clang__)
    /* Menggunakan built-in va_list dari compiler */
    typedef __builtin_va_list va_list;
#else
    /* Implementasi manual untuk compiler non-GCC/Clang */
    #if defined(__x86_64__) || defined(_M_X64)
        /* x86_64 System V ABI */
        typedef struct {
            unsigned int gp_offset;
            unsigned int fp_offset;
            void *overflow_arg_area;
            void *reg_save_area;
        } va_list[1];
    #elif defined(__i386__) || defined(_M_IX86)
        /* x86 32-bit: va_list adalah pointer ke stack */
        typedef char *va_list;
    #elif defined(__aarch64__)
        /* ARM64 (AArch64) */
        typedef struct {
            void *__stack;
            void *__gr_top;
            void *__vr_top;
            int __gr_offs;
            int __vr_offs;
        } va_list;
    #elif defined(__arm__)
        /* ARM 32-bit (AAPCS) */
        typedef struct __va_list {
            void *__ap;
        } va_list;
    #else
        /* Fallback generik */
        typedef char *va_list;
    #endif
#endif

/* ============================================================
 * MAKRO VA_START
 * ============================================================
 * Menginisialisasi va_list untuk mengakses argumen variabel.
 *
 * Parameter:
 *   ap   - variabel va_list yang akan diinisialisasi
 *   last - nama parameter terakhir sebelum argumen variabel
 */
#if defined(__GNUC__) || defined(__clang__)
    #define va_start(ap, last) __builtin_va_start(ap, last)
#else
    #if defined(__i386__)
        #define va_start(ap, last) \
            ((ap) = (va_list)&((last)) + sizeof(last))
    #else
        #define va_start(ap, last) ((void)0)
    #endif
#endif

/* ============================================================
 * MAKRO VA_ARG
 * ============================================================
 * Mengambil argumen berikutnya dari va_list.
 */
#if defined(__GNUC__) || defined(__clang__)
    #define va_arg(ap, type) __builtin_va_arg(ap, type)
#else
    #if defined(__i386__)
        #define va_arg(ap, type) \
            (*(type *)((ap) += sizeof(type), (ap) - sizeof(type)))
    #else
        #define va_arg(ap, type) ((type)0)
    #endif
#endif

/* ============================================================
 * MAKRO VA_END
 * ============================================================
 * Membersihkan va_list setelah penggunaan.
 */
#if defined(__GNUC__) || defined(__clang__)
    #define va_end(ap) __builtin_va_end(ap)
#else
    #define va_end(ap) ((void)0)
#endif

/* ============================================================
 * MAKRO VA_COPY (C99)
 * ============================================================
 * Menyalin state va_list dari satu ke yang lain.
 */
#if defined(__GNUC__) || defined(__clang__)
    #define va_copy(dest, src) __builtin_va_copy(dest, src)
#else
    #define va_copy(dest, src) ((dest) = (src))
#endif

#endif /* LIBC_STDARG_H */
