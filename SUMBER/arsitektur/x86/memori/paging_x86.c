/*
 * PIGURA OS - PAGING x86
 * ----------------------
 * Implementasi sistem paging untuk arsitektur x86.
 *
 * Berkas ini berisi implementasi lengkap sistem paging 32-bit
 * dengan dukungan untuk 4 KB pages dan 4 MB large pages.
 *
 * Arsitektur: x86 (32-bit)
 * Versi: 1.1
 */

#include "../../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA PAGING
 * ============================================================================
 */

/* Ukuran tabel */
#define PDE_JUMLAH              1024
#define PTE_JUMLAH              1024

/* Flag PDE/PTE */
#define PAGING_ADA              0x00000001
#define PAGING_TULIS            0x00000002
#define PAGING_USER             0x00000004
#define PAGING_PWT              0x00000008
#define PAGING_PCD              0x00000010
#define PAGING_AKSES            0x00000020
#define PAGING_KOTOR            0x00000040
#define PAGING_BESAR            0x00000080
#define PAGING_GLOBAL           0x00000100

/* Mask alamat */
#define PAGING_MASK_4KB         0xFFFFF000
#define PAGING_MASK_4MB         0xFFC00000
#define PAGING_OFFSET_4KB       0x00000FFF
#define PAGING_OFFSET_4MB       0x003FFFFF

/* Shift bit */
#define PAGING_SHIFT_4KB        12
#define PAGING_SHIFT_4MB        22
#define PAGING_SHIFT_DIR        22

/* Alamat kernel - menggunakan konstanta dari konstanta.h */
#define KERNEL_VIRT_BASE        ALAMAT_KERNEL_MULAI
#define KERNEL_FISIK_BASE       0x00100000UL  /* 1 MB */

/*
 * ============================================================================
 * STRUKTUR DATA
 * ============================================================================
 */

/* Page Directory Entry (4 byte) */
typedef struct {
    tak_bertanda32_t ada: 1;
    tak_bertanda32_t tulis: 1;
    tak_bertanda32_t user: 1;
    tak_bertanda32_t pwt: 1;
    tak_bertanda32_t pcd: 1;
    tak_bertanda32_t akses: 1;
    tak_bertanda32_t reserv: 1;
    tak_bertanda32_t besar: 1;
    tak_bertanda32_t global: 1;
    tak_bertanda32_t avail: 3;
    tak_bertanda32_t alamat: 20;
} pde_t;

/* Page Table Entry (4 byte) */
typedef struct {
    tak_bertanda32_t ada: 1;
    tak_bertanda32_t tulis: 1;
    tak_bertanda32_t user: 1;
    tak_bertanda32_t pwt: 1;
    tak_bertanda32_t pcd: 1;
    tak_bertanda32_t akses: 1;
    tak_bertanda32_t kotor: 1;
    tak_bertanda32_t pat: 1;
    tak_bertanda32_t global: 1;
    tak_bertanda32_t avail: 3;
    tak_bertanda32_t alamat: 20;
} pte_t;

/* Struktur address space */
struct paging_address_space {
    pde_t *page_directory;
    alamat_fisik_t pd_fisik;
    tak_bertanda32_t ref_count;
};

/*
 * ============================================================================
 * VARIABEL GLOBAL
 * ============================================================================
 */

/* Page directory kernel */
static pde_t g_kernel_pd[PDE_JUMLAH]
    __attribute__((aligned(UKURAN_HALAMAN)));

/* Page table untuk identity mapping */
static pte_t g_identity_pt[PTE_JUMLAH]
    __attribute__((aligned(UKURAN_HALAMAN)));

/* Page table untuk kernel */
static pte_t g_kernel_pt[PTE_JUMLAH]
    __attribute__((aligned(UKURAN_HALAMAN)));

/* Address space kernel */
static struct paging_address_space g_kernel_as;

/* Current page directory */
static pde_t *g_current_pd = NULL;

/*
 * ============================================================================
 * FUNGSI INTERNAL
 * ============================================================================
 */

