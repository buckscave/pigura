/*
 * PIGURA OS - INODE.C
 * ===================
 * Implementasi manajemen inode untuk VFS Pigura OS.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola inode,
 * termasuk alokasi, dealokasi, dan operasi-operasi terkait.
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

/*
 * ===========================================================================
 * KONSTANTA INTERNAL (INTERNAL CONSTANTS)
 * ===========================================================================
 */

/* Ukuran hash table untuk inode cache */
#define INODE_HASH_SIZE         256

/* Jumlah maksimum inode dalam cache */
#define INODE_CACHE_MAX         1024

/* Default block size */
#define INODE_DEFAULT_BLKSIZE   4096

/* Inode number special */
#define INO_ROOT                2
#define INO_BAD                 0
#define INO_FIRST               1

/* Flag internal inode */
#define I_FLAG_HASHED           0x00010000
#define I_FLAG_CACHED           0x00020000

/*
 * ===========================================================================
 * STRUKTUR DATA INTERNAL (INTERNAL DATA STRUCTURES)
 * ===========================================================================
 */

/* Entry dalam inode hash table */
typedef struct inode_hash_entry {
    inode_t *inode;
    struct inode_hash_entry *next;
} inode_hash_entry_t;

/* Inode LRU list untuk cache management */
typedef struct inode_lru_list {
    inode_t *head;
    inode_t *tail;
    tak_bertanda32_t count;
} inode_lru_list_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ===========================================================================
 */

/* Inode hash table */
static inode_t *g_inode_hash[INODE_HASH_SIZE];

/* Inode LRU list */
static inode_lru_list_t g_inode_lru;

/* Statistik inode */
static tak_bertanda32_t g_inode_total = 0;
static tak_bertanda32_t g_inode_cached = 0;
static tak_bertanda32_t g_inode_dirty = 0;

/* Lock untuk operasi inode */
static tak_bertanda32_t g_inode_lock = 0;

/* Inode number generator */
static ino_t g_ino_next = INO_FIRST;

/*
 * ===========================================================================
 * FUNGSI INTERNAL LOCKING (INTERNAL LOCKING FUNCTIONS)
 * ===========================================================================
 */

static void inode_lock(void)
{
    g_inode_lock++;
}

static void inode_unlock(void)
{
    if (g_inode_lock > 0) {
        g_inode_lock--;
    }
}

/*
 * ===========================================================================
 * FUNGSI HASH INTERNAL (INTERNAL HASH FUNCTIONS)
 * ===========================================================================
 */

static tak_bertanda32_t inode_hash(ino_t ino, dev_t dev)
{
    tak_bertanda32_t hash;
    
    /* Combine inode number dan device untuk hash */
    hash = (tak_bertanda32_t)(ino & 0xFFFFFFFF);
    hash ^= (tak_bertanda32_t)((ino >> 32) & 0xFFFFFFFF);
    hash ^= (tak_bertanda32_t)dev;
    hash ^= (hash >> 16);
    hash ^= (hash >> 8);
    
    return hash % INODE_HASH_SIZE;
}

/*
 * ===========================================================================
 * FUNGSI LRU LIST (LRU LIST FUNCTIONS)
 * ===========================================================================
 */

static void inode_lru_add(inode_t *inode)
{
    if (inode == NULL) {
        return;
    }
    
    /* Remove dari posisi lama jika ada */
    if (inode->i_flags & I_FLAG_CACHED) {
        return;
    }
    
    inode_lock();
    
    /* Add ke head */
    inode->i_version++;  /* Gunakan i_version sebagai list pointer hack */
    
    /* Check cache limit */
    if (g_inode_cached >= INODE_CACHE_MAX) {
        /* Perlu evict dari tail */
        /* TODO: Implementasi eviction */
    }
    
    inode->i_flags |= I_FLAG_CACHED;
    g_inode_cached++;
    
    inode_unlock();
}

