/*
 * PIGURA OS - LDT x86
 * --------------------
 * Implementasi Local Descriptor Table untuk arsitektur x86.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola LDT yang
 * digunakan untuk segmentasi per-proses.
 *
 * Arsitektur: x86 (32-bit)
 * Versi: 1.0
 */

#include "../../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA LDT
 * ============================================================================
 */

/* Jumlah entry LDT */
#define LDT_JUMLAH_ENTRY        8192

/* Selector LDT */
#define LDT_SELECTOR_MULAI      0x0000
#define LDT_SELECTOR_AKHIR      0xFFFF

/* Flag deskriptor */
#define LDT_FLAG_ADA            0x80
#define LDT_FLAG_DPL0           0x00
#define LDT_FLAG_DPL3           0x60
#define LDT_FLAG_KODE           0x0A
#define LDT_FLAG_DATA           0x02
#define LDT_FLAG_TULIS          0x02
#define LDT_FLAG_BACA           0x02

/* Granularitas */
#define LDT_GRAN_BYTE           0x00
#define LDT_GRAN_4KB            0x80
#define LDT_32BIT               0x40

/*
 * ============================================================================
 * STRUKTUR DATA
 * ============================================================================
 */

/* Entry LDT (8 byte) */
struct ldt_entry {
    tak_bertanda16_t batas_rendah;
    tak_bertanda16_t basis_rendah;
    tak_bertanda8_t basis_tengah;
    tak_bertanda8_t akses;
    tak_bertanda8_t granularitas;
    tak_bertanda8_t basis_atas;
} __attribute__((packed));

/* LDT pointer */
struct ldt_pointer {
    tak_bertanda16_t batas;
    tak_bertanda32_t basis;
} __attribute__((packed));

/*
 * ============================================================================
 * VARIABEL GLOBAL
 * ============================================================================
 */

/* LDT default untuk proses */
static struct ldt_entry g_ldt_default[LDT_JUMLAH_ENTRY]
    __attribute__((aligned(8)));

/* LDT pointer */
static struct ldt_pointer g_ldt_ptr;

/*
 * ============================================================================
 * FUNGSI INTERNAL
 * ============================================================================
 */

/*
 * _atur_entry_ldt
 * ---------------
 * Mengatur satu entry LDT.
 *
 * Parameter:
 *   ldt   - Pointer ke tabel LDT
 *   index - Index entry
 *   basis - Alamat basis
 *   batas - Batas segmen
 *   akses - Byte akses
 *   gran  - Byte granularitas
 */
static void _atur_entry_ldt(struct ldt_entry *ldt, tak_bertanda32_t index,
                            tak_bertanda32_t basis, tak_bertanda32_t batas,
                            tak_bertanda8_t akses, tak_bertanda8_t gran)
{
    struct ldt_entry *entry;

    if (ldt == NULL || index >= LDT_JUMLAH_ENTRY) {
        return;
    }

    entry = &ldt[index];

    /* Isi basis */
    entry->basis_rendah = (tak_bertanda16_t)(basis & 0xFFFF);
    entry->basis_tengah = (tak_bertanda8_t)((basis >> 16) & 0xFF);
    entry->basis_atas = (tak_bertanda8_t)((basis >> 24) & 0xFF);

    /* Isi batas */
    entry->batas_rendah = (tak_bertanda16_t)(batas & 0xFFFF);
    entry->granularitas = (tak_bertanda8_t)((batas >> 16) & 0x0F);
    entry->granularitas |= (gran & 0xF0);

    /* Set akses */
    entry->akses = akses;
}

/*
 * ============================================================================
 * FUNGSI PUBLIC
 * ============================================================================
 */

/*
 * ldt_init
 * --------
 * Menginisialisasi LDT.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t ldt_init(void)
{
    /* Clear LDT */
    kernel_memset(g_ldt_default, 0, sizeof(g_ldt_default));

    /* Setup pointer */
    g_ldt_ptr.batas = sizeof(g_ldt_default) - 1;
    g_ldt_ptr.basis = (tak_bertanda32_t)(uintptr_t)g_ldt_default;

    return STATUS_BERHASIL;
}

/*
 * ldt_load
 * --------
 * Memuat LDT ke CPU.
 *
 * Parameter:
 *   ldt  - Pointer ke tabel LDT
 *   ukuran - Ukuran LDT dalam byte
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t ldt_load(void *ldt, ukuran_t ukuran)
{
    struct ldt_pointer ptr;

    if (ldt == NULL || ukuran == 0) {
        return STATUS_PARAM_INVALID;
    }

    ptr.batas = (tak_bertanda16_t)(ukuran - 1);
    ptr.basis = (tak_bertanda32_t)(uintptr_t)ldt;

    /* Load LDT menggunakan LLDT instruction */
    __asm__ __volatile__(
        "lldt %0\n\t"
        :
        : "m"(ptr.batas)
        : "memory"
    );

    return STATUS_BERHASIL;
}

/*
 * ldt_load_default
 * ----------------
 * Memuat LDT default.
 */
void ldt_load_default(void)
{
    /* Load default LDT */
    __asm__ __volatile__(
        "lldt %0\n\t"
        :
        : "m"(g_ldt_ptr.batas)
        : "memory"
    );
}

