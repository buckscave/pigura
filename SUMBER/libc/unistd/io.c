/*
 * PIGURA LIBC - UNISTD/IO.C
 * ==========================
 * Implementasi fungsi I/O standar POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 2.0 - Disinkronkan dengan syscall Pigura OS
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
#include <sys/syscall.h>   /* Syscall numbers dari header terpusat */
#include <errno.h>
#include <unistd.h>

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
    result = syscall3(SYS_READ, (long)fd, (long)buf, (long)count);

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
    result = syscall3(SYS_WRITE, (long)fd, (long)buf, (long)count);

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

    /* Implementasi dengan lseek + read + lseek (simple) */
    /* Catatan: Kernel Pigura perlu syscall pread terpisah untuk efisiensi */
    off_t old_offset = lseek(fd, 0, SEEK_CUR);
    if (old_offset < 0) {
        return -1;
    }
    
    if (lseek(fd, offset, SEEK_SET) < 0) {
        return -1;
    }
    
    result = syscall3(SYS_READ, (long)fd, (long)buf, (long)count);
    ssize_t bytes_read = __set_errno(result);
    
    /* Kembalikan offset ke posisi semula */
    lseek(fd, old_offset, SEEK_SET);
    
    return bytes_read;
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

    /* Implementasi dengan lseek + write + lseek (simple) */
    off_t old_offset = lseek(fd, 0, SEEK_CUR);
    if (old_offset < 0) {
        return -1;
    }
    
    if (lseek(fd, offset, SEEK_SET) < 0) {
        return -1;
    }
    
    result = syscall3(SYS_WRITE, (long)fd, (long)buf, (long)count);
    ssize_t bytes_written = __set_errno(result);
    
    /* Kembalikan offset ke posisi semula */
    lseek(fd, old_offset, SEEK_SET);
    
    return bytes_written;
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
    result = syscall1(SYS_CLOSE, (long)fd);

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
    result = syscall1(SYS_DUP, (long)oldfd);

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
        return newfd;
    }

    /* Panggil syscall dup2 */
    result = syscall2(SYS_DUP2, (long)oldfd, (long)newfd);

    return __set_errno(result);
}
