/*
 * PIGURA LIBC - FCNTL.H
 * ======================
 * Header untuk file control options sesuai standar POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.1 - Perbaikan duplikasi dan penambahan konstanta
 */

#ifndef LIBC_FCNTL_H
#define LIBC_FCNTL_H

/* ============================================================
 * DEPENDENSI
 * ============================================================
 */
#include <sys/types.h>

/* ============================================================
 * KONSTANTA UNTUK open()
 * ============================================================
 */

/* Mode akses (harus salah satu dari O_RDONLY, O_WRONLY, O_RDWR) */
#define O_RDONLY    0x0000   /* Buka hanya untuk baca */
#define O_WRONLY    0x0001   /* Buka hanya untuk tulis */
#define O_RDWR      0x0002   /* Buka untuk baca dan tulis */

/* Mask untuk mode akses */
#define O_ACCMODE   0x0003

/* Flags tambahan untuk open() */
#define O_CREAT     0x0040   /* Buat file jika tidak ada */
#define O_EXCL      0x0080   /* Error jika file sudah ada */
#define O_NOCTTY    0x0100   /* Jangan jadikan controlling tty */
#define O_TRUNC     0x0200   /* Truncate file ke panjang 0 */
#define O_APPEND    0x0400   /* Tulis di akhir file */
#define O_NONBLOCK  0x0800   /* Non-blocking I/O */
#define O_DSYNC     0x1000   /* Synchronous data writes */
#define O_ASYNC     0x2000   /* Signal-driven I/O */
#define O_DIRECT    0x4000   /* Direct I/O (no caching) */
#define O_LARGEFILE 0x8000   /* File besar (untuk 32-bit) */
#define O_DIRECTORY 0x10000  /* Harus direktori */
#define O_NOFOLLOW  0x20000  /* Jangan ikuti symlink */
#define O_CLOEXEC   0x40000  /* Close-on-exec */
#define O_SYNC      0x80000  /* Synchronous writes */
#define O_PATH      0x100000 /* Path-only descriptor */

/* Alias */
#define O_RSYNC     O_SYNC   /* Synchronous reads */

/* ============================================================
 * KONSTANTA UNTUK fcntl()
 * ============================================================
 */

/* Commands untuk fcntl() */
#define F_DUPFD      0   /* Duplikasi fd */
#define F_GETFD      1   /* Dapatkan fd flags */
#define F_SETFD      2   /* Set fd flags */
#define F_GETFL      3   /* Dapatkan file status flags */
#define F_SETFL      4   /* Set file status flags */
#define F_GETLK      5   /* Dapatkan record lock */
#define F_SETLK      6   /* Set record lock (non-blocking) */
#define F_SETLKW     7   /* Set record lock (blocking) */
#define F_SETOWN     8   /* Set owner untuk signal I/O */
#define F_GETOWN     9   /* Dapatkan owner */
#define F_SETSIG    10   /* Set signal untuk I/O */
#define F_GETSIG    11   /* Dapatkan signal */
#define F_SETLEASE  12   /* Set lease */
#define F_GETLEASE  13   /* Dapatkan lease */
#define F_NOTIFY    14   /* Notification */
#define F_DUPFD_CLOEXEC 15  /* Dup dengan CLOEXEC */
#define F_GETPIPE_SZ    16  /* Dapatkan pipe buffer size */
#define F_SETPIPE_SZ    17  /* Set pipe buffer size */
#define F_ADD_SEALS     18  /* Add seals */
#define F_GET_SEALS     19  /* Get seals */

/* 64-bit versions */
#define F_GETLK64   20   /* Get lock 64-bit */
#define F_SETLK64   21   /* Set lock 64-bit non-blocking */
#define F_SETLKW64  22   /* Set lock 64-bit blocking */

/* Flags untuk F_SETFD/F_GETFD */
#define FD_CLOEXEC   1   /* Close-on-exec flag */

/* ============================================================
 * TIPE DATA UNTUK LOCKING
 * ============================================================
 */

/* Tipe lock */
#define F_RDLCK      0   /* Read lock (shared) */
#define F_WRLCK      1   /* Write lock (exclusive) */
#define F_UNLCK      2   /* Unlock */

/* Struktur untuk record locking */
struct flock {
    short l_type;    /* Tipe lock: F_RDLCK, F_WRLCK, F_UNLCK */
    short l_whence;  /* Reference point: SEEK_SET, SEEK_CUR, SEEK_END */
    off_t l_start;   /* Offset dari reference point */
    off_t l_len;     /* Panjang; 0 = sampai EOF */
    pid_t l_pid;     /* PID proses yang locking (untuk F_GETLK) */
};

/* Struktur untuk file lock 64-bit */
struct flock64 {
    short l_type;
    short l_whence;
    off_t l_start;
    off_t l_len;
    pid_t l_pid;
};

/* ============================================================
 * KONSTANTA UNTUK LEASE
 * ============================================================
 */
#define F_RDLCK_LEASE   0   /* Read lease */
#define F_WRLCK_LEASE   1   /* Write lease */
#define F_UNLCK_LEASE   2   /* Unlock lease */

/* ============================================================
 * KONSTANTA UNTUK SEALS
 * ============================================================
 */
