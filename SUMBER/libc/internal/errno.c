/*
 * PIGURA LIBC - ERRNO.C
 * ======================
 * Implementasi variabel global errno dan fungsi terkait.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#include <errno.h>

/* ============================================================
 * VARIABEL GLOBAL ERNO
 * ============================================================
 * Variabel ini menyimpan kode error terakhir dari fungsi
 * sistem. Diinisialisasi ke 0 saat program dimulai.
 *
 * Thread-safety note: Dalam implementasi multi-threaded,
 * errno seharusnya thread-local. Untuk implementasi
 * single-threaded ini, menggunakan variabel global sederhana.
 */

int errno = 0;

/* ============================================================
 * TABEL PESAN ERROR
 * ============================================================
 * Array string yang memetakan kode error ke pesan yang
 * dapat dibaca manusia.
 */

static const char * const __error_messages[] = {
    /* 0 */    "Success",
    /* 1 */    "Numerical argument out of domain",     /* EDOM */
    /* 2 */    "Result too large",                     /* ERANGE */
    /* 3 */    "Illegal byte sequence",                /* EILSEQ */
    /* 4 */    "No such file or directory",            /* ENOENT */
    /* 5 */    "No such process",                      /* ESRCH */
    /* 6 */    "Interrupted system call",              /* EINTR */
    /* 7 */    "Input/output error",                   /* EIO */
    /* 8 */    "No such device or address",            /* ENXIO */
    /* 9 */    "Argument list too long",               /* E2BIG */
    /* 10 */   "Exec format error",                    /* ENOEXEC */
    /* 11 */   "Bad file descriptor",                  /* EBADF */
    /* 12 */   "No child processes",                   /* ECHILD */
    /* 13 */   "Resource temporarily unavailable",     /* EAGAIN */
    /* 14 */   "Cannot allocate memory",               /* ENOMEM */
    /* 15 */   "Permission denied",                    /* EACCES */
    /* 16 */   "Bad address",                          /* EFAULT */
    /* 17 */   "Block device required",                /* ENOTBLK */
    /* 18 */   "Device or resource busy",              /* EBUSY */
    /* 19 */   "File exists",                          /* EEXIST */
    /* 20 */   "Invalid cross-device link",            /* EXDEV */
    /* 21 */   "Operation not permitted",              /* EPERM */
    /* 22 */   "Not a directory",                      /* ENOTDIR */
    /* 23 */   "Is a directory",                       /* EISDIR */
    /* 24 */   "Invalid argument",                     /* EINVAL */
    /* 25 */   "Too many open files in system",        /* ENFILE */
    /* 26 */   "Too many open files",                  /* EMFILE */
    /* 27 */   "Inappropriate ioctl for device",       /* ENOTTY */
    /* 28 */   "Text file busy",                       /* ETXTBSY */
    /* 29 */   "File too large",                       /* EFBIG */
    /* 30 */   "No space left on device",              /* ENOSPC */
    /* 31 */   "Illegal seek",                         /* ESPIPE */
    /* 32 */   "Read-only file system",                /* EROFS */
    /* 33 */   "Too many links",                       /* EMLINK */
    /* 34 */   "Broken pipe",                          /* EPIPE */
    /* 35 */   "Value too large for defined data type", /* EOVERFLOW */
    /* 36 */   "Value too small",                      /* EUNDERFLOW */
    /* 37-39 */ NULL, NULL, NULL,
    /* 40 */   "Socket operation on non-socket",       /* ENOTSOCK */
    /* 41 */   "Destination address required",         /* EDESTADDRREQ */
    /* 42 */   "Message too long",                     /* EMSGSIZE */
    /* 43 */   "Protocol wrong type for socket",       /* EPROTOTYPE */
    /* 44 */   "Protocol not available",               /* ENOPROTOOPT */
    /* 45 */   "Protocol not supported",               /* EPROTONOSUPPORT */
    /* 46 */   "Socket type not supported",            /* ESOCKTNOSUPPORT */
    /* 47 */   "Operation not supported",              /* EOPNOTSUPP */
    /* 48 */   "Protocol family not supported",        /* EPFNOSUPPORT */
    /* 49 */   "Address family not supported",         /* EAFNOSUPPORT */
    /* 50 */   "Address already in use",               /* EADDRINUSE */
    /* 51 */   "Cannot assign requested address",      /* EADDRNOTAVAIL */
    /* 52 */   "Network is down",                      /* ENETDOWN */
    /* 53 */   "Network is unreachable",               /* ENETUNREACH */
    /* 54 */   "Network dropped connection on reset",  /* ENETRESET */
    /* 55 */   "Software caused connection abort",     /* ECONNABORTED */
    /* 56 */   "Connection reset by peer",             /* ECONNRESET */
    /* 57 */   "No buffer space available",            /* ENOBUFS */
    /* 58 */   "Transport endpoint is already connected", /* EISCONN */
    /* 59 */   "Transport endpoint is not connected",  /* ENOTCONN */
    /* 60 */   "Cannot send after transport endpoint shutdown", /* ESHUTDOWN */
    /* 61 */   "Connection timed out",                 /* ETIMEDOUT */
    /* 62 */   "Connection refused",                   /* ECONNREFUSED */
    /* 63 */   "Too many levels of symbolic links",    /* ELOOP */
    /* 64 */   "File name too long",                   /* ENAMETOOLONG */
    /* 65 */   "Directory not empty",                  /* ENOTEMPTY */
    /* 66 */   "Disk quota exceeded",                  /* EDQUOT */
    /* 67-69 */ NULL, NULL, NULL,
    /* 70 */   "Identifier removed",                   /* EIDRM */
    /* 71-84 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    /* 85 */   "Resource deadlock avoided",            /* EDEADLK */
    /* 86 */   "No locks available",                   /* ENOLCK */
    /* 87 */   "Function not implemented",             /* ENOSYS */
    /* 88 */   "No message of desired type",           /* ENOMSG */
    /* 89 */   "Protocol error",                       /* EPROTO */
    /* 90 */   "Multihop attempted",                   /* EMULTIHOP */
    /* 91 */   "Bad message",                          /* EBADMSG */
    /* 92 */   "No data available",                    /* ENODATA */
    /* 93 */   "Value too large for data type",        /* EOVERFLOW */
    /* 94 */   "Link has been severed",                /* ENOLINK */
    /* 95 */   "Illegal byte sequence",                /* EILSEQ */
    /* 96 */   "State not recoverable",                /* ENOTRECOVERABLE */
    /* 97 */   "Previous owner died",                  /* EOWNERDEAD */
    /* 98 */   "Operation not possible due to RF-kill", /* ERFKILL */
    /* 99 */   "Memory page has hardware error",       /* EHWPOISON */
};

