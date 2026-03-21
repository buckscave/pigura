/*
 * PIGURA OS - PML4 x86_64
 * -----------------------
 * Implementasi Page Map Level 4 untuk arsitektur x86_64.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk manajemen
 * PML4 (4-level paging) pada arsitektur x86_64.
 *
 * Arsitektur: x86_64 (64-bit)
 * Versi: 1.0
 */

#include "../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA PAGING
 * ============================================================================
 */

/* Jumlah entri per level */
#define PML4_ENTRI              512
#define PDPT_ENTRI              512
#define PD_ENTRI                512
#define PT_ENTRI                512

/* Flag page table */
#define PT_ADA                  0x01
#define PT_TULIS                0x02
#define PT_USER                 0x04
#define PT_PWT                  0x08
#define PT_PCD                  0x10
#define PT_AKSES                0x20
#define PT_BESAR                0x80
#define PT_NX                   (1ULL << 63)

/* Shift untuk setiap level */
#define PML4_SHIFT              39
#define PDPT_SHIFT              30
#define PD_SHIFT                21
#define PT_SHIFT                12

/* Mask */
#define PML4_MASK               0x1FF
#define PDPT_MASK               0x1FF
#define PD_MASK                 0x1FF
#define PT_MASK                 0x1FF
#define OFFSET_MASK             0xFFF

/*
 * ============================================================================
 * STRUKTUR DATA
 * ============================================================================
 */

/* Entry PML4 */
struct pml4e {
    tak_bertanda64_t ada: 1;
    tak_bertanda64_t tulis: 1;
    tak_bertanda64_t user: 1;
    tak_bertanda64_t pwt: 1;
    tak_bertanda64_t pcd: 1;
    tak_bertanda64_t akses: 1;
    tak_bertanda64_t reserv: 1;
    tak_bertanda64_t besar: 1;
    tak_bertanda64_t global: 1;
    tak_bertanda64_t avail: 3;
    tak_bertanda64_t alamat: 40;
    tak_bertanda64_t nx: 1;
    tak_bertanda64_t reserved: 11;
} __attribute__((packed));

/* Entry PDPT */
struct pdpte {
    tak_bertanda64_t ada: 1;
    tak_bertanda64_t tulis: 1;
    tak_bertanda64_t user: 1;
    tak_bertanda64_t pwt: 1;
    tak_bertanda64_t pcd: 1;
    tak_bertanda64_t akses: 1;
    tak_bertanda64_t reserv: 1;
    tak_bertanda64_t besar: 1;
    tak_bertanda64_t global: 1;
    tak_bertanda64_t avail: 3;
    tak_bertanda64_t alamat: 40;
    tak_bertanda64_t nx: 1;
    tak_bertanda64_t reserved: 11;
} __attribute__((packed));

/* Entry PD */
struct pde {
    tak_bertanda64_t ada: 1;
    tak_bertanda64_t tulis: 1;
    tak_bertanda64_t user: 1;
    tak_bertanda64_t pwt: 1;
    tak_bertanda64_t pcd: 1;
    tak_bertanda64_t akses: 1;
    tak_bertanda64_t reserv: 1;
    tak_bertanda64_t besar: 1;
    tak_bertanda64_t global: 1;
    tak_bertanda64_t avail: 3;
    tak_bertanda64_t alamat: 40;
    tak_bertanda64_t nx: 1;
    tak_bertanda64_t reserved: 11;
} __attribute__((packed));

/* Entry PT */
struct pte {
    tak_bertanda64_t ada: 1;
    tak_bertanda64_t tulis: 1;
    tak_bertanda64_t user: 1;
    tak_bertanda64_t pwt: 1;
    tak_bertanda64_t pcd: 1;
    tak_bertanda64_t akses: 1;
    tak_bertanda64_t kotor: 1;
    tak_bertanda64_t pat: 1;
    tak_bertanda64_t global: 1;
    tak_bertanda64_t avail: 3;
    tak_bertanda64_t alamat: 40;
    tak_bertanda64_t nx: 1;
    tak_bertanda64_t reserved: 11;
} __attribute__((packed));

/*
 * ============================================================================
 * VARIABEL GLOBAL
 * ============================================================================
 */

/* PML4 kernel */
static struct pml4e *g_pml4_kernel = NULL;