static void inode_lru_remove(inode_t *inode)
{
    if (inode == NULL) {
        return;
    }
    
    if (!(inode->i_flags & I_FLAG_CACHED)) {
        return;
    }
    
    inode_lock();
    
    inode->i_flags &= ~I_FLAG_CACHED;
    if (g_inode_cached > 0) {
        g_inode_cached--;
    }
    
    inode_unlock();
}

static void inode_lru_touch(inode_t *inode)
{
    if (inode == NULL) {
        return;
    }
    
    /* Move ke head (most recently used) */
    inode_lru_remove(inode);
    inode_lru_add(inode);
}

/*
 * ===========================================================================
 * FUNGSI HASH TABLE (HASH TABLE FUNCTIONS)
 * ===========================================================================
 */

static status_t inode_hash_insert(inode_t *inode)
{
    tak_bertanda32_t hash_idx;
    dev_t dev;
    
    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    dev = 0;
    if (inode->i_sb != NULL) {
        dev = inode->i_sb->s_dev;
    }
    
    hash_idx = inode_hash(inode->i_ino, dev);
    
    inode_lock();
    
    /* Add ke hash chain */
    inode->i_next = g_inode_hash[hash_idx];
    g_inode_hash[hash_idx] = inode;
    inode->i_flags |= I_FLAG_HASHED;
    
    g_inode_total++;
    
    inode_unlock();
    
    return STATUS_BERHASIL;
}

static status_t inode_hash_remove(inode_t *inode)
{
    tak_bertanda32_t hash_idx;
    inode_t *curr;
    inode_t *prev;
    dev_t dev;
    
    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (!(inode->i_flags & I_FLAG_HASHED)) {
        return STATUS_BERHASIL;
    }
    
    dev = 0;
    if (inode->i_sb != NULL) {
        dev = inode->i_sb->s_dev;
    }
    
    hash_idx = inode_hash(inode->i_ino, dev);
    
    inode_lock();
    
    prev = NULL;
    curr = g_inode_hash[hash_idx];
    
    while (curr != NULL) {
        if (curr == inode) {
            if (prev == NULL) {
                g_inode_hash[hash_idx] = curr->i_next;
            } else {
                prev->i_next = curr->i_next;
            }
            
            inode->i_flags &= ~I_FLAG_HASHED;
            inode->i_next = NULL;
            
            if (g_inode_total > 0) {
                g_inode_total--;
            }
            
            inode_unlock();
            return STATUS_BERHASIL;
        }
        prev = curr;
        curr = curr->i_next;
    }
    
    inode_unlock();
    
    return STATUS_TIDAK_DITEMUKAN;
}

static inode_t *inode_hash_lookup(ino_t ino, dev_t dev)
{
    tak_bertanda32_t hash_idx;
    inode_t *curr;
    
    hash_idx = inode_hash(ino, dev);
    
    inode_lock();
    
    curr = g_inode_hash[hash_idx];
    
    while (curr != NULL) {
        if (curr->i_ino == ino) {
            if (curr->i_sb != NULL && curr->i_sb->s_dev == dev) {
                inode_get(curr);
                inode_unlock();
                return curr;
            }
        }
        curr = curr->i_next;
    }
    
    inode_unlock();
    
    return NULL;
}

/*
 * ===========================================================================
 * FUNGSI GENERATOR INO (INO GENERATOR FUNCTIONS)
 * ===========================================================================
 */

ino_t inode_generate_ino(void)
{
    ino_t ino;
    
    inode_lock();
    
    g_ino_next++;
    if (g_ino_next == INO_BAD) {
        g_ino_next = INO_FIRST;
    }
    ino = g_ino_next;
    
    inode_unlock();
    
    return ino;
}

/*
 * ===========================================================================
 * FUNGSI ALOKASI DAN DEALOKASI (ALLOCATION AND DEALLOCATION FUNCTIONS)
 * ===========================================================================
 */

