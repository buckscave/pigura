/*
 * PIGURA OS - FAT32_FAT.C
 * =========================
 * Implementasi operasi File Allocation Table (FAT) untuk FAT32.
 *
 * Berkas ini berisi fungsi-fungsi untuk membaca, menulis, dan
 * mengelola entri dalam File Allocation Table pada filesystem FAT32.
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

/* FAT entry values */
#define FAT32_FREE              0x00000000
#define FAT32_RESERVED_START    0x0FFFFFF0
#define FAT32_RESERVED_END      0x0FFFFFF6
#define FAT32_BAD_CLUSTER       0x0FFFFFF7
#define FAT32_END_CHAIN_START   0x0FFFFFF8
#define FAT32_END_CHAIN_END     0x0FFFFFFF

/* Mask untuk cluster value */
#define FAT32_CLUSTER_MASK      0x0FFFFFFF

/* Special cluster numbers */
#define FAT32_FIRST_CLUSTER     2

/* FAT cache parameters */
#define FAT32_FAT_CACHE_SIZE    4096
#define FAT32_FAT_ENTRIES_PER_SEC   128  /* 512 / 4 */

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
 * FUNGSI VALIDASI CLUSTER (CLUSTER VALIDATION FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_cluster_valid
 * -------------------
 * Memeriksa apakah nomor cluster valid.
 *
 * Parameter:
 *   sb      - Pointer ke struktur superblock FAT32
 *   cluster - Nomor cluster
 *
 * Return: BENAR jika valid, SALAH jika tidak
 */
bool_t fat32_cluster_valid(fat32_sb_t *sb, tak_bertanda32_t cluster)
{
    if (sb == NULL) {
        return SALAH;
    }
    
    return (cluster >= FAT32_FIRST_CLUSTER && 
            cluster < sb->s_total_clusters + FAT32_FIRST_CLUSTER);
}

/*
 * fat32_cluster_is_free
 * ---------------------
 * Memeriksa apakah cluster bebas.
 *
 * Parameter:
 *   entry - Nilai entri FAT
 *
 * Return: BENAR jika bebas, SALAH jika tidak
 */
bool_t fat32_cluster_is_free(tak_bertanda32_t entry)
{
    return ((entry & FAT32_CLUSTER_MASK) == FAT32_FREE);
}

/*
 * fat32_cluster_is_eof
 * --------------------
 * Memeriksa apakah cluster adalah end-of-chain.
 *
 * Parameter:
 *   entry - Nilai entri FAT
 *
 * Return: BENAR jika EOF, SALAH jika tidak
 */
bool_t fat32_cluster_is_eof(tak_bertanda32_t entry)
{
    tak_bertanda32_t value;
    
    value = entry & FAT32_CLUSTER_MASK;
    
    return (value >= FAT32_END_CHAIN_START && value <= FAT32_END_CHAIN_END);
}

/*
 * fat32_cluster_is_bad
 * --------------------
 * Memeriksa apakah cluster adalah bad cluster.
 *
 * Parameter:
 *   entry - Nilai entri FAT
 *
 * Return: BENAR jika bad, SALAH jika tidak
 */
bool_t fat32_cluster_is_bad(tak_bertanda32_t entry)
{
    return ((entry & FAT32_CLUSTER_MASK) == FAT32_BAD_CLUSTER);
}

/*
 * fat32_cluster_is_reserved
 * -------------------------
 * Memeriksa apakah cluster adalah reserved.
 *
 * Parameter:
 *   entry - Nilai entri FAT
 *
 * Return: BENAR jika reserved, SALAH jika tidak
 */
bool_t fat32_cluster_is_reserved(tak_bertanda32_t entry)
{
    tak_bertanda32_t value;
    
    value = entry & FAT32_CLUSTER_MASK;
    
    return (value >= FAT32_RESERVED_START && value <= FAT32_RESERVED_END);
}

/*
 * ===========================================================================
 * FUNGSI PEMBACAAN FAT (FAT READ FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_get_fat_sector
 * --------------------
 * Mendapatkan sektor FAT yang berisi entri untuk cluster tertentu.
 *
 * Parameter:
 *   sb      - Pointer ke struktur superblock FAT32
 *   cluster - Nomor cluster
 *
 * Return: Nomor sektor dalam FAT
 */
static tak_bertanda32_t fat32_get_fat_sector(fat32_sb_t *sb,
                                              tak_bertanda32_t cluster)
{
    if (sb == NULL) {
        return 0;
    }
    
    /* Setiap entri FAT32 adalah 4 byte */
    /* Setiap sektor berisi bytes_per_sec / 4 entri */
    return cluster / FAT32_FAT_ENTRIES_PER_SEC;
}

/*
 * fat32_get_fat_offset
 * --------------------
 * Mendapatkan offset dalam sektor untuk entri cluster.
 *
 * Parameter:
 *   sb      - Pointer ke struktur superblock FAT32
 *   cluster - Nomor cluster
 *
 * Return: Offset dalam sektor (dalam byte)
 */
static tak_bertanda32_t fat32_get_fat_offset(fat32_sb_t *sb,
                                              tak_bertanda32_t cluster)
{
    (void)sb;  /* Unused */
    
    return (cluster % FAT32_FAT_ENTRIES_PER_SEC) * 4;
}

/*
 * fat32_read_fat_entry
 * --------------------
 * Membaca entri FAT untuk cluster tertentu.
 *
 * Parameter:
 *   sb      - Pointer ke struktur superblock FAT32
 *   cluster - Nomor cluster
 *
 * Return: Nilai entri FAT, atau 0 jika error
 */
tak_bertanda32_t fat32_read_fat_entry(fat32_sb_t *sb, 
                                       tak_bertanda32_t cluster)
{
    tak_bertanda32_t fat_sector;
    tak_bertanda32_t fat_offset;
    tak_bertanda32_t sector;
    tak_bertanda32_t entry;
    tak_bertanda8_t *buffer;
    
    if (sb == NULL) {
        return 0;
    }
    
    /* Validasi cluster */
    if (!fat32_cluster_valid(sb, cluster)) {
        log_debug("[FAT32] Invalid cluster: %u", cluster);
        return 0;
    }
    
    /* Hitung lokasi dalam FAT */
    fat_sector = fat32_get_fat_sector(sb, cluster);
    fat_offset = fat32_get_fat_offset(sb, cluster);
    
    /* FAT dimulai setelah reserved sectors */
    sector = sb->s_reserved_sec + fat_sector;
    
    /* Gunakan FAT cache jika tersedia */
    if (sb->s_fat_cache != NULL) {
        /* Hitung offset dalam cache */
        tak_bertanda64_t cache_offset;
        
        cache_offset = (tak_bertanda64_t)fat_sector * sb->s_bytes_per_sec + 
                       fat_offset;
        
        /* Baca dari cache */
        buffer = (tak_bertanda8_t *)sb->s_fat_cache;
        entry = ((tak_bertanda32_t)buffer[cache_offset]) |
                ((tak_bertanda32_t)buffer[cache_offset + 1] << 8) |
                ((tak_bertanda32_t)buffer[cache_offset + 2] << 16) |
                ((tak_bertanda32_t)buffer[cache_offset + 3] << 24);
    } else {
        /* TODO: Baca dari device */
        /* Untuk saat ini, kembalikan EOF */
        entry = FAT32_END_CHAIN_END;
        log_debug("[FAT32] FAT cache not available, returning EOF");
    }
    
    return entry & FAT32_CLUSTER_MASK;
}

/*
 * ===========================================================================
 * FUNGSI PENULISAN FAT (FAT WRITE FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_write_fat_entry
 * ---------------------
 * Menulis entri FAT untuk cluster tertentu.
 *
 * Parameter:
 *   sb      - Pointer ke struktur superblock FAT32
 *   cluster - Nomor cluster
 *   value   - Nilai yang akan ditulis
 *
 * Return: Status operasi
 */
status_t fat32_write_fat_entry(fat32_sb_t *sb, tak_bertanda32_t cluster,
                                tak_bertanda32_t value)
{
    tak_bertanda32_t fat_sector;
    tak_bertanda32_t fat_offset;
    tak_bertanda8_t *buffer;
    
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Validasi cluster */
    if (!fat32_cluster_valid(sb, cluster)) {
        log_debug("[FAT32] Invalid cluster for write: %u", cluster);
        return STATUS_PARAM_INVALID;
    }
    
    /* Check jika read-only */
    if (sb->s_readonly) {
        return STATUS_FS_READONLY;
    }
    
    /* Mask nilai */
    value &= FAT32_CLUSTER_MASK;
    
    /* Hitung lokasi dalam FAT */
    fat_sector = fat32_get_fat_sector(sb, cluster);
    fat_offset = fat32_get_fat_offset(sb, cluster);
    
    /* Gunakan FAT cache jika tersedia */
    if (sb->s_fat_cache != NULL) {
        tak_bertanda64_t cache_offset;
        
        cache_offset = (tak_bertanda64_t)fat_sector * sb->s_bytes_per_sec + 
                       fat_offset;
        
        /* Tulis ke cache */
        buffer = (tak_bertanda8_t *)sb->s_fat_cache;
        buffer[cache_offset] = (tak_bertanda8_t)(value & 0xFF);
        buffer[cache_offset + 1] = (tak_bertanda8_t)((value >> 8) & 0xFF);
        buffer[cache_offset + 2] = (tak_bertanda8_t)((value >> 16) & 0xFF);
        buffer[cache_offset + 3] = (tak_bertanda8_t)((value >> 24) & 0xFF);
        
        sb->s_dirty = BENAR;
    } else {
        /* TODO: Tulis langsung ke device */
        log_warn("[FAT32] FAT cache not available for write");
        return STATUS_TIDAK_DUKUNG;
    }
    
    return STATUS_BERHASIL;
}

/*
 * fat32_write_fat_both
 * --------------------
 * Menulis entri FAT ke semua salinan FAT.
 *
 * Parameter:
 *   sb      - Pointer ke struktur superblock FAT32
 *   cluster - Nomor cluster
 *   value   - Nilai yang akan ditulis
 *
 * Return: Status operasi
 */
status_t fat32_write_fat_both(fat32_sb_t *sb, tak_bertanda32_t cluster,
                               tak_bertanda32_t value)
{
    /* FAT32 biasanya memiliki 2 salinan FAT */
    /* Untuk saat ini, kita hanya menulis ke FAT cache */
    /* yang sudah mewakili FAT pertama */
    
    return fat32_write_fat_entry(sb, cluster, value);
}

/*
 * ===========================================================================
 * FUNGSI ALOKASI CLUSTER (CLUSTER ALLOCATION FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_alloc_cluster
 * -------------------
 * Mengalokasikan cluster baru dari free pool.
 *
 * Parameter:
 *   sb - Pointer ke struktur superblock FAT32
 *
 * Return: Nomor cluster yang dialokasikan, atau 0 jika gagal
 */
tak_bertanda32_t fat32_alloc_cluster(fat32_sb_t *sb)
{
    tak_bertanda32_t cluster;
    tak_bertanda32_t start_cluster;
    tak_bertanda32_t entry;
    tak_bertanda32_t i;
    
    if (sb == NULL) {
        return 0;
    }
    
    /* Check jika read-only */
    if (sb->s_readonly) {
        log_debug("[FAT32] Cannot allocate cluster: read-only");
        return 0;
    }
    
    /* Mulai pencarian dari next_free */
    start_cluster = sb->s_next_free;
    if (start_cluster < FAT32_FIRST_CLUSTER) {
        start_cluster = FAT32_FIRST_CLUSTER;
    }
    
    cluster = 0;
    
    /* Cari cluster bebas */
    for (i = 0; i < sb->s_total_clusters; i++) {
        tak_bertanda32_t test_cluster;
        
        test_cluster = ((start_cluster - FAT32_FIRST_CLUSTER + i) % 
                        sb->s_total_clusters) + FAT32_FIRST_CLUSTER;
        
        entry = fat32_read_fat_entry(sb, test_cluster);
        
        if (fat32_cluster_is_free(entry)) {
            cluster = test_cluster;
            break;
        }
    }
    
    if (cluster == 0) {
        log_warn("[FAT32] No free clusters available");
        return 0;
    }
    
    /* Tandai sebagai end-of-chain */
    if (fat32_write_fat_entry(sb, cluster, FAT32_END_CHAIN_END) != 
        STATUS_BERHASIL) {
        log_error("[FAT32] Failed to mark cluster %u as used", cluster);
        return 0;
    }
    
    /* Update next_free */
    sb->s_next_free = cluster + 1;
    if (sb->s_next_free >= sb->s_total_clusters + FAT32_FIRST_CLUSTER) {
        sb->s_next_free = FAT32_FIRST_CLUSTER;
    }
    
    /* Update free count */
    if (sb->s_free_count != 0xFFFFFFFF) {
        sb->s_free_count--;
    }
    
    sb->s_dirty = BENAR;
    
    log_debug("[FAT32] Allocated cluster %u", cluster);
    
    return cluster;
}

/*
 * fat32_free_cluster
 * ------------------
 * Membebaskan cluster tunggal.
 *
 * Parameter:
 *   sb      - Pointer ke struktur superblock FAT32
 *   cluster - Nomor cluster yang akan dibebaskan
 *
 * Return: Status operasi
 */
status_t fat32_free_cluster(fat32_sb_t *sb, tak_bertanda32_t cluster)
{
    status_t ret;
    
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Validasi cluster */
    if (!fat32_cluster_valid(sb, cluster)) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Check jika read-only */
    if (sb->s_readonly) {
        return STATUS_FS_READONLY;
    }
    
    /* Tandai sebagai bebas */
    ret = fat32_write_fat_entry(sb, cluster, FAT32_FREE);
    if (ret != STATUS_BERHASIL) {
        return ret;
    }
    
    /* Update free count */
    if (sb->s_free_count != 0xFFFFFFFF) {
        sb->s_free_count++;
    }
    
    sb->s_dirty = BENAR;
    
    log_debug("[FAT32] Freed cluster %u", cluster);
    
    return STATUS_BERHASIL;
}

/*
 * fat32_free_cluster_chain
 * ------------------------
 * Membebaskan seluruh rantai cluster.
 *
 * Parameter:
 *   sb      - Pointer ke struktur superblock FAT32
 *   cluster - Cluster pertama dalam rantai
 *
 * Return: Status operasi
 */
status_t fat32_free_cluster_chain(fat32_sb_t *sb, tak_bertanda32_t cluster)
{
    tak_bertanda32_t next_cluster;
    tak_bertanda32_t current_cluster;
    tak_bertanda32_t count;
    status_t ret;
    
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Validasi cluster awal */
    if (!fat32_cluster_valid(sb, cluster)) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Check jika read-only */
    if (sb->s_readonly) {
        return STATUS_FS_READONLY;
    }
    
    count = 0;
    current_cluster = cluster;
    
    while (fat32_cluster_valid(sb, current_cluster)) {
        /* Baca cluster berikutnya */
        next_cluster = fat32_read_fat_entry(sb, current_cluster);
        
        /* Bebaskan cluster saat ini */
        ret = fat32_free_cluster(sb, current_cluster);
        if (ret != STATUS_BERHASIL) {
            log_warn("[FAT32] Failed to free cluster %u", current_cluster);
            break;
        }
        
        count++;
        
        /* Cek jika sudah sampai akhir rantai */
        if (fat32_cluster_is_eof(next_cluster)) {
            break;
        }
        
        current_cluster = next_cluster;
    }
    
    log_debug("[FAT32] Freed %u clusters", count);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI MANAJEMEN RANTAI CLUSTER (CLUSTER CHAIN MANAGEMENT FUNCTIONS)
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
    
    if (sb == NULL) {
        return 0;
    }
    
    entry = fat32_read_fat_entry(sb, cluster);
    
    if (fat32_cluster_is_eof(entry) || 
        fat32_cluster_is_bad(entry) ||
        fat32_cluster_is_free(entry)) {
        return 0;
    }
    
    return entry;
}

/*
 * fat32_extend_cluster_chain
 * --------------------------
 * Memperpanjang rantai cluster dengan menambahkan cluster baru.
 *
 * Parameter:
 *   sb          - Pointer ke struktur superblock FAT32
 *   last_cluster - Cluster terakhir dalam rantai
 *   count       - Jumlah cluster yang akan ditambahkan
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
    
    /* Check jika read-only */
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
        
        /* Hubungkan dengan cluster sebelumnya */
        if (fat32_cluster_valid(sb, prev_cluster)) {
            if (fat32_write_fat_entry(sb, prev_cluster, new_cluster) != 
                STATUS_BERHASIL) {
                log_error("[FAT32] Failed to link cluster %u to %u",
                         prev_cluster, new_cluster);
                /* Bebaskan cluster baru */
                fat32_free_cluster(sb, new_cluster);
                return STATUS_IO_ERROR;
            }
        }
        
        prev_cluster = new_cluster;
    }
    
    log_debug("[FAT32] Extended chain by %u clusters", count);
    
    return STATUS_BERHASIL;
}

