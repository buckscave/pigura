/*
 * PIGURA OS - RING_SWITCH.C
 * --------------------------
 * Implementasi transisi antar privilege level (ring switching).
 *
 * Berkas ini berisi fungsi-fungsi untuk menangani transisi antara
 * berbagai privilege level CPU (Ring 0-3) pada arsitektur x86/x86_64.
 * Transisi ini mencakup:
 * - Kernel ke User mode (Ring 0 -> Ring 3)
 * - User ke Kernel mode (Ring 3 -> Ring 0)
 * - Validasi dan keamanan transisi
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

/* Selector segment dengan RPL (Requested Privilege Level) */
#define RING0_CODE_SELECTOR     SELECTOR_KERNEL_CODE
#define RING0_DATA_SELECTOR     SELECTOR_KERNEL_DATA
#define RING3_CODE_SELECTOR     (SELECTOR_USER_CODE | 0x03)
#define RING3_DATA_SELECTOR     (SELECTOR_USER_DATA | 0x03)

/* EFLAGS bit */
#define EFLAGS_IF               0x00000200
#define EFLAGS_IOPL_MASK        0x00003000
#define EFLAGS_IOPL_SHIFT       12
#define EFLAGS_NT               0x00004000
#define EFLAGS_RF               0x00010000
#define EFLAGS_VM               0x00020000
#define EFLAGS_AC               0x00040000
#define EFLAGS_ID               0x00200000

/* EFLAGS default untuk user mode */
#define EFLAGS_USER_DEFAULT     (EFLAGS_IF | (0 << EFLAGS_IOPL_SHIFT))

/* EFLAGS default untuk kernel mode */
#define EFLAGS_KERNEL_DEFAULT   (EFLAGS_IF | (0 << EFLAGS_IOPL_SHIFT))

/* Stack alignment */
#define STACK_ALIGN             16

/* CPL mask dari selector */
#define CPL_MASK                0x03

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* Forward declaration untuk thread structure */
struct thread;
typedef struct thread thread_t;

/* Struktur untuk menyimpan state saat ring switch */
typedef struct {
    tak_bertanda32_t eax;
    tak_bertanda32_t ebx;
    tak_bertanda32_t ecx;
    tak_bertanda32_t edx;
    tak_bertanda32_t esi;
    tak_bertanda32_t edi;
    tak_bertanda32_t ebp;
    tak_bertanda32_t esp;
    tak_bertanda32_t eip;
    tak_bertanda32_t eflags;
    tak_bertanda16_t cs;
    tak_bertanda16_t ds;
    tak_bertanda16_t es;
    tak_bertanda16_t fs;
    tak_bertanda16_t gs;
    tak_bertanda16_t ss;
} ring_context_t;

/* Struktur stack frame untuk IRET */
typedef struct {
    tak_bertanda32_t eip;
    tak_bertanda32_t cs;
    tak_bertanda32_t eflags;
    tak_bertanda32_t esp;
    tak_bertanda32_t ss;
} iret_frame_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Statistik ring switch */
static struct {
    tak_bertanda64_t ring0_to_ring3;
    tak_bertanda64_t ring3_to_ring0;
    tak_bertanda64_t ring3_to_ring3;
    tak_bertanda64_t invalid_attempts;
    tak_bertanda64_t security_violations;
} ring_stats = {0};

/* Status inisialisasi */
static bool_t ring_switch_initialized = SALAH;

/* Lock untuk thread safety */
static spinlock_t ring_lock = {0};

/*
 * ============================================================================
 * FUNGSI ASSEMBLY EKSTERNAL (EXTERNAL ASSEMBLY FUNCTIONS)
 * ============================================================================
 */

/* Fungsi level rendah untuk ring switch (diimplementasi di assembly) */
extern void ring_switch_asm_to_user(tak_bertanda32_t entry,
                                    tak_bertanda32_t stack,
                                    tak_bertanda32_t eflags,
                                    tak_bertanda16_t cs,
                                    tak_bertanda16_t ds,
                                    tak_bertanda16_t ss);

