/*
 * PIGURA OS - TABEL.C
 * --------------------
 * Implementasi tabel system call.
 *
 * Berkas ini berisi definisi tabel system call yang memetakan
 * nomor syscall ke fungsi handler yang sesuai.
 *
 * Versi: 1.1
 * Tanggal: 2025
 */

#include "../kernel.h"

/*
 * ============================================================================
 * DEKLARASI FUNGSI EKSTERNAL
 * ============================================================================
 */

/* Deklarasi fungsi syscall_register dari syscall.c */
extern status_t syscall_register(tak_bertanda32_t syscall_num, syscall_fn_t handler);

/* Handler proses */
extern tanda32_t sys_execve(const char *path, char *const argv[],
                            char *const envp[]);
extern void sys_exit(tanda32_t status);
extern pid_t sys_getpid(void);
extern pid_t sys_getppid(void);
extern pid_t sys_waitpid(pid_t pid, tanda32_t *status, tak_bertanda32_t opt);

/* Handler memori */
extern void *sys_brk(alamat_virtual_t addr);
extern void *sys_sbrk(tanda32_t increment);
extern void *sys_mmap(void *addr, ukuran_t len, tak_bertanda32_t prot,
                      tak_bertanda32_t flags, tak_bertanda32_t fd,
                      tak_bertanda32_t offset);
extern tanda32_t sys_munmap(void *addr, ukuran_t len);

/* Handler berkas */
extern tak_bertandas_t sys_read(tak_bertanda32_t fd, void *buf, ukuran_t count);
extern tak_bertandas_t sys_write(tak_bertanda32_t fd, const void *buf, ukuran_t count);
extern tanda32_t sys_open(const char *path, tak_bertanda32_t flags, mode_t mode);
extern tanda32_t sys_close(tak_bertanda32_t fd);
extern off_t sys_lseek(tak_bertanda32_t fd, off_t offset, tak_bertanda32_t whence);
extern tanda32_t sys_stat(const char *path, void *statbuf);
extern tanda32_t sys_fstat(tak_bertanda32_t fd, void *statbuf);
extern tanda32_t sys_mkdir(const char *path, mode_t mode);
extern tanda32_t sys_rmdir(const char *path);
extern tanda32_t sys_unlink(const char *path);
extern tanda32_t sys_chdir(const char *path);
extern char *sys_getcwd(char *buf, ukuran_t size);

/* Handler duplikasi */
extern tanda32_t sys_dup(tak_bertanda32_t oldfd);
extern tanda32_t sys_dup2(tak_bertanda32_t oldfd, tak_bertanda32_t newfd);
extern tanda32_t sys_pipe(tak_bertanda32_t pipefd[2]);

/* Handler sinyal */
extern tanda32_t sys_kill(pid_t pid, tak_bertanda32_t sig);
extern void *sys_signal(tak_bertanda32_t sig, void *handler);
extern tanda32_t sys_sigaction(tak_bertanda32_t sig, const void *act, void *oldact);

/* Handler waktu */
extern waktu_t sys_time(waktu_t *tloc);
extern tanda32_t sys_clock_gettime(tak_bertanda32_t clk_id, void *tp);
extern tak_bertanda32_t sys_sleep(tak_bertanda32_t seconds);
extern tak_bertanda32_t sys_usleep(tak_bertanda32_t microseconds);

/* Handler scheduler */
extern void sys_yield(void);

/*
 * ============================================================================
 * WRAPPER FUNCTIONS
 * ============================================================================
 * Wrapper untuk menyesuaikan signature fungsi sys_* dengan syscall_fn_t
 */

/* Wrapper untuk sys_fork */
static tanda64_t wrap_fork(tanda64_t a1, tanda64_t a2, tanda64_t a3,
                           tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_fork();
}

/* Wrapper untuk sys_execve */
static tanda64_t wrap_execve(tanda64_t path, tanda64_t argv, tanda64_t envp,
                              tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_execve((const char *)(alamat_ptr_t)path,
                                  (char *const *)(alamat_ptr_t)argv,
                                  (char *const *)(alamat_ptr_t)envp);
}

/* Wrapper untuk sys_exit */
static tanda64_t wrap_exit(tanda64_t status, tanda64_t a2, tanda64_t a3,
                           tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    sys_exit((tanda32_t)status);
    return 0;
}

