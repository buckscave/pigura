/*
 * PIGURA OS - FILE.C
 * ===================
 * Implementasi manajemen file untuk VFS Pigura OS.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola file,
 * termasuk alokasi, dealokasi, dan operasi-operasi I/O.
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

/* Jumlah maksimum file dalam cache */
#define FILE_CACHE_MAX          256

/* Jumlah maksimum file descriptor */
#define FD_MAX                  VFS_FD_MAKS

/* Nilai FD invalid */
#define FD_INVALID              (-1)

/* Flag internal file */
#define F_FLAG_CACHED           0x00010000

/*
 * ===========================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ===========================================================================
 */

/* File descriptor table */
static file_t *g_fd_table[FD_MAX];
static tak_bertanda32_t g_fd_count = 0;

/* File cache */
static file_t *g_file_cache = NULL;
static tak_bertanda32_t g_file_cached = 0;

/* Lock untuk operasi file */
static tak_bertanda32_t g_file_lock = 0;

/*
 * ===========================================================================
 * FUNGSI INTERNAL LOCKING (INTERNAL LOCKING FUNCTIONS)
 * ===========================================================================
 */

static void file_lock(void)
{
    g_file_lock++;
}

static void file_unlock(void)
{
    if (g_file_lock > 0) {
        g_file_lock--;
    }
}

/*
 * ===========================================================================
 * FUNGSI FILE DESCRIPTOR (FILE DESCRIPTOR FUNCTIONS)
 * ===========================================================================
 */

static int fd_find_free(void)
{
    int fd;
    
    file_lock();
    
    /* Cari slot kosong, mulai dari 3 (0,1,2 reserved) */
    for (fd = 3; fd < FD_MAX; fd++) {
        if (g_fd_table[fd] == NULL) {
            file_unlock();
            return fd;
        }
    }
    
    file_unlock();
    
    return FD_INVALID;
}

static bool_t fd_valid(int fd)
{
    if (fd < 0 || fd >= FD_MAX) {
        return SALAH;
    }
    return BENAR;
}

/*
 * ===========================================================================
 * FUNGSI ALOKASI DAN DEALOKASI (ALLOCATION AND DEALLOCATION FUNCTIONS)
 * ===========================================================================
 */

file_t *file_alloc(void)
{
    file_t *file;
    
    /* Alokasi memori */
    file = (file_t *)kmalloc(sizeof(file_t));
    if (file == NULL) {
        log_error("[VFS:FILE] Gagal alokasi memori");
        return NULL;
    }
    
    /* Clear dan init */
    kernel_memset(file, 0, sizeof(file_t));
    
    file->f_magic = VFS_FILE_MAGIC;
    file->f_refcount = 1;
    file->f_pos = 0;
    file->f_readable = SALAH;
    file->f_writable = SALAH;
    file->f_append = SALAH;
    
    log_debug("[VFS:FILE] File structure dialokasi");
    
    return file;
}

void file_free(file_t *file)
{
    if (file == NULL) {
        return;
    }
    
    /* Validasi magic */
    if (file->f_magic != VFS_FILE_MAGIC) {
        log_warn("[VFS:FILE] Magic invalid saat free");
        return;
    }
    
    /* Jangan free jika masih ada reference */
    if (file->f_refcount > 0) {
        log_debug("[VFS:FILE] File masih di-reference: %u",
                  file->f_refcount);
        return;
    }
    
    /* Panggil release callback */
    if (file->f_op != NULL && file->f_op->release != NULL) {
        if (file->f_inode != NULL) {
            file->f_op->release(file->f_inode, file);
        }
    }
    
    /* Release inode reference */
    if (file->f_inode != NULL) {
        inode_put(file->f_inode);
    }
    
    /* Release dentry reference */
    if (file->f_dentry != NULL) {
        dentry_put(file->f_dentry);
    }
    
    /* Release mount reference */
    if (file->f_mount != NULL) {
        mount_put(file->f_mount);
    }
    
    /* Clear dan free */
    kernel_memset(file, 0, sizeof(file_t));
    kfree(file);
    
    log_debug("[VFS:FILE] File structure dibebaskan");
}

/*
 * ===========================================================================
 * FUNGSI INISIALISASI (INITIALIZATION FUNCTIONS)
 * ===========================================================================
 */

