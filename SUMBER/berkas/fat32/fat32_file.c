/*
 * PIGURA OS - FAT32_FILE.C
 * ==========================
 * Implementasi operasi file untuk filesystem FAT32.
 *
 * Berkas ini berisi fungsi-fungsi untuk membaca, menulis, dan
 * mengelola file pada filesystem FAT32.
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
 * KONSTANTA FILE FAT32 (FAT32 FILE CONSTANTS)
 * ===========================================================================
 */

/* Special cluster numbers */
#define FAT32_FIRST_CLUSTER     2
#define FAT32_END_CHAIN         0x0FFFFFFF
#define FAT32_CLUSTER_MASK      0x0FFFFFFF

/* File open modes */
#define FAT32_OPEN_READ         0x01
#define FAT32_OPEN_WRITE        0x02
#define FAT32_OPEN_RDWR         0x03
#define FAT32_OPEN_CREATE       0x04
#define FAT32_OPEN_APPEND       0x08
#define FAT32_OPEN_TRUNCATE     0x10

/* Seek modes */
#define FAT32_SEEK_SET          0
#define FAT32_SEEK_CUR          1
#define FAT32_SEEK_END          2

/* Attribute flags */
#define FAT32_ATTR_RDONLY       0x01
#define FAT32_ATTR_DIRECTORY    0x10

/*
 * ===========================================================================
 * STRUKTUR FILE INTERNAL (INTERNAL FILE STRUCTURES)
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
    fat32_sb_t     *i_sb;
    bool_t          i_dirty;
    bool_t          i_loaded;
} fat32_inode_t;

typedef struct fat32_file {
    fat32_inode_t  *f_inode;
    off_t           f_pos;
    tak_bertanda32_t f_flags;
    tak_bertanda32_t f_current_cluster;
    tak_bertanda32_t f_cluster_offset;
} fat32_file_t;

/*
 * ===========================================================================
 * FORWARD DECLARATIONS (FROM OTHER MODULES)
 * ===========================================================================
 */

/* From fat32_fat.c */
tak_bertanda32_t fat32_read_fat_entry(fat32_sb_t *sb, tak_bertanda32_t cluster);
status_t fat32_write_fat_entry(fat32_sb_t *sb, tak_bertanda32_t cluster,
                               tak_bertanda32_t value);
tak_bertanda32_t fat32_alloc_cluster(fat32_sb_t *sb);
status_t fat32_free_cluster_chain(fat32_sb_t *sb, tak_bertanda32_t cluster);

/* From fat32_cluster.c */
tak_bertanda32_t fat32_cluster_to_sector(fat32_sb_t *sb, 
                                          tak_bertanda32_t cluster);
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
tak_bertanda32_t fat32_get_next_cluster(fat32_sb_t *sb, 
                                         tak_bertanda32_t cluster);
tak_bertanda32_t fat32_find_last_cluster(fat32_sb_t *sb,
                                          tak_bertanda32_t first_cluster);
tak_bertanda32_t fat32_count_clusters(fat32_sb_t *sb,
                                       tak_bertanda32_t first_cluster);
tak_bertanda32_t fat32_bytes_to_clusters(fat32_sb_t *sb, 
                                          tak_bertanda64_t size);

/* From fat32_dir.c */
status_t fat32_find_dir_entry(fat32_sb_t *sb, tak_bertanda32_t dir_cluster,
                              const char *name, void *entry,
                              tak_bertanda32_t *index);
status_t fat32_create_dir_entry(fat32_sb_t *sb, tak_bertanda32_t dir_cluster,
                                const char *name, tak_bertanda8_t attr,
                                tak_bertanda32_t first_cluster,
                                tak_bertanda32_t size);
status_t fat32_delete_dir_entry(fat32_sb_t *sb, tak_bertanda32_t dir_cluster,
                                tak_bertanda32_t index);

