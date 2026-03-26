/*
 * arch/armv7/syscall.S - Syscall untuk ARMv7 menggunakan SVC
 *
 * Bagian dari Pigura OS C Library
 * Lisensi: MIT
 *
 * KONVENSI SYSCALL ARMv7:
 * -----------------------
 * - Nomor syscall diletakkan di register R7
 * - Argumen diletakkan di: R0, R1, R2, R3, R4, R5 (urutan)
 * - Hasil syscall dikembalikan di R0
 * - Error ditandai dengan nilai negatif di R0
 * - Error code adalah -errno (negasi dari errno)
 * - Instruksi yang digunakan: SVC #0
 *
 * FITUR ARMv7:
 * -----------
 * - Mendukung mode ARM dan Thumb-2
 * - Instruksi yang lebih efisien
 * - Conditional execution yang lebih baik
 * - IT (If-Then) blocks untuk kondisional Thumb
 *
 * Konvensi pemanggilan AAPCS (ARM Architecture Procedure Call Standard):
 * - Argumen: R0, R1, R2, R3, kemudian stack
 * - Return value: R0 (dan R1 untuk 64-bit)
 * - Caller-saved: R0-R3, R12, LR, PSR
 * - Callee-saved: R4-R11, SP
 */

    .syntax unified
    .text
    .align 4

/* ========================================================================
 * DEKLARASI EXTERN - errno
 * ======================================================================== */

    .extern errno

/* ========================================================================
 * KONSTANTA SYSCALL NUMBER (ARM EABI)
 * ======================================================================== */

#define SYS_restart_syscall  0
#define SYS_exit             1
#define SYS_read             3
#define SYS_write            4
#define SYS_open             5
#define SYS_close            6
#define SYS_brk             45
#define SYS_mmap           192
#define SYS_mmap2          192
#define SYS_munmap          91
#define SYS_mprotect       125
#define SYS_madvise        220

/* ========================================================================
 * MAKRO HELPER
 * ======================================================================== */

/*
 * SYSCALL_ERROR - Periksa error dan atur errno jika perlu
 *
 * Pada ARMv7, error ditandai dengan nilai negatif di R0.
 * Nilai absolut dari nilai negatif tersebut adalah errno.
 *
 * Menggunakan conditional execution ARM untuk efisiensi.
 */
.macro SYSCALL_ERROR
    /* Periksa apakah R0 negatif menggunakan conditional execution */
    cmp     r0, #0
    it      lt                      /* If-Then block untuk Thumb mode */
    rsblt   r1, r0, #0              /* Negasikan jika negatif */
    it      lt
    ldrlt   r2, =errno              /* Load alamat errno jika negatif */
    it      lt
    strlt   r1, [r2]                /* Simpan error code ke errno */
    it      lt
    mvnlt   r0, #0                  /* Kembalikan -1 jika error */
.endm

/*
 * SYSCALL_ERROR_ARM - Versi ARM-only (lebih efisien di mode ARM)
 */
.macro SYSCALL_ERROR_ARM
    cmp     r0, #0
    rsblt   r1, r0, #0
    ldrlt   r2, =errno
    strlt   r1, [r2]
    mvnlt   r0, #0
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
    mov     r7, r0              /* Nomor syscall ke R7 */
    
    /* Panggil kernel - menggunakan SVC (Supervisor Call) */
    svc     #0
    
    /* Error handling dengan conditional execution */
    cmp     r0, #0
    bge     1f
    
    /* Error: negasikan dan simpan ke errno */
    rsb     r1, r0, #0
    ldr     r2, =errno
    str     r1, [r2]
    mvn     r0, #0              /* Return -1 */
1:
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
    
    /* Simpan num ke r7, lalu pindahkan a1 ke r0 */
    mov     r7, r0
    mov     r0, r1
    
    svc     #0
    
    /* Error handling */
    cmp     r0, #0
    bge     1f
    rsb     r1, r0, #0
    ldr     r2, =errno
    str     r1, [r2]
    mvn     r0, #0
1:
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
    
    mov     r7, r0
    mov     r0, r1
    mov     r1, r2
    
    svc     #0
    
    cmp     r0, #0
    bge     1f
    rsb     r1, r0, #0
    ldr     r2, =errno
    str     r1, [r2]
    mvn     r0, #0
1:
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
    
    mov     r7, r0
    mov     r0, r1
    mov     r1, r2
    mov     r2, r3
    
    svc     #0
    
    cmp     r0, #0
    bge     1f
    rsb     r1, r0, #0
    ldr     r2, =errno
    str     r1, [r2]
    mvn     r0, #0
