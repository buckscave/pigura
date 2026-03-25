/*
 * syscall.S - Syscall wrapper generik untuk Pigura OS C Library
 *
 * File ini menyediakan wrapper generik untuk syscall yang akan
 * memanggil implementasi spesifik arsitektur.
 *
 * Bagian dari Pigura OS C Library
 * Lisensi: MIT
 */

/*
 * CATATAN IMPLEMENTASI:
 * ---------------------
 * File ini adalah wrapper tingkat tinggi yang memilih implementasi
 * syscall yang tepat berdasarkan arsitektur yang terdeteksi saat kompilasi.
 *
 * Setiap arsitektur harus menyediakan implementasi syscall0 hingga syscall6
 * serta wrapper khusus untuk brk, mmap, dan munmap.
 */

/* ========================================================================
 * SELEKSI ARSITEKTUR
 * ======================================================================== */

#if defined(__i386__) || defined(__i686__) || defined(i386)
    /* x86 32-bit */
    #include "arch/x86/syscall.S"

#elif defined(__x86_64__) || defined(__amd64__)
    /* x86_64 */
    #include "arch/x86_64/syscall.S"

#elif defined(__arm__) && !defined(__ARM_ARCH_7A__)
    /* ARM 32-bit generik */
    #include "arch/arm/syscall.S"

#elif defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__)
    /* ARMv7 */
    #include "arch/armv7/syscall.S"

#else
    #error "Arsitektur tidak didukung: syscall.S memerlukan implementasi spesifik arsitektur"
#endif

/*
 * ========================================================================
 * DOKUMENTASI FUNGSI SYSCALL
 * ========================================================================
 *
 * Semua fungsi syscall mengikuti konvensi pemanggilan C (cdecl untuk x86,
 * System V AMD64 ABI untuk x86_64, AAPCS untuk ARM).
 *
 * Fungsi-fungsi yang disediakan:
 *
 * long syscall0(long num);
 *   - Syscall tanpa argumen
 *   - num: nomor syscall
 *   - Return: nilai kembalian syscall atau -1 pada error
 *
 * long syscall1(long num, long a1);
 *   - Syscall dengan 1 argumen
 *   - num: nomor syscall
 *   - a1: argumen pertama
 *   - Return: nilai kembalian syscall atau -1 pada error
 *
 * long syscall2(long num, long a1, long a2);
 *   - Syscall dengan 2 argumen
 *
 * long syscall3(long num, long a1, long a2, long a3);
 *   - Syscall dengan 3 argumen
 *
 * long syscall4(long num, long a1, long a2, long a3, long a4);
 *   - Syscall dengan 4 argumen
 *
 * long syscall5(long num, long a1, long a2, long a3, long a4, long a5);
 *   - Syscall dengan 5 argumen
 *
 * long syscall6(long num, long a1, long a2, long a3, long a4, long a5, long a6);
 *   - Syscall dengan 6 argumen
 *
 * ========================================================================
 * WRAPPER KHUSUS
 * ========================================================================
 *
 * void* __syscall_brk(void* addr);
 *   - Mengatur atau mendapatkan batas program break
 *   - addr: alamat baru untuk program break (0 untuk mendapatkan nilai saat ini)
 *   - Return: alamat program break baru atau -1 pada error
 *
 * void* __syscall_mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);
 *   - Memetakan file atau perangkat ke memori
 *   - addr: alamat yang diinginkan (bisa NULL)
 *   - length: panjang mapping
 *   - prot: proteksi memori (PROT_READ, PROT_WRITE, PROT_EXEC, PROT_NONE)
 *   - flags: flags mapping (MAP_SHARED, MAP_PRIVATE, MAP_ANONYMOUS, dll)
 *   - fd: file descriptor (untuk anonymous mapping, gunakan -1 dengan MAP_ANONYMOUS)
 *   - offset: offset dalam file
 *   - Return: pointer ke mapped memory atau MAP_FAILED pada error
 *
 * int __syscall_munmap(void* addr, size_t length);
 *   - Menghapus mapping memori
 *   - addr: alamat mapping
 *   - length: panjang mapping
 *   - Return: 0 pada sukses, -1 pada error
 *
 * ========================================================================
 * ERROR HANDLING
 * ========================================================================
 *
 * Semua fungsi syscall mengembalikan -1 pada error dan mengatur errno.
 *
 * Cara mendeteksi error berbeda per arsitektur:
 * - x86 32-bit: nilai negatif di eax
 * - x86_64: nilai dalam rentang -4095 hingga -1 di rax
 * - ARM: nilai negatif di r0
 *
 * Implementasi arsitektur harus mengkonversi error code ke -1 dan mengatur errno.
 */

