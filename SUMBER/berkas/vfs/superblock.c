/*
 * PIGURA OS - SUPERBLOCK.C
 * ========================
 * Implementasi manajemen superblock untuk VFS Pigura OS.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola superblock,
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

/* Jumlah maksimum superblock dalam cache */
#define SUPERBLOCK_CACHE_MAKS   32

/* Ukuran default block */
#define BLOCK_SIZE_DEFAULT      4096

/* Magic number superblock yang valid */
#define SUPERBLOCK_MAGIC_MIN    0x0001
#define SUPERBLOCK_MAGIC_MAX    0xFFFE

/*
 * ===========================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ===========================================================================
 */

/* Cache superblock */
static superblock_t *g_sb_cache[SUPERBLOCK_CACHE_MAKS];
static tak_bertanda32_t g_sb_cache_count = 0;

/* Lock untuk operasi superblock */
static tak_bertanda32_t g_sb_lock = 0;

/*
 * ===========================================================================
 * FUNGSI INTERNAL LOCKING (INTERNAL LOCKING FUNCTIONS)
 * ===========================================================================
 */

static void sb_lock(void)
{
    g_sb_lock++;
}

static void sb_unlock(void)
{
    if (g_sb_lock > 0) {
        g_sb_lock--;
    }
}

/*
 * ===========================================================================
 * FUNGSI CACHE SUPERBLOCK (SUPERBLOCK CACHE FUNCTIONS)
 * ===========================================================================
 */

static status_t sb_cache_insert(superblock_t *sb)
{
    tak_bertanda32_t i;
    
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (g_sb_cache_count >= SUPERBLOCK_CACHE_MAKS) {
        /* Cache penuh, tidak perlu insert */
        return STATUS_BERHASIL;
    }
    
    sb_lock();
    
    for (i = 0; i < SUPERBLOCK_CACHE_MAKS; i++) {
        if (g_sb_cache[i] == NULL) {
            g_sb_cache[i] = sb;
            g_sb_cache_count++;
            sb_unlock();
            return STATUS_BERHASIL;
        }
    }
    
    sb_unlock();
    
    return STATUS_BERHASIL;
}

