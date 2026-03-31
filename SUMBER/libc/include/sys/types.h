/*
 * PIGURA LIBC - SYS/TYPES.H
 * ==========================
 * Header untuk tipe data sistem sesuai standar POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.1 - Perbaikan duplikasi dan penambahan tipe
 */

#ifndef LIBC_SYS_TYPES_H
#define LIBC_SYS_TYPES_H

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
 * TIPE DASAR SISTEM
 * ============================================================
 */

/* Tipe untuk Process ID */
#ifndef _PID_T_DEFINED
#define _PID_T_DEFINED
typedef int pid_t;
#endif

/* Tipe untuk Thread ID */
#ifndef _PTHREAD_T_DEFINED
#define _PTHREAD_T_DEFINED
typedef unsigned long pthread_t;
#endif

/* Tipe untuk User ID */
#ifndef _UID_T_DEFINED
#define _UID_T_DEFINED
typedef unsigned int uid_t;
#endif

/* Tipe untuk Group ID */
#ifndef _GID_T_DEFINED
#define _GID_T_DEFINED
typedef unsigned int gid_t;
#endif

/* Tipe untuk Device ID */
#ifndef _DEV_T_DEFINED
#define _DEV_T_DEFINED
typedef unsigned long dev_t;
#endif

/* Tipe untuk Inode number */
#ifndef _INO_T_DEFINED
#define _INO_T_DEFINED
typedef unsigned long long ino_t;
#endif

/* Tipe untuk Mode permission */
#ifndef _MODE_T_DEFINED
#define _MODE_T_DEFINED
typedef unsigned int mode_t;
#endif

/* Tipe untuk Number of links */
#ifndef _NLINK_T_DEFINED
#define _NLINK_T_DEFINED
typedef unsigned int nlink_t;
#endif

/* Tipe untuk Block count */
#ifndef _BLKCNT_T_DEFINED
#define _BLKCNT_T_DEFINED
typedef long long blkcnt_t;
#endif

/* Tipe untuk Block size */
#ifndef _BLKSIZE_T_DEFINED
#define _BLKSIZE_T_DEFINED
typedef long blksize_t;
#endif

/* Tipe untuk File offset */
#ifndef _OFF_T_DEFINED
#define _OFF_T_DEFINED
typedef long long off_t;
#endif

/* Tipe untuk Signed size */
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef long ssize_t;
#endif

/* Tipe untuk Clock ID */
#ifndef _CLOCKID_T_DEFINED
#define _CLOCKID_T_DEFINED
typedef int clockid_t;
#endif

/* Tipe untuk Timer */
#ifndef _TIMER_T_DEFINED
#define _TIMER_T_DEFINED
typedef int timer_t;
#endif

/* Tipe untuk Key IPC */
typedef int key_t;

/* Tipe untuk Message Queue ID */
typedef int mqd_t;

/* Tipe untuk Semaphore */
typedef int sem_t;

/* Tipe untuk File descriptor */
typedef int fd_t;

/* Tipe untuk Socket */
typedef int sock_t;

/* Tipe untuk usecond */
#ifndef _USECONDS_T_DEFINED
#define _USECONDS_T_DEFINED
typedef unsigned int useconds_t;
#endif

/* Tipe untuk second (signed) */
#ifndef _SUSECONDS_T_DEFINED
#define _SUSECONDS_T_DEFINED
typedef long suseconds_t;
#endif

/* Tipe untuk pthread attribute */
#ifndef _PTHREAD_ATTR_T_DEFINED
#define _PTHREAD_ATTR_T_DEFINED
typedef unsigned long pthread_attr_t;
#endif

/* Tipe untuk pthread mutex */
typedef struct {
    int __lock;
    unsigned int __count;
    unsigned long __owner;
    unsigned int __kind;
    unsigned long __nusers;
} pthread_mutex_t;

/* Tipe untuk pthread mutex attribute */
typedef struct {
    int __prioceiling;
    int __protocol;
    int __pshared;
    int __type;
} pthread_mutexattr_t;

