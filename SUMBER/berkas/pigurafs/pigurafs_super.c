/*
 * PIGURA OS - PIGURAFS_SUPER.C
 * =============================
 * Implementasi operasi superblock untuk filesystem PiguraFS.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola superblock
 * PiguraFS termasuk inisialisasi, validasi, dan sinkronisasi.
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
#include "../../libc/include/stddef.h"

/*
 * ===========================================================================
 * KONSTANTA SUPERBLOCK (SUPERBLOCK CONSTANTS)
 * ===========================================================================
 */

/* Magic number */
#define PFS_SB_MAGIC            0x50465331

/* Versi filesystem */
#define PFS_VERSI_MAJOR         1
#define PFS_VERSI_MINOR         0

/* Ukuran superblock on-disk */
#define PFS_SB_SIZE             1024

/* Offset superblock */
#define PFS_SB_OFFSET           1024

/* Default values */
#define PFS_BLOCK_SIZE_DEFAULT  4096
#define PFS_INODES_PER_GROUP    8192
#define PFS_BLOCKS_PER_GROUP    32768

/* Flag superblock */
#define PFS_SB_FLAG_CLEAN       0x0001
#define PFS_SB_FLAG_DIRTY       0x0002
#define PFS_SB_FLAG_ERROR       0x0004
#define PFS_SB_FLAG_READONLY    0x0008

/* State filesystem */
#define PFS_STATE_CLEAN         0x0001
#define PFS_STATE_DIRTY         0x0002
#define PFS_STATE_ERROR         0x0004

/* Reserved inodes */
#define PFS_ROOT_INO            2
#define PFS_JOURNAL_INO         3
#define PFS_SNAPSHOT_INO        4

/*
 * ===========================================================================
 * STRUKTUR DATA SUPERBLOCK (SUPERBLOCK DATA STRUCTURES)
 * ===========================================================================
 */

/* Struktur superblock on-disk */
typedef struct PACKED {
    tak_bertanda32_t s_magic;           /* Magic number */
    tak_bertanda32_t s_versi_major;     /* Versi major */
    tak_bertanda32_t s_versi_minor;     /* Versi minor */
    tak_bertanda16_t s_block_size;      /* Ukuran block (log2) */
    tak_bertanda16_t s_block_size_bits; /* Bits untuk block size */
    tak_bertanda32_t s_blocks_count;    /* Total block */
    tak_bertanda32_t s_blocks_free;     /* Block bebas */
    tak_bertanda32_t s_inodes_count;    /* Total inode */
    tak_bertanda32_t s_inodes_free;     /* Inode bebas */
    tak_bertanda32_t s_first_block;     /* Block data pertama */
    tak_bertanda32_t s_first_ino;       /* Inode non-reserved pertama */
    tak_bertanda32_t s_inodes_per_group;/* Inode per group */
    tak_bertanda32_t s_blocks_per_group;/* Block per group */
    tak_bertanda32_t s_group_count;     /* Jumlah group */
    tak_bertanda32_t s_journal_ino;     /* Inode journal */
    tak_bertanda32_t s_journal_block;   /* Block awal journal */
    tak_bertanda32_t s_journal_size;    /* Ukuran journal (block) */
    tak_bertanda32_t s_flags;           /* Flag superblock */
    tak_bertanda32_t s_mtime;           /* Waktu mount terakhir */
    tak_bertanda32_t s_wtime;           /* Waktu write terakhir */
    tak_bertanda16_t s_mnt_count;       /* Jumlah mount */
    tak_bertanda16_t s_max_mnt_count;   /* Max mount sebelum check */
    tak_bertanda32_t s_state;           /* State filesystem */
    tak_bertanda32_t s_checksum;        /* Checksum superblock */
    tak_bertanda8_t s_nama[32];         /* Nama volume */
    tak_bertanda8_t s_uuid[16];         /* UUID */
    tak_bertanda8_t s_last_mount[64];   /* Path mount terakhir */
    tak_bertanda8_t s_reserved[836];    /* Reserved */
} pfs_superblock_t;

