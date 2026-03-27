/*
 * PIGURA OS - FAT32_BOOT.C
 * ==========================
 * Implementasi pembacaan dan penulisan boot sector FAT32.
 *
 * Berkas ini berisi fungsi-fungsi untuk membaca, menulis, dan
 * memvalidasi boot sector serta FSInfo structure pada filesystem FAT32.
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
 * KONSTANTA FAT32 BOOT (FAT32 BOOT CONSTANTS)
 * ===========================================================================
 */

/* Boot sector signature */
#define FAT32_BOOT_SIGNATURE    0xAA55

/* FSInfo signatures */
#define FAT32_FSINFO_SIG1       0x41615252  /* "RRaA" */
#define FAT32_FSINFO_SIG2       0x61417272  /* "rrAa" */
#define FAT32_FSINFO_SIG3       0xAA550000

/* Media types */
#define FAT32_MEDIA_FLOPPY      0xF0
#define FAT32_MEDIA_HDD         0xF8

/* FAT types */
#define FAT_TYPE_FAT12          0
#define FAT_TYPE_FAT16          1
#define FAT_TYPE_FAT32          2

/*
 * ===========================================================================
 * STRUKTUR BOOT SECTOR (BOOT SECTOR STRUCTURE)
 * ===========================================================================
 */

typedef struct __attribute__((packed)) {
    tak_bertanda8_t  bs_jump[3];
    tak_bertanda8_t  bs_oem_name[8];
    
    tak_bertanda16_t bpb_bytes_per_sec;
    tak_bertanda8_t  bpb_sec_per_cluster;
    tak_bertanda16_t bpb_reserved_sec;
    tak_bertanda8_t  bpb_num_fats;
    tak_bertanda16_t bpb_root_entries;
    tak_bertanda16_t bpb_total_sec_16;
    tak_bertanda8_t  bpb_media_type;
    tak_bertanda16_t bpb_sec_per_fat_16;
    tak_bertanda16_t bpb_sec_per_track;
    tak_bertanda16_t bpb_num_heads;
    tak_bertanda32_t bpb_hidden_sec;
    tak_bertanda32_t bpb_total_sec_32;
    
    /* FAT32 extended */
    tak_bertanda32_t bpb_fat_size_32;
    tak_bertanda16_t bpb_ext_flags;
    tak_bertanda16_t bpb_fs_version;
    tak_bertanda32_t bpb_root_cluster;
    tak_bertanda16_t bpb_fs_info;
    tak_bertanda16_t bpb_backup_boot;
    tak_bertanda8_t  bpb_reserved[12];
    
    tak_bertanda8_t  bs_drive_num;
    tak_bertanda8_t  bs_reserved1;
    tak_bertanda8_t  bs_boot_sig;
    tak_bertanda32_t bs_volume_id;
    tak_bertanda8_t  bs_volume_label[11];
    tak_bertanda8_t  bs_fs_type[8];
    
    tak_bertanda8_t  bs_boot_code[420];
    tak_bertanda16_t bs_signature;
} fat32_boot_sector_t;

/*
 * ===========================================================================
 * STRUKTUR FSINFO (FSINFO STRUCTURE)
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
 * FUNGSI VALIDASI BOOT SECTOR (BOOT SECTOR VALIDATION FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_validate_boot_sector
 * --------------------------
 * Memvalidasi struktur boot sector FAT32.
 *
 * Parameter:
 *   boot - Pointer ke struktur boot sector
 *
 * Return: BENAR jika valid, SALAH jika tidak
 */
