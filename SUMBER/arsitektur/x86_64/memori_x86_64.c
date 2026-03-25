/*
 * PIGURA OS - MEMORI x86_64
 * -------------------------
 * Implementasi manajemen memori untuk arsitektur x86_64.
 *
 * Berkas ini berisi implementasi fungsi-fungsi manajemen memori
 * yang spesifik untuk arsitektur x86_64, termasuk deteksi memori,
 * inisialisasi physical memory manager, dan setup paging 4-level.
 *
 * Arsitektur: x86_64 (64-bit)
 * Versi: 1.0
 */

#include "../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA MEMORI
 * ============================================================================
 */

/* Alamat kernel untuk x86_64 (higher half) */
#define KERNEL_MULAI_VIRT       0xFFFFFFFF80000000ULL
#define KERNEL_MULAI_FISIK      0x00100000ULL

/* Alamat awal untuk identity mapping */
#define IDENTITAS_MULAI         0x00000000ULL
#define IDENTITAS_AKHIR         0x00200000ULL   /* 2 MB */

/* Jumlah entri page table */
#define PML4_JUMLAH             512
#define PDPT_JUMLAH             512
#define PD_JUMLAH               512
#define PT_JUMLAH               512

/* Flag page table */
#define PAGE_ADA                0x01
#define PAGE_TULIS              0x02
#define PAGE_USER               0x04
#define PAGE_PWT                0x08
#define PAGE_PCD                0x10
#define PAGE_AKSES              0x20
#define PAGE_BESAR              0x80
#define PAGE_NX                 (1ULL << 63)

/* Flag CR0 */
#define CR0_PG                  0x80000000
#define CR0_WP                  0x00010000

/* Flag CR4 */
#define CR4_PSE                 0x00000010
#define CR4_PAE                 0x00000020
#define CR4_PGE                 0x00000080

/*
 * ============================================================================
 * STRUKTUR DATA
 * ============================================================================
 */

/* Entry PML4 (Page Map Level 4) */
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
    tak_bertanda64_t reserved: 12;
} __attribute__((packed));

/* Entry PDPT (Page Directory Pointer Table) */
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
    tak_bertanda64_t reserved: 12;
} __attribute__((packed));

/* Entry Page Directory */
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
    tak_bertanda64_t reserved: 12;
} __attribute__((packed));

/* Entry Page Table */
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

/* Memory map entry dari BIOS */
struct mmap_entry_bios {
    tak_bertanda32_t ukuran;
    tak_bertanda64_t alamat;
    tak_bertanda64_t panjang;
    tak_bertanda32_t tipe;
} __attribute__((packed));

/*
 * ============================================================================
 * VARIABEL GLOBAL
 * ============================================================================
 */

/* PML4 kernel */
static struct pml4e g_pml4[PML4_JUMLAH]
    __attribute__((aligned(UKURAN_HALAMAN)));

/* PDPT untuk identity mapping dan kernel */
static struct pdpte g_pdpt_identitas[PDPT_JUMLAH]
    __attribute__((aligned(UKURAN_HALAMAN)));
static struct pdpte g_pdpt_kernel[PDPT_JUMLAH]
    __attribute__((aligned(UKURAN_HALAMAN)));

/* Page Directory untuk identity mapping dan kernel */
static struct pde g_pd_identitas[PD_JUMLAH]
    __attribute__((aligned(UKURAN_HALAMAN)));
static struct pde g_pd_kernel[PD_JUMLAH]
    __attribute__((aligned(UKURAN_HALAMAN)));

/* Bitmap untuk physical memory manager */
static tak_bertanda64_t *g_bitmap_memori = NULL;
static tak_bertanda64_t g_jumlah_halaman = 0;
static tak_bertanda64_t g_halaman_bebas = 0;
static tak_bertanda64_t g_total_memori = 0;

/* Alamat awal heap kernel */
static alamat_fisik_t g_heap_mulai = 0;

/*
 * ============================================================================
 * FUNGSI INTERNAL - DETEKSI MEMORI
 * ============================================================================
 */

/*
 * _hitung_memori_total
 * --------------------
 * Menghitung total memori dari memory map multiboot.
 *
 * Parameter:
 *   mbi - Pointer ke multiboot info
 *
 * Return:
 *   Total memori dalam byte
 */