/*
 * ldt_buat_segmen_kode
 * --------------------
 * Membuat deskriptor segmen kode di LDT.
 *
 * Parameter:
 *   ldt   - Pointer ke tabel LDT
 *   index - Index entry
 *   basis - Alamat basis
 *   batas - Batas segmen
 *   dpl   - Privilege level (0-3)
 *
 * Return:
 *   Selector LDT
 */
tak_bertanda16_t ldt_buat_segmen_kode(void *ldt, tak_bertanda32_t index,
                                       tak_bertanda32_t basis,
                                       tak_bertanda32_t batas,
                                       tak_bertanda8_t dpl)
{
    tak_bertanda8_t akses;
    tak_bertanda8_t gran;
    struct ldt_entry *ldt_table;

    if (ldt == NULL || index >= LDT_JUMLAH_ENTRY || dpl > 3) {
        return 0;
    }

    ldt_table = (struct ldt_entry *)ldt;

    /* Buat akses byte */
    akses = LDT_FLAG_ADA | LDT_FLAG_KODE | LDT_FLAG_BACA;
    akses |= (dpl << 5);

    /* Granularitas 4 KB, 32-bit */
    gran = LDT_GRAN_4KB | LDT_32BIT;

    _atur_entry_ldt(ldt_table, index, basis, batas, akses, gran);

    /* Return selector dengan TI=1 (LDT) */
    return (tak_bertanda16_t)((index << 3) | (dpl & 0x03) | 0x04);
}

/*
 * ldt_buat_segmen_data
 * --------------------
 * Membuat deskriptor segmen data di LDT.
 *
 * Parameter:
 *   ldt   - Pointer ke tabel LDT
 *   index - Index entry
 *   basis - Alamat basis
 *   batas - Batas segmen
 *   dpl   - Privilege level (0-3)
 *
 * Return:
 *   Selector LDT
 */
tak_bertanda16_t ldt_buat_segmen_data(void *ldt, tak_bertanda32_t index,
                                       tak_bertanda32_t basis,
                                       tak_bertanda32_t batas,
                                       tak_bertanda8_t dpl)
{
    tak_bertanda8_t akses;
    tak_bertanda8_t gran;
    struct ldt_entry *ldt_table;

    if (ldt == NULL || index >= LDT_JUMLAH_ENTRY || dpl > 3) {
        return 0;
    }

    ldt_table = (struct ldt_entry *)ldt;

    /* Buat akses byte */
    akses = LDT_FLAG_ADA | LDT_FLAG_DATA | LDT_FLAG_TULIS;
    akses |= (dpl << 5);

    /* Granularitas 4 KB, 32-bit */
    gran = LDT_GRAN_4KB | LDT_32BIT;

    _atur_entry_ldt(ldt_table, index, basis, batas, akses, gran);

    /* Return selector dengan TI=1 (LDT) */
    return (tak_bertanda16_t)((index << 3) | (dpl & 0x03) | 0x04);
}

/*
 * ldt_hapus_segmen
 * ----------------
 * Menghapus deskriptor segmen dari LDT.
 *
 * Parameter:
 *   ldt   - Pointer ke tabel LDT
 *   index - Index entry
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t ldt_hapus_segmen(void *ldt, tak_bertanda32_t index)
{
    struct ldt_entry *ldt_table;

    if (ldt == NULL || index >= LDT_JUMLAH_ENTRY) {
        return STATUS_PARAM_INVALID;
    }

    ldt_table = (struct ldt_entry *)ldt;

    /* Clear entry */
    kernel_memset(&ldt_table[index], 0, sizeof(struct ldt_entry));

    return STATUS_BERHASIL;
}

/*
 * ldt_alokasi
 * -----------
 * Mengalokasikan LDT baru untuk proses.
 *
 * Parameter:
 *   ukuran - Jumlah entry yang diperlukan
 *
 * Return:
 *   Pointer ke LDT, atau NULL jika gagal
 */
void *ldt_alokasi(tak_bertanda32_t ukuran)
{
    ukuran_t total_ukuran;
    void *ldt;

    if (ukuran == 0 || ukuran > LDT_JUMLAH_ENTRY) {
        return NULL;
    }

    total_ukuran = ukuran * sizeof(struct ldt_entry);

    /* Alokasikan memori untuk LDT */
    ldt = (void *)kmalloc(total_ukuran);
    if (ldt == NULL) {
        return NULL;
    }

    /* Clear LDT */
    kernel_memset(ldt, 0, total_ukuran);

    return ldt;
}

/*
 * ldt_bebaskan
 * ------------
 * Membebaskan LDT yang dialokasikan.
 *
 * Parameter:
 *   ldt - Pointer ke LDT
 */
void ldt_bebaskan(void *ldt)
{
    if (ldt == NULL) {
        return;
    }

    kfree(ldt);
}

/*
 * ldt_get_default
 * ---------------
 * Mendapatkan pointer ke LDT default.
 *
 * Return:
 *   Pointer ke LDT default
 */
void *ldt_get_default(void)
{
    return g_ldt_default;
}

/*
 * ldt_get_pointer
 * ---------------
 * Mendapatkan pointer ke LDT pointer.
 *
 * Return:
 *   Pointer ke LDT pointer
 */
void *ldt_get_pointer(void)
{
    return &g_ldt_ptr;
}
