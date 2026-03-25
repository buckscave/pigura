/*
 * PIGURA LIBC - SIGNAL/SIGNAL.C
 * ==============================
 * Implementasi fungsi penanganan sinyal.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Implementasi untuk Pigura OS dengan dukungan
 * POSIX.1 signal handling.
 */

#include <signal.h>
#include <errno.h>
#include <sys/types.h>

/* ============================================================
 * DEKLARASI SYSCALL
 * ============================================================
 */

/* Syscall interface ke kernel */
extern long __syscall_signal(int signum, void (*handler)(int));
extern long __syscall_sigaction(int signum, const void *act, void *oact);
extern long __syscall_sigprocmask(int how, const sigset_t *set,
                                  sigset_t *oset);
extern long __syscall_sigpending(sigset_t *set);
extern long __syscall_sigsuspend(const sigset_t *mask);
extern long __syscall_kill(pid_t pid, int signum);
extern long __syscall_killpg(pid_t pgrp, int signum);
extern long __syscall_rt_sigreturn(void);

/* ============================================================
 * VARIABEL GLOBAL
 * ============================================================
 */

/* Default signal handlers */
static void sig_default_ignore(int signum);
static void sig_default_terminate(int signum);
static void sig_default_core(int signum);
static void sig_default_stop(int signum);
static void sig_default_continue(int signum);

/* Tabel handler untuk setiap sinyal */
static struct sigaction sig_table[NSIG];

/* Flag inisialisasi */
static int sig_initialized = 0;

/* ============================================================
 * KONSTANTA INTERNAL
 * ============================================================
 */

/* Signal dispositions default */
#define SIG_DFL_IGN     1   /* Ignore by default */
#define SIG_DFL_TERM    2   /* Terminate by default */
#define SIG_DFL_CORE    3   /* Core dump by default */
#define SIG_DFL_STOP    4   /* Stop by default */
#define SIG_DFL_CONT    5   /* Continue by default */

/* Tabel default disposition per signal */
static const unsigned char sig_default_disp[NSIG] = {
    0,              /* 0 - unused */
    SIG_DFL_TERM,   /* 1 - SIGHUP */
    SIG_DFL_TERM,   /* 2 - SIGINT */
    SIG_DFL_CORE,   /* 3 - SIGQUIT */
    SIG_DFL_CORE,   /* 4 - SIGILL */
    SIG_DFL_CORE,   /* 5 - SIGTRAP */
    SIG_DFL_CORE,   /* 6 - SIGABRT */
    SIG_DFL_CORE,   /* 7 - SIGBUS */
    SIG_DFL_CORE,   /* 8 - SIGFPE */
    SIG_DFL_TERM,   /* 9 - SIGKILL */
    SIG_DFL_TERM,   /* 10 - SIGUSR1 */
    SIG_DFL_CORE,   /* 11 - SIGSEGV */
    SIG_DFL_CORE,   /* 12 - SIGSYS */
    SIG_DFL_TERM,   /* 13 - SIGPIPE */
    SIG_DFL_TERM,   /* 14 - SIGALRM */
    SIG_DFL_TERM,   /* 15 - SIGTERM */
    SIG_DFL_IGN,    /* 16 - SIGURG */
    SIG_DFL_STOP,   /* 17 - SIGSTOP */
    SIG_DFL_STOP,   /* 18 - SIGTSTP */
    SIG_DFL_CONT,   /* 19 - SIGCONT */
    SIG_DFL_IGN,    /* 20 - SIGCHLD */
    SIG_DFL_STOP,   /* 21 - SIGTTIN */
    SIG_DFL_STOP,   /* 22 - SIGTTOU */
    SIG_DFL_IGN,    /* 23 - SIGIO */
    SIG_DFL_TERM,   /* 24 - SIGXCPU */
    SIG_DFL_TERM,   /* 25 - SIGXFSZ */
    SIG_DFL_TERM,   /* 26 - SIGVTALRM */
    SIG_DFL_TERM,   /* 27 - SIGPROF */
    SIG_DFL_IGN,    /* 28 - SIGWINCH */
    SIG_DFL_IGN,    /* 29 - SIGINFO */
    SIG_DFL_TERM,   /* 30 - SIGUSR2 */
};

/* ============================================================
 * FUNGSI INISIALISASI
 * ============================================================
 */

/*
 * sig_init - Inisialisasi signal subsystem
 *
 * Dipanggil sekali saat pertama kali signal() dipanggil.
 */
static void sig_init(void) {
    int i;

    if (sig_initialized) {
        return;
    }

    /* Inisialisasi tabel handler dengan default */
    for (i = 1; i < NSIG; i++) {
        sig_table[i].sa_handler = SIG_DFL;
        sig_table[i].sa_mask = 0;
        sig_table[i].sa_flags = 0;
    }

    sig_initialized = 1;
}

