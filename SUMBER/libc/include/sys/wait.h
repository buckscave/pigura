/*
 * PIGURA LIBC - SYS/WAIT.H
 * =========================
 * Header untuk wait functions sesuai standar POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#ifndef LIBC_SYS_WAIT_H
#define LIBC_SYS_WAIT_H

/* ============================================================
 * DEPENDENSI
 * ============================================================
 */
#include <sys/types.h>
#include <signal.h>

/* ============================================================
 * NULL POINTER
 * ============================================================
 */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* ============================================================
 * KONSTANTA UNTUK waitpid() OPTIONS
 * ============================================================
 */

#define WNOHANG     0x00000001  /* Non-blocking, return immediately */
#define WUNTRACED   0x00000002  /* Report stopped children */
#define WCONTINUED  0x00000008  /* Report continued children */
#define WNOWAIT     0x01000000  /* Leave child in waitable state */

/* ============================================================
 * MAKRO UNTUK STATUS ANALYSIS
 * ============================================================
 */

/*
 * WIFEXITED - Cek apakah child exited normally
 *
 * Parameter:
 *   status - Status dari wait()
 *
 * Return: Non-zero jika child exited normally
 */
#define WIFEXITED(status) \
    (((status) & 0x7F) == 0)

/*
 * WEXITSTATUS - Dapatkan exit code
 *
 * Parameter:
 *   status - Status dari wait()
 *
 * Return: Exit code child (hanya valid jika WIFEXITED non-zero)
 */
#define WEXITSTATUS(status) \
    (((status) >> 8) & 0xFF)

/*
 * WIFSIGNALED - Cek apakah child killed by signal
 *
 * Parameter:
 *   status - Status dari wait()
 *
 * Return: Non-zero jika child killed by signal
 */
#define WIFSIGNALED(status) \
    (((status) & 0x7F) != 0 && ((status) & 0x7F) != 0x7F)

/*
 * WTERMSIG - Dapatkan signal number
 *
 * Parameter:
 *   status - Status dari wait()
 *
 * Return: Signal number yang killed child
 */
#define WTERMSIG(status) \
    ((status) & 0x7F)

/*
 * WIFSTOPPED - Cek apakah child stopped
 *
 * Parameter:
 *   status - Status dari wait()
 *
 * Return: Non-zero jika child stopped
 */
#define WIFSTOPPED(status) \
    (((status) & 0xFF) == 0x7F)

/*
 * WSTOPSIG - Dapatkan signal yang stopped child
 *
 * Parameter:
 *   status - Status dari wait()
 *
 * Return: Signal number yang stopped child
 */
#define WSTOPSIG(status) \
    (((status) >> 8) & 0xFF)

/*
 * WIFCONTINUED - Cek apakah child continued
 *
 * Parameter:
 *   status - Status dari wait()
 *
 * Return: Non-zero jika child continued
 */
#define WIFCONTINUED(status) \
    ((status) == 0xFFFF)

/*
 * WCOREDUMP - Cek apakah child produced core dump
 *
 * Parameter:
 *   status - Status dari wait()
 *
 * Return: Non-zero jika core dump produced
 */
#define WCOREDUMP(status) \
    (((status) & 0x80) != 0)

/* ============================================================
 * DEKLARASI FUNGSI
 * ============================================================
 */

/*
 * wait - Tunggu child process berubah state
 *
 * Parameter:
 *   wstatus - Pointer untuk menyimpan status (boleh NULL)
 *
 * Return: PID child yang berubah, atau -1 jika error
 */
extern pid_t wait(int *wstatus);

/*
 * waitpid - Tunggu child process tertentu
 *
 * Parameter:
 *   pid     - PID child atau -1 (any child)
 *   wstatus - Pointer untuk menyimpan status
 *   options - Options (WNOHANG, WUNTRACED, WCONTINUED)
 *
 * Return: PID child yang berubah, 0 jika WNOHANG dan tidak ada
 *         child siap, atau -1 jika error
 */
