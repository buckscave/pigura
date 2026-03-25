/*
 * PIGURA LIBC - STDIO/REMOVE.C
 * ==============================
 * Implementasi fungsi remove dan rename.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

/* ============================================================
 * REMOVE
 * ============================================================
 * Hapus file atau direktori kosong.
 *
 * Parameter:
 *   filename - Nama file yang dihapus
 *
 * Return: 0 jika berhasil, non-zero jika gagal
 */
int remove(const char *filename) {
    struct stat st;

    /* Validasi parameter */
    if (filename == NULL || *filename == '\0') {
        errno = EINVAL;
        return -1;
    }

    /* Cek apakah file ada dan tipenya */
    if (stat(filename, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            /* Gunakan rmdir untuk direktori */
            return rmdir(filename);
        }
    }

    /* Gunakan unlink untuk file */
    return unlink(filename);
}

/* ============================================================
 * RENAME
 * ============================================================
 * Ubah nama file atau pindahkan file.
 *
 * Parameter:
 *   old - Nama lama
 *   new - Nama baru
 *
 * Return: 0 jika berhasil, non-zero jika gagal
 */
int rename(const char *old, const char *new) {
    /* Validasi parameter */
    if (old == NULL || *old == '\0') {
        errno = EINVAL;
        return -1;
    }

    if (new == NULL || *new == '\0') {
        errno = EINVAL;
        return -1;
    }

    /* Gunakan syscall rename */
    return _sys_rename(old, new);
}

/* ============================================================
 * _SYS_RENAME (Internal)
 * ============================================================
 * Wrapper untuk syscall rename.
 *
 * Parameter:
 *   old - Nama lama
 *   new - Nama baru
 *
 * Return: 0 jika berhasil, -1 jika gagal
 */
int _sys_rename(const char *old, const char *new) {
    /* Syscall number untuk rename bervariasi per arsitektur */
    /* Untuk sementara, gunakan implementasi userspace */

    /* Link file baru */
    if (link(old, new) != 0) {
        return -1;
    }

    /* Unlink file lama */
    if (unlink(old) != 0) {
        /* Jika gagal, coba cleanup */
        int saved_errno = errno;
        unlink(new);
        errno = saved_errno;
        return -1;
    }

    return 0;
}

/* ============================================================
 * LINK (POSIX)
 * ============================================================
 * Buat hard link.
 *
 * Parameter:
 *   oldpath - Path file asli
 *   newpath - Path link baru
 *
 * Return: 0 jika berhasil, -1 jika gagal
 */
int link(const char *oldpath, const char *newpath) {
    /* Validasi parameter */
    if (oldpath == NULL || *oldpath == '\0') {
        errno = EINVAL;
        return -1;
    }

    if (newpath == NULL || *newpath == '\0') {
        errno = EINVAL;
        return -1;
    }

    /* Gunakan syscall link */
    return syscall(SYS_link, oldpath, newpath);
}

/* ============================================================
 * UNLINK (POSIX)
 * ============================================================
 * Hapus nama file (link).
 *
 * Parameter:
 *   pathname - Nama file yang dihapus
 *
 * Return: 0 jika berhasil, -1 jika gagal
 */
int unlink(const char *pathname) {
    /* Validasi parameter */
    if (pathname == NULL || *pathname == '\0') {
        errno = EINVAL;
        return -1;
    }

    /* Gunakan syscall unlink */
    return syscall(SYS_unlink, pathname);
}

/* ============================================================
 * SYMLINK (POSIX)
 * ============================================================
 * Buat symbolic link.
 *
 * Parameter:
 *   target   - Target link
 *   linkpath - Path link baru
 *
 * Return: 0 jika berhasil, -1 jika gagal
 */
int symlink(const char *target, const char *linkpath) {
    /* Validasi parameter */
    if (target == NULL || *target == '\0') {
        errno = EINVAL;
        return -1;
    }

    if (linkpath == NULL || *linkpath == '\0') {
        errno = EINVAL;
        return -1;
    }

    /* Gunakan syscall symlink */
    return syscall(SYS_symlink, target, linkpath);
}

