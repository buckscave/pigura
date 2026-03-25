/*
 * arch/x86_64/syscall.S - Syscall untuk x86_64 menggunakan syscall instruction
 *
 * Bagian dari Pigura OS C Library
 * Lisensi: MIT
 *
 * KONVENSI SYSCALL x86_64:
 * ------------------------
 * - Nomor syscall diletakkan di register RAX
 * - Argumen diletakkan di: RDI, RSI, RDX, R10, R8, R9 (urutan)
 * - Hasil syscall dikembalikan di RAX
 * - Error ditandai dengan nilai dalam rentang -4095 hingga -1 di RAX
 * - Instruksi 'syscall' mengubah RCX (return address) dan R11 (rflags)
 * - RCX dan R11 tidak perlu diawetkan
 *
 * Konvensi pemanggilan System V AMD64 ABI:
 * - Argumen: RDI, RSI, RDX, RCX, R8, R9, kemudian stack
 * - Return value: RAX (dan RDX untuk 128-bit)
 * - Caller-saved: RAX, RCX, RDX, RSI, RDI, R8-R11
 * - Callee-saved: RBX, RBP, R12-R15
 */

    .text
    .align 16

/* ========================================================================
 * DEKLARASI EXTERN - errno
 * ======================================================================== */

    .extern errno

/* ========================================================================
 * KONSTANTA
 * ======================================================================== */

#define SYSCALL_ERROR_RANGE 4095

/* ========================================================================
 * MAKRO HELPER
 * ======================================================================== */

/*
 * SYSCALL_ERROR - Periksa error dan atur errno jika perlu
 *
 * Pada x86_64, error ditandai dengan nilai dalam rentang -4095 hingga -1.
 * Nilai negatif tersebut adalah -errno.
 */
.macro SYSCALL_ERROR
    /* Periksa apakah RAX dalam rentang error */
    cmp     $-4095, %rax
    jg      1f                      /* Jika lebih besar dari -4095, tidak error */
    
    /* Ada error: nilai di RAX adalah -errno */
    neg     %eax                    /* Negasikan untuk mendapatkan errno (hanya 32-bit) */
    mov     %eax, errno             /* Simpan ke errno global */
    mov     $-1, %rax               /* Kembalikan -1 sebagai indikator error */
1:
.endm

/*
 * SYSCALL_ERROR_64 - Versi 64-bit dari error check
 */
.macro SYSCALL_ERROR_64
    cmp     $-4095, %rax
    jg      1f
    neg     %rax
    mov     %eax, errno
    mov     $-1, %rax
1:
.endm

/* ========================================================================
 * FUNGSI SYSCALL DASAR
 * ======================================================================== */

/*
 * long syscall0(long num)
 *
 * Syscall tanpa argumen
 *
 * Input:
 *   RDI = num - nomor syscall
 *
 * Output:
 *   RAX = nilai kembalian syscall atau -1 pada error
 */
    .global syscall0
    .type syscall0, @function
syscall0:
    mov     %rdi, %rax              /* Ambil nomor syscall */
    syscall                         /* Panggil kernel */

    SYSCALL_ERROR                   /* Periksa dan handle error */
    ret                             /* Kembali ke pemanggil */
    .size syscall0, . - syscall0

/*
 * long syscall1(long num, long a1)
 *
 * Syscall dengan 1 argumen
 *
 * Input:
 *   RDI = num - nomor syscall
 *   RSI = a1  - argumen pertama
 *
 * Output:
 *   RAX = nilai kembalian syscall atau -1 pada error
 */
    .global syscall1
    .type syscall1, @function
syscall1:
    mov     %rdi, %rax              /* Nomor syscall */
    mov     %rsi, %rdi              /* Argumen 1 (sudah di posisi yang benar) */
    syscall

    SYSCALL_ERROR
    ret
    .size syscall1, . - syscall1

