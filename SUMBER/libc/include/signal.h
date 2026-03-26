/*
 * PIGURA LIBC - SIGNAL.H
 * =======================
 * Header untuk penanganan sinyal sesuai standar C89 dan POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#ifndef LIBC_SIGNAL_H
#define LIBC_SIGNAL_H

/* ============================================================
 * DEPENDENSI
 * ============================================================
 */
#include <stddef.h>

/* ============================================================
 * TIPE DATA
 * ============================================================
 */

/* Tipe untuk menyimpan signal number */
typedef int sig_atomic_t;

/* Tipe untuk signal handler function */
typedef void (*sighandler_t)(int);

/* Tipe untuk signal set */
typedef unsigned long sigset_t;

/* Tipe untuk PID */
typedef int pid_t;

/* Tipe untuk UID */
typedef unsigned int uid_t;

/* ============================================================
 * KONSTANTA
 * ============================================================
 */

/* Special handler values */
#define SIG_DFL  ((sighandler_t)0)   /* Default handler */
#define SIG_IGN  ((sighandler_t)1)   /* Ignore signal */
#define SIG_ERR  ((sighandler_t)-1)  /* Error return */

/* ============================================================
 * SIGNAL NUMBERS (POSIX)
 * ============================================================
 */

/* Signal standar */
#define SIGHUP      1   /* Hangup */
#define SIGINT      2   /* Interrupt (Ctrl+C) */
#define SIGQUIT     3   /* Quit (Ctrl+\) */
#define SIGILL      4   /* Illegal instruction */
#define SIGTRAP     5   /* Trace trap */
#define SIGABRT     6   /* Abort */
#define SIGIOT      6   /* IOT trap (alias SIGABRT) */
#define SIGBUS      7   /* Bus error */
#define SIGFPE      8   /* Floating point exception */
#define SIGKILL     9   /* Kill (cannot be caught/ignored) */
#define SIGUSR1     10  /* User signal 1 */
#define SIGSEGV     11  /* Segmentation fault */
#define SIGUSR2     12  /* User signal 2 */
#define SIGPIPE     13  /* Broken pipe */
#define SIGALRM     14  /* Alarm clock */
#define SIGTERM     15  /* Termination */
#define SIGSTKFLT   16  /* Stack fault */
#define SIGCHLD     17  /* Child status changed */
#define SIGCLD      17  /* Child status (alias SIGCHLD) */
#define SIGCONT     18  /* Continue (if stopped) */
#define SIGSTOP     19  /* Stop (cannot be caught/ignored) */
#define SIGTSTP     20  /* Stop from tty (Ctrl+Z) */
#define SIGTTIN     21  /* Background tty read */
#define SIGTTOU     22  /* Background tty write */
#define SIGURG      23  /* Urgent condition on socket */
#define SIGXCPU     24  /* CPU time limit exceeded */
#define SIGXFSZ     25  /* File size limit exceeded */
#define SIGVTALRM   26  /* Virtual alarm clock */
#define SIGPROF     27  /* Profiling alarm clock */
#define SIGWINCH    28  /* Window size change */
#define SIGIO       29  /* I/O now possible */
#define SIGPOLL     29  /* Pollable event (alias SIGIO) */
#define SIGPWR      30  /* Power failure */
#define SIGSYS      31  /* Bad system call */
#define SIGUNUSED   31  /* Unused signal (alias SIGSYS) */

/* Signal realtime (jika didukung) */
#define SIGRTMIN    32  /* Realtime signal min */
#define SIGRTMAX    63  /* Realtime signal max */

/* Jumlah total signal */
#ifndef NSIG
#define NSIG        64
#endif
#define _NSIG       64

/* ============================================================
 * BITMASK UNTUK SA_FLAGS
 * ============================================================
 */

#define SA_NOCLDSTOP    0x00000001  /* Jangan kirim SIGCHLD saat stop */
#define SA_NOCLDWAIT    0x00000002  /* Jangan create zombie saat exit */
#define SA_SIGINFO      0x00000004  /* Gunakan sa_sigaction handler */
#define SA_ONSTACK      0x00000008  /* Gunakan alternate stack */
#define SA_RESTART      0x00000010  /* Restart system call setelah signal */
#define SA_NODEFER      0x00000020  /* Jangan block signal saat handling */
#define SA_RESETHAND    0x00000040  /* Reset ke default saat handling */
#define SA_EXPOSE_TAGBITS 0x00000800  /* Expose tag bits */

