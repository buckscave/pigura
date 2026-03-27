/*
 * PIGURA OS - EXT2_FILE.C
 * ========================
 * Operasi file filesystem EXT2 untuk kernel Pigura OS.
 *
 * Berkas ini berisi implementasi operasi-operasi file pada filesystem
 * EXT2 termasuk pembacaan, penulisan, seek, dan manajemen data file.
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

/*
 * ===========================================================================
 * KONSTANTA FILE (FILE CONSTANTS)
 * ===========================================================================
 */

/* Jumlah block pointers dalam inode */
#define EXT2_NDIR_BLOCKS            12
#define EXT2_IND_BLOCK              12
#define EXT2_DIND_BLOCK             13
#define EXT2_TIND_BLOCK             14
#define EXT2_N_BLOCKS               15

/* Default block size */
#define EXT2_DEF_BLOCK_SIZE         1024

/* Inode flags */
#define EXT2_SYNC_FL                0x00000008
#define EXT2_IMMUTABLE_FL           0x00000010
#define EXT2_APPEND_FL              0x00000020
#define EXT2_NOATIME_FL             0x00000080

/* File open flags */
#define EXT2_FILE_RDONLY            0x0001
#define EXT2_FILE_WRONLY            0x0002
#define EXT2_FILE_RDWR              0x0003
#define EXT2_FILE_APPEND            0x0008

/* Seek whence */
#define EXT2_SEEK_SET               0
#define EXT2_SEEK_CUR               1
#define EXT2_SEEK_END               2

/*
 * ===========================================================================
 * STRUKTUR INODE (MINIMAL)
 * ===========================================================================
 */

typedef struct ext2_inode {
    tak_bertanda16_t i_mode;
    tak_bertanda16_t i_uid;
    tak_bertanda32_t i_size;
    tak_bertanda32_t i_atime;
    tak_bertanda32_t i_ctime;
    tak_bertanda32_t i_mtime;
    tak_bertanda32_t i_dtime;
    tak_bertanda16_t i_gid;
    tak_bertanda16_t i_links_count;
    tak_bertanda32_t i_blocks;
    tak_bertanda32_t i_flags;
    tak_bertanda32_t i_osd1;
    tak_bertanda32_t i_block[EXT2_N_BLOCKS];
    tak_bertanda32_t i_generation;
    tak_bertanda32_t i_file_acl;
    tak_bertanda32_t i_size_high;
    tak_bertanda32_t i_faddr;
    tak_bertanda8_t  i_frag;
    tak_bertanda8_t  i_fsize;
    tak_bertanda16_t i_uid_high;
    tak_bertanda16_t i_gid_high;
    tak_bertanda32_t i_reserved2;
} ext2_inode_t;

/*
 * ===========================================================================
 * STRUKTUR FILE HANDLE (FILE HANDLE STRUCTURE)
 * ===========================================================================
 */

typedef struct ext2_file_handle {
    ino_t            fh_ino;         /* Inode number */
    ext2_inode_t    *fh_inode;      /* Pointer ke inode */
    tak_bertanda32_t fh_block_size; /* Block size */
    off_t            fh_pos;        /* Posisi file saat ini */
    tak_bertanda32_t fh_flags;      /* Open flags */
    bool_t           fh_readable;   /* Bisa dibaca? */
    bool_t           fh_writable;   /* Bisa ditulis? */
    bool_t           fh_append;     /* Mode append? */
    bool_t           fh_dirty;      /* Perlu sync? */
} ext2_file_handle_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INTERNAL (INTERNAL FUNCTION DECLARATIONS)
 * ===========================================================================
 */

static tak_bertanda64_t ext2_file_get_size(ext2_inode_t *inode);
static status_t ext2_file_set_size(ext2_inode_t *inode,
    tak_bertanda64_t size);
static tak_bertanda32_t ext2_file_block_to_logical(off_t offset,
    tak_bertanda32_t block_size);
