/*
 * PIGURA OS - DISPATCHER.C
 * -------------------------
 * Implementasi dispatcher system call.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengarahkan system call
 * ke handler yang sesuai berdasarkan nomor system call.
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

/*
 * ============================================================================
 * DEKLARASI FORWARD FUNGSI SYS_*
 * ============================================================================
 * Fungsi-fungsi ini diimplementasikan di file lain (tabel.c atau proses/)
 */

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
 * FUNGSI HANDLER SYSCALL INDIVIDUAL
 * ============================================================================
 * Catatan: Handler menggunakan tanda64_t untuk kompatibilitas dengan syscall_fn_t
 */

/*
 * sys_read_handler
 * ----------------
 * Handler sys_read.
 */
static tanda64_t sys_read_handler(tanda64_t fd, tanda64_t buf, tanda64_t count,
                                   tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_read((tak_bertanda32_t)fd, (void *)(alamat_ptr_t)buf, (ukuran_t)count);
}

/*
 * sys_write_handler
 * -----------------
 * Handler sys_write.
 */
static tanda64_t sys_write_handler(tanda64_t fd, tanda64_t buf, tanda64_t count,
                                    tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_write((tak_bertanda32_t)fd, (const void *)(alamat_ptr_t)buf, (ukuran_t)count);
}

/*
 * sys_open_handler
 * ----------------
 * Handler sys_open.
 */
static tanda64_t sys_open_handler(tanda64_t path, tanda64_t flags, tanda64_t mode,
                                   tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_open((const char *)(alamat_ptr_t)path, (tak_bertanda32_t)flags, (mode_t)mode);
}

/*
 * sys_close_handler
 * -----------------
 * Handler sys_close.
 */
static tanda64_t sys_close_handler(tanda64_t fd, tanda64_t arg2, tanda64_t arg3,
                                    tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_close((tak_bertanda32_t)fd);
}

/*
 * sys_fork_handler
 * ----------------
 * Handler sys_fork.
 */
static tanda64_t sys_fork_handler(tanda64_t arg1, tanda64_t arg2, tanda64_t arg3,
                                   tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg1; (void)arg2; (void)arg3;
    (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_fork();
}

/*
 * sys_exec_handler
 * ----------------
 * Handler sys_execve.
 */
static tanda64_t sys_exec_handler(tanda64_t path, tanda64_t argv, tanda64_t envp,
                                   tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_execve((const char *)(alamat_ptr_t)path,
                                  (char *const *)(alamat_ptr_t)argv,
                                  (char *const *)(alamat_ptr_t)envp);
}

/*
 * sys_exit_handler
 * ----------------
 * Handler sys_exit.
 */
static tanda64_t sys_exit_handler(tanda64_t status, tanda64_t arg2, tanda64_t arg3,
                                   tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    sys_exit((tanda32_t)status);
    return 0;  /* Never reached */
}

/*
 * sys_getpid_handler
 * ------------------
 * Handler sys_getpid.
 */
static tanda64_t sys_getpid_handler(tanda64_t arg1, tanda64_t arg2, tanda64_t arg3,
                                     tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg1; (void)arg2; (void)arg3;
    (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_getpid();
}

/*
 * sys_getppid_handler
 * -------------------
 * Handler sys_getppid.
 */
static tanda64_t sys_getppid_handler(tanda64_t arg1, tanda64_t arg2, tanda64_t arg3,
                                      tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg1; (void)arg2; (void)arg3;
    (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_getppid();
}

/*
 * sys_brk_handler
 * ---------------
 * Handler sys_brk.
 */
static tanda64_t sys_brk_handler(tanda64_t addr, tanda64_t arg2, tanda64_t arg3,
                                  tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)(alamat_ptr_t)sys_brk((alamat_virtual_t)addr);
}

/*
 * sys_sbrk_handler
 * ----------------
 * Handler sys_sbrk.
 */
static tanda64_t sys_sbrk_handler(tanda64_t increment, tanda64_t arg2, tanda64_t arg3,
                                   tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)(alamat_ptr_t)sys_sbrk((tanda32_t)increment);
}

/*
 * sys_mmap_handler
 * ----------------
 * Handler sys_mmap.
 */
static tanda64_t sys_mmap_handler(tanda64_t addr, tanda64_t len, tanda64_t prot,
                                   tanda64_t flags, tanda64_t fd, tanda64_t offset)
{
    return (tanda64_t)(alamat_ptr_t)sys_mmap((void *)(alamat_ptr_t)addr, (ukuran_t)len,
                                              (tak_bertanda32_t)prot, (tak_bertanda32_t)flags,
                                              (tak_bertanda32_t)fd, (tak_bertanda32_t)offset);
}

/*
 * sys_munmap_handler
 * ------------------
 * Handler sys_munmap.
 */
static tanda64_t sys_munmap_handler(tanda64_t addr, tanda64_t len, tanda64_t arg3,
                                     tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_munmap((void *)(alamat_ptr_t)addr, (ukuran_t)len);
}

/*
 * sys_sleep_handler
 * -----------------
 * Handler sys_sleep.
 */
static tanda64_t sys_sleep_handler(tanda64_t seconds, tanda64_t arg2, tanda64_t arg3,
                                    tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_sleep((tak_bertanda32_t)seconds);
}

/*
 * sys_usleep_handler
 * ------------------
 * Handler sys_usleep.
 */
static tanda64_t sys_usleep_handler(tanda64_t usec, tanda64_t arg2, tanda64_t arg3,
                                     tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_usleep((tak_bertanda32_t)usec);
}

/*
 * sys_getcwd_handler
 * ------------------
 * Handler sys_getcwd.
 */
static tanda64_t sys_getcwd_handler(tanda64_t buf, tanda64_t size, tanda64_t arg3,
                                     tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)(alamat_ptr_t)sys_getcwd((char *)(alamat_ptr_t)buf, (ukuran_t)size);
}

