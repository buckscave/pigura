/*
 * PIGURA OS - SIGNAL.C
 * ---------------------
 * Implementasi penanganan sinyal.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengirim dan menangani
 * sinyal antar proses dalam sistem operasi Pigura.
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

/* Jumlah sinyal maksimum */
#ifndef SIGNAL_JUMLAH
#define SIGNAL_JUMLAH           32
#endif

/* Nomor sinyal standar (POSIX) */
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
#define SIGCONT                 18   /* Continue */
#define SIGSTOP                 19   /* Stop (cannot be caught) */
#define SIGTSTP                 20   /* Stop from keyboard */
#define SIGTTIN                 21   /* Background read */
#define SIGTTOU                 22   /* Background write */
#define SIGURG                  23   /* Urgent condition */
#define SIGXCPU                 24   /* CPU time limit exceeded */
#define SIGXFSZ                 25   /* File size limit exceeded */
#define SIGVTALRM               26   /* Virtual alarm */
#define SIGPROF                 27   /* Profiling alarm */
#define SIGWINCH                28   /* Window size change */
#define SIGIO                   29   /* I/O now possible */
#define SIGPWR                  30   /* Power failure */
#define SIGSYS                  31   /* Bad system call */

/* Aksi default sinyal */
#define SIG_ACT_TERM            0    /* Terminate */
#define SIG_ACT_CORE            1    /* Terminate with core dump */
#define SIG_ACT_IGNORE          2    /* Ignore */
#define SIG_ACT_STOP            3    /* Stop process */
#define SIG_ACT_CONT            4    /* Continue if stopped */

/* Flag handler */
#define SA_NOCLDSTOP            0x00000001
#define SA_NOCLDWAIT            0x00000002
#define SA_SIGINFO              0x00000004
#define SA_ONSTACK              0x00000008
#define SA_RESTART              0x00000010
#define SA_NODEFER              0x00000020
#define SA_RESETHAND            0x00000040

/* Bit manipulation */
#define SIGNAL_BIT(sig)         (1UL << ((sig) - 1))
#define SIGNAL_VALID(sig)       ((sig) > 0 && (sig) <= SIGNAL_JUMLAH)

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* Tabel aksi default */
static const tak_bertanda8_t default_action[SIGNAL_JUMLAH + 1] = {
    0,                          /* 0 - tidak digunakan */
    SIG_ACT_TERM,               /* 1  - SIGHUP */
    SIG_ACT_TERM,               /* 2  - SIGINT */
    SIG_ACT_CORE,               /* 3  - SIGQUIT */
    SIG_ACT_CORE,               /* 4  - SIGILL */
    SIG_ACT_CORE,               /* 5  - SIGTRAP */
    SIG_ACT_CORE,               /* 6  - SIGABRT */
    SIG_ACT_CORE,               /* 7  - SIGBUS */
    SIG_ACT_CORE,               /* 8  - SIGFPE */
    SIG_ACT_TERM,               /* 9  - SIGKILL */
    SIG_ACT_TERM,               /* 10 - SIGUSR1 */
    SIG_ACT_CORE,               /* 11 - SIGSEGV */
    SIG_ACT_TERM,               /* 12 - SIGUSR2 */
    SIG_ACT_TERM,               /* 13 - SIGPIPE */
    SIG_ACT_TERM,               /* 14 - SIGALRM */
    SIG_ACT_TERM,               /* 15 - SIGTERM */
    SIG_ACT_TERM,               /* 16 - SIGSTKFLT */
    SIG_ACT_IGNORE,             /* 17 - SIGCHLD */
    SIG_ACT_CONT,               /* 18 - SIGCONT */
    SIG_ACT_STOP,               /* 19 - SIGSTOP */
    SIG_ACT_STOP,               /* 20 - SIGTSTP */
    SIG_ACT_STOP,               /* 21 - SIGTTIN */
    SIG_ACT_STOP,               /* 22 - SIGTTOU */
    SIG_ACT_IGNORE,             /* 23 - SIGURG */
    SIG_ACT_TERM,               /* 24 - SIGXCPU */
    SIG_ACT_TERM,               /* 25 - SIGXFSZ */
    SIG_ACT_TERM,               /* 26 - SIGVTALRM */
    SIG_ACT_TERM,               /* 27 - SIGPROF */
    SIG_ACT_IGNORE,             /* 28 - SIGWINCH */
    SIG_ACT_IGNORE,             /* 29 - SIGIO */
    SIG_ACT_TERM,               /* 30 - SIGPWR */
    SIG_ACT_CORE                /* 31 - SIGSYS */
};

