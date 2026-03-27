/*
 * PIGURA OS - JOLIET.C
 * =====================
 * Implementasi ekstensi Joliet untuk filesystem ISO9660.
 *
 * Joliet adalah ekstensi Microsoft untuk ISO9660 yang mendukung
 * nama file Unicode (UCS-2) hingga 64 karakter.
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
 * KONSTANTA JOLIET (JOLIET CONSTANTS)
 * ===========================================================================
 */

/* Escape sequence untuk Joliet */
#define JOLIET_ESCAPE_LEVEL_1    "%/@"   /* UCS-2 Level 1 */
#define JOLIET_ESCAPE_LEVEL_2    "%/C"   /* UCS-2 Level 2 */
#define JOLIET_ESCAPE_LEVEL_3    "%/E"   /* UCS-2 Level 3 */

/* Panjang escape sequence */
#define JOLIET_ESCAPE_LEN        3

/* Panjang nama maksimum Joliet (karakter) */
#define JOLIET_NAMA_MAKS         64

/* Panjang nama dalam byte (UCS-2 = 2 byte per karakter) */
#define JOLIET_NAMA_BYTE_MAKS    (JOLIET_NAMA_MAKS * 2)

/* Level Joliet */
#define JOLIET_LEVEL_1           1
#define JOLIET_LEVEL_2           2
#define JOLIET_LEVEL_3           3

/* Supplementary Volume Descriptor type */
#define JOLIET_VD_SUPPLEMENTARY  0x02

/* Versi SVD */
#define JOLIET_VERSI             0x01

/* Magic string ISO9660 */
#define JOLIET_MAGIC             "CD001"

/*
 * ===========================================================================
 * STRUKTUR DATA JOLIET (JOLIET DATA STRUCTURES)
 * ===========================================================================
 */

/* Struktur Supplementary Volume Descriptor */
typedef struct PACKED {
    tak_bertanda8_t vd_tipe;                    /* Tipe (0x02) */
    tak_bertanda8_t identitas[5];               /* "CD001" */
    tak_bertanda8_t versi;                      /* Versi (1) */
    tak_bertanda8_t flags;                      /* Flags */
    tak_bertanda8_t sistem_id[32];               /* System identifier */
    tak_bertanda8_t volume_id[32];               /* Volume identifier (UCS-2) */
    tak_bertanda8_t padding[8];                 /* Padding */
    tak_bertanda32_t volume_size_le;            /* Volume size LE */
    tak_bertanda32_t volume_size_be;            /* Volume size BE */
    tak_bertanda8_t escape_seq[32];             /* Escape sequences */
    tak_bertanda16_t volume_set_size_le;        /* Volume set size LE */
    tak_bertanda16_t volume_set_size_be;        /* Volume set size BE */
    tak_bertanda16_t volume_seq_le;             /* Volume sequence LE */
    tak_bertanda16_t volume_seq_be;             /* Volume sequence BE */
    tak_bertanda16_t block_size_le;             /* Block size LE */
    tak_bertanda16_t block_size_be;             /* Block size BE */
    tak_bertanda32_t path_table_size_le;        /* Path table size LE */
    tak_bertanda32_t path_table_size_be;        /* Path table size BE */
    tak_bertanda32_t path_table_l_le;           /* Path table L LE */
    tak_bertanda32_t path_table_l_opt_le;       /* Path table L opt LE */
    tak_bertanda32_t path_table_m_be;           /* Path table M BE */
    tak_bertanda32_t path_table_m_opt_be;       /* Path table M opt BE */
    tak_bertanda8_t root_dir[34];               /* Root directory record */
    tak_bertanda8_t volume_set_id[128];         /* Volume set ID (UCS-2) */
    tak_bertanda8_t publisher_id[128];          /* Publisher ID (UCS-2) */
    tak_bertanda8_t preparer_id[128];           /* Preparer ID (UCS-2) */
    tak_bertanda8_t application_id[128];        /* Application ID (UCS-2) */
    tak_bertanda8_t copyright[37];              /* Copyright file */
    tak_bertanda8_t abstract_file[37];          /* Abstract file */
    tak_bertanda8_t bibliographic[37];          /* Bibliographic file */
    tak_bertanda8_t dibuat[17];                 /* Creation time */
    tak_bertanda8_t dimodifikasi[17];           /* Modification time */
    tak_bertanda8_t kadaluarsa[17];             /* Expiration time */
    tak_bertanda8_t berlaku[17];                /* Effective time */
} joliet_svd_t;