1:
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
    
    /* Simpan register callee-saved yang akan digunakan */
    push    {r4, lr}
    
    mov     r7, r0
    mov     r0, r1
    mov     r1, r2
    mov     r2, r3
    ldr     r3, [sp, #8]        /* Load a4 dari stack (setelah push r4, lr) */
    
    svc     #0
    
    cmp     r0, #0
    bge     1f
    rsb     r1, r0, #0
    ldr     r2, =errno
    str     r1, [r2]
    mvn     r0, #0
1:
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
    
    push    {r4, r5, lr}
    
    mov     r7, r0
    mov     r0, r1
    mov     r1, r2
    mov     r2, r3
    ldr     r3, [sp, #12]       /* a4 (offset: 3 regs * 4 bytes = 12) */
    ldr     r4, [sp, #16]       /* a5 */
    
    svc     #0
    
    cmp     r0, #0
    bge     1f
    rsb     r1, r0, #0
    ldr     r2, =errno
    str     r1, [r2]
    mvn     r0, #0
1:
    pop     {r4, r5, pc}
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
    
    push    {r4, r5, r6, lr}
    
    mov     r7, r0
    mov     r0, r1
    mov     r1, r2
    mov     r2, r3
    ldr     r3, [sp, #16]       /* a4 */
    ldr     r4, [sp, #20]       /* a5 */
    ldr     r5, [sp, #24]       /* a6 */
    
    svc     #0
    
    cmp     r0, #0
    bge     1f
    rsb     r1, r0, #0
    ldr     r2, =errno
    str     r1, [r2]
    mvn     r0, #0
1:
    pop     {r4, r5, r6, pc}
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
 *   R0 = alamat program break baru
 *
 * CATATAN:
 *   Syscall brk memiliki nomor 45 pada ARMv7 Linux EABI.
 *   brk tidak mengembalikan error dalam bentuk negatif.
 *   Jika gagal, nilai kembalian sama dengan nilai saat ini.
 */
    .global __syscall_brk
    .type __syscall_brk, %function
    .align 4
__syscall_brk:
    /* Simpan argumen */
    mov     r1, r0              /* addr -> r1 */
    
    /* Setup syscall */
    mov     r7, #SYS_brk        /* 45 */
    mov     r0, r1              /* addr sebagai argumen pertama */
    
    svc     #0
    
    /* Tidak ada error handling untuk brk */
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
 * CATATAN:
 *   Pada ARMv7 EABI, mmap menggunakan syscall nomor 192.
 */
    .global __syscall_mmap
    .type __syscall_mmap, %function
    .align 4
__syscall_mmap:
    /* Simpan callee-saved registers */
    push    {r4, r5, r6, lr}
    
    mov     r7, #SYS_mmap       /* 192 */
    /* r0-r3 sudah di tempat (addr, length, prot, flags) */
    ldr     r4, [sp, #16]       /* fd */
    ldr     r5, [sp, #20]       /* offset low */
    mov     r6, #0              /* offset high (untuk 32-bit offset) */
    
    svc     #0
    
    cmp     r0, #0
    bge     1f
    rsb     r1, r0, #0
    ldr     r2, =errno
    str     r1, [r2]
    mvn     r0, #0
1:
    pop     {r4, r5, r6, pc}
    .size __syscall_mmap, . - __syscall_mmap

/*
 * void* __syscall_mmap2(void* addr, size_t length, int prot, int flags, int fd, unsigned long pgoff)
 *
 * Variasi mmap dengan offset dalam satuan halaman.
 *
 * Input:
 *   R0 = addr   - alamat yang diinginkan
 *   R1 = length - panjang mapping
 *   R2 = prot   - proteksi memori
 *   R3 = flags  - flags mapping
 *   [SP]   = fd     - file descriptor
 *   [SP+4] = pgoff  - offset dalam halaman (offset_bytes / PAGE_SIZE)
 *
 * Output:
 *   R0 = pointer ke mapped memory atau MAP_FAILED (-1) pada error
 *
 * CATATANE:
 *   mmap2 lebih portable untuk file besar karena offset
 *   tidak dibatasi oleh 32-bit signed integer.
 */
    .global __syscall_mmap2
    .type __syscall_mmap2, %function
    .align 4
__syscall_mmap2:
    push    {r4, r5, lr}
    
    mov     r7, #192            /* mmap2 syscall number */
    /* r0-r3 sudah di tempat */
    ldr     r4, [sp, #12]       /* fd */
    ldr     r5, [sp, #16]       /* pgoff */
    
    svc     #0
    
    cmp     r0, #0
    bge     1f
    rsb     r1, r0, #0
    ldr     r2, =errno
    str     r1, [r2]
    mvn     r0, #0
1:
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
 *   Syscall munmap memiliki nomor 91 pada ARMv7 Linux.
 */
    .global __syscall_munmap
    .type __syscall_munmap, %function
    .align 4
__syscall_munmap:
    mov     r7, #SYS_munmap     /* 91 */
    /* r0, r1 sudah di tempat */
    
    svc     #0
    
    cmp     r0, #0
    bge     1f
    rsb     r1, r0, #0
    ldr     r2, =errno
    str     r1, [r2]
    mvn     r0, #0
1:
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
 *   Syscall mprotect memiliki nomor 125 pada ARMv7 Linux.
 */
    .global __syscall_mprotect
    .type __syscall_mprotect, %function
    .align 4
__syscall_mprotect:
    mov     r7, #SYS_mprotect   /* 125 */
    /* r0-r2 sudah di tempat */
    
    svc     #0
    
    cmp     r0, #0
    bge     1f
    rsb     r1, r0, #0
    ldr     r2, =errno
    str     r1, [r2]
    mvn     r0, #0
1:
    bx      lr
    .size __syscall_mprotect, . - __syscall_mprotect

/* ========================================================================
 * FUNGSI VDSO HELPER (untuk ARMv7 dengan VDSO)
 * ======================================================================== */

/*
 * long __vdso_gettimeofday(struct timeval* tv, struct timezone* tz)
 *
 * Helper untuk gettimeofday menggunakan VDSO jika tersedia.
 * Fallback ke syscall jika VDSO tidak tersedia.
 *
 * Input:
 *   R0 = tv - pointer ke struct timeval
 *   R1 = tz - pointer ke struct timezone
 *
 * Output:
 *   R0 = 0 pada sukses, -1 pada error
 */
    .global __vdso_gettimeofday
    .type __vdso_gettimeofday, %function
    .align 4
__vdso_gettimeofday:
    /* Untuk saat ini, gunakan syscall biasa */
    /* Implementasi VDSO memerlukan runtime lookup */
    push    {r4, lr}
    
    mov     r7, #78             /* gettimeofday syscall number */
    /* r0, r1 sudah di tempat */
    
    svc     #0
    
    cmp     r0, #0
    bge     1f
    rsb     r1, r0, #0
    ldr     r2, =errno
    str     r1, [r2]
    mvn     r0, #0
1:
    pop     {r4, pc}
    .size __vdso_gettimeofday, . - __vdso_gettimeofday

/* ========================================================================
 * FUNGSI SYSCALL TANPA ERROR HANDLING (naked)
 * ======================================================================== */

/*
 * Versi naked dari syscall tanpa error handling.
 * Berguna untuk syscall yang tidak mengikuti konvensi error standar.
 */

    .global syscall0_naked
    .type syscall0_naked, %function
    .align 4
syscall0_naked:
    mov     r7, r0
    svc     #0
    bx      lr
    .size syscall0_naked, . - syscall0_naked

    .global syscall1_naked
    .type syscall1_naked, %function
    .align 4
syscall1_naked:
    mov     r7, r0
    mov     r0, r1
    svc     #0
    bx      lr
    .size syscall1_naked, . - syscall1_naked

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

    .global syscall6_naked
    .type syscall6_naked, %function
    .align 4
syscall6_naked:
    push    {r4, r5, r6, lr}
    mov     r7, r0
    mov     r0, r1
    mov     r1, r2
    mov     r2, r3
    ldr     r3, [sp, #16]
    ldr     r4, [sp, #20]
    ldr     r5, [sp, #24]
    svc     #0
    pop     {r4, r5, r6, pc}
    .size syscall6_naked, . - syscall6_naked

/* ========================================================================
 * IMPLEMENTASI THUMB-2 (opsional)
 * ======================================================================== */

#ifdef __thumb__
    .thumb
    .thumb_func

/*
 * Versi Thumb-2 dari syscall0
 */
    .global syscall0_thumb
    .type syscall0_thumb, %function
    .align 2
syscall0_thumb:
    mov     r7, r0
    svc     #0
    
    /* Error handling dengan IT block */
    cmp     r0, #0
    it      lt
    neglt   r1, r0
    it      lt
    ldrlt   r2, =errno
    it      lt
    strlt   r1, [r2]
    it      lt
    movlt   r0, #-1
    
    bx      lr
    .size syscall0_thumb, . - syscall0_thumb
#endif

/* ========================================================================
 * SECTION DATA
 * ======================================================================== */

    .section .note.GNU-stack,"",%progbits

/* End of arch/armv7/syscall.S */
