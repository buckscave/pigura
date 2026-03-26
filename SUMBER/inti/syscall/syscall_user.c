/*
 * PIGURA OS - SYSCALL_USER.C
 * ---------------------------
 * Implementasi wrapper syscall untuk user space.
 *
 * Berkas ini berisi fungsi-fungsi wrapper yang memudahkan
 * program user space untuk memanggil system call.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../kernel.h"

/*
 * ============================================================================
 * MAKRO SYSCALL (SYSCALL MACROS)
 * ============================================================================
 */

/* Nomor interrupt untuk syscall */
#define SYSCALL_INT             0x80

/* Macro untuk melakukan syscall */
#define SYSCALL0(num) \
    ({ \
        long __res; \
        __asm__ __volatile__( \
            "int $0x80" \
            : "=a"(__res) \
            : "a"(num) \
            : "memory" \
        ); \
        __res; \
    })

#define SYSCALL1(num, a1) \
    ({ \
        long __res; \
        __asm__ __volatile__( \
            "int $0x80" \
            : "=a"(__res) \
            : "a"(num), "b"((long)(a1)) \
            : "memory" \
        ); \
        __res; \
    })

#define SYSCALL2(num, a1, a2) \
    ({ \
        long __res; \
        __asm__ __volatile__( \
            "int $0x80" \
            : "=a"(__res) \
            : "a"(num), "b"((long)(a1)), "c"((long)(a2)) \
            : "memory" \
        ); \
        __res; \
    })

#define SYSCALL3(num, a1, a2, a3) \
    ({ \
        long __res; \
        __asm__ __volatile__( \
            "int $0x80" \
            : "=a"(__res) \
            : "a"(num), "b"((long)(a1)), "c"((long)(a2)), \
              "d"((long)(a3)) \
            : "memory" \
        ); \
        __res; \
    })

#define SYSCALL4(num, a1, a2, a3, a4) \
    ({ \
        long __res; \
        __asm__ __volatile__( \
            "int $0x80" \
            : "=a"(__res) \
            : "a"(num), "b"((long)(a1)), "c"((long)(a2)), \
              "d"((long)(a3)), "S"((long)(a4)) \
            : "memory" \
        ); \
        __res; \
    })

#define SYSCALL5(num, a1, a2, a3, a4, a5) \
    ({ \
        long __res; \
        __asm__ __volatile__( \
            "int $0x80" \
            : "=a"(__res) \
            : "a"(num), "b"((long)(a1)), "c"((long)(a2)), \
              "d"((long)(a3)), "S"((long)(a4)), "D"((long)(a5)) \
            : "memory" \
        ); \
        __res; \
    })

#define SYSCALL6(num, a1, a2, a3, a4, a5, a6) \
    ({ \
        long __res; \
        register long __a6 __asm__("ebp") = (long)(a6); \
        __asm__ __volatile__( \
            "int $0x80" \
            : "=a"(__res) \
            : "a"(num), "b"((long)(a1)), "c"((long)(a2)), \
              "d"((long)(a3)), "S"((long)(a4)), "D"((long)(a5)), \
              "r"(__a6) \
            : "memory" \
        ); \
        __res; \
    })

/* Cek error dari syscall */
#define SYSCALL_CHECK(res) \
    do { \
        if ((res) < 0 && (res) >= -255) { \
            errno = -(res); \
            return -1; \
        } \
    } while (0)

/*
 * ============================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ============================================================================
 */

/* Global errno */
int errno = 0;

/*
 * ============================================================================
 * FUNGSI WRAPPER SYSCALL (SYSCALL WRAPPER FUNCTIONS)
 * ============================================================================
 */

/*
 * Proses Management
 * -----------------
 */

/* Fork proses baru */
pid_t fork(void)
{
    long res;

    res = SYSCALL0(SYS_FORK);

    if (res < 0) {
        errno = (int)-res;
        return -1;
    }

    return (pid_t)res;
}

/* Exec program baru */
int execve(const char *path, char *const argv[], char *const envp[])
{
    long res;

    res = SYSCALL3(SYS_EXEC, path, argv, envp);

    SYSCALL_CHECK(res);

    return (int)res;
}

