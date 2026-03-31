/*
 * PIGURA LIBC - SYS/STAT.H
 * =========================
 * Header untuk status file sesuai standar POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#ifndef LIBC_SYS_STAT_H
#define LIBC_SYS_STAT_H

/* ============================================================
 * DEPENDENSI
 * ============================================================
 */
#include <sys/types.h>
#include <time.h>

/* ============================================================
 * STRUKTUR STAT
 * ============================================================
 * Struktur untuk menyimpan informasi file.
 */

#ifndef _STRUCT_STAT
#define _STRUCT_STAT
struct stat {
    dev_t     st_dev;      /* Device ID */
    ino_t     st_ino;      /* Inode number */
    mode_t    st_mode;     /* Mode permission dan file type */
    nlink_t   st_nlink;    /* Number of hard links */
    uid_t     st_uid;      /* User ID owner */
    gid_t     st_gid;      /* Group ID owner */
    dev_t     st_rdev;     /* Device ID (jika device special) */
    off_t     st_size;     /* Ukuran total dalam byte */
    blksize_t st_blksize;  /* Blocksize untuk I/O */
    blkcnt_t  st_blocks;   /* Jumlah 512-byte blocks */
    
    /* Timestamps */
    struct timespec st_atim;  /* Waktu akses terakhir */
    struct timespec st_mtim;  /* Waktu modifikasi terakhir */
    struct timespec st_ctim;  /* Waktu perubahan status */
    
    /* Alias untuk kompatibilitas */
#ifndef st_atime
#define st_atime st_atim.tv_sec
#endif
#ifndef st_mtime
#define st_mtime st_mtim.tv_sec
#endif
#ifndef st_ctime
#define st_ctime st_ctim.tv_sec
#endif
};
#endif /* _STRUCT_STAT */

/* ============================================================
 * FUNGSI STAT
 * ============================================================
 */

/*
 * stat - Dapatkan informasi file
 *
 * Parameter:
 *   pathname - Nama file
 *   statbuf  - Buffer untuk hasil
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int stat(const char *pathname, struct stat *statbuf);

/*
 * fstat - Dapatkan informasi file via descriptor
 *
 * Parameter:
 *   fd      - File descriptor
 *   statbuf - Buffer untuk hasil
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int fstat(int fd, struct stat *statbuf);

/*
 * lstat - Dapatkan informasi file (tidak follow symlink)
 *
 * Parameter:
 *   pathname - Nama file
 *   statbuf  - Buffer untuk hasil
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int lstat(const char *pathname, struct stat *statbuf);

/*
 * fstatat - Dapatkan informasi file relatif ke direktori
 *
 * Parameter:
 *   dirfd    - File descriptor direktori (AT_FDCWD untuk cwd)
 *   pathname - Nama file
 *   statbuf  - Buffer untuk hasil
 *   flags    - Flags (AT_SYMLINK_NOFOLLOW, dll)
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int fstatat(int dirfd, const char *pathname,
                   struct stat *statbuf, int flags);

/* ============================================================
 * FUNGSI PERMISSION
 * ============================================================
 */

/*
 * chmod - Ubah permission file
 *
 * Parameter:
 *   pathname - Nama file
 *   mode     - Mode baru
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int chmod(const char *pathname, mode_t mode);

/*
 * fchmod - Ubah permission via descriptor
 *
 * Parameter:
 *   fd   - File descriptor
 *   mode - Mode baru
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int fchmod(int fd, mode_t mode);

/*
 * fchmodat - Ubah permission relatif ke direktori
 *
 * Parameter:
 *   dirfd    - File descriptor direktori
 *   pathname - Nama file
 *   mode     - Mode baru
 *   flags    - Flags
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int fchmodat(int dirfd, const char *pathname,
                    mode_t mode, int flags);

/*
 * umask - Set file creation mask
 *
 * Parameter:
 *   mask - Mask baru
 *
 * Return: Mask sebelumnya
 */
extern mode_t umask(mode_t mask);

/*
 * mkdir - Buat direktori
 *
 * Parameter:
 *   pathname - Nama direktori
 *   mode     - Mode permission
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int mkdir(const char *pathname, mode_t mode);

/*
 * mkdirat - Buat direktori relatif ke fd
 *
 * Parameter:
 *   dirfd    - File descriptor direktori
 *   pathname - Nama direktori
 *   mode     - Mode permission
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int mkdirat(int dirfd, const char *pathname, mode_t mode);

/*
 * mknod - Buat file khusus (device)
 *
 * Parameter:
 *   pathname - Nama file
 *   mode     - Mode dan tipe file
 *   dev      - Device number
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int mknod(const char *pathname, mode_t mode, dev_t dev);

/*
 * mknodat - Buat file khusus relatif ke fd
 *
 * Parameter:
 *   dirfd    - File descriptor direktori
 *   pathname - Nama file
 *   mode     - Mode dan tipe file
 *   dev      - Device number
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int mknodat(int dirfd, const char *pathname,
                   mode_t mode, dev_t dev);

/*
 * mkfifo - Buat FIFO (named pipe)
 *
 * Parameter:
 *   pathname - Nama FIFO
 *   mode     - Mode permission
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int mkfifo(const char *pathname, mode_t mode);

/*
 * mkfifoat - Buat FIFO relatif ke fd
 *
 * Parameter:
 *   dirfd    - File descriptor direktori
 *   pathname - Nama FIFO
 *   mode     - Mode permission
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int mkfifoat(int dirfd, const char *pathname, mode_t mode);

/* ============================================================
 * KONSTANTA UNTUK AT_* FLAGS
 * ============================================================
 */

#ifndef AT_FDCWD
#define AT_FDCWD -100
#endif

#ifndef AT_SYMLINK_NOFOLLOW
#define AT_SYMLINK_NOFOLLOW 0x100
#endif

#ifndef AT_REMOVEDIR
#define AT_REMOVEDIR 0x200
#endif

#ifndef AT_SYMLINK_FOLLOW
#define AT_SYMLINK_FOLLOW 0x400
#endif

#ifndef AT_NO_AUTOMOUNT
#define AT_NO_AUTOMOUNT 0x800
#endif

#ifndef AT_EMPTY_PATH
#define AT_EMPTY_PATH 0x1000
#endif

/* ============================================================
 * FUNGSI OWNERSHIP
 * ============================================================
 */

/*
 * chown - Ubah owner file
 *
 * Parameter:
 *   pathname - Nama file
 *   owner    - UID baru (-1 tidak berubah)
 *   group    - GID baru (-1 tidak berubah)
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int chown(const char *pathname, uid_t owner, gid_t group);

/*
 * fchown - Ubah owner via descriptor
 *
 * Parameter:
 *   fd    - File descriptor
 *   owner - UID baru
 *   group - GID baru
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int fchown(int fd, uid_t owner, gid_t group);

/*
 * lchown - Ubah owner (tidak follow symlink)
 *
 * Parameter:
 *   pathname - Nama file
 *   owner    - UID baru
 *   group    - GID baru
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int lchown(const char *pathname, uid_t owner, gid_t group);

/*
 * fchownat - Ubah owner relatif ke direktori
 *
 * Parameter:
 *   dirfd    - File descriptor direktori
 *   pathname - Nama file
 *   owner    - UID baru
 *   group    - GID baru
 *   flags    - Flags
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int fchownat(int dirfd, const char *pathname,
                    uid_t owner, gid_t group, int flags);

#endif /* LIBC_SYS_STAT_H */