bool_t fat32_validate_boot_sector(const fat32_boot_sector_t *boot)
{
    tak_bertanda32_t total_clusters;
    tak_bertanda32_t fat_type;
    
    if (boot == NULL) {
        return SALAH;
    }
    
    /* Cek signature */
    if (boot->bs_signature != FAT32_BOOT_SIGNATURE) {
        log_debug("[FAT32] Invalid boot signature: 0x%04X",
                  boot->bs_signature);
        return SALAH;
    }
    
    /* Cek bytes per sector (harus power of 2 dan antara 512-4096) */
    if (boot->bpb_bytes_per_sec < 512 || 
        boot->bpb_bytes_per_sec > 4096) {
        log_debug("[FAT32] Invalid bytes per sector: %u",
                  boot->bpb_bytes_per_sec);
        return SALAH;
    }
    
    /* Cek apakah bytes per sector adalah power of 2 */
    if ((boot->bpb_bytes_per_sec & (boot->bpb_bytes_per_sec - 1)) != 0) {
        log_debug("[FAT32] Bytes per sector not power of 2: %u",
                  boot->bpb_bytes_per_sec);
        return SALAH;
    }
    
    /* Cek sectors per cluster (harus power of 2) */
    if (boot->bpb_sec_per_cluster == 0 ||
        (boot->bpb_sec_per_cluster & (boot->bpb_sec_per_cluster - 1)) != 0) {
        log_debug("[FAT32] Invalid sectors per cluster: %u",
                  boot->bpb_sec_per_cluster);
        return SALAH;
    }
    
    /* Cek reserved sectors */
    if (boot->bpb_reserved_sec == 0) {
        log_debug("[FAT32] Reserved sectors is zero");
        return SALAH;
    }
    
    /* Cek number of FATs */
    if (boot->bpb_num_fats == 0 || boot->bpb_num_fats > 2) {
        log_debug("[FAT32] Invalid number of FATs: %u",
                  boot->bpb_num_fats);
        return SALAH;
    }
    
    /* Cek total sectors */
    if (boot->bpb_total_sec_32 == 0 && boot->bpb_total_sec_16 == 0) {
        log_debug("[FAT32] Total sectors is zero");
        return SALAH;
    }
    
    /* Untuk FAT32, root entries harus 0 dan FAT16 size harus 0 */
    if (boot->bpb_root_entries != 0 || boot->bpb_sec_per_fat_16 != 0) {
        /* Kemungkinan FAT12/FAT16, bukan FAT32 */
        log_debug("[FAT32] Not FAT32 (FAT12/FAT16 detected)");
        return SALAH;
    }
    
    /* Cek FAT32 specific fields */
    if (boot->bpb_fat_size_32 == 0) {
        log_debug("[FAT32] FAT size is zero");
        return SALAH;
    }
    
    /* Hitung total clusters untuk menentukan FAT type */
    {
        tak_bertanda32_t total_sec;
        tak_bertanda32_t data_sec;
        tak_bertanda32_t root_dir_sectors;
        
        total_sec = boot->bpb_total_sec_32;
        if (total_sec == 0) {
            total_sec = boot->bpb_total_sec_16;
        }
        
        root_dir_sectors = ((boot->bpb_root_entries * 32) + 
                           (boot->bpb_bytes_per_sec - 1)) / 
                           boot->bpb_bytes_per_sec;
        
        data_sec = total_sec - 
                   (boot->bpb_reserved_sec + 
                    (boot->bpb_num_fats * boot->bpb_fat_size_32) + 
                    root_dir_sectors);
        
        total_clusters = data_sec / boot->bpb_sec_per_cluster;
    }
    
    /* Tentukan FAT type berdasarkan jumlah cluster */
    if (total_clusters < 4085) {
        fat_type = FAT_TYPE_FAT12;
        log_debug("[FAT32] FAT12 detected (%u clusters)", total_clusters);
        return SALAH;  /* Bukan FAT32 */
    } else if (total_clusters < 65525) {
        fat_type = FAT_TYPE_FAT16;
        log_debug("[FAT32] FAT16 detected (%u clusters)", total_clusters);
        return SALAH;  /* Bukan FAT32 */
    } else {
        fat_type = FAT_TYPE_FAT32;
    }
    
    /* Cek root cluster */
    if (boot->bpb_root_cluster < 2) {
        log_debug("[FAT32] Invalid root cluster: %u",
                  boot->bpb_root_cluster);
        return SALAH;
    }
    
    /* Cek FS version (harus 0) */
    if (boot->bpb_fs_version != 0) {
        log_debug("[FAT32] Unknown FS version: %u",
                  boot->bpb_fs_version);
        return SALAH;
    }
    
    return BENAR;
}

/*
 * ===========================================================================
 * FUNGSI VALIDASI FSINFO (FSINFO VALIDATION FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_validate_fsinfo
 * ---------------------
 * Memvalidasi struktur FSInfo FAT32.
 *
 * Parameter:
 *   fsinfo - Pointer ke struktur FSInfo
 *
 * Return: BENAR jika valid, SALAH jika tidak
 */