/* Flag inisialisasi */
static bool_t g_pml4_diinisialisasi = SALAH;

/*
 * ============================================================================
 * FUNGSI INTERNAL
 * ============================================================================
 */

/*
 * _get_pml4_index
 * ---------------
 * Mendapatkan index PML4 dari alamat virtual.
 *
 * Parameter:
 *   addr - Alamat virtual
 *
 * Return:
 *   Index PML4
 */
static tak_bertanda64_t _get_pml4_index(tak_bertanda64_t addr)
{
    return (addr >> PML4_SHIFT) & PML4_MASK;
}

/*
 * _get_pdpt_index
 * ---------------
 * Mendapatkan index PDPT dari alamat virtual.
 *
 * Parameter:
 *   addr - Alamat virtual
 *
 * Return:
 *   Index PDPT
 */
static tak_bertanda64_t _get_pdpt_index(tak_bertanda64_t addr)
{
    return (addr >> PDPT_SHIFT) & PDPT_MASK;
}

/*
 * _get_pd_index
 * -------------
 * Mendapatkan index PD dari alamat virtual.
 *
 * Parameter:
 *   addr - Alamat virtual
 *
 * Return:
 *   Index PD
 */
static tak_bertanda64_t _get_pd_index(tak_bertanda64_t addr)
{
    return (addr >> PD_SHIFT) & PD_MASK;
}

/*
 * _get_pt_index
 * -------------
 * Mendapatkan index PT dari alamat virtual.
 *
 * Parameter:
 *   addr - Alamat virtual
 *
 * Return:
 *   Index PT
 */
static tak_bertanda64_t _get_pt_index(tak_bertanda64_t addr)
{
    return (addr >> PT_SHIFT) & PT_MASK;
}

/*
 * _get_offset
 * -----------
 * Mendapatkan offset dalam halaman.
 *
 * Parameter:
 *   addr - Alamat virtual
 *
 * Return:
 *   Offset
 */
static tak_bertanda64_t _get_offset(tak_bertanda64_t addr)
{
    return addr & OFFSET_MASK;
}

/*
 * _entry_to_addr
 * --------------
 * Mengkonversi entry ke alamat fisik.
 *
 * Parameter:
 *   entry - Nilai entry
 *
 * Return:
 *   Alamat fisik
 */
static tak_bertanda64_t _entry_to_addr(tak_bertanda64_t entry)
{
    return entry & 0x000FFFFFFFFFF000ULL;
}

/*
 * ============================================================================
 * FUNGSI PUBLIC
 * ============================================================================
 */

/*
 * pml4_x86_64_init
 * ----------------
 * Inisialisasi subsistem PML4.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t pml4_x86_64_init(void)
{
    /* Baca CR3 untuk mendapatkan PML4 kernel */
    g_pml4_kernel = (struct pml4e *)cpu_read_cr3();

    if (g_pml4_kernel == NULL) {
        kernel_printf("[PML4-x86_64] Error: PML4 kernel tidak ditemukan\n");
        return STATUS_GAGAL;
    }

    g_pml4_diinisialisasi = BENAR;

    kernel_printf("[PML4-x86_64] PML4 kernel di 0x%016X\n",
                  (tak_bertanda64_t)g_pml4_kernel);

    return STATUS_BERHASIL;
}

/*
 * pml4_x86_64_buat
 * ----------------
 * Membuat PML4 baru.
 *
 * Return:
 *   Alamat PML4 baru, atau 0 jika gagal
 */
tak_bertanda64_t pml4_x86_64_buat(void)
{
    struct pml4e *pml4;
    alamat_fisik_t pml4_fisik;
    tak_bertanda32_t i;

    /* Alokasikan halaman untuk PML4 */
    pml4_fisik = hal_memory_alloc_page();
    if (pml4_fisik == ALAMAT_FISIK_INVALID) {
        return 0;
    }

    /* Clear PML4 */
    pml4 = (struct pml4e *)pml4_fisik;
    kernel_memset(pml4, 0, UKURAN_HALAMAN);

    /* Copy kernel mapping (entry 511) dari PML4 kernel */
    if (g_pml4_kernel != NULL) {
        pml4[511] = g_pml4_kernel[511];
    }

    return (tak_bertanda64_t)pml4_fisik;
}

