/*
 * arch/x86_64/syscall.S - Syscall untuk x86_64 menggunakan syscall instruction
 *
 * Bagian dari Pigura OS C Library
 * Lisensi: MIT
 *
 * KONVENSI SYSCALL x86_64 (System V AMD64 ABI):
 * ---------------------------------------------
 * - Nomor syscall diletakkan di register RAX
 * - Argumen diletakkan di: RDI, RSI, RDX, R10, R8, R9 (urutan)
 * - Hasil syscall dikembalikan di RAX
 * - Error ditandai dengan nilai negatif di RAX (-errno)
 * - rcx dan r11 dimodifikasi oleh syscall instruction
 *
 * KONVENSI PEMANGGILAN C (System V AMD64 ABI):
 * - Argumen: RDI, RSI, RDX, RCX, R8, R9, kemudian stack
 * - Return value: RAX
 *
 * PERHATIAN: Ada perbedaan urutan argumen ke-4!
 * - C calling convention: arg4 di RCX
 * - Syscall convention: arg4 di R10
 */

    .text
    .align 16

/* ========================================================================
 * DEKLARASI EXTERN - errno
 * ======================================================================== */

    .extern errno

/* ========================================================================
 * MAKRO HELPER
 * ======================================================================== */

/*
 * SYSCALL_ERROR - Periksa error dan atur errno jika perlu
 */
.macro SYSCALL_ERROR
    test    %rax, %rax          /* Periksa apakah RAX negatif */
    jns     1f                  /* Jika positif atau nol, tidak ada error */
    neg     %rax                /* Negasikan untuk mendapatkan error code */
    mov     %eax, errno         /* Simpan ke errno global */
    mov     $-1, %rax           /* Kembalikan -1 sebagai indikator error */
1:
.endm

/* ========================================================================
 * FUNGSI SYSCALL DASAR
 * ======================================================================== */

/*
 * long syscall0(long num)
 *
 * Input: RDI = num
 */
    .global syscall0
    .type syscall0, @function
syscall0:
    mov     %rdi, %rax          /* Nomor syscall */
    syscall
    SYSCALL_ERROR
    ret
    .size syscall0, . - syscall0

/*
 * long syscall1(long num, long a1)
 *
 * Input: RDI = num, RSI = a1
 * Syscall: RAX = num, RDI = a1
 */
    .global syscall1
    .type syscall1, @function
syscall1:
    mov     %rdi, %rax          /* Nomor syscall */
    mov     %rsi, %rdi          /* Argumen 1 */
    syscall
    SYSCALL_ERROR
    ret
    .size syscall1, . - syscall1

/*
 * long syscall2(long num, long a1, long a2)
 *
 * Input: RDI = num, RSI = a1, RDX = a2
 * Syscall: RAX = num, RDI = a1, RSI = a2
 */
    .global syscall2
    .type syscall2, @function
syscall2:
    mov     %rdi, %rax          /* Nomor syscall */
    mov     %rsi, %rdi          /* Argumen 1 */
    /* RDX sudah berisi a2 */
    syscall
    SYSCALL_ERROR
    ret
    .size syscall2, . - syscall2

/*
 * long syscall3(long num, long a1, long a2, long a3)
 *
 * Input: RDI = num, RSI = a1, RDX = a2, RCX = a3
 * Syscall: RAX = num, RDI = a1, RSI = a2, RDX = a3
 */
    .global syscall3
    .type syscall3, @function
syscall3:
    mov     %rdi, %rax          /* num -> rax */
    mov     %rsi, %rdi          /* a1 -> rdi */
    mov     %rdx, %rsi          /* a2 -> rsi */
    mov     %rcx, %rdx          /* a3 -> rdx */
    syscall
    SYSCALL_ERROR
    ret
    .size syscall3, . - syscall3

/*
 * long syscall4(long num, long a1, long a2, long a3, long a4)
 *
 * Input: RDI = num, RSI = a1, RDX = a2, RCX = a3, R8 = a4
 * Syscall: RAX = num, RDI = a1, RSI = a2, RDX = a3, R10 = a4
 */
    .global syscall4
    .type syscall4, @function
