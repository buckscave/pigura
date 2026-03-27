/*
 * PIGURA OS - VFS.H
 * ==================
 * Header Virtual File System untuk kernel Pigura OS.
 *
 * Berkas ini mendefinisikan struktur data dan fungsi API untuk
 * sistem file virtual yang mendukung multiple filesystem.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua struktur dan fungsi didefinisikan secara eksplisit
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#ifndef BERKAS_VFS_VFS_H
#define BERKAS_VFS_VFS_H

/*
 * ===========================================================================
 * INCLUDE
 * ===========================================================================
 */

#include "../../inti/types.h"
#include "../../inti/konstanta.h"

/*
 * ===========================================================================
 * KONSTANTA VFS (VFS CONSTANTS)
 * ===========================================================================
 */

/* Jumlah maksimum filesystem yang terdaftar */
#define VFS_FS_MAKS             16

/* Jumlah maksimum mount point */
#define VFS_MOUNT_MAKS          CONFIG_MAKS_MOUNT

/* Jumlah maksimum inode dalam cache */
#define VFS_INODE_CACHE_MAKS    1024

/* Jumlah maksimum dentry dalam cache */
#define VFS_DENTRY_CACHE_MAKS   2048

/* Jumlah maksimum file descriptor sistem */
#define VFS_FD_MAKS             CONFIG_MAKS_OPEN_FILES

/* Panjang nama file maksimum */
#define VFS_NAMA_MAKS           CONFIG_NAME_MAX

/* Panjang path maksimum */
#define VFS_PATH_MAKS           CONFIG_PATH_MAX

/* Magic number untuk validasi struktur */
#define VFS_SUPER_MAGIC         0x56465353  /* "VFSS" */
#define VFS_INODE_MAGIC         0x56465349  /* "VFSI" */
#define VFS_DENTRY_MAGIC        0x56465344  /* "VFSD" */
#define VFS_FILE_MAGIC          0x56465346  /* "VFSF" */
#define VFS_MOUNT_MAGIC         0x5646534D  /* "VFSM" */

/* Flag inode */
#define VFS_INODE_FLAG_DIRTY    0x0001
#define VFS_INODE_FLAG_LOCKED   0x0002
#define VFS_INODE_FLAG_DELETED  0x0004
#define VFS_INODE_FLAG_MOUNTED  0x0008

/* Flag file */
#define VFS_FILE_FLAG_READ      0x0001
#define VFS_FILE_FLAG_WRITE     0x0002
#define VFS_FILE_FLAG_APPEND    0x0004
#define VFS_FILE_FLAG_NONBLOCK  0x0008
#define VFS_FILE_FLAG_DIR       0x0010

/* Flag mount */
#define VFS_MOUNT_FLAG_RDONLY   0x0001
#define VFS_MOUNT_FLAG_NOEXEC   0x0002
#define VFS_MOUNT_FLAG_NOSUID   0x0004
#define VFS_MOUNT_FLAG_NODEV    0x0008
#define VFS_MOUNT_FLAG_SYNC     0x0010

/* Flag open */
#define VFS_OPEN_RDONLY         0x0001
#define VFS_OPEN_WRONLY         0x0002
#define VFS_OPEN_RDWR           0x0003
#define VFS_OPEN_CREAT          0x0100
#define VFS_OPEN_EXCL           0x0200
#define VFS_OPEN_TRUNC          0x0400
#define VFS_OPEN_APPEND         0x0800
#define VFS_OPEN_DIRECTORY      0x1000

/* Whence untuk seek */
#define VFS_SEEK_SET            0
#define VFS_SEEK_CUR            1
#define VFS_SEEK_END            2

/* Tipe file dalam mode */
#define VFS_S_IFMT              0xF000
#define VFS_S_IFSOCK            0xC000
#define VFS_S_IFLNK             0xA000
#define VFS_S_IFREG             0x8000
#define VFS_S_IFBLK             0x6000
#define VFS_S_IFDIR             0x4000
#define VFS_S_IFCHR             0x2000
#define VFS_S_IFIFO             0x1000