/* Exit proses */
void exit(int status)
{
    SYSCALL1(SYS_EXIT, status);

    /* Should not return */
    while (1) {
        __asm__ __volatile__("hlt");
    }
}

/* Dapatkan PID */
pid_t getpid(void)
{
    long res;

    res = SYSCALL0(SYS_GETPID);

    SYSCALL_CHECK(res);

    return (pid_t)res;
}

/* Dapatkan PPID */
pid_t getppid(void)
{
    long res;

    res = SYSCALL0(SYS_GETPPID);

    SYSCALL_CHECK(res);

    return (pid_t)res;
}

/* Wait untuk child */
pid_t wait(int *status)
{
    long res;

    res = SYSCALL1(SYS_WAIT, status);

    SYSCALL_CHECK(res);

    return (pid_t)res;
}

/* Wait untuk child tertentu */
pid_t waitpid(pid_t pid, int *status, int options)
{
    long res;

    res = SYSCALL3(SYS_WAITPID, pid, status, options);

    SYSCALL_CHECK(res);

    return (pid_t)res;
}

/*
 * Memory Management
 * -----------------
 */

/* Brk system call */
void *sbrk(ptrdiff_t increment)
{
    long res;

    res = SYSCALL1(SYS_SBRK, increment);

    SYSCALL_CHECK(res);

    return (void *)res;
}

/* Brk system call */
int brk(void *addr)
{
    long res;

    res = SYSCALL1(SYS_BRK, addr);

    SYSCALL_CHECK(res);

    return (int)res;
}

/* mmap system call */
void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
    long res;

    res = SYSCALL6(SYS_MMAP, addr, len, prot, flags, fd, offset);

    SYSCALL_CHECK(res);

    return (void *)res;
}

/* munmap system call */
int munmap(void *addr, size_t len)
{
    long res;

    res = SYSCALL2(SYS_MUNMAP, addr, len);

    SYSCALL_CHECK(res);

    return (int)res;
}

/*
 * File Operations
 * ---------------
 */

/* Buka file */
int open(const char *path, int flags, ...)
{
    long res;
    mode_t mode = 0;

    /* Ambil mode jika O_CREAT */
    if (flags & 0x100) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }

    res = SYSCALL3(SYS_OPEN, path, flags, mode);

    SYSCALL_CHECK(res);

    return (int)res;
}

/* Tutup file */
int close(int fd)
{
    long res;

    res = SYSCALL1(SYS_CLOSE, fd);

    SYSCALL_CHECK(res);

    return (int)res;
}

/* Baca file */
ssize_t read(int fd, void *buf, size_t count)
{
    long res;

    res = SYSCALL3(SYS_READ, fd, buf, count);

    SYSCALL_CHECK(res);

    return (ssize_t)res;
}

/* Tulis file */
ssize_t write(int fd, const void *buf, size_t count)
{
    long res;

    res = SYSCALL3(SYS_WRITE, fd, buf, count);

    SYSCALL_CHECK(res);

    return (ssize_t)res;
}

/* Seek file */
off_t lseek(int fd, off_t offset, int whence)
{
    long res;

    res = SYSCALL3(SYS_LSEEK, fd, offset, whence);

    SYSCALL_CHECK(res);

    return (off_t)res;
}

/* Stat file */
int stat(const char *path, struct stat *statbuf)
{
    long res;

    res = SYSCALL2(SYS_STAT, path, statbuf);

    SYSCALL_CHECK(res);

    return (int)res;
}

/* Fstat file */
int fstat(int fd, struct stat *statbuf)
{
    long res;

    res = SYSCALL2(SYS_FSTAT, fd, statbuf);

    SYSCALL_CHECK(res);

    return (int)res;
}

/* Dup file descriptor */
int dup(int oldfd)
{
    long res;

    res = SYSCALL1(SYS_DUP, oldfd);

    SYSCALL_CHECK(res);

    return (int)res;
}

/* Dup2 file descriptor */
int dup2(int oldfd, int newfd)
{
    long res;

    res = SYSCALL2(SYS_DUP2, oldfd, newfd);

    SYSCALL_CHECK(res);

    return (int)res;
}

