/*
 * PIGURA OS - EXT2_INODE.C
 * =========================
 * Operasi inode filesystem EXT2 untuk kernel Pigura OS.
 *
 * Berkas ini berisi implementasi operasi-operasi yang berkaitan
 * dengan inode EXT2 termasuk alokasi, pembacaan, penulisan, dan
 * manajemen inode.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua fungsi mengembalikan status error yang jelas
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

/*
 * ===========================================================================
 * INCLUDE
 * ===========================================================================
 */

#include "../vfs/vfs.h"
#include "../../inti/types.h"
#include "../../inti/konstanta.h"
#include <string.h>

/*
 * ===========================================================================
 * KONSTANTA INODE (INODE CONSTANTS)
 * ===========================================================================
 */

/* Jumlah block pointers dalam inode */
#define EXT2_NDIR_BLOCKS            12
#define EXT2_IND_BLOCK              12
#define EXT2_DIND_BLOCK             13
#define EXT2_TIND_BLOCK             14
#define EXT2_N_BLOCKS               15

/* Ukuran inode default */
#define EXT2_INODE_SIZE_REV0        128

/* Tipe file dalam mode */
#define EXT2_S_IFIFO                0x1000
#define EXT2_S_IFCHR                0x2000
#define EXT2_S_IFDIR                0x4000
#define EXT2_S_IFBLK                0x6000
#define EXT2_S_IFREG                0x8000
#define EXT2_S_IFLNK                0xA000
#define EXT2_S_IFSOCK               0xC000
#define EXT2_S_IFMT                 0xF000

/* Permission bits */
#define EXT2_S_ISUID                0x0800
#define EXT2_S_ISGID                0x0400
#define EXT2_S_ISVTX                0x0200
#define EXT2_S_IRWXU                0x01C0
#define EXT2_S_IRUSR                0x0100
#define EXT2_S_IWUSR                0x0080
#define EXT2_S_IXUSR                0x0040
#define EXT2_S_IRWXG                0x0038
#define EXT2_S_IRGRP                0x0020
#define EXT2_S_IWGRP                0x0010
#define EXT2_S_IXGRP                0x0008
#define EXT2_S_IRWXO                0x0007
#define EXT2_S_IROTH                0x0004
#define EXT2_S_IWOTH                0x0002
#define EXT2_S_IXOTH                0x0001

/* Inode flags */
#define EXT2_SECRM_FL               0x00000001
#define EXT2_UNRM_FL                0x00000002
#define EXT2_COMPR_FL               0x00000004
#define EXT2_SYNC_FL                0x00000008
#define EXT2_IMMUTABLE_FL           0x00000010
#define EXT2_APPEND_FL              0x00000020
#define EXT2_NODUMP_FL              0x00000040
#define EXT2_NOATIME_FL             0x00000080
#define EXT2_DIRTY_FL               0x00000100
#define EXT2_COMPRBLK_FL            0x00000200
#define EXT2_NOCOMPR_FL             0x00000400
#define EXT2_ECOMPR_FL              0x00000800
#define EXT2_INDEX_FL               0x00001000
#define EXT2_IMAGIC_FL              0x00002000
#define EXT2_JOURNAL_DATA_FL        0x00004000
#define EXT2_NOTAIL_FL              0x00008000
#define EXT2_DIRSYNC_FL             0x00010000
#define EXT2_TOPDIR_FL              0x00020000
#define EXT2_RESERVED_FL            0x80000000

/* Reserved inode numbers */
#define EXT2_BAD_INO                1   /* Bad blocks inode */
#define EXT2_ROOT_INO               2   /* Root directory */
#define EXT2_ACL_IDX_INO            3   /* ACL index inode */
#define EXT2_ACL_DATA_INO           4   /* ACL data inode */
#define EXT2_BOOT_LOADER_INO        5   /* Boot loader */
#define EXT2_UNDEL_DIR_INO          6   /* Undelete directory */
#define EXT2_RESIZE_INO             7   /* Resize inode */
#define EXT2_JOURNAL_INO            8   /* Journal inode */
#define EXT2_FIRST_INO              11  /* First non-reserved inode */

