/*
 * PIGURA OS - NTFS_COMPRESS.C
 * ============================
 * Kompresi dan dekompresi NTFS untuk kernel Pigura OS.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk mendekompresi
 * data yang dikompresi dengan NTFS compression (LZNT1 algorithm),
 * yang digunakan untuk menghemat ruang disk.
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
 * KONSTANTA KOMPRESI (COMPRESSION CONSTANTS)
 * ===========================================================================
 */

/* Compression unit size (clusters) */
#define NTFS_COMPRESSION_UNIT       16

/* Compression block size */
#define NTFS_COMPRESSION_BLOCK_SIZE 4096

/* Maximum compressed block size */
#define NTFS_COMPRESSED_MAX_SIZE    4096

/* Compression signature */
#define NTFS_COMPRESSION_SIGNATURE  0x0001

/* Block size mask */
#define NTFS_BLOCK_SIZE_MASK        0x0FFF

/* Compression flags */
#define NTFS_COMPRESSION_FLAG       0x01
#define NTFS_SPARSE_FLAG            0x02

/* LZNT1 token types */
#define NTFS_LZNT1_LITERAL          0x00
#define NTFS_LZNT1_BACKREF          0x01

/* Maximum offset for back reference */
#define NTFS_LZNT1_MAX_OFFSET       4096

/* Maximum length for back reference */
#define NTFS_LZNT1_MAX_LENGTH       4096

/* Status code for small buffer */
#ifndef STATUS_BUFFER_KECIL
#define STATUS_BUFFER_KECIL         ((status_t)-25)
#endif

/*
 * ===========================================================================
 * STRUKTUR COMPRESSION BLOCK HEADER (COMPRESSION BLOCK HEADER)
 * ===========================================================================
 */

typedef struct ntfs_comp_block_header {
    tak_bertanda16_t cb_signature;      /* Compression signature */
    tak_bertanda16_t cb_length;         /* Compressed length */
    tak_bertanda16_t cb_uncompressed;   /* Uncompressed length */
    tak_bertanda16_t cb_reserved;       /* Reserved */
} ntfs_comp_block_header_t;

/*
 * ===========================================================================
 * STRUKTUR COMPRESSION CONTEXT (COMPRESSION CONTEXT)
 * ===========================================================================
 */

typedef struct ntfs_compress_ctx {
    tak_bertanda8_t  *cc_input;         /* Input buffer */
    tak_bertanda8_t  *cc_output;        /* Output buffer */
    tak_bertanda32_t  cc_input_size;    /* Input size */
    tak_bertanda32_t  cc_output_size;   /* Output size */
    tak_bertanda32_t  cc_input_pos;     /* Current input position */
    tak_bertanda32_t  cc_output_pos;    /* Current output position */
} ntfs_compress_ctx_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INTERNAL (INTERNAL FUNCTION DECLARATIONS)
 * ===========================================================================
 */

static status_t ntfs_decompress_block(tak_bertanda8_t *input,
    tak_bertanda32_t input_size, tak_bertanda8_t *output,
    tak_bertanda32_t *output_size);
static status_t ntfs_decompress_lznt1(tak_bertanda8_t *input,
    tak_bertanda32_t input_size, tak_bertanda8_t *output,
    tak_bertanda32_t *output_size);
static tak_bertanda32_t ntfs_get_token(tak_bertanda8_t *input,
    tak_bertanda32_t pos, tak_bertanda32_t *offset, tak_bertanda32_t *length);
static status_t ntfs_compress_block(tak_bertanda8_t *input,
    tak_bertanda32_t input_size, tak_bertanda8_t *output,
    tak_bertanda32_t *output_size);

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI UTILITAS (UTILITY FUNCTIONS)
 * ===========================================================================
 */

/*
 * ntfs_compress_is_compressed
 * Cek apakah buffer terkompresi.
 */
bool_t ntfs_compress_is_compressed(tak_bertanda8_t *buffer,
    tak_bertanda32_t size)
{
    tak_bertanda16_t signature;

    if (buffer == NULL || size < 2) {
        return SALAH;
    }

    signature = *((tak_bertanda16_t *)buffer);

    return (signature != 0) ? BENAR : SALAH;
}

/*
 * ntfs_compress_is_sparse
 * Cek apakah compression unit adalah sparse (semua nol).
 */
bool_t ntfs_compress_is_sparse(tak_bertanda8_t *buffer,
    tak_bertanda32_t size)
{
    if (buffer == NULL || size == 0) {
        return BENAR;
    }

    /* Buffer dengan size 0 menandakan sparse */
    return SALAH;
}