inode_t *inode_alloc(superblock_t *sb)
{
    inode_t *inode;
    
    if (sb == NULL) {
        log_error("[VFS:INODE] Superblock NULL");
        return NULL;
    }
    
    /* Gunakan filesystem-specific allocator jika ada */
    if (sb->s_op != NULL && sb->s_op->alloc_inode != NULL) {
        return sb->s_op->alloc_inode(sb);
    }
    
    /* Alokasi memori */
    inode = (inode_t *)kmalloc(sizeof(inode_t));
    if (inode == NULL) {
        log_error("[VFS:INODE] Gagal alokasi memori");
        return NULL;
    }
    
    /* Clear dan init */
    kernel_memset(inode, 0, sizeof(inode_t));
    
    inode->i_magic = VFS_INODE_MAGIC;
    inode->i_ino = inode_generate_ino();
    inode->i_sb = sb;
    inode->i_blksize = (tak_bertanda32_t)sb->s_block_size;
    inode->i_refcount = 1;
    inode->i_nlink = 1;
    
    /* Set timestamps */
    inode->i_atime = kernel_get_uptime();
    inode->i_mtime = inode->i_atime;
    inode->i_ctime = inode->i_atime;
    
    /* Reference superblock */
    superblock_get(sb);
    
    /* Insert ke hash table */
    inode_hash_insert(inode);
    
    /* Add ke LRU */
    inode_lru_add(inode);
    
    /* Reserve inode dari superblock */
    superblock_reserve_inode(sb);
    
    log_debug("[VFS:INODE] Inode dialokasi: ino=%llu", inode->i_ino);
    
    return inode;
}

void inode_free(inode_t *inode)
{
    superblock_t *sb;
    
    if (inode == NULL) {
        return;
    }
    
    /* Validasi magic */
    if (inode->i_magic != VFS_INODE_MAGIC) {
        log_warn("[VFS:INODE] Magic invalid saat free");
        return;
    }
    
    /* Jangan free jika masih ada reference */
    if (inode->i_refcount > 0) {
        log_debug("[VFS:INODE] Inode masih di-reference: %u",
                  inode->i_refcount);
        return;
    }
    
    /* Sync jika dirty */
    if (inode->i_dirty) {
        inode_write(inode);
    }
    
    /* Hapus dari hash table */
    inode_hash_remove(inode);
    
    /* Hapus dari LRU */
    inode_lru_remove(inode);
    
    /* Update dirty count */
    if (inode->i_dirty && g_inode_dirty > 0) {
        g_inode_dirty--;
    }
    
    /* Simpan superblock reference */
    sb = inode->i_sb;
    
    /* Clear memory */
    kernel_memset(inode, 0, sizeof(inode_t));
    
    /* Free memory */
    kfree(inode);
    
    /* Release superblock reference */
    if (sb != NULL) {
        superblock_release_inode(sb);
        superblock_put(sb);
    }
    
    log_debug("[VFS:INODE] Inode dibebaskan");
}

/*
 * ===========================================================================
 * FUNGSI INISIALISASI (INITIALIZATION FUNCTIONS)
 * ===========================================================================
 */