/* Struktur group descriptor */
typedef struct PACKED {
    tak_bertanda32_t bg_block_bitmap;   /* Block bitmap location */
    tak_bertanda32_t bg_inode_bitmap;   /* Inode bitmap location */
    tak_bertanda32_t bg_inode_table;    /* Inode table location */
    tak_bertanda16_t bg_free_blocks;    /* Free blocks count */
    tak_bertanda16_t bg_free_inodes;    /* Free inodes count */
    tak_bertanda16_t bg_used_dirs;      /* Used directories count */
    tak_bertanda16_t bg_flags;          /* Group flags */
    tak_bertanda32_t bg_checksum;       /* Group checksum */
    tak_bertanda8_t bg_reserved[20];    /* Reserved */
} pfs_group_desc_t;

/* Struktur in-memory superblock info */
typedef struct {
    tak_bertanda32_t pfs_magic;         /* Magic number validasi */
    superblock_t *vfs_sb;               /* VFS superblock */
    pfs_superblock_t *disk_sb;          /* On-disk superblock */
    pfs_group_desc_t *group_desc;       /* Group descriptors */
    tak_bertanda32_t block_size;        /* Ukuran block */
    tak_bertanda32_t block_size_bits;   /* Bits block size */
    tak_bertanda64_t blocks_count;      /* Total block */
    tak_bertanda64_t blocks_free;       /* Block bebas */
    tak_bertanda64_t inodes_count;      /* Total inode */
    tak_bertanda64_t inodes_free;       /* Inode bebas */
    tak_bertanda32_t first_block;       /* Block data pertama */
    tak_bertanda32_t first_ino;         /* Inode pertama */
    tak_bertanda32_t inodes_per_group;  /* Inode per group */
    tak_bertanda32_t blocks_per_group;  /* Block per group */
    tak_bertanda32_t group_count;       /* Jumlah group */
    tak_bertanda32_t journal_ino;       /* Inode journal */
    tak_bertanda32_t journal_block;     /* Block journal */
    tak_bertanda32_t journal_size;      /* Ukuran journal */
    tak_bertanda32_t flags;             /* Flag */
    char nama_volume[33];               /* Nama volume */
    tak_bertanda8_t uuid[16];           /* UUID */
    bool_t journal_aktif;               /* Journal aktif? */
    bool_t read_only;                   /* Read-only? */
    bool_t dirty;                       /* Perlu sync? */
    tak_bertanda32_t refcount;          /* Reference count */
    void *device;                       /* Device context */
} pfs_sb_info_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INTERNAL (INTERNAL FUNCTION DECLARATIONS)
 * ===========================================================================
 */

static status_t pfs_sb_validasi(const pfs_superblock_t *sb);
static tak_bertanda32_t pfs_sb_checksum(const pfs_superblock_t *sb);
static status_t pfs_sb_baca_group_desc(pfs_sb_info_t *sbi);
static status_t pfs_sb_tulis_group_desc(pfs_sb_info_t *sbi);
static void pfs_sb_update_wtime(pfs_superblock_t *sb);
static void pfs_sb_update_mtime(pfs_superblock_t *sb);

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI VALIDASI (VALIDATION FUNCTIONS)
 * ===========================================================================
 */

static status_t pfs_sb_validasi(const pfs_superblock_t *sb)
{
    tak_bertanda32_t checksum;

    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Validasi magic number */
    if (sb->s_magic != PFS_SB_MAGIC) {
        return STATUS_FORMAT_INVALID;
    }

    /* Validasi versi */
    if (sb->s_versi_major > PFS_VERSI_MAJOR) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Validasi block size */
    if (sb->s_block_size < 9 || sb->s_block_size > 16) {
        return STATUS_FORMAT_INVALID;
    }

    /* Validasi ukuran filesystem */
    if (sb->s_blocks_count == 0) {
        return STATUS_FORMAT_INVALID;
    }

    /* Validasi checksum */
    checksum = pfs_sb_checksum(sb);
    if (checksum != sb->s_checksum) {
        return STATUS_FORMAT_INVALID;
    }

    return STATUS_BERHASIL;
}

