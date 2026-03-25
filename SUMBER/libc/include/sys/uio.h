/*
 * PIGURA LIBC - SYS/UIO.H
 * ========================
 * Header untuk scatter/gather I/O sesuai standar POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#ifndef LIBC_SYS_UIO_H
#define LIBC_SYS_UIO_H

/* ============================================================
 * DEPENDENSI
 * ============================================================
 */
#include <sys/types.h>

/* ============================================================
 * NULL POINTER
 * ============================================================
 */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* ============================================================
 * STRUKTUR IVEC
 * ============================================================
 * Struktur untuk satu I/O vector element.
 * Digunakan untuk scatter/gather I/O operations.
 */

struct iovec {
    void  *iov_base;  /* Base address of buffer */
    size_t iov_len;   /* Length of buffer */
};

/* ============================================================
 * STRUKTUR UIO
 * ============================================================
 * Struktur untuk menyimpan informasi I/O operation.
 * Digunakan oleh sistem operasi untuk tracking I/O.
 */

enum uio_rw {
    UIO_READ,   /* Read operation */
    UIO_WRITE   /* Write operation */
};

enum uio_seg {
    UIO_USERSPACE,  /* Data in user space */
    UIO_SYSSPACE,   /* Data in kernel space */
    UIO_NOCOPY      /* Don't copy data */
};

struct uio {
    struct iovec    *uio_iov;      /* Pointer ke array iovec */
    int              uio_iovcnt;    /* Jumlah iovec elements */
    off_t            uio_offset;    /* Offset dalam file */
    ssize_t          uio_resid;     /* Sisa bytes untuk transfer */
    enum uio_rw      uio_rw;        /* Direction: read/write */
    enum uio_seg     uio_segflg;    /* Address space flag */
};

/* ============================================================
 * DEKLARASI FUNGSI
 * ============================================================
 */

/*
 * readv - Scatter read
 *
 * Parameter:
 *   fd   - File descriptor
 *   iov  - Array I/O vectors
 *   iovcnt - Jumlah vectors
 *
 * Return: Total bytes read, atau -1 jika error
 */
extern ssize_t readv(int fd, const struct iovec *iov, int iovcnt);

/*
 * writev - Scatter write
 *
 * Parameter:
 *   fd   - File descriptor
 *   iov  - Array I/O vectors
 *   iovcnt - Jumlah vectors
 *
 * Return: Total bytes written, atau -1 jika error
 */
extern ssize_t writev(int fd, const struct iovec *iov, int iovcnt);

/*
 * preadv - Scatter read at offset
 *
 * Parameter:
 *   fd     - File descriptor
 *   iov    - Array I/O vectors
 *   iovcnt - Jumlah vectors
 *   offset - File offset
 *
 * Return: Total bytes read, atau -1 jika error
 */
extern ssize_t preadv(int fd, const struct iovec *iov, int iovcnt,
                      off_t offset);

/*
 * pwritev - Scatter write at offset
 *
 * Parameter:
 *   fd     - File descriptor
 *   iov    - Array I/O vectors
 *   iovcnt - Jumlah vectors
 *   offset - File offset
 *
 * Return: Total bytes written, atau -1 jika error
 */
extern ssize_t pwritev(int fd, const struct iovec *iov, int iovcnt,
                       off_t offset);

/*
 * preadv2 - Scatter read at offset dengan flags
 *
 * Parameter:
 *   fd     - File descriptor
 *   iov    - Array I/O vectors
 *   iovcnt - Jumlah vectors
 *   offset - File offset
 *   flags  - Flags (RWF_*)
 *
 * Return: Total bytes read, atau -1 jika error
 */
extern ssize_t preadv2(int fd, const struct iovec *iov, int iovcnt,
                       off_t offset, int flags);

/*
 * pwritev2 - Scatter write at offset dengan flags
 *
 * Parameter:
 *   fd     - File descriptor
 *   iov    - Array I/O vectors
 *   iovcnt - Jumlah vectors
 *   offset - File offset
 *   flags  - Flags (RWF_*)
 *
 * Return: Total bytes written, atau -1 jika error
 */
extern ssize_t pwritev2(int fd, const struct iovec *iov, int iovcnt,
                        off_t offset, int flags);

/* ============================================================
 * KONSTANTA RWF_* FLAGS
 * ============================================================
 */

#define RWF_DSYNC       0x00000001  /* Per-file O_DSYNC */
#define RWF_HIPRI       0x00000002  /* High priority request */
#define RWF_SYNC        0x00000004  /* Per-file O_SYNC */
#define RWF_NOWAIT      0x00000008  /* Don't wait for data */
#define RWF_APPEND      0x00000010  /* Append mode */

/* ============================================================
 * FUNGSI UTILITAS UIO
 * ============================================================
 */

/*
 * uiomove - Pindahkan data ke/dari uio
 *
 * Parameter:
 *   cp    - Kernel buffer
 *   n     - Jumlah bytes
 *   rw    - Direction (UIO_READ atau UIO_WRITE)
 *   uio   - UIO structure
 *
 * Return: 0 jika berhasil, error code jika gagal
 *
 * Catatan: Fungsi internal kernel, biasanya tidak
 *          tersedia di user-space.
 */
extern int uiomove(void *cp, size_t n, enum uio_rw rw,
                   struct uio *uio);

/*
 * iovec_count_total - Hitung total bytes dalam iovec array
 *
 * Parameter:
 *   iov    - Array I/O vectors
 *   iovcnt - Jumlah vectors
 *
 * Return: Total bytes
 */
static inline size_t iovec_count_total(const struct iovec *iov,
                                        int iovcnt) {
    size_t total = 0;
    int i;
    for (i = 0; i < iovcnt; i++) {
        total += iov[i].iov_len;
    }
    return total;
}

/*
 * iovec_validate - Validasi iovec array
 *
 * Parameter:
 *   iov    - Array I/O vectors
 *   iovcnt - Jumlah vectors
 *   max_bytes - Maksimum bytes yang diizinkan
 *
 * Return: 0 jika valid, -1 jika tidak valid
 */
static inline int iovec_validate(const struct iovec *iov,
                                  int iovcnt, size_t max_bytes) {
    int i;
    size_t total = 0;
    
    if (iov == NULL || iovcnt <= 0) {
        return -1;
    }
    
    for (i = 0; i < iovcnt; i++) {
        if (iov[i].iov_base == NULL && iov[i].iov_len > 0) {
            return -1;
        }
        
        /* Check for overflow */
        if (total + iov[i].iov_len < total) {
            return -1;
        }
        
        total += iov[i].iov_len;
        
        if (total > max_bytes) {
            return -1;
        }
    }
    
    return 0;
}

/* ============================================================
 * KONSTANTA LIMIT
 * ============================================================
 */

/* Maximum number of iovec elements */
#define IOV_MAX         1024
#define UIO_MAXIOV      IOV_MAX

/* Maximum total I/O size */
#define UIO_MAXSIZE     (INT_MAX)

#endif /* LIBC_SYS_UIO_H */
