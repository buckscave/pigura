/*
 * arch/x86/syscall.S - Syscall untuk x86 32-bit menggunakan int 0x80
 *
 * Bagian dari Pigura OS C Library
 * Lisensi: MIT
 *
 * KONVENSI SYSCALL x86 32-bit:
 * ----------------------------
 * - Nomor syscall diletakkan di register EAX
 * - Argumen diletakkan di: EBX, ECX, EDX, ESI, EDI, EBP (urutan)
 * - Hasil syscall dikembalikan di EAX
 * - Error ditandai dengan nilai negatif di EAX
 * - Error code adalah -errno (negasi dari errno)
 *
 * Register yang diawetkan:
 * - EBX, ESI, EDI, EBP, ESP (callee-saved)
 * - EAX, ECX, EDX (caller-saved, dapat dimodifikasi)
 */

    .text
    .align 4

/* ========================================================================
 * DEKLARASI EXTERN - errno
 * ======================================================================== */

    .extern errno

/* ========================================================================
 * MAKRO HELPER
 * ======================================================================== */

/*
 * SYSCALL_ERROR - Periksa error dan atur errno jika perlu
 *
 * Pada x86 32-bit, error ditandai dengan nilai negatif di EAX.
 * Nilai absolut dari nilai negatif tersebut adalah errno.
 */
.macro SYSCALL_ERROR
    test    %eax, %eax          /* Periksa apakah EAX negatif */
    jns     1f                  /* Jika positif atau nol, tidak ada error */
    neg     %eax                /* Negasikan untuk mendapatkan error code */
    mov     %eax, errno         /* Simpan ke errno global */
    mov     $-1, %eax           /* Kembalikan -1 sebagai indikator error */
1:
.endm

/*
 * SAVE_ARGS - Simpan register callee-saved yang akan digunakan
 */
.macro SAVE_ARGS
    push    %ebx
    push    %esi
    push    %edi
    push    %ebp
.endm

/*
 * RESTORE_ARGS - Kembalikan register callee-saved
 */
.macro RESTORE_ARGS
    pop     %ebp
    pop     %edi
    pop     %esi
    pop     %ebx
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
 *   [ESP+4] = num - nomor syscall
 *
 * Output:
 *   EAX = nilai kembalian syscall atau -1 pada error
 */
    .global syscall0
    .type syscall0, @function
syscall0:
    push    %ebp                    /* Simpan frame pointer */
    mov     %esp, %ebp              /* Setup frame pointer baru */

    mov     8(%ebp), %eax           /* Ambil nomor syscall */
    int     $0x80                   /* Panggil kernel */

    SYSCALL_ERROR                   /* Periksa dan handle error */

    pop     %ebp                    /* Kembalikan frame pointer */
    ret                             /* Kembali ke pemanggil */
    .size syscall0, . - syscall0

/*
 * long syscall1(long num, long a1)
 *
 * Syscall dengan 1 argumen
 *
 * Input:
 *   [ESP+4]  = num - nomor syscall
 *   [ESP+8]  = a1  - argumen pertama
 *
 * Output:
 *   EAX = nilai kembalian syscall atau -1 pada error
 */
    .global syscall1
    .type syscall1, @function
syscall1:
    push    %ebp
    mov     %esp, %ebp

    mov     8(%ebp), %eax           /* Nomor syscall */
    mov     12(%ebp), %ebx          /* Argumen 1 */

    int     $0x80                   /* Panggil kernel */

    SYSCALL_ERROR

    pop     %ebp
    ret
    .size syscall1, . - syscall1

/*
 * long syscall2(long num, long a1, long a2)
 *
 * Syscall dengan 2 argumen
 *
 * Input:
 *   [ESP+4]  = num - nomor syscall
 *   [ESP+8]  = a1  - argumen pertama
 *   [ESP+12] = a2  - argumen kedua
 *
 * Output:
 *   EAX = nilai kembalian syscall atau -1 pada error
 */
    .global syscall2
    .type syscall2, @function
