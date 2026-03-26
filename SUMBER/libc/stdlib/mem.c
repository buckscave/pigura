/*
 * PIGURA LIBC - STDLIB/MEM.C
 * ===========================
 * Implementasi fungsi alokasi memori dinamis.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Implementasi menggunakan simple first-fit allocator
 * dengan linked list block header.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* ============================================================
 * KONFIGURASI ALLOCATOR
 * ============================================================
 */

/* Alignment minimum untuk alokasi */
#define MEM_ALIGN       16
#define MEM_ALIGN_MASK  (MEM_ALIGN - 1)

/* Ukuran minimum block (termasuk header) */
#define MIN_BLOCK_SIZE  64

/* Magic number untuk validasi block */
#define BLOCK_MAGIC_FREE  0xFEEDFACE
#define BLOCK_MAGIC_USED  0xDEADBEEF

/* Bit flag untuk status block */
#define BLOCK_FLAG_FREE   0x01
#define BLOCK_FLAG_MMAP    0x02

/* ============================================================
 * STRUKTUR BLOCK HEADER
 * ============================================================
 * Header yang ditempatkan di awal setiap block memori.
 * User pointer adalah (header + 1).
 */
typedef struct mem_block {
    unsigned int magic;         /* Magic untuk validasi */
    unsigned int flags;         /* Flag status */
    size_t size;                /* Ukuran data (tanpa header) */
    struct mem_block *next;     /* Block berikutnya */
    struct mem_block *prev;     /* Block sebelumnya */
} mem_block_t;

/* Ukuran header */
#define HEADER_SIZE  sizeof(mem_block_t)

/* Makro untuk konversi antara block dan user pointer */
#define BLOCK_TO_PTR(b)  ((void *)((char *)(b) + HEADER_SIZE))
#define PTR_TO_BLOCK(p)  ((mem_block_t *)((char *)(p) - HEADER_SIZE))

/* Makro untuk align size (local version) */
#define MEM_ALIGN_UP(s) \
    (((s) + MEM_ALIGN_MASK) & ~MEM_ALIGN_MASK)

/* ============================================================
 * VARIABEL GLOBAL ALLOCATOR
 * ============================================================
 */

/* Head dari free list */
static mem_block_t *free_list = NULL;

/* Base heap pointer (dari kernel) */
static void *heap_base = NULL;

/* Ukuran heap saat ini */
static size_t heap_size = 0;

/* Total memori yang dialokasikan */
static size_t total_allocated = 0;

/* Flag inisialisasi */
static int mem_initialized = 0;

/* ============================================================
 * DEKLARASI FUNGSI INTERNAL
 * ============================================================
 */

/* Fungsi syscall untuk meminta memori dari kernel */
extern void *__syscall_brk(void *addr);
extern void *__syscall_mmap(void *addr, size_t len, int prot,
                            int flags, int fd, long offset);

/* Fungsi internal */
static void mem_init(void);
static mem_block_t *find_free_block(size_t size);
static mem_block_t *split_block(mem_block_t *block, size_t size);
static void merge_blocks(mem_block_t *block);
static int grow_heap(size_t min_size);

/* ============================================================
 * FUNGSI INISIALISASI
 * ============================================================
 */

/*
 * mem_init - Inisialisasi memory allocator
 *
 * Dipanggil sekali saat malloc pertama kali dipanggil.
 */
static void mem_init(void) {
    if (mem_initialized) {
        return;
    }

    /* Dapatkan heap awal dari kernel (64KB) */
    heap_base = __syscall_brk(NULL);
    if (heap_base == (void *)-1) {
        heap_base = NULL;
        return;
    }

    /* Request memori awal */
    void *new_brk = __syscall_brk((char *)heap_base + 65536);
    if (new_brk == (void *)-1) {
        return;
    }

    heap_size = 65536;

    /* Buat block free awal */
    free_list = (mem_block_t *)heap_base;
    free_list->magic = BLOCK_MAGIC_FREE;
    free_list->flags = BLOCK_FLAG_FREE;
    free_list->size = heap_size - HEADER_SIZE;
    free_list->next = NULL;
    free_list->prev = NULL;

    mem_initialized = 1;
}

/*
 * __stdlib_init - Inisialisasi stdlib
 *
 * Dipanggil dari startup code.
 */
void __stdlib_init(void) {
    mem_init();
}

/*
 * __stdlib_cleanup - Cleanup stdlib
 *
 * Dipanggil saat program exit.
 */
void __stdlib_cleanup(void) {
    /* Tidak ada yang perlu dibersihkan untuk simple allocator */
}

/* ============================================================
 * FUNGSI ALOKASI
 * ============================================================
 */

