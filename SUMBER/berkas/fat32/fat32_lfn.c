/*
 * PIGURA OS - FAT32_LFN.C
 * =========================
 * Implementasi Long Filename (LFN) untuk filesystem FAT32.
 *
 * Berkas ini berisi fungsi-fungsi untuk membaca, menulis, dan
 * mengelola Long Filename entries pada filesystem FAT32.
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
 * KONSTANTA LFN (LFN CONSTANTS)
 * ===========================================================================
 */

/* LFN entry attributes */
#define FAT32_ATTR_LONG_NAME    0x0F

/* LFN entry type */
#define FAT32_LFN_TYPE_NORMAL   0x00
#define FAT32_LFN_TYPE_DELETED  0xE5

/* LFN sequence flags */
#define FAT32_LFN_LAST_ENTRY    0x40
#define FAT32_LFN_MASK          0x1F

/* Maximum LFN length */
#define FAT32_LFN_MAX_CHARS     255

/* Characters per LFN entry */
#define FAT32_LFN_CHARS_PER_ENTRY   13  /* 5 + 6 + 2 = 13 chars */

/* Directory entry size */
#define FAT32_DIR_ENTRY_SIZE    32

/* Special markers */
#define FAT32_DIR_ENTRY_FREE    0xE5
#define FAT32_DIR_ENTRY_LAST    0x00

/*
 * ===========================================================================
 * STRUKTUR LFN ENTRY (LFN ENTRY STRUCTURE)
 * ===========================================================================
 */

