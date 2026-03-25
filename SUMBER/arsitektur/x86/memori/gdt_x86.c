/*
 * =============================================================================
 * PIGURA OS - GLOBAL DESCRIPTOR TABLE (GDT) x86
 * =============================================================================
 * 
 * Berkas ini berisi implementasi Global Descriptor Table (GDT) untuk arsitektur
 * x86. GDT mendefinisikan segmen-segmen memori yang dapat diakses dalam
 * protected mode.
 *
 * Struktur GDT:
 *   - Null descriptor (wajib, index 0)
 *   - Kernel code segment (index 1, selector 0x08)
 *   - Kernel data segment (index 2, selector 0x10)
 *   - User code segment (index 3, selector 0x18)
 *   - User data segment (index 4, selector 0x20)
 *   - TSS descriptor (index 5, selector 0x28)
 *
 * Arsitektur: x86 (32-bit protected mode)
 * Versi: 1.0
 * =============================================================================
 */

#include "../../../inti/kernel.h"

/* Type aliases untuk kompatibilitas dengan kode yang menggunakan stdint.h */
typedef tak_bertanda8_t  uint8_t;
typedef tak_bertanda16_t uint16_t;
typedef tak_bertanda32_t uint32_t;
typedef tak_bertanda64_t uint64_t;

/* =============================================================================
 * KONSTANTA
 * =============================================================================
 */

/* Selector GDT */
#define GDT_SELECTOR_NULL       0x00    /* Selector null */
#define GDT_SELECTOR_KODE       0x08    /* Selector kode kernel */
#define GDT_SELECTOR_DATA       0x10    /* Selector data kernel */
#define GDT_SELECTOR_KODE_USER  0x18    /* Selector kode user */
#define GDT_SELECTOR_DATA_USER  0x20    /* Selector data user */
#define GDT_SELECTOR_TSS        0x28    /* Selector TSS */

/* Index GDT */
#define GDT_INDEX_NULL          0       /* Index null descriptor */
#define GDT_INDEX_KODE          1       /* Index kode kernel */
#define GDT_INDEX_DATA          2       /* Index data kernel */
#define GDT_INDEX_KODE_USER     3       /* Index kode user */
#define GDT_INDEX_DATA_USER     4       /* Index data user */
#define GDT_INDEX_TSS           5       /* Index TSS */
#define GDT_JUMLAH_ENTRY        6       /* Total entry GDT */

/* Flag akses segment */
#define GDT_AKSES_ADA           0x80    /* Segment present */
#define GDT_AKSES_RING0         0x00    /* Privilege level 0 */
#define GDT_AKSES_RING3         0x60    /* Privilege level 3 */
#define GDT_AKSES_SISTEM        0x10    /* System segment */
#define GDT_AKSES_KODE          0x0A    /* Code segment */
#define GDT_AKSES_DATA          0x02    /* Data segment */
#define GDT_AKSES_TSS           0x09    /* TSS segment */

/* Flag granularity */
#define GDT_GRAN_BYTE           0x00    /* Granularitas byte */
#define GDT_GRAN_4KB            0x80    /* Granularitas 4 KB */
#define GDT_32_BIT              0x40    /* Operand 32-bit */
#define GDT_16_BIT              0x00    /* Operand 16-bit */

/* Flag kode segment */
#define GDT_KODE_BACA           0x02    /* Readable */
#define GDT_KODE_EKSEKUSI       0x08    /* Executable */
#define GDT_KODE_KONFORM        0x04    /* Conforming */

/* Flag data segment */
#define GDT_DATA_TULIS          0x02    /* Writable */
#define GDT_DATA_NAIK           0x04    /* Expand-up */

/* Batas segmen */
#define GDT_BATAS_MAKS_4KB      0xFFFFF /* Batas maksimum (4 KB granularity) */
#define GDT_BATAS_MAKS_BYTE     0xFFFF  /* Batas maksimum (byte granularity) */

/* Basis segmen */
#define GDT_BASIS_NOL           0x00000000  /* Basis nol */

/* =============================================================================
 * STRUKTUR DATA
 * =============================================================================
 */

/*
 * Deskriptor GDT (8 byte)
 * Struktur ini mendefinisikan satu entry dalam GDT.
 */
struct gdt_entry {
    uint16_t batas_rendah;      /* Bit 0-15 dari limit */
    uint16_t basis_rendah;      /* Bit 0-15 dari base */
    uint8_t  basis_tengah;      /* Bit 16-23 dari base */
    uint8_t  akses;             /* Access byte */
    uint8_t  granularitas;      /* Flags dan bit 16-19 dari limit */
    uint8_t  basis_atas;        /* Bit 24-31 dari base */
} __attribute__((packed));

/*
 * Pointer GDT (6 byte)
 * Struktur ini digunakan untuk instruksi LGDT.
 */
struct gdt_pointer {
    uint16_t batas;             /* Ukuran GDT minus 1 */
    uint32_t basis;             /* Alamat GDT */
} __attribute__((packed));

/*
 * Segment Selector
 * Struktur bit-field untuk segment selector.
 */
