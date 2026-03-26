/*
 * PIGURA LIBC - FCNTL/FCNTL.C
 * ===========================
 * Implementasi fungsi file control.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Implementasi untuk Pigura OS dengan dukungan
 * POSIX file control operations.
 */

#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

/* ============================================================
 * DEKLARASI SYSCALL
 * ============================================================
 */

/* Syscall interface ke kernel */
extern long __syscall_open(const char *pathname, int flags, mode_t mode);
extern long __syscall_openat(int dirfd, const char *pathname,
                             int flags, mode_t mode);
extern long __syscall_close(int fd);
extern long __syscall_fcntl(int fd, int cmd, long arg);
extern long __syscall_ioctl(int fd, unsigned long request, long arg);

/* ============================================================
 * FUNGSI FCNTL
 * ============================================================
 */

/*
 * fcntl - Manipulasi file descriptor
 *
 * Parameter:
 *   fd  - File descriptor
 *   cmd - Command (F_*)
 *   ... - Argumen tambahan (tergantung cmd)
 *
 * Return: Tergantung command, atau -1 jika error
 */
int fcntl(int fd, int cmd, ...) {
    va_list args;
    long arg;
    long result;
    int int_arg;
    struct flock *flock_arg;

    /* Validasi file descriptor */
    if (fd < 0) {
        errno = EBADF;
        return -1;
    }

    /* Ambil argumen berdasarkan command */
    va_start(args, cmd);

    switch (cmd) {
        /* Commands tanpa argumen */
        case F_GETFD:
        case F_GETFL:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
        case F_GETPIPE_SZ:
            arg = 0;
            break;

        /* Commands dengan integer argumen */
        case F_SETFD:
        case F_SETFL:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_SETPIPE_SZ:
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
            int_arg = va_arg(args, int);
            arg = (long)int_arg;
            break;

        /* Commands dengan pointer argumen */
        case F_GETLK:
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK64:
        case F_SETLK64:
        case F_SETLKW64:
            flock_arg = va_arg(args, struct flock *);
            arg = (long)flock_arg;
            break;

        default:
            va_end(args);
            errno = EINVAL;
            return -1;
    }

    va_end(args);

    /* Panggil syscall */
    result = __syscall_fcntl(fd, cmd, arg);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return (int)result;
}

/* ============================================================
 * FUNGSI OPEN
 * ============================================================
 */

/*
 * open - Buka file
 *
 * Parameter:
 *   pathname - Path ke file
 *   flags    - Flags (O_*)
 *   ...      - Mode (hanya jika O_CREAT digunakan)
 *
 * Return: File descriptor, atau -1 jika error
 */
int open(const char *pathname, int flags, ...) {
    va_list args;
    mode_t mode;
    long result;

    /* Validasi pathname */
    if (pathname == NULL || *pathname == '\0') {
        errno = EINVAL;
        return -1;
    }

    /* Ambil mode jika O_CREAT digunakan */
    if (flags & O_CREAT) {
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    } else {
        mode = 0;
    }

    /* Validasi flags */
    /* Pastikan minimal salah satu dari O_RDONLY, O_WRONLY, O_RDWR */
    if ((flags & O_ACCMODE) != O_RDONLY &&
        (flags & O_ACCMODE) != O_WRONLY &&
        (flags & O_ACCMODE) != O_RDWR) {
        /* Default ke O_RDONLY jika tidak ada yang ditentukan */
        flags = (flags & ~O_ACCMODE) | O_RDONLY;
    }

    /* Panggil syscall */
    result = __syscall_open(pathname, flags, mode);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return (int)result;
}

/*
 * openat - Buka file relatif ke directory fd
 *
 * Parameter:
 *   dirfd    - Directory file descriptor atau AT_FDCWD
 *   pathname - Path ke file (relatif atau absolut)
 *   flags    - Flags (O_*)
 *   ...      - Mode (hanya jika O_CREAT digunakan)
 *
 * Return: File descriptor, atau -1 jika error
 */
