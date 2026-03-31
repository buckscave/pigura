/*
 * PIGURA OS - SYSCALL.C
 * ----------------------
 * Implementasi handler system call utama.
 *
 * Berkas ini berisi fungsi-fungsi untuk menangani system call
 * dari user space dan mengarahkannya ke handler yang sesuai.
 *
 * Versi: 1.2
 * Tanggal: 2025
 */

#include "../kernel.h"

/*
 * ============================================================================
 * DEKLARASI FORWARD
 * ============================================================================
 */

/* Forward declaration untuk syscall_register_handlers di dispatcher.c */
extern void syscall_register_handlers(void);

/* Forward declaration untuk syscall_handler */
tanda64_t syscall_handler(register_context_t *ctx);

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* Jumlah maksimum syscall */
#define SYSCALL_MAX             SYS_JUMLAH

/* Vektor interrupt untuk syscall */
#define SYSCALL_INT_VECTOR      0x80

/* Magic untuk validasi */
#define SYSCALL_MAGIC           0x53595343  /* "SYSC" */

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Tabel syscall */
static syscall_fn_t syscall_table[SYSCALL_MAX];

/* Statistik syscall */
static struct {
    tak_bertanda64_t total_calls;
    tak_bertanda64_t calls_by_num[SYSCALL_MAX];
    tak_bertanda64_t errors;
    tak_bertanda64_t permission_denied;
} syscall_stats = {0};

/* Status inisialisasi */
static bool_t syscall_initialized = SALAH;

/* Lock untuk tabel syscall */
static spinlock_t syscall_lock = {0};

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * syscall_default_handler
 * -----------------------
 * Handler default untuk syscall yang tidak terdefinisi.
 *
 * Parameter:
 *   arg1-arg6 - Argumen syscall
 *
 * Return: -ERROR_NOSYS
 */
static tanda64_t syscall_default_handler(tanda64_t arg1, tanda64_t arg2, tanda64_t arg3,
                                          tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    /* Suppress unused warnings */
    (void)arg1; (void)arg2; (void)arg3;
    (void)arg4; (void)arg5; (void)arg6;

    syscall_stats.errors++;
    return -ERROR_NOSYS;
}

/*
 * syscall_check_permission
 * ------------------------
 * Cek permission untuk syscall.
 *
 * Parameter:
 *   syscall_num - Nomor syscall
 *
 * Return: BENAR jika diizinkan
 */
static bool_t syscall_check_permission(tak_bertanda32_t syscall_num)
{
    proses_t *proses;

    proses = proses_get_current();
    if (proses == NULL) {
        return SALAH;
    }

    /* Syscall yang hanya untuk kernel */
    switch (syscall_num) {
        case SYS_REBOOT:
        case SYS_MOUNT:
        case SYS_UMOUNT:
        case SYS_SETUID:
        case SYS_SETGID:
            if (proses->uid != 0) {
                syscall_stats.permission_denied++;
                return SALAH;
            }
            break;
    }

    return BENAR;
}

/*
 * syscall_log_call
 * ----------------
 * Log syscall call untuk debugging.
 *
 * Parameter:
 *   syscall_num - Nomor syscall
 *   args        - Array argumen
 */
