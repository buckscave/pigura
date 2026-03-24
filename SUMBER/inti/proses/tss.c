/*
 * PIGURA OS - TSS.C
 * ------------------
 * Implementasi Task State Segment (TSS).
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola TSS
 * yang digunakan untuk context switching dan privilege switching.
 * Khusus untuk arsitektur x86 dan x86_64.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua fungsi diimplementasikan lengkap
 * - Batas 80 karakter per baris
 *
 * Versi: 3.1
 * Tanggal: 2025
 */

#include "../kernel.h"

#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)

#include "proses.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* Selector TSS */
#define TSS_SELECTOR            0x28

/* Attribute TSS dalam GDT */
#define TSS_ATTRIBUTES          0x89  /* Present, 32-bit, TSS type */

/* Jumlah TSS maksimum (per CPU) */
#define TSS_MAX_PER_CPU         256

/* IO Permission Bitmap offset */
#define TSS_IOPB_OFFSET         104

/*
 * ============================================================================
 * STRUKTUR DATA GDT (GDT DATA STRUCTURES)
 * ============================================================================
 * Struktur GDT entry untuk x86/x86_64
 */

typedef struct gdt_entry {
    tak_bertanda16_t limit_low;     /* Bit 0-15 dari limit */
    tak_bertanda16_t base_low;      /* Bit 0-15 dari base */
    tak_bertanda8_t  base_middle;   /* Bit 16-23 dari base */
    tak_bertanda8_t  access;        /* Access byte */
    tak_bertanda8_t  granularity;   /* Flags dan bit 16-19 dari limit */
    tak_bertanda8_t  base_high;     /* Bit 24-31 dari base */
} gdt_entry_t;

/*
 * ============================================================================
 * STRUKTUR DATA TSS (TSS DATA STRUCTURES)
 * ============================================================================
 */

/* Struktur TSS 32-bit */
typedef struct {
    tak_bertanda16_t prev_task;     /* Link ke TSS sebelumnya */
    tak_bertanda16_t reserved1;
    
    tak_bertanda32_t esp0;          /* Stack pointer untuk ring 0 */
    tak_bertanda16_t ss0;           /* Stack selector untuk ring 0 */
    tak_bertanda16_t reserved2;
    
    tak_bertanda32_t esp1;          /* Stack pointer untuk ring 1 */
    tak_bertanda16_t ss1;           /* Stack selector untuk ring 1 */
    tak_bertanda16_t reserved3;
    
    tak_bertanda32_t esp2;          /* Stack pointer untuk ring 2 */
    tak_bertanda16_t ss2;           /* Stack selector untuk ring 2 */
    tak_bertanda16_t reserved4;
    
    tak_bertanda32_t cr3;           /* Page directory base */
    tak_bertanda32_t eip;           /* Instruction pointer */
    tak_bertanda32_t eflags;        /* Flags register */
    
    tak_bertanda32_t eax;           /* General purpose registers */
    tak_bertanda32_t ecx;
    tak_bertanda32_t edx;
    tak_bertanda32_t ebx;
    
    tak_bertanda32_t esp;           /* Stack pointer */
    tak_bertanda32_t ebp;           /* Base pointer */
    tak_bertanda32_t esi;           /* Source index */
    tak_bertanda32_t edi;           /* Destination index */
    
    tak_bertanda16_t es;            /* Segment selectors */
    tak_bertanda16_t reserved5;
    tak_bertanda16_t cs;
    tak_bertanda16_t reserved6;
    tak_bertanda16_t ss;
    tak_bertanda16_t reserved7;
    tak_bertanda16_t ds;
    tak_bertanda16_t reserved8;
    tak_bertanda16_t fs;
    tak_bertanda16_t reserved9;
    tak_bertanda16_t gs;
    tak_bertanda16_t reserved10;
    
    tak_bertanda16_t ldt;           /* LDT selector */
    tak_bertanda16_t reserved11;
    
    tak_bertanda16_t trap;          /* Trap flag */
    tak_bertanda16_t iomap_base;    /* I/O map base address */
} tss_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* TSS untuk setiap CPU */
static tss_t *tss_per_cpu[CONFIG_MAKS_CPU];

/* TSS untuk kernel */
static tss_t *tss_kernel;

/* I/O Permission Bitmap */
static tak_bertanda8_t iopb[8192];  /* 65536 bits = 8192 bytes */

/* Status inisialisasi */
static bool_t tss_initialized = SALAH;

/* Lock */
static spinlock_t tss_lock;