/* Mask permission */
#define VFS_S_ISUID             0x0800
#define VFS_S_ISGID             0x0400
#define VFS_S_ISVTX             0x0200
#define VFS_S_IRWXU             0x01C0
#define VFS_S_IRUSR             0x0100
#define VFS_S_IWUSR             0x0080
#define VFS_S_IXUSR             0x0040
#define VFS_S_IRWXG             0x0038
#define VFS_S_IRGRP             0x0020
#define VFS_S_IWGRP             0x0010
#define VFS_S_IXGRP             0x0008
#define VFS_S_IRWXO             0x0007
#define VFS_S_IROTH             0x0004
#define VFS_S_IWOTH             0x0002
#define VFS_S_IXOTH             0x0001

/* Makro untuk cek tipe file */
#define VFS_S_ISREG(m)          (((m) & VFS_S_IFMT) == VFS_S_IFREG)
#define VFS_S_ISDIR(m)          (((m) & VFS_S_IFMT) == VFS_S_IFDIR)
#define VFS_S_ISCHR(m)          (((m) & VFS_S_IFMT) == VFS_S_IFCHR)
#define VFS_S_ISBLK(m)          (((m) & VFS_S_IFMT) == VFS_S_IFBLK)
#define VFS_S_ISFIFO(m)         (((m) & VFS_S_IFMT) == VFS_S_IFIFO)
#define VFS_S_ISLNK(m)          (((m) & VFS_S_IFMT) == VFS_S_IFLNK)
#define VFS_S_ISSOCK(m)         (((m) & VFS_S_IFMT) == VFS_S_IFSOCK)

/*
 * ===========================================================================
 * FORWARD DECLARATIONS
 * ===========================================================================
 */

struct superblock;
struct inode;
struct dentry;
struct file;
struct filesystem;
struct mount;
struct vfs_file_operations;
struct vfs_inode_operations;
struct vfs_super_operations;
struct vfs_dentry_operations;

/*
 * ===========================================================================
 * STRUKTUR STAT FILE (FILE STAT STRUCTURE)
 * ===========================================================================
 */

typedef struct vfs_stat {
    dev_t       st_dev;         /* Device ID */
    ino_t       st_ino;         /* Inode number */
    mode_t      st_mode;        /* Permission dan tipe file */
    tak_bertanda32_t st_nlink;  /* Jumlah hard link */
    uid_t       st_uid;         /* User ID owner */
    gid_t       st_gid;         /* Group ID owner */
    dev_t       st_rdev;        /* Device ID (jika special file) */
    off_t       st_size;        /* Ukuran total (byte) */
    waktu_t     st_atime;       /* Waktu akses terakhir */
    waktu_t     st_mtime;       /* Waktu modifikasi terakhir */
    waktu_t     st_ctime;       /* Waktu perubahan status */
    tak_bertanda32_t st_blksize;/* Ukuran block I/O optimal */
    tak_bertanda64_t st_blocks; /* Jumlah 512-byte block */
} vfs_stat_t;

/*
 * ===========================================================================
 * STRUKTUR DIRENT (DIRECTORY ENTRY)
 * ===========================================================================
 */

typedef struct vfs_dirent {
    ino_t           d_ino;      /* Inode number */
    off_t           d_off;      /* Offset ke entri berikutnya */
    tak_bertanda16_t d_reclen;  /* Panjang record ini */
    tak_bertanda8_t  d_type;    /* Tipe file */
    char            d_name[VFS_NAMA_MAKS + 1];
} vfs_dirent_t;

/* Tipe dirent */
#define VFS_DT_UNKNOWN      0
#define VFS_DT_FIFO         1
#define VFS_DT_CHR          2
#define VFS_DT_DIR          4
#define VFS_DT_BLK          6
#define VFS_DT_REG          8
#define VFS_DT_LNK          10
#define VFS_DT_SOCK         12

/*
 * ===========================================================================
 * STRUKTUR SUPERBLOCK (SUPERBLOCK STRUCTURE)
 * ===========================================================================
 */

