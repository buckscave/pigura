/*
 * PIGURA LIBC - UNISTD/FILE.C
 * ============================
 * Implementasi fungsi operasi file POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Fungsi yang diimplementasikan:
 * - open()      - Membuka file
 * - creat()     - Membuat file baru
 * - access()    - Mengecek akses file
 * - link()      - Membuat hard link
 * - symlink()   - Membuat symbolic link
 * - readlink()  - Membaca target symlink
 * - truncate()  - Memotong file berdasarkan path
 * - ftruncate() - Memotong file berdasarkan fd
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>

/* ============================================================
 * SYSCALL WRAPPER
 * ============================================================
 * Deklarasi fungsi syscall kernel untuk arsitektur x86_64.
 */

/* Syscall number untuk x86_64 Linux */
#define SYS_open      2
#define SYS_access    21
#define SYS_link      86
#define SYS_symlink   88
#define SYS_readlink  89
#define SYS_truncate  76
#define SYS_ftruncate 77

/* Fungsi wrapper syscall assembly */
extern long __syscall1(long n, long a1);
extern long __syscall2(long n, long a1, long a2);
extern long __syscall3(long n, long a1, long a2, long a3);

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
 * IMPLEMENTASI FUNGSI FILE
 * ============================================================
 */

/*
 * open - Membuka file
 *
 * Parameter:
 *   pathname - Path ke file
 *   flags    - Flag pembukaan (O_RDONLY, O_WRONLY, dll)
 *   mode     - Mode untuk file baru (jika O_CREAT)
 *
 * Return: File descriptor, atau -1 jika error
 */
int open(const char *pathname, int flags, ...) {
    long result;
    mode_t mode = 0;

    /* Validasi parameter */
    if (pathname == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Ambil mode dari variadic argument jika O_CREAT */
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);

        /* Panggil syscall open dengan mode */
        result = __syscall3(SYS_open, (long)pathname,
                            (long)flags, (long)mode);
    } else {
        /* Panggil syscall open tanpa mode */
        result = __syscall2(SYS_open, (long)pathname, (long)flags);
    }

    return __set_errno(result);
}

/*
 * creat - Membuat file baru
 *
 * Parameter:
 *   pathname - Path ke file yang akan dibuat
 *   mode     - Permission mode
 *
 * Return: File descriptor, atau -1 jika error
 *
 * Catatan: Equivalent to open(pathname, O_CREAT|O_WRONLY|O_TRUNC, mode)
 */
int creat(const char *pathname, mode_t mode) {
    long result;

    /* Validasi parameter */
    if (pathname == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Gunakan flags O_CREAT|O_WRONLY|O_TRUNC */
    result = __syscall3(SYS_open, (long)pathname,
                        (long)(O_CREAT | O_WRONLY | O_TRUNC), (long)mode);

    return __set_errno(result);
}

/*
 * access - Mengecek akses file
 *
 * Parameter:
 *   pathname - Path ke file
 *   mode     - Mode akses yang dicek (F_OK, R_OK, W_OK, X_OK)
 *
 * Return: 0 jika akses diizinkan, -1 jika error
 */
int access(const char *pathname, int mode) {
    long result;

    /* Validasi parameter */
    if (pathname == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Validasi mode */
    if (mode < 0 || mode > (F_OK | R_OK | W_OK | X_OK)) {
        errno = EINVAL;
        return -1;
    }

    /* Panggil syscall access */
    result = __syscall2(SYS_access, (long)pathname, (long)mode);

    return __set_errno(result);
}

/*
 * link - Membuat hard link
 *
 * Parameter:
 *   oldpath - Path file yang sudah ada
 *   newpath - Path untuk link baru
 *
 * Return: 0 jika berhasil, -1 jika error
 */
int link(const char *oldpath, const char *newpath) {
    long result;

    /* Validasi parameter */
    if (oldpath == NULL || newpath == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Panggil syscall link */
    result = __syscall2(SYS_link, (long)oldpath, (long)newpath);

    return __set_errno(result);
}

/*
 * symlink - Membuat symbolic link
 *
 * Parameter:
 *   oldpath - Path target (tidak harus ada)
 *   newpath - Path untuk symlink baru
 *
 * Return: 0 jika berhasil, -1 jika error
 */
int symlink(const char *oldpath, const char *newpath) {
    long result;

    /* Validasi parameter */
    if (oldpath == NULL || newpath == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Panggil syscall symlink */
    result = __syscall2(SYS_symlink, (long)oldpath, (long)newpath);

    return __set_errno(result);
}

/*
 * readlink - Membaca target symbolic link
 *
 * Parameter:
 *   pathname - Path ke symbolic link
 *   buf      - Buffer untuk menyimpan target
 *   bufsiz   - Ukuran buffer
 *
 * Return: Jumlah byte yang dibaca, atau -1 jika error
 *
 * Catatan: Tidak menambahkan null terminator
 */
ssize_t readlink(const char *pathname, char *buf, size_t bufsiz) {
    long result;

    /* Validasi parameter */
    if (pathname == NULL || buf == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (bufsiz == 0) {
        errno = EINVAL;
        return -1;
    }

    /* Panggil syscall readlink */
    result = __syscall3(SYS_readlink, (long)pathname,
                        (long)buf, (long)bufsiz);

    return __set_errno(result);
}

/*
 * truncate - Memotong file berdasarkan path
 *
 * Parameter:
 *   path   - Path ke file
 *   length - Panjang baru file
 *
 * Return: 0 jika berhasil, -1 jika error
 *
 * Catatan: Jika length > ukuran file, file diperbesar
 *          dengan byte bernilai 0
 */
int truncate(const char *path, off_t length) {
    long result;

    /* Validasi parameter */
    if (path == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (length < 0) {
        errno = EINVAL;
        return -1;
    }

    /* Panggil syscall truncate */
    result = __syscall2(SYS_truncate, (long)path, (long)length);

    return __set_errno(result);
}

/*
 * ftruncate - Memotong file berdasarkan file descriptor
 *
 * Parameter:
 *   fd     - File descriptor
 *   length - Panjang baru file
 *
 * Return: 0 jika berhasil, -1 jika error
 *
 * Catatan: File harus dibuka dengan mode write
 */
int ftruncate(int fd, off_t length) {
    long result;

    /* Validasi parameter */
    if (fd < 0) {
        errno = EBADF;
        return -1;
    }

    if (length < 0) {
        errno = EINVAL;
        return -1;
    }

    /* Panggil syscall ftruncate */
    result = __syscall2(SYS_ftruncate, (long)fd, (long)length);

    return __set_errno(result);
}