static status_t sb_cache_remove(superblock_t *sb)
{
    tak_bertanda32_t i;
    
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    sb_lock();
    
    for (i = 0; i < SUPERBLOCK_CACHE_MAKS; i++) {
        if (g_sb_cache[i] == sb) {
            g_sb_cache[i] = NULL;
            if (g_sb_cache_count > 0) {
                g_sb_cache_count--;
            }
            sb_unlock();
            return STATUS_BERHASIL;
        }
    }
    
    sb_unlock();
    
    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * ===========================================================================
 * FUNGSI VALIDASI SUPERBLOCK (SUPERBLOCK VALIDATION FUNCTIONS)
 * ===========================================================================
 */

bool_t superblock_valid(superblock_t *sb)
{
    if (sb == NULL) {
        return SALAH;
    }
    
    /* Cek magic number */
    if (sb->s_magic != VFS_SUPER_MAGIC) {
        return SALAH;
    }
    
    /* Cek filesystem pointer */
    if (sb->s_fs == NULL) {
        return SALAH;
    }
    
    /* Cek block size valid */
    if (sb->s_block_size == 0 ||
        sb->s_block_size > 65536) {
        return SALAH;
    }
    
    return BENAR;
}

/*
 * ===========================================================================
 * FUNGSI ALOKASI DAN DEALOKASI (ALLOCATION AND DEALLOCATION FUNCTIONS)
 * ===========================================================================
 */

superblock_t *superblock_alloc(filesystem_t *fs)
{
    superblock_t *sb;
    
    if (fs == NULL) {
        log_error("[VFS:SB] Filesystem pointer NULL");
        return NULL;
    }
    
    /* Alokasi memori */
    sb = (superblock_t *)kmalloc(sizeof(superblock_t));
    if (sb == NULL) {
        log_error("[VFS:SB] Gagal alokasi memori superblock");
        return NULL;
    }
    
    /* Initialize dengan zero */
    kernel_memset(sb, 0, sizeof(superblock_t));
    
    /* Set nilai default */
    sb->s_magic = VFS_SUPER_MAGIC;
    sb->s_fs = fs;
    sb->s_block_size = BLOCK_SIZE_DEFAULT;
    sb->s_refcount = 1;
    sb->s_readonly = SALAH;
    sb->s_dirty = SALAH;
    
    /* Set timestamps */
    sb->s_mtime = kernel_get_uptime();
    
    /* Tambah reference filesystem */
    fs->fs_refcount++;
    
    /* Insert ke cache */
    sb_cache_insert(sb);
    
    log_debug("[VFS:SB] Superblock dialokasi untuk %s", fs->fs_nama);
    
    return sb;
}

void superblock_free(superblock_t *sb)
{
    filesystem_t *fs;
    
    if (sb == NULL) {
        return;
    }
    
    /* Validasi magic number */
    if (sb->s_magic != VFS_SUPER_MAGIC) {
        log_warn("[VFS:SB] Magic number invalid saat free");
        return;
    }
    
    /* Jangan free jika masih ada reference */
    if (sb->s_refcount > 0) {
        log_warn("[VFS:SB] Masih ada %d reference", sb->s_refcount);
        return;
    }
    
    /* Sync jika dirty */
    if (sb->s_dirty) {
        superblock_sync(sb);
    }
    
    /* Panggil filesystem destructor jika ada */
    if (sb->s_op != NULL && sb->s_op->put_super != NULL) {
        sb->s_op->put_super(sb);
    }
    
    /* Simpan pointer filesystem */
    fs = sb->s_fs;
    
    /* Hapus dari cache */
    sb_cache_remove(sb);
    
    /* Clear memory */
    kernel_memset(sb, 0, sizeof(superblock_t));
    
    /* Free memory */
    kfree(sb);
    
    /* Kurangi reference filesystem */
    if (fs != NULL && fs->fs_refcount > 0) {
        fs->fs_refcount--;
    }
    
    log_debug("[VFS:SB] Superblock dibebaskan");
}

/*
 * ===========================================================================
 * FUNGSI INISIALISASI (INITIALIZATION FUNCTIONS)
 * ===========================================================================
 */

status_t superblock_init(superblock_t *sb, filesystem_t *fs, dev_t dev)
{
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (fs == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Set device */
    sb->s_dev = dev;
    
    /* Set filesystem */
    sb->s_fs = fs;
    
    /* Set block size default */
    sb->s_block_size = BLOCK_SIZE_DEFAULT;
    
    /* Set flags default */
    sb->s_flags = 0;
    sb->s_readonly = SALAH;
    
    /* Initialize counters */
    sb->s_refcount = 1;
    
    /* Set timestamps */
    sb->s_mtime = kernel_get_uptime();
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI REFERENCE COUNTING (REFERENCE COUNTING FUNCTIONS)
 * ===========================================================================
 */

void superblock_get(superblock_t *sb)
{
    if (sb == NULL) {
        return;
    }
    
    if (sb->s_magic != VFS_SUPER_MAGIC) {
        return;
    }
    
    sb_lock();
    
    sb->s_refcount++;
    
    sb_unlock();
}

void superblock_put(superblock_t *sb)
{
    if (sb == NULL) {
        return;
    }
    
    if (sb->s_magic != VFS_SUPER_MAGIC) {
        return;
    }
    
    sb_lock();
    
    if (sb->s_refcount > 0) {
        sb->s_refcount--;
        
        if (sb->s_refcount == 0) {
            sb_unlock();
            superblock_free(sb);
            return;
        }
    }
    
    sb_unlock();
}

/*
 * ===========================================================================
 * FUNGSI INODE MANAGEMENT (INODE MANAGEMENT FUNCTIONS)
 * ===========================================================================
 */

struct inode *superblock_alloc_inode(superblock_t *sb)
{
    struct inode *inode;
    
    if (sb == NULL) {
        return NULL;
    }
    
    /* Gunakan filesystem-specific allocator jika ada */
    if (sb->s_op != NULL && sb->s_op->alloc_inode != NULL) {
        return sb->s_op->alloc_inode(sb);
    }
    
    /* Default: gunakan inode_alloc dari VFS */
    inode = inode_alloc(sb);
    
    return inode;
}

void superblock_destroy_inode(superblock_t *sb, struct inode *inode)
{
    if (sb == NULL || inode == NULL) {
        return;
    }
    
    /* Gunakan filesystem-specific destructor jika ada */
    if (sb->s_op != NULL && sb->s_op->destroy_inode != NULL) {
        sb->s_op->destroy_inode(inode);
        return;
    }
    
    /* Default: gunakan inode_free dari VFS */
    inode_free(inode);
}

/*
 * ===========================================================================
 * FUNGSI SYNC DAN DIRTY (SYNC AND DIRTY FUNCTIONS)
 * ===========================================================================
 */

status_t superblock_sync(superblock_t *sb)
{
    status_t ret;
    
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (sb->s_magic != VFS_SUPER_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Tidak perlu sync jika tidak dirty */
    if (!sb->s_dirty) {
        return STATUS_BERHASIL;
    }
    
    /* Tidak perlu sync jika readonly */
    if (sb->s_readonly) {
        return STATUS_BERHASIL;
    }
    
    /* Panggil filesystem sync jika ada */
    if (sb->s_op != NULL && sb->s_op->sync_fs != NULL) {
        sb_lock();
        ret = sb->s_op->sync_fs(sb);
        sb_unlock();
        
        if (ret == STATUS_BERHASIL) {
            sb->s_dirty = SALAH;
            log_debug("[VFS:SB] Superblock disync");
        }
        
        return ret;
    }
    
    /* Tidak ada sync function, anggap berhasil */
    sb->s_dirty = SALAH;
    
    return STATUS_BERHASIL;
}

void superblock_mark_dirty(superblock_t *sb)
{
    if (sb == NULL) {
        return;
    }
    
    if (sb->s_magic != VFS_SUPER_MAGIC) {
        return;
    }
    
    sb->s_dirty = BENAR;
    sb->s_mtime = kernel_get_uptime();
}

/*
 * ===========================================================================
 * FUNGSI STATISTIK (STATISTICS FUNCTIONS)
 * ===========================================================================
 */

status_t superblock_statfs(superblock_t *sb, vfs_stat_t *stat)
{
    if (sb == NULL || stat == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (sb->s_magic != VFS_SUPER_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Gunakan filesystem statfs jika ada */
    if (sb->s_op != NULL && sb->s_op->statfs != NULL) {
        return sb->s_op->statfs(sb, stat);
    }
    
    /* Default statfs */
    kernel_memset(stat, 0, sizeof(vfs_stat_t));
    
    stat->st_dev = sb->s_dev;
    stat->st_blksize = (tak_bertanda64_t)sb->s_block_size;
    stat->st_blocks = sb->s_total_blocks;
    stat->st_size = sb->s_total_blocks * sb->s_block_size;
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI BLOCK MANAGEMENT (BLOCK MANAGEMENT FUNCTIONS)
 * ===========================================================================
 */

tak_bertanda64_t superblock_get_free_blocks(superblock_t *sb)
{
    if (sb == NULL) {
        return 0;
    }
    
    return sb->s_free_blocks;
}

tak_bertanda64_t superblock_get_total_blocks(superblock_t *sb)
{
    if (sb == NULL) {
        return 0;
    }
    
    return sb->s_total_blocks;
}

status_t superblock_reserve_blocks(superblock_t *sb,
                                   tak_bertanda64_t count)
{
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (sb->s_readonly) {
        return STATUS_AKSES_DITOLAK;
    }
    
    sb_lock();
    
    if (sb->s_free_blocks < count) {
        sb_unlock();
        return STATUS_MEMORI_HABIS;
    }
    
    sb->s_free_blocks -= count;
    sb->s_dirty = BENAR;
    
    sb_unlock();
    
    return STATUS_BERHASIL;
}

status_t superblock_release_blocks(superblock_t *sb,
                                   tak_bertanda64_t count)
{
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    sb_lock();
    
    sb->s_free_blocks += count;
    if (sb->s_free_blocks > sb->s_total_blocks) {
        sb->s_free_blocks = sb->s_total_blocks;
    }
    sb->s_dirty = BENAR;
    
    sb_unlock();
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI INODE MANAGEMENT (INODE MANAGEMENT FUNCTIONS)
 * ===========================================================================
 */

tak_bertanda64_t superblock_get_free_inodes(superblock_t *sb)
{
    if (sb == NULL) {
        return 0;
    }
    
    return sb->s_free_inodes;
}

tak_bertanda64_t superblock_get_total_inodes(superblock_t *sb)
{
    if (sb == NULL) {
        return 0;
    }
    
    return sb->s_total_inodes;
}

status_t superblock_reserve_inode(superblock_t *sb)
{
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (sb->s_readonly) {
        return STATUS_AKSES_DITOLAK;
    }
    
    sb_lock();
    
    if (sb->s_free_inodes == 0) {
        sb_unlock();
        return STATUS_MEMORI_HABIS;
    }
    
    sb->s_free_inodes--;
    sb->s_dirty = BENAR;
    
    sb_unlock();
    
    return STATUS_BERHASIL;
}

status_t superblock_release_inode(superblock_t *sb)
{
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    sb_lock();
    
    sb->s_free_inodes++;
    if (sb->s_free_inodes > sb->s_total_inodes) {
        sb->s_free_inodes = sb->s_total_inodes;
    }
    sb->s_dirty = BENAR;
    
    sb_unlock();
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI REMOUNT (REMOUNT FUNCTIONS)
 * ===========================================================================
 */

status_t superblock_remount(superblock_t *sb, tak_bertanda32_t flags)
{
    status_t ret;
    bool_t was_readonly;
    bool_t now_readonly;
    
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    was_readonly = sb->s_readonly;
    now_readonly = (flags & VFS_MOUNT_FLAG_RDONLY) != 0;
    
    /* Jika berubah dari RW ke RO, sync dulu */
    if (!was_readonly && now_readonly) {
        superblock_sync(sb);
    }
    
    /* Gunakan filesystem remount jika ada */
    if (sb->s_op != NULL && sb->s_op->remount_fs != NULL) {
        ret = sb->s_op->remount_fs(sb, flags);
        if (ret != STATUS_BERHASIL) {
            return ret;
        }
    }
    
    /* Update flags */
    sb->s_flags = flags;
    sb->s_readonly = now_readonly;
    
    log_info("[VFS:SB] Remount superblock, readonly=%s",
             now_readonly ? "ya" : "tidak");
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI UTILITAS (UTILITY FUNCTIONS)
 * ===========================================================================
 */

bool_t superblock_is_readonly(superblock_t *sb)
{
    if (sb == NULL) {
        return BENAR;
    }
    
    return sb->s_readonly;
}

bool_t superblock_is_dirty(superblock_t *sb)
{
    if (sb == NULL) {
        return SALAH;
    }
    
    return sb->s_dirty;
}

filesystem_t *superblock_get_filesystem(superblock_t *sb)
{
    if (sb == NULL) {
        return NULL;
    }
    
    return sb->s_fs;
}

dentry_t *superblock_get_root(superblock_t *sb)
{
    if (sb == NULL) {
        return NULL;
    }
    
    return sb->s_root;
}

/*
 * ===========================================================================
 * FUNGSI INFORMASI DAN DEBUG (INFORMATION AND DEBUG FUNCTIONS)
 * ===========================================================================
 */

void superblock_print_info(superblock_t *sb)
{
    if (sb == NULL) {
        kernel_printf("[VFS:SB] Superblock: NULL\n");
        return;
    }
    
    kernel_printf("[VFS:SB] Superblock Info:\n");
    kernel_printf("  Magic: 0x%08X %s\n",
                  sb->s_magic,
                  sb->s_magic == VFS_SUPER_MAGIC ? "(valid)" : "(invalid)");
    kernel_printf("  Device: 0x%08X\n", sb->s_dev);
    kernel_printf("  Block Size: %llu bytes\n", sb->s_block_size);
    kernel_printf("  Total Blocks: %llu\n", sb->s_total_blocks);
    kernel_printf("  Free Blocks: %llu\n", sb->s_free_blocks);
    kernel_printf("  Total Inodes: %llu\n", sb->s_total_inodes);
    kernel_printf("  Free Inodes: %llu\n", sb->s_free_inodes);
    kernel_printf("  Refcount: %u\n", sb->s_refcount);
    kernel_printf("  Readonly: %s\n", sb->s_readonly ? "ya" : "tidak");
    kernel_printf("  Dirty: %s\n", sb->s_dirty ? "ya" : "tidak");
    
    if (sb->s_fs != NULL) {
        kernel_printf("  Filesystem: %s\n", sb->s_fs->fs_nama);
    }
}

tak_bertanda32_t superblock_get_cache_count(void)
{
    return g_sb_cache_count;
}

/*
 * ===========================================================================
 * FUNGSI INITIALIZATION MODULE (MODULE INITIALIZATION)
 * ===========================================================================
 */

status_t superblock_module_init(void)
{
    tak_bertanda32_t i;
    
    log_info("[VFS:SB] Menginisialisasi modul superblock...");
    
    /* Clear cache */
    for (i = 0; i < SUPERBLOCK_CACHE_MAKS; i++) {
        g_sb_cache[i] = NULL;
    }
    g_sb_cache_count = 0;
    
    /* Clear lock */
    g_sb_lock = 0;
    
    log_info("[VFS:SB] Inisialisasi selesai");
    
    return STATUS_BERHASIL;
}

void superblock_module_shutdown(void)
{
    tak_bertanda32_t i;
    
    log_info("[VFS:SB] Mematikan modul superblock...");
    
    /* Sync dan free semua superblock di cache */
    for (i = 0; i < SUPERBLOCK_CACHE_MAKS; i++) {
        if (g_sb_cache[i] != NULL) {
            superblock_sync(g_sb_cache[i]);
            /* Jangan free di sini, biarkan VFS yang handle */
            g_sb_cache[i] = NULL;
        }
    }
    
    g_sb_cache_count = 0;
    
    log_info("[VFS:SB] Shutdown selesai");
}
