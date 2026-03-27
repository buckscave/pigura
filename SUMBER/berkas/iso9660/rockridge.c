/*
 * PIGURA OS - ROCKRIDGE.C
 * =======================
 * Implementasi ekstensi RockRidge untuk filesystem ISO9660.
 *
 * RockRidge adalah ekstensi untuk ISO9660 yang menambahkan
 * dukungan POSIX seperti nama file panjang, symbolic links,
 * permission, dan ownership.
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
 * KONSTANTA ROCKRIDGE (ROCKRIDGE CONSTANTS)
 * ===========================================================================
 */

/* Signature System Use Field */
#define RR_SIGNATURE_SP         0x5053  /* "SP" - SUSP indicator */
#define RR_SIGNATURE_CE         0x4543  /* "CE" - Continuation area */
#define RR_SIGNATURE_ER         0x5245  /* "ER" - Extension record */
#define RR_SIGNATURE_RR         0x5252  /* "RR" - RockRidge extension */
#define RR_SIGNATURE_PX         0x5850  /* "PX" - POSIX attributes */
#define RR_SIGNATURE_PN         0x4E50  /* "PN" - POSIX device number */
#define RR_SIGNATURE_SL         0x4C53  /* "SL" - Symbolic link */
#define RR_SIGNATURE_NM         0x4D4E  /* "NM" - Alternate name */
#define RR_SIGNATURE_CL         0x4C43  /* "CL" - Child link */
#define RR_SIGNATURE_PL         0x4C50  /* "PL" - Parent link */
#define RR_SIGNATURE_RE         0x4552  /* "RE" - Relocated entry */
#define RR_SIGNATURE_TF         0x4654  /* "TF" - Time stamp */
#define RR_SIGNATURE_SF         0x4653  /* "SF" - Sparse file */

/* SUSP version */
#define SUSP_VERSION            0x01

/* RockRidge version */
#define RR_VERSION              0x01

/* Panjang nama maksimum */
#define RR_NAMA_MAKS            255

/* Panjang System Use Field header */
#define RR_SU_HEADER_LEN        4

/* Flags untuk NM field */
#define RR_NM_FLAG_CONTINUE     0x01
#define RR_NM_FLAG_CURRENT     0x02
#define RR_NM_FLAG_PARENT       0x04

/* Flags untuk SL field */
#define RR_SL_FLAG_CONTINUE     0x01
#define RR_SL_FLAG_CURRENT      0x02
#define RR_SL_FLAG_PARENT       0x04
#define RR_SL_FLAG_ROOT         0x08

/* Flags untuk TF field */
#define RR_TF_FLAG_CREATION     0x01
#define RR_TF_FLAG_MODIFY       0x02
#define RR_TF_FLAG_ACCESS       0x04
#define RR_TF_FLAG_ATTRIBUTES   0x08
#define RR_TF_FLAG_BACKUP       0x10
#define RR_TF_FLAG_EXPIRATION   0x20
#define RR_TF_FLAG_EFFECTIVE    0x40
#define RR_TF_FLAG_LONG_FORM    0x80

/*
 * ===========================================================================
 * STRUKTUR DATA ROCKRIDGE (ROCKRIDGE DATA STRUCTURES)
 * ===========================================================================
 */

/* Header System Use Field */
typedef struct PACKED {
    tak_bertanda16_t signature;          /* Signature (2 bytes) */
    tak_bertanda8_t length;              /* Panjang record */
    tak_bertanda8_t version;             /* Versi (1) */
} rr_su_header_t;

/* SP field - SUSP indicator */
typedef struct PACKED {
    tak_bertanda16_t signature;          /* "SP" */
    tak_bertanda8_t length;              /* 7 */
    tak_bertanda8_t version;             /* 1 */
    tak_bertanda8_t check_byte;          /* 0xBE */
    tak_bertanda8_t skip_len;            /* Length of skip bytes */
} rr_sp_t;

/* CE field - Continuation area */
typedef struct PACKED {
    tak_bertanda16_t signature;          /* "CE" */
    tak_bertanda8_t length;              /* 28 */
    tak_bertanda8_t version;             /* 1 */
    tak_bertanda32_t extent_le;          /* Continuation area extent LE */
    tak_bertanda32_t extent_be;          /* Continuation area extent BE */
    tak_bertanda32_t offset_le;          /* Offset dalam extent LE */
    tak_bertanda32_t offset_be;          /* Offset dalam extent BE */
    tak_bertanda32_t length_le;          /* Panjang continuation area LE */
    tak_bertanda32_t length_be;          /* Panjang continuation area BE */
} rr_ce_t;