typedef struct superblock {
    tak_bertanda32_t s_magic;       /* Magic number validasi */
    tak_bertanda32_t s_flags;       /* Flag mount */
    dev_t           s_dev;          /* Device identifier */
    tak_bertanda64_t s_block_size;  /* Ukuran block */
    tak_bertanda64_t s_total_blocks;/* Total block */
    tak_bertanda64_t s_free_blocks; /* Block bebas */
    tak_bertanda64_t s_total_inodes;/* Total inode */
    tak_bertanda64_t s_free_inodes; /* Inode bebas */
    tak_bertanda32_t s_inodes_per_group;
    tak_bertanda32_t s_blocks_per_group;
    struct filesystem *s_fs;        /* Filesystem type */
    struct dentry   *s_root;        /* Root dentry */
    struct mount    *s_mount;       /* Mount point */
    void            *s_private;     /* Data private filesystem */
    struct vfs_super_operations *s_op;  /* Superblock operations */
    tak_bertanda32_t s_refcount;    /* Reference count */
    bool_t          s_readonly;     /* Mount read-only? */
    bool_t          s_dirty;        /* Perlu sync? */
    waktu_t         s_mtime;        /* Waktu modifikasi */
} superblock_t;

/*
 * ===========================================================================
 * STRUKTUR INODE (INODE STRUCTURE)
 * ===========================================================================
 */

typedef struct inode {
    tak_bertanda32_t i_magic;       /* Magic number validasi */
    ino_t           i_ino;          /* Inode number */
    mode_t          i_mode;         /* Mode dan tipe file */
    uid_t           i_uid;          /* User ID */
    gid_t           i_gid;          /* Group ID */
    tak_bertanda32_t i_flags;       /* Flag inode */
    off_t           i_size;         /* Ukuran file */
    waktu_t         i_atime;        /* Access time */
    waktu_t         i_mtime;        /* Modify time */
    waktu_t         i_ctime;        /* Change time */
    tak_bertanda32_t i_nlink;       /* Link count */
    dev_t           i_rdev;         /* Device number (untuk special) */
    tak_bertanda64_t i_blocks;      /* Jumlah block */
    tak_bertanda32_t i_blksize;     /* Block size */
    struct superblock *i_sb;        /* Superblock */
    struct dentry   *i_dentry;      /* Dentry list */
    struct vfs_inode_operations *i_op;  /* Inode operations */
    struct vfs_file_operations *i_fop;  /* File operations */
    void            *i_private;     /* Data private */
    tak_bertanda32_t i_refcount;    /* Reference count */
    bool_t          i_dirty;        /* Perlu write? */
    bool_t          i_locked;       /* Terkunci? */
    tak_bertanda32_t i_generation;  /* Generation number */
    tak_bertanda32_t i_version;     /* Version number */
    struct inode    *i_next;        /* Hash chain next */
} inode_t;

/*
 * ===========================================================================
 * STRUKTUR DENTRY (DIRECTORY ENTRY CACHE)
 * ===========================================================================
 */

typedef struct dentry {
    tak_bertanda32_t d_magic;       /* Magic number validasi */
    char            d_name[VFS_NAMA_MAKS + 1]; /* Nama entry */
    ukuran_t        d_namelen;      /* Panjang nama */
    struct inode    *d_inode;       /* Inode terkait */
    struct dentry   *d_parent;      /* Parent dentry */
    struct superblock *d_sb;        /* Superblock */
    struct mount    *d_mount;       /* Mount point */
    struct vfs_dentry_operations *d_op; /* Dentry operations */
    tak_bertanda32_t d_flags;       /* Flag dentry */
    tak_bertanda32_t d_refcount;    /* Reference count */
    bool_t          d_mounted;      /* Ada mount point? */
    struct dentry   *d_child;       /* Anak pertama */
    struct dentry   *d_sibling;     /* Saudara berikutnya */
    struct dentry   *d_next;        /* Hash chain next */
    struct dentry   *d_prev;        /* Hash chain prev */
    void            *d_private;     /* Data private */
} dentry_t;

/*
 * ===========================================================================
 * STRUKTUR FILE (OPEN FILE STRUCTURE)
 * ===========================================================================
 */

