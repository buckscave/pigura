/*
 * PIGURA LIBC - STRING/STRERROR.C
 * ===============================
 * Implementasi fungsi konversi error number ke string.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Fungsi yang diimplementasikan:
 *   - strerror()   : Konversi errno ke string
 *   - strerror_r() : Konversi errno ke string (reentrant)
 */

#include <string.h>
#include <errno.h>

/* ============================================================
 * TABEL PESAN ERROR
 * ============================================================
 * Array struktur untuk mapping error number ke pesan.
 * Urutan tidak perlu terurut karena pencarian linear.
 */

typedef struct {
    int errnum;
    const char *msg;
} errormsg_t;

/* Tabel pesan error standar POSIX/C89 */
static const errormsg_t error_messages[] = {
    {0,      "Success"},
    {EPERM,  "Operation not permitted"},
    {ENOENT, "No such file or directory"},
    {ESRCH,  "No such process"},
    {EINTR,  "Interrupted system call"},
    {EIO,    "I/O error"},
    {ENXIO,  "No such device or address"},
    {E2BIG,  "Argument list too long"},
    {ENOEXEC,"Exec format error"},
    {EBADF,  "Bad file descriptor"},
    {ECHILD, "No child processes"},
    {EAGAIN, "Resource temporarily unavailable"},
    {ENOMEM, "Cannot allocate memory"},
    {EACCES, "Permission denied"},
    {EFAULT, "Bad address"},
    {ENOTBLK,"Block device required"},
    {EBUSY,  "Device or resource busy"},
    {EEXIST, "File exists"},
    {EXDEV,  "Cross-device link"},
    {ENODEV, "No such device"},
    {ENOTDIR,"Not a directory"},
    {EISDIR, "Is a directory"},
    {EINVAL, "Invalid argument"},
    {ENFILE, "File table overflow"},
    {EMFILE, "Too many open files"},
    {ENOTTY, "Not a typewriter"},
    {ETXTBSY,"Text file busy"},
    {EFBIG,  "File too large"},
    {ENOSPC, "No space left on device"},
    {ESPIPE, "Illegal seek"},
    {EROFS,  "Read-only file system"},
    {EMLINK, "Too many links"},
    {EPIPE,  "Broken pipe"},
    {EDOM,   "Numerical argument out of domain"},
    {ERANGE, "Numerical result out of range"},
    {EDEADLK,"Resource deadlock avoided"},
    {ENAMETOOLONG, "File name too long"},
    {ENOLCK, "No locks available"},
    {ENOSYS, "Function not implemented"},
    {ENOTEMPTY, "Directory not empty"},
    {ELOOP,  "Too many symbolic links encountered"},
    {EWOULDBLOCK, "Resource temporarily unavailable"},
    {ENOMSG, "No message of desired type"},
    {EIDRM,  "Identifier removed"},
    {ECHRNG, "Channel number out of range"},
    {EL2NSYNC, "Level 2 not synchronized"},
    {EL3HLT, "Level 3 halted"},
    {EL3RST, "Level 3 reset"},
    {ELNRNG, "Link number out of range"},
    {EUNATCH, "Protocol driver not attached"},
    {ENOCSI, "No CSI structure available"},
    {EL2HLT, "Level 2 halted"},
    {EBADE,  "Invalid exchange"},
    {EBADR,  "Invalid request descriptor"},
    {EXFULL, "Exchange full"},
    {ENOANO, "No anode"},
    {EBADRQC, "Invalid request code"},
    {EBADSLT, "Invalid slot"},
    {EDEADLOCK, "Resource deadlock avoided"},
    {EBFONT, "Bad font file format"},
    {ENOSTR, "Device not a stream"},
    {ENODATA, "No data available"},
    {ETIME,  "Timer expired"},
    {ENOSR,  "Out of streams resources"},
    {ENONET, "Machine is not on the network"},
    {ENOPKG, "Package not installed"},
    {EREMOTE, "Object is remote"},
    {ENOLINK, "Link has been severed"},
    {EADV,   "Advertise error"},
    {ESRMNT, "Srmount error"},
    {ECOMM,  "Communication error on send"},
    {EPROTO, "Protocol error"},
    {EMULTIHOP, "Multihop attempted"},
    {EDOTDOT, "RFS specific error"},
    {EBADMSG, "Bad message"},
    {EOVERFLOW, "Value too large for data type"},
    {ENOTUNIQ, "Name not unique on network"},
    {EBADFD, "File descriptor in bad state"},
    {EREMCHG, "Remote address changed"},
    {ELIBACC, "Cannot access shared library"},
    {ELIBBAD, "Accessing a corrupted shared library"},
    {ELIBSCN, ".lib section in a.out corrupted"},
    {ELIBMAX, "Too many shared libraries"},
    {ELIBEXEC, "Cannot exec a shared library directly"},
    {EILSEQ, "Invalid byte sequence"},
    {ERESTART, "Interrupted syscall should be restarted"},
    {ESTRPIPE, "Streams pipe error"},
    {EUSERS, "Too many users"},
    {ENOTSOCK, "Socket operation on non-socket"},
    {EDESTADDRREQ, "Destination address required"},
    {EMSGSIZE, "Message too long"},
    {EPROTOTYPE, "Protocol wrong type for socket"},
    {ENOPROTOOPT, "Protocol not available"},
    {EPROTONOSUPPORT, "Protocol not supported"},
    {ESOCKTNOSUPPORT, "Socket type not supported"},
    {EOPNOTSUPP, "Operation not supported"},
    {EPFNOSUPPORT, "Protocol family not supported"},
    {EAFNOSUPPORT, "Address family not supported"},
    {EADDRINUSE, "Address already in use"},
    {EADDRNOTAVAIL, "Cannot assign requested address"},
    {ENETDOWN, "Network is down"},
    {ENETUNREACH, "Network is unreachable"},
    {ENETRESET, "Network dropped connection on reset"},
    {ECONNABORTED, "Software caused connection abort"},
    {ECONNRESET, "Connection reset by peer"},
    {ENOBUFS, "No buffer space available"},
    {EISCONN, "Transport endpoint is connected"},
    {ENOTCONN, "Transport endpoint is not connected"},
    {ESHUTDOWN, "Cannot send after endpoint shutdown"},
    {ETIMEDOUT, "Connection timed out"},
    {ECONNREFUSED, "Connection refused"},
    {EHOSTDOWN, "Host is down"},
    {EHOSTUNREACH, "No route to host"},
    {EALREADY, "Operation already in progress"},
    {EINPROGRESS, "Operation now in progress"},
    {ESTALE, "Stale file handle"},
    {EUCLEAN, "Structure needs cleaning"},
    {ENOTNAM, "Not a XENIX named type file"},
    {ENAVAIL, "No XENIX semaphores available"},
    {EISNAM, "Is a named type file"},
    {EREMOTEIO, "Remote I/O error"},
    {EDQUOT, "Disk quota exceeded"},
    {ENOMEDIUM, "No medium found"},
    {EMEDIUMTYPE, "Wrong medium type"},
    {ECANCELED, "Operation canceled"},
    {ENOKEY, "Required key not available"},
    {EKEYEXPIRED, "Key has expired"},
    {EKEYREVOKED, "Key has been revoked"},
    {EKEYREJECTED, "Key was rejected by service"},
    {EOWNERDEAD, "Owner died"},
    {ENOTRECOVERABLE, "State not recoverable"},
    {ERFKILL, "Operation not possible due to RF-kill"},
    {EHWPOISON, "Memory page has hardware error"}
};

