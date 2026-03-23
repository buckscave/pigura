/*
 * PIGURA OS - HEAP.C
 * ------------------
 * Implementasi heap allocator untuk kernel.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk mengelola
 * heap memory dengan algoritma first-fit dan block allocation.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../kernel.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* Heap configuration */
#define HEAP_MIN_BLOCK_SIZE     16      /* Ukuran minimum blok */
#define HEAP_ALIGNMENT          8       /* Alignment alokasi */
#define HEAP_MAGIC              0x48454150  /* "HEAP" */
#define HEAP_FREE_MAGIC         0x46524545  /* "FREE" */
#define HEAP_USED_MAGIC         0x55534544  /* "USED" */

/* Heap flags */
#define HEAP_FLAG_FREE          0x00
#define HEAP_FLAG_USED          0x01

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* Heap block header */
typedef struct heap_block {
    tak_bertanda32_t magic;             /* Magic number untuk validasi */
    tak_bertanda32_t flags;             /* Flag status */
    ukuran_t size;                      /* Ukuran blok (tidak termasuk header) */
    struct heap_block *prev;            /* Pointer ke blok sebelumnya */
    struct heap_block *next;            /* Pointer ke blok berikutnya */
} heap_block_t;

/* Heap state */
typedef struct {
    void *start;                        /* Alamat awal heap */
    void *end;                          /* Alamat akhir heap */
    ukuran_t size;                      /* Total ukuran heap */
    ukuran_t used;                      /* Memori yang digunakan */
    ukuran_t free;                      /* Memori bebas */
    tak_bertanda32_t block_count;       /* Jumlah blok */
    tak_bertanda32_t alloc_count;       /* Jumlah alokasi aktif */
    heap_block_t *first_block;          /* Blok pertama */
    heap_block_t *last_block;           /* Blok terakhir */
    spinlock_t lock;                    /* Lock untuk thread safety */
} heap_state_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Kernel heap state */
static heap_state_t kernel_heap = {0};

/* Flag inisialisasi */
static bool_t heap_initialized = SALAH;

/* Statistik */
static struct {
    tak_bertanda64_t total_allocs;
    tak_bertanda64_t total_frees;
    tak_bertanda64_t alloc_failures;
    tak_bertanda64_t free_errors;
} heap_stats = {0};

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * block_is_valid
 * --------------
 * Validasi integritas blok heap.
 *
 * Parameter:
 *   block - Pointer ke blok
 *
 * Return: BENAR jika valid
 */
static inline bool_t block_is_valid(heap_block_t *block)
{
    if (block == NULL) {
        return SALAH;
    }

    if (block->magic != HEAP_MAGIC) {
        return SALAH;
    }

    if (block->size == 0) {
        return SALAH;
    }

    return BENAR;
}

/*
 * block_is_free
 * -------------
 * Cek apakah blok bebas.
 *
 * Parameter:
 *   block - Pointer ke blok
 *
 * Return: BENAR jika bebas
 */
static inline bool_t block_is_free(heap_block_t *block)
{
    return (block->flags == HEAP_FLAG_FREE);
}

/*
 * block_is_used
 * -------------
 * Cek apakah blok sedang digunakan.
 *
 * Parameter:
 *   block - Pointer ke blok
 *
 * Return: BENAR jika digunakan
 */
static inline bool_t block_is_used(heap_block_t *block)
{
    return (block->flags == HEAP_FLAG_USED);
}

/*
 * block_split
 * -----------
 * Pecah blok menjadi dua jika cukup besar.
 *
 * Parameter:
 *   block     - Blok yang akan dipecah
 *   new_size  - Ukuran blok pertama
 *
 * Return: BENAR jika berhasil dipecah
 */
