/*
 * PIGURA OS - TSS.C
 * ------------------
 * Implementasi Task State Segment (TSS).
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola TSS
 * yang digunakan untuk context switching dan privilege switching.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../kernel.h"

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
static spinlock_t tss_lock = {0};

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * tss_setup_gdt_entry
 * -------------------
 * Setup entry GDT untuk TSS.
 *
 * Parameter:
 *   selector - Selector TSS
 *   tss      - Pointer ke TSS
 */
static void tss_setup_gdt_entry(tak_bertanda16_t selector, tss_t *tss)
{
    tak_bertanda32_t base;
    tak_bertanda32_t limit;
    gdt_entry_t *entry;
    tak_bertanda32_t index;

    if (tss == NULL) {
        return;
    }

    base = (tak_bertanda32_t)(alamat_ptr_t)tss;
    limit = sizeof(tss_t) - 1;
    index = selector >> 3;

    /* Dapatkan entry GDT */
    entry = gdt_get_entry(index);
    if (entry == NULL) {
        return;
    }

    /* Set base */
    entry->base_low = base & 0xFFFF;
    entry->base_middle = (base >> 16) & 0xFF;
    entry->base_high = (base >> 24) & 0xFF;

    /* Set limit */
    entry->limit_low = limit & 0xFFFF;
    entry->granularity = (limit >> 16) & 0x0F;

    /* Set attributes */
    entry->access = TSS_ATTRIBUTES;

    /* Set granularity */
    entry->granularity |= 0x00;  /* Byte granularity */
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
        : "r"(selector)
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

    /* Setup GDT entry */
    tss_setup_gdt_entry(TSS_SELECTOR, tss_kernel);

    /* Load TSS */
    tss_load(TSS_SELECTOR);

    /* Simpan ke array */
    tss_per_cpu[cpu_id] = tss_kernel;

    tss_initialized = BENAR;

    kernel_printf("[TSS] Task State Segment initialized\n");
    kernel_printf("      TSS at 0x%08lX, Selector: 0x%02X\n",
                  (tak_bertanda32_t)(alamat_ptr_t)tss_kernel,
                  TSS_SELECTOR);

    return STATUS_BERHASIL;
}

/*
 * tss_get_current
 * ---------------
 * Dapatkan TSS CPU saat ini.
 *
 * Return: Pointer ke TSS
 */
tss_t *tss_get_current(void)
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
void tss_set_kernel_stack(tak_bertanda32_t esp0)
{
    tss_t *tss;

    tss = tss_get_current();
    if (tss == NULL) {
        return;
    }

    tss->esp0 = esp0;
}

/*
 * tss_set_cr3
 * -----------
 * Set CR3 untuk TSS.
 *
 * Parameter:
 *   cr3 - Nilai CR3 (page directory)
 */
void tss_set_cr3(tak_bertanda32_t cr3)
{
    tss_t *tss;

    tss = tss_get_current();
    if (tss == NULL) {
        return;
    }

    tss->cr3 = cr3;
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
        tss_set_io_permission(start + i, allow);
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

    tss = tss_get_current();
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
 * tss_create
 * ----------
 * Buat TSS baru untuk proses.
 *
 * Return: Pointer ke TSS, atau NULL
 */
tss_t *tss_create(void)
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
 * tss_destroy
 * -----------
 * Hancurkan TSS.
 *
 * Parameter:
 *   tss - Pointer ke TSS
 */
void tss_destroy(tss_t *tss)
{
    if (tss == NULL) {
        return;
    }

    /* Jangan hapus TSS kernel */
    if (tss == tss_kernel) {
        return;
    }

    kfree(tss);
}

/*
 * tss_print_info
 * --------------
 * Print informasi TSS.
 *
 * Parameter:
 *   tss - Pointer ke TSS
 */
void tss_print_info(tss_t *tss)
{
    if (tss == NULL) {
        kernel_printf("[TSS] TSS: NULL\n");
        return;
    }

    kernel_printf("\n[TSS] Informasi Task State Segment:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Previous Task:  0x%04X\n", tss->prev_task);
    kernel_printf("  ESP0:           0x%08lX\n", tss->esp0);
    kernel_printf("  SS0:            0x%04X\n", tss->ss0);
    kernel_printf("  ESP1:           0x%08lX\n", tss->esp1);
    kernel_printf("  SS1:            0x%04X\n", tss->ss1);
    kernel_printf("  ESP2:           0x%08lX\n", tss->esp2);
    kernel_printf("  SS2:            0x%04X\n", tss->ss2);
    kernel_printf("  CR3:            0x%08lX\n", tss->cr3);
    kernel_printf("  EIP:            0x%08lX\n", tss->eip);
    kernel_printf("  EFLAGS:         0x%08lX\n", tss->eflags);
    kernel_printf("  I/O Map Base:   0x%04X\n", tss->iomap_base);
    kernel_printf("========================================\n");
}

/*
 * tss_save_state
 * --------------
 * Simpan state ke TSS.
 *
 * Parameter:
 *   tss    - Pointer ke TSS
 *   state  - Pointer ke state
 */
void tss_save_state(tss_t *tss, const cpu_state_t *state)
{
    if (tss == NULL || state == NULL) {
        return;
    }

    tss->eax = state->eax;
    tss->ebx = state->ebx;
    tss->ecx = state->ecx;
    tss->edx = state->edx;
    tss->esi = state->esi;
    tss->edi = state->edi;
    tss->ebp = state->ebp;
    tss->esp = state->esp;
    tss->eip = state->eip;
    tss->eflags = state->eflags;

    tss->cs = state->cs;
    tss->ds = state->ds;
    tss->es = state->es;
    tss->fs = state->fs;
    tss->gs = state->gs;
    tss->ss = state->ss;
}

/*
 * tss_load_state
 * --------------
 * Load state dari TSS.
 *
 * Parameter:
 *   tss   - Pointer ke TSS
 *   state - Pointer ke state
 */
void tss_load_state(const tss_t *tss, cpu_state_t *state)
{
    if (tss == NULL || state == NULL) {
        return;
    }

    state->eax = tss->eax;
    state->ebx = tss->ebx;
    state->ecx = tss->ecx;
    state->edx = tss->edx;
    state->esi = tss->esi;
    state->edi = tss->edi;
    state->ebp = tss->ebp;
    state->esp = tss->esp;
    state->eip = tss->eip;
    state->eflags = tss->eflags;

    state->cs = tss->cs;
    state->ds = tss->ds;
    state->es = tss->es;
    state->fs = tss->fs;
    state->gs = tss->gs;
    state->ss = tss->ss;
}