/* Jumlah entri dalam tabel */
#define ERROR_MSG_COUNT \
    (sizeof(error_messages) / sizeof(error_messages[0]))

/* Buffer statis untuk strerror() (non-reentrant) */
static char strerror_buffer[64];

/* ============================================================
 * FUNGSI KONVERSI ERROR
 * ============================================================
 */

/*
 * strerror - Konversi error number ke string
 *
 * Parameter:
 *   errnum - Error number (nilai errno)
 *
 * Return: Pointer ke string pesan error
 *
 * Peringatan: Fungsi ini TIDAK reentrant. Buffer internal
 *             digunakan untuk menyimpan hasil. Gunakan
 *             strerror_r() untuk aplikasi multi-threaded.
 */
char *strerror(int errnum) {
    size_t i;

    /* Cari error number dalam tabel */
    for (i = 0; i < ERROR_MSG_COUNT; i++) {
        if (error_messages[i].errnum == errnum) {
            return (char *)error_messages[i].msg;
        }
    }

    /* Error number tidak ditemukan, buat pesan generik */
    /* Catatan: Menggunakan buffer statis (non-reentrant) */
    if (errnum < 0) {
        strcpy(strerror_buffer, "Unknown error");
    } else {
        /* Format: "Unknown error NNN" */
        int num = errnum;
        int digits = 0;
        int temp;
        int pos;

        /* Hitung jumlah digit */
        temp = num;
        while (temp > 0) {
            digits++;
            temp /= 10;
        }
        if (digits == 0) {
            digits = 1;
        }

        /* Buat string */
        strcpy(strerror_buffer, "Unknown error ");
        pos = 14; /* strlen("Unknown error ") */
        
        /* Tambahkan digit dari belakang */
        strerror_buffer[pos + digits] = '\0';
        temp = num;
        for (int j = digits - 1; j >= 0; j--) {
            strerror_buffer[pos + j] = '0' + (temp % 10);
            temp /= 10;
        }
    }

    return strerror_buffer;
}

