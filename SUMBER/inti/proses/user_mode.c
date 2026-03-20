/*
 * PIGURA OS - USER_MODE.C
 * ------------------------
 * Implementasi utilitas mode user.
 *
 * Berkas ini berisi fungsi-fungsi untuk menangani transisi
 * antara kernel mode dan user mode.
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

/* Selector segment untuk user mode */
#define USER_CODE_SELECTOR      (SELECTOR_USER_CODE | 0x03)
#define USER_DATA_SELECTOR      (SELECTOR_USER_DATA | 0x03)

/* EFLAGS untuk user mode */
#define USER_EFLAGS_BASE        0x202  /* IF = 1, bit 1 = 1 */
#define USER_EFLAGS_IOPL        0x000  /* IOPL = 0 */

/* Stack alignment */
#define STACK_ALIGN             16

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Statistik user mode */
static struct {
    tak_bertanda64_t transitions_to_user;
    tak_bertanda64_t transitions_to_kernel;
    tak_bertanda64_t exceptions;
} user_mode_stats = {0};

/* Status inisialisasi */
static bool_t user_mode_initialized = SALAH;

/*
 * ============================================================================
 * FUNGSI ASSEMBLY (ASSEMBLY FUNCTIONS)
 * ============================================================================
 */

/* Entry point ke user mode (di assembly) */
extern void user_mode_entry(void);

/* Exit dari user mode (di assembly) */
extern void user_mode_exit(void);

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * setup_user_stack
 * ----------------
 * Setup stack untuk masuk ke user mode.
 *
 * Parameter:
 *   stack_top - Alamat atas stack
 *   entry     - Entry point
 *   arg       - Argumen program
 *
 * Return: Stack pointer baru
 */
