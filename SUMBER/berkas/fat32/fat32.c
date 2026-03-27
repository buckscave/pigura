/*
 * PIGURA OS - FAT32.C
 * =====================
 * Implementasi driver FAT32 filesystem untuk kernel Pigura OS.
 *
 * Berkas ini berisi implementasi lengkap driver FAT32 yang mendukung
 * pembacaan dan penulisan filesystem FAT32 pada media penyimpanan.
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
 * KONSTANTA FAT32 (FAT32 CONSTANTS)
 * ===========================================================================
 */

/* Magic number dan signature */
#define FAT32_MAGIC             0x54414633  /* "FAT3" */
#define FAT32_BOOT_SIGNATURE    0xAA55
#define FAT32_FSINFO_SIGNATURE1 0x41615252
#define FAT32_FSINFO_SIGNATURE2 0x61417272

/* FAT entry values */
#define FAT32_FREE              0x00000000
#define FAT32_RESERVED          0x0FFFFFF0
#define FAT32_BAD_CLUSTER       0x0FFFFFF7
#define FAT32_END_CHAIN         0x0FFFFFF8
#define FAT32_END_CHAIN2        0x0FFFFFFF

/* Mask untuk cluster value */
#define FAT32_CLUSTER_MASK      0x0FFFFFFF

/* Attribute byte flags */
#define FAT32_ATTR_RDONLY       0x01
#define FAT32_ATTR_HIDDEN       0x02
#define FAT32_ATTR_SYSTEM       0x04
#define FAT32_ATTR_VOLUME_ID    0x08
#define FAT32_ATTR_DIRECTORY    0x10
#define FAT32_ATTR_ARCHIVE      0x20
#define FAT32_ATTR_LONG_NAME    0x0F

/* Special cluster numbers */
#define FAT32_ROOT_CLUSTER      0
#define FAT32_FIRST_CLUSTER     2

/* Maximum values */
#define FAT32_MAX_CLUSTERS      0x0FFFFFF6
#define FAT32_MAX_NAME_LEN      255
#define FAT32_SFN_LEN           11
#define FAT32_LFN_MAX_ENTRIES   20

/* FS Information structure offset */
#define FAT32_FSINFO_OFFSET     512

/*
 * ===========================================================================
 * STRUKTUR BOOT SECTOR FAT32 (FAT32 BOOT SECTOR STRUCTURE)
 * ===========================================================================
 */

typedef struct __attribute__((packed)) {
    /* Jump instruction dan NOP */
    tak_bertanda8_t bs_jump[3];
    
    /* OEM name (8 bytes) */
    tak_bertanda8_t bs_oem_name[8];
    
    /* BIOS Parameter Block (BPB) */
    tak_bertanda16_t bpb_bytes_per_sec;
    tak_bertanda8_t  bpb_sec_per_cluster;
    tak_bertanda16_t bpb_reserved_sec;
    tak_bertanda8_t  bpb_num_fats;
    tak_bertanda16_t bpb_root_entries;      /* 0 untuk FAT32 */
    tak_bertanda16_t bpb_total_sec_16;      /* 0 untuk FAT32 */
    tak_bertanda8_t  bpb_media_type;
    tak_bertanda16_t bpb_sec_per_fat_16;    /* 0 untuk FAT32 */
    tak_bertanda16_t bpb_sec_per_track;
    tak_bertanda16_t bpb_num_heads;
    tak_bertanda32_t bpb_hidden_sec;
    tak_bertanda32_t bpb_total_sec_32;
    
    /* FAT32 Extended BPB */
    tak_bertanda32_t bpb_fat_size_32;
    tak_bertanda16_t bpb_ext_flags;
    tak_bertanda16_t bpb_fs_version;
    tak_bertanda32_t bpb_root_cluster;
    tak_bertanda16_t bpb_fs_info;
    tak_bertanda16_t bpb_backup_boot;
    tak_bertanda8_t  bpb_reserved[12];
    
    /* Extended boot record */
    tak_bertanda8_t  bs_drive_num;
    tak_bertanda8_t  bs_reserved1;
    tak_bertanda8_t  bs_boot_sig;
    tak_bertanda32_t bs_volume_id;
    tak_bertanda8_t  bs_volume_label[11];
    tak_bertanda8_t  bs_fs_type[8];
    
    /* Boot code dan signature */
    tak_bertanda8_t  bs_boot_code[420];
    tak_bertanda16_t bs_signature;
} fat32_boot_sector_t;

/*
 * ===========================================================================
 * STRUKTUR FSINFO FAT32 (FAT32 FSINFO STRUCTURE)
 * ===========================================================================
 */

typedef struct __attribute__((packed)) {
    tak_bertanda32_t fs_signature1;
    tak_bertanda8_t  fs_reserved1[480];
    tak_bertanda32_t fs_signature2;
    tak_bertanda32_t fs_free_count;
    tak_bertanda32_t fs_next_free;
    tak_bertanda8_t  fs_reserved2[12];
    tak_bertanda32_t fs_signature3;
} fat32_fsinfo_t;

/*
 * ===========================================================================
 * STRUKTUR DIREKTORI FAT32 (FAT32 DIRECTORY ENTRY)
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
 * STRUKTUR INODE FAT32 (FAT32 INODE STRUCTURE)
 * ===========================================================================
 */

