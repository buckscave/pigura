/*
 * PIGURA LIBC - STDLIB/SYSTEM.C
 * ==============================
 * Implementasi fungsi system() untuk menjalankan perintah shell.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Fungsi yang diimplementasikan:
 *   - system() : Jalankan perintah shell
 *
 * Implementasi menggunakan fork() dan exec() untuk menjalankan
 * shell command, sesuai standar POSIX.
 */

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

/* ============================================================
 * KONFIGURASI
 * ============================================================
 */

/* Default shell path */
#define DEFAULT_SHELL   "/bin/sh"

/* Shell argument */
#define SHELL_ARG       "-c"

/* ============================================================
 * VARIABEL GLOBAL
 * ============================================================
 */

/* Flag untuk menandai ketersediaan shell */
static int shell_available = -1;  /* -1 = belum dicek */

/* ============================================================
 * DEKLARASI FUNGSI EKSTERNAL
 * ============================================================
 */

/* Deklarasi waitpid jika tidak ada di header */
extern int waitpid(int pid, int *status, int options);

/* Deklarasi environ */
extern char **environ;

/* ============================================================
 * FUNGSI PEMBANTU
 * ============================================================
 */

/*
 * check_shell_available - Cek apakah shell tersedia
 *
 * Return: 1 jika shell tersedia, 0 jika tidak
 */
static int check_shell_available(void) {
    int pid;
    int status;

    /* Coba fork untuk melihat apakah shell bisa dijalankan */
    pid = fork();
    if (pid < 0) {
        /* Fork gagal, shell tidak tersedia */
        return 0;
    }

    if (pid == 0) {
        /* Child process - coba exec shell dengan exit 0 */
        char *const argv[] = { (char *)DEFAULT_SHELL,
                               (char *)SHELL_ARG,
                               (char *)"exit 0",
                               NULL };
        char *const envp[] = { NULL };

        /* Coba exec */
        execve(DEFAULT_SHELL, argv, envp);

        /* Jika exec gagal, exit dengan error */
        _exit(127);
    }

    /* Parent - tunggu child */
    if (waitpid(pid, &status, 0) == -1) {
        return 0;
    }

    /* Child berhasil jika exit dengan status 0 */
    if ((status & 0x7F) == 0) {  /* WIFEXITED */
        int exit_status = (status >> 8) & 0xFF;  /* WEXITSTATUS */
        /* Status 0 berarti shell berhasil
         * Status 127 berarti exec gagal (shell tidak ada) */
        return (exit_status == 0) ? 1 : 0;
    }

    return 0;
}

/*
 * block_sigchld - Block SIGCHLD sementara
 *
 * Parameter:
 *   oldset - Pointer untuk menyimpan mask lama
 */
static void block_sigchld(unsigned long *oldset) {
    unsigned long block_set = 0;
    int sigchld = 17;  /* SIGCHLD */

    /* Buat mask dengan SIGCHLD */
    block_set = (1UL << (sigchld - 1));

    sigprocmask(0, (unsigned long *)&block_set, oldset);
}

/*
 * restore_sigmask - Restore signal mask
 *
 * Parameter:
 *   oldset - Mask yang akan di-restore
 */
static void restore_sigmask(unsigned long *oldset) {
    sigprocmask(2, oldset, NULL);  /* SIG_SETMASK = 2 */
}

/* ============================================================
 * IMPLEMENTASI FUNGSI UTAMA
 * ============================================================
 */

/*
 * system - Jalankan perintah shell
 *
 * Parameter:
 *   command - Perintah yang dijalankan (NULL untuk cek shell)
 *
 * Return:
 *   - Jika command NULL: non-zero jika shell tersedia, 0 jika tidak
 *   - Jika command non-NULL:
 *     - -1 jika error (fork gagal, dll)
 *     - Status exit shell jika berhasil (format wait)
 *
 * Catatan:
 *   - Fungsi ini memblock SIGCHLD selama eksekusi
 *   - SIGINT dan SIGQUIT diignore di parent
 *   - Return value sama format dengan wait(), gunakan
 *     WIFEXITED() dan WEXITSTATUS() untuk interpretasi
 */
int system(const char *command) {
    int pid;
    int status;
    unsigned long old_mask;
    int saved_errno;
    void (*old_int)(int);
    void (*old_quit)(int);

    /* Jika command NULL, cek ketersediaan shell */
    if (command == NULL) {
        if (shell_available == -1) {
            shell_available = check_shell_available();
        }
        return shell_available;
    }

    /* Block SIGCHLD */
    block_sigchld(&old_mask);

    /* Ignore SIGINT dan SIGQUIT */
    old_int = signal(2, (void (*)(int))1);   /* SIGINT = 2, SIG_IGN = 1 */
    old_quit = signal(3, (void (*)(int))1);  /* SIGQUIT = 3, SIG_IGN = 1 */

    /* Fork */
    pid = fork();
    if (pid < 0)        /* Fork gagal */
    {
        saved_errno = errno;

        /* Restore signal handlers */
        signal(2, old_int);
        signal(3, old_quit);
        restore_sigmask(&old_mask);

        errno = saved_errno;
        return -1;
    }

    if (pid == 0) {
        /* Child process */

        /* Restore signal handlers ke default */
        signal(2, (void (*)(int))0);   /* SIG_DFL = 0 */
        signal(3, (void (*)(int))0);

        /* Unblock signals */
        restore_sigmask(&old_mask);

        /* Execute shell */
        {
            char *const argv[] = { (char *)DEFAULT_SHELL,
                                   (char *)SHELL_ARG,
                                   (char *)command,
                                   NULL };

            execve(DEFAULT_SHELL, argv, environ);

            /* Jika exec gagal */
            _exit(127);
        }
    }

    /* Parent process - tunggu child */
    while (1) {
        int result = waitpid(pid, &status, 0);

        if (result == pid) {
            /* Child selesai */
            break;
        }

        if (result < 0 && errno != 6) {  /* EINTR = 6 */
            /* Error selain interupsi */
            status = -1;
            break;
        }

        /* EINTR - lanjutkan menunggu */
    }

    /* Simpan errno */
    saved_errno = errno;

    /* Restore signal handlers */
    signal(2, old_int);
    signal(3, old_quit);

    /* Restore signal mask */
    restore_sigmask(&old_mask);

    errno = saved_errno;

    return status;
}

/* ============================================================
 * FUNGSI TAMBAHAN (NON-STANDAR)
 * ============================================================
 */

/*
 * system_r - Versi thread-safe dengan environment custom
 *
 * Parameter:
 *   command - Perintah yang dijalankan
 *   envp    - Environment array
 *
 * Return: Sama seperti system()
 */
int system_r(const char *command, char *const envp[]) {
    int pid;
    int status;
    unsigned long old_mask;

    if (command == NULL) {
        return system(NULL);
    }

    block_sigchld(&old_mask);

    pid = fork();
    if (pid < 0) {
        restore_sigmask(&old_mask);
        return -1;
    }

    if (pid == 0) {
        char *const argv[] = { (char *)DEFAULT_SHELL,
                               (char *)SHELL_ARG,
                               (char *)command,
                               NULL };

        restore_sigmask(&old_mask);
        execve(DEFAULT_SHELL, argv, envp);
        _exit(127);
    }

    while (waitpid(pid, &status, 0) < 0) {
        if (errno != 6) {  /* EINTR */
            status = -1;
            break;
        }
    }

    restore_sigmask(&old_mask);

    return status;
}