/* Wrapper untuk sys_getpid */
static tanda64_t wrap_getpid(tanda64_t a1, tanda64_t a2, tanda64_t a3,
                              tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_getpid();
}

/* Wrapper untuk sys_getppid */
static tanda64_t wrap_getppid(tanda64_t a1, tanda64_t a2, tanda64_t a3,
                               tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_getppid();
}

/* Wrapper untuk sys_wait */
static tanda64_t wrap_wait(tanda64_t status, tanda64_t a2, tanda64_t a3,
                           tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_wait((tanda32_t *)(alamat_ptr_t)status);
}

/* Wrapper untuk sys_waitpid */
static tanda64_t wrap_waitpid(tanda64_t pid, tanda64_t status, tanda64_t options,
                               tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_waitpid((pid_t)pid, (tanda32_t *)(alamat_ptr_t)status,
                                   (tak_bertanda32_t)options);
}

/* Wrapper untuk sys_brk */
static tanda64_t wrap_brk(tanda64_t addr, tanda64_t a2, tanda64_t a3,
                           tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)(alamat_ptr_t)sys_brk((alamat_virtual_t)addr);
}

/* Wrapper untuk sys_sbrk */
static tanda64_t wrap_sbrk(tanda64_t increment, tanda64_t a2, tanda64_t a3,
                            tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)(alamat_ptr_t)sys_sbrk((tanda32_t)increment);
}

/* Wrapper untuk sys_mmap */
static tanda64_t wrap_mmap(tanda64_t addr, tanda64_t len, tanda64_t prot,
                            tanda64_t flags, tanda64_t fd, tanda64_t offset)
{
    return (tanda64_t)(alamat_ptr_t)sys_mmap((void *)(alamat_ptr_t)addr, (ukuran_t)len,
                                              (tak_bertanda32_t)prot, (tak_bertanda32_t)flags,
                                              (tak_bertanda32_t)fd, (tak_bertanda32_t)offset);
}

/* Wrapper untuk sys_munmap */
static tanda64_t wrap_munmap(tanda64_t addr, tanda64_t len, tanda64_t a3,
                              tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_munmap((void *)(alamat_ptr_t)addr, (ukuran_t)len);
}

/* Wrapper untuk sys_read */
static tanda64_t wrap_read(tanda64_t fd, tanda64_t buf, tanda64_t count,
                            tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_read((tak_bertanda32_t)fd, (void *)(alamat_ptr_t)buf, (ukuran_t)count);
}

/* Wrapper untuk sys_write */
static tanda64_t wrap_write(tanda64_t fd, tanda64_t buf, tanda64_t count,
                             tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_write((tak_bertanda32_t)fd, (const void *)(alamat_ptr_t)buf, (ukuran_t)count);
}

/* Wrapper untuk sys_open */
static tanda64_t wrap_open(tanda64_t path, tanda64_t flags, tanda64_t mode,
                            tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_open((const char *)(alamat_ptr_t)path,
                                (tak_bertanda32_t)flags, (mode_t)mode);
}

/* Wrapper untuk sys_close */
static tanda64_t wrap_close(tanda64_t fd, tanda64_t a2, tanda64_t a3,
                             tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_close((tak_bertanda32_t)fd);
}

/* Wrapper untuk sys_lseek */
static tanda64_t wrap_lseek(tanda64_t fd, tanda64_t offset, tanda64_t whence,
                             tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_lseek((tak_bertanda32_t)fd, (off_t)offset, (tak_bertanda32_t)whence);
}

/* Wrapper untuk sys_stat */
static tanda64_t wrap_stat(tanda64_t path, tanda64_t statbuf, tanda64_t a3,
                            tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_stat((const char *)(alamat_ptr_t)path, (void *)(alamat_ptr_t)statbuf);
}

/* Wrapper untuk sys_fstat */
static tanda64_t wrap_fstat(tanda64_t fd, tanda64_t statbuf, tanda64_t a3,
                             tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_fstat((tak_bertanda32_t)fd, (void *)(alamat_ptr_t)statbuf);
}

/* Wrapper untuk sys_mkdir */
static tanda64_t wrap_mkdir(tanda64_t path, tanda64_t mode, tanda64_t a3,
                             tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_mkdir((const char *)(alamat_ptr_t)path, (mode_t)mode);
}

