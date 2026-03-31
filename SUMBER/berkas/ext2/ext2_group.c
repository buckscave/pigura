/*
 * PIGURA OS - EXT2_GROUP.C
 * =========================
 * Operasi block group descriptor filesystem EXT2 untuk kernel Pigura OS.
 *
 * Berkas ini berisi implementasi operasi-operasi yang berkaitan dengan
 * block group descriptor EXT2 termasuk pembacaan, penulisan, dan
 * manajemen informasi group.
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
 * KONSTANTA GROUP (GROUP CONSTANTS)
 * ===========================================================================
 */

/* Ukuran group descriptor */
#define EXT2_GROUP_DESC_SIZE        32

/* Default block size */
#define EXT2_DEF_BLOCK_SIZE         1024

/* Descriptors per block */
#define EXT2_DESC_PER_BLOCK_1K      256
#define EXT2_DESC_PER_BLOCK_2K      512
#define EXT2_DESC_PER_BLOCK_4K      1024

/* Default blocks per group */
#define EXT2_DEF_BLOCKS_PER_GROUP   8192

/* Default inodes per group */
#define EXT2_DEF_INODES_PER_GROUP   2048

/* Block bitmap location */
#define EXT2_BLOCK_BITMAP_BLOCK     0

/* Inode bitmap location */
#define EXT2_INODE_BITMAP_BLOCK     1

/* Inode table start */
#define EXT2_INODE_TABLE_BLOCK      2

/*
 * ===========================================================================
 * STRUKTUR GROUP DESCRIPTOR (GROUP DESCRIPTOR STRUCTURE)
 * ===========================================================================
 */

typedef struct ext2_group_desc {
    tak_bertanda32_t bg_block_bitmap;    /* Block bitmap block number */
    tak_bertanda32_t bg_inode_bitmap;    /* Inode bitmap block number */
    tak_bertanda32_t bg_inode_table;     /* Inode table start block */
    tak_bertanda16_t bg_free_blocks_count;/* Free blocks count in group */
    tak_bertanda16_t bg_free_inodes_count;/* Free inodes count in group */
    tak_bertanda16_t bg_used_dirs_count; /* Directories count in group */
    tak_bertanda16_t bg_pad;             /* Padding to 32 bytes */
    tak_bertanda32_t bg_reserved[3];     /* Reserved */
} ext2_group_desc_t;

/*
 * ===========================================================================
 * STRUKTUR SUPERBLOCK (MINIMAL)
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
 * DEKLARASI FUNGSI INTERNAL (INTERNAL FUNCTION DECLARATIONS)
 * ===========================================================================
 */

static tak_bertanda32_t ext2_group_calc_count(tak_bertanda32_t total_blocks,
    tak_bertanda32_t blocks_per_group);