typedef struct __attribute__((packed)) {
    tak_bertanda8_t  lfn_seq;           /* Sequence number */
    tak_bertanda16_t lfn_name1[5];      /* First 5 characters */
    tak_bertanda8_t  lfn_attr;          /* Attribute (always 0x0F) */
    tak_bertanda8_t  lfn_type;          /* Type (always 0) */
    tak_bertanda8_t  lfn_checksum;      /* Checksum of short name */
    tak_bertanda16_t lfn_name2[6];      /* Next 6 characters */
    tak_bertanda16_t lfn_fst_cluster;   /* Always 0 */
    tak_bertanda16_t lfn_name3[2];      /* Last 2 characters */
} fat32_lfn_entry_t;

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
 * FUNGSI DETEKSI LFN (LFN DETECTION FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_is_lfn_entry
 * ------------------
 * Memeriksa apakah entry adalah LFN entry.
 *
 * Parameter:
 *   entry - Pointer ke directory entry
 *
 * Return: BENAR jika LFN, SALAH jika tidak
 */
bool_t fat32_is_lfn_entry(const fat32_dir_entry_t *entry)
{
    if (entry == NULL) {
        return SALAH;
    }
    
    return (entry->dir_attr == FAT32_ATTR_LONG_NAME);
}

/*
 * fat32_is_lfn_deleted
 * --------------------
 * Memeriksa apakah LFN entry dihapus.
 *
 * Parameter:
 *   entry - Pointer ke LFN entry
 *
 * Return: BENAR jika dihapus, SALAH jika tidak
 */
bool_t fat32_is_lfn_deleted(const fat32_lfn_entry_t *entry)
{
    if (entry == NULL) {
        return SALAH;
    }
    
    return (entry->lfn_seq == FAT32_DIR_ENTRY_FREE);
}

/*
 * fat32_is_lfn_last
 * -----------------
 * Memeriksa apakah ini adalah LFN entry terakhir.
 *
 * Parameter:
 *   entry - Pointer ke LFN entry
 *
 * Return: BENAR jika entry terakhir, SALAH jika tidak
 */
bool_t fat32_is_lfn_last(const fat32_lfn_entry_t *entry)
{
    if (entry == NULL) {
        return SALAH;
    }
    
    return ((entry->lfn_seq & FAT32_LFN_LAST_ENTRY) != 0);
}

/*
 * ===========================================================================
 * FUNGSI CHECKSUM LFN (LFN CHECKSUM FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_lfn_checksum
 * ------------------
 * Menghitung checksum untuk Short Filename.
 *
 * Parameter:
 *   sfn - Short filename (11 bytes)
 *
 * Return: Checksum value
 */
tak_bertanda8_t fat32_lfn_checksum(const tak_bertanda8_t sfn[11])
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
 * fat32_verify_lfn_checksum
 * -------------------------
 * Memverifikasi checksum LFN.
 *
 * Parameter:
 *   sfn      - Short filename (11 bytes)
 *   checksum - Checksum yang akan diverifikasi
 *
 * Return: BENAR jika valid, SALAH jika tidak
 */
bool_t fat32_verify_lfn_checksum(const tak_bertanda8_t sfn[11],
                                  tak_bertanda8_t checksum)
{
    tak_bertanda8_t calculated;
    
    calculated = fat32_lfn_checksum(sfn);
    
    return (calculated == checksum);
}

/*
 * ===========================================================================
 * FUNGSI KONVERSI NAMA LFN (LFN NAME CONVERSION FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_lfn_entries_needed
 * ------------------------
 * Menghitung jumlah LFN entry yang diperlukan untuk nama.
 *
 * Parameter:
 *   name - Nama panjang
 *
 * Return: Jumlah entry yang diperlukan
 */
tak_bertanda32_t fat32_lfn_entries_needed(const char *name)
{
    ukuran_t len;
    tak_bertanda32_t entries;
    
    if (name == NULL) {
        return 0;
    }
    
    len = kernel_strlen(name);
    
    if (len == 0) {
        return 0;
    }
    
    /* Setiap entry menyimpan 13 karakter */
    entries = (tak_bertanda32_t)((len + FAT32_LFN_CHARS_PER_ENTRY - 1) / 
                                  FAT32_LFN_CHARS_PER_ENTRY);
    
    return entries;
}

/*
 * fat32_unicode_to_ascii
 * ----------------------
 * Mengkonversi karakter Unicode (UTF-16LE) ke ASCII.
 *
 * Parameter:
 *   unicode - Karakter Unicode
 *
 * Return: Karakter ASCII, atau 0 jika tidak valid
 */
static char fat32_unicode_to_ascii(tak_bertanda16_t unicode)
{
    /* Untuk nama file FAT32, biasanya hanya ASCII yang digunakan */
    if (unicode < 0x80) {
        return (char)unicode;
    }
    
    /* Karakter non-ASCII diganti dengan '_' */
    return '_';
}

/*
 * fat32_ascii_to_unicode
 * ----------------------
 * Mengkonversi karakter ASCII ke Unicode (UTF-16LE).
 *
 * Parameter:
 *   ascii - Karakter ASCII
 *
 * Return: Karakter Unicode
 */
static tak_bertanda16_t fat32_ascii_to_unicode(char ascii)
{
    return (tak_bertanda16_t)(tak_bertanda8_t)ascii;
}

/*
 * ===========================================================================
 * FUNGSI PEMBACAAN LFN (LFN READ FUNCTIONS)
 * ===========================================================================
 */

/* Forward declarations */
status_t fat32_read_dir_entry(fat32_sb_t *sb, tak_bertanda32_t dir_cluster,
                               tak_bertanda32_t index, fat32_dir_entry_t *entry);
status_t fat32_write_dir_entry(fat32_sb_t *sb, tak_bertanda32_t dir_cluster,
                                tak_bertanda32_t index,
                                const fat32_dir_entry_t *entry);

/*
 * fat32_read_lfn_entry
 * --------------------
 * Membaca LFN entry pada index tertentu.
 *
 * Parameter:
 *   sb          - Pointer ke struktur superblock FAT32
 *   dir_cluster - Cluster direktori
 *   index       - Index entry
 *   lfn         - Pointer ke buffer LFN entry
 *
 * Return: Status operasi
 */
status_t fat32_read_lfn_entry(fat32_sb_t *sb, tak_bertanda32_t dir_cluster,
                               tak_bertanda32_t index,
                               fat32_lfn_entry_t *lfn)
{
    status_t ret;
    fat32_dir_entry_t entry;
    
    if (sb == NULL || lfn == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Baca sebagai directory entry biasa */
    ret = fat32_read_dir_entry(sb, dir_cluster, index, &entry);
    if (ret != STATUS_BERHASIL) {
        return ret;
    }
    
    /* Konversi ke LFN entry */
    kernel_memcpy(lfn, &entry, sizeof(fat32_lfn_entry_t));
    
    return STATUS_BERHASIL;
}

/*
 * fat32_read_lfn
 * --------------
 * Membaca Long Filename dari serangkaian LFN entries.
 *
 * Parameter:
 *   sb          - Pointer ke struktur superblock FAT32
 *   dir_cluster - Cluster direktori
 *   sfn_index   - Index dari Short Filename entry
 *   name        - Buffer untuk nama (minimal 256 bytes)
 *   size        - Ukuran buffer
 *
 * Return: Status operasi
 */
status_t fat32_read_lfn(fat32_sb_t *sb, tak_bertanda32_t dir_cluster,
                         tak_bertanda32_t sfn_index, char *name,
                         ukuran_t size)
{
    fat32_lfn_entry_t lfn;
    fat32_dir_entry_t sfn;
    tak_bertanda32_t lfn_index;
    tak_bertanda32_t i;
    tak_bertanda32_t j;
    tak_bertanda8_t checksum;
    char temp_name[FAT32_LFN_MAX_CHARS + 1];
    tak_bertanda32_t name_pos;
    status_t ret;
    
    if (sb == NULL || name == NULL || size == 0) {
        return STATUS_PARAM_NULL;
    }
    
    kernel_memset(name, 0, size);
    kernel_memset(temp_name, 0, sizeof(temp_name));
    
    /* Baca SFN untuk mendapatkan checksum */
    ret = fat32_read_dir_entry(sb, dir_cluster, sfn_index, &sfn);
    if (ret != STATUS_BERHASIL) {
        return ret;
    }
    
    checksum = fat32_lfn_checksum(sfn.dir_name);
    
    /* Cari LFN entries sebelum SFN */
    lfn_index = sfn_index - 1;
    name_pos = 0;
    
    while (lfn_index < sfn_index) {  /* Handle wrap-around */
        ret = fat32_read_lfn_entry(sb, dir_cluster, lfn_index, &lfn);
        if (ret != STATUS_BERHASIL) {
            break;
        }
        
        /* Cek jika bukan LFN entry */
        if (lfn.lfn_attr != FAT32_ATTR_LONG_NAME) {
            break;
        }
        
        /* Verifikasi checksum */
        if (lfn.lfn_checksum != checksum) {
            log_warn("[FAT32] LFN checksum mismatch");
            break;
        }
        
        /* Ekstrak karakter dari entry ini */
        {
            tak_bertanda32_t seq;
            tak_bertanda32_t offset;
            
            seq = lfn.lfn_seq & FAT32_LFN_MASK;
            offset = (seq - 1) * FAT32_LFN_CHARS_PER_ENTRY;
            
            /* Copy characters dari name1 (5 chars) */
            for (i = 0; i < 5 && offset + i < FAT32_LFN_MAX_CHARS; i++) {
                if (lfn.lfn_name1[i] == 0 || lfn.lfn_name1[i] == 0xFFFF) {
                    break;
                }
                temp_name[offset + i] = fat32_unicode_to_ascii(lfn.lfn_name1[i]);
            }
            
            /* Copy characters dari name2 (6 chars) */
            for (j = 0; j < 6 && offset + 5 + j < FAT32_LFN_MAX_CHARS; j++) {
                if (lfn.lfn_name2[j] == 0 || lfn.lfn_name2[j] == 0xFFFF) {
                    break;
                }
                temp_name[offset + 5 + j] = fat32_unicode_to_ascii(lfn.lfn_name2[j]);
            }
            
            /* Copy characters dari name3 (2 chars) */
            for (j = 0; j < 2 && offset + 11 + j < FAT32_LFN_MAX_CHARS; j++) {
                if (lfn.lfn_name3[j] == 0 || lfn.lfn_name3[j] == 0xFFFF) {
                    break;
                }
                temp_name[offset + 11 + j] = fat32_unicode_to_ascii(lfn.lfn_name3[j]);
            }
        }
        
        /* Jika ini entry terakhir, selesai */
        if (lfn.lfn_seq & FAT32_LFN_LAST_ENTRY) {
            break;
        }
        
        /* Lanjut ke entry sebelumnya */
        lfn_index--;
    }
    
    /* Copy ke buffer output */
    {
        ukuran_t len;
        len = kernel_strlen(temp_name);
        if (len >= size) {
            len = size - 1;
        }
        kernel_strncpy(name, temp_name, len);
        name[len] = '\0';
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PENULISAN LFN (LFN WRITE FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_write_lfn_entry
 * ---------------------
 * Menulis satu LFN entry.
 *
 * Parameter:
 *   sb          - Pointer ke struktur superblock FAT32
 *   dir_cluster - Cluster direktori
 *   index       - Index entry
 *   lfn         - Pointer ke LFN entry
 *
 * Return: Status operasi
 */
status_t fat32_write_lfn_entry(fat32_sb_t *sb, tak_bertanda32_t dir_cluster,
                                tak_bertanda32_t index,
                                const fat32_lfn_entry_t *lfn)
{
    if (sb == NULL || lfn == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    return fat32_write_dir_entry(sb, dir_cluster, index,
                                  (const fat32_dir_entry_t *)lfn);
}

/*
 * fat32_write_lfn
 * ---------------
 * Menulis Long Filename sebagai serangkaian LFN entries.
 *
 * Parameter:
 *   sb          - Pointer ke struktur superblock FAT32
 *   dir_cluster - Cluster direktori
 *   name        - Nama panjang
 *   sfn_index   - Index dari SFN entry
 *   sfn         - Short filename (11 bytes)
 *
 * Return: Status operasi
 */
status_t fat32_write_lfn(fat32_sb_t *sb, tak_bertanda32_t dir_cluster,
                          const char *name, tak_bertanda32_t sfn_index,
                          const tak_bertanda8_t sfn[11])
{
    fat32_lfn_entry_t lfn;
    tak_bertanda32_t entries_needed;
    tak_bertanda32_t i;
    tak_bertanda32_t j;
    tak_bertanda32_t char_index;
    tak_bertanda8_t checksum;
    ukuran_t name_len;
    status_t ret;
    
    if (sb == NULL || name == NULL || sfn == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (sb->s_readonly) {
        return STATUS_FS_READONLY;
    }
    
    /* Hitung entries yang diperlukan */
    entries_needed = fat32_lfn_entries_needed(name);
    if (entries_needed == 0) {
        return STATUS_BERHASIL;  /* Nama kosong, tidak perlu LFN */
    }
    
    /* Hitung checksum */
    checksum = fat32_lfn_checksum(sfn);
    
    name_len = kernel_strlen(name);
    
    /* Tulis LFN entries dari belakang ke depan */
    for (i = entries_needed; i >= 1; i--) {
        tak_bertanda32_t lfn_index;
        
        kernel_memset(&lfn, 0, sizeof(lfn));
        
        /* Set sequence number */
        lfn.lfn_seq = (tak_bertanda8_t)i;
        if (i == entries_needed) {
            lfn.lfn_seq |= FAT32_LFN_LAST_ENTRY;
        }
        
        /* Set attribute dan type */
        lfn.lfn_attr = FAT32_ATTR_LONG_NAME;
        lfn.lfn_type = FAT32_LFN_TYPE_NORMAL;
        lfn.lfn_checksum = checksum;
        lfn.lfn_fst_cluster = 0;
        
        /* Calculate starting character position */
        char_index = (i - 1) * FAT32_LFN_CHARS_PER_ENTRY;
        
        /* Fill name1 (5 chars) */
        for (j = 0; j < 5; j++) {
            if (char_index + j < name_len) {
                lfn.lfn_name1[j] = fat32_ascii_to_unicode(name[char_index + j]);
            } else if (char_index + j == name_len) {
                lfn.lfn_name1[j] = 0x0000;  /* Null terminator */
            } else {
                lfn.lfn_name1[j] = 0xFFFF;  /* Padding */
            }
        }
        
        /* Fill name2 (6 chars) */
        for (j = 0; j < 6; j++) {
            if (char_index + 5 + j < name_len) {
                lfn.lfn_name2[j] = fat32_ascii_to_unicode(name[char_index + 5 + j]);
            } else if (char_index + 5 + j == name_len) {
                lfn.lfn_name2[j] = 0x0000;
            } else {
                lfn.lfn_name2[j] = 0xFFFF;
            }
        }
        
        /* Fill name3 (2 chars) */
        for (j = 0; j < 2; j++) {
            if (char_index + 11 + j < name_len) {
                lfn.lfn_name3[j] = fat32_ascii_to_unicode(name[char_index + 11 + j]);
            } else if (char_index + 11 + j == name_len) {
                lfn.lfn_name3[j] = 0x0000;
            } else {
                lfn.lfn_name3[j] = 0xFFFF;
            }
        }
        
        /* Write entry */
        lfn_index = sfn_index - (entries_needed - i + 1);
        
        ret = fat32_write_lfn_entry(sb, dir_cluster, lfn_index, &lfn);
        if (ret != STATUS_BERHASIL) {
            return ret;
        }
    }
    
    log_debug("[FAT32] Written LFN '%s' (%u entries)", name, entries_needed);
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PENGHAPUSAN LFN (LFN DELETION FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_delete_lfn
 * ----------------
 * Menghapus LFN entries sebelum SFN.
 *
 * Parameter:
 *   sb          - Pointer ke struktur superblock FAT32
 *   dir_cluster - Cluster direktori
 *   sfn_index   - Index dari SFN entry
 *   sfn         - Short filename untuk validasi
 *
 * Return: Status operasi
 */
status_t fat32_delete_lfn(fat32_sb_t *sb, tak_bertanda32_t dir_cluster,
                           tak_bertanda32_t sfn_index,
                           const tak_bertanda8_t sfn[11])
{
    fat32_lfn_entry_t lfn;
    tak_bertanda8_t checksum;
    tak_bertanda32_t index;
    status_t ret;
    
    if (sb == NULL || sfn == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (sb->s_readonly) {
        return STATUS_FS_READONLY;
    }
    
    checksum = fat32_lfn_checksum(sfn);
    
    /* Scan backwards from SFN */
    index = sfn_index - 1;
    
    while (index < sfn_index) {  /* Handle wrap-around */
        ret = fat32_read_lfn_entry(sb, dir_cluster, index, &lfn);
        if (ret != STATUS_BERHASIL) {
            break;
        }
        
        /* Cek jika bukan LFN */
        if (lfn.lfn_attr != FAT32_ATTR_LONG_NAME) {
            break;
        }
        
        /* Cek checksum */
        if (lfn.lfn_checksum != checksum) {
            break;
        }
        
        /* Tandai sebagai dihapus */
        {
            fat32_dir_entry_t deleted;
            kernel_memset(&deleted, 0, sizeof(deleted));
            deleted.dir_name[0] = FAT32_DIR_ENTRY_FREE;
            
            fat32_write_dir_entry(sb, dir_cluster, index, &deleted);
        }
        
        /* Jika ini entry terakhir, selesai */
        if (lfn.lfn_seq & FAT32_LFN_LAST_ENTRY) {
            break;
        }
        
        index--;
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI GENERATE SFN (SFN GENERATION FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_needs_lfn
 * ---------------
 * Memeriksa apakah nama membutuhkan LFN.
 *
 * Parameter:
 *   name - Nama file
 *
 * Return: BENAR jika membutuhkan LFN, SALAH jika tidak
 */
bool_t fat32_needs_lfn(const char *name)
{
    ukuran_t len;
    ukuran_t i;
    ukuran_t base_len;
    const char *dot;
    
    if (name == NULL) {
        return SALAH;
    }
    
    len = kernel_strlen(name);
    
    /* Cek panjang */
    if (len > 11) {
        return BENAR;
    }
    
    /* Cari ekstensi */
    dot = kernel_strrchr(name, '.');
    
    if (dot != NULL) {
        /* Ada ekstensi */
        base_len = (ukuran_t)(dot - name);
        
        /* Nama utama > 8 atau ekstensi > 3 */
        if (base_len > 8 || (len - base_len - 1) > 3) {
            return BENAR;
        }
    } else {
        /* Tanpa ekstensi, nama > 8 */
        if (len > 8) {
            return BENAR;
        }
    }
    
    /* Cek karakter valid untuk 8.3 */
    for (i = 0; i < len; i++) {
        char c = name[i];
        
        if (c == '.') {
            continue;  /* Separator ekstensi valid */
        }
        
        /* Cek karakter valid */
        if (!((c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'z'))) {
            return BENAR;
        }
        
        /* Lowercase membutuhkan LFN untuk preserve case */
        if (c >= 'a' && c <= 'z') {
            return BENAR;
        }
    }
    
    return SALAH;
}

/*
 * fat32_generate_sfn
 * ------------------
 * Generate Short Filename dari Long Filename.
 *
 * Parameter:
 *   lfn   - Long filename
 *   sfn   - Buffer untuk Short Filename (11 bytes)
 *   num   - Nomor unik untuk konflik
 *
 * Return: Status operasi
 */
status_t fat32_generate_sfn(const char *lfn, tak_bertanda8_t sfn[11],
                             tak_bertanda32_t num)
{
    ukuran_t name_len;
    ukuran_t ext_len;
    const char *dot;
    const char *ext;
    ukuran_t i;
    char num_str[8];
    ukuran_t num_len;
    
    if (lfn == NULL || sfn == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Inisialisasi dengan spasi */
    kernel_memset(sfn, ' ', 11);
    
    name_len = kernel_strlen(lfn);
    if (name_len == 0) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Cari ekstensi */
    dot = kernel_strrchr(lfn, '.');
    if (dot != NULL && dot != lfn) {
        ext = dot + 1;
        ext_len = kernel_strlen(ext);
        if (ext_len > 3) {
            ext_len = 3;
        }
        
        /* Copy ekstensi */
        for (i = 0; i < ext_len; i++) {
            char c = ext[i];
            if (c >= 'a' && c <= 'z') {
                sfn[8 + i] = (tak_bertanda8_t)(c - 'a' + 'A');
            } else {
                sfn[8 + i] = (tak_bertanda8_t)c;
            }
        }
        
        name_len = (ukuran_t)(dot - lfn);
    }
    
    /* Generate num string jika perlu */
    if (num > 0) {
        kernel_snprintf(num_str, sizeof(num_str), "~%u", num);
        num_len = kernel_strlen(num_str);
    } else {
        num_len = 0;
    }
    
    /* Copy nama utama */
    {
        ukuran_t max_name;
        ukuran_t copy_len;
        
        max_name = 8 - num_len;
        copy_len = (name_len > max_name) ? max_name : name_len;
        
        for (i = 0; i < copy_len; i++) {
            char c = lfn[i];
            if (c >= 'a' && c <= 'z') {
                sfn[i] = (tak_bertanda8_t)(c - 'a' + 'A');
            } else if (c == ' ' || c == '.') {
                sfn[i] = '_';  /* Ganti karakter invalid */
            } else {
                sfn[i] = (tak_bertanda8_t)c;
            }
        }
        
        /* Tambahkan num string */
        if (num_len > 0) {
            for (i = 0; i < num_len; i++) {
                sfn[copy_len + i] = (tak_bertanda8_t)num_str[i];
            }
        }
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI DEBUG LFN (LFN DEBUG FUNCTIONS)
 * ===========================================================================
 */

/*
 * fat32_print_lfn_entry
 * ---------------------
 * Mencetak informasi LFN entry untuk debugging.
 *
 * Parameter:
 *   lfn - Pointer ke LFN entry
 */
void fat32_print_lfn_entry(const fat32_lfn_entry_t *lfn)
{
    char name[14];
    ukuran_t i;
    
    if (lfn == NULL) {
        kernel_printf("[FAT32] LFN entry is NULL\n");
        return;
    }
    
    /* Extract characters */
    for (i = 0; i < 5; i++) {
        name[i] = fat32_unicode_to_ascii(lfn->lfn_name1[i]);
    }
    for (i = 0; i < 6; i++) {
        name[5 + i] = fat32_unicode_to_ascii(lfn->lfn_name2[i]);
    }
    for (i = 0; i < 2; i++) {
        name[11 + i] = fat32_unicode_to_ascii(lfn->lfn_name3[i]);
    }
    name[13] = '\0';
    
    kernel_printf("[FAT32] LFN Entry:\n");
    kernel_printf("  Seq:     0x%02X (%u, last=%s)\n",
                  lfn->lfn_seq,
                  lfn->lfn_seq & FAT32_LFN_MASK,
                  (lfn->lfn_seq & FAT32_LFN_LAST_ENTRY) ? "yes" : "no");
    kernel_printf("  Attr:    0x%02X\n", lfn->lfn_attr);
    kernel_printf("  Checksum: 0x%02X\n", lfn->lfn_checksum);
    kernel_printf("  Name:    %.13s\n", name);
}
