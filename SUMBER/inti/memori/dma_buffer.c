/*
 * PIGURA OS - DMA_BUFFER.C
 * ------------------------
 * Implementasi manajemen DMA buffer.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk mengelola
 * buffer DMA (Direct Memory Access) yang memerlukan memori
 * yang contiguous dan biasanya dalam range alamat tertentu.
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

/* DMA zone boundaries */
#define DMA_ZONE_START          0x00000000UL
#define DMA_ZONE_END            0x01000000UL   /* 16 MB - ISA DMA limit */
#define DMA_ZONE_SIZE           (DMA_ZONE_END - DMA_ZONE_START)

/* DMA alignment */
#define DMA_ALIGN_8BIT          1
#define DMA_ALIGN_16BIT         2
#define DMA_ALIGN_32BIT         4

/* Maximum DMA allocations */
#define DMA_MAX_BUFFERS         256

/* DMA buffer flags */
#define DMA_FLAG_NONE           0x00
#define DMA_FLAG_INUSE          0x01
#define DMA_FLAG_MAPPED         0x02
#define DMA_FLAG_COHERENT       0x04
#define DMA_FLAG_RESERVED       0x08

/* Magic number */
#define DMA_BUFFER_MAGIC        0x444D4100  /* "DMA\0" */

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* DMA buffer descriptor */
typedef struct dma_buffer {
    tak_bertanda32_t magic;         /* Magic number */
    tak_bertanda32_t flags;         /* Status flags */
    alamat_fisik_t phys;            /* Alamat fisik */
    void *virt;                     /* Alamat virtual (mapped) */
    ukuran_t size;                  /* Ukuran buffer */
    tak_bertanda32_t alignment;     /* Alignment requirement */
    tak_bertanda32_t owner;         /* Owner (device/driver ID) */
    char name[32];                  /* Nama buffer */
    struct dma_buffer *next;        /* Link ke buffer berikutnya */
    struct dma_buffer *prev;        /* Link ke buffer sebelumnya */
} dma_buffer_t;

/* DMA zone state */
typedef struct {
    tak_bertanda8_t *bitmap;        /* Allocation bitmap */
    tak_bertanda32_t bitmap_size;   /* Ukuran bitmap */
    tak_bertanda32_t total_pages;   /* Total halaman */
    tak_bertanda32_t free_pages;    /* Halaman bebas */
    tak_bertanda32_t used_pages;    /* Halaman terpakai */
    spinlock_t lock;                /* Lock untuk thread safety */
} dma_zone_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* DMA zone */
static dma_zone_t dma_zone = {0};

/* DMA buffer tracking */
static dma_buffer_t dma_buffers[DMA_MAX_BUFFERS];
static dma_buffer_t *dma_free_list = NULL;
static dma_buffer_t *dma_active_list = NULL;

/* Status */
static bool_t dma_initialized = SALAH;

/* Statistik */
static struct {
    tak_bertanda64_t alloc_count;
    tak_bertanda64_t free_count;
    tak_bertanda64_t alloc_failures;
} dma_stats = {0};

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * bitmap_test
 * -----------
 * Test bit dalam bitmap.
 */
static inline bool_t dma_bitmap_test(tak_bertanda32_t index)
{
    tak_bertanda32_t word = index / 8;
    tak_bertanda32_t bit = index % 8;
    return (dma_zone.bitmap[word] & (1 << bit)) ? BENAR : SALAH;
}

/*
 * bitmap_set
 * ----------
 * Set bit dalam bitmap.
 */
static inline void dma_bitmap_set(tak_bertanda32_t index)
{
    tak_bertanda32_t word = index / 8;
    tak_bertanda32_t bit = index % 8;
    dma_zone.bitmap[word] |= (1 << bit);
}

/*
 * bitmap_clear
 * ------------
 * Clear bit dalam bitmap.
 */
static inline void dma_bitmap_clear(tak_bertanda32_t index)
{
    tak_bertanda32_t word = index / 8;
    tak_bertanda32_t bit = index % 8;
    dma_zone.bitmap[word] &= ~(1 << bit);
}

/*
 * find_contiguous_pages
 * ---------------------
 * Cari blok halaman contiguous di DMA zone.
 *
 * Parameter:
 *   count - Jumlah halaman yang dibutuhkan
 *   align - Alignment requirement (dalam halaman)
 *
 * Return: Index halaman pertama, atau -1
 */