/* Nama sinyal */
static const char *signal_names[SIGNAL_JUMLAH + 1] = {
    "NULL",
    "SIGHUP",    "SIGINT",    "SIGQUIT",   "SIGILL",
    "SIGTRAP",   "SIGABRT",   "SIGBUS",    "SIGFPE",
    "SIGKILL",   "SIGUSR1",   "SIGSEGV",   "SIGUSR2",
    "SIGPIPE",   "SIGALRM",   "SIGTERM",   "SIGSTKFLT",
    "SIGCHLD",   "SIGCONT",   "SIGSTOP",   "SIGTSTP",
    "SIGTTIN",   "SIGTTOU",   "SIGURG",    "SIGXCPU",
    "SIGXFSZ",   "SIGVTALRM", "SIGPROF",   "SIGWINCH",
    "SIGIO",     "SIGPWR",    "SIGSYS"
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
    tak_bertanda64_t kernel_signals;
    tak_bertanda64_t user_signals;
    tak_bertanda64_t pending_overflow;
} signal_stats = {0, 0, 0, 0, 0, 0, 0};

/* Status inisialisasi */
static bool_t signal_initialized = SALAH;

/* Lock untuk operasi sinyal */
static spinlock_t signal_lock;

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
    if (proses == NULL || !SIGNAL_VALID(sig)) {
        return SALAH;
    }
    
    return (proses->signal_mask & SIGNAL_BIT(sig)) ? BENAR : SALAH;
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
    if (proses == NULL || !SIGNAL_VALID(sig)) {
        return SALAH;
    }
    
    return (proses->signal_pending & SIGNAL_BIT(sig)) ? BENAR : SALAH;
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
    if (proses == NULL || !SIGNAL_VALID(sig)) {
        return;
    }
    
    proses->signal_pending |= SIGNAL_BIT(sig);
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
    if (proses == NULL || !SIGNAL_VALID(sig)) {
        return;
    }
    
    proses->signal_pending &= ~SIGNAL_BIT(sig);
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
    if (proses == NULL || !SIGNAL_VALID(sig)) {
        return SIGNAL_HANDLER_DEFAULT;
    }
    
    return proses->signal_handlers[sig - 1].handler;
}

/*
 * signal_is_ignored
 * -----------------
 * Cek apakah sinyal diabaikan.
 *
 * Parameter:
 *   proses - Pointer ke proses
 *   sig    - Nomor sinyal
 *
 * Return: BENAR jika diabaikan
 */
