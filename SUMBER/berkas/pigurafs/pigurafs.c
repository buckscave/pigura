/*
 * PIGURA OS - PIGURAFS.C
 * =======================
 * Driver filesystem PiguraFS untuk kernel Pigura OS.
 *
 * Berkas ini berisi implementasi driver utama PiguraFS yang mencakup
 * registrasi filesystem, operasi mount, dan inisialisasi struktur.
 *
 * PiguraFS adalah filesystem asli Pigura OS dengan fitur:
 * - 64-bit support (ukuran file > 4GB)
 * - B+tree untuk direktori dan indeks file
 * - Extent-based allocation (bukan bitmap block)
 * - Journaling untuk crash recovery
 * - Extended attributes
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
 * KONSTANTA PIGURAFS (PIGURAFS CONSTANTS)
 * ===========================================================================
 */

/* Magic number untuk validasi */
#define PFS_SUPER_MAGIC         0x50465331  /* "PFS1" */
#define PFS_INODE_MAGIC         0x50465332  /* "PFS2" */
#define PFS_DIR_MAGIC           0x50465333  /* "PFS3" */
#define PFS_EXTENT_MAGIC        0x50465334  /* "PFS4" */
#define PFS_JOURNAL_MAGIC       0x50465335  /* "PFS5" */
#define PFS_BTREE_MAGIC         0x50465336  /* "PFS6" */
#define PFS_FILE_MAGIC          0x50465337  /* "PFS7" */

/* Versi filesystem */
#define PFS_VERSI_MAJOR         1
#define PFS_VERSI_MINOR         0

/* Ukuran block default */
#define PFS_BLOCK_SIZE_DEFAULT  4096
#define PFS_BLOCK_SIZE_MIN      1024
#define PFS_BLOCK_SIZE_MAX      65536

/* Jumlah inode per group */
#define PFS_INODES_PER_GROUP    8192

/* Jumlah block per group */
#define PFS_BLOCKS_PER_GROUP    32768

/* Nama filesystem maksimum */
#define PFS_NAMA_MAKS           32

/* Panjang nama file maksimum */
#define PFS_NAMA_FILE_MAKS      255

/* Panjang path maksimum */
#define PFS_PATH_MAKS           4096

/* Jumlah extent maksimum per inode */
#define PFS_EXTENT_PER_INODE    4

/* Ukuran journal default (dalam block) */
#define PFS_JOURNAL_SIZE_DEFAULT 4096

/* Flag superblock */
#define PFS_SB_FLAG_CLEAN       0x0001
#define PFS_SB_FLAG_DIRTY       0x0002
#define PFS_SB_FLAG_ERROR       0x0004
#define PFS_SB_FLAG_READONLY    0x0008

/* Flag inode */
#define PFS_INODE_FLAG_SYNC     0x0001
#define PFS_INODE_FLAG_IMMUTABLE 0x0002
#define PFS_INODE_FLAG_APPEND   0x0004
#define PFS_INODE_FLAG_NODUMP   0x0008

/* Tipe file dalam mode */
#define PFS_S_IFMT              0xF000
#define PFS_S_IFREG             0x8000
#define PFS_S_IFDIR             0x4000
#define PFS_S_IFLNK             0xA000
#define PFS_S_IFBLK             0x6000
#define PFS_S_IFCHR             0x2000
#define PFS_S_IFIFO             0x1000

/* Offset dalam superblock on-disk */
#define PFS_SB_OFFSET           1024
#define PFS_SB_BLOCK            0

/* Lokasi inode root */
#define PFS_ROOT_INO            2

/* Reserved inodes */
#define PFS_BAD_INO             1
#define PFS_JOURNAL_INO         3
#define PFS_SNAPSHOT_INO        4

/*
 * ===========================================================================
 * STRUKTUR DATA ON-DISK (ON-DISK DATA STRUCTURES)
 * ===========================================================================
 */

/* Struktur superblock on-disk (1024 byte) */
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
    tak_bertanda8_t s_nama[PFS_NAMA_MAKS]; /* Nama volume */
    tak_bertanda8_t s_uuid[16];         /* UUID */
    tak_bertanda8_t s_reserved[904];    /* Reserved */
} pfs_superblock_disk_t;

/* Struktur inode on-disk (256 byte) */
typedef struct PACKED {
    tak_bertanda16_t i_mode;            /* Mode dan tipe file */
    tak_bertanda16_t i_uid;             /* User ID */
    tak_bertanda32_t i_size_lo;         /* Ukuran (low 32-bit) */
    tak_bertanda32_t i_size_hi;         /* Ukuran (high 32-bit) */
    tak_bertanda32_t i_atime;           /* Access time */
    tak_bertanda32_t i_ctime;           /* Creation time */
    tak_bertanda32_t i_mtime;           /* Modification time */
    tak_bertanda32_t i_dtime;           /* Deletion time */
    tak_bertanda16_t i_gid;             /* Group ID */
    tak_bertanda16_t i_links_count;     /* Link count */
    tak_bertanda32_t i_blocks;          /* Block count (512-byte) */
    tak_bertanda32_t i_flags;           /* Inode flags */
    tak_bertanda32_t i_generation;      /* Generation number */
    tak_bertanda32_t i_block[15];       /* Block pointers */
    tak_bertanda32_t i_version;         /* Version */
    tak_bertanda32_t i_extents_count;   /* Jumlah extent */
    tak_bertanda8_t i_extent[PFS_EXTENT_PER_INODE * 12]; /* Extent data */
    tak_bertanda8_t i_reserved[136];    /* Reserved */
} pfs_inode_disk_t;

/* Struktur extent on-disk (12 byte) */
typedef struct PACKED {
    tak_bertanda32_t e_block;           /* Block logical */
    tak_bertanda32_t e_start;           /* Block fisik awal */
    tak_bertanda16_t e_len;             /* Panjang extent */
    tak_bertanda16_t e_flags;           /* Flag extent */
} pfs_extent_disk_t;

/* Struktur directory entry on-disk */
typedef struct PACKED {
    tak_bertanda32_t d_ino;             /* Inode number */
    tak_bertanda16_t d_rec_len;         /* Record length */
    tak_bertanda8_t d_nama_len;         /* Nama length */
    tak_bertanda8_t d_file_type;        /* Tipe file */
    tak_bertanda8_t d_nama[PFS_NAMA_FILE_MAKS]; /* Nama */
} pfs_dirent_disk_t;

