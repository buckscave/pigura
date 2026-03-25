/*
 * =============================================================================
 * PIGURA OS - GDT.C
 * =================
 * Implementasi Global Descriptor Table untuk Pigura OS.
 *
 * Berkas ini berisi fungsi-fungsi untuk menginisialisasi dan
 * mengelola GDT (Global Descriptor Table) pada arsitektur x86.
 *
 * Arsitektur: x86/x86_64
 * Versi: 1.0
 * =============================================================================
 */

#include <stdint.h>
#include <stddef.h>

/* =============================================================================
 * KONSTANTA
 * =============================================================================
 */

/* GDT Selector indices */
#define GDT_NULL            0       /* Null descriptor */
#define GDT_KERNEL_CODE     1       /* Kernel code segment */
#define GDT_KERNEL_DATA     2       /* Kernel data segment */
#define GDT_USER_CODE       3       /* User code segment */
#define GDT_USER_DATA       4       /* User data segment */
#define GDT_TSS             5       /* Task State Segment */
#define GDT_ENTRIES         6       /* Total entries */

/* Selector values (index * 8) */
#define SELECTOR_NULL       (GDT_NULL << 3)
#define SELECTOR_KERNEL_CS  (GDT_KERNEL_CODE << 3)
#define SELECTOR_KERNEL_DS  (GDT_KERNEL_DATA << 3)
#define SELECTOR_USER_CS    ((GDT_USER_CODE << 3) | 3)  /* RPL = 3 */
#define SELECTOR_USER_DS    ((GDT_USER_DATA << 3) | 3)  /* RPL = 3 */
#define SELECTOR_TSS        (GDT_TSS << 3)

/* Access byte flags */
#define GDT_ACCESS_PRESENT      0x80    /* Segment present */
#define GDT_ACCESS_DPL_0        0x00    /* Ring 0 */
#define GDT_ACCESS_DPL_3        0x60    /* Ring 3 */
#define GDT_ACCESS_SYSTEM       0x00    /* System segment */
#define GDT_ACCESS_CODE_DATA    0x10    /* Code or data segment */
#define GDT_ACCESS_EXECUTABLE   0x08    /* Executable (code) */
#define GDT_ACCESS_READABLE     0x02    /* Readable (code) */
#define GDT_ACCESS_WRITABLE     0x02    /* Writable (data) */
#define GDT_ACCESS_ACCESSED     0x01    /* Accessed */

/* Flags (granularity byte) */
#define GDT_FLAG_GRANULARITY    0x80    /* 4KB granularity */
#define GDT_FLAG_SIZE_32        0x40    /* 32-bit segment */
#define GDT_FLAG_SIZE_16        0x00    /* 16-bit segment */
#define GDT_FLAG_LONG_MODE      0x20    /* 64-bit segment (long mode) */

/* =============================================================================
 * TIPE DATA
 * =============================================================================
 */

/* GDT entry (8 bytes) */
typedef struct {
    uint16_t limit_low;         /* Limit 0:15 */
    uint16_t base_low;          /* Base 0:15 */
    uint8_t base_middle;        /* Base 16:23 */
    uint8_t access;             /* Access byte */
    uint8_t flags_limit_high;   /* Flags + Limit 16:19 */
    uint8_t base_high;          /* Base 24:31 */
} __attribute__((packed)) gdt_entry_t;

/* GDT pointer */
typedef struct {
    uint16_t limit;             /* Size of GDT - 1 */
    uintptr_t base;             /* Address of GDT (32-bit or 64-bit) */
} __attribute__((packed)) gdt_ptr_t;

/* Task State Segment (TSS) for x86 */
typedef struct {
    uint16_t prev_task;
    uint16_t reserved1;
    uint32_t esp0;
    uint16_t ss0;
    uint16_t reserved2;
    uint32_t esp1;
    uint16_t ss1;
    uint16_t reserved3;
    uint32_t esp2;
    uint16_t ss2;
    uint16_t reserved4;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint16_t es;
    uint16_t reserved5;
    uint16_t cs;
    uint16_t reserved6;
    uint16_t ss;
    uint16_t reserved7;
    uint16_t ds;
    uint16_t reserved8;
    uint16_t fs;
    uint16_t reserved9;
    uint16_t gs;
    uint16_t reserved10;
    uint16_t ldt_selector;
    uint16_t reserved11;
    uint16_t trap_bit;
    uint16_t iomap_base;
} __attribute__((packed)) tss_t;

/* =============================================================================
 * VARIABEL GLOBAL
 * =============================================================================
 */

