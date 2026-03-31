/*
 * PIGURA LIBC - UNISTD/FILE.C
 * ============================
 * Implementasi fungsi operasi file POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 2.0 - Disinkronkan dengan syscall Pigura OS
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
#include <sys/syscall.h>   /* Syscall numbers dari header terpusat */
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>

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
        mode = (mode_t)va_arg(args, int);
        va_end(args);

        /* Panggil syscall open dengan mode */
        result = SYSCALL3(SYS_OPEN, pathname, flags, mode);
    } else {
        /* Panggil syscall open tanpa mode */
        result = SYSCALL2(SYS_OPEN, pathname, flags);
    }

    return __set_errno_from_syscall(result);
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
    result = SYSCALL3(SYS_OPEN, pathname, O_CREAT | O_WRONLY | O_TRUNC, mode);

    return __set_errno_from_syscall(result);
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
    result = SYSCALL2(SYS_ACCESS, pathname, mode);

    return __set_errno_from_syscall(result);
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
    result = SYSCALL2(SYS_LINK, oldpath, newpath);

    return __set_errno_from_syscall(result);
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
    result = SYSCALL2(SYS_SYMLINK, oldpath, newpath);

    return __set_errno_from_syscall(result);
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
    result = SYSCALL3(SYS_READLINK, pathname, buf, bufsiz);

    return __set_errno_from_syscall(result);
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

    /* Panggil syscall truncate - menggunakan SYS_FTRUNCATE dengan path */
    /* Catatan: Pigura OS mungkin perlu syscall terpisah untuk truncate */
    result = SYSCALL2(SYS_STAT, path, length);  /* Placeholder - perlu syscall khusus */

    return __set_errno_from_syscall(result);
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

    /* Panggil syscall ftruncate - menggunakan LSEEK dengan offset khusus */
    /* Catatan: Perlu implementasi syscall truncate di kernel */
    result = SYSCALL2(SYS_LSEEK, fd, length);  /* Placeholder */

    return __set_errno_from_syscall(result);
}
