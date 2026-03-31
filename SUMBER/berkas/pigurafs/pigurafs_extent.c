/*
 * PIGURA OS - PIGURAFS_EXTENT.C
 * ==============================
 * Implementasi extent allocation untuk filesystem PiguraFS.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola extent
 * yang digunakan untuk alokasi space file.
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
 * KONSTANTA EXTENT (EXTENT CONSTANTS)
 * ===========================================================================
 */

/* Magic number */
#define PFS_EXTENT_MAGIC        0x50465334

/* Maximum extent per inode */
#define PFS_EXTENT_PER_INODE    4

/* Maximum extent dalam extent tree */
#define PFS_EXTENT_TREE_DEPTH   4

/* Flag extent */
#define PFS_EXTENT_FLAG_NONE    0x0000
#define PFS_EXTENT_FLAG_UNWRITTEN 0x0001
#define PFS_EXTENT_FLAG_INITIALIZED 0x0002

/* Minimum extent length */
#define PFS_EXTENT_MIN_LEN      1

/* Maximum extent length */
#define PFS_EXTENT_MAX_LEN      65535

/*
 * ===========================================================================
 * STRUKTUR DATA EXTENT (EXTENT DATA STRUCTURES)
 * ===========================================================================
 */

/* Struktur extent */
typedef struct {
    tak_bertanda32_t e_block;           /* Block logical awal */
    tak_bertanda32_t e_start;           /* Block fisik awal */
    tak_bertanda16_t e_len;             /* Panjang extent */
    tak_bertanda16_t e_flags;           /* Flag extent */
} pfs_extent_t;

/* Struktur extent header */
typedef struct {
    tak_bertanda32_t eh_magic;          /* Magic number */
    tak_bertanda16_t eh_entries;        /* Jumlah entry */
    tak_bertanda16_t eh_max;            /* Maximum entry */
    tak_bertanda16_t eh_depth;          /* Depth tree */
    tak_bertanda16_t eh_generation;     /* Generation */
} pfs_extent_header_t;

/* Struktur extent index (untuk internal node) */
typedef struct {
    tak_bertanda32_t ei_block;          /* Index block */
    tak_bertanda32_t ei_leaf;           /* Leaf block */
    tak_bertanda16_t ei_leaf_hi;        /* Leaf block high */
    tak_bertanda16_t ei_unused;         /* Reserved */
} pfs_extent_idx_t;

/* Struktur extent leaf entry */
typedef struct {
    tak_bertanda32_t ee_block;          /* First logical block */
    tak_bertanda16_t ee_len;            /* Number of blocks */
    tak_bertanda16_t ee_start_hi;       /* High 16-bit physical block */
    tak_bertanda32_t ee_start_lo;       /* Low 32-bit physical block */
} pfs_extent_leaf_t;

/* Struktur extent map in-memory */
typedef struct {
    tak_bertanda32_t magic;             /* Magic number validasi */
    pfs_extent_t extents[PFS_EXTENT_PER_INODE]; /* Array extent */
    tak_bertanda32_t count;             /* Jumlah extent */
    tak_bertanda32_t capacity;          /* Kapasitas array */
    bool_t dirty;                       /* Perlu sync? */
} pfs_extent_map_t;

/* Struktur extent iterator */
typedef struct {
    tak_bertanda32_t magic;             /* Magic number */
    pfs_extent_map_t *map;              /* Extent map */
    tak_bertanda32_t current_index;     /* Index saat ini */
    tak_bertanda32_t current_block;     /* Block saat ini dalam extent */
} pfs_extent_iter_t;

/* Struktur extent result */
typedef struct {
    tak_bertanda32_t log_block;         /* Block logical */
    tak_bertanda32_t phys_block;        /* Block fisik */
    tak_bertanda32_t length;            /* Panjang run */
    bool_t found;                       /* Apakah ditemukan? */
    bool_t unwritten;                   /* Apakah unwritten? */
} pfs_extent_result_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INTERNAL (INTERNAL FUNCTION DECLARATIONS)
 * ===========================================================================
 */

static status_t pfs_extent_compact(pfs_extent_map_t *map);
static status_t pfs_extent_merge_adjacent(pfs_extent_map_t *map);
static tak_bertanda32_t pfs_extent_find_insert_index(const pfs_extent_map_t *map,
                                                    tak_bertanda32_t log_block);
static status_t pfs_extent_split(pfs_extent_map_t *map,
                                 tak_bertanda32_t index,
                                 tak_bertanda32_t split_point);