/*
 * _buat_pde_4kb
 * -------------
 * Membuat PDE untuk 4 KB page.
 */
static tak_bertanda32_t _buat_pde_4kb(tak_bertanda32_t pt_fisik,
                                       bool_t user, bool_t tulis)
{
    pde_t pde;

    kernel_memset(&pde, 0, sizeof(pde));

    pde.ada = 1;
    pde.tulis = tulis ? 1 : 0;
    pde.user = user ? 1 : 0;
    pde.besar = 0;
    pde.alamat = pt_fisik >> PAGING_SHIFT_4KB;

    { tak_bertanda32_t val; kernel_memcpy(&val, &pde, sizeof(val)); return val; }
}

/*
 * _buat_pde_4mb
 * -------------
 * Membuat PDE untuk 4 MB large page.
 */
static tak_bertanda32_t _buat_pde_4mb(tak_bertanda32_t addr_fisik,
                                       bool_t user, bool_t tulis)
{
    pde_t pde;

    kernel_memset(&pde, 0, sizeof(pde));

    pde.ada = 1;
    pde.tulis = tulis ? 1 : 0;
    pde.user = user ? 1 : 0;
    pde.besar = 1;
    pde.alamat = addr_fisik >> PAGING_SHIFT_4KB;

    { tak_bertanda32_t val; kernel_memcpy(&val, &pde, sizeof(val)); return val; }
}

/*
 * _buat_pte
 * ---------
 * Membuat PTE.
 */
static tak_bertanda32_t _buat_pte(tak_bertanda32_t addr_fisik,
                                   bool_t user, bool_t tulis)
{
    pte_t pte;

    kernel_memset(&pte, 0, sizeof(pte));

    pte.ada = 1;
    pte.tulis = tulis ? 1 : 0;
    pte.user = user ? 1 : 0;
    pte.alamat = addr_fisik >> PAGING_SHIFT_4KB;

    { tak_bertanda32_t val; kernel_memcpy(&val, &pte, sizeof(val)); return val; }
}

/*
 * _baca_pde
 * ---------
 * Membaca PDE dari page directory.
 */
static tak_bertanda32_t _baca_pde(pde_t *pd, tak_bertanda32_t idx)
{
    if (pd == NULL || idx >= PDE_JUMLAH) {
        return 0;
    }
    return *(tak_bertanda32_t *)&pd[idx];
}

/*
 * _tulis_pde
 * ----------
 * Menulis PDE ke page directory.
 */
static void _tulis_pde(pde_t *pd, tak_bertanda32_t idx,
                        tak_bertanda32_t nilai)
{
    if (pd == NULL || idx >= PDE_JUMLAH) {
        return;
    }
    *(tak_bertanda32_t *)&pd[idx] = nilai;
}

/*
 * _baca_pte
 * ---------
 * Membaca PTE dari page table.
 */
static tak_bertanda32_t _baca_pte(pte_t *pt, tak_bertanda32_t idx)
{
    if (pt == NULL || idx >= PTE_JUMLAH) {
        return 0;
    }
    return *(tak_bertanda32_t *)&pt[idx];
}

/*
 * _tulis_pte
 * ----------
 * Menulis PTE ke page table.
 */
static void _tulis_pte(pte_t *pt, tak_bertanda32_t idx,
                        tak_bertanda32_t nilai)
{
    if (pt == NULL || idx >= PTE_JUMLAH) {
        return;
    }
    *(tak_bertanda32_t *)&pt[idx] = nilai;
}

/*
 * _get_pt_virtual
 * ---------------
 * Mendapatkan alamat virtual page table dari PDE.
 */
static pte_t *_get_pt_virtual(tak_bertanda32_t pde_val)
{
    pde_t *pde = (pde_t *)&pde_val;
    tak_bertanda32_t pt_fisik;
    tak_bertanda32_t pt_virt;

    if (!pde->ada || pde->besar) {
        return NULL;
    }

    pt_fisik = (tak_bertanda32_t)pde->alamat << PAGING_SHIFT_4KB;

    /* Konversi ke virtual (higher half) */
    pt_virt = pt_fisik + KERNEL_VIRT_BASE - KERNEL_FISIK_BASE;

    return (pte_t *)(uintptr_t)pt_virt;
}