status_t file_init(file_t *file, inode_t *inode, dentry_t *dentry,
                   tak_bertanda32_t flags)
{
    if (file == NULL || inode == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Set basic fields */
    file->f_inode = inode;
    file->f_dentry = dentry;
    file->f_flags = flags;
    file->f_pos = 0;
    
    /* Set mode dari flags */
    file->f_mode = (mode_t)(flags & VFS_OPEN_RDWR);
    
    /* Set permission flags */
    file->f_readable = (file->f_mode & VFS_OPEN_RDONLY) != 0;
    file->f_writable = (file->f_mode & VFS_OPEN_WRONLY) != 0;
    file->f_append = (flags & VFS_OPEN_APPEND) != 0;
    
    /* Set file operations dari inode */
    if (inode->i_fop != NULL) {
        file->f_op = inode->i_fop;
    }
    
    /* Reference inode dan dentry */
    inode_get(inode);
    if (dentry != NULL) {
        dentry_get(dentry);
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI REFERENCE COUNTING (REFERENCE COUNTING FUNCTIONS)
 * ===========================================================================
 */

void file_get(file_t *file)
{
    if (file == NULL) {
        return;
    }
    
    if (file->f_magic != VFS_FILE_MAGIC) {
        return;
    }
    
    file_lock();
    file->f_refcount++;
    file_unlock();
}

void file_put(file_t *file)
{
    if (file == NULL) {
        return;
    }
    
    if (file->f_magic != VFS_FILE_MAGIC) {
        return;
    }
    
    file_lock();
    
    if (file->f_refcount > 0) {
        file->f_refcount--;
        
        if (file->f_refcount == 0) {
            file_unlock();
            file_free(file);
            return;
        }
    }
    
    file_unlock();
}

/*
 * ===========================================================================
 * FUNGSI I/O (I/O FUNCTIONS)
 * ===========================================================================
 */

tak_bertandas_t file_read(file_t *file, void *buffer, ukuran_t size)
{
    tak_bertandas_t bytes_read;
    off_t pos;
    
    if (file == NULL || buffer == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_NULL;
    }
    
    if (file->f_magic != VFS_FILE_MAGIC) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }
    
    /* Cek permission */
    if (!file->f_readable) {
        return (tak_bertandas_t)STATUS_AKSES_DITOLAK;
    }
    
    /* Cek file operations */
    if (file->f_op == NULL || file->f_op->read == NULL) {
        return (tak_bertandas_t)STATUS_TIDAK_DUKUNG;
    }
    
    /* Baca dengan current position */
    pos = file->f_pos;
    bytes_read = file->f_op->read(file, buffer, size, &pos);
    
    /* Update position jika berhasil */
    if (bytes_read > 0) {
        file->f_pos = pos;
        
        /* Update access time */
        if (file->f_inode != NULL) {
            file->f_inode->i_atime = kernel_get_uptime();
        }
    }
    
    return bytes_read;
}

tak_bertandas_t file_write(file_t *file, const void *buffer,
                           ukuran_t size)
{
    tak_bertandas_t bytes_written;
    off_t pos;
    
    if (file == NULL || buffer == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_NULL;
    }
    
    if (file->f_magic != VFS_FILE_MAGIC) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }
    
    /* Cek permission */
    if (!file->f_writable) {
        return (tak_bertandas_t)STATUS_AKSES_DITOLAK;
    }
    
    /* Cek file operations */
    if (file->f_op == NULL || file->f_op->write == NULL) {
        return (tak_bertandas_t)STATUS_TIDAK_DUKUNG;
    }
    
    /* Set position untuk append mode */
    if (file->f_append && file->f_inode != NULL) {
        file->f_pos = file->f_inode->i_size;
    }
    
    /* Tulis dengan current position */
    pos = file->f_pos;
    bytes_written = file->f_op->write(file, buffer, size, &pos);
    
    /* Update position jika berhasil */
    if (bytes_written > 0) {
        file->f_pos = pos;
        
        /* Update inode */
        if (file->f_inode != NULL) {
            file->f_inode->i_mtime = kernel_get_uptime();
            file->f_inode->i_dirty = BENAR;
            
            /* Update size jika file membesar */
            if (file->f_pos > file->f_inode->i_size) {
                file->f_inode->i_size = file->f_pos;
            }
        }
    }
    
    return bytes_written;
}

off_t file_lseek(file_t *file, off_t offset, tak_bertanda32_t whence)
{
    off_t new_pos;
    off_t size;
    
    if (file == NULL) {
        return (off_t)STATUS_PARAM_NULL;
    }
    
    if (file->f_magic != VFS_FILE_MAGIC) {
        return (off_t)STATUS_PARAM_INVALID;
    }
    
    /* Dapatkan ukuran file */
    size = 0;
    if (file->f_inode != NULL) {
        size = file->f_inode->i_size;
    }
    
    /* Hitung posisi baru */
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
    
    /* Validasi posisi */
    if (new_pos < 0) {
        return (off_t)STATUS_PARAM_INVALID;
    }
    
    /* Gunakan lseek operation jika ada */
    if (file->f_op != NULL && file->f_op->lseek != NULL) {
        new_pos = file->f_op->lseek(file, offset, whence);
        if (new_pos < 0) {
            return new_pos;
        }
    }
    
    file->f_pos = new_pos;
    
    return new_pos;
}

/*
 * ===========================================================================
 * FUNGSI FLUSH DAN SYNC (FLUSH AND SYNC FUNCTIONS)
 * ===========================================================================
 */

status_t file_flush(file_t *file)
{
    if (file == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (file->f_magic != VFS_FILE_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Gunakan flush operation jika ada */
    if (file->f_op != NULL && file->f_op->flush != NULL) {
        return file->f_op->flush(file);
    }
    
    return STATUS_BERHASIL;
}

status_t file_fsync(file_t *file)
{
    if (file == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (file->f_magic != VFS_FILE_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Gunakan fsync operation jika ada */
    if (file->f_op != NULL && file->f_op->fsync != NULL) {
        return file->f_op->fsync(file);
    }
    
    /* Sync inode secara manual */
    if (file->f_inode != NULL && file->f_inode->i_dirty) {
        return inode_sync(file->f_inode);
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI DIREKTORI (DIRECTORY FUNCTIONS)
 * ===========================================================================
 */

tak_bertandas_t file_readdir(file_t *file, vfs_dirent_t *dirent,
                             ukuran_t count)
{
    tak_bertandas_t ret;
    
    if (file == NULL || dirent == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_NULL;
    }
    
    if (file->f_magic != VFS_FILE_MAGIC) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }
    
    /* Harus direktori */
    if (file->f_inode == NULL || !VFS_S_ISDIR(file->f_inode->i_mode)) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }
    
    /* Cek file operations */
    if (file->f_op == NULL || file->f_op->readdir == NULL) {
        return (tak_bertandas_t)STATUS_TIDAK_DUKUNG;
    }
    
    ret = file->f_op->readdir(file, dirent, count);
    
    /* Update access time */
    if (ret > 0 && file->f_inode != NULL) {
        file->f_inode->i_atime = kernel_get_uptime();
    }
    
    return ret;
}

/*
 * ===========================================================================
 * FUNGSI OPEN DAN RELEASE (OPEN AND RELEASE FUNCTIONS)
 * ===========================================================================
 */

status_t file_open(inode_t *inode, file_t *file)
{
    status_t ret;
    
    if (inode == NULL || file == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Panggil open operation jika ada */
    if (file->f_op != NULL && file->f_op->open != NULL) {
        ret = file->f_op->open(inode, file);
        if (ret != STATUS_BERHASIL) {
            return ret;
        }
    }
    
    /* Update access time */
    inode->i_atime = kernel_get_uptime();
    
    return STATUS_BERHASIL;
}

status_t file_release(file_t *file)
{
    if (file == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (file->f_magic != VFS_FILE_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Flush jika writable */
    if (file->f_writable) {
        file_flush(file);
    }
    
    /* Panggil release operation jika ada */
    if (file->f_op != NULL && file->f_op->release != NULL) {
        if (file->f_inode != NULL) {
            return file->f_op->release(file->f_inode, file);
        }
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI IOCTL (IOCTL FUNCTION)
 * ===========================================================================
 */

status_t file_ioctl(file_t *file, tak_bertanda32_t cmd,
                    tak_bertanda64_t arg)
{
    if (file == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (file->f_magic != VFS_FILE_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Gunakan ioctl operation jika ada */
    if (file->f_op != NULL && file->f_op->ioctl != NULL) {
        return file->f_op->ioctl(file, cmd, arg);
    }
    
    return STATUS_TIDAK_DUKUNG;
}

/*
 * ===========================================================================
 * FUNGSI FILE DESCRIPTOR MANAGEMENT (FD MANAGEMENT FUNCTIONS)
 * ===========================================================================
 */

int fd_alloc(file_t *file)
{
    int fd;
    
    if (file == NULL) {
        return FD_INVALID;
    }
    
    file_lock();
    
    fd = fd_find_free();
    if (fd == FD_INVALID) {
        file_unlock();
        log_error("[VFS:FILE] Tidak ada FD tersedia");
        return FD_INVALID;
    }
    
    g_fd_table[fd] = file;
    g_fd_count++;
    
    /* Reference file */
    file_get(file);
    
    file_unlock();
    
    log_debug("[VFS:FILE] FD dialokasi: %d", fd);
    
    return fd;
}

file_t *fd_get(int fd)
{
    file_t *file;
    
    if (!fd_valid(fd)) {
        return NULL;
    }
    
    file_lock();
    
    file = g_fd_table[fd];
    if (file != NULL) {
        file_get(file);
    }
    
    file_unlock();
    
    return file;
}

status_t fd_put(int fd)
{
    file_t *file;
    
    if (!fd_valid(fd)) {
        return STATUS_PARAM_INVALID;
    }
    
    file_lock();
    
    file = g_fd_table[fd];
    if (file == NULL) {
        file_unlock();
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    g_fd_table[fd] = NULL;
    if (g_fd_count > 0) {
        g_fd_count--;
    }
    
    file_unlock();
    
    /* Release file reference */
    file_put(file);
    
    log_debug("[VFS:FILE] FD dibebaskan: %d", fd);
    
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
    if (new_fd == FD_INVALID) {
        file_put(file);
        return STATUS_PENUH;
    }
    
    return new_fd;
}

status_t fd_dup2(int old_fd, int new_fd)
{
    file_t *file;
    file_t *old_file;
    
    if (!fd_valid(old_fd) || !fd_valid(new_fd)) {
        return STATUS_PARAM_INVALID;
    }
    
    file = fd_get(old_fd);
    if (file == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    file_lock();
    
    /* Close existing FD if any */
    old_file = g_fd_table[new_fd];
    if (old_file != NULL) {
        g_fd_table[new_fd] = NULL;
        file_put(old_file);
    }
    
    /* Set new FD */
    g_fd_table[new_fd] = file;
    file_get(file);
    
    file_unlock();
    
    return STATUS_BERHASIL;
}

int fd_first(void)
{
    int fd;
    
    file_lock();
    
    for (fd = 0; fd < FD_MAX; fd++) {
        if (g_fd_table[fd] != NULL) {
            file_unlock();
            return fd;
        }
    }
    
    file_unlock();
    
    return FD_INVALID;
}

int fd_next(int fd)
{
    if (!fd_valid(fd)) {
        return FD_INVALID;
    }
    
    file_lock();
    
    for (fd = fd + 1; fd < FD_MAX; fd++) {
        if (g_fd_table[fd] != NULL) {
            file_unlock();
            return fd;
        }
    }
    
    file_unlock();
    
    return FD_INVALID;
}

/*
 * ===========================================================================
 * FUNGSI UTILITAS (UTILITY FUNCTIONS)
 * ===========================================================================
 */

bool_t file_is_readable(file_t *file)
{
    if (file == NULL) {
        return SALAH;
    }
    return file->f_readable;
}

bool_t file_is_writable(file_t *file)
{
    if (file == NULL) {
        return SALAH;
    }
    return file->f_writable;
}

bool_t file_is_append(file_t *file)
{
    if (file == NULL) {
        return SALAH;
    }
    return file->f_append;
}

bool_t file_is_directory(file_t *file)
{
    if (file == NULL || file->f_inode == NULL) {
        return SALAH;
    }
    return VFS_S_ISDIR(file->f_inode->i_mode);
}

off_t file_get_size(file_t *file)
{
    if (file == NULL || file->f_inode == NULL) {
        return 0;
    }
    return file->f_inode->i_size;
}

off_t file_get_position(file_t *file)
{
    if (file == NULL) {
        return 0;
    }
    return file->f_pos;
}

status_t file_set_position(file_t *file, off_t pos)
{
    if (file == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (pos < 0) {
        return STATUS_PARAM_INVALID;
    }
    
    file->f_pos = pos;
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI STATISTIK (STATISTICS FUNCTIONS)
 * ===========================================================================
 */

tak_bertanda32_t file_get_count(void)
{
    return g_fd_count;
}

tak_bertanda32_t file_get_cached_count(void)
{
    return g_file_cached;
}

/*
 * ===========================================================================
 * FUNGSI INFORMASI DAN DEBUG (INFORMATION AND DEBUG FUNCTIONS)
 * ===========================================================================
 */

void file_print_info(file_t *file)
{
    if (file == NULL) {
        kernel_printf("[VFS:FILE] File: NULL\n");
        return;
    }
    
    kernel_printf("[VFS:FILE] File Info:\n");
    kernel_printf("  Magic: 0x%08X %s\n",
                  file->f_magic,
                  file->f_magic == VFS_FILE_MAGIC ? "(valid)" : "(invalid)");
    kernel_printf("  Flags: 0x%08X\n", file->f_flags);
    kernel_printf("  Mode: 0x%04X\n", file->f_mode);
    kernel_printf("  Position: %lld\n", file->f_pos);
    kernel_printf("  Refcount: %u\n", file->f_refcount);
    kernel_printf("  Readable: %s\n", file->f_readable ? "ya" : "tidak");
    kernel_printf("  Writable: %s\n", file->f_writable ? "ya" : "tidak");
    kernel_printf("  Append: %s\n", file->f_append ? "ya" : "tidak");
    
    if (file->f_inode != NULL) {
        kernel_printf("  Inode: %llu\n", file->f_inode->i_ino);
        kernel_printf("  Inode Size: %lld\n", file->f_inode->i_size);
    }
    
    if (file->f_dentry != NULL) {
        kernel_printf("  Dentry: %s\n", file->f_dentry->d_name);
    }
}

void file_print_fd_table(void)
{
    int fd;
    file_t *file;
    tak_bertanda32_t count = 0;
    
    kernel_printf("[VFS:FILE] File Descriptor Table:\n");
    
    file_lock();
    
    for (fd = 0; fd < FD_MAX; fd++) {
        file = g_fd_table[fd];
        if (file != NULL) {
            kernel_printf("  FD %d: ", fd);
            if (file->f_dentry != NULL) {
                kernel_printf("%s", file->f_dentry->d_name);
            } else {
                kernel_printf("(no name)");
            }
            kernel_printf(" pos=%lld ref=%u\n",
                          file->f_pos, file->f_refcount);
            count++;
        }
    }
    
    file_unlock();
    
    kernel_printf("  Total: %u file descriptors\n", count);
}

/*
 * ===========================================================================
 * FUNGSI INITIALIZATION MODULE (MODULE INITIALIZATION)
 * ===========================================================================
 */

status_t file_module_init(void)
{
    int i;
    
    log_info("[VFS:FILE] Menginisialisasi modul file...");
    
    /* Clear file descriptor table */
    for (i = 0; i < FD_MAX; i++) {
        g_fd_table[i] = NULL;
    }
    g_fd_count = 0;
    
    /* Clear file cache */
    g_file_cache = NULL;
    g_file_cached = 0;
    
    /* Reset lock */
    g_file_lock = 0;
    
    /* Reserve stdin, stdout, stderr (akan di-set nanti) */
    g_fd_table[0] = NULL;  /* stdin */
    g_fd_table[1] = NULL;  /* stdout */
    g_fd_table[2] = NULL;  /* stderr */
    
    log_info("[VFS:FILE] Inisialisasi selesai");
    
    return STATUS_BERHASIL;
}

void file_module_shutdown(void)
{
    int i;
    file_t *file;
    
    log_info("[VFS:FILE] Mematikan modul file...");
    
    file_lock();
    
    /* Tutup semua file descriptor */
    for (i = 0; i < FD_MAX; i++) {
        file = g_fd_table[i];
        if (file != NULL) {
            /* Flush dan sync */
            if (file->f_writable) {
                file_flush(file);
                file_fsync(file);
            }
            
            /* Release */
            file_put(file);
            g_fd_table[i] = NULL;
        }
    }
    
    g_fd_count = 0;
    g_file_cached = 0;
    
    file_unlock();
    
    log_info("[VFS:FILE] Shutdown selesai");
}
