/*
 * PIGURA OS - ALLOCATOR.C
 * -----------------------
 * Implementasi slab allocator untuk kernel objects.
 *
 * Berkas ini berisi implementasi allocator yang efisien untuk
 * alokasi objek-objek kernel dengan ukuran tetap menggunakan
 * teknik slab allocation.
 *
 * Versi: 1.1
 * Tanggal: 2025
 */

#include "../kernel.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* Ukuran minimum dan maksimum objek */
#define SLAB_OBJ_MIN            16
#define SLAB_OBJ_MAX            4096

/* Ukuran slab (dalam halaman) */
#define SLAB_SIZE_PAGES         1

/* Magic number untuk validasi */
#define SLAB_MAGIC              0x534C4142  /* "SLAB" */
#define CACHE_MAGIC             0x43414348  /* "CACH" */

/* Flag slab */
#define SLAB_FLAG_EMPTY         0x00
#define SLAB_FLAG_PARTIAL       0x01
#define SLAB_FLAG_FULL          0x02

/* Jumlah cache pre-defined */
#define CACHE_BUILTIN_COUNT     8

/* Nama maksimum cache */
#define CACHE_NAME_LEN          32

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* Slab descriptor - diletakkan di awal slab */
typedef struct slab {
    tak_bertanda32_t magic;         /* Magic number */
    tak_bertanda32_t flags;         /* Status slab */
    struct slab *next;              /* Slab berikutnya dalam list */
    struct slab *prev;              /* Slab sebelumnya */
    tak_bertanda32_t obj_count;     /* Jumlah objek dalam slab */
    tak_bertanda32_t obj_used;      /* Jumlah objek terpakai */
    tak_bertanda32_t obj_size;      /* Ukuran objek */
    void *free_list;                /* List objek bebas */
    tak_bertanda8_t data[0];        /* Data objek (flexible array) */
} slab_t;

/* Cache descriptor */
typedef struct kmem_cache {
    tak_bertanda32_t magic;         /* Magic number */
    char nama[CACHE_NAME_LEN];      /* Nama cache */
    tak_bertanda32_t obj_size;      /* Ukuran objek */
    tak_bertanda32_t obj_per_slab;  /* Objek per slab */
    tak_bertanda32_t slab_size;     /* Ukuran slab */
    tak_bertanda32_t flags;         /* Flag cache */

    /* List slab */
    slab_t *slabs_empty;            /* Slab kosong */
    slab_t *slabs_partial;          /* Slab sebagian terisi */
    slab_t *slabs_full;             /* Slab penuh */

    /* Statistik */
    tak_bertanda32_t slab_count;    /* Total slab */
    tak_bertanda32_t obj_total;     /* Total objek */
    tak_bertanda32_t obj_active;    /* Objek aktif */
    tak_bertanda64_t alloc_count;   /* Total alokasi */
    tak_bertanda64_t free_count;    /* Total free */

    /* Lock */
    spinlock_t lock;

    /* Constructor dan destructor */
    void (*ctor)(void *obj);
    void (*dtor)(void *obj);
} kmem_cache_t;

/* Slab list untuk berbagai ukuran */
typedef struct {
    kmem_cache_t *cache;
    tak_bertanda32_t size;
} size_cache_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Built-in caches untuk ukuran umum */
static kmem_cache_t builtin_caches[CACHE_BUILTIN_COUNT];

/* Size-to-cache mapping */
static size_cache_t size_caches[] = {
    { NULL, 16 },
    { NULL, 32 },
    { NULL, 64 },
    { NULL, 128 },
    { NULL, 256 },
    { NULL, 512 },
    { NULL, 1024 },
    { NULL, 2048 },
    { NULL, 0 }  /* Sentinel */
};

/* General purpose cache */
static kmem_cache_t *general_cache = NULL;

/* Status */
static bool_t allocator_initialized = SALAH;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * slab_create
 * -----------
 * Buat slab baru untuk cache.
 *
 * Parameter:
 *   cache - Pointer ke cache
 *
 * Return: Pointer ke slab baru, atau NULL jika gagal
 */
