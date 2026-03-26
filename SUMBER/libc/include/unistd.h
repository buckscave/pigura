/*
 * PIGURA LIBC - UNISTD.H
 * =======================
 * Header POSIX untuk fungsi sistem operasi standar.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.1 - Perbaikan duplikasi dan deklarasi
 */

#ifndef LIBC_UNISTD_H
#define LIBC_UNISTD_H

/* ============================================================
 * DEPENDENSI
 * ============================================================
 */
#include <stddef.h>
#include <sys/types.h>

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

/* Seek constants */
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
#define _SC_XOPEN_XPG2       66
#define _SC_XOPEN_XPG3       67
#define _SC_XOPEN_XPG4       68
#define _SC_NZERO            69
#define _SC_XBS5_ILP32_OFF32 70
#define _SC_XBS5_ILP32_OFFBIG 71
#define _SC_XBS5_LP64_OFF64  72
#define _SC_XBS5_LPBIG_OFFBIG 73
#define _SC_SS_REPL_MAX      74
#define _SC_TRACE_EVENT_NAME_MAX 75
#define _SC_TRACE_NAME_MAX   76
#define _SC_TRACE_SYS_MAX    77
#define _SC_TRACE_USER_EVENT_MAX 78
#define _SC_TYPED_MEMORY_OBJECTS 79
#define _SC_V7_ILP32_OFF32   80
#define _SC_V7_ILP32_OFFBIG  81
#define _SC_V7_LP64_OFF64    82
#define _SC_V7_LPBIG_OFFBIG  83

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
#define _CS_V7_ENV           16

/* ============================================================
 * FUNGSI I/O
 * ============================================================
 */

extern ssize_t read(int fd, void *buf, size_t count);
extern ssize_t write(int fd, const void *buf, size_t count);
extern ssize_t pread(int fd, void *buf, size_t count, off_t offset);
extern ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);
extern int close(int fd);
extern int dup(int fd);
extern int dup2(int fd, int fd2);
extern off_t lseek(int fd, off_t offset, int whence);

/* ============================================================
 * FUNGSI FILE
 * ============================================================
 */

extern int open(const char *pathname, int flags, ...);
extern int creat(const char *pathname, mode_t mode);
extern int access(const char *pathname, int mode);
extern int unlink(const char *pathname);
extern int link(const char *oldpath, const char *newpath);
extern int symlink(const char *target, const char *linkpath);
extern ssize_t readlink(const char *pathname, char *buf, size_t bufsiz);
extern int rename(const char *oldpath, const char *newpath);
extern int truncate(const char *path, off_t length);
extern int ftruncate(int fd, off_t length);

/* ============================================================
 * FUNGSI DIREKTORI
 * ============================================================
 */

extern int chdir(const char *path);
extern int fchdir(int fd);
extern char *getcwd(char *buf, size_t size);
extern int mkdir(const char *pathname, mode_t mode);
extern int rmdir(const char *pathname);

/* ============================================================
 * FUNGSI PROSES
 * ============================================================
 */

extern pid_t getpid(void);
extern pid_t getppid(void);
extern uid_t getuid(void);
extern uid_t geteuid(void);
extern gid_t getgid(void);
extern gid_t getegid(void);
extern int setuid(uid_t uid);
extern int setgid(gid_t gid);
extern pid_t fork(void);
extern int execve(const char *pathname, char *const argv[], char *const envp[]);
extern int execv(const char *pathname, char *const argv[]);
extern int execvp(const char *file, char *const argv[]);
extern int execvpe(const char *file, char *const argv[], char *const envp[]);
extern void _exit(int status);
extern unsigned int sleep(unsigned int seconds);
extern int usleep(useconds_t usec);
extern unsigned int alarm(unsigned int seconds);
extern int pause(void);

/* ============================================================
 * FUNGSI SISTEM
 * ============================================================
 */

extern long sysconf(int name);
extern long pathconf(const char *path, int name);
extern long fpathconf(int fd, int name);
extern size_t confstr(int name, char *buf, size_t len);
extern int gethostname(char *name, size_t len);
extern int sethostname(const char *name, size_t len);
extern int getpagesize(void);
extern void sync(void);
extern int fsync(int fd);
extern int fdatasync(int fd);

/* ============================================================
 * FUNGSI PIPE
 * ============================================================
 */

extern int pipe(int pipefd[2]);
extern int pipe2(int pipefd[2], int flags);

#endif /* LIBC_UNISTD_H */