static tak_bertanda64_t _hitung_memori_total(multiboot_info_t *mbi)
{
    tak_bertanda64_t total = 0;

    if (mbi == NULL) {
        return 0;
    }

    /* Gunakan info memori dari multiboot */
    if (mbi->flags & MULTIBOOT_FLAG_MEM) {
        total = (tak_bertanda64_t)mbi->mem_lower * 1024;
        total += (tak_bertanda64_t)mbi->mem_upper * 1024;
    }

    /* Jika ada mmap, gunakan itu */
    if (mbi->flags & MULTIBOOT_FLAG_MMAP) {
        tak_bertanda32_t i;
        tak_bertanda32_t len;
        tak_bertanda8_t *ptr;
        struct mmap_entry_bios *entry;

        len = mbi->mmap_length;
        ptr = (tak_bertanda8_t *)(uintptr_t)mbi->mmap_addr;
        total = 0;

        for (i = 0; i < len; ) {
            entry = (struct mmap_entry_bios *)ptr;

            if (entry->tipe == MMAP_TYPE_RAM) {
                total += entry->panjang;
            }

            ptr += entry->ukuran + 4;
            i += entry->ukuran + 4;

            if (i >= len) {
                break;
            }
        }
    }

    return total;
}

/*
 * ============================================================================
 * FUNGSI INTERNAL - PAGING
 * ============================================================================
 */

/*
 * _setup_identity_mapping
 * -----------------------
 * Setup identity mapping untuk 0-2 MB menggunakan 2 MB huge pages.
 */
static void _setup_identity_mapping(void)
{
    tak_bertanda64_t alamat;
    tak_bertanda32_t i;

    /* Clear PDPT identitas */
    kernel_memset(g_pdpt_identitas, 0, sizeof(g_pdpt_identitas));

    /* Clear PD identitas */
    kernel_memset(g_pd_identitas, 0, sizeof(g_pd_identitas));

    /* Setup PD entry dengan 2 MB huge pages */
    for (i = 0; i < PD_JUMLAH; i++) {
        alamat = i * 0x200000ULL; /* 2 MB */

        g_pd_identitas[i].ada = 1;
        g_pd_identitas[i].tulis = 1;
        g_pd_identitas[i].user = 0;
        g_pd_identitas[i].besar = 1;    /* 2 MB page */
        g_pd_identitas[i].alamat = alamat >> 12;

        /* Hanya map first 2 MB untuk awal */
        if (i > 0) {
            g_pd_identitas[i].ada = 0;
        }
    }

    /* Setup PDPT entry 0 -> PD identitas */
    g_pdpt_identitas[0].ada = 1;
    g_pdpt_identitas[0].tulis = 1;
    g_pdpt_identitas[0].user = 0;
    g_pdpt_identitas[0].alamat = ((tak_bertanda64_t)g_pd_identitas) >> 12;
}

/*
 * _setup_kernel_mapping
 * ---------------------
 * Setup mapping untuk kernel di higher half.
 */
static void _setup_kernel_mapping(void)
{
    tak_bertanda64_t alamat;
    tak_bertanda32_t i;

    /* Clear PDPT kernel */
    kernel_memset(g_pdpt_kernel, 0, sizeof(g_pdpt_kernel));

    /* Clear PD kernel */
    kernel_memset(g_pd_kernel, 0, sizeof(g_pd_kernel));

    /* Setup PD entries untuk kernel (1 MB - 2 MB fisik -> higher half) */
    /* Kernel berada di PML4 entry 511, PDPT entry 510 */
    for (i = 0; i < PD_JUMLAH; i++) {
        alamat = KERNEL_MULAI_FISIK + (i * 0x200000ULL);

        g_pd_kernel[i].ada = 1;
        g_pd_kernel[i].tulis = 1;
        g_pd_kernel[i].user = 0;
        g_pd_kernel[i].besar = 1;
        g_pd_kernel[i].alamat = alamat >> 12;
    }

    /* Setup PDPT entry 510 -> PD kernel */
    g_pdpt_kernel[510].ada = 1;
    g_pdpt_kernel[510].tulis = 1;
    g_pdpt_kernel[510].user = 0;
    g_pdpt_kernel[510].alamat = ((tak_bertanda64_t)g_pd_kernel) >> 12;
}

/*
 * _setup_pml4
 * -----------
 * Setup PML4 entries.
 */
static void _setup_pml4(void)
{
    /* Clear PML4 */
    kernel_memset(g_pml4, 0, sizeof(g_pml4));

    /* Entry 0: Identity mapping */
    g_pml4[0].ada = 1;
    g_pml4[0].tulis = 1;
    g_pml4[0].user = 0;
    g_pml4[0].alamat = ((tak_bertanda64_t)g_pdpt_identitas) >> 12;

    /* Entry 511: Kernel higher half mapping */
    g_pml4[511].ada = 1;
    g_pml4[511].tulis = 1;
    g_pml4[511].user = 0;
    g_pml4[511].alamat = ((tak_bertanda64_t)g_pdpt_kernel) >> 12;
}

