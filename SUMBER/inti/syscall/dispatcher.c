/*
 * PIGURA OS - DISPATCHER.C
 * -------------------------
 * Implementasi dispatcher system call.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengarahkan system call
 * ke handler yang sesuai berdasarkan nomor system call.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../kernel.h"

/*
 * ============================================================================
 * FUNGSI HANDLER SYSCALL INDIVIDUAL
 * ============================================================================
 */

/*
 * sys_read_handler
 * ----------------
 * Handler sys_read.
 */
static long sys_read_handler(long fd, long buf, long count, long arg4,
                             long arg5, long arg6)
{
    (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_read((int)fd, (void *)buf, (ukuran_t)count);
}

/*
 * sys_write_handler
 * -----------------
 * Handler sys_write.
 */
static long sys_write_handler(long fd, long buf, long count, long arg4,
                              long arg5, long arg6)
{
    (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_write((int)fd, (const void *)buf, (ukuran_t)count);
}

/*
 * sys_open_handler
 * ----------------
 * Handler sys_open.
 */
static long sys_open_handler(long path, long flags, long mode, long arg4,
                             long arg5, long arg6)
{
    (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_open((const char *)path, (int)flags, (mode_t)mode);
}

/*
 * sys_close_handler
 * -----------------
 * Handler sys_close.
 */
static long sys_close_handler(long fd, long arg2, long arg3, long arg4,
                              long arg5, long arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_close((int)fd);
}

/*
 * sys_fork_handler
 * ----------------
 * Handler sys_fork.
 */
static long sys_fork_handler(long arg1, long arg2, long arg3, long arg4,
                             long arg5, long arg6)
{
    (void)arg1; (void)arg2; (void)arg3;
    (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_fork();
}

/*
 * sys_exec_handler
 * ----------------
 * Handler sys_execve.
 */
static long sys_exec_handler(long path, long argv, long envp, long arg4,
                             long arg5, long arg6)
{
    (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_execve((const char *)path, (char *const *)argv,
                            (char *const *)envp);
}

/*
 * sys_exit_handler
 * ----------------
 * Handler sys_exit.
 */
static long sys_exit_handler(long status, long arg2, long arg3, long arg4,
                             long arg5, long arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    sys_exit((int)status);
    return 0;  /* Never reached */
}

/*
 * sys_getpid_handler
 * ------------------
 * Handler sys_getpid.
 */
static long sys_getpid_handler(long arg1, long arg2, long arg3, long arg4,
                               long arg5, long arg6)
{
    (void)arg1; (void)arg2; (void)arg3;
    (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_getpid();
}

/*
 * sys_getppid_handler
 * -------------------
 * Handler sys_getppid.
 */
static long sys_getppid_handler(long arg1, long arg2, long arg3, long arg4,
                                long arg5, long arg6)
{
    (void)arg1; (void)arg2; (void)arg3;
    (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_getppid();
}

/*
 * sys_brk_handler
 * ---------------
 * Handler sys_brk.
 */
static long sys_brk_handler(long addr, long arg2, long arg3, long arg4,
                            long arg5, long arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_brk((alamat_virtual_t)addr);
}

/*
 * sys_sbrk_handler
 * ----------------
 * Handler sys_sbrk.
 */
static long sys_sbrk_handler(long increment, long arg2, long arg3, long arg4,
                             long arg5, long arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_sbrk((tanda32_t)increment);
}

/*
 * sys_mmap_handler
 * ----------------
 * Handler sys_mmap.
 */
static long sys_mmap_handler(long addr, long len, long prot, long flags,
                             long fd, long offset)
{
    return (long)sys_mmap((void *)addr, (ukuran_t)len, (int)prot,
                          (int)flags, (int)fd, (off_t)offset);
}

/*
 * sys_munmap_handler
 * ------------------
 * Handler sys_munmap.
 */
static long sys_munmap_handler(long addr, long len, long arg3, long arg4,
                               long arg5, long arg6)
{
    (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_munmap((void *)addr, (ukuran_t)len);
}

/*
 * sys_sleep_handler
 * -----------------
 * Handler sys_sleep.
 */
static long sys_sleep_handler(long seconds, long arg2, long arg3, long arg4,
                              long arg5, long arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_sleep((tak_bertanda32_t)seconds);
}

/*
 * sys_usleep_handler
 * ------------------
 * Handler sys_usleep.
 */
static long sys_usleep_handler(long usec, long arg2, long arg3, long arg4,
                               long arg5, long arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_usleep((tak_bertanda32_t)usec);
}

/*
 * sys_getcwd_handler
 * ------------------
 * Handler sys_getcwd.
 */
static long sys_getcwd_handler(long buf, long size, long arg3, long arg4,
                               long arg5, long arg6)
{
    (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_getcwd((char *)buf, (ukuran_t)size);
}

/*
 * sys_chdir_handler
 * -----------------
 * Handler sys_chdir.
 */
static long sys_chdir_handler(long path, long arg2, long arg3, long arg4,
                              long arg5, long arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_chdir((const char *)path);
}

/*
 * sys_mkdir_handler
 * -----------------
 * Handler sys_mkdir.
 */
static long sys_mkdir_handler(long path, long mode, long arg3, long arg4,
                              long arg5, long arg6)
{
    (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_mkdir((const char *)path, (mode_t)mode);
}

/*
 * sys_rmdir_handler
 * -----------------
 * Handler sys_rmdir.
 */
static long sys_rmdir_handler(long path, long arg2, long arg3, long arg4,
                              long arg5, long arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_rmdir((const char *)path);
}

/*
 * sys_unlink_handler
 * ------------------
 * Handler sys_unlink.
 */
static long sys_unlink_handler(long path, long arg2, long arg3, long arg4,
                               long arg5, long arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_unlink((const char *)path);
}

/*
 * sys_stat_handler
 * ----------------
 * Handler sys_stat.
 */
static long sys_stat_handler(long path, long statbuf, long arg3, long arg4,
                             long arg5, long arg6)
{
    (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_stat((const char *)path, (struct stat *)statbuf);
}

/*
 * sys_fstat_handler
 * -----------------
 * Handler sys_fstat.
 */
static long sys_fstat_handler(long fd, long statbuf, long arg3, long arg4,
                              long arg5, long arg6)
{
    (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_fstat((int)fd, (struct stat *)statbuf);
}

/*
 * sys_lseek_handler
 * -----------------
 * Handler sys_lseek.
 */
static long sys_lseek_handler(long fd, long offset, long whence, long arg4,
                              long arg5, long arg6)
{
    (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_lseek((int)fd, (off_t)offset, (int)whence);
}

/*
 * sys_dup_handler
 * ---------------
 * Handler sys_dup.
 */
static long sys_dup_handler(long fd, long arg2, long arg3, long arg4,
                            long arg5, long arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_dup((int)fd);
}

/*
 * sys_dup2_handler
 * ----------------
 * Handler sys_dup2.
 */
static long sys_dup2_handler(long oldfd, long newfd, long arg3, long arg4,
                             long arg5, long arg6)
{
    (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_dup2((int)oldfd, (int)newfd);
}

/*
 * sys_pipe_handler
 * ----------------
 * Handler sys_pipe.
 */
static long sys_pipe_handler(long pipefd, long arg2, long arg3, long arg4,
                             long arg5, long arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_pipe((int *)pipefd);
}

/*
 * sys_wait_handler
 * ----------------
 * Handler sys_wait.
 */
static long sys_wait_handler(long status, long arg2, long arg3, long arg4,
                             long arg5, long arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_wait((int *)status);
}

/*
 * sys_waitpid_handler
 * -------------------
 * Handler sys_waitpid.
 */
static long sys_waitpid_handler(long pid, long status, long options, long arg4,
                                long arg5, long arg6)
{
    (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_waitpid((pid_t)pid, (int *)status, (int)options);
}

/*
 * sys_kill_handler
 * ----------------
 * Handler sys_kill.
 */
static long sys_kill_handler(long pid, long sig, long arg3, long arg4,
                             long arg5, long arg6)
{
    (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_kill((pid_t)pid, (int)sig);
}

/*
 * sys_signal_handler
 * ------------------
 * Handler sys_signal.
 */
static long sys_signal_handler(long sig, long handler, long arg3, long arg4,
                               long arg5, long arg6)
{
    (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_signal((int)sig, (sighandler_t)handler);
}

/*
 * sys_sigaction_handler
 * ---------------------
 * Handler sys_sigaction.
 */
static long sys_sigaction_handler(long sig, long act, long oldact, long arg4,
                                  long arg5, long arg6)
{
    (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_sigaction((int)sig, (const struct sigaction *)act,
                               (struct sigaction *)oldact);
}

/*
 * sys_time_handler
 * ----------------
 * Handler sys_time.
 */
static long sys_time_handler(long tloc, long arg2, long arg3, long arg4,
                             long arg5, long arg6)
{
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_time((time_t *)tloc);
}

/*
 * sys_gettime_handler
 * -------------------
 * Handler sys_gettime.
 */
static long sys_gettime_handler(long clk_id, long tp, long arg3, long arg4,
                                long arg5, long arg6)
{
    (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return (long)sys_clock_gettime((clockid_t)clk_id,
                                   (struct timespec *)tp);
}

/*
 * sys_yield_handler
 * -----------------
 * Handler sys_yield.
 */
static long sys_yield_handler(long arg1, long arg2, long arg3, long arg4,
                              long arg5, long arg6)
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
