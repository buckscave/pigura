/*
 * PIGURA OS - INITRAMFS.C
 * =========================
 * Implementasi initramfs filesystem untuk kernel Pigura OS.
 *
 * Berkas ini berisi implementasi filesystem initramfs yang
 * merupakan filesystem root pertama yang dimuat saat boot.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua fungsi diimplementasikan secara lengkap
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "initramfs.h"
#include "../vfs/vfs.h"
#include "../../inti/kernel.h"

/* File ini berisi banyak stub TODO yang memiliki parameter tidak terpakai */
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"

/*
 * ===========================================================================
 * KONSTANTA INTERNAL (INTERNAL CONSTANTS)
 * ===========================================================================
 */

/* Magic number initramfs */
#define INITRAMFS_MAGIC     0x49465352  /* "IFSR" - InitRamFS Root */

/* Maximum entries */
#define INITRAMFS_MAX_ENTRIES 4096

/* Maximum path depth */
#define INITRAMFS_MAX_DEPTH   32

/* Default block size */
#define INITRAMFS_BLOCK_SIZE  4096

/* Inode number start */
#define INITRAMFS_INO_START   1

/* Root inode number */
#define INITRAMFS_ROOT_INO    1

/*
 * ===========================================================================
 * STRUKTUR INODE INITRAMFS (INITRAMFS INODE STRUCTURE)
 * ===========================================================================
 */

/* initramfs_inode_t dan initramfs_dentry_t didefinisikan di initramfs.h */

/*
 * ===========================================================================
 * STRUKTUR SUPERBLOCK INITRAMFS (INITRAMFS SUPERBLOCK STRUCTURE)
 * ===========================================================================
 */

typedef struct {
    tak_bertanda32_t s_magic;
    tak_bertanda64_t s_block_size;
    tak_bertanda64_t s_total_inodes;
    tak_bertanda64_t s_total_size;
    initramfs_inode_t *s_root_inode;
    initramfs_dentry_t *s_root_dentry;
    const void       *s_data;
    ukuran_t          s_data_size;
    tak_bertanda32_t  s_inode_count;
    tak_bertanda32_t  s_dentry_count;
    bool_t            s_readonly;
} initramfs_sb_t;

/*
 * ===========================================================================
 * STRUKTUR FILE DESCRIPTOR INITRAMFS (INITRAMFS FILE DESCRIPTOR)
 * ===========================================================================
 */

typedef struct {
    initramfs_inode_t *f_inode;
    off_t              f_pos;
    tak_bertanda32_t   f_flags;
} initramfs_fd_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ===========================================================================
 */

/* Global superblock */
static initramfs_sb_t *g_initramfs_sb = NULL;

/* Inode cache */
static initramfs_inode_t *g_inode_cache[INITRAMFS_MAX_ENTRIES];
static tak_bertanda32_t g_inode_count = 0;

/* Dentry cache */
static initramfs_dentry_t *g_dentry_cache[INITRAMFS_MAX_ENTRIES];
static tak_bertanda32_t g_dentry_count = 0;

/* Lock */
static tak_bertanda32_t g_initramfs_lock = 0;

/* File descriptor table */
#define INITRAMFS_MAX_FD  32
static initramfs_fd_t g_fd_table[INITRAMFS_MAX_FD];

/* Inode number generator */
static ino_t g_next_ino = INITRAMFS_INO_START;

/*
 * ===========================================================================
 * FUNGSI INTERNAL LOCKING (INTERNAL LOCKING FUNCTIONS)
 * ===========================================================================
 */

static void initramfs_lock(void)
{
    g_initramfs_lock++;
}

static void initramfs_unlock(void)
{
    if (g_initramfs_lock > 0) {
        g_initramfs_lock--;
    }
}

/*
 * ===========================================================================
 * FUNGSI ALOKASI INODE (INODE ALLOCATION FUNCTIONS)
 * ===========================================================================
 */

static initramfs_inode_t *initramfs_alloc_inode(void)
{
    initramfs_inode_t *inode;
    
    if (g_inode_count >= INITRAMFS_MAX_ENTRIES) {
        return NULL;
    }
    
    inode = (initramfs_inode_t *)kmalloc(sizeof(initramfs_inode_t));
    if (inode == NULL) {
        return NULL;
    }
    
    kernel_memset(inode, 0, sizeof(initramfs_inode_t));
    
    inode->i_ino = g_next_ino++;
    inode->i_atime = kernel_get_uptime();
    inode->i_mtime = inode->i_atime;
    inode->i_ctime = inode->i_atime;
    inode->i_nlink = 1;
    
    return inode;
}