/*
 * _aktifkan_paging
 * ----------------
 * Mengaktifkan paging di CPU.
 */
static void _aktifkan_paging(void)
{
    tak_bertanda64_t cr0;
    tak_bertanda64_t cr4;
    tak_bertanda64_t cr3;

    /* Set PML4 address ke CR3 */
    cr3 = (tak_bertanda64_t)g_pml4;
    __asm__ __volatile__(
        "mov %0, %%cr3"
        :
        : "r"(cr3)
        : "memory"
    );

    /* Enable PAE, PSE, dan PGE di CR4 */
    __asm__ __volatile__(
        "mov %%cr4, %0"
        : "=r"(cr4)
    );
    cr4 |= CR4_PAE | CR4_PSE | CR4_PGE;
    __asm__ __volatile__(
        "mov %0, %%cr4"
        :
        : "r"(cr4)
        : "memory"
    );

    /* Enable paging di CR0 */
    __asm__ __volatile__(
        "mov %%cr0, %0"
        : "=r"(cr0)
    );
    cr0 |= CR0_PG | CR0_WP;
    __asm__ __volatile__(
        "mov %0, %%cr0"
        :
        : "r"(cr0)
        : "memory"
    );
}

/*
 * ============================================================================
 * FUNGSI INTERNAL - PHYSICAL MEMORY MANAGER
 * ============================================================================
 */

/*
 * _init_bitmap_memori
 * -------------------
 * Inisialisasi bitmap untuk physical memory manager.
 *
 * Parameter:
 *   total_mem - Total memori dalam byte
 */
static void _init_bitmap_memori(tak_bertanda64_t total_mem)
{
    tak_bertanda64_t bitmap_size;
    tak_bertanda32_t i;

    /* Hitung jumlah halaman */
    g_jumlah_halaman = total_mem / UKURAN_HALAMAN;
    g_halaman_bebas = g_jumlah_halaman;

    /* Hitung ukuran bitmap (1 bit per halaman) */
    bitmap_size = (g_jumlah_halaman + 63) / 64 * 8;
    bitmap_size = (bitmap_size + UKURAN_HALAMAN - 1) &
                  ~(UKURAN_HALAMAN - 1);

    /* Bitmap ditempatkan setelah kernel */
    g_bitmap_memori = (tak_bertanda64_t *)
        (KERNEL_MULAI_VIRT + 0x200000);

    /* Clear bitmap (semua bebas) */
    kernel_memset(g_bitmap_memori, 0, (ukuran_t)bitmap_size);

    /* Tandai halaman 0-1 MB sebagai used (BIOS, etc) */
    for (i = 0; i < 256; i++) {
        tak_bertanda64_t idx = i / 64;
        tak_bertanda64_t bit = i % 64;
        g_bitmap_memori[idx] |= (1ULL << bit);
        g_halaman_bebas--;
    }

    /* Tandai halaman kernel sebagai used */
    for (i = 256; i < 512; i++) {
        tak_bertanda64_t idx = i / 64;
        tak_bertanda64_t bit = i % 64;
        g_bitmap_memori[idx] |= (1ULL << bit);
        g_halaman_bebas--;
    }
}

/*
 * _alokasi_halaman
 * ----------------
 * Mengalokasikan satu halaman fisik.
 *
 * Return:
 *   Alamat fisik halaman, atau 0 jika gagal
 */
static alamat_fisik_t _alokasi_halaman(void)
{
    tak_bertanda64_t i;
    tak_bertanda64_t j;

    if (g_halaman_bebas == 0 || g_bitmap_memori == NULL) {
        return 0;
    }

    /* Cari halaman bebas */
    for (i = 0; i < (g_jumlah_halaman + 63) / 64; i++) {
        if (g_bitmap_memori[i] != 0xFFFFFFFFFFFFFFFFULL) {
            /* Ada bit yang 0 */
            for (j = 0; j < 64; j++) {
                if (!(g_bitmap_memori[i] & (1ULL << j))) {
                    /* Tandai sebagai used */
                    g_bitmap_memori[i] |= (1ULL << j);
                    g_halaman_bebas--;

                    return (alamat_fisik_t)((i * 64 + j) *
                            UKURAN_HALAMAN);
                }
            }
        }
    }

    return 0;
}

/*
 * _bebaskan_halaman
 * -----------------
 * Membebaskan satu halaman fisik.
 *
 * Parameter:
 *   addr - Alamat fisik halaman
 */