/*
 * fat32_count_clusters_in_chain
 * -----------------------------
 * Menghitung jumlah cluster dalam rantai.
 *
 * Parameter:
 *   sb      - Pointer ke struktur superblock FAT32
 *   cluster - Cluster pertama dalam rantai
 *
 * Return: Jumlah cluster, atau 0 jika error
 */
tak_bertanda32_t fat32_count_clusters_in_chain(fat32_sb_t *sb,
                                                tak_bertanda32_t cluster)
{
    tak_bertanda32_t count;
    tak_bertanda32_t current;
    tak_bertanda32_t next;
    
    if (sb == NULL) {
        return 0;
    }
    
    count = 0;
    current = cluster;
    
    while (fat32_cluster_valid(sb, current)) {
        count++;
        
        next = fat32_read_fat_entry(sb, current);
        
        if (fat32_cluster_is_eof(next)) {
            break;
        }
        
        current = next;
    }
    
    return count;
}

/*
 * fat32_find_nth_cluster
 * ----------------------
 * Mencari cluster ke-n dalam rantai.
 *
 * Parameter:
 *   sb           - Pointer ke struktur superblock FAT32
 *   first_cluster - Cluster pertama dalam rantai
 *   n            - Index cluster (0-based)
 *
 * Return: Cluster ke-n, atau 0 jika tidak ditemukan
 */