/*
 * long syscall2(long num, long a1, long a2)
 *
 * Syscall dengan 2 argumen
 *
 * Input:
 *   RDI = num - nomor syscall
 *   RSI = a1  - argumen pertama
 *   RDX = a2  - argumen kedua
 *
 * Output:
 *   RAX = nilai kembalian syscall atau -1 pada error
 */
    .global syscall2
    .type syscall2, @function
syscall2:
    mov     %rdi, %rax              /* Nomor syscall */
    /* Argumen 1 (RSI) -> RDI */
    mov     %rsi, %rdi
    /* Argumen 2 (RDX) sudah di tempat yang benar */
    syscall

    SYSCALL_ERROR
    ret
    .size syscall2, . - syscall2

/*
 * long syscall3(long num, long a1, long a2, long a3)
 *
 * Syscall dengan 3 argumen
 *
 * Input:
 *   RDI = num - nomor syscall
 *   RSI = a1  - argumen pertama
 *   RDX = a2  - argumen kedua
 *   RCX = a3  - argumen ketiga
 *
 * Output:
 *   RAX = nilai kembalian syscall atau -1 pada error
 */
    .global syscall3
    .type syscall3, @function
syscall3:
    mov     %rdi, %rax              /* Nomor syscall */
    mov     %rsi, %rdi              /* Argumen 1 */
    /* Argumen 2 (RDX) sudah di tempat */
    mov     %rcx, %rsi              /* Argumen 3 -> RSI untuk syscall? */
    /* TUNGGU: argumen syscall adalah RDI, RSI, RDX, R10, R8, R9 */
    /* Argumen dari C: RDI, RSI, RDX, RCX */
    /* Jadi: num=rdi, a1=rsi, a2=rdx, a3=rcx */
    /* Syscall butuh: rax=num, rdi=a1, rsi=a2, rdx=a3 */
    
    /* Mari kita ulang: */
    /* Input: rdi=num, rsi=a1, rdx=a2, rcx=a3 */
    /* Syscall: rax=num, rdi=a1, rsi=a2, rdx=a3 */
    /* Jadi: */
    mov     %rcx, %rdx              /* a3 -> rdx */
    mov     %rdx, %rsi              /* a2 -> rsi (tapi rdx sudah diubah!) */
    
    /* Lebih baik ulang dari awal: */
    .global syscall3_v2
    .type syscall3_v2, @function
syscall3_v2:
    /* Input: rdi=num, rsi=a1, rdx=a2, rcx=a3 */
    mov     %rdi, %rax              /* num -> rax */
    mov     %rsi, %rdi              /* a1 -> rdi */
    mov     %rdx, %rsi              /* a2 -> rsi */
    mov     %rcx, %rdx              /* a3 -> rdx */
    syscall

    SYSCALL_ERROR
    ret
    .size syscall3_v2, . - syscall3_v2

/* Override syscall3 dengan versi yang benar */
    .size syscall3, . - syscall3

/*
 * Implementasi syscall3 yang benar
 */
    .global syscall3
    .type syscall3, @function
syscall3:
    /* Simpan argumen ke register sementara */
    mov     %rdi, %rax              /* num -> rax */
    mov     %rsi, %rdi              /* a1 -> rdi */
    mov     %rdx, %rsi              /* a2 -> rsi */
    mov     %rcx, %rdx              /* a3 -> rdx */
    syscall

    SYSCALL_ERROR
    ret

/*
 * long syscall4(long num, long a1, long a2, long a3, long a4)
 *
 * Syscall dengan 4 argumen
 *
 * Input:
 *   RDI = num - nomor syscall
 *   RSI = a1  - argumen pertama
 *   RDX = a2  - argumen kedua
 *   RCX = a3  - argumen ketiga
 *   R8  = a4  - argumen keempat
 *
 * Output:
 *   RAX = nilai kembalian syscall atau -1 pada error
 *
 * CATATAN:
 *   Argumen ke-4 untuk syscall menggunakan R10, bukan RCX.
 */
    .global syscall4
    .type syscall4, @function