/*
 * pml4_x86_64_hancurkan
 * ---------------------
 * Menghancurkan PML4.
 *
 * Parameter:
 *   pml4_addr - Alamat PML4
 */
void pml4_x86_64_hancurkan(tak_bertanda64_t pml4_addr)
{
    if (pml4_addr == 0) {
        return;
    }

    hal_memory_free_page((alamat_fisik_t)pml4_addr);
}

/*
 * pml4_x86_64_map
 * ---------------
 * Memetakan alamat virtual ke fisik.
 *
 * Parameter:
 *   pml4_addr - Alamat PML4
 *   virt      - Alamat virtual
 *   fisik     - Alamat fisik
 *   flags     - Flag mapping
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t pml4_x86_64_map(tak_bertanda64_t pml4_addr,
                          tak_bertanda64_t virt,
                          tak_bertanda64_t fisik,
                          tak_bertanda64_t flags)
{
    struct pml4e *pml4;
    struct pdpte *pdpt;
    struct pde *pd;
    struct pte *pt;
    tak_bertanda64_t pml4_idx;
    tak_bertanda64_t pdpt_idx;
    tak_bertanda64_t pd_idx;
    tak_bertanda64_t pt_idx;
    alamat_fisik_t halaman;

    if (pml4_addr == 0) {
        return STATUS_PARAM_INVALID;
    }

    pml4 = (struct pml4e *)pml4_addr;

    pml4_idx = _get_pml4_index(virt);
    pdpt_idx = _get_pdpt_index(virt);
    pd_idx = _get_pd_index(virt);
    pt_idx = _get_pt_index(virt);

    /* Level 1: PML4 */
    if (!pml4[pml4_idx].ada) {
        /* Alokasikan PDPT baru */
        halaman = hal_memory_alloc_page();
        if (halaman == ALAMAT_FISIK_INVALID) {
            return STATUS_MEMORI_HABIS;
        }

        pdpt = (struct pdpte *)halaman;
        kernel_memset(pdpt, 0, UKURAN_HALAMAN);

        pml4[pml4_idx].ada = 1;
        pml4[pml4_idx].tulis = (flags & PT_TULIS) ? 1 : 0;
        pml4[pml4_idx].user = (flags & PT_USER) ? 1 : 0;
        pml4[pml4_idx].alamat = halaman >> 12;
    }

    pdpt = (struct pdpte *)(pml4[pml4_idx].alamat << 12);

    /* Level 2: PDPT */
    if (!pdpt[pdpt_idx].ada) {
        /* Alokasikan PD baru */
        halaman = hal_memory_alloc_page();
        if (halaman == ALAMAT_FISIK_INVALID) {
            return STATUS_MEMORI_HABIS;
        }

        pd = (struct pde *)halaman;
        kernel_memset(pd, 0, UKURAN_HALAMAN);

        pdpt[pdpt_idx].ada = 1;
        pdpt[pdpt_idx].tulis = (flags & PT_TULIS) ? 1 : 0;
        pdpt[pdpt_idx].user = (flags & PT_USER) ? 1 : 0;
        pdpt[pdpt_idx].alamat = halaman >> 12;
    }

    pd = (struct pde *)(pdpt[pdpt_idx].alamat << 12);

    /* Level 3: PD */
    if (!pd[pd_idx].ada) {
        /* Alokasikan PT baru */
        halaman = hal_memory_alloc_page();
        if (halaman == ALAMAT_FISIK_INVALID) {
            return STATUS_MEMORI_HABIS;
        }

        pt = (struct pte *)halaman;
        kernel_memset(pt, 0, UKURAN_HALAMAN);

        pd[pd_idx].ada = 1;
        pd[pd_idx].tulis = (flags & PT_TULIS) ? 1 : 0;
        pd[pd_idx].user = (flags & PT_USER) ? 1 : 0;
        pd[pd_idx].alamat = halaman >> 12;
    }

    pt = (struct pte *)(pd[pd_idx].alamat << 12);

    /* Level 4: PT - set mapping */
    pt[pt_idx].ada = 1;
    pt[pt_idx].tulis = (flags & PT_TULIS) ? 1 : 0;
    pt[pt_idx].user = (flags & PT_USER) ? 1 : 0;
    pt[pt_idx].nx = (flags & PT_NX) ? 1 : 0;
    pt[pt_idx].alamat = fisik >> 12;

    /* Invalidate TLB */
    hal_cpu_invalidate_tlb((void *)virt);

    return STATUS_BERHASIL;
}

