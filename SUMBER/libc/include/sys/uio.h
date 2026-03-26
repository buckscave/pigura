/*
 * PIGURA LIBC - SYS/UIO.H
 * ========================
 * Header untuk scatter/gather I/O sesuai standar POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.1 - Perbaikan duplikasi dan kompatibilitas C89
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
 * STRUKTUR UIO
 * ============================================================
 * Struktur untuk menyimpan informasi I/O operation.
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
 */
extern ssize_t readv(int fd, const struct iovec *iov, int iovcnt);

/*
 * writev - Scatter write
 */
extern ssize_t writev(int fd, const struct iovec *iov, int iovcnt);

/*
 * preadv - Scatter read at offset
 */
extern ssize_t preadv(int fd, const struct iovec *iov, int iovcnt,
                      off_t offset);

/*
 * pwritev - Scatter write at offset
 */
extern ssize_t pwritev(int fd, const struct iovec *iov, int iovcnt,
                       off_t offset);

/*
 * preadv2 - Scatter read at offset dengan flags
 */
extern ssize_t preadv2(int fd, const struct iovec *iov, int iovcnt,
                       off_t offset, int flags);

/*
 * pwritev2 - Scatter write at offset dengan flags
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
 */
extern int uiomove(void *cp, size_t n, enum uio_rw rw,
                   struct uio *uio);

/*
 * iovec_count_total - Hitung total bytes dalam iovec array
 */
extern size_t iovec_count_total(const struct iovec *iov, int iovcnt);

/*
 * iovec_validate - Validasi iovec array
 */
extern int iovec_validate(const struct iovec *iov, int iovcnt,
                          size_t max_bytes);

/* ============================================================
 * KONSTANTA LIMIT
 * ============================================================
 */

/* Maximum number of iovec elements */
#define IOV_MAX         1024
#define UIO_MAXIOV      IOV_MAX

/* Maximum total I/O size */
#define UIO_MAXSIZE     (2147483647)

#endif /* LIBC_SYS_UIO_H */
