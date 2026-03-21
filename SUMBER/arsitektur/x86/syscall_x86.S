# =============================================================================
# PIGURA OS - SYSCALL x86
# =============================================================================
# Berkas ini berisi implementasi system call untuk arsitektur x86.
# System call dipanggil via int 0x80 atau sysenter/sysexit.
#
# Arsitektur: x86 (32-bit)
# Versi: 1.0
# =============================================================================

.section .text
.code32

# =============================================================================
# SYSCALL TABLE
# =============================================================================

.section .data
.global syscall_table
syscall_table:
    .long sys_read          # 0
    .long sys_write         # 1
    .long sys_open          # 2
    .long sys_close         # 3
    .long sys_fork          # 4
    .long sys_exec          # 5
    .long sys_exit          # 6
    .long sys_getpid        # 7
    .long sys_getppid       # 8
    .long sys_brk           # 9
    .long sys_sbrk          # 10
    .long sys_mmap          # 11
    .long sys_munmap        # 12
    .long sys_sleep         # 13
    .long sys_usleep        # 14
    .long sys_getcwd        # 15
    .long sys_chdir         # 16
    .long sys_mkdir         # 17
    .long sys_rmdir         # 18
    .long sys_unlink        # 19
    .long sys_stat          # 20
    .long sys_fstat        # 21
    .long sys_lseek         # 22
    .long sys_dup           # 23
    .long sys_dup2          # 24
    .long sys_pipe          # 25
    .long sys_wait          # 26
    .long sys_waitpid       # 27
    .long sys_kill          # 28
    .long sys_signal        # 29
    .long sys_sigaction     # 30
    .long sys_time          # 31
    .long sys_gettime       # 32
    .long sys_yield         # 33
    .long 0                 # 34-63 reserved

.set SYSCALL_COUNT, 64

# =============================================================================
# SYSCALL HANDLER
# =============================================================================

.section .text

# Syscall handler - dipanggil dari isr_x86.S
.global syscall_handler
syscall_handler:
    # Simpan semua register
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs
    pushal

    # Set data segment ke kernel
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # EAX = syscall number
    # EBX = arg1, ECX = arg2, EDX = arg3
    # ESI = arg4, EDI = arg5, EBP = arg6

    # Cek syscall number
    cmpl $SYSCALL_COUNT, %eax
    jae .syscall_invalid

    # Get syscall handler dari table
    movl syscall_table(,%eax,4), %eax

    # Cek apakah handler valid
    testl %eax, %eax
    jz .syscall_invalid

    # Simpan syscall number
    pushl %eax              # Handler address

    # Setup argumen: handler(arg1, arg2, arg3, arg4, arg5, arg6)
    # Argument sudah di register dari user
    # EBX, ECX, EDX, ESI, EDI, EBP
    # Atau bisa dari stack

    # Panggil handler
    call *%eax

    # Result sudah di EAX
    # Simpan ke stack frame untuk return
    movl %eax, 28(%esp)     # Offset EAX di pushal

    jmp .syscall_done

.syscall_invalid:
    # Return error
    movl $-1, %eax
    movl %eax, 28(%esp)

.syscall_done:
    # Restore register
    popal
    popl %gs
    popl %fs
    popl %es
    popl %ds

    # IRET kembali ke user mode
    addl $8, %esp           # Skip error code dan int_no
    iret

# =============================================================================
# IMPLEMENTASI SYSCALL
# =============================================================================

# sys_read - read(fd, buf, count)
.global sys_read
sys_read:
    # Placeholder - implementasi di C
    movl $-1, %eax
    ret

# sys_write - write(fd, buf, count)
.global sys_write
sys_write:
    # Placeholder - implementasi di C
    movl $-1, %eax
    ret

# sys_open - open(path, flags, mode)
.global sys_open
sys_open:
    # Placeholder
    movl $-1, %eax
    ret

# sys_close - close(fd)
.global sys_close
sys_close:
    # Placeholder
    movl $-1, %eax
    ret

# sys_fork - fork()
.global sys_fork
sys_fork:
    # Placeholder
    movl $-1, %eax
    ret

