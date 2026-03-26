/*
 * arch/arm/syscall.S - Syscall untuk ARM 32-bit menggunakan SVC
 *
 * Bagian dari Pigura OS C Library
 * Lisensi: MIT
 *
 * KONVENSI SYSCALL ARM 32-bit:
 * ----------------------------
 * - Nomor syscall diletakkan di register R7
 * - Argumen diletakkan di: R0, R1, R2, R3, R4, R5 (urutan)
 * - Hasil syscall dikembalikan di R0
 * - Error ditandai dengan nilai negatif di R0
 * - Error code adalah -errno (negasi dari errno)
 * - Instruksi yang digunakan: SVC #0 (atau SWI 0x0)
 *
 * Konvensi pemanggilan AAPCS (ARM Architecture Procedure Call Standard):
 * - Argumen: R0, R1, R2, R3, kemudian stack
 * - Return value: R0 (dan R1 untuk 64-bit)
 * - Caller-saved: R0-R3, R12, LR, PSR
 * - Callee-saved: R4-R11, SP
 */

    .text
    .align 4

/* ========================================================================
 * DEKLARASI EXTERN - errno
 * ======================================================================== */

    .extern errno

/* ========================================================================
 * KONSTANTA SYSCALL NUMBER
 * ======================================================================== */

#define SYS_brk     45
#define SYS_mmap    192
#define SYS_munmap  91

/* ========================================================================
 * MAKRO HELPER
 * ======================================================================== */

/*
 * SYSCALL_ERROR - Periksa error dan atur errno jika perlu
 *
 * Pada ARM, error ditandai dengan nilai negatif di R0.
 * Nilai absolut dari nilai negatif tersebut adalah errno.
 */
.macro SYSCALL_ERROR
    cmp     r0, #0              /* Periksa apakah R0 negatif */
    bge     1f                  /* Jika positif atau nol, tidak ada error */
    rsb     r1, r0, #0          /* Negasikan untuk mendapatkan error code */
    ldr     r2, =errno          /* Alamat errno */
    str     r1, [r2]            /* Simpan ke errno global */
    mvn     r0, #0              /* Kembalikan -1 (mvn r0, #0 = ~0 = -1) */
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
 *   R0 = num - nomor syscall
 *
 * Output:
 *   R0 = nilai kembalian syscall atau -1 pada error
 */
    .global syscall0
    .type syscall0, %function
    .align 4
syscall0:
    /* Simpan nomor syscall ke R7 */
    mov     r7, r0              /* Nomor syscall */
    
    /* Panggil kernel */
    svc     #0                  /* Supervisor call */
    
    /* Error handling */
    SYSCALL_ERROR
    
    /* Kembali - R0 sudah berisi return value */
    bx      lr
    .size syscall0, . - syscall0

/*
 * long syscall1(long num, long a1)
 *
 * Syscall dengan 1 argumen
 *
 * Input:
 *   R0 = num - nomor syscall
 *   R1 = a1  - argumen pertama
 *
 * Output:
 *   R0 = nilai kembalian syscall atau -1 pada error
 */
    .global syscall1
    .type syscall1, %function
    .align 4
syscall1:
    /* Input: r0=num, r1=a1 */
    /* Syscall butuh: r7=num, r0=a1 */
    
    mov     r7, r0              /* r7 = nomor syscall */
    mov     r0, r1              /* r0 = argumen 1 */
    
    svc     #0
    
    SYSCALL_ERROR
    
    bx      lr
    .size syscall1, . - syscall1

/*
 * long syscall2(long num, long a1, long a2)
 *
 * Syscall dengan 2 argumen
 *
 * Input:
 *   R0 = num - nomor syscall
 *   R1 = a1  - argumen pertama
 *   R2 = a2  - argumen kedua
 *
 * Output:
 *   R0 = nilai kembalian syscall atau -1 pada error
 */
    .global syscall2
    .type syscall2, %function
    .align 4
syscall2:
    /* Input: r0=num, r1=a1, r2=a2 */
    /* Syscall butuh: r7=num, r0=a1, r1=a2 */
    
    mov     r7, r0              /* r7 = nomor syscall */
    mov     r0, r1              /* r0 = argumen 1 */
    mov     r1, r2              /* r1 = argumen 2 */
    
    svc     #0
    
    SYSCALL_ERROR
    
    bx      lr
    .size syscall2, . - syscall2

/*
 * long syscall3(long num, long a1, long a2, long a3)
 *
 * Syscall dengan 3 argumen
 *
 * Input:
 *   R0 = num - nomor syscall
 *   R1 = a1  - argumen pertama
 *   R2 = a2  - argumen kedua
 *   R3 = a3  - argumen ketiga
 *
 * Output:
 *   R0 = nilai kembalian syscall atau -1 pada error
 */
    .global syscall3
    .type syscall3, %function
    .align 4