/* Tipe untuk pthread condition */
typedef struct {
    int __lock;
    unsigned long __futex;
    unsigned long long __total_seq;
    unsigned long long __wakeup_seq;
    unsigned long long __woken_seq;
    void *__mutex;
    unsigned int __nwaiters;
} pthread_cond_t;

/* Tipe untuk pthread condition attribute */
typedef struct {
    int __pshared;
    int __clockid;
} pthread_condattr_t;

/* Tipe untuk pthread_rwlock */
typedef struct {
    int __lock;
    unsigned int __readers;
    unsigned int __writer;
    unsigned int __readers_wakeup;
    unsigned int __writer_wakeup;
    unsigned int __nr_readers_queued;
    unsigned int __nr_writers_queued;
} pthread_rwlock_t;

/* Tipe untuk pthread key */
typedef unsigned long pthread_key_t;

/* Tipe untuk pthread once */
typedef int pthread_once_t;

/* Tipe untuk ID types */
typedef unsigned int id_t;

/* ============================================================
 * STRUKTUR IOVEC
 * ============================================================
 * Struktur untuk scatter/gather I/O
 */
struct iovec {
    void  *iov_base;  /* Starting address */
    size_t iov_len;   /* Number of bytes */
};

/* ============================================================
 * KONSTANTA UKURAN
 * ============================================================
 */

/* Ukuran maksimum nama */
#ifndef NAME_MAX
#define NAME_MAX         255
#endif
#ifndef PATH_MAX
#define PATH_MAX         4096
#endif
#ifndef FILENAME_MAX
#define FILENAME_MAX     4096
#endif

/* Ukuran maksimum pipe buffer */
#define PIPE_BUF         4096

/* Ukuran maksimum argument list */
#ifndef ARG_MAX
#define ARG_MAX          131072
#endif

/* Ukuran maksimum child processes */
#define CHILD_MAX        1024

/* Ukuran maksimum open files */
#ifndef OPEN_MAX
#define OPEN_MAX         1024
#endif

/* ============================================================
 * TIPE CPU SET
 * ============================================================
 */
typedef struct {
    unsigned long __bits[16];
} cpu_set_t;

/* CPU set operations */
#define CPU_SETSIZE      1024

#define CPU_SET(cpu, set) \
    ((set)->__bits[(cpu) / (8 * sizeof(unsigned long))] |= \
     (1UL << ((cpu) % (8 * sizeof(unsigned long)))))

#define CPU_CLR(cpu, set) \
    ((set)->__bits[(cpu) / (8 * sizeof(unsigned long))] &= \
     ~(1UL << ((cpu) % (8 * sizeof(unsigned long)))))

#define CPU_ISSET(cpu, set) \
    (((set)->__bits[(cpu) / (8 * sizeof(unsigned long))] & \
      (1UL << ((cpu) % (8 * sizeof(unsigned long))))) != 0)

#define CPU_ZERO(set) \
    do { \
        int __i; \
        for (__i = 0; __i < 16; __i++) \
            (set)->__bits[__i] = 0; \
    } while (0)

#define CPU_COUNT(set) \
    ({ \
        int __count = 0, __i; \
        for (__i = 0; __i < 16; __i++) { \
            unsigned long __bits = (set)->__bits[__i]; \
            while (__bits) { \
                __count += __bits & 1; \
                __bits >>= 1; \
            } \
        } \
        __count; \
    })

/* ============================================================
 * MAKRO UNTUK MODE/TYPE (hanya definisi unik)
 * ============================================================
 * Catatan: Definisi lengkap S_* ada di sys/stat.h
 */

/* Mask untuk file type */
#define S_IFMT      0170000   /* Mask untuk file type */

/* File types */
#define S_IFSOCK    0140000   /* Socket */
#define S_IFLNK     0120000   /* Symbolic link */
#define S_IFREG     0100000   /* Regular file */
#define S_IFBLK     0060000   /* Block device */
#define S_IFDIR     0040000   /* Directory */
#define S_IFCHR     0020000   /* Character device */
#define S_IFIFO     0010000   /* FIFO */

/* Makro untuk cek file type */
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

#endif /* LIBC_SYS_TYPES_H */