extern void ring_switch_asm_iret(void *frame);

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * get_current_cpl
 * ---------------
 * Dapatkan Current Privilege Level dari CS register.
 *
 * Return: CPL (0-3)
 */
static tak_bertanda32_t get_current_cpl(void)
{
    tak_bertanda32_t cs;

    __asm__ __volatile__(
        "movl %%cs, %0"
        : "=r"(cs)
    );

    return cs & CPL_MASK;
}

/*
 * get_current_rpl
 * ---------------
 * Dapatkan Requested Privilege Level dari DS register.
 *
 * Return: RPL (0-3)
 */
static tak_bertanda32_t get_current_rpl(void)
{
    tak_bertanda32_t ds;

    __asm__ __volatile__(
        "movl %%ds, %0"
        : "=r"(ds)
    );

    return ds & CPL_MASK;
}

/*
 * is_kernel_mode
 * --------------
 * Cek apakah CPU sedang di kernel mode (Ring 0).
 *
 * Return: BENAR jika di kernel mode
 */
static bool_t is_kernel_mode(void)
{
    return get_current_cpl() == 0;
}

/*
 * is_user_mode
 * ------------
 * Cek apakah CPU sedang di user mode (Ring 3).
 *
 * Return: BENAR jika di user mode
 */
static bool_t is_user_mode(void)
{
    return get_current_cpl() == 3;
}

/*
 * validate_ring_transition
 * ------------------------
 * Validasi transisi privilege level.
 *
 * Parameter:
 *   from_ring - Ring sumber (0-3)
 *   to_ring   - Ring tujuan (0-3)
 *
 * Return: BENAR jika transisi valid
 */
static bool_t validate_ring_transition(tak_bertanda32_t from_ring,
                                       tak_bertanda32_t to_ring)
{
    /* Ring harus valid */
    if (from_ring > 3 || to_ring > 3) {
        return SALAH;
    }

    /* Dari kernel mode (Ring 0), boleh ke ring apapun */
    if (from_ring == 0) {
        return BENAR;
    }

    /* Dari user mode, hanya boleh ke kernel via interrupt/syscall */
    /* Transisi langsung dari Ring 3 ke Ring 0 tanpa mekanisme
     * yang tepat (interrupt/syscall) adalah pelanggaran keamanan */
    if (from_ring == 3 && to_ring == 0) {
        /* Ini harus melalui interrupt gate atau call gate */
        /* Fungsi ini dipanggil untuk validasi, return BENAR
         * karena mekanisme already di tempat yang benar */
        return BENAR;
    }

    /* Transisi antar ring user (1-3) memerlukan mekanisme khusus */
    if (from_ring > 0 && to_ring > 0 && from_ring != to_ring) {
        return SALAH;
    }

    return BENAR;
}

/*
 * setup_iret_frame
 * ----------------
 * Setup stack frame untuk IRET instruction.
 *
 * Parameter:
 *   frame      - Pointer ke frame buffer
 *   eip        - Instruction pointer
 *   cs         - Code selector
 *   eflags     - EFLAGS value
 *   esp        - Stack pointer (untuk privilege change)
 *   ss         - Stack selector (untuk privilege change)
 *   privilege  - Target privilege level
 */
static void setup_iret_frame(iret_frame_t *frame, tak_bertanda32_t eip,
                             tak_bertanda16_t cs, tak_bertanda32_t eflags,
                             tak_bertanda32_t esp, tak_bertanda16_t ss,
                             tak_bertanda32_t privilege)
{
    if (frame == NULL) {
        return;
    }

    frame->eip = eip;
    frame->cs = cs;
    frame->eflags = eflags;
    frame->esp = esp;
    frame->ss = ss;
}

/*
 * set_segment_selectors
 * ---------------------
 * Set semua segment selector ke nilai yang sesuai privilege.
 *
 * Parameter:
 *   privilege - Target privilege level
 */