/* GDT table */
static gdt_entry_t g_gdt[GDT_ENTRIES];

/* GDT pointer */
static gdt_ptr_t g_gdt_ptr;

/* TSS */
static tss_t g_tss;

/* Flag inisialisasi */
static int g_gdt_initialized = 0;

/* =============================================================================
 * FUNGSI ASSEMBLY INLINE
 * =============================================================================
 */

/*
 * _lgdt
 * -----
 * Load GDT register.
 */
static inline void _lgdt(gdt_ptr_t *ptr)
{
    __asm__ __volatile__(
        "lgdt %0"
        :
        : "m"(*ptr)
    );
}

/*
 * _ltr
 * -----
 * Load Task Register.
 */
static inline void _ltr(uint16_t selector)
{
    __asm__ __volatile__(
        "ltr %0"
        :
        : "r"(selector)
    );
}

/*
 * _reload_segments
 * ----------------
 * Reload segment registers.
 * For x86_64, we use a different approach since far jump syntax differs.
 */
static inline void _reload_segments(void)
{
    /* Reload data segment registers */
    __asm__ __volatile__(
        "mov $0x10, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss"
        :
        :
        : "ax"
    );
}

/* =============================================================================
 * FUNGSI INTERNAL
 * =============================================================================
 */

/*
 * _set_gdt_entry
 * --------------
 * Mengatur entry GDT.
 *
 * Parameter:
 *   index   - Index entry dalam GDT
 *   base    - Alamat base segment
 *   limit   - Batas segment
 *   access  - Access byte
 *   flags   - Flags (granularity, size)
 */
static void _set_gdt_entry(int index, uint32_t base, uint32_t limit,
                            uint8_t access, uint8_t flags)
{
    gdt_entry_t *entry;

    entry = &g_gdt[index];

    /* Set base */
    entry->base_low = (uint16_t)(base & 0xFFFF);
    entry->base_middle = (uint8_t)((base >> 16) & 0xFF);
    entry->base_high = (uint8_t)((base >> 24) & 0xFF);

    /* Set limit */
    entry->limit_low = (uint16_t)(limit & 0xFFFF);
    entry->flags_limit_high = (uint8_t)((limit >> 16) & 0x0F);

    /* Set flags */
    entry->flags_limit_high |= (flags << 4);

    /* Set access */
    entry->access = access;
}

/*
 * _init_tss
 * ---------
 * Menginisialisasi TSS.
 */
static void _init_tss(void)
{
    uint8_t *tss_bytes;
    int i;

    tss_bytes = (uint8_t *)&g_tss;

    /* Clear TSS */
    for (i = 0; i < (int)sizeof(tss_t); i++) {
        tss_bytes[i] = 0;
    }

    /* Set kernel stack pointer untuk ring 0 */
    g_tss.ss0 = SELECTOR_KERNEL_DS;
    g_tss.esp0 = 0;     /* Akan di-set saat context switch */

    /* Set iomap base (no I/O bitmap) */
    g_tss.iomap_base = sizeof(tss_t);
}

/*
 * _set_tss_entry
 * --------------
 * Mengatur entry TSS dalam GDT.
 */
static void _set_tss_entry(void)
{
    uintptr_t base;
    uint32_t limit;
    uint8_t access;
    uint8_t flags;

    base = (uintptr_t)&g_tss;
    limit = sizeof(tss_t) - 1;

    /* TSS descriptor: present, DPL 0, system, 32-bit */
    access = GDT_ACCESS_PRESENT |
             GDT_ACCESS_DPL_0 |
             0x09;        /* TSS type (busy = 0x0B) */

    flags = GDT_FLAG_SIZE_32 | GDT_FLAG_GRANULARITY;

    _set_gdt_entry(GDT_TSS, base, limit, access, flags);
}

/* =============================================================================
 * FUNGSI PUBLIC
 * =============================================================================
 */

/*
 * gdt_init
 * --------
 * Menginisialisasi GDT.
 */