syscall3:
    /* Input: r0=num, r1=a1, r2=a2, r3=a3 */
    /* Syscall butuh: r7=num, r0=a1, r1=a2, r2=a3 */
    
    mov     r7, r0              /* r7 = nomor syscall */
    mov     r0, r1              /* r0 = argumen 1 */
    mov     r1, r2              /* r1 = argumen 2 */
    mov     r2, r3              /* r2 = argumen 3 */
    
    svc     #0
    
    SYSCALL_ERROR
    
    bx      lr
    .size syscall3, . - syscall3

/*
 * long syscall4(long num, long a1, long a2, long a3, long a4)
 *
 * Syscall dengan 4 argumen
 *
 * Input:
 *   R0 = num - nomor syscall
 *   R1 = a1  - argumen pertama
 *   R2 = a2  - argumen kedua
 *   R3 = a3  - argumen ketiga
 *   [SP] = a4 - argumen keempat (di stack)
 *
 * Output:
 *   R0 = nilai kembalian syscall atau -1 pada error
 */
    .global syscall4
    .type syscall4, %function
    .align 4
syscall4:
    /* Input: r0=num, r1=a1, r2=a2, r3=a3, [sp]=a4 */
    /* Syscall butuh: r7=num, r0=a1, r1=a2, r2=a3, r3=a4 */
    
    push    {r4, lr}            /* Simpan r4 dan lr */
    
    mov     r7, r0              /* r7 = nomor syscall */
    mov     r0, r1              /* r0 = argumen 1 */
    mov     r1, r2              /* r1 = argumen 2 */
    mov     r2, r3              /* r2 = argumen 3 */
    ldr     r3, [sp, #8]        /* r3 = argumen 4 dari stack */
    
    svc     #0
    
    SYSCALL_ERROR
    
    pop     {r4, pc}            /* Restore r4 dan return */
    .size syscall4, . - syscall4

/*
 * long syscall5(long num, long a1, long a2, long a3, long a4, long a5)
 *
 * Syscall dengan 5 argumen
 *
 * Input:
 *   R0 = num - nomor syscall
 *   R1 = a1  - argumen pertama
 *   R2 = a2  - argumen kedua
 *   R3 = a3  - argumen ketiga
 *   [SP]    = a4 - argumen keempat
 *   [SP+4]  = a5 - argumen kelima
 *
 * Output:
 *   R0 = nilai kembalian syscall atau -1 pada error
 */
    .global syscall5
    .type syscall5, %function
    .align 4
syscall5:
    /* Input: r0=num, r1=a1, r2=a2, r3=a3, [sp]=a4, [sp+4]=a5 */
    /* Syscall butuh: r7=num, r0=a1, r1=a2, r2=a3, r3=a4, r4=a5 */
    
    push    {r4, r5, lr}        /* Simpan r4, r5, lr */
    
    mov     r7, r0              /* r7 = nomor syscall */
    mov     r0, r1              /* r0 = argumen 1 */
    mov     r1, r2              /* r1 = argumen 2 */
    mov     r2, r3              /* r2 = argumen 3 */
    ldr     r3, [sp, #12]       /* r3 = argumen 4 dari stack */
    ldr     r4, [sp, #16]       /* r4 = argumen 5 dari stack */
    
    svc     #0
    
    SYSCALL_ERROR
    
    pop     {r4, r5, pc}        /* Restore dan return */
    .size syscall5, . - syscall5

/*
 * long syscall6(long num, long a1, long a2, long a3, long a4, long a5, long a6)
 *
 * Syscall dengan 6 argumen
 *
 * Input:
 *   R0 = num - nomor syscall
 *   R1 = a1  - argumen pertama
 *   R2 = a2  - argumen kedua
 *   R3 = a3  - argumen ketiga
 *   [SP]    = a4 - argumen keempat
 *   [SP+4]  = a5 - argumen kelima
 *   [SP+8]  = a6 - argumen keenam
 *
 * Output:
 *   R0 = nilai kembalian syscall atau -1 pada error
 */
    .global syscall6
    .type syscall6, %function
    .align 4
syscall6:
    /* Input: r0=num, r1=a1, r2=a2, r3=a3, [sp]=a4, [sp+4]=a5, [sp+8]=a6 */
    /* Syscall butuh: r7=num, r0=a1, r1=a2, r2=a3, r3=a4, r4=a5, r5=a6 */
    
    push    {r4, r5, lr}        /* Simpan r4, r5, lr */
    
    mov     r7, r0              /* r7 = nomor syscall */
    mov     r0, r1              /* r0 = argumen 1 */
    mov     r1, r2              /* r1 = argumen 2 */
    mov     r2, r3              /* r2 = argumen 3 */
    ldr     r3, [sp, #12]       /* r3 = argumen 4 */
    ldr     r4, [sp, #16]       /* r4 = argumen 5 */
    ldr     r5, [sp, #20]       /* r5 = argumen 6 */
    
    svc     #0
    
    SYSCALL_ERROR
    
    pop     {r4, r5, pc}        /* Restore dan return */
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
 *   R0 = addr - alamat baru untuk program break
 *
 * Output:
 *   R0 = alamat program break baru atau -1 pada error
 *
 * CATATAN:
 *   Syscall brk memiliki nomor 45 pada ARM Linux.
 *   Untuk mendapatkan nilai current break, panggil dengan addr = 0.
 */
    .global __syscall_brk
    .type __syscall_brk, %function
    .align 4
__syscall_brk:
    /* Simpan argumen */
    mov     r1, r0              /* Simpan addr di r1 */
    
    /* Setup syscall */
    mov     r7, #SYS_brk        /* Nomor syscall brk */
    mov     r0, r1              /* addr sebagai argumen */
    
    svc     #0
    
    /* brk tidak mengembalikan error dalam bentuk negatif */
    /* Nilai kembalian adalah alamat break baru */
    
    bx      lr
    .size __syscall_brk, . - __syscall_brk

/*
 * void* __syscall_mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset)
 *
 * Memetakan file atau perangkat ke memori.
 *
 * Input:
 *   R0 = addr   - alamat yang diinginkan
 *   R1 = length - panjang mapping
 *   R2 = prot   - proteksi memori
 *   R3 = flags  - flags mapping
 *   [SP]   = fd     - file descriptor
 *   [SP+4] = offset - offset dalam file
 *
 * Output:
 *   R0 = pointer ke mapped memory atau MAP_FAILED (-1) pada error
 *
 * CATATANE:
 *   Pada ARM, mmap menggunakan syscall nomor 192.
 *   Offset dibagi menjadi dua bagian jika menggunakan mmap2 (offset dalam halaman).
 *   Di sini kita menggunakan mmap biasa.
 */
    .global __syscall_mmap
    .type __syscall_mmap, %function
    .align 4
__syscall_mmap:
    /* Input: r0=addr, r1=length, r2=prot, r3=flags, [sp]=fd, [sp+4]=offset */
    /* Syscall butuh: r0=addr, r1=length, r2=prot, r3=flags, r4=fd, r5=offset_low, r6=offset_high */
    
    push    {r4, r5, r6, lr}
    
    mov     r7, #SYS_mmap       /* Nomor syscall mmap */
    /* r0-r3 sudah di tempat */
    ldr     r4, [sp, #16]       /* fd */
    ldr     r5, [sp, #20]       /* offset (low) */
    mov     r6, #0              /* offset high (untuk mmap 32-bit, offset adalah 32-bit) */
    
    svc     #0
    
    SYSCALL_ERROR
    
    pop     {r4, r5, r6, pc}
    .size __syscall_mmap, . - __syscall_mmap

/*
 * void* __syscall_mmap2(void* addr, size_t length, int prot, int flags, int fd, off_t offset)
 *
 * Variasi mmap dengan offset dalam satuan halaman (4096 bytes).
 *
 * Input:
 *   R0 = addr   - alamat yang diinginkan
 *   R1 = length - panjang mapping
 *   R2 = prot   - proteksi memori
 *   R3 = flags  - flags mapping
 *   [SP]   = fd     - file descriptor
 *   [SP+4] = offset - offset dalam halaman (offset_bytes / 4096)
 *
 * Output:
 *   R0 = pointer ke mapped memory atau MAP_FAILED (-1) pada error
 *
 * CATATAN:
 *   mmap2 menggunakan syscall nomor 192 pada ARM EABI.
 *   Offset adalah offset_bytes / 4096 (tidak dikalikan 4096).
 */
    .global __syscall_mmap2
    .type __syscall_mmap2, %function
    .align 4
__syscall_mmap2:
    push    {r4, r5, lr}
    
    mov     r7, #192            /* Syscall mmap2 number */
    /* r0-r3 sudah di tempat */
    ldr     r4, [sp, #12]       /* fd */
    ldr     r5, [sp, #16]       /* offset dalam halaman */
    
    svc     #0
    
    SYSCALL_ERROR
    
    pop     {r4, r5, pc}
    .size __syscall_mmap2, . - __syscall_mmap2

/*
 * int __syscall_munmap(void* addr, size_t length)
 *
 * Menghapus mapping memori.
 *
 * Input:
 *   R0 = addr   - alamat mapping
 *   R1 = length - panjang mapping
 *
 * Output:
 *   R0 = 0 pada sukses, -1 pada error
 *
 * CATATANE:
 *   Syscall munmap memiliki nomor 91 pada ARM Linux.
 */
    .global __syscall_munmap
    .type __syscall_munmap, %function
    .align 4
__syscall_munmap:
    mov     r7, #SYS_munmap     /* Nomor syscall munmap */
    /* r0=addr, r1=length sudah di tempat */
    
    svc     #0
    
    SYSCALL_ERROR
    
    bx      lr
    .size __syscall_munmap, . - __syscall_munmap

/*
 * int __syscall_mprotect(void* addr, size_t len, int prot)
 *
 * Mengubah proteksi memori.
 *
 * Input:
 *   R0 = addr - alamat awal
 *   R1 = len  - panjang region
 *   R2 = prot - proteksi baru
 *
 * Output:
 *   R0 = 0 pada sukses, -1 pada error
 *
 * CATATAN:
 *   Syscall mprotect memiliki nomor 125 pada ARM Linux.
 */
    .global __syscall_mprotect
    .type __syscall_mprotect, %function
    .align 4
__syscall_mprotect:
    mov     r7, #125            /* Nomor syscall mprotect */
    /* r0-r2 sudah di tempat */
    
    svc     #0
    
    SYSCALL_ERROR
    
    bx      lr
    .size __syscall_mprotect, . - __syscall_mprotect

/* ========================================================================
 * FUNGSI HELPER TAMBAHAN
 * ======================================================================== */

/*
 * long syscall0_naked(long num)
 *
 * Syscall tanpa error handling - mengembalikan nilai mentah dari kernel.
 */
    .global syscall0_naked
    .type syscall0_naked, %function
    .align 4
syscall0_naked:
    mov     r7, r0
    svc     #0
    bx      lr
    .size syscall0_naked, . - syscall0_naked

/*
 * long syscall1_naked(long num, long a1)
 */
    .global syscall1_naked
    .type syscall1_naked, %function
    .align 4
syscall1_naked:
    mov     r7, r0
    mov     r0, r1
    svc     #0
    bx      lr
    .size syscall1_naked, . - syscall1_naked

/*
 * long syscall2_naked(long num, long a1, long a2)
 */
    .global syscall2_naked
    .type syscall2_naked, %function
    .align 4
syscall2_naked:
    mov     r7, r0
    mov     r0, r1
    mov     r1, r2
    svc     #0
    bx      lr
    .size syscall2_naked, . - syscall2_naked

/*
 * long syscall3_naked(long num, long a1, long a2, long a3)
 */
    .global syscall3_naked
    .type syscall3_naked, %function
    .align 4
syscall3_naked:
    mov     r7, r0
    mov     r0, r1
    mov     r1, r2
    mov     r2, r3
    svc     #0
    bx      lr
    .size syscall3_naked, . - syscall3_naked

/*
 * long syscall4_naked(long num, long a1, long a2, long a3, long a4)
 */
    .global syscall4_naked
    .type syscall4_naked, %function
    .align 4
syscall4_naked:
    push    {r4, lr}
    mov     r7, r0
    mov     r0, r1
    mov     r1, r2
    mov     r2, r3
    ldr     r3, [sp, #8]
    svc     #0
    pop     {r4, pc}
    .size syscall4_naked, . - syscall4_naked

/*
 * long syscall5_naked(long num, long a1, long a2, long a3, long a4, long a5)
 */
    .global syscall5_naked
    .type syscall5_naked, %function
    .align 4
syscall5_naked:
    push    {r4, r5, lr}
    mov     r7, r0
    mov     r0, r1
    mov     r1, r2
    mov     r2, r3
    ldr     r3, [sp, #12]
    ldr     r4, [sp, #16]
    svc     #0
    pop     {r4, r5, pc}
    .size syscall5_naked, . - syscall5_naked

/*
 * long syscall6_naked(long num, long a1, long a2, long a3, long a4, long a5, long a6)
 */
    .global syscall6_naked
    .type syscall6_naked, %function
    .align 4
syscall6_naked:
    push    {r4, r5, lr}
    mov     r7, r0
    mov     r0, r1
    mov     r1, r2
    mov     r2, r3
    ldr     r3, [sp, #12]
    ldr     r4, [sp, #16]
    ldr     r5, [sp, #20]
    svc     #0
    pop     {r4, r5, pc}
    .size syscall6_naked, . - syscall6_naked

/* ========================================================================
 * SECTION DATA
 * ======================================================================== */

    .section .note.GNU-stack,"",%progbits

/* End of arch/arm/syscall.S */
