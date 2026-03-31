/*
 * PIGURA OS - KMAP.C
 * ------------------
 * Implementasi kernel virtual memory mapping.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk mapping
 * alamat fisik ke alamat virtual kernel, termasuk temporary
 * mappings dan permanent mappings.
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

/* Area kmap */
#define KMAP_START              0xF0000000UL
#define KMAP_END                0xFFC00000UL
#define KMAP_SIZE               (KMAP_END - KMAP_START)
#define KMAP_PAGES              (KMAP_SIZE / UKURAN_HALAMAN)

/* Fixed mapping area */
#define KMAP_FIXED_START        0xFFC00000UL
#define KMAP_FIXED_END          0xFFE00000UL

/* PKMAP area (persistent kernel mappings) */
#define PKMAP_START             0xFFE00000UL
#define PKMAP_END               0xFFF00000UL
#define PKMAP_COUNT             ((PKMAP_END - PKMAP_START) / UKURAN_HALAMAN)

/* Jumlah temporary mappings */
#define KM_TYPE_COUNT           16
#define KM_TEMP_COUNT           32

/* Flag kmap */
#define KMAP_FLAG_NONE          0x00
#define KMAP_FLAG_INUSE         0x01
#define KMAP_FLAG_PERMANENT     0x02
#define KMAP_FLAG_WIRED         0x04

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* Kmap entry */
typedef struct {
    tak_bertanda32_t flags;         /* Status entry */
    alamat_fisik_t phys;            /* Alamat fisik */
    tak_bertanda32_t ref_count;     /* Reference count */
} kmap_entry_t;

/* Temporary mapping context */
typedef struct {
    tak_bertanda32_t index;         /* Index dalam kmap area */
    tak_bertanda32_t cpu;           /* CPU ID */
} km_temp_ctx_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Kmap tracking */
static kmap_entry_t kmap_table[KMAP_PAGES];

/* PKMAP tracking */
static kmap_entry_t pkmap_table[PKMAP_COUNT];

/* Kmap allocation bitmap */
static tak_bertanda32_t kmap_bitmap[KMAP_PAGES / 32];

/* PKMAP allocation bitmap */
static tak_bertanda32_t pkmap_bitmap[PKMAP_COUNT / 32];

/* Temporary mapping slots per CPU */
static km_temp_ctx_t km_temp_slots[KM_TEMP_COUNT];

/* Status */
static bool_t kmap_initialized = SALAH;

/* Kmap lock */
static spinlock_t kmap_lock;

/* Next free index */
static tak_bertanda32_t kmap_next_free = 0;
static tak_bertanda32_t pkmap_next_free = 0;

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
static inline bool_t bitmap_test(tak_bertanda32_t *bm, tak_bertanda32_t index)
{
    return (bm[index / 32] & (1UL << (index % 32))) ? BENAR : SALAH;
}

/*
 * bitmap_set
 * ----------
 * Set bit dalam bitmap.
 */
static inline void bitmap_set(tak_bertanda32_t *bm, tak_bertanda32_t index)
{
    bm[index / 32] |= (1UL << (index % 32));
}

/*
 * bitmap_clear
 * ------------
 * Clear bit dalam bitmap.
 */
static inline void bitmap_clear(tak_bertanda32_t *bm, tak_bertanda32_t index)
{
    bm[index / 32] &= ~(1UL << (index % 32));
}

/*
 * kmap_vaddr_from_index
 * ---------------------
 * Konversi index ke alamat virtual kmap.
 */
static inline alamat_virtual_t kmap_vaddr_from_index(tak_bertanda32_t index)
{
    return KMAP_START + (index * UKURAN_HALAMAN);
}

/*
 * pkmap_vaddr_from_index
 * ----------------------
 * Konversi index ke alamat virtual pkmap.
 */
static inline alamat_virtual_t pkmap_vaddr_from_index(tak_bertanda32_t index)
{
    return PKMAP_START + (index * UKURAN_HALAMAN);
}

/*
 * find_free_kmap_slot
 * -------------------
 * Cari slot kmap kosong.
 *
 * Return: Index slot, atau -1 jika penuh
 */