static bool_t signal_is_ignored(proses_t *proses, tak_bertanda32_t sig)
{
    void (*handler)(int);
    
    if (proses == NULL || !SIGNAL_VALID(sig)) {
        return SALAH;
    }
    
    /* SIGKILL dan SIGSTOP tidak bisa diabaikan */
    if (sig == SIGKILL || sig == SIGSTOP) {
        return SALAH;
    }
    
    handler = signal_get_handler(proses, sig);
    
    return (handler == SIGNAL_HANDLER_IGNORE) ? BENAR : SALAH;
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
    
    if (proses == NULL || !SIGNAL_VALID(sig)) {
        return;
    }
    
    /* Jangan eksekusi default untuk proses kernel */
    if (proses->flags & PROSES_FLAG_KERNEL) {
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
            proses->exit_signal = sig;
            proses->flags |= PROSES_FLAG_STOPPED;
            scheduler_hapus_thread(proses->main_thread);
            break;
            
        case SIG_ACT_CONT:
            /* Continue jika stopped */
            if (proses->status == PROSES_STATUS_STOP) {
                proses->status = PROSES_STATUS_SIAP;
                proses->flags &= ~PROSES_FLAG_STOPPED;
                proses->flags |= PROSES_FLAG_CONTINUED;
                scheduler_tambah_thread(proses->main_thread);
            }
            break;
            
        default:
            /* Default: terminate */
            exit_by_signal(proses, sig, SALAH);
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
    tak_bertanda32_t action __attribute__((unused));
    
    if (proses == NULL || !SIGNAL_VALID(sig)) {
        return;
    }
    
    signal_stats.delivered++;
    
    /* Dapatkan handler */
    handler = signal_get_handler(proses, sig);
    
    /* Cek tipe handler */
    if (handler == SIGNAL_HANDLER_DEFAULT) {
        /* Eksekusi aksi default */
        signal_execute_default(proses, sig);
    } else if (handler == SIGNAL_HANDLER_IGNORE) {
        /* Abaikan */
        signal_stats.ignored++;
    } else {
        /* Setup frame untuk user handler */
        signal_setup_frame(proses, sig, handler);
    }
}

/*
 * signal_find_next_pending
 * ------------------------
 * Cari sinyal tertunda berikutnya.
 *
 * Parameter:
 *   proses - Pointer ke proses
 *   mask   - Mask sinyal yang diblok
 *
 * Return: Nomor sinyal, atau 0 jika tidak ada
 */
static tak_bertanda32_t signal_find_next_pending(proses_t *proses,
                                                 tak_bertanda32_t mask)
{
    tak_bertanda32_t pending;
    tak_bertanda32_t sig;
    
    if (proses == NULL) {
        return 0;
    }
    
    pending = proses->signal_pending & ~mask;
    
    if (pending == 0) {
        return 0;
    }
    
    /* Cari bit terendah yang set */
    for (sig = 1; sig <= SIGNAL_JUMLAH; sig++) {
        if (pending & SIGNAL_BIT(sig)) {
            return sig;
        }
    }
    
    return 0;
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
    kernel_printf("         Max signals: %d\n", SIGNAL_JUMLAH);
    
    return STATUS_BERHASIL;
}

/*
 * signal_kirim
 * ------------
 * Kirim sinyal ke proses.
 *
 * Parameter:
 *   pid - Process ID target
 *   sig - Nomor sinyal
 *
 * Return: Status operasi
 */
status_t signal_kirim(pid_t pid, tak_bertanda32_t sig)
{
    proses_t *target;
    
    if (!signal_initialized) {
        return STATUS_GAGAL;
    }
    
    /* Validasi sinyal */
    if (!SIGNAL_VALID(sig)) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Cari proses target */
    target = proses_cari(pid);
    if (target == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    return signal_kirim_ke_proses(target, sig);
}

/*
 * signal_kirim_ke_proses
 * ----------------------
 * Kirim sinyal ke proses (dengan pointer proses).
 *
 * Parameter:
 *   target - Pointer ke proses target
 *   sig    - Nomor sinyal
 *
 * Return: Status operasi
 */
status_t signal_kirim_ke_proses(proses_t *target, tak_bertanda32_t sig)
{
    thread_t *thread;
    
    if (!signal_initialized || target == NULL) {
        return STATUS_GAGAL;
    }
    
    /* Validasi sinyal */
    if (!SIGNAL_VALID(sig)) {
        return STATUS_PARAM_INVALID;
    }
    
    spinlock_kunci(&signal_lock);
    
    signal_stats.sent++;
    
    /* Cek apakah sinyal diabaikan */
    if (signal_is_ignored(target, sig)) {
        signal_stats.ignored++;
        spinlock_buka(&signal_lock);
        return STATUS_BERHASIL;
    }
    
    /* Set sinyal tertunda */
    signal_set_pending(target, sig);
    
    /* Update statistik */
    if (target->flags & PROSES_FLAG_KERNEL) {
        signal_stats.kernel_signals++;
    } else {
        signal_stats.user_signals++;
    }
    
    /* Jika proses sedang blocked, wake up */
    if (target->main_thread != NULL &&
        target->main_thread->status == THREAD_STATUS_TUNGGU) {
        scheduler_unblock(target->main_thread);
    }
    
    /* Wake up semua thread dalam proses */
    thread = target->threads;
    while (thread != NULL) {
        if (thread->status == THREAD_STATUS_TUNGGU ||
            thread->status == THREAD_STATUS_SLEEP) {
            scheduler_unblock(thread);
        }
        thread = thread->next;
    }
    
    spinlock_buka(&signal_lock);
    
    return STATUS_BERHASIL;
}

/*
 * signal_kirim_ke_group
 * ---------------------
 * Kirim sinyal ke process group.
 *
 * Parameter:
 *   pgid - Process group ID
 *   sig  - Nomor sinyal
 *
 * Return: Status operasi
 */
status_t signal_kirim_ke_group(pid_t pgid, tak_bertanda32_t sig)
{
    proses_t *proses;
    status_t result;
    tak_bertanda32_t count;
    
    if (!signal_initialized) {
        return STATUS_GAGAL;
    }
    
    if (!SIGNAL_VALID(sig)) {
        return STATUS_PARAM_INVALID;
    }
    
    result = STATUS_BERHASIL;
    count = 0;
    
    /* Kirim ke semua proses dalam group */
    proses = proses_dapat_kernel();
    while (proses != NULL) {
        if (proses->pgid == pgid) {
            if (signal_kirim_ke_proses(proses, sig) == STATUS_BERHASIL) {
                count++;
            }
        }
        proses = proses->next;
    }
    
    if (count == 0) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    return result;
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
    tak_bertanda32_t old_mask;
    
    if (!signal_initialized) {
        return;
    }
    
    proses = proses_dapat_saat_ini();
    if (proses == NULL) {
        return;
    }
    
    /* Cek apakah ada sinyal tertunda */
    if (proses->signal_pending == 0) {
        return;
    }
    
    spinlock_kunci(&signal_lock);
    
    /* Cari sinyal tertunda yang tidak di-mask */
    sig = signal_find_next_pending(proses, proses->signal_mask);
    
    while (sig != 0) {
        /* Clear pending */
        signal_clear_pending(proses, sig);
        
        /* Save old mask dan set new mask */
        old_mask = proses->signal_mask;
        
        /* Saat handling signal, block signal tersebut */
        if (!(proses->signal_handlers[sig - 1].flags & SA_NODEFER)) {
            proses->signal_mask |= SIGNAL_BIT(sig);
        }
        
        /* Deliver sinyal */
        signal_deliver(proses, sig);
        
        /* Restore mask jika bukan SA_NODEFER */
        if (!(proses->signal_handlers[sig - 1].flags & SA_NODEFER)) {
            proses->signal_mask = old_mask;
        }
        
        /* Cari sinyal berikutnya */
        sig = signal_find_next_pending(proses, proses->signal_mask);
    }
    
    spinlock_buka(&signal_lock);
}

/*
 * signal_ada_pending
 * ------------------
 * Cek apakah ada sinyal tertunda.
 *
 * Return: BENAR jika ada
 */
bool_t signal_ada_pending(void)
{
    proses_t *proses;
    
    proses = proses_dapat_saat_ini();
    if (proses == NULL) {
        return SALAH;
    }
    
    /* Cek apakah ada sinyal yang tidak di-mask */
    return (signal_find_next_pending(proses, proses->signal_mask) != 0) ?
            BENAR : SALAH;
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
    if (!SIGNAL_VALID(sig)) {
        return STATUS_PARAM_INVALID;
    }
    
    /* SIGKILL dan SIGSTOP tidak bisa di-handle */
    if (sig == SIGKILL || sig == SIGSTOP) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    proses = proses_dapat_saat_ini();
    if (proses == NULL) {
        return STATUS_GAGAL;
    }
    
    spinlock_kunci(&signal_lock);
    
    proses->signal_handlers[sig - 1].handler = handler;
    proses->signal_handlers[sig - 1].flags = flags;
    proses->signal_handlers[sig - 1].mask = 0;
    
    /* Jika handler di-set, clear ignored flag */
    if (handler != SIGNAL_HANDLER_IGNORE) {
        proses->signal_ignored &= ~SIGNAL_BIT(sig);
    }
    
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
    
    proses = proses_dapat_saat_ini();
    if (proses == NULL) {
        return;
    }
    
    /* SIGKILL dan SIGSTOP tidak bisa di-mask */
    mask &= ~(SIGNAL_BIT(SIGKILL) | SIGNAL_BIT(SIGSTOP));
    
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
    
    proses = proses_dapat_saat_ini();
    if (proses == NULL) {
        return;
    }
    
    /* SIGKILL dan SIGSTOP tidak bisa di-block */
    mask &= ~(SIGNAL_BIT(SIGKILL) | SIGNAL_BIT(SIGSTOP));
    
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
    
    proses = proses_dapat_saat_ini();
    if (proses == NULL) {
        return;
    }
    
    proses->signal_mask &= ~mask;
}

/*
 * signal_dapat_mask
 * -----------------
 * Dapatkan mask sinyal saat ini.
 *
 * Return: Bit mask sinyal
 */
tak_bertanda32_t signal_dapat_mask(void)
{
    proses_t *proses;
    
    proses = proses_dapat_saat_ini();
    if (proses == NULL) {
        return 0;
    }
    
    return proses->signal_mask;
}

/*
 * signal_dapat_pending_set
 * ------------------------
 * Dapatkan set sinyal tertunda.
 *
 * Return: Bit set sinyal tertunda
 */
tak_bertanda32_t signal_dapat_pending_set(void)
{
    proses_t *proses;
    
    proses = proses_dapat_saat_ini();
    if (proses == NULL) {
        return 0;
    }
    
    return proses->signal_pending;
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
    cpu_context_t *ctx;
    tak_bertanda32_t *stack;
    void *user_stack;
    
    if (proses == NULL || handler == NULL ||
        proses->main_thread == NULL) {
        return;
    }
    
    ctx = (cpu_context_t *)proses->main_thread->context;
    if (ctx == NULL) {
        return;
    }
    
    /* Dapatkan stack user */
    user_stack = context_dapat_stack(ctx);
    if (user_stack == NULL) {
        /* Tidak ada stack, eksekusi default */
        signal_execute_default(proses, sig);
        return;
    }
    
    /* Alokasikan frame di stack */
    stack = (tak_bertanda32_t *)user_stack;
    
    /* Simpan context saat ini ke stack */
#if defined(PIGURA_ARSITEKTUR_64BIT)
    stack -= 10;
    stack[0] = (tak_bertanda32_t)ctx->rip;
    stack[1] = (tak_bertanda32_t)ctx->rsp;
    stack[2] = (tak_bertanda32_t)ctx->rflags;
    stack[3] = (tak_bertanda32_t)ctx->rax;
    stack[4] = (tak_bertanda32_t)ctx->rbx;
    stack[5] = (tak_bertanda32_t)ctx->rcx;
    stack[6] = (tak_bertanda32_t)ctx->rdx;
    stack[7] = (tak_bertanda32_t)ctx->rsi;
    stack[8] = (tak_bertanda32_t)ctx->rdi;
    stack[9] = (tak_bertanda32_t)ctx->rbp;
#else
    stack -= 10;
    stack[0] = ctx->eip;
    stack[1] = ctx->esp;
    stack[2] = ctx->eflags;
    stack[3] = ctx->eax;
    stack[4] = ctx->ebx;
    stack[5] = ctx->ecx;
    stack[6] = ctx->edx;
    stack[7] = ctx->esi;
    stack[8] = ctx->edi;
    stack[9] = ctx->ebp;
#endif
    
    /* Push argumen untuk handler */
    stack--;
    *stack = sig;
    
    /* Push return address (signal trampoline) */
    stack--;
    if (proses->signal_trampoline != NULL) {
        *stack = (tak_bertanda32_t)(alamat_ptr_t)proses->signal_trampoline;
    } else {
        *stack = 0;  /* Default return address */
    }
    
    /* Set context untuk handler */
    context_set_entry(ctx, (alamat_virtual_t)handler);
    context_set_stack(ctx, stack);
    
    /* Set signal mask selama handler */
    proses->saved_signal_mask = proses->signal_mask;
    proses->signal_mask |= SIGNAL_BIT(sig);
    
    /* Increment call count */
    proses->signal_handlers[sig - 1].call_count++;
    proses->signal_count++;
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
    cpu_context_t *ctx;
    tak_bertanda32_t *stack;
    
    if (proses == NULL || proses->main_thread == NULL) {
        return;
    }
    
    ctx = (cpu_context_t *)proses->main_thread->context;
    if (ctx == NULL) {
        return;
    }
    
    /* Restore signal mask */
    proses->signal_mask = proses->saved_signal_mask;
    
    /* Dapatkan stack */
    stack = (tak_bertanda32_t *)context_dapat_stack(ctx);
    if (stack == NULL) {
        return;
    }
    
    /* Skip return address dan argumen */
    stack += 2;
    
    /* Restore context dari stack */
#if defined(PIGURA_ARSITEKTUR_64BIT)
    ctx->rip = stack[0];
    ctx->rsp = stack[1];
    ctx->rflags = stack[2];
    ctx->rax = stack[3];
    ctx->rbx = stack[4];
    ctx->rcx = stack[5];
    ctx->rdx = stack[6];
    ctx->rsi = stack[7];
    ctx->rdi = stack[8];
    ctx->rbp = stack[9];
#else
    ctx->eip = stack[0];
    ctx->esp = stack[1];
    ctx->eflags = stack[2];
    ctx->eax = stack[3];
    ctx->ebx = stack[4];
    ctx->ecx = stack[5];
    ctx->edx = stack[6];
    ctx->esi = stack[7];
    ctx->edi = stack[8];
    ctx->ebp = stack[9];
#endif
}

/*
 * signal_dapat_nama
 * -----------------
 * Dapatkan nama sinyal.
 *
 * Parameter:
 *   sig - Nomor sinyal
 *
 * Return: Nama sinyal
 */
const char *signal_dapat_nama(tak_bertanda32_t sig)
{
    if (sig > SIGNAL_JUMLAH) {
        return "UNKNOWN";
    }
    
    return signal_names[sig];
}

/*
 * signal_dapat_statistik
 * ----------------------
 * Dapatkan statistik sinyal.
 *
 * Parameter:
 *   sent     - Pointer untuk sinyal terkirim
 *   delivered - Pointer untuk sinyal terdeliver
 *   ignored  - Pointer untuk sinyal diabaikan
 *   dropped  - Pointer untuk sinyal di-drop
 */
void signal_dapat_statistik(tak_bertanda64_t *sent,
                            tak_bertanda64_t *delivered,
                            tak_bertanda64_t *ignored,
                            tak_bertanda64_t *dropped)
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
    kernel_printf("  Sent           : %lu\n", signal_stats.sent);
    kernel_printf("  Delivered      : %lu\n", signal_stats.delivered);
    kernel_printf("  Ignored        : %lu\n", signal_stats.ignored);
    kernel_printf("  Dropped        : %lu\n", signal_stats.dropped);
    kernel_printf("  Kernel Signals : %lu\n", signal_stats.kernel_signals);
    kernel_printf("  User Signals   : %lu\n", signal_stats.user_signals);
    kernel_printf("  Pending Overflw: %lu\n",
                  signal_stats.pending_overflow);
    kernel_printf("========================================\n");
}