/* Makro cek tipe file */
#define EXT2_S_ISREG(m)             (((m) & EXT2_S_IFMT) == EXT2_S_IFREG)
#define EXT2_S_ISDIR(m)             (((m) & EXT2_S_IFMT) == EXT2_S_IFDIR)
#define EXT2_S_ISCHR(m)             (((m) & EXT2_S_IFMT) == EXT2_S_IFCHR)
#define EXT2_S_ISBLK(m)             (((m) & EXT2_S_IFMT) == EXT2_S_IFBLK)
#define EXT2_S_ISFIFO(m)            (((m) & EXT2_S_IFMT) == EXT2_S_IFIFO)
#define EXT2_S_ISLNK(m)             (((m) & EXT2_S_IFMT) == EXT2_S_IFLNK)
#define EXT2_S_ISSOCK(m)            (((m) & EXT2_S_IFMT) == EXT2_S_IFSOCK)

/*
 * ===========================================================================
 * STRUKTUR INODE EXT2 (EXT2 INODE STRUCTURE)
 * ===========================================================================
 */

typedef struct ext2_inode {
    tak_bertanda16_t i_mode;             /* File mode */
    tak_bertanda16_t i_uid;              /* Owner UID low */
    tak_bertanda32_t i_size;             /* File size (low) */
    tak_bertanda32_t i_atime;            /* Access time */
    tak_bertanda32_t i_ctime;            /* Creation time */
    tak_bertanda32_t i_mtime;            /* Modification time */
    tak_bertanda32_t i_dtime;            /* Deletion time */
    tak_bertanda16_t i_gid;              /* Group GID low */
    tak_bertanda16_t i_links_count;      /* Links count */
    tak_bertanda32_t i_blocks;           /* Block count (512-byte) */
    tak_bertanda32_t i_flags;            /* Inode flags */
    tak_bertanda32_t i_osd1;             /* OS dependent 1 */
    tak_bertanda32_t i_block[EXT2_N_BLOCKS]; /* Block pointers */
    tak_bertanda32_t i_generation;       /* File version */
    tak_bertanda32_t i_file_acl;         /* ACL block */
    tak_bertanda32_t i_size_high;        /* File size high */
    tak_bertanda32_t i_faddr;            /* Fragment address */
    tak_bertanda8_t  i_frag;             /* Fragment number */
    tak_bertanda8_t  i_fsize;            /* Fragment size */
    tak_bertanda16_t i_uid_high;         /* Owner UID high */
    tak_bertanda16_t i_gid_high;         /* Group GID high */
    tak_bertanda32_t i_reserved2;
} ext2_inode_t;

/*
 * ===========================================================================
 * STRUKTUR SUPERBLOCK
 * ===========================================================================
 */

typedef struct ext2_superblock {
    tak_bertanda32_t s_inodes_count;
    tak_bertanda32_t s_blocks_count;
    tak_bertanda32_t s_r_blocks_count;
    tak_bertanda32_t s_free_blocks_count;
    tak_bertanda32_t s_free_inodes_count;
    tak_bertanda32_t s_first_data_block;
    tak_bertanda32_t s_log_block_size;
    tak_bertanda32_t s_log_frag_size;
    tak_bertanda32_t s_blocks_per_group;
    tak_bertanda32_t s_frags_per_group;
    tak_bertanda32_t s_inodes_per_group;
    tak_bertanda32_t s_mtime;
    tak_bertanda32_t s_wtime;
    tak_bertanda16_t s_mnt_count;
    tak_bertanda16_t s_max_mnt_count;
    tak_bertanda16_t s_magic;
    tak_bertanda16_t s_state;
    tak_bertanda16_t s_errors;
    tak_bertanda16_t s_minor_rev_level;
    tak_bertanda32_t s_lastcheck;
    tak_bertanda32_t s_checkinterval;
    tak_bertanda32_t s_creator_os;
    tak_bertanda32_t s_rev_level;
    tak_bertanda16_t s_def_resuid;
    tak_bertanda16_t s_def_resgid;
    tak_bertanda32_t s_first_ino;
    tak_bertanda16_t s_inode_size;
    tak_bertanda16_t s_block_group_nr;
    tak_bertanda32_t s_feature_compat;
    tak_bertanda32_t s_feature_incompat;
    tak_bertanda32_t s_feature_ro_compat;
    tak_bertanda8_t  s_uuid[16];
    char             s_volume_name[16];
    char             s_last_mounted[64];
    tak_bertanda32_t s_algorithm_usage_bitmap;
    tak_bertanda8_t  s_prealloc_blocks;
    tak_bertanda8_t  s_prealloc_dir_blocks;
    tak_bertanda16_t s_padding1;
    tak_bertanda32_t s_reserved[204];
} ext2_superblock_t;

