# =============================================================================
# PIGURA OS - BOOT CODE ARM (32-BIT)
# =============================================================================
#
# Berkas ini berisi kode boot untuk arsitektur ARM 32-bit.
# Dirancang untuk boot dari bootloader atau bare metal pada board ARM.
#
# Fungsi utama:
#   1. Setup stack dan environment
#   2. Inisialisasi UART untuk output
#   3. Setup MMU dan page tables
#   4. Lompat ke kernel C entry
#
# Arsitektur: ARM (ARMv4/v5/v6 - 32-bit)
# Versi: 1.0
# =============================================================================

# =============================================================================
# KONSTANTA
# =============================================================================

# Alamat memory
.set ALAMAT_KERNEL,        0x00010000    # Kernel load address
.set ALAMAT_STACK,         0x00008000    # Stack bottom
.set UKURAN_STACK,         0x8000        # 32 KB stack

# UART PL011 (QEMU virt machine)
.set UART0_BASE,           0x09000000    # UART0 base (QEMU virt)
.set UART0_DR,             0x00          # Data Register
.set UART0_FR,             0x18          # Flag Register
.set UART0_IBRD,           0x24          # Integer Baud Rate
.set UART0_FBRD,           0x28          # Fractional Baud Rate
.set UART0_LCRH,           0x2C          # Line Control
.set UART0_CR,             0x30          # Control Register

# UART flags
.set UART_FR_TXFF,         0x20          # Transmit FIFO full
.set UART_FR_RXFE,         0x10          # Receive FIFO empty

# UART control
.set UART_CR_UARTEN,       0x01          # UART enable
.set UART_CR_TXE,          0x08          # Transmit enable
.set UART_CR_RXE,          0x04          # Receive enable

# UART line control
.set UART_LCRH_FEN,        0x10          # FIFO enable
.set UART_LCRH_WLEN_8,     0x60          # 8-bit word

# CPSR mode bits
.set CPSR_MODE_SVC,        0x13
.set CPSR_MODE_IRQ,        0x12
.set CPSR_MODE_FIQ,        0x11
.set CPSR_MODE_ABT,        0x17
.set CPSR_MODE_UND,        0x1B
.set CPSR_MODE_SYS,        0x1F

# CPSR flags
.set CPSR_IRQ_DIS,         (1 << 7)
.set CPSR_FIQ_DIS,         (1 << 6)

# =============================================================================
# ENTRY POINT
# =============================================================================

.section .text.boot
.global _start
.global _kernel_entry

_start:
    # -------------------------------------------------------------------------
    # Disable interrupts dan switch ke SVC mode
    # -------------------------------------------------------------------------
    cpsid   if                          # Disable IRQ dan FIQ
    cps     #CPSR_MODE_SVC              # Switch ke Supervisor mode

    # -------------------------------------------------------------------------
    # Setup stack
    # -------------------------------------------------------------------------
    ldr     sp, =_stack_top

    # -------------------------------------------------------------------------
    # Zero BSS section
    # -------------------------------------------------------------------------
    ldr     r0, =_bss_start
    ldr     r1, =_bss_end
    mov     r2, #0
._bss_loop:
    cmp     r0, r1
    bge     ._bss_done
    str     r2, [r0], #4
    b       ._bss_loop
._bss_done:

    # -------------------------------------------------------------------------
    # Initialize UART for early output
    # -------------------------------------------------------------------------
    bl      _uart_init

    # -------------------------------------------------------------------------
    # Print boot banner
    # -------------------------------------------------------------------------
    ldr     r0, =_pesan_banner
    bl      _uart_puts

    # -------------------------------------------------------------------------
    # Setup page tables (identity mapping)
    # -------------------------------------------------------------------------
    bl      _mmu_setup

    # -------------------------------------------------------------------------
    # Enable MMU
    # -------------------------------------------------------------------------
    bl      _mmu_enable

    # -------------------------------------------------------------------------
    # Jump to C kernel entry
    # -------------------------------------------------------------------------
    ldr     r0, =_kernel_entry
    bx      r0

    # -------------------------------------------------------------------------
    # Should not reach here
    # -------------------------------------------------------------------------
    b       .

# =============================================================================
# UART FUNCTIONS
# =============================================================================

.section .text