static bool_t pfs_extent_can_merge(const pfs_extent_t *left,
                                   const pfs_extent_t *right);

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI UTILITAS (UTILITY FUNCTIONS)
 * ===========================================================================
 */

static bool_t pfs_extent_can_merge(const pfs_extent_t *left,
                                   const pfs_extent_t *right)
{
    if (left == NULL || right == NULL) {
        return SALAH;
    }

    /* Cek apakah adjacent secara logical */
    if (left->e_block + left->e_len != right->e_block) {
        return SALAH;
    }

    /* Cek apakah adjacent secara fisik */
    if (left->e_start + left->e_len != right->e_start) {
        return SALAH;
    }

    /* Cek apakah flag sama */
    if (left->e_flags != right->e_flags) {
        return SALAH;
    }

    /* Cek apakah total length tidak melebihi maximum */
    if ((tak_bertanda32_t)left->e_len + (tak_bertanda32_t)right->e_len >
        PFS_EXTENT_MAX_LEN) {
        return SALAH;
    }

    return BENAR;
}

static tak_bertanda32_t pfs_extent_find_insert_index(const pfs_extent_map_t *map,
                                                    tak_bertanda32_t log_block)
{
    tak_bertanda32_t low;
    tak_bertanda32_t high;
    tak_bertanda32_t mid;

    if (map == NULL || map->count == 0) {
        return 0;
    }

    /* Binary search */
    low = 0;
    high = map->count;

    while (low < high) {
        mid = (low + high) / 2;
        if (map->extents[mid].e_block < log_block) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }

    return low;
}