union seg_selector {
    struct {
        uint16_t rpl: 2;        /* Requestor Privilege Level */
        uint16_t ti:  1;        /* Table Indicator (0=GDT, 1=LDT) */
        uint16_t index: 13;     /* Index ke dalam GDT/LDT */
    } bit;
    uint16_t nilai;
};

/* =============================================================================
 * VARIABEL GLOBAL
 * =============================================================================
 */

/* Tabel GDT */
static struct gdt_entry g_tabel_gdt[GDT_JUMLAH_ENTRY];

/* Pointer GDT untuk LGDT */
static struct gdt_pointer g_pointer_gdt;

/* =============================================================================
 * FUNGSI INTERNAL
 * =============================================================================
 */

/*
 * _atur_entry_gdt
 * ----------------
 * Mengisi satu entry GDT dengan nilai yang diberikan.
 *
 * Parameter:
 *   index      - Index entry dalam GDT (0-5)
 *   basis      - Alamat basis segmen
 *   batas      - Ukuran segmen (limit)
 *   akses      - Byte akses (type, privilege, present)
 *   gran       - Byte granularitas (flags, limit tinggi)
 *
 * Catatan:
 *   Batas diinterpretasikan berdasarkan flag granularity.
 *   Jika granularitas 4KB, batas sebenarnya = (batas + 1) * 4096 - 1
 */
static void _atur_entry_gdt(int index, uint32_t basis, uint32_t batas,
                            uint8_t akses, uint8_t gran)
{
    struct gdt_entry *entry;

    /* Validasi index */
    if (index < 0 || index >= GDT_JUMLAH_ENTRY) {
        return;
    }

    entry = &g_tabel_gdt[index];

    /* Isi field basis */
    entry->basis_rendah = (uint16_t)(basis & 0xFFFF);
    entry->basis_tengah = (uint8_t)((basis >> 16) & 0xFF);
    entry->basis_atas = (uint8_t)((basis >> 24) & 0xFF);

    /* Isi field batas */
    entry->batas_rendah = (uint16_t)(batas & 0xFFFF);
    entry->granularitas = (uint8_t)((batas >> 16) & 0x0F);

    /* Tambahkan flag granularitas */
    entry->granularitas |= (gran & 0xF0);

    /* Set access byte */
    entry->akses = akses;
}

/*
 * _buat_null_descriptor
 * ----------------------
 * Membuat null descriptor di index 0.
 * Null descriptor wajib ada dan tidak dapat diakses.
 */
static void _buat_null_descriptor(void)
{
    _atur_entry_gdt(GDT_INDEX_NULL, 0, 0, 0, 0);
}

/*
 * _buat_kode_kernel
 * ------------------
 * Membuat code segment descriptor untuk kernel (Ring 0).
 *
 * Karakteristik:
 *   - Basis: 0x00000000
 *   - Batas: 4 GB (dengan granularitas 4 KB)
 *   - Akses: Present, Ring 0, Code, Executable, Readable
 *   - Granularitas: 4 KB, 32-bit
 */
static void _buat_kode_kernel(void)
{
    uint8_t akses;
    uint8_t gran;

    akses = GDT_AKSES_ADA           /* Present */
          | GDT_AKSES_RING0         /* Ring 0 */
          | GDT_AKSES_SISTEM        /* System segment */
          | GDT_AKSES_KODE          /* Code segment */
          | GDT_KODE_EKSEKUSI       /* Executable */
          | GDT_KODE_BACA;          /* Readable */

    gran = GDT_GRAN_4KB              /* Granularitas 4 KB */
         | GDT_32_BIT;               /* 32-bit operand */

    _atur_entry_gdt(GDT_INDEX_KODE, GDT_BASIS_NOL, GDT_BATAS_MAKS_4KB,
                    akses, gran);
}

/*
 * _buat_data_kernel
 * ------------------
 * Membuat data segment descriptor untuk kernel (Ring 0).
 *
 * Karakteristik:
 *   - Basis: 0x00000000
 *   - Batas: 4 GB
 *   - Akses: Present, Ring 0, Data, Writable
 *   - Granularitas: 4 KB, 32-bit
 */
static void _buat_data_kernel(void)
{
    uint8_t akses;
    uint8_t gran;

    akses = GDT_AKSES_ADA           /* Present */
          | GDT_AKSES_RING0         /* Ring 0 */
          | GDT_AKSES_SISTEM        /* System segment */
          | GDT_AKSES_DATA          /* Data segment */
          | GDT_DATA_TULIS;         /* Writable */

    gran = GDT_GRAN_4KB              /* Granularitas 4 KB */
         | GDT_32_BIT;               /* 32-bit operand */

    _atur_entry_gdt(GDT_INDEX_DATA, GDT_BASIS_NOL, GDT_BATAS_MAKS_4KB,
                    akses, gran);
}

/*
 * _buat_kode_user
 * ----------------
 * Membuat code segment descriptor untuk user mode (Ring 3).
 */