syscall4:
    /* Input: rdi=num, rsi=a1, rdx=a2, rcx=a3, r8=a4 */
    mov     %rdi, %rax              /* num -> rax */
    mov     %rsi, %rdi              /* a1 -> rdi */
    mov     %rdx, %rsi              /* a2 -> rsi */
    mov     %rcx, %rdx              /* a3 -> rdx */
    mov     %r8, %r10               /* a4 -> r10 */
    syscall

    SYSCALL_ERROR
    ret
    .size syscall4, . - syscall4

/*
 * long syscall5(long num, long a1, long a2, long a3, long a4, long a5)
 *
 * Syscall dengan 5 argumen
 *
 * Input:
 *   RDI = num - nomor syscall
 *   RSI = a1  - argumen pertama
 *   RDX = a2  - argumen kedua
 *   RCX = a3  - argumen ketiga
 *   R8  = a4  - argumen keempat
 *   R9  = a5  - argumen kelima
 *
 * Output:
 *   RAX = nilai kembalian syscall atau -1 pada error
 */
    .global syscall5
    .type syscall5, @function
syscall5:
    /* Input: rdi=num, rsi=a1, rdx=a2, rcx=a3, r8=a4, r9=a5 */
    mov     %rdi, %rax              /* num -> rax */
    mov     %rsi, %rdi              /* a1 -> rdi */
    mov     %rdx, %rsi              /* a2 -> rsi */
    mov     %rcx, %rdx              /* a3 -> rdx */
    mov     %r8, %r10               /* a4 -> r10 */
    /* a5 (r9) sudah di tempat yang benar */
    syscall

    SYSCALL_ERROR
    ret
    .size syscall5, . - syscall5

/*
 * long syscall6(long num, long a1, long a2, long a3, long a4, long a5, long a6)
 *
 * Syscall dengan 6 argumen
 *
 * Input:
 *   RDI = num - nomor syscall
 *   RSI = a1  - argumen pertama
 *   RDX = a2  - argumen kedua
 *   RCX = a3  - argumen ketiga
 *   R8  = a4  - argumen keempat
 *   R9  = a5  - argumen kelima
 *   [RSP+8] = a6 - argumen keenam (di stack)
 *
 * Output:
 *   RAX = nilai kembalian syscall atau -1 pada error
 */
    .global syscall6
    .type syscall6, @function
syscall6:
    /* Input: rdi=num, rsi=a1, rdx=a2, rcx=a3, r8=a4, r9=a5, [rsp+8]=a6 */
    mov     %rdi, %rax              /* num -> rax */
    mov     %rsi, %rdi              /* a1 -> rdi */
    mov     %rdx, %rsi              /* a2 -> rsi */
    mov     %rcx, %rdx              /* a3 -> rdx */
    mov     %r8, %r10               /* a4 -> r10 */
    /* a5 (r9) sudah di tempat */
    mov     8(%rsp), %r8            /* a6 -> r8 */
    syscall

    SYSCALL_ERROR
    ret
    .size syscall6, . - syscall6

/* ========================================================================
 * WRAPPER KHUSUS
 * ======================================================================== */

/*
 * void* __syscall_brk(void* addr)
 *
 * Mengatur atau mendapatkan batas program break.
 *
 * Input:
 *   RDI = addr - alamat baru untuk program break
 *
 * Output:
 *   RAX = alamat program break baru atau -1 pada error
 *
 * CATATAN:
 *   Syscall brk memiliki nomor 12 pada x86_64 Linux.
 */
    .global __syscall_brk
    .type __syscall_brk, @function
__syscall_brk:
    mov     $12, %rax               /* Nomor syscall brk (x86_64) */
    /* RDI sudah berisi addr */
    syscall

    /* brk tidak mengembalikan error dalam bentuk negatif */
    /* Nilai kembalian adalah alamat break baru atau sama dengan input jika gagal */
    ret
    .size __syscall_brk, . - __syscall_brk

