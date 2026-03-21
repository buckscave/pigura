/*
 * PIGURA OS - GDT x86_64
 * ----------------------
 * Implementasi Global Descriptor Table untuk arsitektur x86_64.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk setup dan
 * manajemen GDT pada arsitektur x86_64, termasuk TSS descriptor.
 *
 * Arsitektur: x86_64 (64-bit)
 * Versi: 1.0
 */

#include "../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA GDT
 * ============================================================================
 */

/* Jumlah entri GDT */
#define GDT_ENTRI_JUMLAH       8

/* Index entri GDT */
#define GDT_INDEX_NULL         0
#define GDT_INDEX_KERNEL_CODE  1
#define GDT_INDEX_KERNEL_DATA  2
#define GDT_INDEX_USER_CODE    3
#define GDT_INDEX_USER_DATA    4
#define GDT_INDEX_TSS_LOW      5
#define GDT_INDEX_TSS_HIGH     6

/* Flag GDT */
#define GDT_PRESENT            0x80
#define GDT_DPL_0              0x00
#define GDT_DPL_3              0x60
#define GDT_S_SYSTEM           0x00
#define GDT_S_CODE_DATA        0x10
#define GDT_TYPE_CODE          0x0A
#define GDT_TYPE_DATA          0x02
#define GDT_TYPE_TSS           0x09
#define GDT_LONG_MODE          0x20
#define GDT_32BIT              0x40
#define GDT_4K_GRAN            0x80

/*
 * ============================================================================
 * STRUKTUR DATA
 * ============================================================================
 */

/* Entry GDT 64-bit */
struct gdt_entry {
    tak_bertanda16_t limit_bawah;
    tak_bertanda16_t base_bawah;
    tak_bertanda8_t  base_tengah;
    tak_bertanda8_t  access;
    tak_bertanda8_t  flags_limit;
    tak_bertanda8_t  base_atas;
} __attribute__((packed));

/* Entry GDT untuk TSS (16 byte) */
struct gdt_entry_tss {
    tak_bertanda16_t limit_bawah;
    tak_bertanda16_t base_bawah;
    tak_bertanda8_t  base_tengah;
    tak_bertanda8_t  access;
    tak_bertanda8_t  flags_limit;
    tak_bertanda8_t  base_atas;
    tak_bertanda32_t base_ext;
    tak_bertanda32_t reserv;
} __attribute__((packed));

/* Register GDTR */
struct gdt_ptr {
    tak_bertanda16_t batas;
    tak_bertanda64_t alamat;
} __attribute__((packed));

/*
 * ============================================================================
 * VARIABEL GLOBAL
 * ============================================================================
 */

/* GDT */
static struct gdt_entry gdt[GDT_INDEX_TSS_LOW]
    __attribute__((aligned(16)));
static struct gdt_entry_tss gdt_tss
    __attribute__((aligned(16)));

/* Pointer GDT */
static struct gdt_ptr gdt_ptr;

/* TSS */
static struct tss_x86_64 {
    tak_bertanda32_t reserv1;
    tak_bertanda64_t rsp0;
    tak_bertanda64_t rsp1;
    tak_bertanda64_t rsp2;
    tak_bertanda64_t reserv2;
    tak_bertanda64_t ist[7];
    tak_bertanda64_t reserv3;
    tak_bertanda16_t reserv4;
    tak_bertanda16_t iomap_base;
} g_tss __attribute__((aligned(16)));

/* Flag inisialisasi */
static bool_t g_gdt_diinisialisasi = SALAH;

/*
 * ============================================================================
 * FUNGSI INTERNAL
 * ============================================================================
 */

/*
 * _gdt_set_entry
 * --------------
 * Mengatur entry GDT standar.
 *
 * Parameter:
 *   index    - Index entry
 *   base     - Alamat base
 *   limit    - Batas
 *   access   - Byte access
 *   flags    - Byte flags
 */
static void _gdt_set_entry(tak_bertanda32_t index,
                            tak_bertanda32_t base,
                            tak_bertanda32_t limit,
                            tak_bertanda8_t access,
                            tak_bertanda8_t flags)
{
    struct gdt_entry *entry;

    if (index >= GDT_INDEX_TSS_LOW) {
        return;
    }

    entry = &gdt[index];

    entry->limit_bawah = (tak_bertanda16_t)(limit & 0xFFFF);
    entry->base_bawah = (tak_bertanda16_t)(base & 0xFFFF);
    entry->base_tengah = (tak_bertanda8_t)((base >> 16) & 0xFF);
    entry->access = access;
    entry->flags_limit = (tak_bertanda8_t)((limit >> 16) & 0x0F);
    entry->flags_limit |= flags & 0xF0;
    entry->base_atas = (tak_bertanda8_t)((base >> 24) & 0xFF);
}

/*
 * _gdt_set_tss
 * ------------
 * Mengatur entry GDT untuk TSS.
 *
 * Parameter:
 *   base  - Alamat TSS
 *   limit - Ukuran TSS
 */