/* Alias lama */
#define SA_INTERRUPT    0x20000000  /* Interrupt system call */
#define SA_NOMASK       SA_NODEFER  /* Jangan mask signal */
#define SA_ONESHOT      SA_RESETHAND /* Reset handler setelah use */

/* ============================================================
 * BITMASK UNTUK SIGPROCMASK
 * ============================================================
 */

#define SIG_BLOCK       0   /* Tambahkan signal ke mask */
#define SIG_UNBLOCK     1   /* Hapus signal dari mask */
#define SIG_SETMASK     2   /* Set mask ke nilai baru */

/* ============================================================
 * STRUKTUR DATA
 * ============================================================
 */

/* Tipe untuk menyimpan pid dan uid */
#ifndef PID_T_DEFINED
typedef int pid_t;
#define PID_T_DEFINED
#endif

#ifndef UID_T_DEFINED
typedef unsigned int uid_t;
#define UID_T_DEFINED
#endif

/* Struktur siginfo_t untuk SA_SIGINFO */
typedef struct {
    int      si_signo;     /* Signal number */
    int      si_errno;     /* Error number */
    int      si_code;      /* Signal code */
    int      si_trapno;    /* Trap number */
    pid_t    si_pid;       /* Sending process ID */
    uid_t    si_uid;       /* Sending process UID */
    int      si_status;    /* Exit value atau signal */
    void    *si_addr;      /* Memory location yang menyebabkan fault */
    int      si_band;      /* Band event */
    int      si_fd;        /* File descriptor */
    int      si_timerid;   /* Timer ID */
    int      si_overrun;   /* Timer overrun count */
    union {
        int _pad[32 - 3 * sizeof(int) - 2 * sizeof(pid_t)
                 - sizeof(uid_t) - sizeof(void *)];
        /* Data tambahan tergantung si_code */
    } _data;
} siginfo_t;

/* Union untuk sigval */
typedef union sigval {
    int  sival_int;   /* Integer value */
    void *sival_ptr;  /* Pointer value */
} sigval_t;

/* Struktur sigaction */
struct sigaction {
    union {
        void (*sa_handler)(int);                    /* Handler standar */
        void (*sa_sigaction)(int, siginfo_t *, void *); /* Handler extended */
    } __sa_handler;
    sigset_t sa_mask;      /* Signal mask selama handler */
    int      sa_flags;     /* Flags */
    void    (*sa_restorer)(void);  /* Fungsi restorer (obsolete) */
};

/* Macro untuk akses handler */
#define sa_handler   __sa_handler.sa_handler
#define sa_sigaction __sa_handler.sa_sigaction

/* Struktur stack untuk sigaltstack */
typedef struct {
    void  *ss_sp;     /* Stack base */
    int    ss_flags;  /* Flags */
    size_t ss_size;   /* Stack size */
} stack_t;

/* Flags untuk ss_flags */
#define SS_ONSTACK   1   /* Stack sedang digunakan */
#define SS_DISABLE   2   /* Stack disabled */

/* MINSIGSTKSZ minimum signal stack size */
#define MINSIGSTKSZ  2048

/* SIGSTKSZ default signal stack size */
#define SIGSTKSZ     8192

/* ============================================================
 * SI_CODE VALUES
 * ============================================================
 */

/* SI_CODE untuk signal generik */
#define SI_USER      0    /* Dikirim oleh kill, raise, sigsend */
#define SI_KERNEL    0x80 /* Dikirim oleh kernel */
#define SI_QUEUE    -1    /* Dikirim oleh sigqueue */
#define SI_TIMER    -2    /* Dikirim oleh timer */
#define SI_MESGQ    -3    /* Dikirim oleh message queue */
#define SI_ASYNCIO  -4    /* Dikirim oleh AIO completion */
#define SI_SIGIO    -5    /* Dikirim oleh queued SIGIO */
#define SI_TKILL    -6    /* Dikirim oleh tkill/tgkill */
#define SI_DETHREAD -7    /* Dikirim oleh execve */
#define SI_ASYNCNL  -60   /* Dikirim oleh async name lookup */