/* Struktur informasi Joliet */
typedef struct {
    bool_t valid;                                /* Apakah Joliet valid? */
    tak_bertanda8_t level;                       /* Level Joliet */
    tak_bertanda32_t root_extent;               /* Root extent */
    tak_bertanda32_t root_size;                 /* Root size */
    tak_bertanda16_t block_size;                /* Block size */
    tak_bertanda32_t path_table_loc;            /* Path table location */
    tak_bertanda32_t path_table_size;           /* Path table size */
    char volume_id[JOLIET_NAMA_MAKS + 1];       /* Volume ID (UTF-8) */
} joliet_info_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INTERNAL (INTERNAL FUNCTION DECLARATIONS)
 * ===========================================================================
 */

static bool_t joliet_cek_escape(const tak_bertanda8_t *escape_seq,
                                tak_bertanda8_t *level);
static tak_bertanda16_t joliet_baca_bothendian16(tak_bertanda16_t le,
                                                tak_bertanda16_t be);
static tak_bertanda32_t joliet_baca_bothendian32(tak_bertanda32_t le,
                                                tak_bertanda32_t be);
static void joliet_ucs2_to_utf8(const tak_bertanda8_t *ucs2,
                                ukuran_t ucs2_len,
                                char *utf8,
                                ukuran_t utf8_size);
static void joliet_trim_ucs2(const tak_bertanda8_t *src,
                             ukuran_t src_len,
                             tak_bertanda8_t *dst,
                             ukuran_t *dst_len);
