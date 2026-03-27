/*
 * PIGURA OS - MOUNT.C
 * ====================
 * Implementasi mount point management untuk VFS Pigura OS.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola mount point,
 * termasuk mount, umount, dan operasi-operasi terkait.
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

/* Jumlah maksimum mount point */
#define MOUNT_MAX               VFS_MOUNT_MAKS

/* Path root */
#define MOUNT_ROOT_PATH         "/"

/*
 * ===========================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ===========================================================================
 */

/* Mount list */
static mount_t *g_mount_list = NULL;
static mount_t *g_root_mount = NULL;
static tak_bertanda32_t g_mount_count = 0;

/* Lock untuk operasi mount */
static tak_bertanda32_t g_mount_lock = 0;

/*
 * ===========================================================================
 * FUNGSI INTERNAL LOCKING (INTERNAL LOCKING FUNCTIONS)
 * ===========================================================================
 */

static void mount_lock(void)
{
    g_mount_lock++;
}

static void mount_unlock(void)
{
    if (g_mount_lock > 0) {
        g_mount_lock--;
    }
}

/*
 * ===========================================================================
 * FUNGSI ALOKASI DAN DEALOKASI (ALLOCATION AND DEALLOCATION FUNCTIONS)
 * ===========================================================================
 */

mount_t *mount_alloc(void)
{
    mount_t *mount;
    
    /* Cek limit */
    if (g_mount_count >= MOUNT_MAX) {
        log_error("[VFS:MOUNT] Jumlah mount maksimum tercapai");
        return NULL;
    }
    
    /* Alokasi memori */
    mount = (mount_t *)kmalloc(sizeof(mount_t));
    if (mount == NULL) {
        log_error("[VFS:MOUNT] Gagal alokasi memori");
        return NULL;
    }
    
    /* Clear dan init */
    kernel_memset(mount, 0, sizeof(mount_t));
    
    mount->m_magic = VFS_MOUNT_MAGIC;
    mount->m_refcount = 1;
    mount->m_readonly = SALAH;
    
    log_debug("[VFS:MOUNT] Mount structure dialokasi");
    
    return mount;
}

void mount_free(mount_t *mount)
{
    if (mount == NULL) {
        return;
    }
    
    /* Validasi magic */
    if (mount->m_magic != VFS_MOUNT_MAGIC) {
        log_warn("[VFS:MOUNT] Magic invalid saat free");
        return;
    }
    
    /* Jangan free jika masih ada reference */
    if (mount->m_refcount > 0) {
        log_debug("[VFS:MOUNT] Mount masih di-reference: %u",
                  mount->m_refcount);
        return;
    }
    
    /* Release superblock */
    if (mount->m_sb != NULL) {
        superblock_put(mount->m_sb);
    }
    
    /* Release filesystem */
    if (mount->m_fs != NULL && mount->m_fs->fs_refcount > 0) {
        mount->m_fs->fs_refcount--;
    }
    
    /* Release mountpoint dentry */
    if (mount->m_mountpoint != NULL) {
        mount->m_mountpoint->d_mounted = SALAH;
        dentry_put(mount->m_mountpoint);
    }
    
    /* Release root dentry */
    if (mount->m_root != NULL) {
        dentry_put(mount->m_root);
    }
    
    /* Clear dan free */
    kernel_memset(mount, 0, sizeof(mount_t));
    kfree(mount);
    
    log_debug("[VFS:MOUNT] Mount structure dibebaskan");
}

/*
 * ===========================================================================
 * FUNGSI REFERENCE COUNTING (REFERENCE COUNTING FUNCTIONS)
 * ===========================================================================
 */

void mount_get(mount_t *mount)
{
    if (mount == NULL) {
        return;
    }
    
    if (mount->m_magic != VFS_MOUNT_MAGIC) {
        return;
    }
    
    mount_lock();
    mount->m_refcount++;
    mount_unlock();
}

void mount_put(mount_t *mount)
{
    if (mount == NULL) {
        return;
    }
    
    if (mount->m_magic != VFS_MOUNT_MAGIC) {
        return;
    }
    
    mount_lock();
    
    if (mount->m_refcount > 0) {
        mount->m_refcount--;
        
        if (mount->m_refcount == 0) {
            mount_unlock();
            mount_free(mount);
            return;
        }
    }
    
    mount_unlock();
}

/*
 * ===========================================================================
 * FUNGSI MOUNT/UMOUNT (MOUNT/UMOUNT FUNCTIONS)
 * ===========================================================================
 */

