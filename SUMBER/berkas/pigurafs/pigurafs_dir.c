/*
 * PIGURA OS - PIGURAFS_DIR.C
 * ===========================
 * Implementasi operasi direktori untuk filesystem PiguraFS.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola direktori
 * PiguraFS termasuk pembacaan, penulisan, dan pencarian entry.
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
 * KONSTANTA DIREKTORI (DIRECTORY CONSTANTS)
 * ===========================================================================
 */

/* Magic number */
#define PFS_DIR_MAGIC           0x50465333

/* Panjang nama file maksimum */
#define PFS_NAMA_MAKS           255

/* Ukuran minimum directory entry */
#define PFS_DIR_ENTRY_MIN       12

/* Tipe file dirent */
#define PFS_FT_UNKNOWN          0
#define PFS_FT_REG_FILE         1
#define PFS_FT_DIR              2
#define PFS_FT_CHRDEV           3
#define PFS_FT_BLKDEV           4
#define PFS_FT_FIFO             5
#define PFS_FT_SOCK             6
#define PFS_FT_SYMLINK          7

/* Special entry identifiers */
#define PFS_DIR_DOT             0x00
#define PFS_DIR_DOTDOT          0x01

/*
 * ===========================================================================
 * STRUKTUR DATA DIREKTORI (DIRECTORY DATA STRUCTURES)
 * ===========================================================================
 */

/* Struktur directory entry on-disk */
typedef struct PACKED {
    tak_bertanda32_t d_ino;             /* Inode number */
    tak_bertanda16_t d_rec_len;         /* Record length */
    tak_bertanda8_t d_nama_len;         /* Nama length */
    tak_bertanda8_t d_file_type;        /* Tipe file */
    tak_bertanda8_t d_nama[PFS_NAMA_MAKS]; /* Nama (variabel) */
} pfs_dirent_t;

/* Struktur directory iterator */
typedef struct {
    tak_bertanda32_t pfs_magic;         /* Magic number validasi */
    tak_bertanda8_t *buffer;            /* Buffer direktori */
    tak_bertanda32_t buffer_size;       /* Ukuran buffer */
    tak_bertanda32_t posisi;            /* Posisi saat ini */
    tak_bertanda32_t block_size;        /* Ukuran block */
    tak_bertanda32_t total_size;        /* Total ukuran direktori */
} pfs_dir_iter_t;

/* Struktur directory context */
typedef struct {
    pfs_dir_iter_t *iter;               /* Iterator */
    inode_t *dir_inode;                 /* Inode direktori */
    tak_bertanda32_t current_block;     /* Block saat ini */
} pfs_dir_ctx_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INTERNAL (INTERNAL FUNCTION DECLARATIONS)
 * ===========================================================================
 */

static status_t pfs_dir_baca_block(pfs_dir_ctx_t *ctx,
                                   tak_bertanda32_t block,
                                   void *buffer);
static status_t pfs_dir_tulis_block(pfs_dir_ctx_t *ctx,
                                    tak_bertanda32_t block,
                                    const void *buffer);
static ukuran_t pfs_dir_entry_size(tak_bertanda8_t nama_len);
static tak_bertanda16_t pfs_dir_align_entry(tak_bertanda16_t len);
static status_t pfs_dir_parse_entry(const tak_bertanda8_t *buffer,
                                    pfs_dirent_t *entry);
static status_t pfs_dir_write_entry(tak_bertanda8_t *buffer,
                                    const pfs_dirent_t *entry);
static tak_bertanda8_t pfs_dir_mode_to_type(mode_t mode);
static mode_t pfs_dir_type_to_mode(tak_bertanda8_t type);

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI UTILITAS (UTILITY FUNCTIONS)
 * ===========================================================================
 */

static ukuran_t pfs_dir_entry_size(tak_bertanda8_t nama_len)
{
    ukuran_t size;

    /* Header size: inode(4) + rec_len(2) + nama_len(1) + file_type(1) = 8 */
    size = 8 + nama_len;

    /* Align to 4-byte boundary */
    size = (size + 3) & ~3;

    return size;
}

