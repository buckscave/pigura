/*
 * PIGURA LIBC - UNISTD.H
 * =======================
 * Header POSIX untuk fungsi sistem operasi standar.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#ifndef LIBC_UNISTD_H
#define LIBC_UNISTD_H

/* ============================================================
 * DEPENDENSI
 * ============================================================
 */
#include <stddef.h>

/* ============================================================
 * NULL POINTER
 * ============================================================
 */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* ============================================================
 * KONSTANTA
 * ============================================================
 */

/* Standar POSIX */
#define _POSIX_VERSION     200809L
#define _POSIX_C_SOURCE    200809L

/* Nilai return error */
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/* Seek constants (sama dengan stdio.h) */
#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif

/* Options untuk access() */
#define F_OK  0   /* File exists */
#define X_OK  1   /* Execute permission */
#define W_OK  2   /* Write permission */
#define R_OK  4   /* Read permission */

/* Options untuk pathconf/fpathconf */
#define _PC_LINK_MAX         0
#define _PC_MAX_CANON        1
#define _PC_MAX_INPUT        2
#define _PC_NAME_MAX         3
#define _PC_PATH_MAX         4
#define _PC_PIPE_BUF         5
#define _PC_CHOWN_RESTRICTED 6
#define _PC_NO_TRUNC         7
#define _PC_VDISABLE         8
#define _PC_SYNC_IO          9
#define _PC_ASYNC_IO         10
#define _PC_PRIO_IO          11
#define _PC_FILESIZEBITS     12

/* Options untuk sysconf */
#define _SC_ARG_MAX          0
#define _SC_CHILD_MAX        1
#define _SC_CLK_TCK          2
#define _SC_NGROUPS_MAX      3
#define _SC_OPEN_MAX         4
#define _SC_STREAM_MAX       5
#define _SC_TZNAME_MAX       6
#define _SC_JOB_CONTROL      7
#define _SC_SAVED_IDS        8
#define _SC_REALTIME_SIGNALS 9
#define _SC_PRIORITY_SCHEDULING 10
#define _SC_TIMERS           11
#define _SC_ASYNCHRONOUS_IO  12
#define _SC_PRIORITIZED_IO   13
#define _SC_SYNCHRONIZED_IO  14
#define _SC_FSYNC            15
#define _SC_MAPPED_FILES     16
#define _SC_MEMLOCK          17
#define _SC_MEMLOCK_RANGE    18
#define _SC_MEMORY_PROTECTION 19
#define _SC_MESSAGE_PASSING  20
#define _SC_SEMAPHORES       21
#define _SC_SHARED_MEMORY_OBJECTS 22
#define _SC_AIO_LISTIO_MAX   23
#define _SC_AIO_MAX          24
#define _SC_AIO_PRIO_DELTA_MAX 25
#define _SC_DELAYTIMER_MAX   26
#define _SC_MQ_OPEN_MAX      27
#define _SC_MQ_PRIO_MAX      28
#define _SC_VERSION          29
#define _SC_PAGESIZE         30
#define _SC_PAGE_SIZE        30
#define _SC_RTSIG_MAX        31
#define _SC_SEM_NSEMS_MAX    32
#define _SC_SEM_VALUE_MAX    33
#define _SC_SIGQUEUE_MAX     34
#define _SC_TIMER_MAX        35
#define _SC_BC_BASE_MAX      36
#define _SC_BC_DIM_MAX       37
#define _SC_BC_SCALE_MAX     38
#define _SC_BC_STRING_MAX    39
#define _SC_COLL_WEIGHTS_MAX 40
#define _SC_EXPR_NEST_MAX    41
#define _SC_LINE_MAX         42
#define _SC_RE_DUP_MAX       43
#define _SC_2_VERSION        44
#define _SC_2_C_BIND         45
#define _SC_2_C_DEV          46
#define _SC_2_C_VERSION      47
#define _SC_2_FORT_DEV       48
#define _SC_2_FORT_RUN       49
#define _SC_2_LOCALEDEF      50
#define _SC_2_SW_DEV         51
#define _SC_2_UPE            52
#define _SC_NPROCESSORS_CONF 53
#define _SC_NPROCESSORS_ONLN 54
#define _SC_PHYS_PAGES       55
#define _SC_AVPHYS_PAGES     56
#define _SC_ATEXIT_MAX       57
#define _SC_PASS_MAX         58
#define _SC_XOPEN_VERSION    59
#define _SC_XOPEN_XCU_VERSION 60
#define _SC_XOPEN_UNIX       61
#define _SC_XOPEN_CRYPT      62
#define _SC_XOPEN_ENH_I18N   63
#define _SC_XOPEN_SHM        64
#define _SC_2_CHAR_TERM      65
#define _SC_2_UPE            66
#define _SC_XOPEN_XPG2       67
#define _SC_XOPEN_XPG3       68
#define _SC_XOPEN_XPG4       69
#define _SC_NZERO            70
#define _SC_XBS5_ILP32_OFF32 71
#define _SC_XBS5_ILP32_OFFBIG 72
#define _SC_XBS5_LP64_OFF64  73
#define _SC_XBS5_LPBIG_OFFBIG 74
#define _SC_SS_REPL_MAX      75
#define _SC_TRACE_EVENT_NAME_MAX 76
#define _SC_TRACE_NAME_MAX   77
#define _SC_TRACE_SYS_MAX    78
#define _SC_TRACE_USER_EVENT_MAX 79
#define _SC_TYPED_MEMORY_OBJECTS 80
#define _SC_V7_ILP32_OFF32   81
#define _SC_V7_ILP32_OFFBIG  82
#define _SC_V7_LP64_OFF64    83
#define _SC_V7_LPBIG_OFFBIG  84