static status_t joliet_parse_svd(const joliet_svd_t *svd,
                                 joliet_info_t *info);

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI KONVERSI (CONVERSION FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

static tak_bertanda16_t joliet_baca_bothendian16(tak_bertanda16_t le,
                                                tak_bertanda16_t be)
{
    tak_bertanda16_t be_swapped;

    be_swapped = (tak_bertanda16_t)(
        ((be & 0x00FF) << 8) |
        ((be & 0xFF00) >> 8)
    );

    if (le != be_swapped) {
        return le;
    }

    return le;
}

static tak_bertanda32_t joliet_baca_bothendian32(tak_bertanda32_t le,
                                                tak_bertanda32_t be)
{
    tak_bertanda32_t be_swapped;

    be_swapped = ((be & 0x000000FF) << 24) |
                 ((be & 0x0000FF00) << 8) |
                 ((be & 0x00FF0000) >> 8) |
                 ((be & 0xFF000000) >> 24);

    if (le != be_swapped) {
        return le;
    }

    return le;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI UCS-2 (UCS-2 FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

static void joliet_ucs2_to_utf8(const tak_bertanda8_t *ucs2,
                                ukuran_t ucs2_len,
                                char *utf8,
                                ukuran_t utf8_size)
{
    ukuran_t i;
    ukuran_t j;
    tak_bertanda16_t ch;

    if (ucs2 == NULL || utf8 == NULL || utf8_size == 0) {
        return;
    }

    j = 0;

    for (i = 0; i + 1 < ucs2_len && j < utf8_size - 1; i += 2) {
        /* Baca karakter UCS-2 (little-endian) */
        ch = (tak_bertanda16_t)(ucs2[i] | (ucs2[i + 1] << 8));

        /* Konversi ke UTF-8 */
        if (ch < 0x80) {
            /* ASCII - 1 byte */
            utf8[j++] = (char)ch;
        } else if (ch < 0x800) {
            /* 2-byte UTF-8 */
            if (j + 2 >= utf8_size) {
                break;
            }
            utf8[j++] = (char)(0xC0 | ((ch >> 6) & 0x1F));
            utf8[j++] = (char)(0x80 | (ch & 0x3F));
        } else {
            /* 3-byte UTF-8 */
            if (j + 3 >= utf8_size) {
                break;
            }
            utf8[j++] = (char)(0xE0 | ((ch >> 12) & 0x0F));
            utf8[j++] = (char)(0x80 | ((ch >> 6) & 0x3F));
            utf8[j++] = (char)(0x80 | (ch & 0x3F));
        }
    }

    utf8[j] = '\0';
}

static void joliet_trim_ucs2(const tak_bertanda8_t *src,
                             ukuran_t src_len,
                             tak_bertanda8_t *dst,
                             ukuran_t *dst_len)
{
    ukuran_t i;
    ukuran_t len;

    if (src == NULL || dst == NULL || dst_len == NULL) {
        return;
    }

    /* Cari panjang aktual tanpa trailing spaces (UCS-2) */
    len = src_len;
    while (len >= 2) {
        if (src[len - 2] == 0x00 && src[len - 1] == 0x00) {
            len -= 2;  /* Null terminator */
        } else if (src[len - 2] == 0x00 && src[len - 1] == 0x20) {
            len -= 2;  /* Space */
        } else {
            break;
        }
    }

    /* Copy */
    for (i = 0; i < len; i++) {
        dst[i] = src[i];
    }

    *dst_len = len;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI DETEKSI (DETECTION FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

static bool_t joliet_cek_escape(const tak_bertanda8_t *escape_seq,
                                tak_bertanda8_t *level)
{
    ukuran_t i;

    if (escape_seq == NULL || level == NULL) {
        return SALAH;
    }

    *level = 0;

    /* Cari escape sequence Joliet dalam 32 byte */
    for (i = 0; i <= 32 - JOLIET_ESCAPE_LEN; i++) {
        if (escape_seq[i] == '%' && escape_seq[i + 1] == '/') {
            tak_bertanda8_t code = escape_seq[i + 2];

            switch (code) {
            case '@':
                *level = JOLIET_LEVEL_1;
                return BENAR;

            case 'C':
                *level = JOLIET_LEVEL_2;
                return BENAR;

            case 'E':
                *level = JOLIET_LEVEL_3;
                return BENAR;

            default:
                break;
            }
        }
    }

    return SALAH;
}

bool_t joliet_apakah_tersedia(const tak_bertanda8_t *svd_data)
{
    const joliet_svd_t *svd;
    tak_bertanda8_t level;

    if (svd_data == NULL) {
        return SALAH;
    }

    svd = (const joliet_svd_t *)svd_data;

    /* Cek tipe */
    if (svd->vd_tipe != JOLIET_VD_SUPPLEMENTARY) {
        return SALAH;
    }

    /* Cek magic */
    if (svd->identitas[0] != 'C' || svd->identitas[1] != 'D' ||
        svd->identitas[2] != '0' || svd->identitas[3] != '0' ||
        svd->identitas[4] != '1') {
        return SALAH;
    }

    /* Cek escape sequence */
    return joliet_cek_escape(svd->escape_seq, &level);
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI PARSING SVD (SVD PARSING FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

static status_t joliet_parse_svd(const joliet_svd_t *svd,
                                 joliet_info_t *info)
{
    tak_bertanda8_t level;
    tak_bertanda8_t trimmed_name[64];
    ukuran_t trimmed_len;

    if (svd == NULL || info == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memset(info, 0, sizeof(joliet_info_t));

    /* Validasi tipe */
    if (svd->vd_tipe != JOLIET_VD_SUPPLEMENTARY) {
        return STATUS_FORMAT_INVALID;
    }

    /* Validasi versi */
    if (svd->versi != JOLIET_VERSI) {
        return STATUS_FORMAT_INVALID;
    }

    /* Cek escape sequence dan level */
    if (!joliet_cek_escape(svd->escape_seq, &level)) {
        return STATUS_FORMAT_INVALID;
    }

    info->valid = BENAR;
    info->level = level;

    /* Parse block size */
    info->block_size = joliet_baca_bothendian16(svd->block_size_le,
                                                svd->block_size_be);

    /* Parse path table */
    info->path_table_loc = svd->path_table_l_le;
    info->path_table_size = joliet_baca_bothendian32(svd->path_table_size_le,
                                                     svd->path_table_size_be);

    /* Parse root directory record */
    {
        const tak_bertanda8_t *root = svd->root_dir;
        tak_bertanda8_t rec_len = root[0];

        if (rec_len >= 33) {
            tak_bertanda32_t extent_le;
            tak_bertanda32_t extent_be;
            tak_bertanda32_t size_le;
            tak_bertanda32_t size_be;

            /* Extent location (offset 2-9) */
            extent_le = *(const tak_bertanda32_t *)(root + 2);
            extent_be = *(const tak_bertanda32_t *)(root + 6);
            info->root_extent = joliet_baca_bothendian32(extent_le, extent_be);

            /* Size (offset 10-17) */
            size_le = *(const tak_bertanda32_t *)(root + 10);
            size_be = *(const tak_bertanda32_t *)(root + 14);
            info->root_size = joliet_baca_bothendian32(size_le, size_be);
        }
    }

    /* Parse volume ID (UCS-2) */
    joliet_trim_ucs2(svd->volume_id, 32, trimmed_name, &trimmed_len);
    joliet_ucs2_to_utf8(trimmed_name, trimmed_len,
                        info->volume_id, sizeof(info->volume_id));

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI UTAMA (MAIN FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

status_t joliet_parse(const tak_bertanda8_t *svd_data,
                      ukuran_t data_len,
                      joliet_info_t *info)
{
    const joliet_svd_t *svd;

    if (svd_data == NULL || info == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (data_len < sizeof(joliet_svd_t)) {
        return STATUS_PARAM_UKURAN;
    }

    svd = (const joliet_svd_t *)svd_data;

    return joliet_parse_svd(svd, info);
}

status_t joliet_dapat_nama(const tak_bertanda8_t *ucs2_data,
                           ukuran_t ucs2_len,
                           char *buffer,
                           ukuran_t buffer_size)
{
    tak_bertanda8_t trimmed[JOLIET_NAMA_BYTE_MAKS];
    ukuran_t trimmed_len;

    if (ucs2_data == NULL || buffer == NULL || buffer_size == 0) {
        return STATUS_PARAM_NULL;
    }

    /* Trim UCS-2 string */
    joliet_trim_ucs2(ucs2_data, ucs2_len, trimmed, &trimmed_len);

    /* Konversi ke UTF-8 */
    joliet_ucs2_to_utf8(trimmed, trimmed_len, buffer, buffer_size);

    return STATUS_BERHASIL;
}

status_t joliet_dapat_volume_id(const tak_bertanda8_t *svd_data,
                                char *buffer,
                                ukuran_t buffer_size)
{
    const joliet_svd_t *svd;
    tak_bertanda8_t trimmed[64];
    ukuran_t trimmed_len;

    if (svd_data == NULL || buffer == NULL || buffer_size == 0) {
        return STATUS_PARAM_NULL;
    }

    svd = (const joliet_svd_t *)svd_data;

    joliet_trim_ucs2(svd->volume_id, 32, trimmed, &trimmed_len);
    joliet_ucs2_to_utf8(trimmed, trimmed_len, buffer, buffer_size);

    return STATUS_BERHASIL;
}

status_t joliet_dapat_root_dir(const tak_bertanda8_t *svd_data,
                               tak_bertanda32_t *extent,
                               tak_bertanda32_t *size)
{
    joliet_info_t info;
    status_t status;

    if (extent == NULL || size == NULL) {
        return STATUS_PARAM_NULL;
    }

    status = joliet_parse(svd_data, sizeof(joliet_svd_t), &info);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    if (!info.valid) {
        return STATUS_FORMAT_INVALID;
    }

    *extent = info.root_extent;
    *size = info.root_size;

    return STATUS_BERHASIL;
}

tak_bertanda8_t joliet_dapat_level(const tak_bertanda8_t *svd_data)
{
    const joliet_svd_t *svd;
    tak_bertanda8_t level;

    if (svd_data == NULL) {
        return 0;
    }

    svd = (const joliet_svd_t *)svd_data;

    if (joliet_cek_escape(svd->escape_seq, &level)) {
        return level;
    }

    return 0;
}

status_t joliet_dapat_path_table(const tak_bertanda8_t *svd_data,
                                 tak_bertanda32_t *location,
                                 tak_bertanda32_t *size)
{
    joliet_info_t info;
    status_t status;

    if (location == NULL || size == NULL) {
        return STATUS_PARAM_NULL;
    }

    status = joliet_parse(svd_data, sizeof(joliet_svd_t), &info);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    if (!info.valid) {
        return STATUS_FORMAT_INVALID;
    }

    *location = info.path_table_loc;
    *size = info.path_table_size;

    return STATUS_BERHASIL;
}

tak_bertanda16_t joliet_dapat_block_size(const tak_bertanda8_t *svd_data)
{
    const joliet_svd_t *svd;

    if (svd_data == NULL) {
        return 0;
    }

    svd = (const joliet_svd_t *)svd_data;

    return joliet_baca_bothendian16(svd->block_size_le, svd->block_size_be);
}

/*
 * ===========================================================================
 * FUNGSI KONVERSI NAMA FILE (FILE NAME CONVERSION FUNCTIONS)
 * ===========================================================================
 */

status_t joliet_konversi_nama_file(const tak_bertanda8_t *nama_ucs2,
                                   ukuran_t nama_len,
                                   char *buffer,
                                   ukuran_t buffer_size)
{
    tak_bertanda8_t trimmed[JOLIET_NAMA_BYTE_MAKS];
    ukuran_t trimmed_len;
    ukuran_t i;

    if (nama_ucs2 == NULL || buffer == NULL || buffer_size == 0) {
        return STATUS_PARAM_NULL;
    }

    /* Trim trailing spaces dan null */
    trimmed_len = nama_len;
    while (trimmed_len >= 2) {
        bool_t is_space = (nama_ucs2[trimmed_len - 2] == 0x00 &&
                           nama_ucs2[trimmed_len - 1] == 0x20);
        bool_t is_null = (nama_ucs2[trimmed_len - 2] == 0x00 &&
                          nama_ucs2[trimmed_len - 1] == 0x00);

        if (is_space || is_null) {
            trimmed_len -= 2;
        } else {
            break;
        }
    }

    /* Copy trimmed */
    for (i = 0; i < trimmed_len && i < JOLIET_NAMA_BYTE_MAKS; i++) {
        trimmed[i] = nama_ucs2[i];
    }

    /* Konversi ke UTF-8 */
    joliet_ucs2_to_utf8(trimmed, trimmed_len, buffer, buffer_size);

    /* Handle separator versi (;1) */
    {
        ukuran_t len = kernel_strlen(buffer);
        if (len > 2 && buffer[len - 2] == ';' && buffer[len - 1] == '1') {
            buffer[len - 2] = '\0';
        }
    }

    return STATUS_BERHASIL;
}

bool_t joliet_nama_valid(const char *nama, ukuran_t len)
{
    ukuran_t i;

    if (nama == NULL || len == 0) {
        return SALAH;
    }

    if (len > JOLIET_NAMA_MAKS) {
        return SALAH;
    }

    for (i = 0; i < len; i++) {
        char c = nama[i];

        /* Karakter kontrol tidak valid */
        if (c < 32 && c != '\0') {
            return SALAH;
        }
    }

    return BENAR;
}