/*
 * ========================================================================
 * KONSTANTA SYSCALL NUMBER (contoh untuk referensi)
 * ========================================================================
 *
 * Nomor syscall berbeda per arsitektur. Berikut adalah contoh untuk x86_64:
 *
 * #define SYS_read            0
 * #define SYS_write           1
 * #define SYS_open            2
 * #define SYS_close           3
 * #define SYS_stat            4
 * #define SYS_fstat           5
 * #define SYS_lstat           6
 * #define SYS_poll            7
 * #define SYS_lseek           8
 * #define SYS_mmap            9
 * #define SYS_mprotect        10
 * #define SYS_munmap          11
 * #define SYS_brk             12
 * #define SYS_rt_sigaction    13
 * #define SYS_rt_sigprocmask  14
 * #define SYS_ioctl           16
 * #define SYS_pread64         17
 * #define SYS_pwrite64        18
 * #define SYS_readv           19
 * #define SYS_writev          20
 * #define SYS_pipe            22
 * #define SYS_select          23
 * #define SYS_madvise         28
 * #define SYS_dup             32
 * #define SYS_dup2            33
 * #define SYS_pause           34
 * #define SYS_nanosleep       35
 * #define SYS_getpid          39
 * #define SYS_socket          41
 * #define SYS_connect         42
 * #define SYS_accept          43
 * #define SYS_sendto          44
 * #define SYS_recvfrom        45
 * #define SYS_shutdown        48
 * #define SYS_bind            49
 * #define SYS_listen          50
 * #define SYS_getsockname     51
 * #define SYS_getpeername     52
 * #define SYS_clone           56
 * #define SYS_fork            57
 * #define SYS_vfork           58
 * #define SYS_execve          59
 * #define SYS_exit            60
 * #define SYS_wait4           61
 * #define SYS_kill            62
 * #define SYS_uname           63
 * #define SYS_fcntl           72
 * #define SYS_flock           73
 * #define SYS_mkdir           83
 * #define SYS_rmdir           84
 * #define SYS_unlink          87
 * #define SYS_symlink         88
 * #define SYS_readlink        89
 * #define SYS_chmod           90
 * #define SYS_fchmod          91
 * #define SYS_chown           92
 * #define SYS_fchown          93
 * #define SYS_getuid          102
 * #define SYS_getgid          104
 * #define SYS_setuid          105
 * #define SYS_setgid          106
 * #define SYS_geteuid         107
 * #define SYS_getegid         108
 * #define SYS_setpgid         109
 * #define SYS_getpgid         121
 * #define SYS_setsid          112
 * #define SYS_getsid          124
 * #define SYS_getcwd          79
 * #define SYS_chdir           80
 * #define SYS_fchdir          81
 * #define SYS_rename          82
 * #define SYS_umask           95
 * #define SYS_gettimeofday    96
 * #define SYS_settimeofday    164
 * #define SYS_time            201
 */

/*
 * ========================================================================
 * CONTOH PENGGUNAAN DARI C
 * ========================================================================
 *
 * // Deklarasi fungsi eksternal
 * extern long syscall0(long num);
 * extern long syscall1(long num, long a1);
 * extern long syscall2(long num, long a1, long a2);
 * extern long syscall3(long num, long a1, long a2, long a3);
 * extern long syscall4(long num, long a1, long a2, long a3, long a4);
 * extern long syscall5(long num, long a1, long a2, long a3, long a4, long a5);
 * extern long syscall6(long num, long a1, long a2, long a3, long a4, long a5, long a6);
 *
 * // Contoh: write(1, "Hello", 5)
 * syscall3(SYS_write, 1, (long)"Hello", 5);
 *
 * // Contoh: exit(0)
 * syscall1(SYS_exit, 0);
 *
 * // Contoh: mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)
 * syscall6(SYS_mmap, 0, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
 */

/* End of syscall.S */