/*
 * _aktifkan_pse
 * -------------
 * Mengaktifkan Page Size Extension untuk 4 MB pages.
 */
static void _aktifkan_pse(void)
{
    tak_bertanda32_t cr4;

    __asm__ __volatile__(
        "mov %%cr4, %0\n\t"
        "or $0x10, %0\n\t"
        "mov %0, %%cr4\n\t"
        : "=r"(cr4)
    );
    (void)cr4;  /* Avoid unused warning */
}

/*
 * _load_cr3
 * ---------
 * Memuat page directory ke CR3.
 */
static void _load_cr3(tak_bertanda32_t pd_fisik)
{
    cpu_write_cr3(pd_fisik);
}

/*
 * _enable_paging
 * --------------
 * Mengaktifkan paging.
 */
static void _enable_paging(void)
{
    tak_bertanda32_t cr0;

    cr0 = cpu_read_cr0();
    cr0 |= 0x80000000;
    cpu_write_cr0(cr0);
}

/*
 * ============================================================================
 * FUNGSI PUBLIC
 * ============================================================================
 */

/*
 * paging_init
 * -----------
 * Menginisialisasi sistem paging.
 */
status_t paging_init(tak_bertanda64_t mem_size)
{
    tak_bertanda32_t i;
    tak_bertanda32_t pt_phys;
    tak_bertanda32_t pd_phys;

    kernel_printf("[PAGING] Menginisialisasi paging...\n");

    /* Avoid unused parameter warning */
    (void)mem_size;

    /* Clear page directory */
    kernel_memset(g_kernel_pd, 0, sizeof(g_kernel_pd));

    /* Clear page tables */
    kernel_memset(g_identity_pt, 0, sizeof(g_identity_pt));
    kernel_memset(g_kernel_pt, 0, sizeof(g_kernel_pt));

    /* Setup identity mapping (0-4 MB) */
    for (i = 0; i < PTE_JUMLAH; i++) {
        tak_bertanda32_t pte_val = _buat_pte(
            (tak_bertanda32_t)(i * UKURAN_HALAMAN), SALAH, BENAR);
        _tulis_pte(g_identity_pt, i, pte_val);
    }

    /* Get physical address of identity page table */
    pt_phys = (tak_bertanda32_t)(uintptr_t)g_identity_pt;

    /* Map identity di PDE 0 */
    _tulis_pde(g_kernel_pd, 0, _buat_pde_4kb(pt_phys, SALAH, BENAR));

    /* Setup kernel mapping (3-4 GB) */
    for (i = 0; i < PTE_JUMLAH; i++) {
        tak_bertanda32_t pte_val = _buat_pte(
            (tak_bertanda32_t)(KERNEL_FISIK_BASE + i * UKURAN_HALAMAN),
            SALAH, BENAR);
        _tulis_pte(g_kernel_pt, i, pte_val);
    }

    /* Get physical address of kernel page table */
    pt_phys = (tak_bertanda32_t)(uintptr_t)g_kernel_pt;

    /* Map kernel di PDE 768 (0xC0000000 = 3 GB) */
    _tulis_pde(g_kernel_pd, 768, _buat_pde_4kb(pt_phys, SALAH, BENAR));

    /* Get physical address of page directory */
    pd_phys = (tak_bertanda32_t)(uintptr_t)g_kernel_pd;

    /* Simpan info address space kernel */
    g_kernel_as.page_directory = g_kernel_pd;
    g_kernel_as.pd_fisik = (alamat_fisik_t)pd_phys;
    g_kernel_as.ref_count = 1;

    g_current_pd = g_kernel_pd;

    /* Aktifkan PSE */
    _aktifkan_pse();

    /* Load CR3 */
    _load_cr3(pd_phys);

    /* Aktifkan paging */
    _enable_paging();

    kernel_printf("[PAGING] Paging aktif\n");

    return STATUS_BERHASIL;
}

