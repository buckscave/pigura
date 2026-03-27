/*
 * PIGURA OS - CPIO.C
 * ====================
 * Implementasi parser format arsip cpio untuk initramfs.
 *
 * Berkas ini berisi fungsi-fungsi untuk membaca dan mengekstrak
 * arsip cpio dalam format newc (SVR4) dan odc (POSIX.1 portable).
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua fungsi diimplementasikan secara lengkap
 *
 * Format CPIO yang didukung:
 * - newc: SVR4 cpio dengan header ASCII hex
 * - odc:  POSIX.1 portable format dengan header oktal
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "initramfs.h"
#include "../vfs/vfs.h"
#include "../../inti/kernel.h"

/*
 * ===========================================================================
 * KONSTANTA FORMAT CPIO (CPIO FORMAT CONSTANTS)
 * ===========================================================================
 */

/* Magic number untuk format cpio */
#define CPIO_MAGIC_NEWC  "070701"  /* SVR4 newc tanpa CRC */
#define CPIO_MAGIC_NEWC_CRC "070702"  /* SVR4 newc dengan CRC */
#define CPIO_MAGIC_ODC   "070707"  /* POSIX.1 portable format */
#define CPIO_MAGIC_LEN   6

/* Panjang header format */
#define CPIO_NEWC_HEADER_LEN  110   /* Panjang header newc */
#define CPIO_ODC_HEADER_LEN   76    /* Panjang header odc */

/* Mask untuk mode file */
#define CPIO_S_IFMT     0170000
#define CPIO_S_IFIFO    0010000
#define CPIO_S_IFCHR    0020000
#define CPIO_S_IFDIR    0040000
#define CPIO_S_IFBLK    0060000
#define CPIO_S_IFREG    0100000
#define CPIO_S_IFLNK    0120000
#define CPIO_S_IFSOCK   0140000

/* Makro untuk cek tipe file */
#define CPIO_S_ISREG(m)  (((m) & CPIO_S_IFMT) == CPIO_S_IFREG)
#define CPIO_S_ISDIR(m)  (((m) & CPIO_S_IFMT) == CPIO_S_IFDIR)
#define CPIO_S_ISCHR(m)  (((m) & CPIO_S_IFMT) == CPIO_S_IFCHR)
#define CPIO_S_ISBLK(m)  (((m) & CPIO_S_IFMT) == CPIO_S_IFBLK)
#define CPIO_S_ISFIFO(m) (((m) & CPIO_S_IFMT) == CPIO_S_IFIFO)
#define CPIO_S_ISLNK(m)  (((m) & CPIO_S_IFMT) == CPIO_S_IFLNK)
#define CPIO_S_ISSOCK(m) (((m) & CPIO_S_IFMT) == CPIO_S_IFSOCK)

/* Trailer marker */
#define CPIO_TRAILER_NAME "TRAILER!!!"
#define CPIO_TRAILER_LEN  10

/* Buffer size untuk operasi */
#define CPIO_BUFFER_SIZE  4096

/* Maximum filename length */
#define CPIO_MAX_FILENAME  PATH_MAX

/*
 * ===========================================================================
 * STRUKTUR HEADER CPIO (CPIO HEADER STRUCTURES)
 * ===========================================================================
 */

/* Header format newc (SVR4) - semua field dalam ASCII hex */
typedef struct {
    char c_magic[6];      /* Magic number "070701" atau "070702" */
    char c_ino[8];        /* Inode number */
    char c_mode[8];       /* Mode dan permission */
    char c_uid[8];        /* User ID */
    char c_gid[8];        /* Group ID */
    char c_nlink[8];      /* Jumlah link */
    char c_mtime[8];      /* Modification time */
    char c_filesize[8];   /* Ukuran file */
    char c_devmajor[8];   /* Device major number */
    char c_devminor[8];   /* Device minor number */
    char c_rdevmajor[8];  /* Rdev major number */
    char c_rdevminor[8];  /* Rdev minor number */
    char c_namesize[8];   /* Panjang nama file */
    char c_check[8];      /* Checksum (untuk format CRC) */
} cpio_newc_header_t;