/* Wrapper untuk sys_rmdir */
static tanda64_t wrap_rmdir(tanda64_t path, tanda64_t a2, tanda64_t a3,
                             tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_rmdir((const char *)(alamat_ptr_t)path);
}

/* Wrapper untuk sys_unlink */
static tanda64_t wrap_unlink(tanda64_t path, tanda64_t a2, tanda64_t a3,
                              tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_unlink((const char *)(alamat_ptr_t)path);
}

/* Wrapper untuk sys_chdir */
static tanda64_t wrap_chdir(tanda64_t path, tanda64_t a2, tanda64_t a3,
                             tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_chdir((const char *)(alamat_ptr_t)path);
}

/* Wrapper untuk sys_getcwd */
static tanda64_t wrap_getcwd(tanda64_t buf, tanda64_t size, tanda64_t a3,
                              tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)(alamat_ptr_t)sys_getcwd((char *)(alamat_ptr_t)buf, (ukuran_t)size);
}

/* Wrapper untuk sys_dup */
static tanda64_t wrap_dup(tanda64_t fd, tanda64_t a2, tanda64_t a3,
                           tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_dup((tak_bertanda32_t)fd);
}

/* Wrapper untuk sys_dup2 */
static tanda64_t wrap_dup2(tanda64_t oldfd, tanda64_t newfd, tanda64_t a3,
                            tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_dup2((tak_bertanda32_t)oldfd, (tak_bertanda32_t)newfd);
}

/* Wrapper untuk sys_pipe */
static tanda64_t wrap_pipe(tanda64_t pipefd, tanda64_t a2, tanda64_t a3,
                            tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_pipe((tak_bertanda32_t *)(alamat_ptr_t)pipefd);
}

/* Wrapper untuk sys_kill */
static tanda64_t wrap_kill(tanda64_t pid, tanda64_t sig, tanda64_t a3,
                            tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_kill((pid_t)pid, (tak_bertanda32_t)sig);
}

/* Wrapper untuk sys_signal */
static tanda64_t wrap_signal(tanda64_t sig, tanda64_t handler, tanda64_t a3,
                              tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)(alamat_ptr_t)sys_signal((tak_bertanda32_t)sig, (void *)(alamat_ptr_t)handler);
}

/* Wrapper untuk sys_sigaction */
static tanda64_t wrap_sigaction(tanda64_t sig, tanda64_t act, tanda64_t oldact,
                                 tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_sigaction((tak_bertanda32_t)sig,
                                     (const void *)(alamat_ptr_t)act,
                                     (void *)(alamat_ptr_t)oldact);
}

/* Wrapper untuk sys_time */
static tanda64_t wrap_time(tanda64_t tloc, tanda64_t a2, tanda64_t a3,
                            tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_time((waktu_t *)(alamat_ptr_t)tloc);
}

/* Wrapper untuk sys_sleep */
static tanda64_t wrap_sleep(tanda64_t seconds, tanda64_t a2, tanda64_t a3,
                             tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_sleep((tak_bertanda32_t)seconds);
}

/* Wrapper untuk sys_usleep */
static tanda64_t wrap_usleep(tanda64_t usec, tanda64_t a2, tanda64_t a3,
                              tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_usleep((tak_bertanda32_t)usec);
}

/* Wrapper untuk sys_clock_gettime */
static tanda64_t wrap_clock_gettime(tanda64_t clk_id, tanda64_t tp, tanda64_t a3,
                                     tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a3; (void)a4; (void)a5; (void)a6;
    return (tanda64_t)sys_clock_gettime((clockid_t)clk_id, (void *)(alamat_ptr_t)tp);
}

/* Wrapper untuk sys_yield */
static tanda64_t wrap_yield(tanda64_t a1, tanda64_t a2, tanda64_t a3,
                             tanda64_t a4, tanda64_t a5, tanda64_t a6)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    sys_yield();
    return 0;
}

/*
 * ============================================================================
 * STRUKTUR TABEL SYSCALL
 * ============================================================================
 */

