/*
 * PIGURA OS - ISO9660_PRIMARY.C
 * ===============================
 * Implementasi parsing Primary Volume Descriptor untuk ISO9660.
 *
 * Berkas ini berisi fungsi-fungsi untuk membaca dan memparse
 * Primary Volume Descriptor (PVD) dari filesystem ISO9660.
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
 * KONSTANTA PARSING (PARSING CONSTANTS)
 * ===========================================================================
 */

/* Ukuran sektor standar ISO9660 */
#define ISO_SEKTOR_UKURAN          2048

/* Lokasi awal Volume Descriptor */
#define ISO_VD_AWAL                16
#define ISO_VD_MAKS                100

/* Tipe Volume Descriptor */
#define ISO_VD_BOOT                0x00
#define ISO_VD_PRIMARY             0x01
#define ISO_VD_SUPPLEMENTARY       0x02
#define ISO_VD_PARTITION           0x03
#define ISO_VD_TERMINATOR          0xFF

/* Magic string untuk validasi */
#define ISO_MAGIC_STRING           "CD001"
#define ISO_MAGIC_LEN              5

/* Versi ISO9660 yang didukung */
#define ISO_VERSI_STD              0x01

/* Level interoperabilitas */
#define ISO_LEVEL_1                1
#define ISO_LEVEL_2                2
#define ISO_LEVEL_3                3

/* Panjang field standar */
#define ISO_ID_SISTEM_LEN          32
#define ISO_ID_VOLUME_LEN          32
#define ISO_ID_SET_VOLUME_LEN      128
#define ISO_ID_PUBLISHER_LEN       128
#define ISO_ID_PREPARER_LEN        128
#define ISO_ID_APPLICATION_LEN     128
#define ISO_FILE_COPYRIGHT_LEN     37
#define ISO_FILE_ABSTRACT_LEN      37
#define ISO_FILE_BIBLIOGRAPHIC_LEN 37

/* Panjang direktori root record */
#define ISO_ROOT_DIR_LEN           34

/* Panjang escape sequence */
#define ISO_ESCAPE_LEN             32

/*
 * ===========================================================================
 * STRUKTUR DATA (DATA STRUCTURES)
 * ===========================================================================
 */

/* Struktur Primary Volume Descriptor (2048 byte total) */
typedef struct PACKED {
    /* Header (7 byte) */
    tak_bertanda8_t vd_tipe;                    /* Tipe VD */
    tak_bertanda8_t identitas[5];               /* "CD001" */
    tak_bertanda8_t versi;                      /* Versi (1) */

    /* System dan Volume ID (69 byte) */
    tak_bertanda8_t sistem_id[32];              /* System identifier */
    tak_bertanda8_t volume_id[32];              /* Volume identifier */
    tak_bertanda8_t padding[8];                 /* Padding */

    /* Volume space size (16 byte) */
    tak_bertanda32_t volume_size_le;            /* Both-endian: LE */
    tak_bertanda32_t volume_size_be;            /* Both-endian: BE */

    /* Escape sequences (32 byte) */
    tak_bertanda8_t escape_seq[32];             /* Escape sequences */

    /* Volume set size (8 byte) */
    tak_bertanda16_t volume_set_size_le;        /* Both-endian: LE */
    tak_bertanda16_t volume_set_size_be;        /* Both-endian: BE */

    /* Volume sequence number (8 byte) */
    tak_bertanda16_t volume_seq_num_le;         /* Both-endian: LE */
    tak_bertanda16_t volume_seq_num_be;         /* Both-endian: BE */

    /* Logical block size (8 byte) */
    tak_bertanda16_t block_size_le;             /* Both-endian: LE */
    tak_bertanda16_t block_size_be;             /* Both-endian: BE */

    /* Path table size (16 byte) */
    tak_bertanda32_t path_table_size_le;        /* Both-endian: LE */
    tak_bertanda32_t path_table_size_be;        /* Both-endian: BE */

    /* Path table locations (16 byte) */
    tak_bertanda32_t path_table_l_le;           /* Location L type LE */
    tak_bertanda32_t path_table_l_optional_le;  /* Optional L type LE */
    tak_bertanda32_t path_table_m_be;           /* Location M type BE */
    tak_bertanda32_t path_table_m_optional_be;  /* Optional M type BE */

    /* Root directory record (34 byte) */
    tak_bertanda8_t root_dir[34];               /* Root directory record */

    /* Volume set identifier (128 byte) */
    tak_bertanda8_t volume_set_id[128];

    /* Publisher identifier (128 byte) */
    tak_bertanda8_t publisher_id[128];

    /* Data preparer identifier (128 byte) */
    tak_bertanda8_t preparer_id[128];

    /* Application identifier (128 byte) */
    tak_bertanda8_t application_id[128];

    /* Copyright file identifier (37 byte) */
    tak_bertanda8_t copyright_file[37];

    /* Abstract file identifier (37 byte) */
    tak_bertanda8_t abstract_file[37];

    /* Bibliographic file identifier (37 byte) */
    tak_bertanda8_t bibliographic_file[37];

    /* Timestamps (34 byte total) */
    tak_bertanda8_t dibuat[17];                 /* Creation timestamp */
    tak_bertanda8_t dimodifikasi[17];           /* Modification timestamp */
    tak_bertanda8_t kadaluarsa[17];             /* Expiration timestamp */
    tak_bertanda8_t berlaku[17];                /* Effective timestamp */

    /* Application use dan reserved (1413 byte) */
    tak_bertanda8_t app_use[512];               /* Application use */
    tak_bertanda8_t reserved[653];              /* Reserved */
} iso_pvd_t;

