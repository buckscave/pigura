/*
 * PIGURA OS - STAT.H
 * ===================
 * Definisi struktur stat untuk kernel.
 *
 * PENTING: Struktur ini HARUS sama dengan yang di libc/include/sys/stat.h
 * untuk memastikan kompatibilitas antara kernel dan userspace.
 *
 * Versi: 1.0
 */

#ifndef INTI_STAT_H
#define INTI_STAT_H

/* ============================================================
 * DEPENDENSI
 * ============================================================
 */
#include "types.h"

/* ============================================================
 * TIPE DATA STAT
 * ============================================================
 * Tipe-tipe yang digunakan dalam struktur stat.
 */

#ifndef dev_t
typedef unsigned long   dev_t;      /* Device ID */
#endif

#ifndef ino_t
typedef unsigned long long ino_t;  /* Inode number */
#endif

#ifndef mode_t
typedef unsigned int    mode_t;     /* Mode permission */
#endif

#ifndef nlink_t
typedef unsigned int    nlink_t;    /* Number of hard links */
#endif

#ifndef uid_t
typedef unsigned int    uid_t;      /* User ID */
#endif

#ifndef gid_t
typedef unsigned int    gid_t;      /* Group ID */
#endif

#ifndef off_t
typedef long long       off_t;      /* File offset/size */
#endif

#ifndef blksize_t
typedef long            blksize_t;  /* Block size */
#endif

#ifndef blkcnt_t
typedef long long       blkcnt_t;   /* Block count */
#endif

/* ============================================================
 * STRUKTUR TIMESPEC
 * ============================================================
 */
struct k_timespec {
    long tv_sec;   /* Detik */
    long tv_nsec;  /* Nanodetik (0-999999999) */
};

/* ============================================================
 * STRUKTUR STAT
 * ============================================================
 * Struktur untuk menyimpan informasi file.
 *
 * PENTING: Harus konsisten dengan libc!
 */
struct kstat {
    dev_t       st_dev;     /* Device ID */
    ino_t       st_ino;     /* Inode number */
    mode_t      st_mode;    /* Mode permission dan file type */
    nlink_t     st_nlink;   /* Number of hard links */
    uid_t       st_uid;     /* User ID owner */
    gid_t       st_gid;     /* Group ID owner */
    dev_t       st_rdev;    /* Device ID (untuk device special) */
    off_t       st_size;    /* Ukuran total dalam byte */
    blksize_t   st_blksize; /* Blocksize untuk I/O */
    blkcnt_t    st_blocks;  /* Jumlah 512-byte blocks */
    
    /* Timestamps */
    struct k_timespec st_atim;  /* Waktu akses terakhir */
    struct k_timespec st_mtim;  /* Waktu modifikasi terakhir */
    struct k_timespec st_ctim;  /* Waktu perubahan status */
    
    /* Reserved untuk padding/alignment */
    unsigned long   __reserved[3];
};

/* ============================================================
 * KONSTANTA MODE
 * ============================================================
 */

/* Tipe file */
#define S_IFMT      0xF000  /* Mask untuk tipe file */
#define S_IFSOCK    0xC000  /* Socket */
#define S_IFLNK     0xA000  /* Symbolic link */
#define S_IFREG     0x8000  /* Regular file */
#define S_IFBLK     0x6000  /* Block device */
#define S_IFDIR     0x4000  /* Directory */
#define S_IFCHR     0x2000  /* Character device */
#define S_IFIFO     0x1000  /* FIFO */

/* Permission bits */
#define S_ISUID     0x0800  /* Set UID */
#define S_ISGID     0x0400  /* Set GID */
#define S_ISVTX     0x0200  /* Sticky bit */

#define S_IRWXU     0x01C0  /* Owner: rwx */
#define S_IRUSR     0x0100  /* Owner: read */
#define S_IWUSR     0x0080  /* Owner: write */
#define S_IXUSR     0x0040  /* Owner: execute */

#define S_IRWXG     0x0038  /* Group: rwx */
#define S_IRGRP     0x0020  /* Group: read */
#define S_IWGRP     0x0010  /* Group: write */
#define S_IXGRP     0x0008  /* Group: execute */

#define S_IRWXO     0x0007  /* Other: rwx */
#define S_IROTH     0x0004  /* Other: read */
#define S_IWOTH     0x0002  /* Other: write */
#define S_IXOTH     0x0001  /* Other: execute */

/* Macro untuk mengecek tipe file */
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

/* ============================================================
 * KONSTANTA UNTUK OPEN/CREATE
 * ============================================================
 */

/* Flag untuk open() */
#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR      0x0002
#define O_CREAT     0x0100
#define O_EXCL      0x0200
#define O_TRUNC     0x0400
#define O_APPEND    0x0800
#define O_NONBLOCK  0x1000
#define O_DIRECTORY 0x2000

/* Mode untuk mkdir/mknod */
#define UMASK_DEFAULT 0022

#endif /* INTI_STAT_H */