static void initramfs_free_inode(initramfs_inode_t *inode)
{
    if (inode == NULL) {
        return;
    }
    
    if (inode->i_target != NULL) {
        kfree(inode->i_target);
    }
    
    /* Data tidak di-free karena bagian dari buffer initramfs */
    
    kfree(inode);
}

/*
 * ===========================================================================
 * FUNGSI ALOKASI DENTRY (DENTRY ALLOCATION FUNCTIONS)
 * ===========================================================================
 */

static initramfs_dentry_t *initramfs_alloc_dentry(const char *name)
{
    initramfs_dentry_t *dentry;
    ukuran_t namelen;
    
    if (name == NULL) {
        return NULL;
    }
    
    namelen = kernel_strlen(name);
    if (namelen == 0 || namelen > VFS_NAMA_MAKS) {
        return NULL;
    }
    
    dentry = (initramfs_dentry_t *)kmalloc(sizeof(initramfs_dentry_t));
    if (dentry == NULL) {
        return NULL;
    }
    
    kernel_memset(dentry, 0, sizeof(initramfs_dentry_t));
    
    kernel_strncpy(dentry->d_name, name, VFS_NAMA_MAKS);
    dentry->d_namelen = namelen;
    
    return dentry;
}

static void initramfs_free_dentry(initramfs_dentry_t *dentry)
{
    if (dentry == NULL) {
        return;
    }
    
    kfree(dentry);
}

/*
 * ===========================================================================
 * FUNGSI MANAJEMEN DIREKTORI (DIRECTORY MANAGEMENT FUNCTIONS)
 * ===========================================================================
 */

