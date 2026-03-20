/*
 * PIGURA OS - TABEL.C
 * --------------------
 * Implementasi tabel system call.
 *
 * Berkas ini berisi definisi tabel system call yang memetakan
 * nomor syscall ke fungsi handler yang sesuai.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../kernel.h"

/*
 * ============================================================================
 * DEKLARASI FUNGSI HANDLER EKSTERNAL
 * ============================================================================
 */

/* Handler proses */
extern tanda32_t sys_fork(void);
extern tanda32_t sys_execve(const char *path, char *const argv[],
                            char *const envp[]);
extern void sys_exit(tanda32_t status);
extern pid_t sys_getpid(void);
extern pid_t sys_getppid(void);
extern tanda32_t sys_wait(tanda32_t *status);
extern pid_t sys_waitpid(pid_t pid, tanda32_t *status, tak_bertanda32_t opt);

/* Handler memori */
extern void *sys_brk(alamat_virtual_t addr);
extern void *sys_sbrk(tanda32_t increment);
extern void *sys_mmap(void *addr, ukuran_t len, tak_bertanda32_t prot,
                      tak_bertanda32_t flags, tak_bertanda32_t fd,
                      tak_bertanda32_t offset);
extern tanda32_t sys_munmap(void *addr, ukuran_t len);

/* Handler berkas */
extern tak_bertandas_t sys_read(tak_bertanda32_t fd, void *buf,
                                 ukuran_t count);
extern tak_bertandas_t sys_write(tak_bertanda32_t fd, const void *buf,
                                  ukuran_t count);
extern tanda32_t sys_open(const char *path, tak_bertanda32_t flags,
                          mode_t mode);
extern tanda32_t sys_close(tak_bertanda32_t fd);
extern off_t sys_lseek(tak_bertanda32_t fd, off_t offset, tak_bertanda32_t w);
extern tanda32_t sys_stat(const char *path, void *statbuf);
extern tanda32_t sys_fstat(tak_bertanda32_t fd, void *statbuf);
extern tanda32_t sys_mkdir(const char *path, mode_t mode);
extern tanda32_t sys_rmdir(const char *path);
extern tanda32_t sys_unlink(const char *path);
extern tanda32_t sys_rename(const char *oldpath, const char *newpath);
extern tanda32_t sys_chdir(const char *path);
extern char *sys_getcwd(char *buf, ukuran_t size);

/* Handler direktori */
extern void *sys_opendir(const char *path);
extern void *sys_readdir(void *dirp);
extern tanda32_t sys_closedir(void *dirp);

/* Handler duplikasi */
extern tanda32_t sys_dup(tak_bertanda32_t oldfd);
extern tanda32_t sys_dup2(tak_bertanda32_t oldfd, tak_bertanda32_t newfd);
extern tanda32_t sys_pipe(tak_bertanda32_t pipefd[2]);

/* Handler sinyal */
extern tanda32_t sys_kill(pid_t pid, tak_bertanda32_t sig);
extern void *sys_signal(tak_bertanda32_t sig, void *handler);
extern tanda32_t sys_sigaction(tak_bertanda32_t sig, const void *act,
                               void *oldact);
extern tanda32_t sys_sigprocmask(tak_bertanda32_t how, const void *set,
                                 void *oldset);
extern tanda32_t sys_sigpending(void *set);

/* Handler waktu */
extern waktu_t sys_time(waktu_t *tloc);
extern tanda32_t sys_clock_gettime(tak_bertanda32_t clk_id, void *tp);
extern tanda32_t sys_clock_settime(tak_bertanda32_t clk_id, const void *tp);
extern tak_bertanda32_t sys_sleep(tak_bertanda32_t seconds);
extern tak_bertanda32_t sys_usleep(tak_bertanda32_t microseconds);
extern tak_bertanda32_t sys_alarm(tak_bertanda32_t seconds);

/* Handler proses lain */
extern tanda32_t sys_nice(tanda32_t inc);
extern tanda32_t sys_getpriority(tak_bertanda32_t which, tak_bertanda32_t who);
extern tanda32_t sys_setpriority(tak_bertanda32_t which, tak_bertanda32_t who,
                                 tanda32_t prio);
extern tanda32_t sys_sched_yield(void);
extern tanda32_t sys_sched_setparam(pid_t pid, const void *param);
extern tanda32_t sys_sched_getparam(pid_t pid, void *param);

/* Handler user/group */
extern uid_t sys_getuid(void);
extern uid_t sys_geteuid(void);
extern gid_t sys_getgid(void);
extern gid_t sys_getegid(void);
extern tanda32_t sys_setuid(uid_t uid);
extern tanda32_t sys_setgid(gid_t gid);
extern tanda32_t sys_setreuid(uid_t ruid, uid_t euid);
extern tanda32_t sys_setregid(gid_t rgid, gid_t egid);
extern tanda32_t sys_getgroups(tanda32_t size, gid_t list[]);
extern tanda32_t sys_setgroups(tanda32_t size, const gid_t list[]);