static tanda32_t find_contiguous_pages(tak_bertanda32_t count,
                                       tak_bertanda32_t align)
{
    tak_bertanda32_t i;
    tak_bertanda32_t j;
    bool_t found;

    if (count == 0 || count > dma_zone.free_pages) {
        return -1;
    }

    for (i = 0; i < dma_zone.total_pages; i += align) {
        /* Cek alignment */
        if (align > 1 && (i % align) != 0) {
            continue;
        }

        /* Cek apakah bisa mulai dari sini */
        if (dma_bitmap_test(i)) {
            continue;
        }

        found = BENAR;

        /* Cek count halaman berikutnya */
        for (j = 0; j < count; j++) {
            if (i + j >= dma_zone.total_pages) {
                found = SALAH;
                break;
            }

            if (dma_bitmap_test(i + j)) {
                found = SALAH;
                i += j;
                break;
            }
        }

        if (found) {
            return (tanda32_t)i;
        }
    }

    return -1;
}

/*
 * allocate_dma_pages
 * ------------------
 * Alokasikan halaman di DMA zone.
 *
 * Parameter:
 *   count - Jumlah halaman
 *   align - Alignment requirement
 *
 * Return: Alamat fisik, atau 0 jika gagal
 */
static alamat_fisik_t allocate_dma_pages(tak_bertanda32_t count,
                                         tak_bertanda32_t align)
{
    tanda32_t index;
    tak_bertanda32_t i;
    alamat_fisik_t phys;

    spinlock_kunci(&dma_zone.lock);

    index = find_contiguous_pages(count, align);
    if (index < 0) {
        spinlock_buka(&dma_zone.lock);
        return 0;
    }

    /* Mark halaman sebagai used */
    for (i = 0; i < count; i++) {
        dma_bitmap_set((tak_bertanda32_t)index + i);
    }

    dma_zone.free_pages -= count;
    dma_zone.used_pages += count;

    spinlock_buka(&dma_zone.lock);

    /* Hitung alamat fisik */
    phys = DMA_ZONE_START + ((tak_bertanda32_t)index * UKURAN_HALAMAN);

    /* Clear memori */
    kernel_memset((void *)(uintptr_t)phys, 0, count * UKURAN_HALAMAN);

    return phys;
}

/*
 * free_dma_pages
 * --------------
 * Bebaskan halaman di DMA zone.
 *
 * Parameter:
 *   phys  - Alamat fisik
 *   count - Jumlah halaman
 */
static void free_dma_pages(alamat_fisik_t phys, tak_bertanda32_t count)
{
    tak_bertanda32_t index;
    tak_bertanda32_t i;

    /* DMA_ZONE_START is 0, so only check upper bound */
    if (phys >= DMA_ZONE_END) {
        return;
    }

    index = (phys - DMA_ZONE_START) / UKURAN_HALAMAN;

    spinlock_kunci(&dma_zone.lock);

    for (i = 0; i < count; i++) {
        dma_bitmap_clear(index + i);
    }

    dma_zone.free_pages += count;
    dma_zone.used_pages -= count;

    spinlock_buka(&dma_zone.lock);
}

/*
 * get_dma_buffer_desc
 * --------------------
 * Dapatkan deskriptor DMA buffer kosong.
 *
 * Return: Pointer ke deskriptor, atau NULL
 */
static dma_buffer_t *get_dma_buffer_desc(void)
{
    dma_buffer_t *buf;

    if (dma_free_list == NULL) {
        return NULL;
    }

    buf = dma_free_list;
    dma_free_list = buf->next;

    if (dma_free_list != NULL) {
        dma_free_list->prev = NULL;
    }

    /* Add ke active list */
    buf->next = dma_active_list;
    buf->prev = NULL;

    if (dma_active_list != NULL) {
        dma_active_list->prev = buf;
    }

    dma_active_list = buf;

    return buf;
}

/*
 * release_dma_buffer_desc
 * -----------------------
 * Bebaskan deskriptor DMA buffer.
 *
 * Parameter:
 *   buf - Pointer ke deskriptor
 */