/* Options untuk confstr */
#define _CS_PATH             0
#define _CS_POSIX_V7_WIDTH_RESTRICTED_ENVS 1
#define _CS_POSIX_V7_ILP32_OFF32_CFLAGS 2
#define _CS_POSIX_V7_ILP32_OFF32_LDFLAGS 3
#define _CS_POSIX_V7_ILP32_OFF32_LIBS 4
#define _CS_POSIX_V7_ILP32_OFFBIG_CFLAGS 5
#define _CS_POSIX_V7_ILP32_OFFBIG_LDFLAGS 6
#define _CS_POSIX_V7_ILP32_OFFBIG_LIBS 7
#define _CS_POSIX_V7_LP64_OFF64_CFLAGS 8
#define _CS_POSIX_V7_LP64_OFF64_LDFLAGS 9
#define _CS_POSIX_V7_LP64_OFF64_LIBS 10
#define _CS_POSIX_V7_LPBIG_OFFBIG_CFLAGS 11
#define _CS_POSIX_V7_LPBIG_OFFBIG_LDFLAGS 12
#define _CS_POSIX_V7_LPBIG_OFFBIG_LIBS 13
#define _CS_POSIX_V7_THREADS_CFLAGS 14
#define _CS_POSIX_V7_THREADS_LDFLAGS 15
#define _CS_POSIX_V7_WIDTH_RESTRICTED_ENVS 16
#define _CS_V7_ENV           17

/* ============================================================
 * TIPE DATA
 * ============================================================
 */

/* Tipe untuk PID */
typedef int pid_t;

/* Tipe untuk UID */
typedef unsigned int uid_t;

/* Tipe untuk GID */
typedef unsigned int gid_t;

/* Tipe untuk off_t */
typedef long long off_t;

/* Tipe untuk ssize_t */
typedef long ssize_t;

/* Tipe untuk useconds_t */
typedef unsigned int useconds_t;

/* Tipe untuk intptr_t */
typedef long intptr_t;