/* ============================================================
 * READLINK (POSIX)
 * ============================================================
 * Baca target symbolic link.
 *
 * Parameter:
 *   pathname - Path link
 *   buf      - Buffer tujuan
 *   bufsiz   - Ukuran buffer
 *
 * Return: Jumlah byte yang dibaca, atau -1 jika gagal
 */
ssize_t readlink(const char *pathname, char *buf, size_t bufsiz) {
    /* Validasi parameter */
    if (pathname == NULL || *pathname == '\0') {
        errno = EINVAL;
        return -1;
    }

    if (buf == NULL || bufsiz == 0) {
        errno = EINVAL;
        return -1;
    }

    /* Gunakan syscall readlink */
    return syscall(SYS_readlink, pathname, buf, bufsiz);
}

/* ============================================================
 * RMDIR (POSIX)
 * ============================================================
 * Hapus direktori kosong.
 *
 * Parameter:
 *   pathname - Nama direktori
 *
 * Return: 0 jika berhasil, -1 jika gagal
 */
int rmdir(const char *pathname) {
    /* Validasi parameter */
    if (pathname == NULL || *pathname == '\0') {
        errno = EINVAL;
        return -1;
    }

    /* Gunakan syscall rmdir */
    return syscall(SYS_rmdir, pathname);
}

/* ============================================================
 * MKDIR (POSIX)
 * ============================================================
 * Buat direktori.
 *
 * Parameter:
 *   pathname - Nama direktori
 *   mode     - Mode permission
 *
 * Return: 0 jika berhasil, -1 jika gagal
 */
int mkdir(const char *pathname, mode_t mode) {
    /* Validasi parameter */
    if (pathname == NULL || *pathname == '\0') {
        errno = EINVAL;
        return -1;
    }

    /* Gunakan syscall mkdir */
    return syscall(SYS_mkdir, pathname, mode);
}

/* ============================================================
 * ACCESS (POSIX)
 * ============================================================
 * Cek akses file.
 *
 * Parameter:
 *   pathname - Nama file
 *   mode     - Mode akses (F_OK, R_OK, W_OK, X_OK)
 *
 * Return: 0 jika diizinkan, -1 jika tidak
 */
int access(const char *pathname, int mode) {
    /* Validasi parameter */
    if (pathname == NULL || *pathname == '\0') {
        errno = EINVAL;
        return -1;
    }

    /* Gunakan syscall access */
    return syscall(SYS_access, pathname, mode);
}

/* ============================================================
 * STAT (POSIX)
 * ============================================================
 * Dapatkan informasi file.
 *
 * Parameter:
 *   pathname - Nama file
 *   statbuf  - Buffer untuk menyimpan informasi
 *
 * Return: 0 jika berhasil, -1 jika gagal
 */
int stat(const char *pathname, struct stat *statbuf) {
    /* Validasi parameter */
    if (pathname == NULL || *pathname == '\0') {
        errno = EINVAL;
        return -1;
    }

    if (statbuf == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Gunakan syscall stat */
    return syscall(SYS_stat, pathname, statbuf);
}

/* ============================================================
 * SYSCALL DEFINITIONS
 * ============================================================
 * Syscall numbers bervariasi per arsitektur.
 * Definisi ini adalah placeholder.
 */
#ifndef SYS_link
#define SYS_link        9
#endif

#ifndef SYS_unlink
#define SYS_unlink      10
#endif

#ifndef SYS_symlink
#define SYS_symlink     83
#endif

#ifndef SYS_readlink
#define SYS_readlink    85
#endif

#ifndef SYS_rmdir
#define SYS_rmdir       40
#endif

#ifndef SYS_mkdir
#define SYS_mkdir       39
#endif

#ifndef SYS_access
#define SYS_access      33
#endif

#ifndef SYS_stat
#define SYS_stat        4
#endif

/* Syscall wrapper */
extern long syscall(long number, ...);