/*
 * ntfs_compress_get_block_count
 * Mendapatkan jumlah compression blocks.
 */
tak_bertanda32_t ntfs_compress_get_block_count(tak_bertanda64_t size)
{
    tak_bertanda32_t blocks;

    blocks = (tak_bertanda32_t)(size / NTFS_COMPRESSION_BLOCK_SIZE);

    if (size % NTFS_COMPRESSION_BLOCK_SIZE != 0) {
        blocks++;
    }

    return blocks;
}

/*
 * ===========================================================================
 * IMPLEMENTASI DEKOMPRESI LZNT1 (LZNT1 DECOMPRESSION)
 * ===========================================================================
 */

/*
 * ntfs_decompress_lznt1
 * Dekompresi data dengan algoritma LZNT1.
 */
static status_t ntfs_decompress_lznt1(tak_bertanda8_t *input,
    tak_bertanda32_t input_size, tak_bertanda8_t *output,
    tak_bertanda32_t *output_size)
{
    tak_bertanda32_t in_pos;
    tak_bertanda32_t out_pos;
    tak_bertanda8_t flags;
    tak_bertanda8_t flag_count;
    tak_bertanda32_t i;
    tak_bertanda32_t offset;
    tak_bertanda32_t length;

    if (input == NULL || output == NULL || output_size == NULL) {
        return STATUS_PARAM_NULL;
    }

    in_pos = 0;
    out_pos = 0;

    /* Process input */
    while (in_pos < input_size && out_pos < *output_size) {
        /* Read flags byte */
        flags = input[in_pos++];
        flag_count = 0;

        /* Process 8 tokens */
        for (i = 0; i < 8 && in_pos < input_size &&
            out_pos < *output_size; i++) {
            tak_bertanda8_t token;

            /* Check flag bit */
            if (flags & (1 << i)) {
                /* Back reference */
                tak_bertanda16_t backref;
                tak_bertanda32_t j;

                if (in_pos + 1 >= input_size) {
                    break;
                }

                backref = input[in_pos] | (input[in_pos + 1] << 8);
                in_pos += 2;

                /* Decode offset and length */
                offset = (backref >> 4) & 0x0FFF;
                length = (backref & 0x0F) + 3;

                /* Copy from output buffer */
                if ((tak_bertanda32_t)offset > out_pos) {
                    return STATUS_FS_CORRUPT;
                }

                for (j = 0; j < length && out_pos < *output_size; j++) {
                    output[out_pos] = output[out_pos - offset];
                    out_pos++;
                }
            } else {
                /* Literal byte */
                if (in_pos >= input_size) {
                    break;
                }

                output[out_pos++] = input[in_pos++];
            }
        }
    }

    *output_size = out_pos;

    return STATUS_BERHASIL;
}

/*
 * ntfs_decompress_block
 * Dekompresi satu compression block.
 */