/* Handler sistem */
extern tanda32_t sys_reboot(tak_bertanda32_t cmd);
extern tanda32_t sys_sysinfo(void *info);
extern tanda32_t sys_uname(void *buf);
extern tanda32_t sys_sysctl(void *args);

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
    {SYS_FORK,      "fork",     (syscall_fn_t)sys_fork,
     0, ""},
    {SYS_EXEC,      "execve",   (syscall_fn_t)sys_execve,
     3, "spp"},
    {SYS_EXIT,      "exit",     (syscall_fn_t)sys_exit,
     1, "i"},
    {SYS_WAIT,      "wait",     (syscall_fn_t)sys_wait,
     1, "p"},
    {SYS_WAITPID,   "waitpid",  (syscall_fn_t)sys_waitpid,
     3, "ipi"},
    {SYS_GETPID,    "getpid",   (syscall_fn_t)sys_getpid,
     0, ""},
    {SYS_GETPPID,   "getppid",  (syscall_fn_t)sys_getppid,
     0, ""},

    /* Memory management */
    {SYS_BRK,       "brk",      (syscall_fn_t)sys_brk,
     1, "v"},
    {SYS_SBRK,      "sbrk",     (syscall_fn_t)sys_sbrk,
     1, "i"},
    {SYS_MMAP,      "mmap",     (syscall_fn_t)sys_mmap,
     6, "viiiii"},
    {SYS_MUNMAP,    "munmap",   (syscall_fn_t)sys_munmap,
     2, "vz"},

    /* File I/O */
    {SYS_READ,      "read",     (syscall_fn_t)sys_read,
     3, "ipz"},
    {SYS_WRITE,     "write",    (syscall_fn_t)sys_write,
     3, "ipz"},
    {SYS_OPEN,      "open",     (syscall_fn_t)sys_open,
     3, "sii"},
    {SYS_CLOSE,     "close",    (syscall_fn_t)sys_close,
     1, "i"},
    {SYS_LSEEK,     "lseek",    (syscall_fn_t)sys_lseek,
     3, "ili"},
    {SYS_STAT,      "stat",     (syscall_fn_t)sys_stat,
     2, "sp"},
    {SYS_FSTAT,     "fstat",    (syscall_fn_t)sys_fstat,
     2, "ip"},
    {SYS_MKDIR,     "mkdir",    (syscall_fn_t)sys_mkdir,
     2, "si"},
    {SYS_RMDIR,     "rmdir",    (syscall_fn_t)sys_rmdir,
     1, "s"},
    {SYS_UNLINK,    "unlink",   (syscall_fn_t)sys_unlink,
     1, "s"},
    {SYS_GETCWD,    "getcwd",   (syscall_fn_t)sys_getcwd,
     2, "pz"},
    {SYS_CHDIR,     "chdir",    (syscall_fn_t)sys_chdir,
     1, "s"},

    /* File descriptor management */
    {SYS_DUP,       "dup",      (syscall_fn_t)sys_dup,
     1, "i"},
    {SYS_DUP2,      "dup2",     (syscall_fn_t)sys_dup2,
     2, "ii"},
    {SYS_PIPE,      "pipe",     (syscall_fn_t)sys_pipe,
     1, "p"},

    /* Signal handling */
    {SYS_KILL,      "kill",     (syscall_fn_t)sys_kill,
     2, "ii"},
    {SYS_SIGNAL,    "signal",   (syscall_fn_t)sys_signal,
     2, "ip"},
    {SYS_SIGACTION, "sigaction", (syscall_fn_t)sys_sigaction,
     3, "ipp"},

    /* Time */
    {SYS_TIME,      "time",     (syscall_fn_t)sys_time,
     1, "p"},
    {SYS_SLEEP,     "sleep",    (syscall_fn_t)sys_sleep,
     1, "u"},
    {SYS_USLEEP,    "usleep",   (syscall_fn_t)sys_usleep,
     1, "u"},
    {SYS_GETTIME,   "clock_gettime", (syscall_fn_t)sys_clock_gettime,
     2, "ip"},

    /* Scheduler */
    {SYS_YIELD,     "yield",    (syscall_fn_t)sys_sched_yield,
     0, ""},
    {SYS_NICE,      "nice",     (syscall_fn_t)sys_nice,
     1, "i"},

    /* User/Group */
    {SYS_GETUID,    "getuid",   (syscall_fn_t)sys_getuid,
     0, ""},
    {SYS_GETGID,    "getgid",   (syscall_fn_t)sys_getgid,
     0, ""},
    {SYS_SETUID,    "setuid",   (syscall_fn_t)sys_setuid,
     1, "u"},
    {SYS_SETGID,    "setgid",   (syscall_fn_t)sys_setgid,
     1, "u"},

    /* System */
    {SYS_REBOOT,    "reboot",   (syscall_fn_t)sys_reboot,
     1, "i"},
    {SYS_UNAME,     "uname",    (syscall_fn_t)sys_uname,
     1, "p"}
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
