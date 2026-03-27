/*
 * PIGURA OS - FAT32_DIR.C
 * =========================
 * Implementasi operasi direktori untuk filesystem FAT32.
 *
 * Berkas ini berisi fungsi-fungsi untuk membaca, menulis, dan
 * mengelola entri direktori pada filesystem FAT32.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua fungsi diimplementasikan secara lengkap
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../vfs/vfs.h"
#include "../../inti/kernel.h"

/*
 * ===========================================================================
 * KONSTANTA DIREKTORI FAT32 (FAT32 DIRECTORY CONSTANTS)
 * ===========================================================================
 */

/* Entry attributes */
#define FAT32_ATTR_RDONLY       0x01
#define FAT32_ATTR_HIDDEN       0x02
#define FAT32_ATTR_SYSTEM       0x04
#define FAT32_ATTR_VOLUME_ID    0x08
#define FAT32_ATTR_DIRECTORY    0x10
#define FAT32_ATTR_ARCHIVE      0x20
#define FAT32_ATTR_LONG_NAME    0x0F

/* Special entry markers */
#define FAT32_DIR_ENTRY_FREE    0xE5
#define FAT32_DIR_ENTRY_LAST    0x00

/* Directory entry size */
#define FAT32_DIR_ENTRY_SIZE    32

/* Special cluster numbers */
#define FAT32_FIRST_CLUSTER     2
#define FAT32_END_CHAIN         0x0FFFFFFF

/*
 * ===========================================================================
 * STRUKTUR DIREKTORI FAT32 (FAT32 DIRECTORY STRUCTURE)
 * ===========================================================================
 */

typedef struct __attribute__((packed)) {
    tak_bertanda8_t  dir_name[11];
    tak_bertanda8_t  dir_attr;
    tak_bertanda8_t  dir_nt_res;
    tak_bertanda8_t  dir_crt_time_tenth;
    tak_bertanda16_t dir_crt_time;
    tak_bertanda16_t dir_crt_date;
    tak_bertanda16_t dir_lst_acc_date;
    tak_bertanda16_t dir_fst_cluster_hi;
    tak_bertanda16_t dir_wrt_time;
    tak_bertanda16_t dir_wrt_date;
    tak_bertanda16_t dir_fst_cluster_lo;
    tak_bertanda32_t dir_file_size;
} fat32_dir_entry_t;

/*
 * ===========================================================================
 * STRUKTUR LFN ENTRY (LONG FILENAME ENTRY)
 * ===========================================================================
 */

typedef struct __attribute__((packed)) {
    tak_bertanda8_t  lfn_seq;
    tak_bertanda16_t lfn_name1[5];
    tak_bertanda8_t  lfn_attr;
    tak_bertanda8_t  lfn_type;
    tak_bertanda8_t  lfn_checksum;
    tak_bertanda16_t lfn_name2[6];
    tak_bertanda16_t lfn_fst_cluster;
    tak_bertanda16_t lfn_name3[2];
} fat32_lfn_entry_t;

/*
 * ===========================================================================
 * STRUKTUR SUPERBLOCK INTERNAL (INTERNAL SUPERBLOCK STRUCTURE)
 * ===========================================================================
 */

typedef struct fat32_sb {
    tak_bertanda32_t s_magic;
    tak_bertanda32_t s_volume_id;
    char            s_volume_label[12];
    char            s_fs_type[9];
    
    tak_bertanda32_t s_bytes_per_sec;
    tak_bertanda8_t  s_sec_per_cluster;
    tak_bertanda32_t s_bytes_per_cluster;
    tak_bertanda32_t s_reserved_sec;
    tak_bertanda32_t s_num_fats;
    tak_bertanda32_t s_fat_size;
    tak_bertanda32_t s_root_cluster;
    tak_bertanda32_t s_total_sec;
    tak_bertanda32_t s_total_clusters;
    tak_bertanda32_t s_data_start_sec;
    
    tak_bertanda32_t s_free_count;
    tak_bertanda32_t s_next_free;
    
    void            *s_fat_cache;
    void            *s_root_inode;
    void            *s_root_dentry;
    
    dev_t           s_dev;
    bool_t          s_readonly;
    bool_t          s_dirty;
    
    tak_bertanda32_t s_inode_count;
    tak_bertanda32_t s_dentry_count;
} fat32_sb_t;

