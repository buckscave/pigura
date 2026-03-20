/*
 * PIGURA OS - MEMORI.C
 * --------------------
 * Implementasi inisialisasi dan manajemen subsistem memori.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk menginisialisasi
 * dan mengelola seluruh subsistem memori kernel secara terintegrasi.
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

/* Memory subsystem state flags */
#define MEM_FLAG_PMM_READY      0x01
#define MEM_FLAG_PAGING_READY   0x02
#define MEM_FLAG_HEAP_READY     0x04
#define MEM_FLAG_VIRTUAL_READY  0x08
#define MEM_FLAG_KMAP_READY     0x10
#define MEM_FLAG_DMA_READY      0x20
#define MEM_FLAG_SLAB_READY     0x40
#define MEM_FLAG_ALL_READY      0x7F

/* Memory zones */
#define ZONE_DMA                0
#define ZONE_NORMAL             1
#define ZONE_HIGHMEM            2
#define ZONE_COUNT              3

/* Zone boundaries */
#define ZONE_DMA_END            0x01000000UL   /* 16 MB */
#define ZONE_NORMAL_END         0x40000000UL   /* 1 GB (on 32-bit) */

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* Memory zone info */
typedef struct {
    const char *nama;
    alamat_fisik_t start;
    alamat_fisik_t end;
    tak_bertanda64_t total_pages;
    tak_bertanda64_t free_pages;
} mem_zone_t;

/* Memory statistics */
typedef struct {
    tak_bertanda64_t total_mem;     /* Total memori fisik */
    tak_bertanda64_t available_mem; /* Memori tersedia */
    tak_bertanda64_t kernel_mem;    /* Memori kernel */
    tak_bertanda64_t user_mem;      /* Memori user */
    tak_bertanda64_t reserved_mem;  /* Memori reserved */
    mem_zone_t zones[ZONE_COUNT];   /* Zone info */
} mem_stats_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Memory subsystem state */
static tak_bertanda32_t mem_flags = 0;

/* Memory statistics */
static mem_stats_t mem_stats = {0};

/* Bitmap storage for PMM (statically allocated) */
#define PMM_BITMAP_SIZE         32768   /* 128K entries = 512 MB max */
static tak_bertanda32_t pmm_bitmap_storage[PMM_BITMAP_SIZE];

/* Kernel heap area */
#define KERNEL_HEAP_START       0xD0000000UL
#define KERNEL_HEAP_SIZE        (4UL * 1024UL * 1024UL)  /* 4 MB */

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * parse_memory_map
 * ----------------
 * Parse memory map dari multiboot.
 *
 * Parameter:
 *   bootinfo - Pointer ke multiboot info
 *
 * Return: Status operasi
 */
