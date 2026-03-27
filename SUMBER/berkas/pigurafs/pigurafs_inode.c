/*
 * PIGURA OS - PIGURAFS_INODE.C
 * =============================
 * Implementasi operasi inode untuk filesystem PiguraFS.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola inode
 * PiguraFS termasuk alokasi, pembacaan, dan penulisan.
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
 * KONSTANTA INODE (INODE CONSTANTS)
 * ===========================================================================
 */

/* Magic number */
#define PFS_INODE_MAGIC         0x50465332

/* Ukuran inode on-disk */
#define PFS_INODE_SIZE          256

/* Jumlah extent per inode */
#define PFS_EXTENT_PER_INODE    4

/* Flag inode */
#define PFS_INODE_FLAG_SYNC     0x0001
#define PFS_INODE_FLAG_IMMUTABLE 0x0002
#define PFS_INODE_FLAG_APPEND   0x0004
#define PFS_INODE_FLAG_NODUMP   0x0008

/* Tipe file */
#define PFS_S_IFMT              0xF000
#define PFS_S_IFREG             0x8000
#define PFS_S_IFDIR             0x4000
#define PFS_S_IFLNK             0xA000

/* Reserved inodes */
#define PFS_ROOT_INO            2

/*
 * ===========================================================================
 * STRUKTUR DATA INODE (INODE DATA STRUCTURES)
 * ===========================================================================
 */

/* Struktur inode on-disk */
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
    tak_bertanda8_t i_extent[48];       /* Extent data */
    tak_bertanda8_t i_reserved[136];    /* Reserved */
} pfs_inode_disk_t;

/* Struktur extent */
typedef struct PACKED {
    tak_bertanda32_t e_block;           /* Block logical */
    tak_bertanda32_t e_start;           /* Block fisik awal */
    tak_bertanda16_t e_len;             /* Panjang extent */
    tak_bertanda16_t e_flags;           /* Flag extent */
} pfs_extent_t;

/* Struktur in-memory inode info */
typedef struct {
    tak_bertanda32_t pfs_magic;         /* Magic number validasi */
    inode_t *vfs_inode;                 /* VFS inode */
    pfs_inode_disk_t *disk_inode;       /* On-disk inode */
    tak_bertanda32_t ino;               /* Inode number */
    tak_bertanda64_t size;              /* Ukuran file */
    tak_bertanda32_t blocks;            /* Block count */
    tak_bertanda32_t extents_count;     /* Jumlah extent */
    pfs_extent_t extents[PFS_EXTENT_PER_INODE]; /* Extent array */
    tak_bertanda32_t flags;             /* Inode flags */
    tak_bertanda32_t generation;        /* Generation number */
    bool_t dirty;                       /* Perlu write? */
} pfs_inode_info_t;

/* Struktur superblock info (forward) */
typedef struct {
    tak_bertanda32_t pfs_magic;
    superblock_t *vfs_sb;
    void *disk_sb;
    tak_bertanda32_t block_size;
    tak_bertanda32_t block_size_bits;
    tak_bertanda64_t blocks_count;
    tak_bertanda64_t blocks_free;
    tak_bertanda64_t inodes_count;
    tak_bertanda64_t inodes_free;
    tak_bertanda32_t first_block;
    tak_bertanda32_t first_ino;
    tak_bertanda32_t inodes_per_group;
    tak_bertanda32_t blocks_per_group;
    tak_bertanda32_t group_count;
    void *group_desc;
    bool_t dirty;
    void *device;
} pfs_sb_info_t;

/* Fungsi dari pigurafs_super.c */
extern tak_bertanda32_t pfs_super_alloc_block(pfs_sb_info_t *sbi);
extern void pfs_super_free_block(pfs_sb_info_t *sbi, tak_bertanda32_t block);
extern tak_bertanda32_t pfs_super_alloc_inode(pfs_sb_info_t *sbi);
extern void pfs_super_free_inode(pfs_sb_info_t *sbi, tak_bertanda32_t ino);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INTERNAL (INTERNAL FUNCTION DECLARATIONS)
 * ===========================================================================
 */

