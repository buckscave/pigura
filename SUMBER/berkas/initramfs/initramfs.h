/*
 * PIGURA OS - INITRAMFS.H
 * =========================
 * Header initramfs filesystem untuk kernel Pigura OS.
 *
 * Berkas ini mendefinisikan struktur data dan fungsi API untuk
 * sistem initramfs filesystem.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua struktur dan fungsi didefinisikan secara eksplisit
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#ifndef BERKAS_INITRAMFS_INITRAMFS_H
#define BERKAS_INITRAMFS_INITRAMFS_H

/*
 * ===========================================================================
 * INCLUDE
 * ===========================================================================
 */

#include "../../inti/types.h"
#include "../../inti/konstanta.h"

/*
 * ===========================================================================
 * KONSTANTA INITRAMFS (INITRAMFS CONSTANTS)
 * ===========================================================================
 */

/* Magic number */
#define INITRAMFS_MAGIC         0x49465352  /* "IFSR" */

/* Maximum path length */
#define INITRAMFS_PATH_MAX      CONFIG_PATH_MAX
#define INITRAMFS_NAME_MAX      CONFIG_NAME_MAX

/*
 * ===========================================================================
 * KONSTANTA CPIO (CPIO CONSTANTS)
 * ===========================================================================
 */

/* Format types */
#define CPIO_FORMAT_UNKNOWN     0
#define CPIO_FORMAT_NEWC        1
#define CPIO_FORMAT_NEWC_CRC    2
#define CPIO_FORMAT_ODC         3

/* Magic length */
#define CPIO_MAGIC_LEN          6

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

/*
 * ===========================================================================
 * STRUKTUR CPIO ENTRY (CPIO ENTRY STRUCTURE)
 * ===========================================================================
 */

typedef struct cpio_entry {
    ino_t       c_ino;        /* Inode number */
    mode_t      c_mode;       /* Mode dan permission */
    uid_t       c_uid;        /* User ID */
    gid_t       c_gid;        /* Group ID */
    tak_bertanda32_t c_nlink; /* Jumlah link */
    waktu_t     c_mtime;      /* Modification time */
    tak_bertanda64_t c_size;  /* Ukuran file */
    dev_t       c_dev;        /* Device number */
    dev_t       c_rdev;       /* Rdev number */
    char       *c_name;       /* Nama file */
    ukuran_t    c_namesize;   /* Panjang nama */
    tak_bertanda32_t c_checksum; /* Checksum (untuk format CRC) */
    void       *c_data;       /* Pointer ke data file */
    struct cpio_entry *c_next;/* Entry berikutnya */
} cpio_entry_t;

/*
 * ===========================================================================
 * STRUKTUR INODE INITRAMFS (INITRAMFS INODE STRUCTURE)
 * ===========================================================================
 */

typedef struct initramfs_inode {
    ino_t           i_ino;      /* Inode number */
    mode_t          i_mode;     /* Mode dan permission */
    uid_t           i_uid;      /* User ID */
    gid_t           i_gid;      /* Group ID */
    waktu_t         i_atime;    /* Access time */
    waktu_t         i_mtime;    /* Modify time */
    waktu_t         i_ctime;    /* Change time */
    tak_bertanda32_t i_nlink;   /* Link count */
    off_t           i_size;     /* File size */
    dev_t           i_rdev;     /* Device number */
    const void     *i_data;     /* Pointer ke data */
    char           *i_target;   /* Symlink target */
    struct initramfs_inode *i_parent; /* Parent directory */
    struct initramfs_dentry *i_children; /* Child entries */
} initramfs_inode_t;

/*
 * ===========================================================================
 * STRUKTUR DENTRY INITRAMFS (INITRAMFS DENTRY STRUCTURE)
 * ===========================================================================
 */

typedef struct initramfs_dentry {
    char                        d_name[INITRAMFS_NAME_MAX + 1];
    ukuran_t                    d_namelen;
    initramfs_inode_t          *d_inode;
    struct initramfs_dentry    *d_next;    /* Sibling */
    struct initramfs_dentry    *d_child;   /* First child */
    struct initramfs_dentry    *d_parent;  /* Parent dentry */
} initramfs_dentry_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI CPIO (CPIO FUNCTIONS)
 * ===========================================================================
 */

/* Format detection */
tak_bertanda32_t cpio_detect_format(const void *buffer, ukuran_t size);

/* Parsing */
status_t cpio_parse(const void *buffer, ukuran_t size,
                    cpio_entry_t **entries, tak_bertanda32_t *count);

/* Cleanup */
void cpio_free_entry(cpio_entry_t *entry);

