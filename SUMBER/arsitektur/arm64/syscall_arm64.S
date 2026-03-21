# =============================================================================
# PIGURA OS - SYSCALL_ARM64.S
# ===========================
# System call handler untuk ARM64 (AArch64).
#
# Berkas ini berisi fungsi-fungsi assembly untuk menangani
# system call pada arsitektur ARM64.
#
# Arsitektur: ARM64/AArch64 (64-bit)
# Versi: 1.0
# =============================================================================

# =============================================================================
# CONSTANTS
# =============================================================================

/* System call numbers */
.set SYS_EXIT,            0
.set SYS_READ,            1
.set SYS_WRITE,           2
.set SYS_OPEN,            3
.set SYS_CLOSE,           4
.set SYS_FORK,            5
.set SYS_EXEC,            6
.set SYS_WAIT,            7
.set SYS_GETPID,          8
.set SYS_KILL,            9
.set SYS_SLEEP,           10
.set SYS_YIELD,           11
.set SYS_MMAP,            12
.set SYS_MUNMAP,          13
.set SYS_BRK,             14
.set SYS_IOCTL,           15
.set SYS_DUP,             16
.set SYS_DUP2,            17
.set SYS_PIPE,            18
.set SYS_MKDIR,           19
.set SYS_RMDIR,           20
.set SYS_UNLINK,          21
.set SYS_STAT,            22
.set SYS_FSTAT,           23
.set SYS_LSEEK,           24
.set SYS_GETCWD,          25
.set SYS_CHDIR,           26
.set SYS_GETUID,          27
.set SYS_GETGID,          28
.set SYS_SETUID,          29
.set SYS_SETGID,          30
.set SYS_TIME,            31
.set SYS_TIMES,           32
.set SYS_GETTIMEOFDAY,    33
.set SYS_SETTIMEOFDAY,    34
.set SYS_NANOSLEEP,       35

/* Maximum syscall number */
.set SYS_MAX,             128

/* Register mapping for syscalls */
/* x0 = syscall number (input) / return value (output) */
/* x1-x5 = arguments */
/* x8 = syscall number (alternate) */

/* SPSR bits */
.set SPSR_EL0t,           0x0
.set SPSR_D,              (1 << 9)
.set SPSR_A,              (1 << 8)
.set SPSR_I,              (1 << 7)
.set SPSR_F,              (1 << 6)

/* ESR bits */
.set ESR_EC_SHIFT,        26
.set ESR_EC_SVC64,        0x15
.set ESR_ISS_SVC_MASK,    0xFFFF

# =============================================================================
# SECTIONS
# =============================================================================

.section .text
.global syscall_handler
.global syscall_enter
.global syscall_return
.global syscall_table

# =============================================================================
# SYSCALL ENTRY POINT
# =============================================================================

/*
 * syscall_handler
 * ---------------
 * Main syscall handler called from exception vector.
 *
 * Input:
 *   x0-x5 = arguments
 *   x7 or x8 = syscall number
 *   SP = kernel stack
 *   ELR_EL1 = return address
 *   SPSR_EL1 = user PSTATE
 */