/*
 * paging_map_page
 * ---------------
 * Memetakan satu halaman.
 */
status_t paging_map_page(alamat_virtual_t vaddr, alamat_fisik_t paddr,
                          tak_bertanda32_t flags)
{
    tak_bertanda32_t pde_idx;
    tak_bertanda32_t pte_idx;
    tak_bertanda32_t pde_val;
    pte_t *pt;
    tak_bertanda32_t pt_fisik;
    bool_t user;
    bool_t tulis;

    /* Align address */
    vaddr &= PAGING_MASK_4KB;
    paddr &= PAGING_MASK_4KB;

    pde_idx = (tak_bertanda32_t)(vaddr >> PAGING_SHIFT_DIR);
    pte_idx = (tak_bertanda32_t)((vaddr >> PAGING_SHIFT_4KB) & 0x3FF);

    user = (flags & PAGING_USER) ? BENAR : SALAH;
    tulis = (flags & PAGING_TULIS) ? BENAR : SALAH;

    pde_val = _baca_pde(g_current_pd, pde_idx);

    /* Cek apakah page table ada */
    if (!(pde_val & PAGING_ADA)) {
        /* Alokasikan page table baru */
        pt_fisik = (tak_bertanda32_t)hal_memory_alloc_page();
        if (pt_fisik == (tak_bertanda32_t)ALAMAT_FISIK_INVALID) {
            return STATUS_MEMORI_HABIS;
        }

        /* Get virtual address */
        pt = (pte_t *)(uintptr_t)(pt_fisik + KERNEL_VIRT_BASE - KERNEL_FISIK_BASE);
        kernel_memset(pt, 0, UKURAN_HALAMAN);

        /* Update PDE */
        _tulis_pde(g_current_pd, pde_idx, _buat_pde_4kb(pt_fisik, user, tulis));
    } else {
        pt = _get_pt_virtual(pde_val);
    }

    if (pt == NULL) {
        return STATUS_GAGAL;
    }

    /* Map halaman */
    _tulis_pte(pt, pte_idx, _buat_pte((tak_bertanda32_t)paddr, user, tulis));

    /* Invalidate TLB */
    cpu_invlpg((void *)(uintptr_t)vaddr);

    return STATUS_BERHASIL;
}

/*
 * paging_unmap_page
 * -----------------
 * Menghapus mapping halaman.
 */
status_t paging_unmap_page(alamat_virtual_t vaddr)
{
    tak_bertanda32_t pde_idx;
    tak_bertanda32_t pte_idx;
    tak_bertanda32_t pde_val;
    pte_t *pt;

    vaddr &= PAGING_MASK_4KB;

    pde_idx = (tak_bertanda32_t)(vaddr >> PAGING_SHIFT_DIR);
    pte_idx = (tak_bertanda32_t)((vaddr >> PAGING_SHIFT_4KB) & 0x3FF);

    pde_val = _baca_pde(g_current_pd, pde_idx);

    if (!(pde_val & PAGING_ADA)) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    pt = _get_pt_virtual(pde_val);
    if (pt == NULL) {
        return STATUS_GAGAL;
    }

    /* Clear PTE */
    _tulis_pte(pt, pte_idx, 0);

    /* Invalidate TLB */
    cpu_invlpg((void *)(uintptr_t)vaddr);

    return STATUS_BERHASIL;
}

/*
 * paging_get_physical
 * -------------------
 * Mendapatkan alamat fisik dari alamat virtual.
 */