/* GDT table reference (akan di-set dari luar) */
static gdt_entry_t *gdt_table = NULL;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * gdt_set_tss_entry
 * -----------------
 * Set entry GDT untuk TSS.
 *
 * Parameter:
 *   index - Index GDT
 *   base  - Alamat base TSS
 *   limit - Limit TSS
 */
static void gdt_set_tss_entry(tak_bertanda32_t index, tak_bertanda32_t base,
                              tak_bertanda32_t limit)
{
    gdt_entry_t *entry;
    
    if (gdt_table == NULL || index >= GDT_ENTRI) {
        return;
    }
    
    entry = &gdt_table[index];
    
    /* Set base */
    entry->base_low = base & 0xFFFF;
    entry->base_middle = (base >> 16) & 0xFF;
    entry->base_high = (base >> 24) & 0xFF;
    
    /* Set limit */
    entry->limit_low = limit & 0xFFFF;
    entry->granularity = (limit >> 16) & 0x0F;
    
    /* Set attributes */
    entry->access = TSS_ATTRIBUTES;
    
    /* Set granularity (byte granularity) */
    entry->granularity &= 0x0F;  /* Clear granularity flags */
}

/*
 * tss_load
 * --------
 * Load TSS ke register TR.
 *
 * Parameter:
 *   selector - Selector TSS
 */