/* Buat pipe */
int pipe(int pipefd[2])
{
    long res;

    res = SYSCALL1(SYS_PIPE, pipefd);

    SYSCALL_CHECK(res);

    return (int)res;
}

/*
 * Directory Operations
 * --------------------
 */

/* Buat direktori */
int mkdir(const char *path, mode_t mode)
{
    long res;

    res = SYSCALL2(SYS_MKDIR, path, mode);

    SYSCALL_CHECK(res);

    return (int)res;
}

/* Hapus direktori */
int rmdir(const char *path)
{
    long res;

    res = SYSCALL1(SYS_RMDIR, path);

    SYSCALL_CHECK(res);

    return (int)res;
}

/* Ubah direktori kerja */
int chdir(const char *path)
{
    long res;

    res = SYSCALL1(SYS_CHDIR, path);

    SYSCALL_CHECK(res);

    return (int)res;
}

/* Dapatkan direktori kerja */
char *getcwd(char *buf, size_t size)
{
    long res;

    res = SYSCALL2(SYS_GETCWD, buf, size);

    if (res < 0) {
        errno = (int)-res;
        return NULL;
    }

    return buf;
}

/* Hapus file */
int unlink(const char *path)
{
    long res;

    res = SYSCALL1(SYS_UNLINK, path);

    SYSCALL_CHECK(res);

    return (int)res;
}

/*
 * Signal Operations
 * -----------------
 */

/* Kirim sinyal */
int kill(pid_t pid, int sig)
{
    long res;

    res = SYSCALL2(SYS_KILL, pid, sig);

    SYSCALL_CHECK(res);

    return (int)res;
}

/* Set signal handler */
sighandler_t signal(int sig, sighandler_t handler)
{
    long res;

    res = SYSCALL2(SYS_SIGNAL, sig, handler);

    if (res < 0) {
        errno = (int)-res;
        return SIG_ERR;
    }

    return (sighandler_t)res;
}

/*
 * Time Operations
 * ---------------
 */

/* Dapatkan waktu */
time_t time(time_t *tloc)
{
    long res;

    res = SYSCALL1(SYS_TIME, tloc);

    SYSCALL_CHECK(res);

    return (time_t)res;
}

/* Sleep */
unsigned int sleep(unsigned int seconds)
{
    long res;

    res = SYSCALL1(SYS_SLEEP, seconds);

    if (res < 0) {
        errno = (int)-res;
        return seconds;
    }

    return (unsigned int)res;
}

/* Usleep */
int usleep(useconds_t usec)
{
    long res;

    res = SYSCALL1(SYS_USLEEP, usec);

    SYSCALL_CHECK(res);

    return (int)res;
}

/* Alarm */
unsigned int alarm(unsigned int seconds)
{
    long res;

    res = SYSCALL1(SYS_ALARM, seconds);

    SYSCALL_CHECK(res);

    return (unsigned int)res;
}

/*
 * User/Group Operations
 * ---------------------
 */

/* Dapatkan UID */
uid_t getuid(void)
{
    long res;

    res = SYSCALL0(SYS_GETUID);

    SYSCALL_CHECK(res);

    return (uid_t)res;
}

/* Dapatkan GID */
gid_t getgid(void)
{
    long res;

    res = SYSCALL0(SYS_GETGID);

    SYSCALL_CHECK(res);

    return (gid_t)res;
}

/* Dapatkan EUID */
uid_t geteuid(void)
{
    long res;

    res = SYSCALL0(SYS_GETEUID);

    SYSCALL_CHECK(res);

    return (uid_t)res;
}

/* Dapatkan EGID */
gid_t getegid(void)
{
    long res;

    res = SYSCALL0(SYS_GETEGID);

    SYSCALL_CHECK(res);

    return (gid_t)res;
}

/* Set UID */
int setuid(uid_t uid)
{
    long res;

    res = SYSCALL1(SYS_SETUID, uid);

    SYSCALL_CHECK(res);

    return (int)res;
}

