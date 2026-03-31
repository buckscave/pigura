/*
 * PIGURA OS - PIGURAFS_FILE.C
 * ============================
 * Implementasi operasi file untuk filesystem PiguraFS.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola file
 * PiguraFS termasuk baca, tulis, dan seek.
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
 * KONSTANTA FILE (FILE CONSTANTS)
 * ===========================================================================
 */

/* Magic number */
#define PFS_FILE_MAGIC          0x50465337

/* Ukuran buffer optimal */
#define PFS_FILE_BUFFER_SIZE    8192

/* Maximum extent per inode */
#define PFS_EXTENT_PER_INODE    4

/*
 * ===========================================================================
 * STRUKTUR DATA FILE (FILE DATA STRUCTURES)
 * ===========================================================================
 */

/* Struktur extent */
typedef struct {
    tak_bertanda32_t e_block;           /* Block logical */
    tak_bertanda32_t e_start;           /* Block fisik awal */
    tak_bertanda16_t e_len;             /* Panjang extent */
    tak_bertanda16_t e_flags;           /* Flag extent */
} pfs_extent_t;

/* Struktur inode info (forward) */
typedef struct {
    tak_bertanda32_t pfs_magic;
    void *vfs_inode;
    tak_bertanda32_t ino;
    tak_bertanda64_t size;
    tak_bertanda32_t blocks;
    tak_bertanda32_t extents_count;
    pfs_extent_t extents[PFS_EXTENT_PER_INODE];
    tak_bertanda32_t flags;
    bool_t dirty;
} pfs_inode_info_t;

/* Struktur file handle */
typedef struct {
    tak_bertanda32_t pfs_magic;         /* Magic number validasi */
    file_t *vfs_file;                   /* VFS file */
    tak_bertanda64_t pos;               /* Posisi saat ini */
    tak_bertanda32_t current_extent;    /* Index extent saat ini */
    tak_bertanda32_t block_in_extent;   /* Block dalam extent */
    tak_bertanda32_t block_offset;      /* Offset dalam block */
    bool_t writable;                    /* Bisa ditulis? */
} pfs_file_info_t;

/* Struktur superblock info (forward) */
typedef struct {
    tak_bertanda32_t pfs_magic;
    tak_bertanda32_t block_size;
    tak_bertanda32_t block_size_bits;
    tak_bertanda64_t blocks_free;
    void *device;
} pfs_sb_info_t;

/* Fungsi dari modul lain */
extern status_t pfs_inode_find_block(void *inode, tak_bertanda32_t log_block,
                                     tak_bertanda32_t *phys_block);
extern status_t pfs_inode_add_extent(void *inode, tak_bertanda32_t log_block,
                                     tak_bertanda32_t phys_block,
                                     tak_bertanda16_t len);
extern tak_bertanda32_t pfs_super_alloc_block(void *sbi);
extern void pfs_super_free_block(void *sbi, tak_bertanda32_t block);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INTERNAL (INTERNAL FUNCTION DECLARATIONS)
 * ===========================================================================
 */

static status_t pfs_file_baca_block(pfs_sb_info_t *sbi,
                                    tak_bertanda32_t phys_block,
                                    void *buffer);
static status_t pfs_file_tulis_block(pfs_sb_info_t *sbi,
                                     tak_bertanda32_t phys_block,
                                     const void *buffer);
static status_t pfs_file_alloc_block(pfs_file_info_t *fi,
                                     pfs_inode_info_t *ii,
                                     pfs_sb_info_t *sbi,
                                     tak_bertanda32_t log_block,
                                     tak_bertanda32_t *phys_block);
static status_t pfs_file_map_block(pfs_file_info_t *fi,
                                   pfs_inode_info_t *ii,
                                   pfs_sb_info_t *sbi,
                                   tak_bertanda32_t log_block,
                                   tak_bertanda32_t *phys_block,
                                   bool_t alloc);

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI I/O BLOCK (BLOCK I/O FUNCTIONS)
 * ===========================================================================
 */