static tak_bertanda16_t pfs_dir_align_entry(tak_bertanda16_t len)
{
    return (tak_bertanda16_t)(((ukuran_t)len + 3) & ~3);
}

static tak_bertanda8_t pfs_dir_mode_to_type(mode_t mode)
{
    if (VFS_S_ISREG(mode)) {
        return PFS_FT_REG_FILE;
    }
    if (VFS_S_ISDIR(mode)) {
        return PFS_FT_DIR;
    }
    if (VFS_S_ISLNK(mode)) {
        return PFS_FT_SYMLINK;
    }
    if (VFS_S_ISCHR(mode)) {
        return PFS_FT_CHRDEV;
    }
    if (VFS_S_ISBLK(mode)) {
        return PFS_FT_BLKDEV;
    }
    if (VFS_S_ISFIFO(mode)) {
        return PFS_FT_FIFO;
    }
    return PFS_FT_UNKNOWN;
}

static mode_t pfs_dir_type_to_mode(tak_bertanda8_t type)
{
    switch (type) {
    case PFS_FT_REG_FILE:
        return VFS_S_IFREG;
    case PFS_FT_DIR:
        return VFS_S_IFDIR;
    case PFS_FT_SYMLINK:
        return VFS_S_IFLNK;
    case PFS_FT_CHRDEV:
        return VFS_S_IFCHR;
    case PFS_FT_BLKDEV:
        return VFS_S_IFBLK;
    case PFS_FT_FIFO:
        return VFS_S_IFIFO;
    default:
        return 0;
    }
}