static tanda32_t find_free_kmap_slot(void)
{
    tak_bertanda32_t i;
    tak_bertanda32_t start;

    start = kmap_next_free;

    for (i = 0; i < KMAP_PAGES; i++) {
        tak_bertanda32_t index = (start + i) % KMAP_PAGES;

        if (!bitmap_test(kmap_bitmap, index)) {
            kmap_next_free = (index + 1) % KMAP_PAGES;
            return (tanda32_t)index;
        }
    }

    return -1;
}

/*
 * find_free_pkmap_slot
 * --------------------
 * Cari slot pkmap kosong.
 *
 * Return: Index slot, atau -1 jika penuh
 */
static tanda32_t find_free_pkmap_slot(void)
{
    tak_bertanda32_t i;
    tak_bertanda32_t start;

    start = pkmap_next_free;

    for (i = 0; i < PKMAP_COUNT; i++) {
        tak_bertanda32_t index = (start + i) % PKMAP_COUNT;

        if (!bitmap_test(pkmap_bitmap, index)) {
            pkmap_next_free = (index + 1) % PKMAP_COUNT;
            return (tanda32_t)index;
        }
    }

    return -1;
}

/*
 * do_kmap
 * -------
 * Lakukan mapping.
 *
 * Parameter:
 *   phys  - Alamat fisik
 *   vaddr - Alamat virtual
 *   flags - Flag mapping
 *
 * Return: Status operasi
 */
static status_t do_kmap(alamat_fisik_t phys, alamat_virtual_t vaddr,
                        tak_bertanda32_t flags)
{
    tak_bertanda32_t pte_flags;

    /* Align ke halaman */
    phys = RATAKAN_BAWAH(phys, UKURAN_HALAMAN);

    /* Siapkan flags */
    pte_flags = 0x01;  /* Present */
    pte_flags |= 0x02; /* Writable */
    pte_flags |= 0x04; /* User accessible (untuk beberapa kasus) */

    if (flags & KMAP_FLAG_WIRED) {
        /* Wired mapping - cache disabled */
        pte_flags |= 0x10;  /* Cache disable */
    }

    /* Lakukan mapping */
    return paging_map_page(vaddr, phys, pte_flags);
}

/*
 * do_kunmap
 * ---------
 * Lakukan unmapping.
 *
 * Parameter:
 *   vaddr - Alamat virtual
 *
 * Return: Status operasi
 */
