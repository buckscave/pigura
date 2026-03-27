/*
 * PIGURA OS - INITRAMFS_EXTRACT.C
 * ==================================
 * Implementasi ekstraksi dan loading initramfs untuk kernel Pigura OS.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengekstrak arsip cpio
 * ke dalam initramfs filesystem saat boot.
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

/*
 * ===========================================================================
 * KONSTANTA INTERNAL (INTERNAL CONSTANTS)
 * ===========================================================================
 */

/* Maximum path depth during extraction */
#define EXTRACT_MAX_DEPTH    32

/* Buffer size untuk operasi */
#define EXTRACT_BUFFER_SIZE  4096

/* Maximum entries yang bisa diekstrak */
#define EXTRACT_MAX_ENTRIES  8192

/*
 * ===========================================================================
 * STRUKTUR STATISTIK EKSTRAKSI (EXTRACTION STATISTICS STRUCTURE)
 * ===========================================================================
 */

typedef struct {
    tak_bertanda32_t es_files;      /* Jumlah file */
    tak_bertanda32_t es_directories;/* Jumlah direktori */
    tak_bertanda32_t es_symlinks;   /* Jumlah symlink */
    tak_bertanda32_t es_devices;    /* Jumlah device files */
    tak_bertanda32_t es_pipes;      /* Jumlah pipes/fifos */
    tak_bertanda32_t es_sockets;    /* Jumlah sockets */
    tak_bertanda32_t es_errors;     /* Jumlah error */
    tak_bertanda64_t es_bytes;      /* Total bytes diekstrak */
    tak_bertanda64_t es_start_time; /* Waktu mulai */
    tak_bertanda64_t es_end_time;   /* Waktu selesai */
} extract_stats_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ===========================================================================
 */

/* Statistik ekstraksi */
static extract_stats_t g_extract_stats;

/* Lock untuk ekstraksi */
static tak_bertanda32_t g_extract_lock = 0;

/* Flag inisialisasi */
static bool_t g_extract_initialized = SALAH;

/*
 * ===========================================================================
 * FUNGSI INTERNAL LOCKING (INTERNAL LOCKING FUNCTIONS)
 * ===========================================================================
 */

static void extract_lock(void)
{
    g_extract_lock++;
}

static void extract_unlock(void)
{
    if (g_extract_lock > 0) {
        g_extract_lock--;
    }
}

/*
 * ===========================================================================
 * FUNGSI UTILITAS PATH (PATH UTILITY FUNCTIONS)
 * ===========================================================================
 */

/* Cari komponen terakhir dalam path */
static const char *extract_basename(const char *path)
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

/* Dapatkan direktori parent dari path */
static status_t extract_dirname(const char *path, char *buffer, 
                                 ukuran_t size)
{
    const char *last_slash;
    ukuran_t len;
    
    if (path == NULL || buffer == NULL || size == 0) {
        return STATUS_PARAM_NULL;
    }
    
    last_slash = kernel_strrchr(path, '/');
    
    if (last_slash == NULL || last_slash == path) {
        /* Root or no directory component */
        buffer[0] = '/';
        buffer[1] = '\0';
        return STATUS_BERHASIL;
    }
    
    len = (ukuran_t)(last_slash - path);
    if (len >= size) {
        return STATUS_PARAM_UKURAN;
    }
    
    kernel_strncpy(buffer, path, len);
    buffer[len] = '\0';
    
    return STATUS_BERHASIL;
}