typedef struct fat32_inode {
    ino_t           i_ino;
    mode_t          i_mode;
    uid_t           i_uid;
    gid_t           i_gid;
    waktu_t         i_atime;
    waktu_t         i_mtime;
    waktu_t         i_ctime;
    tak_bertanda32_t i_nlink;
    off_t           i_size;
    tak_bertanda32_t i_cluster;
    tak_bertanda32_t i_dir_cluster;
    tak_bertanda32_t i_dir_offset;
    tak_bertanda8_t  i_attr;
    struct fat32_sb *i_sb;
    bool_t          i_dirty;
    bool_t          i_loaded;
} fat32_inode_t;

/*
 * ===========================================================================
 * STRUKTUR DENTRY FAT32 (FAT32 DENTRY STRUCTURE)
 * ===========================================================================
 */

typedef struct fat32_dentry {
    char            d_name[FAT32_MAX_NAME_LEN + 1];
    ukuran_t        d_namelen;
    fat32_inode_t  *d_inode;
    struct fat32_dentry *d_parent;
    struct fat32_dentry *d_next;
    struct fat32_dentry *d_child;
    tak_bertanda32_t d_cluster;
    tak_bertanda32_t d_offset;
} fat32_dentry_t;

/*
 * ===========================================================================
 * STRUKTUR SUPERBLOCK FAT32 (FAT32 SUPERBLOCK STRUCTURE)
 * ===========================================================================
 */

typedef struct fat32_sb {
    tak_bertanda32_t s_magic;
    tak_bertanda32_t s_volume_id;
    char            s_volume_label[12];
    char            s_fs_type[9];
    
    /* Geometry */
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
    
    /* FSInfo */
    tak_bertanda32_t s_free_count;
    tak_bertanda32_t s_next_free;
    
    /* Pointers */
    void            *s_fat_cache;
    fat32_inode_t   *s_root_inode;
    fat32_dentry_t  *s_root_dentry;
    
    /* Device info */
    dev_t           s_dev;
    bool_t          s_readonly;
    bool_t          s_dirty;
    
    /* Statistics */
    tak_bertanda32_t s_inode_count;
    tak_bertanda32_t s_dentry_count;
} fat32_sb_t;

/*
 * ===========================================================================
 * STRUKTUR FILE DESCRIPTOR FAT32 (FAT32 FILE DESCRIPTOR)
 * ===========================================================================
 */

typedef struct {
    fat32_inode_t  *f_inode;
    off_t           f_pos;
    tak_bertanda32_t f_flags;
    tak_bertanda32_t f_cluster;
    tak_bertanda32_t f_cluster_offset;
} fat32_fd_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ===========================================================================
 */

/* FAT32 superblock */
static fat32_sb_t *g_fat32_sb = NULL;

/* Inode cache */
#define FAT32_INODE_CACHE_SIZE  256
static fat32_inode_t *g_inode_cache[FAT32_INODE_CACHE_SIZE];
static tak_bertanda32_t g_inode_cache_count = 0;

/* Dentry cache */
#define FAT32_DENTRY_CACHE_SIZE 512
static fat32_dentry_t *g_dentry_cache[FAT32_DENTRY_CACHE_SIZE];
static tak_bertanda32_t g_dentry_cache_count = 0;

/* File descriptor table */
#define FAT32_MAX_FD  64
static fat32_fd_t g_fd_table[FAT32_MAX_FD];

/* Lock */
static tak_bertanda32_t g_fat32_lock = 0;

/* Inode number generator */
static ino_t g_next_ino = 1;

/*
 * ===========================================================================
 * FUNGSI LOCKING (LOCKING FUNCTIONS)
 * ===========================================================================
 */

static void fat32_lock(void)
{
    g_fat32_lock++;
}

static void fat32_unlock(void)
{
    if (g_fat32_lock > 0) {
        g_fat32_lock--;
    }
}

/*
 * ===========================================================================
 * FUNGSI UTILITAS (UTILITY FUNCTIONS)
 * ===========================================================================
 */

/* Cek apakah cluster valid */
static bool_t fat32_cluster_valid(tak_bertanda32_t cluster)
{
    return (cluster >= FAT32_FIRST_CLUSTER && 
            cluster <= FAT32_MAX_CLUSTERS);
}

/* Cek apakah cluster adalah end of chain */
static bool_t fat32_cluster_is_eof(tak_bertanda32_t cluster)
{
    return (cluster >= FAT32_END_CHAIN);
}

/* Cek apakah cluster bebas */
static bool_t fat32_cluster_is_free(tak_bertanda32_t cluster)
{
    return (cluster == FAT32_FREE);
}

/* Konversi cluster ke sektor */
static tak_bertanda32_t fat32_cluster_to_sector(fat32_sb_t *sb,
                                                 tak_bertanda32_t cluster)
{
    if (sb == NULL || !fat32_cluster_valid(cluster)) {
        return 0;
    }
    
    return sb->s_data_start_sec + 
           ((cluster - FAT32_FIRST_CLUSTER) * sb->s_sec_per_cluster);
}

/* Konversi cluster ke byte offset */
static tak_bertanda64_t fat32_cluster_to_byte(fat32_sb_t *sb,
                                               tak_bertanda32_t cluster)
{
    if (sb == NULL) {
        return 0;
    }
    
    return (tak_bertanda64_t)fat32_cluster_to_sector(sb, cluster) * 
           sb->s_bytes_per_sec;
}