/* Struktur entry tabel syscall */
typedef struct {
    tak_bertanda32_t nomor;         /* Nomor syscall */
    const char *nama;               /* Nama syscall */
    syscall_fn_t handler;           /* Fungsi handler */
    tak_bertanda32_t arg_count;     /* Jumlah argumen */
    const char *arg_types;          /* String tipe argumen */
} syscall_table_entry_t;

/*
 * ============================================================================
 * DEFINISI TABEL SYSCALL
 * ============================================================================
 */

/* Tabel syscall lengkap */
static syscall_table_entry_t syscall_table[] = {
    /* Proses management */
    {SYS_FORK,      "fork",     wrap_fork,      0, ""},
    {SYS_EXEC,      "execve",   wrap_execve,    3, "spp"},
    {SYS_EXIT,      "exit",     wrap_exit,      1, "i"},
    {SYS_WAIT,      "wait",     wrap_wait,      1, "p"},
    {SYS_WAITPID,   "waitpid",  wrap_waitpid,   3, "ipi"},
    {SYS_GETPID,    "getpid",   wrap_getpid,    0, ""},
    {SYS_GETPPID,   "getppid",  wrap_getppid,   0, ""},

    /* Memory management */
    {SYS_BRK,       "brk",      wrap_brk,       1, "v"},
    {SYS_SBRK,      "sbrk",     wrap_sbrk,      1, "i"},
    {SYS_MMAP,      "mmap",     wrap_mmap,      6, "viiiii"},
    {SYS_MUNMAP,    "munmap",   wrap_munmap,    2, "vz"},

    /* File I/O */
    {SYS_READ,      "read",     wrap_read,      3, "ipz"},
    {SYS_WRITE,     "write",    wrap_write,     3, "ipz"},
    {SYS_OPEN,      "open",     wrap_open,      3, "sii"},
    {SYS_CLOSE,     "close",    wrap_close,     1, "i"},
    {SYS_LSEEK,     "lseek",    wrap_lseek,     3, "ili"},
    {SYS_STAT,      "stat",     wrap_stat,      2, "sp"},
    {SYS_FSTAT,     "fstat",    wrap_fstat,     2, "ip"},
    {SYS_MKDIR,     "mkdir",    wrap_mkdir,     2, "si"},
    {SYS_RMDIR,     "rmdir",    wrap_rmdir,     1, "s"},
    {SYS_UNLINK,    "unlink",   wrap_unlink,    1, "s"},
    {SYS_GETCWD,    "getcwd",   wrap_getcwd,    2, "pz"},
    {SYS_CHDIR,     "chdir",    wrap_chdir,     1, "s"},

    /* File descriptor management */
    {SYS_DUP,       "dup",      wrap_dup,       1, "i"},
    {SYS_DUP2,      "dup2",     wrap_dup2,      2, "ii"},
    {SYS_PIPE,      "pipe",     wrap_pipe,      1, "p"},

    /* Signal handling */
    {SYS_KILL,      "kill",     wrap_kill,      2, "ii"},
    {SYS_SIGNAL,    "signal",   wrap_signal,    2, "ip"},
    {SYS_SIGACTION, "sigaction", wrap_sigaction, 3, "ipp"},

    /* Time */
    {SYS_TIME,      "time",     wrap_time,      1, "p"},
    {SYS_SLEEP,     "sleep",    wrap_sleep,     1, "u"},
    {SYS_USLEEP,    "usleep",   wrap_usleep,    1, "u"},
    {SYS_GETTIME,   "clock_gettime", wrap_clock_gettime, 2, "ip"},

    /* Scheduler */
    {SYS_YIELD,     "yield",    wrap_yield,     0, ""}
};

/* Ukuran tabel */
#define SYSCALL_TABLE_SIZE \
    (sizeof(syscall_table) / sizeof(syscall_table[0]))

/*
 * ============================================================================
 * FUNGSI PUBLIK
 * ============================================================================
 */

/*
 * syscall_table_init
 * ------------------
 * Inisialisasi tabel syscall.
 *
 * Return: Status operasi
 */
