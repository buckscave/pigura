# =============================================================================
# PIGURA OS - CONTEXT SWITCH x86
# =============================================================================
# Berkas ini berisi implementasi context switching untuk arsitektur x86.
# Context switch menyimpan state proses lama dan memulihkan state
# proses baru.
#
# Arsitektur: x86 (32-bit)
# Versi: 1.0
# =============================================================================

# =============================================================================
# STRUKTUR CONTEXT
# =============================================================================
# struct context {
#     uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
#     uint32_t eip, eflags;
#     uint32_t cs, ds, es, fs, gs, ss;
#     uint32_t cr3;
# };
# =============================================================================

.section .text
.code32

# =============================================================================
# context_save
# =============================================================================
# Menyimpan context proses saat ini.
#
# Parameter:
#   [ESP+4] = Pointer ke struct context
#
# Return:
#   0 jika berhasil
# =============================================================================
.global context_save
context_save:
    movl 4(%esp), %eax        # EAX = pointer ke context

    # Simpan register umum
    movl %edi, 0(%eax)
    movl %esi, 4(%eax)
    movl %ebp, 8(%eax)
    movl %esp, 12(%eax)       # ESP saat ini (sebelum call)
    movl %ebx, 16(%eax)
    movl %edx, 20(%eax)
    movl %ecx, 24(%eax)
    movl %eax, 28(%eax)       # EAX sementara

    # Simpan EIP (return address)
    movl (%esp), %ebx
    movl %ebx, 32(%eax)       # EIP = return address

    # Simpan EFLAGS
    pushfl
    popl %ebx
    movl %ebx, 36(%eax)

    # Simpan segment register
    movw %cs, 40(%eax)
    movw %ds, 44(%eax)
    movw %es, 48(%eax)
    movw %fs, 52(%eax)
    movw %gs, 56(%eax)
    movw %ss, 60(%eax)

    # Simpan CR3
    movl %cr3, %ebx
    movl %ebx, 64(%eax)

    # Return 0
    xorl %eax, %eax
    ret

# =============================================================================
# context_restore
# =============================================================================
# Memulihkan context proses baru.
#
# Parameter:
#   [ESP+4] = Pointer ke struct context
#
# Note: Tidak return, langsung switch ke context baru
# =============================================================================
.global context_restore
context_restore:
    movl 4(%esp), %eax        # EAX = pointer ke context

    # Restore CR3 (page directory)
    movl 64(%eax), %ebx
    movl %ebx, %cr3

    # Restore segment register
    movw 44(%eax), %ds
    movw 48(%eax), %es
    movw 52(%eax), %fs
    movw 56(%eax), %gs

    # Restore register umum
    movl 0(%eax), %edi
    movl 4(%eax), %esi
    movl 8(%eax), %ebp
    movl 16(%eax), %ebx
    movl 20(%eax), %edx
    movl 24(%eax), %ecx

    # Restore EFLAGS
    movl 36(%eax), %ebx
    pushl %ebx
    popfl

    # Get EIP
    movl 32(%eax), %ebx       # EIP
    movl 28(%eax), %eax       # EAX (restore terakhir)

    # Set return address ke EIP baru
    # Stack sekarang: [return addr]
    # Kita perlu pop return lama, dan jmp ke EIP baru

    # Simpan EIP ke stack
    pushl %ebx                # Push EIP baru sebagai return

    ret                       # Jump ke EIP baru

# =============================================================================
# context_switch
# =============================================================================
# Melakukan context switch dari proses lama ke proses baru.
#
# Parameter:
#   [ESP+4] = Pointer ke context lama (output)
#   [ESP+8] = Pointer ke context baru (input)
#
# Return:
#   0 setelah kembali ke context lama
# =============================================================================
.global context_switch
context_switch:
    # Simpan context lama
    pushl 4(%esp)             # Pointer ke context lama
    call context_save
    addl $4, %esp

    # Load context baru
    pushl 8(%esp)             # Pointer ke context baru
    call context_restore

    # Tidak akan sampai sini
    ret