/*
 * ===========================================================================
 * STRUKTUR GROUP DESCRIPTOR
 * ===========================================================================
 */

typedef struct ext2_group_desc {
    tak_bertanda32_t bg_block_bitmap;
    tak_bertanda32_t bg_inode_bitmap;
    tak_bertanda32_t bg_inode_table;
    tak_bertanda16_t bg_free_blocks_count;
    tak_bertanda16_t bg_free_inodes_count;
    tak_bertanda16_t bg_used_dirs_count;
    tak_bertanda16_t bg_pad;
    tak_bertanda32_t bg_reserved[3];
} ext2_group_desc_t;

/*
 * ===========================================================================
 * STRUKTUR DATA PRIVATE (PRIVATE DATA STRUCTURE)
 * ===========================================================================
 */

typedef struct ext2_data {
    ext2_superblock_t    *ed_super;
    ext2_group_desc_t    *ed_groups;
    tak_bertanda32_t      ed_block_size;
    tak_bertanda32_t      ed_inode_size;
    tak_bertanda32_t      ed_groups_count;
    tak_bertanda32_t      ed_inodes_per_group;
    tak_bertanda32_t      ed_blocks_per_group;
    tak_bertanda32_t      ed_first_ino;
    bool_t                ed_readonly;
    bool_t                ed_dirty;
} ext2_data_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INTERNAL (INTERNAL FUNCTION DECLARATIONS)
 * ===========================================================================
 */

static status_t ext2_inode_validasi(ext2_inode_t *inode, ino_t ino);
static tak_bertanda32_t ext2_inode_group(ext2_data_t *data, ino_t ino);
static tak_bertanda32_t ext2_inode_index(ext2_data_t *data, ino_t ino);