/* Mode conversion */
mode_t cpio_mode_to_vfs(tak_bertanda32_t cpio_mode);

/* Entry type check */
bool_t cpio_entry_is_file(cpio_entry_t *entry);
bool_t cpio_entry_is_directory(cpio_entry_t *entry);
bool_t cpio_entry_is_symlink(cpio_entry_t *entry);
bool_t cpio_entry_is_device(cpio_entry_t *entry);

/* Entry access */
const char *cpio_entry_get_name(cpio_entry_t *entry);
const void *cpio_entry_get_data(cpio_entry_t *entry);
tak_bertanda64_t cpio_entry_get_size(cpio_entry_t *entry);

/* Entry search */
cpio_entry_t *cpio_find_entry(cpio_entry_t *entries, const char *name);
cpio_entry_t *cpio_find_entry_by_ino(cpio_entry_t *entries, ino_t ino);

/* Iteration */
cpio_entry_t *cpio_first_entry(cpio_entry_t *entries);
cpio_entry_t *cpio_next_entry(cpio_entry_t *entry);

/* Statistics */
tak_bertanda32_t cpio_get_entries_parsed(void);
tak_bertanda64_t cpio_get_bytes_processed(void);

/* Debug */
void cpio_print_entry(cpio_entry_t *entry);
void cpio_print_list(cpio_entry_t *entries);

/* Module */
status_t cpio_module_init(void);
void cpio_module_shutdown(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INITRAMFS CORE (INITRAMFS CORE FUNCTIONS)
 * ===========================================================================
 */

/* Initialization */
status_t initramfs_init(const void *data, ukuran_t size);
void initramfs_shutdown(void);

/* Filesystem creation */
initramfs_inode_t *initramfs_create_file(const char *path, mode_t mode,
                                          uid_t uid, gid_t gid,
                                          const void *data, off_t size);
initramfs_inode_t *initramfs_create_directory(const char *path, mode_t mode,
                                               uid_t uid, gid_t gid);
initramfs_inode_t *initramfs_create_symlink(const char *path,
                                              const char *target,
                                              uid_t uid, gid_t gid);

/* Path resolution */
initramfs_dentry_t *initramfs_resolve_path(const char *path);

/* Registration */
status_t initramfs_register(void);
status_t initramfs_unregister(void);

/* Statistics */
tak_bertanda32_t initramfs_get_inode_count(void);
tak_bertanda32_t initramfs_get_dentry_count(void);
tak_bertanda64_t initramfs_get_total_size(void);

/* Debug */
void initramfs_print_info(void);
void initramfs_print_tree(void);

/* Module */
status_t initramfs_module_init(void);
void initramfs_module_shutdown(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INITRAMFS EXTRACT (INITRAMFS EXTRACT FUNCTIONS)
 * ===========================================================================
 */

/* Extraction */
status_t initramfs_extract(const void *buffer, ukuran_t size);
status_t initramfs_load_from_module(tak_bertanda32_t module_index);
status_t initramfs_load_from_address(const void *address, ukuran_t size);

/* Mount */
status_t initramfs_mount_root(void);

/* Statistics */
void initramfs_get_extract_stats(tak_bertanda32_t *files,
                                  tak_bertanda32_t *dirs,
                                  tak_bertanda32_t *symlinks,
                                  tak_bertanda64_t *bytes);
tak_bertanda32_t initramfs_get_extract_errors(void);

/* Validation */
bool_t initramfs_validate_cpio(const void *buffer, ukuran_t size);
bool_t initramfs_is_loaded(void);

/* Utility */
const void *initramfs_find_file(const char *path, ukuran_t *size);
tak_bertandas_t initramfs_read_file(const char *path, void *buffer,
                                     ukuran_t size, ukuran_t offset);
bool_t initramfs_path_exists(const char *path);
bool_t initramfs_is_directory(const char *path);
status_t initramfs_list_directory(const char *path,
                                   void (*callback)(const char *name,
                                                    mode_t mode,
                                                    off_t size,
                                                    void *context),
                                   void *context);

/* Debug */
void initramfs_print_extract_stats(void);
void initramfs_print_extract_list(void);

/* Module */
status_t initramfs_extract_module_init(void);
void initramfs_extract_module_shutdown(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI STRING UTILITAS (STRING UTILITY FUNCTIONS)
 * ===========================================================================
 */

/* strtok_r - thread-safe strtok */
char *kernel_strtok_r(char *str, const char *delim, char **saveptr);

#endif /* BERKAS_INITRAMFS_INITRAMFS_H */
