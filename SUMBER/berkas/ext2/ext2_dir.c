/*
 * PIGURA OS - EXT2_DIR.C
 * =======================
 * Operasi direktori filesystem EXT2 untuk kernel Pigura OS.
 *
 * Berkas ini berisi implementasi operasi-operasi yang berkaitan
 * dengan direktori EXT2 termasuk pembacaan entri, pencarian,
 * pembuatan, dan penghapusan direktori.
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
 * KONSTANTA DIREKTORI (DIRECTORY CONSTANTS)
 * ===========================================================================
 */

/* Panjang nama maksimum */
#define EXT2_NAME_LEN_MAX           255

/* Minimum directory entry size */
#define EXT2_DIR_ENTRY_MIN_SIZE     12

/* Default directory block size */
#define EXT2_DIR_BLOCK_SIZE         1024

/* Rec_len untuk terakhir dalam block */
#define EXT2_DIR_REC_LEN(namelen) \
    (((namelen) + 8 + 3) & ~3)

/* File type dalam directory entry */
#define EXT2_FT_UNKNOWN             0
#define EXT2_FT_REG_FILE            1
#define EXT2_FT_DIR                 2
#define EXT2_FT_CHRDEV              3
#define EXT2_FT_BLKDEV              4
#define EXT2_FT_FIFO                5
#define EXT2_FT_SOCK                6
#define EXT2_FT_SYMLINK             7

/* Special directory entries */
#define EXT2_DOT_INO                0   /* Placeholder for . */
#define EXT2_DOTDOT_INO             0   /* Placeholder for .. */

/* Tipe file dalam mode */
#define EXT2_S_IFIFO                0x1000
#define EXT2_S_IFCHR                0x2000
#define EXT2_S_IFDIR                0x4000
#define EXT2_S_IFBLK                0x6000
#define EXT2_S_IFREG                0x8000
#define EXT2_S_IFLNK                0xA000
#define EXT2_S_IFSOCK               0xC000
#define EXT2_S_IFMT                 0xF000

/* Makro cek tipe */
#define EXT2_S_ISDIR(m)             (((m) & EXT2_S_IFMT) == EXT2_S_IFDIR)
#define EXT2_S_ISREG(m)             (((m) & EXT2_S_IFMT) == EXT2_S_IFREG)
#define EXT2_S_ISLNK(m)             (((m) & EXT2_S_IFMT) == EXT2_S_IFLNK)

/*
 * ===========================================================================
 * STRUKTUR DIRECTORY ENTRY (DIRECTORY ENTRY STRUCTURE)
 * ===========================================================================
 */

typedef struct ext2_dir_entry {
    tak_bertanda32_t inode;              /* Inode number */
    tak_bertanda16_t rec_len;            /* Record length */
    tak_bertanda8_t  name_len;           /* Name length */
    tak_bertanda8_t  file_type;          /* File type */
    char             name[EXT2_NAME_LEN_MAX]; /* Filename */
} ext2_dir_entry_t;

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
    tak_bertanda32_t i_block[15];
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
 * STRUKTUR ITERATOR DIREKTORI (DIRECTORY ITERATOR STRUCTURE)
 * ===========================================================================
 */

typedef struct ext2_dir_iterator {
    ino_t            di_dir_ino;         /* Directory inode */
    tak_bertanda32_t di_block;           /* Current block number */
    tak_bertanda32_t di_offset;          /* Offset within block */
    tak_bertanda32_t di_block_size;      /* Block size */
    ext2_dir_entry_t di_entry;           /* Current entry */
    bool_t           di_valid;           /* Iterator valid? */
} ext2_dir_iterator_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INTERNAL (INTERNAL FUNCTION DECLARATIONS)
 * ===========================================================================
 */

static status_t ext2_dir_entry_validasi(ext2_dir_entry_t *entry,
    tak_bertanda32_t block_size);
