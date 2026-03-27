/*
 * PIGURA OS - NTFS_ATTRIB.C
 * ==========================
 * Operasi atribut NTFS untuk kernel Pigura OS.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk membaca dan
 * memproses berbagai jenis atribut dalam sistem file NTFS,
 * termasuk Standard Information, File Name, Data, dan lainnya.
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
 * KONSTANTA ATRIBUT (ATTRIBUTE CONSTANTS)
 * ===========================================================================
 */

/* Attribute types */
#define NTFS_ATTR_STANDARD_INFO     0x10
#define NTFS_ATTR_ATTRIBUTE_LIST    0x20
#define NTFS_ATTR_FILE_NAME         0x30
#define NTFS_ATTR_OBJECT_ID         0x40
#define NTFS_ATTR_SECURITY_DESC     0x50
#define NTFS_ATTR_VOLUME_NAME       0x60
#define NTFS_ATTR_VOLUME_INFO       0x70
#define NTFS_ATTR_DATA              0x80
#define NTFS_ATTR_INDEX_ROOT        0x90
#define NTFS_ATTR_INDEX_ALLOC       0xA0
#define NTFS_ATTR_BITMAP            0xB0
#define NTFS_ATTR_REPARSE_POINT     0xC0
#define NTFS_ATTR_EA_INFO           0xD0
#define NTFS_ATTR_EA                0xE0
#define NTFS_ATTR_PROPERTY_SET      0xF0
#define NTFS_ATTR_LOGGED_UTILITY    0x100
#define NTFS_ATTR_END               0xFFFFFFFF

/* Attribute flags */
#define NTFS_ATTR_FLAG_COMPRESSED   0x0001
#define NTFS_ATTR_FLAG_ENCRYPTED    0x4000
#define NTFS_ATTR_FLAG_SPARSE       0x8000

/* File name flags (Standard Information) */
#define NTFS_FILE_FLAG_READONLY     0x00000001
#define NTFS_FILE_FLAG_HIDDEN       0x00000002
#define NTFS_FILE_FLAG_SYSTEM       0x00000004
#define NTFS_FILE_FLAG_ARCHIVE      0x00000020
#define NTFS_FILE_FLAG_DEVICE       0x00000040
#define NTFS_FILE_FLAG_NORMAL       0x00000080
#define NTFS_FILE_FLAG_TEMPORARY    0x00000100
#define NTFS_FILE_FLAG_SPARSE_FILE  0x00000200
#define NTFS_FILE_FLAG_REPARSE      0x00000400
#define NTFS_FILE_FLAG_COMPRESSED   0x00000800
#define NTFS_FILE_FLAG_OFFLINE      0x00001000
#define NTFS_FILE_FLAG_NOT_INDEXED  0x00002000
#define NTFS_FILE_FLAG_ENCRYPTED    0x00004000
#define NTFS_FILE_FLAG_DIRECTORY    0x10000000
#define NTFS_FILE_FLAG_INDEX_VIEW   0x20000000

/* Maximum file name length */
#define NTFS_MAX_NAME_LEN           255

/* Name spaces */
#define NTFS_NAMESPACE_POSIX        0
#define NTFS_NAMESPACE_WIN32        1
#define NTFS_NAMESPACE_DOS          2
#define NTFS_NAMESPACE_WIN32_DOS    3

/* Size constants */
#define NTFS_STD_INFO_SIZE          48
#define NTFS_STD_INFO_SIZE_V3       96

/*
 * ===========================================================================
 * STRUKTUR ATRIBUT HEADER (ATTRIBUTE HEADER STRUCTURES)
 * ===========================================================================
 */

/* Base attribute header */
typedef struct ntfs_attr_header {
    tak_bertanda32_t ah_type;            /* Attribute type */
    tak_bertanda32_t ah_length;          /* Attribute length */
    tak_bertanda8_t  ah_non_resident;    /* Non-resident flag */
    tak_bertanda8_t  ah_name_len;        /* Name length */
    tak_bertanda16_t ah_name_offset;     /* Name offset */
    tak_bertanda16_t ah_flags;           /* Flags */
    tak_bertanda16_t ah_instance;        /* Instance ID */
} ntfs_attr_header_t;

