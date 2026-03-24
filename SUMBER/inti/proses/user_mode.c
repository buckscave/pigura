/*
 * PIGURA OS - USER_MODE.C
 * ------------------------
 * Implementasi utilitas mode user.
 *
 * Berkas ini berisi fungsi-fungsi untuk menangani transisi
 * antara kernel mode dan user mode dengan dukungan multi-arsitektur.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua fungsi diimplementasikan lengkap
 * - Batas 80 karakter per baris
 *
 * Versi: 3.0
 * Tanggal: 2025
 */

#include "../kernel.h"
#include "proses.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* Selector segment untuk user mode */
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
#define USER_CODE_SELECTOR      (SELECTOR_USER_CODE | 0x03)
#define USER_DATA_SELECTOR      (SELECTOR_USER_DATA | 0x03)
#endif

/* EFLAGS untuk user mode */
#define USER_EFLAGS_BASE        0x202  /* IF = 1, bit 1 = 1 */
#define USER_EFLAGS_IOPL        0x000  /* IOPL = 0 */

/* Stack alignment */
#define STACK_ALIGN             16

/* Maximum retries for validation */
#define MAX_VALIDATION_RETRIES  3

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
    tak_bertanda64_t validation_failures;
    tak_bertanda64_t copy_operations;
    tak_bertanda64_t bytes_transferred;
} user_mode_stats = {0, 0, 0, 0, 0, 0};

/* Status inisialisasi */
static bool_t user_mode_initialized = SALAH;

/* Lock */
static spinlock_t user_mode_lock;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * setup_user_stack_internal
 * -------------------------
 * Setup stack untuk masuk ke user mode.
 *
 * Parameter:
 *   stack_top - Alamat atas stack
 *   entry     - Entry point
 *   arg       - Argumen program
 *
 * Return: Stack pointer baru
 */
static alamat_virtual_t setup_user_stack_internal(alamat_virtual_t stack_top,
                                                  alamat_virtual_t entry,
                                                  tak_bertanda32_t arg)
{
    tak_bertanda32_t *stack;
    
    /* Align stack */
    stack_top &= ~(STACK_ALIGN - 1);
    
    stack = (tak_bertanda32_t *)(alamat_ptr_t)stack_top;
    
    /* Push dummy return address */
    stack--;
    *stack = 0;
    
    /* Push argument */
    stack--;
    *stack = arg;
    
    /* Push entry point (untuk return ke user) */
    stack--;
    *stack = (tak_bertanda32_t)entry;
    
    return (alamat_virtual_t)(alamat_ptr_t)stack;
}

/*
 * validate_user_region
 * --------------------
 * Validasi region memory user.
 *
 * Parameter:
 *   addr  - Alamat awal
 *   size  - Ukuran region
 *   write - Apakah akses tulis
 *
 * Return: BENAR jika valid
 */
