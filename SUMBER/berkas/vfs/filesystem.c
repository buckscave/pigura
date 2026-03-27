/*
 * PIGURA OS - FILESYSTEM.C
 * =========================
 * Implementasi filesystem registration untuk VFS Pigura OS.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola registrasi
 * filesystem dan deteksi filesystem otomatis.
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

/* Jumlah maksimum filesystem terdaftar */
#define FS_MAX                  VFS_FS_MAKS

/* Panjang nama filesystem maksimum */
#define FS_NAMA_MAX             32

/*
 * ===========================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ===========================================================================
 */

/* Filesystem list */
static filesystem_t *g_fs_list = NULL;
static tak_bertanda32_t g_fs_count = 0;

/* Lock untuk operasi filesystem */
static tak_bertanda32_t g_fs_lock = 0;

/* Flag inisialisasi */
static bool_t g_fs_initialized = SALAH;

/*
 * ===========================================================================
 * FUNGSI INTERNAL LOCKING (INTERNAL LOCKING FUNCTIONS)
 * ===========================================================================
 */

static void fs_lock(void)
{
    g_fs_lock++;
}

static void fs_unlock(void)
{
    if (g_fs_lock > 0) {
        g_fs_lock--;
    }
}

/*
 * ===========================================================================
 * FUNGSI REGISTRASI (REGISTRATION FUNCTIONS)
 * ===========================================================================
 */