/*
 * malloc - Alokasikan memori
 *
 * Parameter:
 *   size - Ukuran memori yang dialokasikan
 *
 * Return: Pointer ke memori, atau NULL jika gagal
 */
void *malloc(size_t size) {
    mem_block_t *block;
    size_t aligned_size;

    /* Handle size = 0 */
    if (size == 0) {
        return NULL;
    }

    /* Inisialisasi jika belum */
    if (!mem_initialized) {
        mem_init();
        if (!mem_initialized) {
            errno = ENOMEM;
            return NULL;
        }
    }

    /* Align size */
    aligned_size = MEM_ALIGN_UP(size);
    if (aligned_size + HEADER_SIZE < MIN_BLOCK_SIZE) {
        aligned_size = MIN_BLOCK_SIZE - HEADER_SIZE;
    }

    /* Cari block yang cukup */
    block = find_free_block(aligned_size);

    if (block == NULL) {
        /* Perlu memori baru */
        if (!grow_heap(aligned_size + HEADER_SIZE)) {
            errno = ENOMEM;
            return NULL;
        }
        block = find_free_block(aligned_size);
        if (block == NULL) {
            errno = ENOMEM;
            return NULL;
        }
    }

    /* Split jika block terlalu besar */
    if (block->size >= aligned_size + MIN_BLOCK_SIZE) {
        block = split_block(block, aligned_size);
    }

    /* Mark sebagai used */
    block->magic = BLOCK_MAGIC_USED;
    block->flags &= ~BLOCK_FLAG_FREE;

    /* Hapus dari free list */
    if (block->prev != NULL) {
        block->prev->next = block->next;
    } else {
        free_list = block->next;
    }
    if (block->next != NULL) {
        block->next->prev = block->prev;
    }

    total_allocated += block->size;

    return BLOCK_TO_PTR(block);
}

/*
 * calloc - Alokasikan dan inisialisasi memori
 *
 * Parameter:
 *   nmemb - Jumlah elemen
 *   size  - Ukuran setiap elemen
 *
 * Return: Pointer ke memori yang sudah di-zero, atau NULL
 */
void *calloc(size_t nmemb, size_t size) {
    size_t total;
    void *ptr;

    /* Cek overflow */
    if (nmemb != 0 && size > ((size_t)-1) / nmemb) {
        errno = ENOMEM;
        return NULL;
    }

    total = nmemb * size;

    /* Handle total = 0 */
    if (total == 0) {
        return NULL;
    }

    ptr = malloc(total);
    if (ptr == NULL) {
        return NULL;
    }

    /* Zero memory */
    memset(ptr, 0, total);

    return ptr;
}

/*
 * realloc - Realokasikan memori
 *
 * Parameter:
 *   ptr  - Pointer ke memori lama (boleh NULL)
 *   size - Ukuran baru
 *
 * Return: Pointer ke memori baru, atau NULL jika gagal
 *
 * Catatan: Jika gagal, memori lama tidak di-free.
 */
void *realloc(void *ptr, size_t size) {
    mem_block_t *block;
    void *new_ptr;
    size_t copy_size;

    /* ptr NULL = malloc */
    if (ptr == NULL) {
        return malloc(size);
    }

    /* size 0 = free */
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    block = PTR_TO_BLOCK(ptr);

    /* Validasi block */
    if (block->magic != BLOCK_MAGIC_USED) {
        errno = EINVAL;
        return NULL;
    }

    /* Jika ukuran baru lebih kecil atau sama */
    if (size <= block->size) {
        /* Bisa di-split untuk efisiensi */
        size_t aligned_size = MEM_ALIGN_UP(size);
        if (block->size >= aligned_size + MIN_BLOCK_SIZE) {
            /* Split block */
            mem_block_t *new_block;
            size_t remaining;

            remaining = block->size - aligned_size - HEADER_SIZE;

            new_block = (mem_block_t *)((char *)block +
                         HEADER_SIZE + aligned_size);
            new_block->magic = BLOCK_MAGIC_FREE;
            new_block->flags = BLOCK_FLAG_FREE;
            new_block->size = remaining;

            /* Tambahkan ke free list */
            new_block->next = free_list;
            new_block->prev = NULL;
            if (free_list != NULL) {
                free_list->prev = new_block;
            }
            free_list = new_block;

            block->size = aligned_size;
        }
        return ptr;
    }

    /* Perlu memori baru */
    new_ptr = malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }

    /* Copy data lama */
    copy_size = block->size;
    memcpy(new_ptr, ptr, copy_size);

    /* Free memori lama */
    free(ptr);

    return new_ptr;
}

/*
 * free - Bebaskan memori
 *
 * Parameter:
 *   ptr - Pointer ke memori (boleh NULL)
 */
