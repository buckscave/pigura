/*
 * PIGURA LIBC - SYS/SYSCALL.H
 * =============================
 * Header untuk syscall numbers dan fungsi syscall.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 2.0 - Disinkronkan dengan kernel Pigura OS
 *
 * PENTING: Syscall numbers ini HARUS sama dengan yang didefinisikan
 * di kernel: SUMBER/inti/konstanta.h
 */

#ifndef LIBC_SYS_SYSCALL_H
#define LIBC_SYS_SYSCALL_H

/* ============================================================
 * DEPENDENSI
 * ============================================================
 */
#include <stddef.h>

/* ============================================================
 * SYSCALL NUMBERS - PIGURA OS
 * ============================================================
 * Nomor syscall untuk Pigura OS - konsisten dengan kernel.
 * Lihat: SUMBER/inti/konstanta.h untuk definisi kernel.
 *
 * Konvensi Pigura OS:
 * - Syscall numbers dimulai dari 0
 * - Urutan logis berdasarkan kategori fungsi
 * - Kompatibel dengan semua arsitektur (x86, x86_64, ARM, ARM64)
 */

/* I/O Dasar - 0-9 */
#define SYS_READ        0
#define SYS_WRITE       1
#define SYS_OPEN        2
#define SYS_CLOSE       3
#define SYS_FORK        4
#define SYS_EXEC        5       /* execve */
#define SYS_EXIT        6
#define SYS_GETPID      7
#define SYS_GETPPID     8
#define SYS_BRK         9

/* Memory Management - 10-19 */
#define SYS_SBRK        10
#define SYS_MMAP        11
#define SYS_MUNMAP      12
#define SYS_SLEEP       13
#define SYS_USLEEP      14
#define SYS_GETCWD      15
#define SYS_CHDIR       16
#define SYS_MKDIR       17
#define SYS_RMDIR       18
#define SYS_UNLINK      19

/* File Operations - 20-29 */
#define SYS_STAT        20
#define SYS_FSTAT       21
#define SYS_LSEEK       22
#define SYS_DUP         23
#define SYS_DUP2        24
#define SYS_PIPE        25
#define SYS_WAIT        26
#define SYS_WAITPID     27
#define SYS_KILL        28
#define SYS_SIGNAL      29

/* Signal & Process - 30-39 */
#define SYS_SIGACTION   30
#define SYS_TIME        31
#define SYS_GETTIME     32      /* gettimeofday */
#define SYS_YIELD       33
#define SYS_GETUID      34
#define SYS_GETGID      35
#define SYS_SETUID      36
#define SYS_SETGID      37
#define SYS_IOCTL       38
#define SYS_READDIR     39

/* System Info - 40-49 */
#define SYS_GETINFO     40
/* 41-49 reserved for future use */

/* Network - 50-59 (future) */
#define SYS_SOCKET      50
#define SYS_BIND        51
#define SYS_LISTEN      52
#define SYS_ACCEPT      53
#define SYS_CONNECT     54
#define SYS_SEND        55
#define SYS_RECV        56
#define SYS_SENDTO      57
#define SYS_RECVFROM    58
#define SYS_SHUTDOWN    59

/* Extended - 60-63 */
#define SYS_ACCESS      60
#define SYS_LINK        61
#define SYS_SYMLINK     62
#define SYS_READLINK    63

/* Total syscall count */
#define SYS_MAX         64

/* ============================================================
 * KOMPATIBILITAS - Alias untuk fungsi Linux-like
 * ============================================================
 */

/* execve adalah alias untuk SYS_EXEC */
#define SYS_execve      SYS_EXEC

/* Aliases untuk kompatibilitas dengan kode existing */
#define SYS_geteuid     SYS_GETUID    /* Pigura: single UID */
#define SYS_getegid     SYS_GETGID    /* Pigura: single GID */

/* ============================================================
 * SYSCALL WRAPPER FUNCTIONS
 * ============================================================
 * Fungsi-fungsi ini diimplementasikan dalam assembly untuk
 * masing-masing arsitektur di internal/arch/<arch>/syscall.S
 */

/* Syscall dengan berbagai jumlah argumen */
extern long syscall0(long num);
extern long syscall1(long num, long a1);
extern long syscall2(long num, long a1, long a2);
extern long syscall3(long num, long a1, long a2, long a3);
extern long syscall4(long num, long a1, long a2, long a3, long a4);
extern long syscall5(long num, long a1, long a2, long a3, long a4, long a5);
extern long syscall6(long num, long a1, long a2, long a3, long a4, long a5, long a6);

/* Variadic syscall wrapper */
extern long syscall(long num, ...);

/* ============================================================
 * HELPER MACROS
 * ============================================================
 */

/* Macro untuk memudahkan pemanggilan syscall */
#define SYSCALL0(num)           syscall0(num)
#define SYSCALL1(num, a1)       syscall1(num, (long)(a1))
#define SYSCALL2(num, a1, a2)   syscall2(num, (long)(a1), (long)(a2))
#define SYSCALL3(num, a1, a2, a3) syscall3(num, (long)(a1), (long)(a2), (long)(a3))
#define SYSCALL4(num, a1, a2, a3, a4) syscall4(num, (long)(a1), (long)(a2), (long)(a3), (long)(a4))
#define SYSCALL5(num, a1, a2, a3, a4, a5) syscall5(num, (long)(a1), (long)(a2), (long)(a3), (long)(a4), (long)(a5))
#define SYSCALL6(num, a1, a2, a3, a4, a5, a6) syscall6(num, (long)(a1), (long)(a2), (long)(a3), (long)(a4), (long)(a5), (long)(a6))

/* Macro untuk check error setelah syscall */
#define SYSCALL_CHECK(ret)      ((ret) < 0 ? -1 : (ret))

/* ============================================================
 * HELPER FUNCTIONS (didefinisikan di internal/syscall.c)
 * ============================================================
 */

/*
 * __set_errno_from_syscall - Set errno dari hasil syscall
 *
 * Parameter:
 *   result - Hasil syscall (negatif = error)
 *
 * Return: -1 jika error, result jika sukses
 */
extern int __set_errno_from_syscall(long result);

#endif /* LIBC_SYS_SYSCALL_H */
