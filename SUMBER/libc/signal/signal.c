/*
 * PIGURA LIBC - SIGNAL/SIGNAL.C
 * ==============================
 * Implementasi fungsi penanganan sinyal.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 2.0 - Disinkronkan dengan syscall Pigura OS
 *
 * Implementasi untuk Pigura OS dengan dukungan
 * POSIX.1 signal handling.
 */

#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/syscall.h>   /* Syscall numbers dari header terpusat */
#include <unistd.h>

/* ============================================================
 * VARIABEL GLOBAL
 * ============================================================
 */

/* Tabel handler untuk setiap sinyal */
static struct sigaction sig_table[NSIG];

/* Flag inisialisasi */
static int sig_initialized = 0;

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

/* ============================================================
 * FUNGSI MANIPULASI SIGSET
 * ============================================================
 */

/*
 * sigemptyset - Inisialisasi signal set kosong
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

    /* Panggil syscall sigaction - SYS_SIGACTION dari syscall.h */
    result = syscall3(SYS_SIGACTION, (long)signum, (long)act, (long)NULL);

    if (result < 0) {
        errno = (int)(-result);
        return -1;
    }

    /* Update tabel internal */
    sig_table[signum] = *act;

    return 0;
}

/*
 * sigprocmask - Ubah signal mask
 *
 * Catatan: Pigura OS belum mendukung sigprocmask
 */
int sigprocmask(int how, const sigset_t *set, sigset_t *oset) {
    /* Pigura OS: belum implementasi sigprocmask */
    (void)how;
    (void)set;
    (void)oset;
    errno = ENOSYS;
    return -1;
}

/*
 * sigpending - Dapatkan signal yang pending
 *
 * Catatan: Pigura OS belum mendukung sigpending
 */
int sigpending(sigset_t *set) {
    /* Pigura OS: belum implementasi sigpending */
    (void)set;
    errno = ENOSYS;
    return -1;
}

/*
 * sigsuspend - Tunggu signal
 *
 * Catatan: Pigura OS belum mendukung sigsuspend
 */
int sigsuspend(const sigset_t *mask) {
    /* Pigura OS: belum implementasi sigsuspend */
    (void)mask;
    errno = ENOSYS;
    return -1;
}

/* ============================================================
 * FUNGSI PENGIRIMAN SIGNAL
 * ============================================================
 */

/*
 * raise - Kirim signal ke diri sendiri
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
 */
int kill(pid_t pid, int signum) {
    long result;

    /* Validasi signal number */
    if (signum < 0 || signum >= NSIG) {
        errno = EINVAL;
        return -1;
    }

    /* Panggil syscall kill - SYS_KILL dari syscall.h */
    result = syscall2(SYS_KILL, (long)pid, (long)signum);

    if (result < 0) {
        errno = (int)(-result);
        return -1;
    }

    return 0;
}

/*
 * killpg - Kirim signal ke process group
 *
 * Catatan: Pigura OS belum mendukung process groups
 */
int killpg(pid_t pgrp, int signum) {
    /* Pigura OS: belum implementasi process groups */
    (void)pgrp;
    (void)signum;
    errno = ENOSYS;
    return -1;
}

/* ============================================================
 * FUNGSI UTILITAS
 * ============================================================
 */

/*
 * psignal - Cetak pesan error untuk signal
 */
void psignal(int signum, const char *msg) {
    /* Placeholder - perlu stdio untuk output */
    (void)signum;
    (void)msg;
}

/*
 * strsignal - Dapatkan deskripsi signal
 */
char *strsignal(int signum) {
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
