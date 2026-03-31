/*
 * PIGURA OS - VIRTUAL.C
 * ---------------------
 * Implementasi manajemen virtual memory.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk mengelola
 * virtual address space, termasuk alokasi dan dealokasi
 * region memori virtual.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

/*
 * Cegah proses.h dari mendefinisikan vma_t dan vm_descriptor_t
 * karena berkas ini memiliki definisi lokal yang lebih lengkap
 */
#define _VMA_T_DEFINED
#define _VM_DESCRIPTOR_T_DEFINED
#define _VMA_TYPES_SUPPRESSED

#include "../kernel.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* Virtual address space layout */
#define VIRT_KERNEL_START       0xC0000000UL
#define VIRT_KERNEL_END         0xFFFFFFFFUL
#define VIRT_USER_START         0x00000000UL
#define VIRT_USER_END           0xBFFFFFFFUL

/* Virtual memory area flags */
#ifndef VMA_FLAG_NONE
#define VMA_FLAG_NONE           0x00
#endif
#ifndef VMA_FLAG_READ
#define VMA_FLAG_READ           0x01
#endif
#ifndef VMA_FLAG_WRITE
#define VMA_FLAG_WRITE          0x02
#endif
#ifndef VMA_FLAG_EXEC
#define VMA_FLAG_EXEC           0x04
#endif
#ifndef VMA_FLAG_STACK
#define VMA_FLAG_STACK          0x08
#endif
#ifndef VMA_FLAG_HEAP
#define VMA_FLAG_HEAP           0x10
#endif
#ifndef VMA_FLAG_SHARED
#define VMA_FLAG_SHARED         0x20
#endif
#ifndef VMA_FLAG_FIXED
#define VMA_FLAG_FIXED          0x40
#endif

/* Maximum VMA per process */
#define VMA_MAX_PER_PROCESS     64

/* Magic number */
#define VMA_MAGIC               0x564D4100  /* "VMA\0" */

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* Virtual Memory Area descriptor */
#undef _VMA_T_DEFINED
#ifndef _VMA_T_DEFINED
#define _VMA_T_DEFINED
typedef struct vma {
    tak_bertanda32_t magic;         /* Magic number untuk validasi */
    struct vma *next;               /* VMA berikutnya */
    struct vma *prev;               /* VMA sebelumnya */
    alamat_virtual_t start;         /* Alamat awal */
    alamat_virtual_t end;           /* Alamat akhir (eksklusif) */
    tak_bertanda32_t flags;         /* Flag VMA */
    tak_bertanda32_t protection;    /* Proteksi halaman */
    alamat_fisik_t phys_base;       /* Basis alamat fisik (untuk mapping) */
    ukuran_t size;                  /* Ukuran region */
    tak_bertanda32_t ref_count;     /* Reference count */
} vma_t;
#endif /* _VMA_T_DEFINED */

/* Virtual memory descriptor untuk process */
#undef _VM_DESCRIPTOR_T_DEFINED
#ifndef _VM_DESCRIPTOR_T_DEFINED
#define _VM_DESCRIPTOR_T_DEFINED
typedef struct vm_descriptor {
    vma_t *vma_list;                /* List VMA */
    vma_t *vma_free_list;           /* Free VMA descriptors */
    tak_bertanda32_t vma_count;     /* Jumlah VMA aktif */
    alamat_virtual_t brk_start;     /* Awal heap (brk) */
    alamat_virtual_t brk_current;   /* Posisi brk saat ini */
    alamat_virtual_t brk_end;       /* Akhir heap */
    alamat_virtual_t stack_start;   /* Awal stack */
    alamat_virtual_t stack_end;     /* Akhir stack */
    spinlock_t lock;                /* Lock untuk thread safety */
} vm_descriptor_t;
#endif /* _VM_DESCRIPTOR_T_DEFINED */

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Kernel virtual memory descriptor */
static vm_descriptor_t kernel_vm = {0};

/* VMA pool untuk kernel */
static vma_t kernel_vma_pool[VMA_MAX_PER_PROCESS];
static vma_t *kernel_vma_free = NULL;

/* Status */
static bool_t virtual_initialized = SALAH;