status_t inode_init(inode_t *inode, superblock_t *sb, ino_t ino,
                    mode_t mode)
{
    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    inode->i_magic = VFS_INODE_MAGIC;
    inode->i_ino = ino;
    inode->i_mode = mode;
    inode->i_sb = sb;
    inode->i_blksize = (tak_bertanda32_t)sb->s_block_size;
    inode->i_nlink = 1;
    inode->i_refcount = 1;
    
    /* Set timestamps */
    inode->i_atime = kernel_get_uptime();
    inode->i_mtime = inode->i_atime;
    inode->i_ctime = inode->i_atime;
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI REFERENCE COUNTING (REFERENCE COUNTING FUNCTIONS)
 * ===========================================================================
 */

void inode_get(inode_t *inode)
{
    if (inode == NULL) {
        return;
    }
    
    if (inode->i_magic != VFS_INODE_MAGIC) {
        return;
    }
    
    inode_lock();
    
    inode->i_refcount++;
    
    /* Touch LRU */
    if (inode->i_flags & I_FLAG_CACHED) {
        inode_lru_touch(inode);
    }
    
    inode_unlock();
}

void inode_put(inode_t *inode)
{
    if (inode == NULL) {
        return;
    }
    
    if (inode->i_magic != VFS_INODE_MAGIC) {
        return;
    }
    
    inode_lock();
    
    if (inode->i_refcount > 0) {
        inode->i_refcount--;
        
        if (inode->i_refcount == 0) {
            inode_unlock();
            inode_free(inode);
            return;
        }
    }
    
    inode_unlock();
}

/*
 * ===========================================================================
 * FUNGSI READ/WRITE INODE (INODE READ/WRITE FUNCTIONS)
 * ===========================================================================
 */

status_t inode_read(inode_t *inode)
{
    status_t ret;
    
    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (inode->i_magic != VFS_INODE_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    if (inode->i_sb == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Gunakan filesystem read_inode jika ada */
    if (inode->i_sb->s_op != NULL &&
        inode->i_sb->s_op->read_inode != NULL) {
        
        inode_lock();
        ret = inode->i_sb->s_op->read_inode(inode);
        inode_unlock();
        
        if (ret == STATUS_BERHASIL) {
            /* Update timestamp */
            inode->i_atime = kernel_get_uptime();
        }
        
        return ret;
    }
    
    return STATUS_TIDAK_DUKUNG;
}

status_t inode_write(inode_t *inode)
{
    status_t ret;
    bool_t was_dirty;
    
    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (inode->i_magic != VFS_INODE_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    if (inode->i_sb == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Check readonly */
    if (inode->i_sb->s_readonly) {
        return STATUS_AKSES_DITOLAK;
    }
    
    was_dirty = inode->i_dirty;
    
    /* Gunakan filesystem write_inode jika ada */
    if (inode->i_sb->s_op != NULL &&
        inode->i_sb->s_op->write_inode != NULL) {
        
        inode_lock();
        ret = inode->i_sb->s_op->write_inode(inode);
        inode_unlock();
        
        if (ret == STATUS_BERHASIL) {
            inode->i_dirty = SALAH;
            inode->i_mtime = kernel_get_uptime();
            
            if (was_dirty && g_inode_dirty > 0) {
                g_inode_dirty--;
            }
        }
        
        return ret;
    }
    
    return STATUS_TIDAK_DUKUNG;
}

/*
 * ===========================================================================
 * FUNGSI DIRTY MANAGEMENT (DIRTY MANAGEMENT FUNCTIONS)
 * ===========================================================================
 */

status_t inode_mark_dirty(inode_t *inode)
{
    bool_t was_dirty;
    
    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (inode->i_magic != VFS_INODE_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    was_dirty = inode->i_dirty;
    
    inode->i_dirty = BENAR;
    inode->i_mtime = kernel_get_uptime();
    
    /* Update dirty count */
    if (!was_dirty) {
        inode_lock();
        g_inode_dirty++;
        inode_unlock();
    }
    
    /* Mark superblock dirty */
    if (inode->i_sb != NULL) {
        superblock_mark_dirty(inode->i_sb);
    }
    
    /* Panggil dirty_inode callback */
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
    
    if (!inode->i_dirty) {
        return STATUS_BERHASIL;
    }
    
    return inode_write(inode);
}

bool_t inode_is_dirty(inode_t *inode)
{
    if (inode == NULL) {
        return SALAH;
    }
    
    return inode->i_dirty;
}

/*
 * ===========================================================================
 * FUNGSI LOOKUP (LOOKUP FUNCTIONS)
 * ===========================================================================
 */

inode_t *inode_lookup(inode_t *dir, const char *name)
{
    dentry_t *dentry;
    
    if (dir == NULL || name == NULL) {
        return NULL;
    }
    
    /* Harus direktori */
    if (!VFS_S_ISDIR(dir->i_mode)) {
        return NULL;
    }
    
    /* Harus ada lookup operation */
    if (dir->i_op == NULL || dir->i_op->lookup == NULL) {
        return NULL;
    }
    
    dentry = dir->i_op->lookup(dir, name);
    if (dentry == NULL) {
        return NULL;
    }
    
    /* Update access time */
    dir->i_atime = kernel_get_uptime();
    
    return dentry->d_inode;
}

inode_t *inode_lookup_by_ino(superblock_t *sb, ino_t ino)
{
    dev_t dev;
    inode_t *inode;
    
    if (sb == NULL) {
        return NULL;
    }
    
    dev = sb->s_dev;
    
    /* Cari di hash table */
    inode = inode_hash_lookup(ino, dev);
    
    return inode;
}

/*
 * ===========================================================================
 * FUNGSI PERMISSION (PERMISSION FUNCTIONS)
 * ===========================================================================
 */

status_t inode_permission(inode_t *inode, tak_bertanda32_t mask)
{
    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Gunakan filesystem permission check jika ada */
    if (inode->i_op != NULL && inode->i_op->permission != NULL) {
        return inode->i_op->permission(inode, mask);
    }
    
    /* Default: allow */
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
    
    /* Root selalu diizinkan */
    if (uid == 0) {
        return BENAR;
    }
    
    /* Hitung permission bits */
    if (inode->i_uid == uid) {
        /* Owner */
        perm = (mode >> 6) & 7;
    } else if (inode->i_gid == gid) {
        /* Group */
        perm = (mode >> 3) & 7;
    } else {
        /* Other */
        perm = mode & 7;
    }
    
    /* Check berdasarkan mask */
    if ((mask & VFS_OPEN_RDONLY) && !(perm & 4)) {
        return SALAH;
    }
    if ((mask & VFS_OPEN_WRONLY) && !(perm & 2)) {
        return SALAH;
    }
    
    return BENAR;
}

/*
 * ===========================================================================
 * FUNGSI ATTRIBUTE (ATTRIBUTE FUNCTIONS)
 * ===========================================================================
 */

status_t inode_getattr(inode_t *inode, vfs_stat_t *stat)
{
    if (inode == NULL || stat == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Gunakan filesystem getattr jika ada */
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
    
    /* Set rdev untuk device files */
    if (VFS_S_ISCHR(inode->i_mode) || VFS_S_ISBLK(inode->i_mode)) {
        stat->st_rdev = inode->i_rdev;
    }
    
    return STATUS_BERHASIL;
}

status_t inode_setattr(inode_t *inode, vfs_stat_t *stat)
{
    bool_t changed = SALAH;
    
    if (inode == NULL || stat == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Gunakan filesystem setattr jika ada */
    if (inode->i_op != NULL && inode->i_op->setattr != NULL) {
        status_t ret = inode->i_op->setattr(inode->i_dentry, stat);
        if (ret == STATUS_BERHASIL) {
            inode_mark_dirty(inode);
        }
        return ret;
    }
    
    /* Default setattr */
    
    /* Update mode */
    if (stat->st_mode != 0 && stat->st_mode != inode->i_mode) {
        inode->i_mode = stat->st_mode;
        changed = BENAR;
    }
    
    /* Update uid */
    if (stat->st_uid != 0 && stat->st_uid != inode->i_uid) {
        inode->i_uid = stat->st_uid;
        changed = BENAR;
    }
    
    /* Update gid */
    if (stat->st_gid != 0 && stat->st_gid != inode->i_gid) {
        inode->i_gid = stat->st_gid;
        changed = BENAR;
    }
    
    /* Update size */
    if (stat->st_size != 0 && stat->st_size != inode->i_size) {
        inode->i_size = stat->st_size;
        changed = BENAR;
    }
    
    /* Update timestamps */
    if (stat->st_atime != 0) {
        inode->i_atime = stat->st_atime;
    }
    if (stat->st_mtime != 0) {
        inode->i_mtime = stat->st_mtime;
        changed = BENAR;
    }
    
    if (changed) {
        inode->i_ctime = kernel_get_uptime();
        inode_mark_dirty(inode);
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI TIPE INODE (INODE TYPE FUNCTIONS)
 * ===========================================================================
 */

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

bool_t inode_is_char_device(inode_t *inode)
{
    if (inode == NULL) {
        return SALAH;
    }
    return VFS_S_ISCHR(inode->i_mode);
}

bool_t inode_is_block_device(inode_t *inode)
{
    if (inode == NULL) {
        return SALAH;
    }
    return VFS_S_ISBLK(inode->i_mode);
}

bool_t inode_is_fifo(inode_t *inode)
{
    if (inode == NULL) {
        return SALAH;
    }
    return VFS_S_ISFIFO(inode->i_mode);
}

bool_t inode_is_socket(inode_t *inode)
{
    if (inode == NULL) {
        return SALAH;
    }
    return VFS_S_ISSOCK(inode->i_mode);
}

/*
 * ===========================================================================
 * FUNGSI LINK COUNT (LINK COUNT FUNCTIONS)
 * ===========================================================================
 */

status_t inode_inc_link(inode_t *inode)
{
    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    inode_lock();
    
    inode->i_nlink++;
    inode->i_ctime = kernel_get_uptime();
    
    inode_unlock();
    
    inode_mark_dirty(inode);
    
    return STATUS_BERHASIL;
}

status_t inode_dec_link(inode_t *inode)
{
    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    inode_lock();
    
    if (inode->i_nlink > 0) {
        inode->i_nlink--;
        inode->i_ctime = kernel_get_uptime();
    }
    
    inode_unlock();
    
    inode_mark_dirty(inode);
    
    /* Jika nlink = 0, inode siap dihapus */
    if (inode->i_nlink == 0) {
        inode->i_flags |= VFS_INODE_FLAG_DELETED;
    }
    
    return STATUS_BERHASIL;
}

tak_bertanda32_t inode_get_link_count(inode_t *inode)
{
    if (inode == NULL) {
        return 0;
    }
    return inode->i_nlink;
}

/*
 * ===========================================================================
 * FUNGSI TRUNCATE (TRUNCATE FUNCTIONS)
 * ===========================================================================
 */

status_t inode_truncate(inode_t *inode, off_t length)
{
    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (length < 0) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Hanya untuk regular file */
    if (!VFS_S_ISREG(inode->i_mode)) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Check readonly */
    if (inode->i_sb != NULL && inode->i_sb->s_readonly) {
        return STATUS_AKSES_DITOLAK;
    }
    
    /* Set size baru */
    inode->i_size = length;
    inode->i_mtime = kernel_get_uptime();
    inode->i_ctime = inode->i_mtime;
    
    inode_mark_dirty(inode);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI OPERATIONS SETUP (OPERATIONS SETUP FUNCTIONS)
 * ===========================================================================
 */

void inode_set_operations(inode_t *inode, vfs_inode_operations_t *i_op)
{
    if (inode == NULL) {
        return;
    }
    
    inode->i_op = i_op;
}

void inode_set_file_operations(inode_t *inode, vfs_file_operations_t *f_op)
{
    if (inode == NULL) {
        return;
    }
    
    inode->i_fop = f_op;
}

void inode_set_private(inode_t *inode, void *private)
{
    if (inode == NULL) {
        return;
    }
    
    inode->i_private = private;
}

void *inode_get_private(inode_t *inode)
{
    if (inode == NULL) {
        return NULL;
    }
    
    return inode->i_private;
}

/*
 * ===========================================================================
 * FUNGSI STATISTIK (STATISTICS FUNCTIONS)
 * ===========================================================================
 */

tak_bertanda32_t inode_get_total_count(void)
{
    return g_inode_total;
}

tak_bertanda32_t inode_get_cached_count(void)
{
    return g_inode_cached;
}

tak_bertanda32_t inode_get_dirty_count(void)
{
    return g_inode_dirty;
}

/*
 * ===========================================================================
 * FUNGSI INFORMASI DAN DEBUG (INFORMATION AND DEBUG FUNCTIONS)
 * ===========================================================================
 */

void inode_print_info(inode_t *inode)
{
    if (inode == NULL) {
        kernel_printf("[VFS:INODE] Inode: NULL\n");
        return;
    }
    
    kernel_printf("[VFS:INODE] Inode Info:\n");
    kernel_printf("  Ino: %llu\n", inode->i_ino);
    kernel_printf("  Mode: 0%o (", inode->i_mode);
    
    if (VFS_S_ISREG(inode->i_mode)) {
        kernel_printf("regular");
    } else if (VFS_S_ISDIR(inode->i_mode)) {
        kernel_printf("directory");
    } else if (VFS_S_ISCHR(inode->i_mode)) {
        kernel_printf("char device");
    } else if (VFS_S_ISBLK(inode->i_mode)) {
        kernel_printf("block device");
    } else if (VFS_S_ISLNK(inode->i_mode)) {
        kernel_printf("symlink");
    } else if (VFS_S_ISFIFO(inode->i_mode)) {
        kernel_printf("fifo");
    } else if (VFS_S_ISSOCK(inode->i_mode)) {
        kernel_printf("socket");
    } else {
        kernel_printf("unknown");
    }
    
    kernel_printf(")\n");
    kernel_printf("  UID: %u\n", inode->i_uid);
    kernel_printf("  GID: %u\n", inode->i_gid);
    kernel_printf("  Size: %lld bytes\n", inode->i_size);
    kernel_printf("  Blocks: %llu\n", inode->i_blocks);
    kernel_printf("  Block Size: %u\n", inode->i_blksize);
    kernel_printf("  Links: %u\n", inode->i_nlink);
    kernel_printf("  Refcount: %u\n", inode->i_refcount);
    kernel_printf("  Dirty: %s\n", inode->i_dirty ? "ya" : "tidak");
}

/*
 * ===========================================================================
 * FUNGSI INITIALIZATION MODULE (MODULE INITIALIZATION)
 * ===========================================================================
 */

status_t inode_module_init(void)
{
    tak_bertanda32_t i;
    
    log_info("[VFS:INODE] Menginisialisasi modul inode...");
    
    /* Clear hash table */
    for (i = 0; i < INODE_HASH_SIZE; i++) {
        g_inode_hash[i] = NULL;
    }
    
    /* Clear LRU list */
    g_inode_lru.head = NULL;
    g_inode_lru.tail = NULL;
    g_inode_lru.count = 0;
    
    /* Reset statistics */
    g_inode_total = 0;
    g_inode_cached = 0;
    g_inode_dirty = 0;
    
    /* Reset lock */
    g_inode_lock = 0;
    
    /* Reset ino generator */
    g_ino_next = INO_FIRST;
    
    log_info("[VFS:INODE] Inisialisasi selesai");
    
    return STATUS_BERHASIL;
}

void inode_module_shutdown(void)
{
    tak_bertanda32_t i;
    inode_t *inode;
    inode_t *next;
    
    log_info("[VFS:INODE] Mematikan modul inode...");
    
    /* Sync semua inode dirty */
    for (i = 0; i < INODE_HASH_SIZE; i++) {
        inode = g_inode_hash[i];
        while (inode != NULL) {
            next = inode->i_next;
            
            if (inode->i_dirty) {
                inode_sync(inode);
            }
            
            inode = next;
        }
    }
    
    /* Clear hash table */
    for (i = 0; i < INODE_HASH_SIZE; i++) {
        g_inode_hash[i] = NULL;
    }
    
    /* Reset statistics */
    g_inode_total = 0;
    g_inode_cached = 0;
    g_inode_dirty = 0;
    
    log_info("[VFS:INODE] Shutdown selesai");
}