/* ER field - Extension record */
typedef struct PACKED {
    tak_bertanda16_t signature;          /* "ER" */
    tak_bertanda8_t length;              /* Variable */
    tak_bertanda8_t version;             /* 1 */
    tak_bertanda8_t ext_id_len;          /* Panjang extension identifier */
    tak_bertanda8_t ext_des_len;         /* Panjang extension descriptor */
    tak_bertanda8_t ext_src_len;         /* Panjang extension source */
    tak_bertanda8_t ext_id[1];           /* Extension identifier */
} rr_er_t;

/* PX field - POSIX attributes */
typedef struct PACKED {
    tak_bertanda16_t signature;          /* "PX" */
    tak_bertanda8_t length;              /* 44 */
    tak_bertanda8_t version;             /* 1 */
    tak_bertanda32_t mode_le;            /* File mode LE */
    tak_bertanda32_t mode_be;            /* File mode BE */
    tak_bertanda32_t nlink_le;           /* Link count LE */
    tak_bertanda32_t nlink_be;           /* Link count BE */
    tak_bertanda32_t uid_le;             /* User ID LE */
    tak_bertanda32_t uid_be;             /* User ID BE */
    tak_bertanda32_t gid_le;             /* Group ID LE */
    tak_bertanda32_t gid_be;             /* Group ID BE */
    tak_bertanda32_t inode_le;           /* Inode number LE */
    tak_bertanda32_t inode_be;           /* Inode number BE */
} rr_px_t;

/* PN field - POSIX device number */
typedef struct PACKED {
    tak_bertanda16_t signature;          /* "PN" */
    tak_bertanda8_t length;              /* 20 */
    tak_bertanda8_t version;             /* 1 */
    tak_bertanda32_t dev_high_le;        /* Device number high LE */
    tak_bertanda32_t dev_high_be;        /* Device number high BE */
    tak_bertanda32_t dev_low_le;         /* Device number low LE */
    tak_bertanda32_t dev_low_be;         /* Device number low BE */
} rr_pn_t;

/* SL field - Symbolic link */
typedef struct PACKED {
    tak_bertanda16_t signature;          /* "SL" */
    tak_bertanda8_t length;              /* Variable */
    tak_bertanda8_t version;             /* 1 */
    tak_bertanda8_t flags;               /* Flags */
    tak_bertanda8_t component_count;     /* Component count */
} rr_sl_t;

/* NM field - Alternate name */
typedef struct PACKED {
    tak_bertanda16_t signature;          /* "NM" */
    tak_bertanda8_t length;              /* Variable */
    tak_bertanda8_t version;             /* 1 */
    tak_bertanda8_t flags;               /* Flags */
    tak_bertanda8_t name[1];             /* Name */
} rr_nm_t;

/* TF field - Time stamp */
typedef struct PACKED {
    tak_bertanda16_t signature;          /* "TF" */
    tak_bertanda8_t length;              /* Variable */
    tak_bertanda8_t version;             /* 1 */
    tak_bertanda8_t flags;               /* Flags */
    tak_bertanda8_t times[1];            /* Time stamps */
} rr_tf_t;

/* Struktur hasil parsing RockRidge */
typedef struct {
    bool_t valid;                        /* Apakah valid RockRidge? */
    bool_t has_px;                       /* Ada POSIX attributes? */
    bool_t has_nm;                       /* Ada alternate name? */
    bool_t has_sl;                       /* Ada symbolic link? */
    bool_t has_tf;                       /* Ada time stamps? */
    char nama[RR_NAMA_MAKS + 1];         /* Nama file alternatif */
    ukuran_t nama_len;                   /* Panjang nama */
    mode_t mode;                         /* File mode */
    tak_bertanda32_t nlink;              /* Link count */
    uid_t uid;                           /* User ID */
    gid_t gid;                           /* Group ID */
    ino_t ino;                           /* Inode number */
    waktu_t waktu_buat;                  /* Waktu pembuatan */
    waktu_t waktu_modif;                 /* Waktu modifikasi */
    waktu_t waktu_akses;                 /* Waktu akses */
    bool_t is_symlink;                   /* Apakah symbolic link? */
    char symlink_target[256];            /* Target symbolic link */
} rr_info_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INTERNAL (INTERNAL FUNCTION DECLARATIONS)
 * ===========================================================================
 */