#define NUM_ERROR_MESSAGES \
    (sizeof(__error_messages) / sizeof(__error_messages[0]))

/* ============================================================
 * FUNGSI INTERNAL
 * ============================================================
 */

/*
 * __errno_location - Dapatkan pointer ke errno
 *
 * Fungsi ini digunakan oleh thread-safe code untuk mendapatkan
 * alamat errno yang tepat untuk thread saat ini.
 *
 * Return: Pointer ke variabel errno
 */
int *__errno_location(void) {
    return &errno;
}

/*
 * __get_errno - Dapatkan nilai errno saat ini
 *
 * Return: Nilai errno saat ini
 */
int __get_errno(void) {
    return errno;
}

/*
 * __set_errno - Set nilai errno
 *
 * Parameter:
 *   value - Nilai baru errno
 */
void __set_errno(int value) {
    errno = value;
}

/*
 * __clear_errno - Reset errno ke 0
 */
void __clear_errno(void) {
    errno = 0;
}

/* ============================================================
 * FUNGSI PUBLIK
 * ============================================================
 */

/*
 * __strerror_r - Implementasi internal strerror_r
 *
 * Parameter:
 *   errnum - Kode error
 *   buf    - Buffer untuk pesan
 *   buflen - Panjang buffer
 *
 * Return: 0 jika berhasil, -1 jika error
 */
int __strerror_r(int errnum, char *buf, size_t buflen) {
    const char *msg;
    size_t len;
    int i;
    
    /* Validasi parameter */
    if (buf == NULL || buflen == 0) {
        return -1;
    }
    
    /* Cek apakah errnum valid */
    if (errnum < 0 || errnum >= (int)NUM_ERROR_MESSAGES) {
        /* Error tidak dikenal */
        msg = "Unknown error";
        len = 13;  /* strlen("Unknown error") */
    } else if (__error_messages[errnum] == NULL) {
        /* Slot kosong dalam tabel */
        msg = "Unknown error";
        len = 13;
    } else {
        msg = __error_messages[errnum];
        len = 0;
        while (msg[len] != '\0') {
            len++;
        }
    }
    
    /* Copy pesan ke buffer */
    if (buflen < len + 1) {
        /* Buffer terlalu kecil, copy sebanyak mungkin */
        for (i = 0; i < (int)(buflen - 1); i++) {
            buf[i] = msg[i];
        }
        buf[buflen - 1] = '\0';
        return -1;  /* Indikasi truncation */
    }
    
    /* Copy pesan lengkap */
    for (i = 0; i <= (int)len; i++) {
        buf[i] = msg[i];
    }
    
    return 0;
}

/*
 * strerror - Dapatkan pesan error string
 *
 * Parameter:
 *   errnum - Kode error
 *
 * Return: Pointer ke string pesan error
 *
 * Catatan: String return adalah string statis, tidak boleh
 * dimodifikasi. Jika errnum tidak valid, mengembalikan
 * pesan "Unknown error".
 */
static char __strerror_buf[128];

char *strerror(int errnum) {
    /* Gunakan buffer statis untuk hasil */
    __strerror_r(errnum, __strerror_buf, sizeof(__strerror_buf));
    return __strerror_buf;
}

/*
 * perror - Cetak pesan error ke stderr
 *
 * Parameter:
 *   s - String prefix (boleh NULL)
 *
 * Format output: "s: error message\n" atau "error message\n"
 */
void perror(const char *s) {
    extern void write_to_stderr(const char *msg, int len);
    
    if (s != NULL && s[0] != '\0') {
        /* Tulis prefix */
        int len = 0;
        while (s[len] != '\0') {
            len++;
        }
        write_to_stderr(s, len);
        write_to_stderr(": ", 2);
    }
    
    /* Tulis pesan error */
    char *msg = strerror(errno);
    int len = 0;
    while (msg[len] != '\0') {
        len++;
    }
    write_to_stderr(msg, len);
    write_to_stderr("\n", 1);
}

/* Stub untuk write_to_stderr jika belum diimplementasikan */
void write_to_stderr(const char *msg, int len) {
    /* Gunakan syscall write langsung */
    extern long syscall3(long num, long a1, long a2, long a3);
    /* write(2, msg, len) - syscall number varies by arch */
    (void)msg;
    (void)len;
    /* Placeholder - actual implementation needs syscall */
}