/* Header format odc (POSIX.1 portable) - semua field dalam ASCII oktal */
typedef struct {
    char c_magic[6];      /* Magic number "070707" */
    char c_dev[6];        /* Device number */
    char c_ino[6];        /* Inode number */
    char c_mode[6];       /* Mode dan permission */
    char c_uid[6];        /* User ID */
    char c_gid[6];        /* Group ID */
    char c_nlink[6];      /* Jumlah link */
    char c_rdev[6];       /* Rdev number */
    char c_mtime[11];     /* Modification time */
    char c_namesize[6];   /* Panjang nama file */
    char c_filesize[11];  /* Ukuran file */
} cpio_odc_header_t;

/*
 * ===========================================================================
 * STRUKTUR ENTRI CPIO (CPIO ENTRY STRUCTURE)
 * ===========================================================================
 */

/* cpio_entry_t didefinisikan di initramfs.h */

/*
 * ===========================================================================
 * STRUKTUR CONTEXT CPIO (CPIO CONTEXT STRUCTURE)
 * ===========================================================================
 */

/* Context untuk parsing cpio */
typedef struct {
    const void *c_buffer;     /* Pointer ke buffer arsip */
    ukuran_t    c_size;       /* Ukuran total arsip */
    ukuran_t    c_pos;        /* Posisi saat ini */
    cpio_entry_t *c_entries;  /* List entri */
    tak_bertanda32_t c_count; /* Jumlah entri */
    tak_bertanda32_t c_format;/* Format yang terdeteksi */
    bool_t      c_error;      /* Flag error */
    char        c_error_msg[256]; /* Pesan error */
} cpio_context_t;

/* Format yang terdeteksi */
#define CPIO_FORMAT_UNKNOWN  0
#define CPIO_FORMAT_NEWC     1
#define CPIO_FORMAT_NEWC_CRC 2
#define CPIO_FORMAT_ODC      3

/*
 * ===========================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ===========================================================================
 */

/* Lock untuk operasi cpio */
static tak_bertanda32_t g_cpio_lock = 0;

/* Statistik */
static tak_bertanda32_t g_cpio_entries_parsed = 0;
static tak_bertanda32_t g_cpio_bytes_processed = 0;

/*
 * ===========================================================================
 * FUNGSI INTERNAL LOCKING (INTERNAL LOCKING FUNCTIONS)
 * ===========================================================================
 */

static void cpio_lock(void)
{
    g_cpio_lock++;
}

static void cpio_unlock(void)
{
    if (g_cpio_lock > 0) {
        g_cpio_lock--;
    }
}

/*
 * ===========================================================================
 * FUNGSI UTILITAS INTERNAL (INTERNAL UTILITY FUNCTIONS)
 * ===========================================================================
 */