typedef struct file {
    tak_bertanda32_t f_magic;       /* Magic number validasi */
    struct inode    *f_inode;       /* Inode terkait */
    struct dentry   *f_dentry;      /* Dentry terkait */
    struct mount    *f_mount;       /* Mount point */
    struct vfs_file_operations *f_op;   /* File operations */
    tak_bertanda32_t f_flags;       /* Open flags */
    mode_t          f_mode;         /* Mode akses */
    off_t           f_pos;          /* Posisi file */
    tak_bertanda32_t f_refcount;    /* Reference count */
    pid_t           f_owner;        /* PID owner */
    void            *f_private;     /* Data private */
    bool_t          f_readable;     /* Bisa dibaca? */
    bool_t          f_writable;     /* Bisa ditulis? */
    bool_t          f_append;       /* Mode append? */
} file_t;

/*
 * ===========================================================================
 * STRUKTUR MOUNT (MOUNT POINT STRUCTURE)
 * ===========================================================================
 */

typedef struct mount {
    tak_bertanda32_t m_magic;       /* Magic number validasi */
    struct filesystem *m_fs;        /* Filesystem type */
    struct superblock *m_sb;        /* Superblock */
    struct dentry   *m_mountpoint;  /* Dentry mount point */
    struct dentry   *m_root;        /* Root dentry */
    struct mount    *m_parent;      /* Parent mount */
    char            m_path[VFS_PATH_MAKS + 1]; /* Path mount */
    char            m_device[VFS_PATH_MAKS + 1]; /* Device path */
    tak_bertanda32_t m_flags;       /* Mount flags */
    tak_bertanda32_t m_refcount;    /* Reference count */
    bool_t          m_readonly;     /* Read-only? */
    struct mount    *m_next;        /* Next in list */
} mount_t;

/*
 * ===========================================================================
 * STRUKTUR FILESYSTEM TYPE (FILESYSTEM TYPE STRUCTURE)
 * ===========================================================================
 */

typedef struct filesystem {
    char            fs_nama[32];    /* Nama filesystem */
    tak_bertanda32_t fs_flags;      /* Flag filesystem */
    struct superblock *(*fs_mount)(struct filesystem *fs,
                        const char *device, const char *path,
                        tak_bertanda32_t flags);
    status_t (*fs_umount)(struct superblock *sb);
    status_t (*fs_detect)(const char *device);
    struct filesystem *fs_next;     /* Next in list */
    tak_bertanda32_t fs_refcount;   /* Reference count */
    bool_t          fs_terdaftar;   /* Terdaftar? */
} filesystem_t;

/* Flag filesystem */
#define FS_FLAG_REQUIRES_DEV    0x0001  /* Butuh block device */
#define FS_FLAG_REQUIRES_MOUNT  0x0002  /* Butuh mount point */
#define FS_FLAG_NO_DEV          0x0004  /* Virtual filesystem */
#define FS_FLAG_READ_ONLY       0x0008  /* Hanya baca */
#define FS_FLAG_WRITE_SUPPORT   0x0010  /* Mendukung tulis */

/*
 * ===========================================================================
 * STRUKTUR OPERATIONS (OPERATIONS STRUCTURES)
 * ===========================================================================
 */

/* Superblock operations */
typedef struct vfs_super_operations {
    struct inode *(*alloc_inode)(struct superblock *sb);
    void (*destroy_inode)(struct inode *inode);
    void (*dirty_inode)(struct inode *inode);
    status_t (*write_inode)(struct inode *inode);
    status_t (*read_inode)(struct inode *inode);
    status_t (*sync_fs)(struct superblock *sb);
    void (*put_super)(struct superblock *sb);
    status_t (*statfs)(struct superblock *sb, struct vfs_stat *stat);
    status_t (*remount_fs)(struct superblock *sb, tak_bertanda32_t flags);
} vfs_super_operations_t;