/* Statistik */
static struct {
    tak_bertanda64_t total_allocs;
    tak_bertanda64_t total_frees;
    tak_bertanda64_t alloc_failures;
    tak_bertanda64_t page_faults;
} vm_stats = {0};

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * vma_pool_init
 * -------------
 * Inisialisasi pool VMA.
 *
 * Parameter:
 *   pool - Array VMA pool
 *   count - Jumlah entri
 *   free_list - Pointer ke free list
 */
static void vma_pool_init(vma_t *pool, tak_bertanda32_t count,
                          vma_t **free_list)
{
    tak_bertanda32_t i;

    *free_list = NULL;

    for (i = 0; i < count; i++) {
        pool[i].magic = VMA_MAGIC;
        pool[i].next = *free_list;
        pool[i].prev = NULL;
        *free_list = &pool[i];
    }
}

/*
 * vma_alloc
 * ---------
 * Alokasikan deskriptor VMA.
 *
 * Parameter:
 *   vm - Pointer ke VM descriptor
 *
 * Return: Pointer ke VMA, atau NULL
 */
static vma_t *vma_alloc(vm_descriptor_t *vm)
{
    vma_t *vma;

    if (vm == NULL || vm->vma_free_list == NULL) {
        return NULL;
    }

    vma = vm->vma_free_list;
    vm->vma_free_list = vma->next;

    if (vm->vma_free_list != NULL) {
        vm->vma_free_list->prev = NULL;
    }

    vma->next = NULL;
    vma->prev = NULL;
    vma->start = 0;
    vma->end = 0;
    vma->flags = VMA_FLAG_NONE;
    vma->protection = 0;
    vma->phys_base = 0;
    vma->size = 0;
    vma->ref_count = 1;

    return vma;
}

/*
 * vma_free
 * --------
 * Bebaskan deskriptor VMA.
 *
 * Parameter:
 *   vm - Pointer ke VM descriptor
 *   vma - Pointer ke VMA
 */
static void vma_free(vm_descriptor_t *vm, vma_t *vma)
{
    if (vm == NULL || vma == NULL) {
        return;
    }

    if (vma->magic != VMA_MAGIC) {
        return;
    }

    vma->next = vm->vma_free_list;
    vma->prev = NULL;

    if (vm->vma_free_list != NULL) {
        vm->vma_free_list->prev = vma;
    }

    vm->vma_free_list = vma;
}

/*
 * vma_insert
 * ----------
 * Masukkan VMA ke list (sorted by address).
 *
 * Parameter:
 *   vm - Pointer ke VM descriptor
 *   vma - Pointer ke VMA
 */
static void vma_insert(vm_descriptor_t *vm, vma_t *vma)
{
    vma_t *curr;

    if (vm == NULL || vma == NULL) {
        return;
    }

    /* List kosong */
    if (vm->vma_list == NULL) {
        vm->vma_list = vma;
        vma->next = NULL;
        vma->prev = NULL;
        vm->vma_count++;
        return;
    }

    /* Cari posisi yang tepat */
    curr = vm->vma_list;

    while (curr != NULL && curr->start < vma->start) {
        curr = curr->next;
    }

    if (curr == NULL) {
        /* Insert di akhir */
        vma_t *last = vm->vma_list;

        while (last->next != NULL) {
            last = last->next;
        }

        last->next = vma;
        vma->prev = last;
        vma->next = NULL;
    } else {
        /* Insert sebelum curr */
        vma->next = curr;
        vma->prev = curr->prev;

        if (curr->prev != NULL) {
            curr->prev->next = vma;
        } else {
            vm->vma_list = vma;
        }

        curr->prev = vma;
    }

    vm->vma_count++;
}

/*
 * vma_remove
 * ----------
 * Hapus VMA dari list.
 *
 * Parameter:
 *   vm - Pointer ke VM descriptor
 *   vma - Pointer ke VMA
 */
static void vma_remove(vm_descriptor_t *vm, vma_t *vma)
{
    if (vm == NULL || vma == NULL) {
        return;
    }

    if (vma->prev != NULL) {
        vma->prev->next = vma->next;
    } else {
        vm->vma_list = vma->next;
    }

    if (vma->next != NULL) {
        vma->next->prev = vma->prev;
    }

    vma->next = NULL;
    vma->prev = NULL;
    vm->vma_count--;
}