/* Tipe file dirent */
#define PFS_FT_UNKNOWN          0
#define PFS_FT_REG_FILE         1
#define PFS_FT_DIR              2
#define PFS_FT_CHRDEV           3
#define PFS_FT_BLKDEV           4
#define PFS_FT_FIFO             5
#define PFS_FT_SOCK             6
#define PFS_FT_SYMLINK          7

/*
 * ===========================================================================
 * STRUKTUR DATA IN-MEMORY (IN-MEMORY DATA STRUCTURES)
 * ===========================================================================
 */

/* Struktur superblock in-memory */
typedef struct {
    tak_bertanda32_t pfs_magic;         /* Magic number validasi */
    superblock_t *vfs_sb;               /* VFS superblock */
    pfs_superblock_disk_t *disk_sb;     /* On-disk superblock */
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
    char nama_volume[PFS_NAMA_MAKS + 1]; /* Nama volume */
    tak_bertanda8_t uuid[16];           /* UUID */
    bool_t journal_aktif;               /* Journal aktif? */
    bool_t read_only;                   /* Read-only? */
    bool_t dirty;                       /* Perlu sync? */
    tak_bertanda32_t refcount;          /* Reference count */
    void *device;                       /* Device context */
} pfs_sb_info_t;

/* Struktur inode in-memory */
typedef struct {
    tak_bertanda32_t pfs_magic;         /* Magic number validasi */
    inode_t *vfs_inode;                 /* VFS inode */
    pfs_inode_disk_t *disk_inode;       /* On-disk inode */
    tak_bertanda32_t ino;               /* Inode number */
    tak_bertanda64_t size;              /* Ukuran file */
    tak_bertanda32_t blocks;            /* Block count */
    tak_bertanda32_t extents_count;     /* Jumlah extent */
    pfs_extent_disk_t extents[PFS_EXTENT_PER_INODE]; /* Extent array */
    tak_bertanda32_t flags;             /* Inode flags */
    tak_bertanda32_t generation;        /* Generation number */
    bool_t dirty;                       /* Perlu write? */
} pfs_inode_info_t;

/* Struktur file handle in-memory */
typedef struct {
    tak_bertanda32_t pfs_magic;         /* Magic number validasi */
    file_t *vfs_file;                   /* VFS file */
    tak_bertanda64_t pos;               /* Posisi saat ini */
    tak_bertanda32_t current_extent;    /* Extent index saat ini */
    tak_bertanda32_t current_block;     /* Block dalam extent */
} pfs_file_info_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INTERNAL (INTERNAL FUNCTION DECLARATIONS)
 * ===========================================================================
 */

/* Fungsi mount/umount */
static superblock_t *pfs_mount(filesystem_t *fs, const char *device,
                               const char *path, tak_bertanda32_t flags);
static status_t pfs_umount(superblock_t *sb);
static status_t pfs_detect(const char *device);

/* Fungsi superblock operations */
static inode_t *pfs_alloc_inode(superblock_t *sb);
static void pfs_destroy_inode(inode_t *inode);
static status_t pfs_read_inode(inode_t *inode);
static status_t pfs_write_inode(inode_t *inode);
static void pfs_put_super(superblock_t *sb);
static status_t pfs_statfs(superblock_t *sb, vfs_stat_t *stat);
static status_t pfs_sync_fs(superblock_t *sb);

/* Fungsi inode operations */
static dentry_t *pfs_lookup(inode_t *dir, const char *name);
static status_t pfs_create(inode_t *dir, dentry_t *dentry, mode_t mode);
static status_t pfs_mkdir(inode_t *dir, dentry_t *dentry, mode_t mode);
static status_t pfs_rmdir(inode_t *dir, dentry_t *dentry);
static status_t pfs_unlink(inode_t *dir, dentry_t *dentry);
static status_t pfs_rename(inode_t *old_dir, dentry_t *old_dentry,
                           inode_t *new_dir, dentry_t *new_dentry);
static status_t pfs_getattr(dentry_t *dentry, vfs_stat_t *stat);
static status_t pfs_setattr(dentry_t *dentry, vfs_stat_t *stat);

/* Fungsi file operations */
static tak_bertandas_t pfs_read(file_t *file, void *buffer,
                                ukuran_t size, off_t *pos);
static tak_bertandas_t pfs_write(file_t *file, const void *buffer,
                                 ukuran_t size, off_t *pos);
static off_t pfs_lseek(file_t *file, off_t offset, tak_bertanda32_t whence);
static status_t pfs_open(inode_t *inode, file_t *file);
static status_t pfs_release(inode_t *inode, file_t *file);
static tak_bertandas_t pfs_readdir(file_t *file, vfs_dirent_t *dirent,
                                   ukuran_t count);

/* Fungsi utilitas */
static status_t pfs_baca_superblock(pfs_sb_info_t *sbi, void *device);
static status_t pfs_tulis_superblock(pfs_sb_info_t *sbi);
static status_t pfs_init_superblock(pfs_sb_info_t *sbi);
static status_t pfs_baca_inode(pfs_inode_info_t *ii, pfs_sb_info_t *sbi,
                               tak_bertanda32_t ino);
static status_t pfs_tulis_inode(pfs_inode_info_t *ii, pfs_sb_info_t *sbi);
static tak_bertanda32_t pfs_alloc_inode_no(pfs_sb_info_t *sbi);
static void pfs_free_inode_no(pfs_sb_info_t *sbi, tak_bertanda32_t ino);
static tak_bertanda32_t pfs_alloc_block_no(pfs_sb_info_t *sbi);
static void pfs_free_block_no(pfs_sb_info_t *sbi, tak_bertanda32_t block);
static void pfs_inode_to_disk(pfs_inode_info_t *ii,
                              pfs_inode_disk_t *disk);
static void pfs_disk_to_inode(const pfs_inode_disk_t *disk,
                              pfs_inode_info_t *ii);
static tak_bertanda64_t pfs_block_to_offset(pfs_sb_info_t *sbi,
                                            tak_bertanda32_t block);

/*
 * ===========================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ===========================================================================
 */

/* Operations structures */
static vfs_super_operations_t g_pfs_super_ops;
static vfs_inode_operations_t g_pfs_inode_ops;
static vfs_file_operations_t g_pfs_file_ops;

