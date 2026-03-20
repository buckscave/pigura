/*
 * PIGURA OS - PAGING.C
 * --------------------
 * Implementasi sistem paging untuk manajemen memori virtual.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk mengelola
 * page tables, page directory, dan operasi paging pada x86.
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

/* Page sizes */
#define PAGE_SIZE_4KB           4096UL
#define PAGE_SIZE_4MB           (4UL * 1024UL * 1024UL)

/* Page table entry flags */
#define PTE_PRESENT             0x001
#define PTE_WRITABLE            0x002
#define PTE_USER                0x004
#define PTE_WRITE_THROUGH       0x008
#define PTE_CACHE_DISABLE       0x010
#define PTE_ACCESSED            0x020
#define PTE_DIRTY               0x040
#define PTE_PAT                 0x080
#define PTE_GLOBAL              0x100
#define PTE_FRAME               0xFFFFF000

/* Page directory entry flags */
#define PDE_PRESENT             0x001
#define PDE_WRITABLE            0x002
#define PDE_USER                0x004
#define PDE_WRITE_THROUGH       0x008
#define PDE_CACHE_DISABLE       0x010
#define PDE_ACCESSED            0x020
#define PDE_4MB                 0x080   /* 4MB page */
#define PDE_FRAME               0xFFFFF000

/* Number of entries */
#define ENTRIES_PER_TABLE       1024
#define ENTRIES_PER_DIR         1024

/* Virtual address layout */
#define KERNEL_SPACE_START      0xC0000000UL
#define KERNEL_SPACE_END        0xFFFFFFFFUL
#define USER_SPACE_START        0x00000000UL
#define USER_SPACE_END          0xBFFFFFFFUL

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* Page Table Entry */
typedef struct {
    tak_bertanda32_t value;
} pte_t;

/* Page Directory Entry */
typedef struct {
    tak_bertanda32_t value;
} pde_t;

/* Page Table */
typedef struct {
    pte_t entries[ENTRIES_PER_TABLE];
} __attribute__((aligned(PAGE_SIZE_4KB))) page_table_t;

/* Page Directory */
typedef struct {
    pde_t entries[ENTRIES_PER_DIR];
} __attribute__((aligned(PAGE_SIZE_4KB))) page_directory_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Kernel page directory */
static page_directory_t *kernel_page_dir = NULL;

/* Page tables untuk kernel space */
static page_table_t *kernel_page_tables = NULL;

/* Status */
static bool_t paging_initialized = SALAH;

/* Current page directory */
static page_directory_t *current_page_dir = NULL;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * pte_create
 * ----------
 * Buat Page Table Entry.
 *
 * Parameter:
 *   frame - Alamat frame fisik
 *   flags - Flag PTE
 *
 * Return: PTE value
 */
static inline pte_t pte_create(alamat_fisik_t frame, tak_bertanda32_t flags)
{
    pte_t pte;
    pte.value = (frame & PTE_FRAME) | (flags & 0xFFF);
    return pte;
}

/*
 * pde_create
 * ----------
 * Buat Page Directory Entry.
 *
 * Parameter:
 *   table - Alamat page table fisik
 *   flags - Flag PDE
 *
 * Return: PDE value
 */
static inline pde_t pde_create(alamat_fisik_t table, tak_bertanda32_t flags)
{
    pde_t pde;
    pde.value = (table & PDE_FRAME) | (flags & 0xFFF);
    return pde;
}

/*
 * get_page_table
 * --------------
 * Dapatkan atau buat page table untuk alamat virtual.
 *
 * Parameter:
 *   dir    - Page directory
 *   vaddr  - Alamat virtual
 *   create - Buat jika tidak ada
 *
 * Return: Pointer ke page table, atau NULL
 */
static page_table_t *get_page_table(page_directory_t *dir,
                                    alamat_virtual_t vaddr,
                                    bool_t create)
{
    tak_bertanda32_t index;
    pde_t *pde;
    alamat_fisik_t table_phys;

    index = (vaddr >> 22) & 0x3FF;
    pde = &dir->entries[index];

    if (pde->value & PDE_PRESENT) {
        /* Page table sudah ada */
        table_phys = pde->value & PDE_FRAME;
        return (page_table_t *)table_phys;
    }

    if (!create) {
        return NULL;
    }

    /* Alokasikan page table baru */
    table_phys = pmm_alloc_page();
    if (table_phys == 0) {
        return NULL;
    }

    /* Clear page table */
    kernel_memset((void *)table_phys, 0, PAGE_SIZE_4KB);

    /* Set PDE */
    *pde = pde_create(table_phys, PDE_PRESENT | PDE_WRITABLE | PDE_USER);

    return (page_table_t *)table_phys;
}