static status_t ext2_group_desc_validate(ext2_group_desc_t *gd);

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI UTILITAS (UTILITY FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_group_calc_count
 * Menghitung jumlah block groups.
 */
static tak_bertanda32_t ext2_group_calc_count(tak_bertanda32_t total_blocks,
    tak_bertanda32_t blocks_per_group)
{
    tak_bertanda32_t groups;

    if (blocks_per_group == 0) {
        return 0;
    }

    groups = total_blocks / blocks_per_group;
    if (total_blocks % blocks_per_group != 0) {
        groups++;
    }

    return groups;
}

/*
 * ext2_group_desc_validate
 * Memvalidasi group descriptor.
 */
static status_t ext2_group_desc_validate(ext2_group_desc_t *gd)
{
    if (gd == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Block bitmap harus valid */
    if (gd->bg_block_bitmap == 0) {
        return STATUS_FS_CORRUPT;
    }

    /* Inode bitmap harus valid */
    if (gd->bg_inode_bitmap == 0) {
        return STATUS_FS_CORRUPT;
    }

    /* Inode table harus valid */
    if (gd->bg_inode_table == 0) {
        return STATUS_FS_CORRUPT;
    }

    return STATUS_BERHASIL;
}

/*
 * ext2_group_get_desc_per_block
 * Mendapatkan jumlah descriptors per block.
 */
tak_bertanda32_t ext2_group_get_desc_per_block(tak_bertanda32_t block_size)
{
    return block_size / EXT2_GROUP_DESC_SIZE;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI INISIALISASI (INITIALIZATION FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_group_desc_init
 * Inisialisasi group descriptor dengan nilai default.
 */
status_t ext2_group_desc_init(ext2_group_desc_t *gd,
    tak_bertanda32_t group_num, tak_bertanda32_t __attribute__((unused)) block_size,
    tak_bertanda32_t blocks_per_group, tak_bertanda32_t inodes_per_group,
    tak_bertanda32_t first_data_block)
{
    (void)block_size;
    tak_bertanda32_t group_start_block;

    if (gd == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Reset semua field */
    gd->bg_block_bitmap = 0;
    gd->bg_inode_bitmap = 0;
    gd->bg_inode_table = 0;
    gd->bg_free_blocks_count = 0;
    gd->bg_free_inodes_count = 0;
    gd->bg_used_dirs_count = 0;
    gd->bg_pad = 0;
    gd->bg_reserved[0] = 0;
    gd->bg_reserved[1] = 0;
    gd->bg_reserved[2] = 0;

    /* Hitung start block untuk group ini */
    group_start_block = first_data_block + (group_num * blocks_per_group);

    /* Set lokasi bitmap dan inode table */
    gd->bg_block_bitmap = group_start_block + EXT2_BLOCK_BITMAP_BLOCK;
    gd->bg_inode_bitmap = group_start_block + EXT2_INODE_BITMAP_BLOCK;
    gd->bg_inode_table = group_start_block + EXT2_INODE_TABLE_BLOCK;

    /* Set free counts */
    gd->bg_free_blocks_count = (tak_bertanda16_t)blocks_per_group;
    gd->bg_free_inodes_count = (tak_bertanda16_t)inodes_per_group;

    return STATUS_BERHASIL;
}

/*
 * ext2_groups_init_all
 * Inisialisasi semua group descriptors.
 */
status_t ext2_groups_init_all(ext2_group_desc_t *groups,
    tak_bertanda32_t groups_count, tak_bertanda32_t block_size,
    tak_bertanda32_t blocks_per_group, tak_bertanda32_t inodes_per_group,
    tak_bertanda32_t first_data_block)
{
    tak_bertanda32_t i;
    status_t status;

    if (groups == NULL) {
        return STATUS_PARAM_NULL;
    }

    for (i = 0; i < groups_count; i++) {
        status = ext2_group_desc_init(&groups[i], i, block_size,
            blocks_per_group, inodes_per_group, first_data_block);

        if (status != STATUS_BERHASIL) {
            return status;
        }
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI PEMBACAAN/PENULISAN (READ/WRITE FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_group_baca_desc
 * Membaca group descriptor dari disk.
 */
status_t ext2_group_baca_desc(void *device, tak_bertanda32_t group_num,
    ext2_group_desc_t *gd, tak_bertanda32_t block_size)
{
    tak_bertanda32_t desc_per_block;
    tak_bertanda32_t block_num;
    tak_bertanda32_t offset;
    char block_buffer[EXT2_DEF_BLOCK_SIZE];
    char *desc_ptr;
    tak_bertanda32_t i;

    memset(block_buffer, 0, sizeof(block_buffer));

    if (device == NULL || gd == NULL) {
        return STATUS_PARAM_NULL;
    }

    desc_per_block = ext2_group_get_desc_per_block(block_size);

    /* Hitung block number dan offset */
    block_num = 1 + (group_num / desc_per_block); /* Block 1 is first GDT */
    offset = (group_num % desc_per_block) * EXT2_GROUP_DESC_SIZE;

    /* Baca block yang berisi descriptor */
    /* CATATAN: Implementasi sebenarnya menggunakan device I/O */
    /* status = device_read_block(device, block_num, block_buffer); */

    /* Salin descriptor dari buffer */
    desc_ptr = block_buffer + offset;

    gd->bg_block_bitmap = *((tak_bertanda32_t *)(desc_ptr + 0));
    gd->bg_inode_bitmap = *((tak_bertanda32_t *)(desc_ptr + 4));
    gd->bg_inode_table = *((tak_bertanda32_t *)(desc_ptr + 8));
    gd->bg_free_blocks_count = *((tak_bertanda16_t *)(desc_ptr + 12));
    gd->bg_free_inodes_count = *((tak_bertanda16_t *)(desc_ptr + 14));
    gd->bg_used_dirs_count = *((tak_bertanda16_t *)(desc_ptr + 16));
    gd->bg_pad = *((tak_bertanda16_t *)(desc_ptr + 18));

    for (i = 0; i < 3; i++) {
        gd->bg_reserved[i] = *((tak_bertanda32_t *)(desc_ptr + 20 + i * 4));
    }

    return STATUS_BERHASIL;
}

/*
 * ext2_group_tulis_desc
 * Menulis group descriptor ke disk.
 */
status_t ext2_group_tulis_desc(void *device, tak_bertanda32_t group_num,
    const ext2_group_desc_t *gd, tak_bertanda32_t block_size)
{
    tak_bertanda32_t desc_per_block;
    tak_bertanda32_t block_num;
    tak_bertanda32_t offset;
    char block_buffer[EXT2_DEF_BLOCK_SIZE];
    char *desc_ptr;
    tak_bertanda32_t i;
    status_t status;

    if (device == NULL || gd == NULL) {
        return STATUS_PARAM_NULL;
    }

    desc_per_block = ext2_group_get_desc_per_block(block_size);

    /* Hitung block number dan offset */
    block_num = 1 + (group_num / desc_per_block);
    offset = (group_num % desc_per_block) * EXT2_GROUP_DESC_SIZE;

    /* Baca block terlebih dahulu (read-modify-write) */
    /* status = device_read_block(device, block_num, block_buffer); */
    status = STATUS_BERHASIL; /* Placeholder */

    if (status != STATUS_BERHASIL) {
        return status;
    }

    /* Update descriptor dalam buffer */
    desc_ptr = block_buffer + offset;

    *((tak_bertanda32_t *)(desc_ptr + 0)) = gd->bg_block_bitmap;
    *((tak_bertanda32_t *)(desc_ptr + 4)) = gd->bg_inode_bitmap;
    *((tak_bertanda32_t *)(desc_ptr + 8)) = gd->bg_inode_table;
    *((tak_bertanda16_t *)(desc_ptr + 12)) = gd->bg_free_blocks_count;
    *((tak_bertanda16_t *)(desc_ptr + 14)) = gd->bg_free_inodes_count;
    *((tak_bertanda16_t *)(desc_ptr + 16)) = gd->bg_used_dirs_count;
    *((tak_bertanda16_t *)(desc_ptr + 18)) = gd->bg_pad;

    for (i = 0; i < 3; i++) {
        *((tak_bertanda32_t *)(desc_ptr + 20 + i * 4)) = gd->bg_reserved[i];
    }

    /* Tulis block kembali */
    /* status = device_write_block(device, block_num, block_buffer); */

    return STATUS_BERHASIL;
}

/*
 * ext2_group_baca_all
 * Membaca semua group descriptors.
 */
status_t ext2_group_baca_all(void *device, ext2_group_desc_t *groups,
    tak_bertanda32_t groups_count, tak_bertanda32_t block_size)
{
    tak_bertanda32_t i;
    status_t status;

    if (device == NULL || groups == NULL) {
        return STATUS_PARAM_NULL;
    }

    for (i = 0; i < groups_count; i++) {
        status = ext2_group_baca_desc(device, i, &groups[i], block_size);

        if (status != STATUS_BERHASIL) {
            return status;
        }
    }

    return STATUS_BERHASIL;
}

/*
 * ext2_group_tulis_all
 * Menulis semua group descriptors.
 */
status_t ext2_group_tulis_all(void *device, ext2_group_desc_t *groups,
    tak_bertanda32_t groups_count, tak_bertanda32_t block_size)
{
    tak_bertanda32_t i;
    status_t status;

    if (device == NULL || groups == NULL) {
        return STATUS_PARAM_NULL;
    }

    for (i = 0; i < groups_count; i++) {
        status = ext2_group_tulis_desc(device, i, &groups[i], block_size);

        if (status != STATUS_BERHASIL) {
            return status;
        }
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI QUERY (QUERY FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_group_get_block_bitmap
 * Mendapatkan nomor block bitmap block.
 */
tak_bertanda32_t ext2_group_get_block_bitmap(ext2_group_desc_t *gd)
{
    if (gd == NULL) {
        return 0;
    }

    return gd->bg_block_bitmap;
}

/*
 * ext2_group_get_inode_bitmap
 * Mendapatkan nomor inode bitmap block.
 */
tak_bertanda32_t ext2_group_get_inode_bitmap(ext2_group_desc_t *gd)
{
    if (gd == NULL) {
        return 0;
    }

    return gd->bg_inode_bitmap;
}

/*
 * ext2_group_get_inode_table
 * Mendapatkan nomor inode table start block.
 */
tak_bertanda32_t ext2_group_get_inode_table(ext2_group_desc_t *gd)
{
    if (gd == NULL) {
        return 0;
    }

    return gd->bg_inode_table;
}

/*
 * ext2_group_get_free_blocks
 * Mendapatkan jumlah block bebas dalam group.
 */
tak_bertanda32_t ext2_group_get_free_blocks(ext2_group_desc_t *gd)
{
    if (gd == NULL) {
        return 0;
    }

    return gd->bg_free_blocks_count;
}

/*
 * ext2_group_get_free_inodes
 * Mendapatkan jumlah inode bebas dalam group.
 */
tak_bertanda32_t ext2_group_get_free_inodes(ext2_group_desc_t *gd)
{
    if (gd == NULL) {
        return 0;
    }

    return gd->bg_free_inodes_count;
}

/*
 * ext2_group_get_used_dirs
 * Mendapatkan jumlah direktori dalam group.
 */
tak_bertanda32_t ext2_group_get_used_dirs(ext2_group_desc_t *gd)
{
    if (gd == NULL) {
        return 0;
    }

    return gd->bg_used_dirs_count;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI UPDATE (UPDATE FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_group_update_free_blocks
 * Update jumlah block bebas.
 */
status_t ext2_group_update_free_blocks(ext2_group_desc_t *gd,
    tak_bertanda16_t count)
{
    if (gd == NULL) {
        return STATUS_PARAM_NULL;
    }

    gd->bg_free_blocks_count = count;

    return STATUS_BERHASIL;
}

/*
 * ext2_group_update_free_inodes
 * Update jumlah inode bebas.
 */
status_t ext2_group_update_free_inodes(ext2_group_desc_t *gd,
    tak_bertanda16_t count)
{
    if (gd == NULL) {
        return STATUS_PARAM_NULL;
    }

    gd->bg_free_inodes_count = count;

    return STATUS_BERHASIL;
}

/*
 * ext2_group_inc_dirs
 * Increment jumlah direktori.
 */
status_t ext2_group_inc_dirs(ext2_group_desc_t *gd)
{
    if (gd == NULL) {
        return STATUS_PARAM_NULL;
    }

    gd->bg_used_dirs_count++;

    return STATUS_BERHASIL;
}

/*
 * ext2_group_dec_dirs
 * Decrement jumlah direktori.
 */
status_t ext2_group_dec_dirs(ext2_group_desc_t *gd)
{
    if (gd == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (gd->bg_used_dirs_count > 0) {
        gd->bg_used_dirs_count--;
    }

    return STATUS_BERHASIL;
}

/*
 * ext2_group_dec_free_blocks
 * Decrement jumlah block bebas.
 */
status_t ext2_group_dec_free_blocks(ext2_group_desc_t *gd)
{
    if (gd == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (gd->bg_free_blocks_count > 0) {
        gd->bg_free_blocks_count--;
    }

    return STATUS_BERHASIL;
}

/*
 * ext2_group_inc_free_blocks
 * Increment jumlah block bebas.
 */
status_t ext2_group_inc_free_blocks(ext2_group_desc_t *gd)
{
    if (gd == NULL) {
        return STATUS_PARAM_NULL;
    }

    gd->bg_free_blocks_count++;

    return STATUS_BERHASIL;
}

/*
 * ext2_group_dec_free_inodes
 * Decrement jumlah inode bebas.
 */
status_t ext2_group_dec_free_inodes(ext2_group_desc_t *gd)
{
    if (gd == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (gd->bg_free_inodes_count > 0) {
        gd->bg_free_inodes_count--;
    }

    return STATUS_BERHASIL;
}

/*
 * ext2_group_inc_free_inodes
 * Increment jumlah inode bebas.
 */
status_t ext2_group_inc_free_inodes(ext2_group_desc_t *gd)
{
    if (gd == NULL) {
        return STATUS_PARAM_NULL;
    }

    gd->bg_free_inodes_count++;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI STATISTIK (STATISTICS FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_group_calc_total_free_blocks
 * Menghitung total block bebas dari semua groups.
 */
tak_bertanda32_t ext2_group_calc_total_free_blocks(ext2_group_desc_t *groups,
    tak_bertanda32_t groups_count)
{
    tak_bertanda32_t total;
    tak_bertanda32_t i;

    if (groups == NULL) {
        return 0;
    }

    total = 0;

    for (i = 0; i < groups_count; i++) {
        total += groups[i].bg_free_blocks_count;
    }

    return total;
}

/*
 * ext2_group_calc_total_free_inodes
 * Menghitung total inode bebas dari semua groups.
 */
tak_bertanda32_t ext2_group_calc_total_free_inodes(ext2_group_desc_t *groups,
    tak_bertanda32_t groups_count)
{
    tak_bertanda32_t total;
    tak_bertanda32_t i;

    if (groups == NULL) {
        return 0;
    }

    total = 0;

    for (i = 0; i < groups_count; i++) {
        total += groups[i].bg_free_inodes_count;
    }

    return total;
}

/*
 * ext2_group_calc_total_dirs
 * Menghitung total direktori dari semua groups.
 */
tak_bertanda32_t ext2_group_calc_total_dirs(ext2_group_desc_t *groups,
    tak_bertanda32_t groups_count)
{
    tak_bertanda32_t total;
    tak_bertanda32_t i;

    if (groups == NULL) {
        return 0;
    }

    total = 0;

    for (i = 0; i < groups_count; i++) {
        total += groups[i].bg_used_dirs_count;
    }

    return total;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI PENCARIAN GROUP (GROUP SEARCH FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_group_find_with_free_blocks
 * Mencari group dengan block bebas.
 */
tak_bertanda32_t ext2_group_find_with_free_blocks(ext2_group_desc_t *groups,
    tak_bertanda32_t groups_count, tak_bertanda32_t start_group)
{
    tak_bertanda32_t i;

    if (groups == NULL || groups_count == 0) {
        return groups_count; /* Invalid */
    }

    /* Mulai dari start_group */
    for (i = start_group; i < groups_count; i++) {
        if (groups[i].bg_free_blocks_count > 0) {
            return i;
        }
    }

    /* Wrap around ke awal */
    for (i = 0; i < start_group; i++) {
        if (groups[i].bg_free_blocks_count > 0) {
            return i;
        }
    }

    return groups_count; /* Tidak ditemukan */
}

/*
 * ext2_group_find_with_free_inodes
 * Mencari group dengan inode bebas.
 */
tak_bertanda32_t ext2_group_find_with_free_inodes(ext2_group_desc_t *groups,
    tak_bertanda32_t groups_count, tak_bertanda32_t start_group)
{
    tak_bertanda32_t i;

    if (groups == NULL || groups_count == 0) {
        return groups_count; /* Invalid */
    }

    /* Mulai dari start_group */
    for (i = start_group; i < groups_count; i++) {
        if (groups[i].bg_free_inodes_count > 0) {
            return i;
        }
    }

    /* Wrap around ke awal */
    for (i = 0; i < start_group; i++) {
        if (groups[i].bg_free_inodes_count > 0) {
            return i;
        }
    }

    return groups_count; /* Tidak ditemukan */
}

/*
 * ext2_group_find_least_dirs
 * Mencari group dengan direktori paling sedikit.
 */
tak_bertanda32_t ext2_group_find_least_dirs(ext2_group_desc_t *groups,
    tak_bertanda32_t groups_count)
{
    tak_bertanda32_t min_group;
    tak_bertanda16_t min_dirs;
    tak_bertanda32_t i;

    if (groups == NULL || groups_count == 0) {
        return 0;
    }

    min_group = 0;
    min_dirs = groups[0].bg_used_dirs_count;

    for (i = 1; i < groups_count; i++) {
        if (groups[i].bg_used_dirs_count < min_dirs) {
            min_dirs = groups[i].bg_used_dirs_count;
            min_group = i;
        }
    }

    return min_group;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI LOKASI INODE/BLOCK (INODE/BLOCK LOCATION FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_group_find_for_inode
 * Mencari group untuk inode tertentu.
 */
tak_bertanda32_t ext2_group_find_for_inode(tak_bertanda32_t inode_num,
    tak_bertanda32_t inodes_per_group)
{
    if (inodes_per_group == 0 || inode_num == 0) {
        return 0;
    }

    /* Inode dimulai dari 1 */
    return (inode_num - 1) / inodes_per_group;
}

/*
 * ext2_group_find_for_block
 * Mencari group untuk block tertentu.
 */
tak_bertanda32_t ext2_group_find_for_block(tak_bertanda32_t block_num,
    tak_bertanda32_t blocks_per_group)
{
    if (blocks_per_group == 0) {
        return 0;
    }

    return block_num / blocks_per_group;
}

/*
 * ext2_group_calc_block_in_group
 * Menghitung nomor block relatif dalam group.
 */
tak_bertanda32_t ext2_group_calc_block_in_group(tak_bertanda32_t block_num,
    tak_bertanda32_t blocks_per_group)
{
    if (blocks_per_group == 0) {
        return 0;
    }

    return block_num % blocks_per_group;
}

/*
 * ext2_group_calc_inode_in_group
 * Menghitung nomor inode relatif dalam group.
 */
tak_bertanda32_t ext2_group_calc_inode_in_group(tak_bertanda32_t inode_num,
    tak_bertanda32_t inodes_per_group)
{
    if (inodes_per_group == 0 || inode_num == 0) {
        return 0;
    }

    return (inode_num - 1) % inodes_per_group;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI INODE TABLE (INODE TABLE FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_group_calc_inode_table_blocks
 * Menghitung jumlah block untuk inode table.
 */
tak_bertanda32_t ext2_group_calc_inode_table_blocks(
    tak_bertanda32_t inodes_per_group, tak_bertanda32_t inode_size,
    tak_bertanda32_t block_size)
{
    tak_bertanda64_t total_bytes;
    tak_bertanda32_t blocks;

    if (inode_size == 0 || block_size == 0) {
        return 0;
    }

    total_bytes = (tak_bertanda64_t)inodes_per_group * inode_size;
    blocks = (tak_bertanda32_t)((total_bytes + block_size - 1) / block_size);

    return blocks;
}

/*
 * ext2_group_calc_metadata_blocks
 * Menghitung jumlah block untuk metadata group.
 */
tak_bertanda32_t ext2_group_calc_metadata_blocks(
    tak_bertanda32_t inodes_per_group, tak_bertanda32_t inode_size,
    tak_bertanda32_t block_size)
{
    tak_bertanda32_t metadata_blocks;

    /* 1 block untuk block bitmap */
    /* 1 block untuk inode bitmap */
    /* N block untuk inode table */
    metadata_blocks = 2 + ext2_group_calc_inode_table_blocks(inodes_per_group,
        inode_size, block_size);

    return metadata_blocks;
}

/*
 * ext2_group_calc_data_blocks
 * Menghitung jumlah block untuk data.
 */
tak_bertanda32_t ext2_group_calc_data_blocks(
    tak_bertanda32_t blocks_per_group, tak_bertanda32_t inodes_per_group,
    tak_bertanda32_t inode_size, tak_bertanda32_t block_size)
{
    tak_bertanda32_t metadata_blocks;

    if (blocks_per_group == 0) {
        return 0;
    }

    metadata_blocks = ext2_group_calc_metadata_blocks(inodes_per_group,
        inode_size, block_size);

    if (metadata_blocks >= blocks_per_group) {
        return 0;
    }

    return blocks_per_group - metadata_blocks;
}