static slab_t *slab_create(kmem_cache_t *cache)
{
    slab_t *slab;
    void *mem;
    tak_bertanda8_t *obj;
    tak_bertanda32_t i;
    tak_bertanda32_t offset;

    /* Alokasikan memori untuk slab */
    mem = (void *)pmm_alloc_pages(SLAB_SIZE_PAGES);
    if (mem == NULL) {
        return NULL;
    }

    /* Clear memori */
    kernel_memset(mem, 0, cache->slab_size);

    slab = (slab_t *)mem;
    slab->magic = SLAB_MAGIC;
    slab->flags = SLAB_FLAG_EMPTY;
    slab->next = NULL;
    slab->prev = NULL;
    slab->obj_count = cache->obj_per_slab;
    slab->obj_used = 0;
    slab->obj_size = cache->obj_size;

    /* Hitung offset data (setelah slab header, aligned) */
    offset = sizeof(slab_t);
    offset = RATAKAN_ATAS(offset, 8);

    /* Inisialisasi free list */
    slab->free_list = NULL;
    obj = (tak_bertanda8_t *)mem + offset;

    for (i = 0; i < cache->obj_per_slab; i++) {
        void **obj_ptr = (void **)(obj + (i * cache->obj_size));
        *obj_ptr = slab->free_list;
        slab->free_list = obj_ptr;
    }

    return slab;
}

/*
 * slab_destroy
 * ------------
 * Hancurkan slab.
 *
 * Parameter:
 *   cache - Pointer ke cache
 *   slab  - Pointer ke slab
 */
static void slab_destroy(kmem_cache_t *cache, slab_t *slab)
{
    if (slab == NULL) {
        return;
    }

    /* Panggil destructor untuk setiap objek */
    if (cache->dtor != NULL && slab->obj_used > 0) {
        tak_bertanda32_t i;
        tak_bertanda32_t offset;
        tak_bertanda8_t *obj;

        offset = sizeof(slab_t);
        offset = RATAKAN_ATAS(offset, 8);
        obj = (tak_bertanda8_t *)slab + offset;

        for (i = 0; i < slab->obj_count; i++) {
            void *obj_ptr = obj + (i * cache->obj_size);
            /* Cek apakah objek sedang digunakan */
            /* TODO: Implement proper tracking */
            cache->dtor(obj_ptr);
        }
    }

    /* Bebaskan memori slab */
    pmm_free_pages((alamat_fisik_t)slab, SLAB_SIZE_PAGES);
}

/*
 * slab_alloc_obj
 * --------------
 * Alokasikan objek dari slab.
 *
 * Parameter:
 *   slab - Pointer ke slab
 *
 * Return: Pointer ke objek, atau NULL jika slab penuh
 */
static void *slab_alloc_obj(slab_t *slab)
{
    void *obj;

    if (slab == NULL || slab->free_list == NULL) {
        return NULL;
    }

    /* Ambil dari free list */
    obj = slab->free_list;
    slab->free_list = *(void **)obj;

    slab->obj_used++;

    /* Update status slab */
    if (slab->obj_used == slab->obj_count) {
        slab->flags = SLAB_FLAG_FULL;
    } else {
        slab->flags = SLAB_FLAG_PARTIAL;
    }

    return obj;
}

/*
 * slab_free_obj
 * -------------
 * Bebaskan objek ke slab.
 *
 * Parameter:
 *   slab - Pointer ke slab
 *   obj  - Pointer ke objek
 */
static void slab_free_obj(slab_t *slab, void *obj)
{
    if (slab == NULL || obj == NULL) {
        return;
    }

    /* Tambahkan ke free list */
    *(void **)obj = slab->free_list;
    slab->free_list = obj;

    slab->obj_used--;

    /* Update status slab */
    if (slab->obj_used == 0) {
        slab->flags = SLAB_FLAG_EMPTY;
    } else {
        slab->flags = SLAB_FLAG_PARTIAL;
    }
}

/*
 * slab_add_to_list
 * ----------------
 * Tambahkan slab ke list.
 *
 * Parameter:
 *   list - Pointer ke pointer list
 *   slab - Pointer ke slab
 */
static void slab_add_to_list(slab_t **list, slab_t *slab)
{
    if (list == NULL || slab == NULL) {
        return;
    }

    slab->next = *list;
    slab->prev = NULL;

    if (*list != NULL) {
        (*list)->prev = slab;
    }

    *list = slab;
}