static tak_bertanda8_t ext2_mode_to_filetype(tak_bertanda16_t mode);
static tak_bertanda16_t ext2_filetype_to_mode(tak_bertanda8_t filetype);
static tak_bertanda32_t ext2_dir_entry_size(tak_bertanda8_t name_len);

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI UTILITAS (UTILITY FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_dir_entry_size
 * Menghitung ukuran directory entry.
 */
static tak_bertanda32_t ext2_dir_entry_size(tak_bertanda8_t name_len)
{
    /* Align ke 4-byte boundary */
    tak_bertanda32_t size;

    /* Minimal 12 bytes: inode(4) + rec_len(2) + name_len(1) + file_type(1) */
    size = 8 + name_len;
    size = (size + 3) & ~3; /* Align ke 4 bytes */

    return size;
}

/*
 * ext2_mode_to_filetype
 * Konversi mode ke file type.
 */
static tak_bertanda8_t ext2_mode_to_filetype(tak_bertanda16_t mode)
{
    switch (mode & EXT2_S_IFMT) {
        case EXT2_S_IFREG:
            return EXT2_FT_REG_FILE;
        case EXT2_S_IFDIR:
            return EXT2_FT_DIR;
        case EXT2_S_IFCHR:
            return EXT2_FT_CHRDEV;
        case EXT2_S_IFBLK:
            return EXT2_FT_BLKDEV;
        case EXT2_S_IFIFO:
            return EXT2_FT_FIFO;
        case EXT2_S_IFSOCK:
            return EXT2_FT_SOCK;
        case EXT2_S_IFLNK:
            return EXT2_FT_SYMLINK;
        default:
            return EXT2_FT_UNKNOWN;
    }
}

/*
 * ext2_filetype_to_mode
 * Konversi file type ke mode.
 */
static tak_bertanda16_t ext2_filetype_to_mode(tak_bertanda8_t filetype)
{
    switch (filetype) {
        case EXT2_FT_REG_FILE:
            return EXT2_S_IFREG;
        case EXT2_FT_DIR:
            return EXT2_S_IFDIR;
        case EXT2_FT_CHRDEV:
            return EXT2_S_IFCHR;
        case EXT2_FT_BLKDEV:
            return EXT2_S_IFBLK;
        case EXT2_FT_FIFO:
            return EXT2_S_IFIFO;
        case EXT2_FT_SOCK:
            return EXT2_S_IFSOCK;
        case EXT2_FT_SYMLINK:
            return EXT2_S_IFLNK;
        default:
            return 0;
    }
}

/*
 * ext2_dir_entry_validasi
 * Memvalidasi directory entry.
 */
static status_t ext2_dir_entry_validasi(ext2_dir_entry_t *entry,
    tak_bertanda32_t block_size)
{
    tak_bertanda32_t min_size;

    if (entry == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Cek rec_len minimum */
    min_size = ext2_dir_entry_size(entry->name_len);
    if (entry->rec_len < min_size) {
        return STATUS_FS_CORRUPT;
    }

    /* Cek rec_len tidak melebihi block */
    if (entry->rec_len > block_size) {
        return STATUS_FS_CORRUPT;
    }

    /* Cek name_len */
    if (entry->name_len >= EXT2_NAME_LEN_MAX) {
        return STATUS_FS_CORRUPT;
    }

    /* Cek file_type valid */
    if (entry->file_type > EXT2_FT_SYMLINK) {
        return STATUS_FS_CORRUPT;
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI PEMBACAAN DIREKTORI (DIRECTORY READ FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_dir_baca_entry
 * Membaca satu directory entry dari posisi tertentu.
 */
status_t ext2_dir_baca_entry(void *dir_data, tak_bertanda32_t __attribute__((unused)) block_num,
    tak_bertanda32_t offset, ext2_dir_entry_t *entry,
    tak_bertanda32_t block_size)
{
    (void)block_num;
    char *block_ptr;
    char *entry_ptr;
    tak_bertanda32_t i;

    if (dir_data == NULL || entry == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Validasi offset */
    if (offset >= block_size) {
        return STATUS_PARAM_INVALID;
    }

    block_ptr = (char *)dir_data;
    entry_ptr = block_ptr + offset;

    /* Baca inode number */
    entry->inode = *((tak_bertanda32_t *)entry_ptr);
    entry_ptr += 4;

    /* Baca rec_len */
    entry->rec_len = *((tak_bertanda16_t *)entry_ptr);
    entry_ptr += 2;

    /* Baca name_len */
    entry->name_len = *((tak_bertanda8_t *)entry_ptr);
    entry_ptr += 1;

    /* Baca file_type */
    entry->file_type = *((tak_bertanda8_t *)entry_ptr);
    entry_ptr += 1;

    /* Baca nama */
    for (i = 0; i < entry->name_len && i < EXT2_NAME_LEN_MAX; i++) {
        entry->name[i] = entry_ptr[i];
    }
    entry->name[i] = '\0';

    /* Validasi entry */
    return ext2_dir_entry_validasi(entry, block_size);
}

/*
 * ext2_dir_cari_entry
 * Mencari entry dalam direktori.
 */
status_t ext2_dir_cari_entry(void *dir_data, const char *nama,
    tak_bertanda32_t block_size, ext2_dir_entry_t *entry,
    tak_bertanda32_t *found_offset)
{
    tak_bertanda32_t offset;
    tak_bertanda32_t namelen;
    status_t status;
    tak_bertanda32_t i;

    if (dir_data == NULL || nama == NULL || entry == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Hitung panjang nama */
    namelen = 0;
    while (nama[namelen] != '\0' && namelen < EXT2_NAME_LEN_MAX) {
        namelen++;
    }

    offset = 0;

    /* Iterasi semua entry dalam block */
    while (offset < block_size) {
        /* Baca entry */
        status = ext2_dir_baca_entry(dir_data, 0, offset, entry, block_size);

        if (status != STATUS_BERHASIL) {
            return status;
        }

        /* Cek apakah nama cocok */
        if (entry->inode != 0 && entry->name_len == namelen) {
            bool_t match = BENAR;
            for (i = 0; i < namelen; i++) {
                if (entry->name[i] != nama[i]) {
                    match = SALAH;
                    break;
                }
            }

            if (match) {
                if (found_offset != NULL) {
                    *found_offset = offset;
                }
                return STATUS_BERHASIL;
            }
        }

        /* Lanjut ke entry berikutnya */
        if (entry->rec_len == 0) {
            break; /* Prevent infinite loop */
        }
        offset += entry->rec_len;
    }

    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * ext2_dir_list
 * Mendaftar semua entry dalam direktori.
 */
status_t ext2_dir_list(void *dir_data, tak_bertanda32_t block_size,
    ext2_dir_entry_t *entries, tak_bertanda32_t max_entries,
    tak_bertanda32_t *count)
{
    ext2_dir_entry_t entry;
    tak_bertanda32_t offset;
    tak_bertanda32_t num_entries;
    status_t status;
    tak_bertanda32_t i;

    if (dir_data == NULL || entries == NULL || count == NULL) {
        return STATUS_PARAM_NULL;
    }

    num_entries = 0;
    offset = 0;

    while (offset < block_size && num_entries < max_entries) {
        /* Baca entry */
        status = ext2_dir_baca_entry(dir_data, 0, offset, &entry, block_size);

        if (status != STATUS_BERHASIL) {
            *count = num_entries;
            return status;
        }

        /* Jika entry valid (inode != 0), tambahkan ke list */
        if (entry.inode != 0) {
            /* Salin entry */
            entries[num_entries].inode = entry.inode;
            entries[num_entries].rec_len = entry.rec_len;
            entries[num_entries].name_len = entry.name_len;
            entries[num_entries].file_type = entry.file_type;

            for (i = 0; i <= entry.name_len && i < EXT2_NAME_LEN_MAX; i++) {
                entries[num_entries].name[i] = entry.name[i];
            }

            num_entries++;
        }

        /* Lanjut ke entry berikutnya */
        if (entry.rec_len == 0) {
            break;
        }
        offset += entry.rec_len;
    }

    *count = num_entries;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI PENULISAN DIREKTORI (DIRECTORY WRITE FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_dir_tulis_entry
 * Menulis directory entry ke posisi tertentu.
 */
status_t ext2_dir_tulis_entry(void *dir_data, tak_bertanda32_t offset,
    const ext2_dir_entry_t *entry, tak_bertanda32_t block_size)
{
    char *block_ptr;
    char *entry_ptr;
    tak_bertanda32_t i;

    if (dir_data == NULL || entry == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Validasi offset */
    if (offset + 8 + entry->name_len > block_size) {
        return STATUS_PARAM_INVALID;
    }

    block_ptr = (char *)dir_data;
    entry_ptr = block_ptr + offset;

    /* Tulis inode number */
    *((tak_bertanda32_t *)entry_ptr) = entry->inode;
    entry_ptr += 4;

    /* Tulis rec_len */
    *((tak_bertanda16_t *)entry_ptr) = entry->rec_len;
    entry_ptr += 2;

    /* Tulis name_len */
    *((tak_bertanda8_t *)entry_ptr) = entry->name_len;
    entry_ptr += 1;

    /* Tulis file_type */
    *((tak_bertanda8_t *)entry_ptr) = entry->file_type;
    entry_ptr += 1;

    /* Tulis nama */
    for (i = 0; i < entry->name_len; i++) {
        entry_ptr[i] = entry->name[i];
    }

    return STATUS_BERHASIL;
}

/*
 * ext2_dir_tambah_entry
 * Menambah entry baru ke direktori.
 */
status_t ext2_dir_tambah_entry(void *dir_data, tak_bertanda32_t block_size,
    ino_t inode, const char *nama, tak_bertanda8_t filetype)
{
    ext2_dir_entry_t entry;
    ext2_dir_entry_t last_entry;
    tak_bertanda32_t offset;
    tak_bertanda32_t namelen;
    tak_bertanda32_t needed_size;
    tak_bertanda32_t last_offset;
    status_t status;
    tak_bertanda32_t i;

    if (dir_data == NULL || nama == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Cek apakah nama sudah ada */
    status = ext2_dir_cari_entry(dir_data, nama, block_size, &entry, NULL);
    if (status == STATUS_BERHASIL) {
        return STATUS_SUDAH_ADA; /* Entry sudah ada */
    }

    /* Hitung panjang nama */
    namelen = 0;
    while (nama[namelen] != '\0' && namelen < EXT2_NAME_LEN_MAX) {
        namelen++;
    }

    needed_size = ext2_dir_entry_size((tak_bertanda8_t)namelen);

    /* Cari ruang kosong atau entry terakhir */
    offset = 0;
    last_offset = 0;

    while (offset < block_size) {
        status = ext2_dir_baca_entry(dir_data, 0, offset, &last_entry,
            block_size);

        if (status != STATUS_BERHASIL) {
            break;
        }

        /* Jika entry kosong (inode = 0), cek apakah muat */
        if (last_entry.inode == 0 && last_entry.rec_len >= needed_size) {
            /* Tulis entry baru di sini */
            entry.inode = inode;
            entry.rec_len = last_entry.rec_len;
            entry.name_len = (tak_bertanda8_t)namelen;
            entry.file_type = filetype;

            for (i = 0; i < namelen; i++) {
                entry.name[i] = nama[i];
            }
            entry.name[namelen] = '\0';

            return ext2_dir_tulis_entry(dir_data, offset, &entry, block_size);
        }

        last_offset = offset;

        /* Lanjut ke entry berikutnya */
        if (last_entry.rec_len == 0) {
            break;
        }
        offset += last_entry.rec_len;
    }

    /* Coba split entry terakhir jika ada ruang */
    if (last_offset > 0) {
        status = ext2_dir_baca_entry(dir_data, 0, last_offset, &last_entry,
            block_size);

        if (status == STATUS_BERHASIL) {
            tak_bertanda32_t used_size;

            used_size = ext2_dir_entry_size(last_entry.name_len);
            if (last_entry.rec_len >= used_size + needed_size) {
                /* Split entry */
                tak_bertanda32_t new_offset;

                new_offset = last_offset + used_size;

                /* Update rec_len entry lama */
                last_entry.rec_len = (tak_bertanda16_t)used_size;
                ext2_dir_tulis_entry(dir_data, last_offset, &last_entry,
                    block_size);

                /* Tulis entry baru */
                entry.inode = inode;
                entry.rec_len = (tak_bertanda16_t)(block_size - new_offset);
                entry.name_len = (tak_bertanda8_t)namelen;
                entry.file_type = filetype;

                for (i = 0; i < namelen; i++) {
                    entry.name[i] = nama[i];
                }
                entry.name[namelen] = '\0';

                return ext2_dir_tulis_entry(dir_data, new_offset, &entry,
                    block_size);
            }
        }
    }

    return STATUS_PENUH; /* Tidak ada ruang */
}

/*
 * ext2_dir_hapus_entry
 * Menghapus entry dari direktori.
 */
status_t ext2_dir_hapus_entry(void *dir_data, tak_bertanda32_t block_size,
    const char *nama)
{
    ext2_dir_entry_t entry;
    ext2_dir_entry_t prev_entry;
    tak_bertanda32_t offset;
    tak_bertanda32_t prev_offset;
    status_t status;

    if (dir_data == NULL || nama == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Cari entry */
    status = ext2_dir_cari_entry(dir_data, nama, block_size, &entry, &offset);
    if (status != STATUS_BERHASIL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Jika entry pertama, set inode ke 0 */
    if (offset == 0) {
        entry.inode = 0;
        return ext2_dir_tulis_entry(dir_data, offset, &entry, block_size);
    }

    /* Cari entry sebelumnya */
    prev_offset = 0;

    while (prev_offset < offset) {
        status = ext2_dir_baca_entry(dir_data, 0, prev_offset, &prev_entry,
            block_size);

        if (status != STATUS_BERHASIL) {
            break;
        }

        if (prev_offset + prev_entry.rec_len == offset) {
            /* Gabungkan dengan entry sebelumnya */
            prev_entry.rec_len += entry.rec_len;
            return ext2_dir_tulis_entry(dir_data, prev_offset, &prev_entry,
                block_size);
        }

        prev_offset += prev_entry.rec_len;
    }

    /* Set inode ke 0 */
    entry.inode = 0;
    return ext2_dir_tulis_entry(dir_data, offset, &entry, block_size);
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI DIREKTORI KHUSUS (SPECIAL DIRECTORY FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_dir_buat_dot_entries
 * Membuat entry . dan .. dalam direktori baru.
 */
status_t ext2_dir_buat_dot_entries(void *dir_data, tak_bertanda32_t block_size,
    ino_t dir_ino, ino_t parent_ino)
{
    ext2_dir_entry_t dot_entry;
    ext2_dir_entry_t dotdot_entry;
    status_t status;

    if (dir_data == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Entry untuk . */
    dot_entry.inode = dir_ino;
    dot_entry.rec_len = 12; /* Align ke 4 bytes */
    dot_entry.name_len = 1;
    dot_entry.file_type = EXT2_FT_DIR;
    dot_entry.name[0] = '.';
    dot_entry.name[1] = '\0';

    status = ext2_dir_tulis_entry(dir_data, 0, &dot_entry, block_size);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    /* Entry untuk .. */
    dotdot_entry.inode = parent_ino;
    dotdot_entry.rec_len = (tak_bertanda16_t)(block_size - 12);
    dotdot_entry.name_len = 2;
    dotdot_entry.file_type = EXT2_FT_DIR;
    dotdot_entry.name[0] = '.';
    dotdot_entry.name[1] = '.';
    dotdot_entry.name[2] = '\0';

    return ext2_dir_tulis_entry(dir_data, 12, &dotdot_entry, block_size);
}

/*
 * ext2_dir_count_entries
 * Menghitung jumlah entry dalam direktori.
 */
status_t ext2_dir_count_entries(void *dir_data, tak_bertanda32_t block_size,
    tak_bertanda32_t *count)
{
    ext2_dir_entry_t entry;
    tak_bertanda32_t offset;
    tak_bertanda32_t num_entries;
    status_t status;

    if (dir_data == NULL || count == NULL) {
        return STATUS_PARAM_NULL;
    }

    num_entries = 0;
    offset = 0;

    while (offset < block_size) {
        status = ext2_dir_baca_entry(dir_data, 0, offset, &entry, block_size);

        if (status != STATUS_BERHASIL) {
            break;
        }

        if (entry.inode != 0) {
            num_entries++;
        }

        if (entry.rec_len == 0) {
            break;
        }
        offset += entry.rec_len;
    }

    *count = num_entries;
    return STATUS_BERHASIL;
}

/*
 * ext2_dir_is_empty
 * Cek apakah direktori kosong (hanya . dan ..).
 */
bool_t ext2_dir_is_empty(void *dir_data, tak_bertanda32_t block_size)
{
    ext2_dir_entry_t entry;
    tak_bertanda32_t offset;
    status_t status;

    if (dir_data == NULL) {
        return BENAR;
    }

    offset = 0;

    while (offset < block_size) {
        status = ext2_dir_baca_entry(dir_data, 0, offset, &entry, block_size);

        if (status != STATUS_BERHASIL) {
            break;
        }

        if (entry.inode != 0) {
            /* Skip . dan .. */
            if (entry.name_len > 2 ||
                (entry.name_len == 1 && entry.name[0] != '.') ||
                (entry.name_len == 2 && (entry.name[0] != '.' ||
                 entry.name[1] != '.'))) {
                return SALAH; /* Direktori tidak kosong */
            }
        }

        if (entry.rec_len == 0) {
            break;
        }
        offset += entry.rec_len;
    }

    return BENAR; /* Direktori kosong */
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI ITERATOR (ITERATOR FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_dir_iterator_init
 * Inisialisasi iterator direktori.
 */
status_t ext2_dir_iterator_init(ext2_dir_iterator_t *iter, ino_t dir_ino,
    tak_bertanda32_t block_size)
{
    if (iter == NULL) {
        return STATUS_PARAM_NULL;
    }

    iter->di_dir_ino = dir_ino;
    iter->di_block = 0;
    iter->di_offset = 0;
    iter->di_block_size = block_size;
    iter->di_valid = SALAH;

    return STATUS_BERHASIL;
}

/*
 * ext2_dir_iterator_next
 * Mendapatkan entry berikutnya.
 */
status_t ext2_dir_iterator_next(ext2_dir_iterator_t *iter,
    void *dir_data, ext2_dir_entry_t *entry)
{
    status_t status;

    if (iter == NULL || dir_data == NULL || entry == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Baca entry pada posisi saat ini */
    while (iter->di_offset < iter->di_block_size) {
        status = ext2_dir_baca_entry(dir_data, iter->di_block,
            iter->di_offset, entry, iter->di_block_size);

        if (status != STATUS_BERHASIL) {
            iter->di_valid = SALAH;
            return status;
        }

        /* Update offset untuk next call */
        if (entry->rec_len == 0) {
            iter->di_valid = SALAH;
            return STATUS_KOSONG;
        }

        iter->di_offset += entry->rec_len;
        iter->di_valid = BENAR;

        /* Skip entry kosong */
        if (entry->inode != 0) {
            return STATUS_BERHASIL;
        }
    }

    iter->di_valid = SALAH;
    return STATUS_KOSONG;
}

/*
 * ext2_dir_iterator_reset
 * Reset iterator ke awal.
 */
status_t ext2_dir_iterator_reset(ext2_dir_iterator_t *iter)
{
    if (iter == NULL) {
        return STATUS_PARAM_NULL;
    }

    iter->di_block = 0;
    iter->di_offset = 0;
    iter->di_valid = SALAH;

    return STATUS_BERHASIL;
}