/*
 * void* __syscall_mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset)
 *
 * Memetakan file atau perangkat ke memori.
 *
 * Input:
 *   RDI = addr   - alamat yang diinginkan
 *   RSI = length - panjang mapping
 *   RDX = prot   - proteksi memori
 *   RCX = flags  - flags mapping
 *   R8  = fd     - file descriptor
 *   R9  = offset - offset dalam file
 *
 * Output:
 *   RAX = pointer ke mapped memory atau MAP_FAILED (-1) pada error
 *
 * CATATAN:
 *   Syscall mmap memiliki nomor 9 pada x86_64 Linux.
 *   Pada x86_64, argumen offset adalah 64-bit (tidak seperti x86 32-bit).
 */
    .global __syscall_mmap
    .type __syscall_mmap, @function
__syscall_mmap:
    /* Input: rdi=addr, rsi=length, rdx=prot, rcx=flags, r8=fd, r9=offset */
    /* Syscall: rdi=addr, rsi=length, rdx=prot, r10=flags, r8=fd, r9=offset */
    mov     $9, %rax                /* Nomor syscall mmap (x86_64) */
    /* addr, length, prot sudah di tempat */
    mov     %rcx, %r10              /* flags -> r10 */
    /* fd dan offset sudah di tempat */
    syscall

    SYSCALL_ERROR
    ret
    .size __syscall_mmap, . - __syscall_mmap

/*
 * int __syscall_munmap(void* addr, size_t length)
 *
 * Menghapus mapping memori.
 *
 * Input:
 *   RDI = addr   - alamat mapping
 *   RSI = length - panjang mapping
 *
 * Output:
 *   RAX = 0 pada sukses, -1 pada error
 *
 * CATATAN:
 *   Syscall munmap memiliki nomor 11 pada x86_64 Linux.
 */
    .global __syscall_munmap
    .type __syscall_munmap, @function
__syscall_munmap:
    mov     $11, %rax               /* Nomor syscall munmap (x86_64) */
    /* RDI = addr, RSI = length sudah di tempat */
    syscall

    SYSCALL_ERROR
    ret
    .size __syscall_munmap, . - __syscall_munmap

/* ========================================================================
 * FUNGSI HELPER TAMBAHAN
 * ======================================================================== */

/*
 * long __syscall_error(long result)
 *
 * Helper untuk mengkonversi hasil syscall ke nilai C dengan error handling.
 *
 * Input:
 *   RDI = result - hasil syscall mentah
 *
 * Output:
 *   RAX = result jika sukses, -1 jika error (errno sudah diatur)
 */
    .global __syscall_error
    .type __syscall_error, @function
__syscall_error:
    mov     %rdi, %rax
    SYSCALL_ERROR
    ret
    .size __syscall_error, . - __syscall_error

/*
 * long syscall0_naked(long num)
 *
 * Syscall tanpa error handling - mengembalikan nilai mentah dari kernel.
 * Berguna untuk syscall yang tidak mengikuti konvensi error standar.
 *
 * Input:
 *   RDI = num - nomor syscall
 *
 * Output:
 *   RAX = nilai kembalian mentah dari kernel
 */
    .global syscall0_naked
    .type syscall0_naked, @function
syscall0_naked:
    mov     %rdi, %rax
    syscall
    ret
    .size syscall0_naked, . - syscall0_naked

/*
 * long syscall1_naked(long num, long a1)
 */
    .global syscall1_naked
    .type syscall1_naked, @function
syscall1_naked:
    mov     %rdi, %rax
    mov     %rsi, %rdi
    syscall
    ret
    .size syscall1_naked, . - syscall1_naked

/*
 * long syscall2_naked(long num, long a1, long a2)
 */
    .global syscall2_naked
    .type syscall2_naked, @function
syscall2_naked:
    mov     %rdi, %rax
    mov     %rsi, %rdi
    /* rdx sudah di tempat */
    syscall
    ret
    .size syscall2_naked, . - syscall2_naked

/*
 * long syscall3_naked(long num, long a1, long a2, long a3)
 */
    .global syscall3_naked
    .type syscall3_naked, @function