static void _gdt_set_tss(tak_bertanda64_t base, tak_bertanda32_t limit)
{
    struct gdt_entry_tss *entry;

    entry = &gdt_tss;

    entry->limit_bawah = (tak_bertanda16_t)(limit & 0xFFFF);
    entry->base_bawah = (tak_bertanda16_t)(base & 0xFFFF);
    entry->base_tengah = (tak_bertanda8_t)((base >> 16) & 0xFF);
    entry->access = GDT_PRESENT | GDT_DPL_0 | GDT_TYPE_TSS;
    entry->flags_limit = (tak_bertanda8_t)((limit >> 16) & 0x0F);
    entry->base_atas = (tak_bertanda8_t)((base >> 24) & 0xFF);
    entry->base_ext = (tak_bertanda32_t)((base >> 32) & 0xFFFFFFFF);
    entry->reserv = 0;
}

/*
 * _gdt_load
 * ---------
 * Memuat GDT ke register GDTR.
 */
static void _gdt_load(void)
{
    gdt_ptr.batas = sizeof(gdt) + sizeof(gdt_tss) - 1;
    gdt_ptr.alamat = (tak_bertanda64_t)gdt;

    __asm__ __volatile__(
        "lgdt %0\n\t"
        :
        : "m"(gdt_ptr)
    );
}

/*
 * _tss_load
 * ---------
 * Memuat TSS ke register TR.
 */
static void _tss_load(void)
{
    tak_bertanda16_t tr = (GDT_INDEX_TSS_LOW << 3);

    __asm__ __volatile__(
        "ltr %0\n\t"
        :
        : "r"(tr)
    );
}

/*
 * ============================================================================
 * FUNGSI PUBLIC
 * ============================================================================
 */

/*
 * gdt_x86_64_init
 * ---------------
 * Inisialisasi GDT untuk x86_64.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t gdt_x86_64_init(void)
{
    kernel_printf("[GDT-x86_64] Menginisialisasi GDT...\n");

    /* Clear GDT */
    kernel_memset(&gdt, 0, sizeof(gdt));
    kernel_memset(&gdt_tss, 0, sizeof(gdt_tss));
    kernel_memset(&g_tss, 0, sizeof(g_tss));

    /* Entry 0: Null descriptor */
    _gdt_set_entry(GDT_INDEX_NULL, 0, 0, 0, 0);

    /* Entry 1: Kernel Code (64-bit) */
    _gdt_set_entry(GDT_INDEX_KERNEL_CODE, 0, 0,
                   GDT_PRESENT | GDT_DPL_0 | GDT_S_CODE_DATA | GDT_TYPE_CODE,
                   GDT_LONG_MODE);

    /* Entry 2: Kernel Data */
    _gdt_set_entry(GDT_INDEX_KERNEL_DATA, 0, 0,
                   GDT_PRESENT | GDT_DPL_0 | GDT_S_CODE_DATA | GDT_TYPE_DATA,
                   GDT_4K_GRAN);

    /* Entry 3: User Code (64-bit) */
    _gdt_set_entry(GDT_INDEX_USER_CODE, 0, 0,
                   GDT_PRESENT | GDT_DPL_3 | GDT_S_CODE_DATA | GDT_TYPE_CODE,
                   GDT_LONG_MODE);

    /* Entry 4: User Data */
    _gdt_set_entry(GDT_INDEX_USER_DATA, 0, 0,
                   GDT_PRESENT | GDT_DPL_3 | GDT_S_CODE_DATA | GDT_TYPE_DATA,
                   GDT_4K_GRAN);

    /* Entry 5-6: TSS */
    _gdt_set_tss((tak_bertanda64_t)&g_tss, sizeof(g_tss) - 1);

    /* Setup TSS */
    g_tss.iomap_base = sizeof(g_tss);

    /* Load GDT */
    _gdt_load();

    /* Load TSS */
    _tss_load();

    /* Reload segment registers */
    __asm__ __volatile__(
        "mov $0x10, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss\n\t"
        :
        :
        : "ax"
    );

    g_gdt_diinisialisasi = BENAR;

    kernel_printf("[GDT-x86_64] GDT siap\n");

    return STATUS_BERHASIL;
}

/*
 * gdt_x86_64_set_kernel_stack
 * ---------------------------
 * Mengatur stack kernel di TSS.
 *
 * Parameter:
 *   rsp0 - Alamat stack kernel (RSP0)
 */
void gdt_x86_64_set_kernel_stack(tak_bertanda64_t rsp0)
{
    g_tss.rsp0 = rsp0;
}

/*
 * gdt_x86_64_get_tss
 * ------------------
 * Mendapatkan pointer ke TSS.
 *
 * Return:
 *   Pointer ke TSS
 */
void *gdt_x86_64_get_tss(void)
{
    return &g_tss;
}

/*
 * gdt_x86_64_apakah_siap
 * ----------------------
 * Mengecek apakah GDT sudah diinisialisasi.
 *
 * Return:
 *   BENAR jika sudah, SALAH jika belum
 */
bool_t gdt_x86_64_apakah_siap(void)
{
    return g_gdt_diinisialisasi;
}
