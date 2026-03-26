/*
 * PIGURA LIBC - STDIO/REMOVE.C
 * ==============================
 * Implementasi fungsi remove dan rename.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 2.0 - Disinkronkan dengan syscall Pigura OS
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/syscall.h>

/* ============================================================
 * FORWARD DECLARATIONS
 * ============================================================
 */
int _sys_rename(const char *old, const char *new);

/* ============================================================
 * REMOVE
 * ============================================================
 * Hapus file atau direktori kosong.
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
 */
int _sys_rename(const char *old, const char *new) {
    /* Implementasi userspace menggunakan link + unlink */

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
 */
int link(const char *oldpath, const char *newpath) {
    long result;

    /* Validasi parameter */
    if (oldpath == NULL || *oldpath == '\0') {
        errno = EINVAL;
        return -1;
    }

    if (newpath == NULL || *newpath == '\0') {
        errno = EINVAL;
        return -1;
    }

    /* Gunakan syscall link - SYS_LINK dari syscall.h */
    result = syscall2(SYS_LINK, (long)oldpath, (long)newpath);

    if (result < 0) {
        errno = (int)(-result);
        return -1;
    }
    return (int)result;
}

/* ============================================================
 * UNLINK (POSIX)
 * ============================================================
 * Hapus nama file (link).
 */
int unlink(const char *pathname) {
    long result;

    /* Validasi parameter */
    if (pathname == NULL || *pathname == '\0') {
        errno = EINVAL;
        return -1;
    }

    /* Gunakan syscall unlink - SYS_UNLINK dari syscall.h */
    result = syscall1(SYS_UNLINK, (long)pathname);

    if (result < 0) {
        errno = (int)(-result);
        return -1;
    }
    return (int)result;
}

/* ============================================================
 * SYMLINK (POSIX)
 * ============================================================
 * Buat symbolic link.
 */
int symlink(const char *target, const char *linkpath) {
    long result;

    /* Validasi parameter */
    if (target == NULL || *target == '\0') {
        errno = EINVAL;
        return -1;
    }

    if (linkpath == NULL || *linkpath == '\0') {
        errno = EINVAL;
        return -1;
    }

    /* Gunakan syscall symlink - SYS_SYMLINK dari syscall.h */
    result = syscall2(SYS_SYMLINK, (long)target, (long)linkpath);

    if (result < 0) {
        errno = (int)(-result);
        return -1;
    }
    return (int)result;
}

/* ============================================================
 * READLINK (POSIX)
 * ============================================================
 * Baca target symbolic link.
 */
ssize_t readlink(const char *pathname, char *buf, size_t bufsiz) {
    long result;

    /* Validasi parameter */
    if (pathname == NULL || *pathname == '\0') {
        errno = EINVAL;
        return -1;
    }

    if (buf == NULL || bufsiz == 0) {
        errno = EINVAL;
        return -1;
    }

    /* Gunakan syscall readlink - SYS_READLINK dari syscall.h */
    result = syscall3(SYS_READLINK, (long)pathname, (long)buf, (long)bufsiz);

    if (result < 0) {
        errno = (int)(-result);
        return -1;
    }
    return (ssize_t)result;
}

/* ============================================================
 * RMDIR (POSIX)
 * ============================================================
 * Hapus direktori kosong.
 */
int rmdir(const char *pathname) {
    long result;

    /* Validasi parameter */
    if (pathname == NULL || *pathname == '\0') {
        errno = EINVAL;
        return -1;
    }

    /* Gunakan syscall rmdir - SYS_RMDIR dari syscall.h */
    result = syscall1(SYS_RMDIR, (long)pathname);

    if (result < 0) {
        errno = (int)(-result);
        return -1;
    }
    return (int)result;
}

/* ============================================================
 * MKDIR (POSIX)
 * ============================================================
 * Buat direktori.
 */
int mkdir(const char *pathname, mode_t mode) {
    long result;

    /* Validasi parameter */
    if (pathname == NULL || *pathname == '\0') {
        errno = EINVAL;
        return -1;
    }

    /* Gunakan syscall mkdir - SYS_MKDIR dari syscall.h */
    result = syscall2(SYS_MKDIR, (long)pathname, (long)mode);

    if (result < 0) {
        errno = (int)(-result);
        return -1;
    }
    return (int)result;
}

/* ============================================================
 * ACCESS (POSIX)
 * ============================================================
 * Cek akses file.
 */
int access(const char *pathname, int mode) {
    long result;

    /* Validasi parameter */
    if (pathname == NULL || *pathname == '\0') {
        errno = EINVAL;
        return -1;
    }

    /* Gunakan syscall access - SYS_ACCESS dari syscall.h */
    result = syscall2(SYS_ACCESS, (long)pathname, (long)mode);

    if (result < 0) {
        errno = (int)(-result);
        return -1;
    }
    return (int)result;
}

/* ============================================================
 * STAT (POSIX)
 * ============================================================
 * Dapatkan informasi file.
 */
int stat(const char *pathname, struct stat *statbuf) {
    long result;

    /* Validasi parameter */
    if (pathname == NULL || *pathname == '\0') {
        errno = EINVAL;
        return -1;
    }

    if (statbuf == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Gunakan syscall stat - SYS_STAT dari syscall.h */
    result = syscall2(SYS_STAT, (long)pathname, (long)statbuf);

    if (result < 0) {
        errno = (int)(-result);
        return -1;
    }
    return (int)result;
}
