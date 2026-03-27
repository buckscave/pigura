/*
 * PIGURA OS - EXT2_BITMAP.C
 * ==========================
 * Operasi bitmap filesystem EXT2 untuk kernel Pigura OS.
 *
 * Berkas ini berisi implementasi operasi-operasi bitmap yang digunakan
 * untuk alokasi inode dan block dalam filesystem EXT2. Bitmap adalah
 * struktur data penting untuk manajemen ruang disk.
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
 * KONSTANTA BITMAP (BITMAP CONSTANTS)
 * ===========================================================================
 */

/* Bits per byte */
#define BITS_PER_BYTE               8

/* Default bitmap block size */
#define BITMAP_BLOCK_SIZE           1024

/* Bits per block (for 1K block) */
#define BITS_PER_BLOCK_1K           8192

/* Invalid bit number */
#define BIT_INVALID                 0xFFFFFFFF

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INTERNAL (INTERNAL FUNCTION DECLARATIONS)
 * ===========================================================================
 */

static tak_bertanda32_t ext2_bitmap_find_first_zero(tak_bertanda8_t *bitmap,
    tak_bertanda32_t size_bits);
static tak_bertanda32_t ext2_bitmap_find_first_set(tak_bertanda8_t *bitmap,
    tak_bertanda32_t size_bits);

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI BIT DASAR (BASIC BIT FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_bitmap_test_bit
 * Menguji apakah bit tertentu diset.
 */
bool_t ext2_bitmap_test_bit(tak_bertanda8_t *bitmap, tak_bertanda32_t bit)
{
    tak_bertanda32_t byte_index;
    tak_bertanda32_t bit_index;
    tak_bertanda8_t mask;

    if (bitmap == NULL) {
        return SALAH;
    }

    byte_index = bit / BITS_PER_BYTE;
    bit_index = bit % BITS_PER_BYTE;
    mask = (tak_bertanda8_t)(1 << bit_index);

    return (bitmap[byte_index] & mask) ? BENAR : SALAH;
}

/*
 * ext2_bitmap_set_bit
 * Set bit tertentu ke 1.
 */
status_t ext2_bitmap_set_bit(tak_bertanda8_t *bitmap, tak_bertanda32_t bit)
{
    tak_bertanda32_t byte_index;
    tak_bertanda32_t bit_index;
    tak_bertanda8_t mask;

    if (bitmap == NULL) {
        return STATUS_PARAM_NULL;
    }

    byte_index = bit / BITS_PER_BYTE;
    bit_index = bit % BITS_PER_BYTE;
    mask = (tak_bertanda8_t)(1 << bit_index);

    bitmap[byte_index] |= mask;

    return STATUS_BERHASIL;
}

/*
 * ext2_bitmap_clear_bit
 * Clear bit tertentu ke 0.
 */
status_t ext2_bitmap_clear_bit(tak_bertanda8_t *bitmap, tak_bertanda32_t bit)
{
    tak_bertanda32_t byte_index;
    tak_bertanda32_t bit_index;
    tak_bertanda8_t mask;

    if (bitmap == NULL) {
        return STATUS_PARAM_NULL;
    }

    byte_index = bit / BITS_PER_BYTE;
    bit_index = bit % BITS_PER_BYTE;
    mask = (tak_bertanda8_t)(1 << bit_index);

    bitmap[byte_index] &= (tak_bertanda8_t)~mask;

    return STATUS_BERHASIL;
}

/*
 * ext2_bitmap_toggle_bit
 * Toggle bit tertentu.
 */
status_t ext2_bitmap_toggle_bit(tak_bertanda8_t *bitmap, tak_bertanda32_t bit)
{
    tak_bertanda32_t byte_index;
    tak_bertanda32_t bit_index;
    tak_bertanda8_t mask;

    if (bitmap == NULL) {
        return STATUS_PARAM_NULL;
    }

    byte_index = bit / BITS_PER_BYTE;
    bit_index = bit % BITS_PER_BYTE;
    mask = (tak_bertanda8_t)(1 << bit_index);

    bitmap[byte_index] ^= mask;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI PENCARIAN BIT (BIT SEARCH FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_bitmap_find_first_zero
 * Mencari bit pertama yang 0 dalam bitmap.
 */
static tak_bertanda32_t ext2_bitmap_find_first_zero(tak_bertanda8_t *bitmap,
    tak_bertanda32_t size_bits)
{
    tak_bertanda32_t byte_count;
    tak_bertanda32_t i;
    tak_bertanda32_t j;
    tak_bertanda8_t byte_val;

    if (bitmap == NULL || size_bits == 0) {
        return BIT_INVALID;
    }

    byte_count = (size_bits + BITS_PER_BYTE - 1) / BITS_PER_BYTE;

    /* Cari byte yang tidak semua bitnya 1 */
    for (i = 0; i < byte_count; i++) {
        byte_val = bitmap[i];

        /* Jika byte tidak semua 1 (0xFF) */
        if (byte_val != 0xFF) {
            /* Cari bit 0 dalam byte ini */
            for (j = 0; j < BITS_PER_BYTE; j++) {
                tak_bertanda32_t bit_num = i * BITS_PER_BYTE + j;

                if (bit_num >= size_bits) {
                    return BIT_INVALID; /* Melebihi ukuran */
                }

                if (!(byte_val & (1 << j))) {
                    return bit_num;
                }
            }
        }
    }

    return BIT_INVALID; /* Tidak ada bit 0 */
}

/*
 * ext2_bitmap_find_first_set
 * Mencari bit pertama yang 1 dalam bitmap.
 */
static tak_bertanda32_t ext2_bitmap_find_first_set(tak_bertanda8_t *bitmap,
    tak_bertanda32_t size_bits)
{
    tak_bertanda32_t byte_count;
    tak_bertanda32_t i;
    tak_bertanda32_t j;
    tak_bertanda8_t byte_val;

    if (bitmap == NULL || size_bits == 0) {
        return BIT_INVALID;
    }

    byte_count = (size_bits + BITS_PER_BYTE - 1) / BITS_PER_BYTE;

    /* Cari byte yang tidak semua bitnya 0 */
    for (i = 0; i < byte_count; i++) {
        byte_val = bitmap[i];

        /* Jika byte tidak semua 0 */
        if (byte_val != 0x00) {
            /* Cari bit 1 dalam byte ini */
            for (j = 0; j < BITS_PER_BYTE; j++) {
                tak_bertanda32_t bit_num = i * BITS_PER_BYTE + j;

                if (bit_num >= size_bits) {
                    return BIT_INVALID; /* Melebihi ukuran */
                }

                if (byte_val & (1 << j)) {
                    return bit_num;
                }
            }
        }
    }

    return BIT_INVALID; /* Tidak ada bit 1 */
}

/*
 * ext2_bitmap_find_next_zero
 * Mencari bit 0 berikutnya mulai dari posisi tertentu.
 */
tak_bertanda32_t ext2_bitmap_find_next_zero(tak_bertanda8_t *bitmap,
    tak_bertanda32_t start_bit, tak_bertanda32_t size_bits)
{
    tak_bertanda32_t bit;

    if (bitmap == NULL || start_bit >= size_bits) {
        return BIT_INVALID;
    }

    /* Mulai pencarian dari start_bit */
    for (bit = start_bit; bit < size_bits; bit++) {
        if (!ext2_bitmap_test_bit(bitmap, bit)) {
            return bit;
        }
    }

    return BIT_INVALID;
}

/*
 * ext2_bitmap_find_next_set
 * Mencari bit 1 berikutnya mulai dari posisi tertentu.
 */
tak_bertanda32_t ext2_bitmap_find_next_set(tak_bertanda8_t *bitmap,
    tak_bertanda32_t start_bit, tak_bertanda32_t size_bits)
{
    tak_bertanda32_t bit;

    if (bitmap == NULL || start_bit >= size_bits) {
        return BIT_INVALID;
    }

    /* Mulai pencarian dari start_bit */
    for (bit = start_bit; bit < size_bits; bit++) {
        if (ext2_bitmap_test_bit(bitmap, bit)) {
            return bit;
        }
    }

    return BIT_INVALID;
}

/*
 * ext2_bitmap_find_zero_range
 * Mencari range bit 0 berurutan.
 */
tak_bertanda32_t ext2_bitmap_find_zero_range(tak_bertanda8_t *bitmap,
    tak_bertanda32_t count, tak_bertanda32_t size_bits)
{
    tak_bertanda32_t start_bit;
    tak_bertanda32_t found_count;
    tak_bertanda32_t bit;

    if (bitmap == NULL || count == 0 || count > size_bits) {
        return BIT_INVALID;
    }

    start_bit = 0;
    found_count = 0;

    for (bit = 0; bit < size_bits; bit++) {
        if (!ext2_bitmap_test_bit(bitmap, bit)) {
            if (found_count == 0) {
                start_bit = bit;
            }
            found_count++;

            if (found_count >= count) {
                return start_bit;
            }
        } else {
            found_count = 0;
        }
    }

    return BIT_INVALID;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI PENGHITUNGAN BIT (BIT COUNTING FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_bitmap_count_set
 * Menghitung jumlah bit yang diset (1).
 */
tak_bertanda32_t ext2_bitmap_count_set(tak_bertanda8_t *bitmap,
    tak_bertanda32_t size_bits)
{
    tak_bertanda32_t count;
    tak_bertanda32_t i;
    tak_bertanda8_t byte_val;

    if (bitmap == NULL || size_bits == 0) {
        return 0;
    }

    count = 0;

    for (i = 0; i < size_bits; i++) {
        if (ext2_bitmap_test_bit(bitmap, i)) {
            count++;
        }
    }

    return count;
}

/*
 * ext2_bitmap_count_clear
 * Menghitung jumlah bit yang clear (0).
 */
tak_bertanda32_t ext2_bitmap_count_clear(tak_bertanda8_t *bitmap,
    tak_bertanda32_t size_bits)
{
    if (bitmap == NULL || size_bits == 0) {
        return 0;
    }

    return size_bits - ext2_bitmap_count_set(bitmap, size_bits);
}

/*
 * ext2_bitmap_count_popcount
 * Popcount menggunakan lookup table untuk optimasi.
 */
tak_bertanda32_t ext2_bitmap_count_popcount(tak_bertanda8_t byte_val)
{
    /* Lookup table untuk popcount 8-bit */
    static const tak_bertanda8_t popcount_table[256] = {
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
    };

    return popcount_table[byte_val];
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI OPERASI BITMAP LENGKAP (FULL BITMAP OPERATIONS)
 * ===========================================================================
 */

/*
 * ext2_bitmap_set_range
 * Set range bit ke 1.
 */
status_t ext2_bitmap_set_range(tak_bertanda8_t *bitmap,
    tak_bertanda32_t start_bit, tak_bertanda32_t count)
{
    tak_bertanda32_t i;

    if (bitmap == NULL) {
        return STATUS_PARAM_NULL;
    }

    for (i = 0; i < count; i++) {
        ext2_bitmap_set_bit(bitmap, start_bit + i);
    }

    return STATUS_BERHASIL;
}

/*
 * ext2_bitmap_clear_range
 * Clear range bit ke 0.
 */
status_t ext2_bitmap_clear_range(tak_bertanda8_t *bitmap,
    tak_bertanda32_t start_bit, tak_bertanda32_t count)
{
    tak_bertanda32_t i;

    if (bitmap == NULL) {
        return STATUS_PARAM_NULL;
    }

    for (i = 0; i < count; i++) {
        ext2_bitmap_clear_bit(bitmap, start_bit + i);
    }

    return STATUS_BERHASIL;
}

/*
 * ext2_bitmap_clear_all
 * Clear semua bit dalam bitmap.
 */
status_t ext2_bitmap_clear_all(tak_bertanda8_t *bitmap,
    tak_bertanda32_t size_bits)
{
    tak_bertanda32_t byte_count;
    tak_bertanda32_t i;

    if (bitmap == NULL) {
        return STATUS_PARAM_NULL;
    }

    byte_count = (size_bits + BITS_PER_BYTE - 1) / BITS_PER_BYTE;

    for (i = 0; i < byte_count; i++) {
        bitmap[i] = 0x00;
    }

    return STATUS_BERHASIL;
}

/*
 * ext2_bitmap_set_all
 * Set semua bit dalam bitmap.
 */
status_t ext2_bitmap_set_all(tak_bertanda8_t *bitmap,
    tak_bertanda32_t size_bits)
{
    tak_bertanda32_t byte_count;
    tak_bertanda32_t i;

    if (bitmap == NULL) {
        return STATUS_PARAM_NULL;
    }

    byte_count = (size_bits + BITS_PER_BYTE - 1) / BITS_PER_BYTE;

    for (i = 0; i < byte_count; i++) {
        bitmap[i] = 0xFF;
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI ALOKASI INODE/BLOCK (INODE/BLOCK ALLOCATION)
 * ===========================================================================
 */

/*
 * ext2_alloc_inode
 * Alokasi inode baru dari bitmap.
 */
ino_t ext2_alloc_inode(tak_bertanda8_t *bitmap, tak_bertanda32_t size_bits,
    tak_bertanda32_t first_ino)
{
    tak_bertanda32_t bit;

    if (bitmap == NULL) {
        return 0;
    }

    /* Cari bit 0 pertama mulai dari first_ino */
    bit = ext2_bitmap_find_next_zero(bitmap, first_ino, size_bits);

    if (bit == BIT_INVALID) {
        return 0; /* Tidak ada inode bebas */
    }

    /* Set bit */
    ext2_bitmap_set_bit(bitmap, bit);

    /* Inode number = bit + 1 (karena inode dimulai dari 1) */
    return (ino_t)(bit + 1);
}

/*
 * ext2_free_inode
 * Bebaskan inode ke bitmap.
 */
status_t ext2_free_inode(tak_bertanda8_t *bitmap, ino_t ino)
{
    tak_bertanda32_t bit;

    if (bitmap == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (ino < 1) {
        return STATUS_PARAM_INVALID;
    }

    /* Bit = inode number - 1 */
    bit = (tak_bertanda32_t)(ino - 1);

    /* Clear bit */
    ext2_bitmap_clear_bit(bitmap, bit);

    return STATUS_BERHASIL;
}

/*
 * ext2_alloc_block
 * Alokasi block baru dari bitmap.
 */
tak_bertanda32_t ext2_alloc_block(tak_bertanda8_t *bitmap,
    tak_bertanda32_t size_bits, tak_bertanda32_t first_block)
{
    tak_bertanda32_t bit;

    if (bitmap == NULL) {
        return 0;
    }

    /* Cari bit 0 pertama */
    bit = ext2_bitmap_find_first_zero(bitmap, size_bits);

    if (bit == BIT_INVALID) {
        return 0; /* Tidak ada block bebas */
    }

    /* Set bit */
    ext2_bitmap_set_bit(bitmap, bit);

    /* Block number */
    return bit + first_block;
}

/*
 * ext2_free_block
 * Bebaskan block ke bitmap.
 */
status_t ext2_free_block(tak_bertanda8_t *bitmap,
    tak_bertanda32_t block_num, tak_bertanda32_t first_block)
{
    tak_bertanda32_t bit;

    if (bitmap == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Hitung bit number */
    bit = block_num - first_block;

    /* Clear bit */
    ext2_bitmap_clear_bit(bitmap, bit);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI VALIDASI (VALIDATION FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_bitmap_validate
 * Memvalidasi konsistensi bitmap.
 */
status_t ext2_bitmap_validate(tak_bertanda8_t *bitmap,
    tak_bertanda32_t size_bits, tak_bertanda32_t *free_count)
{
    if (bitmap == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (free_count != NULL) {
        *free_count = ext2_bitmap_count_clear(bitmap, size_bits);
    }

    return STATUS_BERHASIL;
}

/*
 * ext2_bitmap_is_full
 * Cek apakah bitmap penuh (semua bit 1).
 */
bool_t ext2_bitmap_is_full(tak_bertanda8_t *bitmap,
    tak_bertanda32_t size_bits)
{
    tak_bertanda32_t bit;

    if (bitmap == NULL) {
        return BENAR;
    }

    bit = ext2_bitmap_find_first_zero(bitmap, size_bits);

    return (bit == BIT_INVALID) ? BENAR : SALAH;
}

/*
 * ext2_bitmap_is_empty
 * Cek apakah bitmap kosong (semua bit 0).
 */
bool_t ext2_bitmap_is_empty(tak_bertanda8_t *bitmap,
    tak_bertanda32_t size_bits)
{
    tak_bertanda32_t bit;

    if (bitmap == NULL) {
        return BENAR;
    }

    bit = ext2_bitmap_find_first_set(bitmap, size_bits);

    return (bit == BIT_INVALID) ? BENAR : SALAH;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI COPY DAN COMPARE (COPY AND COMPARE FUNCTIONS)
 * ===========================================================================
 */

/*
 * ext2_bitmap_copy
 * Menyalin bitmap.
 */
status_t ext2_bitmap_copy(tak_bertanda8_t *dest, tak_bertanda8_t *src,
    tak_bertanda32_t size_bits)
{
    tak_bertanda32_t byte_count;
    tak_bertanda32_t i;

    if (dest == NULL || src == NULL) {
        return STATUS_PARAM_NULL;
    }

    byte_count = (size_bits + BITS_PER_BYTE - 1) / BITS_PER_BYTE;

    for (i = 0; i < byte_count; i++) {
        dest[i] = src[i];
    }

    return STATUS_BERHASIL;
}

/*
 * ext2_bitmap_compare
 * Membandingkan dua bitmap.
 */
bool_t ext2_bitmap_compare(tak_bertanda8_t *bitmap1,
    tak_bertanda8_t *bitmap2, tak_bertanda32_t size_bits)
{
    tak_bertanda32_t byte_count;
    tak_bertanda32_t i;

    if (bitmap1 == NULL || bitmap2 == NULL) {
        return SALAH;
    }

    byte_count = (size_bits + BITS_PER_BYTE - 1) / BITS_PER_BYTE;

    for (i = 0; i < byte_count; i++) {
        if (bitmap1[i] != bitmap2[i]) {
            return SALAH;
        }
    }

    return BENAR;
}

/*
 * ext2_bitmap_and
 * Operasi AND dua bitmap.
 */
status_t ext2_bitmap_and(tak_bertanda8_t *dest, tak_bertanda8_t *src1,
    tak_bertanda8_t *src2, tak_bertanda32_t size_bits)
{
    tak_bertanda32_t byte_count;
    tak_bertanda32_t i;

    if (dest == NULL || src1 == NULL || src2 == NULL) {
        return STATUS_PARAM_NULL;
    }

    byte_count = (size_bits + BITS_PER_BYTE - 1) / BITS_PER_BYTE;

    for (i = 0; i < byte_count; i++) {
        dest[i] = src1[i] & src2[i];
    }

    return STATUS_BERHASIL;
}

/*
 * ext2_bitmap_or
 * Operasi OR dua bitmap.
 */
status_t ext2_bitmap_or(tak_bertanda8_t *dest, tak_bertanda8_t *src1,
    tak_bertanda8_t *src2, tak_bertanda32_t size_bits)
{
    tak_bertanda32_t byte_count;
    tak_bertanda32_t i;

    if (dest == NULL || src1 == NULL || src2 == NULL) {
        return STATUS_PARAM_NULL;
    }

    byte_count = (size_bits + BITS_PER_BYTE - 1) / BITS_PER_BYTE;

    for (i = 0; i < byte_count; i++) {
        dest[i] = src1[i] | src2[i];
    }

    return STATUS_BERHASIL;
}

/*
 * ext2_bitmap_xor
 * Operasi XOR dua bitmap.
 */
status_t ext2_bitmap_xor(tak_bertanda8_t *dest, tak_bertanda8_t *src1,
    tak_bertanda8_t *src2, tak_bertanda32_t size_bits)
{
    tak_bertanda32_t byte_count;
    tak_bertanda32_t i;

    if (dest == NULL || src1 == NULL || src2 == NULL) {
        return STATUS_PARAM_NULL;
    }

    byte_count = (size_bits + BITS_PER_BYTE - 1) / BITS_PER_BYTE;

    for (i = 0; i < byte_count; i++) {
        dest[i] = src1[i] ^ src2[i];
    }

    return STATUS_BERHASIL;
}

/*
 * ext2_bitmap_not
 * Operasi NOT bitmap (invert).
 */
status_t ext2_bitmap_not(tak_bertanda8_t *dest, tak_bertanda8_t *src,
    tak_bertanda32_t size_bits)
{
    tak_bertanda32_t byte_count;
    tak_bertanda32_t i;

    if (dest == NULL || src == NULL) {
        return STATUS_PARAM_NULL;
    }

    byte_count = (size_bits + BITS_PER_BYTE - 1) / BITS_PER_BYTE;

    for (i = 0; i < byte_count; i++) {
        dest[i] = ~src[i];
    }

    return STATUS_BERHASIL;
}
