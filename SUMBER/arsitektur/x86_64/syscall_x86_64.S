# =============================================================================
# PIGURA OS - SYSCALL x86_64
# =============================================================================
#
# Berkas ini berisi kode assembly untuk system call entry point
# pada arsitektur x86_64 menggunakan instruksi SYSCALL/SYSRET.
#
# Arsitektur: x86_64
# Versi: 1.0
# =============================================================================

# =============================================================================
# KONSTANTA
# =============================================================================

# Segment selectors
.set SELECTOR_KERNEL_CODE,    0x08
.set SELECTOR_KERNEL_DATA,    0x10
.set SELECTOR_USER_CODE,      0x18
.set SELECTOR_USER_DATA,      0x20

# Flag
.set RFLAGS_IF,               0x200

# =============================================================================
# KODE
# =============================================================================

.section .text
.code64

# -----------------------------------------------------------------------------
# _syscall_entry
# ---------------
# Entry point untuk system call via SYSCALL instruction.
#
# Pada x86_64, SYSCALL melakukan:
#   1. Simpan RIP ke RCX
#   2. Simpan RFLAGS ke R11
#   3. Load RIP dari LSTAR MSR
#   4. Load CS dan SS dari STAR MSR
#   5. Clear RFLAGS.IF
#
# Register konvensi syscall (System V AMD64 ABI):
#   RAX = nomor syscall
#   RDI = argumen 1
#   RSI = argumen 2
#   RDX = argumen 3
#   R10 = argumen 4
#   R8  = argumen 5
#   R9  = argumen 6
#   RAX = return value
# -----------------------------------------------------------------------------
.global _syscall_entry
.type _syscall_entry, @function
_syscall_entry:
    # Swap GS untuk akses ke kernel GS base
    swapgs

    # Simpan GS base ke tempat sementara
    movq %gs:0, %rsp
    movq %rsp, %gs:8

    # Load kernel stack
    movq %gs:16, %rsp

    # Simpan context user
    pushq $SELECTOR_USER_DATA        # SS user
    pushq %rcx                        # RIP user (dari SYSCALL)
    pushq %r11                        # RFLAGS user
    pushq $SELECTOR_USER_DATA        # DS user
    pushq %rsp                        # RSP user (salah - perlu fix)

    # Simpan register umum
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # Simpan nomor syscall ke stack untuk debugging
    movq %rax, %rdi                   # Argumen 1 = nomor syscall
    movq %rsi, %rsi                   # Argumen 2
    movq %rdx, %rdx                   # Argumen 3
    movq %r10, %rcx                   # Argumen 4 (R10 -> RCX untuk C calling)
    movq %r8, %r8                     # Argumen 5
    movq %r9, %r9                     # Argumen 6

    # Panggil syscall handler C
    call syscall_handler

    # Simpan return value
    movq %rax, %r12

    # Restore register umum
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax                         # Hapus syscall number yang tersimpan

    # Restore segment dan stack
    addq $8, %rsp                     # Skip RSP user (temporary)
    popq %r11                         # RFLAGS user
    popq %rcx                         # RIP user

    # Swap GS kembali
    swapgs

    # Return ke user mode via SYSRET
    movq %r12, %rax                   # Return value
    sysretq

.size _syscall_entry, . - _syscall_entry

# -----------------------------------------------------------------------------
# _syscall_entry_iret
# -------------------
# Entry point alternatif menggunakan IRETQ.
# Digunakan jika SYSRET tidak memadai.
# -----------------------------------------------------------------------------
.global _syscall_entry_iret
.type _syscall_entry_iret, @function
_syscall_entry_iret:
    # Swap GS
    swapgs

    # Simpan user stack
    movq %rsp, %gs:0

    # Load kernel stack
    movq %gs:16, %rsp

    # Push stack untuk IRETQ
    # IRETQ expects: SS, RSP, RFLAGS, CS, RIP
    pushq $SELECTOR_USER_DATA
    pushq %gs:0                       # User RSP
    pushq %r11                        # User RFLAGS
    pushq $SELECTOR_USER_CODE
    pushq %rcx                        # User RIP

    # Simpan register umum
    pushq %rax
    pushq %rbx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # Setup argument untuk C handler
    movq %rax, %rdi
    movq %rsi, %rsi
    movq %rdx, %rdx
    movq %r10, %rcx
    movq %r8, %r8
    movq %r9, %r9

    # Panggil handler
    call syscall_handler

    # Simpan return value
    movq %rax, %r12

    # Restore register
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rbx
    popq %rax

    # IRETQ akan restore RIP, CS, RFLAGS, RSP, SS
    swapgs
    movq %r12, %rax
    iretq

.size _syscall_entry_iret, . - _syscall_entry_iret

# -----------------------------------------------------------------------------
# syscall_handler
# ---------------
# Handler syscall C (weak symbol untuk override).
# Parameter:
#   RDI = nomor syscall
#   RSI = arg1
#   RDX = arg2
#   RCX = arg3
#   R8  = arg4
#   R9  = arg5
# Return:
#   RAX = return value
# -----------------------------------------------------------------------------
.weak syscall_handler
syscall_handler:
    # Default handler - return -1 (ENOSYS)
    movq $-1, %rax
    ret

# =============================================================================
# DATA
# =============================================================================

.section .bss
.align 16

# Per-CPU data untuk syscall
.globl _syscall_kernel_stack
_syscall_kernel_stack:
    .skip 8192                        # 8 KB kernel stack per CPU

.globl _syscall_user_stack
_syscall_user_stack:
    .skip 8                           # User stack pointer save area

.globl _syscall_kernel_gs
_syscall_kernel_gs:
    .skip 8                           # Kernel GS base save area