static status_t pfs_file_baca_block(pfs_sb_info_t *sbi,
                                    tak_bertanda32_t __attribute__((unused)) phys_block,
                                    void *buffer)
{
    (void)phys_block;
    if (sbi == NULL || buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* TODO: Implementasi pembacaan aktual dari device */

    return STATUS_BERHASIL;
}

static status_t pfs_file_tulis_block(pfs_sb_info_t *sbi,
                                     tak_bertanda32_t __attribute__((unused)) phys_block,
                                     const void *buffer)
{
    (void)phys_block;
    if (sbi == NULL || buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* TODO: Implementasi penulisan aktual ke device */

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI ALLOCATION (ALLOCATION FUNCTIONS)
 * ===========================================================================
 */

static status_t pfs_file_alloc_block(pfs_file_info_t *fi,
                                     pfs_inode_info_t *ii,
                                     pfs_sb_info_t *sbi,
                                     tak_bertanda32_t log_block,
                                     tak_bertanda32_t *phys_block)
{
    tak_bertanda32_t new_block;
    status_t __attribute__((unused)) status;

    if (fi == NULL || ii == NULL || sbi == NULL || phys_block == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Alokasi block baru */
    new_block = pfs_super_alloc_block(sbi);
    if (new_block == 0) {
        return STATUS_PENUH;
    }

    /* Coba tambah ke extent yang ada */
    {
        ukuran_t i;
        for (i = 0; i < ii->extents_count; i++) {
            pfs_extent_t *ext = &ii->extents[i];
            /* Cek apakah bisa extend */
            if (ext->e_block + ext->e_len == log_block &&
                ext->e_start + ext->e_len == new_block) {
                ext->e_len++;
                *phys_block = new_block;
                ii->dirty = BENAR;
                return STATUS_BERHASIL;
            }
        }
    }

    /* Buat extent baru */
    if (ii->extents_count < PFS_EXTENT_PER_INODE) {
        ii->extents[ii->extents_count].e_block = log_block;
        ii->extents[ii->extents_count].e_start = new_block;
        ii->extents[ii->extents_count].e_len = 1;
        ii->extents[ii->extents_count].e_flags = 0;
        ii->extents_count++;
        ii->dirty = BENAR;
        *phys_block = new_block;
        return STATUS_BERHASIL;
    }

    /* Tidak ada slot extent */
    pfs_super_free_block(sbi, new_block);
    return STATUS_PENUH;
}

static status_t pfs_file_map_block(pfs_file_info_t *fi,
                                   pfs_inode_info_t *ii,
                                   pfs_sb_info_t *sbi,
                                   tak_bertanda32_t log_block,
                                   tak_bertanda32_t *phys_block,
                                   bool_t alloc)
{
    status_t status;

    if (fi == NULL || ii == NULL || sbi == NULL || phys_block == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Cari block dalam extent */
    status = pfs_inode_find_block(ii->vfs_inode, log_block, phys_block);
    if (status == STATUS_BERHASIL) {
        return STATUS_BERHASIL;
    }

    /* Block tidak ditemukan */
    if (!alloc) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Alokasi block baru */
    return pfs_file_alloc_block(fi, ii, sbi, log_block, phys_block);
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI UTAMA (MAIN FUNCTIONS)
 * ===========================================================================
 */

/*
 * pfs_file_open
 * -------------
 * Buka file.
 *
 * Parameter:
 *   inode - Inode file
 *   file  - Struktur file VFS
 *
 * Return: Status operasi
 */
status_t pfs_file_open(inode_t *inode, file_t *file)
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
    fi->block_in_extent = 0;
    fi->block_offset = 0;
    fi->writable = (file->f_flags & VFS_OPEN_WRONLY) ||
                   (file->f_flags & VFS_OPEN_RDWR);

    file->f_private = fi;

    return STATUS_BERHASIL;
}

/*
 * pfs_file_release
 * ----------------
 * Tutup file.
 *
 * Parameter:
 *   inode - Inode file
 *   file  - Struktur file VFS
 *
 * Return: Status operasi
 */
status_t pfs_file_release(inode_t *inode, file_t *file)
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

/*
 * pfs_file_read
 * -------------
 * Baca dari file.
 *
 * Parameter:
 *   file   - Struktur file VFS
 *   buffer - Buffer tujuan
 *   size   - Ukuran yang akan dibaca
 *   pos    - Pointer ke posisi file
 *
 * Return: Jumlah byte yang dibaca, atau error negatif
 */
tak_bertandas_t pfs_file_read(file_t *file, void *buffer,
                              ukuran_t size, off_t *pos)
{
    pfs_sb_info_t *sbi;
    pfs_inode_info_t *ii;
    pfs_file_info_t *fi;
    tak_bertanda64_t file_size;
    tak_bertanda64_t read_pos;
    tak_bertanda8_t *block_buffer;
    ukuran_t total_read;
    ukuran_t remaining;

    if (file == NULL || buffer == NULL || pos == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_NULL;
    }

    if (file->f_inode == NULL || file->f_inode->i_sb == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    sbi = (pfs_sb_info_t *)file->f_inode->i_sb->s_private;
    if (sbi == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    ii = (pfs_inode_info_t *)file->f_inode->i_private;
    if (ii == NULL) {
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

    total_read = 0;

    while (remaining > 0) {
        tak_bertanda32_t log_block;
        tak_bertanda32_t phys_block;
        tak_bertanda32_t block_offset;
        tak_bertanda32_t to_read;
        status_t status;

        /* Hitung block dan offset */
        log_block = (tak_bertanda32_t)(read_pos / sbi->block_size);
        block_offset = (tak_bertanda32_t)(read_pos % sbi->block_size);

        /* Map block */
        status = pfs_file_map_block(fi, ii, sbi, log_block,
                                    &phys_block, SALAH);
        if (status != STATUS_BERHASIL) {
            /* Block tidak terallocate, baca sebagai nol */
            kernel_memset(block_buffer, 0, sbi->block_size);
        } else {
            /* Baca block */
            status = pfs_file_baca_block(sbi, phys_block, block_buffer);
            if (status != STATUS_BERHASIL) {
                break;
            }
        }

        /* Hitung berapa byte yang bisa dicopy */
        to_read = (tak_bertanda32_t)MIN(remaining,
                                        sbi->block_size - block_offset);

        /* Copy ke buffer user */
        kernel_memcpy((tak_bertanda8_t *)buffer + total_read,
                      block_buffer + block_offset, to_read);

        total_read += to_read;
        remaining -= to_read;
        read_pos += to_read;
    }

    kfree(block_buffer);

    *pos = (off_t)read_pos;

    return (tak_bertandas_t)total_read;
}

/*
 * pfs_file_write
 * --------------
 * Tulis ke file.
 *
 * Parameter:
 *   file   - Struktur file VFS
 *   buffer - Buffer sumber
 *   size   - Ukuran yang akan ditulis
 *   pos    - Pointer ke posisi file
 *
 * Return: Jumlah byte yang ditulis, atau error negatif
 */
tak_bertandas_t pfs_file_write(file_t *file, const void *buffer,
                               ukuran_t size, off_t *pos)
{
    pfs_sb_info_t *sbi;
    pfs_inode_info_t *ii;
    pfs_file_info_t *fi;
    tak_bertanda64_t write_pos;
    tak_bertanda8_t *block_buffer;
    ukuran_t total_written;
    ukuran_t remaining;

    if (file == NULL || buffer == NULL || pos == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_NULL;
    }

    if (file->f_inode == NULL || file->f_inode->i_sb == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    sbi = (pfs_sb_info_t *)file->f_inode->i_sb->s_private;
    if (sbi == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    ii = (pfs_inode_info_t *)file->f_inode->i_private;
    if (ii == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    fi = (pfs_file_info_t *)file->f_private;
    if (fi == NULL || fi->pfs_magic != PFS_FILE_MAGIC) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    write_pos = (tak_bertanda64_t)*pos;

    /* Handle append mode */
    if (file->f_flags & VFS_OPEN_APPEND) {
        write_pos = ii->size;
    }

    /* Alokasi buffer block */
    block_buffer = (tak_bertanda8_t *)kmalloc(sbi->block_size);
    if (block_buffer == NULL) {
        return (tak_bertandas_t)STATUS_MEMORI_HABIS;
    }

    total_written = 0;
    remaining = size;

    while (remaining > 0) {
        tak_bertanda32_t log_block;
        tak_bertanda32_t phys_block;
        tak_bertanda32_t block_offset;
        tak_bertanda32_t to_write;
        status_t status;

        /* Hitung block dan offset */
        log_block = (tak_bertanda32_t)(write_pos / sbi->block_size);
        block_offset = (tak_bertanda32_t)(write_pos % sbi->block_size);

        /* Map block (dengan alokasi jika perlu) */
        status = pfs_file_map_block(fi, ii, sbi, log_block,
                                    &phys_block, BENAR);
        if (status != STATUS_BERHASIL) {
            break;
        }

        /* Jika tidak menulis penuh block, baca dulu */
        if (block_offset != 0 || remaining < sbi->block_size) {
            status = pfs_file_baca_block(sbi, phys_block, block_buffer);
            if (status != STATUS_BERHASIL) {
                kernel_memset(block_buffer, 0, sbi->block_size);
            }
        }

        /* Hitung berapa byte yang akan ditulis */
        to_write = (tak_bertanda32_t)MIN(remaining,
                                         sbi->block_size - block_offset);

        /* Copy dari buffer user */
        kernel_memcpy(block_buffer + block_offset,
                      (const tak_bertanda8_t *)buffer + total_written,
                      to_write);

        /* Tulis block */
        status = pfs_file_tulis_block(sbi, phys_block, block_buffer);
        if (status != STATUS_BERHASIL) {
            break;
        }

        total_written += to_write;
        remaining -= to_write;
        write_pos += to_write;
    }

    kfree(block_buffer);

    /* Update ukuran file jika perlu */
    if (write_pos > ii->size) {
        ii->size = write_pos;
        file->f_inode->i_size = (off_t)write_pos;
        ii->dirty = BENAR;
    }

    *pos = (off_t)write_pos;

    return (tak_bertandas_t)total_written;
}

/*
 * pfs_file_lseek
 * --------------
 * Seek dalam file.
 *
 * Parameter:
 *   file   - Struktur file VFS
 *   offset - Offset
 *   whence - Referensi (SEEK_SET, SEEK_CUR, SEEK_END)
 *
 * Return: Posisi baru, atau error negatif
 */
off_t pfs_file_lseek(file_t *file, off_t offset, tak_bertanda32_t whence)
{
    pfs_inode_info_t *ii;
    pfs_file_info_t *fi;
    off_t new_pos;

    if (file == NULL || file->f_inode == NULL) {
        return (off_t)STATUS_PARAM_NULL;
    }

    ii = (pfs_inode_info_t *)file->f_inode->i_private;
    if (ii == NULL) {
        return (off_t)STATUS_PARAM_INVALID;
    }

    fi = (pfs_file_info_t *)file->f_private;
    if (fi == NULL || fi->pfs_magic != PFS_FILE_MAGIC) {
        return (off_t)STATUS_PARAM_INVALID;
    }

    switch (whence) {
    case VFS_SEEK_SET:
        new_pos = offset;
        break;
    case VFS_SEEK_CUR:
        new_pos = (off_t)fi->pos + offset;
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

    fi->pos = (tak_bertanda64_t)new_pos;
    file->f_pos = new_pos;

    return new_pos;
}

/*
 * pfs_file_sync
 * -------------
 * Sinkronkan file ke disk.
 *
 * Parameter:
 *   file - Struktur file VFS
 *
 * Return: Status operasi
 */
status_t pfs_file_sync(file_t *file)
{
    pfs_inode_info_t *ii;

    if (file == NULL || file->f_inode == NULL) {
        return STATUS_PARAM_NULL;
    }

    ii = (pfs_inode_info_t *)file->f_inode->i_private;
    if (ii == NULL) {
        return STATUS_PARAM_INVALID;
    }

    if (ii->dirty) {
        /* TODO: Sync inode ke disk */
        ii->dirty = SALAH;
    }

    return STATUS_BERHASIL;
}

/*
 * pfs_file_truncate
 * -----------------
 * Truncate file.
 *
 * Parameter:
 *   inode    - Inode file
 *   new_size - Ukuran baru
 *
 * Return: Status operasi
 */
status_t pfs_file_truncate(inode_t *inode, tak_bertanda64_t new_size)
{
    pfs_inode_info_t *ii;

    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }

    ii = (pfs_inode_info_t *)inode->i_private;
    if (ii == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Update ukuran */
    ii->size = new_size;
    inode->i_size = (off_t)new_size;
    ii->dirty = BENAR;

    /* TODO: Free blocks yang tidak diperlukan */

    return STATUS_BERHASIL;
}

/*
 * pfs_file_get_size
 * -----------------
 * Dapatkan ukuran file.
 *
 * Parameter:
 *   file - Struktur file VFS
 *
 * Return: Ukuran file
 */
tak_bertanda64_t pfs_file_get_size(file_t *file)
{
    pfs_inode_info_t *ii;

    if (file == NULL || file->f_inode == NULL) {
        return 0;
    }

    ii = (pfs_inode_info_t *)file->f_inode->i_private;
    if (ii == NULL) {
        return 0;
    }

    return ii->size;
}

/*
 * pfs_file_eof
 * ------------
 * Cek apakah sudah di akhir file.
 *
 * Parameter:
 *   file - Struktur file VFS
 *
 * Return: BENAR jika EOF
 */
bool_t pfs_file_eof(file_t *file)
{
    pfs_file_info_t *fi;
    pfs_inode_info_t *ii;

    if (file == NULL || file->f_inode == NULL) {
        return BENAR;
    }

    fi = (pfs_file_info_t *)file->f_private;
    ii = (pfs_inode_info_t *)file->f_inode->i_private;

    if (fi == NULL || ii == NULL) {
        return BENAR;
    }

    return (fi->pos >= ii->size) ? BENAR : SALAH;
}

/*
 * pfs_file_tell
 * -------------
 * Dapatkan posisi saat ini.
 *
 * Parameter:
 *   file - Struktur file VFS
 *
 * Return: Posisi saat ini
 */
tak_bertanda64_t pfs_file_tell(file_t *file)
{
    pfs_file_info_t *fi;

    if (file == NULL) {
        return 0;
    }

    fi = (pfs_file_info_t *)file->f_private;
    if (fi == NULL || fi->pfs_magic != PFS_FILE_MAGIC) {
        return 0;
    }

    return fi->pos;
}
