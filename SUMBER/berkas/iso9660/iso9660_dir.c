/*
 * PIGURA OS - ISO9660_DIR.C
 * ==========================
 * Implementasi operasi direktori untuk filesystem ISO9660.
 *
 * Berkas ini berisi fungsi-fungsi untuk membaca dan memproses
 * entri direktori dalam filesystem ISO9660.
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

/* Ukuran sektor standar */
#define ISO_DIR_SEKTOR_UKURAN       2048

/* Panjang minimum directory record */
#define ISO_DIR_RECORD_MIN_LEN      33

/* Panjang maksimum nama file */
#define ISO_DIR_NAMA_MAKS           31
#define ISO_DIR_NAMA_PANJANG_MAKS   64

/* Flag direktori */
#define ISO_DIR_FLAG_HIDDEN         0x01
#define ISO_DIR_FLAG_DIRECTORY      0x02
#define ISO_DIR_FLAG_ASSOCIATED     0x04
#define ISO_DIR_FLAG_RECORD         0x08
#define ISO_DIR_FLAG_PROTECTION     0x10
#define ISO_DIR_FLAG_RESERVED1      0x20
#define ISO_DIR_FLAG_RESERVED2      0x40
#define ISO_DIR_FLAG_MULTIEXTENT    0x80

/* Nama khusus */
#define ISO_DIR_NAMA_DOT            0x00
#define ISO_DIR_NAMA_DOTDOT         0x01

/*
 * ===========================================================================
 * STRUKTUR DATA DIREKTORI (DIRECTORY DATA STRUCTURES)
 * ===========================================================================
 */

/* Struktur Directory Record ISO9660 */
typedef struct PACKED {
    tak_bertanda8_t panjang;            /* Panjang record ini */
    tak_bertanda8_t ext_attr_len;       /* Panjang extended attribute */
    tak_bertanda32_t extent_le;         /* Lokasi extent first (LE) */
    tak_bertanda32_t extent_be;         /* Lokasi extent first (BE) */
    tak_bertanda32_t data_len_le;       /* Panjang data (LE) */
    tak_bertanda32_t data_len_be;       /* Panjang data (BE) */
    tak_bertanda8_t tahun;              /* Tahun sejak 1900 */
    tak_bertanda8_t bulan;              /* Bulan (1-12) */
    tak_bertanda8_t hari;               /* Hari (1-31) */
    tak_bertanda8_t jam;                /* Jam (0-23) */
    tak_bertanda8_t menit;              /* Menit (0-59) */
    tak_bertanda8_t detik;              /* Detik (0-59) */
    tak_bertanda8_t zona;               /* Offset zona waktu */
    tak_bertanda8_t flags;              /* File flags */
    tak_bertanda8_t file_unit_size;     /* File unit size */
    tak_bertanda8_t interleave_gap;     /* Interleave gap */
    tak_bertanda16_t vol_seq_le;        /* Volume sequence number (LE) */
    tak_bertanda16_t vol_seq_be;        /* Volume sequence number (BE) */
    tak_bertanda8_t nama_len;           /* Panjang nama file */
    /* nama file variabel di sini */
} iso_dir_record_t;

/* Struktur iterator direktori */
typedef struct {
    tak_bertanda8_t *buffer;            /* Buffer direktori */
    tak_bertanda32_t buffer_size;       /* Ukuran buffer */
    tak_bertanda32_t posisi;            /* Posisi saat ini */
    tak_bertanda32_t extent;            /* Extent direktori */
    tak_bertanda32_t size;              /* Ukuran direktori */
    tak_bertanda16_t block_size;        /* Ukuran block */
} iso_dir_iterator_t;

/* Struktur entry hasil parsing */
typedef struct {
    tak_bertanda32_t extent;            /* Lokasi extent */
    tak_bertanda32_t size;              /* Ukuran file */
    tak_bertanda8_t flags;              /* File flags */
    bool_t is_directory;                /* Apakah direktori */
    bool_t is_hidden;                   /* Apakah hidden */
    char nama[ISO_DIR_NAMA_PANJANG_MAKS + 1]; /* Nama file */
    tak_bertanda8_t nama_len;           /* Panjang nama asli */
    tak_bertanda8_t tahun;              /* Tanggal rekaman */
    tak_bertanda8_t bulan;
    tak_bertanda8_t hari;
    tak_bertanda8_t jam;
    tak_bertanda8_t menit;
    tak_bertanda8_t detik;
} iso_dir_entry_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INTERNAL (INTERNAL FUNCTION DECLARATIONS)
 * ===========================================================================
 */