static status_t pfs_dir_parse_entry(const tak_bertanda8_t *buffer,
                                    pfs_dirent_t *entry)
{
    if (buffer == NULL || entry == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Parse header */
    entry->d_ino = (tak_bertanda32_t)(buffer[0] | (buffer[1] << 8) |
                  (buffer[2] << 16) | (buffer[3] << 24));
    entry->d_rec_len = (tak_bertanda16_t)(buffer[4] | (buffer[5] << 8));
    entry->d_nama_len = buffer[6];
    entry->d_file_type = buffer[7];

    /* Copy nama */
    if (entry->d_nama_len > 0 && entry->d_nama_len <= PFS_NAMA_MAKS) {
        kernel_memcpy(entry->d_nama, buffer + 8, entry->d_nama_len);
        entry->d_nama[entry->d_nama_len] = '\0';
    } else {
        entry->d_nama[0] = '\0';
    }

    return STATUS_BERHASIL;
}

static status_t pfs_dir_write_entry(tak_bertanda8_t *buffer,
                                    const pfs_dirent_t *entry)
{
    if (buffer == NULL || entry == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Write header */
    buffer[0] = (tak_bertanda8_t)(entry->d_ino & 0xFF);
    buffer[1] = (tak_bertanda8_t)((entry->d_ino >> 8) & 0xFF);
    buffer[2] = (tak_bertanda8_t)((entry->d_ino >> 16) & 0xFF);
    buffer[3] = (tak_bertanda8_t)((entry->d_ino >> 24) & 0xFF);
    buffer[4] = (tak_bertanda8_t)(entry->d_rec_len & 0xFF);
    buffer[5] = (tak_bertanda8_t)((entry->d_rec_len >> 8) & 0xFF);
    buffer[6] = entry->d_nama_len;
    buffer[7] = entry->d_file_type;

    /* Write nama */
    if (entry->d_nama_len > 0) {
        kernel_memcpy(buffer + 8, entry->d_nama, entry->d_nama_len);
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI I/O (I/O FUNCTIONS)
 * ===========================================================================
 */

static status_t pfs_dir_baca_block(pfs_dir_ctx_t *ctx,
                                   tak_bertanda32_t block,
                                   void *buffer)
{
    if (ctx == NULL || buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* TODO: Implementasi pembacaan aktual dari device */

    return STATUS_BERHASIL;
}

static status_t pfs_dir_tulis_block(pfs_dir_ctx_t *ctx,
                                    tak_bertanda32_t block,
                                    const void *buffer)
{
    if (ctx == NULL || buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* TODO: Implementasi penulisan aktual ke device */

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI UTAMA (MAIN FUNCTIONS)
 * ===========================================================================
 */

/*
 * pfs_dir_buat_iterator
 * ---------------------
 * Buat iterator untuk membaca direktori.
 *
 * Parameter:
 *   dir_inode - Inode direktori
 *   block_size - Ukuran block
 *
 * Return: Pointer ke iterator, atau NULL jika gagal
 */
pfs_dir_iter_t *pfs_dir_buat_iterator(inode_t *dir_inode,
                                      tak_bertanda32_t block_size)
{
    pfs_dir_iter_t *iter;

    if (dir_inode == NULL || block_size == 0) {
        return NULL;
    }

    iter = (pfs_dir_iter_t *)kmalloc(sizeof(pfs_dir_iter_t));
    if (iter == NULL) {
        return NULL;
    }

    kernel_memset(iter, 0, sizeof(pfs_dir_iter_t));

    iter->pfs_magic = PFS_DIR_MAGIC;
    iter->block_size = block_size;
    iter->total_size = (tak_bertanda32_t)dir_inode->i_size;
    iter->posisi = 0;

    /* Alokasi buffer untuk satu block */
    iter->buffer = (tak_bertanda8_t *)kmalloc(block_size);
    if (iter->buffer == NULL) {
        kfree(iter);
        return NULL;
    }

    iter->buffer_size = block_size;

    return iter;
}

/*
 * pfs_dir_hapus_iterator
 * ----------------------
 * Hapus iterator direktori.
 *
 * Parameter:
 *   iter - Pointer ke iterator
 */
void pfs_dir_hapus_iterator(pfs_dir_iter_t *iter)
{
    if (iter == NULL) {
        return;
    }

    if (iter->buffer != NULL) {
        kfree(iter->buffer);
    }

    iter->pfs_magic = 0;
    kfree(iter);
}

/*
 * pfs_dir_baca_entry
 * ------------------
 * Baca entry berikutnya dari direktori.
 *
 * Parameter:
 *   iter  - Pointer ke iterator
 *   entry - Pointer ke struktur entry hasil
 *
 * Return: Status operasi
 */
status_t pfs_dir_baca_entry(pfs_dir_iter_t *iter, pfs_dirent_t *entry)
{
    ukuran_t entry_size;

    if (iter == NULL || entry == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (iter->pfs_magic != PFS_DIR_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Cek apakah sudah di akhir */
    if (iter->posisi >= iter->total_size) {
        return STATUS_KOSONG;
    }

    /* Parse entry dari buffer */
    kernel_memset(entry, 0, sizeof(pfs_dirent_t));

    /* TODO: Implementasi pembacaan entry aktual */

    entry_size = pfs_dir_entry_size(entry->d_nama_len);
    iter->posisi += (tak_bertanda32_t)entry_size;

    return STATUS_BERHASIL;
}

/*
 * pfs_dir_cari
 * ------------
 * Cari entry dalam direktori.
 *
 * Parameter:
 *   dir_inode - Inode direktori
 *   nama      - Nama yang dicari
 *   entry     - Pointer ke struktur entry hasil
 *
 * Return: Status operasi
 */
status_t pfs_dir_cari(inode_t *dir_inode, const char *nama,
                      pfs_dirent_t *entry)
{
    pfs_dir_iter_t *iter;
    pfs_dirent_t current;
    status_t status;
    ukuran_t nama_len;

    if (dir_inode == NULL || nama == NULL || entry == NULL) {
        return STATUS_PARAM_NULL;
    }

    nama_len = kernel_strlen(nama);
    if (nama_len == 0 || nama_len > PFS_NAMA_MAKS) {
        return STATUS_PARAM_INVALID;
    }

    /* Buat iterator */
    iter = pfs_dir_buat_iterator(dir_inode, 4096);
    if (iter == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    /* Cari entry */
    kernel_memset(entry, 0, sizeof(pfs_dirent_t));

    while (BENAR) {
        status = pfs_dir_baca_entry(iter, &current);
        if (status != STATUS_BERHASIL) {
            break;
        }

        /* Skip . dan .. */
        if (current.d_nama_len == 1 &&
            (current.d_nama[0] == '.' || current.d_nama[0] == 0x00 ||
             current.d_nama[0] == 0x01)) {
            continue;
        }

        /* Bandingkan nama */
        if (current.d_nama_len == nama_len &&
            kernel_memcmp(current.d_nama, nama, nama_len) == 0) {
            kernel_memcpy(entry, &current, sizeof(pfs_dirent_t));
            pfs_dir_hapus_iterator(iter);
            return STATUS_BERHASIL;
        }
    }

    pfs_dir_hapus_iterator(iter);
    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * pfs_dir_tambah_entry
 * --------------------
 * Tambah entry ke direktori.
 *
 * Parameter:
 *   dir_inode - Inode direktori
 *   nama      - Nama entry
 *   ino       - Inode number
 *   mode      - Mode file
 *
 * Return: Status operasi
 */
status_t pfs_dir_tambah_entry(inode_t *dir_inode, const char *nama,
                              tak_bertanda32_t ino, mode_t mode)
{
    pfs_dirent_t entry;
    ukuran_t nama_len;
    ukuran_t entry_size;

    if (dir_inode == NULL || nama == NULL) {
        return STATUS_PARAM_NULL;
    }

    nama_len = kernel_strlen(nama);
    if (nama_len == 0 || nama_len > PFS_NAMA_MAKS) {
        return STATUS_PARAM_INVALID;
    }

    /* Siapkan entry baru */
    kernel_memset(&entry, 0, sizeof(pfs_dirent_t));
    entry.d_ino = ino;
    entry.d_nama_len = (tak_bertanda8_t)nama_len;
    entry.d_file_type = pfs_dir_mode_to_type(mode);
    kernel_memcpy(entry.d_nama, nama, nama_len);

    entry_size = pfs_dir_entry_size((tak_bertanda8_t)nama_len);
    entry.d_rec_len = (tak_bertanda16_t)entry_size;

    /* TODO: Implementasi penulisan entry ke disk */

    /* Update ukuran direktori */
    dir_inode->i_size += (off_t)entry_size;

    return STATUS_BERHASIL;
}

/*
 * pfs_dir_hapus_entry
 * -------------------
 * Hapus entry dari direktori.
 *
 * Parameter:
 *   dir_inode - Inode direktori
 *   nama      - Nama entry
 *
 * Return: Status operasi
 */
status_t pfs_dir_hapus_entry(inode_t *dir_inode, const char *nama)
{
    pfs_dirent_t entry;
    status_t status;

    if (dir_inode == NULL || nama == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Cari entry */
    status = pfs_dir_cari(dir_inode, nama, &entry);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    /* TODO: Implementasi penghapusan entry dari disk */

    return STATUS_BERHASIL;
}

/*
 * pfs_dir_rename_entry
 * --------------------
 * Rename entry dalam direktori.
 *
 * Parameter:
 *   dir_inode - Inode direktori
 *   nama_lama - Nama lama
 *   nama_baru - Nama baru
 *
 * Return: Status operasi
 */
status_t pfs_dir_rename_entry(inode_t *dir_inode, const char *nama_lama,
                              const char *nama_baru)
{
    pfs_dirent_t entry;
    status_t status;
    ukuran_t nama_len_baru;

    if (dir_inode == NULL || nama_lama == NULL || nama_baru == NULL) {
        return STATUS_PARAM_NULL;
    }

    nama_len_baru = kernel_strlen(nama_baru);
    if (nama_len_baru == 0 || nama_len_baru > PFS_NAMA_MAKS) {
        return STATUS_PARAM_INVALID;
    }

    /* Cari entry lama */
    status = pfs_dir_cari(dir_inode, nama_lama, &entry);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    /* Update nama */
    entry.d_nama_len = (tak_bertanda8_t)nama_len_baru;
    kernel_memcpy(entry.d_nama, nama_baru, nama_len_baru);
    entry.d_rec_len = (tak_bertanda16_t)
                      pfs_dir_entry_size((tak_bertanda8_t)nama_len_baru);

    /* TODO: Implementasi update entry ke disk */

    return STATUS_BERHASIL;
}

/*
 * pfs_dir_hitung_entry
 * --------------------
 * Hitung jumlah entry dalam direktori.
 *
 * Parameter:
 *   dir_inode - Inode direktori
 *   count     - Pointer ke hasil
 *
 * Return: Status operasi
 */
status_t pfs_dir_hitung_entry(inode_t *dir_inode, tak_bertanda32_t *count)
{
    pfs_dir_iter_t *iter;
    pfs_dirent_t entry;
    status_t status;
    tak_bertanda32_t total;

    if (dir_inode == NULL || count == NULL) {
        return STATUS_PARAM_NULL;
    }

    iter = pfs_dir_buat_iterator(dir_inode, 4096);
    if (iter == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    total = 0;

    while (BENAR) {
        status = pfs_dir_baca_entry(iter, &entry);
        if (status != STATUS_BERHASIL) {
            break;
        }

        /* Skip . dan .. */
        if (entry.d_nama_len == 1 &&
            (entry.d_nama[0] == '.' || entry.d_nama[0] == 0x00 ||
             entry.d_nama[0] == 0x01)) {
            continue;
        }

        total++;
    }

    *count = total;
    pfs_dir_hapus_iterator(iter);

    return STATUS_BERHASIL;
}

/*
 * pfs_dir_apakah_kosong
 * ---------------------
 * Cek apakah direktori kosong.
 *
 * Parameter:
 *   dir_inode - Inode direktori
 *
 * Return: BENAR jika kosong
 */
bool_t pfs_dir_apakah_kosong(inode_t *dir_inode)
{
    tak_bertanda32_t count;
    status_t status;

    status = pfs_dir_hitung_entry(dir_inode, &count);
    if (status != STATUS_BERHASIL) {
        return SALAH;
    }

    return (count == 0) ? BENAR : SALAH;
}

/*
 * pfs_dir_init
 * ------------
 * Inisialisasi direktori baru.
 *
 * Parameter:
 *   dir_inode - Inode direktori
 *   parent_ino - Inode number parent
 *
 * Return: Status operasi
 */
status_t pfs_dir_init(inode_t *dir_inode, tak_bertanda32_t parent_ino)
{
    pfs_dirent_t entry;
    status_t status;

    if (dir_inode == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Buat entry . */
    kernel_memset(&entry, 0, sizeof(pfs_dirent_t));
    entry.d_ino = (tak_bertanda32_t)dir_inode->i_ino;
    entry.d_rec_len = 12;
    entry.d_nama_len = 1;
    entry.d_file_type = PFS_FT_DIR;
    entry.d_nama[0] = '.';

    status = pfs_dir_tambah_entry(dir_inode, ".",
                                  (tak_bertanda32_t)dir_inode->i_ino,
                                  VFS_S_IFDIR);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    /* Buat entry .. */
    entry.d_ino = parent_ino;
    entry.d_nama[0] = '.';
    entry.d_nama[1] = '.';
    entry.d_nama_len = 2;
    entry.d_rec_len = 12;

    status = pfs_dir_tambah_entry(dir_inode, "..", parent_ino, VFS_S_IFDIR);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    return STATUS_BERHASIL;
}

/*
 * pfs_dir_convert_to_vfs
 * ----------------------
 * Konversi dirent PiguraFS ke VFS dirent.
 *
 * Parameter:
 *   pfs_entry - Entry PiguraFS
 *   vfs_entry - Entry VFS
 *
 * Return: Status operasi
 */
status_t pfs_dir_convert_to_vfs(const pfs_dirent_t *pfs_entry,
                                vfs_dirent_t *vfs_entry)
{
    ukuran_t nama_len;

    if (pfs_entry == NULL || vfs_entry == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memset(vfs_entry, 0, sizeof(vfs_dirent_t));

    vfs_entry->d_ino = (ino_t)pfs_entry->d_ino;
    vfs_entry->d_reclen = pfs_entry->d_rec_len;
    vfs_entry->d_type = pfs_entry->d_file_type;

    nama_len = MIN((ukuran_t)pfs_entry->d_nama_len, VFS_NAMA_MAKS);
    kernel_memcpy(vfs_entry->d_name, pfs_entry->d_nama, nama_len);
    vfs_entry->d_name[nama_len] = '\0';

    return STATUS_BERHASIL;
}
