# =============================================================================
# PIGURA OS - RING SWITCH x86_64
# =============================================================================
#
# Berkas ini berisi kode assembly untuk switching antar privilege level
# pada arsitektur x86_64.
#
# Arsitektur: x86_64
# Versi: 1.0
# =============================================================================

.section .text
.code64

# -----------------------------------------------------------------------------
# _switch_to_ring3
# ----------------
# Beralih dari Ring 0 ke Ring 3 (user mode).
#
# Parameter:
#   RDI = entry point (RIP user)
#   RSI = stack pointer (RSP user)
# -----------------------------------------------------------------------------
.global _switch_to_ring3
.type _switch_to_ring3, @function
_switch_to_ring3:
    # Simpan parameter
    movq %rdi, %rax                   # Entry point
    movq %rsi, %rdx                   # Stack pointer

    # Load user data segment
    movw $0x23, %cx                   # User data (0x20 | RPL 3)
    movw %cx, %ds
    movw %cx, %es
    movw %cx, %fs
    movw %cx, %gs

    # Setup stack untuk IRETQ
    # Stack layout untuk IRETQ: SS, RSP, RFLAGS, CS, RIP
    pushq $0x23                       # SS
    pushq %rdx                        # RSP
    pushfq                             # RFLAGS
    orq $0x200, (%rsp)                # Enable interrupts
    pushq $0x1B                       # CS (User code | RPL 3)
    pushq %rax                        # RIP

    # Return ke user mode
    iretq

.size _switch_to_ring3, . - _switch_to_ring3

# -----------------------------------------------------------------------------
# _switch_to_ring0
# ----------------
# Beralih dari Ring 3 ke Ring 0 (kernel mode) via interrupt.
# Ini biasanya terjadi otomatis via syscall/interrupt.
#
# Catatan: Fungsi ini jarang dipanggil langsung karena switching
# ke kernel mode dilakukan via interrupt/syscall mechanism.
# -----------------------------------------------------------------------------
.global _switch_to_ring0
.type _switch_to_ring0, @function
_switch_to_ring0:
    # Load kernel data segment
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    ret

.size _switch_to_ring0, . - _switch_to_ring0

# -----------------------------------------------------------------------------
# _get_cs
# -------
# Mendapatkan nilai CS saat ini.
#
# Return:
#   RAX = CS
# -----------------------------------------------------------------------------
.global _get_cs
.type _get_cs, @function
_get_cs:
    movw %cs, %ax
    movzwq %ax, %rax
    ret

.size _get_cs, . - _get_cs

# -----------------------------------------------------------------------------
# _get_ss
# -------
# Mendapatkan nilai SS saat ini.
#
# Return:
#   RAX = SS
# -----------------------------------------------------------------------------
.global _get_ss
.type _get_ss, @function
_get_ss:
    movw %ss, %ax
    movzwq %ax, %rax
    ret

.size _get_ss, . - _get_ss

# -----------------------------------------------------------------------------
# _get_cpl
# --------
# Mendapatkan Current Privilege Level.
#
# Return:
#   RAX = CPL (0-3)
# -----------------------------------------------------------------------------
.global _get_cpl
.type _get_cpl, @function
_get_cpl:
    movw %cs, %ax
    andw $0x03, %ax                   # CPL adalah 2 bit terbawah CS
    movzwq %ax, %rax
    ret

.size _get_cpl, . - _get_cpl

# -----------------------------------------------------------------------------
# _is_kernel_mode
# ---------------
# Cek apakah sedang di kernel mode.
#
# Return:
#   RAX = 1 jika kernel mode, 0 jika user mode
# -----------------------------------------------------------------------------
.global _is_kernel_mode
.type _is_kernel_mode, @function
_is_kernel_mode:
    movw %cs, %ax
    andw $0x03, %ax
    cmpw $0, %ax
    sete %al
    movzbq %al, %rax
    ret

.size _is_kernel_mode, . - _is_kernel_mode

# -----------------------------------------------------------------------------
# _is_user_mode
# -------------
# Cek apakah sedang di user mode.
#
# Return:
#   RAX = 1 jika user mode, 0 jika kernel mode
# -----------------------------------------------------------------------------
.global _is_user_mode
.type _is_user_mode, @function
_is_user_mode:
    movw %cs, %ax
    andw $0x03, %ax
    cmpw $3, %ax
    sete %al
    movzbq %al, %rax
    ret

.size _is_user_mode, . - _is_user_mode

# -----------------------------------------------------------------------------
# _load_fs_base
# -------------
# Load nilai ke FS_BASE MSR (untuk TLS).
#
# Parameter:
#   RDI = nilai FS base
# -----------------------------------------------------------------------------
.global _load_fs_base
.type _load_fs_base, @function
_load_fs_base:
    movl $0xC0000100, %ecx             # MSR_FS_BASE
    movl %edi, %eax
    shrq $32, %rdi
    movl %edi, %edx
    wrmsr
    ret

.size _load_fs_base, . - _load_fs_base

# -----------------------------------------------------------------------------
# _load_gs_base
# -------------
# Load nilai ke GS_BASE MSR (untuk per-CPU data).
#
# Parameter:
#   RDI = nilai GS base
# -----------------------------------------------------------------------------
.global _load_gs_base
.type _load_gs_base, @function
_load_gs_base:
    movl $0xC0000101, %ecx             # MSR_GS_BASE
    movl %edi, %eax
    shrq $32, %rdi
    movl %edi, %edx
    wrmsr
    ret

.size _load_gs_base, . - _load_gs_base

# -----------------------------------------------------------------------------
# _swapgs
# -------
# Swap GS base register (antara kernel dan user).
# -----------------------------------------------------------------------------
.global _swapgs
.type _swapgs, @function
_swapgs:
    swapgs
    ret

.size _swapgs, . - _swapgs