syscall4:
    mov     %rdi, %rax          /* num -> rax */
    mov     %rsi, %rdi          /* a1 -> rdi */
    mov     %rdx, %rsi          /* a2 -> rsi */
    mov     %rcx, %rdx          /* a3 -> rdx */
    mov     %r8, %r10           /* a4 -> r10 */
    syscall
    SYSCALL_ERROR
    ret
    .size syscall4, . - syscall4

/*
 * long syscall5(long num, long a1, long a2, long a3, long a4, long a5)
 *
 * Input: RDI = num, RSI = a1, RDX = a2, RCX = a3, R8 = a4, R9 = a5
 * Syscall: RAX = num, RDI = a1, RSI = a2, RDX = a3, R10 = a4, R8 = a5
 */
    .global syscall5
    .type syscall5, @function
syscall5:
    mov     %rdi, %rax          /* num -> rax */
    mov     %rsi, %rdi          /* a1 -> rdi */
    mov     %rdx, %rsi          /* a2 -> rsi */
    mov     %rcx, %rdx          /* a3 -> rdx */
    mov     %r8, %r10           /* a4 -> r10 */
    mov     %r9, %r8            /* a5 -> r8 */
    syscall
    SYSCALL_ERROR
    ret
    .size syscall5, . - syscall5

/*
 * long syscall6(long num, long a1, long a2, long a3, long a4, long a5, long a6)
 *
 * Input: RDI = num, RSI = a1, RDX = a2, RCX = a3, R8 = a4, R9 = a5
 *         [RSP+8] = a6 (di stack setelah return address)
 * Syscall: RAX = num, RDI = a1, RSI = a2, RDX = a3, R10 = a4, R8 = a5, R9 = a6
 */
    .global syscall6
    .type syscall6, @function
syscall6:
    mov     %rdi, %rax          /* num -> rax */
    mov     %rsi, %rdi          /* a1 -> rdi */
    mov     %rdx, %rsi          /* a2 -> rsi */
    mov     %rcx, %rdx          /* a3 -> rdx */
    mov     %r8, %r10           /* a4 -> r10 */
    mov     %r9, %r8            /* a5 -> r8 */
    mov     8(%rsp), %r9        /* a6 dari stack */
    syscall
    SYSCALL_ERROR
    ret
    .size syscall6, . - syscall6

/* ========================================================================
 * WRAPPER KHUSUS
 * ======================================================================== */

/*
 * void* __syscall_brk(void* addr)
 */
    .global __syscall_brk
    .type __syscall_brk, @function
__syscall_brk:
    mov     $12, %rax           /* __NR_brk pada x86_64 */
    /* addr sudah di RDI */
    syscall
    /* brk tidak mengembalikan error dalam bentuk negatif */
    ret
    .size __syscall_brk, . - __syscall_brk

/*
 * void* __syscall_mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset)
 *
 * Input: RDI = addr, RSI = length, RDX = prot, RCX = flags, R8 = fd, R9 = offset
 * Syscall: RAX = 9, RDI = addr, RSI = length, RDX = prot, R10 = flags, R8 = fd, R9 = offset
 */
    .global __syscall_mmap
    .type __syscall_mmap, @function
__syscall_mmap:
    mov     $9, %rax            /* __NR_mmap pada x86_64 */
    mov     %rcx, %r10          /* flags -> r10 (perbedaan convention) */
    /* RDI, RSI, RDX, R8, R9 sudah di tempat */
    syscall
    SYSCALL_ERROR
    ret
    .size __syscall_mmap, . - __syscall_mmap

/*
 * int __syscall_munmap(void* addr, size_t length)
 */
    .global __syscall_munmap
    .type __syscall_munmap, @function
__syscall_munmap:
    mov     $11, %rax           /* __NR_munmap pada x86_64 */
    /* Argumen sudah di tempat: rdi=addr, rsi=length */
    syscall
    SYSCALL_ERROR
    ret
    .size __syscall_munmap, . - __syscall_munmap

    .section .note.GNU-stack,"",@progbits