#define F_SEAL_SEAL     0x0001  /* Prevent adding more seals */
#define F_SEAL_SHRINK   0x0002  /* Prevent file shrinking */
#define F_SEAL_GROW     0x0004  /* Prevent file growing */
#define F_SEAL_WRITE    0x0008  /* Prevent writes */
#define F_SEAL_FUTURE_WRITE 0x0010  /* Prevent future writes */

/* ============================================================
 * KONSTANTA UNTUK NOTIFY
 * ============================================================
 */
#define DN_ACCESS      0x00000001  /* File diakses */
#define DN_MODIFY      0x00000002  /* File dimodifikasi */
#define DN_CREATE      0x00000004  /* File dibuat */
#define DN_DELETE      0x00000008  /* File dihapus */
#define DN_RENAME      0x00000010  /* File di-rename */
#define DN_ATTRIB      0x00000020  /* Attribute berubah */
#define DN_MULTISHOT   0x80000000  /* Multiple notification */

/* ============================================================
 * KONSTANTA LOCKF
 * ============================================================
 */
#define F_LOCK    0   /* Lock region (blocking) */
#define F_TLOCK   1   /* Test and lock (non-blocking) */
#define F_ULOCK   2   /* Unlock region */
#define F_TEST    3   /* Test for lock */

/* ============================================================
 * KONSTANTA FLOCK (BSD)
 * ============================================================
 */
#define LOCK_SH   1   /* Shared lock */
#define LOCK_EX   2   /* Exclusive lock */
#define LOCK_UN   8   /* Unlock */
#define LOCK_NB   4   /* Non-blocking */

/* ============================================================
 * SEEK CONSTANTS
 * ============================================================
 */
#ifndef SEEK_SET
#define SEEK_SET 0   /* Dari awal file */
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1   /* Dari posisi saat ini */
#endif
#ifndef SEEK_END
#define SEEK_END 2   /* Dari akhir file */
#endif

/* ============================================================
 * MODE BIT UNTUK open()
 * ============================================================
 * Catatan: Definisi S_* lengkap ada di sys/stat.h
 */

/* Permission bits - tidak menduplikasi dari sys/stat.h */
/* Gunakan dari sys/stat.h atau sys/types.h */

/* ============================================================
 * KONSTANTA UNTUK openat()
 * ============================================================
 */

/* Special value untuk dirfd */
#define AT_FDCWD -100   /* Gunakan current working directory */

/* Flags untuk openat() dan fstatat() */
#define AT_SYMLINK_NOFOLLOW   0x100   /* Jangan dereference symlink */
#define AT_REMOVEDIR          0x200   /* Hapus direktori */
#define AT_SYMLINK_FOLLOW     0x400   /* Follow symlink */
#define AT_NO_AUTOMOUNT       0x800   /* Jangan automount */
#define AT_EMPTY_PATH         0x1000  /* Path kosong diizinkan */
#define AT_STATX_SYNC_AS_STAT 0x0000  /* Sync sebagai stat */
#define AT_STATX_SYNC_TYPE    0x6000  /* Sync type mask */
#define AT_STATX_DONT_SYNC    0x4000  /* Jangan sync */

/* ============================================================
 * DEKLARASI FUNGSI
 * ============================================================
 */

/*
 * open - Buka file
 */
extern int open(const char *pathname, int flags, ...);

/*
 * openat - Buka file relatif ke direktori
 */
extern int openat(int dirfd, const char *pathname, int flags, ...);

/*
 * creat - Buat file
 */
extern int creat(const char *pathname, mode_t mode);

/*
 * fcntl - Manipulasi file descriptor
 */
extern int fcntl(int fd, int cmd, ...);

/* ============================================================
 * FUNGSI TAMBAHAN
 * ============================================================
 */

/*
 * posix_fadvise - Beri hint tentang pola akses file
 */
extern int posix_fadvise(int fd, off_t offset, off_t len, int advice);

/* Constants untuk posix_fadvise */
#define POSIX_FADV_NORMAL      0  /* Tidak ada hint khusus */
#define POSIX_FADV_RANDOM      1  /* Akses random */
#define POSIX_FADV_SEQUENTIAL  2  /* Akses sequential */
#define POSIX_FADV_WILLNEED    3  /* Akan dibutuhkan */
#define POSIX_FADV_DONTNEED    4  /* Tidak akan dibutuhkan */
#define POSIX_FADV_NOREUSE     5  /* Akses sekali saja */

/*
 * posix_fallocate - Alokasikan ruang untuk file
 */
extern int posix_fallocate(int fd, off_t offset, off_t len);

/*
 * splice - Pindahkan data antara pipe dan file
 */
extern long splice(int fd_in, off_t *off_in, int fd_out,
                   off_t *off_out, size_t len, unsigned int flags);

/*
 * tee - Duplikasi data pipe
 */
extern long tee(int fd_in, int fd_out, size_t len, unsigned int flags);

/*
 * vmsplice - Map user memory ke pipe
 */
extern long vmsplice(int fd, const struct iovec *iov,
                     unsigned long nr_segs, unsigned int flags);

/*
 * readahead - Baca data ke page cache
 */
extern ssize_t readahead(int fd, off_t offset, size_t count);

/*
 * flock - Apply atau remove advisory lock
 */
extern int flock(int fd, int operation);

/*
 * lockf - Apply atau remove POSIX lock
 */
extern int lockf(int fd, int cmd, off_t len);

#endif /* LIBC_FCNTL_H */
