/*
 * PIGURA LIBC - SETJMP.H
 * =======================
 * Header untuk non-local jump sesuai standar C89.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#ifndef LIBC_SETJMP_H
#define LIBC_SETJMP_H

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
 * TIPE jmp_buf
 * ============================================================
 * Buffer untuk menyimpan konteks lingkungan stack.
 * Ukuran tergantung arsitektur karena harus menyimpan
 * semua register yang perlu di-restore.
 *
 * Isi jmp_buf (tergantung arsitektur):
 * - x86 (32-bit): 6 register + IP = 24 bytes minimum
 * - x86_64: 8 register + IP = 64 bytes minimum
 * - ARM: 9 register + PC + CPSR = 40 bytes minimum
 * - ARM64: 13 register + SP + PC = 120 bytes minimum
 */

/* Deteksi arsitektur */
#if defined(__x86_64__) || defined(_M_X64)
    /* x86_64: rbx, rbp, r12-r15, rsp, rip */
    typedef struct {
        unsigned long long __rbx;
        unsigned long long __rbp;
        unsigned long long __r12;
        unsigned long long __r13;
        unsigned long long __r14;
        unsigned long long __r15;
        unsigned long long __rsp;
        unsigned long long __rip;
    } __jmp_buf_internal;
    typedef __jmp_buf_internal jmp_buf[1];

#elif defined(__i386__) || defined(_M_IX86)
    /* x86 32-bit: ebx, esi, edi, ebp, esp, eip */
    typedef struct {
        unsigned int __ebx;
        unsigned int __esi;
        unsigned int __edi;
        unsigned int __ebp;
        unsigned int __esp;
        unsigned int __eip;
    } __jmp_buf_internal;
    typedef __jmp_buf_internal jmp_buf[1];

#elif defined(__aarch64__)
    /* ARM64: x19-x28, fp, sp, lr */
    typedef struct {
        unsigned long long __x[10];   /* x19-x28 */
        unsigned long long __fp;      /* x29 */
        unsigned long long __sp;      /* sp */
        unsigned long long __lr;      /* x30 */
    } __jmp_buf_internal;
    typedef __jmp_buf_internal jmp_buf[1];

#elif defined(__arm__)
    /* ARM 32-bit: r4-r11, sp, lr */
    typedef struct {
        unsigned int __r4;
        unsigned int __r5;
        unsigned int __r6;
        unsigned int __r7;
        unsigned int __r8;
        unsigned int __r9;
        unsigned int __r10;
        unsigned int __fp;   /* r11 */
        unsigned int __sp;
        unsigned int __lr;
    } __jmp_buf_internal;
    typedef __jmp_buf_internal jmp_buf[1];

#else
    /* Fallback generik - cukup besar untuk kebanyakan arsitektur */
    typedef unsigned long jmp_buf[32];
#endif

/* ============================================================
 * DEKLARASI FUNGSI
 * ============================================================
 */

/*
 * setjmp - Simpan konteks lingkungan stack
 *
 * Parameter:
 *   env - Buffer untuk menyimpan konteks
 *
 * Return: 0 jika return langsung, nilai non-zero jika
 *         return dari longjmp
 *
 * Catatan: Fungsi ini harus dipanggil sebelum longjmp.
 * Return value non-zero adalah nilai yang dilewatkan
 * ke longjmp.
 */
extern int setjmp(jmp_buf env);

/*
 * longjmp - Non-local jump
 *
 * Parameter:
 *   env  - Buffer konteks yang disimpan oleh setjmp
 *   val  - Nilai yang akan di-return oleh setjmp
 *
 * Catatan: Fungsi ini tidak pernah return. Eksekusi
 * dilanjutkan dari titik setjmp dengan nilai val.
 * Jika val = 0, maka akan di-return 1.
 *
 * Peringatan: Jangan jump keluar dari fungsi yang
 * variabel lokalnya sudah tidak valid!
 */
extern void longjmp(jmp_buf env, int val) __attribute__((noreturn));

/* ============================================================
 * TIPE sigjmp_buf (POSIX)
 * ============================================================
 * Versi jmp_buf yang dapat menyimpan signal mask.
 */

typedef struct {
    jmp_buf __jmpbuf;        /* Konteks standar */
    int __mask_was_saved;    /* Flag apakah signal mask disimpan */
    unsigned long __saved_mask; /* Signal mask yang disimpan */
} sigjmp_buf_internal;

typedef sigjmp_buf_internal sigjmp_buf[1];

/* ============================================================
 * FUNGSI POSIX
 * ============================================================
 */

/*
 * sigsetjmp - setjmp dengan signal mask
 *
 * Parameter:
 *   env      - Buffer untuk menyimpan konteks
 *   savemask - Jika non-zero, simpan juga signal mask
 *
 * Return: Sama seperti setjmp
 */
extern int sigsetjmp(sigjmp_buf env, int savemask);

/*
 * siglongjmp - longjmp dengan signal mask
 *
 * Parameter:
 *   env - Buffer konteks
 *   val - Nilai return
 *
 * Catatan: Signal mask di-restore jika savemask
 * non-zero saat sigsetjmp dipanggil.
 */
extern void siglongjmp(sigjmp_buf env, int val) __attribute__((noreturn));

/* ============================================================
 * MAKRO INTERNAL
 * ============================================================
 */

/* Makro untuk mengecek ukuran jmp_buf */
#define _JMP_BUF_SIZE  sizeof(jmp_buf)

/* Alignment untuk jmp_buf */
#define _JMP_BUF_ALIGN 16

/* ============================================================
 * EXTENSION: jmp_buf dengan floating-point
 * ============================================================
 */

/*
 * Untuk arsitektur dengan floating-point register yang perlu
 * disimpan (opsional, non-standar)
 */
#if defined(__x86_64__)
    typedef struct {
        jmp_buf __base;
        /* FXSAVE area untuk menyimpan SSE/MMX state */
        unsigned char __fpstate[512];
        int __fp_saved;
    } __fp_jmp_buf;
#endif

/* ============================================================
 * EXTENSION: Fungsi utilitas
 * ============================================================
 */

/*
 * _setjmp - Versi setjmp tanpa signal mask (internal)
 *
 * Parameter:
 *   env - Buffer konteks
 *
 * Return: Sama seperti setjmp
 */
extern int _setjmp(jmp_buf env);

/*
 * _longjmp - Versi longjmp tanpa signal mask (internal)
 *
 * Parameter:
 *   env - Buffer konteks
 *   val - Nilai return
 */
extern void _longjmp(jmp_buf env, int val) __attribute__((noreturn));

#endif /* LIBC_SETJMP_H */
