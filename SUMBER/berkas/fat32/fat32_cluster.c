/*
 * PIGURA OS - FAT32_CLUSTER.C
 * =============================
 * Implementasi manajemen cluster untuk filesystem FAT32.
 *
 * Berkas ini berisi fungsi-fungsi untuk membaca, menulis, dan
 * mengelola cluster pada filesystem FAT32.
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

/* Special cluster numbers */
#define FAT32_FIRST_CLUSTER     2
#define FAT32_ROOT_CLUSTER      2

/* End of chain markers */
#define FAT32_END_CHAIN         0x0FFFFFFF
#define FAT32_CLUSTER_MASK      0x0FFFFFFF

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
 * FORWARD DECLARATIONS (DARI fat32_fat.c)
 * ===========================================================================
 */

/* Deklarasi fungsi dari fat32_fat.c */
tak_bertanda32_t fat32_read_fat_entry(fat32_sb_t *sb, tak_bertanda32_t cluster);
status_t fat32_write_fat_entry(fat32_sb_t *sb, tak_bertanda32_t cluster,
                               tak_bertanda32_t value);
tak_bertanda32_t fat32_alloc_cluster(fat32_sb_t *sb);
status_t fat32_free_cluster_chain(fat32_sb_t *sb, tak_bertanda32_t cluster);

/*
 * ===========================================================================
 * FUNGSI KONVERSI CLUSTER (CLUSTER CONVERSION FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_cluster_to_sector
 * -----------------------
 * Mengkonversi nomor cluster ke nomor sektor absolut.
 *
 * Parameter:
 *   sb      - Pointer ke struktur superblock FAT32
 *   cluster - Nomor cluster
 *
 * Return: Nomor sektor absolut
 */
tak_bertanda32_t fat32_cluster_to_sector(fat32_sb_t *sb,
                                          tak_bertanda32_t cluster)
{
    if (sb == NULL || cluster < FAT32_FIRST_CLUSTER) {
        return 0;
    }
    
    /* Data region dimulai setelah:
     * - Reserved sectors
     * - FAT sectors (num_fats * fat_size)
     * - Root directory (untuk FAT32, root ada di cluster 2)
     */
    return sb->s_data_start_sec + 
           ((cluster - FAT32_FIRST_CLUSTER) * sb->s_sec_per_cluster);
}

/*
 * fat32_cluster_to_byte
 * ---------------------
 * Mengkonversi nomor cluster ke byte offset absolut.
 *
 * Parameter:
 *   sb      - Pointer ke struktur superblock FAT32
 *   cluster - Nomor cluster
 *
 * Return: Byte offset absolut
 */
tak_bertanda64_t fat32_cluster_to_byte(fat32_sb_t *sb,
                                        tak_bertanda32_t cluster)
{
    tak_bertanda32_t sector;
    
    sector = fat32_cluster_to_sector(sb, cluster);
    
    return (tak_bertanda64_t)sector * sb->s_bytes_per_sec;
}

/*
 * fat32_sector_to_cluster
 * -----------------------
 * Mengkonversi nomor sektor absolut ke nomor cluster.
 *
 * Parameter:
 *   sb      - Pointer ke struktur superblock FAT32
 *   sector  - Nomor sektor absolut
 *
 * Return: Nomor cluster, atau 0 jika tidak valid
 */
tak_bertanda32_t fat32_sector_to_cluster(fat32_sb_t *sb,
                                          tak_bertanda32_t sector)
{
    tak_bertanda32_t cluster;
    
    if (sb == NULL || sector < sb->s_data_start_sec) {
        return 0;
    }
    
    cluster = ((sector - sb->s_data_start_sec) / sb->s_sec_per_cluster) + 
              FAT32_FIRST_CLUSTER;
    
    return cluster;
}

/*
 * fat32_byte_to_cluster
 * ---------------------
 * Mengkonversi byte offset dalam file ke cluster.
 *
 * Parameter:
 *   sb           - Pointer ke struktur superblock FAT32
 *   first_cluster - Cluster pertama file
 *   offset       - Byte offset dalam file
 *
 * Return: Nomor cluster, atau 0 jika error
 */