/* SI_CODE untuk SIGILL */
#define ILL_ILLOPC   1    /* Illegal opcode */
#define ILL_ILLOPN   2    /* Illegal operand */
#define ILL_ILLADR   3    /* Illegal addressing mode */
#define ILL_ILLTRP   4    /* Illegal trap */
#define ILL_PRVOPC   5    /* Privileged opcode */
#define ILL_PRVREG   6    /* Privileged register */
#define ILL_COPROC   7    /* Coprocessor error */
#define ILL_BADSTK   8    /* Internal stack error */
#define ILL_BADIADDR 9    /* Unimplemented instruction address */

/* SI_CODE untuk SIGFPE */
#define FPE_INTDIV   1    /* Integer divide by zero */
#define FPE_INTOVF   2    /* Integer overflow */
#define FPE_FLTDIV   3    /* Floating point divide by zero */
#define FPE_FLTOVF   4    /* Floating point overflow */
#define FPE_FLTUND   5    /* Floating point underflow */
#define FPE_FLTRES   6    /* Floating point inexact result */
#define FPE_FLTINV   7    /* Floating point invalid operation */
#define FPE_FLTSUB   8    /* Subscript out of range */
#define FPE_FLTUNK   14   /* Unknown floating point error */
#define FPE_CONDTRAP 15   /* Trap on condition */

/* SI_CODE untuk SIGSEGV */
#define SEGV_MAPERR  1    /* Address not mapped */
#define SEGV_ACCERR  2    /* Invalid permissions */
#define SEGV_BNDERR  3    /* Bounds check failure */
#define SEGV_PKUERR  4    /* Protection key check failure */
#define SEGV_ACCADI  5    /* ADI not enabled */
#define SEGV_ADIDERR 6    /* ADI not accessible */
#define SEGV_ADIPERR 7    /* ADI precise exception */

/* SI_CODE untuk SIGBUS */
#define BUS_ADRALN   1    /* Invalid address alignment */
#define BUS_ADRERR   2    /* Non-existent physical address */
#define BUS_OBJERR   3    /* Object specific hardware error */
#define BUS_MCEERR_AR 4   /* Hardware memory error action required */
#define BUS_MCEERR_AO 5   /* Hardware memory error action optional */

/* SI_CODE untuk SIGTRAP */
#define TRAP_BRKPT   1    /* Process breakpoint */
#define TRAP_TRACE   2    /* Process trace trap */
#define TRAP_BRANCH  3    /* Branch taken */
#define TRAP_HWBKPT  4    /* Hardware breakpoint/watchpoint */
#define TRAP_UNK     5    /* Unknown trap */

/* SI_CODE untuk SIGCHLD */
#define CLD_EXITED    1   /* Child exited */
#define CLD_KILLED    2   /* Child killed */
#define CLD_DUMPED    3   /* Child dumped core */
#define CLD_TRAPPED   4   /* Traced child trapped */
#define CLD_STOPPED   5   /* Child stopped */
#define CLD_CONTINUED 6   /* Child continued */

/* SI_CODE untuk SIGPOLL */
#define POLL_IN       1   /* Data input available */
#define POLL_OUT      2   /* Output buffers available */
#define POLL_MSG      3   /* Input message available */
#define POLL_ERR      4   /* I/O error */
#define POLL_PRI      5   /* High priority input available */
#define POLL_HUP      6   /* Device disconnected */

/* ============================================================
 * DEKLARASI FUNGSI
 * ============================================================
 */

/*
 * signal - Set signal handler
 *
 * Parameter:
 *   sig     - Signal number
 *   handler - Handler function
 *
 * Return: Handler sebelumnya, atau SIG_ERR jika error
 */
extern sighandler_t signal(int sig, sighandler_t handler);

/*
 * raise - Kirim signal ke proses sendiri
 *
 * Parameter:
 *   sig - Signal number
 *
 * Return: 0 jika berhasil, non-zero jika error
 */