void gdt_init(void)
{
    /* Null descriptor */
    _set_gdt_entry(GDT_NULL, 0, 0, 0, 0);

    /* Kernel code segment: base=0, limit=4GB, ring 0, code, 32-bit */
    _set_gdt_entry(GDT_KERNEL_CODE, 0, 0xFFFFF,
                   GDT_ACCESS_PRESENT |
                   GDT_ACCESS_DPL_0 |
                   GDT_ACCESS_CODE_DATA |
                   GDT_ACCESS_EXECUTABLE |
                   GDT_ACCESS_READABLE,
                   GDT_FLAG_GRANULARITY |
                   GDT_FLAG_SIZE_32);

    /* Kernel data segment: base=0, limit=4GB, ring 0, data, 32-bit */
    _set_gdt_entry(GDT_KERNEL_DATA, 0, 0xFFFFF,
                   GDT_ACCESS_PRESENT |
                   GDT_ACCESS_DPL_0 |
                   GDT_ACCESS_CODE_DATA |
                   GDT_ACCESS_WRITABLE,
                   GDT_FLAG_GRANULARITY |
                   GDT_FLAG_SIZE_32);

    /* User code segment: base=0, limit=4GB, ring 3, code, 32-bit */
    _set_gdt_entry(GDT_USER_CODE, 0, 0xFFFFF,
                   GDT_ACCESS_PRESENT |
                   GDT_ACCESS_DPL_3 |
                   GDT_ACCESS_CODE_DATA |
                   GDT_ACCESS_EXECUTABLE |
                   GDT_ACCESS_READABLE,
                   GDT_FLAG_GRANULARITY |
                   GDT_FLAG_SIZE_32);

    /* User data segment: base=0, limit=4GB, ring 3, data, 32-bit */
    _set_gdt_entry(GDT_USER_DATA, 0, 0xFFFFF,
                   GDT_ACCESS_PRESENT |
                   GDT_ACCESS_DPL_3 |
                   GDT_ACCESS_CODE_DATA |
                   GDT_ACCESS_WRITABLE,
                   GDT_FLAG_GRANULARITY |
                   GDT_FLAG_SIZE_32);

    /* Inisialisasi TSS */
    _init_tss();
    _set_tss_entry();

    /* Set GDT pointer */
    g_gdt_ptr.limit = sizeof(g_gdt) - 1;
    g_gdt_ptr.base = (uintptr_t)&g_gdt;

    /* Load GDT */
    _lgdt(&g_gdt_ptr);

    /* Reload segment registers */
    _reload_segments();

    /* Load TR (Task Register) */
    _ltr(SELECTOR_TSS);

    g_gdt_initialized = 1;
}

/*
 * gdt_set_kernel_stack
 * --------------------
 * Mengatur kernel stack dalam TSS.
 */
void gdt_set_kernel_stack(uint32_t esp0)
{
    g_tss.esp0 = esp0;
}

/*
 * gdt_get_kernel_cs
 * -----------------
 * Mendapatkan kernel code selector.
 */
uint16_t gdt_get_kernel_cs(void)
{
    return SELECTOR_KERNEL_CS;
}

/*
 * gdt_get_kernel_ds
 * -----------------
 * Mendapatkan kernel data selector.
 */
uint16_t gdt_get_kernel_ds(void)
{
    return SELECTOR_KERNEL_DS;
}

/*
 * gdt_get_user_cs
 * ---------------
 * Mendapatkan user code selector.
 */
uint16_t gdt_get_user_cs(void)
{
    return SELECTOR_USER_CS;
}

/*
 * gdt_get_user_ds
 * ---------------
 * Mendapatkan user data selector.
 */
uint16_t gdt_get_user_ds(void)
{
    return SELECTOR_USER_DS;
}

/*
 * gdt_get_tss_selector
 * --------------------
 * Mendapatkan TSS selector.
 */
uint16_t gdt_get_tss_selector(void)
{
    return SELECTOR_TSS;
}

/*
 * gdt_is_initialized
 * ------------------
 * Cek apakah GDT sudah diinisialisasi.
 */
int gdt_is_initialized(void)
{
    return g_gdt_initialized;
}

/*
 * gdt_get_tss
 * -----------
 * Mendapatkan pointer ke TSS.
 */
tss_t *gdt_get_tss(void)
{
    return &g_tss;
}

/*
 * gdt_add_entry
 * -------------
 * Menambah entry baru ke GDT.
 *
 * Parameter:
 *   index   - Index entry
 *   base    - Base address
 *   limit   - Segment limit
 *   access  - Access byte
 *   flags   - Flags
 *
 * Return:
 *   0 jika berhasil, -1 jika error
 */
int gdt_add_entry(int index, uint32_t base, uint32_t limit,
                   uint8_t access, uint8_t flags)
{
    if (index < 0 || index >= GDT_ENTRIES) {
        return -1;
    }

    _set_gdt_entry(index, base, limit, access, flags);

    /* Reload GDT */
    _lgdt(&g_gdt_ptr);

    return 0;
}

/*
 * gdt_get_ptr
 * -----------
 * Mendapatkan pointer ke GDT.
 */
gdt_ptr_t *gdt_get_ptr(void)
{
    return &g_gdt_ptr;
}