/*
 * ===========================================================================
 * FORWARD DECLARATIONS (DARI FILE LAIN)
 * ===========================================================================
 */

/* Dari fat32.c */
tak_bertanda16_t fat32_unix_to_date(waktu_t time);
tak_bertanda16_t fat32_unix_to_time(waktu_t time);

/* Dari fat32_fat.c */
tak_bertanda32_t fat32_read_fat_entry(fat32_sb_t *sb, tak_bertanda32_t cluster);
tak_bertanda32_t fat32_alloc_cluster(fat32_sb_t *sb);
status_t fat32_free_cluster(fat32_sb_t *sb, tak_bertanda32_t cluster);

/* Dari fat32_cluster.c */
status_t fat32_read_cluster(fat32_sb_t *sb, tak_bertanda32_t cluster,
                            void *buffer);
status_t fat32_write_cluster(fat32_sb_t *sb, tak_bertanda32_t cluster,
                             const void *buffer);
tak_bertanda32_t fat32_get_next_cluster(fat32_sb_t *sb, tak_bertanda32_t cluster);
tak_bertanda32_t fat32_get_cluster_size(fat32_sb_t *sb);

/*
 * ===========================================================================
 * FUNGSI UTILITAS NAMA (NAME UTILITY FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_is_valid_sfn_char
 * -----------------------
 * Memeriksa apakah karakter valid untuk Short Filename.
 *
 * Parameter:
 *   c - Karakter yang akan diperiksa
 *
 * Return: BENAR jika valid, SALAH jika tidak
 */
static bool_t fat32_is_valid_sfn_char(char c)
{
    /* Karakter yang diperbolehkan: A-Z, 0-9, dan beberapa simbol */
    if (c >= 'A' && c <= 'Z') return BENAR;
    if (c >= '0' && c <= '9') return BENAR;
    if (c >= 'a' && c <= 'z') return BENAR;  /* Akan dikonversi ke uppercase */
    
    switch (c) {
    case '!': case '#': case '$': case '%': 
    case '&': case '\'': case '(': case ')':
    case '-': case '@': case '^': case '_':
    case '`': case '{': case '}': case '~':
        return BENAR;
    default:
        return SALAH;
    }
}

/*
 * fat32_sfn_to_name
 * -----------------
 * Mengkonversi Short Filename (8.3) ke nama normal.
 *
 * Parameter:
 *   sfn  - Array 11 byte Short Filename
 *   name - Buffer tujuan
 *   size - Ukuran buffer
 */
void fat32_sfn_to_name(const tak_bertanda8_t sfn[11], char *name,
                        ukuran_t size)
{
    tak_bertanda32_t i, j;
    bool_t has_ext;
    
    if (sfn == NULL || name == NULL || size == 0) {
        return;
    }
    
    kernel_memset(name, 0, size);
    
    /* Check jika entry kosong atau dihapus */
    if (sfn[0] == FAT32_DIR_ENTRY_LAST) {
        return;
    }
    
    if (sfn[0] == FAT32_DIR_ENTRY_FREE) {
        name[0] = '?';
        return;
    }
    
    /* Copy nama utama (8 karakter) */
    j = 0;
    for (i = 0; i < 8 && sfn[i] != ' ' && j < size - 1; i++) {
        name[j++] = (char)sfn[i];
    }
    
    /* Check apakah ada ekstensi */
    has_ext = SALAH;
    for (i = 8; i < 11; i++) {
        if (sfn[i] != ' ') {
            has_ext = BENAR;
            break;
        }
    }
    
    /* Tambahkan ekstensi jika ada */
    if (has_ext && j < size - 1) {
        name[j++] = '.';
        for (i = 8; i < 11 && sfn[i] != ' ' && j < size - 1; i++) {
            name[j++] = (char)sfn[i];
        }
    }
    
    name[j] = '\0';
}

/*
 * fat32_name_to_sfn
 * -----------------
 * Mengkonversi nama normal ke Short Filename (8.3).
 *
 * Parameter:
 *   name - Nama normal
 *   sfn  - Buffer 11 byte untuk Short Filename
 */