/*
 * strerror_r - Konversi error number ke string (reentrant)
 *
 * Parameter:
 *   errnum - Error number (nilai errno)
 *   buf    - Buffer tujuan
 *   buflen - Ukuran buffer
 *
 * Return:
 *   - 0 jika berhasil (versi POSIX)
 *   - String error jika gagal (versi GNU)
 *
 * Catatan: Implementasi ini mengikuti versi POSIX.
 */
int strerror_r(int errnum, char *buf, size_t buflen) {
    size_t i;
    size_t msglen;
    const char *msg;

    /* Validasi parameter */
    if (buf == NULL || buflen == 0) {
        return EINVAL;
    }

    /* Cari error number dalam tabel */
    msg = NULL;
    for (i = 0; i < ERROR_MSG_COUNT; i++) {
        if (error_messages[i].errnum == errnum) {
            msg = error_messages[i].msg;
            break;
        }
    }

    if (msg != NULL) {
        /* Pesan ditemukan */
        msglen = strlen(msg);
        if (msglen >= buflen) {
            /* Buffer terlalu kecil */
            /* Truncate jika masih muat sebagian */
            if (buflen > 1) {
                memcpy(buf, msg, buflen - 1);
                buf[buflen - 1] = '\0';
            } else {
                buf[0] = '\0';
            }
            return ERANGE;
        }
        strcpy(buf, msg);
        return 0;
    }

    /* Error number tidak ditemukan, buat pesan generik */
    if (errnum < 0) {
        msg = "Unknown error";
        msglen = strlen(msg);
        if (msglen >= buflen) {
            if (buflen > 1) {
                memcpy(buf, msg, buflen - 1);
                buf[buflen - 1] = '\0';
            } else {
                buf[0] = '\0';
            }
            return ERANGE;
        }
        strcpy(buf, msg);
        return 0;
    }

    /* Format: "Unknown error NNN" */
    {
        int num = errnum;
        int digits = 0;
        int temp;

        /* Hitung jumlah digit */
        temp = num;
        while (temp > 0) {
            digits++;
            temp /= 10;
        }
        if (digits == 0) {
            digits = 1;
        }

        /* Cek apakah buffer cukup */
        msglen = 14 + digits; /* "Unknown error " + digits */
        if (msglen >= buflen) {
            /* Buffer terlalu kecil */
            buf[0] = '\0';
            return ERANGE;
        }

        /* Buat string */
        strcpy(buf, "Unknown error ");
        temp = num;
        buf[msglen] = '\0';
        for (int j = digits - 1; j >= 0; j--) {
            buf[14 + j] = '0' + (temp % 10);
            temp /= 10;
        }
    }

    return 0;
}

/*
 * __strerror_l - strerror dengan locale eksplisit
 *
 * Parameter:
 *   errnum - Error number
 *   locale - Locale (tidak digunakan)
 *
 * Return: Pointer ke string pesan error
 *
 * Catatan: Extension POSIX, saat ini locale diabaikan.
 */
char *__strerror_l(int errnum, void *locale) {
    /* Suppress unused parameter warning */
    (void)locale;
    return strerror(errnum);
}