static bool_t validate_user_region(alamat_virtual_t addr, ukuran_t size,
                                   bool_t write)
{
    proses_t *proses;
    alamat_virtual_t end;
    alamat_virtual_t page;
    
    if (addr == 0 || size == 0) {
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
    
    /* Dapatkan proses saat ini */
    proses = proses_dapat_saat_ini();
    if (proses == NULL || proses->vm == NULL) {
        return SALAH;
    }
    
    /* Validasi setiap halaman */
    for (page = RATAKAN_BAWAH(addr, UKURAN_HALAMAN);
         page < end;
         page += UKURAN_HALAMAN) {
        
        alamat_fisik_t phys;
        tak_bertanda32_t flags;
        
        if (!vm_query(proses->vm, page, &phys, &flags, NULL)) {
            return SALAH;
        }
        
        /* Cek permission */
        if (write && !(flags & VMA_FLAG_WRITE)) {
            return SALAH;
        }
        
        if (!write && !(flags & VMA_FLAG_READ)) {
            return SALAH;
        }
    }
    
    return BENAR;
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
    spinlock_init(&user_mode_lock);
    
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
    alamat_virtual_t stack;
    tak_bertanda32_t eflags;
    
    if (!user_mode_initialized) {
        PANIC("enter_user_mode: tidak diinisialisasi");
    }
    
    /* Disable interrupts */
    cpu_disable_irq();
    
    /* Setup stack */
    stack = setup_user_stack_internal(
                (alamat_virtual_t)(alamat_ptr_t)stack_ptr,
                entry_point,
                arg);
    
    /* Get EFLAGS */
    eflags = USER_EFLAGS_BASE | USER_EFLAGS_IOPL;
    
    /* Update statistik */
    user_mode_stats.transitions_to_user++;
    
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    
    /* Use IRET to transition to user mode */
    __asm__ __volatile__(
        "mov %0, %%ds\n\t"     /* Set data segments */
        "mov %0, %%es\n\t"
        "mov %0, %%fs\n\t"
        "mov %0, %%gs\n\t"
        "pushl %0\n\t"         /* SS */
        "pushl %1\n\t"         /* ESP */
        "pushl %2\n\t"         /* EFLAGS */
        "pushl %3\n\t"         /* CS */
        "pushl %4\n\t"         /* EIP */
        "iret\n\t"
        :
        : "r"((tak_bertanda32_t)USER_DATA_SELECTOR),
          "r"(stack),
          "r"(eflags),
          "r"((tak_bertanda32_t)USER_CODE_SELECTOR),
          "r"((tak_bertanda32_t)entry_point)
        : "memory"
    );
    
#elif defined(ARSITEKTUR_ARM) || defined(ARSITEKTUR_ARMV7)
    
    /* ARM mode switch */
    __asm__ __volatile__(
        "msr cpsr_c, %0\n\t"   /* Switch to user mode */
        "mov sp, %1\n\t"       /* Set stack pointer */
        "mov pc, %2\n\t"       /* Jump to entry */
        :
        : "r"(CPSR_MODE_USR),
          "r"(stack),
          "r"(entry_point)
        : "memory", "sp", "pc"
    );
    
#elif defined(ARSITEKTUR_ARM64)
    
    /* ARM64 mode switch */
    __asm__ __volatile__(
        "msr sp_el0, %0\n\t"   /* Set user stack */
        "msr elr_el1, %1\n\t"  /* Set return address */
        "msr spsr_el1, %2\n\t" /* Set processor state */
        "eret\n\t"
        :
        : "r"(stack),
          "r"(entry_point),
          "r"(0x00000000)  /* EL0t */
        : "memory"
    );
    
#endif
    
    /* Should not reach here */
    PANIC("enter_user_mode: should not return");
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
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    tak_bertanda32_t cs;
    
    __asm__ __volatile__(
        "mov %%cs, %0"
        : "=r"(cs)
    );
    
    /* Bit 0 dan 1 menunjukkan CPL */
    return (cs & 0x03) != 0;
    
#elif defined(ARSITEKTUR_ARM) || defined(ARSITEKTUR_ARMV7)
    tak_bertanda32_t cpsr;
    
    __asm__ __volatile__(
        "mrs %0, cpsr"
        : "=r"(cpsr)
    );
    
    /* Mode bits menunjukkan user mode (0x10) */
    return (cpsr & 0x1F) == 0x10;
    
#elif defined(ARSITEKTUR_ARM64)
    /* PSTATE bit menunjukkan EL0 */
    tak_bertanda64_t currentel;
    
    __asm__ __volatile__(
        "mrs %0, CurrentEL"
        : "=r"(currentel)
    );
    
    return (currentel & 0x0C) == 0x00;  /* EL0 */
    
#else
    return SALAH;
#endif
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
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    tak_bertanda32_t cs;
    
    __asm__ __volatile__(
        "mov %%cs, %0"
        : "=r"(cs)
    );
    
    return cs & 0x03;
    
#elif defined(ARSITEKTUR_ARM) || defined(ARSITEKTUR_ARMV7)
    tak_bertanda32_t cpsr;
    
    __asm__ __volatile__(
        "mrs %0, cpsr"
        : "=r"(cpsr)
    );
    
    /* Return 0 for kernel, 3 for user */
    return ((cpsr & 0x1F) == 0x10) ? 3 : 0;
    
#elif defined(ARSITEKTUR_ARM64)
    tak_bertanda64_t currentel;
    
    __asm__ __volatile__(
        "mrs %0, CurrentEL"
        : "=r"(currentel)
    );
    
    return (currentel >> 2) & 0x03;
    
#else
    return 0;
#endif
}

/*
 * user_mode_validate_address
 * --------------------------
 * Validasi alamat user.
 *
 * Parameter:
 *   addr  - Alamat yang divalidasi
 *   size  - Ukuran akses
 *   write - Apakah akses tulis
 *
 * Return: BENAR jika valid
 */
bool_t user_mode_validate_address(alamat_virtual_t addr, ukuran_t size,
                                  bool_t write)
{
    bool_t result;
    
    result = validate_user_region(addr, size, write);
    
    if (!result) {
        user_mode_stats.validation_failures++;
    }
    
    return result;
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
status_t user_mode_copy_from_user(void *dest, const void *src,
                                   ukuran_t size)
{
    if (dest == NULL || src == NULL || size == 0) {
        return STATUS_PARAM_INVALID;
    }
    
    if (!user_mode_validate_address((alamat_virtual_t)(alamat_ptr_t)src,
                                    size, SALAH)) {
        return STATUS_AKSES_DITOLAK;
    }
    
    kernel_memcpy(dest, src, size);
    
    user_mode_stats.copy_operations++;
    user_mode_stats.bytes_transferred += size;
    
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
status_t user_mode_copy_to_user(void *dest, const void *src,
                                 ukuran_t size)
{
    if (dest == NULL || src == NULL || size == 0) {
        return STATUS_PARAM_INVALID;
    }
    
    if (!user_mode_validate_address((alamat_virtual_t)(alamat_ptr_t)dest,
                                    size, BENAR)) {
        return STATUS_AKSES_DITOLAK;
    }
    
    kernel_memcpy(dest, src, size);
    
    user_mode_stats.copy_operations++;
    user_mode_stats.bytes_transferred += size;
    
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
    kernel_printf("  Validation Failures:   %lu\n",
                  user_mode_stats.validation_failures);
    kernel_printf("  Copy Operations:       %lu\n",
                  user_mode_stats.copy_operations);
    kernel_printf("  Bytes Transferred:     %lu\n",
                  user_mode_stats.bytes_transferred);
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
status_t user_mode_setup_process(proses_t *proses,
                                  alamat_virtual_t entry_point,
                                  alamat_virtual_t stack_top,
                                  tak_bertanda32_t argc,
                                  tak_bertanda32_t argv)
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
    context_set_entry(ctx, entry_point);
    context_set_stack(ctx, stack);
    
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    /* Set segment selectors untuk user mode */
    ctx->cs = USER_CODE_SELECTOR;
    ctx->ds = USER_DATA_SELECTOR;
    ctx->es = USER_DATA_SELECTOR;
    ctx->fs = USER_DATA_SELECTOR;
    ctx->gs = USER_DATA_SELECTOR;
    ctx->ss = USER_DATA_SELECTOR;
    
    /* Set EFLAGS/RFLAGS */
    eflags = USER_EFLAGS_BASE;
#if defined(PIGURA_ARSITEKTUR_64BIT)
    ctx->rflags = eflags;
#else
    ctx->eflags = eflags;
#endif
    
#elif defined(ARSITEKTUR_ARM) || defined(ARSITEKTUR_ARMV7)
    /* Set CPSR untuk user mode */
    ctx->cpsr = CPSR_MODE_USR;
    ctx->spsr = CPSR_MODE_USR;
    
#elif defined(ARSITEKTUR_ARM64)
    /* Set SPSR untuk EL0 */
    ctx->spsr_el1 = 0x00000000;  /* EL0t */
    
#endif
    
    /* Set status */
    proses->status = PROSES_STATUS_SIAP;
    proses->main_thread->status = THREAD_STATUS_SIAP;
    
    return STATUS_BERHASIL;
}
