/*
 * PIGURA OS - MEMORI x86
 * -----------------------
 * Implementasi manajemen memori untuk arsitektur x86.
 *
 * Berkas ini berisi implementasi fungsi-fungsi manajemen memori
 * yang spesifik untuk arsitektur x86, termasuk deteksi memori,
 * inisialisasi physical memory manager, dan setup paging dasar.
 *
 * Arsitektur: x86 (32-bit)
 * Versi: 1.0
 */

#include "../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA MEMORI
 * ============================================================================
 */

/* Alamat kernel */
#define KERNEL_MULAI_VIRT       0xC0000000
#define KERNEL_MULAI_FISIK      0x00100000

/* Alamat awal untuk identity mapping */
#define IDENTITAS_MULAI         0x00000000
#define IDENTITAS_AKHIR         0x00400000

/* Jumlah page directory entry */
#define PDE_JUMLAH              1024
#define PTE_JUMLAH              1024

/* Flag page directory/table */
#define PAGE_ADA                0x01
#define PAGE_TULIS              0x02
#define PAGE_USER               0x04
#define PAGE_PWT                0x08
#define PAGE_PCD                0x10
#define PAGE_AKSES              0x20
#define PAGE_BESAR              0x80

/* Flag CR0 */
#define CR0_PG                  0x80000000
#define CR0_WP                  0x00010000

/* Flag CR4 */
#define CR4_PSE                 0x00000010
#define CR4_PAE                 0x00000020

/*
 * ============================================================================
 * STRUKTUR DATA
 * ============================================================================
 */

/* Entry page directory */
struct pde {
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
} __attribute__((packed));