static bool_t block_split(heap_block_t *block, ukuran_t new_size)
{
    heap_block_t *new_block;
    ukuran_t remaining;
    ukuran_t aligned_size;

    /* Align new_size */
    aligned_size = RATAKAN_ATAS(new_size, HEAP_ALIGNMENT);

    /* Hitung sisa ukuran */
    remaining = block->size - aligned_size - sizeof(heap_block_t);

    /* Cek apakah cukup untuk blok baru */
    if (remaining < HEAP_MIN_BLOCK_SIZE) {
        return SALAH;  /* Terlalu kecil untuk dipecah */
    }

    /* Buat blok baru */
    new_block = (heap_block_t *)((tak_bertanda8_t *)block +
                                 sizeof(heap_block_t) + aligned_size);

    new_block->magic = HEAP_MAGIC;
    new_block->flags = HEAP_FLAG_FREE;
    new_block->size = remaining;

    /* Update links */
    new_block->prev = block;
    new_block->next = block->next;

    if (block->next != NULL) {
        block->next->prev = new_block;
    }

    block->next = new_block;
    block->size = aligned_size;

    /* Update last block jika perlu */
    if (kernel_heap.last_block == block) {
        kernel_heap.last_block = new_block;
    }

    kernel_heap.block_count++;

    return BENAR;
}

/*
 * block_merge
 * -----------
 * Gabungkan blok dengan tetangganya yang bebas.
 *
 * Parameter:
 *   block - Blok yang akan digabung
 *
 * Return: Pointer ke blok hasil gabungan
 */
static heap_block_t *block_merge(heap_block_t *block)
{
    heap_block_t *next;
    heap_block_t *prev;

    /* Gabung dengan blok berikutnya */
    next = block->next;
    if (next != NULL && block_is_free(next)) {
        block->size += sizeof(heap_block_t) + next->size;
        block->next = next->next;

        if (next->next != NULL) {
            next->next->prev = block;
        }

        if (kernel_heap.last_block == next) {
            kernel_heap.last_block = block;
        }

        kernel_heap.block_count--;
    }

    /* Gabung dengan blok sebelumnya */
    prev = block->prev;
    if (prev != NULL && block_is_free(prev)) {
        prev->size += sizeof(heap_block_t) + block->size;
        prev->next = block->next;

        if (block->next != NULL) {
            block->next->prev = prev;
        }

        if (kernel_heap.last_block == block) {
            kernel_heap.last_block = prev;
        }

        kernel_heap.block_count--;
        block = prev;
    }

    return block;
}

/*
 * find_free_block
 * ---------------
 * Cari blok bebas yang cukup besar (first-fit).
 *
 * Parameter:
 *   size - Ukuran yang dibutuhkan
 *
 * Return: Pointer ke blok, atau NULL
 */
