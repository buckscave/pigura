/*
 * PIGURA OS - SIGNAL.C
 * ---------------------
 * Implementasi penanganan sinyal.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengirim dan menangani
 * sinyal antar proses dalam sistem operasi Pigura.
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

/* Jumlah sinyal maksimum */
#define SIGNAL_MAX              32

/* Nomor sinyal standar */
#define SIGHUP                  1    /* Hangup */
#define SIGINT                  2    /* Interrupt */
#define SIGQUIT                 3    /* Quit */
#define SIGILL                  4    /* Illegal instruction */
#define SIGTRAP                 5    /* Trace trap */
#define SIGABRT                 6    /* Abort */
#define SIGBUS                  7    /* Bus error */
#define SIGFPE                  8    /* Floating point exception */
#define SIGKILL                 9    /* Kill (cannot be caught) */
#define SIGUSR1                 10   /* User signal 1 */
#define SIGSEGV                 11   /* Segmentation fault */
#define SIGUSR2                 12   /* User signal 2 */
#define SIGPIPE                 13   /* Broken pipe */
#define SIGALRM                 14   /* Alarm clock */
#define SIGTERM                 15   /* Termination */
#define SIGSTKFLT               16   /* Stack fault */
#define SIGCHLD                 17   /* Child status changed */
#define SIGCONT                 18   /* Continue */
#define SIGSTOP                 19   /* Stop (cannot be caught) */
#define SIGTSTP                 20   /* Stop from keyboard */
#define SIGTTIN                 21   /* Background read */
#define SIGTTOU                 22   /* Background write */

/* Aksi default sinyal */
#define SIG_ACT_TERM            0    /* Terminate */
#define SIG_ACT_CORE            1    /* Terminate with core */
#define SIG_ACT_IGNORE          2    /* Ignore */
#define SIG_ACT_STOP            3    /* Stop process */
#define SIG_ACT_CONT            4    /* Continue if stopped */

/* Flag handler */
#define SIG_HANDLER_DEFAULT     ((void (*)(int))0)
#define SIG_HANDLER_IGNORE      ((void (*)(int))1)
#define SIG_HANDLER_ERROR       ((void (*)(int))-1)

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* Struktur sinyal tertunda */
typedef struct signal_pending {
    tak_bertanda32_t set;           /* Bit set sinyal tertunda */
    struct signal_pending *next;    /* Untuk queue sinyal */
} signal_pending_t;

/* Struktur handler sinyal */
typedef struct {
    void (*handler)(int);           /* Handler function */
    tak_bertanda32_t flags;         /* SA_* flags */
    void (*restorer)(void);         /* Signal restorer */
    tak_bertanda32_t mask;          /* Signal mask during handler */
} signal_handler_t;

/* Tabel aksi default */
static const tak_bertanda8_t default_action[SIGNAL_MAX + 1] = {
    [0] = SIG_ACT_IGNORE,
    [SIGHUP]    = SIG_ACT_TERM,
    [SIGINT]    = SIG_ACT_TERM,
    [SIGQUIT]   = SIG_ACT_CORE,
    [SIGILL]    = SIG_ACT_CORE,
    [SIGTRAP]   = SIG_ACT_CORE,
    [SIGABRT]   = SIG_ACT_CORE,
    [SIGBUS]    = SIG_ACT_CORE,
    [SIGFPE]    = SIG_ACT_CORE,
    [SIGKILL]   = SIG_ACT_TERM,
    [SIGUSR1]   = SIG_ACT_TERM,
    [SIGSEGV]   = SIG_ACT_CORE,
    [SIGUSR2]   = SIG_ACT_TERM,
    [SIGPIPE]   = SIG_ACT_TERM,
    [SIGALRM]   = SIG_ACT_TERM,
    [SIGTERM]   = SIG_ACT_TERM,
    [SIGSTKFLT] = SIG_ACT_TERM,
    [SIGCHLD]   = SIG_ACT_IGNORE,
    [SIGCONT]   = SIG_ACT_CONT,
    [SIGSTOP]   = SIG_ACT_STOP,
    [SIGTSTP]   = SIG_ACT_STOP,
    [SIGTTIN]   = SIG_ACT_STOP,
    [SIGTTOU]   = SIG_ACT_STOP
};

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Statistik sinyal */
static struct {
    tak_bertanda64_t sent;
    tak_bertanda64_t delivered;
    tak_bertanda64_t ignored;
    tak_bertanda64_t dropped;
} signal_stats = {0};