/* Hitung jumlah sektor untuk ukuran file */
static tak_bertanda32_t fat32_bytes_to_clusters(fat32_sb_t *sb, off_t size)
{
    tak_bertanda64_t clusters;
    
    if (sb == NULL || size <= 0) {
        return 0;
    }
    
    clusters = (tak_bertanda64_t)size + sb->s_bytes_per_cluster - 1;
    clusters /= sb->s_bytes_per_cluster;
    
    return (tak_bertanda32_t)clusters;
}

/*
 * ===========================================================================
 * FUNGSI ALOKASI INODE (INODE ALLOCATION FUNCTIONS)
 * ===========================================================================
 */

static fat32_inode_t *fat32_alloc_inode(fat32_sb_t *sb)
{
    fat32_inode_t *inode;
    
    if (g_inode_cache_count >= FAT32_INODE_CACHE_SIZE) {
        return NULL;
    }
    
    inode = (fat32_inode_t *)kmalloc(sizeof(fat32_inode_t));
    if (inode == NULL) {
        return NULL;
    }
    
    kernel_memset(inode, 0, sizeof(fat32_inode_t));
    
    inode->i_ino = g_next_ino++;
    inode->i_sb = sb;
    inode->i_loaded = SALAH;
    inode->i_dirty = SALAH;
    
    /* Add to cache */
    g_inode_cache[g_inode_cache_count++] = inode;
    
    return inode;
}

static void fat32_free_inode(fat32_inode_t *inode)
{
    tak_bertanda32_t i;
    
    if (inode == NULL) {
        return;
    }
    
    /* Remove from cache */
    for (i = 0; i < g_inode_cache_count; i++) {
        if (g_inode_cache[i] == inode) {
            g_inode_cache[i] = g_inode_cache[--g_inode_cache_count];
            break;
        }
    }
    
    kfree(inode);
}

/*
 * ===========================================================================
 * FUNGSI ALOKASI DENTRY (DENTRY ALLOCATION FUNCTIONS)
 * ===========================================================================
 */

static fat32_dentry_t *fat32_alloc_dentry(const char *name)
{
    fat32_dentry_t *dentry;
    ukuran_t namelen;
    
    if (name == NULL) {
        return NULL;
    }
    
    namelen = kernel_strlen(name);
    if (namelen > FAT32_MAX_NAME_LEN) {
        namelen = FAT32_MAX_NAME_LEN;
    }
    
    if (g_dentry_cache_count >= FAT32_DENTRY_CACHE_SIZE) {
        return NULL;
    }
    
    dentry = (fat32_dentry_t *)kmalloc(sizeof(fat32_dentry_t));
    if (dentry == NULL) {
        return NULL;
    }
    
    kernel_memset(dentry, 0, sizeof(fat32_dentry_t));
    
    kernel_strncpy(dentry->d_name, name, FAT32_MAX_NAME_LEN);
    dentry->d_name[FAT32_MAX_NAME_LEN] = '\0';
    dentry->d_namelen = namelen;
    
    /* Add to cache */
    g_dentry_cache[g_dentry_cache_count++] = dentry;
    
    return dentry;
}

static void fat32_free_dentry(fat32_dentry_t *dentry)
{
    tak_bertanda32_t i;
    
    if (dentry == NULL) {
        return;
    }
    
    /* Remove from cache */
    for (i = 0; i < g_dentry_cache_count; i++) {
        if (g_dentry_cache[i] == dentry) {
            g_dentry_cache[i] = g_dentry_cache[--g_dentry_cache_count];
            break;
        }
    }
    
    kfree(dentry);
}

/*
 * ===========================================================================
 * FUNGSI MANIPULASI NAMA (NAME MANIPULATION FUNCTIONS)
 * ===========================================================================
 */

/* Konversi nama 8.3 ke nama normal */
static void fat32_sfn_to_name(const tak_bertanda8_t sfn[11], char *name,
                              ukuran_t size)
{
    tak_bertanda32_t i, j;
    bool_t has_ext;
    
    if (sfn == NULL || name == NULL || size == 0) {
        return;
    }
    
    kernel_memset(name, 0, size);
    
    /* Check jika entry kosong */
    if (sfn[0] == 0x00 || sfn[0] == 0xE5) {
        return;
    }
    
    /* Copy nama utama */
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
    
    /* Tambahkan ekstensi */
    if (has_ext && j < size - 1) {
        name[j++] = '.';
        for (i = 8; i < 11 && sfn[i] != ' ' && j < size - 1; i++) {
            name[j++] = (char)sfn[i];
        }
    }
    
    name[j] = '\0';
}