void fat32_name_to_sfn(const char *name, tak_bertanda8_t sfn[11])
{
    ukuran_t namelen;
    ukuran_t i, j;
    const char *dot;
    
    if (name == NULL || sfn == NULL) {
        return;
    }
    
    /* Inisialisasi dengan spasi */
    kernel_memset(sfn, ' ', 11);
    
    namelen = kernel_strlen(name);
    if (namelen == 0) {
        return;
    }
    
    /* Cari titik ekstensi */
    dot = kernel_strrchr(name, '.');
    
    /* Copy nama utama (max 8 karakter) */
    j = 0;
    for (i = 0; i < 8 && name[i] != '\0' && name[i] != '.'; i++) {
        if (name[i] >= 'a' && name[i] <= 'z') {
            sfn[j++] = (tak_bertanda8_t)(name[i] - 'a' + 'A');
        } else {
            sfn[j++] = (tak_bertanda8_t)name[i];
        }
    }
    
    /* Copy ekstensi (max 3 karakter) */
    if (dot != NULL && dot[1] != '\0') {
        dot++;
        j = 8;
        for (i = 0; i < 3 && dot[i] != '\0'; i++) {
            if (dot[i] >= 'a' && dot[i] <= 'z') {
                sfn[j++] = (tak_bertanda8_t)(dot[i] - 'a' + 'A');
            } else {
                sfn[j++] = (tak_bertanda8_t)dot[i];
            }
        }
    }
}

/*
 * fat32_compare_sfn
 * -----------------
 * Membandingkan nama dengan Short Filename.
 *
 * Parameter:
 *   name - Nama yang akan dibandingkan
 *   sfn  - Short Filename (11 byte)
 *
 * Return: 0 jika sama, <0 jika name < sfn, >0 jika name > sfn
 */
int fat32_compare_sfn(const char *name, const tak_bertanda8_t sfn[11])
{
    char sfn_name[16];
    
    fat32_sfn_to_name(sfn, sfn_name, sizeof(sfn_name));
    
    return kernel_strcasecmp(name, sfn_name);
}

/*
 * ===========================================================================
 * FUNGSI VALIDASI ENTRY (ENTRY VALIDATION FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_is_valid_dir_entry
 * ------------------------
 * Memeriksa apakah entri direktori valid.
 *
 * Parameter:
 *   entry - Pointer ke entri direktori
 *
 * Return: BENAR jika valid, SALAH jika tidak
 */
bool_t fat32_is_valid_dir_entry(const fat32_dir_entry_t *entry)
{
    if (entry == NULL) {
        return SALAH;
    }
    
    /* Check jika entry kosong (akhir direktori) */
    if (entry->dir_name[0] == FAT32_DIR_ENTRY_LAST) {
        return SALAH;
    }
    
    /* Check jika entry dihapus */
    if (entry->dir_name[0] == FAT32_DIR_ENTRY_FREE) {
        return SALAH;
    }
    
    return BENAR;
}

/*
 * fat32_is_lfn_entry
 * ------------------
 * Memeriksa apakah entri adalah Long Filename entry.
 *
 * Parameter:
 *   entry - Pointer ke entri direktori
 *
 * Return: BENAR jika LFN, SALAH jika tidak
 */
bool_t fat32_is_lfn_entry(const fat32_dir_entry_t *entry)
{
    if (entry == NULL) {
        return SALAH;
    }
    
    /* LFN entry memiliki attr = 0x0F */
    return (entry->dir_attr == FAT32_ATTR_LONG_NAME);
}

/*
 * fat32_is_directory
 * ------------------
 * Memeriksa apakah entri adalah direktori.
 *
 * Parameter:
 *   entry - Pointer ke entri direktori
 *
 * Return: BENAR jika direktori, SALAH jika tidak
 */
bool_t fat32_is_directory(const fat32_dir_entry_t *entry)
{
    if (entry == NULL) {
        return SALAH;
    }
    
    return ((entry->dir_attr & FAT32_ATTR_DIRECTORY) != 0);
}