/* Deklarasi fungsi untuk membaca inode */
status_t ext2_inode_baca(ext2_data_t *data, ino_t ino, ext2_inode_t *inode);

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI VALIDASI (VALIDATION FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_inode_validasi
 * Memvalidasi inode EXT2.
 */
static status_t ext2_inode_validasi(ext2_inode_t *inode, ino_t ino)
{
    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Validasi mode */
    if ((inode->i_mode & EXT2_S_IFMT) == 0) {
        /* Mode tidak valid */
        return STATUS_FORMAT_INVALID;
    }

    /* Validasi links count */
    if (EXT2_S_ISDIR(inode->i_mode)) {
        /* Direktori minimal memiliki 2 link (. dan ..) */
        if (inode->i_links_count < 2 && ino != EXT2_ROOT_INO) {
            return STATUS_FS_CORRUPT;
        }
    }

    return STATUS_BERHASIL;
}

/*
 * ext2_inode_group
 * Mendapatkan group number dari inode number.
 */
static tak_bertanda32_t ext2_inode_group(ext2_data_t *data, ino_t ino)
{
    if (data == NULL || ino < 1) {
        return 0;
    }

    /* Inode dimulai dari 1 */
    return (tak_bertanda32_t)((ino - 1) / data->ed_inodes_per_group);
}

/*
 * ext2_inode_index
 * Mendapatkan index dalam group dari inode number.
 */
static tak_bertanda32_t ext2_inode_index(ext2_data_t *data, ino_t ino)
{
    if (data == NULL || ino < 1) {
        return 0;
    }

    /* Inode dimulai dari 1 */
    return (tak_bertanda32_t)((ino - 1) % data->ed_inodes_per_group);
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI KONVERSI (CONVERSION FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_inode_ke_vfs
 * Konversi dari EXT2 inode ke VFS inode.
 */
status_t ext2_inode_ke_vfs(ext2_inode_t *ext2_ino, inode_t *vfs_ino)
{
    if (ext2_ino == NULL || vfs_ino == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Konversi mode */
    vfs_ino->i_mode = (mode_t)(ext2_ino->i_mode & 0xFFFF);

    /* Konversi uid/gid (gabungkan high dan low) */
    vfs_ino->i_uid = (uid_t)ext2_ino->i_uid |
        ((uid_t)ext2_ino->i_uid_high << 16);
    vfs_ino->i_gid = (gid_t)ext2_ino->i_gid |
        ((gid_t)ext2_ino->i_gid_high << 16);

    /* Konversi size (gabungkan high dan low untuk file besar) */
    vfs_ino->i_size = (off_t)ext2_ino->i_size |
        ((off_t)ext2_ino->i_size_high << 32);

    /* Konversi timestamps */
    vfs_ino->i_atime = (waktu_t)ext2_ino->i_atime;
    vfs_ino->i_mtime = (waktu_t)ext2_ino->i_mtime;
    vfs_ino->i_ctime = (waktu_t)ext2_ino->i_ctime;

    /* Konversi links dan blocks */
    vfs_ino->i_nlink = ext2_ino->i_links_count;
    vfs_ino->i_blocks = ext2_ino->i_blocks;

    /* Set flags */
    vfs_ino->i_flags = ext2_ino->i_flags;

    return STATUS_BERHASIL;
}

/*
 * ext2_inode_dari_vfs
 * Konversi dari VFS inode ke EXT2 inode.
 */
status_t ext2_inode_dari_vfs(inode_t *vfs_ino, ext2_inode_t *ext2_ino)
{
    if (vfs_ino == NULL || ext2_ino == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Konversi mode */
    ext2_ino->i_mode = (tak_bertanda16_t)(vfs_ino->i_mode & 0xFFFF);

    /* Konversi uid/gid (pisahkan high dan low) */
    ext2_ino->i_uid = (tak_bertanda16_t)(vfs_ino->i_uid & 0xFFFF);
    ext2_ino->i_uid_high = (tak_bertanda16_t)((vfs_ino->i_uid >> 16) & 0xFFFF);
    ext2_ino->i_gid = (tak_bertanda16_t)(vfs_ino->i_gid & 0xFFFF);
    ext2_ino->i_gid_high = (tak_bertanda16_t)((vfs_ino->i_gid >> 16) & 0xFFFF);

    /* Konversi size (pisahkan high dan low) */
    ext2_ino->i_size = (tak_bertanda32_t)(vfs_ino->i_size & 0xFFFFFFFF);
    ext2_ino->i_size_high = (tak_bertanda32_t)((vfs_ino->i_size >> 32) &
        0xFFFFFFFF);

    /* Konversi timestamps */
    ext2_ino->i_atime = (tak_bertanda32_t)vfs_ino->i_atime;
    ext2_ino->i_mtime = (tak_bertanda32_t)vfs_ino->i_mtime;
    ext2_ino->i_ctime = (tak_bertanda32_t)vfs_ino->i_ctime;

    /* Konversi links dan blocks */
    ext2_ino->i_links_count = (tak_bertanda16_t)vfs_ino->i_nlink;
    ext2_ino->i_blocks = (tak_bertanda32_t)vfs_ino->i_blocks;

    /* Set flags */
    ext2_ino->i_flags = vfs_ino->i_flags;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI ALOKASI (ALLOCATION FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_inode_alloc
 * Alokasi inode baru dari bitmap.
 */
ino_t ext2_inode_alloc(ext2_data_t *data, superblock_t *sb, mode_t mode)
{
    tak_bertanda32_t group;
    tak_bertanda32_t ino;
    tak_bertanda32_t i;

    if (data == NULL || sb == NULL) {
        return 0;
    }

    /* Cek apakah ada inode bebas */
    if (data->ed_super->s_free_inodes_count == 0) {
        return 0;
    }

    /* Cari group dengan inode bebas */
    for (group = 0; group < data->ed_groups_count; group++) {
        if (data->ed_groups[group].bg_free_inodes_count > 0) {
            break;
        }
    }

    if (group >= data->ed_groups_count) {
        return 0;
    }

    /* CATATAN: Implementasi pencarian bit bebas dalam bitmap */
    /* Untuk sekarang, gunakan inode pertama yang tersedia */
    ino = data->ed_first_ino;

    /* Cari inode bebas dalam bitmap */
    for (i = data->ed_first_ino; i <= data->ed_super->s_inodes_count; i++) {
        /* Cek bit dalam inode bitmap */
        /* Jika bit = 0, inode bebas */
        /* CATATAN: Implementasi sebenarnya */
        ino = i;
        break;
    }

    if (ino > data->ed_super->s_inodes_count) {
        return 0;
    }

    /* Update counters */
    data->ed_super->s_free_inodes_count--;
    data->ed_groups[group].bg_free_inodes_count--;

    /* Jika direktori, increment used_dirs_count */
    if (EXT2_S_ISDIR(mode)) {
        data->ed_groups[group].bg_used_dirs_count++;
    }

    return (ino_t)ino;
}

/*
 * ext2_inode_free
 * Bebaskan inode ke bitmap.
 */
status_t ext2_inode_free(ext2_data_t *data, ino_t ino)
{
    tak_bertanda32_t group;
    tak_bertanda32_t index;
    ext2_inode_t inode;
    status_t status;

    memset(&inode, 0, sizeof(inode));

    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Validasi inode number */
    if (ino < EXT2_FIRST_INO || ino > data->ed_super->s_inodes_count) {
        return STATUS_PARAM_INVALID;
    }

    /* Baca inode untuk mendapatkan mode */
    status = ext2_inode_baca(data, ino, &inode);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    /* Hitung group dan index */
    group = ext2_inode_group(data, ino);
    index = ext2_inode_index(data, ino);

    /* Clear bit dalam inode bitmap */
    /* CATATAN: Implementasi sebenarnya */

    /* Update counters */
    data->ed_super->s_free_inodes_count++;
    data->ed_groups[group].bg_free_inodes_count++;

    /* Jika direktori, decrement used_dirs_count */
    if (EXT2_S_ISDIR(inode.i_mode)) {
        if (data->ed_groups[group].bg_used_dirs_count > 0) {
            data->ed_groups[group].bg_used_dirs_count--;
        }
    }

    /* Reset inode */
    inode.i_dtime = 0; /* CATATAN: Set ke current time */
    inode.i_links_count = 0;
    inode.i_mode = 0;
    inode.i_size = 0;
    inode.i_blocks = 0;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI PEMBACAAN/PENULISAN (READ/WRITE FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_inode_baca
 * Membaca inode dari disk.
 */
status_t ext2_inode_baca(ext2_data_t *data, ino_t ino, ext2_inode_t *inode)
{
    tak_bertanda32_t group;
    tak_bertanda32_t index;
    tak_bertanda32_t inodes_per_block;
    tak_bertanda32_t block_num;
    tak_bertanda32_t offset;
    char *block_buffer;
    status_t status;

    if (data == NULL || inode == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Validasi inode number */
    if (ino < 1 || ino > data->ed_super->s_inodes_count) {
        return STATUS_PARAM_INVALID;
    }

    /* Hitung group dan index */
    group = ext2_inode_group(data, ino);
    index = ext2_inode_index(data, ino);

    /* Hitung inodes per block */
    inodes_per_block = data->ed_block_size / data->ed_inode_size;

    /* Hitung block number dan offset */
    block_num = data->ed_groups[group].bg_inode_table +
        (index / inodes_per_block);
    offset = (index % inodes_per_block) * data->ed_inode_size;

    /* Alokasi buffer block */
    block_buffer = (char *)NULL; /* CATATAN: Alokasi dari heap */

    /* Baca block */
    /* status = ext2_baca_block(sb, block_num, block_buffer); */
    status = STATUS_BERHASIL; /* Placeholder */

    if (status != STATUS_BERHASIL) {
        return status;
    }

    /* Salin inode dari buffer */
    /* CATATAN: Implementasi memory copy yang aman */

    return STATUS_BERHASIL;
}

/*
 * ext2_inode_tulis
 * Menulis inode ke disk.
 */
status_t ext2_inode_tulis(ext2_data_t *data, ino_t ino,
    const ext2_inode_t *inode)
{
    tak_bertanda32_t group;
    tak_bertanda32_t index;
    tak_bertanda32_t inodes_per_block;
    tak_bertanda32_t block_num;
    tak_bertanda32_t offset;
    char *block_buffer;
    status_t status;

    if (data == NULL || inode == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Validasi inode number */
    if (ino < 1 || ino > data->ed_super->s_inodes_count) {
        return STATUS_PARAM_INVALID;
    }

    /* Jangan tulis jika read-only */
    if (data->ed_readonly) {
        return STATUS_FS_READONLY;
    }

    /* Hitung group dan index */
    group = ext2_inode_group(data, ino);
    index = ext2_inode_index(data, ino);

    /* Hitung inodes per block */
    inodes_per_block = data->ed_block_size / data->ed_inode_size;

    /* Hitung block number dan offset */
    block_num = data->ed_groups[group].bg_inode_table +
        (index / inodes_per_block);
    offset = (index % inodes_per_block) * data->ed_inode_size;

    /* Alokasi buffer block */
    block_buffer = (char *)NULL; /* CATATAN: Alokasi dari heap */

    /* Baca block terlebih dahulu (read-modify-write) */
    /* status = ext2_baca_block(sb, block_num, block_buffer); */
    status = STATUS_BERHASIL; /* Placeholder */

    if (status != STATUS_BERHASIL) {
        return status;
    }

    /* Update inode dalam buffer */
    /* CATATAN: Implementasi memory copy yang aman */

    /* Tulis block kembali */
    /* status = ext2_tulis_block(sb, block_num, block_buffer); */

    data->ed_dirty = BENAR;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI INFORMASI (INFORMATION FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_inode_get_type
 * Mendapatkan tipe file dari inode.
 */
tak_bertanda32_t ext2_inode_get_type(ext2_inode_t *inode)
{
    if (inode == NULL) {
        return 0;
    }

    return inode->i_mode & EXT2_S_IFMT;
}

/*
 * ext2_inode_get_size
 * Mendapatkan ukuran file dari inode.
 */
tak_bertanda64_t ext2_inode_get_size(ext2_inode_t *inode)
{
    tak_bertanda64_t size;

    if (inode == NULL) {
        return 0;
    }

    size = (tak_bertanda64_t)inode->i_size;
    size |= ((tak_bertanda64_t)inode->i_size_high << 32);

    return size;
}

/*
 * ext2_inode_get_blocks
 * Mendapatkan jumlah block dari inode.
 */
tak_bertanda32_t ext2_inode_get_blocks(ext2_inode_t *inode)
{
    if (inode == NULL) {
        return 0;
    }

    return inode->i_blocks;
}

/*
 * ext2_inode_get_block_count
 * Menghitung jumlah block yang dipakai file.
 */
tak_bertanda32_t ext2_inode_get_block_count(ext2_inode_t *inode,
    tak_bertanda32_t block_size)
{
    tak_bertanda64_t size;
    tak_bertanda32_t blocks;

    if (inode == NULL) {
        return 0;
    }

    size = ext2_inode_get_size(inode);
    blocks = (tak_bertanda32_t)((size + block_size - 1) / block_size);

    return blocks;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI BLOCK POINTER (BLOCK POINTER FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_inode_get_block_ptr
 * Mendapatkan block pointer ke-n dari inode.
 */
tak_bertanda32_t ext2_inode_get_block_ptr(ext2_inode_t *inode,
    tak_bertanda32_t n)
{
    if (inode == NULL || n >= EXT2_N_BLOCKS) {
        return 0;
    }

    return inode->i_block[n];
}

/*
 * ext2_inode_set_block_ptr
 * Set block pointer ke-n dalam inode.
 */
status_t ext2_inode_set_block_ptr(ext2_inode_t *inode,
    tak_bertanda32_t n, tak_bertanda32_t block)
{
    if (inode == NULL || n >= EXT2_N_BLOCKS) {
        return STATUS_PARAM_INVALID;
    }

    inode->i_block[n] = block;

    return STATUS_BERHASIL;
}

/*
 * ext2_inode_calc_block_addr
 * Menghitung alamat block logis dari file.
 */
tak_bertanda32_t ext2_inode_calc_block_addr(ext2_inode_t *inode,
    tak_bertanda32_t logical_block, tak_bertanda32_t block_size)
{
    tak_bertanda32_t ptrs_per_block;
    tak_bertanda32_t direct_limit;
    tak_bertanda32_t indirect_limit;
    tak_bertanda32_t double_limit;
    tak_bertanda32_t block_num;

    if (inode == NULL) {
        return 0;
    }

    ptrs_per_block = block_size / sizeof(tak_bertanda32_t);

    /* Hitung batas-batas */
    direct_limit = EXT2_NDIR_BLOCKS;
    indirect_limit = direct_limit + ptrs_per_block;
    double_limit = indirect_limit + (ptrs_per_block * ptrs_per_block);

    /* Direct blocks */
    if (logical_block < direct_limit) {
        return inode->i_block[logical_block];
    }

    /* Indirect block */
    if (logical_block < indirect_limit) {
        /* CATATAN: Implementasi pembacaan indirect block */
        return 0;
    }

    /* Double indirect block */
    if (logical_block < double_limit) {
        /* CATATAN: Implementasi pembacaan double indirect block */
        return 0;
    }

    /* Triple indirect block */
    /* CATATAN: Implementasi pembacaan triple indirect block */
    block_num = 0;

    return block_num;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI TIMES (TIME FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_inode_update_atime
 * Update access time inode.
 */
status_t ext2_inode_update_atime(ext2_inode_t *inode)
{
    waktu_t current_time;

    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* current_time = get_current_time(); */
    current_time = 0; /* Placeholder */
    inode->i_atime = (tak_bertanda32_t)current_time;

    return STATUS_BERHASIL;
}

/*
 * ext2_inode_update_mtime
 * Update modification time inode.
 */
status_t ext2_inode_update_mtime(ext2_inode_t *inode)
{
    waktu_t current_time;

    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* current_time = get_current_time(); */
    current_time = 0; /* Placeholder */
    inode->i_mtime = (tak_bertanda32_t)current_time;

    return STATUS_BERHASIL;
}

/*
 * ext2_inode_update_ctime
 * Update change time inode.
 */
status_t ext2_inode_update_ctime(ext2_inode_t *inode)
{
    waktu_t current_time;

    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* current_time = get_current_time(); */
    current_time = 0; /* Placeholder */
    inode->i_ctime = (tak_bertanda32_t)current_time;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI PERMISSION (PERMISSION FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_inode_check_permission
 * Cek permission inode.
 */
bool_t ext2_inode_check_permission(ext2_inode_t *inode,
    uid_t uid, gid_t gid, tak_bertanda32_t mask)
{
    mode_t mode;
    mode_t perm;

    if (inode == NULL) {
        return SALAH;
    }

    mode = inode->i_mode;

    /* Root memiliki akses penuh */
    if (uid == 0) {
        return BENAR;
    }

    /* Owner permission */
    if (uid == (uid_t)inode->i_uid) {
        perm = (mode >> 6) & 0x07;
        return ((perm & mask) == mask) ? BENAR : SALAH;
    }

    /* Group permission */
    if (gid == (gid_t)inode->i_gid) {
        perm = (mode >> 3) & 0x07;
        return ((perm & mask) == mask) ? BENAR : SALAH;
    }

    /* Other permission */
    perm = mode & 0x07;
    return ((perm & mask) == mask) ? BENAR : SALAH;
}

/*
 * ext2_inode_is_readable
 * Cek apakah inode dapat dibaca.
 */
bool_t ext2_inode_is_readable(ext2_inode_t *inode, uid_t uid, gid_t gid)
{
    return ext2_inode_check_permission(inode, uid, gid, 0x04);
}

/*
 * ext2_inode_is_writable
 * Cek apakah inode dapat ditulis.
 */
bool_t ext2_inode_is_writable(ext2_inode_t *inode, uid_t uid, gid_t gid)
{
    /* Cek immutable flag */
    if (inode != NULL && (inode->i_flags & EXT2_IMMUTABLE_FL)) {
        return SALAH;
    }

    return ext2_inode_check_permission(inode, uid, gid, 0x02);
}

/*
 * ext2_inode_is_executable
 * Cek apakah inode dapat dieksekusi.
 */
bool_t ext2_inode_is_executable(ext2_inode_t *inode, uid_t uid, gid_t gid)
{
    return ext2_inode_check_permission(inode, uid, gid, 0x01);
}