extern pid_t waitpid(pid_t pid, int *wstatus, int options);

/*
 * waitid - Tunggu child dengan informasi lebih detail
 *
 * Parameter:
 *   idtype   - Tipe ID (P_PID, P_PGID, P_ALL)
 *   id       - ID proses atau group
 *   infop    - Pointer ke siginfo_t untuk hasil
 *   options  - Options
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int waitid(idtype_t idtype, id_t id, siginfo_t *infop,
                  int options);

/* ============================================================
 * ID TYPES UNTUK waitid()
 * ============================================================
 */

#define P_PID       1   /* Tunggu proses dengan PID tertentu */
#define P_PGID      2   /* Tunggu proses dalam group tertentu */
#define P_ALL       3   /* Tunggu semua proses child */
#define P_PIDFD     4   /* Tunggu via PID file descriptor */

/* Options tambahan untuk waitid() */
#define WEXITED     0x00000004  /* Tunggu exited children */
#define WSTOPPED    0x00000008  /* Tunggu stopped children */
#define WCONTINUED  0x00000010  /* Tunggu continued children */
#define WNOWAIT     0x01000000  /* Leave in waitable state */
#define WNOTHREAD   0x20000000  /* Don't wait for other threads' children */

/* ============================================================
 * FUNGSI TAMBAHAN (BSD)
 * ============================================================
 */

/*
 * wait3 - wait dengan resource usage (BSD)
 *
 * Parameter:
 *   wstatus - Pointer untuk menyimpan status
 *   options - Options
 *   rusage  - Pointer untuk menyimpan resource usage
 *
 * Return: PID child, atau -1 jika error
 */
extern pid_t wait3(int *wstatus, int options, struct rusage *rusage);

/*
 * wait4 - wait untuk child tertentu dengan resource usage (BSD)
 *
 * Parameter:
 *   pid     - PID child
 *   wstatus - Pointer untuk menyimpan status
 *   options - Options
 *   rusage  - Pointer untuk menyimpan resource usage
 *
 * Return: PID child, atau -1 jika error
 */
extern pid_t wait4(pid_t pid, int *wstatus, int options,
                   struct rusage *rusage);

/* ============================================================
 * STRUKTUR RUSAGE
 * ============================================================
 */

struct rusage {
    struct timeval ru_utime;    /* User CPU time used */
    struct timeval ru_stime;    /* System CPU time used */
    long           ru_maxrss;   /* Maximum resident set size (KB) */
    long           ru_ixrss;    /* Integral shared memory size */
    long           ru_idrss;    /* Integral unshared data size */
    long           ru_isrss;    /* Integral unshared stack size */
    long           ru_minflt;   /* Page reclaims (soft page faults) */
    long           ru_majflt;   /* Page faults (hard page faults) */
    long           ru_nswap;    /* Swaps */
    long           ru_inblock;  /* Block input operations */
    long           ru_oublock;  /* Block output operations */
    long           ru_msgsnd;   /* IPC messages sent */
    long           ru_msgrcv;   /* IPC messages received */
    long           ru_nsignals; /* Signals received */
    long           ru_nvcsw;    /* Voluntary context switches */
    long           ru_nivcsw;   /* Involuntary context switches */
};

/* Who values untuk getrusage() */
#define RUSAGE_SELF     0   /* Calling process */
#define RUSAGE_CHILDREN -1  /* All children */
#define RUSAGE_THREAD   1   /* Calling thread */

/*
 * getrusage - Dapatkan resource usage
 *
 * Parameter:
 *   who    - RUSAGE_SELF, RUSAGE_CHILDREN, atau RUSAGE_THREAD
 *   rusage - Buffer untuk hasil
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int getrusage(int who, struct rusage *rusage);

#endif /* LIBC_SYS_WAIT_H */