/* Resident attribute header */
typedef struct ntfs_attr_resident {
    tak_bertanda32_t ar_type;
    tak_bertanda32_t ar_length;
    tak_bertanda8_t  ar_non_resident;
    tak_bertanda8_t  ar_name_len;
    tak_bertanda16_t ar_name_offset;
    tak_bertanda16_t ar_flags;
    tak_bertanda16_t ar_instance;
    tak_bertanda32_t ar_value_len;
    tak_bertanda16_t ar_value_offset;
    tak_bertanda8_t  ar_indexed;
    tak_bertanda8_t  ar_reserved;
} ntfs_attr_resident_t;

/* Non-resident attribute header */
typedef struct ntfs_attr_nonresident {
    tak_bertanda32_t anr_type;
    tak_bertanda32_t anr_length;
    tak_bertanda8_t  anr_non_resident;
    tak_bertanda8_t  anr_name_len;
    tak_bertanda16_t anr_name_offset;
    tak_bertanda16_t anr_flags;
    tak_bertanda16_t anr_instance;
    tak_bertanda64_t anr_start_vcn;
    tak_bertanda64_t anr_last_vcn;
    tak_bertanda16_t anr_run_offset;
    tak_bertanda16_t anr_comp_unit;
    tak_bertanda32_t anr_reserved;
    tak_bertanda64_t anr_alloc_size;
    tak_bertanda64_t anr_real_size;
    tak_bertanda64_t anr_init_size;
} ntfs_attr_nonresident_t;

/*
 * ===========================================================================
 * STRUKTUR STANDARD INFORMATION (STANDARD INFORMATION STRUCTURE)
 * ===========================================================================
 */

typedef struct ntfs_std_info {
    tak_bertanda64_t si_create_time;     /* Creation time */
    tak_bertanda64_t si_modify_time;     /* Modification time */
    tak_bertanda64_t si_mft_change_time; /* MFT change time */
    tak_bertanda64_t si_access_time;     /* Access time */
    tak_bertanda32_t si_flags;           /* File flags */
    tak_bertanda32_t si_max_versions;    /* Maximum versions */
    tak_bertanda32_t si_version;         /* Version number */
    tak_bertanda32_t si_class_id;        /* Class ID (XP+) */
    tak_bertanda32_t si_owner_id;        /* Owner ID (XP+) */
    tak_bertanda32_t si_security_id;     /* Security ID (XP+) */
    tak_bertanda64_t si_quota;           /* Quota charged (XP+) */
    tak_bertanda64_t si_usn;             /* USN journal (XP+) */
} ntfs_std_info_t;

/*
 * ===========================================================================
 * STRUKTUR FILE NAME (FILE NAME STRUCTURE)
 * ===========================================================================
 */

typedef struct ntfs_file_name {
    tak_bertanda64_t fn_parent_inode;    /* Parent directory inode */
    tak_bertanda64_t fn_parent_seq;      /* Parent sequence number */
    tak_bertanda64_t fn_create_time;     /* Creation time */
    tak_bertanda64_t fn_modify_time;     /* Modification time */
    tak_bertanda64_t fn_mft_change_time; /* MFT change time */
    tak_bertanda64_t fn_access_time;     /* Access time */
    tak_bertanda64_t fn_alloc_size;      /* Allocated size */
    tak_bertanda64_t fn_data_size;       /* Data size */
    tak_bertanda32_t fn_flags;           /* File flags */
    tak_bertanda32_t fn_reparse;         /* Reparse value */
    tak_bertanda8_t  fn_name_len;        /* Filename length */
    tak_bertanda8_t  fn_namespace;       /* Filename namespace */
    tak_bertanda16_t fn_name[1];         /* Filename (UTF-16LE) */
} ntfs_file_name_t;