# -----------------------------------------------------------------------------
# _uart_init
# ----------
# Initialize UART0 for 115200 baud, 8N1.
# -----------------------------------------------------------------------------
_uart_init:
    # Load UART base address
    ldr     r0, =UART0_BASE

    # Disable UART
    mov     r1, #0
    str     r1, [r0, #UART0_CR]

    # Wait for TX FIFO to empty
._uart_wait_tx:
    ldr     r1, [r0, #UART0_FR]
    tst     r1, #UART_FR_TXFF
    bne     ._uart_wait_tx

    # Set baud rate (115200 @ 24MHz clock)
    # IBRD = 24MHz / (16 * 115200) = 13
    mov     r1, #13
    str     r1, [r0, #UART0_IBRD]

    # FBRD = 0 (fractional part)
    mov     r1, #0
    str     r1, [r0, #UART0_FBRD]

    # Set line control: 8-bit, FIFO enabled
    mov     r1, #(UART_LCRH_WLEN_8 | UART_LCRH_FEN)
    str     r1, [r0, #UART0_LCRH]

    # Enable UART, TX, RX
    mov     r1, #(UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE)
    str     r1, [r0, #UART0_CR]

    mov     pc, lr

# -----------------------------------------------------------------------------
# _uart_putc
# ----------
# Output single character to UART.
# Parameters: r0 = character
# -----------------------------------------------------------------------------
_uart_putc:
    # Save character
    mov     r2, r0

    # Load UART base
    ldr     r1, =UART0_BASE

    # Wait for TX FIFO not full
._uart_putc_wait:
    ldr     r3, [r1, #UART0_FR]
    tst     r3, #UART_FR_TXFF
    bne     ._uart_putc_wait

    # Write character
    str     r2, [r1, #UART0_DR]

    mov     pc, lr

# -----------------------------------------------------------------------------
# _uart_puts
# ----------
# Output null-terminated string to UART.
# Parameters: r0 = pointer to string
# -----------------------------------------------------------------------------
_uart_puts:
    # Save lr
    mov     r3, lr
    mov     r2, r0                         # String pointer

._uart_puts_loop:
    ldrb    r0, [r2], #1                   # Load byte and increment
    cmp     r0, #0                         # Check for null terminator
    beq     ._uart_puts_done

    bl      _uart_putc
    b       ._uart_puts_loop

._uart_puts_done:
    mov     pc, r3

# =============================================================================
# MMU FUNCTIONS
# =============================================================================

# -----------------------------------------------------------------------------
# _mmu_setup
# ----------
# Setup page tables for identity mapping.
# -----------------------------------------------------------------------------
_mmu_setup:
    # Save return address
    mov     r8, lr

    # Get page table base
    ldr     r0, =_page_table

    # Clear page table (16KB = 4096 entries)
    mov     r1, r0
    mov     r2, #0
    mov     r3, #4096
._mmu_clear:
    str     r2, [r1], #4
    subs    r3, r3, #1
    bne     ._mmu_clear

    # Create identity mapping (first 1 GB with 1MB sections)
    # Section descriptor: AP=11 (RW), Domain=0, CB=11, Type=10 (section)
    mov     r1, r0                         # Page table pointer
    mov     r2, #0                         # Physical address
    mov     r3, #0xC12                     # Section flags: C=1, B=1, AP=11, type=10
    orr     r3, r3, #0x400                 # Domain 0 client
    mov     r4, #1024                      # 1024 sections = 1GB

._mmu_section_loop:
    orr     r5, r2, r3                     # Combine address and flags
    str     r5, [r1], #4
    add     r2, r2, #0x100000              # Next 1MB
    subs    r4, r4, #1
    bne     ._mmu_section_loop

    # Set TTBR0 (Translation Table Base Register)
    ldr     r0, =_page_table
    mcr     p15, 0, r0, c2, c0, 0

    # Set Domain Access Control (client mode for domain 0)
    mov     r0, #0x01                      # Domain 0 = client
    mcr     p15, 0, r0, c3, c0, 0

    # Return
    mov     pc, r8

# -----------------------------------------------------------------------------
# _mmu_enable
# -----------
# Enable MMU and caches.
# -----------------------------------------------------------------------------
_mmu_enable:
    # Invalidate TLB
    mov     r0, #0
    mcr     p15, 0, r0, c8, c7, 0          # Invalidate entire TLB

    # Data synchronization barrier
    mcr     p15, 0, r0, c7, c10, 4

    # Read Control Register
    mrc     p15, 0, r0, c1, c0, 0

    # Enable MMU, D-cache, I-cache
    orr     r0, r0, #0x0001                # M bit - MMU enable
    orr     r0, r0, #0x0004                # C bit - Data cache enable
    orr     r0, r0, #0x1000                # I bit - Instruction cache enable

    # Write Control Register
    mcr     p15, 0, r0, c1, c0, 0

    # Instruction synchronization barrier
    mcr     p15, 0, r0, c7, c5, 4

    mov     pc, lr

# =============================================================================
# DATA
# =============================================================================

.section .rodata

_pesan_banner:
    .asciz "\r\nPIGURA OS v1.0 - ARM32 Boot\r\n"
    .asciz "Initializing kernel...\r\n"

# =============================================================================
# BSS
# =============================================================================

.section .bss

.align 12                         # 4KB aligned
_stack_bottom:
    .skip UKURAN_STACK
_stack_top:

.align 14                         # 16KB aligned for page table
_page_table:
    .skip 16384                    # 4096 entries x 4 bytes

_bss_start:
    .skip 1
_bss_end:
