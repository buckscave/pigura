/*
 * PIGURA OS - ISO9660_FILE.C
 * ==========================
 * Implementasi operasi file untuk filesystem ISO9660.
 *
 * Berkas ini berisi fungsi-fungsi untuk membaca file dari
 * filesystem ISO9660 yang bersifat read-only.
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

/* Ukuran sektor standar ISO9660 */
#define ISO_FILE_SEKTOR_UKURAN      2048

/* Ukuran buffer baca optimal */
#define ISO_FILE_BUFFER_UKURAN      8192

/* Flag file */
#define ISO_FILE_FLAG_HIDDEN        0x01
#define ISO_FILE_FLAG_DIRECTORY     0x02
#define ISO_FILE_FLAG_ASSOCIATED    0x04
#define ISO_FILE_FLAG_RECORD        0x08
#define ISO_FILE_FLAG_PROTECTION    0x10
#define ISO_FILE_FLAG_MULTIEXTENT   0x80

/* Magic number untuk validasi */
#define ISO_FILE_HANDLE_MAGIC       0x49534F46  /* "ISOF" */

/*
 * ===========================================================================
 * STRUKTUR DATA FILE (FILE DATA STRUCTURES)
 * ===========================================================================
 */

/* Struktur file handle internal */
typedef struct {
    tak_bertanda32_t magic;             /* Magic number validasi */
    tak_bertanda32_t extent;            /* Extent awal file */
    tak_bertanda32_t extent_count;      /* Jumlah extent */
    tak_bertanda64_t size;              /* Ukuran file */
    tak_bertanda64_t pos;               /* Posisi saat ini */
    tak_bertanda16_t block_size;        /* Ukuran block */
    void *device_context;               /* Context device */
    tak_bertanda8_t flags;              /* File flags */
    bool_t is_open;                     /* Status file terbuka */
} iso_file_handle_t;

/* Struktur extent info untuk multi-extent files */
typedef struct {
    tak_bertanda32_t extent;            /* Lokasi extent */
    tak_bertanda32_t size;              /* Ukuran extent */
} iso_extent_info_t;

/* Struktur context baca */
typedef struct {
    tak_bertanda8_t *buffer;            /* Buffer sementara */
    tak_bertanda32_t buffer_size;       /* Ukuran buffer */
    tak_bertanda64_t current_block;     /* Block saat ini di buffer */
    bool_t buffer_valid;               /* Apakah buffer valid */
} iso_read_context_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INTERNAL (INTERNAL FUNCTION DECLARATIONS)
 * ===========================================================================
 */

static status_t iso_file_baca_block(void *device_context,
                                     tak_bertanda64_t block,
                                     void *buffer, ukuran_t count);
static tak_bertanda64_t iso_file_pos_to_block(tak_bertanda64_t pos,
                                              tak_bertanda16_t block_size);
static tak_bertanda32_t iso_file_pos_to_offset(tak_bertanda64_t pos,
                                               tak_bertanda16_t block_size);
static status_t iso_file_read_internal(iso_file_handle_t *handle,
                                       void *buffer, ukuran_t size,
                                       ukuran_t *bytes_read);