/* Inode operations */
typedef struct vfs_inode_operations {
    struct dentry *(*lookup)(struct inode *dir, const char *name);
    status_t (*create)(struct inode *dir, struct dentry *dentry,
                       mode_t mode);
    status_t (*mkdir)(struct inode *dir, struct dentry *dentry,
                      mode_t mode);
    status_t (*rmdir)(struct inode *dir, struct dentry *dentry);
    status_t (*unlink)(struct inode *dir, struct dentry *dentry);
    status_t (*rename)(struct inode *old_dir, struct dentry *old_dentry,
                       struct inode *new_dir, struct dentry *new_dentry);
    status_t (*symlink)(struct inode *dir, struct dentry *dentry,
                        const char *target);
    tak_bertandas_t (*readlink)(struct dentry *dentry, char *buffer,
                               ukuran_t size);
    status_t (*permission)(struct inode *inode, tak_bertanda32_t mask);
    status_t (*getattr)(struct dentry *dentry, struct vfs_stat *stat);
    status_t (*setattr)(struct dentry *dentry, struct vfs_stat *stat);
} vfs_inode_operations_t;

/* File operations */
typedef struct vfs_file_operations {
    tak_bertandas_t (*read)(struct file *file, void *buffer,
                           ukuran_t size, off_t *pos);
    tak_bertandas_t (*write)(struct file *file, const void *buffer,
                            ukuran_t size, off_t *pos);
    off_t (*lseek)(struct file *file, off_t offset, tak_bertanda32_t whence);
    status_t (*open)(struct inode *inode, struct file *file);
    status_t (*release)(struct inode *inode, struct file *file);
    status_t (*flush)(struct file *file);
    status_t (*fsync)(struct file *file);
    tak_bertandas_t (*readdir)(struct file *file, struct vfs_dirent *dirent,
                              ukuran_t count);
    status_t (*ioctl)(struct file *file, tak_bertanda32_t cmd,
                      tak_bertanda64_t arg);
    tak_bertandas_t (*mmap)(struct file *file, void *addr,
                           ukuran_t size, tak_bertanda32_t prot,
                           tak_bertanda32_t flags, off_t offset);
} vfs_file_operations_t;

/* Dentry operations */
typedef struct vfs_dentry_operations {
    status_t (*d_revalidate)(struct dentry *dentry);
    status_t (*d_hash)(struct dentry *dentry, const char *name);
    status_t (*d_compare)(struct dentry *dentry, const char *name1,
                          const char *name2);
    status_t (*d_delete)(struct dentry *dentry);
    void (*d_release)(struct dentry *dentry);
} vfs_dentry_operations_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI VFS CORE (VFS CORE FUNCTIONS)
 * ===========================================================================
 */

/* Inisialisasi dan shutdown */
status_t vfs_init(void);
void vfs_shutdown(void);

/* Statistik */
tak_bertanda32_t vfs_get_mount_count(void);
tak_bertanda32_t vfs_get_inode_count(void);
tak_bertanda32_t vfs_get_dentry_count(void);
tak_bertanda32_t vfs_get_file_count(void);
tak_bertanda32_t vfs_get_fs_count(void);

/* Path utilities */
status_t vfs_path_normalize(const char *path, char *buffer, ukuran_t size);
status_t vfs_path_join(const char *dir, const char *name, char *buffer,
                       ukuran_t size);
status_t vfs_path_split(const char *path, char *dir, char *name,
                        ukuran_t dir_size, ukuran_t name_size);
const char *vfs_path_basename(const char *path);
status_t vfs_path_dirname(const char *path, char *buffer, ukuran_t size);

/* Validasi */
bool_t vfs_path_valid(const char *path);
bool_t vfs_name_valid(const char *name);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI SUPERBLOCK (SUPERBLOCK FUNCTIONS)
 * ===========================================================================
 */

/* Alokasi dan dealokasi */
superblock_t *superblock_alloc(filesystem_t *fs);
void superblock_free(superblock_t *sb);

/* Inisialisasi */
status_t superblock_init(superblock_t *sb, filesystem_t *fs, dev_t dev);

/* Reference counting */
void superblock_get(superblock_t *sb);
void superblock_put(superblock_t *sb);

/* Sinkronisasi */
status_t superblock_sync(superblock_t *sb);
void superblock_mark_dirty(superblock_t *sb);

/* Inode management */
status_t superblock_reserve_inode(superblock_t *sb);
status_t superblock_release_inode(superblock_t *sb);
struct inode *superblock_alloc_inode(superblock_t *sb);
void superblock_destroy_inode(superblock_t *sb, struct inode *inode);

/* Block management */
status_t superblock_reserve_blocks(superblock_t *sb, tak_bertanda64_t count);
status_t superblock_release_blocks(superblock_t *sb, tak_bertanda64_t count);

/* Statistik */
status_t superblock_statfs(superblock_t *sb, vfs_stat_t *stat);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INODE (INODE FUNCTIONS)
 * ===========================================================================
 */

/* Alokasi dan dealokasi */
inode_t *inode_alloc(superblock_t *sb);
void inode_free(inode_t *inode);

/* Inisialisasi */
status_t inode_init(inode_t *inode, superblock_t *sb, ino_t ino,
                    mode_t mode);

/* Reference counting */
void inode_get(inode_t *inode);
void inode_put(inode_t *inode);

/* Operasi */
status_t inode_read(inode_t *inode);
status_t inode_write(inode_t *inode);
status_t inode_mark_dirty(inode_t *inode);
status_t inode_sync(inode_t *inode);

/* Lookup */
inode_t *inode_lookup(inode_t *dir, const char *name);

/* Permission */
status_t inode_permission(inode_t *inode, tak_bertanda32_t mask);
bool_t inode_permission_check(inode_t *inode, uid_t uid, gid_t gid,
                              tak_bertanda32_t mask);

/* Attribute */
status_t inode_getattr(inode_t *inode, vfs_stat_t *stat);
status_t inode_setattr(inode_t *inode, vfs_stat_t *stat);

/* Tipe */
bool_t inode_is_regular(inode_t *inode);
bool_t inode_is_directory(inode_t *inode);
bool_t inode_is_symlink(inode_t *inode);
bool_t inode_is_device(inode_t *inode);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI DENTRY (DENTRY FUNCTIONS)
 * ===========================================================================
 */

/* Alokasi dan dealokasi */
dentry_t *dentry_alloc(const char *name);
void dentry_free(dentry_t *dentry);

/* Inisialisasi */
status_t dentry_init(dentry_t *dentry, const char *name,
                     dentry_t *parent, inode_t *inode);

/* Reference counting */
void dentry_get(dentry_t *dentry);
void dentry_put(dentry_t *dentry);

/* Cache */
dentry_t *dentry_lookup(dentry_t *parent, const char *name);
void dentry_cache_insert(dentry_t *dentry);
void dentry_cache_remove(dentry_t *dentry);
void dentry_cache_invalidate(dentry_t *dentry);

/* Hierarchy */
status_t dentry_add_child(dentry_t *parent, dentry_t *child);
status_t dentry_remove_child(dentry_t *parent, dentry_t *child);
dentry_t *dentry_find_child(dentry_t *parent, const char *name);

/* Validation */
status_t dentry_revalidate(dentry_t *dentry);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI FILE (FILE FUNCTIONS)
 * ===========================================================================
 */

/* Alokasi dan dealokasi */
file_t *file_alloc(void);
void file_free(file_t *file);

/* Inisialisasi */
status_t file_init(file_t *file, inode_t *inode, dentry_t *dentry,
                   tak_bertanda32_t flags);

/* Reference counting */
void file_get(file_t *file);
void file_put(file_t *file);

/* Operasi I/O */
tak_bertandas_t file_read(file_t *file, void *buffer, ukuran_t size);
tak_bertandas_t file_write(file_t *file, const void *buffer,
                           ukuran_t size);
off_t file_lseek(file_t *file, off_t offset, tak_bertanda32_t whence);
status_t file_flush(file_t *file);
status_t file_fsync(file_t *file);

/* Operasi direktori */
tak_bertandas_t file_readdir(file_t *file, vfs_dirent_t *dirent,
                             ukuran_t count);

/* Operasi lain */
status_t file_open(inode_t *inode, file_t *file);
status_t file_release(file_t *file);
status_t file_ioctl(file_t *file, tak_bertanda32_t cmd,
                    tak_bertanda64_t arg);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI FILESYSTEM (FILESYSTEM FUNCTIONS)
 * ===========================================================================
 */

/* Registrasi */
status_t filesystem_register(filesystem_t *fs);
status_t filesystem_unregister(filesystem_t *fs);
filesystem_t *filesystem_find(const char *nama);