status_t filesystem_register(filesystem_t *fs)
{
    filesystem_t *curr;
    
    if (fs == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Validasi nama filesystem */
    if (fs->fs_nama[0] == '\0') {
        log_error("[VFS:FS] Nama filesystem kosong");
        return STATUS_PARAM_INVALID;
    }
    
    /* Validasi panjang nama */
    if (kernel_strlen(fs->fs_nama) >= FS_NAMA_MAX) {
        log_error("[VFS:FS] Nama filesystem terlalu panjang: %s",
                  fs->fs_nama);
        return STATUS_PARAM_UKURAN;
    }
    
    /* Validasi minimal ada mount function */
    if (fs->fs_mount == NULL) {
        log_error("[VFS:FS] Filesystem tidak memiliki mount function: %s",
                  fs->fs_nama);
        return STATUS_PARAM_INVALID;
    }
    
    fs_lock();
    
    /* Cek apakah sudah terdaftar */
    curr = g_fs_list;
    while (curr != NULL) {
        if (kernel_strcmp(curr->fs_nama, fs->fs_nama) == 0) {
            fs_unlock();
            log_warn("[VFS:FS] Filesystem sudah terdaftar: %s", fs->fs_nama);
            return STATUS_SUDAH_ADA;
        }
        curr = curr->fs_next;
    }
    
    /* Cek limit */
    if (g_fs_count >= FS_MAX) {
        fs_unlock();
        log_error("[VFS:FS] Jumlah filesystem maksimum tercapai");
        return STATUS_PENUH;
    }
    
    /* Init fields */
    fs->fs_next = NULL;
    fs->fs_refcount = 0;
    fs->fs_terdaftar = BENAR;
    
    /* Add ke list */
    fs->fs_next = g_fs_list;
    g_fs_list = fs;
    g_fs_count++;
    
    fs_unlock();
    
    log_info("[VFS:FS] Filesystem terdaftar: %s", fs->fs_nama);
    
    return STATUS_BERHASIL;
}

status_t filesystem_unregister(filesystem_t *fs)
{
    filesystem_t *curr;
    filesystem_t *prev;
    
    if (fs == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    fs_lock();
    
    /* Cari di list */
    prev = NULL;
    curr = g_fs_list;
    
    while (curr != NULL) {
        if (curr == fs) {
            /* Cek reference count */
            if (fs->fs_refcount > 0) {
                fs_unlock();
                log_warn("[VFS:FS] Filesystem masih digunakan: %s (ref=%u)",
                         fs->fs_nama, fs->fs_refcount);
                return STATUS_BUSY;
            }
            
            /* Remove dari list */
            if (prev == NULL) {
                g_fs_list = fs->fs_next;
            } else {
                prev->fs_next = fs->fs_next;
            }
            
            fs->fs_next = NULL;
            fs->fs_terdaftar = SALAH;
            g_fs_count--;
            
            fs_unlock();
            
            log_info("[VFS:FS] Filesystem tidak terdaftar: %s", fs->fs_nama);
            
            return STATUS_BERHASIL;
        }
        prev = curr;
        curr = curr->fs_next;
    }
    
    fs_unlock();
    
    log_warn("[VFS:FS] Filesystem tidak ditemukan: %s", fs->fs_nama);
    
    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * ===========================================================================
 * FUNGSI LOOKUP (LOOKUP FUNCTIONS)
 * ===========================================================================
 */

filesystem_t *filesystem_find(const char *nama)
{
    filesystem_t *curr;
    
    if (nama == NULL || nama[0] == '\0') {
        return NULL;
    }
    
    fs_lock();
    
    curr = g_fs_list;
    while (curr != NULL) {
        if (kernel_strcmp(curr->fs_nama, nama) == 0) {
            fs_unlock();
            return curr;
        }
        curr = curr->fs_next;
    }
    
    fs_unlock();
    
    return NULL;
}

/*
 * ===========================================================================
 * FUNGSI DETEKSI (DETECTION FUNCTIONS)
 * ===========================================================================
 */

filesystem_t *filesystem_detect(const char *device)
{
    filesystem_t *curr;
    filesystem_t *detected;
    status_t ret;
    
    if (device == NULL) {
        return NULL;
    }
    
    detected = NULL;
    
    fs_lock();
    
    /* Iterasi semua filesystem yang punya detect function */
    curr = g_fs_list;
    while (curr != NULL) {
        if (curr->fs_detect != NULL) {
            ret = curr->fs_detect(device);
            if (ret == STATUS_BERHASIL) {
                detected = curr;
                break;
            }
        }
        curr = curr->fs_next;
    }
    
    fs_unlock();
    
    if (detected != NULL) {
        log_info("[VFS:FS] Filesystem terdeteksi: %s pada %s",
                 detected->fs_nama, device);
    }
    
    return detected;
}

/*
 * ===========================================================================
 * FUNGSI ITERASI (ITERATION FUNCTIONS)
 * ===========================================================================
 */

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
 * FUNGSI MOUNT HELPER (MOUNT HELPER FUNCTIONS)
 * ===========================================================================
 */

superblock_t *filesystem_mount(filesystem_t *fs, const char *device,
                               const char *path, tak_bertanda32_t flags)
{
    superblock_t *sb;
    
    if (fs == NULL || path == NULL) {
        return NULL;
    }
    
    /* Cek mount function */
    if (fs->fs_mount == NULL) {
        log_error("[VFS:FS] Filesystem tidak mendukung mount: %s",
                  fs->fs_nama);
        return NULL;
    }
    
    /* Panggil mount function */
    sb = fs->fs_mount(fs, device, path, flags);
    if (sb == NULL) {
        log_error("[VFS:FS] Gagal mount filesystem: %s", fs->fs_nama);
        return NULL;
    }
    
    /* Setup superblock */
    if (sb->s_fs == NULL) {
        sb->s_fs = fs;
    }
    
    /* Reference filesystem */
    fs->fs_refcount++;
    
    log_info("[VFS:FS] Filesystem mounted: %s at %s",
             fs->fs_nama, path);
    
    return sb;
}

status_t filesystem_umount(filesystem_t *fs, superblock_t *sb)
{
    status_t ret;
    
    if (fs == NULL || sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Cek umount function */
    if (fs->fs_umount == NULL) {
        log_warn("[VFS:FS] Filesystem tidak mendukung umount: %s",
                 fs->fs_nama);
        return STATUS_TIDAK_DUKUNG;
    }
    
    /* Panggil umount function */
    ret = fs->fs_umount(sb);
    if (ret != STATUS_BERHASIL) {
        log_error("[VFS:FS] Gagal umount filesystem: %s", fs->fs_nama);
        return ret;
    }
    
    /* Dereference filesystem */
    if (fs->fs_refcount > 0) {
        fs->fs_refcount--;
    }
    
    log_info("[VFS:FS] Filesystem unmounted: %s", fs->fs_nama);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI UTILITAS (UTILITY FUNCTIONS)
 * ===========================================================================
 */

bool_t filesystem_is_registered(filesystem_t *fs)
{
    filesystem_t *curr;
    
    if (fs == NULL) {
        return SALAH;
    }
    
    fs_lock();
    
    curr = g_fs_list;
    while (curr != NULL) {
        if (curr == fs) {
            fs_unlock();
            return BENAR;
        }
        curr = curr->fs_next;
    }
    
    fs_unlock();
    
    return SALAH;
}

bool_t filesystem_requires_device(filesystem_t *fs)
{
    if (fs == NULL) {
        return SALAH;
    }
    
    return (fs->fs_flags & FS_FLAG_REQUIRES_DEV) != 0;
}

bool_t filesystem_is_readonly(filesystem_t *fs)
{
    if (fs == NULL) {
        return BENAR;
    }
    
    return (fs->fs_flags & FS_FLAG_READ_ONLY) != 0;
}

bool_t filesystem_supports_write(filesystem_t *fs)
{
    if (fs == NULL) {
        return SALAH;
    }
    
    return (fs->fs_flags & FS_FLAG_WRITE_SUPPORT) != 0;
}

tak_bertanda32_t filesystem_get_refcount(filesystem_t *fs)
{
    if (fs == NULL) {
        return 0;
    }
    
    return fs->fs_refcount;
}

const char *filesystem_get_name(filesystem_t *fs)
{
    if (fs == NULL) {
        return NULL;
    }
    
    return fs->fs_nama;
}

/*
 * ===========================================================================
 * FUNGSI STATISTIK (STATISTICS FUNCTIONS)
 * ===========================================================================
 */

tak_bertanda32_t filesystem_get_count(void)
{
    return g_fs_count;
}

/*
 * ===========================================================================
 * FUNGSI INFORMASI DAN DEBUG (INFORMATION AND DEBUG FUNCTIONS)
 * ===========================================================================
 */

void filesystem_print_info(filesystem_t *fs)
{
    if (fs == NULL) {
        kernel_printf("[VFS:FS] Filesystem: NULL\n");
        return;
    }
    
    kernel_printf("[VFS:FS] Filesystem Info:\n");
    kernel_printf("  Nama: %s\n", fs->fs_nama);
    kernel_printf("  Flags: 0x%08X\n", fs->fs_flags);
    kernel_printf("  Refcount: %u\n", fs->fs_refcount);
    kernel_printf("  Terdaftar: %s\n", fs->fs_terdaftar ? "ya" : "tidak");
    
    kernel_printf("  Flags Detail:\n");
    if (fs->fs_flags & FS_FLAG_REQUIRES_DEV) {
        kernel_printf("    - Membutuhkan device\n");
    }
    if (fs->fs_flags & FS_FLAG_NO_DEV) {
        kernel_printf("    - Virtual filesystem\n");
    }
    if (fs->fs_flags & FS_FLAG_READ_ONLY) {
        kernel_printf("    - Read-only\n");
    }
    if (fs->fs_flags & FS_FLAG_WRITE_SUPPORT) {
        kernel_printf("    - Mendukung tulis\n");
    }
    
    kernel_printf("  Functions:\n");
    kernel_printf("    mount:   %s\n", fs->fs_mount ? "ada" : "tidak");
    kernel_printf("    umount:  %s\n", fs->fs_umount ? "ada" : "tidak");
    kernel_printf("    detect:  %s\n", fs->fs_detect ? "ada" : "tidak");
}

void filesystem_print_list(void)
{
    filesystem_t *curr;
    tak_bertanda32_t count = 0;
    
    kernel_printf("[VFS:FS] Filesystem List:\n");
    
    fs_lock();
    
    curr = g_fs_list;
    while (curr != NULL) {
        kernel_printf("  %s (ref=%u, flags=0x%08X)\n",
                      curr->fs_nama,
                      curr->fs_refcount,
                      curr->fs_flags);
        count++;
        curr = curr->fs_next;
    }
    
    fs_unlock();
    
    kernel_printf("  Total: %u filesystem\n", count);
}

/*
 * ===========================================================================
 * FUNGSI INITIALIZATION MODULE (MODULE INITIALIZATION)
 * ===========================================================================
 */

status_t filesystem_module_init(void)
{
    log_info("[VFS:FS] Menginisialisasi modul filesystem...");
    
    /* Clear filesystem list */
    g_fs_list = NULL;
    g_fs_count = 0;
    
    /* Reset lock */
    g_fs_lock = 0;
    
    g_fs_initialized = BENAR;
    
    log_info("[VFS:FS] Inisialisasi selesai");
    
    return STATUS_BERHASIL;
}

void filesystem_module_shutdown(void)
{
    filesystem_t *curr;
    filesystem_t *next;
    
    log_info("[VFS:FS] Mematikan modul filesystem...");
    
    fs_lock();
    
    /* Unregister semua filesystem */
    curr = g_fs_list;
    while (curr != NULL) {
        next = curr->fs_next;
        
        /* Cek reference */
        if (curr->fs_refcount > 0) {
            log_warn("[VFS:FS] Filesystem masih digunakan: %s (ref=%u)",
                     curr->fs_nama, curr->fs_refcount);
        }
        
        curr->fs_terdaftar = SALAH;
        curr->fs_next = NULL;
        
        curr = next;
    }
    
    g_fs_list = NULL;
    g_fs_count = 0;
    g_fs_initialized = SALAH;
    
    fs_unlock();
    
    log_info("[VFS:FS] Shutdown selesai");
}