/*
 * sys_chdir_handler
 * -----------------
 * Handler sys_chdir.
 */
static tanda64_t sys_chdir_handler(tanda64_t path, tanda64_t arg2, tanda64_t arg3,
                                    tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_chdir((const char *)(alamat_ptr_t)path);
}

/*
 * sys_mkdir_handler
 * -----------------
 * Handler sys_mkdir.
 */
static tanda64_t sys_mkdir_handler(tanda64_t path, tanda64_t mode, tanda64_t arg3,
                                    tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_mkdir((const char *)(alamat_ptr_t)path, (mode_t)mode);
}

/*
 * sys_rmdir_handler
 * -----------------
 * Handler sys_rmdir.
 */
static tanda64_t sys_rmdir_handler(tanda64_t path, tanda64_t arg2, tanda64_t arg3,
                                    tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_rmdir((const char *)(alamat_ptr_t)path);
}

/*
 * sys_unlink_handler
 * ------------------
 * Handler sys_unlink.
 */
static tanda64_t sys_unlink_handler(tanda64_t path, tanda64_t arg2, tanda64_t arg3,
                                     tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_unlink((const char *)(alamat_ptr_t)path);
}

/*
 * sys_stat_handler
 * ----------------
 * Handler sys_stat.
 */
static tanda64_t sys_stat_handler(tanda64_t path, tanda64_t statbuf, tanda64_t arg3,
                                   tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_stat((const char *)(alamat_ptr_t)path, (void *)(alamat_ptr_t)statbuf);
}

/*
 * sys_fstat_handler
 * -----------------
 * Handler sys_fstat.
 */
static tanda64_t sys_fstat_handler(tanda64_t fd, tanda64_t statbuf, tanda64_t arg3,
                                    tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_fstat((tak_bertanda32_t)fd, (void *)(alamat_ptr_t)statbuf);
}

/*
 * sys_lseek_handler
 * -----------------
 * Handler sys_lseek.
 */
static tanda64_t sys_lseek_handler(tanda64_t fd, tanda64_t offset, tanda64_t whence,
                                    tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_lseek((tak_bertanda32_t)fd, (off_t)offset, (tak_bertanda32_t)whence);
}

/*
 * sys_dup_handler
 * ---------------
 * Handler sys_dup.
 */
static tanda64_t sys_dup_handler(tanda64_t fd, tanda64_t arg2, tanda64_t arg3,
                                  tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_dup((tak_bertanda32_t)fd);
}

/*
 * sys_dup2_handler
 * ----------------
 * Handler sys_dup2.
 */