static heap_block_t *find_free_block(ukuran_t size)
{
    heap_block_t *block;

    block = kernel_heap.first_block;

    while (block != NULL) {
        if (block_is_free(block) && block->size >= size) {
            return block;
        }
        block = block->next;
    }

    return NULL;
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * heap_init
 * ---------
 * Inisialisasi heap kernel.
 *
 * Parameter:
 *   start_addr - Alamat awal heap
 *   size       - Ukuran heap
 *
 * Return: Status operasi
 */
status_t heap_init(void *start_addr, ukuran_t size)
{
    heap_block_t *initial_block;

    if (heap_initialized) {
        return STATUS_SUDAH_ADA;
    }

    if (start_addr == NULL || size < sizeof(heap_block_t) * 2) {
        return STATUS_PARAM_INVALID;
    }

    /* Align start dan size */
    start_addr = (void *)RATAKAN_ATAS((alamat_ptr_t)start_addr, HEAP_ALIGNMENT);
    size = RATAKAN_BAWAH(size, HEAP_ALIGNMENT);

    /* Inisialisasi state */
    kernel_heap.start = start_addr;
    kernel_heap.end = (void *)((tak_bertanda8_t *)start_addr + size);
    kernel_heap.size = size;
    kernel_heap.used = 0;
    kernel_heap.free = size - sizeof(heap_block_t);
    kernel_heap.block_count = 1;
    kernel_heap.alloc_count = 0;

    /* Buat blok awal yang mencakup seluruh heap */
    initial_block = (heap_block_t *)start_addr;
    initial_block->magic = HEAP_MAGIC;
    initial_block->flags = HEAP_FLAG_FREE;
    initial_block->size = size - sizeof(heap_block_t);
    initial_block->prev = NULL;
    initial_block->next = NULL;

    kernel_heap.first_block = initial_block;
    kernel_heap.last_block = initial_block;

    /* Init spinlock */
    spinlock_init(&kernel_heap.lock);

    heap_initialized = BENAR;

    kernel_printf("[HEAP] Initialized: %lu KB at 0x%08lX\n",
                  size / 1024, (alamat_ptr_t)start_addr);

    return STATUS_BERHASIL;
}

/*
 * kmalloc
 * -------
 * Alokasikan memori dari heap kernel.
 *
 * Parameter:
 *   size - Ukuran memori yang dibutuhkan
 *
 * Return: Pointer ke memori, atau NULL jika gagal
 */
void *kmalloc(ukuran_t size)
{
    heap_block_t *block;
    void *ptr;
    ukuran_t aligned_size;

    if (!heap_initialized || size == 0) {
        return NULL;
    }

    /* Align ukuran */
    aligned_size = RATAKAN_ATAS(size, HEAP_ALIGNMENT);

    /* Minimum size */
    if (aligned_size < HEAP_MIN_BLOCK_SIZE) {
        aligned_size = HEAP_MIN_BLOCK_SIZE;
    }

    spinlock_kunci(&kernel_heap.lock);

    /* Cari blok bebas */
    block = find_free_block(aligned_size);

    if (block == NULL) {
        heap_stats.alloc_failures++;
        spinlock_buka(&kernel_heap.lock);
        return NULL;
    }

    /* Pecah blok jika perlu */
    if (block->size >= aligned_size + sizeof(heap_block_t) + HEAP_MIN_BLOCK_SIZE) {
        block_split(block, aligned_size);
    }

    /* Tandai sebagai digunakan */
    block->flags = HEAP_FLAG_USED;

    /* Update statistik */
    kernel_heap.used += block->size;
    kernel_heap.free -= block->size;
    kernel_heap.alloc_count++;

    heap_stats.total_allocs++;

    /* Dapatkan pointer ke data */
    ptr = (void *)((tak_bertanda8_t *)block + sizeof(heap_block_t));

    spinlock_buka(&kernel_heap.lock);

    return ptr;
}

/*
 * kfree
 * -----
 * Bebaskan memori ke heap kernel.
 *
 * Parameter:
 *   ptr - Pointer ke memori yang akan dibebaskan
 */
void kfree(void *ptr)
{
    heap_block_t *block;

    if (!heap_initialized || ptr == NULL) {
        return;
    }

    /* Dapatkan block header */
    block = (heap_block_t *)((tak_bertanda8_t *)ptr - sizeof(heap_block_t));

    spinlock_kunci(&kernel_heap.lock);

    /* Validasi blok */
    if (!block_is_valid(block)) {
        heap_stats.free_errors++;
        spinlock_buka(&kernel_heap.lock);
        kernel_printf("[HEAP] WARNING: Invalid free at 0x%08lX\n",
                      (uintptr_t)ptr);
        return;
    }

    if (block_is_free(block)) {
        heap_stats.free_errors++;
        spinlock_buka(&kernel_heap.lock);
        kernel_printf("[HEAP] WARNING: Double free at 0x%08lX\n",
                      (uintptr_t)ptr);
        return;
    }

    /* Update statistik */
    kernel_heap.used -= block->size;
    kernel_heap.free += block->size;
    kernel_heap.alloc_count--;

    /* Tandai sebagai bebas */
    block->flags = HEAP_FLAG_FREE;

    /* Gabung dengan tetangga */
    block_merge(block);

    heap_stats.total_frees++;

    spinlock_buka(&kernel_heap.lock);
}

/*
 * krealloc
 * --------
 * Realokasi memori.
 *
 * Parameter:
 *   ptr  - Pointer ke memori lama
 *   size - Ukuran baru
 *
 * Return: Pointer ke memori baru, atau NULL jika gagal
 */
void *krealloc(void *ptr, ukuran_t size)
{
    void *new_ptr;
    heap_block_t *block;
    ukuran_t old_size;

    if (!heap_initialized) {
        return NULL;
    }

    if (ptr == NULL) {
        return kmalloc(size);
    }

    if (size == 0) {
        kfree(ptr);
        return NULL;
    }

    /* Dapatkan ukuran lama */
    block = (heap_block_t *)((tak_bertanda8_t *)ptr - sizeof(heap_block_t));

    if (!block_is_valid(block) || block_is_free(block)) {
        return NULL;
    }

    old_size = block->size;

    /* Jika ukuran baru lebih kecil atau sama, return ptr */
    if (size <= old_size) {
        return ptr;
    }

    /* Alokasi baru */
    new_ptr = kmalloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }

    /* Copy data lama */
    kernel_memcpy(new_ptr, ptr, old_size);

    /* Bebaskan lama */
    kfree(ptr);

    return new_ptr;
}

