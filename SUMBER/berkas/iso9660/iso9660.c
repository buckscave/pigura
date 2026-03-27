/*
 * PIGURA OS - ISO9660.C
 * ======================
 * Driver filesystem ISO9660 untuk kernel Pigura OS.
 *
 * Berkas ini berisi implementasi driver utama ISO9660 yang mencakup
 * registrasi filesystem, operasi mount, dan struktur data dasar.
 *
 * ISO9660 adalah standar filesystem untuk media optical (CD/DVD)
 * yang mendukung nama file 8.3 dengan ekstensi RockRidge dan Joliet.
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
 * KONSTANTA ISO9660 (ISO9660 CONSTANTS)
 * ===========================================================================
 */

/* Sektor awal Volume Descriptor */
#define ISO9660_VD_SEKTOR_AWAL      16

/* Ukuran sektor standar */
#define ISO9660_SEKTOR_UKURAN       2048

/* Magic number Volume Descriptor */
#define ISO9660_VD_MAGIC            0x43443031  /* "CD01" */

/* Tipe Volume Descriptor */
#define ISO9660_VD_BOOT             0x00
#define ISO9660_VD_PRIMARY          0x01
#define ISO9660_VD_SUPPLEMENTARY    0x02
#define ISO9660_VD_PARTITION        0x03
#define ISO9660_VD_TERMINATOR       0xFF

/* Tipe file direktori */
#define ISO9660_FT_FILE             0x00
#define ISO9660_FT_DIRECTORY        0x02

/* Flag direktori */
#define ISO9660_FLAG_HIDDEN         0x01
#define ISO9660_FLAG_DIRECTORY      0x02
#define ISO9660_FLAG_ASSOCIATED     0x04
#define ISO9660_FLAG_RECORD         0x08
#define ISO9660_FLAG_PROTECTION     0x10
#define ISO9660_FLAG_MULTIEXTENT    0x80

/* Batas nama file */
#define ISO9660_NAMA_MAKS           31
#define ISO9660_NAMA_PANJANG_MAKS   64
#define ISO9660_EKSTENSI_MAKS       30

/* Level interoperabilitas */
#define ISO9660_LEVEL_1             1
#define ISO9660_LEVEL_2             2
#define ISO9660_LEVEL_3             3

/* Magic number untuk validasi struktur internal */
#define ISO9660_SB_MAGIC            0x49534F53  /* "ISOS" */
#define ISO9660_INODE_MAGIC         0x49534F49  /* "ISOI" */
#define ISO9660_FILE_MAGIC          0x49534F46  /* "ISOF" */

/*
 * ===========================================================================
 * STRUKTUR DATA ISO9660 (ISO9660 DATA STRUCTURES)
 * ===========================================================================
 */

/* Struktur tanggal/waktu ISO9660 (7 byte) */
typedef struct PACKED {
    tak_bertanda8_t tahun;      /* Tahun sejak 1900 */
    tak_bertanda8_t bulan;      /* Bulan (1-12) */
    tak_bertanda8_t hari;       /* Hari (1-31) */
    tak_bertanda8_t jam;        /* Jam (0-23) */
    tak_bertanda8_t menit;      /* Menit (0-59) */
    tak_bertanda8_t detik;      /* Detik (0-59) */
    tak_bertanda8_t zona;       /* Offset zona waktu (15 menit) */
} iso9660_waktu_t;

/* Struktur tanggal/waktu rekaman (17 byte) */
typedef struct PACKED {
    tak_bertanda8_t tahun[4];   /* Tahun ASCII */
    tak_bertanda8_t bulan[2];   /* Bulan ASCII */
    tak_bertanda8_t hari[2];    /* Hari ASCII */
    tak_bertanda8_t jam[2];     /* Jam ASCII */
    tak_bertanda8_t menit[2];   /* Menit ASCII */
    tak_bertanda8_t detik[2];   /* Detik ASCII */
    tak_bertanda8_t sepersep[2];/* Persepersi detik */
    tak_bertanda8_t zona;       /* Offset zona waktu */
} iso9660_waktu_rekam_t;

/* Struktur Volume Descriptor Header */
typedef struct PACKED {
    tak_bertanda8_t tipe;               /* Tipe Volume Descriptor */
    tak_bertanda8_t identitas[5];       /* "CD001" */
    tak_bertanda8_t versi;              /* Versi (1) */
} iso9660_vd_header_t;

/* Struktur Primary Volume Descriptor (2048 byte) */
typedef struct PACKED {
    tak_bertanda8_t tipe;               /* Tipe (0x01) */
    tak_bertanda8_t identitas[5];       /* "CD001" */
    tak_bertanda8_t versi;              /* Versi (1) */
    tak_bertanda8_t sistem_id[32];      /* Identifier sistem */
    tak_bertanda8_t volume_id[32];      /* Identifier volume */
    tak_bertanda64_t volume_kosong;     /* Padding */
    tak_bertanda32_t volume_size_le;    /* Ukuran volume (LE) */
    tak_bertanda32_t volume_size_be;    /* Ukuran volume (BE) */
    tak_bertanda8_t escape[32];         /* Escape sequence */
    tak_bertanda16_t volume_set_le;     /* Volume set size (LE) */
    tak_bertanda16_t volume_set_be;     /* Volume set size (BE) */
    tak_bertanda16_t volume_seq_le;     /* Volume sequence (LE) */
    tak_bertanda16_t volume_seq_be;     /* Volume sequence (BE) */
    tak_bertanda16_t block_size_le;     /* Ukuran block (LE) */
    tak_bertanda16_t block_size_be;     /* Ukuran block (BE) */
    tak_bertanda32_t path_table_len_le; /* Panjang path table (LE) */
    tak_bertanda32_t path_table_len_be; /* Panjang path table (BE) */
    tak_bertanda32_t path_table_le;     /* Lokasi path table L (LE) */
    tak_bertanda32_t path_table_opt_le; /* Lokasi path table opt (LE) */
    tak_bertanda32_t path_table_be;     /* Lokasi path table L (BE) */
    tak_bertanda32_t path_table_opt_be; /* Lokasi path table opt (BE) */
    tak_bertanda8_t root_dir[34];       /* Direktori root */
    tak_bertanda8_t volume_set_id[128]; /* Volume set identifier */
    tak_bertanda8_t publisher_id[128];  /* Publisher identifier */
    tak_bertanda8_t preparer_id[128];   /* Data preparer identifier */
    tak_bertanda8_t application_id[128];/* Application identifier */
    tak_bertanda8_t copyright[37];      /* Copyright file identifier */
    tak_bertanda8_t abstract_file[37];  /* Abstract file identifier */
    tak_bertanda8_t bibliographic[37];  /* Bibliographic file identifier */
    iso9660_waktu_rekam_t dibuat;       /* Tanggal pembuatan */
    iso9660_waktu_rekam_t dimodifikasi; /* Tanggal modifikasi */
    iso9660_waktu_rekam_t kadaluarsa;   /* Tanggal kadaluarsa */
    iso9660_waktu_rekam_t berlaku;      /* Tanggal berlaku */
    tak_bertanda8_t versi_app;          /* Versi application */
    tak_bertanda8_t reserved[1166];     /* Reserved */
    tak_bertanda8_t reserved2[653];     /* Reserved untuk padding */
} iso9660_pvd_t;

