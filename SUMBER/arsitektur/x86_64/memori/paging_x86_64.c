/*
 * PIGURA OS - PAGING x86_64
 * -------------------------
 * Implementasi sistem paging untuk arsitektur x86_64.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk manajemen
 * paging pada arsitektur x86_64.
 *
 * Arsitektur: x86_64 (64-bit)
 * Versi: 1.0
 */

#include "../../../inti/kernel.h"
#include "../cpu_x86_64.h"

/*
 * ============================================================================
 * DEKLARASI FUNGSI EKSTERNAL (dari pml4_x86_64.c)
 * ============================================================================
 */
extern status_t pml4_x86_64_init(void);
extern tak_bertanda64_t pml4_x86_64_buat(void);
extern void pml4_x86_64_hancurkan(tak_bertanda64_t pml4_addr);
extern status_t pml4_x86_64_map(tak_bertanda64_t pml4_addr,
                                alamat_virtual_t vaddr,
                                alamat_fisik_t paddr,
                                tak_bertanda64_t flags);
extern status_t pml4_x86_64_unmap(tak_bertanda64_t pml4_addr,
                                  alamat_virtual_t vaddr);
extern tak_bertanda64_t pml4_x86_64_get_fisik(tak_bertanda64_t pml4_addr,
                                               alamat_virtual_t vaddr);
extern status_t pml4_x86_64_switch(tak_bertanda64_t pml4_addr);

/*
 * ============================================================================
 * KONSTANTA PAGING
 * ============================================================================
 */

/* Flag paging */
#define PAGE_ADA                0x01
#define PAGE_TULIS              0x02
#define PAGE_USER               0x04
#define PAGE_BESAR              0x80
#define PAGE_GLOBAL             0x100
#define PAGE_NX                 (1ULL << 63)

/* Ukuran halaman */
#define UKURAN_HALAMAN_4K       4096
#define UKURAN_HALAMAN_2M       (2 * 1024 * 1024)
#define UKURAN_HALAMAN_1G       (1024 * 1024 * 1024)

/*
 * ============================================================================
 * VARIABEL GLOBAL
 * ============================================================================
 */

/* Alamat PML4 kernel */
static tak_bertanda64_t g_pml4_kernel = 0;

/* Flag inisialisasi */
static bool_t g_paging_diinisialisasi = SALAH;

/*
 * ============================================================================
 * FUNGSI PUBLIC
 * ============================================================================
 */

/*
 * paging_x86_64_init
 * ------------------
 * Inisialisasi sistem paging x86_64.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t paging_x86_64_init(void)
{
    kernel_printf("[PAGING-x86_64] Menginisialisasi paging...\n");

    /* Baca CR3 untuk mendapatkan PML4 kernel */
    g_pml4_kernel = cpu_read_cr3();

    if (g_pml4_kernel == 0) {
        kernel_printf("[PAGING-x86_64] Error: PML4 tidak ditemukan\n");
        return STATUS_GAGAL;
    }

    kernel_printf("[PAGING-x86_64] PML4 kernel: 0x%016X\n", g_pml4_kernel);

    /* Enable global pages (CR4.PGE) */
    tak_bertanda64_t cr4;
    cr4 = cpu_read_cr4();
    cr4 |= 0x80;  /* PGE bit */
    cpu_write_cr4(cr4);

    /* Enable NX bit jika didukung */
    tak_bertanda64_t efer;
    efer = cpu_read_msr(0xC0000080);  /* MSR_EFER */
    efer |= (1 << 11);  /* NXE bit */
    cpu_write_msr(0xC0000080, efer);

    g_paging_diinisialisasi = BENAR;

    kernel_printf("[PAGING-x86_64] Paging siap\n");

    return STATUS_BERHASIL;
}