/* ============================================================
 * FUNGSI I/O
 * ============================================================
 */

/*
 * read - Baca dari file descriptor
 *
 * Parameter:
 *   fd   - File descriptor
 *   buf  - Buffer tujuan
 *   count - Jumlah byte maksimum
 *
 * Return: Jumlah byte yang dibaca, atau -1 jika error
 */
extern ssize_t read(int fd, void *buf, size_t count);

/*
 * write - Tulis ke file descriptor
 *
 * Parameter:
 *   fd    - File descriptor
 *   buf   - Buffer sumber
 *   count - Jumlah byte
 *
 * Return: Jumlah byte yang ditulis, atau -1 jika error
 */
extern ssize_t write(int fd, const void *buf, size_t count);

/*
 * pread - Baca dari posisi tertentu
 *
 * Parameter:
 *   fd    - File descriptor
 *   buf   - Buffer tujuan
 *   count - Jumlah byte
 *   offset - Posisi awal
 *
 * Return: Jumlah byte yang dibaca, atau -1 jika error
 */
extern ssize_t pread(int fd, void *buf, size_t count, off_t offset);

/*
 * pwrite - Tulis ke posisi tertentu
 *
 * Parameter:
 *   fd    - File descriptor
 *   buf   - Buffer sumber
 *   count - Jumlah byte
 *   offset - Posisi awal
 *
 * Return: Jumlah byte yang ditulis, atau -1 jika error
 */
extern ssize_t pwrite(int fd, const void *buf, size_t count,
                      off_t offset);

/*
 * close - Tutup file descriptor
 *
 * Parameter:
 *   fd - File descriptor
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int close(int fd);

/*
 * dup - Duplikasi file descriptor
 *
 * Parameter:
 *   fd - File descriptor
 *
 * Return: File descriptor baru, atau -1 jika error
 */
extern int dup(int fd);

/*
 * dup2 - Duplikasi ke file descriptor tertentu
 *
 * Parameter:
 *   fd  - File descriptor lama
 *   fd2 - File descriptor baru
 *
 * Return: fd2 jika berhasil, -1 jika error
 */
extern int dup2(int fd, int fd2);

/* ============================================================
 * FUNGSI FILE
 * ============================================================
 */

/*
 * open - Buka file
 *
 * Parameter:
 *   pathname - Nama file
 *   flags    - Flag pembukaan
 *   ...      - Mode (jika O_CREAT digunakan)
 *
 * Return: File descriptor, atau -1 jika error
 */
extern int open(const char *pathname, int flags, ...);

/*
 * creat - Buat file
 *
 * Parameter:
 *   pathname - Nama file
 *   mode     - Mode akses
 *
 * Return: File descriptor, atau -1 jika error
 */
extern int creat(const char *pathname, mode_t mode);

/*
 * access - Cek akses file
 *
 * Parameter:
 *   pathname - Nama file
 *   mode     - Mode akses (F_OK, R_OK, W_OK, X_OK)
 *
 * Return: 0 jika diizinkan, -1 jika tidak
 */
extern int access(const char *pathname, int mode);

/*
 * unlink - Hapus file
 *
 * Parameter:
 *   pathname - Nama file
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int unlink(const char *pathname);

/*
 * link - Buat hard link
 *
 * Parameter:
 *   oldpath - Path lama
 *   newpath - Path baru
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int link(const char *oldpath, const char *newpath);

/*
 * symlink - Buat symbolic link
 *
 * Parameter:
 *   target   - Target link
 *   linkpath - Path link
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int symlink(const char *target, const char *linkpath);

/*
 * readlink - Baca target symbolic link
 *
 * Parameter:
 *   pathname - Path link
 *   buf      - Buffer tujuan
 *   bufsiz   - Ukuran buffer
 *
 * Return: Jumlah byte yang dibaca, atau -1 jika error
 */
extern ssize_t readlink(const char *pathname, char *buf, size_t bufsiz);