/*
 * ===========================================================================
 * STRUKTUR VOLUME INFORMATION (VOLUME INFORMATION STRUCTURE)
 * ===========================================================================
 */

typedef struct ntfs_vol_info {
    tak_bertanda64_t vi_reserved1;
    tak_bertanda8_t  vi_major_ver;       /* Major version */
    tak_bertanda8_t  vi_minor_ver;       /* Minor version */
    tak_bertanda16_t vi_flags;           /* Volume flags */
    tak_bertanda32_t vi_reserved2;
} ntfs_vol_info_t;

/* Volume flags */
#define NTFS_VOL_FLAG_DIRTY         0x0001
#define NTFS_VOL_FLAG_RESIZE_LOG    0x0002
#define NTFS_VOL_FLAG_UPGRADE       0x0004
#define NTFS_VOL_FLAG_MOUNTED_NT4   0x0008
#define NTFS_VOL_FLAG_DELETE_USN    0x0010
#define NTFS_VOL_FLAG_REPAIR        0x0020
#define NTFS_VOL_FLAG_MODIFIED      0x80000000

/*
 * ===========================================================================
 * STRUKTUR OBJECT ID (OBJECT ID STRUCTURE)
 * ===========================================================================
 */

typedef struct ntfs_object_id {
    tak_bertanda8_t  oi_object_id[16];   /* Object ID */
    tak_bertanda8_t  oi_birth_vol_id[16];/* Birth volume ID */
    tak_bertanda8_t  oi_birth_obj_id[16];/* Birth object ID */
    tak_bertanda8_t  oi_domain_id[16];   /* Domain ID */
} ntfs_object_id_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INTERNAL (INTERNAL FUNCTION DECLARATIONS)
 * ===========================================================================
 */

static void *ntfs_attr_get_value_ptr(ntfs_attr_resident_t *attr);
static tak_bertanda64_t ntfs_time_to_unix(tak_bertanda64_t ntfs_time);

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI HEADER ATRIBUT (ATTRIBUTE HEADER FUNCTIONS)
 * ===========================================================================
 */

/*
 * ntfs_attr_get_type
 * Mendapatkan tipe atribut.
 */
tak_bertanda32_t ntfs_attr_get_type(void *attr)
{
    ntfs_attr_header_t *header;

    if (attr == NULL) {
        return 0;
    }

    header = (ntfs_attr_header_t *)attr;

    return header->ah_type;
}

/*
 * ntfs_attr_get_length
 * Mendapatkan panjang atribut.
 */
tak_bertanda32_t ntfs_attr_get_length(void *attr)
{
    ntfs_attr_header_t *header;

    if (attr == NULL) {
        return 0;
    }

    header = (ntfs_attr_header_t *)attr;

    return header->ah_length;
}

/*
 * ntfs_attr_is_resident
 * Cek apakah atribut resident.
 */
bool_t ntfs_attr_is_resident(void *attr)
{
    ntfs_attr_header_t *header;

    if (attr == NULL) {
        return BENAR; /* Default resident */
    }

    header = (ntfs_attr_header_t *)attr;

    return (header->ah_non_resident == 0) ? BENAR : SALAH;
}

/*
 * ntfs_attr_get_flags
 * Mendapatkan flags atribut.
 */
tak_bertanda16_t ntfs_attr_get_flags(void *attr)
{
    ntfs_attr_header_t *header;

    if (attr == NULL) {
        return 0;
    }

    header = (ntfs_attr_header_t *)attr;

    return header->ah_flags;
}

/*
 * ntfs_attr_is_compressed
 * Cek apakah atribut terkompresi.
 */
bool_t ntfs_attr_is_compressed(void *attr)
{
    tak_bertanda16_t flags;

    flags = ntfs_attr_get_flags(attr);

    return (flags & NTFS_ATTR_FLAG_COMPRESSED) ? BENAR : SALAH;
}

