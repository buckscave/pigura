# =============================================================================
# PIGURA OS - INTERRUPT SERVICE ROUTINES (ISR) x86
# =============================================================================
# Berkas ini berisi entry point untuk semua interrupt exception (0-31)
# dan IRQ (32-47). Setiap entry menyimpan state, memanggil handler C,
# dan merestore state.
#
# Arsitektur: x86 (32-bit)
# Versi: 1.0
# =============================================================================

# =============================================================================
# KONSTANTA
# =============================================================================

.set VEKTOR_IRQ_MULAI,    32

# =============================================================================
# MAKRO
# =============================================================================

# Makro untuk membuat ISR tanpa error code
.macro ISR_NOERR nomor
.global isr\nomor
isr\nomor:
    pushl $0                  # Push dummy error code
    pushl $\nomor             # Push interrupt number
    jmp isr_common
.endm

# Makro untuk membuat ISR dengan error code
.macro ISR_ERR nomor
.global isr\nomor
isr\nomor:
    pushl $\nomor             # Push interrupt number
    jmp isr_common
.endm

# Makro untuk IRQ handler
.macro IRQ_HANDLER nomor, irq_num
.global irq\nomor
irq\nomor:
    pushl $0                  # Dummy error code
    pushl $\irq_num           # IRQ number (32-47)
    jmp irq_common
.endm

# =============================================================================
# SECTION TEXT
# =============================================================================

.section .text
.code32

# =============================================================================
# EXCEPTION HANDLERS (ISR 0-31)
# =============================================================================

# Exception 0-7: Tanpa error code
ISR_NOERR 0      # Divide Error
ISR_NOERR 1      # Debug Exception
ISR_NOERR 2      # NMI Interrupt
ISR_NOERR 3      # Breakpoint
ISR_NOERR 4      # Overflow
ISR_NOERR 5      # BOUND Range Exceeded
ISR_NOERR 6      # Invalid Opcode
ISR_NOERR 7      # Device Not Available

# Exception 8: Double Fault (dengan error code)
ISR_ERR 8

# Exception 9: Coprocessor Segment Overrun
ISR_NOERR 9

# Exception 10-14: Dengan error code
ISR_ERR 10      # Invalid TSS
ISR_ERR 11      # Segment Not Present
ISR_ERR 12      # Stack-Segment Fault
ISR_ERR 13      # General Protection
ISR_ERR 14      # Page Fault

# Exception 15: Reserved
ISR_NOERR 15

# Exception 16-19: Tanpa error code
ISR_NOERR 16    # x87 FPU Error
ISR_NOERR 17    # Alignment Check
ISR_NOERR 18    # Machine Check
ISR_NOERR 19    # SIMD Exception

# Exception 20-31: Reserved
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

# =============================================================================
# IRQ HANDLERS (IRQ 0-15, vektor 32-47)
# =============================================================================

IRQ_HANDLER 0, 32    # Timer
IRQ_HANDLER 1, 33    # Keyboard
IRQ_HANDLER 2, 34    # Cascade
IRQ_HANDLER 3, 35    # COM2
IRQ_HANDLER 4, 36    # COM1
IRQ_HANDLER 5, 37    # LPT2
IRQ_HANDLER 6, 38    # Floppy
IRQ_HANDLER 7, 39    # LPT1
IRQ_HANDLER 8, 40    # RTC
IRQ_HANDLER 9, 41    # Free
IRQ_HANDLER 10, 42   # Free
IRQ_HANDLER 11, 43   # Free
IRQ_HANDLER 12, 44   # PS/2 Mouse
IRQ_HANDLER 13, 45   # FPU
IRQ_HANDLER 14, 46   # Primary ATA
IRQ_HANDLER 15, 47   # Secondary ATA

# =============================================================================
# COMMON ISR HANDLER
# =============================================================================

.extern exception_handler_umum

isr_common:
    # Simpan semua segment register
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    # Simpan semua register umum
    pushal

    # Set data segment ke kernel
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # Stack pointer sekarang menunjuk ke struct int_frame
    # Push pointer ke stack sebagai argumen
    pushl %esp

    # Panggil handler C
    call exception_handler_umum

    # Bersihkan argumen
    addl $4, %esp

    # Restore register
    popal

    # Restore segment register
    popl %gs
    popl %fs
    popl %es
    popl %ds

    # Skip error code dan interrupt number
    addl $8, %esp

    # Return dari interrupt
    iret

# =============================================================================
# COMMON IRQ HANDLER
# =============================================================================

.extern irq_handler_umum

irq_common:
    # Simpan semua segment register
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    # Simpan semua register umum
    pushal

    # Set data segment ke kernel
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # Ambil IRQ number dari stack
    # Stack: ... int_no, err_code, gs, fs, es, ds, pusha...
    movl 32(%esp), %eax    # int_no ada di offset 32

    # Cek apakah IRQ 7 atau 15 (spurious)
    cmpl $39, %eax
    je .check_spurious_7
    cmpl $47, %eax
    je .check_spurious_15
    jmp .call_irq_handler

.check_spurious_7:
    # Cek ISR register master PIC
    movb $0x0B, %al        # Read ISR command
    outb %al, $0x20
    inb $0x20, %al
    testb $0x80, %al       # Bit 7 = IRQ7
    jz .spurious_irq
    jmp .call_irq_handler

.check_spurious_15:
    # Cek ISR register slave PIC
    movb $0x0B, %al
    outb %al, $0xA0
    inb $0xA0, %al
    testb $0x80, %al       # Bit 7 = IRQ15
    jz .spurious_irq

.call_irq_handler:
    # Panggil handler C dengan IRQ number
    pushl %eax
    call irq_handler_umum
    addl $4, %esp
    jmp .irq_exit

.spurious_irq:
    # Spurious IRQ - skip handler

.irq_exit:
    # Restore register
    popal

    # Restore segment register
    popl %gs
    popl %fs
    popl %es
    popl %ds

    # Skip error code dan interrupt number
    addl $8, %esp

    # Return dari interrupt
    iret

# =============================================================================
# SYSCALL HANDLER (int 0x80)
# =============================================================================

.extern syscall_handler

.global syscall_entry
syscall_entry:
    # Syscall via int 0x80
    # EAX = syscall number
    # EBX, ECX, EDX, ESI, EDI, EBP = argumen

    # Simpan segment register
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    # Simpan register umum
    pushal

    # Set data segment ke kernel
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # Panggil syscall handler
    # Argumen sudah di register dari user space
    call syscall_handler

    # Result sudah di EAX

    # Restore register
    popal

    popl %gs
    popl %fs
    popl %es
    popl %ds

    iret

# =============================================================================
# DATA
# =============================================================================

.section .data

# Pointer ke ISR handler array
.global isr_table
isr_table:
    .long isr0
    .long isr1
    .long isr2
    .long isr3
    .long isr4
    .long isr5
    .long isr6
    .long isr7
    .long isr8
    .long isr9
    .long isr10
    .long isr11
    .long isr12
    .long isr13
    .long isr14
    .long isr15
    .long isr16
    .long isr17
    .long isr18
    .long isr19
    .long isr20
    .long isr21
    .long isr22
    .long isr23
    .long isr24
    .long isr25
    .long isr26
    .long isr27
    .long isr28
    .long isr29
    .long isr30
    .long isr31
