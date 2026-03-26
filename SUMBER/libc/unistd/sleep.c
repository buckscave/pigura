/*
 * PIGURA LIBC - UNISTD/SLEEP.C
 * =============================
 * Implementasi fungsi sleep dan timer POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 2.0 - Disinkronkan dengan syscall Pigura OS
 *
 * Fungsi yang diimplementasikan:
 * - sleep()  - Tidur dalam detik
 * - usleep() - Tidur dalam mikrodetik
 */

#include <sys/types.h>
#include <sys/syscall.h>   /* Syscall numbers dari header terpusat */
#include <errno.h>
#include <unistd.h>
#include <signal.h>

/* ============================================================
 * STRUKTUR TIMESPEC
 * ============================================================
 * Struktur untuk nanosleep syscall.
 */
struct timespec {
    long tv_sec;   /* Detik */
    long tv_nsec;  /* Nanodetik (0-999999999) */
};

/* ============================================================
 * FUNGSI HELPER
 * ============================================================
 */

/*
 * __set_errno - Set errno berdasarkan error code syscall
 */
static int __set_errno(long result) {
    if (result < 0 && result >= -4095) {
        errno = (int)(-result);
        return -1;
    }
    return (int)result;
}

/* ============================================================
 * IMPLEMENTASI FUNGSI SLEEP
 * ============================================================
 */

/*
 * sleep - Menunda eksekusi dalam detik
 */
unsigned int sleep(unsigned int seconds) {
    long result;

    /* Handle kasus seconds = 0 */
    if (seconds == 0) {
        return 0;
    }

    /* Gunakan syscall SLEEP Pigura OS */
    result = syscall1(SYS_SLEEP, (long)seconds);

    /* Jika terinterupsi */
    if (result < 0) {
        __set_errno(result);
        return seconds;  /* Return sisa waktu */
    }

    return 0;
}

/*
 * usleep - Menunda eksekusi dalam mikrodetik
 */
int usleep(useconds_t useconds) {
    long result;

    /* Validasi parameter */
    if (useconds >= 1000000) {
        errno = EINVAL;
        return -1;
    }

    /* Handle kasus useconds = 0 */
    if (useconds == 0) {
        return 0;
    }

    /* Gunakan syscall USLEEP Pigura OS */
    result = syscall1(SYS_USLEEP, (long)useconds);

    /* Handle error */
    if (result < 0) {
        __set_errno(result);
        return -1;
    }

    return 0;
}