static tak_bertanda32_t ext2_file_offset_in_block(off_t offset,
    tak_bertanda32_t block_size);

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI UTILITAS (UTILITY FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_file_get_size
 * Mendapatkan ukuran file dari inode.
 */
static tak_bertanda64_t ext2_file_get_size(ext2_inode_t *inode)
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
 * ext2_file_set_size
 * Set ukuran file dalam inode.
 */
static status_t ext2_file_set_size(ext2_inode_t *inode,
    tak_bertanda64_t size)
{
    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }

    inode->i_size = (tak_bertanda32_t)(size & 0xFFFFFFFF);
    inode->i_size_high = (tak_bertanda32_t)((size >> 32) & 0xFFFFFFFF);

    return STATUS_BERHASIL;
}

/*
 * ext2_file_block_to_logical
 * Konversi offset byte ke nomor block logis.
 */
static tak_bertanda32_t ext2_file_block_to_logical(off_t offset,
    tak_bertanda32_t block_size)
{
    if (block_size == 0) {
        return 0;
    }

    return (tak_bertanda32_t)(offset / block_size);
}

/*
 * ext2_file_offset_in_block
 * Mendapatkan offset dalam block.
 */
static tak_bertanda32_t ext2_file_offset_in_block(off_t offset,
    tak_bertanda32_t block_size)
{
    if (block_size == 0) {
        return 0;
    }

    return (tak_bertanda32_t)(offset % block_size);
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI PEMBACAAN (READ FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_file_baca_block
 * Membaca block data file.
 */
status_t ext2_file_baca_block(ext2_inode_t *inode,
    tak_bertanda32_t logical_block, void *buffer,
    tak_bertanda32_t block_size)
{
    tak_bertanda32_t ptrs_per_block;
    tak_bertanda32_t physical_block;
    tak_bertanda32_t direct_limit;
    tak_bertanda32_t indirect_limit;
    tak_bertanda32_t double_limit;
    tak_bertanda32_t index;
    tak_bertanda32_t offset;
    void *indirect_buffer;
    status_t status;

    if (inode == NULL || buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    ptrs_per_block = block_size / sizeof(tak_bertanda32_t);

    /* Hitung batas-batas */
    direct_limit = EXT2_NDIR_BLOCKS;
    indirect_limit = direct_limit + ptrs_per_block;
    double_limit = indirect_limit + (ptrs_per_block * ptrs_per_block);

    /* Direct blocks */
    if (logical_block < direct_limit) {
        physical_block = inode->i_block[logical_block];

        if (physical_block == 0) {
            /* Block tidak dialokasi, return zeros */
            tak_bertanda32_t i;
            for (i = 0; i < block_size; i++) {
                ((char *)buffer)[i] = 0;
            }
            return STATUS_BERHASIL;
        }

        /* Baca block dari disk */
        /* status = device_read_block(physical_block, buffer); */
        status = STATUS_BERHASIL; /* Placeholder */
        return status;
    }

    /* Indirect block */
    if (logical_block < indirect_limit) {
        tak_bertanda32_t indirect_block;

        indirect_block = inode->i_block[EXT2_IND_BLOCK];
        if (indirect_block == 0) {
            /* Tidak ada indirect block, return zeros */
            tak_bertanda32_t i;
            for (i = 0; i < block_size; i++) {
                ((char *)buffer)[i] = 0;
            }
            return STATUS_BERHASIL;
        }

        /* Baca indirect block */
        indirect_buffer = NULL; /* CATATAN: Alokasi dari heap */

        index = logical_block - direct_limit;
        offset = index * sizeof(tak_bertanda32_t);

        /* Baca entry dari indirect block */
        /* physical_block = *((tak_bertanda32_t *)(indirect_buffer + offset)); */
        physical_block = 0; /* Placeholder */

        if (physical_block == 0) {
            /* Block tidak dialokasi */
            tak_bertanda32_t i;
            for (i = 0; i < block_size; i++) {
                ((char *)buffer)[i] = 0;
            }
            return STATUS_BERHASIL;
        }

        /* Baca data block */
        /* status = device_read_block(physical_block, buffer); */
        return STATUS_BERHASIL;
    }

    /* Double indirect block */
    if (logical_block < double_limit) {
        tak_bertanda32_t dind_block;
        tak_bertanda32_t ind_index;
        tak_bertanda32_t block_index;

        dind_block = inode->i_block[EXT2_DIND_BLOCK];
        if (dind_block == 0) {
            tak_bertanda32_t i;
            for (i = 0; i < block_size; i++) {
                ((char *)buffer)[i] = 0;
            }
            return STATUS_BERHASIL;
        }

        /* Hitung index dalam double indirect */
        index = logical_block - indirect_limit;
        ind_index = index / ptrs_per_block;
        block_index = index % ptrs_per_block;

        /* CATATAN: Implementasi pembacaan double indirect */
        /* Untuk sekarang, return zeros */
        (void)dind_block;
        (void)ind_index;
        (void)block_index;

        tak_bertanda32_t i;
        for (i = 0; i < block_size; i++) {
            ((char *)buffer)[i] = 0;
        }
        return STATUS_BERHASIL;
    }

    /* Triple indirect block */
    /* CATATAN: Implementasi untuk file sangat besar */

    return STATUS_BERHASIL;
}

/*
 * ext2_file_baca
 * Membaca data dari file.
 */
tak_bertandas_t ext2_file_baca(ext2_file_handle_t *handle, void *buffer,
    ukuran_t size)
{
    tak_bertanda64_t file_size;
    tak_bertanda32_t block_size;
    char *buf_ptr;
    ukuran_t remaining;
    ukuran_t total_read;
    tak_bertanda32_t logical_block;
    tak_bertanda32_t offset_in_block;
    tak_bertanda32_t chunk_size;
    char block_buffer[EXT2_DEF_BLOCK_SIZE];
    status_t status;

    if (handle == NULL || buffer == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_NULL;
    }

    if (!handle->fh_readable) {
        return (tak_bertandas_t)STATUS_AKSES_DITOLAK;
    }

    file_size = ext2_file_get_size(handle->fh_inode);
    block_size = handle->fh_block_size;

    /* Batasi pembacaan ke ukuran file */
    if ((tak_bertanda64_t)handle->fh_pos >= file_size) {
        return 0; /* EOF */
    }

    if ((tak_bertanda64_t)(handle->fh_pos + size) > file_size) {
        size = (ukuran_t)(file_size - handle->fh_pos);
    }

    buf_ptr = (char *)buffer;
    remaining = size;
    total_read = 0;

    while (remaining > 0) {
        logical_block = ext2_file_block_to_logical(handle->fh_pos,
            block_size);
        offset_in_block = ext2_file_offset_in_block(handle->fh_pos,
            block_size);

        /* Baca block */
        status = ext2_file_baca_block(handle->fh_inode, logical_block,
            block_buffer, block_size);

        if (status != STATUS_BERHASIL) {
            if (total_read > 0) {
                return (tak_bertandas_t)total_read;
            }
            return (tak_bertandas_t)status;
        }

        /* Hitung berapa banyak yang bisa dibaca dari block ini */
        chunk_size = block_size - offset_in_block;
        if (chunk_size > remaining) {
            chunk_size = (tak_bertanda32_t)remaining;
        }

        /* Salin data ke buffer user */
        {
            tak_bertanda32_t i;
            for (i = 0; i < chunk_size; i++) {
                buf_ptr[total_read + i] = block_buffer[offset_in_block + i];
            }
        }

        total_read += chunk_size;
        handle->fh_pos += chunk_size;
        remaining -= chunk_size;
    }

    /* Update atime jika tidak noatime */
    if (!(handle->fh_inode->i_flags & EXT2_NOATIME_FL)) {
        /* handle->fh_inode->i_atime = get_current_time(); */
    }

    return (tak_bertandas_t)total_read;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI PENULISAN (WRITE FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_file_tulis_block
 * Menulis block data file.
 */
status_t ext2_file_tulis_block(ext2_inode_t *inode,
    tak_bertanda32_t logical_block, const void *buffer,
    tak_bertanda32_t block_size)
{
    tak_bertanda32_t physical_block;
    status_t status;

    if (inode == NULL || buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* CATATAN: Implementasi alokasi block jika belum ada */

    /* Direct blocks */
    if (logical_block < EXT2_NDIR_BLOCKS) {
        physical_block = inode->i_block[logical_block];

        if (physical_block == 0) {
            /* Alokasi block baru */
            /* physical_block = ext2_alloc_block(); */
            /* inode->i_block[logical_block] = physical_block; */
            /* inode->i_blocks += block_size / 512; */
            return STATUS_BELUM_IMPLEMENTASI;
        }

        /* Tulis block ke disk */
        /* status = device_write_block(physical_block, buffer); */
        status = STATUS_BERHASIL; /* Placeholder */
        return status;
    }

    /* CATATAN: Implementasi untuk indirect/double/triple indirect */

    return STATUS_BERHASIL;
}

/*
 * ext2_file_tulis
 * Menulis data ke file.
 */
tak_bertandas_t ext2_file_tulis(ext2_file_handle_t *handle,
    const void *buffer, ukuran_t size)
{
    tak_bertanda64_t file_size;
    tak_bertanda32_t block_size;
    const char *buf_ptr;
    ukuran_t remaining;
    ukuran_t total_written;
    tak_bertanda32_t logical_block;
    tak_bertanda32_t offset_in_block;
    tak_bertanda32_t chunk_size;
    char block_buffer[EXT2_DEF_BLOCK_SIZE];
    status_t status;

    if (handle == NULL || buffer == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_NULL;
    }

    if (!handle->fh_writable) {
        return (tak_bertandas_t)STATUS_AKSES_DITOLAK;
    }

    /* Cek immutable flag */
    if (handle->fh_inode->i_flags & EXT2_IMMUTABLE_FL) {
        return (tak_bertandas_t)STATUS_AKSES_DITOLAK;
    }

    /* Mode append: selalu tulis di akhir file */
    if (handle->fh_append) {
        file_size = ext2_file_get_size(handle->fh_inode);
        handle->fh_pos = (off_t)file_size;
    }

    block_size = handle->fh_block_size;
    buf_ptr = (const char *)buffer;
    remaining = size;
    total_written = 0;

    while (remaining > 0) {
        logical_block = ext2_file_block_to_logical(handle->fh_pos,
            block_size);
        offset_in_block = ext2_file_offset_in_block(handle->fh_pos,
            block_size);

        /* Jika tidak menulis block penuh dari awal, baca block dulu */
        if (offset_in_block != 0 || remaining < block_size) {
            status = ext2_file_baca_block(handle->fh_inode, logical_block,
                block_buffer, block_size);

            if (status != STATUS_BERHASIL) {
                if (total_written > 0) {
                    handle->fh_dirty = BENAR;
                    return (tak_bertandas_t)total_written;
                }
                return (tak_bertandas_t)status;
            }
        }

        /* Hitung berapa banyak yang bisa ditulis ke block ini */
        chunk_size = block_size - offset_in_block;
        if (chunk_size > remaining) {
            chunk_size = (tak_bertanda32_t)remaining;
        }

        /* Salin data dari buffer user ke block buffer */
        {
            tak_bertanda32_t i;
            for (i = 0; i < chunk_size; i++) {
                block_buffer[offset_in_block + i] = buf_ptr[total_written + i];
            }
        }

        /* Tulis block */
        status = ext2_file_tulis_block(handle->fh_inode, logical_block,
            block_buffer, block_size);

        if (status != STATUS_BERHASIL) {
            if (total_written > 0) {
                handle->fh_dirty = BENAR;
                return (tak_bertandas_t)total_written;
            }
            return (tak_bertandas_t)status;
        }

        total_written += chunk_size;
        handle->fh_pos += chunk_size;
        remaining -= chunk_size;
    }

    /* Update file size jika perlu */
    file_size = ext2_file_get_size(handle->fh_inode);
    if ((tak_bertanda64_t)handle->fh_pos > file_size) {
        ext2_file_set_size(handle->fh_inode, (tak_bertanda64_t)handle->fh_pos);
    }

    /* Update mtime */
    /* handle->fh_inode->i_mtime = get_current_time(); */

    handle->fh_dirty = BENAR;

    return (tak_bertandas_t)total_written;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI SEEK (SEEK FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_file_seek
 * Mengubah posisi file.
 */
off_t ext2_file_seek(ext2_file_handle_t *handle, off_t offset,
    tak_bertanda32_t whence)
{
    tak_bertanda64_t file_size;
    off_t new_pos;

    if (handle == NULL) {
        return (off_t)STATUS_PARAM_NULL;
    }

    file_size = ext2_file_get_size(handle->fh_inode);

    switch (whence) {
        case EXT2_SEEK_SET:
            new_pos = offset;
            break;

        case EXT2_SEEK_CUR:
            new_pos = handle->fh_pos + offset;
            break;

        case EXT2_SEEK_END:
            new_pos = (off_t)file_size + offset;
            break;

        default:
            return (off_t)STATUS_PARAM_INVALID;
    }

    /* Validasi posisi baru */
    if (new_pos < 0) {
        return (off_t)STATUS_PARAM_INVALID;
    }

    handle->fh_pos = new_pos;

    return new_pos;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI TRUNCATE (TRUNCATE FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_file_truncate
 * Memotong atau memperbesar file.
 */
status_t ext2_file_truncate(ext2_inode_t *inode, off_t new_size,
    tak_bertanda32_t block_size)
{
    tak_bertanda64_t old_size;
    tak_bertanda32_t old_blocks;
    tak_bertanda32_t new_blocks;
    tak_bertanda32_t i;

    if (inode == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (new_size < 0) {
        return STATUS_PARAM_INVALID;
    }

    /* Cek immutable flag */
    if (inode->i_flags & EXT2_IMMUTABLE_FL) {
        return STATUS_AKSES_DITOLAK;
    }

    old_size = ext2_file_get_size(inode);

    if ((tak_bertanda64_t)new_size == old_size) {
        return STATUS_BERHASIL; /* Tidak ada perubahan */
    }

    /* Hitung jumlah blocks */
    old_blocks = (tak_bertanda32_t)((old_size + block_size - 1) / block_size);
    new_blocks = (tak_bertanda32_t)((new_size + block_size - 1) / block_size);

    if ((tak_bertanda64_t)new_size < old_size) {
        /* Shrink: bebaskan blocks yang tidak diperlukan */
        for (i = new_blocks; i < old_blocks; i++) {
            /* CATATAN: Implementasi free block */
            /* ext2_free_block(inode, i); */
        }
    } else {
        /* Grow: alokasi blocks baru jika perlu */
        for (i = old_blocks; i < new_blocks; i++) {
            /* CATATAN: Implementasi alloc block */
            /* status = ext2_alloc_block(inode, i); */
        }
    }

    /* Update file size */
    ext2_file_set_size(inode, (tak_bertanda64_t)new_size);

    /* Update mtime dan ctime */
    /* inode->i_mtime = get_current_time(); */
    /* inode->i_ctime = get_current_time(); */

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI OPEN/CLOSE (OPEN/CLOSE FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_file_open
 * Membuka file.
 */
status_t ext2_file_open(ext2_file_handle_t *handle, ext2_inode_t *inode,
    tak_bertanda32_t flags, tak_bertanda32_t block_size)
{
    if (handle == NULL || inode == NULL) {
        return STATUS_PARAM_NULL;
    }

    handle->fh_ino = 0; /* CATATAN: Set dari inode */
    handle->fh_inode = inode;
    handle->fh_block_size = block_size;
    handle->fh_pos = 0;
    handle->fh_flags = flags;
    handle->fh_dirty = SALAH;

    /* Set permission flags */
    handle->fh_readable = (flags & EXT2_FILE_RDONLY) ||
        (flags & EXT2_FILE_RDWR);
    handle->fh_writable = (flags & EXT2_FILE_WRONLY) ||
        (flags & EXT2_FILE_RDWR) || (flags & EXT2_FILE_APPEND);
    handle->fh_append = (flags & EXT2_FILE_APPEND) ? BENAR : SALAH;

    /* Mode append: posisi di akhir file */
    if (handle->fh_append) {
        handle->fh_pos = (off_t)ext2_file_get_size(inode);
    }

    return STATUS_BERHASIL;
}

/*
 * ext2_file_close
 * Menutup file.
 */
status_t ext2_file_close(ext2_file_handle_t *handle)
{
    status_t status;

    if (handle == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Sync jika dirty */
    if (handle->fh_dirty) {
        /* CATATAN: Implementasi sync inode */
        status = STATUS_BERHASIL;
    } else {
        status = STATUS_BERHASIL;
    }

    handle->fh_inode = NULL;
    handle->fh_pos = 0;

    return status;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI SYNC (SYNC FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_file_sync
 * Sinkronkan file ke disk.
 */
status_t ext2_file_sync(ext2_file_handle_t *handle)
{
    if (handle == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* CATATAN: Implementasi sync inode dan metadata */

    handle->fh_dirty = SALAH;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI STAT (STAT FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_file_stat
 * Mendapatkan informasi file.
 */
status_t ext2_file_stat(ext2_inode_t *inode, vfs_stat_t *stat)
{
    if (inode == NULL || stat == NULL) {
        return STATUS_PARAM_NULL;
    }

    stat->st_mode = (mode_t)inode->i_mode;
    stat->st_uid = (uid_t)inode->i_uid | ((uid_t)inode->i_uid_high << 16);
    stat->st_gid = (gid_t)inode->i_gid | ((gid_t)inode->i_gid_high << 16);
    stat->st_size = (off_t)ext2_file_get_size(inode);
    stat->st_atime = (waktu_t)inode->i_atime;
    stat->st_mtime = (waktu_t)inode->i_mtime;
    stat->st_ctime = (waktu_t)inode->i_ctime;
    stat->st_nlink = inode->i_links_count;
    stat->st_blocks = inode->i_blocks;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI QUERY (QUERY FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_file_is_readable
 * Cek apakah file bisa dibaca.
 */
bool_t ext2_file_is_readable(ext2_file_handle_t *handle)
{
    if (handle == NULL) {
        return SALAH;
    }

    return handle->fh_readable;
}

/*
 * ext2_file_is_writable
 * Cek apakah file bisa ditulis.
 */
bool_t ext2_file_is_writable(ext2_file_handle_t *handle)
{
    if (handle == NULL) {
        return SALAH;
    }

    /* Cek immutable flag */
    if (handle->fh_inode != NULL &&
        (handle->fh_inode->i_flags & EXT2_IMMUTABLE_FL)) {
        return SALAH;
    }

    return handle->fh_writable;
}

/*
 * ext2_file_get_position
 * Mendapatkan posisi file saat ini.
 */
off_t ext2_file_get_position(ext2_file_handle_t *handle)
{
    if (handle == NULL) {
        return 0;
    }

    return handle->fh_pos;
}

/*
 * ext2_file_get_size_handle
 * Mendapatkan ukuran file dari handle.
 */
tak_bertanda64_t ext2_file_get_size_handle(ext2_file_handle_t *handle)
{
    if (handle == NULL || handle->fh_inode == NULL) {
        return 0;
    }

    return ext2_file_get_size(handle->fh_inode);
}

/*
 * ext2_file_eof
 * Cek apakah sudah mencapai end of file.
 */
bool_t ext2_file_eof(ext2_file_handle_t *handle)
{
    tak_bertanda64_t file_size;

    if (handle == NULL || handle->fh_inode == NULL) {
        return BENAR;
    }

    file_size = ext2_file_get_size(handle->fh_inode);

    return (tak_bertanda64_t)handle->fh_pos >= file_size;
}