# =============================================================================
# context_fork
# =============================================================================
# Membuat copy context untuk proses baru (fork).
#
# Parameter:
#   [ESP+4] = Pointer ke context parent (input)
#   [ESP+8] = Pointer ke context child (output)
#   [ESP+12] = CR3 baru untuk child
#
# Return:
#   Di parent: child PID
#   Di child: 0
# =============================================================================
.global context_fork
context_fork:
    pushl %ebp
    movl %esp, %ebp

    movl 8(%ebp), %eax        # Context parent
    movl 12(%ebp), %ebx       # Context child
    movl 16(%ebp), %ecx       # CR3 baru

    # Copy context parent ke child
    pushl %esi
    pushl %edi

    movl %eax, %esi           # Source
    movl %ebx, %edi           # Dest
    movl $17, %ecx            # 17 dwords (68 bytes)
    rep movsl

    popl %edi
    popl %esi

    movl 12(%ebp), %ebx       # Context child

    # Set CR3 baru untuk child
    movl 16(%ebp), %ecx
    movl %ecx, 64(%ebx)

    # Set EAX = 0 di child context (return value)
    xorl %ecx, %ecx
    movl %ecx, 28(%ebx)

    # Return child PID (akan diset oleh caller)
    movl $1, %eax

    popl %ebp
    ret

# =============================================================================
# context_init_kernel
# =============================================================================
# Menginisialisasi context untuk kernel thread.
#
# Parameter:
#   [ESP+4] = Pointer ke context
#   [ESP+8] = Entry point (EIP)
#   [ESP+12] = Stack pointer (ESP)
#   [ESP+16] = CR3 (page directory)
# =============================================================================
.global context_init_kernel
context_init_kernel:
    pushl %ebp
    movl %esp, %ebp

    movl 8(%ebp), %eax        # Context

    # Clear semua register
    xorl %ecx, %ecx
    movl %ecx, 0(%eax)        # EDI
    movl %ecx, 4(%eax)        # ESI
    movl %ecx, 8(%eax)        # EBP

    # Set ESP
    movl 16(%ebp), %ecx
    movl %ecx, 12(%eax)       # ESP

    movl %ecx, 16(%eax)       # EBX
    movl %ecx, 20(%eax)       # EDX
    movl %ecx, 24(%eax)       # ECX
    movl %ecx, 28(%eax)       # EAX

    # Set EIP
    movl 12(%ebp), %ecx
    movl %ecx, 32(%eax)       # EIP

    # Set EFLAGS (interrupts enabled)
    movl $0x202, %ecx
    movl %ecx, 36(%eax)       # EFLAGS

    # Set segment selectors (kernel)
    movw $0x08, 40(%eax)      # CS = kernel code
    movw $0x10, 44(%eax)      # DS = kernel data
    movw $0x10, 48(%eax)      # ES
    movw $0x10, 52(%eax)      # FS
    movw $0x10, 56(%eax)      # GS
    movw $0x10, 60(%eax)      # SS

    # Set CR3
    movl 20(%ebp), %ecx
    movl %ecx, 64(%eax)

    popl %ebp
    ret

# =============================================================================
# context_init_user
# =============================================================================
# Menginisialisasi context untuk user process.
#
# Parameter:
#   [ESP+4] = Pointer ke context
#   [ESP+8] = Entry point (EIP)
#   [ESP+12] = Stack pointer (ESP)
#   [ESP+16] = CR3 (page directory)
# =============================================================================
.global context_init_user
context_init_user:
    pushl %ebp
    movl %esp, %ebp

    movl 8(%ebp), %eax        # Context

    # Clear semua register
    xorl %ecx, %ecx
    movl %ecx, 0(%eax)        # EDI
    movl %ecx, 4(%eax)        # ESI
    movl %ecx, 8(%eax)        # EBP

    # Set ESP
    movl 16(%ebp), %ecx
    movl %ecx, 12(%eax)       # ESP

    movl %ecx, 16(%eax)       # EBX
    movl %ecx, 20(%eax)       # EDX
    movl %ecx, 24(%eax)       # ECX
    movl %ecx, 28(%eax)       # EAX

    # Set EIP
    movl 12(%ebp), %ecx
    movl %ecx, 32(%eax)       # EIP

    # Set EFLAGS (interrupts enabled, IOPL=0)
    movl $0x202, %ecx
    movl %ecx, 36(%eax)       # EFLAGS

    # Set segment selectors (user dengan RPL=3)
    movw $0x1B, 40(%eax)      # CS = user code (0x18 | 3)
    movw $0x23, 44(%eax)      # DS = user data (0x20 | 3)
    movw $0x23, 48(%eax)      # ES
    movw $0x23, 52(%eax)      # FS
    movw $0x23, 56(%eax)      # GS
    movw $0x23, 60(%eax)      # SS

    # Set CR3
    movl 20(%ebp), %ecx
    movl %ecx, 64(%eax)

    popl %ebp
    ret