void free(void *ptr) {
    mem_block_t *block;

    /* ptr NULL = tidak ada yang dilakukan */
    if (ptr == NULL) {
        return;
    }

    block = PTR_TO_BLOCK(ptr);

    /* Validasi block */
    if (block->magic != BLOCK_MAGIC_USED) {
        /* Double free atau korupsi */
        return;
    }

    /* Update statistik */
    total_allocated -= block->size;

    /* Mark sebagai free */
    block->magic = BLOCK_MAGIC_FREE;
    block->flags |= BLOCK_FLAG_FREE;

    /* Tambahkan ke free list */
    block->next = free_list;
    block->prev = NULL;
    if (free_list != NULL) {
        free_list->prev = block;
    }
    free_list = block;

    /* Merge dengan block tetangga jika free */
    merge_blocks(block);
}

/* ============================================================
 * FUNGSI ALOKASI ALIGNED
 * ============================================================
 */

/*
 * aligned_alloc - Alokasikan memori dengan alignment
 *
 * Parameter:
 *   alignment - Alignment yang diminta
 *   size      - Ukuran memori
 *
 * Return: Pointer ke memori yang aligned, atau NULL
 */
void *aligned_alloc(size_t alignment, size_t size) {
    void *ptr;
    size_t aligned_size;
    void **aligned_ptr;

    /* Validasi parameter */
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        errno = EINVAL;
        return NULL;
    }

    /* size harus kelipatan alignment */
    if ((size % alignment) != 0) {
        errno = EINVAL;
        return NULL;
    }

    /* Alokasi dengan ruang untuk alignment */
    aligned_size = size + alignment + sizeof(void *);
    ptr = malloc(aligned_size);
    if (ptr == NULL) {
        return NULL;
    }

    /* Align pointer */
    aligned_ptr = (void **)(((size_t)ptr + sizeof(void *) +
                alignment - 1) & ~(alignment - 1));

    /* Simpan original pointer sebelum aligned pointer */
    aligned_ptr[-1] = ptr;

    return aligned_ptr;
}

/*
 * memalign - Alokasikan memori dengan alignment (POSIX)
 *
 * Parameter:
 *   alignment - Alignment yang diminta
 *   size      - Ukuran memori
 *
 * Return: Pointer ke memori yang aligned, atau NULL
 */
void *memalign(size_t alignment, size_t size) {
    void *ptr;
    size_t aligned_size;
    void **aligned_ptr;

    /* Validasi parameter */
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        errno = EINVAL;
        return NULL;
    }

    /* Alokasi dengan ruang untuk alignment */
    aligned_size = size + alignment + sizeof(void *);
    ptr = malloc(aligned_size);
    if (ptr == NULL) {
        return NULL;
    }

    /* Align pointer */
    aligned_ptr = (void **)(((size_t)ptr + sizeof(void *) +
                alignment - 1) & ~(alignment - 1));

    /* Simpan original pointer */
    aligned_ptr[-1] = ptr;

    return aligned_ptr;
}

/*
 * posix_memalign - Alokasikan memori aligned (POSIX)
 *
 * Parameter:
 *   memptr    - Pointer untuk menyimpan hasil
 *   alignment - Alignment yang diminta
 *   size      - Ukuran memori
 *
 * Return: 0 jika berhasil, error code jika gagal
 */
int posix_memalign(void **memptr, size_t alignment, size_t size) {
    void *ptr;

    /* Validasi alignment (harus kelipatan sizeof(void *)) */
    if (alignment < sizeof(void *) ||
        (alignment & (alignment - 1)) != 0) {
        return EINVAL;
    }

    ptr = memalign(alignment, size);
    if (ptr == NULL) {
        return ENOMEM;
    }

    *memptr = ptr;
    return 0;
}

/*
 * valloc - Alokasikan memori page-aligned (BSD)
 *
 * Parameter:
 *   size - Ukuran memori
 *
 * Return: Pointer ke memori yang page-aligned, atau NULL
 */
void *valloc(size_t size) {
    /* Page size default 4KB */
    return memalign(4096, size);
}

/* ============================================================
 * FUNGSI INTERNAL ALLOCATOR
 * ============================================================
 */

/*
 * find_free_block - Cari block free yang cukup besar
 *
 * Parameter:
 *   size - Ukuran minimum yang dibutuhkan
 *
 * Return: Pointer ke block, atau NULL jika tidak ada
 *
 * Menggunakan first-fit algorithm.
 */
static mem_block_t *find_free_block(size_t size) {
    mem_block_t *block;

    block = free_list;
    while (block != NULL) {
        if ((block->flags & BLOCK_FLAG_FREE) &&
            block->size >= size) {
            return block;
        }
        block = block->next;
    }

    return NULL;
}