/*
 * vma_find
 * --------
 * Cari VMA yang mengandung alamat.
 *
 * Parameter:
 *   vm - Pointer ke VM descriptor
 *   addr - Alamat virtual
 *
 * Return: Pointer ke VMA, atau NULL
 */
static vma_t *vma_find(vm_descriptor_t *vm, alamat_virtual_t addr)
{
    vma_t *vma;

    if (vm == NULL) {
        return NULL;
    }

    vma = vm->vma_list;

    while (vma != NULL) {
        if (addr >= vma->start && addr < vma->end) {
            return vma;
        }

        vma = vma->next;
    }

    return NULL;
}

/*
 * vma_find_free_range
 * -------------------
 * Cari range alamat bebas.
 *
 * Parameter:
 *   vm - Pointer ke VM descriptor
 *   size - Ukuran yang dibutuhkan
 *   align - Alignment
 *   hint - Hint alamat (boleh 0)
 *
 * Return: Alamat awal, atau 0
 */
static alamat_virtual_t vma_find_free_range(vm_descriptor_t *vm,
                                            ukuran_t size,
                                            ukuran_t align,
                                            alamat_virtual_t hint)
{
    vma_t *vma;
    alamat_virtual_t start;
    alamat_virtual_t end;

    if (vm == NULL || size == 0) {
        return 0;
    }

    /* Default alignment */
    if (align == 0) {
        align = UKURAN_HALAMAN;
    }

    /* Coba mulai dari hint */
    if (hint != 0) {
        start = RATAKAN_ATAS(hint, align);
    } else {
        start = VIRT_USER_START + UKURAN_HALAMAN;
    }

    /* Cek overlap dengan VMA yang ada */
    vma = vm->vma_list;

    while (vma != NULL) {
        end = start + size;

        /* Cek overlap */
        if (start < vma->end && end > vma->start) {
            /* Pindahkan start setelah VMA ini */
            start = RATAKAN_ATAS(vma->end, align);
        }

        vma = vma->next;
    }

    /* Cek batas */
    if (start + size > VIRT_USER_END) {
        return 0;
    }

    return start;
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * virtual_init
 * ------------
 * Inisialisasi virtual memory manager.
 *
 * Return: Status operasi
 */
status_t virtual_init(void)
{
    if (virtual_initialized) {
        return STATUS_SUDAH_ADA;
    }

    /* Inisialisasi kernel VM descriptor */
    kernel_memset(&kernel_vm, 0, sizeof(vm_descriptor_t));

    /* Inisialisasi VMA pool */
    vma_pool_init(kernel_vma_pool, VMA_MAX_PER_PROCESS,
                  &kernel_vma_free);

    kernel_vm.vma_list = NULL;
    kernel_vm.vma_free_list = kernel_vma_free;
    kernel_vm.vma_count = 0;

    /* Init lock */
    spinlock_init(&kernel_vm.lock);

    /* Reset stats */
    kernel_memset(&vm_stats, 0, sizeof(vm_stats));

    virtual_initialized = BENAR;

    kernel_printf("[VM] Virtual memory manager initialized\n");

    return STATUS_BERHASIL;
}

/*
 * vm_create_address_space
 * -----------------------
 * Buat address space baru untuk proses.
 *
 * Return: Pointer ke VM descriptor, atau NULL
 */
vm_descriptor_t *vm_create_address_space(void)
{
    vm_descriptor_t *vm;
    vma_t *vma_pool;

    if (!virtual_initialized) {
        return NULL;
    }

    /* Alokasikan VM descriptor */
    vm = kmalloc(sizeof(vm_descriptor_t));
    if (vm == NULL) {
        return NULL;
    }

    /* Alokasikan VMA pool */
    vma_pool = kmalloc(sizeof(vma_t) * VMA_MAX_PER_PROCESS);
    if (vma_pool == NULL) {
        kfree(vm);
        return NULL;
    }

    /* Inisialisasi */
    kernel_memset(vm, 0, sizeof(vm_descriptor_t));
    vma_pool_init(vma_pool, VMA_MAX_PER_PROCESS, &vm->vma_free_list);

    vm->vma_list = NULL;
    vm->vma_count = 0;

    /* Set default brk area */
    vm->brk_start = 0x00400000UL;  /* 4 MB */
    vm->brk_current = vm->brk_start;
    vm->brk_end = 0x00800000UL;    /* 8 MB */

    /* Set default stack area */
    vm->stack_start = VIRT_USER_END - UKURAN_HALAMAN;
    vm->stack_end = VIRT_USER_END;

    /* Init lock */
    spinlock_init(&vm->lock);

    return vm;
}

/*
 * vm_destroy_address_space
 * ------------------------
 * Hancurkan address space.
 *
 * Parameter:
 *   vm - Pointer ke VM descriptor
 *
 * Return: Status operasi
 */
void vm_destroy_address_space(vm_descriptor_t *vm)
{
    vma_t *vma;
    vma_t *next;

    if (vm == NULL) {
        return;
    }

    spinlock_kunci(&vm->lock);

    /* Unmap semua VMA */
    vma = vm->vma_list;

    while (vma != NULL) {
        next = vma->next;

        /* Unmap halaman */
        {
            alamat_virtual_t addr;

            for (addr = vma->start; addr < vma->end;
                 addr += UKURAN_HALAMAN) {
                paging_unmap_page(addr);
            }
        }

        vma = next;
    }

    /* Bebaskan VMA pool (diasumsikan kmalloc) */
    /* VMA pool pertama adalah awal array */
    if (vm->vma_free_list != NULL) {
        /* Diasumsikan vma_pool dialokasi sebagai satu blok */
        kfree(vm->vma_free_list);
    }

    spinlock_buka(&vm->lock);

    /* Bebaskan descriptor */
    kfree(vm);
}

/*
 * vm_map
 * ------
 * Map region memori.
 *
 * Parameter:
 *   vm - Pointer ke VM descriptor (NULL untuk kernel)
 *   start - Alamat awal (0 untuk auto)
 *   size - Ukuran region
 *   prot - Proteksi (R/W/X)
 *   flags - Flag VMA
 *
 * Return: Alamat awal region, atau 0 jika gagal
 */
alamat_virtual_t vm_map(vm_descriptor_t *vm, alamat_virtual_t start,
                        ukuran_t size, tak_bertanda32_t prot,
                        tak_bertanda32_t flags)
{
    vm_descriptor_t *target_vm;
    vma_t *vma;
    alamat_virtual_t vstart;
    alamat_virtual_t vend;
    alamat_fisik_t phys;
    tak_bertanda32_t i;
    tak_bertanda32_t pages;

    if (!virtual_initialized || size == 0) {
        return 0;
    }

    /* Pilih target VM */
    target_vm = (vm != NULL) ? vm : &kernel_vm;

    /* Align size */
    size = RATAKAN_ATAS(size, UKURAN_HALAMAN);
    pages = size / UKURAN_HALAMAN;

    spinlock_kunci(&target_vm->lock);

    /* Cari alamat bebas jika tidak ditentukan */
    if (start == 0) {
        start = vma_find_free_range(target_vm, size, UKURAN_HALAMAN, 0);
        if (start == 0) {
            spinlock_buka(&target_vm->lock);
            vm_stats.alloc_failures++;
            return 0;
        }
    } else {
        /* Align start */
        start = RATAKAN_BAWAH(start, UKURAN_HALAMAN);

        /* Cek apakah region bebas */
        vend = start + size;

        vma = target_vm->vma_list;

        while (vma != NULL) {
            if (start < vma->end && vend > vma->start) {
                /* Overlap */
                spinlock_buka(&target_vm->lock);
                vm_stats.alloc_failures++;
                return 0;
            }

            vma = vma->next;
        }
    }

    vstart = start;
    vend = start + size;

    /* Alokasikan VMA descriptor */
    vma = vma_alloc(target_vm);
    if (vma == NULL) {
        spinlock_buka(&target_vm->lock);
        vm_stats.alloc_failures++;
        return 0;
    }

    vma->start = vstart;
    vma->end = vend;
    vma->flags = flags;
    vma->protection = prot;
    vma->size = size;

    /* Alokasikan halaman fisik dan map */
    for (i = 0; i < pages; i++) {
        alamat_virtual_t vaddr = vstart + (i * UKURAN_HALAMAN);

        /* Alokasikan halaman fisik */
        phys = pmm_alloc_page();
        if (phys == 0) {
            /* Rollback */
            tak_bertanda32_t j;

            for (j = 0; j < i; j++) {
                alamat_virtual_t va = vstart + (j * UKURAN_HALAMAN);
                alamat_fisik_t p = paging_get_physical(va);

                paging_unmap_page(va);

                if (p != 0) {
                    pmm_free_page(p);
                }
            }

            vma_free(target_vm, vma);
            spinlock_buka(&target_vm->lock);
            vm_stats.alloc_failures++;
            return 0;
        }

        /* Map halaman */
        {
            tak_bertanda32_t pte_flags = 0x01;  /* Present */

            if (prot & 0x02) {
                pte_flags |= 0x02;  /* Writable */
            }

            if (prot & 0x04) {
                pte_flags |= 0x04;  /* User */
            }

            paging_map_page(vaddr, phys, pte_flags);
        }
    }

    /* Insert VMA ke list */
    vma_insert(target_vm, vma);

    vm_stats.total_allocs++;

    spinlock_buka(&target_vm->lock);

    return vstart;
}

/*
 * vm_unmap
 * --------
 * Unmap region memori.
 *
 * Parameter:
 *   vm - Pointer ke VM descriptor (NULL untuk kernel)
 *   start - Alamat awal
 *   size - Ukuran region
 *
 * Return: Status operasi
 */
status_t vm_unmap(vm_descriptor_t *vm, alamat_virtual_t start,
                  ukuran_t size)
{
    vm_descriptor_t *target_vm;
    vma_t *vma;
    tak_bertanda32_t i;
    tak_bertanda32_t pages;

    if (!virtual_initialized || size == 0) {
        return STATUS_PARAM_INVALID;
    }

    /* Pilih target VM */
    target_vm = (vm != NULL) ? vm : &kernel_vm;

    /* Align */
    start = RATAKAN_BAWAH(start, UKURAN_HALAMAN);
    size = RATAKAN_ATAS(size, UKURAN_HALAMAN);
    pages = size / UKURAN_HALAMAN;

    spinlock_kunci(&target_vm->lock);

    /* Cari VMA */
    vma = vma_find(target_vm, start);
    if (vma == NULL) {
        spinlock_buka(&target_vm->lock);
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Unmap halaman */
    for (i = 0; i < pages; i++) {
        alamat_virtual_t vaddr = start + (i * UKURAN_HALAMAN);
        alamat_fisik_t phys = paging_get_physical(vaddr);

        paging_unmap_page(vaddr);

        if (phys != 0) {
            pmm_free_page(phys);
        }
    }

    /* Hapus VMA jika seluruh region di-unmap */
    if (start == vma->start && size == vma->size) {
        vma_remove(target_vm, vma);
        vma_free(target_vm, vma);
    } else if (start == vma->start) {
        /* Shrink dari awal */
        vma->start = start + size;
        vma->size -= size;
    } else if (start + size == vma->end) {
        /* Shrink dari akhir */
        vma->end = start;
        vma->size -= size;
    } else {
        /* Split VMA - untuk sekarang, hanya unmap halaman */
        /* TODO: Implementasi split VMA */
    }

    vm_stats.total_frees++;

    spinlock_buka(&target_vm->lock);

    return STATUS_BERHASIL;
}

/*
 * vm_brk
 * -------
 * Ubah break address (heap).
 *
 * Parameter:
 *   vm - Pointer ke VM descriptor
 *   addr - Alamat break baru (0 untuk query)
 *
 * Return: Alamat break saat ini
 */
alamat_virtual_t vm_brk(vm_descriptor_t *vm, alamat_virtual_t addr)
{
    vm_descriptor_t *target_vm;
    alamat_virtual_t old_brk;

    if (!virtual_initialized) {
        return 0;
    }

    target_vm = (vm != NULL) ? vm : &kernel_vm;

    spinlock_kunci(&target_vm->lock);

    old_brk = target_vm->brk_current;

    if (addr == 0) {
        spinlock_buka(&target_vm->lock);
        return old_brk;
    }

    /* Validasi range */
    if (addr < target_vm->brk_start || addr > target_vm->brk_end) {
        spinlock_buka(&target_vm->lock);
        return old_brk;
    }

    /* Expand atau shrink heap */
    if (addr > old_brk) {
        /* Expand - alokasikan halaman baru */
        alamat_virtual_t vaddr;
        alamat_fisik_t phys;

        for (vaddr = RATAKAN_ATAS(old_brk, UKURAN_HALAMAN);
             vaddr < addr;
             vaddr += UKURAN_HALAMAN) {
            /* Cek apakah sudah di-map */
            if (paging_is_mapped(vaddr)) {
                continue;
            }

            phys = pmm_alloc_page();
            if (phys == 0) {
                break;
            }

            paging_map_page(vaddr, phys, 0x03);  /* RW */
        }
    }

    target_vm->brk_current = addr;

    spinlock_buka(&target_vm->lock);

    return addr;
}

/*
 * vm_handle_page_fault
 * --------------------
 * Handler untuk page fault.
 *
 * Parameter:
 *   addr - Alamat yang menyebabkan fault
 *   error - Error code
 *   vm - Pointer ke VM descriptor
 *
 * Return: BENAR jika berhasil dihandle
 */
bool_t vm_handle_page_fault(alamat_virtual_t addr,
                            tak_bertanda32_t error,
                            vm_descriptor_t *vm)
{
    vm_descriptor_t *target_vm;
    vma_t *vma;
    alamat_fisik_t phys;

    if (!virtual_initialized) {
        return SALAH;
    }

    vm_stats.page_faults++;

    target_vm = (vm != NULL) ? vm : &kernel_vm;

    spinlock_kunci(&target_vm->lock);

    /* Cari VMA yang mengandung alamat */
    vma = vma_find(target_vm, addr);
    if (vma == NULL) {
        spinlock_buka(&target_vm->lock);
        return SALAH;
    }

    /* Cek permission */
    if ((error & 0x02) && !(vma->protection & 0x02)) {
        /* Write fault tapi tidak writable */
        spinlock_buka(&target_vm->lock);
        return SALAH;
    }

    /* Cek apakah halaman sudah di-map */
    if (paging_is_mapped(RATAKAN_BAWAH(addr, UKURAN_HALAMAN))) {
        /* Sudah di-map, mungkin protection fault */
        spinlock_buka(&target_vm->lock);
        return SALAH;
    }

    /* Alokasikan halaman baru */
    phys = pmm_alloc_page();
    if (phys == 0) {
        spinlock_buka(&target_vm->lock);
        return SALAH;
    }

    /* Map halaman */
    {
        tak_bertanda32_t flags = 0x01;  /* Present */

        if (vma->protection & 0x02) {
            flags |= 0x02;  /* Writable */
        }

        if (vma->protection & 0x04) {
            flags |= 0x04;  /* User */
        }

        paging_map_page(RATAKAN_BAWAH(addr, UKURAN_HALAMAN),
                        phys, flags);
    }

    spinlock_buka(&target_vm->lock);

    return BENAR;
}

/*
 * vm_get_stats
 * ------------
 * Dapatkan statistik VM.
 *
 * Parameter:
 *   allocs - Pointer untuk jumlah alokasi
 *   frees - Pointer untuk jumlah free
 *   faults - Pointer untuk jumlah page fault
 */
void vm_get_stats(tak_bertanda64_t *allocs, tak_bertanda64_t *frees,
                  tak_bertanda64_t *faults)
{
    if (allocs != NULL) {
        *allocs = vm_stats.total_allocs;
    }

    if (frees != NULL) {
        *frees = vm_stats.total_frees;
    }

    if (faults != NULL) {
        *faults = vm_stats.page_faults;
    }
}

/*
 * vm_print_stats
 * --------------
 * Print statistik VM.
 */
void vm_print_stats(void)
{
    kernel_printf("\n[VM] Virtual Memory Statistics:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Total allocations: %lu\n", vm_stats.total_allocs);
    kernel_printf("  Total frees:       %lu\n", vm_stats.total_frees);
    kernel_printf("  Alloc failures:    %lu\n", vm_stats.alloc_failures);
    kernel_printf("  Page faults:       %lu\n", vm_stats.page_faults);
    kernel_printf("========================================\n");
}