/*
 * paging_x86_64_map_page
 * ----------------------
 * Memetakan satu halaman.
 *
 * Parameter:
 *   vaddr - Alamat virtual
 *   paddr - Alamat fisik
 *   flags - Flag mapping
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t paging_x86_64_map_page(alamat_virtual_t vaddr,
                                 alamat_fisik_t paddr,
                                 tak_bertanda32_t flags)
{
    if (!g_paging_diinisialisasi) {
        return STATUS_GAGAL;
    }

    /* Gunakan fungsi dari pml4_x86_64 */
    return pml4_x86_64_map(g_pml4_kernel, vaddr, paddr,
                           (tak_bertanda64_t)flags);
}

/*
 * paging_x86_64_unmap_page
 * ------------------------
 * Menghapus mapping halaman.
 *
 * Parameter:
 *   vaddr - Alamat virtual
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t paging_x86_64_unmap_page(alamat_virtual_t vaddr)
{
    if (!g_paging_diinisialisasi) {
        return STATUS_GAGAL;
    }

    return pml4_x86_64_unmap(g_pml4_kernel, vaddr);
}

/*
 * paging_x86_64_get_physical
 * --------------------------
 * Mendapatkan alamat fisik dari virtual.
 *
 * Parameter:
 *   vaddr - Alamat virtual
 *
 * Return:
 *   Alamat fisik, atau 0 jika tidak di-map
 */
alamat_fisik_t paging_x86_64_get_physical(alamat_virtual_t vaddr)
{
    if (!g_paging_diinisialisasi) {
        return 0;
    }

    return (alamat_fisik_t)pml4_x86_64_get_fisik(g_pml4_kernel, vaddr);
}

/*
 * paging_x86_64_is_mapped
 * -----------------------
 * Cek apakah halaman sudah di-map.
 *
 * Parameter:
 *   vaddr - Alamat virtual
 *
 * Return:
 *   BENAR jika sudah di-map
 */
bool_t paging_x86_64_is_mapped(alamat_virtual_t vaddr)
{
    if (!g_paging_diinisialisasi) {
        return SALAH;
    }

    return (pml4_x86_64_get_fisik(g_pml4_kernel, vaddr) != 0) ? BENAR : SALAH;
}

/*
 * paging_x86_64_create_address_space
 * ----------------------------------
 * Membuat address space baru.
 *
 * Return:
 *   Pointer ke page directory, atau NULL jika gagal
 */
struct page_directory *paging_x86_64_create_address_space(void)
{
    tak_bertanda64_t pml4;

    pml4 = pml4_x86_64_buat();
    if (pml4 == 0) {
        return NULL;
    }

    return (struct page_directory *)pml4;
}

/*
 * paging_x86_64_destroy_address_space
 * -----------------------------------
 * Menghancurkan address space.
 *
 * Parameter:
 *   dir - Pointer ke page directory
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t paging_x86_64_destroy_address_space(struct page_directory *dir)
{
    if (dir == NULL) {
        return STATUS_PARAM_INVALID;
    }

    pml4_x86_64_hancurkan((tak_bertanda64_t)dir);

    return STATUS_BERHASIL;
}

/*
 * paging_x86_64_switch_directory
 * ------------------------------
 * Beralih ke page directory tertentu.
 *
 * Parameter:
 *   dir - Pointer ke page directory
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t paging_x86_64_switch_directory(struct page_directory *dir)
{
    if (dir == NULL) {
        return STATUS_PARAM_INVALID;
    }

    return pml4_x86_64_switch((tak_bertanda64_t)dir);
}

/*
 * paging_x86_64_get_kernel_pml4
 * -----------------------------
 * Mendapatkan PML4 kernel.
 *
 * Return:
 *   Alamat PML4 kernel
 */
tak_bertanda64_t paging_x86_64_get_kernel_pml4(void)
{
    return g_pml4_kernel;
}

/*
 * paging_x86_64_apakah_siap
 * -------------------------
 * Mengecek apakah paging sudah diinisialisasi.
 *
 * Return:
 *   BENAR jika sudah, SALAH jika belum
 */
bool_t paging_x86_64_apakah_siap(void)
{
    return g_paging_diinisialisasi;
}