/*
 * fat32_is_volume_label
 * ---------------------
 * Memeriksa apakah entri adalah volume label.
 *
 * Parameter:
 *   entry - Pointer ke entri direktori
 *
 * Return: BENAR jika volume label, SALAH jika tidak
 */
bool_t fat32_is_volume_label(const fat32_dir_entry_t *entry)
{
    if (entry == NULL) {
        return SALAH;
    }
    
    return ((entry->dir_attr & FAT32_ATTR_VOLUME_ID) != 0);
}

/*
 * fat32_is_hidden
 * ---------------
 * Memeriksa apakah entri adalah hidden.
 *
 * Parameter:
 *   entry - Pointer ke entri direktori
 *
 * Return: BENAR jika hidden, SALAH jika tidak
 */
bool_t fat32_is_hidden(const fat32_dir_entry_t *entry)
{
    if (entry == NULL) {
        return SALAH;
    }
    
    return ((entry->dir_attr & FAT32_ATTR_HIDDEN) != 0);
}

/*
 * ===========================================================================
 * FUNGSI PEMBACAAN DIREKTORI (DIRECTORY READ FUNCTIONS)
 * ===========================================================================
 */

/* Forward declarations sudah ada di atas file */

/*
 * fat32_calc_dir_entry_offset
 * ---------------------------
 * Menghitung cluster dan offset untuk entry index tertentu.
 *
 * Parameter:
 *   sb           - Pointer ke struktur superblock
 *   dir_cluster  - Cluster awal direktori
 *   index        - Index entry (0-based)
 *   cluster      - Pointer untuk menyimpan cluster
 *   offset       - Pointer untuk menyimpan offset dalam cluster
 *
 * Return: Status operasi
 */