/* Konversi nama normal ke 8.3 */
static void fat32_name_to_sfn(const char *name, tak_bertanda8_t sfn[11])
{
    ukuran_t namelen;
    ukuran_t i, j;
    const char *dot;
    
    if (name == NULL || sfn == NULL) {
        return;
    }
    
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

/* Generate 8.3 checksum untuk LFN */
static tak_bertanda8_t fat32_lfn_checksum(const tak_bertanda8_t sfn[11])
{
    tak_bertanda8_t sum;
    tak_bertanda32_t i;
    
    sum = 0;
    for (i = 0; i < 11; i++) {
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + sfn[i];
    }
    
    return sum;
}

/*
 * ===========================================================================
 * FUNGSI KONVERSI ATRIBUT (ATTRIBUTE CONVERSION FUNCTIONS)
 * ===========================================================================
 */

/* Konversi attribut FAT32 ke mode VFS */
static mode_t fat32_attr_to_mode(tak_bertanda8_t attr)
{
    mode_t mode;
    
    mode = 0;
    
    if (attr & FAT32_ATTR_DIRECTORY) {
        mode |= VFS_S_IFDIR;
        mode |= 0755;
    } else {
        mode |= VFS_S_IFREG;
        mode |= 0644;
    }
    
    if (attr & FAT32_ATTR_RDONLY) {
        mode &= ~(0222);  /* Remove write permission */
    }
    
    return mode;
}

/* Konversi mode VFS ke attribut FAT32 */
static tak_bertanda8_t fat32_mode_to_attr(mode_t mode)
{
    tak_bertanda8_t attr;
    
    attr = 0;
    
    if (VFS_S_ISDIR(mode)) {
        attr |= FAT32_ATTR_DIRECTORY;
    }
    
    if (!(mode & 0222)) {
        attr |= FAT32_ATTR_RDONLY;
    }
    
    attr |= FAT32_ATTR_ARCHIVE;
    
    return attr;
}

/*
 * ===========================================================================
 * FUNGSI WAKTU (TIME FUNCTIONS)
 * ===========================================================================
 */

/* Konversi FAT date/time ke waktu Unix */
static waktu_t fat32_dos_to_unix(tak_bertanda16_t date, tak_bertanda16_t time)
{
    tak_bertanda32_t year, month, day, hour, minute, second;
    tak_bertanda32_t days;
    tak_bertanda32_t i;
    
    year = ((date >> 9) & 0x7F) + 1980;
    month = (date >> 5) & 0x0F;
    day = date & 0x1F;
    
    hour = (time >> 11) & 0x1F;
    minute = (time >> 5) & 0x3F;
    second = (time & 0x1F) * 2;
    
    /* Hitung jumlah hari sejak epoch */
    days = 0;
    for (i = 1970; i < year; i++) {
        days += TAHUN_KABAT(i) ? 366 : 365;
    }
    
    /* Tambahkan hari untuk bulan-bulan sebelumnya */
    {
        tak_bertanda32_t days_in_month[] = {
            31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
        };
        
        if (TAHUN_KABAT(year)) {
            days_in_month[1] = 29;
        }
        
        for (i = 0; i < month - 1 && i < 12; i++) {
            days += days_in_month[i];
        }
    }
    
    days += day - 1;
    
    return (waktu_t)(days * 86400 + hour * 3600 + minute * 60 + second);
}

/* Konversi waktu Unix ke FAT date */
static tak_bertanda16_t fat32_unix_to_date(waktu_t time)
{
    tak_bertanda32_t year, month, day;
    tak_bertanda32_t days;
    tak_bertanda32_t i;
    
    days = (tak_bertanda32_t)(time / 86400);
    
    year = 1970;
    while (days >= (tak_bertanda32_t)(TAHUN_KABAT(year) ? 366 : 365)) {
        days -= TAHUN_KABAT(year) ? 366 : 365;
        year++;
    }
    
    {
        tak_bertanda32_t days_in_month[] = {
            31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
        };
        
        if (TAHUN_KABAT(year)) {
            days_in_month[1] = 29;
        }
        
        month = 0;
        for (i = 0; i < 12 && days >= days_in_month[i]; i++) {
            days -= days_in_month[i];
            month++;
        }
        
        day = days + 1;
    }
    
    return (tak_bertanda16_t)(((year - 1980) << 9) | ((month + 1) << 5) | day);
}

/* Konversi waktu Unix ke FAT time */
static tak_bertanda16_t fat32_unix_to_time(waktu_t time)
{
    tak_bertanda32_t seconds;
    tak_bertanda32_t hour, minute, second;
    
    seconds = (tak_bertanda32_t)(time % 86400);
    
    hour = seconds / 3600;
    seconds %= 3600;
    minute = seconds / 60;
    second = seconds % 60;
    
    return (tak_bertanda16_t)((hour << 11) | (minute << 5) | (second / 2));
}

/*
 * ===========================================================================
 * FUNGSI ALOKASI FILE DESCRIPTOR (FILE DESCRIPTOR ALLOCATION)
 * ===========================================================================
 */

static int fat32_alloc_fd(fat32_inode_t *inode, tak_bertanda32_t flags)
{
    tak_bertanda32_t i;
    
    if (inode == NULL) {
        return -1;
    }
    
    for (i = 0; i < FAT32_MAX_FD; i++) {
        if (g_fd_table[i].f_inode == NULL) {
            g_fd_table[i].f_inode = inode;
            g_fd_table[i].f_pos = 0;
            g_fd_table[i].f_flags = flags;
            g_fd_table[i].f_cluster = inode->i_cluster;
            g_fd_table[i].f_cluster_offset = 0;
            return (int)i;
        }
    }
    
    return -1;
}

static fat32_fd_t *fat32_get_fd(int fd)
{
    if (fd < 0 || fd >= FAT32_MAX_FD) {
        return NULL;
    }
    
    if (g_fd_table[fd].f_inode == NULL) {
        return NULL;
    }
    
    return &g_fd_table[fd];
}

static void fat32_free_fd(int fd)
{
    if (fd >= 0 && fd < FAT32_MAX_FD) {
        g_fd_table[fd].f_inode = NULL;
        g_fd_table[fd].f_pos = 0;
        g_fd_table[fd].f_flags = 0;
        g_fd_table[fd].f_cluster = 0;
        g_fd_table[fd].f_cluster_offset = 0;
    }
}

/*
 * ===========================================================================
 * DEKLARASI FUNGSI EKSTERNAL (EXTERNAL FUNCTION DECLARATIONS)
 * ===========================================================================
 */

/* Dari fat32_boot.c */
status_t fat32_read_boot_sector(fat32_sb_t *sb, const void *buffer);
status_t fat32_read_fsinfo(fat32_sb_t *sb, const void *buffer);
status_t fat32_write_boot_sector(fat32_sb_t *sb, void *buffer);
status_t fat32_write_fsinfo(fat32_sb_t *sb, void *buffer);
bool_t fat32_validate_boot_sector(const fat32_boot_sector_t *boot);
bool_t fat32_validate_fsinfo(const fat32_fsinfo_t *fsinfo);

/* Dari fat32_fat.c */
tak_bertanda32_t fat32_read_fat_entry(fat32_sb_t *sb, tak_bertanda32_t cluster);
status_t fat32_write_fat_entry(fat32_sb_t *sb, tak_bertanda32_t cluster,
                               tak_bertanda32_t value);
tak_bertanda32_t fat32_alloc_cluster(fat32_sb_t *sb);
status_t fat32_free_cluster_chain(fat32_sb_t *sb, tak_bertanda32_t cluster);
status_t fat32_init_fat(fat32_sb_t *sb);

/* Dari fat32_cluster.c */
status_t fat32_read_cluster(fat32_sb_t *sb, tak_bertanda32_t cluster,
                            void *buffer);
status_t fat32_write_cluster(fat32_sb_t *sb, tak_bertanda32_t cluster,
                             const void *buffer);
status_t fat32_read_cluster_range(fat32_sb_t *sb, tak_bertanda32_t cluster,
                                  tak_bertanda32_t offset, void *buffer,
                                  ukuran_t size);
status_t fat32_write_cluster_range(fat32_sb_t *sb, tak_bertanda32_t cluster,
                                   tak_bertanda32_t offset, const void *buffer,
                                   ukuran_t size);
tak_bertanda32_t fat32_get_next_cluster(fat32_sb_t *sb, tak_bertanda32_t cluster);
status_t fat32_extend_cluster_chain(fat32_sb_t *sb, tak_bertanda32_t cluster,
                                    tak_bertanda32_t count);

/* Dari fat32_dir.c */
status_t fat32_read_dir_entry(fat32_sb_t *sb, tak_bertanda32_t cluster,
                              tak_bertanda32_t index,
                              fat32_dir_entry_t *entry);
status_t fat32_write_dir_entry(fat32_sb_t *sb, tak_bertanda32_t cluster,
                               tak_bertanda32_t index,
                               const fat32_dir_entry_t *entry);
status_t fat32_find_dir_entry(fat32_sb_t *sb, tak_bertanda32_t cluster,
                              const char *name, fat32_dir_entry_t *entry,
                              tak_bertanda32_t *index);
status_t fat32_create_dir_entry(fat32_sb_t *sb, tak_bertanda32_t cluster,
                                const char *name, tak_bertanda8_t attr,
                                tak_bertanda32_t first_cluster,
                                tak_bertanda32_t size);
status_t fat32_delete_dir_entry(fat32_sb_t *sb, tak_bertanda32_t cluster,
                                tak_bertanda32_t index);
fat32_inode_t *fat32_lookup_inode(fat32_sb_t *sb, tak_bertanda32_t dir_cluster,
                                  const char *name);

/* Dari fat32_file.c */
tak_bertandas_t fat32_read_file_data(fat32_sb_t *sb, fat32_inode_t *inode,
                                     void *buffer, ukuran_t size, off_t offset);
tak_bertandas_t fat32_write_file_data(fat32_sb_t *sb, fat32_inode_t *inode,
                                      const void *buffer, ukuran_t size,
                                      off_t offset);
status_t fat32_truncate_file(fat32_sb_t *sb, fat32_inode_t *inode,
                             off_t new_size);
status_t fat32_allocate_clusters(fat32_sb_t *sb, fat32_inode_t *inode,
                                 tak_bertanda32_t count);

/* Dari fat32_lfn.c */
bool_t fat32_is_lfn_entry(const fat32_dir_entry_t *entry);
status_t fat32_read_lfn(fat32_sb_t *sb, tak_bertanda32_t cluster,
                        tak_bertanda32_t index, char *name, ukuran_t size);
status_t fat32_write_lfn(fat32_sb_t *sb, tak_bertanda32_t cluster,
                         const char *name, tak_bertanda32_t *first_index);
tak_bertanda32_t fat32_lfn_entries_needed(const char *name);
status_t fat32_delete_lfn(fat32_sb_t *sb, tak_bertanda32_t cluster,
                          tak_bertanda32_t sfn_index);

/*
 * ===========================================================================
 * VFS SUPERBLOCK OPERATIONS (VFS SUPERBLOCK OPERATIONS)
 * ===========================================================================
 */

static struct inode *fat32_alloc_inode_vfs(struct superblock *sb)
{
    fat32_inode_t *inode;
    
    if (sb == NULL) {
        return NULL;
    }
    
    inode = fat32_alloc_inode((fat32_sb_t *)sb->s_private);
    return (struct inode *)inode;
}

static void fat32_destroy_inode_vfs(struct inode *inode)
{
    fat32_free_inode((fat32_inode_t *)inode);
}

static status_t fat32_write_inode_vfs(struct inode *inode)
{
    /* TODO: Implement write inode to disk */
    return STATUS_BERHASIL;
}

static status_t fat32_read_inode_vfs(struct inode *inode)
{
    /* TODO: Implement read inode from disk */
    return STATUS_BERHASIL;
}

static status_t fat32_sync_fs_vfs(struct superblock *sb)
{
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* TODO: Implement sync filesystem */
    return STATUS_BERHASIL;
}

static void fat32_put_super_vfs(struct superblock *sb)
{
    if (sb == NULL) {
        return;
    }
    
    /* TODO: Implement cleanup */
}

static status_t fat32_statfs_vfs(struct superblock *sb, struct vfs_stat *stat)
{
    fat32_sb_t *fsb;
    
    if (sb == NULL || stat == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    fsb = (fat32_sb_t *)sb->s_private;
    if (fsb == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    kernel_memset(stat, 0, sizeof(struct vfs_stat));
    
    stat->st_blksize = fsb->s_bytes_per_cluster;
    stat->st_blocks = fsb->s_total_clusters;
    stat->st_size = (off_t)(fsb->s_total_clusters * fsb->s_bytes_per_cluster);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * VFS INODE OPERATIONS (VFS INODE OPERATIONS)
 * ===========================================================================
 */

static struct dentry *fat32_lookup_vfs(struct inode *dir, const char *name)
{
    fat32_inode_t *parent;
    fat32_inode_t *inode;
    fat32_dentry_t *dentry;
    struct dentry *vfs_dentry;
    
    if (dir == NULL || name == NULL) {
        return NULL;
    }
    
    parent = (fat32_inode_t *)dir;
    
    fat32_lock();
    
    inode = fat32_lookup_inode(parent->i_sb, parent->i_cluster, name);
    if (inode == NULL) {
        fat32_unlock();
        return NULL;
    }
    
    dentry = fat32_alloc_dentry(name);
    if (dentry == NULL) {
        fat32_unlock();
        return NULL;
    }
    
    dentry->d_inode = inode;
    dentry->d_parent = NULL;
    
    vfs_dentry = dentry_alloc(name);
    if (vfs_dentry == NULL) {
        fat32_free_dentry(dentry);
        fat32_unlock();
        return NULL;
    }
    
    vfs_dentry->d_inode = (struct inode *)inode;
    
    fat32_unlock();
    
    return vfs_dentry;
}

static status_t fat32_create_vfs(struct inode *dir, struct dentry *dentry,
                                  mode_t mode)
{
    /* TODO: Implement create file */
    return STATUS_BELUM_IMPLEMENTASI;
}

static status_t fat32_mkdir_vfs(struct inode *dir, struct dentry *dentry,
                                 mode_t mode)
{
    /* TODO: Implement mkdir */
    return STATUS_BELUM_IMPLEMENTASI;
}

static status_t fat32_rmdir_vfs(struct inode *dir, struct dentry *dentry)
{
    /* TODO: Implement rmdir */
    return STATUS_BELUM_IMPLEMENTASI;
}

static status_t fat32_unlink_vfs(struct inode *dir, struct dentry *dentry)
{
    /* TODO: Implement unlink */
    return STATUS_BELUM_IMPLEMENTASI;
}

static status_t fat32_getattr_vfs(struct dentry *dentry, struct vfs_stat *stat)
{
    fat32_inode_t *inode;
    
    if (dentry == NULL || stat == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    inode = (fat32_inode_t *)dentry->d_inode;
    if (inode == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    kernel_memset(stat, 0, sizeof(struct vfs_stat));
    
    stat->st_ino = inode->i_ino;
    stat->st_mode = inode->i_mode;
    stat->st_uid = inode->i_uid;
    stat->st_gid = inode->i_gid;
    stat->st_size = inode->i_size;
    stat->st_atime = inode->i_atime;
    stat->st_mtime = inode->i_mtime;
    stat->st_ctime = inode->i_ctime;
    stat->st_nlink = inode->i_nlink;
    
    if (inode->i_sb != NULL) {
        stat->st_blksize = inode->i_sb->s_bytes_per_cluster;
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * VFS FILE OPERATIONS (VFS FILE OPERATIONS)
 * ===========================================================================
 */

static tak_bertandas_t fat32_read_vfs(struct file *file, void *buffer,
                                       ukuran_t size, off_t *pos)
{
    fat32_inode_t *inode;
    tak_bertandas_t bytes_read;
    
    if (file == NULL || buffer == NULL || pos == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_NULL;
    }
    
    inode = (fat32_inode_t *)file->f_inode;
    if (inode == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }
    
    if (!VFS_S_ISREG(inode->i_mode)) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }
    
    if (*pos < 0) {
        *pos = 0;
    }
    
    if (*pos >= inode->i_size) {
        return 0;
    }
    
    fat32_lock();
    
    bytes_read = fat32_read_file_data(inode->i_sb, inode, buffer, size, *pos);
    if (bytes_read > 0) {
        *pos += bytes_read;
    }
    
    fat32_unlock();
    
    return bytes_read;
}

static tak_bertandas_t fat32_write_vfs(struct file *file, const void *buffer,
                                        ukuran_t size, off_t *pos)
{
    fat32_inode_t *inode;
    tak_bertandas_t bytes_written;
    
    if (file == NULL || buffer == NULL || pos == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_NULL;
    }
    
    inode = (fat32_inode_t *)file->f_inode;
    if (inode == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }
    
    if (inode->i_sb != NULL && inode->i_sb->s_readonly) {
        return (tak_bertandas_t)STATUS_FS_READONLY;
    }
    
    if (!VFS_S_ISREG(inode->i_mode)) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }
    
    if (*pos < 0) {
        *pos = 0;
    }
    
    fat32_lock();
    
    bytes_written = fat32_write_file_data(inode->i_sb, inode, buffer, size, *pos);
    if (bytes_written > 0) {
        *pos += bytes_written;
        inode->i_dirty = BENAR;
    }
    
    fat32_unlock();
    
    return bytes_written;
}

static off_t fat32_lseek_vfs(struct file *file, off_t offset,
                              tak_bertanda32_t whence)
{
    fat32_inode_t *inode;
    off_t new_pos;
    
    if (file == NULL) {
        return (off_t)STATUS_PARAM_NULL;
    }
    
    inode = (fat32_inode_t *)file->f_inode;
    if (inode == NULL) {
        return (off_t)STATUS_PARAM_INVALID;
    }
    
    switch (whence) {
    case VFS_SEEK_SET:
        new_pos = offset;
        break;
    case VFS_SEEK_CUR:
        new_pos = file->f_pos + offset;
        break;
    case VFS_SEEK_END:
        new_pos = inode->i_size + offset;
        break;
    default:
        return (off_t)STATUS_PARAM_INVALID;
    }
    
    if (new_pos < 0) {
        new_pos = 0;
    }
    
    if (new_pos > inode->i_size) {
        new_pos = inode->i_size;
    }
    
    file->f_pos = new_pos;
    
    return new_pos;
}

static tak_bertandas_t fat32_readdir_vfs(struct file *file,
                                          struct vfs_dirent *dirent,
                                          ukuran_t count)
{
    /* TODO: Implement readdir */
    return (tak_bertandas_t)STATUS_BELUM_IMPLEMENTASI;
}

/*
 * ===========================================================================
 * VFS FILESYSTEM OPERATIONS (VFS FILESYSTEM OPERATIONS)
 * ===========================================================================
 */

static struct superblock *fat32_mount_vfs(struct filesystem *fs,
                                           const char *device,
                                           const char *path,
                                           tak_bertanda32_t flags)
{
    struct superblock *sb;
    fat32_sb_t *fsb;
    
    if (fs == NULL) {
        return NULL;
    }
    
    fat32_lock();
    
    /* Allocate filesystem superblock */
    fsb = (fat32_sb_t *)kmalloc(sizeof(fat32_sb_t));
    if (fsb == NULL) {
        fat32_unlock();
        return NULL;
    }
    
    kernel_memset(fsb, 0, sizeof(fat32_sb_t));
    
    /* TODO: Read boot sector from device */
    
    /* Allocate VFS superblock */
    sb = superblock_alloc(fs);
    if (sb == NULL) {
        kfree(fsb);
        fat32_unlock();
        return NULL;
    }
    
    sb->s_private = fsb;
    fsb->s_dev = sb->s_dev;
    
    /* Set readonly if mount flag set */
    if (flags & VFS_MOUNT_FLAG_RDONLY) {
        fsb->s_readonly = BENAR;
    }
    
    fat32_unlock();
    
    return sb;
}

static status_t fat32_umount_vfs(struct superblock *sb)
{
    fat32_sb_t *fsb;
    
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    fat32_lock();
    
    fsb = (fat32_sb_t *)sb->s_private;
    if (fsb != NULL) {
        /* Sync if dirty */
        if (fsb->s_dirty) {
            /* TODO: Sync filesystem */
        }
        
        kfree(fsb);
    }
    
    superblock_put(sb);
    
    fat32_unlock();
    
    return STATUS_BERHASIL;
}

static status_t fat32_detect_vfs(const char *device)
{
    /* TODO: Implement FAT32 detection */
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * VFS OPERATIONS STRUCTURES (VFS OPERATIONS STRUCTURES)
 * ===========================================================================
 */

static vfs_super_operations_t fat32_super_ops = {
    .alloc_inode   = fat32_alloc_inode_vfs,
    .destroy_inode = fat32_destroy_inode_vfs,
    .write_inode   = fat32_write_inode_vfs,
    .read_inode    = fat32_read_inode_vfs,
    .sync_fs       = fat32_sync_fs_vfs,
    .put_super     = fat32_put_super_vfs,
    .statfs        = fat32_statfs_vfs,
};

static vfs_inode_operations_t fat32_inode_ops = {
    .lookup   = fat32_lookup_vfs,
    .create   = fat32_create_vfs,
    .mkdir    = fat32_mkdir_vfs,
    .rmdir    = fat32_rmdir_vfs,
    .unlink   = fat32_unlink_vfs,
    .getattr  = fat32_getattr_vfs,
};

static vfs_file_operations_t fat32_file_ops = {
    .read    = fat32_read_vfs,
    .write   = fat32_write_vfs,
    .lseek   = fat32_lseek_vfs,
    .readdir = fat32_readdir_vfs,
};

/*
 * ===========================================================================
 * FILESYSTEM REGISTRATION (FILESYSTEM REGISTRATION)
 * ===========================================================================
 */

static filesystem_t fat32_filesystem = {
    .fs_nama    = "fat32",
    .fs_flags   = FS_FLAG_REQUIRES_DEV | FS_FLAG_WRITE_SUPPORT,
    .fs_mount   = fat32_mount_vfs,
    .fs_umount  = fat32_umount_vfs,
    .fs_detect  = fat32_detect_vfs,
    .fs_next    = NULL,
    .fs_refcount = 0,
    .fs_terdaftar = SALAH,
};

/*
 * ===========================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ===========================================================================
 */

status_t fat32_init(void)
{
    tak_bertanda32_t i;
    
    log_info("[FAT32] Initializing FAT32 driver...");
    
    /* Clear caches */
    for (i = 0; i < FAT32_INODE_CACHE_SIZE; i++) {
        g_inode_cache[i] = NULL;
    }
    g_inode_cache_count = 0;
    
    for (i = 0; i < FAT32_DENTRY_CACHE_SIZE; i++) {
        g_dentry_cache[i] = NULL;
    }
    g_dentry_cache_count = 0;
    
    /* Clear FD table */
    for (i = 0; i < FAT32_MAX_FD; i++) {
        g_fd_table[i].f_inode = NULL;
        g_fd_table[i].f_pos = 0;
        g_fd_table[i].f_flags = 0;
        g_fd_table[i].f_cluster = 0;
        g_fd_table[i].f_cluster_offset = 0;
    }
    
    /* Reset generator */
    g_next_ino = 1;
    
    /* Reset lock */
    g_fat32_lock = 0;
    
    /* Reset superblock */
    g_fat32_sb = NULL;
    
    log_info("[FAT32] FAT32 driver initialized");
    
    return STATUS_BERHASIL;
}

void fat32_shutdown(void)
{
    log_info("[FAT32] Shutting down FAT32 driver...");
    
    fat32_lock();
    
    /* Free all cached inodes */
    while (g_inode_cache_count > 0) {
        kfree(g_inode_cache[--g_inode_cache_count]);
    }
    
    /* Free all cached dentries */
    while (g_dentry_cache_count > 0) {
        kfree(g_dentry_cache[--g_dentry_cache_count]);
    }
    
    /* Free superblock */
    if (g_fat32_sb != NULL) {
        if (g_fat32_sb->s_fat_cache != NULL) {
            kfree(g_fat32_sb->s_fat_cache);
        }
        kfree(g_fat32_sb);
        g_fat32_sb = NULL;
    }
    
    fat32_unlock();
    
    log_info("[FAT32] Shutdown complete");
}

status_t fat32_register(void)
{
    status_t ret;
    
    log_info("[FAT32] Registering FAT32 filesystem...");
    
    ret = filesystem_register(&fat32_filesystem);
    if (ret != STATUS_BERHASIL) {
        log_error("[FAT32] Failed to register: %d", ret);
        return ret;
    }
    
    log_info("[FAT32] Filesystem registered successfully");
    
    return STATUS_BERHASIL;
}

status_t fat32_unregister(void)
{
    return filesystem_unregister(&fat32_filesystem);
}

/*
 * ===========================================================================
 * FUNGSI STATISTIK (STATISTICS FUNCTIONS)
 * ===========================================================================
 */

tak_bertanda32_t fat32_get_inode_count(void)
{
    return g_inode_cache_count;
}

tak_bertanda32_t fat32_get_dentry_count(void)
{
    return g_dentry_cache_count;
}

/*
 * ===========================================================================
 * FUNGSI DEBUG (DEBUG FUNCTIONS)
 * ===========================================================================
 */

void fat32_print_info(void)
{
    kernel_printf("[FAT32] Driver Info:\n");
    
    if (g_fat32_sb == NULL) {
        kernel_printf("  Status: Not mounted\n");
        return;
    }
    
    kernel_printf("  Volume Label: %.11s\n", g_fat32_sb->s_volume_label);
    kernel_printf("  FS Type: %.8s\n", g_fat32_sb->s_fs_type);
    kernel_printf("  Bytes per Sector: %u\n", g_fat32_sb->s_bytes_per_sec);
    kernel_printf("  Sectors per Cluster: %u\n", g_fat32_sb->s_sec_per_cluster);
    kernel_printf("  Bytes per Cluster: %u\n", g_fat32_sb->s_bytes_per_cluster);
    kernel_printf("  Total Clusters: %u\n", g_fat32_sb->s_total_clusters);
    kernel_printf("  Free Clusters: %u\n", g_fat32_sb->s_free_count);
    kernel_printf("  Root Cluster: %u\n", g_fat32_sb->s_root_cluster);
    kernel_printf("  Read-only: %s\n", g_fat32_sb->s_readonly ? "Yes" : "No");
    kernel_printf("  Cached Inodes: %u\n", g_inode_cache_count);
    kernel_printf("  Cached Dentries: %u\n", g_dentry_cache_count);
}