/* Set GID */
int setgid(gid_t gid)
{
    long res;

    res = SYSCALL1(SYS_SETGID, gid);

    SYSCALL_CHECK(res);

    return (int)res;
}

/*
 * Scheduler Operations
 * --------------------
 */

/* Yield CPU */
int sched_yield(void)
{
    long res;

    res = SYSCALL0(SYS_YIELD);

    SYSCALL_CHECK(res);

    return (int)res;
}

/* Nice */
int nice(int inc)
{
    long res;

    res = SYSCALL1(SYS_NICE, inc);

    SYSCALL_CHECK(res);

    return (int)res;
}

/*
 * System Information
 * ------------------
 */

/* Uname */
int uname(struct utsname *buf)
{
    long res;

    res = SYSCALL1(SYS_UNAME, buf);

    SYSCALL_CHECK(res);

    return (int)res;
}

/*
 * Utility Functions
 * -----------------
 */

/* Dapatkan errno */
int *__errno_location(void)
{
    return &errno;
}

/* Print error message */
void perror(const char *s)
{
    const char *err_str;
    static const char *err_strings[] = {
        [0]         = "Success",
        [EPERM]     = "Operation not permitted",
        [ENOENT]    = "No such file or directory",
        [ESRCH]     = "No such process",
        [EINTR]     = "Interrupted system call",
        [EIO]       = "I/O error",
        [ENXIO]     = "No such device or address",
        [E2BIG]     = "Argument list too long",
        [ENOEXEC]   = "Exec format error",
        [EBADF]     = "Bad file number",
        [ECHILD]    = "No child processes",
        [EAGAIN]    = "Try again",
        [ENOMEM]    = "Out of memory",
        [EACCES]    = "Permission denied",
        [EFAULT]    = "Bad address",
        [ENOTBLK]   = "Block device required",
        [EBUSY]     = "Device or resource busy",
        [EEXIST]    = "File exists",
        [EXDEV]     = "Cross-device link",
        [ENODEV]    = "No such device",
        [ENOTDIR]   = "Not a directory",
        [EISDIR]    = "Is a directory",
        [EINVAL]    = "Invalid argument",
        [ENFILE]    = "File table overflow",
        [EMFILE]    = "Too many open files",
        [ENOTTY]    = "Not a typewriter",
        [ETXTBSY]   = "Text file busy",
        [EFBIG]     = "File too large",
        [ENOSPC]    = "No space left on device",
        [ESPIPE]    = "Illegal seek",
        [EROFS]     = "Read-only file system",
        [EMLINK]    = "Too many links",
        [EPIPE]     = "Broken pipe",
        [EDOM]      = "Math argument out of domain",
        [ERANGE]    = "Math result not representable"
    };

    if (errno >= 0 && errno < (int)(sizeof(err_strings) /
        sizeof(err_strings[0])) && err_strings[errno] != NULL) {
        err_str = err_strings[errno];
    } else {
        err_str = "Unknown error";
    }

    if (s != NULL && *s != '\0') {
        write(2, s, strlen(s));
        write(2, ": ", 2);
    }

    write(2, err_str, strlen(err_str));
    write(2, "\n", 1);
}

/* Dapatkan string error */
char *strerror(int errnum)
{
    static const char *err_strings[] = {
        [0]         = "Success",
        [EPERM]     = "Operation not permitted",
        [ENOENT]    = "No such file or directory",
        [ESRCH]     = "No such process",
        [EINTR]     = "Interrupted system call",
        [EIO]       = "I/O error",
        [ENXIO]     = "No such device or address",
        [E2BIG]     = "Argument list too long",
        [ENOEXEC]   = "Exec format error",
        [EBADF]     = "Bad file number",
        [ECHILD]    = "No child processes",
        [EAGAIN]    = "Try again",
        [ENOMEM]    = "Out of memory",
        [EACCES]    = "Permission denied",
        [EFAULT]    = "Bad address"
    };

    if (errnum >= 0 && errnum < (int)(sizeof(err_strings) /
        sizeof(err_strings[0])) && err_strings[errnum] != NULL) {
        return (char *)err_strings[errnum];
    }

    return "Unknown error";
}