syscall2:
    push    %ebp
    mov     %esp, %ebp

    mov     8(%ebp), %eax           /* Nomor syscall */
    mov     12(%ebp), %ebx          /* Argumen 1 */
    mov     16(%ebp), %ecx          /* Argumen 2 */

    int     $0x80

    SYSCALL_ERROR

    pop     %ebp
    ret
    .size syscall2, . - syscall2

/*
 * long syscall3(long num, long a1, long a2, long a3)
 *
 * Syscall dengan 3 argumen
 *
 * Input:
 *   [ESP+4]  = num - nomor syscall
 *   [ESP+8]  = a1  - argumen pertama
 *   [ESP+12] = a2  - argumen kedua
 *   [ESP+16] = a3  - argumen ketiga
 *
 * Output:
 *   EAX = nilai kembalian syscall atau -1 pada error
 */
    .global syscall3
    .type syscall3, @function
syscall3:
    push    %ebp
    mov     %esp, %ebp

    mov     8(%ebp), %eax           /* Nomor syscall */
    mov     12(%ebp), %ebx          /* Argumen 1 */
    mov     16(%ebp), %ecx          /* Argumen 2 */
    mov     20(%ebp), %edx          /* Argumen 3 */

    int     $0x80

    SYSCALL_ERROR

    pop     %ebp
    ret
    .size syscall3, . - syscall3

/*
 * long syscall4(long num, long a1, long a2, long a3, long a4)
 *
 * Syscall dengan 4 argumen
 *
 * Input:
 *   [ESP+4]  = num - nomor syscall
 *   [ESP+8]  = a1  - argumen pertama
 *   [ESP+12] = a2  - argumen kedua
 *   [ESP+16] = a3  - argumen ketiga
 *   [ESP+20] = a4  - argumen keempat
 *
 * Output:
 *   EAX = nilai kembalian syscall atau -1 pada error
 */
    .global syscall4
    .type syscall4, @function
syscall4:
    push    %ebp
    mov     %esp, %ebp

    push    %ebx                    /* Simpan EBX (callee-saved) */
    push    %esi                    /* Simpan ESI (callee-saved) */

    mov     8(%ebp), %eax           /* Nomor syscall */
    mov     12(%ebp), %ebx          /* Argumen 1 */
    mov     16(%ebp), %ecx          /* Argumen 2 */
    mov     20(%ebp), %edx          /* Argumen 3 */
    mov     24(%ebp), %esi          /* Argumen 4 */

    int     $0x80

    SYSCALL_ERROR

    pop     %esi                    /* Kembalikan ESI */
    pop     %ebx                    /* Kembalikan EBX */
    pop     %ebp
    ret
    .size syscall4, . - syscall4

/*
 * long syscall5(long num, long a1, long a2, long a3, long a4, long a5)
 *
 * Syscall dengan 5 argumen
 *
 * Input:
 *   [ESP+4]  = num - nomor syscall
 *   [ESP+8]  = a1  - argumen pertama
 *   [ESP+12] = a2  - argumen kedua
 *   [ESP+16] = a3  - argumen ketiga
 *   [ESP+20] = a4  - argumen keempat
 *   [ESP+24] = a5  - argumen kelima
 *
 * Output:
 *   EAX = nilai kembalian syscall atau -1 pada error
 */
    .global syscall5
    .type syscall5, @function
syscall5:
    push    %ebp
    mov     %esp, %ebp

    push    %ebx
    push    %esi
    push    %edi

    mov     8(%ebp), %eax           /* Nomor syscall */
    mov     12(%ebp), %ebx          /* Argumen 1 */
    mov     16(%ebp), %ecx          /* Argumen 2 */
    mov     20(%ebp), %edx          /* Argumen 3 */
    mov     24(%ebp), %esi          /* Argumen 4 */
    mov     28(%ebp), %edi          /* Argumen 5 */

    int     $0x80

    SYSCALL_ERROR

    pop     %edi
    pop     %esi
    pop     %ebx
    pop     %ebp
    ret
    .size syscall5, . - syscall5

