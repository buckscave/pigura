/*
 * PIGURA LIBC - STDARG.H
 * =======================
 * Definisi untuk fungsi dengan jumlah argumen variabel (variadic
 * functions) sesuai standar C89/C90.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#ifndef LIBC_STDARG_H
#define LIBC_STDARG_H

/* ============================================================
 * TIPE VA_LIST
 * ============================================================
 * Tipe untuk menyimpan informasi tentang argumen variabel.
 * Implementasi spesifik tergantung pada ABI (Application
 * Binary Interface) yang digunakan.
 *
 * Untuk x86 (cdecl): argumen di-stack, tumbuh ke bawah
 * Untuk x86_64 (System V AMD64): argumen di register + stack
 * Untuk ARM (AAPCS): argumen di register (r0-r3) + stack
 */

/* Deteksi arsitektur untuk implementasi va_list yang tepat */
#if defined(__x86_64__) || defined(_M_X64)
    /* x86_64 System V ABI: va_list adalah array 1 elemen */
    typedef struct {
        unsigned int gp_offset;      /* Offset ke register GP */
        unsigned int fp_offset;      /* Offset ke register FP */
        void *overflow_arg_area;     /* Area argumen overflow */
        void *reg_save_area;         /* Area penyimpanan register */
    } __va_list_element[1];

    typedef __va_list_element va_list;

#elif defined(__i386__) || defined(_M_IX86)
    /* x86 32-bit: va_list adalah pointer ke stack */
    typedef char *va_list;

#elif defined(__aarch64__)
    /* ARM64 (AArch64): va_list adalah struktur */
    typedef struct {
        void *__stack;               /* Pointer ke stack args */
        void *__gr_top;              /* Pointer ke GP reg save */
        void *__vr_top;              /* Pointer ke FP reg save */
        int __gr_offs;               /* Offset GP register */
        int __vr_offs;               /* Offset FP register */
    } va_list;

#elif defined(__arm__)
    /* ARM 32-bit (AAPCS): va_list adalah pointer */
    typedef struct __va_list {
        void *__ap;                  /* Pointer ke argumen */
    } va_list;

#else
    /* Fallback generik */
    typedef char *va_list;
#endif

/* ============================================================
 * MAKRO VA_START
 * ============================================================
 * Menginisialisasi va_list untuk mengakses argumen variabel.
 *
 * Parameter:
 *   ap   - variabel va_list yang akan diinisialisasi
 *   last - nama parameter terakhir sebelum argumen variabel
 *
 * Contoh:
 *   void fungsi(int count, ...) {
 *       va_list ap;
 *       va_start(ap, count);
 *       ...
 *       va_end(ap);
 *   }
 */
#if defined(__GNUC__) || defined(__clang__)
    /* Menggunakan built-in compiler untuk portabilitas */
    #define va_start(ap, last) __builtin_va_start(ap, last)
#else
    /* Implementasi manual untuk x86 32-bit */
    #if defined(__i386__)
        #define va_start(ap, last) \
            ((ap) = (va_list)&((last)) + sizeof(last))
    #else
        /* Fallback - butuh implementasi spesifik platform */
        #define va_start(ap, last) ((void)0)
    #endif
#endif

/* ============================================================
 * MAKRO VA_ARG
 * ============================================================
 * Mengambil argumen berikutnya dari va_list.
 *
 * Parameter:
 *   ap  - variabel va_list
 *   type - tipe argumen yang akan diambil
 *
 * Return:
 *   Nilai argumen berikutnya dengan tipe yang ditentukan
 *
 * Catatan: Tipe yang dilewatkan harus kompatibel dengan
 * argumen yang sebenarnya, atau hasilnya undefined.
 */
#if defined(__GNUC__) || defined(__clang__)
    #define va_arg(ap, type) __builtin_va_arg(ap, type)
#else
    /* Implementasi manual untuk x86 32-bit */
    #if defined(__i386__)
        #define va_arg(ap, type) \
            (*(type *)((ap) += sizeof(type), (ap) - sizeof(type)))
    #else
        /* Fallback */
        #define va_arg(ap, type) ((type)0)
    #endif
#endif

/* ============================================================
 * MAKRO VA_END
 * ============================================================
 * Membersihkan va_list setelah penggunaan. Wajib dipanggil
 * setelah va_start sebelum fungsi return.
 *
 * Parameter:
 *   ap - variabel va_list yang akan dibersihkan
 */
#if defined(__GNUC__) || defined(__clang__)
    #define va_end(ap) __builtin_va_end(ap)
#else
    #define va_end(ap) ((void)0)
#endif

/* ============================================================
 * MAKRO VA_COPY (C99, disertakan untuk kompatibilitas)
 * ============================================================
 * Menyalin state va_list dari satu ke yang lain.
 *
 * Parameter:
 *   dest - va_list tujuan
 *   src  - va_list sumber
 */
#if defined(__GNUC__) || defined(__clang__)
    #define va_copy(dest, src) __builtin_va_copy(dest, src)
#elif defined(__i386__)
    #define va_copy(dest, src) ((dest) = (src))
#elif defined(__x86_64__)
    #define va_copy(dest, src) (*(dest) = *(src))
#else
    #define va_copy(dest, src) ((dest) = (src))
#endif

#endif /* LIBC_STDARG_H */