/*
 * ntfs_attr_is_encrypted
 * Cek apakah atribut terenkripsi.
 */
bool_t ntfs_attr_is_encrypted(void *attr)
{
    tak_bertanda16_t flags;

    flags = ntfs_attr_get_flags(attr);

    return (flags & NTFS_ATTR_FLAG_ENCRYPTED) ? BENAR : SALAH;
}

/*
 * ntfs_attr_is_sparse
 * Cek apakah atribut sparse.
 */
bool_t ntfs_attr_is_sparse(void *attr)
{
    tak_bertanda16_t flags;

    flags = ntfs_attr_get_flags(attr);

    return (flags & NTFS_ATTR_FLAG_SPARSE) ? BENAR : SALAH;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI RESIDENT ATRIBUT (RESIDENT ATTRIBUTE FUNCTIONS)
 * ===========================================================================
 */

/*
 * ntfs_attr_get_value_ptr
 * Mendapatkan pointer ke nilai atribut resident.
 */
static void *ntfs_attr_get_value_ptr(ntfs_attr_resident_t *attr)
{
    if (attr == NULL) {
        return NULL;
    }

    return (char *)attr + attr->ar_value_offset;
}

/*
 * ntfs_attr_get_value_length
 * Mendapatkan panjang nilai atribut resident.
 */
tak_bertanda32_t ntfs_attr_get_value_length(void *attr)
{
    ntfs_attr_resident_t *resident;

    if (attr == NULL) {
        return 0;
    }

    if (!ntfs_attr_is_resident(attr)) {
        return 0;
    }

    resident = (ntfs_attr_resident_t *)attr;

    return resident->ar_value_len;
}

/*
 * ntfs_attr_read_value
 * Membaca nilai dari atribut resident.
 */
status_t ntfs_attr_read_value(void *attr, void *buffer, tak_bertanda32_t size)
{
    ntfs_attr_resident_t *resident;
    void *value_ptr;
    tak_bertanda32_t copy_size;
    tak_bertanda32_t i;
    char *src;
    char *dst;

    if (attr == NULL || buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (!ntfs_attr_is_resident(attr)) {
        return STATUS_PARAM_INVALID;
    }

    resident = (ntfs_attr_resident_t *)attr;
    value_ptr = ntfs_attr_get_value_ptr(resident);

    copy_size = (size < resident->ar_value_len) ? size :
        resident->ar_value_len;

    /* Copy data */
    src = (char *)value_ptr;
    dst = (char *)buffer;

    for (i = 0; i < copy_size; i++) {
        dst[i] = src[i];
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI NON-RESIDENT ATRIBUT (NON-RESIDENT ATTRIBUTE FUNCTIONS)
 * ===========================================================================
 */

/*
 * ntfs_attr_get_alloc_size
 * Mendapatkan allocated size dari atribut non-resident.
 */
tak_bertanda64_t ntfs_attr_get_alloc_size(void *attr)
{
    ntfs_attr_nonresident_t *nonres;

    if (attr == NULL) {
        return 0;
    }

    if (ntfs_attr_is_resident(attr)) {
        return ntfs_attr_get_value_length(attr);
    }

    nonres = (ntfs_attr_nonresident_t *)attr;

    return nonres->anr_alloc_size;
}

/*
 * ntfs_attr_get_real_size
 * Mendapatkan real size dari atribut non-resident.
 */
tak_bertanda64_t ntfs_attr_get_real_size(void *attr)
{
    ntfs_attr_nonresident_t *nonres;

    if (attr == NULL) {
        return 0;
    }

    if (ntfs_attr_is_resident(attr)) {
        return ntfs_attr_get_value_length(attr);
    }

    nonres = (ntfs_attr_nonresident_t *)attr;

    return nonres->anr_real_size;
}

/*
 * ntfs_attr_get_init_size
 * Mendapatkan initialized size dari atribut non-resident.
 */
tak_bertanda64_t ntfs_attr_get_init_size(void *attr)
{
    ntfs_attr_nonresident_t *nonres;

    if (attr == NULL) {
        return 0;
    }

    if (ntfs_attr_is_resident(attr)) {
        return ntfs_attr_get_value_length(attr);
    }

    nonres = (ntfs_attr_nonresident_t *)attr;

    return nonres->anr_init_size;
}

/*
 * ntfs_attr_get_start_vcn
 * Mendapatkan start VCN dari atribut non-resident.
 */
tak_bertanda64_t ntfs_attr_get_start_vcn(void *attr)
{
    ntfs_attr_nonresident_t *nonres;

    if (attr == NULL) {
        return 0;
    }

    if (ntfs_attr_is_resident(attr)) {
        return 0;
    }

    nonres = (ntfs_attr_nonresident_t *)attr;

    return nonres->anr_start_vcn;
}

/*
 * ntfs_attr_get_last_vcn
 * Mendapatkan last VCN dari atribut non-resident.
 */
tak_bertanda64_t ntfs_attr_get_last_vcn(void *attr)
{
    ntfs_attr_nonresident_t *nonres;

    if (attr == NULL) {
        return 0;
    }

    if (ntfs_attr_is_resident(attr)) {
        return 0;
    }

    nonres = (ntfs_attr_nonresident_t *)attr;

    return nonres->anr_last_vcn;
}

/*
 * ntfs_attr_get_run_offset
 * Mendapatkan offset ke data runs.
 */
tak_bertanda16_t ntfs_attr_get_run_offset(void *attr)
{
    ntfs_attr_nonresident_t *nonres;

    if (attr == NULL) {
        return 0;
    }

    if (ntfs_attr_is_resident(attr)) {
        return 0;
    }

    nonres = (ntfs_attr_nonresident_t *)attr;

    return nonres->anr_run_offset;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI STANDARD INFORMATION (STANDARD INFORMATION FUNCTIONS)
 * ===========================================================================
 */

/*
 * ntfs_time_to_unix
 * Konversi waktu NTFS (100-nanosecond since 1601) ke Unix time.
 */
static tak_bertanda64_t ntfs_time_to_unix(tak_bertanda64_t ntfs_time)
{
    /* NTFS epoch: January 1, 1601 */
    /* Unix epoch: January 1, 1970 */
    /* Difference: 11644473600 seconds */
    /* 1 NTFS time unit = 100 nanoseconds */
    tak_bertanda64_t unix_time;

    if (ntfs_time == 0) {
        return 0;
    }

    /* Convert 100ns units to seconds and subtract epoch difference */
    unix_time = ntfs_time / 10000000ULL;
    unix_time -= 11644473600ULL;

    return unix_time;
}

/*
 * ntfs_std_info_parse
 * Parse Standard Information attribute.
 */
status_t ntfs_std_info_parse(void *attr, ntfs_std_info_t *info)
{
    ntfs_attr_resident_t *resident;
    char *value_ptr;
    tak_bertanda32_t value_len;

    if (attr == NULL || info == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (!ntfs_attr_is_resident(attr)) {
        return STATUS_PARAM_INVALID;
    }

    if (ntfs_attr_get_type(attr) != NTFS_ATTR_STANDARD_INFO) {
        return STATUS_PARAM_INVALID;
    }

    resident = (ntfs_attr_resident_t *)attr;
    value_ptr = (char *)attr + resident->ar_value_offset;
    value_len = resident->ar_value_len;

    /* Parse times */
    info->si_create_time = *((tak_bertanda64_t *)(value_ptr + 0));
    info->si_modify_time = *((tak_bertanda64_t *)(value_ptr + 8));
    info->si_mft_change_time = *((tak_bertanda64_t *)(value_ptr + 16));
    info->si_access_time = *((tak_bertanda64_t *)(value_ptr + 24));
    info->si_flags = *((tak_bertanda32_t *)(value_ptr + 32));

    /* Extended fields (v3.0+) */
    if (value_len >= NTFS_STD_INFO_SIZE_V3) {
        info->si_max_versions = *((tak_bertanda32_t *)(value_ptr + 36));
        info->si_version = *((tak_bertanda32_t *)(value_ptr + 40));
        info->si_class_id = *((tak_bertanda32_t *)(value_ptr + 44));
        info->si_owner_id = *((tak_bertanda32_t *)(value_ptr + 48));
        info->si_security_id = *((tak_bertanda32_t *)(value_ptr + 52));
        info->si_quota = *((tak_bertanda64_t *)(value_ptr + 56));
        info->si_usn = *((tak_bertanda64_t *)(value_ptr + 64));
    } else {
        info->si_max_versions = 0;
        info->si_version = 0;
        info->si_class_id = 0;
        info->si_owner_id = 0;
        info->si_security_id = 0;
        info->si_quota = 0;
        info->si_usn = 0;
    }

    return STATUS_BERHASIL;
}

/*
 * ntfs_std_info_get_create_time
 * Mendapatkan waktu pembuatan file.
 */
waktu_t ntfs_std_info_get_create_time(ntfs_std_info_t *info)
{
    if (info == NULL) {
        return 0;
    }

    return (waktu_t)ntfs_time_to_unix(info->si_create_time);
}

/*
 * ntfs_std_info_get_modify_time
 * Mendapatkan waktu modifikasi file.
 */
waktu_t ntfs_std_info_get_modify_time(ntfs_std_info_t *info)
{
    if (info == NULL) {
        return 0;
    }

    return (waktu_t)ntfs_time_to_unix(info->si_modify_time);
}

/*
 * ntfs_std_info_get_access_time
 * Mendapatkan waktu akses file.
 */
waktu_t ntfs_std_info_get_access_time(ntfs_std_info_t *info)
{
    if (info == NULL) {
        return 0;
    }

    return (waktu_t)ntfs_time_to_unix(info->si_access_time);
}

/*
 * ntfs_std_info_get_flags
 * Mendapatkan file flags.
 */
tak_bertanda32_t ntfs_std_info_get_flags(ntfs_std_info_t *info)
{
    if (info == NULL) {
        return 0;
    }

    return info->si_flags;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI FILE NAME (FILE NAME FUNCTIONS)
 * ===========================================================================
 */

/*
 * ntfs_file_name_parse
 * Parse File Name attribute.
 */
status_t ntfs_file_name_parse(void *attr, ntfs_file_name_t *fname,
    char *buffer, tak_bertanda32_t buffer_size)
{
    ntfs_attr_resident_t *resident;
    char *value_ptr;
    tak_bertanda8_t name_len;
    tak_bertanda16_t *name_ptr;
    tak_bertanda32_t i;

    if (attr == NULL || fname == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (!ntfs_attr_is_resident(attr)) {
        return STATUS_PARAM_INVALID;
    }

    if (ntfs_attr_get_type(attr) != NTFS_ATTR_FILE_NAME) {
        return STATUS_PARAM_INVALID;
    }

    resident = (ntfs_attr_resident_t *)attr;
    value_ptr = (char *)attr + resident->ar_value_offset;

    /* Parse fixed fields */
    fname->fn_parent_inode = *((tak_bertanda64_t *)(value_ptr + 0));
    fname->fn_parent_seq = *((tak_bertanda64_t *)(value_ptr + 8));
    fname->fn_create_time = *((tak_bertanda64_t *)(value_ptr + 16));
    fname->fn_modify_time = *((tak_bertanda64_t *)(value_ptr + 24));
    fname->fn_mft_change_time = *((tak_bertanda64_t *)(value_ptr + 32));
    fname->fn_access_time = *((tak_bertanda64_t *)(value_ptr + 40));
    fname->fn_alloc_size = *((tak_bertanda64_t *)(value_ptr + 48));
    fname->fn_data_size = *((tak_bertanda64_t *)(value_ptr + 56));
    fname->fn_flags = *((tak_bertanda32_t *)(value_ptr + 64));
    fname->fn_reparse = *((tak_bertanda32_t *)(value_ptr + 68));
    fname->fn_name_len = *((tak_bertanda8_t *)(value_ptr + 72));
    fname->fn_namespace = *((tak_bertanda8_t *)(value_ptr + 73));

    /* Convert filename from UTF-16LE to ASCII/UTF-8 */
    if (buffer != NULL && buffer_size > 0) {
        name_ptr = (tak_bertanda16_t *)(value_ptr + 74);
        name_len = fname->fn_name_len;

        /* Limit to buffer size */
        if (name_len >= buffer_size) {
            name_len = (tak_bertanda8_t)(buffer_size - 1);
        }

        /* Convert UTF-16LE to ASCII (simple conversion) */
        for (i = 0; i < name_len; i++) {
            tak_bertanda16_t wchar = name_ptr[i];
            buffer[i] = (char)(wchar & 0xFF);
        }
        buffer[name_len] = '\0';
    }

    return STATUS_BERHASIL;
}

/*
 * ntfs_file_name_get_parent
 * Mendapatkan inode parent directory.
 */
tak_bertanda64_t ntfs_file_name_get_parent(ntfs_file_name_t *fname)
{
    if (fname == NULL) {
        return 0;
    }

    return fname->fn_parent_inode;
}

/*
 * ntfs_file_name_get_size
 * Mendapatkan ukuran file dari File Name attribute.
 */
tak_bertanda64_t ntfs_file_name_get_size(ntfs_file_name_t *fname)
{
    if (fname == NULL) {
        return 0;
    }

    return fname->fn_data_size;
}

/*
 * ntfs_file_name_get_flags
 * Mendapatkan file flags dari File Name attribute.
 */
tak_bertanda32_t ntfs_file_name_get_flags(ntfs_file_name_t *fname)
{
    if (fname == NULL) {
        return 0;
    }

    return fname->fn_flags;
}

/*
 * ntfs_file_name_is_directory
 * Cek apakah File Name menunjukkan direktori.
 */
bool_t ntfs_file_name_is_directory(ntfs_file_name_t *fname)
{
    if (fname == NULL) {
        return SALAH;
    }

    return (fname->fn_flags & NTFS_FILE_FLAG_DIRECTORY) ? BENAR : SALAH;
}

/*
 * ntfs_file_name_is_hidden
 * Cek apakah file tersembunyi.
 */
bool_t ntfs_file_name_is_hidden(ntfs_file_name_t *fname)
{
    if (fname == NULL) {
        return SALAH;
    }

    return (fname->fn_flags & NTFS_FILE_FLAG_HIDDEN) ? BENAR : SALAH;
}

/*
 * ntfs_file_name_is_system
 * Cek apakah file system.
 */
bool_t ntfs_file_name_is_system(ntfs_file_name_t *fname)
{
    if (fname == NULL) {
        return SALAH;
    }

    return (fname->fn_flags & NTFS_FILE_FLAG_SYSTEM) ? BENAR : SALAH;
}

/*
 * ntfs_file_name_get_name_len
 * Mendapatkan panjang nama file.
 */
tak_bertanda8_t ntfs_file_name_get_name_len(ntfs_file_name_t *fname)
{
    if (fname == NULL) {
        return 0;
    }

    return fname->fn_name_len;
}

/*
 * ntfs_file_name_get_namespace
 * Mendapatkan namespace nama file.
 */
tak_bertanda8_t ntfs_file_name_get_namespace(ntfs_file_name_t *fname)
{
    if (fname == NULL) {
        return NTFS_NAMESPACE_WIN32;
    }

    return fname->fn_namespace;
}
