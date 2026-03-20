/*
 * PIGURA OS - STDARG.H
 * --------------------
 * Definisi untuk variadic argument handling.
 *
 * Berkas ini mendefinisikan tipe dan makro untuk mengakses
 * argumen fungsi dengan jumlah parameter variabel.
 *
 * Versi: 1.0
 * Tanggal: 2025
 *
 * CATATAN: Ini adalah definisi C90 compliant.
 *          Implementasi spesifik compiler mungkin berbeda.
 */

#ifndef INTI_STDARG_H
#define INTI_STDARG_H

/*
 * Tipe va_list untuk menyimpan informasi argumen variabel.
 * Ukuran dan representasi bergantung pada arsitektur.
 */

/* Untuk x86 dan arsitektur 32-bit lainnya */
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_ARM32)

typedef struct {
    char *arg_ptr;
    int arg_offset;
} va_list;

#define va_start(ap, last) \
    ((ap).arg_ptr = (char *)&(last) + sizeof(last))

#define va_arg(ap, type) \
    (*(type *)((ap).arg_ptr += sizeof(type), \
               (ap).arg_ptr - sizeof(type)))

#define va_end(ap) \
    ((void)0)

/* Untuk x86_64 dan arsitektur 64-bit lainnya */
#elif defined(ARSITEKTUR_X86_64) || defined(ARSITEKTUR_ARM64)

typedef struct {
    char *arg_ptr;
    int gp_offset;
    int fp_offset;
    char *overflow_arg_area;
    char *reg_save_area;
} va_list;

#define va_start(ap, last) \
    __builtin_va_start(&(ap), last)

#define va_arg(ap, type) \
    __builtin_va_arg((ap), type)

#define va_end(ap) \
    __builtin_va_end(&(ap))

#else

/* Fallback menggunakan builtin compiler */
typedef __builtin_va_list va_list;

#define va_start(ap, last) \
    __builtin_va_start((ap), last)

#define va_arg(ap, type) \
    __builtin_va_arg((ap), type)

#define va_end(ap) \
    __builtin_va_end((ap))

#endif

/*
 * va_copy (C99, tapi disertakan untuk kompatibilitas)
 * Menyalin state va_list.
 */
#ifndef va_copy
#define va_copy(dest, src) \
    ((dest) = (src))
#endif

#endif /* INTI_STDARG_H */