extern int raise(int sig);

/*
 * sigaction - Set signal action (POSIX)
 *
 * Parameter:
 *   sig  - Signal number
 *   act  - Aksi baru (boleh NULL)
 *   oact - Aksi lama (boleh NULL)
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int sigaction(int sig, const struct sigaction *act,
                     struct sigaction *oact);

/*
 * sigprocmask - Ubah signal mask
 *
 * Parameter:
 *   how  - Cara perubahan (SIG_BLOCK, SIG_UNBLOCK, SIG_SETMASK)
 *   set  - Set baru (boleh NULL)
 *   oset - Set lama (boleh NULL)
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int sigprocmask(int how, const sigset_t *set, sigset_t *oset);

/*
 * sigpending - Dapatkan signal pending
 *
 * Parameter:
 *   set - Buffer untuk hasil
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int sigpending(sigset_t *set);

/*
 * sigsuspend - Tunggu signal dengan mask baru
 *
 * Parameter:
 *   mask - Mask sementara
 *
 * Return: -1 dengan errno=EINTR setelah signal
 */
extern int sigsuspend(const sigset_t *mask);

/*
 * sigaltstack - Set/ambil alternate signal stack
 *
 * Parameter:
 *   ss  - Stack baru (boleh NULL)
 *   oss - Stack lama (boleh NULL)
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int sigaltstack(const stack_t *ss, stack_t *oss);

/*
 * kill - Kirim signal ke proses
 *
 * Parameter:
 *   pid - Target process ID
 *   sig - Signal number
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int kill(pid_t pid, int sig);

/*
 * killpg - Kirim signal ke process group
 *
 * Parameter:
 *   pgrp - Process group ID
 *   sig  - Signal number
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int killpg(int pgrp, int sig);

/*
 * pthread_kill - Kirim signal ke thread (stub)
 *
 * Parameter:
 *   thread - Thread ID
 *   sig    - Signal number
 *
 * Return: 0 jika berhasil, error number jika gagal
 */
extern int pthread_kill(unsigned long thread, int sig);

/*
 * sigqueue - Kirim signal dengan data
 *
 * Parameter:
 *   pid  - Target process ID
 *   sig  - Signal number
 *   value - Data yang dikirim
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int sigqueue(pid_t pid, int sig, const sigval_t value);

/*
 * alarm - Set alarm timer
 *
 * Parameter:
 *   seconds - Waktu alarm (0 untuk cancel)
 *
 * Return: Waktu sisa alarm sebelumnya
 */
extern unsigned int alarm(unsigned int seconds);

/*
 * pause - Tunggu signal
 *
 * Return: -1 dengan errno=EINTR
 */
extern int pause(void);

/* ============================================================
 * FUNGSI MANIPULASI SIGSET_T
 * ============================================================
 */

/*
 * sigemptyset - Inisialisasi set kosong
 *
 * Parameter:
 *   set - Signal set
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int sigemptyset(sigset_t *set);

/*
 * sigfillset - Inisialisasi set penuh
 *
 * Parameter:
 *   set - Signal set
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int sigfillset(sigset_t *set);

/*
 * sigaddset - Tambah signal ke set
 *
 * Parameter:
 *   set - Signal set
 *   sig - Signal number
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int sigaddset(sigset_t *set, int sig);

/*
 * sigdelset - Hapus signal dari set
 *
 * Parameter:
 *   set - Signal set
 *   sig - Signal number
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int sigdelset(sigset_t *set, int sig);

/*
 * sigismember - Cek signal dalam set
 *
 * Parameter:
 *   set - Signal set
 *   sig - Signal number
 *
 * Return: 1 jika ada, 0 jika tidak, -1 jika error
 */
extern int sigismember(const sigset_t *set, int sig);

/* ============================================================
 * MAKRO SIGSET
 * ============================================================
 */

/* Makro untuk sigset_t dengan satu unsigned long */
#define __sigmask(sig)    (1UL << ((sig) - 1))
#define __sigword(sig)    (((sig) - 1) / (8 * sizeof(unsigned long)))

#endif /* LIBC_SIGNAL_H */