status_t syscall_table_init(void)
{
    tak_bertanda32_t i;
    status_t status;

    kernel_printf("[SYSCALL_TABLE] Initializing syscall table...\n");

    for (i = 0; i < SYSCALL_TABLE_SIZE; i++) {
        status = syscall_register(syscall_table[i].nomor,
                                  syscall_table[i].handler);

        if (status != STATUS_BERHASIL) {
            kernel_printf("  Warning: Failed to register syscall %u\n",
                          syscall_table[i].nomor);
        }
    }

    kernel_printf("[SYSCALL_TABLE] Initialized %u syscalls\n",
                  SYSCALL_TABLE_SIZE);

    return STATUS_BERHASIL;
}

/*
 * syscall_table_get_entry
 * -----------------------
 * Dapatkan entry tabel syscall.
 *
 * Parameter:
 *   nomor - Nomor syscall
 *
 * Return: Pointer ke entry, atau NULL
 */
const syscall_table_entry_t *syscall_table_get_entry(tak_bertanda32_t nomor)
{
    tak_bertanda32_t i;

    for (i = 0; i < SYSCALL_TABLE_SIZE; i++) {
        if (syscall_table[i].nomor == nomor) {
            return &syscall_table[i];
        }
    }

    return NULL;
}

/*
 * syscall_table_get_handler
 * -------------------------
 * Dapatkan handler syscall.
 *
 * Parameter:
 *   nomor - Nomor syscall
 *
 * Return: Fungsi handler, atau NULL
 */
syscall_fn_t syscall_table_get_handler(tak_bertanda32_t nomor)
{
    const syscall_table_entry_t *entry;

    entry = syscall_table_get_entry(nomor);
    if (entry != NULL) {
        return entry->handler;
    }

    return NULL;
}

/*
 * syscall_table_get_name
 * ----------------------
 * Dapatkan nama syscall.
 *
 * Parameter:
 *   nomor - Nomor syscall
 *
 * Return: Nama syscall, atau "unknown"
 */
const char *syscall_table_get_name(tak_bertanda32_t nomor)
{
    const syscall_table_entry_t *entry;

    entry = syscall_table_get_entry(nomor);
    if (entry != NULL) {
        return entry->nama;
    }

    return "unknown";
}

/*
 * syscall_table_get_arg_count
 * ---------------------------
 * Dapatkan jumlah argumen syscall.
 *
 * Parameter:
 *   nomor - Nomor syscall
 *
 * Return: Jumlah argumen, atau 0
 */
tak_bertanda32_t syscall_table_get_arg_count(tak_bertanda32_t nomor)
{
    const syscall_table_entry_t *entry;

    entry = syscall_table_get_entry(nomor);
    if (entry != NULL) {
        return entry->arg_count;
    }

    return 0;
}

/*
 * syscall_table_print
 * -------------------
 * Print tabel syscall.
 */
void syscall_table_print(void)
{
    tak_bertanda32_t i;

    kernel_printf("\n[SYSCALL_TABLE] Tabel System Call:\n");
    kernel_printf("========================================");
    kernel_printf("========================================\n");
    kernel_printf("  Num   Name          Args  Types\n");
    kernel_printf("----------------------------------------");
    kernel_printf("----------------------------------------\n");

    for (i = 0; i < SYSCALL_TABLE_SIZE; i++) {
        kernel_printf("  %3u   %-12s  %u     %s\n",
                      syscall_table[i].nomor,
                      syscall_table[i].nama,
                      syscall_table[i].arg_count,
                      syscall_table[i].arg_types);
    }

    kernel_printf("========================================");
    kernel_printf("========================================\n");
    kernel_printf("  Total: %u syscalls\n", SYSCALL_TABLE_SIZE);
}

/*
 * syscall_table_count
 * -------------------
 * Dapatkan jumlah syscall dalam tabel.
 *
 * Return: Jumlah syscall
 */
tak_bertanda32_t syscall_table_count(void)
{
    return SYSCALL_TABLE_SIZE;
}

/*
 * syscall_table_iterate
 * ---------------------
 * Iterasi tabel syscall dengan callback.
 *
 * Parameter:
 *   callback - Fungsi callback
 *   data     - Data tambahan untuk callback
 */
void syscall_table_iterate(void (*callback)(const syscall_table_entry_t *,
                                            void *), void *data)
{
    tak_bertanda32_t i;

    if (callback == NULL) {
        return;
    }

    for (i = 0; i < SYSCALL_TABLE_SIZE; i++) {
        callback(&syscall_table[i], data);
    }
}
