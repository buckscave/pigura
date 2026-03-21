# =============================================================================
# PIGURA OS - CONTEXT SWITCH x86_64
# =============================================================================
#
# Berkas ini berisi kode assembly untuk context switching pada x86_64.
#
# Arsitektur: x86_64
# Versi: 1.0
# =============================================================================

.section .text
.code64

# -----------------------------------------------------------------------------
# _context_switch_64
# ------------------
# Melakukan context switch dari proses lama ke proses baru.
#
# Parameter:
#   RDI = pointer ke context lama
#   RSI = pointer ke context baru
#
# Context structure (harus sesuai dengan proses_x86_64.c):
#   offset 0:  r15
#   offset 8:  r14
#   offset 16: r13
#   offset 24: r12
#   offset 32: rbp
#   offset 40: rbx
#   offset 48: rip
#   offset 56: rsp
#   offset 64: rflags
#   offset 72: cr3
#   offset 80: cs
#   offset 82: ds
#   offset 84: es
#   offset 86: fs
#   offset 88: gs
#   offset 90: ss
# -----------------------------------------------------------------------------
.global _context_switch_64
.type _context_switch_64, @function
_context_switch_64:
    # Simpan context lama
    # RDI = pointer ke context lama

    movq %r15, 0(%rdi)
    movq %r14, 8(%rdi)
    movq %r13, 16(%rdi)
    movq %r12, 24(%rdi)
    movq %rbp, 32(%rdi)
    movq %rbx, 40(%rdi)

    # Simpan return address sebagai RIP
    movq (%rsp), %rax
    movq %rax, 48(%rdi)

    # Simpan RSP (setelah call)
    leaq 8(%rsp), %rax
    movq %rax, 56(%rdi)

    # Simpan RFLAGS
    pushfq
    popq %rax
    movq %rax, 64(%rdi)

    # Simpan CR3
    movq %cr3, %rax
    movq %rax, 72(%rdi)

    # Simpan segment registers
    movw %cs, 80(%rdi)
    movw %ds, 82(%rdi)
    movw %es, 84(%rdi)
    movw %fs, 86(%rdi)
    movw %gs, 88(%rdi)
    movw %ss, 90(%rdi)

    # Load context baru
    # RSI = pointer ke context baru

    # Load CR3 jika berbeda
    movq 72(%rsi), %rax
    movq %cr3, %rdx
    cmpq %rax, %rdx
    je .no_cr3_switch
    movq %rax, %cr3
.no_cr3_switch:

    # Load segment registers
    movw 82(%rsi), %dx
    movw %dx, %ds
    movw 84(%rsi), %dx
    movw %dx, %es
    movw 86(%rsi), %dx
    movw %dx, %fs
    movw 88(%rsi), %dx
    movw %dx, %gs

    # Load register umum
    movq 0(%rsi), %r15
    movq 8(%rsi), %r14
    movq 16(%rsi), %r13
    movq 24(%rsi), %r12
    movq 32(%rsi), %rbp
    movq 40(%rsi), %rbx

    # Load RFLAGS
    movq 64(%rsi), %rax
    pushq %rax
    popfq

    # Load RSP dan RIP
    movq 56(%rsi), %rsp

    # Jump ke context baru
    jmp *48(%rsi)

.size _context_switch_64, . - _context_switch_64

# -----------------------------------------------------------------------------
# _jump_to_usermode_64
# --------------------
# Melompat ke user mode (Ring 3).
#
# Parameter:
#   RDI = entry point (RIP user)
#   RSI = stack pointer (RSP user)
# -----------------------------------------------------------------------------
.global _jump_to_usermode_64
.type _jump_to_usermode_64, @function
_jump_to_usermode_64:
    # Setup stack untuk IRETQ
    # IRETQ expects: SS, RSP, RFLAGS, CS, RIP

    # Load user data segment selector
    movw $0x23, %ax                   # User data (0x20 | 3)
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # Push stack untuk IRETQ
    pushq $0x23                       # SS (User data segment | RPL 3)
    pushq %rsi                        # RSP user
    pushfq                             # RFLAGS
    orq $0x200, (%rsp)                # Set IF (interrupt flag)
    pushq $0x1B                       # CS (User code segment | RPL 3)
    pushq %rdi                        # RIP user

    # Return ke user mode
    iretq

.size _jump_to_usermode_64, . - _jump_to_usermode_64

# -----------------------------------------------------------------------------
# _fork_return
# ------------
# Return path untuk proses baru setelah fork.
# -----------------------------------------------------------------------------
.global _fork_return
.type _fork_return, @function
_fork_return:
    # Set return value ke 0 (child process)
    xorq %rax, %rax
    ret

.size _fork_return, . - _fork_return

# -----------------------------------------------------------------------------
# _get_current_stack
# ------------------
# Mendapatkan stack pointer saat ini.
#
# Return:
#   RAX = RSP
# -----------------------------------------------------------------------------
.global _get_current_stack
.type _get_current_stack, @function
_get_current_stack:
    movq %rsp, %rax
    ret

.size _get_current_stack, . - _get_current_stack

# -----------------------------------------------------------------------------
# _set_stack_pointer
# ------------------
# Mengatur stack pointer.
#
# Parameter:
#   RDI = nilai RSP baru
# -----------------------------------------------------------------------------
.global _set_stack_pointer
.type _set_stack_pointer, @function
_set_stack_pointer:
    movq %rdi, %rsp
    ret

.size _set_stack_pointer, . - _set_stack_pointer