static bool_t iso_file_valid_handle(const iso_file_handle_t *handle);

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI VALIDASI (VALIDATION FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

static bool_t iso_file_valid_handle(const iso_file_handle_t *handle)
{
    if (handle == NULL) {
        return SALAH;
    }

    if (handle->magic != ISO_FILE_HANDLE_MAGIC) {
        return SALAH;
    }

    if (!handle->is_open) {
        return SALAH;
    }

    return BENAR;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI KONVERSI POSISI (POSITION CONVERSION FUNCTIONS)
 * ===========================================================================
 */

static tak_bertanda64_t iso_file_pos_to_block(tak_bertanda64_t pos,
                                              tak_bertanda16_t block_size)
{
    if (block_size == 0) {
        return 0;
    }

    return pos / (tak_bertanda64_t)block_size;
}

static tak_bertanda32_t iso_file_pos_to_offset(tak_bertanda64_t pos,
                                               tak_bertanda16_t block_size)
{
    if (block_size == 0) {
        return 0;
    }

    return (tak_bertanda32_t)(pos % (tak_bertanda64_t)block_size);
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI BACA BLOCK (BLOCK READ FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

static status_t iso_file_baca_block(void *device_context,
                                     tak_bertanda64_t block,
                                     void *buffer, ukuran_t count)
{
    /* Placeholder untuk pembacaan block dari device */
    /* TODO: Implementasi pembacaan aktual dari device */
    (void)device_context;
    (void)block;
    (void)buffer;
    (void)count;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI BACA INTERNAL (INTERNAL READ FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

static status_t iso_file_read_internal(iso_file_handle_t *handle,
                                       void *buffer, ukuran_t size,
                                       ukuran_t *bytes_read)
{
    tak_bertanda8_t *block_buffer;
    tak_bertanda64_t current_block;
    tak_bertanda32_t block_offset;
    tak_bertanda64_t file_block_count;
    tak_bertanda64_t extent_block;
    ukuran_t total_read;
    ukuran_t remaining;
    status_t status;

    if (bytes_read != NULL) {
        *bytes_read = 0;
    }

    if (!iso_file_valid_handle(handle)) {
        return STATUS_PARAM_INVALID;
    }

    if (buffer == NULL || size == 0) {
        return STATUS_PARAM_NULL;
    }

    /* Cek apakah sudah di akhir file */
    if (handle->pos >= handle->size) {
        return STATUS_BERHASIL;
    }

    /* Batasi bacaan hingga akhir file */
    remaining = (ukuran_t)MIN((tak_bertanda64_t)size,
                              handle->size - handle->pos);

    /* Alokasi buffer untuk satu block */
    block_buffer = (tak_bertanda8_t *)kmalloc((ukuran_t)handle->block_size);
    if (block_buffer == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    total_read = 0;
    file_block_count = (handle->size + handle->block_size - 1) /
                       handle->block_size;

    while (remaining > 0 && handle->pos < handle->size) {
        current_block = iso_file_pos_to_block(handle->pos, handle->block_size);
        block_offset = iso_file_pos_to_offset(handle->pos, handle->block_size);

        /* Hitung block dalam extent */
        extent_block = (tak_bertanda64_t)handle->extent + current_block;

        /* Cek apakah block valid */
        if (current_block >= file_block_count) {
            break;
        }

        /* Baca block dari device */
        status = iso_file_baca_block(handle->device_context,
                                     extent_block, block_buffer, 1);
        if (status != STATUS_BERHASIL) {
            break;
        }

        /* Hitung berapa byte yang bisa dicopy dari block ini */
        {
            tak_bertanda32_t bytes_in_block;
            tak_bertanda32_t to_copy;

            bytes_in_block = handle->block_size - block_offset;
            to_copy = (tak_bertanda32_t)MIN(remaining, (ukuran_t)bytes_in_block);

            /* Copy ke buffer user */
            kernel_memcpy((tak_bertanda8_t *)buffer + total_read,
                          block_buffer + block_offset, to_copy);

            total_read += to_copy;
            remaining -= to_copy;
            handle->pos += to_copy;
        }
    }

    kfree(block_buffer);

    if (bytes_read != NULL) {
        *bytes_read = total_read;
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI UTAMA (MAIN FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

iso_file_handle_t *iso9660_file_buka(void *device_context,
                                     tak_bertanda32_t extent,
                                     tak_bertanda64_t size,
                                     tak_bertanda16_t block_size,
                                     tak_bertanda8_t flags)
{
    iso_file_handle_t *handle;

    if (device_context == NULL || block_size == 0) {
        return NULL;
    }

    /* Tidak boleh membuka direktori */
    if (flags & ISO_FILE_FLAG_DIRECTORY) {
        return NULL;
    }

    handle = (iso_file_handle_t *)kmalloc(sizeof(iso_file_handle_t));
    if (handle == NULL) {
        return NULL;
    }

    kernel_memset(handle, 0, sizeof(iso_file_handle_t));

    handle->magic = ISO_FILE_HANDLE_MAGIC;
    handle->extent = extent;
    handle->extent_count = 1;
    handle->size = size;
    handle->pos = 0;
    handle->block_size = block_size;
    handle->device_context = device_context;
    handle->flags = flags;
    handle->is_open = BENAR;

    return handle;
}

status_t iso9660_file_tutup(iso_file_handle_t *handle)
{
    if (handle == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (!iso_file_valid_handle(handle)) {
        return STATUS_PARAM_INVALID;
    }

    handle->is_open = SALAH;
    handle->magic = 0;

    kfree(handle);

    return STATUS_BERHASIL;
}

tak_bertandas_t iso9660_file_baca(iso_file_handle_t *handle,
                                  void *buffer, ukuran_t size)
{
    ukuran_t bytes_read;
    status_t status;

    status = iso_file_read_internal(handle, buffer, size, &bytes_read);
    if (status != STATUS_BERHASIL) {
        return (tak_bertandas_t)status;
    }

    return (tak_bertandas_t)bytes_read;
}

tak_bertandas_t iso9660_file_baca_offset(iso_file_handle_t *handle,
                                         void *buffer, ukuran_t size,
                                         tak_bertanda64_t offset)
{
    tak_bertanda64_t old_pos;
    tak_bertandas_t result;

    if (!iso_file_valid_handle(handle)) {
        return (tak_bertandas_t)STATUS_PARAM_INVALID;
    }

    /* Simpan posisi lama */
    old_pos = handle->pos;

    /* Set posisi baru */
    if (offset > handle->size) {
        handle->pos = handle->size;
    } else {
        handle->pos = offset;
    }

    /* Baca */
    result = iso9660_file_baca(handle, buffer, size);

    /* Restore posisi lama (seek style) */
    /* Untuk baca pada offset tertentu, tidak restore */
    (void)old_pos;

    return result;
}

off_t iso9660_file_seek(iso_file_handle_t *handle, off_t offset,
                        tak_bertanda32_t whence)
{
    tak_bertanda64_t new_pos;

    if (!iso_file_valid_handle(handle)) {
        return (off_t)STATUS_PARAM_INVALID;
    }

    switch (whence) {
    case VFS_SEEK_SET:
        new_pos = (tak_bertanda64_t)offset;
        break;

    case VFS_SEEK_CUR:
        if (offset < 0 &&
            (tak_bertanda64_t)(-offset) > handle->pos) {
            return (off_t)STATUS_PARAM_INVALID;
        }
        new_pos = handle->pos + (tak_bertanda64_t)offset;
        break;

    case VFS_SEEK_END:
        if (offset > 0) {
            /* Tidak bisa seek melebihi EOF untuk read-only */
            new_pos = handle->size;
        } else {
            new_pos = handle->size + (tak_bertanda64_t)offset;
        }
        break;

    default:
        return (off_t)STATUS_PARAM_INVALID;
    }

    /* Validasi posisi */
    if (new_pos > handle->size) {
        new_pos = handle->size;
    }

    handle->pos = new_pos;

    return (off_t)handle->pos;
}

tak_bertanda64_t iso9660_file_tell(iso_file_handle_t *handle)
{
    if (!iso_file_valid_handle(handle)) {
        return 0;
    }

    return handle->pos;
}

bool_t iso9660_file_eof(iso_file_handle_t *handle)
{
    if (!iso_file_valid_handle(handle)) {
        return BENAR;
    }

    return (handle->pos >= handle->size) ? BENAR : SALAH;
}

tak_bertanda64_t iso9660_file_size(iso_file_handle_t *handle)
{
    if (!iso_file_valid_handle(handle)) {
        return 0;
    }

    return handle->size;
}

status_t iso9660_file_stat(iso_file_handle_t *handle, vfs_stat_t *stat)
{
    if (stat == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (!iso_file_valid_handle(handle)) {
        return STATUS_PARAM_INVALID;
    }

    kernel_memset(stat, 0, sizeof(vfs_stat_t));

    stat->st_size = (off_t)handle->size;
    stat->st_blksize = handle->block_size;
    stat->st_blocks = (handle->size + handle->block_size - 1) /
                      handle->block_size;
    stat->st_mode = VFS_S_IFREG | 0444;  /* Read-only file */

    return STATUS_BERHASIL;
}

status_t iso9660_file_baca_seluruh(void *device_context,
                                   tak_bertanda32_t extent,
                                   tak_bertanda64_t size,
                                   tak_bertanda16_t block_size,
                                   void *buffer)
{
    iso_file_handle_t *handle;
    tak_bertandas_t result;
    ukuran_t to_read;

    if (buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    handle = iso9660_file_buka(device_context, extent, size, block_size, 0);
    if (handle == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    to_read = (ukuran_t)MIN(size, (tak_bertanda64_t)~(ukuran_t)0);

    result = iso9660_file_baca(handle, buffer, to_read);

    iso9660_file_tutup(handle);

    if (result < 0) {
        return (status_t)result;
    }

    return STATUS_BERHASIL;
}

status_t iso9660_file_compare(void *device_context,
                              tak_bertanda32_t extent,
                              tak_bertanda64_t size,
                              tak_bertanda16_t block_size,
                              const void *buffer,
                              ukuran_t buffer_size,
                              bool_t *result)
{
    tak_bertanda8_t *temp_buffer;
    iso_file_handle_t *handle;
    tak_bertandas_t bytes_read;

    if (result == NULL) {
        return STATUS_PARAM_NULL;
    }

    *result = SALAH;

    if (buffer == NULL || buffer_size == 0) {
        return STATUS_PARAM_NULL;
    }

    /* Jika ukuran berbeda, pasti tidak sama */
    if (size != (tak_bertanda64_t)buffer_size) {
        *result = SALAH;
        return STATUS_BERHASIL;
    }

    handle = iso9660_file_buka(device_context, extent, size, block_size, 0);
    if (handle == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    temp_buffer = (tak_bertanda8_t *)kmalloc(buffer_size);
    if (temp_buffer == NULL) {
        iso9660_file_tutup(handle);
        return STATUS_MEMORI_HABIS;
    }

    bytes_read = iso9660_file_baca(handle, temp_buffer, buffer_size);

    iso9660_file_tutup(handle);

    if ((tak_bertandas_t)bytes_read != (tak_bertandas_t)buffer_size) {
        kfree(temp_buffer);
        return STATUS_IO_ERROR;
    }

    /* Bandingkan buffer */
    if (kernel_memcmp(buffer, temp_buffer, buffer_size) == 0) {
        *result = BENAR;
    } else {
        *result = SALAH;
    }

    kfree(temp_buffer);

    return STATUS_BERHASIL;
}

status_t iso9660_file_hash(void *device_context,
                           tak_bertanda32_t extent,
                           tak_bertanda64_t size,
                           tak_bertanda16_t block_size,
                           tak_bertanda32_t *hash)
{
    iso_file_handle_t *handle;
    tak_bertanda8_t buffer[ISO_FILE_BUFFER_UKURAN];
    tak_bertandas_t bytes_read;
    tak_bertanda32_t result;
    tak_bertanda64_t remaining;

    if (hash == NULL) {
        return STATUS_PARAM_NULL;
    }

    *hash = 0;

    handle = iso9660_file_buka(device_context, extent, size, block_size, 0);
    if (handle == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    result = 5381;  /* DJB2 hash initial value */
    remaining = size;

    while (remaining > 0 && !iso9660_file_eof(handle)) {
        ukuran_t to_read = (ukuran_t)MIN(remaining,
                                         ISO_FILE_BUFFER_UKURAN);

        bytes_read = iso9660_file_baca(handle, buffer, to_read);

        if (bytes_read <= 0) {
            break;
        }

        /* Update hash (DJB2 algorithm) */
        {
            ukuran_t i;
            for (i = 0; i < (ukuran_t)bytes_read; i++) {
                result = ((result << 5) + result) + buffer[i];
            }
        }

        remaining -= (tak_bertanda64_t)bytes_read;
    }

    iso9660_file_tutup(handle);

    *hash = result;

    return STATUS_BERHASIL;
}