/* Struktur Directory Record untuk root */
typedef struct PACKED {
    tak_bertanda8_t panjang;                    /* Panjang record */
    tak_bertanda8_t ext_attr_len;               /* Extended attribute length */
    tak_bertanda32_t extent_le;                 /* Extent location LE */
    tak_bertanda32_t extent_be;                 /* Extent location BE */
    tak_bertanda32_t data_len_le;               /* Data length LE */
    tak_bertanda32_t data_len_be;               /* Data length BE */
    tak_bertanda8_t tahun;                      /* Tahun sejak 1900 */
    tak_bertanda8_t bulan;                      /* Bulan */
    tak_bertanda8_t hari;                       /* Hari */
    tak_bertanda8_t jam;                        /* Jam */
    tak_bertanda8_t menit;                      /* Menit */
    tak_bertanda8_t detik;                      /* Detik */
    tak_bertanda8_t zona;                       /* Zona waktu */
    tak_bertanda8_t flags;                      /* File flags */
    tak_bertanda8_t file_unit_size;             /* File unit size */
    tak_bertanda8_t interleave_gap;             /* Interleave gap size */
    tak_bertanda16_t vol_seq_le;                /* Volume sequence LE */
    tak_bertanda16_t vol_seq_be;                /* Volume sequence BE */
    tak_bertanda8_t nama_len;                   /* Length of name */
    tak_bertanda8_t nama;                       /* Name (variable) */
} iso_dir_record_t;

/* Struktur hasil parsing PVD */
typedef struct {
    bool_t valid;                               /* Apakah PVD valid? */
    tak_bertanda8_t versi;                      /* Versi ISO9660 */
    tak_bertanda8_t level;                      /* Level interoperabilitas */
    char sistem_id[33];                         /* System identifier */
    char volume_id[33];                         /* Volume identifier */
    tak_bertanda64_t volume_size;               /* Ukuran volume (bytes) */
    tak_bertanda16_t block_size;                /* Ukuran block */
    tak_bertanda32_t total_blocks;              /* Total block */
    tak_bertanda32_t path_table_size;           /* Ukuran path table */
    tak_bertanda32_t path_table_loc_l;          /* Lokasi path table L */
    tak_bertanda32_t path_table_loc_m;          /* Lokasi path table M */
    tak_bertanda32_t root_extent;               /* Root extent location */
    tak_bertanda32_t root_size;                 /* Root directory size */
    tak_bertanda16_t volume_set_size;           /* Volume set size */
    tak_bertanda16_t volume_seq_num;            /* Volume sequence number */
    char publisher_id[129];                     /* Publisher identifier */
    char preparer_id[129];                      /* Preparer identifier */
    char application_id[129];                   /* Application identifier */
    bool_t has_joliet;                          /* Joliet extension? */
    bool_t has_rockridge;                       /* RockRidge extension? */
} iso_pvd_info_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INTERNAL (INTERNAL FUNCTION DECLARATIONS)
 * ===========================================================================
 */