static void syscall_log_call(tak_bertanda32_t syscall_num, tanda64_t *args)
{
    /* Hanya log jika debug mode */
#ifdef DEBUG_SYSCALL
    kernel_printf("[SYSCALL] num=%u args=[%llx,%llx,%llx,%llx,%llx,%llx]\n",
                  syscall_num, args[0], args[1], args[2],
                  args[3], args[4], args[5]);
#else
    (void)syscall_num;
    (void)args;
#endif
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * syscall_init
 * ------------
 * Inisialisasi subsistem syscall.
 *
 * Return: Status operasi
 */
status_t syscall_init(void)
{
    tak_bertanda32_t i;

    if (syscall_initialized) {
        return STATUS_SUDAH_ADA;
    }

    /* Reset tabel */
    for (i = 0; i < SYSCALL_MAX; i++) {
        syscall_table[i] = syscall_default_handler;
    }

    /* Reset statistik */
    kernel_memset(&syscall_stats, 0, sizeof(syscall_stats));

    /* Init lock */
    spinlock_init(&syscall_lock);

    /* Register default syscall handlers */
    syscall_register_handlers();

    /* Setup interrupt handler */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
    isr_set_handler(SYSCALL_INT_VECTOR, (isr_handler_t)syscall_handler);
#pragma GCC diagnostic pop

    syscall_initialized = BENAR;

    kernel_printf("[SYSCALL] System call handler initialized\n");
    kernel_printf("          Vector: 0x%02X, Max: %u\n",
                  SYSCALL_INT_VECTOR, SYSCALL_MAX);

    return STATUS_BERHASIL;
}

/*
 * syscall_register
 * ----------------
 * Register handler syscall.
 *
 * Parameter:
 *   syscall_num - Nomor syscall
 *   handler     - Fungsi handler
 *
 * Return: Status operasi
 */
status_t syscall_register(tak_bertanda32_t syscall_num, syscall_fn_t handler)
{
    if (!syscall_initialized) {
        return STATUS_GAGAL;
    }

    if (syscall_num >= SYSCALL_MAX || handler == NULL) {
        return STATUS_PARAM_INVALID;
    }

    spinlock_kunci(&syscall_lock);
    syscall_table[syscall_num] = handler;
    spinlock_buka(&syscall_lock);

    return STATUS_BERHASIL;
}

/*
 * syscall_unregister
 * ------------------
 * Unregister handler syscall.
 *
 * Parameter:
 *   syscall_num - Nomor syscall
 *
 * Return: Status operasi
 */
status_t syscall_unregister(tak_bertanda32_t syscall_num)
{
    if (!syscall_initialized) {
        return STATUS_GAGAL;
    }

    if (syscall_num >= SYSCALL_MAX) {
        return STATUS_PARAM_INVALID;
    }

    spinlock_kunci(&syscall_lock);
    syscall_table[syscall_num] = syscall_default_handler;
    spinlock_buka(&syscall_lock);

    return STATUS_BERHASIL;
}

/*
 * syscall_handler
 * ---------------
 * Handler interrupt syscall utama.
 * Dipanggil dari assembly (syscall_entry.S).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 *
 * Return: Nilai return syscall
 */
tanda64_t syscall_handler(register_context_t *ctx)
{
    tak_bertanda32_t syscall_num;
    tanda64_t args[SYSCALL_ARG_MAKS];
    tanda64_t result;
    syscall_fn_t handler;

    if (!syscall_initialized || ctx == NULL) {
        return -ERROR_NOSYS;
    }

    /*
     * Ambil nomor syscall dan argumen dari register.
     * Menggunakan conditional compilation untuk mendukung berbagai arsitektur.
     */
#if defined(ARSITEKTUR_X86_64)
    /* x86_64: RAX = syscall number, argumen di RDI, RSI, RDX, R10, R8, R9 */
    syscall_num = (tak_bertanda32_t)ctx->rax;
    args[0] = (tanda64_t)ctx->rdi;
    args[1] = (tanda64_t)ctx->rsi;
    args[2] = (tanda64_t)ctx->rdx;
    args[3] = (tanda64_t)ctx->r10;
    args[4] = (tanda64_t)ctx->r8;
    args[5] = (tanda64_t)ctx->r9;

#elif defined(ARSITEKTUR_X86)
    /* x86 (32-bit): EAX = syscall number, argumen di EBX, ECX, EDX, ESI, EDI, EBP */
    syscall_num = (tak_bertanda32_t)ctx->eax;
    args[0] = (tanda64_t)ctx->ebx;
    args[1] = (tanda64_t)ctx->ecx;
    args[2] = (tanda64_t)ctx->edx;
    args[3] = (tanda64_t)ctx->esi;
    args[4] = (tanda64_t)ctx->edi;
    args[5] = (tanda64_t)ctx->ebp;

#elif defined(ARSITEKTUR_ARM) || defined(ARSITEKTUR_ARMV7)
    /* ARM 32-bit: r7 = syscall number, argumen di r0-r5 */
    syscall_num = (tak_bertanda32_t)ctx->r[7];
    args[0] = (tanda64_t)ctx->r[0];
    args[1] = (tanda64_t)ctx->r[1];
    args[2] = (tanda64_t)ctx->r[2];
    args[3] = (tanda64_t)ctx->r[3];
    args[4] = (tanda64_t)ctx->r[4];
    args[5] = (tanda64_t)ctx->r[5];

#elif defined(ARSITEKTUR_ARM64)
    /* ARM64: x8 = syscall number, argumen di x0-x5 */
    syscall_num = (tak_bertanda32_t)ctx->x[8];
    args[0] = (tanda64_t)ctx->x[0];
    args[1] = (tanda64_t)ctx->x[1];
    args[2] = (tanda64_t)ctx->x[2];
    args[3] = (tanda64_t)ctx->x[3];
    args[4] = (tanda64_t)ctx->x[4];
    args[5] = (tanda64_t)ctx->x[5];

#else
    /* Fallback: gunakan struktur generik */
    syscall_num = (tak_bertanda32_t)ctx->exception_no;
    args[0] = 0;
    args[1] = 0;
    args[2] = 0;
    args[3] = 0;
    args[4] = 0;
    args[5] = 0;
#endif

    /* Validasi nomor syscall */
    if (syscall_num >= SYSCALL_MAX) {
        syscall_stats.errors++;
        return -ERROR_NOSYS;
    }

    /* Log untuk debug */
    syscall_log_call(syscall_num, args);

    /* Cek permission */
    if (!syscall_check_permission(syscall_num)) {
        return -ERROR_PERMISI;
    }

    /* Update statistik */
    syscall_stats.total_calls++;
    syscall_stats.calls_by_num[syscall_num]++;

    /* Dapatkan handler */
    spinlock_kunci(&syscall_lock);
    handler = syscall_table[syscall_num];
    spinlock_buka(&syscall_lock);

    /* Panggil handler */
    result = handler(args[0], args[1], args[2], args[3], args[4], args[5]);

    /* Validasi return value */
    if (result < 0 && result > -100) {
        /* Error code valid */
    }

    return result;
}

/*
 * syscall_get_stats
 * -----------------
 * Dapatkan statistik syscall.
 *
 * Parameter:
 *   total   - Pointer untuk total calls
 *   errors  - Pointer untuk error count
 *   denied  - Pointer untuk permission denied
 */
void syscall_get_stats(tak_bertanda64_t *total, tak_bertanda64_t *errors,
                       tak_bertanda64_t *denied)
{
    if (total != NULL) {
        *total = syscall_stats.total_calls;
    }

    if (errors != NULL) {
        *errors = syscall_stats.errors;
    }

    if (denied != NULL) {
        *denied = syscall_stats.permission_denied;
    }
}

/*
 * syscall_print_stats
 * -------------------
 * Print statistik syscall.
 */
void syscall_print_stats(void)
{
    tak_bertanda32_t i;

    kernel_printf("\n[SYSCALL] Statistik System Call:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Total Calls       : %lu\n", syscall_stats.total_calls);
    kernel_printf("  Errors            : %lu\n", syscall_stats.errors);
    kernel_printf("  Permission Denied : %lu\n",
                  syscall_stats.permission_denied);
    kernel_printf("========================================\n");

    /* Print top 10 syscalls */
    kernel_printf("\n  Top 10 Syscalls:\n");

    for (i = 0; i < 10 && i < SYSCALL_MAX; i++) {
        if (syscall_stats.calls_by_num[i] > 0) {
            kernel_printf("    [%2u] %lu calls\n", i,
                          syscall_stats.calls_by_num[i]);
        }
    }
}

/*
 * syscall_print_table
 * -------------------
 * Print tabel syscall.
 */
void syscall_print_table(void)
{
    tak_bertanda32_t i;
    tak_bertanda32_t count = 0;

    kernel_printf("\n[SYSCALL] Tabel System Call:\n");
    kernel_printf("========================================\n");

    for (i = 0; i < SYSCALL_MAX; i++) {
        if (syscall_table[i] != syscall_default_handler) {
            kernel_printf("  [%3u] Registered\n", i);
            count++;
        }
    }

    kernel_printf("========================================\n");
    kernel_printf("  Total: %u/%u registered\n", count, SYSCALL_MAX);
}

/*
 * syscall_execute
 * ---------------
 * Eksekusi syscall dari kernel.
 *
 * Parameter:
 *   syscall_num - Nomor syscall
 *   arg1-arg6   - Argumen syscall
 *
 * Return: Hasil syscall
 */
tanda64_t syscall_execute(tak_bertanda32_t syscall_num,
                           tanda64_t arg1, tanda64_t arg2, tanda64_t arg3,
                           tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    syscall_fn_t handler;

    if (!syscall_initialized) {
        return -ERROR_NOSYS;
    }

    if (syscall_num >= SYSCALL_MAX) {
        return -ERROR_NOSYS;
    }

    /* Dapatkan handler */
    spinlock_kunci(&syscall_lock);
    handler = syscall_table[syscall_num];
    spinlock_buka(&syscall_lock);

    /* Update statistik */
    syscall_stats.total_calls++;
    syscall_stats.calls_by_num[syscall_num]++;

    /* Panggil handler */
    return handler(arg1, arg2, arg3, arg4, arg5, arg6);
}

/*
 * syscall_from_user
 * -----------------
 * Validasi dan eksekusi syscall dari user space.
 *
 * Parameter:
 *   syscall_num - Nomor syscall
 *   args        - Array argumen
 *   arg_count   - Jumlah argumen
 *
 * Return: Hasil syscall
 */
tanda64_t syscall_from_user(tak_bertanda32_t syscall_num, tanda64_t *args,
                             tak_bertanda32_t arg_count)
{
    tak_bertanda32_t i;
    tanda64_t result;

    /* Validasi syscall number */
    if (syscall_num >= SYSCALL_MAX) {
        return -ERROR_NOSYS;
    }

    /* Validasi argumen */
    if (arg_count > SYSCALL_ARG_MAKS) {
        arg_count = SYSCALL_ARG_MAKS;
    }

    /* Validasi pointer user */
    for (i = 0; i < arg_count; i++) {
        if (args[i] != 0 && !validasi_pointer_user((void *)(alamat_ptr_t)args[i])) {
            return -ERROR_FAULT;
        }
    }

    /* Execute syscall */
    result = syscall_execute(syscall_num,
                             args[0], args[1], args[2],
                             args[3], args[4], args[5]);

    return result;
}