static void _bebaskan_halaman(alamat_fisik_t addr)
{
    tak_bertanda64_t nomor_halaman;
    tak_bertanda64_t idx;
    tak_bertanda64_t bit;

    if (addr == 0 || g_bitmap_memori == NULL) {
        return;
    }

    nomor_halaman = addr / UKURAN_HALAMAN;
    idx = nomor_halaman / 64;
    bit = nomor_halaman % 64;

    /* Clear bit */
    g_bitmap_memori[idx] &= ~(1ULL << bit);
    g_halaman_bebas++;
}

/*
 * ============================================================================
 * FUNGSI PUBLIC
 * ============================================================================
 */

/*
 * memori_x86_64_init
 * ------------------
 * Inisialisasi subsistem memori x86_64.
 *
 * Parameter:
 *   mbi - Pointer ke multiboot info
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t memori_x86_64_init(multiboot_info_t *mbi)
{
    kernel_printf("[MEM-x86_64] Menginisialisasi manajemen memori...\n");

    /* Hitung total memori */
    g_total_memori = _hitung_memori_total(mbi);
    kernel_printf("[MEM-x86_64] Total memori: %u MB\n",
                  (tak_bertanda32_t)(g_total_memori / (1024 * 1024)));

    /* Clear page tables */
    kernel_memset(g_pml4, 0, sizeof(g_pml4));
    kernel_memset(g_pdpt_identitas, 0, sizeof(g_pdpt_identitas));
    kernel_memset(g_pdpt_kernel, 0, sizeof(g_pdpt_kernel));
    kernel_memset(g_pd_identitas, 0, sizeof(g_pd_identitas));
    kernel_memset(g_pd_kernel, 0, sizeof(g_pd_kernel));

    /* Setup paging */
    kernel_printf("[MEM-x86_64] Setup identity mapping...\n");
    _setup_identity_mapping();

    kernel_printf("[MEM-x86_64] Setup kernel mapping...\n");
    _setup_kernel_mapping();

    kernel_printf("[MEM-x86_64] Setup PML4...\n");
    _setup_pml4();

    /* Init physical memory manager */
    kernel_printf("[MEM-x86_64] Inisialisasi physical memory manager...\n");
    _init_bitmap_memori(g_total_memori);

    /* Aktifkan paging */
    kernel_printf("[MEM-x86_64] Mengaktifkan paging...\n");
    _aktifkan_paging();

    kernel_printf("[MEM-x86_64] Paging aktif\n");
    kernel_printf("[MEM-x86_64] Halaman bebas: %u (%u MB)\n",
                  (tak_bertanda32_t)g_halaman_bebas,
                  (tak_bertanda32_t)((g_halaman_bebas * UKURAN_HALAMAN) /
                                     (1024 * 1024)));

    return STATUS_BERHASIL;
}

/*
 * memori_x86_64_get_total
 * -----------------------
 * Mendapatkan total memori.
 *
 * Return:
 *   Total memori dalam byte
 */
tak_bertanda64_t memori_x86_64_get_total(void)
{
    return g_total_memori;
}

/*
 * memori_x86_64_get_bebas
 * -----------------------
 * Mendapatkan memori bebas.
 *
 * Return:
 *   Memori bebas dalam byte
 */
tak_bertanda64_t memori_x86_64_get_bebas(void)
{
    return g_halaman_bebas * UKURAN_HALAMAN;
}

/*
 * memori_x86_64_get_terpakai
 * --------------------------
 * Mendapatkan memori terpakai.
 *
 * Return:
 *   Memori terpakai dalam byte
 */
tak_bertanda64_t memori_x86_64_get_terpakai(void)
{
    return (g_jumlah_halaman - g_halaman_bebas) * UKURAN_HALAMAN;
}

/*
 * memori_x86_64_alokasi_halaman
 * -----------------------------
 * Mengalokasikan satu halaman fisik.
 *
 * Return:
 *   Alamat fisik halaman
 */
alamat_fisik_t memori_x86_64_alokasi_halaman(void)
{
    return _alokasi_halaman();
}

/*
 * memori_x86_64_bebaskan_halaman
 * ------------------------------
 * Membebaskan satu halaman fisik.
 *
 * Parameter:
 *   addr - Alamat fisik halaman
 */
void memori_x86_64_bebaskan_halaman(alamat_fisik_t addr)
{
    _bebaskan_halaman(addr);
}

/*
 * memori_x86_64_alokasi_halaman_n
 * -------------------------------
 * Mengalokasikan N halaman fisik berurutan.
 *
 * Parameter:
 *   jumlah - Jumlah halaman
 *
 * Return:
 *   Alamat fisik halaman pertama
 */