static tanda64_t sys_dup2_handler(tanda64_t oldfd, tanda64_t newfd, tanda64_t arg3,
                                   tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_dup2((tak_bertanda32_t)oldfd, (tak_bertanda32_t)newfd);
}

/*
 * sys_pipe_handler
 * ----------------
 * Handler sys_pipe.
 */
static tanda64_t sys_pipe_handler(tanda64_t pipefd, tanda64_t arg2, tanda64_t arg3,
                                   tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_pipe((tak_bertanda32_t *)(alamat_ptr_t)pipefd);
}

/*
 * sys_wait_handler
 * ----------------
 * Handler sys_wait.
 */
static tanda64_t sys_wait_handler(tanda64_t status, tanda64_t arg2, tanda64_t arg3,
                                   tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_wait((tanda32_t *)(alamat_ptr_t)status);
}

/*
 * sys_waitpid_handler
 * -------------------
 * Handler sys_waitpid.
 */
static tanda64_t sys_waitpid_handler(tanda64_t pid, tanda64_t status, tanda64_t options,
                                      tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_waitpid((pid_t)pid, (tanda32_t *)(alamat_ptr_t)status, (tak_bertanda32_t)options);
}

/*
 * sys_kill_handler
 * ----------------
 * Handler sys_kill.
 */
static tanda64_t sys_kill_handler(tanda64_t pid, tanda64_t sig, tanda64_t arg3,
                                   tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_kill((pid_t)pid, (tak_bertanda32_t)sig);
}

/*
 * sys_signal_handler
 * ------------------
 * Handler sys_signal.
 */
static tanda64_t sys_signal_handler(tanda64_t sig, tanda64_t handler, tanda64_t arg3,
                                     tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)(alamat_ptr_t)sys_signal((tak_bertanda32_t)sig, (void *)(alamat_ptr_t)handler);
}

/*
 * sys_sigaction_handler
 * ---------------------
 * Handler sys_sigaction.
 */
static tanda64_t sys_sigaction_handler(tanda64_t sig, tanda64_t act, tanda64_t oldact,
                                        tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_sigaction((tak_bertanda32_t)sig,
                                     (const void *)(alamat_ptr_t)act,
                                     (void *)(alamat_ptr_t)oldact);
}

/*
 * sys_time_handler
 * ----------------
 * Handler sys_time.
 */
static tanda64_t sys_time_handler(tanda64_t tloc, tanda64_t arg2, tanda64_t arg3,
                                   tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_time((waktu_t *)(alamat_ptr_t)tloc);
}

/*
 * sys_gettime_handler
 * -------------------
 * Handler sys_gettime.
 */
static tanda64_t sys_gettime_handler(tanda64_t clk_id, tanda64_t tp, tanda64_t arg3,
                                      tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (tanda64_t)sys_clock_gettime((clockid_t)clk_id, (void *)(alamat_ptr_t)tp);
}

/*
 * sys_yield_handler
 * -----------------
 * Handler sys_yield.
 */
static tanda64_t sys_yield_handler(tanda64_t arg1, tanda64_t arg2, tanda64_t arg3,
                                    tanda64_t arg4, tanda64_t arg5, tanda64_t arg6)
{
    (void)arg1; (void)arg2; (void)arg3;
    (void)arg4; (void)arg5; (void)arg6;
    sys_yield();
    return 0;
}

/*
 * ============================================================================
 * TABEL DISPATCHER
 * ============================================================================
 */

/* Struktur entry dispatcher */
typedef struct {
    tak_bertanda32_t syscall_num;   /* Nomor syscall */
    syscall_fn_t handler;           /* Handler */
    const char *nama;               /* Nama syscall */
} syscall_entry_t;