/* ============================================================
 * DEFAULT SIGNAL HANDLERS
 * ============================================================
 */

/*
 * sig_default_ignore - Handler default untuk signal yang diabaikan
 */
static void sig_default_ignore(int signum) {
    (void)signum;
    /* Tidak melakukan apa-apa */
}

/*
 * sig_default_terminate - Handler default untuk signal terminate
 */
static void sig_default_terminate(int signum) {
    /* Exit dengan signal number sebagai status */
    _exit(128 + signum);
}

/*
 * sig_default_core - Handler default untuk signal core dump
 */
static void sig_default_core(int signum) {
    /* TODO: Generate core dump jika diizinkan */
    _exit(128 + signum);
}

/*
 * sig_default_stop - Handler default untuk signal stop
 */
static void sig_default_stop(int signum) {
    (void)signum;
    /* Syscall untuk stop process */
    /* __syscall_kill(getpid(), SIGSTOP); */
}

/*
 * sig_default_continue - Handler default untuk signal continue
 */
static void sig_default_continue(int signum) {
    (void)signum;
    /* Process akan melanjutkan eksekusi */
}

/* ============================================================
 * FUNGSI MANIPULASI SIGSET
 * ============================================================
 */

/*
 * sigemptyset - Inisialisasi signal set kosong
 *
 * Parameter:
 *   set - Pointer ke signal set
 *
 * Return: 0 jika berhasil, -1 jika gagal
 */
int sigemptyset(sigset_t *set) {
    if (set == NULL) {
        errno = EINVAL;
        return -1;
    }

    *set = 0;
    return 0;
}

/*
 * sigfillset - Inisialisasi signal set dengan semua signal
 *
 * Parameter:
 *   set - Pointer ke signal set
 *
 * Return: 0 jika berhasil, -1 jika gagal
 */