static tak_bertanda32_t pfs_sb_checksum(const pfs_superblock_t *sb)
{
    tak_bertanda32_t checksum;
    const tak_bertanda8_t *data;
    ukuran_t i;

    if (sb == NULL) {
        return 0;
    }

    /* Simple checksum: sum of all bytes */
    checksum = 0;
    data = (const tak_bertanda8_t *)sb;

    for (i = 0; i < PFS_SB_SIZE; i++) {
        if (i < offsetof(pfs_superblock_t, s_checksum) ||
            i >= offsetof(pfs_superblock_t, s_checksum) + 4) {
            checksum += data[i];
        }
    }

    return checksum;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI GROUP DESCRIPTOR (GROUP DESCRIPTOR FUNCTIONS)
 * ===========================================================================
 */

static status_t pfs_sb_baca_group_desc(pfs_sb_info_t *sbi)
{
    ukuran_t group_size;

    if (sbi == NULL || sbi->disk_sb == NULL) {
        return STATUS_PARAM_NULL;
    }

    group_size = (ukuran_t)sbi->group_count * sizeof(pfs_group_desc_t);

    sbi->group_desc = (pfs_group_desc_t *)kmalloc(group_size);
    if (sbi->group_desc == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    kernel_memset(sbi->group_desc, 0, group_size);

    /* TODO: Baca group descriptor dari disk */

    return STATUS_BERHASIL;
}

static status_t pfs_sb_tulis_group_desc(pfs_sb_info_t *sbi)
{
    if (sbi == NULL || sbi->group_desc == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* TODO: Tulis group descriptor ke disk */

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI TIME (TIME FUNCTIONS)
 * ===========================================================================
 */

static void pfs_sb_update_wtime(pfs_superblock_t *sb)
{
    if (sb == NULL) {
        return;
    }

    /* TODO: Dapatkan waktu aktual */
    sb->s_wtime = 0;
}

static void pfs_sb_update_mtime(pfs_superblock_t *sb)
{
    if (sb == NULL) {
        return;
    }

    /* TODO: Dapatkan waktu aktual */
    sb->s_mtime = 0;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI UTAMA (MAIN FUNCTIONS)
 * ===========================================================================
 */

/*
 * pfs_super_baca
 * --------------
 * Baca superblock dari device.
 *
 * Parameter:
 *   sbi    - Pointer ke superblock info
 *   device - Device context
 *
 * Return: Status operasi
 */
status_t pfs_super_baca(pfs_sb_info_t *sbi, void *device)
{
    pfs_superblock_t *disk_sb;
    status_t status;

    if (sbi == NULL || device == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Alokasi buffer superblock */
    disk_sb = (pfs_superblock_t *)kmalloc(sizeof(pfs_superblock_t));
    if (disk_sb == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    kernel_memset(disk_sb, 0, sizeof(pfs_superblock_t));

    /* TODO: Baca dari device */
    /* Untuk sekarang, init dengan default */
    disk_sb->s_magic = PFS_SB_MAGIC;
    disk_sb->s_versi_major = PFS_VERSI_MAJOR;
    disk_sb->s_versi_minor = PFS_VERSI_MINOR;
    disk_sb->s_block_size = 12;
    disk_sb->s_block_size_bits = 12;
    disk_sb->s_blocks_count = 1000000;
    disk_sb->s_blocks_free = 990000;
    disk_sb->s_inodes_count = 100000;
    disk_sb->s_inodes_free = 99000;
    disk_sb->s_first_block = 100;
    disk_sb->s_first_ino = PFS_ROOT_INO + 1;
    disk_sb->s_inodes_per_group = PFS_INODES_PER_GROUP;
    disk_sb->s_blocks_per_group = PFS_BLOCKS_PER_GROUP;
    disk_sb->s_group_count = 31;
    disk_sb->s_journal_ino = PFS_JOURNAL_INO;
    disk_sb->s_journal_block = 50;
    disk_sb->s_journal_size = 4096;
    disk_sb->s_flags = PFS_SB_FLAG_CLEAN;
    disk_sb->s_state = PFS_STATE_CLEAN;
    kernel_strncpy((char *)disk_sb->s_nama, "PiguraFS", 32);
    disk_sb->s_checksum = pfs_sb_checksum(disk_sb);

    /* Validasi superblock */
    status = pfs_sb_validasi(disk_sb);
    if (status != STATUS_BERHASIL) {
        kfree(disk_sb);
        return status;
    }

    /* Copy ke in-memory structure */
    sbi->disk_sb = disk_sb;
    sbi->block_size = (tak_bertanda32_t)(1 << disk_sb->s_block_size);
    sbi->block_size_bits = disk_sb->s_block_size_bits;
    sbi->blocks_count = disk_sb->s_blocks_count;
    sbi->blocks_free = disk_sb->s_blocks_free;
    sbi->inodes_count = disk_sb->s_inodes_count;
    sbi->inodes_free = disk_sb->s_inodes_free;
    sbi->first_block = disk_sb->s_first_block;
    sbi->first_ino = disk_sb->s_first_ino;
    sbi->inodes_per_group = disk_sb->s_inodes_per_group;
    sbi->blocks_per_group = disk_sb->s_blocks_per_group;
    sbi->group_count = disk_sb->s_group_count;
    sbi->journal_ino = disk_sb->s_journal_ino;
    sbi->journal_block = disk_sb->s_journal_block;
    sbi->journal_size = disk_sb->s_journal_size;
    sbi->flags = disk_sb->s_flags;
    kernel_strncpy(sbi->nama_volume, (const char *)disk_sb->s_nama, 32);
    sbi->nama_volume[32] = '\0';
    kernel_memcpy(sbi->uuid, disk_sb->s_uuid, 16);
    sbi->device = device;

    /* Baca group descriptors */
    status = pfs_sb_baca_group_desc(sbi);
    if (status != STATUS_BERHASIL) {
        kfree(disk_sb);
        sbi->disk_sb = NULL;
        return status;
    }

    return STATUS_BERHASIL;
}

/*
 * pfs_super_tulis
 * ---------------
 * Tulis superblock ke device.
 *
 * Parameter:
 *   sbi - Pointer ke superblock info
 *
 * Return: Status operasi
 */
status_t pfs_super_tulis(pfs_sb_info_t *sbi)
{
    pfs_superblock_t *disk_sb;

    if (sbi == NULL || sbi->disk_sb == NULL) {
        return STATUS_PARAM_NULL;
    }

    disk_sb = sbi->disk_sb;

    /* Update nilai dari in-memory */
    disk_sb->s_blocks_free = (tak_bertanda32_t)sbi->blocks_free;
    disk_sb->s_inodes_free = (tak_bertanda32_t)sbi->inodes_free;
    disk_sb->s_flags = sbi->flags;

    /* Update waktu */
    pfs_sb_update_wtime(disk_sb);

    /* Update checksum */
    disk_sb->s_checksum = pfs_sb_checksum(disk_sb);

    /* TODO: Tulis ke device */

    sbi->dirty = SALAH;

    return STATUS_BERHASIL;
}

/*
 * pfs_super_buat
 * --------------
 * Buat superblock baru.
 *
 * Parameter:
 *   sbi       - Pointer ke superblock info
 *   device    - Device context
 *   ukuran    - Ukuran filesystem dalam block
 *   nama      - Nama volume
 *
 * Return: Status operasi
 */
status_t pfs_super_buat(pfs_sb_info_t *sbi, void *device,
                        tak_bertanda64_t ukuran, const char *nama)
{
    pfs_superblock_t *disk_sb;
    tak_bertanda32_t groups;
    status_t status;

    if (sbi == NULL || device == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Alokasi buffer superblock */
    disk_sb = (pfs_superblock_t *)kmalloc(sizeof(pfs_superblock_t));
    if (disk_sb == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    kernel_memset(disk_sb, 0, sizeof(pfs_superblock_t));

    /* Hitung jumlah group */
    groups = (tak_bertanda32_t)((ukuran + PFS_BLOCKS_PER_GROUP - 1) /
                                PFS_BLOCKS_PER_GROUP);

    /* Inisialisasi superblock */
    disk_sb->s_magic = PFS_SB_MAGIC;
    disk_sb->s_versi_major = PFS_VERSI_MAJOR;
    disk_sb->s_versi_minor = PFS_VERSI_MINOR;
    disk_sb->s_block_size = 12;
    disk_sb->s_block_size_bits = 12;
    disk_sb->s_blocks_count = (tak_bertanda32_t)ukuran;
    disk_sb->s_blocks_free = (tak_bertanda32_t)(ukuran - 100);
    disk_sb->s_inodes_count = (tak_bertanda32_t)(ukuran / 10);
    disk_sb->s_inodes_free = (tak_bertanda32_t)(ukuran / 10 - 10);
    disk_sb->s_first_block = 100;
    disk_sb->s_first_ino = PFS_ROOT_INO + 1;
    disk_sb->s_inodes_per_group = PFS_INODES_PER_GROUP;
    disk_sb->s_blocks_per_group = PFS_BLOCKS_PER_GROUP;
    disk_sb->s_group_count = groups;
    disk_sb->s_journal_ino = PFS_JOURNAL_INO;
    disk_sb->s_journal_block = 50;
    disk_sb->s_journal_size = 4096;
    disk_sb->s_flags = PFS_SB_FLAG_CLEAN;
    disk_sb->s_state = PFS_STATE_CLEAN;
    disk_sb->s_mnt_count = 0;
    disk_sb->s_max_mnt_count = 20;

    if (nama != NULL) {
        kernel_strncpy((char *)disk_sb->s_nama, nama, 32);
    } else {
        kernel_strncpy((char *)disk_sb->s_nama, "PiguraFS", 32);
    }

    /* Generate UUID */
    {
        ukuran_t i;
        for (i = 0; i < 16; i++) {
            disk_sb->s_uuid[i] = (tak_bertanda8_t)(i ^ 0x5A);
        }
    }

    /* Calculate checksum */
    disk_sb->s_checksum = pfs_sb_checksum(disk_sb);

    /* Copy ke in-memory structure */
    sbi->disk_sb = disk_sb;
    sbi->block_size = PFS_BLOCK_SIZE_DEFAULT;
    sbi->block_size_bits = 12;
    sbi->blocks_count = disk_sb->s_blocks_count;
    sbi->blocks_free = disk_sb->s_blocks_free;
    sbi->inodes_count = disk_sb->s_inodes_count;
    sbi->inodes_free = disk_sb->s_inodes_free;
    sbi->first_block = disk_sb->s_first_block;
    sbi->first_ino = disk_sb->s_first_ino;
    sbi->inodes_per_group = disk_sb->s_inodes_per_group;
    sbi->blocks_per_group = disk_sb->s_blocks_per_group;
    sbi->group_count = disk_sb->s_group_count;
    sbi->journal_ino = disk_sb->s_journal_ino;
    sbi->journal_block = disk_sb->s_journal_block;
    sbi->journal_size = disk_sb->s_journal_size;
    sbi->flags = disk_sb->s_flags;
    kernel_strncpy(sbi->nama_volume, (const char *)disk_sb->s_nama, 32);
    sbi->nama_volume[32] = '\0';
    kernel_memcpy(sbi->uuid, disk_sb->s_uuid, 16);
    sbi->device = device;
    sbi->dirty = BENAR;

    /* Alokasi group descriptors */
    sbi->group_desc = (pfs_group_desc_t *)
                      kmalloc(groups * sizeof(pfs_group_desc_t));
    if (sbi->group_desc == NULL) {
        kfree(disk_sb);
        sbi->disk_sb = NULL;
        return STATUS_MEMORI_HABIS;
    }

    kernel_memset(sbi->group_desc, 0, groups * sizeof(pfs_group_desc_t));

    /* Init group descriptors */
    {
        ukuran_t i;
        for (i = 0; i < groups; i++) {
            pfs_group_desc_t *gd = &sbi->group_desc[i];
            gd->bg_free_blocks = PFS_BLOCKS_PER_GROUP;
            gd->bg_free_inodes = PFS_INODES_PER_GROUP;
            gd->bg_used_dirs = 0;
        }
    }

    /* Tulis ke disk */
    status = pfs_super_tulis(sbi);
    if (status != STATUS_BERHASIL) {
        kfree(sbi->group_desc);
        kfree(disk_sb);
        sbi->disk_sb = NULL;
        sbi->group_desc = NULL;
        return status;
    }

    return STATUS_BERHASIL;
}

/*
 * pfs_super_sync
 * --------------
 * Sinkronkan superblock ke disk.
 *
 * Parameter:
 *   sbi - Pointer ke superblock info
 *
 * Return: Status operasi
 */
status_t pfs_super_sync(pfs_sb_info_t *sbi)
{
    status_t status;

    if (sbi == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (!sbi->dirty) {
        return STATUS_BERHASIL;
    }

    /* Tulis group descriptors */
    status = pfs_sb_tulis_group_desc(sbi);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    /* Tulis superblock */
    return pfs_super_tulis(sbi);
}

/*
 * pfs_super_free
 * --------------
 * Bebaskan superblock info.
 *
 * Parameter:
 *   sbi - Pointer ke superblock info
 */
void pfs_super_free(pfs_sb_info_t *sbi)
{
    if (sbi == NULL) {
        return;
    }

    /* Sync jika dirty */
    if (sbi->dirty) {
        pfs_super_sync(sbi);
    }

    /* Free group descriptors */
    if (sbi->group_desc != NULL) {
        kfree(sbi->group_desc);
        sbi->group_desc = NULL;
    }

    /* Free disk superblock */
    if (sbi->disk_sb != NULL) {
        kfree(sbi->disk_sb);
        sbi->disk_sb = NULL;
    }

    sbi->pfs_magic = 0;
}

/*
 * pfs_super_inc_mnt_count
 * -----------------------
 * Increment mount count.
 *
 * Parameter:
 *   sbi - Pointer ke superblock info
 */
void pfs_super_inc_mnt_count(pfs_sb_info_t *sbi)
{
    if (sbi == NULL || sbi->disk_sb == NULL) {
        return;
    }

    sbi->disk_sb->s_mnt_count++;
    pfs_sb_update_mtime(sbi->disk_sb);
    sbi->dirty = BENAR;
}

/*
 * pfs_super_check_mnt_count
 * -------------------------
 * Cek apakah perlu fsck.
 *
 * Parameter:
 *   sbi - Pointer ke superblock info
 *
 * Return: BENAR jika perlu fsck
 */
bool_t pfs_super_check_mnt_count(pfs_sb_info_t *sbi)
{
    if (sbi == NULL || sbi->disk_sb == NULL) {
        return SALAH;
    }

    return (sbi->disk_sb->s_mnt_count >=
            sbi->disk_sb->s_max_mnt_count) ? BENAR : SALAH;
}

/*
 * pfs_super_mark_dirty
 * --------------------
 * Tandai superblock sebagai dirty.
 *
 * Parameter:
 *   sbi - Pointer ke superblock info
 */
void pfs_super_mark_dirty(pfs_sb_info_t *sbi)
{
    if (sbi == NULL) {
        return;
    }

    sbi->dirty = BENAR;
    if (sbi->disk_sb != NULL) {
        sbi->disk_sb->s_state = PFS_STATE_DIRTY;
        sbi->disk_sb->s_flags = PFS_SB_FLAG_DIRTY;
    }
}

/*
 * pfs_super_mark_clean
 * --------------------
 * Tandai superblock sebagai clean.
 *
 * Parameter:
 *   sbi - Pointer ke superblock info
 */
void pfs_super_mark_clean(pfs_sb_info_t *sbi)
{
    if (sbi == NULL) {
        return;
    }

    sbi->dirty = SALAH;
    if (sbi->disk_sb != NULL) {
        sbi->disk_sb->s_state = PFS_STATE_CLEAN;
        sbi->disk_sb->s_flags = PFS_SB_FLAG_CLEAN;
    }
}

/*
 * pfs_super_set_error
 * -------------------
 * Tandai superblock dengan error.
 *
 * Parameter:
 *   sbi - Pointer ke superblock info
 */
void pfs_super_set_error(pfs_sb_info_t *sbi)
{
    if (sbi == NULL) {
        return;
    }

    if (sbi->disk_sb != NULL) {
        sbi->disk_sb->s_state |= PFS_STATE_ERROR;
        sbi->disk_sb->s_flags |= PFS_SB_FLAG_ERROR;
    }
    sbi->dirty = BENAR;
}

/*
 * pfs_super_has_error
 * -------------------
 * Cek apakah superblock memiliki error.
 *
 * Parameter:
 *   sbi - Pointer ke superblock info
 *
 * Return: BENAR jika ada error
 */
bool_t pfs_super_has_error(pfs_sb_info_t *sbi)
{
    if (sbi == NULL || sbi->disk_sb == NULL) {
        return BENAR;
    }

    return (sbi->disk_sb->s_state & PFS_STATE_ERROR) ? BENAR : SALAH;
}

/*
 * pfs_super_alloc_block
 * ---------------------
 * Alokasi block dari filesystem.
 *
 * Parameter:
 *   sbi - Pointer ke superblock info
 *
 * Return: Block number, atau 0 jika gagal
 */
tak_bertanda32_t pfs_super_alloc_block(pfs_sb_info_t *sbi)
{
    tak_bertanda32_t group;
    tak_bertanda32_t block;

    if (sbi == NULL || sbi->group_desc == NULL) {
        return 0;
    }

    if (sbi->blocks_free == 0) {
        return 0;
    }

    /* Cari group dengan free blocks */
    for (group = 0; group < sbi->group_count; group++) {
        if (sbi->group_desc[group].bg_free_blocks > 0) {
            break;
        }
    }

    if (group >= sbi->group_count) {
        return 0;
    }

    /* Alokasi block dari group */
    block = sbi->first_block + group * sbi->blocks_per_group +
            (sbi->blocks_per_group - sbi->group_desc[group].bg_free_blocks);

    sbi->group_desc[group].bg_free_blocks--;
    sbi->blocks_free--;
    sbi->dirty = BENAR;

    return block;
}

/*
 * pfs_super_free_block
 * --------------------
 * Bebaskan block ke filesystem.
 *
 * Parameter:
 *   sbi    - Pointer ke superblock info
 *   block  - Block number
 */
void pfs_super_free_block(pfs_sb_info_t *sbi, tak_bertanda32_t block)
{
    tak_bertanda32_t group;

    if (sbi == NULL || sbi->group_desc == NULL) {
        return;
    }

    if (block < sbi->first_block) {
        return;
    }

    /* Hitung group */
    group = (block - sbi->first_block) / sbi->blocks_per_group;
    if (group >= sbi->group_count) {
        return;
    }

    sbi->group_desc[group].bg_free_blocks++;
    sbi->blocks_free++;
    sbi->dirty = BENAR;
}

/*
 * pfs_super_alloc_inode
 * ---------------------
 * Alokasi inode dari filesystem.
 *
 * Parameter:
 *   sbi - Pointer ke superblock info
 *
 * Return: Inode number, atau 0 jika gagal
 */
tak_bertanda32_t pfs_super_alloc_inode(pfs_sb_info_t *sbi)
{
    tak_bertanda32_t group;
    tak_bertanda32_t ino;

    if (sbi == NULL || sbi->group_desc == NULL) {
        return 0;
    }

    if (sbi->inodes_free == 0) {
        return 0;
    }

    /* Cari group dengan free inodes */
    for (group = 0; group < sbi->group_count; group++) {
        if (sbi->group_desc[group].bg_free_inodes > 0) {
            break;
        }
    }

    if (group >= sbi->group_count) {
        return 0;
    }

    /* Alokasi inode dari group */
    ino = sbi->first_ino + group * sbi->inodes_per_group +
          (sbi->inodes_per_group - sbi->group_desc[group].bg_free_inodes);

    sbi->group_desc[group].bg_free_inodes--;
    sbi->inodes_free--;
    sbi->dirty = BENAR;

    return ino;
}

/*
 * pfs_super_free_inode
 * --------------------
 * Bebaskan inode ke filesystem.
 *
 * Parameter:
 *   sbi - Pointer ke superblock info
 *   ino - Inode number
 */
void pfs_super_free_inode(pfs_sb_info_t *sbi, tak_bertanda32_t ino)
{
    tak_bertanda32_t group;

    if (sbi == NULL || sbi->group_desc == NULL) {
        return;
    }

    if (ino < sbi->first_ino) {
        return;
    }

    /* Hitung group */
    group = (ino - sbi->first_ino) / sbi->inodes_per_group;
    if (group >= sbi->group_count) {
        return;
    }

    sbi->group_desc[group].bg_free_inodes++;
    sbi->inodes_free++;
    sbi->dirty = BENAR;
}

/*
 * pfs_super_statfs
 * ----------------
 * Dapatkan statistik filesystem.
 *
 * Parameter:
 *   sbi  - Pointer ke superblock info
 *   stat - Pointer ke struktur stat
 *
 * Return: Status operasi
 */
status_t pfs_super_statfs(pfs_sb_info_t *sbi, vfs_stat_t *stat)
{
    if (sbi == NULL || stat == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memset(stat, 0, sizeof(vfs_stat_t));

    stat->st_blksize = (tak_bertanda32_t)sbi->block_size;
    stat->st_blocks = sbi->blocks_count;
    stat->st_size = stat->st_blocks * (off_t)stat->st_blksize;

    return STATUS_BERHASIL;
}