/* Tabel dispatcher */
static syscall_entry_t dispatch_table[] = {
    {SYS_READ,         sys_read_handler,       "read"},
    {SYS_WRITE,        sys_write_handler,      "write"},
    {SYS_OPEN,         sys_open_handler,       "open"},
    {SYS_CLOSE,        sys_close_handler,      "close"},
    {SYS_FORK,         sys_fork_handler,       "fork"},
    {SYS_EXEC,         sys_exec_handler,       "execve"},
    {SYS_EXIT,         sys_exit_handler,       "exit"},
    {SYS_GETPID,       sys_getpid_handler,     "getpid"},
    {SYS_GETPPID,      sys_getppid_handler,    "getppid"},
    {SYS_BRK,          sys_brk_handler,        "brk"},
    {SYS_SBRK,         sys_sbrk_handler,       "sbrk"},
    {SYS_MMAP,         sys_mmap_handler,       "mmap"},
    {SYS_MUNMAP,       sys_munmap_handler,     "munmap"},
    {SYS_SLEEP,        sys_sleep_handler,      "sleep"},
    {SYS_USLEEP,       sys_usleep_handler,     "usleep"},
    {SYS_GETCWD,       sys_getcwd_handler,     "getcwd"},
    {SYS_CHDIR,        sys_chdir_handler,      "chdir"},
    {SYS_MKDIR,        sys_mkdir_handler,      "mkdir"},
    {SYS_RMDIR,        sys_rmdir_handler,      "rmdir"},
    {SYS_UNLINK,       sys_unlink_handler,     "unlink"},
    {SYS_STAT,         sys_stat_handler,       "stat"},
    {SYS_FSTAT,        sys_fstat_handler,      "fstat"},
    {SYS_LSEEK,        sys_lseek_handler,      "lseek"},
    {SYS_DUP,          sys_dup_handler,        "dup"},
    {SYS_DUP2,         sys_dup2_handler,       "dup2"},
    {SYS_PIPE,         sys_pipe_handler,       "pipe"},
    {SYS_WAIT,         sys_wait_handler,       "wait"},
    {SYS_WAITPID,      sys_waitpid_handler,    "waitpid"},
    {SYS_KILL,         sys_kill_handler,       "kill"},
    {SYS_SIGNAL,       sys_signal_handler,     "signal"},
    {SYS_SIGACTION,    sys_sigaction_handler,  "sigaction"},
    {SYS_TIME,         sys_time_handler,       "time"},
    {SYS_GETTIME,      sys_gettime_handler,    "clock_gettime"},
    {SYS_YIELD,        sys_yield_handler,      "yield"}
};

#define DISPATCH_TABLE_SIZE \
    (sizeof(dispatch_table) / sizeof(dispatch_table[0]))

/*
 * ============================================================================
 * FUNGSI PUBLIK
 * ============================================================================
 */

/*
 * syscall_register_handlers
 * -------------------------
 * Register semua handler syscall.
 */
void syscall_register_handlers(void)
{
    tak_bertanda32_t i;
    status_t status;

    kernel_printf("[DISPATCHER] Registering syscall handlers...\n");

    for (i = 0; i < DISPATCH_TABLE_SIZE; i++) {
        status = syscall_register(dispatch_table[i].syscall_num,
                                  dispatch_table[i].handler);
        if (status == STATUS_BERHASIL) {
            kernel_printf("  [%3u] %s registered\n",
                          dispatch_table[i].syscall_num,
                          dispatch_table[i].nama);
        }
    }

    kernel_printf("[DISPATCHER] Registered %u syscall handlers\n",
                  DISPATCH_TABLE_SIZE);
}

/*
 * syscall_get_name
 * ----------------
 * Dapatkan nama syscall.
 *
 * Parameter:
 *   syscall_num - Nomor syscall
 *
 * Return: Nama syscall, atau "unknown"
 */
const char *syscall_get_name(tak_bertanda32_t syscall_num)
{
    tak_bertanda32_t i;

    for (i = 0; i < DISPATCH_TABLE_SIZE; i++) {
        if (dispatch_table[i].syscall_num == syscall_num) {
            return dispatch_table[i].nama;
        }
    }

    return "unknown";
}

/*
 * syscall_print_dispatch_table
 * ----------------------------
 * Print tabel dispatch.
 */
void syscall_print_dispatch_table(void)
{
    tak_bertanda32_t i;

    kernel_printf("\n[DISPATCHER] Tabel Dispatch:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Num   Name        Handler\n");
    kernel_printf("----------------------------------------\n");

    for (i = 0; i < DISPATCH_TABLE_SIZE; i++) {
        kernel_printf("  %3u   %-10s 0x%08lX\n",
                      dispatch_table[i].syscall_num,
                      dispatch_table[i].nama,
                      (tak_bertanda32_t)(alamat_ptr_t)
                      dispatch_table[i].handler);
    }

    kernel_printf("========================================\n");
}