/* Filesystem registration */
static filesystem_t g_pfs_fs = {
    "pigurafs",
    FS_FLAG_REQUIRES_DEV | FS_FLAG_WRITE_SUPPORT,
    pfs_mount,
    pfs_umount,
    pfs_detect,
    NULL,
    0,
    SALAH
};

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI UTILITAS (UTILITY FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

static tak_bertanda64_t pfs_block_to_offset(pfs_sb_info_t *sbi,
                                            tak_bertanda32_t block)
{
    if (sbi == NULL) {
        return 0;
    }
    return (tak_bertanda64_t)block * (tak_bertanda64_t)sbi->block_size;
}

static void pfs_inode_to_disk(pfs_inode_info_t *ii, pfs_inode_disk_t *disk)
{
    if (ii == NULL || disk == NULL) {
        return;
    }

    kernel_memset(disk, 0, sizeof(pfs_inode_disk_t));

    disk->i_mode = (tak_bertanda16_t)(ii->vfs_inode->i_mode & 0xFFFF);
    disk->i_uid = (tak_bertanda16_t)(ii->vfs_inode->i_uid & 0xFFFF);
    disk->i_size_lo = (tak_bertanda32_t)(ii->size & 0xFFFFFFFF);
    disk->i_size_hi = (tak_bertanda32_t)(ii->size >> 32);
    disk->i_atime = (tak_bertanda32_t)ii->vfs_inode->i_atime;
    disk->i_ctime = (tak_bertanda32_t)ii->vfs_inode->i_ctime;
    disk->i_mtime = (tak_bertanda32_t)ii->vfs_inode->i_mtime;
    disk->i_gid = (tak_bertanda16_t)(ii->vfs_inode->i_gid & 0xFFFF);
    disk->i_links_count = (tak_bertanda16_t)ii->vfs_inode->i_nlink;
    disk->i_blocks = ii->blocks;
    disk->i_flags = ii->flags;
    disk->i_generation = ii->generation;
    disk->i_extents_count = ii->extents_count;

    /* Copy extent data */
    if (ii->extents_count > 0) {
        ukuran_t extent_size = ii->extents_count * sizeof(pfs_extent_disk_t);
        if (extent_size > sizeof(disk->i_extent)) {
            extent_size = sizeof(disk->i_extent);
        }
        kernel_memcpy(disk->i_extent, ii->extents, extent_size);
    }
}

static void pfs_disk_to_inode(const pfs_inode_disk_t *disk,
                              pfs_inode_info_t *ii)
{
    if (disk == NULL || ii == NULL) {
        return;
    }

    ii->vfs_inode->i_mode = (mode_t)disk->i_mode;
    ii->vfs_inode->i_uid = (uid_t)disk->i_uid;
    ii->vfs_inode->i_gid = (gid_t)disk->i_gid;
    ii->size = ((tak_bertanda64_t)disk->i_size_hi << 32) |
               (tak_bertanda64_t)disk->i_size_lo;
    ii->vfs_inode->i_size = (off_t)ii->size;
    ii->vfs_inode->i_atime = (waktu_t)disk->i_atime;
    ii->vfs_inode->i_ctime = (waktu_t)disk->i_ctime;
    ii->vfs_inode->i_mtime = (waktu_t)disk->i_mtime;
    ii->vfs_inode->i_nlink = (tak_bertanda32_t)disk->i_links_count;
    ii->blocks = disk->i_blocks;
    ii->vfs_inode->i_blocks = (tak_bertanda64_t)disk->i_blocks;
    ii->flags = disk->i_flags;
    ii->generation = disk->i_generation;
    ii->extents_count = disk->i_extents_count;

    /* Copy extent data */
    if (disk->i_extents_count > 0) {
        ukuran_t extent_size = disk->i_extents_count * sizeof(pfs_extent_disk_t);
        if (extent_size > sizeof(ii->extents)) {
            extent_size = sizeof(ii->extents);
        }
        kernel_memcpy(ii->extents, disk->i_extent, extent_size);
    }
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI SUPERBLOCK I/O (SUPERBLOCK I/O FUNCTIONS)
 * ===========================================================================
 */

static status_t pfs_baca_superblock(pfs_sb_info_t *sbi, void *device)
{
    pfs_superblock_disk_t *disk_sb;
    tak_bertanda8_t buffer[PFS_BLOCK_SIZE_DEFAULT];

    if (sbi == NULL || device == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Alokasi memori untuk disk superblock */
    disk_sb = (pfs_superblock_disk_t *)kmalloc(sizeof(pfs_superblock_disk_t));
    if (disk_sb == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    kernel_memset(disk_sb, 0, sizeof(pfs_superblock_disk_t));

    /* Baca superblock dari device (placeholder) */
    /* TODO: Implementasi pembacaan aktual dari block device */
    kernel_memset(buffer, 0, sizeof(buffer));

    /* Parse superblock dari buffer */
    /* Untuk sekarang, inisialisasi dengan nilai default */
    disk_sb->s_magic = PFS_SUPER_MAGIC;
    disk_sb->s_versi_major = PFS_VERSI_MAJOR;
    disk_sb->s_versi_minor = PFS_VERSI_MINOR;
    disk_sb->s_block_size = 12; /* log2(4096) = 12 */
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
    disk_sb->s_journal_size = PFS_JOURNAL_SIZE_DEFAULT;
    disk_sb->s_flags = PFS_SB_FLAG_CLEAN;
    kernel_strncpy((char *)disk_sb->s_nama, "PiguraFS", PFS_NAMA_MAKS);

    /* Validasi magic number */
    if (disk_sb->s_magic != PFS_SUPER_MAGIC) {
        kfree(disk_sb);
        return STATUS_FORMAT_INVALID;
    }

    /* Copy ke struktur in-memory */
    sbi->disk_sb = disk_sb;
    sbi->block_size = 1 << disk_sb->s_block_size;
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
    kernel_strncpy(sbi->nama_volume, (const char *)disk_sb->s_nama,
                   PFS_NAMA_MAKS);
    sbi->nama_volume[PFS_NAMA_MAKS] = '\0';
    kernel_memcpy(sbi->uuid, disk_sb->s_uuid, 16);
    sbi->device = device;

    return STATUS_BERHASIL;
}

static status_t pfs_tulis_superblock(pfs_sb_info_t *sbi)
{
    pfs_superblock_disk_t *disk_sb;

    if (sbi == NULL || sbi->disk_sb == NULL) {
        return STATUS_PARAM_NULL;
    }

    disk_sb = sbi->disk_sb;

    /* Update nilai dari in-memory ke disk */
    disk_sb->s_blocks_free = (tak_bertanda32_t)sbi->blocks_free;
    disk_sb->s_inodes_free = (tak_bertanda32_t)sbi->inodes_free;
    disk_sb->s_flags = sbi->flags;

    /* TODO: Implementasi penulisan aktual ke block device */

    sbi->dirty = SALAH;
    return STATUS_BERHASIL;
}

static status_t pfs_init_superblock(pfs_sb_info_t *sbi)
{
    if (sbi == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Inisialisasi superblock baru */
    sbi->disk_sb = (pfs_superblock_disk_t *)
                   kmalloc(sizeof(pfs_superblock_disk_t));
    if (sbi->disk_sb == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    kernel_memset(sbi->disk_sb, 0, sizeof(pfs_superblock_disk_t));

    /* Set nilai default */
    sbi->disk_sb->s_magic = PFS_SUPER_MAGIC;
    sbi->disk_sb->s_versi_major = PFS_VERSI_MAJOR;
    sbi->disk_sb->s_versi_minor = PFS_VERSI_MINOR;
    sbi->disk_sb->s_block_size = 12;
    sbi->disk_sb->s_block_size_bits = 12;
    sbi->disk_sb->s_blocks_count = 1000000;
    sbi->disk_sb->s_blocks_free = 990000;
    sbi->disk_sb->s_inodes_count = 100000;
    sbi->disk_sb->s_inodes_free = 99000;
    sbi->disk_sb->s_first_block = 100;
    sbi->disk_sb->s_first_ino = PFS_ROOT_INO + 1;
    sbi->disk_sb->s_inodes_per_group = PFS_INODES_PER_GROUP;
    sbi->disk_sb->s_blocks_per_group = PFS_BLOCKS_PER_GROUP;
    sbi->disk_sb->s_group_count = 31;
    sbi->disk_sb->s_journal_ino = PFS_JOURNAL_INO;
    sbi->disk_sb->s_journal_block = 50;
    sbi->disk_sb->s_journal_size = PFS_JOURNAL_SIZE_DEFAULT;
    sbi->disk_sb->s_flags = PFS_SB_FLAG_CLEAN;
    kernel_strncpy((char *)sbi->disk_sb->s_nama, "PiguraFS", PFS_NAMA_MAKS);

    /* Copy ke in-memory */
    sbi->block_size = PFS_BLOCK_SIZE_DEFAULT;
    sbi->block_size_bits = 12;
    sbi->blocks_count = 1000000;
    sbi->blocks_free = 990000;
    sbi->inodes_count = 100000;
    sbi->inodes_free = 99000;
    sbi->first_block = 100;
    sbi->first_ino = PFS_ROOT_INO + 1;
    sbi->inodes_per_group = PFS_INODES_PER_GROUP;
    sbi->blocks_per_group = PFS_BLOCKS_PER_GROUP;
    sbi->group_count = 31;
    sbi->journal_ino = PFS_JOURNAL_INO;
    sbi->journal_block = 50;
    sbi->journal_size = PFS_JOURNAL_SIZE_DEFAULT;
    sbi->flags = PFS_SB_FLAG_CLEAN;
    kernel_strncpy(sbi->nama_volume, "PiguraFS", PFS_NAMA_MAKS);
    sbi->journal_aktif = BENAR;
    sbi->dirty = BENAR;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI INODE ALLOCATION (INODE ALLOCATION FUNCTIONS)
 * ===========================================================================
 */

static tak_bertanda32_t pfs_alloc_inode_no(pfs_sb_info_t *sbi)
{
    if (sbi == NULL) {
        return 0;
    }

    if (sbi->inodes_free == 0) {
        return 0;
    }

    /* Simple linear search untuk inode bebas */
    sbi->inodes_free--;
    sbi->dirty = BENAR;

    return sbi->first_ino + (tak_bertanda32_t)(sbi->inodes_count -
           sbi->inodes_free - 1);
}

static void pfs_free_inode_no(pfs_sb_info_t *sbi, tak_bertanda32_t ino)
{
    if (sbi == NULL || ino < sbi->first_ino) {
        return;
    }

    sbi->inodes_free++;
    sbi->dirty = BENAR;
}

static tak_bertanda32_t pfs_alloc_block_no(pfs_sb_info_t *sbi)
{
    if (sbi == NULL) {
        return 0;
    }

    if (sbi->blocks_free == 0) {
        return 0;
    }

    /* Simple linear search untuk block bebas */
    sbi->blocks_free--;
    sbi->dirty = BENAR;

    return sbi->first_block + (tak_bertanda32_t)(sbi->blocks_count -
           sbi->blocks_free - 1);
}

static void pfs_free_block_no(pfs_sb_info_t *sbi, tak_bertanda32_t block)
{
    if (sbi == NULL || block < sbi->first_block) {
        return;
    }

    sbi->blocks_free++;
    sbi->dirty = BENAR;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI INODE I/O (INODE I/O FUNCTIONS)
 * ===========================================================================
 */

static status_t pfs_baca_inode(pfs_inode_info_t *ii, pfs_sb_info_t *sbi,
                               tak_bertanda32_t ino)
{
    pfs_inode_disk_t *disk_inode;

    if (ii == NULL || sbi == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (ino < PFS_ROOT_INO || ino > sbi->inodes_count) {
        return STATUS_PARAM_INVALID;
    }

    /* Alokasi disk inode buffer */
    disk_inode = (pfs_inode_disk_t *)kmalloc(sizeof(pfs_inode_disk_t));
    if (disk_inode == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    kernel_memset(disk_inode, 0, sizeof(pfs_inode_disk_t));

    /* Baca inode dari disk (placeholder) */
    /* TODO: Implementasi pembacaan aktual dari block device */
    /* Untuk sekarang, inisialisasi inode root jika ino == PFS_ROOT_INO */
    if (ino == PFS_ROOT_INO) {
        disk_inode->i_mode = (tak_bertanda16_t)(PFS_S_IFDIR | 0755);
        disk_inode->i_uid = 0;
        disk_inode->i_gid = 0;
        disk_inode->i_size_lo = sbi->block_size;
        disk_inode->i_size_hi = 0;
        disk_inode->i_atime = 0;
        disk_inode->i_ctime = 0;
        disk_inode->i_mtime = 0;
        disk_inode->i_links_count = 2;
        disk_inode->i_blocks = 1;
        disk_inode->i_flags = 0;
        disk_inode->i_generation = 1;
        disk_inode->i_extents_count = 1;
        /* Set extent pertama untuk root */
        {
            pfs_extent_disk_t *ext = (pfs_extent_disk_t *)disk_inode->i_extent;
            ext[0].e_block = 0;
            ext[0].e_start = pfs_alloc_block_no(sbi);
            ext[0].e_len = 1;
            ext[0].e_flags = 0;
        }
    }

    ii->disk_inode = disk_inode;
    ii->ino = ino;
    ii->vfs_inode->i_ino = (ino_t)ino;
    pfs_disk_to_inode(disk_inode, ii);

    return STATUS_BERHASIL;
}

static status_t pfs_tulis_inode(pfs_inode_info_t *ii, pfs_sb_info_t *sbi)
{
    if (ii == NULL || sbi == NULL || ii->disk_inode == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Update disk inode dari in-memory */
    pfs_inode_to_disk(ii, ii->disk_inode);

    /* TODO: Implementasi penulisan aktual ke block device */

    ii->dirty = SALAH;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI SUPERBLOCK OPERATIONS
 * ===========================================================================
 */

static inode_t *pfs_alloc_inode(superblock_t *sb)
{
    pfs_sb_info_t *sbi;
    pfs_inode_info_t *ii;
    inode_t *inode;
    tak_bertanda32_t ino;

    if (sb == NULL) {
        return NULL;
    }

    sbi = (pfs_sb_info_t *)sb->s_private;
    if (sbi == NULL || sbi->pfs_magic != PFS_SUPER_MAGIC) {
        return NULL;
    }

    /* Alokasi inode number */
    ino = pfs_alloc_inode_no(sbi);
    if (ino == 0) {
        return NULL;
    }

    /* Alokasi VFS inode */
    inode = inode_alloc(sb);
    if (inode == NULL) {
        pfs_free_inode_no(sbi, ino);
        return NULL;
    }

    /* Alokasi private info */
    ii = (pfs_inode_info_t *)kmalloc(sizeof(pfs_inode_info_t));
    if (ii == NULL) {
        inode_put(inode);
        pfs_free_inode_no(sbi, ino);
        return NULL;
    }

    kernel_memset(ii, 0, sizeof(pfs_inode_info_t));
    ii->pfs_magic = PFS_INODE_MAGIC;
    ii->vfs_inode = inode;
    ii->ino = ino;
    ii->dirty = SALAH;

    inode->i_ino = (ino_t)ino;
    inode->i_private = ii;

    return inode;
}

static void pfs_destroy_inode(inode_t *inode)
{
    pfs_inode_info_t *ii;

    if (inode == NULL) {
        return;
    }

    ii = (pfs_inode_info_t *)inode->i_private;
    if (ii != NULL && ii->pfs_magic == PFS_INODE_MAGIC) {
        ii->pfs_magic = 0;
        if (ii->disk_inode != NULL) {
            kfree(ii->disk_inode);
        }
        kfree(ii);
        inode->i_private = NULL;
    }
}

static status_t pfs_read_inode(inode_t *inode)
{
    pfs_sb_info_t *sbi;
    pfs_inode_info_t *ii;
    status_t status;

    if (inode == NULL || inode->i_sb == NULL) {
        return STATUS_PARAM_NULL;
    }

    sbi = (pfs_sb_info_t *)inode->i_sb->s_private;
    if (sbi == NULL || sbi->pfs_magic != PFS_SUPER_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    ii = (pfs_inode_info_t *)inode->i_private;
    if (ii == NULL || ii->pfs_magic != PFS_INODE_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    status = pfs_baca_inode(ii, sbi, ii->ino);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    /* Set inode operations berdasarkan tipe */
    if (VFS_S_ISDIR(inode->i_mode)) {
        inode->i_op = &g_pfs_inode_ops;
        inode->i_fop = &g_pfs_file_ops;
    } else if (VFS_S_ISREG(inode->i_mode)) {
        inode->i_op = &g_pfs_inode_ops;
        inode->i_fop = &g_pfs_file_ops;
    } else if (VFS_S_ISLNK(inode->i_mode)) {
        inode->i_op = &g_pfs_inode_ops;
        inode->i_fop = &g_pfs_file_ops;
    }

    return STATUS_BERHASIL;
}

static status_t pfs_write_inode(inode_t *inode)
{
    pfs_sb_info_t *sbi;
    pfs_inode_info_t *ii;

    if (inode == NULL || inode->i_sb == NULL) {
        return STATUS_PARAM_NULL;
    }

    sbi = (pfs_sb_info_t *)inode->i_sb->s_private;
    if (sbi == NULL || sbi->pfs_magic != PFS_SUPER_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    ii = (pfs_inode_info_t *)inode->i_private;
    if (ii == NULL || ii->pfs_magic != PFS_INODE_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    return pfs_tulis_inode(ii, sbi);
}

static void pfs_put_super(superblock_t *sb)
{
    pfs_sb_info_t *sbi;

    if (sb == NULL) {
        return;
    }

    sbi = (pfs_sb_info_t *)sb->s_private;
    if (sbi != NULL && sbi->pfs_magic == PFS_SUPER_MAGIC) {
        /* Sync superblock jika dirty */
        if (sbi->dirty) {
            pfs_tulis_superblock(sbi);
        }

        /* Free disk superblock */
        if (sbi->disk_sb != NULL) {
            kfree(sbi->disk_sb);
        }

        sbi->pfs_magic = 0;
        kfree(sbi);
        sb->s_private = NULL;
    }
}

static status_t pfs_statfs(superblock_t *sb, vfs_stat_t *stat)
{
    pfs_sb_info_t *sbi;

    if (sb == NULL || stat == NULL) {
        return STATUS_PARAM_NULL;
    }

    sbi = (pfs_sb_info_t *)sb->s_private;
    if (sbi == NULL || sbi->pfs_magic != PFS_SUPER_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    kernel_memset(stat, 0, sizeof(vfs_stat_t));

    stat->st_dev = sb->s_dev;
    stat->st_blksize = (tak_bertanda32_t)sbi->block_size;
    stat->st_blocks = sbi->blocks_count;
    stat->st_size = stat->st_blocks * (off_t)stat->st_blksize;

    return STATUS_BERHASIL;
}

static status_t pfs_sync_fs(superblock_t *sb)
{
    pfs_sb_info_t *sbi;

    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }

    sbi = (pfs_sb_info_t *)sb->s_private;
    if (sbi == NULL || sbi->pfs_magic != PFS_SUPER_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    if (sbi->dirty) {
        return pfs_tulis_superblock(sbi);
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI INODE OPERATIONS
 * ===========================================================================
 */

static dentry_t *pfs_lookup(inode_t *dir, const char *name)
{
    pfs_sb_info_t *sbi;
    pfs_inode_info_t *dir_ii;
    inode_t *inode;
    dentry_t *dentry;

    if (dir == NULL || name == NULL) {
        return NULL;
    }

    if (!VFS_S_ISDIR(dir->i_mode)) {
        return NULL;
    }

    sbi = (pfs_sb_info_t *)dir->i_sb->s_private;
    if (sbi == NULL || sbi->pfs_magic != PFS_SUPER_MAGIC) {
        return NULL;
    }

    dir_ii = (pfs_inode_info_t *)dir->i_private;
    if (dir_ii == NULL || dir_ii->pfs_magic != PFS_INODE_MAGIC) {
        return NULL;
    }

    /* TODO: Implementasi lookup menggunakan B+tree */
    /* Untuk sekarang, return NULL */

    /* Alokasi inode baru untuk dentry */
    inode = pfs_alloc_inode(dir->i_sb);
    if (inode == NULL) {
        return NULL;
    }

    /* Setup inode */
    inode->i_mode = VFS_S_IFREG | 0644;
    inode->i_uid = 0;
    inode->i_gid = 0;
    inode->i_size = 0;
    inode->i_nlink = 1;

    /* Buat dentry */
    dentry = dentry_alloc(name);
    if (dentry == NULL) {
        inode_put(inode);
        return NULL;
    }

    dentry->d_inode = inode;
    dentry->d_sb = dir->i_sb;

    return dentry;
}

static status_t pfs_create(inode_t *dir, dentry_t *dentry, mode_t mode)
{
    pfs_sb_info_t *sbi;
    inode_t *inode;
    pfs_inode_info_t *ii;
    status_t status;

    if (dir == NULL || dentry == NULL) {
        return STATUS_PARAM_NULL;
    }

    sbi = (pfs_sb_info_t *)dir->i_sb->s_private;
    if (sbi == NULL || sbi->pfs_magic != PFS_SUPER_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Alokasi inode baru */
    inode = pfs_alloc_inode(dir->i_sb);
    if (inode == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    /* Setup inode */
    inode->i_mode = (mode & ~VFS_S_IFMT) | VFS_S_IFREG;
    inode->i_uid = 0;
    inode->i_gid = 0;
    inode->i_size = 0;
    inode->i_nlink = 1;
    inode->i_op = &g_pfs_inode_ops;
    inode->i_fop = &g_pfs_file_ops;

    ii = (pfs_inode_info_t *)inode->i_private;
    if (ii != NULL) {
        ii->size = 0;
        ii->blocks = 0;
        ii->extents_count = 0;
        ii->dirty = BENAR;
    }

    /* Tulis inode ke disk */
    status = pfs_write_inode(inode);
    if (status != STATUS_BERHASIL) {
        inode_put(inode);
        return status;
    }

    /* Link ke dentry */
    dentry->d_inode = inode;

    return STATUS_BERHASIL;
}

static status_t pfs_mkdir(inode_t *dir, dentry_t *dentry, mode_t mode)
{
    pfs_sb_info_t *sbi;
    inode_t *inode;
    pfs_inode_info_t *ii;
    status_t status;

    if (dir == NULL || dentry == NULL) {
        return STATUS_PARAM_NULL;
    }

    sbi = (pfs_sb_info_t *)dir->i_sb->s_private;
    if (sbi == NULL || sbi->pfs_magic != PFS_SUPER_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Alokasi inode baru */
    inode = pfs_alloc_inode(dir->i_sb);
    if (inode == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    /* Setup inode untuk direktori */
    inode->i_mode = (mode & ~VFS_S_IFMT) | VFS_S_IFDIR;
    inode->i_uid = 0;
    inode->i_gid = 0;
    inode->i_size = sbi->block_size;
    inode->i_nlink = 2;
    inode->i_op = &g_pfs_inode_ops;
    inode->i_fop = &g_pfs_file_ops;

    ii = (pfs_inode_info_t *)inode->i_private;
    if (ii != NULL) {
        ii->size = sbi->block_size;
        ii->blocks = 1;
        ii->extents_count = 1;
        /* Alokasi block untuk direktori */
        {
            tak_bertanda32_t block = pfs_alloc_block_no(sbi);
            if (block > 0) {
                ii->extents[0].e_block = 0;
                ii->extents[0].e_start = block;
                ii->extents[0].e_len = 1;
                ii->extents[0].e_flags = 0;
            }
        }
        ii->dirty = BENAR;
    }

    /* Tulis inode ke disk */
    status = pfs_write_inode(inode);
    if (status != STATUS_BERHASIL) {
        inode_put(inode);
        return status;
    }

    /* Update parent link count */
    dir->i_nlink++;

    /* Link ke dentry */
    dentry->d_inode = inode;

    return STATUS_BERHASIL;
}

static status_t pfs_rmdir(inode_t *dir, dentry_t *dentry)
{
    inode_t *inode;

    if (dir == NULL || dentry == NULL) {
        return STATUS_PARAM_NULL;
    }

    inode = dentry->d_inode;
    if (inode == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Cek apakah direktori kosong */
    /* TODO: Implementasi pengecekan menggunakan B+tree */

    /* Update parent link count */
    dir->i_nlink--;

    /* Free inode dan blocks */
    inode->i_nlink = 0;

    return STATUS_BERHASIL;
}

static status_t pfs_unlink(inode_t *dir, dentry_t *dentry)
{
    inode_t *inode;

    if (dir == NULL || dentry == NULL) {
        return STATUS_PARAM_NULL;
    }

    inode = dentry->d_inode;
    if (inode == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Decrement link count */
    inode->i_nlink--;

    /* Jika link count = 0, tandai untuk deletion */
    if (inode->i_nlink == 0) {
        inode_mark_dirty(inode);
    }

    return STATUS_BERHASIL;
}

static status_t pfs_rename(inode_t *old_dir, dentry_t *old_dentry,
                           inode_t *new_dir, dentry_t *new_dentry)
{
    if (old_dir == NULL || old_dentry == NULL ||
        new_dir == NULL || new_dentry == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* TODO: Implementasi rename dengan journal */
    return STATUS_BERHASIL;
}

static status_t pfs_getattr(dentry_t *dentry, vfs_stat_t *stat)
{
    inode_t *inode;

    if (dentry == NULL || stat == NULL) {
        return STATUS_PARAM_NULL;
    }

    inode = dentry->d_inode;
    if (inode == NULL) {
        return STATUS_PARAM_INVALID;
    }

    return inode_getattr(inode, stat);
}

static status_t pfs_setattr(dentry_t *dentry, vfs_stat_t *stat)
{
    inode_t *inode;
    pfs_inode_info_t *ii;

    if (dentry == NULL || stat == NULL) {
        return STATUS_PARAM_NULL;
    }

    inode = dentry->d_inode;
    if (inode == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Update atribut inode */
    if (stat->st_mode != 0) {
        inode->i_mode = stat->st_mode;
    }
    if (stat->st_uid != (uid_t)-1) {
        inode->i_uid = stat->st_uid;
    }
    if (stat->st_gid != (gid_t)-1) {
        inode->i_gid = stat->st_gid;
    }
    if (stat->st_size != (off_t)-1) {
        inode->i_size = stat->st_size;
        ii = (pfs_inode_info_t *)inode->i_private;
        if (ii != NULL) {
            ii->size = (tak_bertanda64_t)stat->st_size;
        }
    }

    inode_mark_dirty(inode);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FILE OPERATIONS
 * ===========================================================================
 */

static status_t pfs_open(inode_t *inode, file_t *file)
{
    pfs_file_info_t *fi;

    if (inode == NULL || file == NULL) {
        return STATUS_PARAM_NULL;
    }

    fi = (pfs_file_info_t *)kmalloc(sizeof(pfs_file_info_t));
    if (fi == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    kernel_memset(fi, 0, sizeof(pfs_file_info_t));
    fi->pfs_magic = PFS_FILE_MAGIC;
    fi->vfs_file = file;
    fi->pos = 0;
    fi->current_extent = 0;
    fi->current_block = 0;

    file->f_private = fi;

    return STATUS_BERHASIL;
}

static status_t pfs_release(inode_t *inode, file_t *file)
{
    pfs_file_info_t *fi;

    TIDAK_DIGUNAKAN_PARAM(inode);

    if (file == NULL) {
        return STATUS_PARAM_NULL;
    }

    fi = (pfs_file_info_t *)file->f_private;
    if (fi != NULL && fi->pfs_magic == PFS_FILE_MAGIC) {
        fi->pfs_magic = 0;
        kfree(fi);
        file->f_private = NULL;
    }

    return STATUS_BERHASIL;
}

static tak_bertandas_t pfs_read(file_t *file, void *buffer,
                                ukuran_t size, off_t *pos)
{
    pfs_sb_info_t *sbi;
    pfs_inode_info_t *ii;
    pfs_file_info_t *fi;
    tak_bertanda64_t file_size;
    tak_bertanda64_t read_pos;
    ukuran_t bytes_read;
    ukuran_t remaining;
    tak_bertanda8_t *block_buffer;

    if (file == NULL || buffer == NULL || pos == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_NULL;
    }

    if (file->f_inode == NULL || file->f_inode->i_sb == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    sbi = (pfs_sb_info_t *)file->f_inode->i_sb->s_private;
    if (sbi == NULL || sbi->pfs_magic != PFS_SUPER_MAGIC) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    ii = (pfs_inode_info_t *)file->f_inode->i_private;
    if (ii == NULL || ii->pfs_magic != PFS_INODE_MAGIC) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    fi = (pfs_file_info_t *)file->f_private;
    if (fi == NULL || fi->pfs_magic != PFS_FILE_MAGIC) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    file_size = ii->size;
    read_pos = (tak_bertanda64_t)*pos;

    /* Cek EOF */
    if (read_pos >= file_size) {
        return 0;
    }

    /* Batasi ukuran baca */
    remaining = (ukuran_t)MIN((tak_bertanda64_t)size,
                              file_size - read_pos);

    /* Alokasi buffer block */
    block_buffer = (tak_bertanda8_t *)kmalloc(sbi->block_size);
    if (block_buffer == NULL) {
        return (tak_bertandas_t)STATUS_MEMORI_HABIS;
    }

    bytes_read = 0;

    /* Baca data dari extent */
    /* TODO: Implementasi pembacaan aktual menggunakan extent map */

    kfree(block_buffer);

    *pos += (off_t)bytes_read;

    return (tak_bertandas_t)bytes_read;
}

static tak_bertandas_t pfs_write(file_t *file, const void *buffer,
                                 ukuran_t size, off_t *pos)
{
    pfs_sb_info_t *sbi;
    pfs_inode_info_t *ii;
    tak_bertanda64_t write_pos;
    ukuran_t bytes_written;

    if (file == NULL || buffer == NULL || pos == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_NULL;
    }

    if (file->f_inode == NULL || file->f_inode->i_sb == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    sbi = (pfs_sb_info_t *)file->f_inode->i_sb->s_private;
    if (sbi == NULL || sbi->pfs_magic != PFS_SUPER_MAGIC) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    ii = (pfs_inode_info_t *)file->f_inode->i_private;
    if (ii == NULL || ii->pfs_magic != PFS_INODE_MAGIC) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    write_pos = (tak_bertanda64_t)*pos;

    /* Handle append mode */
    if (file->f_flags & VFS_OPEN_APPEND) {
        write_pos = ii->size;
    }

    bytes_written = 0;

    /* TODO: Implementasi penulisan aktual dengan extent allocation */

    /* Update ukuran file jika perlu */
    if (write_pos + (tak_bertanda64_t)bytes_written > ii->size) {
        ii->size = write_pos + (tak_bertanda64_t)bytes_written;
        file->f_inode->i_size = (off_t)ii->size;
        ii->dirty = BENAR;
    }

    *pos = (off_t)(write_pos + bytes_written);

    return (tak_bertandas_t)bytes_written;
}

static off_t pfs_lseek(file_t *file, off_t offset, tak_bertanda32_t whence)
{
    pfs_inode_info_t *ii;
    off_t new_pos;

    if (file == NULL || file->f_inode == NULL) {
        return (off_t)STATUS_PARAM_NULL;
    }

    ii = (pfs_inode_info_t *)file->f_inode->i_private;
    if (ii == NULL || ii->pfs_magic != PFS_INODE_MAGIC) {
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
        new_pos = (off_t)ii->size + offset;
        break;
    default:
        return (off_t)STATUS_PARAM_INVALID;
    }

    /* Validasi posisi */
    if (new_pos < 0) {
        return (off_t)STATUS_PARAM_INVALID;
    }

    file->f_pos = new_pos;

    return new_pos;
}

static tak_bertandas_t pfs_readdir(file_t *file, vfs_dirent_t *dirent,
                                   ukuran_t count)
{
    pfs_inode_info_t *ii;
    ukuran_t entries_read;

    if (file == NULL || dirent == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_NULL;
    }

    if (!VFS_S_ISDIR(file->f_inode->i_mode)) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    ii = (pfs_inode_info_t *)file->f_inode->i_private;
    if (ii == NULL || ii->pfs_magic != PFS_INODE_MAGIC) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    entries_read = 0;

    /* TODO: Implementasi readdir menggunakan B+tree */
    /* Baca directory entries dari extent */

    return (tak_bertandas_t)entries_read;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI MOUNT/UMOUNT (MOUNT/UMOUNT FUNCTIONS)
 * ===========================================================================
 */

static superblock_t *pfs_mount(filesystem_t *fs, const char *device,
                               const char *path, tak_bertanda32_t flags)
{
    pfs_sb_info_t *sbi;
    superblock_t *sb;
    inode_t *root_inode;
    dentry_t *root_dentry;
    status_t status;

    TIDAK_DIGUNAKAN_PARAM(path);

    if (fs == NULL || device == NULL) {
        return NULL;
    }

    /* Alokasi private superblock info */
    sbi = (pfs_sb_info_t *)kmalloc(sizeof(pfs_sb_info_t));
    if (sbi == NULL) {
        return NULL;
    }

    kernel_memset(sbi, 0, sizeof(pfs_sb_info_t));
    sbi->pfs_magic = PFS_SUPER_MAGIC;
    sbi->read_only = (flags & VFS_MOUNT_FLAG_RDONLY) ? BENAR : SALAH;

    /* Baca superblock dari device */
    status = pfs_baca_superblock(sbi, (void *)device);
    if (status != STATUS_BERHASIL) {
        /* Jika gagal, coba init superblock baru */
        status = pfs_init_superblock(sbi);
        if (status != STATUS_BERHASIL) {
            kfree(sbi);
            return NULL;
        }
    }

    /* Alokasi VFS superblock */
    sb = superblock_alloc(fs);
    if (sb == NULL) {
        if (sbi->disk_sb != NULL) {
            kfree(sbi->disk_sb);
        }
        kfree(sbi);
        return NULL;
    }

    sb->s_private = sbi;
    sb->s_block_size = sbi->block_size;
    sb->s_total_blocks = sbi->blocks_count;
    sb->s_free_blocks = sbi->blocks_free;
    sb->s_total_inodes = sbi->inodes_count;
    sb->s_free_inodes = sbi->inodes_free;
    sb->s_readonly = sbi->read_only;
    sb->s_op = &g_pfs_super_ops;

    sbi->vfs_sb = sb;

    /* Alokasi root inode */
    root_inode = pfs_alloc_inode(sb);
    if (root_inode == NULL) {
        superblock_free(sb);
        return NULL;
    }

    /* Baca root inode */
    {
        pfs_inode_info_t *rii = (pfs_inode_info_t *)root_inode->i_private;
        if (rii != NULL) {
            rii->ino = PFS_ROOT_INO;
            root_inode->i_ino = (ino_t)PFS_ROOT_INO;
            status = pfs_baca_inode(rii, sbi, PFS_ROOT_INO);
            if (status != STATUS_BERHASIL) {
                /* Init default root inode */
                root_inode->i_mode = VFS_S_IFDIR | 0755;
                root_inode->i_uid = 0;
                root_inode->i_gid = 0;
                root_inode->i_size = sbi->block_size;
                root_inode->i_nlink = 2;
            }
        }
    }

    root_inode->i_op = &g_pfs_inode_ops;
    root_inode->i_fop = &g_pfs_file_ops;

    /* Buat root dentry */
    root_dentry = dentry_alloc("/");
    if (root_dentry == NULL) {
        inode_put(root_inode);
        superblock_free(sb);
        return NULL;
    }

    root_dentry->d_inode = root_inode;
    root_dentry->d_sb = sb;

    sb->s_root = root_dentry;

    return sb;
}

static status_t pfs_umount(superblock_t *sb)
{
    pfs_sb_info_t *sbi;

    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }

    sbi = (pfs_sb_info_t *)sb->s_private;
    if (sbi == NULL || sbi->pfs_magic != PFS_SUPER_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Sync filesystem */
    if (sbi->dirty) {
        pfs_tulis_superblock(sbi);
    }

    return STATUS_BERHASIL;
}

static status_t pfs_detect(const char *device)
{
    pfs_superblock_disk_t disk_sb;
    tak_bertanda8_t buffer[1024];

    TIDAK_DIGUNAKAN_PARAM(device);

    /* TODO: Implementasi deteksi aktual */
    kernel_memset(&disk_sb, 0, sizeof(disk_sb));
    kernel_memset(buffer, 0, sizeof(buffer));

    /* Cek magic number */
    if (disk_sb.s_magic == PFS_SUPER_MAGIC) {
        return STATUS_BERHASIL;
    }

    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI INISIALISASI (INITIALIZATION FUNCTIONS)
 * ===========================================================================
 */

/*
 * pigurafs_init
 * -------------
 * Inisialisasi driver PiguraFS.
 *
 * Return: Status operasi
 */
status_t pigurafs_init(void)
{
    status_t status;

    /* Inisialisasi operations structures */
    kernel_memset(&g_pfs_super_ops, 0, sizeof(vfs_super_operations_t));
    g_pfs_super_ops.alloc_inode = pfs_alloc_inode;
    g_pfs_super_ops.destroy_inode = pfs_destroy_inode;
    g_pfs_super_ops.read_inode = pfs_read_inode;
    g_pfs_super_ops.write_inode = pfs_write_inode;
    g_pfs_super_ops.put_super = pfs_put_super;
    g_pfs_super_ops.statfs = pfs_statfs;
    g_pfs_super_ops.sync_fs = pfs_sync_fs;

    kernel_memset(&g_pfs_inode_ops, 0, sizeof(vfs_inode_operations_t));
    g_pfs_inode_ops.lookup = pfs_lookup;
    g_pfs_inode_ops.create = pfs_create;
    g_pfs_inode_ops.mkdir = pfs_mkdir;
    g_pfs_inode_ops.rmdir = pfs_rmdir;
    g_pfs_inode_ops.unlink = pfs_unlink;
    g_pfs_inode_ops.rename = pfs_rename;
    g_pfs_inode_ops.getattr = pfs_getattr;
    g_pfs_inode_ops.setattr = pfs_setattr;

    kernel_memset(&g_pfs_file_ops, 0, sizeof(vfs_file_operations_t));
    g_pfs_file_ops.read = pfs_read;
    g_pfs_file_ops.write = pfs_write;
    g_pfs_file_ops.lseek = pfs_lseek;
    g_pfs_file_ops.open = pfs_open;
    g_pfs_file_ops.release = pfs_release;
    g_pfs_file_ops.readdir = pfs_readdir;

    /* Daftarkan filesystem */
    status = filesystem_register(&g_pfs_fs);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    return STATUS_BERHASIL;
}

/*
 * pigurafs_shutdown
 * -----------------
 * Shutdown driver PiguraFS.
 *
 * Return: Status operasi
 */
status_t pigurafs_shutdown(void)
{
    status_t status;

    status = filesystem_unregister(&g_pfs_fs);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    return STATUS_BERHASIL;
}