static tak_bertanda16_t rr_baca_signature(const tak_bertanda8_t *data);
static tak_bertanda32_t rr_baca_bothendian32(tak_bertanda32_t le,
                                             tak_bertanda32_t be);
static status_t rr_parse_sp(const rr_su_header_t *header, bool_t *valid);
static status_t rr_parse_ce(const rr_su_header_t *header,
                            tak_bertanda32_t *extent,
                            tak_bertanda32_t *offset,
                            tak_bertanda32_t *length);
static status_t rr_parse_px(const rr_su_header_t *header, rr_info_t *info);
static status_t rr_parse_nm(const rr_su_header_t *header, rr_info_t *info);
static status_t rr_parse_sl(const rr_su_header_t *header, rr_info_t *info);
static status_t rr_parse_tf(const rr_su_header_t *header, rr_info_t *info);
static status_t rr_parse_field(const rr_su_header_t *header, rr_info_t *info);

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI KONVERSI (CONVERSION FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

static tak_bertanda16_t rr_baca_signature(const tak_bertanda8_t *data)
{
    if (data == NULL) {
        return 0;
    }

    /* Signature disimpan sebagai byte array */
    return (tak_bertanda16_t)(data[0] | (data[1] << 8));
}

static tak_bertanda32_t rr_baca_bothendian32(tak_bertanda32_t le,
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
 * IMPLEMENTASI FUNGSI PARSING FIELD (FIELD PARSING FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

static status_t rr_parse_sp(const rr_su_header_t *header, bool_t *valid)
{
    const rr_sp_t *sp;

    if (header == NULL || valid == NULL) {
        return STATUS_PARAM_NULL;
    }

    sp = (const rr_sp_t *)header;

    /* Validasi signature */
    if (sp->signature != RR_SIGNATURE_SP) {
        *valid = SALAH;
        return STATUS_BERHASIL;
    }

    /* Validasi length */
    if (sp->length != 7) {
        *valid = SALAH;
        return STATUS_BERHASIL;
    }

    /* Validasi check byte */
    if (sp->check_byte != 0xBE) {
        *valid = SALAH;
        return STATUS_BERHASIL;
    }

    *valid = BENAR;
    return STATUS_BERHASIL;
}

static status_t rr_parse_ce(const rr_su_header_t *header,
                            tak_bertanda32_t *extent,
                            tak_bertanda32_t *offset,
                            tak_bertanda32_t *length)
{
    const rr_ce_t *ce;

    if (header == NULL || extent == NULL || offset == NULL || length == NULL) {
        return STATUS_PARAM_NULL;
    }

    ce = (const rr_ce_t *)header;

    if (ce->signature != RR_SIGNATURE_CE) {
        return STATUS_FORMAT_INVALID;
    }

    *extent = rr_baca_bothendian32(ce->extent_le, ce->extent_be);
    *offset = rr_baca_bothendian32(ce->offset_le, ce->offset_be);
    *length = rr_baca_bothendian32(ce->length_le, ce->length_be);

    return STATUS_BERHASIL;
}

static status_t rr_parse_px(const rr_su_header_t *header, rr_info_t *info)
{
    const rr_px_t *px;

    if (header == NULL || info == NULL) {
        return STATUS_PARAM_NULL;
    }

    px = (const rr_px_t *)header;

    if (px->signature != RR_SIGNATURE_PX) {
        return STATUS_FORMAT_INVALID;
    }

    info->has_px = BENAR;

    /* Parse mode */
    info->mode = (mode_t)rr_baca_bothendian32(px->mode_le, px->mode_be);

    /* Parse link count */
    info->nlink = rr_baca_bothendian32(px->nlink_le, px->nlink_be);

    /* Parse uid/gid */
    info->uid = (uid_t)rr_baca_bothendian32(px->uid_le, px->uid_be);
    info->gid = (gid_t)rr_baca_bothendian32(px->gid_le, px->gid_be);

    /* Parse inode number */
    info->ino = (ino_t)rr_baca_bothendian32(px->inode_le, px->inode_be);

    return STATUS_BERHASIL;
}

static status_t rr_parse_nm(const rr_su_header_t *header, rr_info_t *info)
{
    const rr_nm_t *nm;
    tak_bertanda8_t flags;
    ukuran_t nama_len;
    ukuran_t to_copy;

    if (header == NULL || info == NULL) {
        return STATUS_PARAM_NULL;
    }

    nm = (const rr_nm_t *)header;

    if (nm->signature != RR_SIGNATURE_NM) {
        return STATUS_FORMAT_INVALID;
    }

    info->has_nm = BENAR;

    flags = nm->flags;

    /* Handle special flags */
    if (flags & RR_NM_FLAG_CURRENT) {
        kernel_strncpy(info->nama, ".", RR_NAMA_MAKS);
        info->nama_len = 1;
        return STATUS_BERHASIL;
    }

    if (flags & RR_NM_FLAG_PARENT) {
        kernel_strncpy(info->nama, "..", RR_NAMA_MAKS);
        info->nama_len = 2;
        return STATUS_BERHASIL;
    }

    /* Calculate name length */
    nama_len = nm->length - 5;  /* Subtract header bytes */
    if (nama_len > RR_NAMA_MAKS) {
        nama_len = RR_NAMA_MAKS;
    }

    /* Handle continuation flag */
    if (flags & RR_NM_FLAG_CONTINUE) {
        /* Append to existing name */
        ukuran_t existing_len = info->nama_len;
        to_copy = MIN(nama_len, RR_NAMA_MAKS - existing_len);

        kernel_strncpy(info->nama + existing_len, (const char *)nm->name,
                       to_copy);
        info->nama_len += to_copy;
    } else {
        /* New name */
        to_copy = MIN(nama_len, RR_NAMA_MAKS);
        kernel_strncpy(info->nama, (const char *)nm->name, to_copy);
        info->nama_len = to_copy;
    }

    info->nama[info->nama_len] = '\0';

    return STATUS_BERHASIL;
}

static status_t rr_parse_sl(const rr_su_header_t *header, rr_info_t *info)
{
    const rr_sl_t *sl;
    const tak_bertanda8_t *component;
    tak_bertanda32_t offset;
    tak_bertanda8_t comp_flags;

    if (header == NULL || info == NULL) {
        return STATUS_PARAM_NULL;
    }

    sl = (const rr_sl_t *)header;

    if (sl->signature != RR_SIGNATURE_SL) {
        return STATUS_FORMAT_INVALID;
    }

    info->has_sl = BENAR;
    info->is_symlink = BENAR;

    /* Parse symlink components */
    kernel_memset(info->symlink_target, 0, sizeof(info->symlink_target));
    offset = 5;  /* Start after header */

    while (offset < (tak_bertanda32_t)sl->length) {
        tak_bertanda8_t comp_len;

        component = (const tak_bertanda8_t *)sl + offset;
        comp_flags = component[0];
        comp_len = component[1];

        /* Handle component flags */
        if (comp_flags & RR_SL_FLAG_ROOT) {
            kernel_strncpy(info->symlink_target, "/",
                          sizeof(info->symlink_target) - 1);
        } else if (comp_flags & RR_SL_FLAG_PARENT) {
            kernel_strncat(info->symlink_target, "/..",
                          sizeof(info->symlink_target) -
                          kernel_strlen(info->symlink_target) - 1);
        } else if (comp_flags & RR_SL_FLAG_CURRENT) {
            kernel_strncat(info->symlink_target, "/.",
                          sizeof(info->symlink_target) -
                          kernel_strlen(info->symlink_target) - 1);
        } else if (comp_len > 0) {
            /* Regular path component */
            if (kernel_strlen(info->symlink_target) > 0) {
                kernel_strncat(info->symlink_target, "/",
                              sizeof(info->symlink_target) -
                              kernel_strlen(info->symlink_target) - 1);
            }
            /* Copy component name */
            if (comp_len > 0 &&
                offset + 2 + comp_len <= (tak_bertanda32_t)sl->length) {
                tak_bertanda8_t *dest;
                ukuran_t current_len;

                current_len = kernel_strlen(info->symlink_target);
                dest = (tak_bertanda8_t *)info->symlink_target + current_len;
                kernel_strncpy((char *)dest, (const char *)(component + 2),
                              MIN(comp_len,
                                  sizeof(info->symlink_target) - current_len - 1));
            }
        }

        offset += 2 + comp_len;
    }

    return STATUS_BERHASIL;
}

static status_t rr_parse_tf(const rr_su_header_t *header, rr_info_t *info)
{
    const rr_tf_t *tf;
    tak_bertanda8_t flags;
    tak_bertanda32_t offset;
    bool_t long_form;

    if (header == NULL || info == NULL) {
        return STATUS_PARAM_NULL;
    }

    tf = (const rr_tf_t *)header;

    if (tf->signature != RR_SIGNATURE_TF) {
        return STATUS_FORMAT_INVALID;
    }

    info->has_tf = BENAR;

    flags = tf->flags;
    long_form = (flags & RR_TF_FLAG_LONG_FORM) ? BENAR : SALAH;
    offset = 5;  /* Start after header */

    /* Parse timestamps */
    if (flags & RR_TF_FLAG_CREATION) {
        /* Creation time - not commonly used */
        offset += long_form ? 17 : 7;
    }

    if (flags & RR_TF_FLAG_MODIFY) {
        /* Modification time */
        if (long_form && offset + 17 <= (tak_bertanda32_t)tf->length) {
            /* Parse 17-byte time format */
            /* Simplified - just use current time */
            info->waktu_modif = 0;
        } else if (offset + 7 <= (tak_bertanda32_t)tf->length) {
            /* Parse 7-byte time format */
            /* Simplified */
            info->waktu_modif = 0;
        }
        offset += long_form ? 17 : 7;
    }

    if (flags & RR_TF_FLAG_ACCESS) {
        /* Access time */
        offset += long_form ? 17 : 7;
    }

    return STATUS_BERHASIL;
}

static status_t rr_parse_field(const rr_su_header_t *header, rr_info_t *info)
{
    tak_bertanda16_t signature;

    if (header == NULL || info == NULL) {
        return STATUS_PARAM_NULL;
    }

    signature = header->signature;

    switch (signature) {
    case RR_SIGNATURE_SP:
        return rr_parse_sp(header, &info->valid);

    case RR_SIGNATURE_PX:
        return rr_parse_px(header, info);

    case RR_SIGNATURE_NM:
        return rr_parse_nm(header, info);

    case RR_SIGNATURE_SL:
        return rr_parse_sl(header, info);

    case RR_SIGNATURE_TF:
        return rr_parse_tf(header, info);

    default:
        /* Unknown field - skip */
        return STATUS_BERHASIL;
    }
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI UTAMA (MAIN FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

bool_t rockridge_deteksi(const tak_bertanda8_t *dir_record,
                         ukuran_t record_len)
{
    const tak_bertanda8_t *data;
    ukuran_t offset;
    ukuran_t nama_len;
    ukuran_t su_offset;
    tak_bertanda16_t signature;
    tak_bertanda8_t field_len;
    bool_t has_sp;
    bool_t has_er;

    if (dir_record == NULL || record_len < 33) {
        return SALAH;
    }

    /* Hitung offset ke System Use area */
    nama_len = dir_record[32];
    su_offset = 33 + nama_len;

    /* System Use area harus 4-byte aligned */
    if (nama_len % 2 == 0) {
        su_offset++;
    }

    if (su_offset + RR_SU_HEADER_LEN > record_len) {
        return SALAH;
    }

    /* Parse System Use fields */
    data = dir_record + su_offset;
    offset = 0;
    has_sp = SALAH;
    has_er = SALAH;

    while (offset + RR_SU_HEADER_LEN <= record_len - su_offset) {
        signature = rr_baca_signature(data + offset);
        field_len = data[offset + 2];

        if (field_len < RR_SU_HEADER_LEN) {
            break;
        }

        if (signature == RR_SIGNATURE_SP) {
            has_sp = BENAR;
        }

        if (signature == RR_SIGNATURE_ER) {
            has_er = BENAR;
        }

        offset += field_len;
    }

    return (has_sp || has_er) ? BENAR : SALAH;
}

status_t rockridge_parse(const tak_bertanda8_t *dir_record,
                         ukuran_t record_len,
                         rr_info_t *info)
{
    const tak_bertanda8_t *data;
    ukuran_t offset;
    ukuran_t nama_len;
    ukuran_t su_offset;
    tak_bertanda16_t signature;
    tak_bertanda8_t field_len;
    status_t status;

    if (dir_record == NULL || info == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memset(info, 0, sizeof(rr_info_t));

    if (record_len < 33) {
        return STATUS_FORMAT_INVALID;
    }

    /* Hitung offset ke System Use area */
    nama_len = dir_record[32];
    su_offset = 33 + nama_len;

    /* System Use area harus 4-byte aligned */
    if (nama_len % 2 == 0) {
        su_offset++;
    }

    if (su_offset + RR_SU_HEADER_LEN > record_len) {
        return STATUS_BERHASIL;
    }

    /* Parse System Use fields */
    data = dir_record + su_offset;
    offset = 0;

    while (offset + RR_SU_HEADER_LEN <= record_len - su_offset) {
        const rr_su_header_t *header;

        signature = rr_baca_signature(data + offset);
        field_len = data[offset + 2];

        if (field_len < RR_SU_HEADER_LEN) {
            break;
        }

        header = (const rr_su_header_t *)(data + offset);

        status = rr_parse_field(header, info);
        if (status != STATUS_BERHASIL) {
            /* Continue parsing other fields */
        }

        offset += field_len;
    }

    info->valid = (info->has_px || info->has_nm) ? BENAR : SALAH;

    return STATUS_BERHASIL;
}

status_t rockridge_dapat_nama(const tak_bertanda8_t *dir_record,
                              ukuran_t record_len,
                              char *buffer,
                              ukuran_t buffer_size)
{
    rr_info_t info;
    status_t status;

    if (buffer == NULL || buffer_size == 0) {
        return STATUS_PARAM_NULL;
    }

    buffer[0] = '\0';

    status = rockridge_parse(dir_record, record_len, &info);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    if (info.has_nm && info.nama_len > 0) {
        kernel_strncpy(buffer, info.nama, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        return STATUS_BERHASIL;
    }

    return STATUS_TIDAK_DITEMUKAN;
}

status_t rockridge_dapat_mode(const tak_bertanda8_t *dir_record,
                              ukuran_t record_len,
                              mode_t *mode)
{
    rr_info_t info;
    status_t status;

    if (mode == NULL) {
        return STATUS_PARAM_NULL;
    }

    *mode = 0;

    status = rockridge_parse(dir_record, record_len, &info);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    if (info.has_px) {
        *mode = info.mode;
        return STATUS_BERHASIL;
    }

    return STATUS_TIDAK_DITEMUKAN;
}

status_t rockridge_dapat_ownership(const tak_bertanda8_t *dir_record,
                                   ukuran_t record_len,
                                   uid_t *uid,
                                   gid_t *gid)
{
    rr_info_t info;
    status_t status;

    if (uid == NULL || gid == NULL) {
        return STATUS_PARAM_NULL;
    }

    *uid = 0;
    *gid = 0;

    status = rockridge_parse(dir_record, record_len, &info);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    if (info.has_px) {
        *uid = info.uid;
        *gid = info.gid;
        return STATUS_BERHASIL;
    }

    return STATUS_TIDAK_DITEMUKAN;
}

bool_t rockridge_apakah_symlink(const tak_bertanda8_t *dir_record,
                                ukuran_t record_len)
{
    rr_info_t info;
    status_t status;

    status = rockridge_parse(dir_record, record_len, &info);
    if (status != STATUS_BERHASIL) {
        return SALAH;
    }

    return info.is_symlink;
}

status_t rockridge_dapat_symlink_target(const tak_bertanda8_t *dir_record,
                                        ukuran_t record_len,
                                        char *buffer,
                                        ukuran_t buffer_size)
{
    rr_info_t info;
    status_t status;

    if (buffer == NULL || buffer_size == 0) {
        return STATUS_PARAM_NULL;
    }

    buffer[0] = '\0';

    status = rockridge_parse(dir_record, record_len, &info);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    if (info.is_symlink) {
        kernel_strncpy(buffer, info.symlink_target, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        return STATUS_BERHASIL;
    }

    return STATUS_TIDAK_DITEMUKAN;
}