alamat_fisik_t paging_get_physical(alamat_virtual_t vaddr)
{
    tak_bertanda32_t pde_idx;
    tak_bertanda32_t pte_idx;
    tak_bertanda32_t pde_val;
    tak_bertanda32_t pte_val;
    pte_t *pt;
    pte_t *pte;

    pde_idx = (tak_bertanda32_t)(vaddr >> PAGING_SHIFT_DIR);
    pte_idx = (tak_bertanda32_t)((vaddr >> PAGING_SHIFT_4KB) & 0x3FF);

    pde_val = _baca_pde(g_current_pd, pde_idx);

    if (!(pde_val & PAGING_ADA)) {
        return ALAMAT_FISIK_INVALID;
    }

    /* Check for 4 MB page */
    if (pde_val & PAGING_BESAR) {
        pde_t *pde = (pde_t *)&pde_val;
        tak_bertanda32_t base = (tak_bertanda32_t)pde->alamat << 
                              PAGING_SHIFT_4KB;
        return (alamat_fisik_t)(base + (vaddr & PAGING_OFFSET_4MB));
    }

    /* 4 KB page */
    pt = _get_pt_virtual(pde_val);
    if (pt == NULL) {
        return ALAMAT_FISIK_INVALID;
    }

    pte_val = _baca_pte(pt, pte_idx);

    if (!(pte_val & PAGING_ADA)) {
        return ALAMAT_FISIK_INVALID;
    }

    pte = (pte_t *)&pte_val;
    return (alamat_fisik_t)((tak_bertanda32_t)(pte->alamat << PAGING_SHIFT_4KB) + 
           (vaddr & PAGING_OFFSET_4KB));
}

/*
 * paging_is_mapped
 * ----------------
 * Cek apakah halaman sudah di-map.
 */
bool_t paging_is_mapped(alamat_virtual_t vaddr)
{
    alamat_fisik_t paddr;

    paddr = paging_get_physical(vaddr);

    return (paddr != ALAMAT_FISIK_INVALID) ? BENAR : SALAH;
}

/*
 * paging_create_address_space
 * ---------------------------
 * Membuat address space baru.
 */
struct page_directory *paging_create_address_space(void)
{
    pde_t *pd;
    tak_bertanda32_t pd_fisik;
    tak_bertanda32_t i;

    /* Alokasikan page directory */
    pd_fisik = (tak_bertanda32_t)hal_memory_alloc_page();
    if (pd_fisik == (tak_bertanda32_t)ALAMAT_FISIK_INVALID) {
        return NULL;
    }

    pd = (pde_t *)(uintptr_t)(pd_fisik + KERNEL_VIRT_BASE - KERNEL_FISIK_BASE);
    kernel_memset(pd, 0, UKURAN_HALAMAN);

    /* Copy kernel mapping */
    for (i = 768; i < PDE_JUMLAH; i++) {
        _tulis_pde(pd, i, _baca_pde(g_kernel_pd, i));
    }

    return (struct page_directory *)pd;
}

/*
 * paging_destroy_address_space
 * ----------------------------
 * Menghancurkan address space.
 */
status_t paging_destroy_address_space(struct page_directory *dir)
{
    pde_t *pd;
    tak_bertanda32_t i;
    tak_bertanda32_t pde_val;
    tak_bertanda32_t pt_fisik;

    if (dir == NULL) {
        return STATUS_PARAM_INVALID;
    }

    pd = (pde_t *)dir;

    /* Bebaskan semua page table user */
    for (i = 0; i < 768; i++) {
        pde_val = _baca_pde(pd, i);

        if ((pde_val & PAGING_ADA) && !(pde_val & PAGING_BESAR)) {
            pde_t *pde = (pde_t *)&pde_val;
            pt_fisik = (tak_bertanda32_t)pde->alamat << PAGING_SHIFT_4KB;
            hal_memory_free_page((alamat_fisik_t)pt_fisik);
        }
    }

    /* Bebaskan page directory - convert virtual to physical */
    hal_memory_free_page((alamat_fisik_t)((tak_bertanda32_t)(uintptr_t)pd - 
                         KERNEL_VIRT_BASE + KERNEL_FISIK_BASE));

    return STATUS_BERHASIL;
}

/*
 * paging_switch_directory
 * -----------------------
 * Beralih ke page directory lain.
 */
