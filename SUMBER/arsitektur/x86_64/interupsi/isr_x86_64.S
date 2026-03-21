# =============================================================================
# PIGURA OS - ISR x86_64
# =============================================================================
#
# Berkas ini berisi kode assembly untuk Interrupt Service Routines (ISR)
# pada arsitektur x86_64.
#
# Arsitektur: x86_64
# Versi: 1.0
# =============================================================================

# =============================================================================
# MAKRO
# =============================================================================

# Macro untuk membuat ISR tanpa error code
.macro ISR_NOERR index
.global isr_\index
.type isr_\index, @function
isr_\index:
    pushq $0                          # Push dummy error code
    pushq $\index                      # Push interrupt number
    jmp isr_common
.size isr_\index, . - isr_\index
.endm

# Macro untuk membuat ISR dengan error code
.macro ISR_ERR index
.global isr_\index
.type isr_\index, @function
isr_\index:
    pushq $\index                      # Push interrupt number
    jmp isr_common
.size isr_\index, . - isr_\index
.endm

# Macro untuk membuat IRQ handler
.macro IRQ_HANDLER index, vector
.global irq_\index
.type irq_\index, @function
irq_\index:
    pushq $0                          # Dummy error code
    pushq $\vector                     # Vector number
    jmp irq_common
.size irq_\index, . - irq_\index
.endm

# =============================================================================
# KODE
# =============================================================================

.section .text
.code64

# -----------------------------------------------------------------------------
# Exception handlers (ISR 0-31)
# -----------------------------------------------------------------------------

# ISR 0: Divide Error (#DE)
ISR_NOERR 0

# ISR 1: Debug Exception (#DB)
ISR_NOERR 1

# ISR 2: Non-Maskable Interrupt (NMI)
ISR_NOERR 2

# ISR 3: Breakpoint (#BP)
ISR_NOERR 3

# ISR 4: Overflow (#OF)
ISR_NOERR 4

# ISR 5: BOUND Range Exceeded (#BR)
ISR_NOERR 5

# ISR 6: Invalid Opcode (#UD)
ISR_NOERR 6

# ISR 7: Device Not Available (#NM)
ISR_NOERR 7

# ISR 8: Double Fault (#DF) - has error code
ISR_ERR 8

# ISR 9: Coprocessor Segment Overrun
ISR_NOERR 9

# ISR 10: Invalid TSS (#TS) - has error code
ISR_ERR 10

# ISR 11: Segment Not Present (#NP) - has error code
ISR_ERR 11

# ISR 12: Stack-Segment Fault (#SS) - has error code
ISR_ERR 12

# ISR 13: General Protection (#GP) - has error code
ISR_ERR 13

# ISR 14: Page Fault (#PF) - has error code
ISR_ERR 14

# ISR 15: Reserved
ISR_NOERR 15

# ISR 16: x87 FPU Error (#MF)
ISR_NOERR 16

# ISR 17: Alignment Check (#AC) - has error code
ISR_ERR 17

# ISR 18: Machine Check (#MC)
ISR_NOERR 18

# ISR 19: SIMD Exception (#XM)
ISR_NOERR 19

# ISR 20: Virtualization Exception (#VE)
ISR_NOERR 20

# ISR 21-29: Reserved
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29

# ISR 30: Security Exception (#SX) - has error code
ISR_ERR 30

# ISR 31: Reserved
ISR_NOERR 31

# -----------------------------------------------------------------------------
# IRQ handlers (ISR 32-47)
# -----------------------------------------------------------------------------

IRQ_HANDLER 0,  32
IRQ_HANDLER 1,  33
IRQ_HANDLER 2,  34
IRQ_HANDLER 3,  35
IRQ_HANDLER 4,  36
IRQ_HANDLER 5,  37
IRQ_HANDLER 6,  38
IRQ_HANDLER 7,  39
IRQ_HANDLER 8,  40
IRQ_HANDLER 9,  41
IRQ_HANDLER 10, 42
IRQ_HANDLER 11, 43
IRQ_HANDLER 12, 44
IRQ_HANDLER 13, 45
IRQ_HANDLER 14, 46
IRQ_HANDLER 15, 47

# -----------------------------------------------------------------------------
# isr_common
# ----------
# Common entry point untuk semua exception.
# -----------------------------------------------------------------------------
.global isr_common
.type isr_common, @function
isr_common:
    # Simpan semua register umum
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

    # Simpan segment data
    movw %ds, %ax
    pushq %rax

    # Load kernel data segment
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # Panggil C handler
    movq %rsp, %rdi                   # Argumen 1: pointer ke stack frame
    call _dispatcher_interupsi

    # Restore segment
    popq %rax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

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
    popq %rax

    # Hapus error code dan interrupt number
    addq $16, %rsp

    iretq

.size isr_common, . - isr_common

# -----------------------------------------------------------------------------
# irq_common
# ----------
# Common entry point untuk semua IRQ.
# -----------------------------------------------------------------------------
.global irq_common
.type irq_common, @function
irq_common:
    # Simpan semua register umum
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

    # Simpan segment data
    movw %ds, %ax
    pushq %rax

    # Load kernel data segment
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # Panggil C handler
    movq %rsp, %rdi
    call _dispatcher_interupsi

    # Restore segment
    popq %rax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

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
    popq %rax

    # Hapus error code dan interrupt number
    addq $16, %rsp

    iretq

.size irq_common, . - irq_common

# =============================================================================
# DATA
# =============================================================================

.section .rodata

# Daftar pointer ISR (untuk inisialisasi IDT)
.global isr_table
isr_table:
    .quad isr_0
    .quad isr_1
    .quad isr_2
    .quad isr_3
    .quad isr_4
    .quad isr_5
    .quad isr_6
    .quad isr_7
    .quad isr_8
    .quad isr_9
    .quad isr_10
    .quad isr_11
    .quad isr_12
    .quad isr_13
    .quad isr_14
    .quad isr_15
    .quad isr_16
    .quad isr_17
    .quad isr_18
    .quad isr_19
    .quad isr_20
    .quad isr_21
    .quad isr_22
    .quad isr_23
    .quad isr_24
    .quad isr_25
    .quad isr_26
    .quad isr_27
    .quad isr_28
    .quad isr_29
    .quad isr_30
    .quad isr_31