/*
 * ===========================================================================
 * FUNGSI PEMBACAAN FILE (FILE READ FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_read_file_data
 * --------------------
 * Membaca data dari file pada offset tertentu.
 *
 * Parameter:
 *   sb     - Pointer ke struktur superblock FAT32
 *   inode  - Pointer ke inode file
 *   buffer - Buffer tujuan
 *   size   - Jumlah byte yang akan dibaca
 *   offset - Offset dalam file
 *
 * Return: Jumlah byte yang dibaca, atau error code negatif
 */
tak_bertandas_t fat32_read_file_data(fat32_sb_t *sb, fat32_inode_t *inode,
                                      void *buffer, ukuran_t size,
                                      off_t offset)
{
    tak_bertanda32_t cluster_size;
    tak_bertanda32_t current_cluster;
    tak_bertanda32_t cluster_offset;
    tak_bertanda32_t bytes_in_cluster;
    tak_bertanda32_t bytes_to_read;
    tak_bertanda32_t bytes_read;
    tak_bertanda8_t *buf_ptr;
    off_t file_offset;
    ukuran_t remaining;
    
    if (sb == NULL || inode == NULL || buffer == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_NULL;
    }
    
    /* Cek jika offset melebihi ukuran file */
    if (offset >= inode->i_size) {
        return 0;
    }
    
    /* Sesuaikan size jika melebihi sisa file */
    if (offset + (off_t)size > inode->i_size) {
        size = (ukuran_t)(inode->i_size - offset);
    }
    
    cluster_size = sb->s_bytes_per_cluster;
    buf_ptr = (tak_bertanda8_t *)buffer;
    bytes_read = 0;
    remaining = size;
    file_offset = offset;
    
    /* Cari cluster awal */
    current_cluster = inode->i_cluster;
    cluster_offset = (tak_bertanda32_t)(file_offset / cluster_size);
    
    /* Traverse ke cluster yang benar */
    {
        tak_bertanda32_t i;
        for (i = 0; i < cluster_offset && current_cluster >= FAT32_FIRST_CLUSTER; i++) {
            current_cluster = fat32_get_next_cluster(sb, current_cluster);
        }
    }
    
    /* Hitung offset dalam cluster */
    cluster_offset = (tak_bertanda32_t)(file_offset % cluster_size);
    
    /* Baca data dari cluster */
    while (remaining > 0 && current_cluster >= FAT32_FIRST_CLUSTER) {
        /* Hitung bytes yang tersedia dalam cluster ini */
        bytes_in_cluster = cluster_size - cluster_offset;
        bytes_to_read = (remaining > bytes_in_cluster) ? bytes_in_cluster : remaining;
        
        /* Baca dari cluster */
        if (fat32_read_cluster_range(sb, current_cluster, cluster_offset,
                                     buf_ptr, bytes_to_read) != STATUS_BERHASIL) {
            break;
        }
        
        buf_ptr += bytes_to_read;
        bytes_read += bytes_to_read;
        remaining -= bytes_to_read;
        file_offset += bytes_to_read;
        
        /* Pindah ke cluster berikutnya */
        cluster_offset = 0;
        current_cluster = fat32_get_next_cluster(sb, current_cluster);
    }
    
    /* Update access time */
    inode->i_atime = kernel_get_uptime();
    
    return (tak_bertandas_t)bytes_read;
}

/*
 * ===========================================================================
 * FUNGSI PENULISAN FILE (FILE WRITE FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_write_file_data
 * ---------------------
 * Menulis data ke file pada offset tertentu.
 *
 * Parameter:
 *   sb     - Pointer ke struktur superblock FAT32
 *   inode  - Pointer ke inode file
 *   buffer - Buffer sumber
 *   size   - Jumlah byte yang akan ditulis
 *   offset - Offset dalam file
 *
 * Return: Jumlah byte yang ditulis, atau error code negatif
 */
