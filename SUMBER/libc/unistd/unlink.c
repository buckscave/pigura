/*
 * PIGURA LIBC - UNISTD/UNLINK.C
 * ==============================
 * Implementasi fungsi hapus dan rename file POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 2.0 - Disinkronkan dengan syscall Pigura OS
 *
 * Fungsi yang diimplementasikan:
 * - unlink() - Menghapus file (nama file dari direktori)
 * - rename() - Mengubah nama atau memindahkan file
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
 */
static int __set_errno(long result) {
    if (result < 0 && result >= -4095) {
        errno = (int)(-result);
        return -1;
    }
    return (int)result;
}

/* ============================================================
 * IMPLEMENTASI FUNGSI UNLINK
 * ============================================================
 */

/*
 * unlink - Menghapus nama file dari filesystem
 */
int unlink(const char *pathname) {
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

    /* Panggil syscall unlink */
    result = syscall1(SYS_UNLINK, (long)pathname);

    return __set_errno(result);
}

/*
 * rename - Mengubah nama atau memindahkan file
 *
 * Catatan: Implementasi menggunakan link + unlink
 * karena Pigura OS belum memiliki syscall rename terpisah.
 */
int rename(const char *oldpath, const char *newpath) {
    /* Validasi parameter */
    if (oldpath == NULL || newpath == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Validasi path kosong */
    if (oldpath[0] == '\0' || newpath[0] == '\0') {
        errno = ENOENT;
        return -1;
    }

    /* 
     * Pigura OS: Implementasi menggunakan link + unlink
     * TODO: Tambahkan syscall rename di kernel untuk operasi atomic
     */
    
    /* 1. Buat link baru */
    long result = syscall2(SYS_LINK, (long)oldpath, (long)newpath);
    if (result < 0) {
        __set_errno(result);
        return -1;
    }

    /* 2. Hapus link lama */
    result = syscall1(SYS_UNLINK, (long)oldpath);
    if (result < 0) {
        /* Rollback: hapus link baru */
        syscall1(SYS_UNLINK, (long)newpath);
        __set_errno(result);
        return -1;
    }

    return 0;
}