/* Entry page table */
struct pte {
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

/* Page directory kernel */
static struct pde g_page_directory[PDE_JUMLAH]
    __attribute__((aligned(UKURAN_HALAMAN)));

/* Page table untuk identity mapping (0-4 MB) */
static struct pte g_page_table_identitas[PTE_JUMLAH]
    __attribute__((aligned(UKURAN_HALAMAN)));

/* Page table untuk kernel (3-4 GB) */
static struct pte g_page_table_kernel[PTE_JUMLAH]
    __attribute__((aligned(UKURAN_HALAMAN)));

/* Bitmap untuk physical memory manager */
static tak_bertanda32_t *g_bitmap_memori = NULL;
static tak_bertanda64_t g_jumlah_halaman = 0;
static tak_bertanda64_t g_halaman_bebas = 0;
static tak_bertanda64_t g_total_memori = 0;

/* Alamat awal heap kernel */
static alamat_fisik_t g_heap_mulai __attribute__((unused)) = 0;

/*
 * ============================================================================
 * FUNGSI INTERNAL - DETEKSI MEMORI
 * ============================================================================
 */

/*
 * _baca_mmap_bios
 * ---------------
 * Membaca memory map dari BIOS melalui int 0x15.
 *
 * Return:
 *   Jumlah entry memory map, atau 0 jika gagal
 */
static tak_bertanda32_t _baca_mmap_bios(struct mmap_entry_bios *buffer,
                                         tak_bertanda32_t max_entries)
{
    tak_bertanda32_t count = 0;
    tak_bertanda32_t continuation = 0;
    tak_bertanda32_t signature;
    struct mmap_entry_bios *entry;

    if (buffer == NULL || max_entries == 0) {
        return 0;
    }

    entry = buffer;

    do {
        tak_bertanda32_t ebx;
        tak_bertanda32_t ecx;
        tak_bertanda32_t edx;
        tak_bertanda32_t eax;

        __asm__ __volatile__(
            "int $0x15\n\t"
            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(0xE820), "b"(continuation), "c"(sizeof(*entry)),
              "d"(0x534D4150), "D"(entry)
            : "memory"
        );

        signature = eax;

        if (signature != 0x534D4150) {
            break;
        }

        continuation = ebx;

        if (entry->tipe == 1) {
            /* Tipe 1 = RAM usable */
            count++;
            entry++;
        }

        if (count >= max_entries) {
            break;
        }

    } while (continuation != 0);

    return count;
}

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
            entry = (struct mmap_entry_bios *)(uintptr_t)ptr;

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
 * Setup identity mapping untuk 0-4 MB.
 */
static void _setup_identity_mapping(void)
{
    tak_bertanda32_t i;
    tak_bertanda32_t alamat;

    /* Clear page table identitas */
    kernel_memset(g_page_table_identitas, 0, sizeof(g_page_table_identitas));

    /* Map 0-4 MB */
    for (i = 0; i < PTE_JUMLAH; i++) {
        alamat = i * UKURAN_HALAMAN;

        g_page_table_identitas[i].ada = 1;
        g_page_table_identitas[i].tulis = 1;
        g_page_table_identitas[i].user = 0;
        g_page_table_identitas[i].alamat = alamat >> 12;
    }

    /* Set di page directory */
    g_page_directory[0].ada = 1;
    g_page_directory[0].tulis = 1;
    g_page_directory[0].user = 0;
    g_page_directory[0].besar = 0;
    g_page_directory[0].alamat = 
        ((tak_bertanda32_t)(uintptr_t)g_page_table_identitas) >> 12;
}

/*
 * _setup_kernel_mapping
 * ---------------------
 * Setup mapping untuk kernel di higher half (3 GB+).
 */
static void _setup_kernel_mapping(void)
{
    tak_bertanda32_t i;
    tak_bertanda32_t alamat;

    /* Clear page table kernel */
    kernel_memset(g_page_table_kernel, 0, sizeof(g_page_table_kernel));

    /* Map 1-2 MB fisik ke 0xC0000000 (kernel) */
    for (i = 0; i < PTE_JUMLAH; i++) {
        alamat = KERNEL_MULAI_FISIK + (i * UKURAN_HALAMAN);

        g_page_table_kernel[i].ada = 1;
        g_page_table_kernel[i].tulis = 1;
        g_page_table_kernel[i].user = 0;
        g_page_table_kernel[i].alamat = alamat >> 12;
    }

    /* Set di page directory entry 768 (3 GB / 4 MB) */
    g_page_directory[768].ada = 1;
    g_page_directory[768].tulis = 1;
    g_page_directory[768].user = 0;
    g_page_directory[768].besar = 0;
    g_page_directory[768].alamat = 
        ((tak_bertanda32_t)(uintptr_t)g_page_table_kernel) >> 12;
}

/*
 * _aktifkan_paging
 * ----------------
 * Mengaktifkan paging di CPU.
 */
static void _aktifkan_paging(void)
{
    tak_bertanda32_t cr0;
    tak_bertanda32_t cr4;

    /* Set page directory - cast through uintptr_t for x86 */
    cpu_write_cr3((tak_bertanda32_t)(uintptr_t)g_page_directory);

    /* Enable 4 MB pages (PSE) */
    __asm__ __volatile__(
        "mov %%cr4, %0\n\t"
        "or %1, %0\n\t"
        "mov %0, %%cr4\n\t"
        : "=r"(cr4)
        : "i"(CR4_PSE)
    );

    /* Enable paging */
    cr0 = cpu_read_cr0();
    cr0 |= CR0_PG | CR0_WP;
    cpu_write_cr0(cr0);
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
    bitmap_size = (g_jumlah_halaman + 7) / 8;
    bitmap_size = (bitmap_size + UKURAN_HALAMAN - 1) & 
                  ~(UKURAN_HALAMAN - 1);

    /* Bitmap ditempatkan setelah kernel */
    g_bitmap_memori = (tak_bertanda32_t *)(KERNEL_MULAI_FISIK + 
                       0x200000 + KERNEL_MULAI_VIRT - KERNEL_MULAI_FISIK);

    /* Clear bitmap (semua bebas) */
    kernel_memset(g_bitmap_memori, 0, (ukuran_t)bitmap_size);

    /* Tandai halaman 0-1 MB sebagai used (BIOS, etc) */
    for (i = 0; i < 256; i++) {
        tak_bertanda32_t idx = i / 32;
        tak_bertanda32_t bit = i % 32;
        g_bitmap_memori[idx] |= (1 << bit);
        g_halaman_bebas--;
    }

    /* Tandai halaman kernel sebagai used */
    for (i = 256; i < 512; i++) {
        tak_bertanda32_t idx = i / 32;
        tak_bertanda32_t bit = i % 32;
        g_bitmap_memori[idx] |= (1 << bit);
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
    tak_bertanda32_t i;
    tak_bertanda32_t j;
    tak_bertanda32_t idx;
    tak_bertanda32_t bit;

    if (g_halaman_bebas == 0 || g_bitmap_memori == NULL) {
        return 0;
    }

    /* Cari halaman bebas */
    for (i = 0; i < (g_jumlah_halaman + 31) / 32; i++) {
        if (g_bitmap_memori[i] != 0xFFFFFFFF) {
            /* Ada bit yang 0 */
            for (j = 0; j < 32; j++) {
                if (!(g_bitmap_memori[i] & (1 << j))) {
                    idx = i;
                    bit = j;

                    /* Tandai sebagai used */
                    g_bitmap_memori[idx] |= (1 << bit);
                    g_halaman_bebas--;

                    return (alamat_fisik_t)((idx * 32 + bit) * 
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
    tak_bertanda32_t nomor_halaman;
    tak_bertanda32_t idx;
    tak_bertanda32_t bit;

    if (addr == 0 || g_bitmap_memori == NULL) {
        return;
    }

    nomor_halaman = (tak_bertanda32_t)(addr / UKURAN_HALAMAN);
    idx = nomor_halaman / 32;
    bit = nomor_halaman % 32;

    /* Clear bit */
    g_bitmap_memori[idx] &= ~(1 << bit);
    g_halaman_bebas++;
}

/*
 * ============================================================================
 * FUNGSI PUBLIC
 * ============================================================================
 */

/*
 * memori_x86_init
 * ---------------
 * Inisialisasi subsistem memori x86.
 *
 * Parameter:
 *   mbi - Pointer ke multiboot info
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t memori_x86_init(multiboot_info_t *mbi)
{
    kernel_printf("[MEM-x86] Menginisialisasi manajemen memori...\n");

    /* Hitung total memori */
    g_total_memori = _hitung_memori_total(mbi);
    kernel_printf("[MEM-x86] Total memori: %u MB\n",
                  (tak_bertanda32_t)(g_total_memori / (1024 * 1024)));

    /* Clear page directory */
    kernel_memset(g_page_directory, 0, sizeof(g_page_directory));

    /* Setup paging */
    kernel_printf("[MEM-x86] Setup identity mapping...\n");
    _setup_identity_mapping();

    kernel_printf("[MEM-x86] Setup kernel mapping...\n");
    _setup_kernel_mapping();

    /* Init physical memory manager */
    kernel_printf("[MEM-x86] Inisialisasi physical memory manager...\n");
    _init_bitmap_memori(g_total_memori);

    /* Aktifkan paging */
    kernel_printf("[MEM-x86] Mengaktifkan paging...\n");
    _aktifkan_paging();

    kernel_printf("[MEM-x86] Paging aktif\n");
    kernel_printf("[MEM-x86] Halaman bebas: %u (%u MB)\n",
                  (tak_bertanda32_t)g_halaman_bebas,
                  (tak_bertanda32_t)((g_halaman_bebas * UKURAN_HALAMAN) / 
                                     (1024 * 1024)));

    return STATUS_BERHASIL;
}

/*
 * memori_x86_get_total
 * --------------------
 * Mendapatkan total memori.
 *
 * Return:
 *   Total memori dalam byte
 */
tak_bertanda64_t memori_x86_get_total(void)
{
    return g_total_memori;
}

/*
 * memori_x86_get_bebas
 * --------------------
 * Mendapatkan memori bebas.
 *
 * Return:
 *   Memori bebas dalam byte
 */
tak_bertanda64_t memori_x86_get_bebas(void)
{
    return g_halaman_bebas * UKURAN_HALAMAN;
}

/*
 * memori_x86_get_terpakai
 * -----------------------
 * Mendapatkan memori terpakai.
 *
 * Return:
 *   Memori terpakai dalam byte
 */
tak_bertanda64_t memori_x86_get_terpakai(void)
{
    return (g_jumlah_halaman - g_halaman_bebas) * UKURAN_HALAMAN;
}

/*
 * memori_x86_alokasi_halaman
 * --------------------------
 * Mengalokasikan satu halaman fisik.
 *
 * Return:
 *   Alamat fisik halaman
 */
alamat_fisik_t memori_x86_alokasi_halaman(void)
{
    return _alokasi_halaman();
}

/*
 * memori_x86_bebaskan_halaman
 * ---------------------------
 * Membebaskan satu halaman fisik.
 *
 * Parameter:
 *   addr - Alamat fisik halaman
 */
void memori_x86_bebaskan_halaman(alamat_fisik_t addr)
{
    _bebaskan_halaman(addr);
}

/*
 * memori_x86_alokasi_halaman_n
 * ----------------------------
 * Mengalokasikan N halaman fisik berurutan.
 *
 * Parameter:
 *   jumlah - Jumlah halaman
 *
 * Return:
 *   Alamat fisik halaman pertama
 */
alamat_fisik_t memori_x86_alokasi_halaman_n(tak_bertanda32_t jumlah)
{
    tak_bertanda32_t i;
    tak_bertanda32_t j;
    tak_bertanda32_t k;
    tak_bertanda32_t start = 0;
    bool_t found = SALAH;

    if (jumlah == 0 || jumlah > g_halaman_bebas) {
        return 0;
    }

    /* Cari N halaman berurutan */
    for (i = 0; i < g_jumlah_halaman - jumlah + 1; i++) {
        found = BENAR;

        for (j = 0; j < jumlah; j++) {
            tak_bertanda32_t idx = (i + j) / 32;
            tak_bertanda32_t bit = (i + j) % 32;

            if (g_bitmap_memori[idx] & (1 << bit)) {
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
    for (k = 0; k < jumlah; k++) {
        tak_bertanda32_t idx = (start + k) / 32;
        tak_bertanda32_t bit = (start + k) % 32;
        g_bitmap_memori[idx] |= (1 << bit);
    }

    g_halaman_bebas -= jumlah;

    return (alamat_fisik_t)(start * UKURAN_HALAMAN);
}

/*
 * memori_x86_bebaskan_halaman_n
 * -----------------------------
 * Membebaskan N halaman fisik.
 *
 * Parameter:
 *   addr   - Alamat fisik halaman pertama
 *   jumlah - Jumlah halaman
 */
void memori_x86_bebaskan_halaman_n(alamat_fisik_t addr,
                                    tak_bertanda32_t jumlah)
{
    tak_bertanda32_t i;
    tak_bertanda32_t nomor_halaman;
    tak_bertanda32_t idx;
    tak_bertanda32_t bit;

    if (addr == 0 || jumlah == 0) {
        return;
    }

    nomor_halaman = (tak_bertanda32_t)(addr / UKURAN_HALAMAN);

    for (i = 0; i < jumlah; i++) {
        idx = (nomor_halaman + i) / 32;
        bit = (nomor_halaman + i) % 32;

        g_bitmap_memori[idx] &= ~(1 << bit);
        g_halaman_bebas++;
    }
}

/*
 * memori_x86_map_halaman
 * ----------------------
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
status_t memori_x86_map_halaman(alamat_virtual_t virt,
                                 alamat_fisik_t fisik,
                                 tak_bertanda32_t flags)
{
    tak_bertanda32_t pde_idx;
    tak_bertanda32_t pte_idx;
    struct pte *pt;
    tak_bertanda32_t pt_fisik;

    pde_idx = (tak_bertanda32_t)(virt >> 22);
    pte_idx = (tak_bertanda32_t)((virt >> 12) & 0x3FF);

    /* Cek apakah page table ada */
    if (!g_page_directory[pde_idx].ada) {
        /* Alokasikan page table baru */
        pt_fisik = (tak_bertanda32_t)_alokasi_halaman();
        if (pt_fisik == 0) {
            return STATUS_MEMORI_HABIS;
        }

        pt = (struct pte *)(uintptr_t)(pt_fisik + KERNEL_MULAI_VIRT - 
             KERNEL_MULAI_FISIK);
        kernel_memset(pt, 0, UKURAN_HALAMAN);

        g_page_directory[pde_idx].ada = 1;
        g_page_directory[pde_idx].tulis = 1;
        g_page_directory[pde_idx].user = (flags & PAGE_USER) ? 1 : 0;
        g_page_directory[pde_idx].alamat = pt_fisik >> 12;
    } else {
        pt_fisik = g_page_directory[pde_idx].alamat << 12;
        pt = (struct pte *)(uintptr_t)(pt_fisik + KERNEL_MULAI_VIRT - 
             KERNEL_MULAI_FISIK);
    }

    /* Set page table entry */
    pt[pte_idx].ada = 1;
    pt[pte_idx].tulis = (flags & PAGE_TULIS) ? 1 : 0;
    pt[pte_idx].user = (flags & PAGE_USER) ? 1 : 0;
    pt[pte_idx].alamat = (tak_bertanda32_t)(fisik >> 12);

    /* Invalidate TLB */
    cpu_invlpg((void *)(ukuran_t)virt);

    return STATUS_BERHASIL;
}

/*
 * memori_x86_unmap_halaman
 * ------------------------
 * Menghapus mapping halaman.
 *
 * Parameter:
 *   virt - Alamat virtual
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t memori_x86_unmap_halaman(alamat_virtual_t virt)
{
    tak_bertanda32_t pde_idx;
    tak_bertanda32_t pte_idx;
    struct pte *pt;
    tak_bertanda32_t pt_fisik;

    pde_idx = (tak_bertanda32_t)(virt >> 22);
    pte_idx = (tak_bertanda32_t)((virt >> 12) & 0x3FF);

    if (!g_page_directory[pde_idx].ada) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    pt_fisik = g_page_directory[pde_idx].alamat << 12;
    pt = (struct pte *)(uintptr_t)(pt_fisik + KERNEL_MULAI_VIRT - 
         KERNEL_MULAI_FISIK);

    /* Clear entry */
    kernel_memset(&pt[pte_idx], 0, sizeof(struct pte));

    /* Invalidate TLB */
    cpu_invlpg((void *)(ukuran_t)virt);

    return STATUS_BERHASIL;
}

/*
 * memori_x86_get_fisik
 * --------------------
 * Mendapatkan alamat fisik dari alamat virtual.
 *
 * Parameter:
 *   virt - Alamat virtual
 *
 * Return:
 *   Alamat fisik, atau 0 jika tidak di-map
 */
alamat_fisik_t memori_x86_get_fisik(alamat_virtual_t virt)
{
    tak_bertanda32_t pde_idx;
    tak_bertanda32_t pte_idx;
    struct pte *pt;
    tak_bertanda32_t pt_fisik;

    pde_idx = (tak_bertanda32_t)(virt >> 22);
    pte_idx = (tak_bertanda32_t)((virt >> 12) & 0x3FF);

    if (!g_page_directory[pde_idx].ada) {
        return 0;
    }

    pt_fisik = g_page_directory[pde_idx].alamat << 12;
    pt = (struct pte *)(uintptr_t)(pt_fisik + KERNEL_MULAI_VIRT - 
         KERNEL_MULAI_FISIK);

    if (!pt[pte_idx].ada) {
        return 0;
    }

    return (alamat_fisik_t)(pt[pte_idx].alamat << 12);
}

/*
 * memori_x86_get_page_directory
 * -----------------------------
 * Mendapatkan pointer ke page directory.
 *
 * Return:
 *   Pointer ke page directory
 */
void *memori_x86_get_page_directory(void)
{
    return g_page_directory;
}