/*
 * long syscall6(long num, long a1, long a2, long a3, long a4, long a5, long a6)
 *
 * Syscall dengan 6 argumen
 *
 * Input:
 *   [ESP+4]  = num - nomor syscall
 *   [ESP+8]  = a1  - argumen pertama
 *   [ESP+12] = a2  - argumen kedua
 *   [ESP+16] = a3  - argumen ketiga
 *   [ESP+20] = a4  - argumen keempat
 *   [ESP+24] = a5  - argumen kelima
 *   [ESP+28] = a6  - argumen keenam
 *
 * Output:
 *   EAX = nilai kembalian syscall atau -1 pada error
 *
 * CATATAN:
 *   Karena x86 hanya memiliki 6 register untuk argumen,
 *   kita perlu menggunakan EBP untuk argumen ke-6.
 *   Frame pointer disimpan di stack dan dipulihkan setelah syscall.
 */
    .global syscall6
    .type syscall6, @function
syscall6:
    /* Simpan frame pointer lama dan setup frame baru */
    push    %ebp
    mov     %esp, %ebp

    /* Simpan callee-saved registers yang akan digunakan */
    push    %ebx
    push    %esi
    push    %edi

    /* Load argumen dari stack:
     * [ebp+8]  = num
     * [ebp+12] = a1
     * [ebp+16] = a2
     * [ebp+20] = a3
     * [ebp+24] = a4
     * [ebp+28] = a5
     * [ebp+32] = a6
     */
    mov     8(%ebp), %eax           /* Nomor syscall */
    mov     12(%ebp), %ebx          /* Argumen 1 */
    mov     16(%ebp), %ecx          /* Argumen 2 */
    mov     20(%ebp), %edx          /* Argumen 3 */
    mov     24(%ebp), %esi          /* Argumen 4 */
    mov     28(%ebp), %edi          /* Argumen 5 */

    /* Simpan frame pointer saat ini di stack */
    /* Catatan: Ini menambah item di stack, tapi tidak mengubah ebp */
    push    %ebp

    /* Load argumen 6 ke EBP - gunakan offset yang benar */
    mov     32(%ebp), %ebp          /* Argumen 6 */

    /* Sekarang semua register siap:
     * EAX = num, EBX = a1, ECX = a2, EDX = a3
     * ESI = a4, EDI = a5, EBP = a6
     */
    int     $0x80

    /* Simpan hasil syscall */
    push    %eax

    /* Pulihkan frame pointer dari stack */
    pop     %eax                    /* Ambil hasil kembali ke EAX */
    pop     %ebp                    /* Pulihkan frame pointer */

    /* Handle error */
    SYSCALL_ERROR

    /* Pulihkan callee-saved registers */
    pop     %edi
    pop     %esi
    pop     %ebx
    pop     %ebp                    /* Pulihkan frame pointer asli */
    ret
    .size syscall6, . - syscall6

/* ========================================================================
 * IMPLEMENTASI ALTERNATIF syscall6 yang lebih aman
 * ======================================================================== */

    .global syscall6_alt
    .type syscall6_alt, @function
syscall6_alt:
    push    %ebp
    mov     %esp, %ebp

    /* Simpan callee-saved registers */
    push    %ebx
    push    %esi
    push    %edi

    /* Load argumen */
    mov     8(%ebp), %eax           /* Nomor syscall */
    mov     12(%ebp), %ebx          /* Argumen 1 */
    mov     16(%ebp), %ecx          /* Argumen 2 */
    mov     20(%ebp), %edx          /* Argumen 3 */
    mov     24(%ebp), %esi          /* Argumen 4 */
    mov     28(%ebp), %edi          /* Argumen 5 */

    /* Simpan frame pointer saat ini di stack */
    push    %ebp

    /* Load argumen 6 ke EBP - offset yang benar */
    mov     32(%ebp), %ebp          /* Argumen 6 */

    /* Sekarang: EAX=num, EBX=a1, ECX=a2, EDX=a3, ESI=a4, EDI=a5, EBP=a6 */
    int     $0x80

    /* Simpan hasil */
    push    %eax                    /* Simpan hasil di stack */

    /* Restore EBP dari stack */
    pop     %eax                    /* Ambil hasil kembali */
    pop     %ebp                    /* Restore frame pointer asli */

    SYSCALL_ERROR

    pop     %edi
    pop     %esi
    pop     %ebx
    pop     %ebp
    ret
    .size syscall6_alt, . - syscall6_alt

