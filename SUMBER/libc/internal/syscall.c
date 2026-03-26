/*
 * PIGURA LIBC - SYSCALL.C
 * ========================
 * Implementasi wrapper syscall variadic dan helper functions.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 2.0 - Disinkronkan dengan syscall Pigura OS
 *
 * File ini menyediakan fungsi syscall() variadic yang
 * memanggil syscallN() yang sesuai berdasarkan jumlah argumen.
 */

#include <sys/syscall.h>
#include <stdarg.h>
#include <errno.h>

/* ============================================================
 * HELPER FUNCTIONS
 * ============================================================
 */

/*
 * __set_errno_from_syscall - Set errno dari hasil syscall
 *
 * Parameter:
 *   result - Hasil syscall (negatif = error)
 *
 * Return: -1 jika error, result jika sukses
 */
int __set_errno_from_syscall(long result) {
    if (result < 0) {
        errno = (int)(-result);
        return -1;
    }
    return (int)result;
}

/* ============================================================
 * SYSCALL VARIADIC WRAPPER
 * ============================================================
 */

/*
 * syscall - Variadic syscall wrapper
 *
 * Parameter:
 *   num - Nomor syscall
 *   ... - Argumen syscall (maksimal 6)
 *
 * Return: Nilai kembalian syscall atau -1 pada error
 *
 * CATATAN PENTING:
 *   Fungsi ini diasumsikan dipanggil dengan jumlah argumen
 *   yang benar untuk syscall yang bersangkutan.
 *   Argumen tambahan akan diabaikan.
 */
long syscall(long num, ...) {
    va_list args;
    long a1, a2, a3, a4, a5, a6;
    long ret;

    va_start(args, num);

    /* Ambil semua argumen potensial */
    a1 = va_arg(args, long);
    a2 = va_arg(args, long);
    a3 = va_arg(args, long);
    a4 = va_arg(args, long);
    a5 = va_arg(args, long);
    a6 = va_arg(args, long);

    va_end(args);

    /*
     * Panggil syscall6 dengan semua argumen.
     * Syscall yang membutuhkan lebih sedikit argumen akan
     * mengabaikan argumen tambahan di register yang tidak digunakan.
     *
     * Ini aman karena kernel hanya membaca register yang
     * diperlukan untuk syscall tertentu.
     */
    ret = syscall6(num, a1, a2, a3, a4, a5, a6);

    return ret;
}