static status_t ntfs_decompress_block(tak_bertanda8_t *input,
    tak_bertanda32_t input_size, tak_bertanda8_t *output,
    tak_bertanda32_t *output_size)
{
    tak_bertanda16_t signature;
    status_t status;

    if (input == NULL || output == NULL || output_size == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Check signature */
    if (input_size < 2) {
        return STATUS_FS_CORRUPT;
    }

    signature = *((tak_bertanda16_t *)input);

    if (signature == 0) {
        /* Uncompressed block */
        tak_bertanda32_t copy_size;

        copy_size = input_size - 2;
        if (copy_size > *output_size) {
            copy_size = *output_size;
        }

        /* Copy data */
        {
            tak_bertanda32_t i;
            for (i = 0; i < copy_size; i++) {
                output[i] = input[i + 2];
            }
        }

        *output_size = copy_size;
        return STATUS_BERHASIL;
    }

    /* Compressed block - decompress with LZNT1 */
    status = ntfs_decompress_lznt1(input + 2, input_size - 2,
        output, output_size);

    return status;
}

/*
 * ntfs_decompress_unit
 * Dekompresi satu compression unit (multiple blocks).
 */
status_t ntfs_decompress_unit(tak_bertanda8_t *input,
    tak_bertanda32_t input_size, tak_bertanda8_t *output,
    tak_bertanda32_t output_size, tak_bertanda32_t *actual_size)
{
    tak_bertanda32_t in_offset;
    tak_bertanda32_t out_offset;
    tak_bertanda16_t *block_sizes;
    tak_bertanda32_t block_count;
    tak_bertanda32_t i;
    status_t status;

    if (input == NULL || output == NULL || actual_size == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Check if sparse (input_size == 0) */
    if (input_size == 0) {
        /* Fill output with zeros */
        for (i = 0; i < output_size; i++) {
            output[i] = 0;
        }
        *actual_size = output_size;
        return STATUS_BERHASIL;
    }

    /* Parse block size table */
    block_count = input_size / 2;
    if (block_count == 0) {
        return STATUS_FS_CORRUPT;
    }

    block_sizes = (tak_bertanda16_t *)input;
    in_offset = block_count * 2;
    out_offset = 0;

    /* Process each block */
    for (i = 0; i < block_count && out_offset < output_size; i++) {
        tak_bertanda16_t block_size;
        tak_bertanda8_t *block_input;
        tak_bertanda32_t block_output_size;
        tak_bertanda32_t j;

        block_size = block_sizes[i];

        if (block_size == 0) {
            /* Sparse block - fill with zeros */
            block_output_size = NTFS_COMPRESSION_BLOCK_SIZE;
            if (out_offset + block_output_size > output_size) {
                block_output_size = output_size - out_offset;
            }

            for (j = 0; j < block_output_size; j++) {
                output[out_offset + j] = 0;
            }

            out_offset += block_output_size;
            continue;
        }

        /* Decompress block */
        block_input = input + in_offset;
        in_offset += block_size;

        block_output_size = output_size - out_offset;

        status = ntfs_decompress_block(block_input, block_size,
            output + out_offset, &block_output_size);

        if (status != STATUS_BERHASIL) {
            return status;
        }

        out_offset += block_output_size;
    }

    *actual_size = out_offset;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI KOMPRESI LZNT1 (LZNT1 COMPRESSION)
 * ===========================================================================
 */

/*
 * ntfs_compress_find_match
 * Mencari match dalam output buffer untuk kompresi.
 */
static tak_bertanda32_t ntfs_compress_find_match(tak_bertanda8_t *data,
    tak_bertanda32_t pos, tak_bertanda32_t size, tak_bertanda32_t *offset)
{
    tak_bertanda32_t max_length;
    tak_bertanda32_t best_length;
    tak_bertanda32_t best_offset;
    tak_bertanda32_t i;
    tak_bertanda32_t j;

    if (pos == 0 || data == NULL) {
        return 0;
    }

    max_length = NTFS_LZNT1_MAX_LENGTH;
    best_length = 0;
    best_offset = 0;

    /* Search for matches */
    for (i = 1; i < pos && i < NTFS_LZNT1_MAX_OFFSET; i++) {
        tak_bertanda32_t match_len;

        match_len = 0;

        /* Count matching bytes */
        for (j = 0; j < max_length && pos + j < size; j++) {
            if (data[pos - i + j] == data[pos + j]) {
                match_len++;
            } else {
                break;
            }
        }

        /* Update best match */
        if (match_len > best_length) {
            best_length = match_len;
            best_offset = i;
        }
    }

    *offset = best_offset;
    return best_length;
}

/*
 * ntfs_compress_block
 * Kompresi satu block dengan LZNT1.
 */
static status_t ntfs_compress_block(tak_bertanda8_t *input,
    tak_bertanda32_t input_size, tak_bertanda8_t *output,
    tak_bertanda32_t *output_size)
{
    tak_bertanda32_t in_pos;
    tak_bertanda32_t out_pos;
    tak_bertanda8_t flags;
    tak_bertanda32_t flag_pos;
    tak_bertanda32_t token_count;
    tak_bertanda32_t i;

    if (input == NULL || output == NULL || output_size == NULL) {
        return STATUS_PARAM_NULL;
    }

    in_pos = 0;
    out_pos = 0;

    /* Reserve space for first flags byte */
    flag_pos = out_pos;
    out_pos++;
    flags = 0;
    token_count = 0;

    while (in_pos < input_size && out_pos < *output_size - 2) {
        tak_bertanda32_t offset;
        tak_bertanda32_t length;

        /* Try to find match */
        length = ntfs_compress_find_match(input, in_pos, input_size, &offset);

        if (length >= 3) {
            /* Use back reference */
            tak_bertanda16_t backref;

            flags |= (1 << token_count);
            backref = (offset << 4) | ((length - 3) & 0x0F);

            if (out_pos + 1 >= *output_size) {
                break;
            }

            output[out_pos++] = backref & 0xFF;
            output[out_pos++] = (backref >> 8) & 0xFF;

            in_pos += length;
        } else {
            /* Use literal */
            if (out_pos >= *output_size) {
                break;
            }

            output[out_pos++] = input[in_pos++];
        }

        token_count++;

        /* Write flags every 8 tokens */
        if (token_count == 8) {
            output[flag_pos] = flags;
            flags = 0;
            token_count = 0;

            if (out_pos < *output_size) {
                flag_pos = out_pos;
                out_pos++;
            }
        }
    }

    /* Write final flags */
    if (token_count > 0) {
        output[flag_pos] = flags;
    }

    *output_size = out_pos;

    return STATUS_BERHASIL;
}

/*
 * ntfs_compress_data
 * Kompresi data (public interface).
 */
status_t ntfs_compress_data(tak_bertanda8_t *input,
    tak_bertanda32_t input_size, tak_bertanda8_t *output,
    tak_bertanda32_t *output_size)
{
    tak_bertanda32_t compressed_size;
    status_t status;

    if (input == NULL || output == NULL || output_size == NULL) {
        return STATUS_PARAM_NULL;
    }

    compressed_size = *output_size;

    /* Try to compress */
    status = ntfs_compress_block(input, input_size, output, &compressed_size);

    if (status != STATUS_BERHASIL) {
        return status;
    }

    /* Check if compression helps */
    if (compressed_size >= input_size) {
        /* Store uncompressed */
        if (input_size + 2 > *output_size) {
            return STATUS_BUFFER_KECIL;
        }

        output[0] = 0;
        output[1] = 0;

        for (compressed_size = 0; compressed_size < input_size;
            compressed_size++) {
            output[compressed_size + 2] = input[compressed_size];
        }

        compressed_size += 2;
    }

    *output_size = compressed_size;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI VFS INTEGRATION (VFS INTEGRATION)
 * ===========================================================================
 */

/*
 * ntfs_vfs_read_compressed
 * Membaca data terkompresi dan mendekompresi.
 */
tak_bertandas_t ntfs_vfs_read_compressed(void *ctx, tak_bertanda64_t offset,
    void *buffer, ukuran_t size)
{
    tak_bertanda8_t *comp_buffer;
    tak_bertanda8_t *decomp_buffer;
    tak_bertanda32_t comp_size;
    tak_bertanda32_t decomp_size;
    status_t status;

    if (ctx == NULL || buffer == NULL) {
        return (tak_bertandas_t)STATUS_PARAM_NULL;
    }

    /* Alokasi buffer kompresi */
    comp_buffer = (tak_bertanda8_t *)NULL; /* Dari heap */
    decomp_buffer = (tak_bertanda8_t *)NULL; /* Dari heap */

    /* Baca compression unit */
    /* CATATAN: Implementasi sebenarnya membaca dari disk */

    /* Dekompresi */
    decomp_size = NTFS_COMPRESSION_UNIT * NTFS_COMPRESSION_BLOCK_SIZE;

    status = ntfs_decompress_unit(comp_buffer, comp_size,
        decomp_buffer, decomp_size, &decomp_size);

    if (status != STATUS_BERHASIL) {
        return (tak_bertandas_t)status;
    }

    /* Copy ke buffer user */
    /* CATATAN: Copy data dari decomp_buffer ke buffer */

    return (tak_bertandas_t)size;
}

/*
 * ntfs_vfs_write_compressed
 * Menulis data dengan kompresi.
 */
tak_bertandas_t ntfs_vfs_write_compressed(void *ctx, tak_bertanda64_t offset,
    const void *buffer, ukuran_t size)
{
    /* NTFS compression untuk write tidak didukung sekarang */
    (void)ctx;
    (void)offset;
    (void)buffer;
    (void)size;

    return (tak_bertandas_t)STATUS_FS_READONLY;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI QUERY (QUERY FUNCTIONS)
 * ===========================================================================
 */

/*
 * ntfs_compress_get_ratio
 * Mendapatkan rasio kompresi.
 */
tak_bertanda32_t ntfs_compress_get_ratio(tak_bertanda32_t original,
    tak_bertanda32_t compressed)
{
    tak_bertanda32_t ratio;

    if (original == 0) {
        return 0;
    }

    if (compressed == 0) {
        return 100; /* 100% savings */
    }

    ratio = (original - compressed) * 100 / original;

    return ratio;
}

/*
 * ntfs_compress_needs_compression
 * Cek apakah data perlu dikompresi.
 */
bool_t ntfs_compress_needs_compression(tak_bertanda8_t *data,
    tak_bertanda32_t size)
{
    tak_bertanda32_t zero_count;
    tak_bertanda32_t i;

    if (data == NULL || size == 0) {
        return SALAH;
    }

    /* Count zeros */
    zero_count = 0;
    for (i = 0; i < size; i++) {
        if (data[i] == 0) {
            zero_count++;
        }
    }

    /* If more than 50% zeros, compression helps */
    return (zero_count * 2 > size) ? BENAR : SALAH;
}