bool_t fat32_validate_fsinfo(const fat32_fsinfo_t *fsinfo)
{
    if (fsinfo == NULL) {
        return SALAH;
    }
    
    /* Cek signature 1 */
    if (fsinfo->fs_signature1 != FAT32_FSINFO_SIG1) {
        log_debug("[FAT32] Invalid FSInfo signature1: 0x%08X",
                  fsinfo->fs_signature1);
        return SALAH;
    }
    
    /* Cek signature 2 */
    if (fsinfo->fs_signature2 != FAT32_FSINFO_SIG2) {
        log_debug("[FAT32] Invalid FSInfo signature2: 0x%08X",
                  fsinfo->fs_signature2);
        return SALAH;
    }
    
    /* Cek signature 3 (optional, tidak semua implementasi memiliki ini) */
    /* Signature 3 tidak wajib divalidasi */
    
    return BENAR;
}

/*
 * ===========================================================================
 * FUNGSI PEMBACAAN BOOT SECTOR (BOOT SECTOR READ FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_read_boot_sector
 * ----------------------
 * Membaca dan mem-parse boot sector FAT32 dari buffer.
 *
 * Parameter:
 *   sb     - Pointer ke struktur superblock FAT32
 *   buffer - Buffer yang berisi data boot sector
 *
 * Return: Status operasi
 */