/*
 * invalidate_page
 * ---------------
 * Invalidate satu halaman di TLB.
 *
 * Parameter:
 *   vaddr - Alamat virtual
 */
static inline void invalidate_page(alamat_virtual_t vaddr)
{
    __asm__ __volatile__("invlpg (%0)" : : "r"(vaddr) : "memory");
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * paging_init
 * -----------
 * Inisialisasi sistem paging.
 *
 * Parameter:
 *   mem_size - Total ukuran memori fisik
 *
 * Return: Status operasi
 */
status_t paging_init(tak_bertanda64_t mem_size)
{
    tak_bertanda32_t i;
    tak_bertanda32_t num_tables;
    alamat_fisik_t phys;

    if (paging_initialized) {
        return STATUS_SUDAH_ADA;
    }

    /* Alokasikan page directory */
    kernel_page_dir = (page_directory_t *)pmm_alloc_page();
    if (kernel_page_dir == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    kernel_memset(kernel_page_dir, 0, PAGE_SIZE_4KB);

    /* Hitung jumlah page tables yang dibutuhkan untuk kernel space */
    /* Kernel space: 0xC0000000 - 0xFFFFFFFF = 1 GB = 256 page tables */
    num_tables = 256;

    /* Alokasikan page tables untuk kernel */
    kernel_page_tables = (page_table_t *)pmm_alloc_pages(num_tables);
    if (kernel_page_tables == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    kernel_memset(kernel_page_tables, 0, num_tables * PAGE_SIZE_4KB);

    /* Identity map first 4 MB (untuk boot) */
    for (i = 0; i < 1024; i++) {
        phys = i * PAGE_SIZE_4KB;

        /* Identity mapping */
        kernel_page_tables[i].entries[i] = pte_create(phys,
            PTE_PRESENT | PTE_WRITABLE);
    }

    /* Map kernel space (3-4 GB) ke physical 0-1 GB */
    for (i = 0; i < num_tables; i++) {
        tak_bertanda32_t table_index = 768 + i;  /* 768 = 0xC0000000 >> 22 */
        tak_bertanda32_t j;
        alamat_fisik_t table_phys;

        /* Set PDE untuk kernel space */
        table_phys = (alamat_fisik_t)&kernel_page_tables[i];
        kernel_page_dir->entries[table_index] = pde_create(table_phys,
            PDE_PRESENT | PTE_WRITABLE);

        /* Map semua entries dalam page table */
        for (j = 0; j < ENTRIES_PER_TABLE; j++) {
            phys = (i * ENTRIES_PER_TABLE + j) * PAGE_SIZE_4KB;

            if (phys < mem_size) {
                kernel_page_tables[i].entries[j] = pte_create(phys,
                    PTE_PRESENT | PTE_WRITABLE);
            }
        }
    }

    /* Load page directory */
    current_page_dir = kernel_page_dir;
    cpu_write_cr3((tak_bertanda32_t)kernel_page_dir);

    /* Enable paging */
    {
        tak_bertanda32_t cr0 = cpu_read_cr0();
        cr0 |= (1 << 31);  /* Set PG bit */
        cpu_write_cr0(cr0);
    }

    paging_initialized = BENAR;

    kernel_printf("[PAGING] Enabled - Kernel dir at 0x%08lX\n",
                  (alamat_ptr_t)kernel_page_dir);

    return STATUS_BERHASIL;
}

/*
 * paging_map_page
 * ---------------
 * Map satu halaman virtual ke fisik.
 *
 * Parameter:
 *   vaddr  - Alamat virtual
 *   paddr  - Alamat fisik
 *   flags  - Flag untuk mapping
 *
 * Return: Status operasi
 */
status_t paging_map_page(alamat_virtual_t vaddr, alamat_fisik_t paddr,
                         tak_bertanda32_t flags)
{
    page_directory_t *dir;
    page_table_t *table;
    tak_bertanda32_t table_index;
    tak_bertanda32_t page_index;

    if (!paging_initialized) {
        return STATUS_GAGAL;
    }

    /* Validasi alignment */
    if ((vaddr & 0xFFF) != 0 || (paddr & 0xFFF) != 0) {
        return STATUS_PARAM_INVALID;
    }

    dir = current_page_dir;

    /* Dapatkan page table */
    table = get_page_table(dir, vaddr, BENAR);
    if (table == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    /* Set PTE */
    table_index = (vaddr >> 22) & 0x3FF;
    page_index = (vaddr >> 12) & 0x3FF;

    table->entries[page_index] = pte_create(paddr, flags);

    /* Invalidate TLB */
    invalidate_page(vaddr);

    return STATUS_BERHASIL;
}

/*
 * paging_unmap_page
 * -----------------
 * Unmap satu halaman virtual.
 *
 * Parameter:
 *   vaddr - Alamat virtual
 *
 * Return: Status operasi
 */
status_t paging_unmap_page(alamat_virtual_t vaddr)
{
    page_directory_t *dir;
    page_table_t *table;
    tak_bertanda32_t page_index;

    if (!paging_initialized) {
        return STATUS_GAGAL;
    }

    dir = current_page_dir;
    table = get_page_table(dir, vaddr, SALAH);

    if (table == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    page_index = (vaddr >> 12) & 0x3FF;
    table->entries[page_index].value = 0;

    invalidate_page(vaddr);

    return STATUS_BERHASIL;
}

/*
 * paging_get_physical
 * -------------------
 * Dapatkan alamat fisik dari alamat virtual.
 *
 * Parameter:
 *   vaddr - Alamat virtual
 *
 * Return: Alamat fisik, atau 0 jika tidak mapped
 */
alamat_fisik_t paging_get_physical(alamat_virtual_t vaddr)
{
    page_directory_t *dir;
    page_table_t *table;
    tak_bertanda32_t page_index;
    alamat_fisik_t paddr;

    if (!paging_initialized) {
        return 0;
    }

    dir = current_page_dir;
    table = get_page_table(dir, vaddr, SALAH);

    if (table == NULL) {
        return 0;
    }

    page_index = (vaddr >> 12) & 0x3FF;

    if (!(table->entries[page_index].value & PTE_PRESENT)) {
        return 0;
    }

    paddr = table->entries[page_index].value & PTE_FRAME;
    paddr |= vaddr & 0xFFF;  /* Tambahkan offset */

    return paddr;
}

/*
 * paging_create_address_space
 * ---------------------------
 * Buat address space baru.
 *
 * Return: Pointer ke page directory baru
 */
page_directory_t *paging_create_address_space(void)
{
    page_directory_t *new_dir;
    tak_bertanda32_t i;

    if (!paging_initialized) {
        return NULL;
    }

    /* Alokasikan page directory baru */
    new_dir = (page_directory_t *)pmm_alloc_page();
    if (new_dir == NULL) {
        return NULL;
    }
    kernel_memset(new_dir, 0, PAGE_SIZE_4KB);

    /* Copy kernel space mappings (entries 768-1023) */
    for (i = 768; i < 1024; i++) {
        new_dir->entries[i] = kernel_page_dir->entries[i];
    }

    return new_dir;
}

/*
 * paging_destroy_address_space
 * ----------------------------
 * Hancurkan address space.
 *
 * Parameter:
 *   dir - Page directory
 *
 * Return: Status operasi
 */
status_t paging_destroy_address_space(page_directory_t *dir)
{
    tak_bertanda32_t i;

    if (!paging_initialized || dir == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Jangan destroy kernel page directory */
    if (dir == kernel_page_dir) {
        return STATUS_PARAM_INVALID;
    }

    /* Bebaskan page tables user space (entries 0-767) */
    for (i = 0; i < 768; i++) {
        if (dir->entries[i].value & PDE_PRESENT) {
            alamat_fisik_t table_phys = dir->entries[i].value & PDE_FRAME;
            pmm_free_page(table_phys);
        }
    }

    /* Bebaskan page directory */
    pmm_free_page((alamat_fisik_t)dir);

    return STATUS_BERHASIL;
}

/*
 * paging_switch_directory
 * -----------------------
 * Switch ke page directory lain.
 *
 * Parameter:
 *   dir - Page directory baru
 *
 * Return: Status operasi
 */
status_t paging_switch_directory(page_directory_t *dir)
{
    if (!paging_initialized || dir == NULL) {
        return STATUS_PARAM_INVALID;
    }

    current_page_dir = dir;
    cpu_write_cr3((tak_bertanda32_t)dir);

    return STATUS_BERHASIL;
}

/*
 * paging_get_current_directory
 * ----------------------------
 * Dapatkan page directory saat ini.
 *
 * Return: Pointer ke page directory
 */
page_directory_t *paging_get_current_directory(void)
{
    return current_page_dir;
}

/*
 * paging_is_mapped
 * ----------------
 * Cek apakah alamat virtual sudah di-map.
 *
 * Parameter:
 *   vaddr - Alamat virtual
 *
 * Return: BENAR jika sudah mapped
 */
bool_t paging_is_mapped(alamat_virtual_t vaddr)
{
    return paging_get_physical(vaddr) != 0;
}