static void tss_load(tak_bertanda16_t selector)
{
    __asm__ __volatile__(
        "ltr %0"
        :
        : "rm"(selector)
    );
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * tss_init
 * --------
 * Inisialisasi subsistem TSS.
 *
 * Return: Status operasi
 */
status_t tss_init(void)
{
    tak_bertanda32_t cpu_id;
    
    if (tss_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
    spinlock_init(&tss_lock);
    
    /* Reset array */
    kernel_memset(tss_per_cpu, 0, sizeof(tss_per_cpu));
    
    /* Inisialisasi I/O Permission Bitmap */
    kernel_memset(iopb, 0xFF, sizeof(iopb));  /* Block semua I/O */
    
    /* Dapatkan CPU ID */
    cpu_id = hal_cpu_get_id();
    
    /* Alokasikan TSS kernel */
    tss_kernel = (tss_t *)kmalloc(sizeof(tss_t));
    if (tss_kernel == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    kernel_memset(tss_kernel, 0, sizeof(tss_t));
    
    /* Setup TSS kernel */
    tss_kernel->ss0 = SELECTOR_KERNEL_DATA;
    tss_kernel->esp0 = 0;  /* Akan di-set saat context switch */
    tss_kernel->iomap_base = sizeof(tss_t);
    
    /* Setup GDT entry untuk TSS */
    gdt_set_tss_entry(5, (tak_bertanda32_t)(alamat_ptr_t)tss_kernel,
                      sizeof(tss_t) - 1);
    
    /* Load TSS */
    tss_load(TSS_SELECTOR);
    
    /* Simpan ke array */
    if (cpu_id < CONFIG_MAKS_CPU) {
        tss_per_cpu[cpu_id] = tss_kernel;
    }
    
    tss_initialized = BENAR;
    
    kernel_printf("[TSS] Task State Segment initialized\n");
    kernel_printf("      TSS at 0x%08lX, Selector: 0x%02X\n",
                  (tak_bertanda32_t)(alamat_ptr_t)tss_kernel,
                  TSS_SELECTOR);
    
    return STATUS_BERHASIL;
}

/*
 * tss_dapat_saat_ini
 * ------------------
 * Dapatkan TSS CPU saat ini.
 *
 * Return: Pointer ke TSS
 */
void *tss_dapat_saat_ini(void)
{
    tak_bertanda32_t cpu_id;
    
    cpu_id = hal_cpu_get_id();
    
    if (cpu_id >= CONFIG_MAKS_CPU) {
        return NULL;
    }
    
    return tss_per_cpu[cpu_id];
}

/*
 * tss_set_kernel_stack
 * --------------------
 * Set stack kernel untuk TSS.
 *
 * Parameter:
 *   esp0 - Stack pointer ring 0
 */
void tss_set_kernel_stack(tak_bertanda64_t esp0)
{
    tss_t *tss;
    
    tss = (tss_t *)tss_dapat_saat_ini();
    if (tss == NULL) {
        return;
    }
    
    tss->esp0 = (tak_bertanda32_t)esp0;
}

/*
 * tss_set_cr3
 * -----------
 * Set CR3 untuk TSS.
 *
 * Parameter:
 *   cr3 - Nilai CR3 (page directory)
 */
void tss_set_cr3(tak_bertanda64_t cr3)
{
    tss_t *tss;
    
    tss = (tss_t *)tss_dapat_saat_ini();
    if (tss == NULL) {
        return;
    }
    
    tss->cr3 = (tak_bertanda32_t)cr3;
}

/*
 * tss_set_io_permission
 * ---------------------
 * Set permission port I/O.
 *
 * Parameter:
 *   port  - Nomor port
 *   allow - BENAR untuk allow, SALAH untuk block
 */
void tss_set_io_permission(tak_bertanda16_t port, bool_t allow)
{
    tak_bertanda32_t byte_index;
    tak_bertanda8_t bit_mask;
    
    byte_index = port / 8;
    bit_mask = 1 << (port % 8);
    
    if (byte_index >= sizeof(iopb)) {
        return;
    }
    
    if (allow) {
        /* Clear bit = allow */
        iopb[byte_index] &= ~bit_mask;
    } else {
        /* Set bit = block */
        iopb[byte_index] |= bit_mask;
    }
}

/*
 * tss_set_io_permission_range
 * ---------------------------
 * Set permission range port I/O.
 *
 * Parameter:
 *   start - Port awal
 *   count - Jumlah port
 *   allow - BENAR untuk allow
 */
void tss_set_io_permission_range(tak_bertanda16_t start,
                                 tak_bertanda32_t count, bool_t allow)
{
    tak_bertanda32_t i;
    
    for (i = 0; i < count; i++) {
        tss_set_io_permission(start + (tak_bertanda16_t)i, allow);
    }
}

/*
 * tss_set_io_bitmap
 * -----------------
 * Set seluruh I/O bitmap.
 *
 * Parameter:
 *   bitmap - Pointer ke bitmap (boleh NULL untuk block all)
 */
void tss_set_io_bitmap(const tak_bertanda8_t *bitmap)
{
    tss_t *tss;
    
    tss = (tss_t *)tss_dapat_saat_ini();
    if (tss == NULL) {
        return;
    }
    
    if (bitmap == NULL) {
        /* Block semua I/O */
        kernel_memset(iopb, 0xFF, sizeof(iopb));
    } else {
        kernel_memcpy(iopb, bitmap, sizeof(iopb));
    }
    
    /* Update iomap_base */
    tss->iomap_base = sizeof(tss_t);
}

/*
 * tss_buat
 * --------
 * Buat TSS baru untuk proses.
 *
 * Return: Pointer ke TSS, atau NULL
 */
void *tss_buat(void)
{
    tss_t *tss;
    
    if (!tss_initialized) {
        return NULL;
    }
    
    tss = (tss_t *)kmalloc(sizeof(tss_t));
    if (tss == NULL) {
        return NULL;
    }
    
    kernel_memset(tss, 0, sizeof(tss_t));
    
    /* Set default values */
    tss->ss0 = SELECTOR_KERNEL_DATA;
    tss->iomap_base = sizeof(tss_t);
    
    return tss;
}

/*
 * tss_hancurkan
 * -------------
 * Hancurkan TSS.
 *
 * Parameter:
 *   tss - Pointer ke TSS
 */
void tss_hancurkan(void *tss_ptr)
{
    tss_t *tss_local = (tss_t *)tss_ptr;
    
    if (tss_local == NULL) {
        return;
    }
    
    /* Jangan hapus TSS kernel */
    if (tss_local == tss_kernel) {
        return;
    }
    
    kfree(tss_local);
}

/*
 * tss_print_info
 * --------------
 * Print informasi TSS.
 *
 * Parameter:
 *   tss - Pointer ke TSS
 */
void tss_print_info(void *tss_ptr)
{
    tss_t *tss_local = (tss_t *)tss_ptr;
    
    if (tss_local == NULL) {
        kernel_printf("[TSS] TSS: NULL\n");
        return;
    }
    
    kernel_printf("\n[TSS] Informasi Task State Segment:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Previous Task:  0x%04X\n", tss_local->prev_task);
    kernel_printf("  ESP0:           0x%08lX\n", tss_local->esp0);
    kernel_printf("  SS0:            0x%04X\n", tss_local->ss0);
    kernel_printf("  ESP1:           0x%08lX\n", tss_local->esp1);
    kernel_printf("  SS1:            0x%04X\n", tss_local->ss1);
    kernel_printf("  ESP2:           0x%08lX\n", tss_local->esp2);
    kernel_printf("  SS2:            0x%04X\n", tss_local->ss2);
    kernel_printf("  CR3:            0x%08lX\n", tss_local->cr3);
    kernel_printf("  EIP:            0x%08lX\n", tss_local->eip);
    kernel_printf("  EFLAGS:         0x%08lX\n", tss_local->eflags);
    kernel_printf("  I/O Map Base:   0x%04X\n", tss_local->iomap_base);
    kernel_printf("========================================\n");
}

/*
 * tss_simpan_state
 * ----------------
 * Simpan state ke TSS.
 *
 * Parameter:
 *   tss   - Pointer ke TSS
 *   state - Pointer ke state
 */
void tss_simpan_state(void *tss_ptr, const void *state)
{
    tss_t *tss_local = (tss_t *)tss_ptr;
    const cpu_context_t *ctx = (const cpu_context_t *)state;
    
    if (tss_local == NULL || ctx == NULL) {
        return;
    }
    
    /* Simpan sesuai arsitektur */
#if defined(PIGURA_ARSITEKTUR_64BIT)
    /* Untuk 64-bit, gunakan register 64-bit */
    tss_local->eax = (tak_bertanda32_t)ctx->rax;
    tss_local->ecx = (tak_bertanda32_t)ctx->rcx;
    tss_local->edx = (tak_bertanda32_t)ctx->rdx;
    tss_local->ebx = (tak_bertanda32_t)ctx->rbx;
    tss_local->esi = (tak_bertanda32_t)ctx->rsi;
    tss_local->edi = (tak_bertanda32_t)ctx->rdi;
    tss_local->ebp = (tak_bertanda32_t)ctx->rbp;
    tss_local->esp = (tak_bertanda32_t)ctx->rsp;
    tss_local->eip = (tak_bertanda32_t)ctx->rip;
    tss_local->eflags = (tak_bertanda32_t)ctx->rflags;
#else
    /* Untuk 32-bit */
    tss_local->eax = ctx->eax;
    tss_local->ecx = ctx->ecx;
    tss_local->edx = ctx->edx;
    tss_local->ebx = ctx->ebx;
    tss_local->esi = ctx->esi;
    tss_local->edi = ctx->edi;
    tss_local->ebp = ctx->ebp;
    tss_local->esp = ctx->esp;
    tss_local->eip = ctx->eip;
    tss_local->eflags = ctx->eflags;
#endif
    
    tss_local->cs = ctx->cs;
    tss_local->ds = ctx->ds;
    tss_local->es = ctx->es;
    tss_local->fs = ctx->fs;
    tss_local->gs = ctx->gs;
    tss_local->ss = ctx->ss;
}

/*
 * tss_muat_state
 * --------------
 * Load state dari TSS.
 *
 * Parameter:
 *   tss   - Pointer ke TSS
 *   state - Pointer ke state
 */
void tss_muat_state(const void *tss_ptr, void *state)
{
    const tss_t *tss_local = (const tss_t *)tss_ptr;
    cpu_context_t *ctx = (cpu_context_t *)state;
    
    if (tss_local == NULL || ctx == NULL) {
        return;
    }
    
    /* Load sesuai arsitektur */
#if defined(PIGURA_ARSITEKTUR_64BIT)
    /* Untuk 64-bit */
    ctx->rax = tss_local->eax;
    ctx->rcx = tss_local->ecx;
    ctx->rdx = tss_local->edx;
    ctx->rbx = tss_local->ebx;
    ctx->rsi = tss_local->esi;
    ctx->rdi = tss_local->edi;
    ctx->rbp = tss_local->ebp;
    ctx->rsp = tss_local->esp;
    ctx->rip = tss_local->eip;
    ctx->rflags = tss_local->eflags;
#else
    /* Untuk 32-bit */
    ctx->eax = tss_local->eax;
    ctx->ecx = tss_local->ecx;
    ctx->edx = tss_local->edx;
    ctx->ebx = tss_local->ebx;
    ctx->esi = tss_local->esi;
    ctx->edi = tss_local->edi;
    ctx->ebp = tss_local->ebp;
    ctx->esp = tss_local->esp;
    ctx->eip = tss_local->eip;
    ctx->eflags = tss_local->eflags;
#endif
    
    ctx->cs = tss_local->cs;
    ctx->ds = tss_local->ds;
    ctx->es = tss_local->es;
    ctx->fs = tss_local->fs;
    ctx->gs = tss_local->gs;
    ctx->ss = tss_local->ss;
}

/*
 * tss_set_gdt_table
 * -----------------
 * Set pointer ke GDT table.
 *
 * Parameter:
 *   table - Pointer ke GDT table
 */
void tss_set_gdt_table(void *table)
{
    gdt_table = (gdt_entry_t *)table;
}

#endif /* ARSITEKTUR_X86 || ARSITEKTUR_X86_64 */