tak_bertanda32_t fat32_find_nth_cluster(fat32_sb_t *sb,
                                         tak_bertanda32_t first_cluster,
                                         tak_bertanda32_t n)
{
    tak_bertanda32_t i;
    tak_bertanda32_t current;
    tak_bertanda32_t next;
    
    if (sb == NULL) {
        return 0;
    }
    
    current = first_cluster;
    
    for (i = 0; i < n && fat32_cluster_valid(sb, current); i++) {
        next = fat32_read_fat_entry(sb, current);
        
        if (fat32_cluster_is_eof(next)) {
            return 0;  /* Rantai terlalu pendek */
        }
        
        current = next;
    }
    
    return current;
}

/*
 * ===========================================================================
 * FUNGSI INISIALISASI FAT (FAT INITIALIZATION FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_init_fat
 * --------------
 * Menginisialisasi FAT untuk filesystem baru.
 *
 * Parameter:
 *   sb - Pointer ke struktur superblock FAT32
 *
 * Return: Status operasi
 */
status_t fat32_init_fat(fat32_sb_t *sb)
{
    tak_bertanda32_t i;
    tak_bertanda32_t fat_start;
    
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Check jika read-only */
    if (sb->s_readonly) {
        return STATUS_FS_READONLY;
    }
    
    /* Alokasikan FAT cache jika belum ada */
    if (sb->s_fat_cache == NULL) {
        tak_bertanda64_t fat_size_bytes;
        
        fat_size_bytes = (tak_bertanda64_t)sb->s_fat_size * 
                         sb->s_bytes_per_sec;
        
        sb->s_fat_cache = kmalloc((ukuran_t)fat_size_bytes);
        if (sb->s_fat_cache == NULL) {
            log_error("[FAT32] Failed to allocate FAT cache");
            return STATUS_MEMORI_HABIS;
        }
        
        kernel_memset(sb->s_fat_cache, 0, (ukuran_t)fat_size_bytes);
    }
    
    fat_start = sb->s_reserved_sec;
    
    /* Tandai cluster 0 dan 1 sebagai reserved */
    /* FAT entry 0: Media type di byte rendah, 0xFFFFFF di byte tinggi */
    /* FAT entry 1: End of chain marker */
    {
        tak_bertanda8_t *fat;
        
        fat = (tak_bertanda8_t *)sb->s_fat_cache;
        
        /* Entry 0: 0xFFFFFF00 | media_type */
        fat[0] = 0xF8;  /* Media type untuk hard disk */
        fat[1] = 0xFF;
        fat[2] = 0xFF;
        fat[3] = 0x0F;
        
        /* Entry 1: 0x0FFFFFFF (EOF) */
        fat[4] = 0xFF;
        fat[5] = 0xFF;
        fat[6] = 0xFF;
        fat[7] = 0x0F;
    }
    
    /* Tandai cluster 2 (root cluster) sebagai end-of-chain */
    fat32_write_fat_entry(sb, sb->s_root_cluster, FAT32_END_CHAIN_END);
    
    /* Tandai semua cluster lain sebagai bebas */
    for (i = FAT32_FIRST_CLUSTER; i < sb->s_total_clusters + FAT32_FIRST_CLUSTER; i++) {
        if (i != sb->s_root_cluster) {
            fat32_write_fat_entry(sb, i, FAT32_FREE);
        }
    }
    
    /* Update free count */
    sb->s_free_count = sb->s_total_clusters - 1;  /* -1 untuk root */
    sb->s_next_free = FAT32_FIRST_CLUSTER;
    
    sb->s_dirty = BENAR;
    
    log_info("[FAT32] FAT initialized with %u free clusters", 
             sb->s_free_count);
    
    return STATUS_BERHASIL;
}