status_t paging_switch_directory(struct page_directory *dir)
{
    pde_t *pd;
    tak_bertanda32_t pd_fisik;

    if (dir == NULL) {
        return STATUS_PARAM_INVALID;
    }

    pd = (pde_t *)dir;

    /* Konversi ke alamat fisik */
    pd_fisik = (tak_bertanda32_t)(uintptr_t)pd - KERNEL_VIRT_BASE + KERNEL_FISIK_BASE;

    /* Load ke CR3 */
    _load_cr3(pd_fisik);

    g_current_pd = pd;

    return STATUS_BERHASIL;
}

/*
 * paging_map_range
 * ----------------
 * Memetakan range alamat.
 */
status_t paging_map_range(alamat_virtual_t vaddr_start,
                           alamat_fisik_t paddr_start,
                           tak_bertanda64_t ukuran,
                           tak_bertanda32_t flags)
{
    tak_bertanda64_t offset;
    status_t status;

    /* Align ke halaman */
    vaddr_start &= PAGING_MASK_4KB;
    paddr_start &= PAGING_MASK_4KB;
    ukuran = (ukuran + UKURAN_HALAMAN - 1) & ~((tak_bertanda64_t)
             UKURAN_HALAMAN - 1);

    for (offset = 0; offset < ukuran; offset += UKURAN_HALAMAN) {
        status = paging_map_page(vaddr_start + offset,
                                  paddr_start + offset, flags);
        if (status != STATUS_BERHASIL) {
            return status;
        }
    }

    return STATUS_BERHASIL;
}

/*
 * paging_unmap_range
 * ------------------
 * Menghapus mapping range alamat.
 */
status_t paging_unmap_range(alamat_virtual_t vaddr_start,
                             tak_bertanda64_t ukuran)
{
    tak_bertanda64_t offset;
    status_t status;

    vaddr_start &= PAGING_MASK_4KB;
    ukuran = (ukuran + UKURAN_HALAMAN - 1) & ~((tak_bertanda64_t)
             UKURAN_HALAMAN - 1);

    for (offset = 0; offset < ukuran; offset += UKURAN_HALAMAN) {
        status = paging_unmap_page(vaddr_start + offset);
        if (status != STATUS_BERHASIL) {
            return status;
        }
    }

    return STATUS_BERHASIL;
}

/*
 * paging_set_flags
 * ----------------
 * Mengatur flag halaman.
 */
status_t paging_set_flags(alamat_virtual_t vaddr, tak_bertanda32_t flags)
{
    tak_bertanda32_t pde_idx;
    tak_bertanda32_t pte_idx;
    tak_bertanda32_t pde_val;
    pte_t *pt;
    alamat_fisik_t paddr;

    vaddr &= PAGING_MASK_4KB;

    pde_idx = (tak_bertanda32_t)(vaddr >> PAGING_SHIFT_DIR);
    pte_idx = (tak_bertanda32_t)((vaddr >> PAGING_SHIFT_4KB) & 0x3FF);

    pde_val = _baca_pde(g_current_pd, pde_idx);

    if (!(pde_val & PAGING_ADA)) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    pt = _get_pt_virtual(pde_val);
    if (pt == NULL) {
        return STATUS_GAGAL;
    }

    /* Ambil alamat fisik */
    paddr = paging_get_physical(vaddr);

    /* Buat PTE baru dengan flag baru */
    _tulis_pte(pt, pte_idx, _buat_pte((tak_bertanda32_t)paddr,
        (flags & PAGING_USER) ? BENAR : SALAH,
        (flags & PAGING_TULIS) ? BENAR : SALAH));

    /* Invalidate TLB */
    cpu_invlpg((void *)(uintptr_t)vaddr);

    return STATUS_BERHASIL;
}

/*
 * paging_get_page_directory
 * -------------------------
 * Mendapatkan page directory saat ini.
 */
void *paging_get_page_directory(void)
{
    return g_current_pd;
}

/*
 * paging_get_kernel_pd
 * -------------------
 * Mendapatkan page directory kernel.
 */
void *paging_get_kernel_pd(void)
{
    return g_kernel_pd;
}