/* Normalize path */
static status_t extract_normalize_path(const char *path, char *buffer,
                                        ukuran_t size)
{
    char temp[VFS_PATH_MAKS + 1];
    char *parts[64];
    tak_bertanda32_t part_count = 0;
    char *token;
    char *saveptr;
    ukuran_t i;
    ukuran_t pos;
    
    if (path == NULL || buffer == NULL || size == 0) {
        return STATUS_PARAM_NULL;
    }
    
    /* Copy path */
    kernel_strncpy(temp, path, VFS_PATH_MAKS);
    temp[VFS_PATH_MAKS] = '\0';
    
    /* Start with root */
    buffer[0] = '/';
    pos = 1;
    
    /* Parse components */
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
    for (i = 0; i < part_count; i++) {
        ukuran_t len = kernel_strlen(parts[i]);
        if (pos + len + 1 >= size) {
            return STATUS_PARAM_UKURAN;
        }
        
        if (pos > 1) {
            buffer[pos++] = '/';
        }
        
        kernel_strncpy(buffer + pos, parts[i], len);
        pos += len;
    }
    
    if (pos == 1) {
        buffer[1] = '\0';
    } else {
        buffer[pos] = '\0';
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PEMBUATAN DIREKTORI REKURSIF (RECURSIVE DIRECTORY CREATION)
 * ===========================================================================
 */

static status_t extract_mkdir_recursive(const char *path)
{
    char temp[VFS_PATH_MAKS + 1];
    char *token;
    char *saveptr;
    char curr_path[VFS_PATH_MAKS + 1];
    ukuran_t pos = 0;
    
    if (path == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Path harus absolut */
    if (path[0] != '/') {
        return STATUS_PARAM_INVALID;
    }
    
    /* Copy path */
    kernel_strncpy(temp, path, VFS_PATH_MAKS);
    temp[VFS_PATH_MAKS] = '\0';
    
    /* Start with root */
    curr_path[0] = '/';
    curr_path[1] = '\0';
    pos = 1;
    
    /* Parse dan buat setiap komponen */
    token = kernel_strtok_r(temp, "/", &saveptr);
    while (token != NULL) {
        ukuran_t len = kernel_strlen(token);
        
        if (pos + len + 2 >= VFS_PATH_MAKS) {
            return STATUS_PARAM_UKURAN;
        }
        
        if (pos > 1) {
            curr_path[pos++] = '/';
        }
        
        kernel_strncpy(curr_path + pos, token, len);
        pos += len;
        curr_path[pos] = '\0';
        
        /* Cek apakah direktori sudah ada */
        {
            initramfs_inode_t *inode;
            inode = initramfs_resolve_path(curr_path) ?
                    (initramfs_inode_t *)
                    ((initramfs_dentry_t*)
                     initramfs_resolve_path(curr_path))->d_inode : NULL;
            
            if (inode == NULL) {
                /* Buat direktori baru */
                initramfs_create_directory(curr_path, 0755, 0, 0);
            } else if (!VFS_S_ISDIR(inode->i_mode)) {
                return STATUS_BUKAN_DIR;
            }
        }
        
        token = kernel_strtok_r(NULL, "/", &saveptr);
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI EKSTRAKSI ENTRI (ENTRY EXTRACTION FUNCTIONS)
 * ===========================================================================
 */

static status_t extract_file(cpio_entry_t *cpio_entry,
                              const char *path)
{
    char dir_path[VFS_PATH_MAKS + 1];
    status_t ret;
    initramfs_inode_t *inode;
    
    if (cpio_entry == NULL || path == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Buat direktori parent jika belum ada */
    ret = extract_dirname(path, dir_path, sizeof(dir_path));
    if (ret != STATUS_BERHASIL) {
        return ret;
    }
    
    ret = extract_mkdir_recursive(dir_path);
    if (ret != STATUS_BERHASIL) {
        return ret;
    }
    
    /* Buat file */
    inode = initramfs_create_file(
        path,
        (mode_t)(cpio_entry->c_mode & 0777),
        cpio_entry->c_uid,
        cpio_entry->c_gid,
        cpio_entry->c_data,
        (off_t)cpio_entry->c_size
    );
    
    if (inode == NULL) {
        return STATUS_GAGAL;
    }
    
    g_extract_stats.es_files++;
    g_extract_stats.es_bytes += cpio_entry->c_size;
    
    return STATUS_BERHASIL;
}

static status_t extract_directory(cpio_entry_t *cpio_entry,
                                   const char *path)
{
    char normalized[VFS_PATH_MAKS + 1];
    status_t ret;
    initramfs_inode_t *inode;
    
    if (cpio_entry == NULL || path == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Normalize path */
    ret = extract_normalize_path(path, normalized, sizeof(normalized));
    if (ret != STATUS_BERHASIL) {
        return ret;
    }
    
    /* Buat direktori secara rekursif */
    ret = extract_mkdir_recursive(normalized);
    if (ret != STATUS_BERHASIL) {
        return ret;
    }
    
    /* Set permission jika berbeda */
    {
        initramfs_dentry_t *dentry = initramfs_resolve_path(normalized);
        if (dentry != NULL && dentry->d_inode != NULL) {
            inode = dentry->d_inode;
            inode->i_mode = VFS_S_IFDIR | (mode_t)(cpio_entry->c_mode & 0777);
            inode->i_uid = cpio_entry->c_uid;
            inode->i_gid = cpio_entry->c_gid;
        }
    }
    
    g_extract_stats.es_directories++;
    
    return STATUS_BERHASIL;
}

static status_t extract_symlink(cpio_entry_t *cpio_entry,
                                 const char *path)
{
    char dir_path[VFS_PATH_MAKS + 1];
    status_t ret;
    initramfs_inode_t *inode;
    const char *target;
    
    if (cpio_entry == NULL || path == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Target ada di data */
    if (cpio_entry->c_data == NULL || cpio_entry->c_size == 0) {
        return STATUS_PARAM_INVALID;
    }
    
    target = (const char *)cpio_entry->c_data;
    
    /* Buat direktori parent */
    ret = extract_dirname(path, dir_path, sizeof(dir_path));
    if (ret != STATUS_BERHASIL) {
        return ret;
    }
    
    ret = extract_mkdir_recursive(dir_path);
    if (ret != STATUS_BERHASIL) {
        return ret;
    }
    
    /* Buat symlink */
    inode = initramfs_create_symlink(
        path,
        target,
        cpio_entry->c_uid,
        cpio_entry->c_gid
    );
    
    if (inode == NULL) {
        return STATUS_GAGAL;
    }
    
    g_extract_stats.es_symlinks++;
    
    return STATUS_BERHASIL;
}

static status_t extract_device(cpio_entry_t *cpio_entry,
                                const char *path)
{
    char dir_path[VFS_PATH_MAKS + 1];
    status_t ret;
    
    if (cpio_entry == NULL || path == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Buat direktori parent */
    ret = extract_dirname(path, dir_path, sizeof(dir_path));
    if (ret != STATUS_BERHASIL) {
        return ret;
    }
    
    ret = extract_mkdir_recursive(dir_path);
    if (ret != STATUS_BERHASIL) {
        return ret;
    }
    
    /* Device files tidak di-support untuk initramfs */
    log_warn("[INITRAMFS] Device file %s not supported, skipping", path);
    
    g_extract_stats.es_devices++;
    
    return STATUS_BERHASIL;
}

static status_t extract_pipe(cpio_entry_t *cpio_entry, const char *path)
{
    /* Named pipes tidak di-support untuk initramfs */
    log_warn("[INITRAMFS] Named pipe %s not supported, skipping", path);
    
    g_extract_stats.es_pipes++;
    
    return STATUS_BERHASIL;
}

static status_t extract_socket(cpio_entry_t *cpio_entry, const char *path)
{
    /* Unix sockets tidak di-support untuk initramfs */
    log_warn("[INITRAMFS] Unix socket %s not supported, skipping", path);
    
    g_extract_stats.es_sockets++;
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI EKSTRAKSI UTAMA (MAIN EXTRACTION FUNCTIONS)
 * ===========================================================================
 */

static status_t extract_entry(cpio_entry_t *cpio_entry)
{
    const char *path;
    mode_t mode;
    status_t ret;
    
    if (cpio_entry == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    path = cpio_entry->c_name;
    
    /* Skip entries tanpa nama */
    if (path == NULL || path[0] == '\0') {
        return STATUS_BERHASIL;
    }
    
    /* Skip trailer */
    if (kernel_strcmp(path, "TRAILER!!!") == 0) {
        return STATUS_BERHASIL;
    }
    
    /* Konversi mode */
    mode = cpio_mode_to_vfs(cpio_entry->c_mode);
    
    /* Extract berdasarkan tipe */
    if (VFS_S_ISREG(mode)) {
        ret = extract_file(cpio_entry, path);
    } else if (VFS_S_ISDIR(mode)) {
        ret = extract_directory(cpio_entry, path);
    } else if (VFS_S_ISLNK(mode)) {
        ret = extract_symlink(cpio_entry, path);
    } else if (VFS_S_ISCHR(mode) || VFS_S_ISBLK(mode)) {
        ret = extract_device(cpio_entry, path);
    } else if (VFS_S_ISFIFO(mode)) {
        ret = extract_pipe(cpio_entry, path);
    } else if (VFS_S_ISSOCK(mode)) {
        ret = extract_socket(cpio_entry, path);
    } else {
        log_warn("[INITRAMFS] Unknown file type for %s, skipping", path);
        ret = STATUS_BERHASIL;
    }
    
    if (ret != STATUS_BERHASIL) {
        g_extract_stats.es_errors++;
        log_error("[INITRAMFS] Failed to extract %s: %d", path, ret);
    }
    
    return ret;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ===========================================================================
 */

status_t initramfs_extract(const void *buffer, ukuran_t size)
{
    cpio_entry_t *entries;
    cpio_entry_t *curr;
    tak_bertanda32_t count;
    status_t ret;
    tak_bertanda32_t i;
    
    if (buffer == NULL || size == 0) {
        return STATUS_PARAM_NULL;
    }
    
    log_info("[INITRAMFS] Starting extraction (%lu bytes)...",
             (unsigned long)size);
    
    extract_lock();
    
    /* Reset statistik */
    kernel_memset(&g_extract_stats, 0, sizeof(g_extract_stats));
    g_extract_stats.es_start_time = kernel_get_uptime();
    
    /* Parse cpio archive */
    ret = cpio_parse(buffer, size, &entries, &count);
    if (ret != STATUS_BERHASIL) {
        log_error("[INITRAMFS] Failed to parse cpio: %d", ret);
        extract_unlock();
        return ret;
    }
    
    log_info("[INITRAMFS] Found %u entries in cpio archive", count);
    
    /* Initialize initramfs */
    ret = initramfs_init(buffer, size);
    if (ret != STATUS_BERHASIL) {
        log_error("[INITRAMFS] Failed to init initramfs: %d", ret);
        cpio_free_entry(entries);
        extract_unlock();
        return ret;
    }
    
    /* Sort entries: directories first */
    /* TODO: Implement sorting untuk memastikan direktori dibuat dulu */
    
    /* First pass: buat semua direktori */
    curr = entries;
    while (curr != NULL) {
        if (CPIO_S_ISDIR(curr->c_mode)) {
            extract_entry(curr);
        }
        curr = curr->c_next;
    }
    
    /* Second pass: buat semua file dan entri lainnya */
    curr = entries;
    while (curr != NULL) {
        if (!CPIO_S_ISDIR(curr->c_mode)) {
            extract_entry(curr);
        }
        curr = curr->c_next;
    }
    
    /* Free cpio entries */
    cpio_free_entry(entries);
    
    g_extract_stats.es_end_time = kernel_get_uptime();
    
    /* Log statistik */
    log_info("[INITRAMFS] Extraction complete:");
    log_info("  Files:      %u", g_extract_stats.es_files);
    log_info("  Directories: %u", g_extract_stats.es_directories);
    log_info("  Symlinks:   %u", g_extract_stats.es_symlinks);
    log_info("  Devices:    %u", g_extract_stats.es_devices);
    log_info("  Pipes:      %u", g_extract_stats.es_pipes);
    log_info("  Sockets:    %u", g_extract_stats.es_sockets);
    log_info("  Errors:     %u", g_extract_stats.es_errors);
    log_info("  Total bytes: %llu", g_extract_stats.es_bytes);
    
    extract_unlock();
    
    return STATUS_BERHASIL;
}

status_t initramfs_load_from_module(tak_bertanda32_t module_index)
{
    alamat_fisik_t start;
    alamat_fisik_t end;
    void *buffer;
    ukuran_t size;
    status_t ret;
    
    log_info("[INITRAMFS] Loading from module %u...", module_index);
    
    /* Dapatkan info module dari multiboot */
    ret = multiboot_get_module(module_index, &start, &end);
    if (ret != STATUS_BERHASIL) {
        log_error("[INITRAMFS] Module %u not found", module_index);
        return ret;
    }
    
    /* Hitung ukuran */
    size = (ukuran_t)(end - start);
    if (size == 0) {
        log_error("[INITRAMFS] Module %u is empty", module_index);
        return STATUS_PARAM_UKURAN;
    }
    
    /* Buffer sudah di memori */
    buffer = (void *)(uintptr_t)start;
    
    /* Extract */
    ret = initramfs_extract(buffer, size);
    if (ret != STATUS_BERHASIL) {
        log_error("[INITRAMFS] Failed to extract module %u", module_index);
        return ret;
    }
    
    log_info("[INITRAMFS] Module %u loaded successfully", module_index);
    
    return STATUS_BERHASIL;
}

status_t initramfs_load_from_address(const void *address, ukuran_t size)
{
    status_t ret;
    
    if (address == NULL || size == 0) {
        return STATUS_PARAM_NULL;
    }
    
    log_info("[INITRAMFS] Loading from address 0x%p (%lu bytes)...",
             address, (unsigned long)size);
    
    ret = initramfs_extract(address, size);
    if (ret != STATUS_BERHASIL) {
        log_error("[INITRAMFS] Failed to load from address");
        return ret;
    }
    
    log_info("[INITRAMFS] Load from address successful");
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI MOUNT (MOUNT FUNCTIONS)
 * ===========================================================================
 */

status_t initramfs_mount_root(void)
{
    status_t ret;
    
    log_info("[INITRAMFS] Mounting as root filesystem...");
    
    /* Register filesystem */
    ret = initramfs_register();
    if (ret != STATUS_BERHASIL) {
        log_error("[INITRAMFS] Failed to register filesystem");
        return ret;
    }
    
    /* Mount sebagai root */
    ret = mount_create(NULL, "/", "initramfs", 0);
    if (ret != STATUS_BERHASIL) {
        log_error("[INITRAMFS] Failed to mount root: %d", ret);
        return ret;
    }
    
    log_info("[INITRAMFS] Root filesystem mounted successfully");
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI STATISTIK (STATISTICS FUNCTIONS)
 * ===========================================================================
 */

void initramfs_get_extract_stats(tak_bertanda32_t *files,
                                  tak_bertanda32_t *dirs,
                                  tak_bertanda32_t *symlinks,
                                  tak_bertanda64_t *bytes)
{
    if (files != NULL) {
        *files = g_extract_stats.es_files;
    }
    if (dirs != NULL) {
        *dirs = g_extract_stats.es_directories;
    }
    if (symlinks != NULL) {
        *symlinks = g_extract_stats.es_symlinks;
    }
    if (bytes != NULL) {
        *bytes = g_extract_stats.es_bytes;
    }
}

tak_bertanda32_t initramfs_get_extract_errors(void)
{
    return g_extract_stats.es_errors;
}

/*
 * ===========================================================================
 * FUNGSI VALIDASI (VALIDATION FUNCTIONS)
 * ===========================================================================
 */

bool_t initramfs_validate_cpio(const void *buffer, ukuran_t size)
{
    tak_bertanda32_t format;
    
    if (buffer == NULL || size < CPIO_MAGIC_LEN) {
        return SALAH;
    }
    
    format = cpio_detect_format(buffer, size);
    
    return (format != CPIO_FORMAT_UNKNOWN) ? BENAR : SALAH;
}

bool_t initramfs_is_loaded(void)
{
    return (g_extract_initialized && 
            g_extract_stats.es_files > 0 ||
            g_extract_stats.es_directories > 0) ? BENAR : SALAH;
}

/*
 * ===========================================================================
 * FUNGSI INFORMASI DAN DEBUG (INFORMATION AND DEBUG FUNCTIONS)
 * ===========================================================================
 */

void initramfs_print_extract_stats(void)
{
    tak_bertanda64_t duration;
    
    kernel_printf("[INITRAMFS] Extraction Statistics:\n");
    kernel_printf("  Files:       %u\n", g_extract_stats.es_files);
    kernel_printf("  Directories: %u\n", g_extract_stats.es_directories);
    kernel_printf("  Symlinks:    %u\n", g_extract_stats.es_symlinks);
    kernel_printf("  Devices:     %u\n", g_extract_stats.es_devices);
    kernel_printf("  Pipes:       %u\n", g_extract_stats.es_pipes);
    kernel_printf("  Sockets:     %u\n", g_extract_stats.es_sockets);
    kernel_printf("  Errors:      %u\n", g_extract_stats.es_errors);
    kernel_printf("  Total bytes: %llu\n", g_extract_stats.es_bytes);
    
    if (g_extract_stats.es_end_time >= g_extract_stats.es_start_time) {
        duration = g_extract_stats.es_end_time - g_extract_stats.es_start_time;
        kernel_printf("  Duration:    %llu ticks\n", duration);
    }
}

void initramfs_print_extract_list(void)
{
    /* TODO: Implement list printing */
    kernel_printf("[INITRAMFS] Entry list not implemented yet\n");
}

/*
 * ===========================================================================
 * FUNGSI INITIALIZATION MODULE (MODULE INITIALIZATION)
 * ===========================================================================
 */

status_t initramfs_extract_module_init(void)
{
    log_info("[INITRAMFS_EXTRACT] Initializing extraction module...");
    
    /* Reset statistik */
    kernel_memset(&g_extract_stats, 0, sizeof(g_extract_stats));
    
    /* Reset lock */
    g_extract_lock = 0;
    
    /* Set flag */
    g_extract_initialized = BENAR;
    
    log_info("[INITRAMFS_EXTRACT] Module initialized");
    
    return STATUS_BERHASIL;
}

void initramfs_extract_module_shutdown(void)
{
    log_info("[INITRAMFS_EXTRACT] Shutting down extraction module...");
    
    extract_lock();
    
    /* Clear statistik */
    kernel_memset(&g_extract_stats, 0, sizeof(g_extract_stats));
    
    g_extract_initialized = SALAH;
    
    extract_unlock();
    
    log_info("[INITRAMFS_EXTRACT] Module shutdown complete");
}

/*
 * ===========================================================================
 * FUNGSI UTILITY TAMBAHAN (ADDITIONAL UTILITY FUNCTIONS)
 * ===========================================================================
 */

/* Cari file dalam initramfs */
const void *initramfs_find_file(const char *path, ukuran_t *size)
{
    initramfs_dentry_t *dentry;
    initramfs_inode_t *inode;
    
    if (path == NULL) {
        return NULL;
    }
    
    dentry = initramfs_resolve_path(path);
    if (dentry == NULL) {
        return NULL;
    }
    
    inode = dentry->d_inode;
    if (inode == NULL || !VFS_S_ISREG(inode->i_mode)) {
        return NULL;
    }
    
    if (size != NULL) {
        *size = (ukuran_t)inode->i_size;
    }
    
    return inode->i_data;
}

/* Baca file dari initramfs */
tak_bertandas_t initramfs_read_file(const char *path, void *buffer,
                                     ukuran_t size, ukuran_t offset)
{
    const void *data;
    ukuran_t file_size;
    ukuran_t to_read;
    
    if (path == NULL || buffer == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_NULL;
    }
    
    data = initramfs_find_file(path, &file_size);
    if (data == NULL) {
        return (tak_bertandas_t)STATUS_TIDAK_DITEMUKAN;
    }
    
    if (offset >= file_size) {
        return 0;
    }
    
    to_read = file_size - offset;
    if (to_read > size) {
        to_read = size;
    }
    
    kernel_memcpy(buffer, (const void *)((uintptr_t)data + offset), to_read);
    
    return (tak_bertandas_t)to_read;
}

/* Cek apakah path ada dalam initramfs */
bool_t initramfs_path_exists(const char *path)
{
    initramfs_dentry_t *dentry;
    
    if (path == NULL) {
        return SALAH;
    }
    
    dentry = initramfs_resolve_path(path);
    
    return (dentry != NULL) ? BENAR : SALAH;
}

/* Cek apakah path adalah direktori */
bool_t initramfs_is_directory(const char *path)
{
    initramfs_dentry_t *dentry;
    initramfs_inode_t *inode;
    
    if (path == NULL) {
        return SALAH;
    }
    
    dentry = initramfs_resolve_path(path);
    if (dentry == NULL) {
        return SALAH;
    }
    
    inode = dentry->d_inode;
    if (inode == NULL) {
        return SALAH;
    }
    
    return VFS_S_ISDIR(inode->i_mode) ? BENAR : SALAH;
}

/* List isi direktori */
status_t initramfs_list_directory(const char *path,
                                   void (*callback)(const char *name,
                                                    mode_t mode,
                                                    off_t size,
                                                    void *context),
                                   void *context)
{
    initramfs_dentry_t *dentry;
    initramfs_dentry_t *curr;
    initramfs_inode_t *dir_inode;
    
    if (path == NULL || callback == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    dentry = initramfs_resolve_path(path);
    if (dentry == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    dir_inode = dentry->d_inode;
    if (dir_inode == NULL || !VFS_S_ISDIR(dir_inode->i_mode)) {
        return STATUS_BUKAN_DIR;
    }
    
    curr = dir_inode->i_children;
    while (curr != NULL) {
        if (curr->d_inode != NULL) {
            callback(curr->d_name,
                     curr->d_inode->i_mode,
                     curr->d_inode->i_size,
                     context);
        }
        curr = curr->d_next;
    }
    
    return STATUS_BERHASIL;
}