status_t mount_create(const char *device, const char *path,
                      const char *fs_nama, tak_bertanda32_t flags)
{
    filesystem_t *fs;
    mount_t *mount;
    mount_t *existing;
    superblock_t *sb;
    dentry_t *mountpoint;
    dentry_t *root;
    
    if (path == NULL || fs_nama == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Cari filesystem */
    fs = filesystem_find(fs_nama);
    if (fs == NULL) {
        log_error("[VFS:MOUNT] Filesystem tidak ditemukan: %s", fs_nama);
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Validasi path */
    if (!vfs_path_valid(path)) {
        log_error("[VFS:MOUNT] Path tidak valid: %s", path);
        return STATUS_PARAM_INVALID;
    }
    
    mount_lock();
    
    /* Cek apakah sudah di-mount */
    existing = NULL;
    {
        mount_t *curr = g_mount_list;
        while (curr != NULL) {
            if (kernel_strcmp(curr->m_path, path) == 0) {
                existing = curr;
                break;
            }
            curr = curr->m_next;
        }
    }
    
    if (existing != NULL) {
        mount_unlock();
        log_warn("[VFS:MOUNT] Path sudah di-mount: %s", path);
        return STATUS_SUDAH_ADA;
    }
    
    /* Cari mountpoint dentry */
    if (kernel_strcmp(path, MOUNT_ROOT_PATH) == 0) {
        /* Root mount */
        mountpoint = NULL;
    } else {
        /* Non-root mount */
        mount_unlock();
        mountpoint = namei_lookup(path);
        mount_lock();
        
        if (mountpoint == NULL) {
            mount_unlock();
            log_error("[VFS:MOUNT] Mountpoint tidak ditemukan: %s", path);
            return STATUS_TIDAK_DITEMUKAN;
        }
        
        /* Harus direktori */
        if (mountpoint->d_inode == NULL ||
            !VFS_S_ISDIR(mountpoint->d_inode->i_mode)) {
            dentry_put(mountpoint);
            mount_unlock();
            log_error("[VFS:MOUNT] Mountpoint bukan direktori: %s", path);
            return STATUS_PARAM_INVALID;
        }
        
        /* Cek apakah sudah ada mount */
        if (mountpoint->d_mounted) {
            dentry_put(mountpoint);
            mount_unlock();
            log_error("[VFS:MOUNT] Mountpoint sudah di-mount: %s", path);
            return STATUS_SUDAH_ADA;
        }
    }
    
    /* Cek limit */
    if (g_mount_count >= MOUNT_MAX) {
        if (mountpoint != NULL) {
            dentry_put(mountpoint);
        }
        mount_unlock();
        log_error("[VFS:MOUNT] Jumlah mount maksimum tercapai");
        return STATUS_PENUH;
    }
    
    /* Panggil filesystem mount function */
    if (fs->fs_mount == NULL) {
        if (mountpoint != NULL) {
            dentry_put(mountpoint);
        }
        mount_unlock();
        log_error("[VFS:MOUNT] Filesystem tidak mendukung mount: %s",
                  fs_nama);
        return STATUS_TIDAK_DUKUNG;
    }
    
    sb = fs->fs_mount(fs, device, path, flags);
    if (sb == NULL) {
        if (mountpoint != NULL) {
            dentry_put(mountpoint);
        }
        mount_unlock();
        log_error("[VFS:MOUNT] Gagal mount filesystem: %s", fs_nama);
        return STATUS_GAGAL;
    }
    
    /* Buat mount structure */
    mount = mount_alloc();
    if (mount == NULL) {
        if (mountpoint != NULL) {
            dentry_put(mountpoint);
        }
        mount_unlock();
        return STATUS_MEMORI_HABIS;
    }
    
    /* Setup mount structure */
    mount->m_fs = fs;
    mount->m_sb = sb;
    mount->m_mountpoint = mountpoint;
    mount->m_flags = flags;
    mount->m_readonly = (flags & VFS_MOUNT_FLAG_RDONLY) != 0;
    
    /* Copy path dan device */
    kernel_strncpy(mount->m_path, path, VFS_PATH_MAKS);
    mount->m_path[VFS_PATH_MAKS] = '\0';
    
    if (device != NULL) {
        kernel_strncpy(mount->m_device, device, VFS_PATH_MAKS);
        mount->m_device[VFS_PATH_MAKS] = '\0';
    } else {
        mount->m_device[0] = '\0';
    }
    
    /* Dapatkan root dentry dari superblock */
    root = sb->s_root;
    if (root != NULL) {
        mount->m_root = root;
        dentry_get(root);
    }
    
    /* Set mountpoint mounted flag */
    if (mountpoint != NULL) {
        mountpoint->d_mounted = BENAR;
    }
    
    /* Set parent mount */
    if (mountpoint != NULL && mountpoint->d_mount != NULL) {
        mount->m_parent = mountpoint->d_mount;
    } else if (g_root_mount != NULL && mount != g_root_mount) {
        mount->m_parent = g_root_mount;
    }
    
    /* Set superblock mount reference */
    sb->s_mount = mount;
    
    /* Reference filesystem */
    fs->fs_refcount++;
    
    /* Add ke mount list */
    mount->m_next = g_mount_list;
    g_mount_list = mount;
    g_mount_count++;
    
    /* Set sebagai root mount jika pertama kali */
    if (g_root_mount == NULL) {
        g_root_mount = mount;
    }
    
    mount_unlock();
    
    log_info("[VFS:MOUNT] Mounted %s at %s (fs=%s)",
             device ? device : "none", path, fs_nama);
    
    return STATUS_BERHASIL;
}

status_t mount_destroy(const char *path)
{
    mount_t *mount;
    mount_t *curr;
    mount_t *prev;
    
    if (path == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    mount_lock();
    
    /* Cari mount */
    mount = NULL;
    prev = NULL;
    curr = g_mount_list;
    
    while (curr != NULL) {
        if (kernel_strcmp(curr->m_path, path) == 0) {
            mount = curr;
            break;
        }
        prev = curr;
        curr = curr->m_next;
    }
    
    if (mount == NULL) {
        mount_unlock();
        log_error("[VFS:MOUNT] Mount tidak ditemukan: %s", path);
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Jangan umount root jika masih ada mount lain */
    if (mount == g_root_mount && g_mount_count > 1) {
        mount_unlock();
        log_error("[VFS:MOUNT] Tidak dapat umount root dengan mount aktif");
        return STATUS_BUSY;
    }
    
    /* Cek reference count */
    if (mount->m_refcount > 1) {
        mount_unlock();
        log_warn("[VFS:MOUNT] Mount masih digunakan: %s", path);
        return STATUS_BUSY;
    }
    
    /* Cek apakah ada child mount */
    curr = g_mount_list;
    while (curr != NULL) {
        if (curr->m_parent == mount) {
            mount_unlock();
            log_error("[VFS:MOUNT] Ada mount child: %s", curr->m_path);
            return STATUS_BUSY;
        }
        curr = curr->m_next;
    }
    
    /* Sync superblock */
    if (mount->m_sb != NULL && !mount->m_readonly) {
        superblock_sync(mount->m_sb);
    }
    
    /* Panggil filesystem umount */
    if (mount->m_fs != NULL && mount->m_fs->fs_umount != NULL) {
        status_t ret = mount->m_fs->fs_umount(mount->m_sb);
        if (ret != STATUS_BERHASIL) {
            mount_unlock();
            log_error("[VFS:MOUNT] Gagal umount filesystem: %s", path);
            return ret;
        }
    }
    
    /* Remove dari list */
    if (prev == NULL) {
        g_mount_list = mount->m_next;
    } else {
        prev->m_next = mount->m_next;
    }
    
    g_mount_count--;
    
    /* Clear root mount jika perlu */
    if (mount == g_root_mount) {
        g_root_mount = NULL;
    }
    
    mount_unlock();
    
    log_info("[VFS:MOUNT] Unmounted %s", path);
    
    /* Free mount structure */
    mount->m_refcount = 0;
    mount_free(mount);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI LOOKUP (LOOKUP FUNCTIONS)
 * ===========================================================================
 */

mount_t *mount_find(const char *path)
{
    mount_t *curr;
    mount_t *found = NULL;
    ukuran_t best_len = 0;
    ukuran_t curr_len;
    
    if (path == NULL) {
        return NULL;
    }
    
    mount_lock();
    
    /* Cari mount dengan path terpanjang yang match */
    curr = g_mount_list;
    while (curr != NULL) {
        curr_len = kernel_strlen(curr->m_path);
        
        /* Path harus diawali dengan mount path */
        if (kernel_strncmp(curr->m_path, path, curr_len) == 0) {
            /* Cek apakah ini match terbaik */
            if (curr_len > best_len) {
                best_len = curr_len;
                found = curr;
            }
        }
        
        curr = curr->m_next;
    }
    
    if (found != NULL) {
        mount_get(found);
    }
    
    mount_unlock();
    
    return found;
}

mount_t *mount_find_by_dev(dev_t dev)
{
    mount_t *curr;
    mount_t *found = NULL;
    
    mount_lock();
    
    curr = g_mount_list;
    while (curr != NULL) {
        if (curr->m_sb != NULL && curr->m_sb->s_dev == dev) {
            found = curr;
            break;
        }
        curr = curr->m_next;
    }
    
    if (found != NULL) {
        mount_get(found);
    }
    
    mount_unlock();
    
    return found;
}

mount_t *mount_root(void)
{
    if (g_root_mount == NULL) {
        return NULL;
    }
    
    mount_get(g_root_mount);
    return g_root_mount;
}

/*
 * ===========================================================================
 * FUNGSI ITERASI (ITERATION FUNCTIONS)
 * ===========================================================================
 */

mount_t *mount_first(void)
{
    mount_t *mount;
    
    mount_lock();
    
    mount = g_mount_list;
    if (mount != NULL) {
        mount_get(mount);
    }
    
    mount_unlock();
    
    return mount;
}

mount_t *mount_next(mount_t *mount)
{
    mount_t *next;
    
    if (mount == NULL) {
        return NULL;
    }
    
    mount_lock();
    
    next = mount->m_next;
    if (next != NULL) {
        mount_get(next);
    }
    
    mount_unlock();
    
    return next;
}

/*
 * ===========================================================================
 * FUNGSI PATH RESOLUTION (PATH RESOLUTION FUNCTIONS)
 * ===========================================================================
 */

dentry_t *mount_resolve_path(const char *path)
{
    mount_t *mount;
    dentry_t *dentry;
    const char *relative;
    ukuran_t mount_path_len;
    
    if (path == NULL) {
        return NULL;
    }
    
    /* Cari mount point */
    mount = mount_find(path);
    if (mount == NULL) {
        return NULL;
    }
    
    /* Dapatkan root dentry */
    dentry = mount->m_root;
    if (dentry == NULL) {
        mount_put(mount);
        return NULL;
    }
    
    dentry_get(dentry);
    
    /* Hitung path relatif */
    mount_path_len = kernel_strlen(mount->m_path);
    relative = path + mount_path_len;
    
    /* Skip leading slash */
    while (*relative == '/') {
        relative++;
    }
    
    /* Jika tidak ada path relatif, return root */
    if (*relative == '\0') {
        mount_put(mount);
        return dentry;
    }
    
    /* TODO: Implementasi path traversal dengan namei */
    
    mount_put(mount);
    
    return dentry;
}

/*
 * ===========================================================================
 * FUNGSI UTILITAS (UTILITY FUNCTIONS)
 * ===========================================================================
 */

bool_t mount_is_readonly(mount_t *mount)
{
    if (mount == NULL) {
        return BENAR;
    }
    return mount->m_readonly;
}

bool_t mount_is_root(mount_t *mount)
{
    if (mount == NULL) {
        return SALAH;
    }
    return mount == g_root_mount;
}

filesystem_t *mount_get_filesystem(mount_t *mount)
{
    if (mount == NULL) {
        return NULL;
    }
    return mount->m_fs;
}

superblock_t *mount_get_superblock(mount_t *mount)
{
    if (mount == NULL) {
        return NULL;
    }
    return mount->m_sb;
}

const char *mount_get_path(mount_t *mount)
{
    if (mount == NULL) {
        return NULL;
    }
    return mount->m_path;
}

const char *mount_get_device(mount_t *mount)
{
    if (mount == NULL) {
        return NULL;
    }
    return mount->m_device;
}

/*
 * ===========================================================================
 * FUNGSI REMOUNT (REMOUNT FUNCTION)
 * ===========================================================================
 */

status_t mount_remount(const char *path, tak_bertanda32_t flags)
{
    mount_t *mount;
    mount_t *curr;
    
    if (path == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    mount_lock();
    
    /* Cari mount */
    mount = NULL;
    curr = g_mount_list;
    while (curr != NULL) {
        if (kernel_strcmp(curr->m_path, path) == 0) {
            mount = curr;
            break;
        }
        curr = curr->m_next;
    }
    
    if (mount == NULL) {
        mount_unlock();
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Remount superblock */
    if (mount->m_sb != NULL) {
        /* Sync dan update flags */
        if (!mount->m_readonly && (flags & VFS_MOUNT_FLAG_RDONLY)) {
            superblock_sync(mount->m_sb);
        }
        mount->m_sb->s_flags = flags;
        mount->m_sb->s_readonly = (flags & VFS_MOUNT_FLAG_RDONLY) != 0;
    }
    
    /* Update flags */
    mount->m_flags = flags;
    mount->m_readonly = (flags & VFS_MOUNT_FLAG_RDONLY) != 0;
    
    mount_unlock();
    
    log_info("[VFS:MOUNT] Remounted %s, readonly=%s",
             path, mount->m_readonly ? "ya" : "tidak");
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI STATISTIK (STATISTICS FUNCTIONS)
 * ===========================================================================
 */

tak_bertanda32_t mount_get_count(void)
{
    return g_mount_count;
}

/*
 * ===========================================================================
 * FUNGSI INFORMASI DAN DEBUG (INFORMATION AND DEBUG FUNCTIONS)
 * ===========================================================================
 */

void mount_print_info(mount_t *mount)
{
    if (mount == NULL) {
        kernel_printf("[VFS:MOUNT] Mount: NULL\n");
        return;
    }
    
    kernel_printf("[VFS:MOUNT] Mount Info:\n");
    kernel_printf("  Magic: 0x%08X %s\n",
                  mount->m_magic,
                  mount->m_magic == VFS_MOUNT_MAGIC ? "(valid)" : "(invalid)");
    kernel_printf("  Path: %s\n", mount->m_path);
    kernel_printf("  Device: %s\n", mount->m_device[0] ? mount->m_device : "none");
    kernel_printf("  Flags: 0x%08X\n", mount->m_flags);
    kernel_printf("  Readonly: %s\n", mount->m_readonly ? "ya" : "tidak");
    kernel_printf("  Refcount: %u\n", mount->m_refcount);
    
    if (mount->m_fs != NULL) {
        kernel_printf("  Filesystem: %s\n", mount->m_fs->fs_nama);
    }
    
    if (mount->m_sb != NULL) {
        kernel_printf("  Block Size: %llu\n", mount->m_sb->s_block_size);
        kernel_printf("  Total Blocks: %llu\n", mount->m_sb->s_total_blocks);
        kernel_printf("  Free Blocks: %llu\n", mount->m_sb->s_free_blocks);
    }
    
    if (mount->m_root != NULL) {
        kernel_printf("  Root: %s\n", mount->m_root->d_name);
    }
    
    if (mount->m_mountpoint != NULL) {
        kernel_printf("  Mountpoint: %s\n", mount->m_mountpoint->d_name);
    }
}

void mount_print_list(void)
{
    mount_t *curr;
    tak_bertanda32_t count = 0;
    
    kernel_printf("[VFS:MOUNT] Mount List:\n");
    
    mount_lock();
    
    curr = g_mount_list;
    while (curr != NULL) {
        kernel_printf("  %s on %s type %s (%s",
                      curr->m_device[0] ? curr->m_device : "none",
                      curr->m_path,
                      curr->m_fs ? curr->m_fs->fs_nama : "unknown",
                      curr->m_readonly ? "ro" : "rw");
        
        if (curr == g_root_mount) {
            kernel_printf(", root");
        }
        
        kernel_printf(")\n");
        
        count++;
        curr = curr->m_next;
    }
    
    mount_unlock();
    
    kernel_printf("  Total: %u mount points\n", count);
}

/*
 * ===========================================================================
 * FUNGSI INITIALIZATION MODULE (MODULE INITIALIZATION)
 * ===========================================================================
 */

status_t mount_module_init(void)
{
    log_info("[VFS:MOUNT] Menginisialisasi modul mount...");
    
    /* Clear mount list */
    g_mount_list = NULL;
    g_root_mount = NULL;
    g_mount_count = 0;
    
    /* Reset lock */
    g_mount_lock = 0;
    
    log_info("[VFS:MOUNT] Inisialisasi selesai");
    
    return STATUS_BERHASIL;
}

void mount_module_shutdown(void)
{
    mount_t *curr;
    mount_t *next;
    
    log_info("[VFS:MOUNT] Mematikan modul mount...");
    
    mount_lock();
    
    /* Sync dan umount semua mount */
    curr = g_mount_list;
    while (curr != NULL) {
        next = curr->m_next;
        
        /* Sync jika writable */
        if (curr->m_sb != NULL && !curr->m_readonly) {
            superblock_sync(curr->m_sb);
        }
        
        /* Panggil umount */
        if (curr->m_fs != NULL && curr->m_fs->fs_umount != NULL) {
            curr->m_fs->fs_umount(curr->m_sb);
        }
        
        /* Free mount structure */
        curr->m_refcount = 0;
        mount_free(curr);
        
        curr = next;
    }
    
    g_mount_list = NULL;
    g_root_mount = NULL;
    g_mount_count = 0;
    
    mount_unlock();
    
    log_info("[VFS:MOUNT] Shutdown selesai");
}
