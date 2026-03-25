/*
 * PIGURA LIBC - UNISTD/IO.C
 * ==========================
 * Implementasi fungsi I/O standar POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Fungsi yang diimplementasikan:
 * - read()    - Membaca dari file descriptor
 * - write()   - Menulis ke file descriptor
 * - pread()   - Membaca pada offset tertentu
 * - pwrite()  - Menulis pada offset tertentu
 * - close()   - Menutup file descriptor
 * - dup()     - Duplikasi file descriptor
 * - dup2()    - Duplikasi ke fd tertentu
 */

#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

/* ============================================================
 * SYSCALL WRAPPER
 * ============================================================
 * Deklarasi fungsi syscall kernel untuk arsitektur x86_64.
 */

/* Syscall number untuk x86_64 Linux */
#define SYS_read    0
#define SYS_write   1
#define SYS_close   3
#define SYS_dup     32
#define SYS_dup2    33
#define SYS_pread64 17
#define SYS_pwrite64 18

/* Fungsi wrapper syscall assembly */
extern long __syscall0(long n);
extern long __syscall1(long n, long a1);
extern long __syscall2(long n, long a1, long a2);
extern long __syscall3(long n, long a1, long a2, long a3);
extern long __syscall4(long n, long a1, long a2, long a3, long a4);

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
 * IMPLEMENTASI FUNGSI I/O
 * ============================================================
 */

/*
 * read - Membaca data dari file descriptor
 *
 * Parameter:
 *   fd    - File descriptor
 *   buf   - Buffer untuk menyimpan data
 *   count - Jumlah byte yang akan dibaca
 *
 * Return: Jumlah byte yang dibaca, atau -1 jika error
 */
ssize_t read(int fd, void *buf, size_t count) {
    long result;

    /* Validasi parameter */
    if (buf == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Panggil syscall read */
    result = __syscall3(SYS_read, (long)fd, (long)buf, (long)count);

    return __set_errno(result);
}

/*
 * write - Menulis data ke file descriptor
 *
 * Parameter:
 *   fd    - File descriptor
 *   buf   - Buffer yang berisi data
 *   count - Jumlah byte yang akan ditulis
 *
 * Return: Jumlah byte yang ditulis, atau -1 jika error
 */
ssize_t write(int fd, const void *buf, size_t count) {
    long result;

    /* Validasi parameter */
    if (buf == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Panggil syscall write */
    result = __syscall3(SYS_write, (long)fd, (long)buf, (long)count);

    return __set_errno(result);
}

/*
 * pread - Membaca dari file descriptor pada offset tertentu
 *
 * Parameter:
 *   fd    - File descriptor
 *   buf   - Buffer untuk menyimpan data
 *   count - Jumlah byte yang akan dibaca
 *   offset - Offset posisi baca
 *
 * Return: Jumlah byte yang dibaca, atau -1 jika error
 *
 * Catatan: Tidak mengubah file offset
 */
ssize_t pread(int fd, void *buf, size_t count, off_t offset) {
    long result;

    /* Validasi parameter */
    if (buf == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (offset < 0) {
        errno = EINVAL;
        return -1;
    }

    /* Panggil syscall pread64 */
    result = __syscall4(SYS_pread64, (long)fd, (long)buf,
                        (long)count, (long)offset);

    return __set_errno(result);
}

/*
 * pwrite - Menulis ke file descriptor pada offset tertentu
 *
 * Parameter:
 *   fd    - File descriptor
 *   buf   - Buffer yang berisi data
 *   count - Jumlah byte yang akan ditulis
 *   offset - Offset posisi tulis
 *
 * Return: Jumlah byte yang ditulis, atau -1 jika error
 *
 * Catatan: Tidak mengubah file offset
 */
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset) {
    long result;

    /* Validasi parameter */
    if (buf == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (offset < 0) {
        errno = EINVAL;
        return -1;
    }

    /* Panggil syscall pwrite64 */
    result = __syscall4(SYS_pwrite64, (long)fd, (long)buf,
                        (long)count, (long)offset);

    return __set_errno(result);
}

/*
 * close - Menutup file descriptor
 *
 * Parameter:
 *   fd - File descriptor yang akan ditutup
 *
 * Return: 0 jika berhasil, -1 jika error
 */
int close(int fd) {
    long result;

    /* Validasi file descriptor */
    if (fd < 0) {
        errno = EBADF;
        return -1;
    }

    /* Panggil syscall close */
    result = __syscall1(SYS_close, (long)fd);

    return __set_errno(result);
}

/*
 * dup - Membuat duplikat file descriptor
 *
 * Parameter:
 *   oldfd - File descriptor yang akan diduplikasi
 *
 * Return: File descriptor baru, atau -1 jika error
 *
 * Catatan: Menggunakan fd terendah yang tersedia
 */
int dup(int oldfd) {
    long result;

    /* Validasi file descriptor */
    if (oldfd < 0) {
        errno = EBADF;
        return -1;
    }

    /* Panggil syscall dup */
    result = __syscall1(SYS_dup, (long)oldfd);

    return __set_errno(result);
}

/*
 * dup2 - Membuat duplikat file descriptor ke fd tertentu
 *
 * Parameter:
 *   oldfd - File descriptor sumber
 *   newfd - File descriptor tujuan
 *
 * Return: File descriptor baru, atau -1 jika error
 *
 * Catatan: Jika newfd sudah terbuka, akan ditutup terlebih dahulu
 *          Jika oldfd == newfd, tidak melakukan apa-apa
 */
int dup2(int oldfd, int newfd) {
    long result;

    /* Validasi file descriptor */
    if (oldfd < 0 || newfd < 0) {
        errno = EBADF;
        return -1;
    }

    /* Kasus khusus: oldfd == newfd */
    if (oldfd == newfd) {
        /* Periksa apakah oldfd valid */
        /* syscall dup2 akan handle ini, tapi kita bisa optimasi */
        return newfd;
    }

    /* Panggil syscall dup2 */
    result = __syscall2(SYS_dup2, (long)oldfd, (long)newfd);

    return __set_errno(result);
}