static status_t fat32_calc_dir_entry_offset(fat32_sb_t *sb,
                                             tak_bertanda32_t dir_cluster,
                                             tak_bertanda32_t index,
                                             tak_bertanda32_t *cluster,
                                             tak_bertanda32_t *offset)
{
    tak_bertanda32_t entries_per_cluster;
    tak_bertanda32_t cluster_index;
    tak_bertanda32_t current;
    tak_bertanda32_t i;
    
    if (sb == NULL || cluster == NULL || offset == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    entries_per_cluster = fat32_get_cluster_size(sb) / FAT32_DIR_ENTRY_SIZE;
    
    cluster_index = index / entries_per_cluster;
    *offset = (index % entries_per_cluster) * FAT32_DIR_ENTRY_SIZE;
    
    /* Traverse cluster chain */
    current = dir_cluster;
    for (i = 0; i < cluster_index; i++) {
        current = fat32_get_next_cluster(sb, current);
        if (current == 0) {
            return STATUS_TIDAK_DITEMUKAN;
        }
    }
    
    *cluster = current;
    
    return STATUS_BERHASIL;
}

/*
 * fat32_read_dir_entry
 * --------------------
 * Membaca entri direktori pada index tertentu.
 *
 * Parameter:
 *   sb          - Pointer ke struktur superblock FAT32
 *   dir_cluster - Cluster awal direktori
 *   index       - Index entry (0-based)
 *   entry       - Pointer ke buffer entry
 *
 * Return: Status operasi
 */
status_t fat32_read_dir_entry(fat32_sb_t *sb, tak_bertanda32_t dir_cluster,
                               tak_bertanda32_t index,
                               fat32_dir_entry_t *entry)
{
    tak_bertanda32_t cluster;
    tak_bertanda32_t offset;
    tak_bertanda8_t *buffer;
    status_t ret;
    
    if (sb == NULL || entry == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Hitung lokasi entry */
    ret = fat32_calc_dir_entry_offset(sb, dir_cluster, index, &cluster, &offset);
    if (ret != STATUS_BERHASIL) {
        return ret;
    }
    
    /* Alokasikan buffer cluster */
    buffer = (tak_bertanda8_t *)kmalloc(fat32_get_cluster_size(sb));
    if (buffer == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    /* Baca cluster */
    ret = fat32_read_cluster(sb, cluster, buffer);
    if (ret != STATUS_BERHASIL) {
        kfree(buffer);
        return ret;
    }
    
    /* Copy entry */
    kernel_memcpy(entry, buffer + offset, sizeof(fat32_dir_entry_t));
    
    kfree(buffer);
    
    return STATUS_BERHASIL;
}

/*
 * fat32_find_dir_entry
 * --------------------
 * Mencari entri direktori berdasarkan nama.
 *
 * Parameter:
 *   sb          - Pointer ke struktur superblock FAT32
 *   dir_cluster - Cluster awal direktori
 *   name        - Nama yang dicari
 *   entry       - Pointer ke buffer entry
 *   index       - Pointer untuk menyimpan index entry
 *
 * Return: Status operasi
 */
status_t fat32_find_dir_entry(fat32_sb_t *sb, tak_bertanda32_t dir_cluster,
                               const char *name, fat32_dir_entry_t *entry,
                               tak_bertanda32_t *index)
{
    tak_bertanda32_t i;
    fat32_dir_entry_t current;
    status_t ret;
    
    if (sb == NULL || name == NULL || entry == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Scan semua entry */
    for (i = 0; ; i++) {
        ret = fat32_read_dir_entry(sb, dir_cluster, i, &current);
        if (ret != STATUS_BERHASIL) {
            break;
        }
        
        /* Check jika akhir direktori */
        if (current.dir_name[0] == FAT32_DIR_ENTRY_LAST) {
            break;
        }
        
        /* Skip deleted entries */
        if (current.dir_name[0] == FAT32_DIR_ENTRY_FREE) {
            continue;
        }
        
        /* Skip LFN entries */
        if (fat32_is_lfn_entry(&current)) {
            continue;
        }
        
        /* Skip volume label */
        if (fat32_is_volume_label(&current)) {
            continue;
        }
        
        /* Bandingkan nama */
        if (fat32_compare_sfn(name, current.dir_name) == 0) {
            kernel_memcpy(entry, &current, sizeof(fat32_dir_entry_t));
            if (index != NULL) {
                *index = i;
            }
            return STATUS_BERHASIL;
        }
    }
    
    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * ===========================================================================
 * FUNGSI PENULISAN DIREKTORI (DIRECTORY WRITE FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_write_dir_entry
 * ---------------------
 * Menulis entri direktori pada index tertentu.
 *
 * Parameter:
 *   sb          - Pointer ke struktur superblock FAT32
 *   dir_cluster - Cluster awal direktori
 *   index       - Index entry (0-based)
 *   entry       - Pointer ke entry yang akan ditulis
 *
 * Return: Status operasi
 */
status_t fat32_write_dir_entry(fat32_sb_t *sb, tak_bertanda32_t dir_cluster,
                                tak_bertanda32_t index,
                                const fat32_dir_entry_t *entry)
{
    tak_bertanda32_t cluster;
    tak_bertanda32_t offset;
    tak_bertanda8_t *buffer;
    status_t ret;
    
    if (sb == NULL || entry == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (sb->s_readonly) {
        return STATUS_FS_READONLY;
    }
    
    /* Hitung lokasi entry */
    ret = fat32_calc_dir_entry_offset(sb, dir_cluster, index, &cluster, &offset);
    if (ret != STATUS_BERHASIL) {
        return ret;
    }
    
    /* Alokasikan buffer cluster */
    buffer = (tak_bertanda8_t *)kmalloc(fat32_get_cluster_size(sb));
    if (buffer == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    /* Baca cluster */
    ret = fat32_read_cluster(sb, cluster, buffer);
    if (ret != STATUS_BERHASIL) {
        kfree(buffer);
        return ret;
    }
    
    /* Modify entry */
    kernel_memcpy(buffer + offset, entry, sizeof(fat32_dir_entry_t));
    
    /* Tulis kembali cluster */
    ret = fat32_write_cluster(sb, cluster, buffer);
    
    kfree(buffer);
    
    return ret;
}

/*
 * fat32_create_dir_entry
 * ----------------------
 * Membuat entri direktori baru.
 *
 * Parameter:
 *   sb            - Pointer ke struktur superblock FAT32
 *   dir_cluster   - Cluster awal direktori parent
 *   name          - Nama file/direktori
 *   attr          - Attribute
 *   first_cluster - Cluster pertama (0 untuk file kosong)
 *   size          - Ukuran file (0 untuk direktori)
 *
 * Return: Status operasi
 */
status_t fat32_create_dir_entry(fat32_sb_t *sb, tak_bertanda32_t dir_cluster,
                                 const char *name, tak_bertanda8_t attr,
                                 tak_bertanda32_t first_cluster,
                                 tak_bertanda32_t size)
{
    fat32_dir_entry_t entry;
    tak_bertanda32_t i;
    tak_bertanda32_t free_index;
    status_t ret;
    bool_t found_free;
    
    if (sb == NULL || name == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (sb->s_readonly) {
        return STATUS_FS_READONLY;
    }
    
    /* Cek apakah nama sudah ada */
    ret = fat32_find_dir_entry(sb, dir_cluster, name, &entry, NULL);
    if (ret == STATUS_BERHASIL) {
        return STATUS_SUDAH_ADA;
    }
    
    /* Cari slot kosong */
    found_free = SALAH;
    free_index = 0;
    
    for (i = 0; ; i++) {
        ret = fat32_read_dir_entry(sb, dir_cluster, i, &entry);
        if (ret != STATUS_BERHASIL) {
            /* Perlu extend direktori */
            break;
        }
        
        /* Check jika akhir direktori atau entry bebas */
        if (entry.dir_name[0] == FAT32_DIR_ENTRY_LAST ||
            entry.dir_name[0] == FAT32_DIR_ENTRY_FREE) {
            found_free = BENAR;
            free_index = i;
            break;
        }
    }
    
    /* Buat entry baru */
    kernel_memset(&entry, 0, sizeof(fat32_dir_entry_t));
    
    /* Set nama */
    fat32_name_to_sfn(name, entry.dir_name);
    
    /* Set attribute */
    entry.dir_attr = attr;
    
    /* Set cluster */
    entry.dir_fst_cluster_hi = (tak_bertanda16_t)(first_cluster >> 16);
    entry.dir_fst_cluster_lo = (tak_bertanda16_t)(first_cluster & 0xFFFF);
    
    /* Set size */
    entry.dir_file_size = size;
    
    /* Set timestamp */
    {
        waktu_t now;
        now = kernel_get_uptime();
        entry.dir_crt_date = fat32_unix_to_date(now);
        entry.dir_crt_time = fat32_unix_to_time(now);
        entry.dir_wrt_date = entry.dir_crt_date;
        entry.dir_wrt_time = entry.dir_crt_time;
        entry.dir_lst_acc_date = entry.dir_crt_date;
    }
    
    /* Tulis entry */
    ret = fat32_write_dir_entry(sb, dir_cluster, free_index, &entry);
    if (ret != STATUS_BERHASIL) {
        return ret;
    }
    
    /* Jika ini adalah entri terakhir, tambahkan marker baru */
    {
        fat32_dir_entry_t last;
        kernel_memset(&last, 0, sizeof(fat32_dir_entry_t));
        last.dir_name[0] = FAT32_DIR_ENTRY_LAST;
        fat32_write_dir_entry(sb, dir_cluster, free_index + 1, &last);
    }
    
    log_debug("[FAT32] Created directory entry '%s' at index %u",
              name, free_index);
    
    return STATUS_BERHASIL;
}

/*
 * fat32_delete_dir_entry
 * ----------------------
 * Menghapus entri direktori.
 *
 * Parameter:
 *   sb          - Pointer ke struktur superblock FAT32
 *   dir_cluster - Cluster awal direktori
 *   index       - Index entry yang akan dihapus
 *
 * Return: Status operasi
 */
status_t fat32_delete_dir_entry(fat32_sb_t *sb, tak_bertanda32_t dir_cluster,
                                 tak_bertanda32_t index)
{
    fat32_dir_entry_t entry;
    status_t ret;
    
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (sb->s_readonly) {
        return STATUS_FS_READONLY;
    }
    
    /* Baca entry */
    ret = fat32_read_dir_entry(sb, dir_cluster, index, &entry);
    if (ret != STATUS_BERHASIL) {
        return ret;
    }
    
    /* Tandai sebagai dihapus */
    entry.dir_name[0] = FAT32_DIR_ENTRY_FREE;
    
    /* Tulis kembali */
    ret = fat32_write_dir_entry(sb, dir_cluster, index, &entry);
    
    log_debug("[FAT32] Deleted directory entry at index %u", index);
    
    return ret;
}

/*
 * ===========================================================================
 * FUNGSI DIREKTORI KHUSUS (SPECIAL DIRECTORY FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_create_directory
 * ----------------------
 * Membuat direktori baru.
 *
 * Parameter:
 *   sb          - Pointer ke struktur superblock FAT32
 *   dir_cluster - Cluster direktori parent
 *   name        - Nama direktori baru
 *
 * Return: Cluster direktori baru, atau 0 jika gagal
 */
tak_bertanda32_t fat32_create_directory(fat32_sb_t *sb,
                                         tak_bertanda32_t dir_cluster,
                                         const char *name)
{
    tak_bertanda32_t new_cluster;
    status_t ret;
    
    if (sb == NULL || name == NULL) {
        return 0;
    }
    
    if (sb->s_readonly) {
        return 0;
    }
    
    /* Alokasikan cluster untuk direktori baru */
    new_cluster = fat32_alloc_cluster(sb);
    if (new_cluster == 0) {
        log_warn("[FAT32] Failed to allocate cluster for new directory");
        return 0;
    }
    
    /* Buat entry di parent */
    ret = fat32_create_dir_entry(sb, dir_cluster, name,
                                  FAT32_ATTR_DIRECTORY,
                                  new_cluster, 0);
    if (ret != STATUS_BERHASIL) {
        fat32_free_cluster(sb, new_cluster);
        return 0;
    }
    
    /* Inisialisasi direktori baru dengan "." dan ".." */
    {
        tak_bertanda8_t *buffer;
        fat32_dir_entry_t *dot_entry;
        fat32_dir_entry_t *dotdot_entry;
        waktu_t now;
        
        buffer = (tak_bertanda8_t *)kmalloc(fat32_get_cluster_size(sb));
        if (buffer == NULL) {
            return new_cluster;  /* Direktori dibuat tapi tidak diinisialisasi */
        }
        
        kernel_memset(buffer, 0, fat32_get_cluster_size(sb));
        
        now = kernel_get_uptime();
        
        /* Entry "." */
        dot_entry = (fat32_dir_entry_t *)buffer;
        kernel_memset(dot_entry, ' ', 11);
        dot_entry->dir_name[0] = '.';
        dot_entry->dir_attr = FAT32_ATTR_DIRECTORY;
        dot_entry->dir_fst_cluster_hi = (tak_bertanda16_t)(new_cluster >> 16);
        dot_entry->dir_fst_cluster_lo = (tak_bertanda16_t)(new_cluster & 0xFFFF);
        dot_entry->dir_file_size = 0;
        dot_entry->dir_crt_date = fat32_unix_to_date(now);
        dot_entry->dir_crt_time = fat32_unix_to_time(now);
        dot_entry->dir_wrt_date = dot_entry->dir_crt_date;
        dot_entry->dir_wrt_time = dot_entry->dir_crt_time;
        
        /* Entry ".." */
        dotdot_entry = (fat32_dir_entry_t *)(buffer + FAT32_DIR_ENTRY_SIZE);
        kernel_memset(dotdot_entry, ' ', 11);
        dotdot_entry->dir_name[0] = '.';
        dotdot_entry->dir_name[1] = '.';
        dotdot_entry->dir_attr = FAT32_ATTR_DIRECTORY;
        dotdot_entry->dir_fst_cluster_hi = (tak_bertanda16_t)(dir_cluster >> 16);
        dotdot_entry->dir_fst_cluster_lo = (tak_bertanda16_t)(dir_cluster & 0xFFFF);
        dotdot_entry->dir_file_size = 0;
        dotdot_entry->dir_crt_date = fat32_unix_to_date(now);
        dotdot_entry->dir_crt_time = fat32_unix_to_time(now);
        dotdot_entry->dir_wrt_date = dotdot_entry->dir_crt_date;
        dotdot_entry->dir_wrt_time = dotdot_entry->dir_crt_time;
        
        /* End marker */
        buffer[FAT32_DIR_ENTRY_SIZE * 2] = FAT32_DIR_ENTRY_LAST;
        
        /* Tulis cluster */
        fat32_write_cluster(sb, new_cluster, buffer);
        
        kfree(buffer);
    }
    
    log_info("[FAT32] Created directory '%s' at cluster %u", name, new_cluster);
    
    return new_cluster;
}

/*
 * ===========================================================================
 * FUNGSI UTILITAS WAKTU (TIME UTILITY FUNCTIONS)
 * ===========================================================================
 */

/* Forward declarations sudah ada di atas file */

/*
 * ===========================================================================
 * FUNGSI DEBUG DIREKTORI (DIRECTORY DEBUG FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_print_dir_entry
 * ---------------------
 * Mencetak informasi entri direktori untuk debugging.
 *
 * Parameter:
 *   entry - Pointer ke entri direktori
 */
void fat32_print_dir_entry(const fat32_dir_entry_t *entry)
{
    char name[16];
    
    if (entry == NULL) {
        kernel_printf("[FAT32] Entry is NULL\n");
        return;
    }
    
    fat32_sfn_to_name(entry->dir_name, name, sizeof(name));
    
    kernel_printf("[FAT32] Entry: %.11s\n", entry->dir_name);
    kernel_printf("  Name: %s\n", name);
    kernel_printf("  Attr: 0x%02X (", entry->dir_attr);
    
    if (entry->dir_attr & FAT32_ATTR_RDONLY) kernel_printf("R");
    if (entry->dir_attr & FAT32_ATTR_HIDDEN) kernel_printf("H");
    if (entry->dir_attr & FAT32_ATTR_SYSTEM) kernel_printf("S");
    if (entry->dir_attr & FAT32_ATTR_VOLUME_ID) kernel_printf("V");
    if (entry->dir_attr & FAT32_ATTR_DIRECTORY) kernel_printf("D");
    if (entry->dir_attr & FAT32_ATTR_ARCHIVE) kernel_printf("A");
    kernel_printf(")\n");
    
    kernel_printf("  Cluster: %u\n", 
                  ((tak_bertanda32_t)entry->dir_fst_cluster_hi << 16) |
                  entry->dir_fst_cluster_lo);
    kernel_printf("  Size: %u bytes\n", entry->dir_file_size);
}

/*
 * fat32_list_directory
 * --------------------
 * Mendaftar isi direktori.
 *
 * Parameter:
 *   sb          - Pointer ke struktur superblock FAT32
 *   dir_cluster - Cluster awal direktori
 *   max_entries - Maksimum entry yang ditampilkan
 */
void fat32_list_directory(fat32_sb_t *sb, tak_bertanda32_t dir_cluster,
                           tak_bertanda32_t max_entries)
{
    fat32_dir_entry_t entry;
    char name[16];
    tak_bertanda32_t i;
    status_t ret;
    
    if (sb == NULL) {
        kernel_printf("[FAT32] Superblock is NULL\n");
        return;
    }
    
    kernel_printf("[FAT32] Directory listing (cluster %u):\n", dir_cluster);
    kernel_printf("========================================\n");
    
    for (i = 0; i < max_entries; i++) {
        ret = fat32_read_dir_entry(sb, dir_cluster, i, &entry);
        if (ret != STATUS_BERHASIL) {
            break;
        }
        
        if (entry.dir_name[0] == FAT32_DIR_ENTRY_LAST) {
            break;
        }
        
        if (entry.dir_name[0] == FAT32_DIR_ENTRY_FREE) {
            continue;
        }
        
        if (fat32_is_lfn_entry(&entry)) {
            continue;
        }
        
        fat32_sfn_to_name(entry.dir_name, name, sizeof(name));
        
        kernel_printf("%c %10u %s\n",
                      (entry.dir_attr & FAT32_ATTR_DIRECTORY) ? 'D' : 'F',
                      entry.dir_file_size, name);
    }
    
    kernel_printf("========================================\n");
    kernel_printf("Total entries shown: %u\n", i);
}