static void _buat_kode_user(void)
{
    uint8_t akses;
    uint8_t gran;

    akses = GDT_AKSES_ADA           /* Present */
          | GDT_AKSES_RING3         /* Ring 3 */
          | GDT_AKSES_SISTEM        /* System segment */
          | GDT_AKSES_KODE          /* Code segment */
          | GDT_KODE_EKSEKUSI       /* Executable */
          | GDT_KODE_BACA;          /* Readable */

    gran = GDT_GRAN_4KB              /* Granularitas 4 KB */
         | GDT_32_BIT;               /* 32-bit operand */

    _atur_entry_gdt(GDT_INDEX_KODE_USER, GDT_BASIS_NOL, GDT_BATAS_MAKS_4KB,
                    akses, gran);
}

/*
 * _buat_data_user
 * ----------------
 * Membuat data segment descriptor untuk user mode (Ring 3).
 */
static void _buat_data_user(void)
{
    uint8_t akses;
    uint8_t gran;

    akses = GDT_AKSES_ADA           /* Present */
          | GDT_AKSES_RING3         /* Ring 3 */
          | GDT_AKSES_SISTEM        /* System segment */
          | GDT_AKSES_DATA          /* Data segment */
          | GDT_DATA_TULIS;         /* Writable */

    gran = GDT_GRAN_4KB              /* Granularitas 4 KB */
         | GDT_32_BIT;               /* 32-bit operand */

    _atur_entry_gdt(GDT_INDEX_DATA_USER, GDT_BASIS_NOL, GDT_BATAS_MAKS_4KB,
                    akses, gran);
}

/*
 * _buat_tss_descriptor
 * ---------------------
 * Membuat TSS descriptor untuk task switching.
 *
 * Parameter:
 *   basis - Alamat struktur TSS
 *   batas - Ukuran struktur TSS
 */
static void _buat_tss_descriptor(uint32_t basis, uint32_t batas)
{
    uint8_t akses;
    uint8_t gran;

    akses = GDT_AKSES_ADA           /* Present */
          | GDT_AKSES_RING0         /* Ring 0 */
          | GDT_AKSES_TSS;          /* TSS segment (busy=0) */

    gran = GDT_GRAN_BYTE;            /* Granularitas byte */

    _atur_entry_gdt(GDT_INDEX_TSS, basis, batas, akses, gran);
}

/* =============================================================================
 * FUNGSI PUBLIC
 * =============================================================================
 */

/*
 * gdt_init
 * ---------
 * Menginisialisasi Global Descriptor Table.
 *
 * Fungsi ini membuat semua descriptor dan memuat GDT ke CPU.
 *
 * Return:
 *   0 jika berhasil, nilai negatif jika gagal
 */
int gdt_init(void)
{
    /* Buat semua descriptor */
    _buat_null_descriptor();
    _buat_kode_kernel();
    _buat_data_kernel();
    _buat_kode_user();
    _buat_data_user();
    
    /* TSS descriptor akan dibuat nanti setelah TSS dialokasi */
    _buat_tss_descriptor(0, 0);

    /* Setup pointer GDT */
    g_pointer_gdt.batas = sizeof(g_tabel_gdt) - 1;
    g_pointer_gdt.basis = (uint32_t)(uintptr_t)&g_tabel_gdt;

    /* Load GDT menggunakan assembly */
    __asm__ __volatile__(
        "lgdt %0\n\t"
        :
        : "m"(g_pointer_gdt)
        : "memory"
    );

    /* Reload segment register */
    __asm__ __volatile__(
        "movw %0, %%ax\n\t"
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"
        "movw %%ax, %%ss\n\t"
        :
        : "i"(GDT_SELECTOR_DATA)
        : "ax", "memory"
    );

    /* Reload code segment dengan far jump */
    __asm__ __volatile__(
        "ljmp %0, $1f\n\t"
        "1:\n\t"
        :
        : "i"(GDT_SELECTOR_KODE)
        : "memory"
    );

    return 0;
}

/*
 * gdt_set_tss
 * ------------
 * Mengatur alamat TSS dalam descriptor GDT.
 *
 * Parameter:
 *   basis - Alamat struktur TSS
 *   batas - Ukuran struktur TSS
 */
void gdt_set_tss(uint32_t basis, uint32_t batas)
{
    _buat_tss_descriptor(basis, batas);
}

/*
 * gdt_get_selector
 * -----------------
 * Mendapatkan segment selector untuk index tertentu.
 *
 * Parameter:
 *   index - Index GDT
 *   ring  - Privilege level (0-3)
 *
 * Return:
 *   Segment selector
 */
uint16_t gdt_get_selector(int index, int ring)
{
    union seg_selector sel;

    sel.bit.index = (uint16_t)index;
    sel.bit.ti = 0;         /* GDT */
    sel.bit.rpl = (uint16_t)(ring & 0x03);

    return sel.nilai;
}

/*
 * gdt_get_pointer
 * ----------------
 * Mendapatkan pointer ke struktur GDT pointer.
 *
 * Return:
 *   Pointer ke gdt_pointer
 */
const struct gdt_pointer *gdt_get_pointer(void)
{
    return &g_pointer_gdt;
}