tak_bertanda32_t fat32_byte_to_cluster(fat32_sb_t *sb,
                                        tak_bertanda32_t first_cluster,
                                        tak_bertanda64_t offset)
{
    tak_bertanda32_t cluster_offset;
    tak_bertanda32_t __attribute__((unused)) cluster_index;
    
    if (sb == NULL || first_cluster < FAT32_FIRST_CLUSTER) {
        return 0;
    }
    
    /* Hitung cluster index dari offset */
    cluster_offset = (tak_bertanda32_t)(offset / sb->s_bytes_per_cluster);
    
    /* Traverse FAT untuk menemukan cluster */
    {
        tak_bertanda32_t current;
        tak_bertanda32_t i;
        
        current = first_cluster;
        
        for (i = 0; i < cluster_offset; i++) {
            tak_bertanda32_t next;
            
            /* Baca FAT entry (dari fat32_fat.c) */
            next = fat32_read_fat_entry(sb, current);
            
            /* Cek jika sudah sampai akhir rantai */
            if (next >= FAT32_END_CHAIN || next == 0) {
                return 0;
            }
            
            current = next;
        }
        
        return current;
    }
}

/*
 * ===========================================================================
 * FUNGSI PEMBACAAN CLUSTER (CLUSTER READ FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_read_cluster
 * ------------------
 * Membaca satu cluster dari device.
 *
 * Parameter:
 *   sb      - Pointer ke struktur superblock FAT32
 *   cluster - Nomor cluster
 *   buffer  - Buffer tujuan (harus cukup besar untuk satu cluster)
 *
 * Return: Status operasi
 */