/*
 * rename - Ubah nama file
 *
 * Parameter:
 *   oldpath - Nama lama
 *   newpath - Nama baru
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int rename(const char *oldpath, const char *newpath);

/*
 * truncate - Truncate file
 *
 * Parameter:
 *   path   - Nama file
 *   length - Panjang baru
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int truncate(const char *path, off_t length);

/*
 * ftruncate - Truncate file via descriptor
 *
 * Parameter:
 *   fd     - File descriptor
 *   length - Panjang baru
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int ftruncate(int fd, off_t length);

/* ============================================================
 * FUNGSI DIREKTORI
 * ============================================================
 */

/*
 * chdir - Ubah current working directory
 *
 * Parameter:
 *   path - Path direktori baru
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int chdir(const char *path);

/*
 * fchdir - Ubah cwd via file descriptor
 *
 * Parameter:
 *   fd - File descriptor direktori
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int fchdir(int fd);

/*
 * getcwd - Dapatkan current working directory
 *
 * Parameter:
 *   buf  - Buffer tujuan
 *   size - Ukuran buffer
 *
 * Return: Pointer ke buf, atau NULL jika error
 */
extern char *getcwd(char *buf, size_t size);

/*
 * mkdir - Buat direktori
 *
 * Parameter:
 *   pathname - Nama direktori
 *   mode     - Mode akses
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int mkdir(const char *pathname, mode_t mode);

/*
 * rmdir - Hapus direktori kosong
 *
 * Parameter:
 *   pathname - Nama direktori
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int rmdir(const char *pathname);

/* ============================================================
 * FUNGSI PROSES
 * ============================================================
 */

/*
 * getpid - Dapatkan process ID
 *
 * Return: PID proses saat ini
 */
extern pid_t getpid(void);

/*
 * getppid - Dapatkan parent process ID
 *
 * Return: PID proses parent
 */
extern pid_t getppid(void);

/*
 * getuid - Dapatkan user ID
 *
 * Return: UID proses saat ini
 */
extern uid_t getuid(void);

/*
 * geteuid - Dapatkan effective user ID
 *
 * Return: EUID proses saat ini
 */
extern uid_t geteuid(void);

/*
 * getgid - Dapatkan group ID
 *
 * Return: GID proses saat ini
 */
extern gid_t getgid(void);

/*
 * getegid - Dapatkan effective group ID
 *
 * Return: EGID proses saat ini
 */
extern gid_t getegid(void);

/*
 * setuid - Set user ID
 *
 * Parameter:
 *   uid - UID baru
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int setuid(uid_t uid);

/*
 * setgid - Set group ID
 *
 * Parameter:
 *   gid - GID baru
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int setgid(gid_t gid);

/*
 * fork - Buat proses baru
 *
 * Return: 0 di child, PID child di parent, -1 jika error
 */
extern pid_t fork(void);

/*
 * execve - Execute program
 *
 * Parameter:
 *   pathname - Path program
 *   argv     - Argumen
 *   envp     - Environment
 *
 * Return: -1 jika error (tidak return jika berhasil)
 */
extern int execve(const char *pathname, char *const argv[],
                  char *const envp[]);

/*
 * execv - Execute program dengan environment saat ini
 *
 * Parameter:
 *   pathname - Path program
 *   argv     - Argumen
 *
 * Return: -1 jika error
 */
extern int execv(const char *pathname, char *const argv[]);

/*
 * execvp - Execute program dengan PATH search
 *
 * Parameter:
 *   file - Nama program
 *   argv - Argumen
 *
 * Return: -1 jika error
 */
extern int execvp(const char *file, char *const argv[]);

/*
 * execvpe - Execute dengan PATH search dan custom env
 *
 * Parameter:
 *   file - Nama program
 *   argv - Argumen
 *   envp - Environment
 *
 * Return: -1 jika error
 */