/*
 * kcalloc
 * -------
 * Alokasi memori dan inisialisasi dengan nol.
 *
 * Parameter:
 *   num  - Jumlah elemen
 *   size - Ukuran setiap elemen
 *
 * Return: Pointer ke memori, atau NULL
 */
void *kcalloc(ukuran_t num, ukuran_t size)
{
    void *ptr;
    ukuran_t total;

    if (!heap_initialized || num == 0 || size == 0) {
        return NULL;
    }

    /* Cek overflow */
    total = num * size;
    if (total / num != size) {
        return NULL;  /* Overflow */
    }

    ptr = kmalloc(total);
    if (ptr == NULL) {
        return NULL;
    }

    /* Clear */
    kernel_memset(ptr, 0, total);

    return ptr;
}

/*
 * heap_get_stats
 * --------------
 * Dapatkan statistik heap.
 *
 * Parameter:
 *   total       - Pointer untuk total size
 *   used        - Pointer untuk used size
 *   free        - Pointer untuk free size
 *   block_count - Pointer untuk block count
 */
void heap_get_stats(ukuran_t *total, ukuran_t *used, ukuran_t *free,
                    tak_bertanda32_t *block_count)
{
    if (total != NULL) {
        *total = kernel_heap.size;
    }

    if (used != NULL) {
        *used = kernel_heap.used;
    }

    if (free != NULL) {
        *free = kernel_heap.free;
    }

    if (block_count != NULL) {
        *block_count = kernel_heap.block_count;
    }
}

/*
 * heap_print_stats
 * ----------------
 * Print statistik heap.
 */
void heap_print_stats(void)
{
    kernel_printf("\n[HEAP] Kernel Heap Statistics:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Total size:     %lu bytes\n", kernel_heap.size);
    kernel_printf("  Used:           %lu bytes\n", kernel_heap.used);
    kernel_printf("  Free:           %lu bytes\n", kernel_heap.free);
    kernel_printf("  Block count:    %lu\n", kernel_heap.block_count);
    kernel_printf("  Active allocs:  %lu\n", kernel_heap.alloc_count);
    kernel_printf("  Total allocs:   %lu\n", heap_stats.total_allocs);
    kernel_printf("  Total frees:    %lu\n", heap_stats.total_frees);
    kernel_printf("  Alloc failures: %lu\n", heap_stats.alloc_failures);
    kernel_printf("  Free errors:    %lu\n", heap_stats.free_errors);
    kernel_printf("========================================\n");
}

/*
 * heap_validate
 * -------------
 * Validasi integritas heap.
 *
 * Return: BENAR jika valid
 */
bool_t heap_validate(void)
{
    heap_block_t *block;
    ukuran_t total_size;
    tak_bertanda32_t block_count;

    if (!heap_initialized) {
        return SALAH;
    }

    spinlock_kunci(&kernel_heap.lock);

    block = kernel_heap.first_block;
    total_size = 0;
    block_count = 0;

    while (block != NULL) {
        if (!block_is_valid(block)) {
            spinlock_buka(&kernel_heap.lock);
            kernel_printf("[HEAP] Invalid block found!\n");
            return SALAH;
        }

        total_size += sizeof(heap_block_t) + block->size;
        block_count++;

        block = block->next;
    }

    if (total_size != kernel_heap.size) {
        spinlock_buka(&kernel_heap.lock);
        kernel_printf("[HEAP] Size mismatch: %lu != %lu\n",
                      total_size, kernel_heap.size);
        return SALAH;
    }

    if (block_count != kernel_heap.block_count) {
        spinlock_buka(&kernel_heap.lock);
        kernel_printf("[HEAP] Block count mismatch!\n");
        return SALAH;
    }

    spinlock_buka(&kernel_heap.lock);

    return BENAR;
}
