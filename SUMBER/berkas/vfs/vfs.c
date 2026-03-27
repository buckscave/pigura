/*
 * PIGURA OS - VFS.C
 * =================
 * Implementasi Virtual File System untuk kernel Pigura OS.
 *
 * Berkas ini berisi implementasi core VFS yang menyediakan
 * abstraksi file system yang seragam untuk berbagai filesystem.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua fungsi diimplementasikan secara lengkap
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "vfs.h"
#include "../../inti/kernel.h"
#include "../../inti/types.h"

/*
 * ===========================================================================
 * VARIABEL GLOBAL VFS (VFS GLOBAL VARIABLES)
 * ===========================================================================
 */

/* Daftar filesystem terdaftar */
static filesystem_t *g_fs_list = NULL;
static tak_bertanda32_t g_fs_count = 0;

/* Daftar mount point */
static mount_t *g_mount_list = NULL;
static tak_bertanda32_t g_mount_count = 0;

/* Root mount */
static mount_t *g_root_mount = NULL;

/* Inode cache */
static inode_t *g_inode_cache[VFS_INODE_CACHE_MAKS];
static tak_bertanda32_t g_inode_count = 0;

/* Dentry cache hash table */
#define DENTRY_HASH_SIZE 256
static dentry_t *g_dentry_hash[DENTRY_HASH_SIZE];
static tak_bertanda32_t g_dentry_count = 0;

/* File descriptor table */
static file_t *g_fd_table[VFS_FD_MAKS];
static tak_bertanda32_t g_fd_count = 0;

/* Current working directory per proses (simplified) */
static dentry_t *g_cwd_dentry = NULL;

/* Lock untuk VFS */
static tak_bertanda32_t g_vfs_lock = 0;

/* Flag inisialisasi */
static bool_t g_vfs_initialized = SALAH;

/*
 * ===========================================================================
 * FUNGSI INTERNAL LOCKING (INTERNAL LOCKING FUNCTIONS)
 * ===========================================================================
 */

static void vfs_lock(void)
{
    g_vfs_lock++;
}

