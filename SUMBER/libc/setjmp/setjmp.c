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
 *   [8]  signal mask (untuk sigsetjmp)
 */

#ifdef ARCH_X86_64

/*
 * setjmp - Simpan konteks untuk non-local goto
 *
 * Parameter:
 *   env - Buffer untuk menyimpan konteks
 *
 * Return: 0 saat dipanggil, nilai dari longjmp saat return
 */
int setjmp(jmp_buf env) {
    register long rbx __asm__("rbx");
    register long rbp __asm__("rbp");
    register long r12 __asm__("r12");
    register long r13 __asm__("r13");
    register long r14 __asm__("r14");
    register long r15 __asm__("r15");
    register long rsp __asm__("rsp");

    /* Simpan callee-saved registers */
    env->__jb[0] = rbx;
    env->__jb[1] = rbp;
    env->__jb[2] = r12;
    env->__jb[3] = r13;
    env->__jb[4] = r14;
    env->__jb[5] = r15;

    /* Simpan stack pointer */
    env->__jb[6] = rsp;

    /* Simpan return address */
    env->__jb[7] = (long)__builtin_return_address(0);

    /* Return 0 untuk panggilan langsung */
    return 0;
}

/*
 * longjmp - Lompat ke konteks yang disimpan
 *
 * Parameter:
 *   env   - Buffer dengan konteks yang disimpan
 *   val   - Nilai yang akan di-return
 *
 * Return: Tidak pernah return ke caller
 */
