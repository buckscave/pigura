/*
 * PIGURA LIBC - UNISTD/UNLINK.C
 * ==============================
 * Implementasi fungsi hapus dan rename file POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Fungsi yang diimplementasikan:
 * - unlink() - Menghapus file (nama file dari direktori)
 * - rename() - Mengubah nama atau memindahkan file
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
#define SYS_unlink  87
#define SYS_rename  82

/* Fungsi wrapper syscall assembly */
extern long __syscall1(long n, long a1);
extern long __syscall2(long n, long a1, long a2);

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
 * IMPLEMENTASI FUNGSI UNLINK
 * ============================================================
 */

/*
 * unlink - Menghapus nama file dari filesystem
 *
 * Parameter:
 *   pathname - Path ke file yang akan dihapus
 *
 * Return: 0 jika berhasil, -1 jika error
 *
 * Catatan:
 * - File sebenarnya dihapus ketika semua referensi ditutup
 * - Jika file adalah symlink, symlink yang dihapus
 * - Jika file adalah socket/FIFO, nama dihapus tapi
 *   proses yang masih membuka tetap bisa menggunakannya
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
    result = __syscall1(SYS_unlink, (long)pathname);

    return __set_errno(result);
}

/*
 * rename - Mengubah nama atau memindahkan file
 *
 * Parameter:
 *   oldpath - Path file sumber
 *   newpath - Path file tujuan
 *
 * Return: 0 jika berhasil, -1 jika error
 *
 * Catatan:
 * - Jika newpath sudah ada, akan ditimpa (atomic operation)
 * - Bisa memindahkan file antar filesystem (tidak atomic)
 * - Tidak mengikuti symlink untuk oldpath
 * - Jika oldpath adalah symlink, symlink yang direname
 */
int rename(const char *oldpath, const char *newpath) {
    long result;

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

    /* Panggil syscall rename */
    result = __syscall2(SYS_rename, (long)oldpath, (long)newpath);

    return __set_errno(result);
}