static status_t pfs_inode_baca_dari_disk(pfs_inode_info_t *ii,
                                         pfs_sb_info_t *sbi,
                                         tak_bertanda32_t ino);
static status_t pfs_inode_tulis_ke_disk(pfs_inode_info_t *ii,
                                        pfs_sb_info_t *sbi);
static void pfs_inode_disk_to_mem(const pfs_inode_disk_t *disk,
                                  pfs_inode_info_t *ii);
static void pfs_inode_mem_to_disk(const pfs_inode_info_t *ii,
                                  pfs_inode_disk_t *disk);
static tak_bertanda64_t pfs_inode_calc_offset(pfs_sb_info_t *sbi,
                                              tak_bertanda32_t ino);

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI KONVERSI (CONVERSION FUNCTIONS)
 * ===========================================================================
 */

static void pfs_inode_disk_to_mem(const pfs_inode_disk_t *disk,
                                  pfs_inode_info_t *ii)
{
    ukuran_t i;

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

    /* Copy extents */
    for (i = 0; i < ii->extents_count && i < PFS_EXTENT_PER_INODE; i++) {
        const pfs_extent_t *src = (const pfs_extent_t *)
                                  &disk->i_extent[i * sizeof(pfs_extent_t)];
        ii->extents[i].e_block = src->e_block;
        ii->extents[i].e_start = src->e_start;
        ii->extents[i].e_len = src->e_len;
        ii->extents[i].e_flags = src->e_flags;
    }
}

static void pfs_inode_mem_to_disk(const pfs_inode_info_t *ii,
                                  pfs_inode_disk_t *disk)
{
    ukuran_t i;

    if (ii == NULL || disk == NULL) {
        return;
    }

    kernel_memset(disk, 0, sizeof(pfs_inode_disk_t));

    disk->i_mode = (tak_bertanda16_t)(ii->vfs_inode->i_mode & 0xFFFF);
    disk->i_uid = (tak_bertanda16_t)(ii->vfs_inode->i_uid & 0xFFFF);
    disk->i_gid = (tak_bertanda16_t)(ii->vfs_inode->i_gid & 0xFFFF);
    disk->i_size_lo = (tak_bertanda32_t)(ii->size & 0xFFFFFFFF);
    disk->i_size_hi = (tak_bertanda32_t)(ii->size >> 32);
    disk->i_atime = (tak_bertanda32_t)ii->vfs_inode->i_atime;
    disk->i_ctime = (tak_bertanda32_t)ii->vfs_inode->i_ctime;
    disk->i_mtime = (tak_bertanda32_t)ii->vfs_inode->i_mtime;
    disk->i_links_count = (tak_bertanda16_t)ii->vfs_inode->i_nlink;
    disk->i_blocks = ii->blocks;
    disk->i_flags = ii->flags;
    disk->i_generation = ii->generation;
    disk->i_extents_count = ii->extents_count;

    /* Copy extents */
    for (i = 0; i < ii->extents_count && i < PFS_EXTENT_PER_INODE; i++) {
        pfs_extent_t *dst = (pfs_extent_t *)
                            &disk->i_extent[i * sizeof(pfs_extent_t)];
        dst->e_block = ii->extents[i].e_block;
        dst->e_start = ii->extents[i].e_start;
        dst->e_len = ii->extents[i].e_len;
        dst->e_flags = ii->extents[i].e_flags;
    }
}