/*
 * split_block - Bagi block menjadi dua
 *
 * Parameter:
 *   block - Block yang dibagi
 *   size  - Ukuran untuk block pertama
 *
 * Return: Pointer ke block pertama
 */
static mem_block_t *split_block(mem_block_t *block, size_t size) {
    mem_block_t *new_block;
    size_t remaining;

    remaining = block->size - size - HEADER_SIZE;

    /* Hanya split jika sisa cukup besar */
    if (remaining + HEADER_SIZE < MIN_BLOCK_SIZE) {
        return block;
    }

    /* Buat block baru */
    new_block = (mem_block_t *)((char *)block +
                 HEADER_SIZE + size);
    new_block->magic = BLOCK_MAGIC_FREE;
    new_block->flags = BLOCK_FLAG_FREE;
    new_block->size = remaining;
    new_block->next = block->next;
    new_block->prev = block;

    /* Update links */
    if (block->next != NULL) {
        block->next->prev = new_block;
    }
    block->next = new_block;
    block->size = size;

    return block;
}

/*
 * merge_blocks - Gabung block dengan tetangga yang free
 *
 * Parameter:
 *   block - Block yang akan digabung
 */
static void merge_blocks(mem_block_t *block) {
    mem_block_t *next_block;
    mem_block_t *prev_block;

    /* Cek block berikutnya */
    next_block = block->next;
    if (next_block != NULL &&
        (next_block->flags & BLOCK_FLAG_FREE) &&
        (char *)block + HEADER_SIZE + block->size ==
        (char *)next_block) {

        /* Gabung dengan block berikutnya */
        block->size += HEADER_SIZE + next_block->size;
        block->next = next_block->next;
        if (next_block->next != NULL) {
            next_block->next->prev = block;
        }
    }

    /* Cek block sebelumnya */
    prev_block = block->prev;
    if (prev_block != NULL &&
        (prev_block->flags & BLOCK_FLAG_FREE) &&
        (char *)prev_block + HEADER_SIZE + prev_block->size ==
        (char *)block) {

        /* Gabung dengan block sebelumnya */
        prev_block->size += HEADER_SIZE + block->size;
        prev_block->next = block->next;
        if (block->next != NULL) {
            block->next->prev = prev_block;
        }
    }
}

/*
 * grow_heap - Perbesar heap
 *
 * Parameter:
 *   min_size - Ukuran minimum yang dibutuhkan
 *
 * Return: 1 jika berhasil, 0 jika gagal
 */
static int grow_heap(size_t min_size) {
    size_t grow_size;
    void *new_brk;
    mem_block_t *new_block;

    /* Hitung ukuran growth (minimal 64KB) */
    grow_size = min_size + HEADER_SIZE;
    if (grow_size < 65536) {
        grow_size = 65536;
    }

    /* Align ke page boundary */
    grow_size = (grow_size + 4095) & ~4095;

    /* Request memori dari kernel */
    new_brk = __syscall_brk((char *)heap_base +
                            heap_size + grow_size);
    if (new_brk == (void *)-1) {
        return 0;
    }

    /* Buat block baru di akhir heap */
    new_block = (mem_block_t *)((char *)heap_base +
                 heap_size);
    new_block->magic = BLOCK_MAGIC_FREE;
    new_block->flags = BLOCK_FLAG_FREE;
    new_block->size = grow_size - HEADER_SIZE;
    new_block->next = free_list;
    new_block->prev = NULL;

    if (free_list != NULL) {
        free_list->prev = new_block;
    }
    free_list = new_block;

    /* Update heap size */
    heap_size += grow_size;

    /* Coba merge dengan block sebelumnya */
    merge_blocks(new_block);

    return 1;
}

/* ============================================================
 * FUNGSI UTILITAS
 * ============================================================
 */

/*
 * malloc_usable_size - Dapatkan ukuran yang dapat digunakan
 *
 * Parameter:
 *   ptr - Pointer dari malloc/calloc/realloc
 *
 * Return: Ukuran block, atau 0 jika ptr invalid
 */
size_t malloc_usable_size(void *ptr) {
    mem_block_t *block;

    if (ptr == NULL) {
        return 0;
    }

    block = PTR_TO_BLOCK(ptr);

    if (block->magic != BLOCK_MAGIC_USED) {
        return 0;
    }

    return block->size;
}

/*
 * malloc_stats - Cetak statistik memori (debug)
 */
void malloc_stats(void) {
    /* Placeholder - implementasi tergantung output */
}

/*
 * malloc_trim - Return memori ke sistem
 *
 * Parameter:
 *   pad - Padding yang dipertahankan
 *
 * Return: 1 jika berhasil, 0 jika tidak
 */
int malloc_trim(size_t pad) {
    /* Simple allocator tidak mendukung trim */
    (void)pad;
    return 0;
}