/*
 * pml4_x86_64_unmap
 * -----------------
 * Menghapus mapping.
 *
 * Parameter:
 *   pml4_addr - Alamat PML4
 *   virt      - Alamat virtual
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t pml4_x86_64_unmap(tak_bertanda64_t pml4_addr,
                            tak_bertanda64_t virt)
{
    struct pml4e *pml4;
    struct pdpte *pdpt;
    struct pde *pd;
    struct pte *pt;
    tak_bertanda64_t pml4_idx;
    tak_bertanda64_t pdpt_idx;
    tak_bertanda64_t pd_idx;
    tak_bertanda64_t pt_idx;

    if (pml4_addr == 0) {
        return STATUS_PARAM_INVALID;
    }

    pml4 = (struct pml4e *)pml4_addr;

    pml4_idx = _get_pml4_index(virt);
    pdpt_idx = _get_pdpt_index(virt);
    pd_idx = _get_pd_index(virt);
    pt_idx = _get_pt_index(virt);

    /* Traverse page tables */
    if (!pml4[pml4_idx].ada) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    pdpt = (struct pdpte *)(pml4[pml4_idx].alamat << 12);
    if (!pdpt[pdpt_idx].ada) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    pd = (struct pde *)(pdpt[pdpt_idx].alamat << 12);
    if (!pd[pd_idx].ada) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    pt = (struct pte *)(pd[pd_idx].alamat << 12);

    /* Clear entry */
    kernel_memset(&pt[pt_idx], 0, sizeof(struct pte));

    /* Invalidate TLB */
    hal_cpu_invalidate_tlb((void *)virt);

    return STATUS_BERHASIL;
}

/*
 * pml4_x86_64_get_fisik
 * ---------------------
 * Mendapatkan alamat fisik dari virtual.
 *
 * Parameter:
 *   pml4_addr - Alamat PML4
 *   virt      - Alamat virtual
 *
 * Return:
 *   Alamat fisik, atau 0 jika tidak di-map
 */
tak_bertanda64_t pml4_x86_64_get_fisik(tak_bertanda64_t pml4_addr,
                                        tak_bertanda64_t virt)
{
    struct pml4e *pml4;
    struct pdpte *pdpt;
    struct pde *pd;
    struct pte *pt;
    tak_bertanda64_t pml4_idx;
    tak_bertanda64_t pdpt_idx;
    tak_bertanda64_t pd_idx;
    tak_bertanda64_t pt_idx;

    if (pml4_addr == 0) {
        return 0;
    }

    pml4 = (struct pml4e *)pml4_addr;

    pml4_idx = _get_pml4_index(virt);
    pdpt_idx = _get_pdpt_index(virt);
    pd_idx = _get_pd_index(virt);
    pt_idx = _get_pt_index(virt);

    /* Traverse page tables */
    if (!pml4[pml4_idx].ada) {
        return 0;
    }

    pdpt = (struct pdpte *)(pml4[pml4_idx].alamat << 12);
    if (!pdpt[pdpt_idx].ada) {
        return 0;
    }

    pd = (struct pde *)(pdpt[pdpt_idx].alamat << 12);
    if (!pd[pd_idx].ada) {
        return 0;
    }

    pt = (struct pte *)(pd[pd_idx].alamat << 12);
    if (!pt[pt_idx].ada) {
        return 0;
    }

    return (pt[pt_idx].alamat << 12) | _get_offset(virt);
}

/*
 * pml4_x86_64_switch
 * ------------------
 * Beralih ke PML4 tertentu.
 *
 * Parameter:
 *   pml4_addr - Alamat PML4
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t pml4_x86_64_switch(tak_bertanda64_t pml4_addr)
{
    if (pml4_addr == 0) {
        return STATUS_PARAM_INVALID;
    }

    cpu_write_cr3(pml4_addr);

    return STATUS_BERHASIL;
}

/*
 * pml4_x86_64_apakah_siap
 * -----------------------
 * Mengecek apakah PML4 sudah diinisialisasi.
 *
 * Return:
 *   BENAR jika sudah, SALAH jika belum
 */
bool_t pml4_x86_64_apakah_siap(void)
{
    return g_pml4_diinisialisasi;
}