static tak_bertanda32_t setup_user_stack(tak_bertanda32_t stack_top,
                                         tak_bertanda32_t entry,
                                         tak_bertanda32_t arg)
{
    tak_bertanda32_t *stack;

    /* Align stack */
    stack_top &= ~(STACK_ALIGN - 1);

    stack = (tak_bertanda32_t *)stack_top;

    /* Push dummy return address */
    stack--;
    *stack = 0;

    /* Push argument */
    stack--;
    *stack = arg;

    /* Push entry point (untuk return ke user) */
    stack--;
    *stack = entry;

    return (tak_bertanda32_t)stack;
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * user_mode_init
 * --------------
 * Inisialisasi subsistem user mode.
 *
 * Return: Status operasi
 */
status_t user_mode_init(void)
{
    if (user_mode_initialized) {
        return STATUS_SUDAH_ADA;
    }

    kernel_memset(&user_mode_stats, 0, sizeof(user_mode_stats));

    user_mode_initialized = BENAR;

    kernel_printf("[USER_MODE] User mode utilities initialized\n");

    return STATUS_BERHASIL;
}

/*
 * enter_user_mode
 * ---------------
 * Masuk ke user mode.
 *
 * Parameter:
 *   entry_point - Alamat entry point
 *   stack_ptr   - Pointer stack user
 *   arg         - Argumen untuk program
 */
void enter_user_mode(alamat_virtual_t entry_point, void *stack_ptr,
                     tak_bertanda32_t arg)
{
    tak_bertanda32_t stack;
    tak_bertanda32_t eflags;

    if (!user_mode_initialized) {
        kernel_panic("user_mode: tidak diinisialisasi");
    }

    /* Disable interrupts */
    cpu_disable_irq();

    /* Setup stack */
    stack = setup_user_stack((tak_bertanda32_t)(alamat_ptr_t)stack_ptr,
                             (tak_bertanda32_t)entry_point,
                             arg);

    /* Get EFLAGS */
    eflags = USER_EFLAGS_BASE | USER_EFLAGS_IOPL;

    /* Update statistik */
    user_mode_stats.transitions_to_user++;

    /* Use IRET to transition to user mode */
    __asm__ __volatile__(
        "movw %0, %%ds\n\t"     /* Set data segments */
        "movw %0, %%es\n\t"
        "movw %0, %%fs\n\t"
        "movw %0, %%gs\n\t"
        "pushl %0\n\t"          /* SS */
        "pushl %1\n\t"          /* ESP */
        "pushl %2\n\t"          /* EFLAGS */
        "pushl %3\n\t"          /* CS */
        "pushl %4\n\t"          /* EIP */
        "iret\n\t"
        :
        : "r"(USER_DATA_SELECTOR),
          "r"(stack),
          "r"(eflags),
          "r"(USER_CODE_SELECTOR),
          "r"(entry_point)
        : "memory"
    );

    /* Should not reach here */
    kernel_panic("enter_user_mode: should not return");
}

/*
 * switch_to_user
 * --------------
 * Switch ke user mode dengan context yang diberikan.
 *
 * Parameter:
 *   context - Pointer ke context
 */
void switch_to_user(void *context)
{
    cpu_context_t *ctx;
    proses_t *proses;

    if (context == NULL) {
        return;
    }

    ctx = (cpu_context_t *)context;

    /* Set segment selectors untuk user mode */
    ctx->cs = USER_CODE_SELECTOR;
    ctx->ds = USER_DATA_SELECTOR;
    ctx->es = USER_DATA_SELECTOR;
    ctx->fs = USER_DATA_SELECTOR;
    ctx->gs = USER_DATA_SELECTOR;
    ctx->ss = USER_DATA_SELECTOR;

    /* Set EFLAGS */
    ctx->eflags = USER_EFLAGS_BASE;

    /* Update TSS */
    tss = tss_get_current();
    if (tss != NULL) {
        proses = proses_get_current();
        if (proses != NULL && proses->kernel_stack != NULL) {
            tss->esp0 = (tak_bertanda32_t)(alamat_ptr_t)proses->kernel_stack;
        }
    }

    /* Switch context */
    context_switch_to(ctx);
}

/*
 * return_to_user
 * --------------
 * Return ke user mode dari system call.
 *
 * Parameter:
 *   retval - Return value
 */
void return_to_user(long retval)
{
    proses_t *proses;
    thread_t *thread;

    proses = proses_get_current();
    if (proses == NULL) {
        kernel_panic("return_to_user: tidak ada proses current");
    }

    thread = proses->main_thread;
    if (thread == NULL || thread->context == NULL) {
        kernel_panic("return_to_user: tidak ada context");
    }

    /* Set return value */
    context_set_return(thread->context, retval);

    /* Return ke user mode */
    context_return_to_user(thread->context);
}

/*
 * user_mode_is_user
 * -----------------
 * Cek apakah CPU sedang di user mode.
 *
 * Return: BENAR jika di user mode
 */
bool_t user_mode_is_user(void)
{
    tak_bertanda32_t cs;

    __asm__ __volatile__(
        "movl %%cs, %0"
        : "=r"(cs)
    );

    /* Bit 0 dan 1 menunjukkan CPL */
    return (cs & 0x03) != 0;
}

/*
 * user_mode_get_cpl
 * -----------------
 * Dapatkan Current Privilege Level.
 *
 * Return: CPL (0-3)
 */
tak_bertanda32_t user_mode_get_cpl(void)
{
    tak_bertanda32_t cs;

    __asm__ __volatile__(
        "movl %%cs, %0"
        : "=r"(cs)
    );

    return cs & 0x03;
}

/*
 * user_mode_validate_address
 * --------------------------
 * Validasi alamat user.
 *
 * Parameter:
 *   addr - Alamat yang divalidasi
 *   size - Ukuran akses
 *   write - Apakah akses tulis
 *
 * Return: BENAR jika valid
 */
bool_t user_mode_validate_address(alamat_virtual_t addr, ukuran_t size,
                                  bool_t write)
{
    proses_t *proses;
    alamat_fisik_t phys;
    tak_bertanda32_t flags;
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

    /* Harus di user space */
    if (addr < ALAMAT_USER_MULAI || end > ALAMAT_USER_AKHIR) {
        return SALAH;
    }

    proses = proses_get_current();
    if (proses == NULL || proses->vm == NULL) {
        return SALAH;
    }

    /* Cek mapping */
    addr = RATAKAN_BAWAH(addr, UKURAN_HALAMAN);

    while (addr < end) {
        if (!vm_query(proses->vm, addr, &phys, &flags, NULL)) {
            return SALAH;
        }

        if (write && !(flags & VMA_FLAG_WRITE)) {
            return SALAH;
        }

        if (!write && !(flags & VMA_FLAG_READ)) {
            return SALAH;
        }

        addr += UKURAN_HALAMAN;
    }

    return BENAR;
}

/*
 * user_mode_copy_from_user
 * ------------------------
 * Copy data dari user space dengan validasi.
 *
 * Parameter:
 *   dest - Buffer kernel
 *   src  - Buffer user
 *   size - Ukuran data
 *
 * Return: Status operasi
 */
status_t user_mode_copy_from_user(void *dest, const void *src, ukuran_t size)
{
    if (dest == NULL || src == NULL || size == 0) {
        return STATUS_PARAM_INVALID;
    }

    if (!user_mode_validate_address((alamat_virtual_t)(alamat_ptr_t)src,
                                    size, SALAH)) {
        return STATUS_AKSES_DITOLAK;
    }

    kernel_memcpy(dest, src, size);

    return STATUS_BERHASIL;
}

/*
 * user_mode_copy_to_user
 * ----------------------
 * Copy data ke user space dengan validasi.
 *
 * Parameter:
 *   dest - Buffer user
 *   src  - Buffer kernel
 *   size - Ukuran data
 *
 * Return: Status operasi
 */
status_t user_mode_copy_to_user(void *dest, const void *src, ukuran_t size)
{
    if (dest == NULL || src == NULL || size == 0) {
        return STATUS_PARAM_INVALID;
    }

    if (!user_mode_validate_address((alamat_virtual_t)(alamat_ptr_t)dest,
                                    size, BENAR)) {
        return STATUS_AKSES_DITOLAK;
    }

    kernel_memcpy(dest, src, size);

    return STATUS_BERHASIL;
}

/*
 * user_mode_get_stats
 * -------------------
 * Dapatkan statistik user mode.
 *
 * Parameter:
 *   to_user    - Transisi ke user mode
 *   to_kernel  - Transisi ke kernel mode
 *   exceptions - Jumlah exceptions
 */
void user_mode_get_stats(tak_bertanda64_t *to_user,
                         tak_bertanda64_t *to_kernel,
                         tak_bertanda64_t *exceptions)
{
    if (to_user != NULL) {
        *to_user = user_mode_stats.transitions_to_user;
    }

    if (to_kernel != NULL) {
        *to_kernel = user_mode_stats.transitions_to_kernel;
    }

    if (exceptions != NULL) {
        *exceptions = user_mode_stats.exceptions;
    }
}

/*
 * user_mode_print_stats
 * ---------------------
 * Print statistik user mode.
 */
void user_mode_print_stats(void)
{
    kernel_printf("\n[USER_MODE] Statistik User Mode:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Transitions to User:   %lu\n",
                  user_mode_stats.transitions_to_user);
    kernel_printf("  Transitions to Kernel: %lu\n",
                  user_mode_stats.transitions_to_kernel);
    kernel_printf("  Exceptions:            %lu\n",
                  user_mode_stats.exceptions);
    kernel_printf("========================================\n");
}

/*
 * user_mode_setup_process
 * -----------------------
 * Setup proses untuk masuk ke user mode.
 *
 * Parameter:
 *   proses      - Pointer ke proses
 *   entry_point - Entry point
 *   stack_top   - Alamat atas stack
 *   argc        - Jumlah argumen
 *   argv        - Array argumen
 *
 * Return: Status operasi
 */
status_t user_mode_setup_process(proses_t *proses, alamat_virtual_t entry_point,
                                 alamat_virtual_t stack_top,
                                 tak_bertanda32_t argc, tak_bertanda32_t argv)
{
    cpu_context_t *ctx;
    tak_bertanda32_t *stack;
    tak_bertanda32_t eflags;

    if (proses == NULL || proses->main_thread == NULL) {
        return STATUS_PARAM_INVALID;
    }

    ctx = proses->main_thread->context;
    if (ctx == NULL) {
        return STATUS_GAGAL;
    }

    /* Setup stack */
    stack = (tak_bertanda32_t *)stack_top;

    /* Push argument */
    stack--;
    *stack = argv;
    stack--;
    *stack = argc;

    /* Set context */
    ctx->eip = (tanda32_t)entry_point;
    ctx->esp = (tanda32_t)(alamat_ptr_t)stack;
    ctx->ebp = 0;

    /* Set segments untuk user mode */
    ctx->cs = USER_CODE_SELECTOR;
    ctx->ds = USER_DATA_SELECTOR;
    ctx->es = USER_DATA_SELECTOR;
    ctx->fs = USER_DATA_SELECTOR;
    ctx->gs = USER_DATA_SELECTOR;
    ctx->ss = USER_DATA_SELECTOR;

    /* Set EFLAGS */
    eflags = USER_EFLAGS_BASE;
    ctx->eflags = eflags;

    /* Set status */
    proses->status = PROSES_STATUS_SIAP;
    proses->main_thread->status = PROSES_STATUS_SIAP;

    return STATUS_BERHASIL;
}