syscall_handler:
    /* Save user context */
    stp     x0, x1, [sp, #-16]!
    stp     x2, x3, [sp, #-16]!
    stp     x4, x5, [sp, #-16]!
    stp     x6, x7, [sp, #-16]!
    stp     x8, x9, [sp, #-16]!
    stp     x10, x11, [sp, #-16]!
    stp     x12, x13, [sp, #-16]!
    stp     x14, x15, [sp, #-16]!
    stp     x16, x17, [sp, #-16]!
    stp     x18, x19, [sp, #-16]!
    stp     x20, x21, [sp, #-16]!
    stp     x22, x23, [sp, #-16]!
    stp     x24, x25, [sp, #-16]!
    stp     x26, x27, [sp, #-16]!
    stp     x28, x29, [sp, #-16]!
    str     x30, [sp, #-8]!

    /* Get syscall number from ESR */
    mrs     x8, ESR_EL1
    lsr     x8, x8, #ESR_EC_SHIFT
    cmp     x8, #ESR_EC_SVC64
    b.ne    syscall_invalid

    /* Get immediate value (syscall number) */
    mrs     x8, ESR_EL1
    and     x8, x8, #ESR_ISS_SVC_MASK

    /* Check syscall number */
    cmp     x8, #SYS_MAX
    b.ge    syscall_invalid

    /* Load syscall table entry */
    ldr     x9, =syscall_table
    ldr     x9, [x9, x8, lsl #3]

    /* Check if handler exists */
    cbz     x9, syscall_invalid

    /* Restore arguments */
    ldp     x0, x1, [sp, #216]
    ldp     x2, x3, [sp, #200]
    ldp     x4, x5, [sp, #184]

    /* Call syscall handler */
    blr     x9

    /* Save return value */
    str     x0, [sp, #216]      /* Store at x0 position */

    /* Restore all registers */
    ldr     x30, [sp], #8
    ldp     x28, x29, [sp], #16
    ldp     x26, x27, [sp], #16
    ldp     x24, x25, [sp], #16
    ldp     x22, x23, [sp], #16
    ldp     x20, x21, [sp], #16
    ldp     x18, x19, [sp], #16
    ldp     x16, x17, [sp], #16
    ldp     x14, x15, [sp], #16
    ldp     x12, x13, [sp], #16
    ldp     x10, x11, [sp], #16
    ldp     x8, x9, [sp], #16
    ldp     x6, x7, [sp], #16
    /* x0-x5 restored below */

    /* Return to user */
    ldp     x0, x1, [sp], #16
    ldp     x2, x3, [sp], #16
    ldp     x4, x5, [sp], #16

    eret

syscall_invalid:
    /* Return error */
    mov     x0, #-1
    ldp     x1, x2, [sp], #16
    ldp     x3, x4, [sp], #16
    ldp     x5, x6, [sp], #16
    eret

# =============================================================================
# SYSCALL ENTRY/EXIT FOR USER
# =============================================================================

/*
 * syscall_enter
 * -------------
 * Enter syscall from user mode.
 * Users call this via SVC instruction.
 */
syscall_enter:
    svc     #0
    ret

/*
 * syscall_return
 * --------------
 * Return from syscall with value.
 *
 * Input:
 *   x0 = return value
 */
syscall_return:
    /* Just return - value is already in x0 */
    ret

# =============================================================================
# SYSCALL TABLE
# =============================================================================

.section .data
.align 3

/*
 * syscall_table
 * -------------
 * Table of syscall handlers.
 * Indexed by syscall number.
 */
syscall_table:
    .quad sys_exit              /* 0: exit */
    .quad sys_read              /* 1: read */
    .quad sys_write             /* 2: write */
    .quad sys_open              /* 3: open */
    .quad sys_close             /* 4: close */
    .quad sys_fork              /* 5: fork */
    .quad sys_exec              /* 6: exec */
    .quad sys_wait              /* 7: wait */
    .quad sys_getpid            /* 8: getpid */
    .quad sys_kill              /* 9: kill */
    .quad sys_sleep             /* 10: sleep */
    .quad sys_yield             /* 11: yield */
    .quad sys_mmap              /* 12: mmap */
    .quad sys_munmap            /* 13: munmap */
    .quad sys_brk               /* 14: brk */
    .quad sys_ioctl             /* 15: ioctl */
    .quad sys_dup               /* 16: dup */
    .quad sys_dup2              /* 17: dup2 */
    .quad sys_pipe              /* 18: pipe */
    .quad sys_mkdir             /* 19: mkdir */
    .quad sys_rmdir             /* 20: rmdir */
    .quad sys_unlink            /* 21: unlink */
    .quad sys_stat              /* 22: stat */
    .quad sys_fstat             /* 23: fstat */
    .quad sys_lseek             /* 24: lseek */
    .quad sys_getcwd            /* 25: getcwd */
    .quad sys_chdir             /* 26: chdir */
    .quad sys_getuid            /* 27: getuid */
    .quad sys_getgid            /* 28: getgid */
    .quad sys_setuid            /* 29: setuid */
    .quad sys_setgid            /* 30: setgid */
    .quad sys_time              /* 31: time */
    .quad sys_times             /* 32: times */
    .quad sys_gettimeofday      /* 33: gettimeofday */
    .quad sys_settimeofday      /* 34: settimeofday */
    .quad sys_nanosleep         /* 35: nanosleep */

    /* Padding to SYS_MAX */
    .rept (SYS_MAX - 36)
    .quad 0
    .endr

# =============================================================================
# SYSCALL WRAPPERS
# =============================================================================

.section .text

/* Default syscall implementations */
.weak sys_exit
.weak sys_read
.weak sys_write
.weak sys_open
.weak sys_close
.weak sys_fork
.weak sys_exec
.weak sys_wait
.weak sys_getpid
.weak sys_kill
.weak sys_sleep
.weak sys_yield
.weak sys_mmap
.weak sys_munmap
.weak sys_brk
.weak sys_ioctl
.weak sys_dup
.weak sys_dup2
.weak sys_pipe
.weak sys_mkdir
.weak sys_rmdir
.weak sys_unlink
.weak sys_stat
.weak sys_fstat
.weak sys_lseek
.weak sys_getcwd
.weak sys_chdir
.weak sys_getuid
.weak sys_getgid
.weak sys_setuid
.weak sys_setgid
.weak sys_time
.weak sys_times
.weak sys_gettimeofday
.weak sys_settimeofday
.weak sys_nanosleep

sys_exit:
    /* x0 = exit code */
    /* Call proses_exit */
    b       proses_exit

sys_read:
    mov     x0, #-1
    ret

sys_write:
    /* x0 = fd, x1 = buf, x2 = count */
    /* Call console write for stdout/stderr */
    cmp     x0, #1
    b.eq    1f
    cmp     x0, #2
    b.eq    1f
    mov     x0, #-1
    ret
1:
    /* Write to console */
    /* Simple implementation */
    mov     x0, x2
    ret

sys_open:
    mov     x0, #-1
    ret

sys_close:
    mov     x0, #0
    ret

sys_fork:
    mov     x0, #-1
    ret

sys_exec:
    mov     x0, #-1
    ret

sys_wait:
    mov     x0, #-1
    ret

sys_getpid:
    /* Get current PID */
    bl      proses_get_pid
    ret

sys_kill:
    mov     x0, #-1
    ret

sys_sleep:
    /* x0 = milliseconds */
    bl      proses_sleep
    mov     x0, #0
    ret

sys_yield:
    bl      proses_yield
    mov     x0, #0
    ret

sys_mmap:
    mov     x0, #-1
    ret

sys_munmap:
    mov     x0, #-1
    ret

sys_brk:
    mov     x0, #-1
    ret

sys_ioctl:
    mov     x0, #-1
    ret

sys_dup:
    mov     x0, #-1
    ret

sys_dup2:
    mov     x0, #-1
    ret

sys_pipe:
    mov     x0, #-1
    ret

sys_mkdir:
    mov     x0, #-1
    ret

sys_rmdir:
    mov     x0, #-1
    ret

sys_unlink:
    mov     x0, #-1
    ret

sys_stat:
    mov     x0, #-1
    ret

sys_fstat:
    mov     x0, #-1
    ret

sys_lseek:
    mov     x0, #-1
    ret

sys_getcwd:
    mov     x0, #-1
    ret

sys_chdir:
    mov     x0, #-1
    ret

sys_getuid:
    mov     x0, #0
    ret

sys_getgid:
    mov     x0, #0
    ret

sys_setuid:
    mov     x0, #-1
    ret

sys_setgid:
    mov     x0, #-1
    ret

sys_time:
    /* Return current time */
    bl      hal_timer_get_uptime
    ret

sys_times:
    mov     x0, #-1
    ret

sys_gettimeofday:
    mov     x0, #-1
    ret

sys_settimeofday:
    mov     x0, #-1
    ret

sys_nanosleep:
    mov     x0, #-1
    ret