tak_bertandas_t fat32_write_file_data(fat32_sb_t *sb, fat32_inode_t *inode,
                                       const void *buffer, ukuran_t size,
                                       off_t offset)
{
    tak_bertanda32_t cluster_size;
    tak_bertanda32_t current_cluster;
    tak_bertanda32_t last_cluster;
    tak_bertanda32_t cluster_offset;
    tak_bertanda32_t bytes_in_cluster;
    tak_bertanda32_t bytes_to_write;
    tak_bertanda32_t bytes_written;
    tak_bertanda32_t clusters_needed;
    tak_bertanda32_t clusters_current;
    tak_bertanda32_t clusters_to_alloc;
    tak_bertanda32_t i;
    const tak_bertanda8_t *buf_ptr;
    off_t new_size;
    ukuran_t remaining;
    
    if (sb == NULL || inode == NULL || buffer == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_NULL;
    }
    
    /* Cek read-only */
    if (sb->s_readonly) {
        return (tak_bertandas_t)STATUS_FS_READONLY;
    }
    
    /* Cek read-only attribute */
    if (inode->i_attr & FAT32_ATTR_RDONLY) {
        return (tak_bertandas_t)STATUS_AKSES_DITOLAK;
    }
    
    cluster_size = sb->s_bytes_per_cluster;
    buf_ptr = (const tak_bertanda8_t *)buffer;
    bytes_written = 0;
    remaining = size;
    
    /* Hitung ukuran baru */
    new_size = offset + (off_t)size;
    
    /* Perlu extend file? */
    if (new_size > inode->i_size) {
        clusters_needed = fat32_bytes_to_clusters(sb, (tak_bertanda64_t)new_size);
        clusters_current = fat32_count_clusters(sb, inode->i_cluster);
        
        if (clusters_needed > clusters_current) {
            clusters_to_alloc = clusters_needed - clusters_current;
            
            /* Cari cluster terakhir */
            if (inode->i_cluster >= FAT32_FIRST_CLUSTER) {
                last_cluster = fat32_find_last_cluster(sb, inode->i_cluster);
            } else {
                last_cluster = 0;
            }
            
            /* Alokasikan cluster baru */
            for (i = 0; i < clusters_to_alloc; i++) {
                tak_bertanda32_t new_cluster;
                
                new_cluster = fat32_alloc_cluster(sb);
                if (new_cluster == 0) {
                    break;  /* Disk penuh */
                }
                
                /* Hubungkan ke rantai */
                if (last_cluster >= FAT32_FIRST_CLUSTER) {
                    fat32_write_fat_entry(sb, last_cluster, new_cluster);
                } else {
                    inode->i_cluster = new_cluster;
                }
                
                last_cluster = new_cluster;
            }
        }
    }
    
    /* Jika file masih kosong, alokasikan cluster pertama */
    if (inode->i_cluster < FAT32_FIRST_CLUSTER && size > 0) {
        current_cluster = fat32_alloc_cluster(sb);
        if (current_cluster == 0) {
            return (tak_bertandas_t)STATUS_FS_PENUH;
        }
        inode->i_cluster = current_cluster;
    } else {
        current_cluster = inode->i_cluster;
    }
    
    /* Cari cluster awal berdasarkan offset */
    cluster_offset = (tak_bertanda32_t)(offset / cluster_size);
    
    for (i = 0; i < cluster_offset && current_cluster >= FAT32_FIRST_CLUSTER; i++) {
        current_cluster = fat32_get_next_cluster(sb, current_cluster);
    }
    
    /* Hitung offset dalam cluster */
    cluster_offset = (tak_bertanda32_t)(offset % cluster_size);
    
    /* Tulis data ke cluster */
    while (remaining > 0 && current_cluster >= FAT32_FIRST_CLUSTER) {
        /* Hitung bytes yang bisa ditulis dalam cluster ini */
        bytes_in_cluster = cluster_size - cluster_offset;
        bytes_to_write = (remaining > bytes_in_cluster) ? bytes_in_cluster : remaining;
        
        /* Tulis ke cluster */
        if (fat32_write_cluster_range(sb, current_cluster, cluster_offset,
                                      buf_ptr, bytes_to_write) != STATUS_BERHASIL) {
            break;
        }
        
        buf_ptr += bytes_to_write;
        bytes_written += bytes_to_write;
        remaining -= bytes_to_write;
        
        /* Pindah ke cluster berikutnya jika masih ada data */
        cluster_offset = 0;
        if (remaining > 0) {
            current_cluster = fat32_get_next_cluster(sb, current_cluster);
        }
    }
    
    /* Update file size jika perlu */
    if (offset + (off_t)bytes_written > inode->i_size) {
        inode->i_size = offset + (off_t)bytes_written;
    }
    
    /* Update timestamps */
    inode->i_mtime = kernel_get_uptime();
    inode->i_dirty = BENAR;
    sb->s_dirty = BENAR;
    
    return (tak_bertandas_t)bytes_written;
}