int openat(int dirfd, const char *pathname, int flags, ...) {
    va_list args;
    mode_t mode;
    long result;

    /* Validasi pathname */
    if (pathname == NULL || *pathname == '\0') {
        /* Pathname kosong dengan AT_EMPTY_PATH diizinkan */
        if (!(flags & AT_EMPTY_PATH)) {
            errno = ENOENT;
            return -1;
        }
    }

    /* Ambil mode jika O_CREAT digunakan */
    if (flags & O_CREAT) {
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    } else {
        mode = 0;
    }

    /* Validasi dirfd */
    if (dirfd < 0 && dirfd != AT_FDCWD) {
        errno = EBADF;
        return -1;
    }

    /* Jika pathname absolut, dirfd diabaikan */
    if (pathname != NULL && pathname[0] == '/') {
        return open(pathname, flags, mode);
    }

    /* Panggil syscall */
    result = __syscall_openat(dirfd, pathname, flags, mode);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return (int)result;
}

/*
 * creat - Buat file baru (equiv: open(path, O_CREAT|O_WRONLY|O_TRUNC))
 *
 * Parameter:
 *   pathname - Path ke file
 *   mode     - Permission mode
 *
 * Return: File descriptor, atau -1 jika error
 */
int creat(const char *pathname, mode_t mode) {
    /* creat() equivalen dengan open() dengan flags tertentu */
    return open(pathname, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

/* ============================================================
 * FUNGSI POSIX_FADVISE/POSIX_FALLOCATE
 * ============================================================
 */

/*
 * posix_fadvise - Beri advice tentang akses file
 *
 * Parameter:
 *   fd     - File descriptor
 *   offset - Offset awal
 *   len    - Panjang (0 = sampai EOF)
 *   advice - Advice (POSIX_FADV_*)
 *
 * Return: 0 jika berhasil, error number jika gagal
 */
int posix_fadvise(int fd, off_t offset, off_t len, int advice) {
    long result;

    /* Validasi file descriptor */
    if (fd < 0) {
        return EBADF;
    }

    /* Validasi advice */
    switch (advice) {
        case POSIX_FADV_NORMAL:
        case POSIX_FADV_RANDOM:
        case POSIX_FADV_SEQUENTIAL:
        case POSIX_FADV_WILLNEED:
        case POSIX_FADV_DONTNEED:
        case POSIX_FADV_NOREUSE:
            break;
        default:
            return EINVAL;
    }

    /* Validasi offset dan len */
    if (offset < 0 || len < 0) {
        return EINVAL;
    }

    /* Panggil syscall */
    extern long __syscall_fadvise64(int fd, off_t offset, off_t len,
                                    int advice);
    result = __syscall_fadvise64(fd, offset, len, advice);

    if (result < 0) {
        return (int)-result;
    }

    return 0;
}

/*
 * posix_fallocate - Alokasikan ruang untuk file
 *
 * Parameter:
 *   fd     - File descriptor
 *   offset - Offset awal
 *   len    - Panjang alokasi
 *
 * Return: 0 jika berhasil, error number jika gagal
 */
int posix_fallocate(int fd, off_t offset, off_t len) {
    long result;
    struct stat st;

    /* Validasi file descriptor */
    if (fd < 0) {
        return EBADF;
    }

    /* Validasi offset dan len */
    if (offset < 0 || len <= 0) {
        return EINVAL;
    }

    /* Cek apakah fd adalah regular file */
    if (fstat(fd, &st) < 0) {
        return errno;
    }

    if (!S_ISREG(st.st_mode)) {
        return ENODEV;
    }

    /* Cek apakah file bisa ditulis */
    /* (ini seharusnya dicek di syscall) */

    /* Panggil syscall */
    extern long __syscall_fallocate(int fd, int mode, off_t offset,
                                    off_t len);
    result = __syscall_fallocate(fd, 0, offset, len);

    if (result < 0) {
        return (int)-result;
    }

    return 0;
}

/* ============================================================
 * FUNGSI UTILITAS FILE
 * ============================================================
 */

/*
 * readahead - Preload file content ke page cache
 *
 * Parameter:
 *   fd     - File descriptor
 *   offset - Offset awal
 *   count  - Jumlah byte
 *
 * Return: Jumlah byte yang di-readahead, atau -1 jika error
 */
ssize_t readahead(int fd, off_t offset, size_t count) {
    long result;

    if (fd < 0) {
        errno = EBADF;
        return -1;
    }

    extern long __syscall_readahead(int fd, off_t offset, size_t count);
    result = __syscall_readahead(fd, offset, count);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return (ssize_t)result;
}

/*
 * splice - Transfer data antara pipe dan file
 *
 * Parameter:
 *   fd_in      - Input file descriptor
 *   off_in     - Input offset (boleh NULL)
 *   fd_out     - Output file descriptor
 *   off_out    - Output offset (boleh NULL)
 *   len        - Jumlah byte
 *   flags      - Flags (SPLICE_F_*)
 *
 * Return: Jumlah byte transfer, atau -1 jika error
 */
ssize_t splice(int fd_in, off_t *off_in, int fd_out, off_t *off_out,
               size_t len, unsigned int flags) {
    long result;

    if (fd_in < 0 || fd_out < 0) {
        errno = EBADF;
        return -1;
    }

    extern long __syscall_splice(int fd_in, off_t *off_in,
                                 int fd_out, off_t *off_out,
                                 size_t len, unsigned int flags);
    result = __syscall_splice(fd_in, off_in, fd_out, off_out, len, flags);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return (ssize_t)result;
}

/*
 * tee - Duplicate pipe content
 *
 * Parameter:
 *   fd_in  - Input pipe
 *   fd_out - Output pipe
 *   len    - Jumlah byte
 *   flags  - Flags
 *
 * Return: Jumlah byte, atau -1 jika error
 */
ssize_t tee(int fd_in, int fd_out, size_t len, unsigned int flags) {
    long result;

    if (fd_in < 0 || fd_out < 0) {
        errno = EBADF;
        return -1;
    }

    extern long __syscall_tee(int fd_in, int fd_out, size_t len,
                              unsigned int flags);
    result = __syscall_tee(fd_in, fd_out, len, flags);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return (ssize_t)result;
}

/*
 * vmsplice - Transfer user memory ke pipe
 *
 * Parameter:
 *   fd    - Pipe file descriptor
 *   iov   - Iovec array
 *   nr_segs - Jumlah segments
 *   flags - Flags
 *
 * Return: Jumlah byte, atau -1 jika error
 */
ssize_t vmsplice(int fd, const struct iovec *iov, unsigned long nr_segs,
                 unsigned int flags) {
    long result;

    if (fd < 0) {
        errno = EBADF;
        return -1;
    }

    if (iov == NULL && nr_segs > 0) {
        errno = EINVAL;
        return -1;
    }

    extern long __syscall_vmsplice(int fd, const struct iovec *iov,
                                   unsigned long nr_segs,
                                   unsigned int flags);
    result = __syscall_vmsplice(fd, iov, nr_segs, flags);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return (ssize_t)result;
}

/* ============================================================
 * FUNGSI FILE LOCKING
 * ============================================================
 */

/*
 * flock - Apply atau remove advisory lock
 *
 * Parameter:
 *   fd    - File descriptor
 *   operation - LOCK_SH, LOCK_EX, LOCK_UN, atau dengan LOCK_NB
 *
 * Return: 0 jika berhasil, -1 jika error
 */
int flock(int fd, int operation) {
    struct flock fl;
    int cmd;
    int result;

    if (fd < 0) {
        errno = EBADF;
        return -1;
    }

    /* Setup flock structure */
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;  /* Lock entire file */
    fl.l_pid = 0;

    /* Convert operation ke flock */
    switch (operation & ~LOCK_NB) {
        case LOCK_SH:
            fl.l_type = F_RDLCK;
            break;
        case LOCK_EX:
            fl.l_type = F_WRLCK;
            break;
        case LOCK_UN:
            fl.l_type = F_UNLCK;
            break;
        default:
            errno = EINVAL;
            return -1;
    }

    /* Set command */
    cmd = (operation & LOCK_NB) ? F_SETLK : F_SETLKW;

    /* Apply lock */
    result = fcntl(fd, cmd, &fl);

    if (result < 0) {
        return -1;
    }

    return 0;
}

/*
 * lockf - Apply, test, atau remove POSIX lock
 *
 * Parameter:
 *   fd        - File descriptor
 *   cmd       - F_LOCK, F_TLOCK, F_ULOCK, F_TEST
 *   len       - Panjang lock (dari posisi saat ini)
 *
 * Return: 0 jika berhasil, -1 jika error
 */
int lockf(int fd, int cmd, off_t len) {
    struct flock fl;
    int fcntl_cmd;

    if (fd < 0) {
        errno = EBADF;
        return -1;
    }

    if (len < 0) {
        errno = EINVAL;
        return -1;
    }

    /* Setup flock structure */
    fl.l_whence = SEEK_CUR;
    fl.l_start = 0;
    fl.l_len = len;
    fl.l_pid = 0;

    /* Convert command */
    switch (cmd) {
        case F_LOCK:
            fl.l_type = F_WRLCK;
            fcntl_cmd = F_SETLKW;
            break;
        case F_TLOCK:
            fl.l_type = F_WRLCK;
            fcntl_cmd = F_SETLK;
            break;
        case F_ULOCK:
            fl.l_type = F_UNLCK;
            fcntl_cmd = F_SETLK;
            break;
        case F_TEST:
            fl.l_type = F_WRLCK;
            if (fcntl(fd, F_GETLK, &fl) < 0) {
                return -1;
            }
            if (fl.l_type == F_UNLCK) {
                return 0;  /* Tidak ada lock */
            }
            errno = EACCES;
            return -1;
        default:
            errno = EINVAL;
            return -1;
    }

    return fcntl(fd, fcntl_cmd, &fl);
}

/* ============================================================
 * FUNGSI DUP
 * ============================================================
 */

/*
 * dup - Duplicate file descriptor
 *
 * Parameter:
 *   oldfd - File descriptor lama
 *
 * Return: File descriptor baru, atau -1 jika error
 */
int dup(int oldfd) {
    if (oldfd < 0) {
        errno = EBADF;
        return -1;
    }

    return fcntl(oldfd, F_DUPFD, 0);
}

/*
 * dup2 - Duplicate file descriptor ke fd tertentu
 *
 * Parameter:
 *   oldfd - File descriptor lama
 *   newfd - File descriptor yang diinginkan
 *
 * Return: newfd jika berhasil, -1 jika error
 */
int dup2(int oldfd, int newfd) {
    if (oldfd < 0 || newfd < 0) {
        errno = EBADF;
        return -1;
    }

    /* Jika sama, tidak ada yang dilakukan */
    if (oldfd == newfd) {
        return newfd;
    }

    /* Gunakan F_DUPFD dengan newfd sebagai minimum */
    return fcntl(oldfd, F_DUPFD, newfd);
}

/*
 * dup3 - Duplicate file descriptor dengan flags
 *
 * Parameter:
 *   oldfd - File descriptor lama
 *   newfd - File descriptor yang diinginkan
 *   flags - Flags (O_CLOEXEC)
 *
 * Return: newfd jika berhasil, -1 jika error
 */
int dup3(int oldfd, int newfd, int flags) {
    int result;

    if (oldfd < 0 || newfd < 0) {
        errno = EBADF;
        return -1;
    }

    if (oldfd == newfd) {
        errno = EINVAL;
        return -1;
    }

    /* Gunakan F_DUPFD_CLOEXEC jika O_CLOEXEC set */
    if (flags & O_CLOEXEC) {
        result = fcntl(oldfd, F_DUPFD_CLOEXEC, newfd);
    } else {
        result = fcntl(oldfd, F_DUPFD, newfd);
    }

    /* Set flags tambahan jika diperlukan */
    if (result >= 0 && result == newfd) {
        if (flags & ~O_CLOEXEC) {
            int fl_flags = fcntl(result, F_GETFL);
            if (fl_flags >= 0) {
                fcntl(result, F_SETFL, fl_flags | (flags & ~O_CLOEXEC));
            }
        }
    }

    return result;
}

/* Fungsi fstat dideklarasikan di tempat lain */
extern int fstat(int fd, struct stat *buf);
