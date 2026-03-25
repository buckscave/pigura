/*
 * PIGURA LIBC - UNISTD/SLEEP.C
 * =============================
 * Implementasi fungsi sleep dan timer POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Fungsi yang diimplementasikan:
 * - sleep()  - Tidur dalam detik
 * - usleep() - Tidur dalam mikrodetik
 * - alarm()  - Set alarm timer
 * - pause()  - Menunggu sinyal
 */

#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

/* ============================================================
 * SYSCALL WRAPPER
 * ============================================================
 * Deklarasi fungsi syscall kernel untuk arsitektur x86_64.
 */

/* Syscall number untuk x86_64 Linux */
#define SYS_nanosleep  35
#define SYS_alarm      37
#define SYS_pause      34

/* Fungsi wrapper syscall assembly */
extern long __syscall0(long n);
extern long __syscall1(long n, long a1);
extern long __syscall2(long n, long a1, long a2);
extern long __syscall4(long n, long a1, long a2, long a3, long a4);

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
 *
 * Parameter:
 *   result - Hasil syscall (negatif = error)
 *
 * Return: -1 untuk menandakan error
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
 *
 * Parameter:
 *   seconds - Jumlah detik untuk tidur
 *
 * Return: 0 jika tidur penuh, atau sisa waktu jika terinterupsi
 *
 * Catatan: Bisa terinterupsi oleh sinyal
 */
unsigned int sleep(unsigned int seconds) {
    struct timespec req;
    struct timespec rem;
    long result;

    /* Handle kasus seconds = 0 */
    if (seconds == 0) {
        return 0;
    }

    /* Setup request time */
    req.tv_sec = (long)seconds;
    req.tv_nsec = 0;

    /* Panggil nanosleep syscall */
    result = __syscall4(SYS_nanosleep, (long)&req, (long)&rem, 0, 0);

    /* Jika terinterupsi oleh sinyal */
    if (result == -4 /* EINTR */) {
        errno = EINTR;
        /* Kembalikan sisa waktu dalam detik */
        return (unsigned int)rem.tv_sec +
               (rem.tv_nsec > 0 ? 1 : 0);
    }

    /* Handle error lain */
    if (result < 0) {
        __set_errno(result);
        return seconds;
    }

    return 0;
}

/*
 * usleep - Menunda eksekusi dalam mikrodetik
 *
 * Parameter:
 *   useconds - Jumlah mikrodetik untuk tidur
 *
 * Return: 0 jika berhasil, -1 jika error
 *
 * Catatan: useconds harus < 1.000.000 (1 detik)
 *          Deprecated, gunakan nanosleep()
 */
int usleep(useconds_t useconds) {
    struct timespec req;
    struct timespec rem;
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

    /* Konversi mikrodetik ke detik dan nanodetik */
    req.tv_sec = (long)(useconds / 1000000);
    req.tv_nsec = (long)((useconds % 1000000) * 1000);

    /* Panggil nanosleep syscall */
    result = __syscall4(SYS_nanosleep, (long)&req, (long)&rem, 0, 0);

    /* Handle error */
    if (result < 0) {
        if (result == -4) { /* EINTR */
            errno = EINTR;
        } else {
            __set_errno(result);
        }
        return -1;
    }

    return 0;
}

/*
 * alarm - Mengatur timer SIGALRM
 *
 * Parameter:
 *   seconds - Detik sampai alarm, 0 untuk membatalkan
 *
 * Return: Waktu tersisa dari alarm sebelumnya, atau 0
 *
 * Catatan: Mengirim SIGALRM saat waktu habis
 */
unsigned int alarm(unsigned int seconds) {
    long result;

    /* Panggil syscall alarm */
    result = __syscall1(SYS_alarm, (long)seconds);

    /* alarm tidak pernah gagal */
    return (unsigned int)result;
}

/*
 * pause - Menunggu sinyal
 *
 * Return: -1 dengan errno = EINTR jika terinterupsi
 *
 * Catatan: Selalu return -1 karena hanya kembali
 *          setelah menerima sinyal
 */
int pause(void) {
    long result;

    /* Panggil syscall pause */
    result = __syscall0(SYS_pause);

    /* pause hanya kembali jika ada sinyal */
    errno = EINTR;
    return -1;
}
