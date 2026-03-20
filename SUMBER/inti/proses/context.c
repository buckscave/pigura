/*
 * PIGURA OS - CONTEXT.C
 * ----------------------
 * Implementasi context switching antar proses dan thread.
 *
 * Berkas ini berisi fungsi-fungsi untuk menyimpan dan memulihkan
 * konteks CPU saat terjadi context switch antar proses atau thread.
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

/* Ukuran context structure dalam byte */
#define CONTEXT_UKURAN         72

/* Offset dalam context structure */
#define CONTEXT_EAX            0
#define CONTEXT_EBX            4
#define CONTEXT_ECX            8
#define CONTEXT_EDX            12
#define CONTEXT_ESI            16
#define CONTEXT_EDI            20
#define CONTEXT_EBP            24
#define CONTEXT_ESP            28
#define CONTEXT_EIP            32
#define CONTEXT_EFLAGS         36
#define CONTEXT_CS             40
#define CONTEXT_DS             44
#define CONTEXT_ES             48
#define CONTEXT_FS             52
#define CONTEXT_GS             56
#define CONTEXT_SS             60
#define CONTEXT_CR3            64

/* Flag EFLAGS */
#define EFLAGS_IF              0x200
#define EFLAGS_IOPL            0x3000

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* Struktur context CPU untuk 32-bit */
typedef struct {
    tanda32_t eax;                  /* Register general purpose */
    tanda32_t ebx;
    tanda32_t ecx;
    tanda32_t edx;
    tanda32_t esi;
    tanda32_t edi;
    tanda32_t ebp;
    tanda32_t esp;                  /* Stack pointer */
    tanda32_t eip;                  /* Instruction pointer */
    tak_bertanda32_t eflags;        /* Flags register */
    tak_bertanda16_t cs;            /* Code segment */
    tak_bertanda16_t ds;            /* Data segment */
    tak_bertanda16_t es;
    tak_bertanda16_t fs;
    tak_bertanda16_t gs;
    tak_bertanda16_t ss;            /* Stack segment */
    tak_bertanda32_t cr3;           /* Page directory */
} cpu_context_t;

/* Struktur stack frame untuk interrupt */
typedef struct {
    tak_bertanda32_t gs;
    tak_bertanda32_t fs;
    tak_bertanda32_t es;
    tak_bertanda32_t ds;
    tak_bertanda32_t edi;
    tak_bertanda32_t esi;
    tak_bertanda32_t ebp;
    tak_bertanda32_t esp_kernel;
    tak_bertanda32_t ebx;
    tak_bertanda32_t edx;
    tak_bertanda32_t ecx;
    tak_bertanda32_t eax;
    tak_bertanda32_t int_no;
    tak_bertanda32_t err_code;
    tak_bertanda32_t eip;
    tak_bertanda32_t cs;
    tak_bertanda32_t eflags;
    tak_bertanda32_t esp_user;
    tak_bertanda32_t ss;
} interrupt_frame_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Statistik context switch */
static struct {
    tak_bertanda64_t total_switches;
    tak_bertanda64_t kernel_to_user;
    tak_bertanda64_t user_to_kernel;
    tak_bertanda64_t user_to_user;
} context_stats = {0};

/* Status inisialisasi */
static bool_t context_initialized = SALAH;

/*
 * ============================================================================
 * FUNGSI ASSEMBLY EKSTERNAL (EXTERNAL ASSEMBLY FUNCTIONS)
 * ============================================================================
 */

/* Fungsi context switch level rendah (diimplementasi di assembly) */
extern void context_switch_asm(void *old_ctx, void *new_ctx);

/* Fungsi untuk switch ke user mode */
extern void switch_to_user_mode(void *entry, void *stack,
                                tak_bertanda32_t argc,
                                tak_bertanda32_t argv);

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * context_save
 * ------------
 * Simpan context CPU ke struktur context.
 *
 * Parameter:
 *   ctx - Pointer ke struktur context
 *
 * Return: 0 jika berhasil
 */