extern int execvpe(const char *file, char *const argv[],
                   char *const envp[]);

/*
 * _exit - Exit tanpa cleanup
 *
 * Parameter:
 *   status - Exit status
 */
extern void _exit(int status) __attribute__((noreturn));

/*
 * sleep - Sleep dalam detik
 *
 * Parameter:
 *   seconds - Durasi sleep
 *
 * Return: 0 atau sisa waktu jika terinterupsi
 */
extern unsigned int sleep(unsigned int seconds);

/*
 * usleep - Sleep dalam microsecond
 *
 * Parameter:
 *   usec - Durasi sleep dalam microsecond
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int usleep(useconds_t usec);

/*
 * alarm - Set alarm signal
 *
 * Parameter:
 *   seconds - Waktu alarm (0 untuk cancel)
 *
 * Return: Waktu sisa alarm sebelumnya
 */
extern unsigned int alarm(unsigned int seconds);

/*
 * pause - Tunggu signal
 *
 * Return: -1 dengan errno=EINTR setelah signal
 */
extern int pause(void);

/* ============================================================
 * FUNGSI SISTEM
 * ============================================================
 */

/*
 * sysconf - Dapatkan konfigurasi sistem
 *
 * Parameter:
 *   name - Nama konfigurasi (_SC_*)
 *
 * Return: Nilai konfigurasi, atau -1 jika error
 */
extern long sysconf(int name);

/*
 * pathconf - Dapatkan konfigurasi path
 *
 * Parameter:
 *   path - Path yang diperiksa
 *   name - Nama konfigurasi (_PC_*)
 *
 * Return: Nilai konfigurasi, atau -1 jika error
 */
extern long pathconf(const char *path, int name);

/*
 * fpathconf - Dapatkan konfigurasi via fd
 *
 * Parameter:
 *   fd   - File descriptor
 *   name - Nama konfigurasi
 *
 * Return: Nilai konfigurasi, atau -1 jika error
 */
extern long fpathconf(int fd, int name);

/*
 * confstr - Dapatkan string konfigurasi
 *
 * Parameter:
 *   name  - Nama konfigurasi (_CS_*)
 *   buf   - Buffer tujuan
 *   len   - Ukuran buffer
 *
 * Return: Panjang string yang diperlukan
 */
extern size_t confstr(int name, char *buf, size_t len);

/*
 * gethostname - Dapatkan hostname sistem
 *
 * Parameter:
 *   name - Buffer tujuan
 *   len  - Ukuran buffer
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int gethostname(char *name, size_t len);

/*
 * sethostname - Set hostname sistem
 *
 * Parameter:
 *   name - Hostname baru
 *   len  - Panjang hostname
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int sethostname(const char *name, size_t len);

/*
 * getpagesize - Dapatkan ukuran halaman memori
 *
 * Return: Ukuran halaman dalam byte
 */
extern int getpagesize(void);

/*
 * sync - Flush semua buffer filesystem
 */
extern void sync(void);

/*
 * fsync - Flush buffer file ke disk
 *
 * Parameter:
 *   fd - File descriptor
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int fsync(int fd);

/*
 * fdatasync - Flush data file ke disk (tanpa metadata)
 *
 * Parameter:
 *   fd - File descriptor
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int fdatasync(int fd);

/* ============================================================
 * FUNGSI PIPE
 * ============================================================
 */

/*
 * pipe - Buat pipe
 *
 * Parameter:
 *   pipefd - Array untuk menyimpan fd [read, write]
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int pipe(int pipefd[2]);

/*
 * pipe2 - Buat pipe dengan flags
 *
 * Parameter:
 *   pipefd - Array untuk menyimpan fd
 *   flags  - Flags (O_CLOEXEC, O_DIRECT, O_NONBLOCK)
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int pipe2(int pipefd[2], int flags);

#endif /* LIBC_UNISTD_H */