/* ========================================================================
 * WRAPPER KHUSUS
 * ======================================================================== */

/*
 * void* __syscall_brk(void* addr)
 *
 * Mengatur atau mendapatkan batas program break.
 *
 * Input:
 *   [ESP+4] = addr - alamat baru untuk program break
 *
 * Output:
 *   EAX = alamat program break baru atau -1 pada error
 *
 * CATATAN:
 *   Syscall brk memiliki nomor 45 pada x86 32-bit Linux.
 *   Untuk mendapatkan nilai current break, panggil dengan addr = 0.
 */
    .global __syscall_brk
    .type __syscall_brk, @function
__syscall_brk:
    push    %ebp
    mov     %esp, %ebp

    mov     $45, %eax               /* Nomor syscall brk (x86 32-bit) */
    mov     8(%ebp), %ebx           /* Alamat baru */

    int     $0x80

    /* brk tidak mengembalikan error dalam bentuk negatif */
    /* Nilai kembalian adalah alamat break baru */

    pop     %ebp
    ret
    .size __syscall_brk, . - __syscall_brk

/*
 * void* __syscall_mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset)
 *
 * Memetakan file atau perangkat ke memori.
 *
 * Input:
 *   [ESP+4]  = addr   - alamat yang diinginkan
 *   [ESP+8]  = length - panjang mapping
 *   [ESP+12] = prot   - proteksi memori
 *   [ESP+16] = flags  - flags mapping
 *   [ESP+20] = fd     - file descriptor
 *   [ESP+24] = offset - offset dalam file
 *
 * Output:
 *   EAX = pointer ke mapped memory atau MAP_FAILED (-1) pada error
 *
 * CATATAN:
 *   Pada x86 32-bit, mmap menggunakan struct mmap_arg_struct
 *   yang diletakkan di memori, bukan register langsung.
 *   Nomor syscall mmap adalah 90.
 */
    .global __syscall_mmap
    .type __syscall_mmap, @function
__syscall_mmap:
    push    %ebp
    mov     %esp, %ebp
    sub     $24, %esp               /* Alokasi stack untuk struct */

    /* Bangun struct mmap_arg_struct di stack */
    mov     8(%ebp), %eax           /* addr */
    mov     %eax, (%esp)
    mov     12(%ebp), %eax          /* length */
    mov     %eax, 4(%esp)
    mov     16(%ebp), %eax          /* prot */
    mov     %eax, 8(%esp)
    mov     20(%ebp), %eax          /* flags */
    mov     %eax, 12(%esp)
    mov     24(%ebp), %eax          /* fd */
    mov     %eax, 16(%esp)
    mov     28(%ebp), %eax          /* offset */
    mov     %eax, 20(%esp)

    mov     $90, %eax               /* Nomor syscall mmap (x86 32-bit) */
    mov     %esp, %ebx              /* Pointer ke struct */

    int     $0x80

    add     $24, %esp               /* Bersihkan stack */

    SYSCALL_ERROR

    pop     %ebp
    ret
    .size __syscall_mmap, . - __syscall_mmap

/*
 * int __syscall_munmap(void* addr, size_t length)
 *
 * Menghapus mapping memori.
 *
 * Input:
 *   [ESP+4]  = addr   - alamat mapping
 *   [ESP+8]  = length - panjang mapping
 *
 * Output:
 *   EAX = 0 pada sukses, -1 pada error
 *
 * CATATANE:
 *   Nomor syscall munmap adalah 91 pada x86 32-bit.
 */
    .global __syscall_munmap
    .type __syscall_munmap, @function
__syscall_munmap:
    push    %ebp
    mov     %esp, %ebp

    mov     $91, %eax               /* Nomor syscall munmap (x86 32-bit) */
    mov     8(%ebp), %ebx           /* addr */
    mov     12(%ebp), %ecx          /* length */

    int     $0x80

    SYSCALL_ERROR

    pop     %ebp
    ret
    .size __syscall_munmap, . - __syscall_munmap

/* ========================================================================
 * SECTION DATA - Untuk errno (jika diperlukan)
 * ======================================================================== */

    .section .note.GNU-stack,"",@progbits

/* End of arch/x86/syscall.S */