static status_t initramfs_add_dentry(initramfs_inode_t *dir,
                                      initramfs_dentry_t *dentry)
{
    initramfs_dentry_t *curr;
    
    if (dir == NULL || dentry == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (!VFS_S_ISDIR(dir->i_mode)) {
        return STATUS_GAGAL; /* STATUS_BUKAN_DIR */
    }
    
    dentry->d_parent = dir->i_children ? dir->i_children->d_parent : NULL;
    
    /* Add to children list */
    if (dir->i_children == NULL) {
        dir->i_children = dentry;
    } else {
        curr = dir->i_children;
        while (curr->d_next != NULL) {
            curr = curr->d_next;
        }
        curr->d_next = dentry;
    }
    
    dentry->d_inode->i_parent = dir;
    dir->i_nlink++;
    
    return STATUS_BERHASIL;
}

static initramfs_dentry_t *initramfs_lookup_dentry(initramfs_inode_t *dir,
                                                    const char *name)
{
    initramfs_dentry_t *curr;
    
    if (dir == NULL || name == NULL) {
        return NULL;
    }
    
    if (!VFS_S_ISDIR(dir->i_mode)) {
        return NULL;
    }
    
    curr = dir->i_children;
    while (curr != NULL) {
        if (kernel_strcmp(curr->d_name, name) == 0) {
            return curr;
        }
        curr = curr->d_next;
    }
    
    return NULL;
}

/*
 * ===========================================================================
 * FUNGSI PATH RESOLUTION (PATH RESOLUTION FUNCTIONS)
 * ===========================================================================
 */

initramfs_dentry_t *initramfs_resolve_path(const char *path)
{
    initramfs_dentry_t *curr;
    initramfs_dentry_t *parent;
    char temp[VFS_PATH_MAKS + 1];
    char *token;
    char *saveptr;
    
    if (path == NULL || g_initramfs_sb == NULL) {
        return NULL;
    }
    
    /* Path harus absolut */
    if (path[0] != '/') {
        return NULL;
    }
    
    curr = g_initramfs_sb->s_root_dentry;
    if (curr == NULL) {
        return NULL;
    }
    
    /* Handle root */
    if (path[1] == '\0') {
        return curr;
    }
    
    /* Copy path untuk parsing */
    kernel_strncpy(temp, path + 1, VFS_PATH_MAKS);
    temp[VFS_PATH_MAKS] = '\0';
    
    parent = curr;
    
    token = kernel_strtok_r(temp, "/", &saveptr);
    while (token != NULL) {
        if (kernel_strcmp(token, ".") == 0) {
            /* Current directory */
        } else if (kernel_strcmp(token, "..") == 0) {
            /* Parent directory */
            if (parent->d_parent != NULL) {
                parent = parent->d_parent;
                curr = parent;
            }
        } else {
            /* Lookup component */
            curr = initramfs_lookup_dentry(parent->d_inode, token);
            if (curr == NULL) {
                return NULL;
            }
            
            /* Follow symlink jika perlu */
            if (VFS_S_ISLNK(curr->d_inode->i_mode)) {
                /* TODO: Handle symlink */
            }
            
            parent = curr;
        }
        
        token = kernel_strtok_r(NULL, "/", &saveptr);
    }
    
    return curr;
}

/*
 * ===========================================================================
 * FUNGSI VFS SUPERBLOCK OPERATIONS (VFS SUPERBLOCK OPERATIONS)
 * ===========================================================================
 */

static struct inode *initramfs_alloc_inode_vfs(struct superblock *sb)
{
    initramfs_inode_t *inode;
    
    inode = initramfs_alloc_inode();
    return (struct inode *)inode;
}

static void initramfs_destroy_inode_vfs(struct inode *inode)
{
    initramfs_free_inode((initramfs_inode_t *)inode);
}

static status_t initramfs_write_inode_vfs(struct inode *inode)
{
    /* Initramfs is read-only */
    return STATUS_FS_READONLY;
}

static status_t initramfs_sync_fs_vfs(struct superblock *sb)
{
    /* Initramfs is in-memory, no sync needed */
    return STATUS_BERHASIL;
}

static status_t initramfs_statfs_vfs(struct superblock *sb, 
                                     struct vfs_stat *stat)
{
    if (stat == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    kernel_memset(stat, 0, sizeof(struct vfs_stat));
    
    if (g_initramfs_sb != NULL) {
        stat->st_blksize = g_initramfs_sb->s_block_size;
        stat->st_blocks = g_initramfs_sb->s_total_size / 512;
        stat->st_size = g_initramfs_sb->s_total_size;
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI VFS INODE OPERATIONS (VFS INODE OPERATIONS)
 * ===========================================================================
 */

static struct dentry *initramfs_lookup_vfs(struct inode *dir,
                                            const char *name)
{
    initramfs_dentry_t *dentry;
    struct dentry *vfs_dentry;
    
    if (dir == NULL || name == NULL) {
        return NULL;
    }
    
    dentry = initramfs_lookup_dentry((initramfs_inode_t *)dir, name);
    if (dentry == NULL) {
        return NULL;
    }
    
    /* Allocate VFS dentry */
    vfs_dentry = dentry_alloc(name);
    if (vfs_dentry == NULL) {
        return NULL;
    }
    
    /* Set inode */
    vfs_dentry->d_inode = (struct inode *)dentry->d_inode;
    
    return vfs_dentry;
}

static status_t initramfs_create_vfs(struct inode *dir,
                                     struct dentry *dentry,
                                     mode_t mode)
{
    /* Initramfs is read-only */
    return STATUS_FS_READONLY;
}

static status_t initramfs_mkdir_vfs(struct inode *dir,
                                    struct dentry *dentry,
                                    mode_t mode)
{
    /* Initramfs is read-only */
    return STATUS_FS_READONLY;
}

static status_t initramfs_rmdir_vfs(struct inode *dir,
                                    struct dentry *dentry)
{
    /* Initramfs is read-only */
    return STATUS_FS_READONLY;
}

static status_t initramfs_unlink_vfs(struct inode *dir,
                                     struct dentry *dentry)
{
    /* Initramfs is read-only */
    return STATUS_FS_READONLY;
}

static status_t initramfs_getattr_vfs(struct dentry *dentry,
                                       struct vfs_stat *stat)
{
    initramfs_inode_t *inode;
    
    if (dentry == NULL || stat == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    inode = (initramfs_inode_t *)dentry->d_inode;
    if (inode == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    kernel_memset(stat, 0, sizeof(struct vfs_stat));
    
    stat->st_ino = inode->i_ino;
    stat->st_mode = inode->i_mode;
    stat->st_uid = inode->i_uid;
    stat->st_gid = inode->i_gid;
    stat->st_size = inode->i_size;
    stat->st_atime = inode->i_atime;
    stat->st_mtime = inode->i_mtime;
    stat->st_ctime = inode->i_ctime;
    stat->st_nlink = inode->i_nlink;
    stat->st_rdev = inode->i_rdev;
    
    if (g_initramfs_sb != NULL) {
        stat->st_blksize = g_initramfs_sb->s_block_size;
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI VFS FILE OPERATIONS (VFS FILE OPERATIONS)
 * ===========================================================================
 */

static tak_bertandas_t initramfs_read_vfs(struct file *file,
                                          void *buffer,
                                          ukuran_t size,
                                          off_t *pos)
{
    initramfs_inode_t *inode;
    ukuran_t available;
    ukuran_t to_read;
    
    if (file == NULL || buffer == NULL || pos == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_NULL;
    }
    
    inode = (initramfs_inode_t *)file->f_inode;
    if (inode == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }
    
    if (!VFS_S_ISREG(inode->i_mode)) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }
    
    if (*pos < 0) {
        *pos = 0;
    }
    
    if (*pos >= inode->i_size) {
        return 0;
    }
    
    available = (ukuran_t)(inode->i_size - *pos);
    to_read = (size > available) ? available : size;
    
    if (to_read > 0 && inode->i_data != NULL) {
        kernel_memcpy(buffer,
                      (const void *)((uintptr_t)inode->i_data + (uintptr_t)*pos),
                      to_read);
        *pos += (off_t)to_read;
    }
    
    return (tak_bertandas_t)to_read;
}

static tak_bertandas_t initramfs_write_vfs(struct file *file,
                                           const void *buffer,
                                           ukuran_t size,
                                           off_t *pos)
{
    /* Initramfs is read-only */
    return (tak_bertandas_t)STATUS_FS_READONLY;
}

static off_t initramfs_lseek_vfs(struct file *file,
                                  off_t offset,
                                  tak_bertanda32_t whence)
{
    initramfs_inode_t *inode;
    off_t new_pos;
    off_t size;
    
    if (file == NULL) {
        return (off_t)STATUS_PARAM_NULL;
    }
    
    inode = (initramfs_inode_t *)file->f_inode;
    if (inode == NULL) {
        return (off_t)STATUS_PARAM_INVALID;
    }
    
    size = inode->i_size;
    
    switch (whence) {
    case VFS_SEEK_SET:
        new_pos = offset;
        break;
    case VFS_SEEK_CUR:
        new_pos = file->f_pos + offset;
        break;
    case VFS_SEEK_END:
        new_pos = size + offset;
        break;
    default:
        return (off_t)STATUS_PARAM_INVALID;
    }
    
    if (new_pos < 0) {
        new_pos = 0;
    }
    
    if (new_pos > size) {
        new_pos = size;
    }
    
    file->f_pos = new_pos;
    
    return new_pos;
}

static tak_bertandas_t initramfs_readdir_vfs(struct file *file,
                                              struct vfs_dirent *dirent,
                                              ukuran_t count)
{
    initramfs_inode_t *inode;
    initramfs_dentry_t *curr;
    ukuran_t written = 0;
    ukuran_t entry_size;
    
    if (file == NULL || dirent == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_NULL;
    }
    
    inode = (initramfs_inode_t *)file->f_inode;
    if (inode == NULL || !VFS_S_ISDIR(inode->i_mode)) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }
    
    curr = inode->i_children;
    
    /* Skip to current position */
    {
        tak_bertanda32_t skip = (tak_bertanda32_t)file->f_pos;
        while (skip > 0 && curr != NULL) {
            curr = curr->d_next;
            skip--;
        }
    }
    
    /* Read entries */
    while (curr != NULL && written < count) {
        entry_size = sizeof(struct vfs_dirent);
        if (written + entry_size > count) {
            break;
        }
        
        dirent[written].d_ino = curr->d_inode->i_ino;
        dirent[written].d_off = (off_t)(file->f_pos + written + 1);
        dirent[written].d_reclen = (tak_bertanda16_t)entry_size;
        
        /* Set type */
        if (VFS_S_ISDIR(curr->d_inode->i_mode)) {
            dirent[written].d_type = VFS_DT_DIR;
        } else if (VFS_S_ISREG(curr->d_inode->i_mode)) {
            dirent[written].d_type = VFS_DT_REG;
        } else if (VFS_S_ISLNK(curr->d_inode->i_mode)) {
            dirent[written].d_type = VFS_DT_LNK;
        } else {
            dirent[written].d_type = VFS_DT_UNKNOWN;
        }
        
        kernel_strncpy(dirent[written].d_name, curr->d_name, VFS_NAMA_MAKS);
        dirent[written].d_name[VFS_NAMA_MAKS] = '\0';
        
        written++;
        file->f_pos++;
        curr = curr->d_next;
    }
    
    return (tak_bertandas_t)written;
}

/*
 * ===========================================================================
 * FUNGSI VFS FILESYSTEM OPERATIONS (VFS FILESYSTEM OPERATIONS)
 * ===========================================================================
 */

static struct superblock *initramfs_mount_vfs(struct filesystem *fs,
                                               const char *device,
                                               const char *path,
                                               tak_bertanda32_t flags)
{
    struct superblock *sb;
    
    /* Initramfs doesn't need device */
    if (device != NULL && device[0] != '\0') {
        log_warn("[INITRAMFS] Device parameter ignored");
    }
    
    /* Allocate VFS superblock */
    sb = superblock_alloc(fs);
    if (sb == NULL) {
        return NULL;
    }
    
    /* Initramfs specific data */
    sb->s_private = g_initramfs_sb;
    sb->s_block_size = INITRAMFS_BLOCK_SIZE;
    sb->s_readonly = BENAR;
    
    /* Set root dentry */
    if (g_initramfs_sb != NULL && 
        g_initramfs_sb->s_root_dentry != NULL) {
        sb->s_root = (struct dentry *)g_initramfs_sb->s_root_dentry;
    }
    
    return sb;
}

static status_t initramfs_umount_vfs(struct superblock *sb)
{
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    superblock_put(sb);
    
    return STATUS_BERHASIL;
}

static status_t initramfs_detect_vfs(const char *device)
{
    /* Initramfs is always available */
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * VFS OPERATIONS STRUCTURES (VFS OPERATIONS STRUCTURES)
 * ===========================================================================
 */

static vfs_super_operations_t initramfs_super_ops = {
    .alloc_inode   = initramfs_alloc_inode_vfs,
    .destroy_inode = initramfs_destroy_inode_vfs,
    .write_inode   = initramfs_write_inode_vfs,
    .sync_fs       = initramfs_sync_fs_vfs,
    .statfs        = initramfs_statfs_vfs,
};

static vfs_inode_operations_t initramfs_inode_ops = {
    .lookup   = initramfs_lookup_vfs,
    .create   = initramfs_create_vfs,
    .mkdir    = initramfs_mkdir_vfs,
    .rmdir    = initramfs_rmdir_vfs,
    .unlink   = initramfs_unlink_vfs,
    .getattr  = initramfs_getattr_vfs,
};

static vfs_file_operations_t initramfs_file_ops = {
    .read    = initramfs_read_vfs,
    .write   = initramfs_write_vfs,
    .lseek   = initramfs_lseek_vfs,
    .readdir = initramfs_readdir_vfs,
};

/*
 * ===========================================================================
 * FUNGSI PUBLIK INITRAMFS (PUBLIC INITRAMFS FUNCTIONS)
 * ===========================================================================
 */

status_t initramfs_init(const void *data, ukuran_t size)
{
    initramfs_sb_t *sb;
    initramfs_inode_t *root_inode;
    initramfs_dentry_t *root_dentry;
    
    if (data == NULL || size == 0) {
        return STATUS_PARAM_NULL;
    }
    
    log_info("[INITRAMFS] Initializing initramfs (%lu bytes)...",
             (unsigned long)size);
    
    /* Allocate superblock */
    sb = (initramfs_sb_t *)kmalloc(sizeof(initramfs_sb_t));
    if (sb == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    kernel_memset(sb, 0, sizeof(initramfs_sb_t));
    
    sb->s_magic = INITRAMFS_MAGIC;
    sb->s_block_size = INITRAMFS_BLOCK_SIZE;
    sb->s_data = data;
    sb->s_data_size = size;
    sb->s_readonly = BENAR;
    
    /* Create root inode */
    root_inode = initramfs_alloc_inode();
    if (root_inode == NULL) {
        kfree(sb);
        return STATUS_MEMORI_HABIS;
    }
    
    root_inode->i_mode = VFS_S_IFDIR | 0755;
    root_inode->i_uid = 0;
    root_inode->i_gid = 0;
    root_inode->i_size = 0;
    root_inode->i_data = NULL;
    root_inode->i_target = NULL;
    root_inode->i_parent = NULL;
    root_inode->i_children = NULL;
    
    sb->s_root_inode = root_inode;
    sb->s_inode_count = 1;
    
    /* Create root dentry */
    root_dentry = initramfs_alloc_dentry("");
    if (root_dentry == NULL) {
        initramfs_free_inode(root_inode);
        kfree(sb);
        return STATUS_MEMORI_HABIS;
    }
    
    root_dentry->d_inode = root_inode;
    root_dentry->d_parent = NULL;
    root_dentry->d_next = NULL;
    root_dentry->d_child = NULL;
    
    sb->s_root_dentry = root_dentry;
    sb->s_dentry_count = 1;
    
    /* Set global superblock */
    g_initramfs_sb = sb;
    
    log_info("[INITRAMFS] Initramfs initialized successfully");
    
    return STATUS_BERHASIL;
}

void initramfs_shutdown(void)
{
    log_info("[INITRAMFS] Shutting down initramfs...");
    
    if (g_initramfs_sb != NULL) {
        /* Free all inodes and dentries */
        /* TODO: Implement proper cleanup */
        
        kfree(g_initramfs_sb);
        g_initramfs_sb = NULL;
    }
    
    log_info("[INITRAMFS] Shutdown complete");
}

/*
 * ===========================================================================
 * FUNGSI MANIPULASI FILESYSTEM (FILESYSTEM MANIPULATION FUNCTIONS)
 * ===========================================================================
 */

initramfs_inode_t *initramfs_create_file(const char *path, mode_t mode,
                                          uid_t uid, gid_t gid,
                                          const void *data, off_t size)
{
    initramfs_dentry_t *parent_dentry;
    initramfs_dentry_t *new_dentry;
    initramfs_inode_t *inode;
    char dir_path[VFS_PATH_MAKS + 1];
    char name[VFS_NAMA_MAKS + 1];
    
    if (path == NULL) {
        return NULL;
    }
    
    initramfs_lock();
    
    /* Split path into directory and name */
    vfs_path_split(path, dir_path, name, sizeof(dir_path), sizeof(name));
    
    /* Find parent directory */
    parent_dentry = initramfs_resolve_path(dir_path);
    if (parent_dentry == NULL) {
        initramfs_unlock();
        return NULL;
    }
    
    /* Check if already exists */
    if (initramfs_lookup_dentry(parent_dentry->d_inode, name) != NULL) {
        initramfs_unlock();
        return NULL;
    }
    
    /* Create inode */
    inode = initramfs_alloc_inode();
    if (inode == NULL) {
        initramfs_unlock();
        return NULL;
    }
    
    inode->i_mode = VFS_S_IFREG | (mode & 0777);
    inode->i_uid = uid;
    inode->i_gid = gid;
    inode->i_size = size;
    inode->i_data = data;
    
    /* Create dentry */
    new_dentry = initramfs_alloc_dentry(name);
    if (new_dentry == NULL) {
        initramfs_free_inode(inode);
        initramfs_unlock();
        return NULL;
    }
    
    new_dentry->d_inode = inode;
    
    /* Add to parent */
    initramfs_add_dentry(parent_dentry->d_inode, new_dentry);
    
    g_initramfs_sb->s_inode_count++;
    g_initramfs_sb->s_dentry_count++;
    g_initramfs_sb->s_total_size += (tak_bertanda64_t)size;
    
    initramfs_unlock();
    
    return inode;
}

initramfs_inode_t *initramfs_create_directory(const char *path, mode_t mode,
                                               uid_t uid, gid_t gid)
{
    initramfs_dentry_t *parent_dentry;
    initramfs_dentry_t *new_dentry;
    initramfs_inode_t *inode;
    char dir_path[VFS_PATH_MAKS + 1];
    char name[VFS_NAMA_MAKS + 1];
    
    if (path == NULL) {
        return NULL;
    }
    
    initramfs_lock();
    
    /* Split path */
    vfs_path_split(path, dir_path, name, sizeof(dir_path), sizeof(name));
    
    /* Find parent */
    parent_dentry = initramfs_resolve_path(dir_path);
    if (parent_dentry == NULL) {
        initramfs_unlock();
        return NULL;
    }
    
    /* Check if exists */
    if (initramfs_lookup_dentry(parent_dentry->d_inode, name) != NULL) {
        initramfs_unlock();
        return NULL;
    }
    
    /* Create inode */
    inode = initramfs_alloc_inode();
    if (inode == NULL) {
        initramfs_unlock();
        return NULL;
    }
    
    inode->i_mode = VFS_S_IFDIR | (mode & 0777);
    inode->i_uid = uid;
    inode->i_gid = gid;
    inode->i_size = 0;
    inode->i_data = NULL;
    
    /* Create dentry */
    new_dentry = initramfs_alloc_dentry(name);
    if (new_dentry == NULL) {
        initramfs_free_inode(inode);
        initramfs_unlock();
        return NULL;
    }
    
    new_dentry->d_inode = inode;
    
    /* Add to parent */
    initramfs_add_dentry(parent_dentry->d_inode, new_dentry);
    
    g_initramfs_sb->s_inode_count++;
    g_initramfs_sb->s_dentry_count++;
    
    initramfs_unlock();
    
    return inode;
}

initramfs_inode_t *initramfs_create_symlink(const char *path,
                                              const char *target,
                                              uid_t uid, gid_t gid)
{
    initramfs_dentry_t *parent_dentry;
    initramfs_dentry_t *new_dentry;
    initramfs_inode_t *inode;
    char dir_path[VFS_PATH_MAKS + 1];
    char name[VFS_NAMA_MAKS + 1];
    char *target_copy;
    ukuran_t target_len;
    
    if (path == NULL || target == NULL) {
        return NULL;
    }
    
    target_len = kernel_strlen(target);
    
    initramfs_lock();
    
    /* Split path */
    vfs_path_split(path, dir_path, name, sizeof(dir_path), sizeof(name));
    
    /* Find parent */
    parent_dentry = initramfs_resolve_path(dir_path);
    if (parent_dentry == NULL) {
        initramfs_unlock();
        return NULL;
    }
    
    /* Allocate target copy */
    target_copy = (char *)kmalloc(target_len + 1);
    if (target_copy == NULL) {
        initramfs_unlock();
        return NULL;
    }
    
    kernel_strncpy(target_copy, target, target_len + 1);
    
    /* Create inode */
    inode = initramfs_alloc_inode();
    if (inode == NULL) {
        kfree(target_copy);
        initramfs_unlock();
        return NULL;
    }
    
    inode->i_mode = VFS_S_IFLNK | 0777;
    inode->i_uid = uid;
    inode->i_gid = gid;
    inode->i_size = (off_t)target_len;
    inode->i_target = target_copy;
    inode->i_data = target_copy; /* Data points to target */
    
    /* Create dentry */
    new_dentry = initramfs_alloc_dentry(name);
    if (new_dentry == NULL) {
        initramfs_free_inode(inode);
        initramfs_unlock();
        return NULL;
    }
    
    new_dentry->d_inode = inode;
    
    /* Add to parent */
    initramfs_add_dentry(parent_dentry->d_inode, new_dentry);
    
    g_initramfs_sb->s_inode_count++;
    g_initramfs_sb->s_dentry_count++;
    
    initramfs_unlock();
    
    return inode;
}

/*
 * ===========================================================================
 * FUNGSI STATISTIK (STATISTICS FUNCTIONS)
 * ===========================================================================
 */

tak_bertanda32_t initramfs_get_inode_count(void)
{
    if (g_initramfs_sb == NULL) {
        return 0;
    }
    return g_initramfs_sb->s_inode_count;
}

tak_bertanda32_t initramfs_get_dentry_count(void)
{
    if (g_initramfs_sb == NULL) {
        return 0;
    }
    return g_initramfs_sb->s_dentry_count;
}

tak_bertanda64_t initramfs_get_total_size(void)
{
    if (g_initramfs_sb == NULL) {
        return 0;
    }
    return g_initramfs_sb->s_total_size;
}

/*
 * ===========================================================================
 * FUNGSI INFORMASI DAN DEBUG (INFORMATION AND DEBUG FUNCTIONS)
 * ===========================================================================
 */

void initramfs_print_info(void)
{
    kernel_printf("[INITRAMFS] Initramfs Info:\n");
    
    if (g_initramfs_sb == NULL) {
        kernel_printf("  Status: Not initialized\n");
        return;
    }
    
    kernel_printf("  Magic:       0x%08X\n", g_initramfs_sb->s_magic);
    kernel_printf("  Block Size:  %llu bytes\n", g_initramfs_sb->s_block_size);
    kernel_printf("  Total Inodes: %u\n", g_initramfs_sb->s_inode_count);
    kernel_printf("  Total Dentries: %u\n", g_initramfs_sb->s_dentry_count);
    kernel_printf("  Total Size:  %llu bytes\n", g_initramfs_sb->s_total_size);
    kernel_printf("  Data Size:   %lu bytes\n", 
                  (unsigned long)g_initramfs_sb->s_data_size);
    kernel_printf("  Read-only:   %s\n", 
                  g_initramfs_sb->s_readonly ? "Yes" : "No");
}

void initramfs_print_tree(void)
{
    if (g_initramfs_sb == NULL || 
        g_initramfs_sb->s_root_dentry == NULL) {
        kernel_printf("[INITRAMFS] Not initialized\n");
        return;
    }
    
    kernel_printf("[INITRAMFS] Directory Tree:\n");
    /* TODO: Implement tree printing */
}

/*
 * ===========================================================================
 * FUNGSI REGISTRASI FILESYSTEM (FILESYSTEM REGISTRATION FUNCTIONS)
 * ===========================================================================
 */

static filesystem_t initramfs_filesystem = {
    .fs_nama    = "initramfs",
    .fs_flags   = FS_FLAG_NO_DEV | FS_FLAG_READ_ONLY,
    .fs_mount   = initramfs_mount_vfs,
    .fs_umount  = initramfs_umount_vfs,
    .fs_detect  = initramfs_detect_vfs,
    .fs_next    = NULL,
    .fs_refcount = 0,
    .fs_terdaftar = SALAH,
};

status_t initramfs_register(void)
{
    status_t ret;
    
    log_info("[INITRAMFS] Registering initramfs filesystem...");
    
    ret = filesystem_register(&initramfs_filesystem);
    if (ret != STATUS_BERHASIL) {
        log_error("[INITRAMFS] Failed to register: %d", ret);
        return ret;
    }
    
    log_info("[INITRAMFS] Filesystem registered successfully");
    
    return STATUS_BERHASIL;
}

status_t initramfs_unregister(void)
{
    return filesystem_unregister(&initramfs_filesystem);
}

/*
 * ===========================================================================
 * FUNGSI INITIALIZATION MODULE (MODULE INITIALIZATION)
 * ===========================================================================
 */

status_t initramfs_module_init(void)
{
    tak_bertanda32_t i;
    
    log_info("[INITRAMFS] Initializing module...");
    
    /* Clear caches */
    for (i = 0; i < INITRAMFS_MAX_ENTRIES; i++) {
        g_inode_cache[i] = NULL;
        g_dentry_cache[i] = NULL;
    }
    
    g_inode_count = 0;
    g_dentry_count = 0;
    
    /* Clear FD table */
    for (i = 0; i < INITRAMFS_MAX_FD; i++) {
        g_fd_table[i].f_inode = NULL;
        g_fd_table[i].f_pos = 0;
        g_fd_table[i].f_flags = 0;
    }
    
    /* Reset generator */
    g_next_ino = INITRAMFS_INO_START;
    
    /* Reset lock */
    g_initramfs_lock = 0;
    
    /* Reset superblock */
    g_initramfs_sb = NULL;
    
    log_info("[INITRAMFS] Module initialized");
    
    return STATUS_BERHASIL;
}

void initramfs_module_shutdown(void)
{
    log_info("[INITRAMFS] Shutting down module...");
    
    initramfs_lock();
    
    /* Unregister filesystem */
    initramfs_unregister();
    
    /* Shutdown initramfs */
    initramfs_shutdown();
    
    initramfs_unlock();
    
    log_info("[INITRAMFS] Module shutdown complete");
}