static tak_bertanda64_t pfs_inode_calc_offset(pfs_sb_info_t *sbi,
                                              tak_bertanda32_t ino)
{
    tak_bertanda64_t offset;
    tak_bertanda32_t group;
    tak_bertanda32_t index;

    if (sbi == NULL) {
        return 0;
    }

    /* Hitung group dan index dalam group */
    group = (ino - sbi->first_ino) / sbi->inodes_per_group;
    index = (ino - sbi->first_ino) % sbi->inodes_per_group;

    /* Hitung offset dalam byte */
    offset = (tak_bertanda64_t)sbi->first_block * sbi->block_size;
    offset += (tak_bertanda64_t)group * sbi->blocks_per_group * sbi->block_size;
    offset += (tak_bertanda64_t)index * PFS_INODE_SIZE;

    return offset;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI I/O (I/O FUNCTIONS)
 * ===========================================================================
 */

static status_t pfs_inode_baca_dari_disk(pfs_inode_info_t *ii,
                                         pfs_sb_info_t *sbi,
                                         tak_bertanda32_t ino)
{
    pfs_inode_disk_t *disk;
    tak_bertanda64_t offset;
    tak_bertanda8_t buffer[PFS_INODE_SIZE];

    if (ii == NULL || sbi == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Alokasi disk inode buffer */
    disk = (pfs_inode_disk_t *)kmalloc(sizeof(pfs_inode_disk_t));
    if (disk == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    /* Hitung offset inode */
    offset = pfs_inode_calc_offset(sbi, ino);

    /* TODO: Baca dari device */
    kernel_memset(buffer, 0, PFS_INODE_SIZE);

    /* Init default untuk root inode */
    if (ino == PFS_ROOT_INO) {
        disk->i_mode = (tak_bertanda16_t)(PFS_S_IFDIR | 0755);
        disk->i_uid = 0;
        disk->i_gid = 0;
        disk->i_size_lo = sbi->block_size;
        disk->i_size_hi = 0;
        disk->i_atime = 0;
        disk->i_ctime = 0;
        disk->i_mtime = 0;
        disk->i_links_count = 2;
        disk->i_blocks = 1;
        disk->i_flags = 0;
        disk->i_generation = 1;
        disk->i_extents_count = 1;
    } else {
        kernel_memset(disk, 0, sizeof(pfs_inode_disk_t));
    }

    ii->disk_inode = disk;
    ii->ino = ino;
    ii->vfs_inode->i_ino = (ino_t)ino;
    pfs_inode_disk_to_mem(disk, ii);

    return STATUS_BERHASIL;
}

static status_t pfs_inode_tulis_ke_disk(pfs_inode_info_t *ii,
                                        pfs_sb_info_t *sbi)
{
    if (ii == NULL || sbi == NULL || ii->disk_inode == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Update disk inode */
    pfs_inode_mem_to_disk(ii, ii->disk_inode);

    /* TODO: Tulis ke device */

    ii->dirty = SALAH;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI UTAMA (MAIN FUNCTIONS)
 * ===========================================================================
 */

/*
 * pfs_inode_alloc
 * ---------------
 * Alokasi inode baru.
 *
 * Parameter:
 *   sb - VFS superblock
 *
 * Return: Pointer ke inode, atau NULL jika gagal
 */
inode_t *pfs_inode_alloc(superblock_t *sb)
{
    pfs_sb_info_t *sbi;
    pfs_inode_info_t *ii;
    inode_t *inode;
    tak_bertanda32_t ino;

    if (sb == NULL) {
        return NULL;
    }

    sbi = (pfs_sb_info_t *)sb->s_private;
    if (sbi == NULL || sbi->pfs_magic != 0x50465331) {
        return NULL;
    }

    /* Alokasi inode number */
    ino = pfs_super_alloc_inode(sbi);
    if (ino == 0) {
        return NULL;
    }

    /* Alokasi VFS inode */
    inode = inode_alloc(sb);
    if (inode == NULL) {
        pfs_super_free_inode(sbi, ino);
        return NULL;
    }

    /* Alokasi private info */
    ii = (pfs_inode_info_t *)kmalloc(sizeof(pfs_inode_info_t));
    if (ii == NULL) {
        inode_put(inode);
        pfs_super_free_inode(sbi, ino);
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

/*
 * pfs_inode_free
 * --------------
 * Bebaskan inode.
 *
 * Parameter:
 *   inode - Pointer ke inode
 */
void pfs_inode_free(inode_t *inode)
{
    pfs_inode_info_t *ii;
    pfs_sb_info_t *sbi;

    if (inode == NULL) {
        return;
    }

    ii = (pfs_inode_info_t *)inode->i_private;
    if (ii == NULL || ii->pfs_magic != PFS_INODE_MAGIC) {
        return;
    }

    sbi = (pfs_sb_info_t *)inode->i_sb->s_private;

    /* Free inode number */
    if (sbi != NULL) {
        pfs_super_free_inode(sbi, ii->ino);
    }

    /* Free disk inode */
    if (ii->disk_inode != NULL) {
        kfree(ii->disk_inode);
    }

    ii->pfs_magic = 0;
    kfree(ii);
    inode->i_private = NULL;
}

/*
 * pfs_inode_read
 * --------------
 * Baca inode dari disk.
 *
 * Parameter:
 *   inode - Pointer ke inode
 *
 * Return: Status operasi
 */
status_t pfs_inode_read(inode_t *inode)
{
    pfs_sb_info_t *sbi;
    pfs_inode_info_t *ii;

    if (inode == NULL || inode->i_sb == NULL) {
        return STATUS_PARAM_NULL;
    }

    sbi = (pfs_sb_info_t *)inode->i_sb->s_private;
    if (sbi == NULL || sbi->pfs_magic != 0x50465331) {
        return STATUS_PARAM_INVALID;
    }

    ii = (pfs_inode_info_t *)inode->i_private;
    if (ii == NULL || ii->pfs_magic != PFS_INODE_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    return pfs_inode_baca_dari_disk(ii, sbi, ii->ino);
}

/*
 * pfs_inode_write
 * ---------------
 * Tulis inode ke disk.
 *
 * Parameter:
 *   inode - Pointer ke inode
 *
 * Return: Status operasi
 */
status_t pfs_inode_write(inode_t *inode)
{
    pfs_sb_info_t *sbi;
    pfs_inode_info_t *ii;

    if (inode == NULL || inode->i_sb == NULL) {
        return STATUS_PARAM_NULL;
    }

    sbi = (pfs_sb_info_t *)inode->i_sb->s_private;
    if (sbi == NULL || sbi->pfs_magic != 0x50465331) {
        return STATUS_PARAM_INVALID;
    }

    ii = (pfs_inode_info_t *)inode->i_private;
    if (ii == NULL || ii->pfs_magic != PFS_INODE_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    return pfs_inode_tulis_ke_disk(ii, sbi);
}

/*
 * pfs_inode_mark_dirty
 * --------------------
 * Tandai inode sebagai dirty.
 *
 * Parameter:
 *   inode - Pointer ke inode
 */
void pfs_inode_mark_dirty(inode_t *inode)
{
    pfs_inode_info_t *ii;

    if (inode == NULL) {
        return;
    }

    ii = (pfs_inode_info_t *)inode->i_private;
    if (ii == NULL || ii->pfs_magic != PFS_INODE_MAGIC) {
        return;
    }

    ii->dirty = BENAR;
    inode->i_dirty = BENAR;
}

/*
 * pfs_inode_sync
 * --------------
 * Sinkronkan inode ke disk.
 *
 * Parameter:
 *   inode - Pointer ke inode
 *
 * Return: Status operasi
 */
status_t pfs_inode_sync(inode_t *inode)
{
    pfs_inode_info_t *ii;

    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }

    ii = (pfs_inode_info_t *)inode->i_private;
    if (ii == NULL || ii->pfs_magic != PFS_INODE_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    if (!ii->dirty) {
        return STATUS_BERHASIL;
    }

    return pfs_inode_write(inode);
}

/*
 * pfs_inode_truncate
 * ------------------
 * Truncate inode ke ukuran tertentu.
 *
 * Parameter:
 *   inode    - Pointer ke inode
 *   new_size - Ukuran baru
 *
 * Return: Status operasi
 */
status_t pfs_inode_truncate(inode_t *inode, tak_bertanda64_t new_size)
{
    pfs_inode_info_t *ii;
    pfs_sb_info_t *sbi;
    tak_bertanda64_t old_size;
    tak_bertanda32_t old_blocks;
    tak_bertanda32_t new_blocks;
    tak_bertanda32_t i;

    if (inode == NULL || inode->i_sb == NULL) {
        return STATUS_PARAM_NULL;
    }

    sbi = (pfs_sb_info_t *)inode->i_sb->s_private;
    if (sbi == NULL) {
        return STATUS_PARAM_INVALID;
    }

    ii = (pfs_inode_info_t *)inode->i_private;
    if (ii == NULL || ii->pfs_magic != PFS_INODE_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    old_size = ii->size;
    old_blocks = (tak_bertanda32_t)((old_size + sbi->block_size - 1) /
                                    sbi->block_size);
    new_blocks = (tak_bertanda32_t)((new_size + sbi->block_size - 1) /
                                    sbi->block_size);

    /* Jika mengecil, free blocks yang tidak diperlukan */
    if (new_blocks < old_blocks) {
        tak_bertanda32_t blocks_to_free = old_blocks - new_blocks;

        /* Free extent blocks */
        for (i = 0; i < ii->extents_count && blocks_to_free > 0; i++) {
            pfs_extent_t *ext = &ii->extents[i];
            if (ext->e_len > 0) {
                tak_bertanda32_t j;
                for (j = 0; j < ext->e_len && blocks_to_free > 0; j++) {
                    if (ext->e_block + j >= new_blocks) {
                        pfs_super_free_block(sbi, ext->e_start + j);
                        blocks_to_free--;
                    }
                }
            }
        }
    }

    /* Update size */
    ii->size = new_size;
    inode->i_size = (off_t)new_size;
    ii->blocks = new_blocks;
    inode->i_blocks = (tak_bertanda64_t)new_blocks;
    ii->dirty = BENAR;

    return STATUS_BERHASIL;
}

/*
 * pfs_inode_add_extent
 * --------------------
 * Tambah extent ke inode.
 *
 * Parameter:
 *   inode      - Pointer ke inode
 *   log_block  - Block logical
 *   phys_block - Block fisik
 *   len        - Panjang extent
 *
 * Return: Status operasi
 */
status_t pfs_inode_add_extent(inode_t *inode, tak_bertanda32_t log_block,
                              tak_bertanda32_t phys_block,
                              tak_bertanda16_t len)
{
    pfs_inode_info_t *ii;

    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }

    ii = (pfs_inode_info_t *)inode->i_private;
    if (ii == NULL || ii->pfs_magic != PFS_INODE_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    if (ii->extents_count >= PFS_EXTENT_PER_INODE) {
        return STATUS_PENUH;
    }

    /* Tambah extent baru */
    ii->extents[ii->extents_count].e_block = log_block;
    ii->extents[ii->extents_count].e_start = phys_block;
    ii->extents[ii->extents_count].e_len = len;
    ii->extents[ii->extents_count].e_flags = 0;
    ii->extents_count++;
    ii->dirty = BENAR;

    return STATUS_BERHASIL;
}

/*
 * pfs_inode_find_block
 * --------------------
 * Cari block fisik dari block logical.
 *
 * Parameter:
 *   inode     - Pointer ke inode
 *   log_block - Block logical
 *   phys_block - Pointer ke hasil block fisik
 *
 * Return: Status operasi
 */
status_t pfs_inode_find_block(inode_t *inode, tak_bertanda32_t log_block,
                              tak_bertanda32_t *phys_block)
{
    pfs_inode_info_t *ii;
    ukuran_t i;

    if (inode == NULL || phys_block == NULL) {
        return STATUS_PARAM_NULL;
    }

    ii = (pfs_inode_info_t *)inode->i_private;
    if (ii == NULL || ii->pfs_magic != PFS_INODE_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    *phys_block = 0;

    /* Cari dalam extent array */
    for (i = 0; i < ii->extents_count; i++) {
        pfs_extent_t *ext = &ii->extents[i];
        if (log_block >= ext->e_block &&
            log_block < ext->e_block + ext->e_len) {
            *phys_block = ext->e_start + (log_block - ext->e_block);
            return STATUS_BERHASIL;
        }
    }

    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * pfs_inode_update_time
 * ---------------------
 * Update waktu inode.
 *
 * Parameter:
 *   inode - Pointer ke inode
 *   flags - Flag waktu (ATIME, MTIME, CTIME)
 */
void pfs_inode_update_time(inode_t *inode, tak_bertanda32_t flags)
{
    pfs_inode_info_t *ii;
    waktu_t now;

    if (inode == NULL) {
        return;
    }

    ii = (pfs_inode_info_t *)inode->i_private;
    if (ii == NULL || ii->pfs_magic != PFS_INODE_MAGIC) {
        return;
    }

    /* TODO: Dapatkan waktu aktual */
    now = 0;

    if (flags & 0x01) { /* ATIME */
        inode->i_atime = now;
    }
    if (flags & 0x02) { /* MTIME */
        inode->i_mtime = now;
    }
    if (flags & 0x04) { /* CTIME */
        inode->i_ctime = now;
    }

    ii->dirty = BENAR;
}

/*
 * pfs_inode_inc_link
 * ------------------
 * Increment link count inode.
 *
 * Parameter:
 *   inode - Pointer ke inode
 */
void pfs_inode_inc_link(inode_t *inode)
{
    pfs_inode_info_t *ii;

    if (inode == NULL) {
        return;
    }

    ii = (pfs_inode_info_t *)inode->i_private;
    if (ii == NULL || ii->pfs_magic != PFS_INODE_MAGIC) {
        return;
    }

    inode->i_nlink++;
    ii->dirty = BENAR;
}

/*
 * pfs_inode_dec_link
 * ------------------
 * Decrement link count inode.
 *
 * Parameter:
 *   inode - Pointer ke inode
 */
void pfs_inode_dec_link(inode_t *inode)
{
    pfs_inode_info_t *ii;

    if (inode == NULL) {
        return;
    }

    ii = (pfs_inode_info_t *)inode->i_private;
    if (ii == NULL || ii->pfs_magic != PFS_INODE_MAGIC) {
        return;
    }

    if (inode->i_nlink > 0) {
        inode->i_nlink--;
        ii->dirty = BENAR;
    }
}

/*
 * pfs_inode_get_size
 * ------------------
 * Dapatkan ukuran inode.
 *
 * Parameter:
 *   inode - Pointer ke inode
 *
 * Return: Ukuran file
 */
tak_bertanda64_t pfs_inode_get_size(inode_t *inode)
{
    pfs_inode_info_t *ii;

    if (inode == NULL) {
        return 0;
    }

    ii = (pfs_inode_info_t *)inode->i_private;
    if (ii == NULL || ii->pfs_magic != PFS_INODE_MAGIC) {
        return 0;
    }

    return ii->size;
}

/*
 * pfs_inode_set_size
 * ------------------
 * Set ukuran inode.
 *
 * Parameter:
 *   inode - Pointer ke inode
 *   size  - Ukuran baru
 */
void pfs_inode_set_size(inode_t *inode, tak_bertanda64_t size)
{
    pfs_inode_info_t *ii;

    if (inode == NULL) {
        return;
    }

    ii = (pfs_inode_info_t *)inode->i_private;
    if (ii == NULL || ii->pfs_magic != PFS_INODE_MAGIC) {
        return;
    }

    ii->size = size;
    inode->i_size = (off_t)size;
    ii->dirty = BENAR;
}