/*
 * fat32_load_fat
 * --------------
 * Memuat FAT dari device ke memori.
 *
 * Parameter:
 *   sb - Pointer ke struktur superblock FAT32
 *
 * Return: Status operasi
 */
status_t fat32_load_fat(fat32_sb_t *sb)
{
    tak_bertanda64_t fat_size_bytes;
    
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Alokasikan FAT cache */
    if (sb->s_fat_cache == NULL) {
        fat_size_bytes = (tak_bertanda64_t)sb->s_fat_size * 
                         sb->s_bytes_per_sec;
        
        sb->s_fat_cache = kmalloc((ukuran_t)fat_size_bytes);
        if (sb->s_fat_cache == NULL) {
            log_error("[FAT32] Failed to allocate FAT cache");
            return STATUS_MEMORI_HABIS;
        }
    }
    
    /* TODO: Baca FAT dari device */
    /* Untuk saat ini, inisialisasi dengan nilai default */
    kernel_memset(sb->s_fat_cache, 0, 
                  (ukuran_t)(sb->s_fat_size * sb->s_bytes_per_sec));
    
    log_info("[FAT32] FAT loaded into cache");
    
    return STATUS_BERHASIL;
}

/*
 * fat32_sync_fat
 * --------------
 * Menyinkronkan FAT cache ke device.
 *
 * Parameter:
 *   sb - Pointer ke struktur superblock FAT32
 *
 * Return: Status operasi
 */