static void vfs_unlock(void)
{
    if (g_vfs_lock > 0) {
        g_vfs_lock--;
    }
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL HASH (INTERNAL HASH FUNCTIONS)
 * ===========================================================================
 */

static tak_bertanda32_t dentry_hash_func(const char *nama,
                                         dentry_t *parent)
{
    tak_bertanda32_t hash = 5381;
    tak_bertanda32_t c;
    const char *str = nama;
    
    /* Hash string menggunakan djb2 algorithm */
    while ((c = (tak_bertanda32_t)(*str++)) != 0) {
        hash = ((hash << 5) + hash) + c;
    }
    
    /* XOR dengan pointer parent untuk uniqueness */
    hash ^= (tak_bertanda32_t)(uintptr_t)parent;
    
    return hash % DENTRY_HASH_SIZE;
}

static tak_bertanda32_t inode_hash_func(ino_t ino, dev_t dev)
{
    tak_bertanda32_t hash;
    
    hash = (tak_bertanda32_t)(ino & 0xFFFFFFFF);
    hash ^= (tak_bertanda32_t)(dev & 0xFFFF);
    hash ^= (tak_bertanda32_t)((dev >> 16) & 0xFFFF);
    
    return hash % VFS_INODE_CACHE_MAKS;
}

/*
 * ===========================================================================
 * FUNGSI UTILITAS PATH (PATH UTILITY FUNCTIONS)
 * ===========================================================================
 */

status_t vfs_path_normalize(const char *path, char *buffer, ukuran_t size)
{
    ukuran_t i;
    ukuran_t j;
    ukuran_t len;
    char temp[VFS_PATH_MAKS + 1];
    char *parts[64];
    tak_bertanda32_t part_count = 0;
    char *token;
    char *saveptr;
    
    if (path == NULL || buffer == NULL || size == 0) {
        return STATUS_PARAM_INVALID;
    }
    
    len = kernel_strlen(path);
    if (len == 0 || len >= VFS_PATH_MAKS) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Copy ke temp buffer */
    kernel_strncpy(temp, path, VFS_PATH_MAKS);
    temp[VFS_PATH_MAKS] = '\0';
    
    /* Handle absolute path */
    j = 0;
    if (temp[0] == '/') {
        buffer[j++] = '/';
    }
    
    /* Parse path components */
    token = kernel_strtok_r(temp, "/", &saveptr);
    while (token != NULL && part_count < 64) {
        if (kernel_strcmp(token, ".") == 0) {
            /* Skip current directory */
        } else if (kernel_strcmp(token, "..") == 0) {
            /* Go up one directory */
            if (part_count > 0) {
                part_count--;
            }
        } else {
            parts[part_count++] = token;
        }
        token = kernel_strtok_r(NULL, "/", &saveptr);
    }
    
    /* Reconstruct path */
    if (part_count == 0) {
        if (j == 0) {
            buffer[0] = '/';
            buffer[1] = '\0';
        } else {
            buffer[j] = '\0';
        }
        return STATUS_BERHASIL;
    }
    
    for (i = 0; i < part_count; i++) {
        len = kernel_strlen(parts[i]);
        if (j + len + 2 >= size) {
            return STATUS_PARAM_UKURAN;
        }
        
        if (j > 1 || (j == 1 && buffer[0] != '/')) {
            buffer[j++] = '/';
        } else if (j == 0) {
            buffer[j++] = '/';
        }
        
        kernel_strncpy(buffer + j, parts[i], len);
        j += len;
    }
    
    buffer[j] = '\0';
    return STATUS_BERHASIL;
}

status_t vfs_path_join(const char *dir, const char *name, char *buffer,
                       ukuran_t size)
{
    ukuran_t dir_len;
    ukuran_t name_len;
    ukuran_t total_len;
    
    if (dir == NULL || name == NULL || buffer == NULL || size == 0) {
        return STATUS_PARAM_INVALID;
    }
    
    dir_len = kernel_strlen(dir);
    name_len = kernel_strlen(name);
    
    if (dir_len >= VFS_PATH_MAKS || name_len >= VFS_NAMA_MAKS) {
        return STATUS_PARAM_UKURAN;
    }
    
    /* Hitung total panjang */
    total_len = dir_len + name_len;
    if (dir_len > 0 && dir[dir_len - 1] != '/') {
        total_len++;
    }
    
    if (total_len >= size) {
        return STATUS_PARAM_UKURAN;
    }
    
    /* Copy direktori */
    kernel_strncpy(buffer, dir, dir_len);
    buffer[dir_len] = '\0';
    
    /* Tambah separator jika perlu */
    if (dir_len > 0 && dir[dir_len - 1] != '/') {
        buffer[dir_len] = '/';
        buffer[dir_len + 1] = '\0';
        dir_len++;
    }
    
    /* Tambah nama */
    kernel_strncpy(buffer + dir_len, name, name_len);
    buffer[total_len] = '\0';
    
    return STATUS_BERHASIL;
}

status_t vfs_path_split(const char *path, char *dir, char *name,
                        ukuran_t dir_size, ukuran_t name_size)
{
    const char *last_slash;
    ukuran_t dir_len;
    ukuran_t name_len;
    
    if (path == NULL || dir == NULL || name == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Cari slash terakhir */
    last_slash = kernel_strrchr(path, '/');
    
    if (last_slash == NULL) {
        /* Tidak ada slash, seluruh path adalah nama */
        dir[0] = '\0';
        kernel_strncpy(name, path, name_size - 1);
        name[name_size - 1] = '\0';
    } else if (last_slash == path) {
        /* Slash di awal, root directory */
        dir[0] = '/';
        dir[1] = '\0';
        kernel_strncpy(name, last_slash + 1, name_size - 1);
        name[name_size - 1] = '\0';
    } else {
        /* Normal case */
        dir_len = (ukuran_t)(last_slash - path);
        if (dir_len >= dir_size) {
            return STATUS_PARAM_UKURAN;
        }
        kernel_strncpy(dir, path, dir_len);
        dir[dir_len] = '\0';
        
        name_len = kernel_strlen(last_slash + 1);
        if (name_len >= name_size) {
            return STATUS_PARAM_UKURAN;
        }
        kernel_strncpy(name, last_slash + 1, name_size - 1);
        name[name_size - 1] = '\0';
    }
    
    return STATUS_BERHASIL;
}

const char *vfs_path_basename(const char *path)
{
    const char *last_slash;
    
    if (path == NULL) {
        return "";
    }
    
    last_slash = kernel_strrchr(path, '/');
    
    if (last_slash == NULL) {
        return path;
    }
    
    return last_slash + 1;
}

status_t vfs_path_dirname(const char *path, char *buffer, ukuran_t size)
{
    const char *last_slash;
    ukuran_t dir_len;
    
    if (path == NULL || buffer == NULL || size == 0) {
        return STATUS_PARAM_INVALID;
    }
    
    last_slash = kernel_strrchr(path, '/');
    
    if (last_slash == NULL) {
        /* Tidak ada slash, parent adalah current directory */
        buffer[0] = '.';
        buffer[1] = '\0';
    } else if (last_slash == path) {
        /* Slash di awal, parent adalah root */
        if (size < 2) {
            return STATUS_PARAM_UKURAN;
        }
        buffer[0] = '/';
        buffer[1] = '\0';
    } else {
        /* Normal case */
        dir_len = (ukuran_t)(last_slash - path);
        if (dir_len >= size) {
            return STATUS_PARAM_UKURAN;
        }
        kernel_strncpy(buffer, path, dir_len);
        buffer[dir_len] = '\0';
    }
    
    return STATUS_BERHASIL;
}

bool_t vfs_path_valid(const char *path)
{
    ukuran_t len;
    ukuran_t i;
    
    if (path == NULL) {
        return SALAH;
    }
    
    len = kernel_strlen(path);
    if (len == 0 || len >= VFS_PATH_MAKS) {
        return SALAH;
    }
    
    /* Path harus diawali dengan / (absolute) */
    if (path[0] != '/') {
        return SALAH;
    }
    
    /* Cek karakter invalid */
    for (i = 0; i < len; i++) {
        char c = path[i];
        if (c == '\0') {
            break;
        }
        if (c < 32 || c > 126) {
            return SALAH;
        }
    }
    
    return BENAR;
}

bool_t vfs_name_valid(const char *name)
{
    ukuran_t len;
    ukuran_t i;
    
    if (name == NULL) {
        return SALAH;
    }
    
    len = kernel_strlen(name);
    if (len == 0 || len >= VFS_NAMA_MAKS) {
        return SALAH;
    }
    
    /* Cek karakter invalid */
    for (i = 0; i < len; i++) {
        char c = name[i];
        if (c == '\0') {
            break;
        }
        if (c == '/' || c < 32 || c > 126) {
            return SALAH;
        }
    }
    
    /* Nama tidak boleh . atau .. */
    if (kernel_strcmp(name, ".") == 0 || kernel_strcmp(name, "..") == 0) {
        return SALAH;
    }
    
    return BENAR;
}

/*
 * ===========================================================================
 * FUNGSI INISIALISASI VFS (VFS INITIALIZATION FUNCTIONS)
 * ===========================================================================
 */

status_t vfs_init(void)
{
    tak_bertanda32_t i;
    
    if (g_vfs_initialized) {
        return STATUS_BERHASIL;
    }
    
    log_info("[VFS] Menginisialisasi VFS...");
    
    /* Reset filesystem list */
    g_fs_list = NULL;
    g_fs_count = 0;
    
    /* Reset mount list */
    g_mount_list = NULL;
    g_mount_count = 0;
    g_root_mount = NULL;
    
    /* Reset inode cache */
    for (i = 0; i < VFS_INODE_CACHE_MAKS; i++) {
        g_inode_cache[i] = NULL;
    }
    g_inode_count = 0;
    
    /* Reset dentry cache */
    for (i = 0; i < DENTRY_HASH_SIZE; i++) {
        g_dentry_hash[i] = NULL;
    }
    g_dentry_count = 0;
    
    /* Reset file descriptor table */
    for (i = 0; i < VFS_FD_MAKS; i++) {
        g_fd_table[i] = NULL;
    }
    g_fd_count = 0;
    
    /* Reset CWD */
    g_cwd_dentry = NULL;
    
    /* Reset lock */
    g_vfs_lock = 0;
    
    g_vfs_initialized = BENAR;
    
    log_info("[VFS] Inisialisasi selesai");
    
    return STATUS_BERHASIL;
}

void vfs_shutdown(void)
{
    if (!g_vfs_initialized) {
        return;
    }
    
    log_info("[VFS] Mematikan VFS...");
    
    vfs_lock();
    
    /* Tutup semua file descriptor */
    /* TODO: Implementasi cleanup file descriptor */
    
    /* Unmount semua mount point */
    /* TODO: Implementasi unmount semua */
    
    /* Clear caches */
    kernel_memset(g_inode_cache, 0, sizeof(g_inode_cache));
    kernel_memset(g_dentry_hash, 0, sizeof(g_dentry_hash));
    kernel_memset(g_fd_table, 0, sizeof(g_fd_table));
    
    g_fs_list = NULL;
    g_mount_list = NULL;
    g_root_mount = NULL;
    g_cwd_dentry = NULL;
    
    g_fs_count = 0;
    g_mount_count = 0;
    g_inode_count = 0;
    g_dentry_count = 0;
    g_fd_count = 0;
    
    g_vfs_initialized = SALAH;
    
    vfs_unlock();
    
    log_info("[VFS] Shutdown selesai");
}

/*
 * ===========================================================================
 * FUNGSI STATISTIK VFS (VFS STATISTICS FUNCTIONS)
 * ===========================================================================
 */

tak_bertanda32_t vfs_get_mount_count(void)
{
    return g_mount_count;
}

tak_bertanda32_t vfs_get_inode_count(void)
{
    return g_inode_count;
}

tak_bertanda32_t vfs_get_dentry_count(void)
{
    return g_dentry_count;
}

tak_bertanda32_t vfs_get_file_count(void)
{
    return g_fd_count;
}

tak_bertanda32_t vfs_get_fs_count(void)
{
    return g_fs_count;
}

/*
 * ===========================================================================
 * FUNGSI FILESYSTEM REGISTRATION (FILESYSTEM REGISTRATION FUNCTIONS)
 * ===========================================================================
 */

status_t filesystem_register(filesystem_t *fs)
{
    filesystem_t *curr;
    
    if (fs == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (!vfs_name_valid(fs->fs_nama)) {
        log_error("[VFS] Nama filesystem invalid: %s", fs->fs_nama);
        return STATUS_PARAM_INVALID;
    }
    
    vfs_lock();
    
    /* Cek apakah sudah terdaftar */
    curr = g_fs_list;
    while (curr != NULL) {
        if (kernel_strcmp(curr->fs_nama, fs->fs_nama) == 0) {
            vfs_unlock();
            log_warn("[VFS] Filesystem sudah terdaftar: %s", fs->fs_nama);
            return STATUS_SUDAH_ADA;
        }
        curr = curr->fs_next;
    }
    
    /* Tambah ke list */
    fs->fs_next = g_fs_list;
    fs->fs_refcount = 0;
    fs->fs_terdaftar = BENAR;
    g_fs_list = fs;
    g_fs_count++;
    
    vfs_unlock();
    
    log_info("[VFS] Filesystem terdaftar: %s", fs->fs_nama);
    
    return STATUS_BERHASIL;
}

status_t filesystem_unregister(filesystem_t *fs)
{
    filesystem_t *curr;
    filesystem_t *prev = NULL;
    
    if (fs == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    vfs_lock();
    
    curr = g_fs_list;
    while (curr != NULL) {
        if (curr == fs) {
            /* Cek apakah masih ada mount point aktif */
            if (fs->fs_refcount > 0) {
                vfs_unlock();
                log_warn("[VFS] Filesystem masih digunakan: %s", fs->fs_nama);
                return STATUS_BUSY;
            }
            
            /* Hapus dari list */
            if (prev == NULL) {
                g_fs_list = fs->fs_next;
            } else {
                prev->fs_next = fs->fs_next;
            }
            
            fs->fs_next = NULL;
            fs->fs_terdaftar = SALAH;
            g_fs_count--;
            
            vfs_unlock();
            
            log_info("[VFS] Filesystem tidak terdaftar: %s", fs->fs_nama);
            
            return STATUS_BERHASIL;
        }
        prev = curr;
        curr = curr->fs_next;
    }
    
    vfs_unlock();
    
    return STATUS_TIDAK_DITEMUKAN;
}

filesystem_t *filesystem_find(const char *nama)
{
    filesystem_t *curr;
    
    if (nama == NULL) {
        return NULL;
    }
    
    vfs_lock();
    
    curr = g_fs_list;
    while (curr != NULL) {
        if (kernel_strcmp(curr->fs_nama, nama) == 0) {
            vfs_unlock();
            return curr;
        }
        curr = curr->fs_next;
    }
    
    vfs_unlock();
    
    return NULL;
}

filesystem_t *filesystem_detect(const char *device)
{
    filesystem_t *curr;
    filesystem_t *detected = NULL;
    
    if (device == NULL) {
        return NULL;
    }
    
    vfs_lock();
    
    curr = g_fs_list;
    while (curr != NULL) {
        if (curr->fs_detect != NULL) {
            if (curr->fs_detect(device) == STATUS_BERHASIL) {
                detected = curr;
                break;
            }
        }
        curr = curr->fs_next;
    }
    
    vfs_unlock();
    
    return detected;
}

filesystem_t *filesystem_first(void)
{
    return g_fs_list;
}

filesystem_t *filesystem_next(filesystem_t *fs)
{
    if (fs == NULL) {
        return NULL;
    }
    return fs->fs_next;
}

/*
 * ===========================================================================
 * FUNGSI SUPERBLOCK (SUPERBLOCK FUNCTIONS)
 * ===========================================================================
 */

superblock_t *superblock_alloc(filesystem_t *fs)
{
    superblock_t *sb;
    
    if (fs == NULL) {
        return NULL;
    }
    
    sb = (superblock_t *)kmalloc(sizeof(superblock_t));
    if (sb == NULL) {
        log_error("[VFS] Gagal alokasi superblock");
        return NULL;
    }
    
    kernel_memset(sb, 0, sizeof(superblock_t));
    
    sb->s_magic = VFS_SUPER_MAGIC;
    sb->s_fs = fs;
    sb->s_refcount = 1;
    sb->s_readonly = SALAH;
    sb->s_dirty = SALAH;
    
    fs->fs_refcount++;
    
    return sb;
}

void superblock_free(superblock_t *sb)
{
    if (sb == NULL) {
        return;
    }
    
    if (sb->s_magic != VFS_SUPER_MAGIC) {
        return;
    }
    
    if (sb->s_refcount > 0) {
        return;
    }
    
    /* Panggil put_super jika ada */
    if (sb->s_op != NULL && sb->s_op->put_super != NULL) {
        sb->s_op->put_super(sb);
    }
    
    /* Kurangi reference filesystem */
    if (sb->s_fs != NULL) {
        sb->s_fs->fs_refcount--;
    }
    
    /* Clear dan free */
    kernel_memset(sb, 0, sizeof(superblock_t));
    kfree(sb);
}

status_t superblock_init(superblock_t *sb, filesystem_t *fs, dev_t dev)
{
    if (sb == NULL || fs == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    sb->s_dev = dev;
    sb->s_fs = fs;
    sb->s_block_size = 4096;  /* Default */
    sb->s_refcount = 1;
    
    return STATUS_BERHASIL;
}

void superblock_get(superblock_t *sb)
{
    if (sb == NULL || sb->s_magic != VFS_SUPER_MAGIC) {
        return;
    }
    
    sb->s_refcount++;
}

void superblock_put(superblock_t *sb)
{
    if (sb == NULL || sb->s_magic != VFS_SUPER_MAGIC) {
        return;
    }
    
    if (sb->s_refcount > 0) {
        sb->s_refcount--;
        
        if (sb->s_refcount == 0) {
            superblock_free(sb);
        }
    }
}

status_t superblock_sync(superblock_t *sb)
{
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (sb->s_magic != VFS_SUPER_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    if (!sb->s_dirty) {
        return STATUS_BERHASIL;
    }
    
    /* Panggil sync_fs jika ada */
    if (sb->s_op != NULL && sb->s_op->sync_fs != NULL) {
        status_t ret = sb->s_op->sync_fs(sb);
        if (ret == STATUS_BERHASIL) {
            sb->s_dirty = SALAH;
        }
        return ret;
    }
    
    return STATUS_BERHASIL;
}

status_t superblock_statfs(superblock_t *sb, vfs_stat_t *stat)
{
    if (sb == NULL || stat == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (sb->s_magic != VFS_SUPER_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Panggil statfs jika ada */
    if (sb->s_op != NULL && sb->s_op->statfs != NULL) {
        return sb->s_op->statfs(sb, stat);
    }
    
    /* Default values */
    kernel_memset(stat, 0, sizeof(vfs_stat_t));
    stat->st_dev = sb->s_dev;
    stat->st_blksize = (tak_bertanda64_t)sb->s_block_size;
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI INODE (INODE FUNCTIONS)
 * ===========================================================================
 */

inode_t *inode_alloc(superblock_t *sb)
{
    inode_t *inode;
    
    if (sb == NULL) {
        return NULL;
    }
    
    inode = (inode_t *)kmalloc(sizeof(inode_t));
    if (inode == NULL) {
        log_error("[VFS] Gagal alokasi inode");
        return NULL;
    }
    
    kernel_memset(inode, 0, sizeof(inode_t));
    
    inode->i_magic = VFS_INODE_MAGIC;
    inode->i_sb = sb;
    inode->i_refcount = 1;
    inode->i_blksize = (tak_bertanda32_t)sb->s_block_size;
    
    /* Default timestamps */
    inode->i_atime = kernel_get_uptime();
    inode->i_mtime = inode->i_atime;
    inode->i_ctime = inode->i_atime;
    
    /* Tambah ke cache */
    if (g_inode_count < VFS_INODE_CACHE_MAKS) {
        tak_bertanda32_t hash = inode_hash_func(inode->i_ino, sb->s_dev);
        if (g_inode_cache[hash] == NULL) {
            g_inode_cache[hash] = inode;
            g_inode_count++;
        }
    }
    
    /* Reference superblock */
    superblock_get(sb);
    
    return inode;
}

void inode_free(inode_t *inode)
{
    if (inode == NULL) {
        return;
    }
    
    if (inode->i_magic != VFS_INODE_MAGIC) {
        return;
    }
    
    if (inode->i_refcount > 0) {
        return;
    }
    
    /* Hapus dari cache */
    if (g_inode_count > 0) {
        tak_bertanda32_t hash = inode_hash_func(inode->i_ino,
                                inode->i_sb->s_dev);
        if (g_inode_cache[hash] == inode) {
            g_inode_cache[hash] = NULL;
            g_inode_count--;
        }
    }
    
    /* Panggil destroy_inode jika ada */
    if (inode->i_sb != NULL && inode->i_sb->s_op != NULL &&
        inode->i_sb->s_op->destroy_inode != NULL) {
        inode->i_sb->s_op->destroy_inode(inode);
    }
    
    /* Release superblock reference */
    if (inode->i_sb != NULL) {
        superblock_put(inode->i_sb);
    }
    
    /* Clear dan free */
    kernel_memset(inode, 0, sizeof(inode_t));
    kfree(inode);
}

status_t inode_init(inode_t *inode, superblock_t *sb, ino_t ino,
                    mode_t mode)
{
    if (inode == NULL || sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    inode->i_ino = ino;
    inode->i_mode = mode;
    inode->i_sb = sb;
    inode->i_nlink = 1;
    
    return STATUS_BERHASIL;
}

void inode_get(inode_t *inode)
{
    if (inode == NULL || inode->i_magic != VFS_INODE_MAGIC) {
        return;
    }
    
    inode->i_refcount++;
}

void inode_put(inode_t *inode)
{
    if (inode == NULL || inode->i_magic != VFS_INODE_MAGIC) {
        return;
    }
    
    if (inode->i_refcount > 0) {
        inode->i_refcount--;
        
        if (inode->i_refcount == 0) {
            /* Sync jika dirty */
            if (inode->i_dirty) {
                inode_write(inode);
            }
            inode_free(inode);
        }
    }
}

status_t inode_read(inode_t *inode)
{
    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (inode->i_magic != VFS_INODE_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    if (inode->i_sb == NULL || inode->i_sb->s_op == NULL ||
        inode->i_sb->s_op->read_inode == NULL) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    return inode->i_sb->s_op->read_inode(inode);
}

status_t inode_write(inode_t *inode)
{
    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (inode->i_magic != VFS_INODE_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    if (inode->i_sb == NULL || inode->i_sb->s_op == NULL ||
        inode->i_sb->s_op->write_inode == NULL) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    status_t ret = inode->i_sb->s_op->write_inode(inode);
    if (ret == STATUS_BERHASIL) {
        inode->i_dirty = SALAH;
    }
    
    return ret;
}

status_t inode_mark_dirty(inode_t *inode)
{
    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (inode->i_magic != VFS_INODE_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    inode->i_dirty = BENAR;
    
    if (inode->i_sb != NULL) {
        inode->i_sb->s_dirty = BENAR;
    }
    
    /* Panggil dirty_inode jika ada */
    if (inode->i_sb != NULL && inode->i_sb->s_op != NULL &&
        inode->i_sb->s_op->dirty_inode != NULL) {
        inode->i_sb->s_op->dirty_inode(inode);
    }
    
    return STATUS_BERHASIL;
}

status_t inode_sync(inode_t *inode)
{
    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (inode->i_dirty) {
        return inode_write(inode);
    }
    
    return STATUS_BERHASIL;
}

inode_t *inode_lookup(inode_t *dir, const char *name)
{
    dentry_t *dentry;
    
    if (dir == NULL || name == NULL) {
        return NULL;
    }
    
    if (!VFS_S_ISDIR(dir->i_mode)) {
        return NULL;
    }
    
    if (dir->i_op == NULL || dir->i_op->lookup == NULL) {
        return NULL;
    }
    
    dentry = dir->i_op->lookup(dir, name);
    if (dentry == NULL) {
        return NULL;
    }
    
    return dentry->d_inode;
}

status_t inode_permission(inode_t *inode, tak_bertanda32_t mask)
{
    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (inode->i_op != NULL && inode->i_op->permission != NULL) {
        return inode->i_op->permission(inode, mask);
    }
    
    /* Default permission check */
    return STATUS_BERHASIL;
}

bool_t inode_permission_check(inode_t *inode, uid_t uid, gid_t gid,
                              tak_bertanda32_t mask)
{
    mode_t mode;
    mode_t perm;
    
    if (inode == NULL) {
        return SALAH;
    }
    
    mode = inode->i_mode;
    
    /* Root selalu punya akses */
    if (uid == 0) {
        return BENAR;
    }
    
    /* Owner permission */
    if (inode->i_uid == uid) {
        perm = (mode >> 6) & 7;
    }
    /* Group permission */
    else if (inode->i_gid == gid) {
        perm = (mode >> 3) & 7;
    }
    /* Other permission */
    else {
        perm = mode & 7;
    }
    
    /* Cek permission */
    if ((mask & VFS_OPEN_RDONLY) && !(perm & 4)) {
        return SALAH;
    }
    if ((mask & VFS_OPEN_WRONLY) && !(perm & 2)) {
        return SALAH;
    }
    
    return BENAR;
}

status_t inode_getattr(inode_t *inode, vfs_stat_t *stat)
{
    if (inode == NULL || stat == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (inode->i_op != NULL && inode->i_op->getattr != NULL) {
        return inode->i_op->getattr(inode->i_dentry, stat);
    }
    
    /* Default getattr */
    kernel_memset(stat, 0, sizeof(vfs_stat_t));
    stat->st_ino = inode->i_ino;
    stat->st_mode = inode->i_mode;
    stat->st_nlink = inode->i_nlink;
    stat->st_uid = inode->i_uid;
    stat->st_gid = inode->i_gid;
    stat->st_size = inode->i_size;
    stat->st_atime = inode->i_atime;
    stat->st_mtime = inode->i_mtime;
    stat->st_ctime = inode->i_ctime;
    stat->st_blksize = inode->i_blksize;
    stat->st_blocks = inode->i_blocks;
    
    if (inode->i_sb != NULL) {
        stat->st_dev = inode->i_sb->s_dev;
    }
    
    return STATUS_BERHASIL;
}

status_t inode_setattr(inode_t *inode, vfs_stat_t *stat)
{
    if (inode == NULL || stat == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (inode->i_op != NULL && inode->i_op->setattr != NULL) {
        return inode->i_op->setattr(inode->i_dentry, stat);
    }
    
    /* Default setattr */
    if (stat->st_mode != 0) {
        inode->i_mode = stat->st_mode;
    }
    if (stat->st_uid != 0) {
        inode->i_uid = stat->st_uid;
    }
    if (stat->st_gid != 0) {
        inode->i_gid = stat->st_gid;
    }
    if (stat->st_size != 0) {
        inode->i_size = stat->st_size;
    }
    
    inode->i_mtime = kernel_get_uptime();
    inode->i_dirty = BENAR;
    
    return STATUS_BERHASIL;
}

bool_t inode_is_regular(inode_t *inode)
{
    if (inode == NULL) {
        return SALAH;
    }
    return VFS_S_ISREG(inode->i_mode);
}

bool_t inode_is_directory(inode_t *inode)
{
    if (inode == NULL) {
        return SALAH;
    }
    return VFS_S_ISDIR(inode->i_mode);
}

bool_t inode_is_symlink(inode_t *inode)
{
    if (inode == NULL) {
        return SALAH;
    }
    return VFS_S_ISLNK(inode->i_mode);
}

bool_t inode_is_device(inode_t *inode)
{
    if (inode == NULL) {
        return SALAH;
    }
    return VFS_S_ISCHR(inode->i_mode) || VFS_S_ISBLK(inode->i_mode);
}

/*
 * ===========================================================================
 * FUNGSI DENTRY (DENTRY FUNCTIONS)
 * ===========================================================================
 */

dentry_t *dentry_alloc(const char *name)
{
    dentry_t *dentry;
    ukuran_t namelen;
    
    if (name == NULL) {
        return NULL;
    }
    
    namelen = kernel_strlen(name);
    if (namelen == 0 || namelen >= VFS_NAMA_MAKS) {
        return NULL;
    }
    
    dentry = (dentry_t *)kmalloc(sizeof(dentry_t));
    if (dentry == NULL) {
        log_error("[VFS] Gagal alokasi dentry");
        return NULL;
    }
    
    kernel_memset(dentry, 0, sizeof(dentry_t));
    
    dentry->d_magic = VFS_DENTRY_MAGIC;
    kernel_strncpy(dentry->d_name, name, VFS_NAMA_MAKS);
    dentry->d_namelen = namelen;
    dentry->d_refcount = 1;
    
    return dentry;
}

void dentry_free(dentry_t *dentry)
{
    if (dentry == NULL) {
        return;
    }
    
    if (dentry->d_magic != VFS_DENTRY_MAGIC) {
        return;
    }
    
    if (dentry->d_refcount > 0) {
        return;
    }
    
    /* Panggil d_release jika ada */
    if (dentry->d_op != NULL && dentry->d_op->d_release != NULL) {
        dentry->d_op->d_release(dentry);
    }
    
    /* Release inode reference */
    if (dentry->d_inode != NULL) {
        inode_put(dentry->d_inode);
    }
    
    /* Clear dan free */
    kernel_memset(dentry, 0, sizeof(dentry_t));
    kfree(dentry);
}

status_t dentry_init(dentry_t *dentry, const char *name,
                     dentry_t *parent, inode_t *inode)
{
    if (dentry == NULL || name == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    kernel_strncpy(dentry->d_name, name, VFS_NAMA_MAKS);
    dentry->d_namelen = kernel_strlen(name);
    dentry->d_parent = parent;
    dentry->d_inode = inode;
    
    if (inode != NULL) {
        dentry->d_sb = inode->i_sb;
        inode_get(inode);
    }
    
    if (parent != NULL) {
        dentry_get(parent);
    }
    
    return STATUS_BERHASIL;
}

void dentry_get(dentry_t *dentry)
{
    if (dentry == NULL || dentry->d_magic != VFS_DENTRY_MAGIC) {
        return;
    }
    
    dentry->d_refcount++;
}

void dentry_put(dentry_t *dentry)
{
    if (dentry == NULL || dentry->d_magic != VFS_DENTRY_MAGIC) {
        return;
    }
    
    if (dentry->d_refcount > 0) {
        dentry->d_refcount--;
        
        if (dentry->d_refcount == 0) {
            dentry_free(dentry);
        }
    }
}

dentry_t *dentry_lookup(dentry_t *parent, const char *name)
{
    dentry_t *dentry;
    tak_bertanda32_t hash;
    
    if (parent == NULL || name == NULL) {
        return NULL;
    }
    
    /* Cari di hash table */
    hash = dentry_hash_func(name, parent);
    dentry = g_dentry_hash[hash];
    
    while (dentry != NULL) {
        if (dentry->d_parent == parent &&
            kernel_strcmp(dentry->d_name, name) == 0) {
            /* Revalidate jika perlu */
            if (dentry->d_op != NULL &&
                dentry->d_op->d_revalidate != NULL) {
                if (dentry->d_op->d_revalidate(dentry) != STATUS_BERHASIL) {
                    return NULL;
                }
            }
            dentry_get(dentry);
            return dentry;
        }
        dentry = dentry->d_next;
    }
    
    return NULL;
}

void dentry_cache_insert(dentry_t *dentry)
{
    tak_bertanda32_t hash;
    
    if (dentry == NULL) {
        return;
    }
    
    hash = dentry_hash_func(dentry->d_name, dentry->d_parent);
    
    dentry->d_next = g_dentry_hash[hash];
    dentry->d_prev = NULL;
    
    if (g_dentry_hash[hash] != NULL) {
        g_dentry_hash[hash]->d_prev = dentry;
    }
    
    g_dentry_hash[hash] = dentry;
    g_dentry_count++;
}

void dentry_cache_remove(dentry_t *dentry)
{
    if (dentry == NULL) {
        return;
    }
    
    if (dentry->d_prev != NULL) {
        dentry->d_prev->d_next = dentry->d_next;
    }
    
    if (dentry->d_next != NULL) {
        dentry->d_next->d_prev = dentry->d_prev;
    }
    
    /* Update hash head jika perlu */
    if (dentry->d_prev == NULL) {
        tak_bertanda32_t hash = dentry_hash_func(dentry->d_name,
                                dentry->d_parent);
        if (g_dentry_hash[hash] == dentry) {
            g_dentry_hash[hash] = dentry->d_next;
        }
    }
    
    dentry->d_next = NULL;
    dentry->d_prev = NULL;
    
    if (g_dentry_count > 0) {
        g_dentry_count--;
    }
}

void dentry_cache_invalidate(dentry_t *dentry)
{
    if (dentry == NULL) {
        return;
    }
    
    dentry_cache_remove(dentry);
    
    if (dentry->d_op != NULL && dentry->d_op->d_delete != NULL) {
        dentry->d_op->d_delete(dentry);
    }
}

status_t dentry_add_child(dentry_t *parent, dentry_t *child)
{
    if (parent == NULL || child == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    child->d_parent = parent;
    child->d_sibling = parent->d_child;
    parent->d_child = child;
    
    dentry_get(parent);
    
    return STATUS_BERHASIL;
}

status_t dentry_remove_child(dentry_t *parent, dentry_t *child)
{
    dentry_t *curr;
    dentry_t *prev = NULL;
    
    if (parent == NULL || child == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    curr = parent->d_child;
    while (curr != NULL) {
        if (curr == child) {
            if (prev == NULL) {
                parent->d_child = child->d_sibling;
            } else {
                prev->d_sibling = child->d_sibling;
            }
            
            child->d_parent = NULL;
            child->d_sibling = NULL;
            
            dentry_put(parent);
            
            return STATUS_BERHASIL;
        }
        prev = curr;
        curr = curr->d_sibling;
    }
    
    return STATUS_TIDAK_DITEMUKAN;
}

dentry_t *dentry_find_child(dentry_t *parent, const char *name)
{
    dentry_t *curr;
    
    if (parent == NULL || name == NULL) {
        return NULL;
    }
    
    curr = parent->d_child;
    while (curr != NULL) {
        if (kernel_strcmp(curr->d_name, name) == 0) {
            dentry_get(curr);
            return curr;
        }
        curr = curr->d_sibling;
    }
    
    return NULL;
}

status_t dentry_revalidate(dentry_t *dentry)
{
    if (dentry == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (dentry->d_op != NULL && dentry->d_op->d_revalidate != NULL) {
        return dentry->d_op->d_revalidate(dentry);
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI FILE (FILE FUNCTIONS)
 * ===========================================================================
 */

file_t *file_alloc(void)
{
    file_t *file;
    
    file = (file_t *)kmalloc(sizeof(file_t));
    if (file == NULL) {
        log_error("[VFS] Gagal alokasi file");
        return NULL;
    }
    
    kernel_memset(file, 0, sizeof(file_t));
    
    file->f_magic = VFS_FILE_MAGIC;
    file->f_refcount = 1;
    file->f_pos = 0;
    
    return file;
}

void file_free(file_t *file)
{
    if (file == NULL) {
        return;
    }
    
    if (file->f_magic != VFS_FILE_MAGIC) {
        return;
    }
    
    if (file->f_refcount > 0) {
        return;
    }
    
    /* Release inode dan dentry */
    if (file->f_inode != NULL) {
        inode_put(file->f_inode);
    }
    if (file->f_dentry != NULL) {
        dentry_put(file->f_dentry);
    }
    if (file->f_mount != NULL) {
        mount_put(file->f_mount);
    }
    
    /* Clear dan free */
    kernel_memset(file, 0, sizeof(file_t));
    kfree(file);
}

status_t file_init(file_t *file, inode_t *inode, dentry_t *dentry,
                   tak_bertanda32_t flags)
{
    if (file == NULL || inode == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    file->f_inode = inode;
    file->f_dentry = dentry;
    file->f_flags = flags;
    file->f_pos = 0;
    
    /* Setup mode dari flags */
    file->f_mode = (mode_t)(flags & 3);
    file->f_readable = (file->f_mode & VFS_OPEN_RDONLY) != 0;
    file->f_writable = (file->f_mode & VFS_OPEN_WRONLY) != 0;
    file->f_append = (flags & VFS_OPEN_APPEND) != 0;
    
    /* Reference */
    inode_get(inode);
    if (dentry != NULL) {
        dentry_get(dentry);
    }
    
    /* Setup operations */
    file->f_op = inode->i_fop;
    
    return STATUS_BERHASIL;
}

void file_get(file_t *file)
{
    if (file == NULL || file->f_magic != VFS_FILE_MAGIC) {
        return;
    }
    
    file->f_refcount++;
}

void file_put(file_t *file)
{
    if (file == NULL || file->f_magic != VFS_FILE_MAGIC) {
        return;
    }
    
    if (file->f_refcount > 0) {
        file->f_refcount--;
        
        if (file->f_refcount == 0) {
            file_free(file);
        }
    }
}

tak_bertandas_t file_read(file_t *file, void *buffer, ukuran_t size)
{
    tak_bertandas_t ret;
    
    if (file == NULL || buffer == NULL || size == 0) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }
    
    if (!file->f_readable) {
        return (tak_bertandas_t)STATUS_AKSES_DITOLAK;
    }
    
    if (file->f_op == NULL || file->f_op->read == NULL) {
        return (tak_bertandas_t)STATUS_TIDAK_DUKUNG;
    }
    
    ret = file->f_op->read(file, buffer, size, &file->f_pos);
    
    return ret;
}

tak_bertandas_t file_write(file_t *file, const void *buffer,
                           ukuran_t size)
{
    tak_bertandas_t ret;
    
    if (file == NULL || buffer == NULL || size == 0) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }
    
    if (!file->f_writable) {
        return (tak_bertandas_t)STATUS_AKSES_DITOLAK;
    }
    
    if (file->f_op == NULL || file->f_op->write == NULL) {
        return (tak_bertandas_t)STATUS_TIDAK_DUKUNG;
    }
    
    /* Append mode: seek ke akhir */
    if (file->f_append && file->f_inode != NULL) {
        file->f_pos = file->f_inode->i_size;
    }
    
    ret = file->f_op->write(file, buffer, size, &file->f_pos);
    
    return ret;
}

off_t file_lseek(file_t *file, off_t offset, tak_bertanda32_t whence)
{
    off_t new_pos;
    
    if (file == NULL) {
        return (off_t)STATUS_PARAM_NULL;
    }
    
    if (file->f_op == NULL || file->f_op->lseek == NULL) {
        /* Default lseek implementation */
        switch (whence) {
            case VFS_SEEK_SET:
                new_pos = offset;
                break;
            case VFS_SEEK_CUR:
                new_pos = file->f_pos + offset;
                break;
            case VFS_SEEK_END:
                if (file->f_inode == NULL) {
                    return (off_t)STATUS_PARAM_INVALID;
                }
                new_pos = file->f_inode->i_size + offset;
                break;
            default:
                return (off_t)STATUS_PARAM_INVALID;
        }
        
        if (new_pos < 0) {
            return (off_t)STATUS_PARAM_INVALID;
        }
        
        file->f_pos = new_pos;
        return new_pos;
    }
    
    return file->f_op->lseek(file, offset, whence);
}

status_t file_flush(file_t *file)
{
    if (file == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (file->f_op == NULL || file->f_op->flush == NULL) {
        return STATUS_BERHASIL;
    }
    
    return file->f_op->flush(file);
}

status_t file_fsync(file_t *file)
{
    if (file == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (file->f_op == NULL || file->f_op->fsync == NULL) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    return file->f_op->fsync(file);
}

tak_bertandas_t file_readdir(file_t *file, vfs_dirent_t *dirent,
                             ukuran_t count)
{
    if (file == NULL || dirent == NULL || count == 0) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }
    
    if (file->f_inode == NULL || !VFS_S_ISDIR(file->f_inode->i_mode)) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }
    
    if (file->f_op == NULL || file->f_op->readdir == NULL) {
        return (tak_bertandas_t)STATUS_TIDAK_DUKUNG;
    }
    
    return file->f_op->readdir(file, dirent, count);
}

status_t file_open(inode_t *inode, file_t *file)
{
    if (inode == NULL || file == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (file->f_op == NULL || file->f_op->open == NULL) {
        return STATUS_BERHASIL;
    }
    
    return file->f_op->open(inode, file);
}

status_t file_release(file_t *file)
{
    if (file == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (file->f_op == NULL || file->f_op->release == NULL) {
        return STATUS_BERHASIL;
    }
    
    return file->f_op->release(file->f_inode, file);
}

status_t file_ioctl(file_t *file, tak_bertanda32_t cmd,
                    tak_bertanda64_t arg)
{
    if (file == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (file->f_op == NULL || file->f_op->ioctl == NULL) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    return file->f_op->ioctl(file, cmd, arg);
}

/*
 * ===========================================================================
 * FUNGSI MOUNT (MOUNT FUNCTIONS)
 * ===========================================================================
 */

status_t mount_create(const char *device, const char *path,
                      const char *fs_nama, tak_bertanda32_t flags)
{
    filesystem_t *fs;
    mount_t *mount;
    superblock_t *sb;
    dentry_t *mountpoint;
    
    if (path == NULL || fs_nama == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Cari filesystem */
    fs = filesystem_find(fs_nama);
    if (fs == NULL) {
        log_error("[VFS] Filesystem tidak ditemukan: %s", fs_nama);
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Cek apakah path sudah di-mount */
    if (mount_find(path) != NULL) {
        log_warn("[VFS] Path sudah di-mount: %s", path);
        return STATUS_SUDAH_ADA;
    }
    
    /* Resolve mountpoint */
    if (g_root_mount != NULL) {
        mountpoint = namei_lookup(path);
        if (mountpoint == NULL) {
            log_error("[VFS] Mount point tidak ditemukan: %s", path);
            return STATUS_TIDAK_DITEMUKAN;
        }
        
        if (!VFS_S_ISDIR(mountpoint->d_inode->i_mode)) {
            dentry_put(mountpoint);
            return STATUS_PARAM_INVALID;
        }
    } else {
        mountpoint = NULL;
    }
    
    /* Panggil filesystem mount */
    if (fs->fs_mount == NULL) {
        if (mountpoint != NULL) {
            dentry_put(mountpoint);
        }
        return STATUS_TIDAK_DUKUNG;
    }
    
    sb = fs->fs_mount(fs, device, path, flags);
    if (sb == NULL) {
        if (mountpoint != NULL) {
            dentry_put(mountpoint);
        }
        log_error("[VFS] Gagal mount filesystem: %s", fs_nama);
        return STATUS_GAGAL;
    }
    
    /* Alokasi mount structure */
    mount = (mount_t *)kmalloc(sizeof(mount_t));
    if (mount == NULL) {
        if (mountpoint != NULL) {
            dentry_put(mountpoint);
        }
        /* Perlu cleanup superblock */
        return STATUS_MEMORI_HABIS;
    }
    
    kernel_memset(mount, 0, sizeof(mount_t));
    
    mount->m_magic = VFS_MOUNT_MAGIC;
    mount->m_fs = fs;
    mount->m_sb = sb;
    mount->m_mountpoint = mountpoint;
    mount->m_root = sb->s_root;
    mount->m_flags = flags;
    mount->m_refcount = 1;
    mount->m_readonly = (flags & VFS_MOUNT_FLAG_RDONLY) != 0;
    
    /* Copy path dan device */
    kernel_strncpy(mount->m_path, path, VFS_PATH_MAKS);
    if (device != NULL) {
        kernel_strncpy(mount->m_device, device, VFS_PATH_MAKS);
    }
    
    /* Link superblock */
    sb->s_mount = mount;
    
    /* Set root mount jika belum ada */
    if (g_root_mount == NULL) {
        g_root_mount = mount;
        g_cwd_dentry = mount->m_root;
        if (g_cwd_dentry != NULL) {
            dentry_get(g_cwd_dentry);
        }
    }
    
    /* Tambah ke list */
    vfs_lock();
    mount->m_next = g_mount_list;
    g_mount_list = mount;
    g_mount_count++;
    vfs_unlock();
    
    log_info("[VFS] Mounted: %s at %s", fs_nama, path);
    
    return STATUS_BERHASIL;
}

status_t mount_destroy(const char *path)
{
    mount_t *mount;
    mount_t *curr;
    mount_t *prev = NULL;
    
    if (path == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Cari mount */
    mount = mount_find(path);
    if (mount == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Tidak bisa unmount root jika masih ada mount lain */
    if (mount == g_root_mount && g_mount_count > 1) {
        return STATUS_BUSY;
    }
    
    /* Sync filesystem */
    if (mount->m_sb != NULL) {
        superblock_sync(mount->m_sb);
    }
    
    vfs_lock();
    
    /* Hapus dari list */
    curr = g_mount_list;
    while (curr != NULL) {
        if (curr == mount) {
            if (prev == NULL) {
                g_mount_list = mount->m_next;
            } else {
                prev->m_next = mount->m_next;
            }
            break;
        }
        prev = curr;
        curr = curr->m_next;
    }
    
    g_mount_count--;
    
    /* Clear root mount jika perlu */
    if (g_root_mount == mount) {
        g_root_mount = NULL;
    }
    
    vfs_unlock();
    
    /* Release references */
    if (mount->m_mountpoint != NULL) {
        dentry_put(mount->m_mountpoint);
    }
    if (mount->m_root != NULL) {
        dentry_put(mount->m_root);
    }
    if (mount->m_sb != NULL) {
        superblock_put(mount->m_sb);
    }
    
    /* Free mount structure */
    kernel_memset(mount, 0, sizeof(mount_t));
    kfree(mount);
    
    log_info("[VFS] Unmounted: %s", path);
    
    return STATUS_BERHASIL;
}

mount_t *mount_find(const char *path)
{
    mount_t *curr;
    char norm_path[VFS_PATH_MAKS + 1];
    
    if (path == NULL) {
        return NULL;
    }
    
    /* Normalize path */
    if (vfs_path_normalize(path, norm_path, VFS_PATH_MAKS) != STATUS_BERHASIL) {
        return NULL;
    }
    
    vfs_lock();
    
    curr = g_mount_list;
    while (curr != NULL) {
        if (kernel_strcmp(curr->m_path, norm_path) == 0) {
            vfs_unlock();
            return curr;
        }
        curr = curr->m_next;
    }
    
    vfs_unlock();
    
    return NULL;
}

mount_t *mount_find_by_dev(dev_t dev)
{
    mount_t *curr;
    
    vfs_lock();
    
    curr = g_mount_list;
    while (curr != NULL) {
        if (curr->m_sb != NULL && curr->m_sb->s_dev == dev) {
            vfs_unlock();
            return curr;
        }
        curr = curr->m_next;
    }
    
    vfs_unlock();
    
    return NULL;
}

mount_t *mount_root(void)
{
    return g_root_mount;
}

void mount_get(mount_t *mount)
{
    if (mount == NULL || mount->m_magic != VFS_MOUNT_MAGIC) {
        return;
    }
    
    mount->m_refcount++;
}

void mount_put(mount_t *mount)
{
    if (mount == NULL || mount->m_magic != VFS_MOUNT_MAGIC) {
        return;
    }
    
    if (mount->m_refcount > 0) {
        mount->m_refcount--;
    }
}

mount_t *mount_first(void)
{
    return g_mount_list;
}

mount_t *mount_next(mount_t *mount)
{
    if (mount == NULL) {
        return NULL;
    }
    return mount->m_next;
}

dentry_t *mount_resolve_path(const char *path)
{
    mount_t *curr;
    mount_t *best = NULL;
    ukuran_t best_len = 0;
    ukuran_t path_len;
    char norm_path[VFS_PATH_MAKS + 1];
    
    if (path == NULL) {
        return NULL;
    }
    
    /* Normalize path */
    if (vfs_path_normalize(path, norm_path, VFS_PATH_MAKS) != STATUS_BERHASIL) {
        return NULL;
    }
    
    path_len = kernel_strlen(norm_path);
    
    /* Cari mount dengan path terpanjang yang match */
    vfs_lock();
    
    curr = g_mount_list;
    while (curr != NULL) {
        ukuran_t mnt_len = kernel_strlen(curr->m_path);
        
        /* Check if path starts with mount path */
        if (kernel_strncmp(norm_path, curr->m_path, mnt_len) == 0) {
            if (mnt_len > best_len) {
                best = curr;
                best_len = mnt_len;
            }
        }
        
        curr = curr->m_next;
    }
    
    vfs_unlock();
    
    if (best == NULL) {
        if (g_root_mount != NULL) {
            return g_root_mount->m_root;
        }
        return NULL;
    }
    
    return best->m_root;
}

/*
 * ===========================================================================
 * FUNGSI NAMEI (NAMEI FUNCTIONS)
 * ===========================================================================
 */

dentry_t *namei_lookup(const char *path)
{
    char norm_path[VFS_PATH_MAKS + 1];
    char component[VFS_NAMA_MAKS + 1];
    dentry_t *dentry;
    dentry_t *parent;
    char *token;
    char *saveptr;
    inode_t *inode;
    
    if (path == NULL) {
        return NULL;
    }
    
    /* Normalize path */
    if (vfs_path_normalize(path, norm_path, VFS_PATH_MAKS) != STATUS_BERHASIL) {
        return NULL;
    }
    
    /* Mulai dari root atau cwd */
    if (norm_path[0] == '/') {
        if (g_root_mount == NULL || g_root_mount->m_root == NULL) {
            return NULL;
        }
        dentry = g_root_mount->m_root;
        dentry_get(dentry);
    } else {
        if (g_cwd_dentry == NULL) {
            return NULL;
        }
        dentry = g_cwd_dentry;
        dentry_get(dentry);
    }
    
    /* Parse dan traverse path */
    token = kernel_strtok_r(norm_path, "/", &saveptr);
    
    while (token != NULL) {
        /* Skip empty */
        if (kernel_strlen(token) == 0) {
            token = kernel_strtok_r(NULL, "/", &saveptr);
            continue;
        }
        
        kernel_strncpy(component, token, VFS_NAMA_MAKS);
        component[VFS_NAMA_MAKS] = '\0';
        
        /* Handle . dan .. */
        if (kernel_strcmp(component, ".") == 0) {
            token = kernel_strtok_r(NULL, "/", &saveptr);
            continue;
        }
        
        if (kernel_strcmp(component, "..") == 0) {
            if (dentry->d_parent != NULL) {
                parent = dentry->d_parent;
                dentry_get(parent);
                dentry_put(dentry);
                dentry = parent;
            }
            token = kernel_strtok_r(NULL, "/", &saveptr);
            continue;
        }
        
        /* Lookup component */
        parent = dentry;
        dentry = dentry_lookup(parent, component);
        
        if (dentry == NULL) {
            /* Coba lookup dari inode */
            inode = parent->d_inode;
            if (inode == NULL || inode->i_op == NULL ||
                inode->i_op->lookup == NULL) {
                dentry_put(parent);
                return NULL;
            }
            
            dentry = inode->i_op->lookup(inode, component);
            if (dentry == NULL) {
                dentry_put(parent);
                return NULL;
            }
            
            /* Init dentry */
            dentry->d_parent = parent;
            dentry->d_sb = inode->i_sb;
            
            /* Insert ke cache */
            dentry_cache_insert(dentry);
        }
        
        dentry_put(parent);
        
        /* Follow symlink jika perlu */
        if (dentry->d_inode != NULL &&
            VFS_S_ISLNK(dentry->d_inode->i_mode)) {
            /* TODO: Implementasi symlink following */
        }
        
        token = kernel_strtok_r(NULL, "/", &saveptr);
    }
    
    return dentry;
}

dentry_t *namei_lookup_parent(const char *path, char *nama,
                              ukuran_t nama_size)
{
    char dir_path[VFS_PATH_MAKS + 1];
    dentry_t *parent;
    const char *base;
    
    if (path == NULL || nama == NULL || nama_size == 0) {
        return NULL;
    }
    
    /* Split path */
    if (vfs_path_split(path, dir_path, nama, VFS_PATH_MAKS, nama_size)
        != STATUS_BERHASIL) {
        return NULL;
    }
    
    /* Lookup parent directory */
    parent = namei_lookup(dir_path);
    if (parent == NULL) {
        return NULL;
    }
    
    /* Verify parent is directory */
    if (parent->d_inode == NULL ||
        !VFS_S_ISDIR(parent->d_inode->i_mode)) {
        dentry_put(parent);
        return NULL;
    }
    
    return parent;
}

status_t namei_path_lookup(const char *path, dentry_t **dentry,
                           tak_bertanda32_t flags)
{
    dentry_t *result;
    
    if (path == NULL || dentry == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    result = namei_lookup(path);
    
    if (result == NULL) {
        if (flags & NAMEI_LOOKUP_CREATE) {
            /* Return parent untuk create */
            char nama[VFS_NAMA_MAKS + 1];
            *dentry = namei_lookup_parent(path, nama, VFS_NAMA_MAKS);
            if (*dentry == NULL) {
                return STATUS_TIDAK_DITEMUKAN;
            }
            return STATUS_TIDAK_DITEMUKAN;
        }
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    if (flags & NAMEI_LOOKUP_EXCL) {
        /* File sudah ada, tapi kita ingin create exclusive */
        dentry_put(result);
        return STATUS_SUDAH_ADA;
    }
    
    *dentry = result;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI FILE DESCRIPTOR (FILE DESCRIPTOR FUNCTIONS)
 * ===========================================================================
 */

int fd_alloc(file_t *file)
{
    tak_bertanda32_t i;
    
    if (file == NULL) {
        return -1;
    }
    
    vfs_lock();
    
    /* Cari slot kosong */
    for (i = 0; i < VFS_FD_MAKS; i++) {
        if (g_fd_table[i] == NULL) {
            g_fd_table[i] = file;
            file_get(file);
            g_fd_count++;
            vfs_unlock();
            return (int)i;
        }
    }
    
    vfs_unlock();
    
    return -1;
}

file_t *fd_get(int fd)
{
    file_t *file;
    
    if (fd < 0 || fd >= (int)VFS_FD_MAKS) {
        return NULL;
    }
    
    vfs_lock();
    
    file = g_fd_table[fd];
    if (file != NULL) {
        file_get(file);
    }
    
    vfs_unlock();
    
    return file;
}

status_t fd_put(int fd)
{
    file_t *file;
    
    if (fd < 0 || fd >= (int)VFS_FD_MAKS) {
        return STATUS_PARAM_INVALID;
    }
    
    vfs_lock();
    
    file = g_fd_table[fd];
    if (file == NULL) {
        vfs_unlock();
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Release file */
    file_release(file);
    file_put(file);
    
    g_fd_table[fd] = NULL;
    g_fd_count--;
    
    vfs_unlock();
    
    return STATUS_BERHASIL;
}

status_t fd_dup(int old_fd)
{
    file_t *file;
    int new_fd;
    
    file = fd_get(old_fd);
    if (file == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    new_fd = fd_alloc(file);
    file_put(file);
    
    if (new_fd < 0) {
        return STATUS_PENUH;
    }
    
    return (status_t)new_fd;
}

status_t fd_dup2(int old_fd, int new_fd)
{
    file_t *file;
    
    if (old_fd < 0 || old_fd >= (int)VFS_FD_MAKS ||
        new_fd < 0 || new_fd >= (int)VFS_FD_MAKS) {
        return STATUS_PARAM_INVALID;
    }
    
    if (old_fd == new_fd) {
        return (status_t)new_fd;
    }
    
    file = fd_get(old_fd);
    if (file == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Close new_fd jika terbuka */
    if (g_fd_table[new_fd] != NULL) {
        fd_put(new_fd);
    }
    
    /* Set new_fd */
    vfs_lock();
    g_fd_table[new_fd] = file;
    g_fd_count++;
    vfs_unlock();
    
    return (status_t)new_fd;
}

int fd_first(void)
{
    tak_bertanda32_t i;
    
    for (i = 0; i < VFS_FD_MAKS; i++) {
        if (g_fd_table[i] != NULL) {
            return (int)i;
        }
    }
    
    return -1;
}

int fd_next(int fd)
{
    tak_bertanda32_t i;
    
    if (fd < 0) {
        return -1;
    }
    
    for (i = (tak_bertanda32_t)(fd + 1); i < VFS_FD_MAKS; i++) {
        if (g_fd_table[i] != NULL) {
            return (int)i;
        }
    }
    
    return -1;
}

/*
 * ===========================================================================
 * FUNGSI VFS API (VFS API FUNCTIONS)
 * ===========================================================================
 */

int vfs_open(const char *path, tak_bertanda32_t flags, mode_t mode)
{
    dentry_t *dentry;
    inode_t *inode;
    file_t *file;
    int fd;
    status_t status;
    char nama[VFS_NAMA_MAKS + 1];
    dentry_t *parent;
    
    if (path == NULL) {
        return (int)STATUS_PARAM_NULL;
    }
    
    /* Lookup file */
    dentry = namei_lookup(path);
    
    if (dentry == NULL) {
        /* File tidak ada, cek apakah boleh create */
        if (!(flags & VFS_OPEN_CREAT)) {
            return (int)STATUS_TIDAK_DITEMUKAN;
        }
        
        /* Get parent directory */
        parent = namei_lookup_parent(path, nama, VFS_NAMA_MAKS);
        if (parent == NULL) {
            return (int)STATUS_TIDAK_DITEMUKAN;
        }
        
        /* Create file */
        if (parent->d_inode == NULL || parent->d_inode->i_op == NULL ||
            parent->d_inode->i_op->create == NULL) {
            dentry_put(parent);
            return (int)STATUS_TIDAK_DUKUNG;
        }
        
        /* Alokasi inode baru */
        dentry = dentry_alloc(nama);
        if (dentry == NULL) {
            dentry_put(parent);
            return (int)STATUS_MEMORI_HABIS;
        }
        
        dentry->d_parent = parent;
        dentry->d_sb = parent->d_sb;
        
        /* Create inode */
        status = parent->d_inode->i_op->create(parent->d_inode, dentry,
                                               mode);
        if (status != STATUS_BERHASIL) {
            dentry_free(dentry);
            dentry_put(parent);
            return (int)status;
        }
        
        /* Insert ke cache */
        dentry_cache_insert(dentry);
        dentry_get(dentry);
        dentry_put(parent);
    }
    
    /* Check exclusive */
    if ((flags & VFS_OPEN_EXCL) && (flags & VFS_OPEN_CREAT)) {
        dentry_put(dentry);
        return (int)STATUS_SUDAH_ADA;
    }
    
    /* Get inode */
    inode = dentry->d_inode;
    if (inode == NULL) {
        dentry_put(dentry);
        return (int)STATUS_PARAM_INVALID;
    }
    
    /* Check permissions */
    /* TODO: Implement proper permission check */
    
    /* Truncate jika perlu */
    if ((flags & VFS_OPEN_TRUNC) && VFS_S_ISREG(inode->i_mode)) {
        inode->i_size = 0;
        inode_mark_dirty(inode);
    }
    
    /* Allocate file structure */
    file = file_alloc();
    if (file == NULL) {
        dentry_put(dentry);
        return (int)STATUS_MEMORI_HABIS;
    }
    
    /* Initialize file */
    status = file_init(file, inode, dentry, flags);
    if (status != STATUS_BERHASIL) {
        file_free(file);
        dentry_put(dentry);
        return (int)status;
    }
    
    /* Call open */
    status = file_open(inode, file);
    if (status != STATUS_BERHASIL) {
        file_free(file);
        dentry_put(dentry);
        return (int)status;
    }
    
    /* Allocate file descriptor */
    fd = fd_alloc(file);
    if (fd < 0) {
        file_free(file);
        dentry_put(dentry);
        return (int)STATUS_PENUH;
    }
    
    file_put(file);
    dentry_put(dentry);
    
    return fd;
}

status_t vfs_close(int fd)
{
    return fd_put(fd);
}

tak_bertandas_t vfs_read(int fd, void *buffer, ukuran_t size)
{
    file_t *file;
    tak_bertandas_t ret;
    
    file = fd_get(fd);
    if (file == NULL) {
        return (tak_bertandas_t)STATUS_TIDAK_DITEMUKAN;
    }
    
    ret = file_read(file, buffer, size);
    
    file_put(file);
    
    return ret;
}

tak_bertandas_t vfs_write(int fd, const void *buffer, ukuran_t size)
{
    file_t *file;
    tak_bertandas_t ret;
    
    file = fd_get(fd);
    if (file == NULL) {
        return (tak_bertandas_t)STATUS_TIDAK_DITEMUKAN;
    }
    
    ret = file_write(file, buffer, size);
    
    file_put(file);
    
    return ret;
}

off_t vfs_lseek(int fd, off_t offset, tak_bertanda32_t whence)
{
    file_t *file;
    off_t ret;
    
    file = fd_get(fd);
    if (file == NULL) {
        return (off_t)STATUS_TIDAK_DITEMUKAN;
    }
    
    ret = file_lseek(file, offset, whence);
    
    file_put(file);
    
    return ret;
}

status_t vfs_fstat(int fd, vfs_stat_t *stat)
{
    file_t *file;
    status_t ret;
    
    if (stat == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    file = fd_get(fd);
    if (file == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    ret = inode_getattr(file->f_inode, stat);
    
    file_put(file);
    
    return ret;
}

status_t vfs_fsync(int fd)
{
    file_t *file;
    status_t ret;
    
    file = fd_get(fd);
    if (file == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    ret = file_fsync(file);
    
    file_put(file);
    
    return ret;
}

int vfs_opendir(const char *path)
{
    dentry_t *dentry;
    file_t *file;
    int fd;
    status_t status;
    
    if (path == NULL) {
        return (int)STATUS_PARAM_NULL;
    }
    
    dentry = namei_lookup(path);
    if (dentry == NULL) {
        return (int)STATUS_TIDAK_DITEMUKAN;
    }
    
    if (dentry->d_inode == NULL ||
        !VFS_S_ISDIR(dentry->d_inode->i_mode)) {
        dentry_put(dentry);
        return (int)STATUS_PARAM_INVALID;
    }
    
    file = file_alloc();
    if (file == NULL) {
        dentry_put(dentry);
        return (int)STATUS_MEMORI_HABIS;
    }
    
    status = file_init(file, dentry->d_inode, dentry,
                       VFS_OPEN_RDONLY | VFS_OPEN_DIRECTORY);
    if (status != STATUS_BERHASIL) {
        file_free(file);
        dentry_put(dentry);
        return (int)status;
    }
    
    fd = fd_alloc(file);
    if (fd < 0) {
        file_free(file);
        dentry_put(dentry);
        return (int)STATUS_PENUH;
    }
    
    file_put(file);
    dentry_put(dentry);
    
    return fd;
}

tak_bertandas_t vfs_readdir(int fd, vfs_dirent_t *dirent,
                            ukuran_t count)
{
    file_t *file;
    tak_bertandas_t ret;
    
    if (dirent == NULL || count == 0) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }
    
    file = fd_get(fd);
    if (file == NULL) {
        return (tak_bertandas_t)STATUS_TIDAK_DITEMUKAN;
    }
    
    ret = file_readdir(file, dirent, count);
    
    file_put(file);
    
    return ret;
}

status_t vfs_closedir(int fd)
{
    return fd_put(fd);
}

status_t vfs_mkdir(const char *path, mode_t mode)
{
    dentry_t *parent;
    dentry_t *dentry;
    char nama[VFS_NAMA_MAKS + 1];
    status_t status;
    
    if (path == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    parent = namei_lookup_parent(path, nama, VFS_NAMA_MAKS);
    if (parent == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    if (parent->d_inode == NULL || parent->d_inode->i_op == NULL ||
        parent->d_inode->i_op->mkdir == NULL) {
        dentry_put(parent);
        return STATUS_TIDAK_DUKUNG;
    }
    
    /* Cek apakah sudah ada */
    dentry = dentry_lookup(parent, nama);
    if (dentry != NULL) {
        dentry_put(dentry);
        dentry_put(parent);
        return STATUS_SUDAH_ADA;
    }
    
    /* Buat dentry baru */
    dentry = dentry_alloc(nama);
    if (dentry == NULL) {
        dentry_put(parent);
        return STATUS_MEMORI_HABIS;
    }
    
    dentry->d_parent = parent;
    dentry->d_sb = parent->d_sb;
    
    /* Create directory */
    status = parent->d_inode->i_op->mkdir(parent->d_inode, dentry,
                                          mode | VFS_S_IFDIR);
    if (status != STATUS_BERHASIL) {
        dentry_free(dentry);
        dentry_put(parent);
        return status;
    }
    
    dentry_cache_insert(dentry);
    
    dentry_put(dentry);
    dentry_put(parent);
    
    return STATUS_BERHASIL;
}

status_t vfs_rmdir(const char *path)
{
    dentry_t *dentry;
    dentry_t *parent;
    status_t status;
    
    if (path == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    dentry = namei_lookup(path);
    if (dentry == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    if (dentry->d_inode == NULL ||
        !VFS_S_ISDIR(dentry->d_inode->i_mode)) {
        dentry_put(dentry);
        return STATUS_PARAM_INVALID;
    }
    
    parent = dentry->d_parent;
    if (parent == NULL || parent->d_inode == NULL ||
        parent->d_inode->i_op == NULL ||
        parent->d_inode->i_op->rmdir == NULL) {
        dentry_put(dentry);
        return STATUS_TIDAK_DUKUNG;
    }
    
    /* Remove directory */
    status = parent->d_inode->i_op->rmdir(parent->d_inode, dentry);
    
    if (status == STATUS_BERHASIL) {
        dentry_cache_remove(dentry);
    }
    
    dentry_put(dentry);
    
    return status;
}

status_t vfs_unlink(const char *path)
{
    dentry_t *dentry;
    dentry_t *parent;
    status_t status;
    
    if (path == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    dentry = namei_lookup(path);
    if (dentry == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    if (dentry->d_inode == NULL ||
        VFS_S_ISDIR(dentry->d_inode->i_mode)) {
        dentry_put(dentry);
        return STATUS_PARAM_INVALID;
    }
    
    parent = dentry->d_parent;
    if (parent == NULL || parent->d_inode == NULL ||
        parent->d_inode->i_op == NULL ||
        parent->d_inode->i_op->unlink == NULL) {
        dentry_put(dentry);
        return STATUS_TIDAK_DUKUNG;
    }
    
    /* Unlink file */
    status = parent->d_inode->i_op->unlink(parent->d_inode, dentry);
    
    if (status == STATUS_BERHASIL) {
        dentry_cache_remove(dentry);
    }
    
    dentry_put(dentry);
    
    return status;
}

status_t vfs_rename(const char *old_path, const char *new_path)
{
    dentry_t *old_dentry;
    dentry_t *new_dentry;
    dentry_t *old_parent;
    dentry_t *new_parent;
    char new_nama[VFS_NAMA_MAKS + 1];
    status_t status;
    
    if (old_path == NULL || new_path == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    old_dentry = namei_lookup(old_path);
    if (old_dentry == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    old_parent = old_dentry->d_parent;
    
    new_parent = namei_lookup_parent(new_path, new_nama, VFS_NAMA_MAKS);
    if (new_parent == NULL) {
        dentry_put(old_dentry);
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Cek apakah target sudah ada */
    new_dentry = dentry_lookup(new_parent, new_nama);
    if (new_dentry != NULL) {
        dentry_put(new_dentry);
        dentry_put(new_parent);
        dentry_put(old_dentry);
        return STATUS_SUDAH_ADA;
    }
    
    /* Rename */
    if (old_parent->d_inode == NULL ||
        old_parent->d_inode->i_op == NULL ||
        old_parent->d_inode->i_op->rename == NULL) {
        dentry_put(new_parent);
        dentry_put(old_dentry);
        return STATUS_TIDAK_DUKUNG;
    }
    
    new_dentry = dentry_alloc(new_nama);
    if (new_dentry == NULL) {
        dentry_put(new_parent);
        dentry_put(old_dentry);
        return STATUS_MEMORI_HABIS;
    }
    
    new_dentry->d_parent = new_parent;
    new_dentry->d_sb = new_parent->d_sb;
    
    status = old_parent->d_inode->i_op->rename(old_parent->d_inode,
                                               old_dentry,
                                               new_parent->d_inode,
                                               new_dentry);
    
    if (status == STATUS_BERHASIL) {
        dentry_cache_remove(old_dentry);
        dentry_cache_insert(new_dentry);
    }
    
    dentry_put(new_dentry);
    dentry_put(new_parent);
    dentry_put(old_dentry);
    
    return status;
}

status_t vfs_stat(const char *path, vfs_stat_t *stat)
{
    dentry_t *dentry;
    status_t status;
    
    if (path == NULL || stat == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    dentry = namei_lookup(path);
    if (dentry == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    status = inode_getattr(dentry->d_inode, stat);
    
    dentry_put(dentry);
    
    return status;
}

status_t vfs_lstat(const char *path, vfs_stat_t *stat)
{
    /* TODO: Implement lstat yang tidak follow symlink */
    return vfs_stat(path, stat);
}

status_t vfs_chmod(const char *path, mode_t mode)
{
    dentry_t *dentry;
    vfs_stat_t stat;
    
    if (path == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    dentry = namei_lookup(path);
    if (dentry == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    kernel_memset(&stat, 0, sizeof(vfs_stat_t));
    stat.st_mode = mode;
    
    status_t status = inode_setattr(dentry->d_inode, &stat);
    
    dentry_put(dentry);
    
    return status;
}

status_t vfs_chown(const char *path, uid_t uid, gid_t gid)
{
    dentry_t *dentry;
    vfs_stat_t stat;
    
    if (path == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    dentry = namei_lookup(path);
    if (dentry == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    kernel_memset(&stat, 0, sizeof(vfs_stat_t));
    stat.st_uid = uid;
    stat.st_gid = gid;
    
    status_t status = inode_setattr(dentry->d_inode, &stat);
    
    dentry_put(dentry);
    
    return status;
}

status_t vfs_truncate(const char *path, off_t length)
{
    dentry_t *dentry;
    vfs_stat_t stat;
    
    if (path == NULL || length < 0) {
        return STATUS_PARAM_NULL;
    }
    
    dentry = namei_lookup(path);
    if (dentry == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    if (dentry->d_inode == NULL ||
        !VFS_S_ISREG(dentry->d_inode->i_mode)) {
        dentry_put(dentry);
        return STATUS_PARAM_INVALID;
    }
    
    kernel_memset(&stat, 0, sizeof(vfs_stat_t));
    stat.st_size = length;
    
    status_t status = inode_setattr(dentry->d_inode, &stat);
    
    dentry_put(dentry);
    
    return status;
}

status_t vfs_symlink(const char *target, const char *linkpath)
{
    dentry_t *parent;
    dentry_t *dentry;
    char nama[VFS_NAMA_MAKS + 1];
    status_t status;
    
    if (target == NULL || linkpath == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    parent = namei_lookup_parent(linkpath, nama, VFS_NAMA_MAKS);
    if (parent == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    if (parent->d_inode == NULL || parent->d_inode->i_op == NULL ||
        parent->d_inode->i_op->symlink == NULL) {
        dentry_put(parent);
        return STATUS_TIDAK_DUKUNG;
    }
    
    dentry = dentry_alloc(nama);
    if (dentry == NULL) {
        dentry_put(parent);
        return STATUS_MEMORI_HABIS;
    }
    
    dentry->d_parent = parent;
    dentry->d_sb = parent->d_sb;
    
    status = parent->d_inode->i_op->symlink(parent->d_inode, dentry,
                                            target);
    
    if (status == STATUS_BERHASIL) {
        dentry_cache_insert(dentry);
    }
    
    dentry_put(dentry);
    dentry_put(parent);
    
    return status;
}

tak_bertandas_t vfs_readlink(const char *path, char *buffer,
                             ukuran_t size)
{
    dentry_t *dentry;
    tak_bertandas_t ret;
    
    if (path == NULL || buffer == NULL || size == 0) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }
    
    dentry = namei_lookup(path);
    if (dentry == NULL) {
        return (tak_bertandas_t)STATUS_TIDAK_DITEMUKAN;
    }
    
    if (dentry->d_inode == NULL ||
        !VFS_S_ISLNK(dentry->d_inode->i_mode)) {
        dentry_put(dentry);
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }
    
    if (dentry->d_inode->i_op == NULL ||
        dentry->d_inode->i_op->readlink == NULL) {
        dentry_put(dentry);
        return (tak_bertandas_t)STATUS_TIDAK_DUKUNG;
    }
    
    if (dentry->d_inode->i_op->readlink(dentry, buffer, size) == NULL) {
        dentry_put(dentry);
        return (tak_bertandas_t)STATUS_GAGAL;
    }
    
    ret = (tak_bertandas_t)kernel_strlen(buffer);
    
    dentry_put(dentry);
    
    return ret;
}

status_t vfs_chdir(const char *path)
{
    dentry_t *dentry;
    
    if (path == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    dentry = namei_lookup(path);
    if (dentry == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    if (dentry->d_inode == NULL ||
        !VFS_S_ISDIR(dentry->d_inode->i_mode)) {
        dentry_put(dentry);
        return STATUS_PARAM_INVALID;
    }
    
    /* Update cwd */
    if (g_cwd_dentry != NULL) {
        dentry_put(g_cwd_dentry);
    }
    
    g_cwd_dentry = dentry;
    dentry_get(g_cwd_dentry);
    
    return STATUS_BERHASIL;
}

status_t vfs_getcwd(char *buffer, ukuran_t size)
{
    dentry_t *dentry;
    dentry_t *path[VFS_PATH_MAKS / 2];
    tak_bertanda32_t depth = 0;
    ukuran_t pos;
    ukuran_t len;
    tak_bertanda32_t i;
    
    if (buffer == NULL || size == 0) {
        return STATUS_PARAM_NULL;
    }
    
    if (g_cwd_dentry == NULL) {
        buffer[0] = '/';
        buffer[1] = '\0';
        return STATUS_BERHASIL;
    }
    
    /* Build path dari cwd ke root */
    dentry = g_cwd_dentry;
    while (dentry != NULL && dentry->d_parent != dentry) {
        if (depth >= VFS_PATH_MAKS / 2) {
            return STATUS_PARAM_UKURAN;
        }
        path[depth++] = dentry;
        dentry = dentry->d_parent;
    }
    
    /* Reconstruct path */
    if (depth == 0) {
        buffer[0] = '/';
        buffer[1] = '\0';
        return STATUS_BERHASIL;
    }
    
    pos = 0;
    buffer[pos++] = '/';
    
    for (i = depth; i > 0; i--) {
        dentry = path[i - 1];
        len = dentry->d_namelen;
        
        if (pos + len + 2 >= size) {
            return STATUS_PARAM_UKURAN;
        }
        
        if (i < depth) {
            buffer[pos++] = '/';
        }
        
        kernel_strncpy(buffer + pos, dentry->d_name, len);
        pos += len;
    }
    
    buffer[pos] = '\0';
    
    return STATUS_BERHASIL;
}

status_t vfs_mount(const char *device, const char *path,
                   const char *fs_nama, tak_bertanda32_t flags)
{
    return mount_create(device, path, fs_nama, flags);
}

status_t vfs_umount(const char *path)
{
    return mount_destroy(path);
}