/*
 * slab_remove_from_list
 * ---------------------
 * Hapus slab dari list.
 *
 * Parameter:
 *   list - Pointer ke pointer list
 *   slab - Pointer ke slab
 */
static void slab_remove_from_list(slab_t **list, slab_t *slab)
{
    if (list == NULL || slab == NULL) {
        return;
    }

    if (slab->prev != NULL) {
        slab->prev->next = slab->next;
    } else {
        *list = slab->next;
    }

    if (slab->next != NULL) {
        slab->next->prev = slab->prev;
    }

    slab->prev = NULL;
    slab->next = NULL;
}

/*
 * find_cache_for_size
 * -------------------
 * Cari cache yang sesuai untuk ukuran.
 *
 * Parameter:
 *   size - Ukuran yang dibutuhkan
 *
 * Return: Pointer ke cache, atau NULL
 */
static kmem_cache_t *find_cache_for_size(tak_bertanda32_t size)
{
    tak_bertanda32_t i;

    for (i = 0; size_caches[i].size != 0; i++) {
        if (size <= size_caches[i].size && size_caches[i].cache != NULL) {
            return size_caches[i].cache;
        }
    }

    return NULL;
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * kmem_cache_create
 * -----------------
 * Buat cache baru.
 *
 * Parameter:
 *   nama     - Nama cache
 *   size     - Ukuran objek
 *   flags    - Flag cache
 *   ctor     - Constructor (boleh NULL)
 *   dtor     - Destructor (boleh NULL)
 *
 * Return: Pointer ke cache, atau NULL jika gagal
 */
kmem_cache_t *kmem_cache_create(const char *nama, tak_bertanda32_t size,
                                tak_bertanda32_t flags,
                                void (*ctor)(void *),
                                void (*dtor)(void *))
{
    kmem_cache_t *cache;
    tak_bertanda32_t slab_data_size;

    if (allocator_initialized == SALAH || nama == NULL || size == 0) {
        return NULL;
    }

    if (size < SLAB_OBJ_MIN) {
        size = SLAB_OBJ_MIN;
    }

    if (size > SLAB_OBJ_MAX) {
        return NULL;  /* Terlalu besar untuk slab allocator */
    }

    /* Align size */
    size = RATAKAN_ATAS(size, 8);

    /* Alokasikan cache descriptor */
    cache = kmalloc(sizeof(kmem_cache_t));
    if (cache == NULL) {
        return NULL;
    }

    /* Inisialisasi cache */
    cache->magic = CACHE_MAGIC;
    kernel_strncpy(cache->nama, nama, CACHE_NAME_LEN - 1);
    cache->nama[CACHE_NAME_LEN - 1] = '\0';
    cache->obj_size = size;
    cache->flags = flags;
    cache->ctor = ctor;
    cache->dtor = dtor;

    /* Hitung ukuran slab dan objek per slab */
    cache->slab_size = SLAB_SIZE_PAGES * UKURAN_HALAMAN;
    slab_data_size = cache->slab_size - sizeof(slab_t);
    cache->obj_per_slab = slab_data_size / size;

    /* Inisialisasi list */
    cache->slabs_empty = NULL;
    cache->slabs_partial = NULL;
    cache->slabs_full = NULL;

    /* Reset statistik */
    cache->slab_count = 0;
    cache->obj_total = 0;
    cache->obj_active = 0;
    cache->alloc_count = 0;
    cache->free_count = 0;

    /* Init lock */
    spinlock_init(&cache->lock);

    kernel_printf("[SLAB] Created cache '%s' (obj_size=%lu, per_slab=%lu)\n",
                  nama, cache->obj_size, cache->obj_per_slab);

    return cache;
}

/*
 * kmem_cache_destroy
 * ------------------
 * Hancurkan cache.
 *
 * Parameter:
 *   cache - Pointer ke cache
 *
 * Return: Status operasi
 */
status_t kmem_cache_destroy(kmem_cache_t *cache)
{
    slab_t *slab;
    slab_t *next;

    if (cache == NULL || cache->magic != CACHE_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    spinlock_kunci(&cache->lock);

    /* Hancurkan semua slab */
    slab = cache->slabs_empty;
    while (slab != NULL) {
        next = slab->next;
        slab_destroy(cache, slab);
        slab = next;
    }

    slab = cache->slabs_partial;
    while (slab != NULL) {
        next = slab->next;
        slab_destroy(cache, slab);
        slab = next;
    }

    slab = cache->slabs_full;
    while (slab != NULL) {
        next = slab->next;
        slab_destroy(cache, slab);
        slab = next;
    }

    spinlock_buka(&cache->lock);

    /* Bebaskan cache descriptor */
    cache->magic = 0;
    kfree(cache);

    return STATUS_BERHASIL;
}

/*
 * kmem_cache_alloc
 * ----------------
 * Alokasikan objek dari cache.
 *
 * Parameter:
 *   cache - Pointer ke cache
 *
 * Return: Pointer ke objek, atau NULL jika gagal
 */
void *kmem_cache_alloc(kmem_cache_t *cache)
{
    slab_t *slab;
    void *obj;

    if (cache == NULL || cache->magic != CACHE_MAGIC) {
        return NULL;
    }

    spinlock_kunci(&cache->lock);

    /* Coba cari dari partial slabs dulu */
    if (cache->slabs_partial != NULL) {
        slab = cache->slabs_partial;
        obj = slab_alloc_obj(slab);

        if (slab->flags == SLAB_FLAG_FULL) {
            /* Pindahkan ke full list */
            slab_remove_from_list(&cache->slabs_partial, slab);
            slab_add_to_list(&cache->slabs_full, slab);
        }
    }
    /* Coba dari empty slabs */
    else if (cache->slabs_empty != NULL) {
        slab = cache->slabs_empty;
        obj = slab_alloc_obj(slab);

        /* Pindahkan ke partial list */
        slab_remove_from_list(&cache->slabs_empty, slab);
        slab_add_to_list(&cache->slabs_partial, slab);
    }
    /* Buat slab baru */
    else {
        slab = slab_create(cache);
        if (slab == NULL) {
            spinlock_buka(&cache->lock);
            return NULL;
        }

        cache->slab_count++;
        cache->obj_total += cache->obj_per_slab;

        obj = slab_alloc_obj(slab);
        slab_add_to_list(&cache->slabs_partial, slab);
    }

    cache->obj_active++;
    cache->alloc_count++;

    spinlock_buka(&cache->lock);

    /* Panggil constructor */
    if (cache->ctor != NULL) {
        cache->ctor(obj);
    }

    return obj;
}

/*
 * kmem_cache_free
 * ---------------
 * Bebaskan objek ke cache.
 *
 * Parameter:
 *   cache - Pointer ke cache
 *   obj   - Pointer ke objek
 */
void kmem_cache_free(kmem_cache_t *cache, void *obj)
{
    slab_t *slab;
    tak_bertanda32_t offset;
    tak_bertanda32_t slab_data_start;

    if (cache == NULL || cache->magic != CACHE_MAGIC || obj == NULL) {
        return;
    }

    /* Cari slab yang berisi objek ini */
    /* Objek berada dalam range slab */
    slab = (slab_t *)((uintptr_t)obj & ~(UKURAN_HALAMAN - 1));

    if (slab->magic != SLAB_MAGIC) {
        kernel_printf("[SLAB] ERROR: Invalid slab for object %p\n", obj);
        return;
    }

    /* Panggil destructor */
    if (cache->dtor != NULL) {
        cache->dtor(obj);
    }

    spinlock_kunci(&cache->lock);

    /* Simpan status lama */
    tak_bertanda32_t old_flags = slab->flags;

    /* Free objek */
    slab_free_obj(slab, obj);

    cache->obj_active--;
    cache->free_count++;

    /* Pindahkan slab jika perlu */
    if (slab->flags == SLAB_FLAG_EMPTY && old_flags != SLAB_FLAG_EMPTY) {
        /* Pindahkan ke empty list */
        if (old_flags == SLAB_FLAG_PARTIAL) {
            slab_remove_from_list(&cache->slabs_partial, slab);
        } else {
            slab_remove_from_list(&cache->slabs_full, slab);
        }
        slab_add_to_list(&cache->slabs_empty, slab);
    } else if (slab->flags == SLAB_FLAG_PARTIAL &&
               old_flags == SLAB_FLAG_FULL) {
        /* Pindahkan dari full ke partial */
        slab_remove_from_list(&cache->slabs_full, slab);
        slab_add_to_list(&cache->slabs_partial, slab);
    }

    spinlock_buka(&cache->lock);
}

/*
 * kmem_cache_stat
 * ---------------
 * Dapatkan statistik cache.
 *
 * Parameter:
 *   cache      - Pointer ke cache
 *   slab_count - Pointer untuk jumlah slab
 *   obj_total  - Pointer untuk total objek
 *   obj_active - Pointer untuk objek aktif
 *
 * Return: Status operasi
 */
status_t kmem_cache_stat(kmem_cache_t *cache,
                         tak_bertanda32_t *slab_count,
                         tak_bertanda32_t *obj_total,
                         tak_bertanda32_t *obj_active)
{
    if (cache == NULL || cache->magic != CACHE_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    if (slab_count != NULL) {
        *slab_count = cache->slab_count;
    }

    if (obj_total != NULL) {
        *obj_total = cache->obj_total;
    }

    if (obj_active != NULL) {
        *obj_active = cache->obj_active;
    }

    return STATUS_BERHASIL;
}

/*
 * kmalloc_slab
 * ------------
 * Alokasikan memori menggunakan slab allocator.
 *
 * Parameter:
 *   size - Ukuran yang dibutuhkan
 *
 * Return: Pointer ke memori, atau NULL
 */
void *kmalloc_slab(ukuran_t size)
{
    kmem_cache_t *cache;

    if (size == 0 || size > SLAB_OBJ_MAX) {
        return NULL;
    }

    cache = find_cache_for_size((tak_bertanda32_t)size);
    if (cache == NULL) {
        return NULL;
    }

    return kmem_cache_alloc(cache);
}

/*
 * kfree_slab
 * ----------
 * Bebaskan memori yang dialokasi dengan slab allocator.
 *
 * Parameter:
 *   ptr  - Pointer ke memori
 *   size - Ukuran asli saat alokasi
 */
void kfree_slab(void *ptr, ukuran_t size)
{
    kmem_cache_t *cache;

    if (ptr == NULL || size == 0 || size > SLAB_OBJ_MAX) {
        return;
    }

    cache = find_cache_for_size((tak_bertanda32_t)size);
    if (cache == NULL) {
        return;
    }

    kmem_cache_free(cache, ptr);
}

/*
 * allocator_init
 * --------------
 * Inisialisasi slab allocator.
 *
 * Return: Status operasi
 */
status_t allocator_init(void)
{
    tak_bertanda32_t i;
    char nama[16];
    tak_bertanda32_t sizes[] = { 16, 32, 64, 128, 256, 512, 1024, 2048 };

    if (allocator_initialized) {
        return STATUS_SUDAH_ADA;
    }

    /* Buat cache untuk setiap ukuran */
    for (i = 0; i < CACHE_BUILTIN_COUNT; i++) {
        kernel_snprintf(nama, sizeof(nama), "size-%lu", sizes[i]);

        builtin_caches[i].magic = 0;  /* Akan diisi oleh create */

        size_caches[i].cache = kmem_cache_create(
            nama,
            sizes[i],
            0,
            NULL,
            NULL
        );

        if (size_caches[i].cache == NULL) {
            kernel_printf("[SLAB] ERROR: Failed to create cache %s\n", nama);
            /* Continue anyway */
        }
    }

    /* Buat general purpose cache untuk ukuran sedang */
    general_cache = kmem_cache_create("general", 256, 0, NULL, NULL);

    allocator_initialized = BENAR;

    kernel_printf("[SLAB] Slab allocator initialized\n");

    return STATUS_BERHASIL;
}

/*
 * allocator_print_stats
 * ---------------------
 * Print statistik allocator.
 */
void allocator_print_stats(void)
{
    tak_bertanda32_t i;

    kernel_printf("\n[SLAB] Slab Allocator Statistics:\n");
    kernel_printf("========================================\n");

    for (i = 0; i < CACHE_BUILTIN_COUNT; i++) {
        if (size_caches[i].cache != NULL) {
            kmem_cache_t *cache = size_caches[i].cache;
            kernel_printf("  %-12s: %lu/%lu objects, %lu slabs\n",
                          cache->nama,
                          cache->obj_active,
                          cache->obj_total,
                          cache->slab_count);
        }
    }

    kernel_printf("========================================\n");
}