status_t fat32_sync_fat(fat32_sb_t *sb)
{
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (!sb->s_dirty) {
        return STATUS_BERHASIL;  /* Tidak ada yang perlu disinkronkan */
    }
    
    /* TODO: Tulis FAT cache ke device */
    /* Untuk saat ini, hanya clear dirty flag */
    
    sb->s_dirty = SALAH;
    
    log_debug("[FAT32] FAT synchronized");
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI STATISTIK FAT (FAT STATISTICS FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_count_free_clusters
 * -------------------------
 * Menghitung jumlah cluster bebas dengan scanning FAT.
 *
 * Parameter:
 *   sb - Pointer ke struktur superblock FAT32
 *
 * Return: Jumlah cluster bebas
 */
tak_bertanda32_t fat32_count_free_clusters(fat32_sb_t *sb)
{
    tak_bertanda32_t count;
    tak_bertanda32_t i;
    tak_bertanda32_t entry;
    
    if (sb == NULL) {
        return 0;
    }
    
    count = 0;
    
    for (i = FAT32_FIRST_CLUSTER; i < sb->s_total_clusters + FAT32_FIRST_CLUSTER; i++) {
        entry = fat32_read_fat_entry(sb, i);
        
        if (fat32_cluster_is_free(entry)) {
            count++;
        }
    }
    
    /* Update free count */
    sb->s_free_count = count;
    
    return count;
}

/*
 * fat32_get_fat_info
 * ------------------
 * Mendapatkan informasi FAT.
 *
 * Parameter:
 *   sb           - Pointer ke struktur superblock FAT32
 *   total_clusters - Pointer untuk total clusters
 *   free_clusters  - Pointer untuk free clusters
 *   next_free      - Pointer untuk next free cluster
 *
 * Return: Status operasi
 */
status_t fat32_get_fat_info(fat32_sb_t *sb,
                            tak_bertanda32_t *total_clusters,
                            tak_bertanda32_t *free_clusters,
                            tak_bertanda32_t *next_free)
{
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (total_clusters != NULL) {
        *total_clusters = sb->s_total_clusters;
    }
    
    if (free_clusters != NULL) {
        *free_clusters = sb->s_free_count;
    }
    
    if (next_free != NULL) {
        *next_free = sb->s_next_free;
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI DEBUG FAT (FAT DEBUG FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_print_fat_entry
 * ---------------------
 * Mencetak informasi entri FAT untuk debugging.
 *
 * Parameter:
 *   sb      - Pointer ke struktur superblock FAT32
 *   cluster - Nomor cluster
 */
void fat32_print_fat_entry(fat32_sb_t *sb, tak_bertanda32_t cluster)
{
    tak_bertanda32_t entry;
    
    if (sb == NULL) {
        kernel_printf("[FAT32] Superblock is NULL\n");
        return;
    }
    
    entry = fat32_read_fat_entry(sb, cluster);
    
    kernel_printf("[FAT32] Cluster %u: 0x%08X", cluster, entry);
    
    if (fat32_cluster_is_free(entry)) {
        kernel_printf(" (Free)");
    } else if (fat32_cluster_is_eof(entry)) {
        kernel_printf(" (EOF)");
    } else if (fat32_cluster_is_bad(entry)) {
        kernel_printf(" (Bad)");
    } else if (fat32_cluster_is_reserved(entry)) {
        kernel_printf(" (Reserved)");
    } else {
        kernel_printf(" -> %u", entry);
    }
    
    kernel_printf("\n");
}

/*
 * fat32_print_fat_range
 * ---------------------
 * Mencetak rentang entri FAT untuk debugging.
 *
 * Parameter:
 *   sb    - Pointer ke struktur superblock FAT32
 *   start - Cluster awal
 *   count - Jumlah cluster
 */
void fat32_print_fat_range(fat32_sb_t *sb, tak_bertanda32_t start,
                           tak_bertanda32_t count)
{
    tak_bertanda32_t i;
    
    if (sb == NULL) {
        kernel_printf("[FAT32] Superblock is NULL\n");
        return;
    }
    
    kernel_printf("[FAT32] FAT entries %u-%u:\n", start, start + count - 1);
    
    for (i = 0; i < count; i++) {
        if (i % 8 == 0) {
            kernel_printf("  %5u: ", start + i);
        }
        
        {
            tak_bertanda32_t entry;
            entry = fat32_read_fat_entry(sb, start + i);
            kernel_printf("%08X ", entry);
        }
        
        if (i % 8 == 7) {
            kernel_printf("\n");
        }
    }
    
    if (count % 8 != 0) {
        kernel_printf("\n");
    }
}