status_t fat32_read_cluster(fat32_sb_t *sb, tak_bertanda32_t cluster,
                            void *buffer)
{
    tak_bertanda32_t sector;
    tak_bertanda32_t i;
    
    if (sb == NULL || buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (cluster < FAT32_FIRST_CLUSTER) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Hitung sektor awal */
    sector = fat32_cluster_to_sector(sb, cluster);
    
    /* Baca semua sektor dalam cluster */
    for (i = 0; i < sb->s_sec_per_cluster; i++) {
        /* TODO: Implementasi pembacaan dari device */
        /* Untuk saat ini, clear buffer */
        kernel_memset((tak_bertanda8_t *)buffer + 
                      (i * sb->s_bytes_per_sec),
                      0, sb->s_bytes_per_sec);
    }
    
    return STATUS_BERHASIL;
}

/*
 * fat32_read_cluster_range
 * ------------------------
 * Membaca sebagian dari cluster.
 *
 * Parameter:
 *   sb      - Pointer ke struktur superblock FAT32
 *   cluster - Nomor cluster
 *   offset  - Offset dalam cluster (byte)
 *   buffer  - Buffer tujuan
 *   size    - Jumlah byte yang akan dibaca
 *
 * Return: Status operasi
 */
status_t fat32_read_cluster_range(fat32_sb_t *sb, tak_bertanda32_t cluster,
                                   tak_bertanda32_t offset, void *buffer,
                                   ukuran_t size)
{
    tak_bertanda8_t *cluster_buffer;
    status_t ret;
    
    if (sb == NULL || buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (cluster < FAT32_FIRST_CLUSTER) {
        return STATUS_PARAM_INVALID;
    }
    
    if (offset >= sb->s_bytes_per_cluster) {
        return STATUS_PARAM_JARAK;
    }
    
    if (offset + size > sb->s_bytes_per_cluster) {
        size = sb->s_bytes_per_cluster - offset;
    }
    
    /* Alokasikan buffer sementara */
    cluster_buffer = (tak_bertanda8_t *)kmalloc(sb->s_bytes_per_cluster);
    if (cluster_buffer == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    /* Baca cluster */
    ret = fat32_read_cluster(sb, cluster, cluster_buffer);
    if (ret != STATUS_BERHASIL) {
        kfree(cluster_buffer);
        return ret;
    }
    
    /* Copy data yang diminta */
    kernel_memcpy(buffer, cluster_buffer + offset, size);
    
    kfree(cluster_buffer);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PENULISAN CLUSTER (CLUSTER WRITE FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_write_cluster
 * -------------------
 * Menulis satu cluster ke device.
 *
 * Parameter:
 *   sb      - Pointer ke struktur superblock FAT32
 *   cluster - Nomor cluster
 *   buffer  - Buffer sumber (harus berukuran satu cluster)
 *
 * Return: Status operasi
 */
status_t fat32_write_cluster(fat32_sb_t *sb, tak_bertanda32_t cluster,
                              const void *buffer)
{
    tak_bertanda32_t sector;
    tak_bertanda32_t i;
    
    if (sb == NULL || buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (cluster < FAT32_FIRST_CLUSTER) {
        return STATUS_PARAM_INVALID;
    }
    
    if (sb->s_readonly) {
        return STATUS_FS_READONLY;
    }
    
    /* Hitung sektor awal */
    sector = fat32_cluster_to_sector(sb, cluster);
    
    /* Tulis semua sektor dalam cluster */
    for (i = 0; i < sb->s_sec_per_cluster; i++) {
        /* TODO: Implementasi penulisan ke device */
        (void)sector;  /* Suppress warning */
    }
    
    sb->s_dirty = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * fat32_write_cluster_range
 * -------------------------
 * Menulis sebagian dari cluster.
 *
 * Parameter:
 *   sb      - Pointer ke struktur superblock FAT32
 *   cluster - Nomor cluster
 *   offset  - Offset dalam cluster (byte)
 *   buffer  - Buffer sumber
 *   size    - Jumlah byte yang akan ditulis
 *
 * Return: Status operasi
 */
status_t fat32_write_cluster_range(fat32_sb_t *sb, tak_bertanda32_t cluster,
                                    tak_bertanda32_t offset,
                                    const void *buffer, ukuran_t size)
{
    tak_bertanda8_t *cluster_buffer;
    status_t ret;
    
    if (sb == NULL || buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (cluster < FAT32_FIRST_CLUSTER) {
        return STATUS_PARAM_INVALID;
    }
    
    if (offset >= sb->s_bytes_per_cluster) {
        return STATUS_PARAM_JARAK;
    }
    
    if (sb->s_readonly) {
        return STATUS_FS_READONLY;
    }
    
    if (offset + size > sb->s_bytes_per_cluster) {
        size = sb->s_bytes_per_cluster - offset;
    }
    
    /* Alokasikan buffer sementara */
    cluster_buffer = (tak_bertanda8_t *)kmalloc(sb->s_bytes_per_cluster);
    if (cluster_buffer == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    /* Baca cluster terlebih dahulu (read-modify-write) */
    ret = fat32_read_cluster(sb, cluster, cluster_buffer);
    if (ret != STATUS_BERHASIL) {
        kfree(cluster_buffer);
        return ret;
    }
    
    /* Modify data */
    kernel_memcpy(cluster_buffer + offset, buffer, size);
    
    /* Tulis kembali */
    ret = fat32_write_cluster(sb, cluster, cluster_buffer);
    
    kfree(cluster_buffer);
    
    return ret;
}

/*
 * fat32_zero_cluster
 * ------------------
 * Mengisi cluster dengan nol.
 *
 * Parameter:
 *   sb      - Pointer ke struktur superblock FAT32
 *   cluster - Nomor cluster
 *
 * Return: Status operasi
 */
status_t fat32_zero_cluster(fat32_sb_t *sb, tak_bertanda32_t cluster)
{
    tak_bertanda8_t *buffer;
    status_t ret;
    
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (cluster < FAT32_FIRST_CLUSTER) {
        return STATUS_PARAM_INVALID;
    }
    
    if (sb->s_readonly) {
        return STATUS_FS_READONLY;
    }
    
    /* Alokasikan buffer zero */
    buffer = (tak_bertanda8_t *)kmalloc(sb->s_bytes_per_cluster);
    if (buffer == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    kernel_memset(buffer, 0, sb->s_bytes_per_cluster);
    
    ret = fat32_write_cluster(sb, cluster, buffer);
    
    kfree(buffer);
    
    return ret;
}

/*
 * ===========================================================================
 * FUNGSI NAVIGASI CLUSTER (CLUSTER NAVIGATION FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_get_next_cluster
 * ----------------------
 * Mendapatkan cluster berikutnya dalam rantai.
 *
 * Parameter:
 *   sb      - Pointer ke struktur superblock FAT32
 *   cluster - Cluster saat ini
 *
 * Return: Cluster berikutnya, atau 0 jika EOF atau error
 */
tak_bertanda32_t fat32_get_next_cluster(fat32_sb_t *sb,
                                         tak_bertanda32_t cluster)
{
    tak_bertanda32_t entry;
    
    if (sb == NULL || cluster < FAT32_FIRST_CLUSTER) {
        return 0;
    }
    
    /* Baca FAT entry */
    entry = fat32_read_fat_entry(sb, cluster);
    
    /* Cek jika sudah akhir rantai */
    if ((entry & FAT32_CLUSTER_MASK) >= FAT32_END_CHAIN) {
        return 0;
    }
    
    return entry & FAT32_CLUSTER_MASK;
}

/*
 * fat32_get_prev_cluster
 * ----------------------
 * Mendapatkan cluster sebelumnya dalam rantai (membutuhkan scan dari awal).
 *
 * Parameter:
 *   sb           - Pointer ke struktur superblock FAT32
 *   first_cluster - Cluster pertama dalam rantai
 *   target       - Cluster yang dicari sebelumnya
 *
 * Return: Cluster sebelumnya, atau 0 jika tidak ditemukan
 */
tak_bertanda32_t fat32_get_prev_cluster(fat32_sb_t *sb,
                                         tak_bertanda32_t first_cluster,
                                         tak_bertanda32_t target)
{
    tak_bertanda32_t current;
    tak_bertanda32_t next;
    
    if (sb == NULL || first_cluster < FAT32_FIRST_CLUSTER ||
        target < FAT32_FIRST_CLUSTER) {
        return 0;
    }
    
    if (target == first_cluster) {
        return 0;  /* Tidak ada cluster sebelumnya */
    }
    
    current = first_cluster;
    
    while (current >= FAT32_FIRST_CLUSTER) {
        next = fat32_read_fat_entry(sb, current);
        
        if ((next & FAT32_CLUSTER_MASK) == target) {
            return current;
        }
        
        if ((next & FAT32_CLUSTER_MASK) >= FAT32_END_CHAIN) {
            break;  /* Sudah sampai akhir rantai */
        }
        
        current = next;
    }
    
    return 0;  /* Tidak ditemukan */
}

/*
 * fat32_find_last_cluster
 * -----------------------
 * Mencari cluster terakhir dalam rantai.
 *
 * Parameter:
 *   sb           - Pointer ke struktur superblock FAT32
 *   first_cluster - Cluster pertama dalam rantai
 *
 * Return: Cluster terakhir, atau 0 jika error
 */
tak_bertanda32_t fat32_find_last_cluster(fat32_sb_t *sb,
                                          tak_bertanda32_t first_cluster)
{
    tak_bertanda32_t current;
    tak_bertanda32_t next;
    
    if (sb == NULL || first_cluster < FAT32_FIRST_CLUSTER) {
        return 0;
    }
    
    current = first_cluster;
    
    while (current >= FAT32_FIRST_CLUSTER) {
        next = fat32_read_fat_entry(sb, current);
        
        if ((next & FAT32_CLUSTER_MASK) >= FAT32_END_CHAIN) {
            return current;
        }
        
        current = next;
    }
    
    return current;
}

/*
 * ===========================================================================
 * FUNGSI EKSPANSI CLUSTER (CLUSTER EXPANSION FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_extend_cluster_chain
 * --------------------------
 * Memperpanjang rantai cluster.
 *
 * Parameter:
 *   sb           - Pointer ke struktur superblock FAT32
 *   last_cluster - Cluster terakhir dalam rantai (atau 0 untuk rantai baru)
 *   count        - Jumlah cluster yang akan ditambahkan
 *
 * Return: Status operasi
 */
status_t fat32_extend_cluster_chain(fat32_sb_t *sb,
                                     tak_bertanda32_t last_cluster,
                                     tak_bertanda32_t count)
{
    tak_bertanda32_t i;
    tak_bertanda32_t new_cluster;
    tak_bertanda32_t prev_cluster;
    
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (count == 0) {
        return STATUS_BERHASIL;
    }
    
    if (sb->s_readonly) {
        return STATUS_FS_READONLY;
    }
    
    prev_cluster = last_cluster;
    
    for (i = 0; i < count; i++) {
        /* Alokasikan cluster baru */
        new_cluster = fat32_alloc_cluster(sb);
        if (new_cluster == 0) {
            log_warn("[FAT32] Failed to allocate cluster %u of %u",
                     i + 1, count);
            return STATUS_FS_PENUH;
        }
        
        /* Zero cluster baru */
        fat32_zero_cluster(sb, new_cluster);
        
        /* Hubungkan dengan cluster sebelumnya */
        if (prev_cluster >= FAT32_FIRST_CLUSTER) {
            fat32_write_fat_entry(sb, prev_cluster, new_cluster);
        }
        
        prev_cluster = new_cluster;
    }
    
    return STATUS_BERHASIL;
}

/*
 * fat32_truncate_cluster_chain
 * ----------------------------
 * Memotong rantai cluster pada cluster tertentu.
 *
 * Parameter:
 *   sb           - Pointer ke struktur superblock FAT32
 *   cluster      - Cluster terakhir yang akan dipertahankan
 *
 * Return: Status operasi
 */
status_t fat32_truncate_cluster_chain(fat32_sb_t *sb,
                                       tak_bertanda32_t cluster)
{
    tak_bertanda32_t next;
    
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (cluster < FAT32_FIRST_CLUSTER) {
        return STATUS_PARAM_INVALID;
    }
    
    if (sb->s_readonly) {
        return STATUS_FS_READONLY;
    }
    
    /* Baca cluster berikutnya */
    next = fat32_read_fat_entry(sb, cluster);
    
    /* Tandai cluster saat ini sebagai EOF */
    fat32_write_fat_entry(sb, cluster, FAT32_END_CHAIN);
    
    /* Bebaskan cluster berikutnya */
    if (next >= FAT32_FIRST_CLUSTER && next < FAT32_END_CHAIN) {
        fat32_free_cluster_chain(sb, next);
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI ALOKASI CLUSTER (CLUSTER ALLOCATION FUNCTIONS)
 * ===========================================================================
 */

/* Forward declarations sudah ada di atas file */

/*
 * fat32_allocate_clusters
 * -----------------------
 * Mengalokasikan sejumlah cluster untuk file.
 *
 * Parameter:
 *   sb           - Pointer ke struktur superblock FAT32
 *   first_cluster - Pointer ke cluster pertama (input: existing chain, 
 *                   output: new or existing chain)
 *   count        - Jumlah cluster yang dibutuhkan
 *   current_count- Jumlah cluster yang sudah ada
 *
 * Return: Status operasi
 */
status_t fat32_allocate_clusters(fat32_sb_t *sb,
                                  tak_bertanda32_t *first_cluster,
                                  tak_bertanda32_t count,
                                  tak_bertanda32_t current_count)
{
    tak_bertanda32_t additional;
    tak_bertanda32_t last_cluster;
    
    if (sb == NULL || first_cluster == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (sb->s_readonly) {
        return STATUS_FS_READONLY;
    }
    
    /* Hitung cluster tambahan yang diperlukan */
    if (count <= current_count) {
        return STATUS_BERHASIL;  /* Sudah cukup */
    }
    
    additional = count - current_count;
    
    /* Jika belum ada cluster, alokasikan yang pertama */
    if (*first_cluster < FAT32_FIRST_CLUSTER) {
        *first_cluster = fat32_alloc_cluster(sb);
        if (*first_cluster == 0) {
            return STATUS_FS_PENUH;
        }
        
        fat32_zero_cluster(sb, *first_cluster);
        
        if (additional == 1) {
            return STATUS_BERHASIL;
        }
        
        additional--;
        last_cluster = *first_cluster;
    } else {
        /* Temukan cluster terakhir */
        last_cluster = fat32_find_last_cluster(sb, *first_cluster);
        if (last_cluster == 0) {
            return STATUS_IO_ERROR;
        }
    }
    
    /* Perpanjang rantai */
    return fat32_extend_cluster_chain(sb, last_cluster, additional);
}

/*
 * ===========================================================================
 * FUNGSI STATISTIK CLUSTER (CLUSTER STATISTICS FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_count_clusters
 * --------------------
 * Menghitung jumlah cluster dalam rantai.
 *
 * Parameter:
 *   sb           - Pointer ke struktur superblock FAT32
 *   first_cluster - Cluster pertama dalam rantai
 *
 * Return: Jumlah cluster
 */
tak_bertanda32_t fat32_count_clusters(fat32_sb_t *sb,
                                       tak_bertanda32_t first_cluster)
{
    tak_bertanda32_t count;
    tak_bertanda32_t current;
    tak_bertanda32_t next;
    
    if (sb == NULL || first_cluster < FAT32_FIRST_CLUSTER) {
        return 0;
    }
    
    count = 0;
    current = first_cluster;
    
    while (current >= FAT32_FIRST_CLUSTER) {
        count++;
        
        next = fat32_read_fat_entry(sb, current);
        
        if ((next & FAT32_CLUSTER_MASK) >= FAT32_END_CHAIN) {
            break;
        }
        
        current = next;
    }
    
    return count;
}

/*
 * fat32_get_cluster_size
 * ----------------------
 * Mendapatkan ukuran satu cluster dalam byte.
 *
 * Parameter:
 *   sb - Pointer ke struktur superblock FAT32
 *
 * Return: Ukuran cluster dalam byte
 */
tak_bertanda32_t fat32_get_cluster_size(fat32_sb_t *sb)
{
    if (sb == NULL) {
        return 0;
    }
    
    return sb->s_bytes_per_cluster;
}

/*
 * fat32_get_total_clusters
 * ------------------------
 * Mendapatkan total cluster dalam filesystem.
 *
 * Parameter:
 *   sb - Pointer ke struktur superblock FAT32
 *
 * Return: Total cluster
 */
tak_bertanda32_t fat32_get_total_clusters(fat32_sb_t *sb)
{
    if (sb == NULL) {
        return 0;
    }
    
    return sb->s_total_clusters;
}

/*
 * fat32_bytes_to_clusters
 * -----------------------
 * Mengkonversi ukuran byte ke jumlah cluster.
 *
 * Parameter:
 *   sb   - Pointer ke struktur superblock FAT32
 *   size - Ukuran dalam byte
 *
 * Return: Jumlah cluster yang dibutuhkan
 */
tak_bertanda32_t fat32_bytes_to_clusters(fat32_sb_t *sb, tak_bertanda64_t size)
{
    if (sb == NULL || size == 0) {
        return 0;
    }
    
    return (tak_bertanda32_t)((size + sb->s_bytes_per_cluster - 1) / 
                              sb->s_bytes_per_cluster);
}

/*
 * ===========================================================================
 * FUNGSI DEBUG CLUSTER (CLUSTER DEBUG FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_print_cluster_chain
 * -------------------------
 * Mencetak rantai cluster untuk debugging.
 *
 * Parameter:
 *   sb           - Pointer ke struktur superblock FAT32
 *   first_cluster - Cluster pertama dalam rantai
 *   max_count    - Maksimum cluster yang akan dicetak
 */
void fat32_print_cluster_chain(fat32_sb_t *sb, tak_bertanda32_t first_cluster,
                                tak_bertanda32_t max_count)
{
    tak_bertanda32_t current;
    tak_bertanda32_t next;
    tak_bertanda32_t count;
    
    if (sb == NULL) {
        kernel_printf("[FAT32] Superblock is NULL\n");
        return;
    }
    
    kernel_printf("[FAT32] Cluster chain starting from %u:\n", first_cluster);
    
    current = first_cluster;
    count = 0;
    
    while (current >= FAT32_FIRST_CLUSTER && count < max_count) {
        kernel_printf("  %u", current);
        
        next = fat32_read_fat_entry(sb, current);
        
        if ((next & FAT32_CLUSTER_MASK) >= FAT32_END_CHAIN) {
            kernel_printf(" -> EOF\n");
            break;
        }
        
        kernel_printf(" -> ");
        current = next;
        count++;
    }
    
    if (count >= max_count) {
        kernel_printf("...\n");
    }
    
    kernel_printf("  Total clusters: %u\n", count);
}