alamat_fisik_t memori_x86_64_alokasi_halaman_n(tak_bertanda32_t jumlah)
{
    tak_bertanda64_t i;
    tak_bertanda64_t j;
    tak_bertanda64_t start = 0;
    bool_t found = SALAH;

    if (jumlah == 0 || jumlah > g_halaman_bebas) {
        return 0;
    }

    /* Cari N halaman berurutan */
    for (i = 0; i < g_jumlah_halaman - jumlah + 1; i++) {
        found = BENAR;

        for (j = 0; j < jumlah; j++) {
            tak_bertanda64_t idx = (i + j) / 64;
            tak_bertanda64_t bit = (i + j) % 64;

            if (g_bitmap_memori[idx] & (1ULL << bit)) {
                found = SALAH;
                break;
            }
        }

        if (found) {
            start = i;
            break;
        }
    }

    if (!found) {
        return 0;
    }

    /* Tandai semua halaman sebagai used */
    for (i = 0; i < jumlah; i++) {
        tak_bertanda64_t idx = (start + i) / 64;
        tak_bertanda64_t bit = (start + i) % 64;
        g_bitmap_memori[idx] |= (1ULL << bit);
    }

    g_halaman_bebas -= jumlah;

    return (alamat_fisik_t)(start * UKURAN_HALAMAN);
}

/*
 * memori_x86_64_bebaskan_halaman_n
 * --------------------------------
 * Membebaskan N halaman fisik.
 *
 * Parameter:
 *   addr   - Alamat fisik halaman pertama
 *   jumlah - Jumlah halaman
 */
void memori_x86_64_bebaskan_halaman_n(alamat_fisik_t addr,
                                       tak_bertanda32_t jumlah)
{
    tak_bertanda64_t i;
    tak_bertanda64_t nomor_halaman;
    tak_bertanda64_t idx;
    tak_bertanda64_t bit;

    if (addr == 0 || jumlah == 0) {
        return;
    }

    nomor_halaman = addr / UKURAN_HALAMAN;

    for (i = 0; i < jumlah; i++) {
        idx = (nomor_halaman + i) / 64;
        bit = (nomor_halaman + i) % 64;

        g_bitmap_memori[idx] &= ~(1ULL << bit);
        g_halaman_bebas++;
    }
}

/*
 * memori_x86_64_map_halaman
 * -------------------------
 * Memetakan halaman fisik ke alamat virtual.
 *
 * Parameter:
 *   virt   - Alamat virtual
 *   fisik  - Alamat fisik
 *   flags  - Flag halaman
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t memori_x86_64_map_halaman(alamat_virtual_t virt,
                                    alamat_fisik_t fisik,
                                    tak_bertanda64_t flags)
{
    tak_bertanda64_t pml4_idx;
    tak_bertanda64_t pdpt_idx;
    tak_bertanda64_t pd_idx;
    tak_bertanda64_t pt_idx;

    pml4_idx = (virt >> 39) & 0x1FF;
    pdpt_idx = (virt >> 30) & 0x1FF;
    pd_idx = (virt >> 21) & 0x1FF;
    pt_idx = (virt >> 12) & 0x1FF;

    /* Untuk saat ini, gunakan 2 MB huge pages */
    /* TODO: Implementasi full 4-level paging */

    (void)pml4_idx;
    (void)pdpt_idx;
    (void)pd_idx;
    (void)pt_idx;
    (void)flags;
    (void)fisik;

    return STATUS_BERHASIL;
}

/*
 * memori_x86_64_unmap_halaman
 * ---------------------------
 * Menghapus mapping halaman.
 *
 * Parameter:
 *   virt - Alamat virtual
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t memori_x86_64_unmap_halaman(alamat_virtual_t virt)
{
    (void)virt;
    return STATUS_BERHASIL;
}

/*
 * memori_x86_64_get_fisik
 * -----------------------
 * Mendapatkan alamat fisik dari alamat virtual.
 *
 * Parameter:
 *   virt - Alamat virtual
 *
 * Return:
 *   Alamat fisik, atau 0 jika tidak di-map
 */
alamat_fisik_t memori_x86_64_get_fisik(alamat_virtual_t virt)
{
    (void)virt;
    return 0;
}

/*
 * memori_x86_64_get_pml4
 * ----------------------
 * Mendapatkan pointer ke PML4.
 *
 * Return:
 *   Pointer ke PML4
 */
void *memori_x86_64_get_pml4(void)
{
    return g_pml4;
}