static status_t pfs_extent_compact(pfs_extent_map_t *map)
{
    tak_bertanda32_t write_idx;
    tak_bertanda32_t read_idx;

    if (map == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Compact extent array (remove zero-length extents) */
    write_idx = 0;

    for (read_idx = 0; read_idx < map->count; read_idx++) {
        if (map->extents[read_idx].e_len > 0) {
            if (write_idx != read_idx) {
                map->extents[write_idx] = map->extents[read_idx];
            }
            write_idx++;
        }
    }

    map->count = write_idx;

    return STATUS_BERHASIL;
}

static status_t pfs_extent_merge_adjacent(pfs_extent_map_t *map)
{
    tak_bertanda32_t i;
    tak_bertanda32_t write_idx;

    if (map == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (map->count <= 1) {
        return STATUS_BERHASIL;
    }

    /* Compact dulu */
    pfs_extent_compact(map);

    /* Merge adjacent extents */
    write_idx = 0;

    for (i = 0; i < map->count; i++) {
        if (write_idx == 0) {
            map->extents[write_idx] = map->extents[i];
            write_idx++;
        } else {
            /* Cek apakah bisa merge dengan extent sebelumnya */
            if (pfs_extent_can_merge(&map->extents[write_idx - 1],
                                     &map->extents[i])) {
                /* Merge */
                map->extents[write_idx - 1].e_len += map->extents[i].e_len;
            } else {
                /* Tidak bisa merge, tambah entry baru */
                map->extents[write_idx] = map->extents[i];
                write_idx++;
            }
        }
    }

    map->count = write_idx;

    return STATUS_BERHASIL;
}

static status_t pfs_extent_split(pfs_extent_map_t *map,
                                 tak_bertanda32_t index,
                                 tak_bertanda32_t split_point)
{
    pfs_extent_t *ext;
    pfs_extent_t new_ext;
    tak_bertanda32_t i;

    if (map == NULL || index >= map->count) {
        return STATUS_PARAM_INVALID;
    }

    if (map->count >= map->capacity) {
        return STATUS_PENUH;
    }

    ext = &map->extents[index];

    /* Cek validitas split point */
    if (split_point <= ext->e_block ||
        split_point >= ext->e_block + ext->e_len) {
        return STATUS_PARAM_INVALID;
    }

    /* Buat extent baru untuk bagian kedua */
    new_ext.e_block = split_point;
    new_ext.e_start = ext->e_start + (split_point - ext->e_block);
    new_ext.e_len = (tak_bertanda16_t)(ext->e_block + ext->e_len - split_point);
    new_ext.e_flags = ext->e_flags;

    /* Update extent asli */
    ext->e_len = (tak_bertanda16_t)(split_point - ext->e_block);

    /* Shift extents ke kanan */
    for (i = map->count; i > index + 1; i--) {
        map->extents[i] = map->extents[i - 1];
    }

    /* Insert extent baru */
    map->extents[index + 1] = new_ext;
    map->count++;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI UTAMA (MAIN FUNCTIONS)
 * ===========================================================================
 */

/*
 * pfs_extent_map_init
 * -------------------
 * Inisialisasi extent map.
 *
 * Parameter:
 *   map      - Pointer ke extent map
 *   capacity - Kapasitas maksimum
 *
 * Return: Status operasi
 */
status_t pfs_extent_map_init(pfs_extent_map_t *map, tak_bertanda32_t capacity)
{
    if (map == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memset(map, 0, sizeof(pfs_extent_map_t));
    map->magic = PFS_EXTENT_MAGIC;
    map->count = 0;
    map->capacity = (capacity > PFS_EXTENT_PER_INODE) ?
                    PFS_EXTENT_PER_INODE : capacity;
    map->dirty = SALAH;

    return STATUS_BERHASIL;
}

/*
 * pfs_extent_map_destroy
 * ----------------------
 * Hancurkan extent map.
 *
 * Parameter:
 *   map - Pointer ke extent map
 */
void pfs_extent_map_destroy(pfs_extent_map_t *map)
{
    if (map == NULL) {
        return;
    }

    map->magic = 0;
}

/*
 * pfs_extent_add
 * --------------
 * Tambah extent ke map.
 *
 * Parameter:
 *   map       - Pointer ke extent map
 *   log_block - Block logical awal
 *   phys_block - Block fisik awal
 *   len       - Panjang extent
 *   flags     - Flag extent
 *
 * Return: Status operasi
 */
status_t pfs_extent_add(pfs_extent_map_t *map, tak_bertanda32_t log_block,
                        tak_bertanda32_t phys_block, tak_bertanda16_t len,
                        tak_bertanda16_t flags)
{
    tak_bertanda32_t insert_index;
    tak_bertanda32_t i;
    status_t status;

    if (map == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (len == 0 || len >= PFS_EXTENT_MAX_LEN) {
        return STATUS_PARAM_INVALID;
    }

    /* Cek apakah sudah penuh */
    if (map->count >= map->capacity) {
        /* Coba compact dan merge */
        status = pfs_extent_compact(map);
        if (status != STATUS_BERHASIL) {
            return status;
        }

        status = pfs_extent_merge_adjacent(map);
        if (status != STATUS_BERHASIL) {
            return status;
        }

        if (map->count >= map->capacity) {
            return STATUS_PENUH;
        }
    }

    /* Cari posisi insert */
    insert_index = pfs_extent_find_insert_index(map, log_block);

    /* Cek overlap dengan extent sebelumnya */
    if (insert_index > 0) {
        pfs_extent_t *prev = &map->extents[insert_index - 1];
        if (prev->e_block + prev->e_len > log_block) {
            /* Ada overlap, split atau tolak */
            return STATUS_SUDAH_ADA;
        }
    }

    /* Cek overlap dengan extent sesudahnya */
    if (insert_index < map->count) {
        pfs_extent_t *next = &map->extents[insert_index];
        if (log_block + len > next->e_block) {
            return STATUS_SUDAH_ADA;
        }
    }

    /* Shift extents */
    for (i = map->count; i > insert_index; i--) {
        map->extents[i] = map->extents[i - 1];
    }

    /* Insert extent baru */
    map->extents[insert_index].e_block = log_block;
    map->extents[insert_index].e_start = phys_block;
    map->extents[insert_index].e_len = len;
    map->extents[insert_index].e_flags = flags;
    map->count++;
    map->dirty = BENAR;

    /* Coba merge dengan adjacent */
    pfs_extent_merge_adjacent(map);

    return STATUS_BERHASIL;
}

/*
 * pfs_extent_remove
 * -----------------
 * Hapus extent dari map.
 *
 * Parameter:
 *   map       - Pointer ke extent map
 *   log_block - Block logical awal
 *   len       - Panjang yang akan dihapus
 *
 * Return: Status operasi
 */
status_t pfs_extent_remove(pfs_extent_map_t *map, tak_bertanda32_t log_block,
                           tak_bertanda32_t len)
{
    tak_bertanda32_t i;
    tak_bertanda32_t end_block;
    bool_t found;

    if (map == NULL || len == 0) {
        return STATUS_PARAM_NULL;
    }

    end_block = log_block + len;
    found = SALAH;

    /* Cari dan hapus/modifikasi extent yang overlap */
    for (i = 0; i < map->count; i++) {
        pfs_extent_t *ext = &map->extents[i];
        tak_bertanda32_t ext_end = ext->e_block + ext->e_len;

        /* Cek overlap */
        if (ext_end <= log_block || ext->e_block >= end_block) {
            continue;
        }

        found = BENAR;

        /* Case 1: Seluruh extent dihapus */
        if (ext->e_block >= log_block && ext_end <= end_block) {
            ext->e_len = 0;  /* Mark sebagai deleted */
        }
        /* Case 2: Bagian awal dihapus */
        else if (ext->e_block < log_block && ext_end > log_block &&
                 ext_end <= end_block) {
            ext->e_len = (tak_bertanda16_t)(log_block - ext->e_block);
        }
        /* Case 3: Bagian akhir dihapus */
        else if (ext->e_block >= log_block && ext->e_block < end_block &&
                 ext_end > end_block) {
            tak_bertanda32_t cut = end_block - ext->e_block;
            ext->e_block = end_block;
            ext->e_start += cut;
            ext->e_len -= (tak_bertanda16_t)cut;
        }
        /* Case 4: Bagian tengah dihapus (perlu split) */
        else if (ext->e_block < log_block && ext_end > end_block) {
            status_t status;
            status = pfs_extent_split(map, i, log_block);
            if (status != STATUS_BERHASIL) {
                return status;
            }
            /* Sekarang hapus extent kedua */
            map->extents[i + 1].e_block = end_block;
            map->extents[i + 1].e_start += (end_block - log_block);
            map->extents[i + 1].e_len -= (tak_bertanda16_t)len;
        }
    }

    if (!found) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Compact array */
    pfs_extent_compact(map);
    map->dirty = BENAR;

    return STATUS_BERHASIL;
}

/*
 * pfs_extent_lookup
 * -----------------
 * Cari extent yang mengandung block logical.
 *
 * Parameter:
 *   map       - Pointer ke extent map
 *   log_block - Block logical yang dicari
 *   result    - Pointer ke hasil
 *
 * Return: Status operasi
 */
status_t pfs_extent_lookup(pfs_extent_map_t *map, tak_bertanda32_t log_block,
                           pfs_extent_result_t *result)
{
    tak_bertanda32_t low;
    tak_bertanda32_t high;
    tak_bertanda32_t mid;
    pfs_extent_t *ext;

    if (map == NULL || result == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memset(result, 0, sizeof(pfs_extent_result_t));
    result->found = SALAH;

    if (map->count == 0) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Binary search */
    low = 0;
    high = map->count;

    while (low < high) {
        mid = (low + high) / 2;
        ext = &map->extents[mid];

        if (log_block < ext->e_block) {
            high = mid;
        } else if (log_block >= ext->e_block + ext->e_len) {
            low = mid + 1;
        } else {
            /* Found! */
            result->log_block = log_block;
            result->phys_block = ext->e_start + (log_block - ext->e_block);
            result->length = ext->e_len - (log_block - ext->e_block);
            result->found = BENAR;
            result->unwritten = (ext->e_flags & PFS_EXTENT_FLAG_UNWRITTEN) ?
                               BENAR : SALAH;
            return STATUS_BERHASIL;
        }
    }

    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * pfs_extent_get_range
 * --------------------
 * Dapatkan range extent yang mengandung range logical.
 *
 * Parameter:
 *   map        - Pointer ke extent map
 *   log_start  - Block logical awal
 *   log_end    - Block logical akhir
 *   extents    - Array hasil extent
 *   max_count  - Kapasitas array
 *   actual_count - Jumlah extent aktual
 *
 * Return: Status operasi
 */
status_t pfs_extent_get_range(pfs_extent_map_t *map,
                              tak_bertanda32_t log_start,
                              tak_bertanda32_t log_end,
                              pfs_extent_t *extents,
                              tak_bertanda32_t max_count,
                              tak_bertanda32_t *actual_count)
{
    tak_bertanda32_t i;
    tak_bertanda32_t count;

    if (map == NULL || extents == NULL || actual_count == NULL) {
        return STATUS_PARAM_NULL;
    }

    *actual_count = 0;
    count = 0;

    for (i = 0; i < map->count && count < max_count; i++) {
        pfs_extent_t *ext = &map->extents[i];

        /* Cek overlap dengan range */
        if (ext->e_block + ext->e_len <= log_start) {
            continue;
        }
        if (ext->e_block >= log_end) {
            break;
        }

        /* Copy extent */
        extents[count] = *ext;
        count++;
    }

    *actual_count = count;

    return (count > 0) ? STATUS_BERHASIL : STATUS_TIDAK_DITEMUKAN;
}

/*
 * pfs_extent_get_total_blocks
 * ---------------------------
 * Hitung total block dalam extent map.
 *
 * Parameter:
 *   map - Pointer ke extent map
 *
 * Return: Total block
 */
tak_bertanda64_t pfs_extent_get_total_blocks(pfs_extent_map_t *map)
{
    tak_bertanda64_t total;
    tak_bertanda32_t i;

    if (map == NULL) {
        return 0;
    }

    total = 0;

    for (i = 0; i < map->count; i++) {
        total += map->extents[i].e_len;
    }

    return total;
}

/*
 * pfs_extent_is_contiguous
 * ------------------------
 * Cek apakah range block contiguous.
 *
 * Parameter:
 *   map       - Pointer ke extent map
 *   log_start - Block logical awal
 *   len       - Panjang range
 *
 * Return: BENAR jika contiguous
 */
bool_t pfs_extent_is_contiguous(pfs_extent_map_t *map,
                                tak_bertanda32_t log_start,
                                tak_bertanda32_t len)
{
    pfs_extent_result_t result;
    status_t status;

    if (map == NULL || len == 0) {
        return SALAH;
    }

    /* Cek extent pertama */
    status = pfs_extent_lookup(map, log_start, &result);
    if (status != STATUS_BERHASIL) {
        return SALAH;
    }

    /* Cek apakah extent cukup panjang */
    return (result.length >= len) ? BENAR : SALAH;
}

/*
 * pfs_extent_defrag
 * -----------------
 * Defragment extent map.
 *
 * Parameter:
 *   map - Pointer ke extent map
 *
 * Return: Status operasi
 */
status_t pfs_extent_defrag(pfs_extent_map_t *map)
{
    if (map == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Compact */
    pfs_extent_compact(map);

    /* Merge adjacent */
    pfs_extent_merge_adjacent(map);

    return STATUS_BERHASIL;
}

/*
 * pfs_extent_iterator_create
 * --------------------------
 * Buat iterator untuk extent map.
 *
 * Parameter:
 *   map - Pointer ke extent map
 *
 * Return: Pointer ke iterator, atau NULL jika gagal
 */
pfs_extent_iter_t *pfs_extent_iterator_create(pfs_extent_map_t *map)
{
    pfs_extent_iter_t *iter;

    if (map == NULL) {
        return NULL;
    }

    iter = (pfs_extent_iter_t *)kmalloc(sizeof(pfs_extent_iter_t));
    if (iter == NULL) {
        return NULL;
    }

    kernel_memset(iter, 0, sizeof(pfs_extent_iter_t));
    iter->magic = PFS_EXTENT_MAGIC;
    iter->map = map;
    iter->current_index = 0;
    iter->current_block = 0;

    return iter;
}

/*
 * pfs_extent_iterator_destroy
 * ---------------------------
 * Hancurkan iterator.
 *
 * Parameter:
 *   iter - Pointer ke iterator
 */
void pfs_extent_iterator_destroy(pfs_extent_iter_t *iter)
{
    if (iter == NULL) {
        return;
    }

    iter->magic = 0;
    kfree(iter);
}

/*
 * pfs_extent_iterator_next
 * ------------------------
 * Dapatkan extent berikutnya.
 *
 * Parameter:
 *   iter    - Pointer ke iterator
 *   extent  - Pointer ke hasil extent
 *
 * Return: Status operasi
 */
status_t pfs_extent_iterator_next(pfs_extent_iter_t *iter,
                                  pfs_extent_t *extent)
{
    if (iter == NULL || extent == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (iter->current_index >= iter->map->count) {
        return STATUS_KOSONG;
    }

    *extent = iter->map->extents[iter->current_index];
    iter->current_index++;

    return STATUS_BERHASIL;
}

/*
 * pfs_extent_iterator_reset
 * -------------------------
 * Reset iterator ke awal.
 *
 * Parameter:
 *   iter - Pointer ke iterator
 */
void pfs_extent_iterator_reset(pfs_extent_iter_t *iter)
{
    if (iter == NULL) {
        return;
    }

    iter->current_index = 0;
    iter->current_block = 0;
}