int sigfillset(sigset_t *set) {
    if (set == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Set semua bit kecuali signal 0 */
    *set = (sigset_t)-1;

    /* Unset bit untuk signal yang tidak bisa di-block */
    *set &= ~(1 << (SIGKILL - 1));
    *set &= ~(1 << (SIGSTOP - 1));

    return 0;
}

/*
 * sigaddset - Tambah signal ke signal set
 *
 * Parameter:
 *   set    - Pointer ke signal set
 *   signum - Signal number
 *
 * Return: 0 jika berhasil, -1 jika gagal
 */
int sigaddset(sigset_t *set, int signum) {
    if (set == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Validasi signal number */
    if (signum <= 0 || signum >= NSIG) {
        errno = EINVAL;
        return -1;
    }

    /* Set bit untuk signal ini */
    *set |= (1 << (signum - 1));

    return 0;
}

/*
 * sigdelset - Hapus signal dari signal set
 *
 * Parameter:
 *   set    - Pointer ke signal set
 *   signum - Signal number
 *
 * Return: 0 jika berhasil, -1 jika gagal
 */
int sigdelset(sigset_t *set, int signum) {
    if (set == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Validasi signal number */
    if (signum <= 0 || signum >= NSIG) {
        errno = EINVAL;
        return -1;
    }

    /* Clear bit untuk signal ini */
    *set &= ~(1 << (signum - 1));

    return 0;
}

/*
 * sigismember - Cek apakah signal ada dalam signal set
 *
 * Parameter:
 *   set    - Pointer ke signal set
 *   signum - Signal number
 *
 * Return: 1 jika ada, 0 jika tidak, -1 jika error
 */
int sigismember(const sigset_t *set, int signum) {
    if (set == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Validasi signal number */
    if (signum <= 0 || signum >= NSIG) {
        errno = EINVAL;
        return -1;
    }

    /* Cek bit untuk signal ini */
    if (*set & (1 << (signum - 1))) {
        return 1;
    }

    return 0;
}

/* ============================================================
 * FUNGSI SIGNAL HANDLING
 * ============================================================
 */

/*
 * signal - Set signal handler (BSD/ANSI style)
 *
 * Parameter:
 *   signum  - Signal number
 *   handler - Signal handler atau SIG_DFL/SIG_IGN
 *
 * Return: Handler sebelumnya, atau SIG_ERR jika error
 */
void (*signal(int signum, void (*handler)(int)))(int) {
    struct sigaction act;
    struct sigaction oact;
    void (*prev)(int);

    /* Validasi signal number */
    if (signum <= 0 || signum >= NSIG) {
        errno = EINVAL;
        return SIG_ERR;
    }

    /* SIGKILL dan SIGSTOP tidak bisa di-handle */
    if (signum == SIGKILL || signum == SIGSTOP) {
        errno = EINVAL;
        return SIG_ERR;
    }

    /* Inisialisasi jika belum */
    if (!sig_initialized) {
        sig_init();
    }

    /* Setup sigaction structure */
    act.sa_handler = handler;
    act.sa_mask = 0;

    /* Flag untuk BSD-style signal handling */
    act.sa_flags = SA_RESTART;

    /* Tambahkan signal ini ke mask saat handling */
    sigaddset(&act.sa_mask, signum);

    /* Gunakan sigaction untuk implementasi */
    if (sigaction(signum, &act, &oact) < 0) {
        return SIG_ERR;
    }

    /* Simpan handler ke tabel */
    prev = sig_table[signum].sa_handler;
    sig_table[signum].sa_handler = handler;

    return prev;
}

/*
 * sigaction - Set signal action (POSIX style)
 *
 * Parameter:
 *   signum - Signal number
 *   act    - Action baru (boleh NULL)
 *   oact   - Action lama (boleh NULL)
 *
 * Return: 0 jika berhasil, -1 jika gagal
 */
int sigaction(int signum, const struct sigaction *act,
              struct sigaction *oact) {
    long result;

    /* Validasi signal number */
    if (signum <= 0 || signum >= NSIG) {
        errno = EINVAL;
        return -1;
    }

    /* SIGKILL dan SIGSTOP tidak bisa di-handle */
    if (signum == SIGKILL || signum == SIGSTOP) {
        if (act != NULL && act->sa_handler != SIG_DFL) {
            errno = EINVAL;
            return -1;
        }
    }

    /* Inisialisasi jika belum */
    if (!sig_initialized) {
        sig_init();
    }

    /* Simpan old action jika diminta */
    if (oact != NULL) {
        *oact = sig_table[signum];
    }

    /* Jika tidak ada action baru, selesai */
    if (act == NULL) {
        return 0;
    }

    /* Panggil syscall */
    result = __syscall_sigaction(signum, act, NULL);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    /* Update tabel internal */
    sig_table[signum] = *act;

    return 0;
}

/*
 * sigprocmask - Ubah signal mask
 *
 * Parameter:
 *   how  - SIG_BLOCK, SIG_UNBLOCK, atau SIG_SETMASK
 *   set  - Signal set baru (boleh NULL)
 *   oset - Signal set lama (boleh NULL)
 *
 * Return: 0 jika berhasil, -1 jika gagal
 */
int sigprocmask(int how, const sigset_t *set, sigset_t *oset) {
    sigset_t old_mask;
    long result;

    /* Validasi how */
    if (how != SIG_BLOCK && how != SIG_UNBLOCK &&
        how != SIG_SETMASK) {
        errno = EINVAL;
        return -1;
    }

    /* Simpan old mask jika diminta */
    if (oset != NULL) {
        result = __syscall_sigprocmask(0, NULL, &old_mask);
        if (result < 0) {
            errno = -result;
            return -1;
        }
        *oset = old_mask;
    }

    /* Jika tidak ada set baru, selesai */
    if (set == NULL) {
        return 0;
    }

    /* Panggil syscall */
    result = __syscall_sigprocmask(how, set, NULL);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return 0;
}

/*
 * sigpending - Dapatkan signal yang pending
 *
 * Parameter:
 *   set - Pointer untuk menyimpan signal set
 *
 * Return: 0 jika berhasil, -1 jika gagal
 */
int sigpending(sigset_t *set) {
    long result;

    if (set == NULL) {
        errno = EINVAL;
        return -1;
    }

    result = __syscall_sigpending(set);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return 0;
}

/*
 * sigsuspend - Tunggu signal
 *
 * Parameter:
 *   mask - Signal mask baru selama menunggu
 *
 * Return: -1 dengan errno = EINTR
 */
int sigsuspend(const sigset_t *mask) {
    long result;

    if (mask == NULL) {
        errno = EINVAL;
        return -1;
    }

    result = __syscall_sigsuspend(mask);

    /* sigsuspend selalu return -1 dengan EINTR */
    errno = EINTR;
    return -1;
}

/* ============================================================
 * FUNGSI PENGIRIMAN SIGNAL
 * ============================================================
 */

/*
 * raise - Kirim signal ke diri sendiri
 *
 * Parameter:
 *   signum - Signal number
 *
 * Return: 0 jika berhasil, nilai non-zero jika gagal
 */
int raise(int signum) {
    /* Validasi signal number */
    if (signum <= 0 || signum >= NSIG) {
        errno = EINVAL;
        return -1;
    }

    /* Kirim signal ke diri sendiri */
    return kill(getpid(), signum);
}

/*
 * kill - Kirim signal ke process
 *
 * Parameter:
 *   pid    - Process ID
 *   signum - Signal number
 *
 * Return: 0 jika berhasil, -1 jika gagal
 */
int kill(pid_t pid, int signum) {
    long result;

    /* Validasi signal number */
    if (signum < 0 || signum >= NSIG) {
        errno = EINVAL;
        return -1;
    }

    /* Panggil syscall */
    result = __syscall_kill(pid, signum);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return 0;
}

/*
 * killpg - Kirim signal ke process group
 *
 * Parameter:
 *   pgrp   - Process group ID
 *   signum - Signal number
 *
 * Return: 0 jika berhasil, -1 jika gagal
 */
int killpg(pid_t pgrp, int signum) {
    long result;

    /* Validasi signal number */
    if (signum < 0 || signum >= NSIG) {
        errno = EINVAL;
        return -1;
    }

    /* Validasi process group */
    if (pgrp < 0) {
        errno = EINVAL;
        return -1;
    }

    /* Panggil syscall */
    result = __syscall_killpg(pgrp, signum);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return 0;
}

/* ============================================================
 * FUNGSI UTILITAS
 * ============================================================
 */

/*
 * psignal - Cetak pesan error untuk signal
 *
 * Parameter:
 *   signum - Signal number
 *   msg    - Pesan tambahan (boleh NULL)
 */
void psignal(int signum, const char *msg) {
    const char *sig_name;

    /* Validasi signal number */
    if (signum <= 0 || signum >= NSIG) {
        return;
    }

    /* Dapatkan nama signal */
    switch (signum) {
        case SIGHUP:    sig_name = "Hangup"; break;
        case SIGINT:    sig_name = "Interrupt"; break;
        case SIGQUIT:   sig_name = "Quit"; break;
        case SIGILL:    sig_name = "Illegal instruction"; break;
        case SIGTRAP:   sig_name = "Trace trap"; break;
        case SIGABRT:   sig_name = "Abort"; break;
        case SIGBUS:    sig_name = "Bus error"; break;
        case SIGFPE:    sig_name = "Floating exception"; break;
        case SIGKILL:   sig_name = "Killed"; break;
        case SIGUSR1:   sig_name = "User signal 1"; break;
        case SIGSEGV:   sig_name = "Segmentation fault"; break;
        case SIGUSR2:   sig_name = "User signal 2"; break;
        case SIGPIPE:   sig_name = "Broken pipe"; break;
        case SIGALRM:   sig_name = "Alarm clock"; break;
        case SIGTERM:   sig_name = "Terminated"; break;
        case SIGCHLD:   sig_name = "Child status"; break;
        case SIGCONT:   sig_name = "Continued"; break;
        case SIGSTOP:   sig_name = "Stopped"; break;
        case SIGTSTP:   sig_name = "Stopped (tty)"; break;
        case SIGTTIN:   sig_name = "Stopped (tty in)"; break;
        case SIGTTOU:   sig_name = "Stopped (tty out)"; break;
        default:        sig_name = "Unknown signal"; break;
    }

    /* Cetak pesan */
    if (msg != NULL && *msg != '\0') {
        /* write(STDERR_FILENO, msg, strlen(msg)); */
        /* write(STDERR_FILENO, ": ", 2); */
    }
    /* write(STDERR_FILENO, sig_name, strlen(sig_name)); */
    /* write(STDERR_FILENO, "\n", 1); */
}

/*
 * strsignal - Dapatkan deskripsi signal
 *
 * Parameter:
 *   signum - Signal number
 *
 * Return: String deskripsi signal
 */
char *strsignal(int signum) {
    static char unknown_buf[32];

    /* Validasi signal number */
    if (signum <= 0 || signum >= NSIG) {
        return "Unknown signal";
    }

    /* Return deskripsi signal */
    switch (signum) {
        case SIGHUP:    return "Hangup";
        case SIGINT:    return "Interrupt";
        case SIGQUIT:   return "Quit";
        case SIGILL:    return "Illegal instruction";
        case SIGTRAP:   return "Trace trap";
        case SIGABRT:   return "Abort trap";
        case SIGBUS:    return "Bus error";
        case SIGFPE:    return "Floating point exception";
        case SIGKILL:   return "Killed";
        case SIGUSR1:   return "User defined signal 1";
        case SIGSEGV:   return "Segmentation fault";
        case SIGUSR2:   return "User defined signal 2";
        case SIGPIPE:   return "Broken pipe";
        case SIGALRM:   return "Alarm clock";
        case SIGTERM:   return "Terminated";
        case SIGCHLD:   return "Child exited";
        case SIGCONT:   return "Continued";
        case SIGSTOP:   return "Suspended";
        case SIGTSTP:   return "Suspended";
        case SIGTTIN:   return "Suspended (tty input)";
        case SIGTTOU:   return "Suspended (tty output)";
        default: break;
    }

    return "Unknown signal";
}