static void release_dma_buffer_desc(dma_buffer_t *buf)
{
    if (buf == NULL) {
        return;
    }

    /* Remove dari active list */
    if (buf->prev != NULL) {
        buf->prev->next = buf->next;
    } else {
        dma_active_list = buf->next;
    }

    if (buf->next != NULL) {
        buf->next->prev = buf->prev;
    }

    /* Clear dan add ke free list */
    kernel_memset(buf, 0, sizeof(dma_buffer_t));
    buf->next = dma_free_list;
    dma_free_list = buf;
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * dma_init
 * --------
 * Inisialisasi DMA buffer manager.
 *
 * Return: Status operasi
 */
status_t dma_init(void)
{
    tak_bertanda32_t i;

    if (dma_initialized) {
        return STATUS_SUDAH_ADA;
    }

    /* Hitung ukuran bitmap */
    dma_zone.total_pages = DMA_ZONE_SIZE / UKURAN_HALAMAN;
    dma_zone.bitmap_size = (dma_zone.total_pages + 7) / 8;

    /* Alokasikan bitmap (dari heap) */
    dma_zone.bitmap = kmalloc(dma_zone.bitmap_size);
    if (dma_zone.bitmap == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    /* Clear bitmap - semua halaman bebas awalnya */
    kernel_memset(dma_zone.bitmap, 0, dma_zone.bitmap_size);

    /* Mark halaman 0 sebagai used (protected) */
    dma_bitmap_set(0);
    dma_zone.free_pages = dma_zone.total_pages - 1;
    dma_zone.used_pages = 1;

    /* Init lock */
    spinlock_init(&dma_zone.lock);

    /* Init buffer descriptors */
    for (i = 0; i < DMA_MAX_BUFFERS; i++) {
        dma_buffers[i].magic = DMA_BUFFER_MAGIC;
        dma_buffers[i].flags = DMA_FLAG_NONE;
        dma_buffers[i].next = dma_free_list;
        if (dma_free_list != NULL) {
            dma_free_list->prev = &dma_buffers[i];
        }
        dma_free_list = &dma_buffers[i];
    }

    dma_active_list = NULL;

    /* Reset stats */
    dma_stats.alloc_count = 0;
    dma_stats.free_count = 0;
    dma_stats.alloc_failures = 0;

    dma_initialized = BENAR;

    kernel_printf("[DMA] DMA buffer manager initialized\n");
    kernel_printf("      Zone: 0x%08lX - 0x%08lX (%lu MB)\n",
                  DMA_ZONE_START, DMA_ZONE_END, DMA_ZONE_SIZE / (1024 * 1024));
    kernel_printf("      Free pages: %lu\n", dma_zone.free_pages);

    return STATUS_BERHASIL;
}

/*
 * dma_alloc_coherent
 * ------------------
 * Alokasikan buffer DMA coherent.
 *
 * Parameter:
 *   size  - Ukuran buffer
 *   align - Alignment requirement
 *   name  - Nama buffer (untuk debugging)
 *
 * Return: Pointer ke dma_buffer_t, atau NULL
 */
dma_buffer_t *dma_alloc_coherent(ukuran_t size, tak_bertanda32_t align,
                                 const char *name)
{
    dma_buffer_t *buf;
    tak_bertanda32_t pages;
    alamat_fisik_t phys;
    void *virt;

    if (!dma_initialized || size == 0) {
        return NULL;
    }

    /* Align size ke halaman */
    size = RATAKAN_ATAS(size, UKURAN_HALAMAN);
    pages = size / UKURAN_HALAMAN;

    /* Handle alignment */
    if (align == 0) {
        align = 1;
    }
    align = RATAKAN_ATAS(align, UKURAN_HALAMAN) / UKURAN_HALAMAN;
    if (align == 0) {
        align = 1;
    }

    /* Get buffer descriptor */
    buf = get_dma_buffer_desc();
    if (buf == NULL) {
        dma_stats.alloc_failures++;
        return NULL;
    }

    /* Allocate physical pages */
    phys = allocate_dma_pages(pages, align);
    if (phys == 0) {
        release_dma_buffer_desc(buf);
        dma_stats.alloc_failures++;
        kernel_printf("[DMA] Failed to allocate %lu pages\n", pages);
        return NULL;
    }

    /* Map ke virtual address */
    virt = kmap(phys);
    if (virt == NULL) {
        free_dma_pages(phys, pages);
        release_dma_buffer_desc(buf);
        dma_stats.alloc_failures++;
        return NULL;
    }

    /* Setup buffer descriptor */
    buf->magic = DMA_BUFFER_MAGIC;
    buf->flags = DMA_FLAG_INUSE | DMA_FLAG_COHERENT | DMA_FLAG_MAPPED;
    buf->phys = phys;
    buf->virt = virt;
    buf->size = size;
    buf->alignment = align * UKURAN_HALAMAN;
    buf->owner = 0;  /* TODO: Set owner */

    if (name != NULL) {
        kernel_strncpy(buf->name, name, sizeof(buf->name) - 1);
        buf->name[sizeof(buf->name) - 1] = '\0';
    }

    dma_stats.alloc_count++;

    return buf;
}

/*
 * dma_free_coherent
 * -----------------
 * Bebaskan buffer DMA coherent.
 *
 * Parameter:
 *   buf - Pointer ke dma_buffer_t
 */
void dma_free_coherent(dma_buffer_t *buf)
{
    tak_bertanda32_t pages;

    if (buf == NULL || buf->magic != DMA_BUFFER_MAGIC) {
        return;
    }

    if (!(buf->flags & DMA_FLAG_INUSE)) {
        return;
    }

    pages = buf->size / UKURAN_HALAMAN;

    /* Unmap virtual address */
    if (buf->flags & DMA_FLAG_MAPPED) {
        kunmap(buf->virt);
    }

    /* Free physical pages */
    free_dma_pages(buf->phys, pages);

    /* Release descriptor */
    release_dma_buffer_desc(buf);

    dma_stats.free_count++;
}

/*
 * dma_map_single
 * --------------
 * Map buffer single untuk DMA transfer.
 *
 * Parameter:
 *   virt - Alamat virtual buffer
 *   size - Ukuran buffer
 *   dir  - Arah DMA (baca/tulis)
 *
 * Return: Alamat DMA (bus address), atau 0
 */
alamat_fisik_t dma_map_single(void *virt, ukuran_t size, int dir)
{
    alamat_fisik_t phys;

    if (!dma_initialized || virt == NULL || size == 0) {
        return 0;
    }

    /* Dapatkan alamat fisik */
    phys = paging_get_physical((alamat_virtual_t)virt);
    if (phys == 0) {
        return 0;
    }

    /* Cek apakah dalam DMA zone */
    /* DMA_ZONE_START is 0, so only check upper bound */
    if (phys + size > DMA_ZONE_END) {
        /* Buffer tidak dalam DMA zone - perlu bounce buffer */
        /* TODO: Implement bounce buffer */
        kernel_printf("[DMA] Warning: Buffer outside DMA zone\n");
        return phys;  /* Sementara kembalikan phys saja */
    }

    /* Flush cache untuk device read */
    if (dir == 1) {  /* DMA_TO_DEVICE */
        /* TODO: Implement cache flush */
    }

    return phys;
}

/*
 * dma_unmap_single
 * ----------------
 * Unmap buffer DMA single.
 *
 * Parameter:
 *   dma_addr - Alamat DMA
 *   size     - Ukuran buffer
 *   dir      - Arah DMA
 */
void dma_unmap_single(alamat_fisik_t dma_addr, ukuran_t size, int dir)
{
    if (!dma_initialized || dma_addr == 0 || size == 0) {
        return;
    }

    /* Invalidate cache untuk device write */
    if (dir == 2) {  /* DMA_FROM_DEVICE */
        /* TODO: Implement cache invalidate */
    }
}

/*
 * dma_alloc_bounce
 * ----------------
 * Alokasikan bounce buffer.
 *
 * Parameter:
 *   size - Ukuran buffer
 *
 * Return: Pointer ke bounce buffer, atau NULL
 */
void *dma_alloc_bounce(ukuran_t size)
{
    dma_buffer_t *buf;

    buf = dma_alloc_coherent(size, UKURAN_HALAMAN, "bounce");
    if (buf == NULL) {
        return NULL;
    }

    return buf->virt;
}

/*
 * dma_free_bounce
 * ---------------
 * Bebaskan bounce buffer.
 *
 * Parameter:
 *   ptr  - Pointer ke bounce buffer
 *   size - Ukuran buffer
 */
void dma_free_bounce(void *ptr, ukuran_t size)
{
    tak_bertanda32_t i;

    (void)size;  /* Currently unused */

    if (ptr == NULL) {
        return;
    }

    /* Cari buffer descriptor */
    for (i = 0; i < DMA_MAX_BUFFERS; i++) {
        if (dma_buffers[i].virt == ptr &&
            (dma_buffers[i].flags & DMA_FLAG_INUSE)) {
            dma_free_coherent(&dma_buffers[i]);
            return;
        }
    }
}

/*
 * dma_get_phys
 * ------------
 * Dapatkan alamat fisik DMA buffer.
 *
 * Parameter:
 *   buf - Pointer ke dma_buffer_t
 *
 * Return: Alamat fisik
 */
alamat_fisik_t dma_get_phys(dma_buffer_t *buf)
{
    if (buf == NULL || buf->magic != DMA_BUFFER_MAGIC) {
        return 0;
    }

    return buf->phys;
}

/*
 * dma_get_virt
 * ------------
 * Dapatkan alamat virtual DMA buffer.
 *
 * Parameter:
 *   buf - Pointer ke dma_buffer_t
 *
 * Return: Alamat virtual
 */
void *dma_get_virt(dma_buffer_t *buf)
{
    if (buf == NULL || buf->magic != DMA_BUFFER_MAGIC) {
        return NULL;
    }

    return buf->virt;
}

/*
 * dma_sync_single_for_cpu
 * -----------------------
 * Sync buffer untuk CPU access.
 *
 * Parameter:
 *   dma_addr - Alamat DMA
 *   size     - Ukuran
 *   dir      - Arah
 */
void dma_sync_single_for_cpu(alamat_fisik_t dma_addr, ukuran_t size, int dir)
{
    /* TODO: Implement cache invalidate */
    (void)dma_addr;
    (void)size;
    (void)dir;
}

/*
 * dma_sync_single_for_device
 * --------------------------
 * Sync buffer untuk device access.
 *
 * Parameter:
 *   dma_addr - Alamat DMA
 *   size     - Ukuran
 *   dir      - Arah
 */
void dma_sync_single_for_device(alamat_fisik_t dma_addr, ukuran_t size,
                                int dir)
{
    /* TODO: Implement cache flush */
    (void)dma_addr;
    (void)size;
    (void)dir;
}

/*
 * dma_get_stats
 * -------------
 * Dapatkan statistik DMA.
 *
 * Parameter:
 *   total_pages - Pointer untuk total halaman
 *   free_pages  - Pointer untuk halaman bebas
 *   alloc_count - Pointer untuk jumlah alokasi
 */
void dma_get_stats(tak_bertanda32_t *total_pages, tak_bertanda32_t *free_pages,
                   tak_bertanda64_t *alloc_count)
{
    if (total_pages != NULL) {
        *total_pages = dma_zone.total_pages;
    }

    if (free_pages != NULL) {
        *free_pages = dma_zone.free_pages;
    }

    if (alloc_count != NULL) {
        *alloc_count = dma_stats.alloc_count;
    }
}

/*
 * dma_print_stats
 * ---------------
 * Print statistik DMA.
 */
void dma_print_stats(void)
{
    kernel_printf("\n[DMA] DMA Buffer Statistics:\n");
    kernel_printf("========================================\n");
    kernel_printf("  DMA Zone: 0x%08lX - 0x%08lX\n",
                  DMA_ZONE_START, DMA_ZONE_END);
    kernel_printf("  Total pages: %lu (%lu MB)\n",
                  dma_zone.total_pages,
                  (dma_zone.total_pages * UKURAN_HALAMAN) / (1024 * 1024));
    kernel_printf("  Free pages:  %lu (%lu MB)\n",
                  dma_zone.free_pages,
                  (dma_zone.free_pages * UKURAN_HALAMAN) / (1024 * 1024));
    kernel_printf("  Used pages:  %lu\n", dma_zone.used_pages);
    kernel_printf("  Total allocs: %lu\n", dma_stats.alloc_count);
    kernel_printf("  Total frees:  %lu\n", dma_stats.free_count);
    kernel_printf("  Failures:     %lu\n", dma_stats.alloc_failures);
    kernel_printf("========================================\n");
}