status_t fat32_read_boot_sector(fat32_sb_t *sb, const void *buffer)
{
    const fat32_boot_sector_t *boot;
    tak_bertanda32_t total_sec;
    tak_bertanda32_t root_dir_sectors;
    tak_bertanda32_t data_sec;
    
    if (sb == NULL || buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    boot = (const fat32_boot_sector_t *)buffer;
    
    /* Validasi boot sector */
    if (!fat32_validate_boot_sector(boot)) {
        log_error("[FAT32] Boot sector validation failed");
        return STATUS_FORMAT_INVALID;
    }
    
    /* Parse informasi dasar */
    sb->s_magic = FAT32_BOOT_SIGNATURE;
    sb->s_volume_id = boot->bs_volume_id;
    
    /* Copy volume label */
    kernel_memcpy(sb->s_volume_label, boot->bs_volume_label, 11);
    sb->s_volume_label[11] = '\0';
    
    /* Trim trailing spaces */
    {
        int i;
        for (i = 10; i >= 0 && sb->s_volume_label[i] == ' '; i--) {
            sb->s_volume_label[i] = '\0';
        }
    }
    
    /* Copy FS type */
    kernel_memcpy(sb->s_fs_type, boot->bs_fs_type, 8);
    sb->s_fs_type[8] = '\0';
    
    /* Parse BPB */
    sb->s_bytes_per_sec = boot->bpb_bytes_per_sec;
    sb->s_sec_per_cluster = boot->bpb_sec_per_cluster;
    sb->s_bytes_per_cluster = sb->s_bytes_per_sec * sb->s_sec_per_cluster;
    sb->s_reserved_sec = boot->bpb_reserved_sec;
    sb->s_num_fats = boot->bpb_num_fats;
    sb->s_fat_size = boot->bpb_fat_size_32;
    sb->s_root_cluster = boot->bpb_root_cluster;
    
    /* Hitung total sectors */
    if (boot->bpb_total_sec_32 != 0) {
        total_sec = boot->bpb_total_sec_32;
    } else {
        total_sec = boot->bpb_total_sec_16;
    }
    sb->s_total_sec = total_sec;
    
    /* Hitung data start sector */
    root_dir_sectors = ((boot->bpb_root_entries * 32) + 
                        (sb->s_bytes_per_sec - 1)) / 
                        sb->s_bytes_per_sec;
    
    sb->s_data_start_sec = sb->s_reserved_sec + 
                           (sb->s_num_fats * sb->s_fat_size) + 
                           root_dir_sectors;
    
    /* Hitung total clusters */
    data_sec = total_sec - sb->s_data_start_sec;
    sb->s_total_clusters = data_sec / sb->s_sec_per_cluster;
    
    log_info("[FAT32] Boot sector parsed successfully");
    log_info("[FAT32] Volume: %.11s, Type: %.8s",
             boot->bs_volume_label, boot->bs_fs_type);
    log_info("[FAT32] Bytes/Sector: %u, Sectors/Cluster: %u",
             sb->s_bytes_per_sec, sb->s_sec_per_cluster);
    log_info("[FAT32] Total Clusters: %u, Root Cluster: %u",
             sb->s_total_clusters, sb->s_root_cluster);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PEMBACAAN FSINFO (FSINFO READ FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_read_fsinfo
 * -----------------
 * Membaca dan mem-parse FSInfo structure dari buffer.
 *
 * Parameter:
 *   sb     - Pointer ke struktur superblock FAT32
 *   buffer - Buffer yang berisi data FSInfo
 *
 * Return: Status operasi
 */
status_t fat32_read_fsinfo(fat32_sb_t *sb, const void *buffer)
{
    const fat32_fsinfo_t *fsinfo;
    
    if (sb == NULL || buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    fsinfo = (const fat32_fsinfo_t *)buffer;
    
    /* Validasi FSInfo */
    if (!fat32_validate_fsinfo(fsinfo)) {
        log_warn("[FAT32] FSInfo validation failed, using defaults");
        sb->s_free_count = 0xFFFFFFFF;
        sb->s_next_free = 0xFFFFFFFF;
        return STATUS_BERHASIL;  /* Non-critical error */
    }
    
    /* Parse free count dan next free */
    sb->s_free_count = fsinfo->fs_free_count;
    sb->s_next_free = fsinfo->fs_next_free;
    
    /* Validasi nilai */
    if (sb->s_free_count > sb->s_total_clusters) {
        log_warn("[FAT32] Invalid free count, recalculating needed");
        sb->s_free_count = 0xFFFFFFFF;  /* Unknown */
    }
    
    if (sb->s_next_free < 2 || sb->s_next_free > sb->s_total_clusters + 1) {
        sb->s_next_free = 2;  /* Start from first valid cluster */
    }
    
    log_info("[FAT32] FSInfo parsed: Free=%u, NextFree=%u",
             sb->s_free_count, sb->s_next_free);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PENULISAN BOOT SECTOR (BOOT SECTOR WRITE FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_write_boot_sector
 * -----------------------
 * Menulis boot sector FAT32 ke buffer.
 *
 * Parameter:
 *   sb     - Pointer ke struktur superblock FAT32
 *   buffer - Buffer tujuan untuk data boot sector
 *
 * Return: Status operasi
 */
status_t fat32_write_boot_sector(fat32_sb_t *sb, void *buffer)
{
    fat32_boot_sector_t *boot;
    
    if (sb == NULL || buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    boot = (fat32_boot_sector_t *)buffer;
    kernel_memset(boot, 0, sizeof(fat32_boot_sector_t));
    
    /* Jump instruction */
    boot->bs_jump[0] = 0xEB;  /* JMP */
    boot->bs_jump[1] = 0x58;  /* Offset */
    boot->bs_jump[2] = 0x90;  /* NOP */
    
    /* OEM name */
    kernel_strncpy((char *)boot->bs_oem_name, "PIGURAOS", 8);
    
    /* BPB */
    boot->bpb_bytes_per_sec = (tak_bertanda16_t)sb->s_bytes_per_sec;
    boot->bpb_sec_per_cluster = sb->s_sec_per_cluster;
    boot->bpb_reserved_sec = (tak_bertanda16_t)sb->s_reserved_sec;
    boot->bpb_num_fats = (tak_bertanda8_t)sb->s_num_fats;
    boot->bpb_root_entries = 0;  /* FAT32 */
    boot->bpb_total_sec_16 = 0;  /* FAT32 */
    boot->bpb_media_type = FAT32_MEDIA_HDD;
    boot->bpb_sec_per_fat_16 = 0;  /* FAT32 */
    boot->bpb_sec_per_track = 63;  /* Default */
    boot->bpb_num_heads = 255;     /* Default */
    boot->bpb_hidden_sec = 0;
    boot->bpb_total_sec_32 = sb->s_total_sec;
    
    /* FAT32 extended */
    boot->bpb_fat_size_32 = sb->s_fat_size;
    boot->bpb_ext_flags = 0;
    boot->bpb_fs_version = 0;
    boot->bpb_root_cluster = sb->s_root_cluster;
    boot->bpb_fs_info = 1;  /* FSInfo at sector 1 */
    boot->bpb_backup_boot = 6;  /* Backup at sector 6 */
    
    /* Extended boot record */
    boot->bs_drive_num = 0x80;  /* Hard disk */
    boot->bs_reserved1 = 0;
    boot->bs_boot_sig = 0x29;  /* Extended boot signature */
    boot->bs_volume_id = sb->s_volume_id;
    
    /* Volume label */
    kernel_memset(boot->bs_volume_label, ' ', 11);
    kernel_strncpy((char *)boot->bs_volume_label, sb->s_volume_label, 11);
    
    /* FS type */
    kernel_strncpy((char *)boot->bs_fs_type, "FAT32   ", 8);
    
    /* Signature */
    boot->bs_signature = FAT32_BOOT_SIGNATURE;
    
    log_info("[FAT32] Boot sector written");
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PENULISAN FSINFO (FSINFO WRITE FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_write_fsinfo
 * ------------------
 * Menulis FSInfo structure ke buffer.
 *
 * Parameter:
 *   sb     - Pointer ke struktur superblock FAT32
 *   buffer - Buffer tujuan untuk data FSInfo
 *
 * Return: Status operasi
 */
status_t fat32_write_fsinfo(fat32_sb_t *sb, void *buffer)
{
    fat32_fsinfo_t *fsinfo;
    
    if (sb == NULL || buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    fsinfo = (fat32_fsinfo_t *)buffer;
    kernel_memset(fsinfo, 0, sizeof(fat32_fsinfo_t));
    
    /* Signatures */
    fsinfo->fs_signature1 = FAT32_FSINFO_SIG1;
    fsinfo->fs_signature2 = FAT32_FSINFO_SIG2;
    fsinfo->fs_signature3 = FAT32_FSINFO_SIG3;
    
    /* Free count dan next free */
    fsinfo->fs_free_count = sb->s_free_count;
    fsinfo->fs_next_free = sb->s_next_free;
    
    log_debug("[FAT32] FSInfo written: Free=%u, NextFree=%u",
              sb->s_free_count, sb->s_next_free);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI DETEKSI FAT TYPE (FAT TYPE DETECTION FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_detect_fat_type
 * ---------------------
 * Mendeteksi tipe FAT berdasarkan jumlah cluster.
 *
 * Parameter:
 *   total_clusters - Total jumlah cluster
 *
 * Return: Tipe FAT (FAT12, FAT16, atau FAT32)
 */
tak_bertanda32_t fat32_detect_fat_type(tak_bertanda32_t total_clusters)
{
    if (total_clusters < 4085) {
        return FAT_TYPE_FAT12;
    } else if (total_clusters < 65525) {
        return FAT_TYPE_FAT16;
    } else {
        return FAT_TYPE_FAT32;
    }
}

/*
 * ===========================================================================
 * FUNGSI INFORMASI BOOT SECTOR (BOOT SECTOR INFO FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_get_volume_label
 * ----------------------
 * Mendapatkan volume label dari boot sector.
 *
 * Parameter:
 *   sb   - Pointer ke struktur superblock FAT32
 *   label - Buffer untuk menyimpan label (min 12 bytes)
 *
 * Return: Status operasi
 */
status_t fat32_get_volume_label(fat32_sb_t *sb, char *label)
{
    if (sb == NULL || label == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    kernel_strncpy(label, sb->s_volume_label, 11);
    label[11] = '\0';
    
    return STATUS_BERHASIL;
}

/*
 * fat32_get_volume_id
 * -------------------
 * Mendapatkan volume ID dari boot sector.
 *
 * Parameter:
 *   sb - Pointer ke struktur superblock FAT32
 *
 * Return: Volume ID
 */
tak_bertanda32_t fat32_get_volume_id(fat32_sb_t *sb)
{
    if (sb == NULL) {
        return 0;
    }
    
    return sb->s_volume_id;
}

/*
 * fat32_get_geometry
 * ------------------
 * Mendapatkan informasi geometry filesystem.
 *
 * Parameter:
 *   sb              - Pointer ke struktur superblock FAT32
 *   bytes_per_sec   - Pointer untuk bytes per sector
 *   sec_per_cluster - Pointer untuk sectors per cluster
 *   total_clusters  - Pointer untuk total clusters
 *
 * Return: Status operasi
 */
status_t fat32_get_geometry(fat32_sb_t *sb,
                            tak_bertanda32_t *bytes_per_sec,
                            tak_bertanda8_t *sec_per_cluster,
                            tak_bertanda32_t *total_clusters)
{
    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (bytes_per_sec != NULL) {
        *bytes_per_sec = sb->s_bytes_per_sec;
    }
    
    if (sec_per_cluster != NULL) {
        *sec_per_cluster = sb->s_sec_per_cluster;
    }
    
    if (total_clusters != NULL) {
        *total_clusters = sb->s_total_clusters;
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PRINT BOOT SECTOR (BOOT SECTOR PRINT FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_print_boot_sector
 * -----------------------
 * Mencetak informasi boot sector untuk debugging.
 *
 * Parameter:
 *   buffer - Buffer yang berisi data boot sector
 */
void fat32_print_boot_sector(const void *buffer)
{
    const fat32_boot_sector_t *boot;
    
    if (buffer == NULL) {
        kernel_printf("[FAT32] Boot sector buffer is NULL\n");
        return;
    }
    
    boot = (const fat32_boot_sector_t *)buffer;
    
    kernel_printf("\n[FAT32] Boot Sector Info:\n");
    kernel_printf("========================================\n");
    kernel_printf("OEM Name:        %.8s\n", boot->bs_oem_name);
    kernel_printf("Bytes/Sector:    %u\n", boot->bpb_bytes_per_sec);
    kernel_printf("Sectors/Cluster: %u\n", boot->bpb_sec_per_cluster);
    kernel_printf("Reserved Sectors:%u\n", boot->bpb_reserved_sec);
    kernel_printf("Number of FATs:  %u\n", boot->bpb_num_fats);
    kernel_printf("Root Entries:    %u\n", boot->bpb_root_entries);
    kernel_printf("Total Sectors:   %u\n", 
                  boot->bpb_total_sec_32 ? boot->bpb_total_sec_32 : 
                                           boot->bpb_total_sec_16);
    kernel_printf("Media Type:      0x%02X\n", boot->bpb_media_type);
    kernel_printf("Sectors/FAT:     %u\n", boot->bpb_fat_size_32);
    kernel_printf("Sectors/Track:   %u\n", boot->bpb_sec_per_track);
    kernel_printf("Number of Heads: %u\n", boot->bpb_num_heads);
    kernel_printf("Hidden Sectors:  %u\n", boot->bpb_hidden_sec);
    kernel_printf("Root Cluster:    %u\n", boot->bpb_root_cluster);
    kernel_printf("FS Info Sector:  %u\n", boot->bpb_fs_info);
    kernel_printf("Backup Boot:     %u\n", boot->bpb_backup_boot);
    kernel_printf("Drive Number:    0x%02X\n", boot->bs_drive_num);
    kernel_printf("Volume ID:       0x%08X\n", boot->bs_volume_id);
    kernel_printf("Volume Label:    %.11s\n", boot->bs_volume_label);
    kernel_printf("FS Type:         %.8s\n", boot->bs_fs_type);
    kernel_printf("Signature:       0x%04X\n", boot->bs_signature);
    kernel_printf("========================================\n");
}

/*
 * fat32_print_fsinfo
 * ------------------
 * Mencetak informasi FSInfo untuk debugging.
 *
 * Parameter:
 *   buffer - Buffer yang berisi data FSInfo
 */
void fat32_print_fsinfo(const void *buffer)
{
    const fat32_fsinfo_t *fsinfo;
    
    if (buffer == NULL) {
        kernel_printf("[FAT32] FSInfo buffer is NULL\n");
        return;
    }
    
    fsinfo = (const fat32_fsinfo_t *)buffer;
    
    kernel_printf("\n[FAT32] FSInfo:\n");
    kernel_printf("========================================\n");
    kernel_printf("Signature 1:     0x%08X\n", fsinfo->fs_signature1);
    kernel_printf("Signature 2:     0x%08X\n", fsinfo->fs_signature2);
    kernel_printf("Free Count:      %u\n", fsinfo->fs_free_count);
    kernel_printf("Next Free:       %u\n", fsinfo->fs_next_free);
    kernel_printf("Signature 3:     0x%08X\n", fsinfo->fs_signature3);
    kernel_printf("========================================\n");
}
