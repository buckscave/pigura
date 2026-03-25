/*
 * PIGURA LIBC - UNISTD/CWD.C
 * ===========================
 * Implementasi fungsi direktori kerja POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Fungsi yang diimplementasikan:
 * - chdir()  - Mengubah direktori kerja
 * - fchdir() - Mengubah direktori kerja via fd
 * - getcwd() - Mendapatkan direktori kerja saat ini
 * - mkdir()  - Membuat direktori baru
 * - rmdir()  - Menghapus direktori kosong
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================
 * SYSCALL WRAPPER
 * ============================================================
 * Deklarasi fungsi syscall kernel untuk arsitektur x86_64.
 */

/* Syscall number untuk x86_64 Linux */
#define SYS_chdir   80
#define SYS_fchdir  81
#define SYS_getcwd  79
#define SYS_mkdir   83
#define SYS_rmdir   84

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
 * IMPLEMENTASI FUNGSI DIREKTORI
 * ============================================================
 */

/*
 * chdir - Mengubah direktori kerja saat ini
 *
 * Parameter:
 *   path - Path ke direktori baru
 *
 * Return: 0 jika berhasil, -1 jika error
 */
int chdir(const char *path) {
    long result;

    /* Validasi parameter */
    if (path == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Validasi path kosong */
    if (path[0] == '\0') {
        errno = ENOENT;
        return -1;
    }

    /* Panggil syscall chdir */
    result = __syscall1(SYS_chdir, (long)path);

    return __set_errno(result);
}

/*
 * fchdir - Mengubah direktori kerja via file descriptor
 *
 * Parameter:
 *   fd - File descriptor yang menunjuk ke direktori
 *
 * Return: 0 jika berhasil, -1 jika error
 */
int fchdir(int fd) {
    long result;

    /* Validasi file descriptor */
    if (fd < 0) {
        errno = EBADF;
        return -1;
    }

    /* Panggil syscall fchdir */
    result = __syscall1(SYS_fchdir, (long)fd);

    return __set_errno(result);
}

/*
 * getcwd - Mendapatkan path direktori kerja saat ini
 *
 * Parameter:
 *   buf  - Buffer untuk menyimpan path (boleh NULL)
 *   size - Ukuran buffer
 *
 * Return: Pointer ke path, atau NULL jika error
 *
 * Catatan: Jika buf NULL, akan mengalokasikan memori
 *          secara dinamis dengan malloc()
 */
char *getcwd(char *buf, size_t size) {
    long result;
    char *path;
    size_t allocated = 0;

    /* Jika buf NULL, alokasikan buffer */
    if (buf == NULL) {
        /* Ukuran default untuk path */
        if (size == 0) {
            size = 4096;  /* PATH_MAX */
        }
        buf = (char *)malloc(size);
        if (buf == NULL) {
            errno = ENOMEM;
            return NULL;
        }
        allocated = 1;
    }

    /* Validasi ukuran buffer */
    if (size == 0) {
        errno = EINVAL;
        if (allocated) {
            free(buf);
        }
        return NULL;
    }

    /* Panggil syscall getcwd */
    result = __syscall2(SYS_getcwd, (long)buf, (long)size);

    /* Handle error */
    if (result < 0) {
        if (allocated) {
            free(buf);
        }
        __set_errno(result);
        return NULL;
    }

    /* Handle buffer terlalu kecil */
    if (result == 0) {
        errno = ERANGE;
        if (allocated) {
            free(buf);
        }
        return NULL;
    }

    return buf;
}

/*
 * mkdir - Membuat direktori baru
 *
 * Parameter:
 *   pathname - Path untuk direktori baru
 *   mode     - Permission mode untuk direktori
 *
 * Return: 0 jika berhasil, -1 jika error
 */
int mkdir(const char *pathname, mode_t mode) {
    long result;

    /* Validasi parameter */
    if (pathname == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Validasi path kosong */
    if (pathname[0] == '\0') {
        errno = ENOENT;
        return -1;
    }

    /* Panggil syscall mkdir */
    result = __syscall2(SYS_mkdir, (long)pathname, (long)mode);

    return __set_errno(result);
}

/*
 * rmdir - Menghapus direktori kosong
 *
 * Parameter:
 *   pathname - Path ke direktori yang akan dihapus
 *
 * Return: 0 jika berhasil, -1 jika error
 *
 * Catatan: Direktori harus kosong untuk bisa dihapus
 */
int rmdir(const char *pathname) {
    long result;

    /* Validasi parameter */
    if (pathname == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Validasi path kosong */
    if (pathname[0] == '\0') {
        errno = ENOENT;
        return -1;
    }

    /* Panggil syscall rmdir */
    result = __syscall1(SYS_rmdir, (long)pathname);

    return __set_errno(result);
}
