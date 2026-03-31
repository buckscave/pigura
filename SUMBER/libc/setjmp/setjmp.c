/*
 * PIGURA LIBC - SETJMP/SETJMP.C
 * =============================
 * Implementasi fungsi non-local goto.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Implementasi untuk berbagai arsitektur:
 * - x86 (32-bit)
 * - x86_64 (64-bit)
 * - ARM (32-bit)
 * - AArch64 (64-bit)
 */

#include <setjmp.h>
#include <signal.h>
#include <errno.h>

/* ============================================================
 * KONFIGURASI ARSITEKTUR
 * ============================================================
 */

/* Deteksi arsitektur berdasarkan compiler macros */
#if defined(__x86_64__) || defined(_M_X64)
    #define ARCH_X86_64
#elif defined(__i386__) || defined(_M_IX86)
    #define ARCH_X86
#elif defined(__arm__) || defined(__TARGET_ARCH_ARM)
    #define ARCH_ARM
#elif defined(__aarch64__) || defined(__ARM_ARCH_ISA_A64)
    #define ARCH_AARCH64
#else
    #define ARCH_GENERIC
#endif

/* ============================================================
 * IMPLEMENTASI X86_64 (AMD64)
 * ============================================================
 * Jumper buffer layout:
 *   [0]  rbx
 *   [1]  rbp
 *   [2]  r12
 *   [3]  r13
 *   [4]  r14
 *   [5]  r15
 *   [6]  rsp
 *   [7]  rip (return address)
 */

#ifdef ARCH_X86_64

/*
 * setjmp - Simpan konteks untuk non-local goto
 */
int __attribute__((noinline)) setjmp(jmp_buf env) {
    __asm__ volatile (
        "movq %%rbx, %0\n"
        "movq %%rbp, %1\n"
        "movq %%r12, %2\n"
        "movq %%r13, %3\n"
        "movq %%r14, %4\n"
        "movq %%r15, %5\n"
        "movq %%rsp, %6\n"
        : "=m"(env->__jb[0]), "=m"(env->__jb[1]), "=m"(env->__jb[2]),
          "=m"(env->__jb[3]), "=m"(env->__jb[4]), "=m"(env->__jb[5]),
          "=m"(env->__jb[6])
        :
        : "memory"
    );
    
    /* Simpan return address */
    env->__jb[7] = (unsigned long long)__builtin_return_address(0);
    
    return 0;
}

/*
 * longjmp - Lompat ke konteks yang disimpan
 */
void longjmp(jmp_buf env, int val) {
    if (val == 0) {
        val = 1;
    }
    
    /* Load return address */
    unsigned long long rip = env->__jb[7];
    
    __asm__ volatile (
        "movq %0, %%rbx\n"
        "movq %1, %%rbp\n"
        "movq %2, %%r12\n"
        "movq %3, %%r13\n"
        "movq %4, %%r14\n"
        "movq %5, %%r15\n"
        "movq %6, %%rsp\n"
        "movq %7, %%rdx\n"
        "movl %8, %%eax\n"
        "jmp *%%rdx\n"
        :
        : "m"(env->__jb[0]), "m"(env->__jb[1]), "m"(env->__jb[2]),
          "m"(env->__jb[3]), "m"(env->__jb[4]), "m"(env->__jb[5]),
          "m"(env->__jb[6]), "r"(rip), "r"((unsigned int)val)
        : "memory"
    );
    
    __builtin_unreachable();
}

#endif /* ARCH_X86_64 */

/* ============================================================
 * IMPLEMENTASI X86 (32-bit)
 * ============================================================
 * Jumper buffer layout:
 *   [0]  ebx
 *   [1]  esi
 *   [2]  edi
 *   [3]  ebp
 *   [4]  esp
 *   [5]  eip (return address)
 */

#ifdef ARCH_X86

/*
 * setjmp - Simpan konteks untuk non-local goto
 */
int __attribute__((noinline)) setjmp(jmp_buf env) {
    __asm__ volatile (
        "movl %%ebx, %0\n"
        "movl %%esi, %1\n"
        "movl %%edi, %2\n"
        "movl %%ebp, %3\n"
        "movl %%esp, %4\n"
        : "=m"(env->__jb[0]), "=m"(env->__jb[1]), "=m"(env->__jb[2]),
          "=m"(env->__jb[3]), "=m"(env->__jb[4])
        :
        : "memory"
    );
    
    /* Simpan return address */
    env->__jb[5] = (unsigned int)(unsigned long)__builtin_return_address(0);
    
    return 0;
}

/*
 * longjmp - Lompat ke konteks yang disimpan
 *
 * Menggunakan pendekatan berbeda: load semua nilai dari
 * pointer ke struct, lalu lompat. Ini menghindari
 * masalah register pressure pada x86.
 */
void longjmp(jmp_buf env, int val) {
    if (val == 0) {
        val = 1;
    }
    
    /* 
     * Gunakan pendekatan direct memory access melalui pointer.
     * Load env ke register, lalu akses member satu per satu.
     */
    __asm__ volatile (
        /* Load all values from env->__jb array */
        "movl 0(%0), %%ebx\n"     /* env->__jb[0] -> ebx */
        "movl 4(%0), %%esi\n"     /* env->__jb[1] -> esi */
        "movl 8(%0), %%edi\n"     /* env->__jb[2] -> edi */
        "movl 12(%0), %%ebp\n"    /* env->__jb[3] -> ebp */
        "movl 20(%0), %%edx\n"    /* env->__jb[5] -> edx (eip) */
        "movl 16(%0), %%esp\n"    /* env->__jb[4] -> esp (do this last before jump) */
        "movl %1, %%eax\n"        /* val -> eax (return value) */
        "jmp *%%edx\n"            /* jump to saved eip */
        :
        : "r"(env->__jb), "r"(val)
        : "memory"
    );
    
    __builtin_unreachable();
}