void longjmp(jmp_buf env, int val) {
    register long rbx_val;
    register long rbp_val;
    register long r12_val;
    register long r13_val;
    register long r14_val;
    register long r15_val;
    register long rsp_val;
    register long rip_val;

    /* Validasi nilai */
    if (val == 0) {
        val = 1;
    }

    /* Restore callee-saved registers */
    rbx_val = env->__jb[0];
    rbp_val = env->__jb[1];
    r12_val = env->__jb[2];
    r13_val = env->__jb[3];
    r14_val = env->__jb[4];
    r15_val = env->__jb[5];
    rsp_val = env->__jb[6];
    rip_val = env->__jb[7];

    /* Set registers dan lompat menggunakan assembly */
    __asm__ volatile (
        "movq %0, %%rbx\n"
        "movq %1, %%rbp\n"
        "movq %2, %%r12\n"
        "movq %3, %%r13\n"
        "movq %4, %%r14\n"
        "movq %5, %%r15\n"
        "movq %6, %%rsp\n"
        "movq %7, %%rdx\n"      /* Return address */
        "movl %8, %%eax\n"      /* Return value */
        "jmp *%%rdx\n"
        :
        : "r"(rbx_val), "r"(rbp_val), "r"(r12_val),
          "r"(r13_val), "r"(r14_val), "r"(r15_val),
          "r"(rsp_val), "r"(rip_val), "r"(val)
        : "memory"
    );

    /* Tidak akan sampai sini */
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
int setjmp(jmp_buf env) {
    register int ebx __asm__("ebx");
    register int esi __asm__("esi");
    register int edi __asm__("edi");
    register int ebp __asm__("ebp");
    register int esp __asm__("esp");

    /* Simpan callee-saved registers */
    env->__jb[0] = ebx;
    env->__jb[1] = esi;
    env->__jb[2] = edi;
    env->__jb[3] = ebp;

    /* Simpan stack pointer */
    env->__jb[4] = esp;

    /* Simpan return address */
    env->__jb[5] = (int)__builtin_return_address(0);

    return 0;
}

/*
 * longjmp - Lompat ke konteks yang disimpan
 */
void longjmp(jmp_buf env, int val) {
    register int ebx_val;
    register int esi_val;
    register int edi_val;
    register int ebp_val;
    register int esp_val;
    register int eip_val;

    if (val == 0) {
        val = 1;
    }

    ebx_val = env->__jb[0];
    esi_val = env->__jb[1];
    edi_val = env->__jb[2];
    ebp_val = env->__jb[3];
    esp_val = env->__jb[4];
    eip_val = env->__jb[5];

    __asm__ volatile (
        "movl %0, %%ebx\n"
        "movl %1, %%esi\n"
        "movl %2, %%edi\n"
        "movl %3, %%ebp\n"
        "movl %4, %%esp\n"
        "movl %5, %%edx\n"
        "movl %6, %%eax\n"
        "jmp *%%edx\n"
        :
        : "r"(ebx_val), "r"(esi_val), "r"(edi_val),
          "r"(ebp_val), "r"(esp_val), "r"(eip_val), "r"(val)
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
int setjmp(jmp_buf env) {
    register unsigned int r4 __asm__("r4");
    register unsigned int r5 __asm__("r5");
    register unsigned int r6 __asm__("r6");
    register unsigned int r7 __asm__("r7");
    register unsigned int r8 __asm__("r8");
    register unsigned int r9 __asm__("r9");
    register unsigned int r10 __asm__("r10");
    register unsigned int r11 __asm__("r11");
    register unsigned int sp __asm__("sp");
    register unsigned int lr __asm__("lr");

    env->__jb[0] = r4;
    env->__jb[1] = r5;
    env->__jb[2] = r6;
    env->__jb[3] = r7;
    env->__jb[4] = r8;
    env->__jb[5] = r9;
    env->__jb[6] = r10;
    env->__jb[7] = r11;
    env->__jb[8] = sp;
    env->__jb[9] = lr;

    return 0;
}

/*
 * longjmp - Lompat ke konteks yang disimpan
 */
void longjmp(jmp_buf env, int val) {
    register unsigned int r4_val;
    register unsigned int r5_val;
    register unsigned int r6_val;
    register unsigned int r7_val;
    register unsigned int r8_val;
    register unsigned int r9_val;
    register unsigned int r10_val;
    register unsigned int r11_val;
    register unsigned int sp_val;
    register unsigned int lr_val;

    if (val == 0) {
        val = 1;
    }

    r4_val = env->__jb[0];
    r5_val = env->__jb[1];
    r6_val = env->__jb[2];
    r7_val = env->__jb[3];
    r8_val = env->__jb[4];
    r9_val = env->__jb[5];
    r10_val = env->__jb[6];
    r11_val = env->__jb[7];
    sp_val = env->__jb[8];
    lr_val = env->__jb[9];

    __asm__ volatile (
        "mov r4, %0\n"
        "mov r5, %1\n"
        "mov r6, %2\n"
        "mov r7, %3\n"
        "mov r8, %4\n"
        "mov r9, %5\n"
        "mov r10, %6\n"
        "mov r11, %7\n"
        "mov sp, %8\n"
        "mov r0, %9\n"
        "bx %10\n"
        :
        : "r"(r4_val), "r"(r5_val), "r"(r6_val),
          "r"(r7_val), "r"(r8_val), "r"(r9_val),
          "r"(r10_val), "r"(r11_val), "r"(sp_val),
          "r"(val), "r"(lr_val)
        : "memory"
    );

    __builtin_unreachable();
}

#endif /* ARCH_ARM */

/* ============================================================
 * IMPLEMENTASI AARCH64 (ARM64)
 * ============================================================
 * Jumper buffer layout:
 *   [0-17]  x19-x30 (callee-saved)
 *   [18]    sp
 *   [19]    lr (return address)
 */

#ifdef ARCH_AARCH64

/*
 * setjmp - Simpan konteks untuk non-local goto
 */
int setjmp(jmp_buf env) {
    register unsigned long x19 __asm__("x19");
    register unsigned long x20 __asm__("x20");
    register unsigned long x21 __asm__("x21");
    register unsigned long x22 __asm__("x22");
    register unsigned long x23 __asm__("x23");
    register unsigned long x24 __asm__("x24");
    register unsigned long x25 __asm__("x25");
    register unsigned long x26 __asm__("x26");
    register unsigned long x27 __asm__("x27");
    register unsigned long x28 __asm__("x28");
    register unsigned long x29 __asm__("x29");
    register unsigned long x30 __asm__("x30");
    register unsigned long sp __asm__("sp");

    env->__jb[0] = x19;
    env->__jb[1] = x20;
    env->__jb[2] = x21;
    env->__jb[3] = x22;
    env->__jb[4] = x23;
    env->__jb[5] = x24;
    env->__jb[6] = x25;
    env->__jb[7] = x26;
    env->__jb[8] = x27;
    env->__jb[9] = x28;
    env->__jb[10] = x29;
    env->__jb[11] = x30;
    env->__jb[12] = sp;

    return 0;
}

/*
 * longjmp - Lompat ke konteks yang disimpan
 */
void longjmp(jmp_buf env, int val) {
    register unsigned long x19_val;
    register unsigned long x20_val;
    register unsigned long x21_val;
    register unsigned long x22_val;
    register unsigned long x23_val;
    register unsigned long x24_val;
    register unsigned long x25_val;
    register unsigned long x26_val;
    register unsigned long x27_val;
    register unsigned long x28_val;
    register unsigned long x29_val;
    register unsigned long x30_val;
    register unsigned long sp_val;

    if (val == 0) {
        val = 1;
    }

    x19_val = env->__jb[0];
    x20_val = env->__jb[1];
    x21_val = env->__jb[2];
    x22_val = env->__jb[3];
    x23_val = env->__jb[4];
    x24_val = env->__jb[5];
    x25_val = env->__jb[6];
    x26_val = env->__jb[7];
    x27_val = env->__jb[8];
    x28_val = env->__jb[9];
    x29_val = env->__jb[10];
    x30_val = env->__jb[11];
    sp_val = env->__jb[12];

    __asm__ volatile (
        "mov x19, %0\n"
        "mov x20, %1\n"
        "mov x21, %2\n"
        "mov x22, %3\n"
        "mov x23, %4\n"
        "mov x24, %5\n"
        "mov x25, %6\n"
        "mov x26, %7\n"
        "mov x27, %8\n"
        "mov x28, %9\n"
        "mov x29, %10\n"
        "mov x30, %11\n"
        "mov sp, %12\n"
        "mov x0, %13\n"
        "br x30\n"
        :
        : "r"(x19_val), "r"(x20_val), "r"(x21_val),
          "r"(x22_val), "r"(x23_val), "r"(x24_val),
          "r"(x25_val), "r"(x26_val), "r"(x27_val),
          "r"(x28_val), "r"(x29_val), "r"(x30_val),
          "r"(sp_val), "r"((unsigned long)val)
        : "memory"
    );

    __builtin_unreachable();
}

#endif /* ARCH_AARCH64 */

/* ============================================================
 * IMPLEMENTASI GENERIC (FALLBACK)
 * ============================================================
 * Menggunakan standard C dengan ucontext jika tersedia
 */

#ifdef ARCH_GENERIC

/*
 * setjmp - Simpan konteks untuk non-local goto (generic)
 */
int setjmp(jmp_buf env) {
    /* Stack buffer untuk menyimpan informasi minimal */
    volatile int sp_marker;
    
    /* Simpan stack pointer (approximate) */
    env->__jb[0] = (long)&sp_marker;
    
    /* Simpan return address */
    env->__jb[1] = (long)__builtin_return_address(0);
    
    return 0;
}

/*
 * longjmp - Lompat ke konteks yang disimpan (generic)
 *
 * Catatan: Implementasi generic tidak dapat sepenuhnya
 * memulihkan semua register. Digunakan hanya sebagai fallback.
 */
void longjmp(jmp_buf env, int val) {
    if (val == 0) {
        val = 1;
    }
    
    /* Tidak dapat mengimplementasikan longjmp secara portable */
    /* Ini akan menyebabkan undefined behavior */
    while (1) {
        /* Infinite loop - seharusnya tidak sampai sini */
    }
}

#endif /* ARCH_GENERIC */

/* ============================================================
 * IMPLEMENTASI SIGSETJMP/SIGLONGJMP
 * ============================================================
 */

/*
 * sigsetjmp - Simpan konteks dengan signal mask
 *
 * Parameter:
 *   env      - Buffer untuk menyimpan konteks
 *   savemask - Apakah menyimpan signal mask
 *
 * Return: 0 saat dipanggil, nilai dari siglongjmp saat return
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
 *
 * Parameter:
 *   env - Buffer dengan konteks yang disimpan
 *   val - Nilai yang akan di-return
 *
 * Return: Tidak pernah return ke caller
 */
void siglongjmp(sigjmp_buf env, int val) {
    /* Restore signal mask jika sebelumnya disimpan */
    if (env->__savemask != 0) {
        sigprocmask(SIG_SETMASK, &env->__sigmask, NULL);
    }

    /* Panggil longjmp normal */
    longjmp(env->__jmpbuf, val);
}

/* ============================================================
 * FUNGSI UTILITAS
 * ============================================================
 */

/*
 * _setjmp - Versi setjmp tanpa signal mask (BSD)
 *
 * Parameter:
 *   env - Buffer untuk menyimpan konteks
 *
 * Return: 0 saat dipanggil
 */
int _setjmp(jmp_buf env) {
    return setjmp(env);
}

/*
 * _longjmp - Versi longjmp tanpa signal mask (BSD)
 *
 * Parameter:
 *   env - Buffer dengan konteks yang disimpan
 *   val - Nilai yang akan di-return
 */
void _longjmp(jmp_buf env, int val) {
    longjmp(env, val);
}