/* Deteksi */
filesystem_t *filesystem_detect(const char *device);

/* Iterasi */
filesystem_t *filesystem_first(void);
filesystem_t *filesystem_next(filesystem_t *fs);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI MOUNT (MOUNT FUNCTIONS)
 * ===========================================================================
 */

/* Mount/umount */
status_t mount_create(const char *device, const char *path,
                      const char *fs_nama, tak_bertanda32_t flags);
status_t mount_destroy(const char *path);

/* Lookup */
mount_t *mount_find(const char *path);
mount_t *mount_find_by_dev(dev_t dev);
mount_t *mount_root(void);

/* Reference counting */
void mount_get(mount_t *mount);
void mount_put(mount_t *mount);

/* Iterasi */
mount_t *mount_first(void);
mount_t *mount_next(mount_t *mount);

/* Path resolution */
dentry_t *mount_resolve_path(const char *path);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI NAMEI (NAMEI FUNCTIONS)
 * ===========================================================================
 */

/* Path lookup */
dentry_t *namei_lookup(const char *path);
dentry_t *namei_lookup_parent(const char *path, char *nama,
                              ukuran_t nama_size);
status_t namei_path_lookup(const char *path, dentry_t **dentry,
                           tak_bertanda32_t flags);

/* Flag lookup */
#define NAMEI_LOOKUP_FOLLOW     0x0001  /* Follow symlink */
#define NAMEI_LOOKUP_PARENT     0x0002  /* Dapatkan parent */
#define NAMEI_LOOKUP_CREATE     0x0004  /* Untuk create */
#define NAMEI_LOOKUP_EXCL       0x0008  /* Exclusive create */

/*
 * ===========================================================================
 * DEKLARASI FUNGSI VFS API (VFS API FUNCTIONS)
 * ===========================================================================
 */

/* File operations */
int vfs_open(const char *path, tak_bertanda32_t flags, mode_t mode);
status_t vfs_close(int fd);
tak_bertandas_t vfs_read(int fd, void *buffer, ukuran_t size);
tak_bertandas_t vfs_write(int fd, const void *buffer, ukuran_t size);
off_t vfs_lseek(int fd, off_t offset, tak_bertanda32_t whence);
status_t vfs_fstat(int fd, vfs_stat_t *stat);
status_t vfs_fsync(int fd);

/* Directory operations */
int vfs_opendir(const char *path);
tak_bertandas_t vfs_readdir(int fd, vfs_dirent_t *dirent,
                            ukuran_t count);
status_t vfs_closedir(int fd);
status_t vfs_mkdir(const char *path, mode_t mode);
status_t vfs_rmdir(const char *path);

/* File management */
status_t vfs_unlink(const char *path);
status_t vfs_rename(const char *old_path, const char *new_path);
status_t vfs_stat(const char *path, vfs_stat_t *stat);
status_t vfs_lstat(const char *path, vfs_stat_t *stat);
status_t vfs_chmod(const char *path, mode_t mode);
status_t vfs_chown(const char *path, uid_t uid, gid_t gid);
status_t vfs_truncate(const char *path, off_t length);

/* Symlink */
status_t vfs_symlink(const char *target, const char *linkpath);
tak_bertandas_t vfs_readlink(const char *path, char *buffer,
                             ukuran_t size);

/* Directory navigation */
status_t vfs_chdir(const char *path);
status_t vfs_getcwd(char *buffer, ukuran_t size);

/* Mount operations */
status_t vfs_mount(const char *device, const char *path,
                   const char *fs_nama, tak_bertanda32_t flags);
status_t vfs_umount(const char *path);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI FILE DESCRIPTOR (FILE DESCRIPTOR FUNCTIONS)
 * ===========================================================================
 */

/* Management */
int fd_alloc(file_t *file);
file_t *fd_get(int fd);
status_t fd_put(int fd);
status_t fd_dup(int old_fd);
status_t fd_dup2(int old_fd, int new_fd);

/* Iterasi */
int fd_first(void);
int fd_next(int fd);

#endif /* BERKAS_VFS_VFS_H */