syscall3_naked:
    mov     %rdi, %rax
    mov     %rsi, %rdi
    mov     %rcx, %rdx
    syscall
    ret
    .size syscall3_naked, . - syscall3_naked

/*
 * long syscall4_naked(long num, long a1, long a2, long a3, long a4)
 */
    .global syscall4_naked
    .type syscall4_naked, @function
syscall4_naked:
    mov     %rdi, %rax
    mov     %rsi, %rdi
    mov     %rcx, %rdx
    mov     %r8, %r10
    syscall
    ret
    .size syscall4_naked, . - syscall4_naked

/*
 * long syscall5_naked(long num, long a1, long a2, long a3, long a4, long a5)
 */
    .global syscall5_naked
    .type syscall5_naked, @function
syscall5_naked:
    mov     %rdi, %rax
    mov     %rsi, %rdi
    mov     %rcx, %rdx
    mov     %r8, %r10
    /* r9 sudah di tempat */
    syscall
    ret
    .size syscall5_naked, . - syscall5_naked

/*
 * long syscall6_naked(long num, long a1, long a2, long a3, long a4, long a5, long a6)
 */
    .global syscall6_naked
    .type syscall6_naked, @function
syscall6_naked:
    mov     %rdi, %rax
    mov     %rsi, %rdi
    mov     %rcx, %rdx
    mov     %r8, %r10
    mov     8(%rsp), %r8
    syscall
    ret
    .size syscall6_naked, . - syscall6_naked

/* ========================================================================
 * WRAPPER KHUSUS UNTUK MANAJEMEN MEMORI
 * ======================================================================== */

/*
 * void* __syscall_mmap2(void* addr, size_t length, int prot, int flags, int fd, off_t offset)
 *
 * Variasi mmap untuk kompatibilitas dengan kode lama.
 * Sama seperti mmap biasa pada x86_64.
 */
    .global __syscall_mmap2
    .type __syscall_mmap2, @function
__syscall_mmap2:
    /* Sama seperti __syscall_mmap */
    jmp     __syscall_mmap
    .size __syscall_mmap2, . - __syscall_mmap2

/*
 * int __syscall_mprotect(void* addr, size_t len, int prot)
 *
 * Mengubah proteksi memori.
 *
 * Input:
 *   RDI = addr - alamat awal
 *   RSI = len  - panjang region
 *   RDX = prot - proteksi baru
 *
 * Output:
 *   RAX = 0 pada sukses, -1 pada error
 *
 * CATATAN:
 *   Syscall mprotect memiliki nomor 10 pada x86_64 Linux.
 */
    .global __syscall_mprotect
    .type __syscall_mprotect, @function
__syscall_mprotect:
    mov     $10, %rax               /* Nomor syscall mprotect (x86_64) */
    /* Argumen sudah di tempat */
    syscall

    SYSCALL_ERROR
    ret
    .size __syscall_mprotect, . - __syscall_mprotect

/*
 * void* __syscall_sbrk(long increment)
 *
 * Mengincrement program break.
 *
 * Input:
 *   RDI = increment - nilai increment (bisa negatif)
 *
 * Output:
 *   RAX = alamat break sebelum increment atau -1 pada error
 *
 * CATATAN:
 *   Sbrk bukan syscall, tapi wrapper di atas brk.
 *   Implementasi ini memerlukan state (current break).
 */
    .global __syscall_sbrk
    .type __syscall_sbrk, @function
__syscall_sbrk:
    /* Ini memerlukan implementasi di C karena ada state */
    /* Untuk saat ini, return error */
    mov     $-1, %rax
    mov     $ENOMEM, %edi
    /* Tidak bisa langsung set errno dari assembly tanpa pthread */
    ret
    .size __syscall_sbrk, . - __syscall_sbrk

/* ========================================================================
 * SECTION DATA
 * ======================================================================== */

    .section .note.GNU-stack,"",@progbits

/* End of arch/x86_64/syscall.S */