static status_t parse_memory_map(multiboot_info_t *bootinfo)
{
    mmap_entry_t *mmap;
    mmap_entry_t *mmap_end;
    tak_bertanda64_t total = 0;
    tak_bertanda64_t available = 0;

    if (bootinfo == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Cek apakah ada memory map */
    if (!(bootinfo->flags & MULTIBOOT_FLAG_MMAP)) {
        /* Gunakan mem_lower dan mem_upper */
        total = (bootinfo->mem_lower + bootinfo->mem_upper) * 1024UL;
        available = total;

        kernel_printf("[MEM] No memory map, using mem_lower/upper: %lu KB\n",
                      total / 1024);
    } else {
        /* Parse memory map */
        mmap = (mmap_entry_t *)bootinfo->mmap_addr;
        mmap_end = (mmap_entry_t *)(bootinfo->mmap_addr + bootinfo->mmap_length);

        kernel_printf("[MEM] Memory map at 0x%08lX, length %lu:\n",
                      bootinfo->mmap_addr, bootinfo->mmap_length);

        while (mmap < mmap_end) {
            kernel_printf("  [0x%08lX - 0x%08lX] %s\n",
                          mmap->base_addr,
                          mmap->base_addr + mmap->length,
                          (mmap->type == MMAP_TYPE_RAM) ? "Available" :
                          (mmap->type == MMAP_TYPE_RESERVED) ? "Reserved" :
                          (mmap->type == MMAP_TYPE_ACPI) ? "ACPI" :
                          (mmap->type == MMAP_TYPE_NVS) ? "NVS" : "Unknown");

            if (mmap->type == MMAP_TYPE_RAM) {
                available += mmap->length;
            }

            total += mmap->length;

            /* Next entry */
            mmap = (mmap_entry_t *)((tak_bertanda8_t *)mmap + mmap->size + 4);
        }
    }

    mem_stats.total_mem = total;
    mem_stats.available_mem = available;

    return STATUS_BERHASIL;
}

/*
 * init_zones
 * ----------
 * Inisialisasi informasi memory zones.
 */
static void init_zones(void)
{
    /* DMA zone (0 - 16 MB) */
    mem_stats.zones[ZONE_DMA].nama = "DMA";
    mem_stats.zones[ZONE_DMA].start = 0;
    mem_stats.zones[ZONE_DMA].end = ZONE_DMA_END;
    mem_stats.zones[ZONE_DMA].total_pages = ZONE_DMA_END / UKURAN_HALAMAN;

    /* Normal zone (16 MB - 1 GB) */
    mem_stats.zones[ZONE_NORMAL].nama = "Normal";
    mem_stats.zones[ZONE_NORMAL].start = ZONE_DMA_END;
    mem_stats.zones[ZONE_NORMAL].end = ZONE_NORMAL_END;
    mem_stats.zones[ZONE_NORMAL].total_pages =
        (ZONE_NORMAL_END - ZONE_DMA_END) / UKURAN_HALAMAN;

    /* Highmem zone (1 GB+) - untuk sistem dengan >1GB RAM */
    if (mem_stats.total_mem > ZONE_NORMAL_END) {
        mem_stats.zones[ZONE_HIGHMEM].nama = "HighMem";
        mem_stats.zones[ZONE_HIGHMEM].start = ZONE_NORMAL_END;
        mem_stats.zones[ZONE_HIGHMEM].end = mem_stats.total_mem;
        mem_stats.zones[ZONE_HIGHMEM].total_pages =
            (mem_stats.total_mem - ZONE_NORMAL_END) / UKURAN_HALAMAN;
    } else {
        mem_stats.zones[ZONE_HIGHMEM].nama = "HighMem";
        mem_stats.zones[ZONE_HIGHMEM].start = ZONE_NORMAL_END;
        mem_stats.zones[ZONE_HIGHMEM].end = ZONE_NORMAL_END;
        mem_stats.zones[ZONE_HIGHMEM].total_pages = 0;
    }
}

/*
 * count_free_pages
 * ----------------
 * Hitung halaman bebas di setiap zone.
 */
static void count_free_pages(void)
{
    tak_bertanda64_t total, free, used;

    pmm_get_stats(&total, &free, &used);

    /* Distribute ke zones */
    if (free > mem_stats.zones[ZONE_DMA].total_pages) {
        mem_stats.zones[ZONE_DMA].free_pages =
            mem_stats.zones[ZONE_DMA].total_pages;
        free -= mem_stats.zones[ZONE_DMA].total_pages;
    } else {
        mem_stats.zones[ZONE_DMA].free_pages = free;
        free = 0;
    }

    if (free > mem_stats.zones[ZONE_NORMAL].total_pages) {
        mem_stats.zones[ZONE_NORMAL].free_pages =
            mem_stats.zones[ZONE_NORMAL].total_pages;
        free -= mem_stats.zones[ZONE_NORMAL].total_pages;
    } else {
        mem_stats.zones[ZONE_NORMAL].free_pages = free;
        free = 0;
    }

    mem_stats.zones[ZONE_HIGHMEM].free_pages = free;
}

/*
 * reserve_kernel_memory
 * ---------------------
 * Reserve memori yang digunakan kernel.
 *
 * Parameter:
 *   bootinfo - Pointer ke multiboot info
 */
static void reserve_kernel_memory(multiboot_info_t *bootinfo)
{
    tak_bertanda64_t kernel_start;
    tak_bertanda64_t kernel_end;
    tak_bertanda64_t kernel_size;

    /* Kernel berada di lower memory (identity mapped) */
    /* Asumsikan kernel dimulai di 1 MB */
    kernel_start = 0x100000;  /* 1 MB */

    /* Hitung ukuran kernel dari modules atau gunakan default */
    if (bootinfo != NULL && (bootinfo->flags & MULTIBOOT_FLAG_MODS)) {
        module_t *mods = (module_t *)bootinfo->mods_addr;
        if (bootinfo->mods_count > 0) {
            kernel_end = mods[0].mod_end;
        } else {
            kernel_end = kernel_start + (1 * 1024 * 1024);  /* 1 MB default */
        }
    } else {
        kernel_end = kernel_start + (1 * 1024 * 1024);  /* 1 MB default */
    }

    kernel_size = kernel_end - kernel_start;

    /* Reserve kernel region */
    pmm_reserve(kernel_start, kernel_size);

    /* Reserve boot info dan memory map */
    if (bootinfo != NULL) {
        pmm_reserve((alamat_fisik_t)bootinfo, sizeof(multiboot_info_t));

        if (bootinfo->flags & MULTIBOOT_FLAG_MMAP) {
            pmm_reserve(bootinfo->mmap_addr, bootinfo->mmap_length);
        }

        if (bootinfo->flags & MULTIBOOT_FLAG_MODS) {
            pmm_reserve(bootinfo->mods_addr,
                        bootinfo->mods_count * sizeof(module_t));
        }
    }

    /* Reserve PMM bitmap */
    pmm_reserve((alamat_fisik_t)pmm_bitmap_storage,
                PMM_BITMAP_SIZE * sizeof(tak_bertanda32_t));

    mem_stats.kernel_mem = kernel_size;
    mem_stats.reserved_mem += kernel_size;
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * memori_init
 * -----------
 * Inisialisasi subsistem memori lengkap.
 *
 * Parameter:
 *   bootinfo - Pointer ke multiboot info
 *
 * Return: Status operasi
 */
status_t memori_init(multiboot_info_t *bootinfo)
{
    status_t status;
    tak_bertanda64_t mem_size;

    if (mem_flags & MEM_FLAG_ALL_READY) {
        return STATUS_SUDAH_ADA;
    }

    kernel_printf("\n[MEM] Initializing memory subsystem...\n");
    kernel_printf("========================================\n");

    /* Parse memory map */
    status = parse_memory_map(bootinfo);
    if (status != STATUS_BERHASIL) {
        kernel_printf("[MEM] ERROR: Failed to parse memory map\n");
        return status;
    }

    /* Init zones */
    init_zones();

    kernel_printf("[MEM] Total memory: %lu MB\n", mem_stats.total_mem / (1024 * 1024));
    kernel_printf("[MEM] Available:    %lu MB\n", mem_stats.available_mem / (1024 * 1024));

    /* 1. Initialize PMM */
    mem_size = mem_stats.total_mem;
    if (mem_size == 0) {
        mem_size = 128 * 1024 * 1024;  /* Default 128 MB */
    }

    status = pmm_init(mem_size, pmm_bitmap_storage);
    if (status != STATUS_BERHASIL) {
        kernel_printf("[MEM] ERROR: PMM init failed: %d\n", status);
        return status;
    }
    mem_flags |= MEM_FLAG_PMM_READY;

    /* Add memory regions dari memory map */
    if (bootinfo != NULL && (bootinfo->flags & MULTIBOOT_FLAG_MMAP)) {
        mmap_entry_t *mmap = (mmap_entry_t *)bootinfo->mmap_addr;
        mmap_entry_t *mmap_end = (mmap_entry_t *)
            (bootinfo->mmap_addr + bootinfo->mmap_length);

        while (mmap < mmap_end) {
            if (mmap->type == MMAP_TYPE_RAM) {
                pmm_add_region(mmap->base_addr,
                               mmap->base_addr + mmap->length,
                               mmap->type);
            }
            mmap = (mmap_entry_t *)((tak_bertanda8_t *)mmap + mmap->size + 4);
        }
    } else {
        /* Gunakan mem_lower dan mem_upper */
        pmm_add_region(0, bootinfo->mem_lower * 1024UL, MEMORY_TYPE_USABLE);
        pmm_add_region(0x100000, 0x100000 + bootinfo->mem_upper * 1024UL,
                       MEMORY_TYPE_USABLE);
    }

    /* Reserve kernel memory */
    reserve_kernel_memory(bootinfo);

    /* 2. Initialize paging */
    status = paging_init(mem_size);
    if (status != STATUS_BERHASIL) {
        kernel_printf("[MEM] ERROR: Paging init failed: %d\n", status);
        return status;
    }
    mem_flags |= MEM_FLAG_PAGING_READY;

    /* 3. Initialize heap */
    /* Heap menggunakan virtual memory, jadi harus setelah paging */
    {
        alamat_virtual_t heap_vstart = KERNEL_HEAP_START;
        tak_bertanda32_t heap_pages = KERNEL_HEAP_SIZE / UKURAN_HALAMAN;
        alamat_fisik_t heap_phys;
        tak_bertanda32_t i;

        /* Alokasikan halaman fisik untuk heap */
        heap_phys = pmm_alloc_pages(heap_pages);
        if (heap_phys == 0) {
            kernel_printf("[MEM] ERROR: Failed to allocate heap memory\n");
            return STATUS_MEMORI_HABIS;
        }

        /* Map heap ke virtual address */
        for (i = 0; i < heap_pages; i++) {
            paging_map_page(heap_vstart + (i * UKURAN_HALAMAN),
                            heap_phys + (i * UKURAN_HALAMAN),
                            0x03);  /* Present | Writable */
        }

        status = heap_init((void *)heap_vstart, KERNEL_HEAP_SIZE);
        if (status != STATUS_BERHASIL) {
            kernel_printf("[MEM] ERROR: Heap init failed: %d\n", status);
            return status;
        }
    }
    mem_flags |= MEM_FLAG_HEAP_READY;

    /* 4. Initialize virtual memory manager */
    status = virtual_init();
    if (status != STATUS_BERHASIL) {
        kernel_printf("[MEM] ERROR: Virtual init failed: %d\n", status);
        return status;
    }
    mem_flags |= MEM_FLAG_VIRTUAL_READY;

    /* 5. Initialize kmap */
    status = kmap_init();
    if (status != STATUS_BERHASIL) {
        kernel_printf("[MEM] ERROR: Kmap init failed: %d\n", status);
        return status;
    }
    mem_flags |= MEM_FLAG_KMAP_READY;

    /* 6. Initialize DMA buffer manager */
    status = dma_init();
    if (status != STATUS_BERHASIL) {
        kernel_printf("[MEM] ERROR: DMA init failed: %d\n", status);
        return status;
    }
    mem_flags |= MEM_FLAG_DMA_READY;

    /* 7. Initialize slab allocator */
    status = allocator_init();
    if (status != STATUS_BERHASIL) {
        kernel_printf("[MEM] ERROR: Slab allocator init failed: %d\n", status);
        return status;
    }
    mem_flags |= MEM_FLAG_SLAB_READY;

    /* Update free page count */
    count_free_pages();

    kernel_printf("========================================\n");
    kernel_printf("[MEM] Memory subsystem initialized successfully\n\n");

    return STATUS_BERHASIL;
}

/*
 * memori_get_total
 * ----------------
 * Dapatkan total memori fisik.
 *
 * Return: Total memori dalam byte
 */
tak_bertanda64_t memori_get_total(void)
{
    return mem_stats.total_mem;
}

/*
 * memori_get_available
 * --------------------
 * Dapatkan memori tersedia.
 *
 * Return: Memori tersedia dalam byte
 */
tak_bertanda64_t memori_get_available(void)
{
    return mem_stats.available_mem;
}

/*
 * memori_get_free
 * ---------------
 * Dapatkan memori bebas.
 *
 * Return: Memori bebas dalam byte
 */
tak_bertanda64_t memori_get_free(void)
{
    tak_bertanda64_t total, free, used;
    pmm_get_stats(&total, &free, &used);
    return free * UKURAN_HALAMAN;
}

/*
 * memori_get_used
 * ---------------
 * Dapatkan memori terpakai.
 *
 * Return: Memori terpakai dalam byte
 */
tak_bertanda64_t memori_get_used(void)
{
    tak_bertanda64_t total, free, used;
    pmm_get_stats(&total, &free, &used);
    return used * UKURAN_HALAMAN;
}

/*
 * memori_get_zone_info
 * --------------------
 * Dapatkan informasi memory zone.
 *
 * Parameter:
 *   zone_id - ID zone
 *   name    - Pointer untuk nama zone
 *   start   - Pointer untuk alamat awal
 *   end     - Pointer untuk alamat akhir
 *   pages   - Pointer untuk jumlah halaman
 *   free    - Pointer untuk halaman bebas
 *
 * Return: Status operasi
 */
status_t memori_get_zone_info(tak_bertanda32_t zone_id,
                              const char **name,
                              tak_bertanda64_t *start,
                              tak_bertanda64_t *end,
                              tak_bertanda64_t *pages,
                              tak_bertanda64_t *free)
{
    if (zone_id >= ZONE_COUNT) {
        return STATUS_PARAM_INVALID;
    }

    if (name != NULL) {
        *name = mem_stats.zones[zone_id].nama;
    }

    if (start != NULL) {
        *start = mem_stats.zones[zone_id].start;
    }

    if (end != NULL) {
        *end = mem_stats.zones[zone_id].end;
    }

    if (pages != NULL) {
        *pages = mem_stats.zones[zone_id].total_pages;
    }

    if (free != NULL) {
        *free = mem_stats.zones[zone_id].free_pages;
    }

    return STATUS_BERHASIL;
}

/*
 * memori_is_ready
 * ---------------
 * Cek apakah subsistem memori sudah siap.
 *
 * Return: BENAR jika siap
 */
bool_t memori_is_ready(void)
{
    return (mem_flags & MEM_FLAG_ALL_READY) == MEM_FLAG_ALL_READY;
}

/*
 * memori_print_info
 * -----------------
 * Print informasi memori lengkap.
 */
void memori_print_info(void)
{
    kernel_printf("\n[MEM] Memory Information:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Total Physical:  %lu MB\n",
                  mem_stats.total_mem / (1024 * 1024));
    kernel_printf("  Available:       %lu MB\n",
                  mem_stats.available_mem / (1024 * 1024));
    kernel_printf("  Kernel:          %lu KB\n",
                  mem_stats.kernel_mem / 1024);
    kernel_printf("  Reserved:        %lu KB\n",
                  mem_stats.reserved_mem / 1024);
    kernel_printf("----------------------------------------\n");
    kernel_printf("  Memory Zones:\n");

    {
        tak_bertanda32_t i;
        for (i = 0; i < ZONE_COUNT; i++) {
            if (mem_stats.zones[i].total_pages > 0) {
                kernel_printf("    %s: %lu MB, %lu pages free\n",
                              mem_stats.zones[i].nama,
                              (mem_stats.zones[i].end -
                               mem_stats.zones[i].start) / (1024 * 1024),
                              mem_stats.zones[i].free_pages);
            }
        }
    }

    kernel_printf("========================================\n");

    /* Print statistik PMM */
    pmm_print_stats();

    /* Print statistik heap */
    heap_print_stats();

    /* Print statistik kmap */
    kmap_print_stats();

    /* Print statistik DMA */
    dma_print_stats();

    /* Print statistik slab allocator */
    allocator_print_stats();
}

/*
 * memori_validate
 * ---------------
 * Validasi integritas memori.
 *
 * Return: BENAR jika valid
 */
bool_t memori_validate(void)
{
    if (!(mem_flags & MEM_FLAG_HEAP_READY)) {
        return BENAR;  /* Heap belum ready, tidak bisa validate */
    }

    return heap_validate();
}

/*
 * memori_alloc_lowmem
 * -------------------
 * Alokasikan memori dari zona DMA/low memory.
 *
 * Parameter:
 *   size - Ukuran yang dibutuhkan
 *
 * Return: Alamat fisik, atau 0 jika gagal
 */
alamat_fisik_t memori_alloc_lowmem(ukuran_t size)
{
    tak_bertanda32_t pages;
    alamat_fisik_t phys;
    tak_bertanda32_t i;

    if (!(mem_flags & MEM_FLAG_PMM_READY)) {
        return 0;
    }

    pages = RATAKAN_ATAS(size, UKURAN_HALAMAN) / UKURAN_HALAMAN;

    /* Coba alokasi di DMA zone */
    /* PMM menggunakan first-fit, jadi alokasi dari awal */
    phys = pmm_alloc_pages(pages);

    /* Cek apakah dalam DMA zone */
    if (phys >= ZONE_DMA_END) {
        /* Tidak dalam DMA zone, free dan coba cara lain */
        pmm_free_pages(phys, pages);
        return 0;
    }

    return phys;
}