# sys_exec - exec(path, argv, envp)
.global sys_exec
sys_exec:
    # Placeholder
    movl $-1, %eax
    ret

# sys_exit - exit(status)
.global sys_exit
sys_exit:
    # Placeholder
    movl $0, %eax
    # Tidak return
    jmp .

# sys_getpid - getpid()
.global sys_getpid
sys_getpid:
    # Placeholder
    movl $1, %eax
    ret

# sys_getppid - getppid()
.global sys_getppid
sys_getppid:
    # Placeholder
    movl $0, %eax
    ret

# sys_brk - brk(addr)
.global sys_brk
sys_brk:
    # Placeholder
    movl $-1, %eax
    ret

# sys_sbrk - sbrk(increment)
.global sys_sbrk
sys_sbrk:
    # Placeholder
    movl $-1, %eax
    ret

# sys_mmap - mmap(addr, length, prot, flags, fd, offset)
.global sys_mmap
sys_mmap:
    # Placeholder
    movl $-1, %eax
    ret

# sys_munmap - munmap(addr, length)
.global sys_munmap
sys_munmap:
    # Placeholder
    movl $-1, %eax
    ret

# sys_sleep - sleep(seconds)
.global sys_sleep
sys_sleep:
    # Placeholder
    movl $0, %eax
    ret

# sys_usleep - usleep(usec)
.global sys_usleep
sys_usleep:
    # Placeholder
    movl $0, %eax
    ret

# sys_getcwd - getcwd(buf, size)
.global sys_getcwd
sys_getcwd:
    # Placeholder
    movl $-1, %eax
    ret

# sys_chdir - chdir(path)
.global sys_chdir
sys_chdir:
    # Placeholder
    movl $-1, %eax
    ret

# sys_mkdir - mkdir(path, mode)
.global sys_mkdir
sys_mkdir:
    # Placeholder
    movl $-1, %eax
    ret

# sys_rmdir - rmdir(path)
.global sys_rmdir
sys_rmdir:
    # Placeholder
    movl $-1, %eax
    ret

# sys_unlink - unlink(path)
.global sys_unlink
sys_unlink:
    # Placeholder
    movl $-1, %eax
    ret

# sys_stat - stat(path, statbuf)
.global sys_stat
sys_stat:
    # Placeholder
    movl $-1, %eax
    ret

# sys_fstat - fstat(fd, statbuf)
.global sys_fstat
sys_fstat:
    # Placeholder
    movl $-1, %eax
    ret

# sys_lseek - lseek(fd, offset, whence)
.global sys_lseek
sys_lseek:
    # Placeholder
    movl $-1, %eax
    ret

# sys_dup - dup(oldfd)
.global sys_dup
sys_dup:
    # Placeholder
    movl $-1, %eax
    ret

# sys_dup2 - dup2(oldfd, newfd)
.global sys_dup2
sys_dup2:
    # Placeholder
    movl $-1, %eax
    ret

# sys_pipe - pipe(pipefd)
.global sys_pipe
sys_pipe:
    # Placeholder
    movl $-1, %eax
    ret

# sys_wait - wait(status)
.global sys_wait
sys_wait:
    # Placeholder
    movl $-1, %eax
    ret

# sys_waitpid - waitpid(pid, status, options)
.global sys_waitpid
sys_waitpid:
    # Placeholder
    movl $-1, %eax
    ret

# sys_kill - kill(pid, sig)
.global sys_kill
sys_kill:
    # Placeholder
    movl $-1, %eax
    ret

# sys_signal - signal(sig, handler)
.global sys_signal
sys_signal:
    # Placeholder
    movl $-1, %eax
    ret

# sys_sigaction - sigaction(sig, act, oact)
.global sys_sigaction
sys_sigaction:
    # Placeholder
    movl $-1, %eax
    ret

# sys_time - time(tloc)
.global sys_time
sys_time:
    # Placeholder
    movl $0, %eax
    ret

# sys_gettime - gettime(tp)
.global sys_gettime
sys_gettime:
    # Placeholder
    movl $-1, %eax
    ret

# sys_yield - yield()
.global sys_yield
sys_yield:
    # Placeholder
    movl $0, %eax
    ret