#endif /* ARCH_X86 */

/* ============================================================
 * IMPLEMENTASI ARM (32-bit)
 * ============================================================
 * Jumper buffer layout:
 *   [0]  r4
 *   [1]  r5
 *   [2]  r6
 *   [3]  r7
 *   [4]  r8
 *   [5]  r9
 *   [6]  r10
 *   [7]  r11 (fp)
 *   [8]  sp
 *   [9]  lr (return address)
 */

#ifdef ARCH_ARM

/*
 * setjmp - Simpan konteks untuk non-local goto
 */
int __attribute__((noinline)) setjmp(jmp_buf env) {
    __asm__ volatile (
        "stmia %0, {r4-r11, sp, lr}\n"
        :
        : "r"(env->__jb)
        : "memory"
    );
    
    return 0;
}

/*
 * longjmp - Lompat ke konteks yang disimpan
 */
void longjmp(jmp_buf env, int val) {
    if (val == 0) {
        val = 1;
    }
    
    __asm__ volatile (
        "ldmia %0, {r4-r11, sp, lr}\n"
        "mov r0, %1\n"
        "bx lr\n"
        :
        : "r"(env->__jb), "r"(val)
        : "memory", "r0"
    );
    
    __builtin_unreachable();
}

#endif /* ARCH_ARM */

/* ============================================================
 * IMPLEMENTASI AARCH64 (ARM64)
 * ============================================================
 * Jumper buffer layout:
 *   [0-9]   x19-x28
 *   [10]    x29 (fp)
 *   [11]    x30 (lr)
 *   [12]    sp
 *   [13]    reserved
 */

#ifdef ARCH_AARCH64

/*
 * setjmp - Simpan konteks untuk non-local goto
 */
int __attribute__((noinline)) setjmp(jmp_buf env) {
    __asm__ volatile (
        "stp x19, x20, [%0, #0]\n"
        "stp x21, x22, [%0, #16]\n"
        "stp x23, x24, [%0, #32]\n"
        "stp x25, x26, [%0, #48]\n"
        "stp x27, x28, [%0, #64]\n"
        "stp x29, x30, [%0, #80]\n"
        "mov x1, sp\n"
        "str x1, [%0, #96]\n"
        :
        : "r"(env->__jb)
        : "memory", "x1"
    );
    
    return 0;
}

/*
 * longjmp - Lompat ke konteks yang disimpan
 */
void longjmp(jmp_buf env, int val) {
    if (val == 0) {
        val = 1;
    }
    
    __asm__ volatile (
        "ldp x19, x20, [%0, #0]\n"
        "ldp x21, x22, [%0, #16]\n"
        "ldp x23, x24, [%0, #32]\n"
        "ldp x25, x26, [%0, #48]\n"
        "ldp x27, x28, [%0, #64]\n"
        "ldp x29, x30, [%0, #80]\n"
        "ldr x1, [%0, #96]\n"
        "mov sp, x1\n"
        "mov x0, %1\n"
        "br x30\n"
        :
        : "r"(env->__jb), "r"((unsigned long long)val)
        : "memory", "x0", "x1"
    );
    
    __builtin_unreachable();
}

#endif /* ARCH_AARCH64 */

/* ============================================================
 * IMPLEMENTASI GENERIC (FALLBACK)
 * ============================================================
 */

#ifdef ARCH_GENERIC

/*
 * setjmp - Simpan konteks untuk non-local goto (generic)
 *
 * Catatan: Implementasi ini tidak lengkap dan hanya untuk
 * kompilasi fallback. Implementasi sebenarnya memerlukan
 * kode assembly spesifik arsitektur.
 */
int __attribute__((noinline)) setjmp(jmp_buf env) {
    /* Placeholder - tidak portable */
    (void)env;
    return 0;
}

/*
 * longjmp - Lompat ke konteks yang disimpan (generic)
 */
void longjmp(jmp_buf env, int val) {
    (void)env;
    (void)val;
    /* Tidak dapat diimplementasikan secara portable */
    while (1) {}
}

#endif /* ARCH_GENERIC */

/* ============================================================
 * IMPLEMENTASI SIGSETJMP/SIGLONGJMP
 * ============================================================
 */

/*
 * sigsetjmp - Simpan konteks dengan signal mask
 */
int sigsetjmp(sigjmp_buf env, int savemask) {
    /* Simpan flag savemask */
    env->__savemask = savemask;

    /* Simpan signal mask jika diminta */
    if (savemask != 0) {
        sigprocmask(0, NULL, &env->__sigmask);
    }

    /* Panggil setjmp normal */
    return setjmp(env->__jmpbuf);
}

/*
 * siglongjmp - Lompat ke konteks dengan signal mask
 */
void siglongjmp(sigjmp_buf env, int val) {
    /* Restore signal mask jika sebelumnya disimpan */
    if (env->__savemask != 0) {
        sigprocmask(SIG_SETMASK, &env->__sigmask, NULL);
    }

    /* Panggil longjmp normal */
    longjmp(env->__jmpbuf, val);
}