static tak_bertanda32_t context_save(cpu_context_t *ctx)
{
    if (ctx == NULL) {
        return 1;
    }

    /* Simpan general purpose registers */
    __asm__ __volatile__(
        "movl %%eax, %0\n\t"
        "movl %%ebx, %1\n\t"
        "movl %%ecx, %2\n\t"
        "movl %%edx, %3\n\t"
        "movl %%esi, %4\n\t"
        "movl %%edi, %5\n\t"
        "movl %%ebp, %6\n\t"
        : "=m"(ctx->eax), "=m"(ctx->ebx), "=m"(ctx->ecx),
          "=m"(ctx->edx), "=m"(ctx->esi), "=m"(ctx->edi),
          "=m"(ctx->ebp)
    );

    /* Simpan segment registers */
    __asm__ __volatile__(
        "movw %%cs, %0\n\t"
        "movw %%ds, %1\n\t"
        "movw %%es, %2\n\t"
        "movw %%fs, %3\n\t"
        "movw %%gs, %4\n\t"
        "movw %%ss, %5\n\t"
        : "=m"(ctx->cs), "=m"(ctx->ds), "=m"(ctx->es),
          "=m"(ctx->fs), "=m"(ctx->gs), "=m"(ctx->ss)
    );

    /* Simpan CR3 */
    ctx->cr3 = cpu_read_cr3();

    /* Simpan EFLAGS */
    __asm__ __volatile__(
        "pushfl\n\t"
        "popl %0\n\t"
        : "=m"(ctx->eflags)
    );

    return 0;
}

/*
 * context_restore
 * ---------------
 * Pulihkan context CPU dari struktur context.
 *
 * Parameter:
 *   ctx - Pointer ke struktur context
 */
static void context_restore(cpu_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    /* Pulihkan CR3 jika berbeda */
    if (ctx->cr3 != cpu_read_cr3()) {
        cpu_write_cr3(ctx->cr3);
    }

    /* Pulihkan segment registers */
    __asm__ __volatile__(
        "movw %0, %%ds\n\t"
        "movw %1, %%es\n\t"
        "movw %2, %%fs\n\t"
        "movw %3, %%gs\n\t"
        :
        : "r"(ctx->ds), "r"(ctx->es), "r"(ctx->fs), "r"(ctx->gs)
    );

    /* Pulihkan general purpose registers */
    __asm__ __volatile__(
        "movl %0, %%eax\n\t"
        "movl %1, %%ebx\n\t"
        "movl %2, %%ecx\n\t"
        "movl %3, %%edx\n\t"
        "movl %4, %%esi\n\t"
        "movl %5, %%edi\n\t"
        "movl %6, %%ebp\n\t"
        :
        : "m"(ctx->eax), "m"(ctx->ebx), "m"(ctx->ecx),
          "m"(ctx->edx), "m"(ctx->esi), "m"(ctx->edi),
          "m"(ctx->ebp)
    );

    /* Pulihkan EFLAGS */
    __asm__ __volatile__(
        "pushl %0\n\t"
        "popfl\n\t"
        :
        : "m"(ctx->eflags)
    );
}

/*
 * context_alloc
 * -------------
 * Alokasikan struktur context baru.
 *
 * Return: Pointer ke context, atau NULL jika gagal
 */
static cpu_context_t *context_alloc(void)
{
    cpu_context_t *ctx;

    ctx = (cpu_context_t *)kmalloc(sizeof(cpu_context_t));

    if (ctx == NULL) {
        return NULL;
    }

    kernel_memset(ctx, 0, sizeof(cpu_context_t));

    return ctx;
}

/*
 * context_free
 * ------------
 * Bebaskan struktur context.
 *
 * Parameter:
 *   ctx - Pointer ke context
 */