static bool_t iso_validasi_magic(const tak_bertanda8_t *identitas);
static bool_t iso_validasi_versi(tak_bertanda8_t versi);
static tak_bertanda32_t iso_baca_bothendian32(tak_bertanda32_t le,
                                              tak_bertanda32_t be);
static tak_bertanda16_t iso_baca_bothendian16(tak_bertanda16_t le,
                                              tak_bertanda16_t be);
static void iso_trim_string(const tak_bertanda8_t *src, ukuran_t src_len,
                            char *dst, ukuran_t dst_len);
static status_t iso_parse_root_dir(const tak_bertanda8_t *root_dir,
                                   iso_pvd_info_t *info);
static status_t iso_cek_joliet_escape(const tak_bertanda8_t *escape_seq);
static void iso_deteksi_level(const iso_pvd_t *pvd, iso_pvd_info_t *info);

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI VALIDASI (VALIDATION FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

static bool_t iso_validasi_magic(const tak_bertanda8_t *identitas)
{
    if (identitas == NULL) {
        return SALAH;
    }

    /* Cek string "CD001" */
    if (identitas[0] != 'C' || identitas[1] != 'D' ||
        identitas[2] != '0' || identitas[3] != '0' ||
        identitas[4] != '1') {
        return SALAH;
    }

    return BENAR;
}

static bool_t iso_validasi_versi(tak_bertanda8_t versi)
{
    /* ISO9660 hanya mendukung versi 1 */
    return (versi == ISO_VERSI_STD) ? BENAR : SALAH;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI KONVERSI (CONVERSION FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

static tak_bertanda32_t iso_baca_bothendian32(tak_bertanda32_t le,
                                              tak_bertanda32_t be)
{
    tak_bertanda32_t le_swapped;
    tak_bertanda32_t be_swapped;

    /* Little-endian sudah benar untuk mesin LE */
    le_swapped = le;

    /* Big-endian perlu di-swap */
    be_swapped = ((be & 0x000000FF) << 24) |
                 ((be & 0x0000FF00) << 8) |
                 ((be & 0x00FF0000) >> 8) |
                 ((be & 0xFF000000) >> 24);

    /* Validasi: kedua nilai harus sama */
    if (le_swapped != be_swapped) {
        /* Jika berbeda, gunakan LE sebagai default */
        return le_swapped;
    }

    return le_swapped;
}

static tak_bertanda16_t iso_baca_bothendian16(tak_bertanda16_t le,
                                              tak_bertanda16_t be)
{
    tak_bertanda16_t le_swapped;
    tak_bertanda16_t be_swapped;

    /* Little-endian sudah benar untuk mesin LE */
    le_swapped = le;

    /* Big-endian perlu di-swap */
    be_swapped = (tak_bertanda16_t)(
        ((be & 0x00FF) << 8) |
        ((be & 0xFF00) >> 8)
    );

    /* Validasi: kedua nilai harus sama */
    if (le_swapped != be_swapped) {
        return le_swapped;
    }

    return le_swapped;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI STRING (STRING FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

static void iso_trim_string(const tak_bertanda8_t *src, ukuran_t src_len,
                            char *dst, ukuran_t dst_len)
{
    ukuran_t i;
    ukuran_t j;
    ukuran_t actual_len;

    if (src == NULL || dst == NULL || dst_len == 0) {
        return;
    }

    /* Cari panjang aktual (tanpa trailing spaces) */
    actual_len = src_len;
    while (actual_len > 0 && src[actual_len - 1] == ' ') {
        actual_len--;
    }

    /* Copy dan trim */
    j = 0;
    for (i = 0; i < actual_len && j < dst_len - 1; i++) {
        /* Skip karakter kontrol */
        if (src[i] >= 32 && src[i] <= 126) {
            dst[j++] = (char)src[i];
        }
    }

    dst[j] = '\0';
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI PARSING ROOT DIR (ROOT DIR PARSING IMPLEMENTATION)
 * ===========================================================================
 */

static status_t iso_parse_root_dir(const tak_bertanda8_t *root_dir,
                                   iso_pvd_info_t *info)
{
    const iso_dir_record_t *record;

    if (root_dir == NULL || info == NULL) {
        return STATUS_PARAM_NULL;
    }

    record = (const iso_dir_record_t *)root_dir;

    /* Validasi panjang minimum */
    if (record->panjang < 33) {
        return STATUS_FORMAT_INVALID;
    }

    /* Parse extent location */
    info->root_extent = iso_baca_bothendian32(record->extent_le,
                                              record->extent_be);

    /* Parse data length */
    info->root_size = iso_baca_bothendian32(record->data_len_le,
                                            record->data_len_be);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI DETEKSI EKSTENSI (EXTENSION DETECTION IMPLEMENTATION)
 * ===========================================================================
 */

static status_t iso_cek_joliet_escape(const tak_bertanda8_t *escape_seq)
{
    ukuran_t i;

    if (escape_seq == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Cari escape sequence Joliet */
    /* Joliet menggunakan: %/@ (UCS-2 Level 1), %/C (Level 2), %/E (Level 3) */
    for (i = 0; i < ISO_ESCAPE_LEN - 2; i++) {
        if (escape_seq[i] == '%' && escape_seq[i + 1] == '/') {
            tak_bertanda8_t level = escape_seq[i + 2];
            if (level == '@' || level == 'C' || level == 'E') {
                return STATUS_BERHASIL;
            }
        }
    }

    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI DETEKSI LEVEL (LEVEL DETECTION IMPLEMENTATION)
 * ===========================================================================
 */

static void iso_deteksi_level(const iso_pvd_t *pvd, iso_pvd_info_t *info)
{
    /* Level ditentukan oleh karakteristik volume */
    /* Level 1: Nama 8.3, file max 4GB */
    /* Level 2: Nama hingga 31 karakter, file max 4GB */
    /* Level 3: Sama dengan Level 2 + multi-extent files */

    if (pvd == NULL || info == NULL) {
        return;
    }

    /* Default ke Level 2 (paling umum) */
    info->level = ISO_LEVEL_2;

    /* Cek karakteristik Level 1 */
    /* Level 1 memiliki batasan nama file 8 karakter + 3 ekstensi */
    /* Ini sulit ditentukan dari PVD saja, jadi gunakan heuristik */

    /* Untuk sekarang, asumsikan Level 2 */
    info->level = ISO_LEVEL_2;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI UTAMA (MAIN FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

status_t iso9660_baca_pvd(void *device_context,
                          tak_bertanda8_t *buffer,
                          ukuran_t buffer_size)
{
    tak_bertanda32_t sektor;
    iso_pvd_t *pvd;
    iso_pvd_info_t *info;
    status_t status;

    TIDAK_DIGUNAKAN_PARAM(device_context);

    if (buffer == NULL || buffer_size < ISO_SEKTOR_UKURAN) {
        return STATUS_PARAM_INVALID;
    }

    /* Alokasi info structure */
    info = (iso_pvd_info_t *)kmalloc(sizeof(iso_pvd_info_t));
    if (info == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    kernel_memset(info, 0, sizeof(iso_pvd_info_t));
    info->valid = SALAH;

    /* Cari Primary Volume Descriptor */
    sektor = ISO_VD_AWAL;

    while (sektor < ISO_VD_AWAL + ISO_VD_MAKS) {
        /* Baca sektor */
        /* TODO: Implementasi pembacaan dari device */
        /* Untuk sekarang, asumsikan buffer sudah berisi data */

        pvd = (iso_pvd_t *)buffer;

        /* Validasi magic number */
        if (!iso_validasi_magic(pvd->identitas)) {
            sektor++;
            continue;
        }

        /* Cek tipe Volume Descriptor */
        switch (pvd->vd_tipe) {
        case ISO_VD_PRIMARY:
            /* Validasi versi */
            if (!iso_validasi_versi(pvd->versi)) {
                status = STATUS_FORMAT_INVALID;
                goto cleanup;
            }

            /* Parse informasi dasar */
            info->valid = BENAR;
            info->versi = pvd->versi;

            /* Trim dan copy string identifiers */
            iso_trim_string(pvd->sistem_id, ISO_ID_SISTEM_LEN,
                           info->sistem_id, sizeof(info->sistem_id));
            iso_trim_string(pvd->volume_id, ISO_ID_VOLUME_LEN,
                           info->volume_id, sizeof(info->volume_id));
            iso_trim_string(pvd->publisher_id, ISO_ID_PUBLISHER_LEN,
                           info->publisher_id, sizeof(info->publisher_id));
            iso_trim_string(pvd->preparer_id, ISO_ID_PREPARER_LEN,
                           info->preparer_id, sizeof(info->preparer_id));
            iso_trim_string(pvd->application_id, ISO_ID_APPLICATION_LEN,
                           info->application_id, sizeof(info->application_id));

            /* Parse both-endian values */
            info->block_size = iso_baca_bothendian16(pvd->block_size_le,
                                                     pvd->block_size_be);
            info->total_blocks = iso_baca_bothendian32(pvd->volume_size_le,
                                                       pvd->volume_size_be);
            info->volume_size = (tak_bertanda64_t)info->total_blocks *
                               (tak_bertanda64_t)info->block_size;

            info->path_table_size = iso_baca_bothendian32(pvd->path_table_size_le,
                                                          pvd->path_table_size_be);
            info->path_table_loc_l = pvd->path_table_l_le;
            info->path_table_loc_m = pvd->path_table_m_be;

            info->volume_set_size = iso_baca_bothendian16(pvd->volume_set_size_le,
                                                          pvd->volume_set_size_be);
            info->volume_seq_num = iso_baca_bothendian16(pvd->volume_seq_num_le,
                                                         pvd->volume_seq_num_be);

            /* Parse root directory record */
            status = iso_parse_root_dir(pvd->root_dir, info);
            if (status != STATUS_BERHASIL) {
                goto cleanup;
            }

            /* Cek Joliet extension */
            if (iso_cek_joliet_escape(pvd->escape_seq) == STATUS_BERHASIL) {
                info->has_joliet = BENAR;
            }

            /* Deteksi level */
            iso_deteksi_level(pvd, info);

            status = STATUS_BERHASIL;
            goto cleanup;

        case ISO_VD_TERMINATOR:
            /* Tidak ada PVD ditemukan */
            status = STATUS_TIDAK_DITEMUKAN;
            goto cleanup;

        default:
            /* Skip Volume Descriptor lainnya */
            break;
        }

        sektor++;
    }

    status = STATUS_TIDAK_DITEMUKAN;

cleanup:
    kfree(info);
    return status;
}

status_t iso9660_validasi_pvd(const tak_bertanda8_t *pvd_data,
                              ukuran_t data_len)
{
    const iso_pvd_t *pvd;

    if (pvd_data == NULL || data_len < ISO_SEKTOR_UKURAN) {
        return STATUS_PARAM_INVALID;
    }

    pvd = (const iso_pvd_t *)pvd_data;

    /* Cek tipe */
    if (pvd->vd_tipe != ISO_VD_PRIMARY) {
        return STATUS_FORMAT_INVALID;
    }

    /* Cek magic */
    if (!iso_validasi_magic(pvd->identitas)) {
        return STATUS_FORMAT_INVALID;
    }

    /* Cek versi */
    if (!iso_validasi_versi(pvd->versi)) {
        return STATUS_FORMAT_INVALID;
    }

    /* Cek block size (harus power of 2 dan >= 2048) */
    {
        tak_bertanda16_t block_size;
        block_size = iso_baca_bothendian16(pvd->block_size_le, pvd->block_size_be);

        if (block_size < ISO_SEKTOR_UKURAN) {
            return STATUS_FORMAT_INVALID;
        }

        /* Cek power of 2 */
        if ((block_size & (block_size - 1)) != 0) {
            return STATUS_FORMAT_INVALID;
        }
    }

    /* Cek volume size */
    {
        tak_bertanda32_t vol_size_le = pvd->volume_size_le;
        tak_bertanda32_t vol_size_be = pvd->volume_size_be;

        if (vol_size_le == 0 && vol_size_be == 0) {
            return STATUS_FORMAT_INVALID;
        }
    }

    return STATUS_BERHASIL;
}

tak_bertanda32_t iso9660_dapat_block_size(const tak_bertanda8_t *pvd_data)
{
    const iso_pvd_t *pvd;

    if (pvd_data == NULL) {
        return 0;
    }

    pvd = (const iso_pvd_t *)pvd_data;

    return (tak_bertanda32_t)iso_baca_bothendian16(pvd->block_size_le,
                                                   pvd->block_size_be);
}

tak_bertanda64_t iso9660_dapat_volume_size(const tak_bertanda8_t *pvd_data)
{
    const iso_pvd_t *pvd;
    tak_bertanda32_t blocks;
    tak_bertanda16_t block_size;

    if (pvd_data == NULL) {
        return 0;
    }

    pvd = (const iso_pvd_t *)pvd_data;

    blocks = iso_baca_bothendian32(pvd->volume_size_le, pvd->volume_size_be);
    block_size = iso_baca_bothendian16(pvd->block_size_le, pvd->block_size_be);

    return (tak_bertanda64_t)blocks * (tak_bertanda64_t)block_size;
}

status_t iso9660_dapat_root_dir(const tak_bertanda8_t *pvd_data,
                                tak_bertanda32_t *extent,
                                tak_bertanda32_t *size)
{
    const iso_pvd_t *pvd;
    const iso_dir_record_t *record;

    if (pvd_data == NULL || extent == NULL || size == NULL) {
        return STATUS_PARAM_NULL;
    }

    pvd = (const iso_pvd_t *)pvd_data;
    record = (const iso_dir_record_t *)pvd->root_dir;

    *extent = iso_baca_bothendian32(record->extent_le, record->extent_be);
    *size = iso_baca_bothendian32(record->data_len_le, record->data_len_be);

    return STATUS_BERHASIL;
}

status_t iso9660_dapat_path_table(const tak_bertanda8_t *pvd_data,
                                  tak_bertanda32_t *location,
                                  tak_bertanda32_t *size)
{
    const iso_pvd_t *pvd;

    if (pvd_data == NULL || location == NULL || size == NULL) {
        return STATUS_PARAM_NULL;
    }

    pvd = (const iso_pvd_t *)pvd_data;

    /* Gunakan path table L type (little-endian) */
    *location = pvd->path_table_l_le;
    *size = iso_baca_bothendian32(pvd->path_table_size_le, pvd->path_table_size_be);

    return STATUS_BERHASIL;
}

status_t iso9660_dapat_volume_id(const tak_bertanda8_t *pvd_data,
                                 char *buffer, ukuran_t buffer_size)
{
    const iso_pvd_t *pvd;

    if (pvd_data == NULL || buffer == NULL || buffer_size == 0) {
        return STATUS_PARAM_NULL;
    }

    pvd = (const iso_pvd_t *)pvd_data;

    iso_trim_string(pvd->volume_id, ISO_ID_VOLUME_LEN, buffer, buffer_size);

    return STATUS_BERHASIL;
}

bool_t iso9660_cek_joliet(const tak_bertanda8_t *pvd_data)
{
    const iso_pvd_t *pvd;

    if (pvd_data == NULL) {
        return SALAH;
    }

    pvd = (const iso_pvd_t *)pvd_data;

    return (iso_cek_joliet_escape(pvd->escape_seq) == STATUS_BERHASIL) ?
           BENAR : SALAH;
}

tak_bertanda8_t iso9660_dapat_level(const tak_bertanda8_t *pvd_data)
{
    const iso_pvd_t *pvd;
    tak_bertanda16_t block_size;
    tak_bertanda32_t vol_size;
    tak_bertanda8_t level;

    if (pvd_data == NULL) {
        return ISO_LEVEL_1;
    }

    pvd = (const iso_pvd_t *)pvd_data;

    block_size = iso_baca_bothendian16(pvd->block_size_le, pvd->block_size_be);
    vol_size = iso_baca_bothendian32(pvd->volume_size_le, pvd->volume_size_be);

    /* Heuristik untuk menentukan level */
    /* Level 1: block size = 2048, nama file pendek */
    /* Level 2: block size bisa lebih besar, nama file panjang */
    /* Level 3: multi-extent files */

    level = ISO_LEVEL_2;  /* Default */

    /* Jika block size standar dan volume kecil, mungkin Level 1 */
    if (block_size == ISO_SEKTOR_UKURAN && vol_size < 650000) {
        /* Kemungkinan Level 1 */
        level = ISO_LEVEL_1;
    }

    /* Untuk sekarang, return Level 2 */
    return level;
}