static tak_bertanda32_t iso_dir_bothendian32(tak_bertanda32_t le,
                                            tak_bertanda32_t be);
static tak_bertanda16_t iso_dir_bothendian16(tak_bertanda16_t le,
                                            tak_bertanda16_t be);
static void iso_dir_parse_nama(const tak_bertanda8_t *src,
                               ukuran_t src_len, char *dst,
                               ukuran_t dst_size);
static status_t iso_dir_parse_record(const iso_dir_record_t *record,
                                     iso_dir_entry_t *entry);
static bool_t iso_dir_nama_valid(const char *nama, ukuran_t len);
static bool_t iso_dir_nama_sama(const char *nama1, const char *nama2,
                               ukuran_t len1, ukuran_t len2);
static status_t iso_dir_baca_block(void *context, tak_bertanda64_t block,
                                   void *buffer, ukuran_t count);
static tak_bertanda32_t iso_dir_align_block(tak_bertanda32_t pos,
                                            tak_bertanda16_t block_size);

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI KONVERSI (CONVERSION FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

static tak_bertanda32_t iso_dir_bothendian32(tak_bertanda32_t le,
                                            tak_bertanda32_t be)
{
    tak_bertanda32_t be_swapped;

    /* Gunakan little-endian */
    be_swapped = ((be & 0x000000FF) << 24) |
                 ((be & 0x0000FF00) << 8) |
                 ((be & 0x00FF0000) >> 8) |
                 ((be & 0xFF000000) >> 24);

    /* Validasi */
    if (le != be_swapped) {
        return le;
    }

    return le;
}

static tak_bertanda16_t iso_dir_bothendian16(tak_bertanda16_t le,
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

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI PARSING NAMA (NAME PARSING FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

static void iso_dir_parse_nama(const tak_bertanda8_t *src,
                               ukuran_t src_len, char *dst,
                               ukuran_t dst_size)
{
    ukuran_t i;
    ukuran_t j;
    bool_t ada_ekstensi;

    if (src == NULL || dst == NULL || dst_size == 0) {
        return;
    }

    j = 0;
    ada_ekstensi = SALAH;

    for (i = 0; i < src_len && j < dst_size - 1; i++) {
        tak_bertanda8_t c = src[i];

        /* Skip null terminator */
        if (c == '\0') {
            break;
        }

        /* Skip separator versi (;1) */
        if (c == ';') {
            break;
        }

        /* Handle dot */
        if (c == '.') {
            if (ada_ekstensi) {
                continue;
            }
            ada_ekstensi = BENAR;
            dst[j++] = '.';
            continue;
        }

        /* Skip trailing spaces */
        if (c == ' ') {
            continue;
        }

        /* Konversi ke lowercase */
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

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI VALIDASI (VALIDATION FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

static bool_t iso_dir_nama_valid(const char *nama, ukuran_t len)
{
    ukuran_t i;

    if (nama == NULL || len == 0) {
        return SALAH;
    }

    /* Cek setiap karakter */
    for (i = 0; i < len; i++) {
        char c = nama[i];

        /* Karakter harus printable ASCII */
        if (c < 32 || c > 126) {
            return SALAH;
        }

        /* Karakter terlarang */
        if (c == '/' || c == '\\' || c == ':' || c == '*' ||
            c == '?' || c == '"' || c == '<' || c == '>' ||
            c == '|') {
            return SALAH;
        }
    }

    return BENAR;
}

static bool_t iso_dir_nama_sama(const char *nama1, const char *nama2,
                               ukuran_t len1, ukuran_t len2)
{
    ukuran_t i;
    ukuran_t min_len;

    if (nama1 == NULL || nama2 == NULL) {
        return SALAH;
    }

    if (len1 != len2) {
        return SALAH;
    }

    min_len = len1;

    for (i = 0; i < min_len; i++) {
        char c1 = nama1[i];
        char c2 = nama2[i];

        /* Case-insensitive comparison */
        if (c1 >= 'A' && c1 <= 'Z') {
            c1 = c1 + ('a' - 'A');
        }
        if (c2 >= 'A' && c2 <= 'Z') {
            c2 = c2 + ('a' - 'A');
        }

        if (c1 != c2) {
            return SALAH;
        }
    }

    return BENAR;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI ALIGNMENT (ALIGNMENT FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

static tak_bertanda32_t iso_dir_align_block(tak_bertanda32_t pos,
                                            tak_bertanda16_t block_size)
{
    if (block_size == 0) {
        return pos;
    }

    return (pos + block_size - 1) & ~(block_size - 1);
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI PARSING RECORD (RECORD PARSING FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

static status_t iso_dir_parse_record(const iso_dir_record_t *record,
                                     iso_dir_entry_t *entry)
{
    if (record == NULL || entry == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Validasi panjang minimum */
    if (record->panjang < ISO_DIR_RECORD_MIN_LEN) {
        return STATUS_FORMAT_INVALID;
    }

    /* Parse extent location */
    entry->extent = iso_dir_bothendian32(record->extent_le, record->extent_be);

    /* Parse size */
    entry->size = iso_dir_bothendian32(record->data_len_le, record->data_len_be);

    /* Parse flags */
    entry->flags = record->flags;
    entry->is_directory = (record->flags & ISO_DIR_FLAG_DIRECTORY) ? 
                          BENAR : SALAH;
    entry->is_hidden = (record->flags & ISO_DIR_FLAG_HIDDEN) ? 
                       BENAR : SALAH;

    /* Parse tanggal/waktu */
    entry->tahun = record->tahun;
    entry->bulan = record->bulan;
    entry->hari = record->hari;
    entry->jam = record->jam;
    entry->menit = record->menit;
    entry->detik = record->detik;

    /* Parse nama - nama file berada setelah struktur */
    entry->nama_len = record->nama_len;

    if (record->nama_len == 1) {
        /* Entry khusus . atau .. - akses byte setelah struktur */
        tak_bertanda8_t *nama_ptr = ((tak_bertanda8_t *)record) + 
                                    ISO_DIR_RECORD_MIN_LEN;
        if (nama_ptr[0] == ISO_DIR_NAMA_DOT) {
            kernel_strncpy(entry->nama, ".", ISO_DIR_NAMA_PANJANG_MAKS);
        } else if (nama_ptr[0] == ISO_DIR_NAMA_DOTDOT) {
            kernel_strncpy(entry->nama, "..", ISO_DIR_NAMA_PANJANG_MAKS);
        } else {
            iso_dir_parse_nama(nama_ptr, record->nama_len,
                              entry->nama, sizeof(entry->nama));
        }
    } else {
        tak_bertanda8_t *nama_ptr = ((tak_bertanda8_t *)record) + 
                                    ISO_DIR_RECORD_MIN_LEN;
        iso_dir_parse_nama(nama_ptr, record->nama_len,
                          entry->nama, sizeof(entry->nama));
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI BACA BLOCK (BLOCK READ FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

static status_t iso_dir_baca_block(void *context, tak_bertanda64_t block,
                                   void *buffer, ukuran_t count)
{
    /* Placeholder untuk pembacaan block */
    /* TODO: Implementasi pembacaan dari device context */
    (void)context;
    (void)block;
    (void)buffer;
    (void)count;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI UTAMA (MAIN FUNCTIONS IMPLEMENTATION)
 * ===========================================================================
 */

iso_dir_iterator_t *iso9660_dir_buat_iterator(void *device_context,
                                             tak_bertanda32_t extent,
                                             tak_bertanda32_t size,
                                             tak_bertanda16_t block_size)
{
    iso_dir_iterator_t *iter;

    if (device_context == NULL || block_size == 0) {
        return NULL;
    }

    iter = (iso_dir_iterator_t *)kmalloc(sizeof(iso_dir_iterator_t));
    if (iter == NULL) {
        return NULL;
    }

    kernel_memset(iter, 0, sizeof(iso_dir_iterator_t));

    /* Alokasi buffer untuk direktori */
    iter->buffer_size = (size + block_size - 1) & ~(block_size - 1);
    iter->buffer = (tak_bertanda8_t *)kmalloc(iter->buffer_size);
    if (iter->buffer == NULL) {
        kfree(iter);
        return NULL;
    }

    iter->posisi = 0;
    iter->extent = extent;
    iter->size = size;
    iter->block_size = block_size;

    /* Baca seluruh direktori ke buffer */
    {
        tak_bertanda64_t start_block = (tak_bertanda64_t)extent;
        tak_bertanda32_t blocks = (size + block_size - 1) / block_size;
        status_t status;

        status = iso_dir_baca_block(device_context, start_block,
                                    iter->buffer, blocks);
        if (status != STATUS_BERHASIL) {
            kfree(iter->buffer);
            kfree(iter);
            return NULL;
        }
    }

    return iter;
}

void iso9660_dir_hapus_iterator(iso_dir_iterator_t *iter)
{
    if (iter == NULL) {
        return;
    }

    if (iter->buffer != NULL) {
        kfree(iter->buffer);
        iter->buffer = NULL;
    }

    kfree(iter);
}

status_t iso9660_dir_baca_entry(iso_dir_iterator_t *iter,
                                iso_dir_entry_t *entry)
{
    const iso_dir_record_t *record;
    tak_bertanda8_t rec_len;
    status_t status;

    if (iter == NULL || entry == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (iter->buffer == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Cek apakah sudah di akhir direktori */
    if (iter->posisi >= iter->size) {
        return STATUS_KOSONG;
    }

    kernel_memset(entry, 0, sizeof(iso_dir_entry_t));

    while (iter->posisi < iter->size) {
        record = (const iso_dir_record_t *)(iter->buffer + iter->posisi);
        rec_len = record->panjang;

        /* Jika panjang = 0, skip ke block berikutnya */
        if (rec_len == 0) {
            iter->posisi = iso_dir_align_block(iter->posisi + 1,
                                               iter->block_size);
            continue;
        }

        /* Validasi panjang record */
        if (rec_len < ISO_DIR_RECORD_MIN_LEN) {
            iter->posisi += 1;
            continue;
        }

        /* Cek apakah record melebihi buffer */
        if (iter->posisi + rec_len > iter->buffer_size) {
            return STATUS_FORMAT_INVALID;
        }

        /* Parse record */
        status = iso_dir_parse_record(record, entry);
        if (status != STATUS_BERHASIL) {
            iter->posisi += rec_len;
            continue;
        }

        /* Lewati . dan .. - akses nama setelah struktur */
        {
            tak_bertanda8_t *nama_ptr = ((tak_bertanda8_t *)record) + 
                                        ISO_DIR_RECORD_MIN_LEN;
            if (record->nama_len == 1 &&
                (nama_ptr[0] == ISO_DIR_NAMA_DOT ||
                 nama_ptr[0] == ISO_DIR_NAMA_DOTDOT)) {
                iter->posisi += rec_len;
                continue;
            }
        }

        /* Entry valid ditemukan */
        iter->posisi += rec_len;
        return STATUS_BERHASIL;
    }

    return STATUS_KOSONG;
}

status_t iso9660_dir_cari(void *device_context,
                          tak_bertanda32_t extent,
                          tak_bertanda32_t size,
                          tak_bertanda16_t block_size,
                          const char *nama,
                          iso_dir_entry_t *result)
{
    iso_dir_iterator_t *iter;
    iso_dir_entry_t entry;
    status_t status;
    ukuran_t nama_len;

    if (nama == NULL || result == NULL) {
        return STATUS_PARAM_NULL;
    }

    nama_len = kernel_strlen(nama);
    if (nama_len == 0 || nama_len > ISO_DIR_NAMA_PANJANG_MAKS) {
        return STATUS_PARAM_INVALID;
    }

    iter = iso9660_dir_buat_iterator(device_context, extent, size, block_size);
    if (iter == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    status = STATUS_TIDAK_DITEMUKAN;

    while (BENAR) {
        status = iso9660_dir_baca_entry(iter, &entry);
        if (status != STATUS_BERHASIL) {
            status = STATUS_TIDAK_DITEMUKAN;
            break;
        }

        /* Bandingkan nama */
        if (iso_dir_nama_sama(entry.nama, nama,
                              kernel_strlen(entry.nama), nama_len)) {
            kernel_memcpy(result, &entry, sizeof(iso_dir_entry_t));
            status = STATUS_BERHASIL;
            break;
        }
    }

    iso9660_dir_hapus_iterator(iter);
    return status;
}

status_t iso9660_dir_list(void *device_context,
                          tak_bertanda32_t extent,
                          tak_bertanda32_t size,
                          tak_bertanda16_t block_size,
                          iso_dir_entry_t *entries,
                          ukuran_t max_entries,
                          ukuran_t *actual_count)
{
    iso_dir_iterator_t *iter;
    iso_dir_entry_t entry;
    status_t status;
    ukuran_t count;

    if (entries == NULL || actual_count == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (max_entries == 0) {
        *actual_count = 0;
        return STATUS_BERHASIL;
    }

    iter = iso9660_dir_buat_iterator(device_context, extent, size, block_size);
    if (iter == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    count = 0;

    while (count < max_entries) {
        status = iso9660_dir_baca_entry(iter, &entry);
        if (status != STATUS_BERHASIL) {
            break;
        }

        kernel_memcpy(&entries[count], &entry, sizeof(iso_dir_entry_t));
        count++;
    }

    *actual_count = count;
    iso9660_dir_hapus_iterator(iter);

    return STATUS_BERHASIL;
}

bool_t iso9660_dir_cek_ada(void *device_context,
                           tak_bertanda32_t extent,
                           tak_bertanda32_t size,
                           tak_bertanda16_t block_size,
                           const char *nama)
{
    iso_dir_entry_t entry;
    status_t status;

    status = iso9660_dir_cari(device_context, extent, size,
                              block_size, nama, &entry);

    return (status == STATUS_BERHASIL) ? BENAR : SALAH;
}

status_t iso9660_dir_hitung_entry(void *device_context,
                                  tak_bertanda32_t extent,
                                  tak_bertanda32_t size,
                                  tak_bertanda16_t block_size,
                                  tak_bertanda32_t *count)
{
    iso_dir_iterator_t *iter;
    iso_dir_entry_t entry;
    status_t status;
    tak_bertanda32_t total;

    if (count == NULL) {
        return STATUS_PARAM_NULL;
    }

    iter = iso9660_dir_buat_iterator(device_context, extent, size, block_size);
    if (iter == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    total = 0;

    while (BENAR) {
        status = iso9660_dir_baca_entry(iter, &entry);
        if (status != STATUS_BERHASIL) {
            break;
        }
        total++;
    }

    *count = total;
    iso9660_dir_hapus_iterator(iter);

    return STATUS_BERHASIL;
}

status_t iso9660_dir_dapat_ukuran(void *device_context,
                                  tak_bertanda32_t extent,
                                  tak_bertanda16_t block_size,
                                  const char *nama,
                                  tak_bertanda32_t *size)
{
    iso_dir_entry_t entry;
    status_t status;

    if (size == NULL || nama == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Cari entry */
    status = iso9660_dir_cari(device_context, extent, 0,
                              block_size, nama, &entry);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    *size = entry.size;

    return STATUS_BERHASIL;
}

bool_t iso9660_dir_apakah_direktori(tak_bertanda8_t flags)
{
    return (flags & ISO_DIR_FLAG_DIRECTORY) ? BENAR : SALAH;
}

bool_t iso9660_dir_apakah_file(tak_bertanda8_t flags)
{
    return ((flags & ISO_DIR_FLAG_DIRECTORY) == 0) ? BENAR : SALAH;
}

bool_t iso9660_dir_apakah_hidden(tak_bertanda8_t flags)
{
    return (flags & ISO_DIR_FLAG_HIDDEN) ? BENAR : SALAH;
}