/* Status inisialisasi */
static bool_t signal_initialized = SALAH;

/* Lock untuk operasi sinyal */
static spinlock_t signal_lock = {0};

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * signal_is_masked
 * ----------------
 * Cek apakah sinyal di-mask.
 *
 * Parameter:
 *   proses - Pointer ke proses
 *   sig    - Nomor sinyal
 *
 * Return: BENAR jika di-mask
 */
static bool_t signal_is_masked(proses_t *proses, tak_bertanda32_t sig)
{
    if (proses == NULL || sig == 0 || sig > SIGNAL_MAX) {
        return SALAH;
    }

    return (proses->signal_mask & (1UL << sig)) ? BENAR : SALAH;
}

/*
 * signal_is_pending
 * -----------------
 * Cek apakah sinyal tertunda.
 *
 * Parameter:
 *   proses - Pointer ke proses
 *   sig    - Nomor sinyal
 *
 * Return: BENAR jika tertunda
 */
static bool_t signal_is_pending(proses_t *proses, tak_bertanda32_t sig)
{
    if (proses == NULL || sig == 0 || sig > SIGNAL_MAX) {
        return SALAH;
    }

    return (proses->signal_pending & (1UL << sig)) ? BENAR : SALAH;
}

/*
 * signal_set_pending
 * ------------------
 * Set sinyal sebagai tertunda.
 *
 * Parameter:
 *   proses - Pointer ke proses
 *   sig    - Nomor sinyal
 */
static void signal_set_pending(proses_t *proses, tak_bertanda32_t sig)
{
    if (proses == NULL || sig == 0 || sig > SIGNAL_MAX) {
        return;
    }

    proses->signal_pending |= (1UL << sig);
}

/*
 * signal_clear_pending
 * --------------------
 * Clear sinyal tertunda.
 *
 * Parameter:
 *   proses - Pointer ke proses
 *   sig    - Nomor sinyal
 */
static void signal_clear_pending(proses_t *proses, tak_bertanda32_t sig)
{
    if (proses == NULL || sig == 0 || sig > SIGNAL_MAX) {
        return;
    }

    proses->signal_pending &= ~(1UL << sig);
}

/*
 * signal_get_handler
 * ------------------
 * Dapatkan handler sinyal.
 *
 * Parameter:
 *   proses - Pointer ke proses
 *   sig    - Nomor sinyal
 *
 * Return: Pointer ke handler
 */
static void (*signal_get_handler(proses_t *proses,
                                 tak_bertanda32_t sig))(int)
{
    if (proses == NULL || sig == 0 || sig > SIGNAL_MAX) {
        return SIG_HANDLER_DEFAULT;
    }

    return proses->signal_handlers[sig].handler;
}

/*
 * signal_execute_default
 * ----------------------
 * Eksekusi aksi default sinyal.
 *
 * Parameter:
 *   proses - Pointer ke proses
 *   sig    - Nomor sinyal
 */
static void signal_execute_default(proses_t *proses, tak_bertanda32_t sig)
{
    tak_bertanda32_t action;

    if (proses == NULL || sig == 0 || sig > SIGNAL_MAX) {
        return;
    }

    action = default_action[sig];

    switch (action) {
        case SIG_ACT_TERM:
            /* Terminate proses */
            exit_by_signal(proses, sig, SALAH);
            break;

        case SIG_ACT_CORE:
            /* Terminate dengan core dump */
            exit_by_signal(proses, sig, BENAR);
            break;

        case SIG_ACT_IGNORE:
            /* Abaikan */
            signal_stats.ignored++;
            break;

        case SIG_ACT_STOP:
            /* Stop proses */
            proses->status = PROSES_STATUS_STOP;
            proses->stop_signal = sig;
            scheduler_hapus_proses(proses);
            break;

        case SIG_ACT_CONT:
            /* Continue jika stopped */
            if (proses->status == PROSES_STATUS_STOP) {
                proses->status = PROSES_STATUS_SIAP;
                proses->flags |= PROSES_FLAG_CONTINUED;
                scheduler_tambah_proses(proses);
            }
            break;
    }
}

