# =============================================================================
# PIGURA OS - RING SWITCH x86
# =============================================================================
# Berkas ini berisi implementasi untuk beralih dari Ring 0 (kernel)
# ke Ring 3 (user mode) dan sebaliknya.
#
# Arsitektur: x86 (32-bit)
# Versi: 1.0
# =============================================================================

.section .text
.code32

# =============================================================================
# KONSTANTA
# =============================================================================

.set SELECTOR_USER_KODE, 0x1B    # User code (0x18 | 3)
.set SELECTOR_USER_DATA, 0x23    # User data (0x20 | 3)
.set SELECTOR_KERNEL_KODE, 0x08   # Kernel code
.set SELECTOR_KERNEL_DATA, 0x10   # Kernel data

# =============================================================================
# ring_switch_ke_user
# =============================================================================
# Beralih dari kernel mode (Ring 0) ke user mode (Ring 3).
#
# Parameter:
#   [ESP+4] = Entry point (EIP)
#   [ESP+8] = Stack pointer (ESP)
#
# Note: Tidak return, langsung ke user mode
# =============================================================================
.global ring_switch_ke_user
ring_switch_ke_user:
    # Ambil parameter
    movl 4(%esp), %eax        # EAX = entry point
    movl 8(%esp), %ebx        # EBX = stack pointer

    # Setup stack untuk IRET
    # Stack untuk IRET:
    #   [SS]      - User data selector
    #   [ESP]     - User stack pointer
    #   [EFLAGS]  - EFLAGS dengan IF=1
    #   [CS]      - User code selector
    #   [EIP]     - Entry point

    # Push SS
    pushl $SELECTOR_USER_DATA

    # Push ESP
    pushl %ebx

    # Push EFLAGS (enable interrupts)
    pushfl
    orl $0x200, (%esp)        # Set IF (interrupt flag)

    # Push CS
    pushl $SELECTOR_USER_KODE

    # Push EIP
    pushl %eax

    # Set segment register ke user data
    movw $SELECTOR_USER_DATA, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # IRET akan:
    #   1. Pop EIP
    #   2. Pop CS (dengan RPL=3, switch ke Ring 3)
    #   3. Pop EFLAGS
    #   4. Pop ESP
    #   5. Pop SS
    iret

# =============================================================================
# ring_switch_ke_kernel
# =============================================================================
# Beralih dari user mode ke kernel mode (via interrupt/trap).
# Fungsi ini biasanya dipanggil dari interrupt handler.
#
# Parameter:
#   Tidak ada - dipanggil saat syscall/interrupt
# =============================================================================
.global ring_switch_ke_kernel
ring_switch_ke_kernel:
    # Fungsi ini adalah placeholder
    # Switch ke kernel terjadi otomatis saat interrupt
    ret

# =============================================================================
# ring_switch_ke_user_with_args
# =============================================================================
# Beralih ke user mode dengan argumen.
#
# Parameter:
#   [ESP+4] = Entry point
#   [ESP+8] = Stack pointer
#   [ESP+12] = Argumen argc
#   [ESP+16] = Argumen argv
# =============================================================================
.global ring_switch_ke_user_with_args
ring_switch_ke_user_with_args:
    # Ambil parameter
    movl 4(%esp), %eax        # Entry point
    movl 8(%esp), %ebx        # Stack pointer
    movl 12(%esp), %ecx       # argc
    movl 16(%esp), %edx       # argv

    # Push argumen ke user stack
    # User stack layout:
    #   [argv]   <- ESP
    #   [argc]
    #   [return addr = 0 (exit)]  (pseudo return)

    subl $12, %ebx            # Make room

    # Push ke user stack (menggunakan segment override)
    # Catatan: Ini mengasumsikan user space sudah di-map
    movl %edx, 8(%ebx)        # argv
    movl %ecx, 4(%ebx)        # argc
    movl $0, 0(%ebx)          # return address (exit)

    # Setup stack untuk IRET
    pushl $SELECTOR_USER_DATA
    pushl %ebx                # ESP (dengan argumen)
    pushfl
    orl $0x200, (%esp)
    pushl $SELECTOR_USER_KODE
    pushl %eax

    # Set segment register
    movw $SELECTOR_USER_DATA, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    iret

# =============================================================================
# ring_get_current
# =============================================================================
# Mendapatkan current privilege level (CPL).
#
# Return:
#   EAX = 0 (Ring 0) atau 3 (Ring 3)
# =============================================================================
.global ring_get_current
ring_get_current:
    movw %cs, %ax
    andw $0x03, %ax           # RPL bits
    movzxw %ax, %eax
    ret

# =============================================================================
# ring_is_kernel
# =============================================================================
# Cek apakah saat ini di kernel mode.
#
# Return:
#   EAX = 1 jika kernel mode, 0 jika user mode
# =============================================================================
.global ring_is_kernel
ring_is_kernel:
    movw %cs, %ax
    andw $0x03, %ax
    cmpw $0, %ax
    setz %al
    movzxb %al, %eax
    ret

# =============================================================================
# ring_is_user
# =============================================================================
# Cek apakah saat ini di user mode.
#
# Return:
#   EAX = 1 jika user mode, 0 jika kernel mode
# =============================================================================
.global ring_is_user
ring_is_user:
    movw %cs, %ax
    andw $0x03, %ax
    cmpw $3, %ax
    setz %al
    movzxb %al, %eax
    ret

# =============================================================================
# sysenter_entry
# =============================================================================
# Entry point untuk sysenter syscall (jika didukung).
# Ini adalah alternatif untuk int 0x80 yang lebih cepat.
# =============================================================================
.global sysenter_entry
sysenter_entry:
    # Simpan segment register user
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    # Set segment ke kernel
    movw $SELECTOR_KERNEL_DATA, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # Simpan register
    pushal

    # Panggil syscall handler
    # EAX = syscall number
    # EBX, ECX, EDX, ESI, EDI, EBP = argumen
    call syscall_handler

    # Result di EAX

    # Restore register
    popal

    # Restore segment
    popl %gs
    popl %fs
    popl %es
    popl %ds

    # Return ke user mode via sysexit
    # EDX = EIP user, ECX = ESP user
    sysexit

# =============================================================================
# sysexit_setup
# =============================================================================
# Setup register untuk sysexit.
#
# Parameter:
#   [ESP+4] = EIP user
#   [ESP+8] = ESP user
# =============================================================================
.global sysexit_setup
sysexit_setup:
    movl 4(%esp), %edx        # EIP untuk sysexit
    movl 8(%esp), %ecx        # ESP untuk sysexit
    ret