static status_t do_kunmap(alamat_virtual_t vaddr)
{
    return paging_unmap_page(vaddr);
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * kmap_init
 * ---------
 * Inisialisasi kmap system.
 *
 * Return: Status operasi
 */
status_t kmap_init(void)
{
    tak_bertanda32_t i;

    if (kmap_initialized) {
        return STATUS_SUDAH_ADA;
    }

    /* Clear tracking tables */
    for (i = 0; i < KMAP_PAGES; i++) {
        kmap_table[i].flags = KMAP_FLAG_NONE;
        kmap_table[i].phys = 0;
        kmap_table[i].ref_count = 0;
    }

    for (i = 0; i < PKMAP_COUNT; i++) {
        pkmap_table[i].flags = KMAP_FLAG_NONE;
        pkmap_table[i].phys = 0;
        pkmap_table[i].ref_count = 0;
    }

    /* Clear bitmaps */
    for (i = 0; i < (KMAP_PAGES / 32); i++) {
        kmap_bitmap[i] = 0;
    }

    for (i = 0; i < (PKMAP_COUNT / 32); i++) {
        pkmap_bitmap[i] = 0;
    }

    /* Clear temp slots */
    for (i = 0; i < KM_TEMP_COUNT; i++) {
        km_temp_slots[i].index = -1;
        km_temp_slots[i].cpu = 0;
    }

    /* Init lock */
    spinlock_init(&kmap_lock);

    kmap_next_free = 0;
    pkmap_next_free = 0;
    kmap_initialized = BENAR;

    kernel_printf("[KMAP] Kernel mapping area initialized\n");
    kernel_printf("       KMAP: 0x%08lX - 0x%08lX (%lu MB)\n",
                  KMAP_START, KMAP_END, KMAP_SIZE / (1024 * 1024));
    kernel_printf("       PKMAP: 0x%08lX - 0x%08lX (%lu pages)\n",
                  PKMAP_START, PKMAP_END, PKMAP_COUNT);

    return STATUS_BERHASIL;
}

/*
 * kmap
 * ----
 * Map halaman fisik ke alamat virtual kernel.
 *
 * Parameter:
 *   phys - Alamat fisik (tidak harus aligned)
 *
 * Return: Alamat virtual yang di-map, atau NULL jika gagal
 */
void *kmap(alamat_fisik_t phys)
{
    tanda32_t index;
    alamat_virtual_t vaddr;
    alamat_fisik_t page_phys;

    if (!kmap_initialized) {
        return NULL;
    }

    /* Align ke halaman */
    page_phys = RATAKAN_BAWAH(phys, UKURAN_HALAMAN);

    spinlock_kunci(&kmap_lock);

    /* Cari slot kosong */
    index = find_free_kmap_slot();
    if (index < 0) {
        spinlock_buka(&kmap_lock);
        kernel_printf("[KMAP] ERROR: No free kmap slots\n");
        return NULL;
    }

    /* Set bitmap */
    bitmap_set(kmap_bitmap, (tak_bertanda32_t)index);

    /* Track mapping */
    kmap_table[index].flags = KMAP_FLAG_INUSE;
    kmap_table[index].phys = page_phys;
    kmap_table[index].ref_count = 1;

    spinlock_buka(&kmap_lock);

    /* Hitung alamat virtual */
    vaddr = kmap_vaddr_from_index((tak_bertanda32_t)index);

    /* Lakukan mapping */
    if (do_kmap(page_phys, vaddr, KMAP_FLAG_NONE) != STATUS_BERHASIL) {
        /* Rollback */
        spinlock_kunci(&kmap_lock);
        bitmap_clear(kmap_bitmap, (tak_bertanda32_t)index);
        kmap_table[index].flags = KMAP_FLAG_NONE;
        spinlock_buka(&kmap_lock);
        return NULL;
    }

    /* Return alamat dengan offset */
    return (void *)(uintptr_t)(vaddr + (phys & 0xFFF));
}

/*
 * kunmap
 * ------
 * Unmap alamat virtual kernel.
 *
 * Parameter:
 *   vaddr - Alamat virtual yang di-map
 *
 * Return: Status operasi
 */
status_t kunmap(void *vaddr)
{
    tak_bertanda32_t index;
    alamat_virtual_t va;

    if (!kmap_initialized || vaddr == NULL) {
        return STATUS_PARAM_INVALID;
    }

    va = (alamat_virtual_t)vaddr;

    /* Cek range */
    if (va < KMAP_START || va >= KMAP_END) {
        return STATUS_PARAM_INVALID;
    }

    /* Hitung index */
    index = (va - KMAP_START) / UKURAN_HALAMAN;

    spinlock_kunci(&kmap_lock);

    /* Cek apakah di-map */
    if (!(kmap_table[index].flags & KMAP_FLAG_INUSE)) {
        spinlock_buka(&kmap_lock);
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Decrement ref count */
    kmap_table[index].ref_count--;

    if (kmap_table[index].ref_count == 0) {
        /* Unmap */
        do_kunmap(kmap_vaddr_from_index(index));

        /* Clear tracking */
        bitmap_clear(kmap_bitmap, index);
        kmap_table[index].flags = KMAP_FLAG_NONE;
        kmap_table[index].phys = 0;
    }

    spinlock_buka(&kmap_lock);

    return STATUS_BERHASIL;
}

/*
 * kmap_permanent
 * --------------
 * Map halaman fisik secara permanen.
 *
 * Parameter:
 *   phys - Alamat fisik
 *
 * Return: Alamat virtual, atau NULL
 */
void *kmap_permanent(alamat_fisik_t phys)
{
    tanda32_t index;
    alamat_virtual_t vaddr;
    alamat_fisik_t page_phys;

    if (!kmap_initialized) {
        return NULL;
    }

    page_phys = RATAKAN_BAWAH(phys, UKURAN_HALAMAN);

    spinlock_kunci(&kmap_lock);

    /* Cek apakah sudah di-map */
    for (index = 0; index < (tanda32_t)PKMAP_COUNT; index++) {
        if ((pkmap_table[index].flags & KMAP_FLAG_INUSE) &&
            pkmap_table[index].phys == page_phys) {
            /* Sudah di-map, increment ref count */
            pkmap_table[index].ref_count++;
            vaddr = pkmap_vaddr_from_index((tak_bertanda32_t)index);
            spinlock_buka(&kmap_lock);
            return (void *)(uintptr_t)(vaddr + (phys & 0xFFF));
        }
    }

    /* Cari slot kosong */
    index = find_free_pkmap_slot();
    if (index < 0) {
        spinlock_buka(&kmap_lock);
        kernel_printf("[KMAP] ERROR: No free pkmap slots\n");
        return NULL;
    }

    /* Set bitmap */
    bitmap_set(pkmap_bitmap, (tak_bertanda32_t)index);

    /* Track mapping */
    pkmap_table[index].flags = KMAP_FLAG_INUSE | KMAP_FLAG_PERMANENT;
    pkmap_table[index].phys = page_phys;
    pkmap_table[index].ref_count = 1;

    spinlock_buka(&kmap_lock);

    /* Hitung alamat virtual */
    vaddr = pkmap_vaddr_from_index((tak_bertanda32_t)index);

    /* Lakukan mapping */
    if (do_kmap(page_phys, vaddr, KMAP_FLAG_NONE) != STATUS_BERHASIL) {
        spinlock_kunci(&kmap_lock);
        bitmap_clear(pkmap_bitmap, (tak_bertanda32_t)index);
        pkmap_table[index].flags = KMAP_FLAG_NONE;
        spinlock_buka(&kmap_lock);
        return NULL;
    }

    return (void *)(uintptr_t)(vaddr + (phys & 0xFFF));
}

/*
 * kunmap_permanent
 * ----------------
 * Unmap halaman permanen.
 *
 * Parameter:
 *   vaddr - Alamat virtual
 *
 * Return: Status operasi
 */
status_t kunmap_permanent(void *vaddr)
{
    tak_bertanda32_t index;
    alamat_virtual_t va;

    if (!kmap_initialized || vaddr == NULL) {
        return STATUS_PARAM_INVALID;
    }

    va = (alamat_virtual_t)vaddr;

    /* Cek range */
    if (va < PKMAP_START || va >= PKMAP_END) {
        return STATUS_PARAM_INVALID;
    }

    /* Hitung index */
    index = (va - PKMAP_START) / UKURAN_HALAMAN;

    spinlock_kunci(&kmap_lock);

    if (!(pkmap_table[index].flags & KMAP_FLAG_INUSE)) {
        spinlock_buka(&kmap_lock);
        return STATUS_TIDAK_DITEMUKAN;
    }

    pkmap_table[index].ref_count--;

    if (pkmap_table[index].ref_count == 0) {
        do_kunmap(pkmap_vaddr_from_index(index));
        bitmap_clear(pkmap_bitmap, index);
        pkmap_table[index].flags = KMAP_FLAG_NONE;
        pkmap_table[index].phys = 0;
    }

    spinlock_buka(&kmap_lock);

    return STATUS_BERHASIL;
}

/*
 * kmap_atomic
 * -----------
 * Map halaman secara atomic (untuk ISR/context).
 *
 * Parameter:
 *   phys - Alamat fisik
 *
 * Return: Alamat virtual, atau NULL
 */
void *kmap_atomic(alamat_fisik_t phys)
{
    tak_bertanda32_t index;
    alamat_virtual_t vaddr;
    alamat_fisik_t page_phys;
    tak_bertanda32_t cpu = 0;  /* TODO: Get actual CPU ID */

    if (!kmap_initialized) {
        return NULL;
    }

    page_phys = RATAKAN_BAWAH(phys, UKURAN_HALAMAN);

    /* Gunakan fixed slot untuk atomic mapping */
    index = (cpu * KM_TYPE_COUNT) % KMAP_PAGES;

    spinlock_kunci(&kmap_lock);

    /* Cek apakah slot tersedia */
    if (kmap_table[index].flags & KMAP_FLAG_INUSE) {
        /* Slot digunakan, perlu flush */
        do_kunmap(kmap_vaddr_from_index(index));
    }

    /* Set tracking */
    bitmap_set(kmap_bitmap, index);
    kmap_table[index].flags = KMAP_FLAG_INUSE;
    kmap_table[index].phys = page_phys;
    kmap_table[index].ref_count = 1;

    spinlock_buka(&kmap_lock);

    /* Map */
    vaddr = kmap_vaddr_from_index(index);
    do_kmap(page_phys, vaddr, KMAP_FLAG_WIRED);

    return (void *)(uintptr_t)(vaddr + (phys & 0xFFF));
}

/*
 * kunmap_atomic
 * -------------
 * Unmap atomic mapping.
 *
 * Parameter:
 *   vaddr - Alamat virtual
 */
void kunmap_atomic(void *vaddr)
{
    /* Untuk atomic mapping, kita tidak langsung unmap
       tapi menunggu sampai diperlukan atau explicit flush */
    /* Untuk sekarang, lakukan unmap langsung */
    kunmap(vaddr);
}

/*
 * kmap_get_phys
 * -------------
 * Dapatkan alamat fisik dari alamat virtual kmap.
 *
 * Parameter:
 *   vaddr - Alamat virtual
 *
 * Return: Alamat fisik, atau 0 jika tidak ditemukan
 */
alamat_fisik_t kmap_get_phys(alamat_virtual_t vaddr)
{
    tak_bertanda32_t index;

    if (!kmap_initialized) {
        return 0;
    }

    /* Cek range */
    if (vaddr >= KMAP_START && vaddr < KMAP_END) {
        index = (vaddr - KMAP_START) / UKURAN_HALAMAN;
        return kmap_table[index].phys + (vaddr & 0xFFF);
    }

    if (vaddr >= PKMAP_START && vaddr < PKMAP_END) {
        index = (vaddr - PKMAP_START) / UKURAN_HALAMAN;
        return pkmap_table[index].phys + (vaddr & 0xFFF);
    }

    return 0;
}

/*
 * kmap_flush_tlb
 * --------------
 * Flush TLB untuk area kmap.
 */
void kmap_flush_tlb(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < KMAP_PAGES; i++) {
        cpu_invlpg((void *)kmap_vaddr_from_index(i));
    }

    for (i = 0; i < PKMAP_COUNT; i++) {
        cpu_invlpg((void *)pkmap_vaddr_from_index(i));
    }
}

/*
 * kmap_get_stats
 * --------------
 * Dapatkan statistik kmap.
 *
 * Parameter:
 *   kmap_used - Pointer untuk kmap slots terpakai
 *   pkmap_used - Pointer untuk pkmap slots terpakai
 */
void kmap_get_stats(tak_bertanda32_t *kmap_used, tak_bertanda32_t *pkmap_used)
{
    tak_bertanda32_t i;
    tak_bertanda32_t kcount = 0;
    tak_bertanda32_t pcount = 0;

    if (!kmap_initialized) {
        if (kmap_used != NULL) {
            *kmap_used = 0;
        }
        if (pkmap_used != NULL) {
            *pkmap_used = 0;
        }
        return;
    }

    for (i = 0; i < KMAP_PAGES; i++) {
        if (kmap_table[i].flags & KMAP_FLAG_INUSE) {
            kcount++;
        }
    }

    for (i = 0; i < PKMAP_COUNT; i++) {
        if (pkmap_table[i].flags & KMAP_FLAG_INUSE) {
            pcount++;
        }
    }

    if (kmap_used != NULL) {
        *kmap_used = kcount;
    }

    if (pkmap_used != NULL) {
        *pkmap_used = pcount;
    }
}

/*
 * kmap_print_stats
 * ----------------
 * Print statistik kmap.
 */
void kmap_print_stats(void)
{
    tak_bertanda32_t kmap_used, pkmap_used;

    kmap_get_stats(&kmap_used, &pkmap_used);

    kernel_printf("\n[KMAP] Kernel Mapping Statistics:\n");
    kernel_printf("========================================\n");
    kernel_printf("  KMAP:   %lu/%lu slots used\n", kmap_used, KMAP_PAGES);
    kernel_printf("  PKMAP:  %lu/%lu slots used\n", pkmap_used, PKMAP_COUNT);
    kernel_printf("========================================\n");
}