/*
 * signal_deliver
 * --------------
 * Deliver sinyal ke proses.
 *
 * Parameter:
 *   proses - Pointer ke proses
 *   sig    - Nomor sinyal
 */
static void signal_deliver(proses_t *proses, tak_bertanda32_t sig)
{
    void (*handler)(int);

    if (proses == NULL || sig == 0 || sig > SIGNAL_MAX) {
        return;
    }

    signal_stats.delivered++;

    /* Dapatkan handler */
    handler = signal_get_handler(proses, sig);

    /* Cek tipe handler */
    if (handler == SIG_HANDLER_DEFAULT) {
        signal_execute_default(proses, sig);
    } else if (handler == SIG_HANDLER_IGNORE) {
        signal_stats.ignored++;
    } else {
        /* Setup frame untuk user handler */
        signal_setup_frame(proses, sig, handler);
    }
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * signal_init
 * -----------
 * Inisialisasi subsistem sinyal.
 *
 * Return: Status operasi
 */
status_t signal_init(void)
{
    if (signal_initialized) {
        return STATUS_SUDAH_ADA;
    }

    kernel_memset(&signal_stats, 0, sizeof(signal_stats));
    spinlock_init(&signal_lock);

    signal_initialized = BENAR;

    kernel_printf("[SIGNAL] Signal subsystem initialized\n");

    return STATUS_BERHASIL;
}

/*
 * signal_send
 * -----------
 * Kirim sinyal ke proses.
 *
 * Parameter:
 *   pid - Process ID target
 *   sig - Nomor sinyal
 *
 * Return: Status operasi
 */
status_t signal_send(pid_t pid, tak_bertanda32_t sig)
{
    proses_t *target;

    if (!signal_initialized) {
        return STATUS_GAGAL;
    }

    /* Validasi sinyal */
    if (sig > SIGNAL_MAX) {
        return STATUS_PARAM_INVALID;
    }

    /* Cari proses target */
    target = proses_cari(pid);
    if (target == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    return signal_send_to_proses(target, sig);
}

/*
 * signal_send_to_proses
 * ---------------------
 * Kirim sinyal ke proses (dengan pointer proses).
 *
 * Parameter:
 *   target - Pointer ke proses target
 *   sig    - Nomor sinyal
 *
 * Return: Status operasi
 */
status_t signal_send_to_proses(proses_t *target, tak_bertanda32_t sig)
{
    spinlock_kunci(&signal_lock);

    signal_stats.sent++;

    /* Cek apakah sinyal di-ignore */
    void (*handler)(int) = signal_get_handler(target, sig);
    if (handler == SIG_HANDLER_IGNORE) {
        signal_stats.ignored++;
        spinlock_buka(&signal_lock);
        return STATUS_BERHASIL;
    }

    /* Set sinyal tertunda */
    signal_set_pending(target, sig);

    /* Jika proses sedang blocked, wake up */
    if (target->status == PROSES_STATUS_TUNGGU) {
        scheduler_unblock(target);
    }

    spinlock_buka(&signal_lock);

    return STATUS_BERHASIL;
}

/*
 * signal_handle_pending
 * ---------------------
 * Handle sinyal tertunda untuk proses saat ini.
 */
void signal_handle_pending(void)
{
    proses_t *proses;
    tak_bertanda32_t sig;
    tak_bertanda32_t pending;

    if (!signal_initialized) {
        return;
    }

    proses = proses_get_current();
    if (proses == NULL) {
        return;
    }

    spinlock_kunci(&signal_lock);

    pending = proses->signal_pending;

    /* Proses setiap sinyal tertunda */
    for (sig = 1; sig <= SIGNAL_MAX && pending != 0; sig++) {
        if (pending & (1UL << sig)) {
            /* Clear pending */
            signal_clear_pending(proses, sig);

            /* Cek jika tidak di-mask */
            if (!signal_is_masked(proses, sig)) {
                /* Deliver sinyal */
                signal_deliver(proses, sig);
            }
        }
    }

    spinlock_buka(&signal_lock);
}

/*
 * signal_pending
 * --------------
 * Cek apakah ada sinyal tertunda.
 *
 * Return: BENAR jika ada
 */
bool_t signal_pending(void)
{
    proses_t *proses;

    proses = proses_get_current();
    if (proses == NULL) {
        return SALAH;
    }

    return (proses->signal_pending != 0) ? BENAR : SALAH;
}

/*
 * signal_set_handler
 * ------------------
 * Set handler sinyal.
 *
 * Parameter:
 *   sig     - Nomor sinyal
 *   handler - Fungsi handler
 *   flags   - Flag handler
 *
 * Return: Status operasi
 */
status_t signal_set_handler(tak_bertanda32_t sig, void (*handler)(int),
                            tak_bertanda32_t flags)
{
    proses_t *proses;

    if (!signal_initialized) {
        return STATUS_GAGAL;
    }

    /* Validasi sinyal */
    if (sig == 0 || sig > SIGNAL_MAX) {
        return STATUS_PARAM_INVALID;
    }

    /* SIGKILL dan SIGSTOP tidak bisa di-handle */
    if (sig == SIGKILL || sig == SIGSTOP) {
        return STATUS_TIDAK_DUKUNG;
    }

    proses = proses_get_current();
    if (proses == NULL) {
        return STATUS_GAGAL;
    }

    spinlock_kunci(&signal_lock);

    proses->signal_handlers[sig].handler = handler;
    proses->signal_handlers[sig].flags = flags;

    spinlock_buka(&signal_lock);

    return STATUS_BERHASIL;
}

/*
 * signal_set_mask
 * ---------------
 * Set mask sinyal.
 *
 * Parameter:
 *   mask - Mask baru
 */
void signal_set_mask(tak_bertanda32_t mask)
{
    proses_t *proses;

    proses = proses_get_current();
    if (proses == NULL) {
        return;
    }

    /* SIGKILL dan SIGSTOP tidak bisa di-mask */
    mask &= ~((1UL << SIGKILL) | (1UL << SIGSTOP));

    proses->signal_mask = mask;
}

/*
 * signal_block
 * ------------
 * Block sinyal tertentu.
 *
 * Parameter:
 *   mask - Sinyal yang di-block
 */
void signal_block(tak_bertanda32_t mask)
{
    proses_t *proses;

    proses = proses_get_current();
    if (proses == NULL) {
        return;
    }

    /* SIGKILL dan SIGSTOP tidak bisa di-block */
    mask &= ~((1UL << SIGKILL) | (1UL << SIGSTOP));

    proses->signal_mask |= mask;
}

/*
 * signal_unblock
 * --------------
 * Unblock sinyal tertentu.
 *
 * Parameter:
 *   mask - Sinyal yang di-unblock
 */
void signal_unblock(tak_bertanda32_t mask)
{
    proses_t *proses;

    proses = proses_get_current();
    if (proses == NULL) {
        return;
    }

    proses->signal_mask &= ~mask;
}

/*
 * signal_get_pending_set
 * ----------------------
 * Dapatkan set sinyal tertunda.
 *
 * Return: Bit set sinyal tertunda
 */
tak_bertanda32_t signal_get_pending_set(void)
{
    proses_t *proses;

    proses = proses_get_current();
    if (proses == NULL) {
        return 0;
    }

    return proses->signal_pending;
}

/*
 * signal_get_mask
 * ---------------
 * Dapatkan mask sinyal saat ini.
 *
 * Return: Bit mask sinyal
 */
tak_bertanda32_t signal_get_mask(void)
{
    proses_t *proses;

    proses = proses_get_current();
    if (proses == NULL) {
        return 0;
    }

    return proses->signal_mask;
}

/*
 * signal_setup_frame
 * ------------------
 * Setup stack frame untuk signal handler.
 *
 * Parameter:
 *   proses  - Pointer ke proses
 *   sig     - Nomor sinyal
 *   handler - Fungsi handler
 */
void signal_setup_frame(proses_t *proses, tak_bertanda32_t sig,
                        void (*handler)(int))
{
    tak_bertanda32_t *stack;
    void *user_stack;

    if (proses == NULL || handler == NULL) {
        return;
    }

    /* Dapatkan stack user */
    user_stack = context_get_stack(proses->main_thread->context);
    if (user_stack == NULL) {
        return;
    }

    /* Alokasikan frame di stack */
    stack = (tak_bertanda32_t *)((tak_bertanda8_t *)user_stack - 128);

    /* Simpan context saat ini */
    context_save_to_stack(proses->main_thread->context, stack);

    /* Push argumen untuk handler */
    stack--;
    *stack = sig;  /* Argument */

    /* Push return address (signal trampoline) */
    stack--;
    *stack = (tak_bertanda32_t)(alamat_ptr_t)proses->signal_trampoline;

    /* Set context untuk handler */
    context_set_entry(proses->main_thread->context, (alamat_virtual_t)handler);
    context_set_stack(proses->main_thread->context, stack);

    /* Set signal mask selama handler */
    proses->saved_signal_mask = proses->signal_mask;
    proses->signal_mask |= (1UL << sig);
}

/*
 * signal_restore
 * --------------
 * Restore context setelah signal handler.
 *
 * Parameter:
 *   proses - Pointer ke proses
 */
void signal_restore(proses_t *proses)
{
    if (proses == NULL) {
        return;
    }

    /* Restore signal mask */
    proses->signal_mask = proses->saved_signal_mask;

    /* Restore context dari stack */
    context_restore_from_stack(proses->main_thread->context);
}

/*
 * signal_get_name
 * ---------------
 * Dapatkan nama sinyal.
 *
 * Parameter:
 *   sig - Nomor sinyal
 *
 * Return: Nama sinyal
 */
const char *signal_get_name(tak_bertanda32_t sig)
{
    static const char *names[] = {
        [0]         = "NULL",
        [SIGHUP]    = "SIGHUP",
        [SIGINT]    = "SIGINT",
        [SIGQUIT]   = "SIGQUIT",
        [SIGILL]    = "SIGILL",
        [SIGTRAP]   = "SIGTRAP",
        [SIGABRT]   = "SIGABRT",
        [SIGBUS]    = "SIGBUS",
        [SIGFPE]    = "SIGFPE",
        [SIGKILL]   = "SIGKILL",
        [SIGUSR1]   = "SIGUSR1",
        [SIGSEGV]   = "SIGSEGV",
        [SIGUSR2]   = "SIGUSR2",
        [SIGPIPE]   = "SIGPIPE",
        [SIGALRM]   = "SIGALRM",
        [SIGTERM]   = "SIGTERM",
        [SIGCHLD]   = "SIGCHLD",
        [SIGCONT]   = "SIGCONT",
        [SIGSTOP]   = "SIGSTOP",
        [SIGTSTP]   = "SIGTSTP"
    };

    if (sig > SIGNAL_MAX) {
        return "UNKNOWN";
    }

    return names[sig] ? names[sig] : "UNKNOWN";
}

/*
 * signal_get_stats
 * ----------------
 * Dapatkan statistik sinyal.
 *
 * Parameter:
 *   sent     - Pointer untuk sinyal terkirim
 *   delivered - Pointer untuk sinyal terdeliver
 *   ignored  - Pointer untuk sinyal diabaikan
 *   dropped  - Pointer untuk sinyal di-drop
 */
void signal_get_stats(tak_bertanda64_t *sent, tak_bertanda64_t *delivered,
                      tak_bertanda64_t *ignored, tak_bertanda64_t *dropped)
{
    if (sent != NULL) {
        *sent = signal_stats.sent;
    }

    if (delivered != NULL) {
        *delivered = signal_stats.delivered;
    }

    if (ignored != NULL) {
        *ignored = signal_stats.ignored;
    }

    if (dropped != NULL) {
        *dropped = signal_stats.dropped;
    }
}

/*
 * signal_print_stats
 * ------------------
 * Print statistik sinyal.
 */
void signal_print_stats(void)
{
    kernel_printf("\n[SIGNAL] Statistik Sinyal:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Sent      : %lu\n", signal_stats.sent);
    kernel_printf("  Delivered : %lu\n", signal_stats.delivered);
    kernel_printf("  Ignored   : %lu\n", signal_stats.ignored);
    kernel_printf("  Dropped   : %lu\n", signal_stats.dropped);
    kernel_printf("========================================\n");
}
