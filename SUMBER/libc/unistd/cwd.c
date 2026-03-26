/*
 * PIGURA LIBC - UNISTD/CWD.C
 * ===========================
 * Implementasi fungsi direktori kerja POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 2.0 - Disinkronkan dengan syscall Pigura OS
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
#include <sys/syscall.h>   /* Syscall numbers dari header terpusat */
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

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
 * IMPLEMENTASI FUNGSI DIREKTORI
 * ============================================================
 */

/*
 * chdir - Mengubah direktori kerja saat ini
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
    result = syscall1(SYS_CHDIR, (long)path);

    return __set_errno(result);
}

/*
 * fchdir - Mengubah direktori kerja via file descriptor
 *
 * Catatan: Pigura OS belum mendukung fchdir, gunakan chdir
 */
int fchdir(int fd) {
    /* Pigura OS: belum implementasi fchdir */
    (void)fd;
    errno = ENOSYS;
    return -1;
}

/*
 * getcwd - Mendapatkan path direktori kerja saat ini
 */
char *getcwd(char *buf, size_t size) {
    long result;
    size_t allocated = 0;

    /* Jika buf NULL, alokasikan buffer */
    if (buf == NULL) {
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
    result = syscall2(SYS_GETCWD, (long)buf, (long)size);

    /* Handle error */
    if (result < 0) {
        if (allocated) {
            free(buf);
        }
        __set_errno(result);
        return NULL;
    }

    return buf;
}

/*
 * mkdir - Membuat direktori baru
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
    result = syscall2(SYS_MKDIR, (long)pathname, (long)mode);

    return __set_errno(result);
}

/*
 * rmdir - Menghapus direktori kosong
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
    result = syscall1(SYS_RMDIR, (long)pathname);

    return __set_errno(result);
}
