# =============================================================================
# PIGURA OS - SYSCALL_ARM.S
# ==========================
# Implementasi system call handler untuk ARM (32-bit).
#
# Arsitektur: ARM (ARMv4/v5/v6 - 32-bit)
# Versi: 1.0
# =============================================================================

# =============================================================================
# KONSTANTA
# =============================================================================

# Maximum syscall arguments
.set SYSCALL_MAX_ARGS,  6

# Register mapping untuk syscall
# r0 = syscall number (input) / return value (output)
# r1-r5 = arguments
# r7 = syscall number (alternate)

# CPSR bits
.set CPSR_MODE_SVC,     0x13
.set CPSR_MODE_USR,     0x10
.set CPSR_IRQ_DIS,      (1 << 7)
.set CPSR_FIQ_DIS,      (1 << 6)

# =============================================================================
# SECTIONS
# =============================================================================

.section .text
.align 4

# -----------------------------------------------------------------------------
# _syscall_handler
# ----------------
# Handler untuk SWI exception.
# Dipanggil dari vector table saat SWI instruction dieksekusi.
# -----------------------------------------------------------------------------
.global _syscall_handler
_syscall_handler:
    # Save user context
    push    {r4-r12, lr}

    # Get syscall number dari r7
    mov     r0, r7                          # syscall number

    # Arguments sudah di r1-r5
    # r1 = arg1, r2 = arg2, r3 = arg3
    # r4 = arg4, r5 = arg5, r6 = arg6

    # Call C handler
    bl      _syscall_dispatch

    # Return value sudah di r0

    # Restore context dan return ke user mode
    pop     {r4-r12, pc}^

# -----------------------------------------------------------------------------
# _syscall_enter
# --------------
# Entry point untuk syscall dari user mode.
# Parameter: r7 = syscall number, r0-r5 = arguments
# Returns: r0 = return value
# -----------------------------------------------------------------------------
.global _syscall_enter
_syscall_enter:
    # SWI 0 untuk syscall
    swi     #0
    bx      lr

# -----------------------------------------------------------------------------
# _syscall_call
# -------------
# Panggil syscall dengan nomor tertentu.
# Parameter: r0 = syscall number, r1-r5 = arguments
# Returns: r0 = return value
# -----------------------------------------------------------------------------
.global _syscall_call
_syscall_call:
    # Save syscall number ke r7
    mov     r7, r0

    # Shift arguments
    mov     r0, r1
    mov     r1, r2
    mov     r2, r3
    mov     r3, r4
    mov     r4, r5
    mov     r5, r6

    # Execute syscall
    swi     #0

    bx      lr

# -----------------------------------------------------------------------------
# _syscall_return
# ---------------
# Return dari syscall dengan nilai tertentu.
# Parameter: r0 = return value
# -----------------------------------------------------------------------------
.global _syscall_return
_syscall_return:
    # Return value sudah di r0
    # Pop stack dan return ke user mode
    pop     {r4-r12, pc}^

# -----------------------------------------------------------------------------
# _syscall_get_number
# -------------------
# Dapatkan nomor syscall dari stack frame.
# Parameter: r0 = pointer ke stack frame
# Returns: r0 = syscall number
# -----------------------------------------------------------------------------
.global _syscall_get_number
_syscall_get_number:
    # Syscall number disimpan di r7
    ldr     r0, [r0, #28]                   # r7 offset dalam saved regs
    bx      lr

# -----------------------------------------------------------------------------
# _syscall_get_arg
# ----------------
# Dapatkan argument syscall dari stack frame.
# Parameter: r0 = pointer ke stack frame, r1 = argument index (0-5)
# Returns: r0 = argument value
# -----------------------------------------------------------------------------
.global _syscall_get_arg
_syscall_get_arg:
    cmp     r1, #0
    ldreq   r0, [r0, #0]                    # r0 = arg0
    beq     _syscall_get_arg_done

    cmp     r1, #1
    ldreq   r0, [r0, #4]                    # r1 = arg1
    beq     _syscall_get_arg_done

    cmp     r1, #2
    ldreq   r0, [r0, #8]                    # r2 = arg2
    beq     _syscall_get_arg_done

    cmp     r1, #3
    ldreq   r0, [r0, #12]                   # r3 = arg3
    beq     _syscall_get_arg_done

    cmp     r1, #4
    ldreq   r0, [r0, #16]                   # r4 = arg4
    beq     _syscall_get_arg_done

    cmp     r1, #5
    ldreq   r0, [r0, #20]                   # r5 = arg5
    beq     _syscall_get_arg_done

    mov     r0, #0                          # Invalid argument

_syscall_get_arg_done:
    bx      lr

# -----------------------------------------------------------------------------
# _syscall_set_return
# -------------------
# Set return value di stack frame.
# Parameter: r0 = pointer ke stack frame, r1 = return value
# -----------------------------------------------------------------------------
.global _syscall_set_return
_syscall_set_return:
    str     r1, [r0, #0]                    # r0 = return value
    bx      lr

# =============================================================================
# WRAPPER FUNGSI SYSCALL (SYSCALL WRAPPER FUNCTIONS)
# =============================================================================

.section .text

# -----------------------------------------------------------------------------
# _sys_read
# ---------
# Syscall read wrapper.
# Parameter: r0 = fd, r1 = buffer, r2 = count
# Returns: r0 = bytes read atau error
# -----------------------------------------------------------------------------
.global _sys_read
_sys_read:
    mov     r7, #0                          # SYS_READ
    swi     #0
    bx      lr

# -----------------------------------------------------------------------------
# _sys_write
# ----------
# Syscall write wrapper.
# Parameter: r0 = fd, r1 = buffer, r2 = count
# Returns: r0 = bytes written atau error
# -----------------------------------------------------------------------------
.global _sys_write
_sys_write:
    mov     r7, #1                          # SYS_WRITE
    swi     #0
    bx      lr

# -----------------------------------------------------------------------------
# _sys_exit
# ---------
# Syscall exit wrapper.
# Parameter: r0 = exit code
# Does not return
# -----------------------------------------------------------------------------
.global _sys_exit
_sys_exit:
    mov     r7, #6                          # SYS_EXIT
    swi     #0
    b       .                               # Should not return

# -----------------------------------------------------------------------------
# _sys_getpid
# -----------
# Syscall getpid wrapper.
# Returns: r0 = PID
# -----------------------------------------------------------------------------
.global _sys_getpid
_sys_getpid:
    mov     r7, #7                          # SYS_GETPID
    swi     #0
    bx      lr

# -----------------------------------------------------------------------------
# _sys_fork
# ---------
# Syscall fork wrapper.
# Returns: r0 = 0 di child, PID di parent, atau error
# -----------------------------------------------------------------------------
.global _sys_fork
_sys_fork:
    mov     r7, #4                          # SYS_FORK
    swi     #0
    bx      lr

# -----------------------------------------------------------------------------
# _sys_exec
# ---------
# Syscall exec wrapper.
# Parameter: r0 = path, r1 = argv
# Returns: r0 = error jika gagal
# -----------------------------------------------------------------------------
.global _sys_exec
_sys_exec:
    mov     r7, #5                          # SYS_EXEC
    swi     #0
    bx      lr

# -----------------------------------------------------------------------------
# _sys_yield
# ----------
# Syscall yield wrapper.
# Returns: r0 = 0
# -----------------------------------------------------------------------------
.global _sys_yield
_sys_yield:
    mov     r7, #33                         # SYS_YIELD
    swi     #0
    bx      lr