/*
 * ===========================================================================
 * FUNGSI TRUNCATE FILE (FILE TRUNCATE FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_truncate_file
 * -------------------
 * Memotong atau memperpanjang file ke ukuran tertentu.
 *
 * Parameter:
 *   sb       - Pointer ke struktur superblock FAT32
 *   inode    - Pointer ke inode file
 *   new_size - Ukuran baru file
 *
 * Return: Status operasi
 */
status_t fat32_truncate_file(fat32_sb_t *sb, fat32_inode_t *inode,
                              off_t new_size)
{
    tak_bertanda32_t clusters_old;
    tak_bertanda32_t clusters_new;
    tak_bertanda32_t last_cluster;
    tak_bertanda32_t i;
    
    if (sb == NULL || inode == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Cek read-only */
    if (sb->s_readonly) {
        return STATUS_FS_READONLY;
    }
    
    /* Cek read-only attribute */
    if (inode->i_attr & FAT32_ATTR_RDONLY) {
        return STATUS_AKSES_DITOLAK;
    }
    
    /* Jika new_size sama, tidak ada yang perlu dilakukan */
    if (new_size == inode->i_size) {
        return STATUS_BERHASIL;
    }
    
    /* Hitung cluster yang dibutuhkan */
    clusters_old = fat32_count_clusters(sb, inode->i_cluster);
    clusters_new = fat32_bytes_to_clusters(sb, (tak_bertanda64_t)new_size);
    
    if (new_size < inode->i_size) {
        /* Shrink file */
        if (clusters_new == 0) {
            /* File menjadi kosong */
            if (inode->i_cluster >= FAT32_FIRST_CLUSTER) {
                fat32_free_cluster_chain(sb, inode->i_cluster);
                inode->i_cluster = 0;
            }
        } else if (clusters_new < clusters_old) {
            /* Cari cluster terakhir yang akan dipertahankan */
            last_cluster = inode->i_cluster;
            for (i = 1; i < clusters_new; i++) {
                last_cluster = fat32_get_next_cluster(sb, last_cluster);
            }
            
            /* Bebaskan cluster setelah last_cluster */
            {
                tak_bertanda32_t next;
                next = fat32_get_next_cluster(sb, last_cluster);
                
                if (next >= FAT32_FIRST_CLUSTER) {
                    fat32_free_cluster_chain(sb, next);
                }
                
                /* Tandai last_cluster sebagai EOF */
                fat32_write_fat_entry(sb, last_cluster, FAT32_END_CHAIN);
            }
        }
    } else {
        /* Extend file - alokasikan cluster baru */
        if (clusters_new > clusters_old) {
            tak_bertanda32_t clusters_to_alloc;
            
            clusters_to_alloc = clusters_new - clusters_old;
            
            /* Cari cluster terakhir */
            if (inode->i_cluster >= FAT32_FIRST_CLUSTER) {
                last_cluster = fat32_find_last_cluster(sb, inode->i_cluster);
            } else {
                /* File baru, alokasikan cluster pertama */
                inode->i_cluster = fat32_alloc_cluster(sb);
                if (inode->i_cluster == 0) {
                    return STATUS_FS_PENUH;
                }
                last_cluster = inode->i_cluster;
                clusters_to_alloc--;
            }
            
            /* Alokasikan cluster tambahan */
            for (i = 0; i < clusters_to_alloc; i++) {
                tak_bertanda32_t new_cluster;
                
                new_cluster = fat32_alloc_cluster(sb);
                if (new_cluster == 0) {
                    break;
                }
                
                /* Hubungkan */
                fat32_write_fat_entry(sb, last_cluster, new_cluster);
                last_cluster = new_cluster;
            }
        }
    }
    
    /* Update size dan timestamps */
    inode->i_size = new_size;
    inode->i_mtime = kernel_get_uptime();
    inode->i_dirty = BENAR;
    sb->s_dirty = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI ALOKASI FILE (FILE ALLOCATION FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_allocate_clusters
 * -----------------------
 * Mengalokasikan cluster untuk file.
 *
 * Parameter:
 *   sb     - Pointer ke struktur superblock FAT32
 *   inode  - Pointer ke inode file
 *   count  - Jumlah cluster yang dibutuhkan
 *
 * Return: Status operasi
 */
status_t fat32_allocate_clusters(fat32_sb_t *sb, fat32_inode_t *inode,
                                  tak_bertanda32_t count)
{
    tak_bertanda32_t current_count;
    tak_bertanda32_t last_cluster;
    tak_bertanda32_t i;
    
    if (sb == NULL || inode == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Cek read-only */
    if (sb->s_readonly) {
        return STATUS_FS_READONLY;
    }
    
    /* Hitung cluster yang sudah ada */
    current_count = fat32_count_clusters(sb, inode->i_cluster);
    
    if (current_count >= count) {
        return STATUS_BERHASIL;  /* Sudah cukup */
    }
    
    /* Cari cluster terakhir */
    if (inode->i_cluster >= FAT32_FIRST_CLUSTER) {
        last_cluster = fat32_find_last_cluster(sb, inode->i_cluster);
    } else {
        /* File kosong, alokasikan cluster pertama */
        inode->i_cluster = fat32_alloc_cluster(sb);
        if (inode->i_cluster == 0) {
            return STATUS_FS_PENUH;
        }
        last_cluster = inode->i_cluster;
        current_count = 1;
    }
    
    /* Alokasikan cluster tambahan */
    for (i = current_count; i < count; i++) {
        tak_bertanda32_t new_cluster;
        
        new_cluster = fat32_alloc_cluster(sb);
        if (new_cluster == 0) {
            return STATUS_FS_PENUH;
        }
        
        /* Hubungkan */
        fat32_write_fat_entry(sb, last_cluster, new_cluster);
        last_cluster = new_cluster;
    }
    
    inode->i_dirty = BENAR;
    sb->s_dirty = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI SYNC FILE (FILE SYNC FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_sync_file
 * ---------------
 * Menyinkronkan file ke disk.
 *
 * Parameter:
 *   sb    - Pointer ke struktur superblock FAT32
 *   inode - Pointer ke inode file
 *
 * Return: Status operasi
 */
status_t fat32_sync_file(fat32_sb_t *sb, fat32_inode_t *inode)
{
    if (sb == NULL || inode == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (!inode->i_dirty) {
        return STATUS_BERHASIL;
    }
    
    /* TODO: Update directory entry dengan informasi terbaru */
    /* Untuk saat ini, clear dirty flag */
    
    inode->i_dirty = SALAH;
    
    log_debug("[FAT32] File synced");
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI SEEK FILE (FILE SEEK FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_file_lseek
 * ----------------
 * Mengubah posisi file pointer.
 *
 * Parameter:
 *   file   - Pointer ke struktur file
 *   offset - Offset
 *   whence - Mode seek (SEEK_SET, SEEK_CUR, SEEK_END)
 *
 * Return: Posisi baru, atau error code negatif
 */
off_t fat32_file_lseek(fat32_file_t *file, off_t offset, 
                        tak_bertanda32_t whence)
{
    off_t new_pos;
    
    if (file == NULL || file->f_inode == NULL) {
        return (off_t)STATUS_PARAM_NULL;
    }
    
    switch (whence) {
    case FAT32_SEEK_SET:
        new_pos = offset;
        break;
        
    case FAT32_SEEK_CUR:
        new_pos = file->f_pos + offset;
        break;
        
    case FAT32_SEEK_END:
        new_pos = file->f_inode->i_size + offset;
        break;
        
    default:
        return (off_t)STATUS_PARAM_INVALID;
    }
    
    /* Validasi posisi */
    if (new_pos < 0) {
        new_pos = 0;
    }
    
    /* Jika posisi melebihi ukuran file, batasi (tidak extend) */
    if (new_pos > file->f_inode->i_size) {
        /* Untuk seek melebihi EOF, kita tidak extend file otomatis */
        /* Ini sesuai dengan perilaku Linux */
    }
    
    file->f_pos = new_pos;
    
    return new_pos;
}

/*
 * ===========================================================================
 * FUNGSI STAT FILE (FILE STAT FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_file_stat
 * ---------------
 * Mendapatkan informasi file.
 *
 * Parameter:
 *   sb    - Pointer ke struktur superblock FAT32
 *   inode - Pointer ke inode file
 *   stat  - Pointer ke struktur stat
 *
 * Return: Status operasi
 */
status_t fat32_file_stat(fat32_sb_t *sb, fat32_inode_t *inode,
                          struct vfs_stat *stat)
{
    if (sb == NULL || inode == NULL || stat == NULL) {
        return STATUS_PARAM_NULL;
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
    stat->st_blksize = sb->s_bytes_per_cluster;
    
    /* Hitung jumlah blocks (dalam 512-byte units) */
    stat->st_blocks = (inode->i_size + 511) / 512;
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI DEBUG FILE (FILE DEBUG FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_print_file_info
 * ---------------------
 * Mencetak informasi file untuk debugging.
 *
 * Parameter:
 *   inode - Pointer ke inode file
 */
void fat32_print_file_info(fat32_inode_t *inode)
{
    if (inode == NULL) {
        kernel_printf("[FAT32] Inode is NULL\n");
        return;
    }
    
    kernel_printf("[FAT32] File Info:\n");
    kernel_printf("  Inode:     %u\n", inode->i_ino);
    kernel_printf("  Mode:      0x%04X\n", inode->i_mode);
    kernel_printf("  Size:      %lld bytes\n", (long long)inode->i_size);
    kernel_printf("  Cluster:   %u\n", inode->i_cluster);
    kernel_printf("  Attr:      0x%02X\n", inode->i_attr);
    kernel_printf("  Dirty:     %s\n", inode->i_dirty ? "Yes" : "No");
}

/*
 * fat32_print_file_clusters
 * -------------------------
 * Mencetak cluster-chain file untuk debugging.
 *
 * Parameter:
 *   sb    - Pointer ke struktur superblock FAT32
 *   inode - Pointer ke inode file
 */
void fat32_print_file_clusters(fat32_sb_t *sb, fat32_inode_t *inode)
{
    tak_bertanda32_t cluster;
    tak_bertanda32_t count;
    
    if (sb == NULL || inode == NULL) {
        kernel_printf("[FAT32] Invalid parameters\n");
        return;
    }
    
    kernel_printf("[FAT32] File clusters:\n");
    
    if (inode->i_cluster < FAT32_FIRST_CLUSTER) {
        kernel_printf("  No clusters allocated\n");
        return;
    }
    
    cluster = inode->i_cluster;
    count = 0;
    
    while (cluster >= FAT32_FIRST_CLUSTER) {
        kernel_printf("  %u", cluster);
        
        count++;
        if (count % 8 == 0) {
            kernel_printf("\n");
        } else {
            kernel_printf(" -> ");
        }
        
        cluster = fat32_get_next_cluster(sb, cluster);
    }
    
    if (count % 8 != 0) {
        kernel_printf("EOF\n");
    }
    
    kernel_printf("  Total: %u clusters\n", count);
}