/* Konversi hex ASCII ke integer */
static tak_bertanda32_t cpio_hex_to_uint(const char *str, ukuran_t len)
{
    tak_bertanda32_t result = 0;
    ukuran_t i;
    char c;
    
    for (i = 0; i < len; i++) {
        c = str[i];
        result <<= 4;
        
        if (c >= '0' && c <= '9') {
            result |= (tak_bertanda32_t)(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            result |= (tak_bertanda32_t)(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            result |= (tak_bertanda32_t)(c - 'A' + 10);
        }
    }
    
    return result;
}

/* Konversi oktal ASCII ke integer */
static tak_bertanda32_t cpio_octal_to_uint(const char *str, ukuran_t len)
{
    tak_bertanda32_t result = 0;
    ukuran_t i;
    char c;
    
    for (i = 0; i < len; i++) {
        c = str[i];
        if (c >= '0' && c <= '7') {
            result = (result << 3) | (tak_bertanda32_t)(c - '0');
        }
    }
    
    return result;
}

/* Konversi hex ASCII ke 64-bit integer */
static tak_bertanda64_t cpio_hex_to_uint64(const char *str, ukuran_t len)
{
    tak_bertanda64_t result = 0;
    ukuran_t i;
    char c;
    
    for (i = 0; i < len; i++) {
        c = str[i];
        result <<= 4;
        
        if (c >= '0' && c <= '9') {
            result |= (tak_bertanda64_t)(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            result |= (tak_bertanda64_t)(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            result |= (tak_bertanda64_t)(c - 'A' + 10);
        }
    }
    
    return result;
}

/* Konversi oktal ASCII ke 64-bit integer */
static tak_bertanda64_t cpio_octal_to_uint64(const char *str, ukuran_t len)
{
    tak_bertanda64_t result = 0;
    ukuran_t i;
    char c;
    
    for (i = 0; i < len; i++) {
        c = str[i];
        if (c >= '0' && c <= '7') {
            result = (result << 3) | (tak_bertanda64_t)(c - '0');
        }
    }
    
    return result;
}

/* Hitung CRC32 untuk validasi */
static tak_bertanda32_t cpio_crc32(const void *data, ukuran_t len)
{
    const tak_bertanda8_t *bytes = (const tak_bertanda8_t *)data;
    tak_bertanda32_t crc = 0;
    ukuran_t i;
    
    for (i = 0; i < len; i++) {
        crc ^= (tak_bertanda32_t)bytes[i] << 24;
        
        {
            int j;
            for (j = 0; j < 8; j++) {
                if (crc & 0x80000000) {
                    crc = (crc << 1) ^ 0x04C11DB7;
                } else {
                    crc <<= 1;
                }
            }
        }
    }
    
    return crc;
}

/* Cek alignment untuk newc format */
static ukuran_t cpio_align_newc(ukuran_t offset)
{
    /* newc format menggunakan 4-byte alignment */
    return (offset + 3) & ~((ukuran_t)3);
}

/* Baca data dari buffer */
static status_t cpio_read_data(cpio_context_t *ctx, void *dest, 
                               ukuran_t len)
{
    if (ctx->c_pos + len > ctx->c_size) {
        return STATUS_PARAM_UKURAN;
    }
    
    kernel_memcpy(dest, (const void *)
                  ((uintptr_t)ctx->c_buffer + ctx->c_pos), len);
    ctx->c_pos += len;
    
    return STATUS_BERHASIL;
}

/* Skip data dalam buffer */
static status_t cpio_skip_data(cpio_context_t *ctx, ukuran_t len)
{
    if (ctx->c_pos + len > ctx->c_size) {
        return STATUS_PARAM_UKURAN;
    }
    
    ctx->c_pos += len;
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PARSING HEADER NEWC (NEWC HEADER PARSING FUNCTIONS)
 * ===========================================================================
 */

static status_t cpio_parse_newc_header(cpio_context_t *ctx, 
                                        cpio_entry_t *entry)
{
    cpio_newc_header_t header;
    status_t ret;
    const char *name_ptr;
    
    if (ctx == NULL || entry == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Baca header */
    ret = cpio_read_data(ctx, &header, sizeof(header));
    if (ret != STATUS_BERHASIL) {
        return ret;
    }
    
    /* Parse fields dari header */
    entry->c_ino = (ino_t)cpio_hex_to_uint64(header.c_ino, 8);
    entry->c_mode = (mode_t)cpio_hex_to_uint(header.c_mode, 8);
    entry->c_uid = (uid_t)cpio_hex_to_uint(header.c_uid, 8);
    entry->c_gid = (gid_t)cpio_hex_to_uint(header.c_gid, 8);
    entry->c_nlink = cpio_hex_to_uint(header.c_nlink, 8);
    entry->c_mtime = (waktu_t)cpio_hex_to_uint64(header.c_mtime, 8);
    entry->c_size = cpio_hex_to_uint64(header.c_filesize, 8);
    entry->c_namesize = cpio_hex_to_uint(header.c_namesize, 8);
    entry->c_checksum = cpio_hex_to_uint(header.c_check, 8);
    
    /* Parse device numbers */
    {
        tak_bertanda32_t major, minor;
        major = cpio_hex_to_uint(header.c_devmajor, 8);
        minor = cpio_hex_to_uint(header.c_devminor, 8);
        entry->c_dev = (dev_t)((major << 8) | minor);
        
        major = cpio_hex_to_uint(header.c_rdevmajor, 8);
        minor = cpio_hex_to_uint(header.c_rdevminor, 8);
        entry->c_rdev = (dev_t)((major << 8) | minor);
    }
    
    /* Alokasi memori untuk nama */
    if (entry->c_namesize == 0 || entry->c_namesize > CPIO_MAX_FILENAME) {
        return STATUS_PARAM_UKURAN;
    }
    
    entry->c_name = (char *)kmalloc(entry->c_namesize + 1);
    if (entry->c_name == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    /* Baca nama file */
    name_ptr = (const char *)((uintptr_t)ctx->c_buffer + ctx->c_pos);
    kernel_strncpy(entry->c_name, name_ptr, entry->c_namesize);
    entry->c_name[entry->c_namesize] = '\0';
    
    /* Skip nama dan padding */
    ctx->c_pos += entry->c_namesize;
    
    /* Align ke 4-byte boundary */
    {
        ukuran_t padded_namesize;
        padded_namesize = cpio_align_newc(
            CPIO_NEWC_HEADER_LEN + entry->c_namesize);
        if (padded_namesize > CPIO_NEWC_HEADER_LEN + entry->c_namesize) {
            ctx->c_pos += (padded_namesize - 
                          (CPIO_NEWC_HEADER_LEN + entry->c_namesize));
        }
    }
    
    /* Set pointer ke data */
    if (entry->c_size > 0) {
        entry->c_data = (void *)((uintptr_t)ctx->c_buffer + ctx->c_pos);
        ctx->c_pos += (ukuran_t)entry->c_size;
        
        /* Align setelah data */
        {
            ukuran_t padded_size = cpio_align_newc((ukuran_t)entry->c_size);
            if (padded_size > entry->c_size) {
                ctx->c_pos += (padded_size - (ukuran_t)entry->c_size);
            }
        }
    } else {
        entry->c_data = NULL;
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PARSING HEADER ODC (ODC HEADER PARSING FUNCTIONS)
 * ===========================================================================
 */

static status_t cpio_parse_odc_header(cpio_context_t *ctx, 
                                       cpio_entry_t *entry)
{
    cpio_odc_header_t header;
    status_t ret;
    const char *name_ptr;
    
    if (ctx == NULL || entry == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Baca header */
    ret = cpio_read_data(ctx, &header, sizeof(header));
    if (ret != STATUS_BERHASIL) {
        return ret;
    }
    
    /* Parse fields dari header */
    entry->c_dev = (dev_t)cpio_octal_to_uint(header.c_dev, 6);
    entry->c_ino = (ino_t)cpio_octal_to_uint64(header.c_ino, 6);
    entry->c_mode = (mode_t)cpio_octal_to_uint(header.c_mode, 6);
    entry->c_uid = (uid_t)cpio_octal_to_uint(header.c_uid, 6);
    entry->c_gid = (gid_t)cpio_octal_to_uint(header.c_gid, 6);
    entry->c_nlink = cpio_octal_to_uint(header.c_nlink, 6);
    entry->c_rdev = (dev_t)cpio_octal_to_uint(header.c_rdev, 6);
    entry->c_mtime = (waktu_t)cpio_octal_to_uint64(header.c_mtime, 11);
    entry->c_namesize = cpio_octal_to_uint(header.c_namesize, 6);
    entry->c_size = cpio_octal_to_uint64(header.c_filesize, 11);
    entry->c_checksum = 0; /* odc tidak punya checksum */
    
    /* Alokasi memori untuk nama */
    if (entry->c_namesize == 0 || entry->c_namesize > CPIO_MAX_FILENAME) {
        return STATUS_PARAM_UKURAN;
    }
    
    entry->c_name = (char *)kmalloc(entry->c_namesize + 1);
    if (entry->c_name == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    /* Baca nama file */
    name_ptr = (const char *)((uintptr_t)ctx->c_buffer + ctx->c_pos);
    kernel_strncpy(entry->c_name, name_ptr, entry->c_namesize);
    entry->c_name[entry->c_namesize] = '\0';
    
    /* Skip nama */
    ctx->c_pos += entry->c_namesize;
    
    /* Set pointer ke data */
    if (entry->c_size > 0) {
        entry->c_data = (void *)((uintptr_t)ctx->c_buffer + ctx->c_pos);
        ctx->c_pos += (ukuran_t)entry->c_size;
    } else {
        entry->c_data = NULL;
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI DETEKSI FORMAT (FORMAT DETECTION FUNCTIONS)
 * ===========================================================================
 */

tak_bertanda32_t cpio_detect_format(const void *buffer, ukuran_t size)
{
    const char *magic;
    
    if (buffer == NULL || size < CPIO_MAGIC_LEN) {
        return CPIO_FORMAT_UNKNOWN;
    }
    
    magic = (const char *)buffer;
    
    if (kernel_strncmp(magic, CPIO_MAGIC_NEWC, CPIO_MAGIC_LEN) == 0) {
        return CPIO_FORMAT_NEWC;
    }
    
    if (kernel_strncmp(magic, CPIO_MAGIC_NEWC_CRC, CPIO_MAGIC_LEN) == 0) {
        return CPIO_FORMAT_NEWC_CRC;
    }
    
    if (kernel_strncmp(magic, CPIO_MAGIC_ODC, CPIO_MAGIC_LEN) == 0) {
        return CPIO_FORMAT_ODC;
    }
    
    return CPIO_FORMAT_UNKNOWN;
}

/*
 * ===========================================================================
 * FUNGSI PARSING CPIO UTAMA (MAIN CPIO PARSING FUNCTIONS)
 * ===========================================================================
 */

status_t cpio_parse(const void *buffer, ukuran_t size, 
                    cpio_entry_t **entries, tak_bertanda32_t *count)
{
    cpio_context_t ctx;
    cpio_entry_t *entry;
    cpio_entry_t *tail = NULL;
    status_t ret;
    tak_bertanda32_t format;
    
    if (buffer == NULL || entries == NULL || count == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Deteksi format */
    format = cpio_detect_format(buffer, size);
    if (format == CPIO_FORMAT_UNKNOWN) {
        log_error("[CPIO] Format cpio tidak dikenali");
        return STATUS_FORMAT_TIDAK_DUKUNG;
    }
    
    /* Inisialisasi context */
    kernel_memset(&ctx, 0, sizeof(ctx));
    ctx.c_buffer = buffer;
    ctx.c_size = size;
    ctx.c_pos = 0;
    ctx.c_format = format;
    ctx.c_entries = NULL;
    ctx.c_count = 0;
    
    *entries = NULL;
    *count = 0;
    
    cpio_lock();
    
    /* Parse semua entry */
    while (ctx.c_pos < ctx.c_size) {
        /* Alokasi entry baru */
        entry = (cpio_entry_t *)kmalloc(sizeof(cpio_entry_t));
        if (entry == NULL) {
            cpio_unlock();
            return STATUS_MEMORI_HABIS;
        }
        
        kernel_memset(entry, 0, sizeof(cpio_entry_t));
        
        /* Parse header berdasarkan format */
        switch (format) {
        case CPIO_FORMAT_NEWC:
        case CPIO_FORMAT_NEWC_CRC:
            ret = cpio_parse_newc_header(&ctx, entry);
            break;
            
        case CPIO_FORMAT_ODC:
            ret = cpio_parse_odc_header(&ctx, entry);
            break;
            
        default:
            ret = STATUS_FORMAT_TIDAK_DUKUNG;
            break;
        }
        
        if (ret != STATUS_BERHASIL) {
            if (entry->c_name != NULL) {
                kfree(entry->c_name);
            }
            kfree(entry);
            break;
        }
        
        /* Cek untuk trailer */
        if (kernel_strcmp(entry->c_name, CPIO_TRAILER_NAME) == 0) {
            if (entry->c_name != NULL) {
                kfree(entry->c_name);
            }
            kfree(entry);
            break;
        }
        
        /* Validasi CRC jika perlu */
        if (format == CPIO_FORMAT_NEWC_CRC && entry->c_size > 0) {
            tak_bertanda32_t calculated_crc;
            calculated_crc = cpio_crc32(entry->c_data, 
                                         (ukuran_t)entry->c_size);
            if (calculated_crc != entry->c_checksum) {
                log_warn("[CPIO] CRC mismatch untuk %s: expected %08X, "
                         "got %08X", entry->c_name, entry->c_checksum,
                         calculated_crc);
            }
        }
        
        /* Tambah ke list */
        if (ctx.c_entries == NULL) {
            ctx.c_entries = entry;
            tail = entry;
        } else {
            tail->c_next = entry;
            tail = entry;
        }
        
        ctx.c_count++;
        g_cpio_entries_parsed++;
    }
    
    g_cpio_bytes_processed += size;
    
    *entries = ctx.c_entries;
    *count = ctx.c_count;
    
    cpio_unlock();
    
    log_info("[CPIO] Parsed %u entries, %lu bytes", ctx.c_count, 
             (unsigned long)size);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI FREE ENTRI (ENTRY FREE FUNCTIONS)
 * ===========================================================================
 */

void cpio_free_entry(cpio_entry_t *entry)
{
    cpio_entry_t *curr;
    cpio_entry_t *next;
    
    curr = entry;
    
    while (curr != NULL) {
        next = curr->c_next;
        
        if (curr->c_name != NULL) {
            kfree(curr->c_name);
        }
        
        /* Data tidak di-free karena masih bagian dari buffer asli */
        
        kfree(curr);
        curr = next;
    }
}

/*
 * ===========================================================================
 * FUNGSI KONVERSI MODE (MODE CONVERSION FUNCTIONS)
 * ===========================================================================
 */

mode_t cpio_mode_to_vfs(tak_bertanda32_t cpio_mode)
{
    mode_t vfs_mode = 0;
    
    /* Konversi tipe file */
    if (CPIO_S_ISREG(cpio_mode)) {
        vfs_mode |= VFS_S_IFREG;
    } else if (CPIO_S_ISDIR(cpio_mode)) {
        vfs_mode |= VFS_S_IFDIR;
    } else if (CPIO_S_ISCHR(cpio_mode)) {
        vfs_mode |= VFS_S_IFCHR;
    } else if (CPIO_S_ISBLK(cpio_mode)) {
        vfs_mode |= VFS_S_IFBLK;
    } else if (CPIO_S_ISFIFO(cpio_mode)) {
        vfs_mode |= VFS_S_IFIFO;
    } else if (CPIO_S_ISLNK(cpio_mode)) {
        vfs_mode |= VFS_S_IFLNK;
    } else if (CPIO_S_ISSOCK(cpio_mode)) {
        vfs_mode |= VFS_S_IFSOCK;
    }
    
    /* Konversi permission bits */
    vfs_mode |= (mode_t)(cpio_mode & 0777);
    
    return vfs_mode;
}

/*
 * ===========================================================================
 * FUNGSI UTILITAS ENTRI (ENTRY UTILITY FUNCTIONS)
 * ===========================================================================
 */

bool_t cpio_entry_is_file(cpio_entry_t *entry)
{
    if (entry == NULL) {
        return SALAH;
    }
    return CPIO_S_ISREG(entry->c_mode) ? BENAR : SALAH;
}

bool_t cpio_entry_is_directory(cpio_entry_t *entry)
{
    if (entry == NULL) {
        return SALAH;
    }
    return CPIO_S_ISDIR(entry->c_mode) ? BENAR : SALAH;
}

bool_t cpio_entry_is_symlink(cpio_entry_t *entry)
{
    if (entry == NULL) {
        return SALAH;
    }
    return CPIO_S_ISLNK(entry->c_mode) ? BENAR : SALAH;
}

bool_t cpio_entry_is_device(cpio_entry_t *entry)
{
    if (entry == NULL) {
        return SALAH;
    }
    return (CPIO_S_ISCHR(entry->c_mode) || 
            CPIO_S_ISBLK(entry->c_mode)) ? BENAR : SALAH;
}

const char *cpio_entry_get_name(cpio_entry_t *entry)
{
    if (entry == NULL) {
        return NULL;
    }
    return entry->c_name;
}

const void *cpio_entry_get_data(cpio_entry_t *entry)
{
    if (entry == NULL) {
        return NULL;
    }
    return entry->c_data;
}

tak_bertanda64_t cpio_entry_get_size(cpio_entry_t *entry)
{
    if (entry == NULL) {
        return 0;
    }
    return entry->c_size;
}

/*
 * ===========================================================================
 * FUNGSI PENCARIAN ENTRI (ENTRY SEARCH FUNCTIONS)
 * ===========================================================================
 */

cpio_entry_t *cpio_find_entry(cpio_entry_t *entries, const char *name)
{
    cpio_entry_t *curr;
    
    if (entries == NULL || name == NULL) {
        return NULL;
    }
    
    curr = entries;
    while (curr != NULL) {
        if (curr->c_name != NULL && 
            kernel_strcmp(curr->c_name, name) == 0) {
            return curr;
        }
        curr = curr->c_next;
    }
    
    return NULL;
}

cpio_entry_t *cpio_find_entry_by_ino(cpio_entry_t *entries, ino_t ino)
{
    cpio_entry_t *curr;
    
    if (entries == NULL) {
        return NULL;
    }
    
    curr = entries;
    while (curr != NULL) {
        if (curr->c_ino == ino) {
            return curr;
        }
        curr = curr->c_next;
    }
    
    return NULL;
}

/*
 * ===========================================================================
 * FUNGSI ITERASI ENTRI (ENTRY ITERATION FUNCTIONS)
 * ===========================================================================
 */

cpio_entry_t *cpio_first_entry(cpio_entry_t *entries)
{
    return entries;
}

cpio_entry_t *cpio_next_entry(cpio_entry_t *entry)
{
    if (entry == NULL) {
        return NULL;
    }
    return entry->c_next;
}

/*
 * ===========================================================================
 * FUNGSI STATISTIK (STATISTICS FUNCTIONS)
 * ===========================================================================
 */

tak_bertanda32_t cpio_get_entries_parsed(void)
{
    return g_cpio_entries_parsed;
}

tak_bertanda64_t cpio_get_bytes_processed(void)
{
    return g_cpio_bytes_processed;
}

/*
 * ===========================================================================
 * FUNGSI INFORMASI DAN DEBUG (INFORMATION AND DEBUG FUNCTIONS)
 * ===========================================================================
 */

void cpio_print_entry(cpio_entry_t *entry)
{
    if (entry == NULL) {
        kernel_printf("[CPIO] Entry: NULL\n");
        return;
    }
    
    kernel_printf("[CPIO] Entry: %s\n", entry->c_name ? entry->c_name : "(null)");
    kernel_printf("  Ino:    %llu\n", entry->c_ino);
    kernel_printf("  Mode:   0%06o (", entry->c_mode);
    
    if (CPIO_S_ISREG(entry->c_mode)) {
        kernel_printf("file");
    } else if (CPIO_S_ISDIR(entry->c_mode)) {
        kernel_printf("dir");
    } else if (CPIO_S_ISCHR(entry->c_mode)) {
        kernel_printf("char");
    } else if (CPIO_S_ISBLK(entry->c_mode)) {
        kernel_printf("block");
    } else if (CPIO_S_ISLNK(entry->c_mode)) {
        kernel_printf("symlink");
    } else if (CPIO_S_ISFIFO(entry->c_mode)) {
        kernel_printf("fifo");
    } else if (CPIO_S_ISSOCK(entry->c_mode)) {
        kernel_printf("socket");
    } else {
        kernel_printf("unknown");
    }
    
    kernel_printf(")\n");
    kernel_printf("  UID:    %u\n", entry->c_uid);
    kernel_printf("  GID:    %u\n", entry->c_gid);
    kernel_printf("  Size:   %llu bytes\n", entry->c_size);
    kernel_printf("  Nlink:  %u\n", entry->c_nlink);
    kernel_printf("  Mtime:  %lld\n", entry->c_mtime);
}

void cpio_print_list(cpio_entry_t *entries)
{
    cpio_entry_t *curr;
    tak_bertanda32_t count = 0;
    tak_bertanda32_t dir_count = 0;
    tak_bertanda32_t file_count = 0;
    tak_bertanda64_t total_size = 0;
    
    kernel_printf("[CPIO] Entry List:\n");
    
    curr = entries;
    while (curr != NULL) {
        kernel_printf("  %s", curr->c_name ? curr->c_name : "(null)");
        
        if (CPIO_S_ISDIR(curr->c_mode)) {
            kernel_printf("/");
            dir_count++;
        } else if (CPIO_S_ISLNK(curr->c_mode)) {
            kernel_printf(" -> %s", 
                         curr->c_data ? (const char *)curr->c_data : "?");
            file_count++;
        } else {
            file_count++;
            total_size += curr->c_size;
        }
        
        kernel_printf(" (%llu bytes)\n", curr->c_size);
        count++;
        curr = curr->c_next;
    }
    
    kernel_printf("[CPIO] Total: %u entries (%u dirs, %u files)\n",
                  count, dir_count, file_count);
    kernel_printf("[CPIO] Total size: %llu bytes\n", total_size);
}

/*
 * ===========================================================================
 * FUNGSI INITIALIZATION MODULE (MODULE INITIALIZATION)
 * ===========================================================================
 */

status_t cpio_module_init(void)
{
    log_info("[CPIO] Menginisialisasi modul cpio...");
    
    /* Reset statistik */
    g_cpio_entries_parsed = 0;
    g_cpio_bytes_processed = 0;
    
    /* Reset lock */
    g_cpio_lock = 0;
    
    log_info("[CPIO] Inisialisasi selesai");
    
    return STATUS_BERHASIL;
}

void cpio_module_shutdown(void)
{
    log_info("[CPIO] Mematikan modul cpio...");
    
    /* Reset statistik */
    g_cpio_entries_parsed = 0;
    g_cpio_bytes_processed = 0;
    
    log_info("[CPIO] Shutdown selesai");
}

/*
 * ===========================================================================
 * FUNGSI UTILITAS STRING (STRING UTILITY FUNCTIONS)
 * ===========================================================================
 */

/* strtok_r - thread-safe strtok replacement */
char *kernel_strtok_r(char *str, const char *delim, char **saveptr)
{
    char *token;
    char *p;
    
    if (str == NULL) {
        str = *saveptr;
    }
    
    if (str == NULL || *str == '\0') {
        *saveptr = NULL;
        return NULL;
    }
    
    /* Skip leading delimiters */
    p = str;
    while (*p != '\0') {
        const char *d = delim;
        bool_t found = SALAH;
        while (*d != '\0') {
            if (*p == *d) {
                found = BENAR;
                break;
            }
            d++;
        }
        if (!found) {
            break;
        }
        p++;
    }
    
    if (*p == '\0') {
        *saveptr = NULL;
        return NULL;
    }
    
    token = p;
    
    /* Find end of token */
    while (*p != '\0') {
        const char *d = delim;
        bool_t found = SALAH;
        while (*d != '\0') {
            if (*p == *d) {
                found = BENAR;
                break;
            }
            d++;
        }
        if (found) {
            *p = '\0';
            *saveptr = p + 1;
            return token;
        }
        p++;
    }
    
    *saveptr = NULL;
    return token;
}
