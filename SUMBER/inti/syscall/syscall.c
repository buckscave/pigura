/*
 * PIGURA OS - SYSCALL.C
 * ----------------------
 * Implementasi handler system call utama.
 *
 * Berkas ini berisi fungsi-fungsi untuk menangani system call
 * dari user space dan mengarahkannya ke handler yang sesuai.
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
static long syscall_default_handler(long arg1, long arg2, long arg3,
                                    long arg4, long arg5, long arg6)
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
static void syscall_log_call(tak_bertanda32_t syscall_num, long *args)
{
    /* Hanya log jika debug mode */
#ifdef DEBUG_SYSCALL
    kernel_printf("[SYSCALL] num=%u args=[%lx,%lx,%lx,%lx,%lx,%lx]\n",
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
    interupsi_set_handler(SYSCALL_INT_VECTOR, syscall_handler);

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
 *   frame - Pointer ke stack frame
 *
 * Return: Nilai return syscall
 */
long syscall_handler(interrupt_frame_t *frame)
{
    tak_bertanda32_t syscall_num;
    long args[SYSCALL_ARG_MAKS];
    long result;
    syscall_fn_t handler;

    if (!syscall_initialized || frame == NULL) {
        return -ERROR_NOSYS;
    }

    /* Ambil nomor syscall dari EAX */
    syscall_num = (tak_bertanda32_t)frame->eax;

    /* Validasi nomor syscall */
    if (syscall_num >= SYSCALL_MAX) {
        syscall_stats.errors++;
        return -ERROR_NOSYS;
    }

    /* Ambil argumen dari stack/registers */
    args[0] = (long)frame->ebx;
    args[1] = (long)frame->ecx;
    args[2] = (long)frame->edx;
    args[3] = (long)frame->esi;
    args[4] = (long)frame->edi;
    args[5] = (long)frame->ebp;

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
long syscall_execute(tak_bertanda32_t syscall_num,
                     long arg1, long arg2, long arg3,
                     long arg4, long arg5, long arg6)
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
long syscall_from_user(tak_bertanda32_t syscall_num, long *args,
                       tak_bertanda32_t arg_count)
{
    tak_bertanda32_t i;
    long result;

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
        if (args[i] != 0 && !validasi_pointer_user((void *)args[i])) {
            return -ERROR_FAULT;
        }
    }

    /* Execute syscall */
    result = syscall_execute(syscall_num,
                             args[0], args[1], args[2],
                             args[3], args[4], args[5]);

    return result;
}