static void context_free(cpu_context_t *ctx)
{
    if (ctx != NULL) {
        kfree(ctx);
    }
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * context_init
 * ------------
 * Inisialisasi subsistem context.
 *
 * Return: Status operasi
 */
status_t context_init(void)
{
    if (context_initialized) {
        return STATUS_SUDAH_ADA;
    }

    /* Reset statistik */
    kernel_memset(&context_stats, 0, sizeof(context_stats));

    context_initialized = BENAR;

    kernel_printf("[CONTEXT] Context manager initialized\n");

    return STATUS_BERHASIL;
}

/*
 * context_create
 * --------------
 * Buat context baru untuk proses/thread.
 *
 * Parameter:
 *   entry_point - Alamat entry point
 *   stack_ptr   - Pointer ke stack
 *   is_user     - Flag user mode
 *
 * Return: Pointer ke context, atau NULL jika gagal
 */
void *context_create(alamat_virtual_t entry_point, void *stack_ptr,
                     bool_t is_user)
{
    cpu_context_t *ctx;

    ctx = context_alloc();
    if (ctx == NULL) {
        return NULL;
    }

    /* Set entry point */
    ctx->eip = (tanda32_t)entry_point;

    /* Set stack pointer */
    ctx->esp = (tanda32_t)stack_ptr;

    /* Set segment selectors */
    if (is_user) {
        ctx->cs = SELECTOR_USER_CODE;
        ctx->ds = SELECTOR_USER_DATA;
        ctx->es = SELECTOR_USER_DATA;
        ctx->fs = SELECTOR_USER_DATA;
        ctx->gs = SELECTOR_USER_DATA;
        ctx->ss = SELECTOR_USER_DATA;
    } else {
        ctx->cs = SELECTOR_KERNEL_CODE;
        ctx->ds = SELECTOR_KERNEL_DATA;
        ctx->es = SELECTOR_KERNEL_DATA;
        ctx->fs = SELECTOR_KERNEL_DATA;
        ctx->gs = SELECTOR_KERNEL_DATA;
        ctx->ss = SELECTOR_KERNEL_DATA;
    }

    /* Set EFLAGS dengan interrupt enabled */
    ctx->eflags = EFLAGS_IF;

    /* Set IOPL untuk user mode */
    if (is_user) {
        ctx->eflags |= EFLAGS_IOPL;
    }

    return (void *)ctx;
}

/*
 * context_destroy
 * ---------------
 * Hancurkan context.
 *
 * Parameter:
 *   ctx - Pointer ke context
 */
void context_destroy(void *ctx)
{
    context_free((cpu_context_t *)ctx);
}

/*
 * context_switch
 * --------------
 * Lakukan context switch dari proses lama ke proses baru.
 *
 * Parameter:
 *   old_proses - Pointer ke proses lama (boleh NULL)
 *   new_proses - Pointer ke proses baru
 */
void context_switch(proses_t *old_proses, proses_t *new_proses)
{
    cpu_context_t *old_ctx;
    cpu_context_t *new_ctx;

    if (!context_initialized || new_proses == NULL) {
        return;
    }

    /* Update statistik */
    context_stats.total_switches++;

    /* Dapatkan context */
    new_ctx = (cpu_context_t *)new_proses->main_thread->context;

    if (new_ctx == NULL) {
        kernel_printf("[CONTEXT] Error: context NULL untuk PID %lu\n",
                      (tak_bertanda64_t)new_proses->pid);
        return;
    }

    /* Simpan context lama */
    if (old_proses != NULL && old_proses->main_thread != NULL &&
        old_proses->main_thread->context != NULL) {

        old_ctx = (cpu_context_t *)old_proses->main_thread->context;
        context_save(old_ctx);
    }

    /* Set current process */
    proses_set_current(new_proses);

    /* Switch CR3 jika berbeda address space */
    if (new_proses->vm != NULL) {
        /* Proses user - switch ke page directory proses */
        if (new_ctx->cr3 != cpu_read_cr3()) {
            cpu_write_cr3(new_ctx->cr3);
        }
    } else {
        /* Proses kernel - gunakan kernel page directory */
        cpu_reload_cr3();
    }

    /* Pulihkan context baru */
    context_restore(new_ctx);

    /* Jump ke context baru */
    __asm__ __volatile__(
        "movl %0, %%esp\n\t"
        "movl %1, %%ebp\n\t"
        "jmp *%2\n\t"
        :
        : "r"(new_ctx->esp), "r"(new_ctx->ebp), "r"(new_ctx->eip)
        : "memory"
    );
}

/*
 * context_switch_to
 * -----------------
 * Switch ke context tertentu tanpa menyimpan context lama.
 *
 * Parameter:
 *   ctx - Pointer ke context
 */
void context_switch_to(void *ctx)
{
    cpu_context_t *context;

    if (ctx == NULL) {
        return;
    }

    context = (cpu_context_t *)ctx;

    /* Switch CR3 */
    if (context->cr3 != 0 && context->cr3 != cpu_read_cr3()) {
        cpu_write_cr3(context->cr3);
    }

    /* Pulihkan context */
    context_restore(context);

    /* Jump ke context */
    __asm__ __volatile__(
        "movl %0, %%esp\n\t"
        "movl %1, %%ebp\n\t"
        "jmp *%2\n\t"
        :
        : "r"(context->esp), "r"(context->ebp), "r"(context->eip)
        : "memory"
    );
}

/*
 * context_set_entry
 * -----------------
 * Set entry point context.
 *
 * Parameter:
 *   ctx         - Pointer ke context
 *   entry_point - Alamat entry point
 *
 * Return: Status operasi
 */
status_t context_set_entry(void *ctx, alamat_virtual_t entry_point)
{
    cpu_context_t *context;

    if (ctx == NULL) {
        return STATUS_PARAM_INVALID;
    }

    context = (cpu_context_t *)ctx;
    context->eip = (tanda32_t)entry_point;

    return STATUS_BERHASIL;
}

/*
 * context_set_stack
 * -----------------
 * Set stack pointer context.
 *
 * Parameter:
 *   ctx        - Pointer ke context
 *   stack_ptr  - Pointer ke stack
 *
 * Return: Status operasi
 */
status_t context_set_stack(void *ctx, void *stack_ptr)
{
    cpu_context_t *context;

    if (ctx == NULL) {
        return STATUS_PARAM_INVALID;
    }

    context = (cpu_context_t *)ctx;
    context->esp = (tanda32_t)stack_ptr;

    return STATUS_BERHASIL;
}

/*
 * context_set_cr3
 * ---------------
 * Set CR3 (page directory) context.
 *
 * Parameter:
 *   ctx  - Pointer ke context
 *   cr3  - Nilai CR3
 *
 * Return: Status operasi
 */
status_t context_set_cr3(void *ctx, tak_bertanda32_t cr3)
{
    cpu_context_t *context;

    if (ctx == NULL) {
        return STATUS_PARAM_INVALID;
    }

    context = (cpu_context_t *)ctx;
    context->cr3 = cr3;

    return STATUS_BERHASIL;
}

/*
 * context_get_stack
 * -----------------
 * Dapatkan stack pointer dari context.
 *
 * Parameter:
 *   ctx - Pointer ke context
 *
 * Return: Stack pointer
 */
void *context_get_stack(void *ctx)
{
    cpu_context_t *context;

    if (ctx == NULL) {
        return NULL;
    }

    context = (cpu_context_t *)ctx;
    return (void *)(alamat_ptr_t)context->esp;
}

/*
 * context_get_entry
 * -----------------
 * Dapatkan entry point dari context.
 *
 * Parameter:
 *   ctx - Pointer ke context
 *
 * Return: Entry point
 */
alamat_virtual_t context_get_entry(void *ctx)
{
    cpu_context_t *context;

    if (ctx == NULL) {
        return 0;
    }

    context = (cpu_context_t *)ctx;
    return (alamat_virtual_t)context->eip;
}

/*
 * context_setup_user
 * ------------------
 * Setup context untuk user mode.
 *
 * Parameter:
 *   ctx         - Pointer ke context
 *   entry_point - Entry point
 *   stack_top   - Alamat atas stack
 *   argc        - Jumlah argumen
 *   argv        - Pointer ke array argumen
 *
 * Return: Status operasi
 */
status_t context_setup_user(void *ctx, alamat_virtual_t entry_point,
                            alamat_virtual_t stack_top,
                            tak_bertanda32_t argc, tak_bertanda32_t argv)
{
    cpu_context_t *context;
    tak_bertanda32_t *stack;
    tak_bertanda32_t i;

    if (ctx == NULL) {
        return STATUS_PARAM_INVALID;
    }

    context = (cpu_context_t *)ctx;

    /* Set segment selectors untuk user mode */
    context->cs = SELECTOR_USER_CODE | 0x03;
    context->ds = SELECTOR_USER_DATA | 0x03;
    context->es = SELECTOR_USER_DATA | 0x03;
    context->fs = SELECTOR_USER_DATA | 0x03;
    context->gs = SELECTOR_USER_DATA | 0x03;
    context->ss = SELECTOR_USER_DATA | 0x03;

    /* Set EFLAGS dengan IF dan IOPL(3) */
    context->eflags = EFLAGS_IF | (3 << 12);

    /* Set entry point */
    context->eip = (tanda32_t)entry_point;

    /* Setup stack untuk iret */
    stack = (tak_bertanda32_t *)stack_top;

    /* Push argument count dan pointer */
    stack--;
    *stack = argv;
    stack--;
    *stack = argc;

    /* Set stack pointer */
    context->esp = (tanda32_t)stack;

    return STATUS_BERHASIL;
}

/*
 * context_setup_kernel
 * --------------------
 * Setup context untuk kernel thread.
 *
 * Parameter:
 *   ctx         - Pointer ke context
 *   entry_point - Entry point
 *   arg         - Argumen untuk thread
 *
 * Return: Status operasi
 */
status_t context_setup_kernel(void *ctx, alamat_virtual_t entry_point,
                              void *arg)
{
    cpu_context_t *context;
    tak_bertanda32_t *stack;
    void *stack_mem;
    ukuran_t stack_size;

    if (ctx == NULL) {
        return STATUS_PARAM_INVALID;
    }

    context = (cpu_context_t *)ctx;

    /* Set segment selectors untuk kernel mode */
    context->cs = SELECTOR_KERNEL_CODE;
    context->ds = SELECTOR_KERNEL_DATA;
    context->es = SELECTOR_KERNEL_DATA;
    context->fs = SELECTOR_KERNEL_DATA;
    context->gs = SELECTOR_KERNEL_DATA;
    context->ss = SELECTOR_KERNEL_DATA;

    /* Set EFLAGS dengan IF */
    context->eflags = EFLAGS_IF;

    /* Set entry point */
    context->eip = (tanda32_t)entry_point;

    /* Alokasikan stack kernel */
    stack_size = CONFIG_STACK_KERNEL;
    stack_mem = kmalloc(stack_size);

    if (stack_mem == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    /* Setup stack */
    stack = (tak_bertanda32_t *)((tak_bertanda8_t *)stack_mem + stack_size);

    /* Push dummy return address */
    stack--;
    *stack = 0;

    /* Push argument */
    stack--;
    *stack = (tak_bertanda32_t)(alamat_ptr_t)arg;

    /* Set stack pointer */
    context->esp = (tanda32_t)stack;

    return STATUS_BERHASIL;
}

/*
 * context_get_stats
 * -----------------
 * Dapatkan statistik context switch.
 *
 * Parameter:
 *   total         - Pointer untuk total switches
 *   kernel_to_user - Pointer untuk kernel to user switches
 *   user_to_kernel - Pointer untuk user to kernel switches
 *   user_to_user   - Pointer untuk user to user switches
 */
void context_get_stats(tak_bertanda64_t *total,
                       tak_bertanda64_t *kernel_to_user,
                       tak_bertanda64_t *user_to_kernel,
                       tak_bertanda64_t *user_to_user)
{
    if (total != NULL) {
        *total = context_stats.total_switches;
    }

    if (kernel_to_user != NULL) {
        *kernel_to_user = context_stats.kernel_to_user;
    }

    if (user_to_kernel != NULL) {
        *user_to_kernel = context_stats.user_to_kernel;
    }

    if (user_to_user != NULL) {
        *user_to_user = context_stats.user_to_user;
    }
}

/*
 * context_print_info
 * ------------------
 * Print informasi context.
 *
 * Parameter:
 *   ctx - Pointer ke context
 */
void context_print_info(void *ctx)
{
    cpu_context_t *context;

    if (ctx == NULL) {
        kernel_printf("[CONTEXT] Context: NULL\n");
        return;
    }

    context = (cpu_context_t *)ctx;

    kernel_printf("\n[CONTEXT] Informasi Context:\n");
    kernel_printf("========================================\n");
    kernel_printf("  EIP:    0x%08lX\n", (tak_bertanda32_t)context->eip);
    kernel_printf("  ESP:    0x%08lX\n", context->esp);
    kernel_printf("  EBP:    0x%08lX\n", context->ebp);
    kernel_printf("  EFLAGS: 0x%08lX\n", context->eflags);
    kernel_printf("  CR3:    0x%08lX\n", context->cr3);
    kernel_printf("  CS:     0x%04X  DS: 0x%04X\n",
                  context->cs, context->ds);
    kernel_printf("  SS:     0x%04X\n", context->ss);
    kernel_printf("========================================\n");
}