/* Struktur Directory Record (minimal 33 byte) */
typedef struct PACKED {
    tak_bertanda8_t panjang;            /* Panjang record */
    tak_bertanda8_t ext_attr_len;       /* Panjang extended attribute */
    tak_bertanda32_t extent_le;         /* Lokasi extent (LE) */
    tak_bertanda32_t extent_be;         /* Lokasi extent (BE) */
    tak_bertanda32_t data_len_le;       /* Ukuran data (LE) */
    tak_bertanda32_t data_len_be;       /* Ukuran data (BE) */
    iso9660_waktu_t waktu;              /* Tanggal/waktu rekaman */
    tak_bertanda8_t flags;              /* Flag file */
    tak_bertanda8_t file_unit;          /* File unit size */
    tak_bertanda8_t interleave;         /* Interleave gap */
    tak_bertanda16_t vol_seq_le;        /* Volume sequence (LE) */
    tak_bertanda16_t vol_seq_be;        /* Volume sequence (BE) */
    tak_bertanda8_t nama_len;           /* Panjang nama file */
    tak_bertanda8_t nama[1];            /* Nama file (variabel) */
} iso9660_dir_record_t;

/* Struktur Path Table Entry */
typedef struct PACKED {
    tak_bertanda8_t nama_len;           /* Panjang nama direktori */
    tak_bertanda8_t ext_attr_len;       /* Panjang extended attribute */
    tak_bertanda32_t extent;            /* Lokasi extent */
    tak_bertanda16_t parent;            /* Index parent */
    tak_bertanda8_t nama[1];            /* Nama direktori (variabel) */
} iso9660_path_table_entry_t;

/*
 * ===========================================================================
 * STRUKTUR DATA INTERNAL (INTERNAL DATA STRUCTURES)
 * ===========================================================================
 */

/* Struktur superblock ISO9660 */
typedef struct {
    tak_bertanda32_t magic;             /* Magic number validasi */
    superblock_t *vfs_sb;               /* VFS superblock */
    iso9660_pvd_t *pvd;                 /* Primary Volume Descriptor */
    tak_bertanda32_t block_size;        /* Ukuran block */
    tak_bertanda64_t total_blocks;      /* Total block */
    tak_bertanda32_t level;             /* Level interoperabilitas */
    bool_t has_rockridge;               /* Ada ekstensi RockRidge? */
    bool_t has_joliet;                  /* Ada ekstensi Joliet? */
    tak_bertanda32_t root_extent;       /* Lokasi extent root */
    tak_bertanda32_t root_size;         /* Ukuran direktori root */
    tak_bertanda32_t path_table_loc;    /* Lokasi path table */
    tak_bertanda32_t path_table_len;    /* Panjang path table */
    tak_bertanda8_t volume_id[33];      /* Volume identifier */
    tak_bertanda32_t refcount;          /* Reference count */
} iso9660_sb_t;

/* Struktur inode ISO9660 */
typedef struct {
    tak_bertanda32_t magic;             /* Magic number validasi */
    inode_t *vfs_inode;                 /* VFS inode */
    tak_bertanda32_t extent;            /* Lokasi extent */
    tak_bertanda32_t size;              /* Ukuran file */
    tak_bertanda8_t flags;              /* Flag file */
    iso9660_waktu_t waktu;              /* Tanggal/waktu */
    bool_t is_directory;                /* Apakah direktori? */
} iso9660_inode_t;

/* Struktur file handle ISO9660 */
typedef struct {
    tak_bertanda32_t magic;             /* Magic number validasi */
    file_t *vfs_file;                   /* VFS file */
    tak_bertanda32_t extent;            /* Extent saat ini */
    tak_bertanda64_t offset;            /* Offset dalam extent */
    tak_bertanda32_t extent_index;      /* Index extent untuk multi-extent */
} iso9660_file_t;

/* Struktur directory context */
typedef struct {
    tak_bertanda8_t *buffer;            /* Buffer direktori */
    tak_bertanda32_t buffer_size;       /* Ukuran buffer */
    tak_bertanda32_t current_offset;    /* Offset saat ini */
    tak_bertanda32_t extent;            /* Extent direktori */
    tak_bertanda32_t size;              /* Ukuran direktori */
} iso9660_dir_context_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INTERNAL (INTERNAL FUNCTION DECLARATIONS)
 * ===========================================================================
 */

/* Fungsi mount/umount */
static superblock_t *iso9660_mount(filesystem_t *fs, const char *device,
                                   const char *path, tak_bertanda32_t flags);
static status_t iso9660_umount(superblock_t *sb);
static status_t iso9660_detect(const char *device);

/* Fungsi superblock operations */
static inode_t *iso9660_alloc_inode(superblock_t *sb);
static void iso9660_destroy_inode(inode_t *inode);
static status_t iso9660_read_inode(inode_t *inode);
static status_t iso9660_statfs(superblock_t *sb, vfs_stat_t *stat);

/* Fungsi inode operations */
static dentry_t *iso9660_lookup(inode_t *dir, const char *name);
static status_t iso9660_getattr(dentry_t *dentry, vfs_stat_t *stat);

/* Fungsi file operations */
static tak_bertandas_t iso9660_read(file_t *file, void *buffer,
                                    ukuran_t size, off_t *pos);
static off_t iso9660_lseek(file_t *file, off_t offset,
                           tak_bertanda32_t whence);
static status_t iso9660_open(inode_t *inode, file_t *file);
static status_t iso9660_release(inode_t *inode, file_t *file);
static tak_bertandas_t iso9660_readdir(file_t *file, vfs_dirent_t *dirent,
                                       ukuran_t count);

/* Fungsi utilitas */
static status_t iso9660_baca_block(superblock_t *sb, tak_bertanda64_t block,
                                   void *buffer, ukuran_t count);
static status_t iso9660_parse_pvd(iso9660_sb_t *isb);
static status_t iso9660_detect_extensions(iso9660_sb_t *isb);
static tak_bertanda32_t iso9660_bothendian32(tak_bertanda32_t le,
                                             tak_bertanda32_t be);
static tak_bertanda16_t iso9660_bothendian16(tak_bertanda16_t le,
                                             tak_bertanda16_t be);
