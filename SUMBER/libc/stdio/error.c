/*
 * PIGURA LIBC - STDIO/ERROR.C
 * =============================
 * Implementasi fungsi clearerr, feof, ferror, perror.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/* ============================================================
 * CLEARERR
 * ============================================================
 * Reset error dan EOF flag.
 *
 * Parameter:
 *   stream - Stream yang di-reset
 */
void clearerr(FILE *stream) {
    /* Validasi parameter */
    if (stream == NULL) {
        return;
    }

    /* Reset flags */
    stream->flags &= ~(F_EOF | F_ERR);
}

/* ============================================================
 * FEOF
 * ============================================================
 * Cek apakah EOF tercapai.
 *
 * Parameter:
 *   stream - Stream yang diperiksa
 *
 * Return: Non-zero jika EOF
 */
int feof(FILE *stream) {
    /* Validasi parameter */
    if (stream == NULL) {
        return 0;
    }

    return (stream->flags & F_EOF) ? 1 : 0;
}

/* ============================================================
 * FERROR
 * ============================================================
 * Cek apakah error terjadi.
 *
 * Parameter:
 *   stream - Stream yang diperiksa
 *
 * Return: Non-zero jika error
 */
int ferror(FILE *stream) {
    /* Validasi parameter */
    if (stream == NULL) {
        return 0;
    }

    return (stream->flags & F_ERR) ? 1 : 0;
}

/* ============================================================
 * PERROR
 * ============================================================
 * Cetak pesan error ke stderr.
 *
 * Parameter:
 *   s - Pesan tambahan (boleh NULL)
 */
void perror(const char *s) {
    const char *err_msg;

    /* Dapatkan pesan error dari errno */
    err_msg = strerror(errno);

    /* Cetak ke stderr */
    if (s != NULL && *s != '\0') {
        fprintf(stderr, "%s: %s\n", s, err_msg);
    } else {
        fprintf(stderr, "%s\n", err_msg);
    }

    /* Flush stderr */
    fflush(stderr);
}

/* ============================================================
 * __STDIO_ERROR_STR (Internal)
 * ============================================================
 * Dapatkan string deskripsi error.
 *
 * Parameter:
 *   errnum - Kode error
 *
 * Return: String deskripsi error
 */
const char *__stdio_error_str(int errnum) {
    static char unknown_buf[32];

    switch (errnum) {
        case 0:
            return "Success";
        case EDOM:
            return "Argument out of domain";
        case ERANGE:
            return "Result out of range";
        case EILSEQ:
            return "Illegal byte sequence";
        case ENOENT:
            return "No such file or directory";
        case ESRCH:
            return "No such process";
        case EINTR:
            return "Interrupted system call";
        case EIO:
            return "I/O error";
        case ENXIO:
            return "No such device or address";
        case E2BIG:
            return "Argument list too long";
        case ENOEXEC:
            return "Exec format error";
        case EBADF:
            return "Bad file descriptor";
        case ECHILD:
            return "No child processes";
        case EAGAIN:
            return "Resource temporarily unavailable";
        case ENOMEM:
            return "Out of memory";
        case EACCES:
            return "Permission denied";
        case EFAULT:
            return "Bad address";
        case EEXIST:
            return "File exists";
        case EXDEV:
            return "Cross-device link";
        case ENODEV:
            return "No such device";
        case ENOTDIR:
            return "Not a directory";
        case EISDIR:
            return "Is a directory";
        case EINVAL:
            return "Invalid argument";
        case ENFILE:
            return "File table overflow";
        case EMFILE:
            return "Too many open files";
        case ENOTTY:
            return "Not a typewriter";
        case EFBIG:
            return "File too large";
        case ENOSPC:
            return "No space left on device";
        case ESPIPE:
            return "Illegal seek";
        case EROFS:
            return "Read-only file system";
        case EMLINK:
            return "Too many links";
        case EPIPE:
            return "Broken pipe";
        case ELOOP:
            return "Too many symbolic links";
        case ENAMETOOLONG:
            return "File name too long";
        case ENOTEMPTY:
            return "Directory not empty";
        case ENOSYS:
            return "Function not implemented";
        default:
            /* Buat pesan unknown */
            sprintf(unknown_buf, "Unknown error %d", errnum);
            return unknown_buf;
    }
}

/* ============================================================
 * FILNO
 * ============================================================
 * Dapatkan file descriptor dari stream.
 *
 * Parameter:
 *   stream - Stream yang diperiksa
 *
 * Return: File descriptor, atau -1 jika error
 */
int fileno(FILE *stream) {
    if (stream == NULL) {
        errno = EBADF;
        return -1;
    }

    if (stream->fd < 0) {
        errno = EBADF;
        return -1;
    }

    return stream->fd;
}