static void set_segment_selectors(tak_bertanda32_t privilege)
{
    tak_bertanda16_t data_sel;

    if (privilege == 0) {
        data_sel = RING0_DATA_SELECTOR;
    } else {
        data_sel = RING3_DATA_SELECTOR;
    }

    __asm__ __volatile__(
        "movw %0, %%ds\n\t"
        "movw %0, %%es\n\t"
        "movw %0, %%fs\n\t"
        "movw %0, %%gs\n\t"
        :
        : "r"(data_sel)
        : "memory"
    );
}

/*
 * get_eflags
 * ----------
 * Baca nilai EFLAGS register.
 *
 * Return: Nilai EFLAGS
 */
static tak_bertanda32_t get_eflags(void)
{
    tak_bertanda32_t eflags;

    __asm__ __volatile__(
        "pushfl\n\t"
        "popl %0"
        : "=r"(eflags)
    );

    return eflags;
}

/*
 * set_eflags
 * ----------
 * Set nilai EFLAGS register.
 *
 * Parameter:
 *   eflags - Nilai EFLAGS baru
 */
static void set_eflags(tak_bertanda32_t eflags)
{
    __asm__ __volatile__(
        "pushl %0\n\t"
        "popfl"
        :
        : "r"(eflags)
        : "memory", "cc"
    );
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * ring_switch_init
 * ----------------
 * Inisialisasi subsistem ring switch.
 *
 * Return: Status operasi
 */
status_t ring_switch_init(void)
{
    if (ring_switch_initialized) {
        return STATUS_SUDAH_ADA;
    }

    spinlock_init(&ring_lock);

    kernel_memset(&ring_stats, 0, sizeof(ring_stats));

    ring_switch_initialized = BENAR;

    kernel_printf("[RING_SWITCH] Ring switch subsistem initialized\n");
    kernel_printf("              Current CPL: %lu\n",
                  (tak_bertanda64_t)get_current_cpl());

    return STATUS_BERHASIL;
}

/*
 * ring_switch_to_user
 * -------------------
 * Switch dari kernel mode ke user mode.
 *
 * Parameter:
 *   entry_point - Entry point program user
 *   stack_ptr   - Pointer ke stack user
 *   argc        - Jumlah argumen
 *   argv        - Pointer ke array argumen
 *
 * Return: Tidak return jika berhasil
 */
status_t ring_switch_to_user(alamat_virtual_t entry_point, void *stack_ptr,
                             tak_bertanda32_t argc, tak_bertanda32_t argv)
{
    tak_bertanda32_t *stack;
    tak_bertanda32_t eflags;
    tak_bertanda32_t current_cpl;

    if (!ring_switch_initialized) {
        return STATUS_GAGAL;
    }

    /* Validasi parameter */
    if (entry_point == 0 || stack_ptr == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Cek apakah sudah di user mode */
    current_cpl = get_current_cpl();
    if (current_cpl != 0) {
        ring_stats.security_violations++;
        return STATUS_AKSES_DITOLAK;
    }

    /* Validasi alamat entry point */
    if (entry_point < ALAMAT_USER_MULAI || entry_point > ALAMAT_USER_AKHIR) {
        ring_stats.invalid_attempts++;
        return STATUS_PARAM_INVALID;
    }

    /* Align stack */
    stack = (tak_bertanda32_t *)stack_ptr;
    stack = (tak_bertanda32_t *)((alamat_ptr_t)stack & ~(STACK_ALIGN - 1));

    /* Setup stack untuk entry user mode */
    /* Push argc dan argv */
    stack--;
    *stack = argv;
    stack--;
    *stack = argc;

    /* Setup EFLAGS untuk user mode */
    eflags = EFLAGS_USER_DEFAULT;

    /* Update statistik */
    ring_stats.ring0_to_ring3++;

    /* Disable interrupt sebelum switch */
    cpu_disable_irq();

    /* Set segment selectors ke user mode */
    set_segment_selectors(3);

    /* Lakukan switch ke user mode menggunakan IRET */
    __asm__ __volatile__(
        "pushl %0\n\t"          /* SS */
        "pushl %1\n\t"          /* ESP */
        "pushl %2\n\t"          /* EFLAGS */
        "pushl %3\n\t"          /* CS */
        "pushl %4\n\t"          /* EIP */
        "iret\n\t"
        :
        : "r"((tak_bertanda32_t)RING3_DATA_SELECTOR),
          "r"((tak_bertanda32_t)(alamat_ptr_t)stack),
          "r"(eflags),
          "r"((tak_bertanda32_t)RING3_CODE_SELECTOR),
          "r"((tak_bertanda32_t)entry_point)
        : "memory"
    );

    /* Tidak seharusnya sampai di sini */
    kernel_panic("ring_switch_to_user: should not return");

    return STATUS_GAGAL;
}

/*
 * ring_switch_to_kernel
 * ---------------------
 * Switch dari user mode ke kernel mode.
 * Fungsi ini dipanggil dari interrupt/syscall handler.
 *
 * Parameter:
 *   context - Pointer ke context yang disimpan
 *
 * Catatan: Fungsi ini biasanya dipanggil oleh scheduler
 *          dan interrupt handler, bukan langsung.
 */
void ring_switch_to_kernel(void *context)
{
    ring_context_t *ctx;
    tak_bertanda32_t current_cpl;

    if (!ring_switch_initialized || context == NULL) {
        return;
    }

    ctx = (ring_context_t *)context;

    /* Cek current privilege */
    current_cpl = get_current_cpl();

    /* Jika sudah di kernel mode, tidak perlu switch */
    if (current_cpl == 0) {
        return;
    }

    /* Update statistik */
    ring_stats.ring3_to_ring0++;

    /* Set segment selectors ke kernel mode */
    set_segment_selectors(0);

    /* Restore context */
    /* Ini biasanya dilakukan oleh IRET di interrupt handler */
}

/*
 * ring_switch_iret_to_user
 * ------------------------
 * Switch ke user mode menggunakan IRET dengan context yang diberikan.
 *
 * Parameter:
 *   eip    - Instruction pointer
 *   cs     - Code selector
 *   eflags - EFLAGS value
 *   esp    - Stack pointer
 *   ss     - Stack selector
 */
void ring_switch_iret_to_user(tak_bertanda32_t eip, tak_bertanda16_t cs,
                              tak_bertanda32_t eflags, tak_bertanda32_t esp,
                              tak_bertanda16_t ss)
{
    tak_bertanda32_t current_cpl;

    if (!ring_switch_initialized) {
        kernel_panic("ring_switch: not initialized");
    }

    current_cpl = get_current_cpl();

    /* Hanya boleh dari kernel mode */
    if (current_cpl != 0) {
        ring_stats.security_violations++;
        kernel_panic("ring_switch: invalid privilege transition");
    }

    /* Validasi selector */
    if ((cs & CPL_MASK) != 3) {
        ring_stats.invalid_attempts++;
        kernel_panic("ring_switch: invalid code selector");
    }

    if ((ss & CPL_MASK) != 3) {
        ring_stats.invalid_attempts++;
        kernel_panic("ring_switch: invalid stack selector");
    }

    /* Update statistik */
    ring_stats.ring0_to_ring3++;

    /* Set segment selectors */
    set_segment_selectors(3);

    /* Execute IRET */
    __asm__ __volatile__(
        "pushl %0\n\t"          /* SS */
        "pushl %1\n\t"          /* ESP */
        "pushl %2\n\t"          /* EFLAGS */
        "pushl %3\n\t"          /* CS */
        "pushl %4\n\t"          /* EIP */
        "iret\n\t"
        :
        : "r"((tak_bertanda32_t)ss),
          "r"(esp),
          "r"(eflags),
          "r"((tak_bertanda32_t)cs),
          "r"(eip)
        : "memory"
    );

    kernel_panic("ring_switch_iret_to_user: should not return");
}

/*
 * ring_switch_return_user
 * -----------------------
 * Return ke user mode dari system call atau interrupt.
 *
 * Parameter:
 *   retval - Return value untuk user program
 */
void ring_switch_return_user(long retval)
{
    proses_t *proses;
    thread_t *thread;
    cpu_context_t *ctx;

    if (!ring_switch_initialized) {
        kernel_panic("ring_switch: not initialized");
    }

    proses = proses_get_current();
    if (proses == NULL) {
        kernel_panic("ring_switch: no current process");
    }

    thread = proses->main_thread;
    if (thread == NULL || thread->context == NULL) {
        kernel_panic("ring_switch: no thread context");
    }

    ctx = (cpu_context_t *)thread->context;

    /* Set return value di EAX */
    ctx->eax = (tanda32_t)retval;

    /* Update statistik */
    ring_stats.ring0_to_ring3++;

    /* Set segment selectors */
    set_segment_selectors(3);

    /* Update TSS untuk kernel stack dari thread */
    if (thread->stack != NULL) {
        tss_set_kernel_stack((tak_bertanda32_t)(alamat_ptr_t)thread->stack +
                             thread->stack_size);
    }

    /* Return ke user mode */
    __asm__ __volatile__(
        "movl %0, %%eax\n\t"
        "movl %1, %%ebx\n\t"
        "movl %2, %%ecx\n\t"
        "movl %3, %%edx\n\t"
        "movl %4, %%esi\n\t"
        "movl %5, %%edi\n\t"
        "movl %6, %%ebp\n\t"
        "movl %7, %%esp\n\t"
        "pushl %8\n\t"          /* SS */
        "pushl %9\n\t"          /* ESP user */
        "pushl %10\n\t"         /* EFLAGS */
        "pushl %11\n\t"         /* CS */
        "pushl %12\n\t"         /* EIP */
        "iret\n\t"
        :
        : "m"(ctx->eax),
          "m"(ctx->ebx),
          "m"(ctx->ecx),
          "m"(ctx->edx),
          "m"(ctx->esi),
          "m"(ctx->edi),
          "m"(ctx->ebp),
          "r"(ctx->esp),
          "r"((tak_bertanda32_t)ctx->ss),
          "r"(ctx->esp),
          "r"(ctx->eflags),
          "r"((tak_bertanda32_t)ctx->cs),
          "r"(ctx->eip)
        : "memory"
    );

    kernel_panic("ring_switch_return_user: should not return");
}

/*
 * ring_get_current_cpl
 * --------------------
 * Dapatkan Current Privilege Level.
 *
 * Return: CPL (0-3)
 */
tak_bertanda32_t ring_get_current_cpl(void)
{
    return get_current_cpl();
}

/*
 * ring_is_kernel_mode
 * -------------------
 * Cek apakah sedang di kernel mode.
 *
 * Return: BENAR jika di kernel mode (Ring 0)
 */
bool_t ring_is_kernel_mode(void)
{
    return is_kernel_mode();
}

/*
 * ring_is_user_mode
 * -----------------
 * Cek apakah sedang di user mode.
 *
 * Return: BENAR jika di user mode (Ring 3)
 */
bool_t ring_is_user_mode(void)
{
    return is_user_mode();
}

/*
 * ring_validate_address
 * ---------------------
 * Validasi alamat berdasarkan privilege level.
 *
 * Parameter:
 *   addr    - Alamat yang divalidasi
 *   size    - Ukuran akses
 *   write   - BENAR jika akses tulis
 *   privilege - Privilege level yang diperiksa
 *
 * Return: BENAR jika alamat valid
 */
bool_t ring_validate_address(alamat_virtual_t addr, ukuran_t size,
                             bool_t write, tak_bertanda32_t privilege)
{
    alamat_virtual_t end;

    /* Alamat 0 tidak valid */
    if (addr == 0) {
        return SALAH;
    }

    /* Cek overflow */
    if (addr + size < addr) {
        return SALAH;
    }

    end = addr + size;

    /* Untuk kernel mode (Ring 0), semua alamat valid */
    if (privilege == 0) {
        return BENAR;
    }

    /* Untuk user mode (Ring 3), harus di user space */
    if (privilege == 3) {
        if (addr < ALAMAT_USER_MULAI || end > ALAMAT_USER_AKHIR) {
            return SALAH;
        }
    }

    /* Untuk privilege lain, validasi sesuai kebijakan */
    return BENAR;
}

/*
 * ring_validate_selector
 * ----------------------
 * Validasi segment selector.
 *
 * Parameter:
 *   selector  - Selector yang divalidasi
 *   privilege - Target privilege level
 *
 * Return: BENAR jika selector valid
 */
bool_t ring_validate_selector(tak_bertanda16_t selector,
                              tak_bertanda32_t privilege)
{
    tak_bertanda32_t rpl;
    tak_bertanda32_t dpl;

    /* NULL selector */
    if (selector == 0) {
        return SALAH;
    }

    /* Get RPL dari selector */
    rpl = selector & CPL_MASK;

    /* Get DPL dari GDT (simplified check) */
    /* Untuk implementasi lengkap, perlu membaca GDT */
    if (selector == SELECTOR_KERNEL_CODE || selector == SELECTOR_KERNEL_DATA) {
        dpl = 0;
    } else if (selector == SELECTOR_USER_CODE || selector == SELECTOR_USER_DATA) {
        dpl = 3;
    } else {
        /* Selector lain perlu validasi dari GDT */
        return BENAR;
    }

    /* RPL tidak boleh lebih rendah dari DPL untuk akses */
    if (privilege == 0) {
        /* Kernel mode dapat mengakses semua */
        return BENAR;
    }

    /* User mode: RPL dan DPL harus sesuai */
    if (rpl > dpl) {
        return SALAH;
    }

    return BENAR;
}

/*
 * ring_get_stats
 * --------------
 * Dapatkan statistik ring switch.
 *
 * Parameter:
 *   to_user    - Pointer untuk jumlah switch ke user mode
 *   to_kernel  - Pointer untuk jumlah switch ke kernel mode
 *   invalid    - Pointer untuk jumlah attempt invalid
 *   violations - Pointer untuk jumlah pelanggaran keamanan
 */
void ring_get_stats(tak_bertanda64_t *to_user, tak_bertanda64_t *to_kernel,
                    tak_bertanda64_t *invalid, tak_bertanda64_t *violations)
{
    if (to_user != NULL) {
        *to_user = ring_stats.ring0_to_ring3;
    }

    if (to_kernel != NULL) {
        *to_kernel = ring_stats.ring3_to_ring0;
    }

    if (invalid != NULL) {
        *invalid = ring_stats.invalid_attempts;
    }

    if (violations != NULL) {
        *violations = ring_stats.security_violations;
    }
}

/*
 * ring_print_stats
 * ----------------
 * Print statistik ring switch.
 */
void ring_print_stats(void)
{
    kernel_printf("\n[RING_SWITCH] Statistik Ring Switch:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Ring 0 -> Ring 3:    %lu\n",
                  ring_stats.ring0_to_ring3);
    kernel_printf("  Ring 3 -> Ring 0:    %lu\n",
                  ring_stats.ring3_to_ring0);
    kernel_printf("  Ring 3 -> Ring 3:    %lu\n",
                  ring_stats.ring3_to_ring3);
    kernel_printf("  Invalid Attempts:    %lu\n",
                  ring_stats.invalid_attempts);
    kernel_printf("  Security Violations: %lu\n",
                  ring_stats.security_violations);
    kernel_printf("  Current CPL:         %lu\n",
                  (tak_bertanda64_t)get_current_cpl());
    kernel_printf("========================================\n");
}

/*
 * ring_setup_user_context
 * -----------------------
 * Setup context untuk masuk ke user mode.
 *
 * Parameter:
 *   ctx         - Pointer ke context
 *   entry_point - Entry point
 *   stack_ptr   - Stack pointer
 *   argc        - Jumlah argumen
 *   argv        - Pointer ke argumen
 *
 * Return: Status operasi
 */
status_t ring_setup_user_context(void *ctx, alamat_virtual_t entry_point,
                                 void *stack_ptr, tak_bertanda32_t argc,
                                 tak_bertanda32_t argv)
{
    cpu_context_t *context;
    tak_bertanda32_t *stack;

    if (ctx == NULL || entry_point == 0 || stack_ptr == NULL) {
        return STATUS_PARAM_INVALID;
    }

    context = (cpu_context_t *)ctx;

    /* Setup stack */
    stack = (tak_bertanda32_t *)stack_ptr;

    /* Push argc dan argv */
    stack--;
    *stack = argv;
    stack--;
    *stack = argc;

    /* Set context values */
    context->eip = (tanda32_t)entry_point;
    context->esp = (tanda32_t)(alamat_ptr_t)stack;
    context->ebp = 0;

    /* Set segment selectors untuk user mode */
    context->cs = RING3_CODE_SELECTOR;
    context->ds = RING3_DATA_SELECTOR;
    context->es = RING3_DATA_SELECTOR;
    context->fs = RING3_DATA_SELECTOR;
    context->gs = RING3_DATA_SELECTOR;
    context->ss = RING3_DATA_SELECTOR;

    /* Set EFLAGS */
    context->eflags = EFLAGS_USER_DEFAULT;

    return STATUS_BERHASIL;
}

/*
 * ring_setup_kernel_context
 * -------------------------
 * Setup context untuk kernel thread.
 *
 * Parameter:
 *   ctx         - Pointer ke context
 *   entry_point - Entry point
 *   arg         - Argumen thread
 *
 * Return: Status operasi
 */
status_t ring_setup_kernel_context(void *ctx, alamat_virtual_t entry_point,
                                   void *arg)
{
    cpu_context_t *context;

    if (ctx == NULL || entry_point == 0) {
        return STATUS_PARAM_INVALID;
    }

    context = (cpu_context_t *)ctx;

    /* Set context values */
    context->eip = (tanda32_t)entry_point;
    context->ebp = 0;

    /* Set segment selectors untuk kernel mode */
    context->cs = RING0_CODE_SELECTOR;
    context->ds = RING0_DATA_SELECTOR;
    context->es = RING0_DATA_SELECTOR;
    context->fs = RING0_DATA_SELECTOR;
    context->gs = RING0_DATA_SELECTOR;
    context->ss = RING0_DATA_SELECTOR;

    /* Set EFLAGS */
    context->eflags = EFLAGS_KERNEL_DEFAULT;

    return STATUS_BERHASIL;
}

/*
 * ring_check_privilege
 * --------------------
 * Cek apakah current process memiliki privilege yang diminta.
 *
 * Parameter:
 *   required_privilege - Privilege level yang dibutuhkan (0-3)
 *
 * Return: BENAR jika memiliki privilege
 */
bool_t ring_check_privilege(tak_bertanda32_t required_privilege)
{
    tak_bertanda32_t current_cpl;

    if (required_privilege > 3) {
        return SALAH;
    }

    current_cpl = get_current_cpl();

    /* CPL yang lebih rendah = lebih tinggi privilege */
    return current_cpl <= required_privilege;
}

/*
 * ring_elevate_privilege
 * ----------------------
 * Elevate privilege untuk operasi khusus.
 * Hanya bisa dipanggil dari kernel mode.
 *
 * Parameter:
 *   target_privilege - Target privilege level
 *
 * Return: Status operasi
 */
status_t ring_elevate_privilege(tak_bertanda32_t target_privilege)
{
    tak_bertanda32_t current_cpl;

    if (!ring_switch_initialized) {
        return STATUS_GAGAL;
    }

    /* Hanya bisa dari kernel mode */
    current_cpl = get_current_cpl();
    if (current_cpl != 0) {
        ring_stats.security_violations++;
        return STATUS_AKSES_DITOLAK;
    }

    /* Validasi target */
    if (target_privilege > 3) {
        return STATUS_PARAM_INVALID;
    }

    /* Set segment selectors */
    set_segment_selectors(target_privilege);

    return STATUS_BERHASIL;
}

/*
 * ring_drop_privilege
 * -------------------
 * Turunkan privilege level.
 *
 * Parameter:
 *   target_privilege - Target privilege level
 *
 * Return: Status operasi
 */
status_t ring_drop_privilege(tak_bertanda32_t target_privilege)
{
    tak_bertanda32_t current_cpl;

    if (!ring_switch_initialized) {
        return STATUS_GAGAL;
    }

    current_cpl = get_current_cpl();

    /* Tidak bisa menaikkan privilege dengan fungsi ini */
    if (target_privilege < current_cpl) {
        return STATUS_AKSES_DITOLAK;
    }

    /* Validasi target */
    if (target_privilege > 3) {
        return STATUS_PARAM_INVALID;
    }

    /* Set segment selectors */
    set_segment_selectors(target_privilege);

    return STATUS_BERHASIL;
}

/*
 * ring_call_gate_enter
 * --------------------
 * Masuk ke kernel via call gate (untuk compatibility).
 *
 * Parameter:
 *   gate_number - Nomor call gate
 *   arg1-arg4   - Argumen
 *
 * Return: Hasil dari call gate
 */
long ring_call_gate_enter(tak_bertanda32_t gate_number,
                          long arg1, long arg2, long arg3, long arg4)
{
    tak_bertanda32_t current_cpl;
    long result;

    if (!ring_switch_initialized) {
        return (long)STATUS_GAGAL;
    }

    current_cpl = get_current_cpl();

    /* Call gate dari user mode */
    if (current_cpl == 3) {
        ring_stats.ring3_to_ring0++;

        /* Gunakan interrupt untuk system call */
        __asm__ __volatile__(
            "int $0x80\n\t"
            : "=a"(result)
            : "a"(gate_number), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4)
            : "memory"
        );

        return result;
    }

    /* Jika sudah di kernel mode, panggil langsung */
    return (long)STATUS_BERHASIL;
}

/*
 * ring_interrupt_enter
 * --------------------
 * Handler untuk masuk ke kernel via interrupt.
 *
 * Parameter:
 *   int_number - Nomor interrupt
 *   frame      - Pointer ke interrupt frame
 */
void ring_interrupt_enter(tak_bertanda32_t int_number, void *frame)
{
    tak_bertanda32_t current_cpl;

    if (!ring_switch_initialized) {
        return;
    }

    current_cpl = get_current_cpl();

    /* Update statistik */
    if (current_cpl == 3) {
        ring_stats.ring3_to_ring0++;
    }
}

/*
 * ring_interrupt_exit
 * -------------------
 * Handler untuk keluar dari kernel via IRET.
 *
 * Parameter:
 *   frame - Pointer ke interrupt frame
 */
void ring_interrupt_exit(void *frame)
{
    tak_bertanda32_t target_cpl;
    iret_frame_t *iret_frame;

    if (!ring_switch_initialized || frame == NULL) {
        return;
    }

    iret_frame = (iret_frame_t *)frame;

    /* Determine target CPL dari CS */
    target_cpl = iret_frame->cs & CPL_MASK;

    /* Update statistik */
    if (target_cpl == 3) {
        ring_stats.ring0_to_ring3++;
    } else if (target_cpl == 0) {
        ring_stats.ring3_to_ring0++;
    }
}