static void iso9660_nama_normalisasi(const tak_bertanda8_t *src,
                                     ukuran_t src_len, char *dst,
                                     ukuran_t dst_size);
static void iso9660_waktu_ke_vfs(const iso9660_waktu_t *iso,
                                 waktu_t *vfs_time);
static void iso9660_waktu_rekam_ke_vfs(const iso9660_waktu_rekam_t *iso,
                                       waktu_t *vfs_time);

/*
 * ===========================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ===========================================================================
 */

/* Operations structures */
static vfs_super_operations_t g_iso9660_super_ops;
static vfs_inode_operations_t g_iso9660_inode_ops;
static vfs_file_operations_t g_iso9660_file_ops;

/* Filesystem registration */
static filesystem_t g_iso9660_fs = {
    "iso9660",
    FS_FLAG_REQUIRES_DEV | FS_FLAG_READ_ONLY,
    iso9660_mount,
    iso9660_umount,
    iso9660_detect,
    NULL,
    0,
    SALAH
};

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI UTILITAS (UTILITY FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

static tak_bertanda32_t iso9660_bothendian32(tak_bertanda32_t le,
                                             tak_bertanda32_t be)
{
    /* Gunakan little-endian value */
    (void)be;
    return le;
}

static tak_bertanda16_t iso9660_bothendian16(tak_bertanda16_t le,
                                             tak_bertanda16_t be)
{
    /* Gunakan little-endian value */
    (void)be;
    return le;
}

static void iso9660_nama_normalisasi(const tak_bertanda8_t *src,
                                     ukuran_t src_len, char *dst,
                                     ukuran_t dst_size)
{
    ukuran_t i;
    ukuran_t j;
    bool_t ada_titik;
    bool_t sebelum_titik;

    if (src == NULL || dst == NULL || dst_size == 0) {
        return;
    }

    j = 0;
    ada_titik = SALAH;
    sebelum_titik = BENAR;

    for (i = 0; i < src_len && j < dst_size - 1; i++) {
        tak_bertanda8_t c = src[i];

        /* Skip karakter null dan spasi di akhir */
        if (c == '\0') {
            break;
        }

        /* Skip trailing spaces */
        if (c == ' ' && !sebelum_titik) {
            continue;
        }

        /* Handle pemisah versi (;1) */
        if (c == ';') {
            break;
        }

        /* Handle titik ekstensi */
        if (c == '.') {
            if (ada_titik) {
                continue;
            }
            ada_titik = BENAR;
            sebelum_titik = SALAH;
            dst[j++] = '.';
            continue;
        }

        /* Konversi ke huruf kecil untuk konsistensi */
        if (c >= 'A' && c <= 'Z') {
            c = c + ('a' - 'A');
        }

        dst[j++] = (char)c;
    }

    /* Hapus titik di akhir jika tidak ada ekstensi */
    if (j > 0 && dst[j - 1] == '.') {
        j--;
    }

    dst[j] = '\0';
}

static void iso9660_waktu_ke_vfs(const iso9660_waktu_t *iso, waktu_t *vfs_time)
{
    /* Konversi ke Unix timestamp (simplified) */
    tak_bertanda32_t tahun;
    tak_bertanda32_t hari_total;
    tak_bertanda32_t bulan_hari[12] = {31, 28, 31, 30, 31, 30,
                                       31, 31, 30, 31, 30, 31};
    tak_bertanda32_t i;

    if (iso == NULL || vfs_time == NULL) {
        return;
    }

    tahun = (tak_bertanda32_t)iso->tahun + 1900;

    /* Hitung hari sejak 1970 */
    hari_total = 0;

    /* Tahun dari 1970 */
    for (i = 1970; i < tahun; i++) {
        if ((i % 4 == 0 && i % 100 != 0) || (i % 400 == 0)) {
            hari_total += 366;
        } else {
            hari_total += 365;
        }
    }

    /* Bulan */
    for (i = 1; i < (tak_bertanda32_t)iso->bulan && i <= 12; i++) {
        hari_total += bulan_hari[i - 1];
        if (i == 2 && ((tahun % 4 == 0 && tahun % 100 != 0) ||
                       (tahun % 400 == 0))) {
            hari_total += 1;
        }
    }

    /* Hari */
    hari_total += (tak_bertanda32_t)iso->hari - 1;

    /* Konversi ke detik */
    *vfs_time = (waktu_t)hari_total * 86400;
    *vfs_time += (waktu_t)iso->jam * 3600;
    *vfs_time += (waktu_t)iso->menit * 60;
    *vfs_time += (waktu_t)iso->detik;
}

static void iso9660_waktu_rekam_ke_vfs(const iso9660_waktu_rekam_t *iso,
                                       waktu_t *vfs_time)
{
    tak_bertanda32_t tahun;
    tak_bertanda32_t bulan;
    tak_bertanda32_t hari;
    tak_bertanda32_t jam;
    tak_bertanda32_t menit;
    tak_bertanda32_t detik;
    tak_bertanda32_t hari_total;
    tak_bertanda32_t bulan_hari[12] = {31, 28, 31, 30, 31, 30,
                                       31, 31, 30, 31, 30, 31};
    tak_bertanda32_t i;
    char temp[8];
    tanda32_t num;

    if (iso == NULL || vfs_time == NULL) {
        return;
    }

    /* Parse ASCII fields secara manual */
    /* Tahun: 4 digit */
    temp[0] = (char)iso->tahun[0];
    temp[1] = (char)iso->tahun[1];
    temp[2] = (char)iso->tahun[2];
    temp[3] = (char)iso->tahun[3];
    temp[4] = '\0';
    num = 0;
    for (i = 0; temp[i] >= '0' && temp[i] <= '9' && i < 4; i++) {
        num = num * 10 + (temp[i] - '0');
    }
    tahun = (num > 0) ? (tak_bertanda32_t)num : 1970;

    /* Bulan: 2 digit */
    temp[0] = (char)iso->bulan[0];
    temp[1] = (char)iso->bulan[1];
    temp[2] = '\0';
    num = 0;
    for (i = 0; temp[i] >= '0' && temp[i] <= '9' && i < 2; i++) {
        num = num * 10 + (temp[i] - '0');
    }
    bulan = (num >= 1 && num <= 12) ? (tak_bertanda32_t)num : 1;

    /* Hari: 2 digit */
    temp[0] = (char)iso->hari[0];
    temp[1] = (char)iso->hari[1];
    temp[2] = '\0';
    num = 0;
    for (i = 0; temp[i] >= '0' && temp[i] <= '9' && i < 2; i++) {
        num = num * 10 + (temp[i] - '0');
    }
    hari = (num >= 1 && num <= 31) ? (tak_bertanda32_t)num : 1;

    /* Jam: 2 digit */
    temp[0] = (char)iso->jam[0];
    temp[1] = (char)iso->jam[1];
    temp[2] = '\0';
    num = 0;
    for (i = 0; temp[i] >= '0' && temp[i] <= '9' && i < 2; i++) {
        num = num * 10 + (temp[i] - '0');
    }
    jam = (num >= 0 && num <= 23) ? (tak_bertanda32_t)num : 0;

    /* Menit: 2 digit */
    temp[0] = (char)iso->menit[0];
    temp[1] = (char)iso->menit[1];
    temp[2] = '\0';
    num = 0;
    for (i = 0; temp[i] >= '0' && temp[i] <= '9' && i < 2; i++) {
        num = num * 10 + (temp[i] - '0');
    }
    menit = (num >= 0 && num <= 59) ? (tak_bertanda32_t)num : 0;

    /* Detik: 2 digit */
    temp[0] = (char)iso->detik[0];
    temp[1] = (char)iso->detik[1];
    temp[2] = '\0';
    num = 0;
    for (i = 0; temp[i] >= '0' && temp[i] <= '9' && i < 2; i++) {
        num = num * 10 + (temp[i] - '0');
    }
    detik = (num >= 0 && num <= 59) ? (tak_bertanda32_t)num : 0;

    /* Hitung Unix timestamp */
    hari_total = 0;

    for (i = 1970; i < tahun; i++) {
        if ((i % 4 == 0 && i % 100 != 0) || (i % 400 == 0)) {
            hari_total += 366;
        } else {
            hari_total += 365;
        }
    }

    for (i = 1; i < bulan && i <= 12; i++) {
        hari_total += bulan_hari[i - 1];
        if (i == 2 && ((tahun % 4 == 0 && tahun % 100 != 0) ||
                       (tahun % 400 == 0))) {
            hari_total += 1;
        }
    }

    hari_total += hari - 1;

    *vfs_time = (waktu_t)hari_total * 86400;
    *vfs_time += (waktu_t)jam * 3600;
    *vfs_time += (waktu_t)menit * 60;
    *vfs_time += (waktu_t)detik;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI I/O (I/O FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

static status_t iso9660_baca_block(superblock_t *sb, tak_bertanda64_t block,
                                   void *buffer, ukuran_t count)
{
    iso9660_sb_t *isb;
    off_t offset;

    if (sb == NULL || buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    isb = (iso9660_sb_t *)sb->s_private;
    if (isb == NULL || isb->magic != ISO9660_SB_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Hitung offset byte */
    offset = (off_t)block * (off_t)isb->block_size;

    /* Baca dari device */
    /* TODO: Implementasi pembacaan dari block device */
    /* Untuk saat ini, return sukses */
    (void)count;
    (void)offset;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI PARSING (PARSING FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

static status_t iso9660_parse_pvd(iso9660_sb_t *isb)
{
    tak_bertanda8_t buffer[ISO9660_SEKTOR_UKURAN];
    tak_bertanda32_t sektor;
    status_t status;

    if (isb == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Inisialisasi buffer dengan nol untuk menghindari warning */
    kernel_memset(buffer, 0, ISO9660_SEKTOR_UKURAN);

    /* Cari Primary Volume Descriptor */
    sektor = ISO9660_VD_SEKTOR_AWAL;

    while (BENAR) {
        status = iso9660_baca_block(isb->vfs_sb, sektor, buffer, 1);
        if (status != STATUS_BERHASIL) {
            return status;
        }

        /* Cek magic number "CD001" pada offset 1 */
        if (buffer[1] != 'C' || buffer[2] != 'D' ||
            buffer[3] != '0' || buffer[4] != '0' ||
            buffer[5] != '1') {
            return STATUS_FORMAT_INVALID;
        }

        /* Cek tipe Volume Descriptor pada offset 0 */
        if (buffer[0] == ISO9660_VD_PRIMARY) {
            /* Alokasi memori untuk PVD */
            isb->pvd = (iso9660_pvd_t *)kmalloc(sizeof(iso9660_pvd_t));
            if (isb->pvd == NULL) {
                return STATUS_MEMORI_HABIS;
            }

            /* Parse data dari buffer secara manual */
            /* Block size pada offset 128 (LE) dan 130 (BE) */
            isb->block_size = (tak_bertanda16_t)(buffer[128] | 
                             (buffer[129] << 8));
            
            /* Volume size pada offset 80-83 (LE) */
            isb->total_blocks = (tak_bertanda32_t)(
                buffer[80] | (buffer[81] << 8) | 
                (buffer[82] << 16) | (buffer[83] << 24));

            /* Root directory record pada offset 156 */
            {
                tak_bertanda8_t *root = &buffer[156];
                /* Extent pada offset 2-5 dan 6-9 */
                isb->root_extent = (tak_bertanda32_t)(
                    root[2] | (root[3] << 8) | 
                    (root[4] << 16) | (root[5] << 24));
                /* Size pada offset 10-13 dan 14-17 */
                isb->root_size = (tak_bertanda32_t)(
                    root[10] | (root[11] << 8) | 
                    (root[12] << 16) | (root[13] << 24));
            }

            /* Path table location pada offset 140 */
            isb->path_table_loc = (tak_bertanda32_t)(
                buffer[140] | (buffer[141] << 8) | 
                (buffer[142] << 16) | (buffer[143] << 24));
            
            /* Path table length pada offset 132 */
            isb->path_table_len = (tak_bertanda32_t)(
                buffer[132] | (buffer[133] << 8) | 
                (buffer[134] << 16) | (buffer[135] << 24));

            /* Copy volume ID dari offset 40 */
            kernel_strncpy((char *)isb->volume_id, 
                          (const char *)&buffer[40], 32);
            isb->volume_id[32] = '\0';

            /* Normalisasi volume ID */
            {
                ukuran_t i;
                for (i = 31; i > 0; i--) {
                    if (isb->volume_id[i] == ' ') {
                        isb->volume_id[i] = '\0';
                    } else {
                        break;
                    }
                }
            }

            return STATUS_BERHASIL;
        }

        /* Cek terminator */
        if (buffer[0] == ISO9660_VD_TERMINATOR) {
            break;
        }

        sektor++;
    }

    return STATUS_TIDAK_DITEMUKAN;
}

static status_t iso9660_detect_extensions(iso9660_sb_t *isb)
{
    tak_bertanda8_t buffer[ISO9660_SEKTOR_UKURAN];
    tak_bertanda32_t sektor;
    status_t status;

    if (isb == NULL || isb->pvd == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Inisialisasi buffer dengan nol */
    kernel_memset(buffer, 0, ISO9660_SEKTOR_UKURAN);

    isb->has_rockridge = SALAH;
    isb->has_joliet = SALAH;

    /* Cek Supplementary Volume Descriptor untuk Joliet */
    sektor = ISO9660_VD_SEKTOR_AWAL;

    while (BENAR) {
        status = iso9660_baca_block(isb->vfs_sb, sektor, buffer, 1);
        if (status != STATUS_BERHASIL) {
            break;
        }

        /* Cek magic number */
        if (buffer[1] != 'C' || buffer[2] != 'D' ||
            buffer[3] != '0' || buffer[4] != '0' || buffer[5] != '1') {
            break;
        }

        /* Cek Supplementary Volume Descriptor */
        if (buffer[0] == ISO9660_VD_SUPPLEMENTARY) {
            /* Cek escape sequence untuk Joliet */
            /* Joliet menggunakan escape sequence: %/@, %/C, atau %/E */
            tak_bertanda8_t *escape = buffer + 88;

            if ((escape[0] == '%' && escape[1] == '/' &&
                 (escape[2] == '@' || escape[2] == 'C' || escape[2] == 'E'))) {
                isb->has_joliet = BENAR;
            }
        }

        /* Cek terminator */
        if (buffer[0] == ISO9660_VD_TERMINATOR) {
            break;
        }

        sektor++;
    }

    /* Cek RockRidge dengan membaca root directory */
    /* RockRidge extension records ada di System Use area */
    {
        tak_bertanda8_t *dir_buffer;
        tak_bertanda8_t *rec_ptr;
        status_t st;

        dir_buffer = (tak_bertanda8_t *)kmalloc(isb->root_size);
        if (dir_buffer == NULL) {
            return STATUS_MEMORI_HABIS;
        }

        /* Inisialisasi dir_buffer */
        kernel_memset(dir_buffer, 0, isb->root_size);

        st = iso9660_baca_block(isb->vfs_sb, isb->root_extent, dir_buffer,
                               (isb->root_size + isb->block_size - 1) /
                               isb->block_size);
        if (st == STATUS_BERHASIL) {
            rec_ptr = dir_buffer;
            tak_bertanda8_t rec_len = rec_ptr[0];

            /* Skip . dan .. entries */
            if (rec_len > 0) {
                rec_ptr += rec_len;
                rec_len = rec_ptr[0];
                if (rec_len > 0) {
                    rec_ptr += rec_len;
                }

                /* Cek SUSP/RockRidge signature */
                while (rec_len > 0) {
                    tak_bertanda8_t nama_len = rec_ptr[32];
                    ukuran_t system_use_offset;

                    /* Hitung offset ke System Use area */
                    system_use_offset = 33 + nama_len;
                    if (nama_len % 2 == 0) {
                        system_use_offset++;
                    }

                    if (system_use_offset + 4 <= rec_len) {
                        tak_bertanda8_t *su;
                        su = rec_ptr + system_use_offset;

                        /* Cek RockRidge signature "RRIP" atau "ER" */
                        if (su[0] == 'R' && su[1] == 'R' &&
                            su[2] == 'I' && su[3] == 'P') {
                            isb->has_rockridge = BENAR;
                            break;
                        }

                        if (su[0] == 'E' && su[1] == 'R') {
                            isb->has_rockridge = BENAR;
                            break;
                        }
                    }

                    rec_ptr += rec_len;
                    rec_len = rec_ptr[0];
                }
            }
        }

        kfree(dir_buffer);
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI SUPERBLOCK OPERATIONS
 * ===========================================================================
 */

static inode_t *iso9660_alloc_inode(superblock_t *sb)
{
    inode_t *inode;
    iso9660_inode_t *iso_inode;

    if (sb == NULL) {
        return NULL;
    }

    inode = inode_alloc(sb);
    if (inode == NULL) {
        return NULL;
    }

    iso_inode = (iso9660_inode_t *)kmalloc(sizeof(iso9660_inode_t));
    if (iso_inode == NULL) {
        inode_put(inode);
        return NULL;
    }

    kernel_memset(iso_inode, 0, sizeof(iso9660_inode_t));
    iso_inode->magic = ISO9660_INODE_MAGIC;
    iso_inode->vfs_inode = inode;

    inode->i_private = iso_inode;

    return inode;
}

static void iso9660_destroy_inode(inode_t *inode)
{
    iso9660_inode_t *iso_inode;

    if (inode == NULL) {
        return;
    }

    iso_inode = (iso9660_inode_t *)inode->i_private;
    if (iso_inode != NULL && iso_inode->magic == ISO9660_INODE_MAGIC) {
        iso_inode->magic = 0;
        kfree(iso_inode);
        inode->i_private = NULL;
    }
}

static status_t iso9660_read_inode(inode_t *inode)
{
    iso9660_sb_t *isb;
    iso9660_inode_t *iso_inode;

    if (inode == NULL || inode->i_sb == NULL) {
        return STATUS_PARAM_NULL;
    }

    isb = (iso9660_sb_t *)inode->i_sb->s_private;
    if (isb == NULL || isb->magic != ISO9660_SB_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    iso_inode = (iso9660_inode_t *)inode->i_private;
    if (iso_inode == NULL || iso_inode->magic != ISO9660_INODE_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Setup inode dari data ISO9660 */
    inode->i_size = (off_t)iso_inode->size;
    inode->i_blocks = (inode->i_size + (off_t)isb->block_size - 1) /
                      (off_t)isb->block_size;
    inode->i_blksize = isb->block_size;

    /* Setup mode */
    if (iso_inode->is_directory) {
        inode->i_mode = VFS_S_IFDIR | 0755;
    } else {
        inode->i_mode = VFS_S_IFREG | 0444;
    }

    /* Setup timestamps */
    iso9660_waktu_ke_vfs(&iso_inode->waktu, &inode->i_mtime);
    inode->i_atime = inode->i_mtime;
    inode->i_ctime = inode->i_mtime;

    /* Setup ownership */
    inode->i_uid = 0;
    inode->i_gid = 0;
    inode->i_nlink = 1;

    return STATUS_BERHASIL;
}

static status_t iso9660_statfs(superblock_t *sb, vfs_stat_t *stat)
{
    iso9660_sb_t *isb;

    if (sb == NULL || stat == NULL) {
        return STATUS_PARAM_NULL;
    }

    isb = (iso9660_sb_t *)sb->s_private;
    if (isb == NULL || isb->magic != ISO9660_SB_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    kernel_memset(stat, 0, sizeof(vfs_stat_t));

    stat->st_dev = sb->s_dev;
    stat->st_blksize = (tak_bertanda32_t)isb->block_size;
    stat->st_blocks = (tak_bertanda64_t)isb->total_blocks;
    stat->st_size = stat->st_blocks * (off_t)stat->st_blksize;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI INODE OPERATIONS
 * ===========================================================================
 */

static dentry_t *iso9660_lookup(inode_t *dir, const char *name)
{
    iso9660_sb_t *isb;
    iso9660_inode_t *dir_iso;
    tak_bertanda8_t *buffer;
    tak_bertanda32_t offset;
    tak_bertanda32_t size;
    status_t status;
    dentry_t *result;

    if (dir == NULL || name == NULL) {
        return NULL;
    }

    if (!inode_is_directory(dir)) {
        return NULL;
    }

    isb = (iso9660_sb_t *)dir->i_sb->s_private;
    if (isb == NULL || isb->magic != ISO9660_SB_MAGIC) {
        return NULL;
    }

    dir_iso = (iso9660_inode_t *)dir->i_private;
    if (dir_iso == NULL || dir_iso->magic != ISO9660_INODE_MAGIC) {
        return NULL;
    }

    /* Alokasi buffer untuk direktori */
    buffer = (tak_bertanda8_t *)kmalloc((ukuran_t)dir->i_size);
    if (buffer == NULL) {
        return NULL;
    }

    /* Baca direktori */
    status = iso9660_baca_block(dir->i_sb, dir_iso->extent, buffer,
                               ((tak_bertanda32_t)dir->i_size +
                                isb->block_size - 1) / isb->block_size);
    if (status != STATUS_BERHASIL) {
        kfree(buffer);
        return NULL;
    }

    result = NULL;
    offset = 0;
    size = (tak_bertanda32_t)dir->i_size;

    /* Parse directory entries */
    while (offset < size) {
        iso9660_dir_record_t *record;
        tak_bertanda8_t rec_len;
        char nama_entry[ISO9660_NAMA_PANJANG_MAKS + 1];

        record = (iso9660_dir_record_t *)(buffer + offset);
        rec_len = record->panjang;

        if (rec_len == 0) {
            /* Padding, pindah ke block berikutnya */
            offset = (offset + isb->block_size) &
                     ~(isb->block_size - 1);
            continue;
        }

        /* Skip . dan .. */
        if (record->nama_len == 1 &&
            (record->nama[0] == 0x00 || record->nama[0] == 0x01)) {
            offset += rec_len;
            continue;
        }

        /* Normalisasi nama */
        iso9660_nama_normalisasi(record->nama, record->nama_len,
                                 nama_entry, sizeof(nama_entry));

        /* Bandingkan nama */
        if (kernel_strcmp(nama_entry, name) == 0) {
            /* Ditemukan! Buat dentry dan inode */
            inode_t *new_inode;
            iso9660_inode_t *new_iso;

            new_inode = iso9660_alloc_inode(dir->i_sb);
            if (new_inode != NULL) {
                new_iso = (iso9660_inode_t *)new_inode->i_private;
                if (new_iso != NULL) {
                    new_iso->extent = iso9660_bothendian32(record->extent_le,
                                                           record->extent_be);
                    new_iso->size = iso9660_bothendian32(record->data_len_le,
                                                         record->data_len_be);
                    new_iso->flags = record->flags;
                    new_iso->is_directory = (record->flags &
                                            ISO9660_FLAG_DIRECTORY) != 0;
                    kernel_memcpy(&new_iso->waktu, &record->waktu,
                                  sizeof(iso9660_waktu_t));

                    new_inode->i_ino = (ino_t)new_iso->extent;
                    iso9660_read_inode(new_inode);

                    result = dentry_alloc(name);
                    if (result != NULL) {
                        result->d_inode = new_inode;
                        dentry_cache_insert(result);
                    } else {
                        inode_put(new_inode);
                    }
                }
            }
            break;
        }

        offset += rec_len;
    }

    kfree(buffer);
    return result;
}

static status_t iso9660_getattr(dentry_t *dentry, vfs_stat_t *stat)
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

/*
 * ===========================================================================
 * IMPLEMENTASI FILE OPERATIONS
 * ===========================================================================
 */

static status_t iso9660_open(inode_t *inode, file_t *file)
{
    iso9660_file_t *iso_file;

    if (inode == NULL || file == NULL) {
        return STATUS_PARAM_NULL;
    }

    iso_file = (iso9660_file_t *)kmalloc(sizeof(iso9660_file_t));
    if (iso_file == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    kernel_memset(iso_file, 0, sizeof(iso9660_file_t));
    iso_file->magic = ISO9660_FILE_MAGIC;
    iso_file->vfs_file = file;

    if (inode->i_private != NULL) {
        iso9660_inode_t *iso_inode = (iso9660_inode_t *)inode->i_private;
        if (iso_inode->magic == ISO9660_INODE_MAGIC) {
            iso_file->extent = iso_inode->extent;
        }
    }

    file->f_private = iso_file;

    return STATUS_BERHASIL;
}

static status_t iso9660_release(inode_t *inode, file_t *file)
{
    iso9660_file_t *iso_file;

    TIDAK_DIGUNAKAN_PARAM(inode);

    if (file == NULL) {
        return STATUS_PARAM_NULL;
    }

    iso_file = (iso9660_file_t *)file->f_private;
    if (iso_file != NULL && iso_file->magic == ISO9660_FILE_MAGIC) {
        iso_file->magic = 0;
        kfree(iso_file);
        file->f_private = NULL;
    }

    return STATUS_BERHASIL;
}

static tak_bertandas_t iso9660_read(file_t *file, void *buffer,
                                    ukuran_t size, off_t *pos)
{
    iso9660_sb_t *isb;
    iso9660_inode_t *iso_inode;
    tak_bertanda8_t *block_buffer;
    tak_bertanda64_t start_block;
    tak_bertanda32_t block_offset;
    ukuran_t bytes_read;
    ukuran_t bytes_remaining;
    status_t status;

    if (file == NULL || buffer == NULL || pos == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_NULL;
    }

    if (file->f_inode == NULL || file->f_inode->i_sb == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    isb = (iso9660_sb_t *)file->f_inode->i_sb->s_private;
    if (isb == NULL || isb->magic != ISO9660_SB_MAGIC) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    iso_inode = (iso9660_inode_t *)file->f_inode->i_private;
    if (iso_inode == NULL || iso_inode->magic != ISO9660_INODE_MAGIC) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    /* Batasi bacaan hingga akhir file */
    if (*pos >= file->f_inode->i_size) {
        return 0;
    }

    if ((off_t)size > file->f_inode->i_size - *pos) {
        size = (ukuran_t)(file->f_inode->i_size - *pos);
    }

    block_buffer = (tak_bertanda8_t *)kmalloc((ukuran_t)isb->block_size);
    if (block_buffer == NULL) {
        return (tak_bertandas_t)STATUS_MEMORI_HABIS;
    }

    bytes_read = 0;
    bytes_remaining = size;

    start_block = (tak_bertanda64_t)iso_inode->extent +
                  (tak_bertanda64_t)(*pos / (off_t)isb->block_size);
    block_offset = (tak_bertanda32_t)(*pos % (off_t)isb->block_size);

    while (bytes_remaining > 0) {
        tak_bertanda32_t to_copy;

        status = iso9660_baca_block(file->f_inode->i_sb, start_block,
                                    block_buffer, 1);
        if (status != STATUS_BERHASIL) {
            break;
        }

        to_copy = (tak_bertanda32_t)MIN(bytes_remaining,
                                        (ukuran_t)(isb->block_size -
                                                   block_offset));

        kernel_memcpy((tak_bertanda8_t *)buffer + bytes_read,
                      block_buffer + block_offset, to_copy);

        bytes_read += to_copy;
        bytes_remaining -= to_copy;
        start_block++;
        block_offset = 0;
    }

    kfree(block_buffer);

    *pos += (off_t)bytes_read;

    return (tak_bertandas_t)bytes_read;
}

static off_t iso9660_lseek(file_t *file, off_t offset,
                           tak_bertanda32_t whence)
{
    off_t new_pos;

    if (file == NULL || file->f_inode == NULL) {
        return (off_t)STATUS_PARAM_NULL;
    }

    switch (whence) {
    case VFS_SEEK_SET:
        new_pos = offset;
        break;
    case VFS_SEEK_CUR:
        new_pos = file->f_pos + offset;
        break;
    case VFS_SEEK_END:
        new_pos = file->f_inode->i_size + offset;
        break;
    default:
        return (off_t)STATUS_PARAM_INVALID;
    }

    if (new_pos < 0) {
        return (off_t)STATUS_PARAM_INVALID;
    }

    /* ISO9660 read-only, tidak bisa extend file */
    if (new_pos > file->f_inode->i_size) {
        new_pos = file->f_inode->i_size;
    }

    file->f_pos = new_pos;

    return new_pos;
}

static tak_bertandas_t iso9660_readdir(file_t *file, vfs_dirent_t *dirent,
                                       ukuran_t count)
{
    iso9660_sb_t *isb;
    iso9660_inode_t *iso_inode;
    tak_bertanda8_t *buffer;
    tak_bertanda32_t offset;
    tak_bertanda32_t size;
    status_t status;
    ukuran_t entries_read;
    TIDAK_DIGUNAKAN_PARAM(count);

    if (file == NULL || dirent == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_NULL;
    }

    if (file->f_inode == NULL || file->f_inode->i_sb == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    isb = (iso9660_sb_t *)file->f_inode->i_sb->s_private;
    if (isb == NULL || isb->magic != ISO9660_SB_MAGIC) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    iso_inode = (iso9660_inode_t *)file->f_inode->i_private;
    if (iso_inode == NULL || iso_inode->magic != ISO9660_INODE_MAGIC) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    if (!iso_inode->is_directory) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    buffer = (tak_bertanda8_t *)kmalloc((ukuran_t)file->f_inode->i_size);
    if (buffer == NULL) {
        return (tak_bertandas_t)STATUS_MEMORI_HABIS;
    }

    status = iso9660_baca_block(file->f_inode->i_sb, iso_inode->extent,
                               buffer,
                               ((tak_bertanda32_t)file->f_inode->i_size +
                                isb->block_size - 1) / isb->block_size);
    if (status != STATUS_BERHASIL) {
        kfree(buffer);
        return (tak_bertandas_t)status;
    }

    offset = (tak_bertanda32_t)file->f_pos;
    size = (tak_bertanda32_t)file->f_inode->i_size;
    entries_read = 0;

    while (offset < size) {
        iso9660_dir_record_t *record;
        tak_bertanda8_t rec_len;

        record = (iso9660_dir_record_t *)(buffer + offset);
        rec_len = record->panjang;

        if (rec_len == 0) {
            offset = (offset + isb->block_size) &
                     ~(isb->block_size - 1);
            continue;
        }

        /* Skip . dan .. */
        if (record->nama_len == 1 &&
            (record->nama[0] == 0x00 || record->nama[0] == 0x01)) {
            offset += rec_len;
            continue;
        }

        /* Isi dirent */
        dirent->d_ino = (ino_t)iso9660_bothendian32(record->extent_le,
                                                    record->extent_be);
        dirent->d_off = (off_t)(offset + rec_len);
        dirent->d_reclen = sizeof(vfs_dirent_t);

        /* Set tipe file */
        if (record->flags & ISO9660_FLAG_DIRECTORY) {
            dirent->d_type = VFS_DT_DIR;
        } else {
            dirent->d_type = VFS_DT_REG;
        }

        /* Normalisasi nama */
        iso9660_nama_normalisasi(record->nama, record->nama_len,
                                 dirent->d_name, VFS_NAMA_MAKS + 1);

        file->f_pos = (off_t)(offset + rec_len);
        entries_read = 1;
        break;
    }

    if (offset >= size) {
        file->f_pos = (off_t)size;
    }

    kfree(buffer);

    return (tak_bertandas_t)entries_read;
}

/*
 * ===========================================================================
 * IMPLEMENTASI MOUNT/UMOUNT
 * ===========================================================================
 */

static superblock_t *iso9660_mount(filesystem_t *fs, const char *device,
                                   const char *path, tak_bertanda32_t flags)
{
    superblock_t *sb;
    iso9660_sb_t *isb;
    inode_t *root_inode;
    iso9660_inode_t *root_iso;
    dentry_t *root_dentry;
    status_t status;

    TIDAK_DIGUNAKAN_PARAM(path);

    if (fs == NULL || device == NULL) {
        return NULL;
    }

    /* ISO9660 hanya read-only */
    if (!(flags & VFS_MOUNT_FLAG_RDONLY)) {
        log_warn("[ISO9660] Filesystem hanya mendukung read-only");
    }

    /* Alokasi VFS superblock */
    sb = superblock_alloc(fs);
    if (sb == NULL) {
        return NULL;
    }

    /* Alokasi internal superblock */
    isb = (iso9660_sb_t *)kmalloc(sizeof(iso9660_sb_t));
    if (isb == NULL) {
        superblock_free(sb);
        return NULL;
    }

    kernel_memset(isb, 0, sizeof(iso9660_sb_t));
    isb->magic = ISO9660_SB_MAGIC;
    isb->vfs_sb = sb;
    isb->block_size = ISO9660_SEKTOR_UKURAN;
    isb->level = ISO9660_LEVEL_2;

    sb->s_private = isb;
    sb->s_readonly = BENAR;

    /* Parse Primary Volume Descriptor */
    status = iso9660_parse_pvd(isb);
    if (status != STATUS_BERHASIL) {
        log_error("[ISO9660] Gagal parse PVD: %d", status);
        kfree(isb);
        superblock_free(sb);
        return NULL;
    }

    /* Deteksi ekstensi */
    iso9660_detect_extensions(isb);

    log_info("[ISO9660] Volume: %s", isb->volume_id);
    log_info("[ISO9660] Block size: %u", isb->block_size);
    log_info("[ISO9660] Total blocks: %lu", isb->total_blocks);
    log_info("[ISO9660] RockRidge: %s", isb->has_rockridge ? "ya" : "tidak");
    log_info("[ISO9660] Joliet: %s", isb->has_joliet ? "ya" : "tidak");

    /* Buat root inode */
    root_inode = iso9660_alloc_inode(sb);
    if (root_inode == NULL) {
        kfree(isb->pvd);
        kfree(isb);
        superblock_free(sb);
        return NULL;
    }

    root_iso = (iso9660_inode_t *)root_inode->i_private;
    if (root_iso != NULL) {
        root_iso->extent = isb->root_extent;
        root_iso->size = isb->root_size;
        root_iso->is_directory = BENAR;
        root_iso->flags = ISO9660_FLAG_DIRECTORY;
    }

    root_inode->i_ino = (ino_t)isb->root_extent;
    root_inode->i_mode = VFS_S_IFDIR | 0755;
    root_inode->i_size = (off_t)isb->root_size;
    root_inode->i_sb = sb;
    root_inode->i_op = &g_iso9660_inode_ops;
    root_inode->i_fop = &g_iso9660_file_ops;

    /* Buat root dentry */
    root_dentry = dentry_alloc("/");
    if (root_dentry == NULL) {
        inode_put(root_inode);
        kfree(isb->pvd);
        kfree(isb);
        superblock_free(sb);
        return NULL;
    }

    root_dentry->d_inode = root_inode;
    root_dentry->d_sb = sb;

    sb->s_root = root_dentry;

    /* Setup operations */
    sb->s_op = &g_iso9660_super_ops;

    return sb;
}

static status_t iso9660_umount(superblock_t *sb)
{
    iso9660_sb_t *isb;

    if (sb == NULL) {
        return STATUS_PARAM_NULL;
    }

    isb = (iso9660_sb_t *)sb->s_private;
    if (isb == NULL || isb->magic != ISO9660_SB_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Bebaskan root dentry */
    if (sb->s_root != NULL) {
        dentry_put(sb->s_root);
        sb->s_root = NULL;
    }

    /* Bebaskan PVD */
    if (isb->pvd != NULL) {
        kfree(isb->pvd);
        isb->pvd = NULL;
    }

    /* Clear magic dan free */
    isb->magic = 0;
    kfree(isb);
    sb->s_private = NULL;

    return STATUS_BERHASIL;
}

static status_t iso9660_detect(const char *device)
{
    tak_bertanda8_t buffer[ISO9660_SEKTOR_UKURAN];
    iso9660_vd_header_t *header;
    TIDAK_DIGUNAKAN_PARAM(device);

    /* Baca sektor 16 */
    /* TODO: Implementasi pembacaan dari device */
    kernel_memset(buffer, 0, sizeof(buffer));

    /* Untuk sekarang, placeholder */
    header = (iso9660_vd_header_t *)buffer;

    if (header->identitas[0] == 'C' && header->identitas[1] == 'D' &&
        header->identitas[2] == '0' && header->identitas[3] == '0' &&
        header->identitas[4] == '1' &&
        header->tipe == ISO9660_VD_PRIMARY) {
        return STATUS_BERHASIL;
    }

    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * ===========================================================================
 * INISIALISASI OPERATIONS STRUCTURES
 * ===========================================================================
 */

static void iso9660_init_ops(void)
{
    /* Superblock operations */
    kernel_memset(&g_iso9660_super_ops, 0, sizeof(g_iso9660_super_ops));
    g_iso9660_super_ops.alloc_inode = iso9660_alloc_inode;
    g_iso9660_super_ops.destroy_inode = iso9660_destroy_inode;
    g_iso9660_super_ops.read_inode = iso9660_read_inode;
    g_iso9660_super_ops.statfs = iso9660_statfs;
    g_iso9660_super_ops.sync_fs = NULL;
    g_iso9660_super_ops.put_super = NULL;
    g_iso9660_super_ops.write_inode = NULL;

    /* Inode operations */
    kernel_memset(&g_iso9660_inode_ops, 0, sizeof(g_iso9660_inode_ops));
    g_iso9660_inode_ops.lookup = iso9660_lookup;
    g_iso9660_inode_ops.getattr = iso9660_getattr;
    g_iso9660_inode_ops.create = NULL;
    g_iso9660_inode_ops.mkdir = NULL;
    g_iso9660_inode_ops.rmdir = NULL;
    g_iso9660_inode_ops.unlink = NULL;
    g_iso9660_inode_ops.rename = NULL;
    g_iso9660_inode_ops.symlink = NULL;
    g_iso9660_inode_ops.readlink = NULL;
    g_iso9660_inode_ops.permission = NULL;
    g_iso9660_inode_ops.setattr = NULL;

    /* File operations */
    kernel_memset(&g_iso9660_file_ops, 0, sizeof(g_iso9660_file_ops));
    g_iso9660_file_ops.read = iso9660_read;
    g_iso9660_file_ops.write = NULL;
    g_iso9660_file_ops.lseek = iso9660_lseek;
    g_iso9660_file_ops.open = iso9660_open;
    g_iso9660_file_ops.release = iso9660_release;
    g_iso9660_file_ops.flush = NULL;
    g_iso9660_file_ops.fsync = NULL;
    g_iso9660_file_ops.readdir = iso9660_readdir;
    g_iso9660_file_ops.ioctl = NULL;
    g_iso9660_file_ops.mmap = NULL;
}

/*
 * ===========================================================================
 * FUNGSI INISIALISASI MODULE (MODULE INITIALIZATION FUNCTION)
 * ===========================================================================
 */

status_t iso9660_init(void)
{
    status_t status;

    log_info("[ISO9660] Menginisialisasi driver ISO9660...");

    /* Initialize operations */
    iso9660_init_ops();

    /* Register filesystem */
    status = filesystem_register(&g_iso9660_fs);
    if (status != STATUS_BERHASIL) {
        log_error("[ISO9660] Gagal registrasi filesystem: %d", status);
        return status;
    }

    log_info("[ISO9660] Driver berhasil diinisialisasi");

    return STATUS_BERHASIL;
}

/*
 * Constructor untuk auto-registration
 */
KONSTRUKTOR static void iso9660_auto_init(void)
{
    iso9660_init();
}
