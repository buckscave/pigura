# =============================================================================
# PIGURA OS - BOOT CODE ARMv7 (AArch32)
# =============================================================================
#
# Berkas ini berisi kode boot untuk arsitektur ARMv7 (Cortex-A series).
# Dirancang untuk boot dari U-Boot atau bare metal pada board ARMv7.
#
# Fungsi utama:
#   1. Setup vector table
#   2. Inisialisasi FPU/VFP
#   3. Setup MMU dan page tables
#   4. Lompat ke kernel C entry
#
# Arsitektur: ARMv7 (AArch32)
# Versi: 1.0
# =============================================================================

# =============================================================================
# KONSTANTA
# =============================================================================

# Alamat memory
.set ALAMAT_KERNEL,        0x40008000    # Kernel load address
.set ALAMAT_STACK,         0x40004000    # Stack bottom
.set UKURAN_STACK,         0x4000        # 16 KB stack

# UART PL011 (QEMU virt machine)
.set UART0_BASE,           0x09000000    # UART0 base address
.set UART0_DR,             0x00          # Data Register
.set UART0_FR,             0x18          # Flag Register
.set UART0_IBRD,           0x24          # Integer Baud Rate
.set UART0_FBRD,           0x28          # Fractional Baud Rate
.set UART0_LCRH,           0x2C          # Line Control
.set UART0_CR,             0x30          # Control Register

# UART flags
.set UART_FR_TXFF,         0x20          # Transmit FIFO full

# UART control bits
.set UART_CR_UARTEN,       0x01          # UART enable
.set UART_CR_TXE,          0x08          # Transmit enable
.set UART_CR_RXE,          0x04          # Receive enable
.set UART_LCRH_FEN,        0x10          # FIFO enable
.set UART_LCRH_WLEN_8,     0x60          # 8-bit word

# CP15 registers
.set CP15_SCTLR,           0, c1, c0, 0  # System Control Register
.set CP15_TTBR0,           0, c2, c0, 0  # Translation Table Base 0
.set CP15_TTBR1,           0, c2, c0, 1  # Translation Table Base 1
.set CP15_TTBCR,           0, c2, c0, 2  # Translation Table Base Control
.set CP15_DACR,            0, c3, c0, 0  # Domain Access Control
.set CP15_MAIR0,           0, c10, c2, 0 # Memory Attribute Indirection
.set CP15_MAIR1,           0, c10, c2, 1 # Memory Attribute Indirection
.set CP15_VBAR,            0, c12, c0, 0 # Vector Base Address Register

# SCTLR bits
.set SCTLR_MMU,            0x0001        # MMU enable
.set SCTLR_DCACHE,         0x0004        # Data cache enable
.set SCTLR_ICACHE,         0x1000        # Instruction cache enable
.set SCTLR_BRANCH_PRED,    0x0800        # Branch prediction

# Page table flags (short descriptor format)
.set PT_SECTION,           0x02          # Section descriptor
.set PT_VALID,             0x01          # Valid bit
.set PT_CACHEABLE,         0x08          # Cacheable
.set PT_BUFFERABLE,        0x04          # Bufferable
.set PT_AP_RW,             (1 << 10)     # AP[2:0] = 001 (RW for all)
.set PT_TEX0,              (1 << 12)     # TEX[0]
.set PT_DOMAIN,            (0 << 5)      # Domain 0

# =============================================================================
# ENTRY POINT
# =============================================================================

.section .text.boot
.global _start
.global _kernel_entry

_start:
    # -------------------------------------------------------------------------
    # Disable interrupts
    # -------------------------------------------------------------------------
    cpsid   aif                             # Disable IRQ, FIQ, async abort

    # -------------------------------------------------------------------------
    # Set CPU mode to SVC (supervisor) with IRQ/FIQ disabled
    # -------------------------------------------------------------------------
    mrs     r0, cpsr
    bic     r0, r0, #0x1F                   # Clear mode bits
    orr     r0, r0, #0xD3                   # SVC mode + disable interrupts
    msr     cpsr_c, r0

    # -------------------------------------------------------------------------
    # Check if primary CPU (for SMP systems)
    # -------------------------------------------------------------------------
    mrc     p15, 0, r0, c0, c0, 5           # Read MPIDR
    ands    r0, r0, #3                      # Extract CPU number
    bne     ._cpu_halt                      # Halt secondary CPUs

    # -------------------------------------------------------------------------
    # Setup vector table
    # -------------------------------------------------------------------------
    ldr     r0, =_exception_vectors
    mcr     CP15_VBAR, r0                   # Set VBAR

    # -------------------------------------------------------------------------
    # Setup stack
    # -------------------------------------------------------------------------
    ldr     sp, =_stack_top

    # -------------------------------------------------------------------------
    # Initialize FPU/VFP
    # -------------------------------------------------------------------------
    bl      _vfp_init

    # -------------------------------------------------------------------------
    # Zero BSS section
    # -------------------------------------------------------------------------
    ldr     r0, =_bss_start
    ldr     r1, =_bss_end
    mov     r2, #0
._bss_loop:
    cmp     r0, r1
    bge     ._bss_done
    stmia   r0!, {r2}
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
    # Setup page tables
    # -------------------------------------------------------------------------
    bl      _mmu_setup

    # -------------------------------------------------------------------------
    # Enable MMU and caches
    # -------------------------------------------------------------------------
    bl      _mmu_enable

    # -------------------------------------------------------------------------
    # Jump to C kernel entry
    # -------------------------------------------------------------------------
    ldr     r0, =_kernel_entry
    bx      r0

._cpu_halt:
    wfi
    b       ._cpu_halt

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

    # Set baud rate (115200 @ 24MHz)
    mov     r1, #13
    str     r1, [r0, #UART0_IBRD]
    mov     r1, #0
    str     r1, [r0, #UART0_FBRD]

    # Line control: 8-bit, FIFO
    mov     r1, #(UART_LCRH_WLEN_8 | UART_LCRH_FEN)
    str     r1, [r0, #UART0_LCRH]

    # Enable UART, TX, RX
    mov     r1, #(UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE)
    str     r1, [r0, #UART0_CR]

    bx      lr

# -----------------------------------------------------------------------------
# _uart_putc
# ----------
# Output single character to UART.
# Parameters: r0 = character
# -----------------------------------------------------------------------------
_uart_putc:
    push    {r1, r2, lr}

    mov     r2, r0                          # Save character
    ldr     r1, =UART0_BASE

._uart_putc_wait:
    ldr     r0, [r1, #UART0_FR]
    tst     r0, #UART_FR_TXFF
    bne     ._uart_putc_wait

    str     r2, [r1, #UART0_DR]

    pop     {r1, r2, pc}

# -----------------------------------------------------------------------------
# _uart_puts
# ----------
# Output null-terminated string to UART.
# Parameters: r0 = pointer to string
# -----------------------------------------------------------------------------
_uart_puts:
    push    {r1, r2, lr}

    mov     r2, r0                          # String pointer

._uart_puts_loop:
    ldrb    r0, [r2], #1
    cmp     r0, #0
    beq     ._uart_puts_done

    bl      _uart_putc
    b       ._uart_puts_loop

._uart_puts_done:
    pop     {r1, r2, pc}

# -----------------------------------------------------------------------------
# _uart_puthex
# ------------
# Output 32-bit value in hexadecimal.
# Parameters: r0 = value
# -----------------------------------------------------------------------------
_uart_puthex:
    push    {r1-r4, lr}

    mov     r3, r0                          # Save value
    mov     r4, #8                          # 8 hex digits

._uart_puthex_loop:
    sub     r4, r4, #1

    # Extract nibble
    mov     r0, r3, lsr #(r4 * 4)
    and     r0, r0, #0xF

    # Convert to ASCII
    cmp     r0, #10
    addlt   r0, r0, #'0'
    addge   r0, r0, #('A' - 10)

    bl      _uart_putc

    cmp     r4, #0
    bne     ._uart_puthex_loop

    pop     {r1-r4, pc}

# =============================================================================
# VFP INITIALIZATION
# =============================================================================

# -----------------------------------------------------------------------------
# _vfp_init
# ---------
# Initialize VFP/NEON unit.
# -----------------------------------------------------------------------------
_vfp_init:
    # Enable VFP access in CPACR
    mrc     p15, 0, r0, c1, c0, 2           # Read CPACR
    orr     r0, r0, #(0xF << 20)            # Enable CP10, CP11
    mcr     p15, 0, r0, c1, c0, 2           # Write CPACR

    # Enable VFP
    vmrs    r0, FPEXC
    orr     r0, r0, #(1 << 30)              # Enable bit
    vmsr    FPEXC, r0

    # Clear FPSCR
    mov     r0, #0
    vmsr    FPSCR, r0

    bx      lr

# =============================================================================
# MMU FUNCTIONS
# =============================================================================

# -----------------------------------------------------------------------------
# _mmu_setup
# ----------
# Setup page tables for identity mapping.
# Uses short descriptor format with 1 MB sections.
# -----------------------------------------------------------------------------
_mmu_setup:
    push    {r4-r8, lr}

    # Get page table base
    ldr     r0, =_page_table

    # Clear page table (16 KB for 4096 entries)
    mov     r1, r0
    mov     r2, #0
    mov     r3, #4096
._mmu_clear:
    str     r2, [r1], #4
    subs    r3, r3, #1
    bne     ._mmu_clear

    # Create identity mapping (first 1 GB)
    # Each entry maps 1 MB
    mov     r1, r0                          # Page table pointer
    mov     r2, #0                          # Physical address
    mov     r3, #(PT_SECTION | PT_VALID | PT_CACHEABLE | \
                 PT_BUFFERABLE | PT_AP_RW | PT_TEX0)
    mov     r4, #1024                       # 1024 entries (1 GB)

._mmu_section_loop:
    orr     r5, r2, r3                      # Combine PA and flags
    str     r5, [r1], #4
    add     r2, r2, #(1 << 20)              # Next 1 MB
    subs    r4, r4, #1
    bne     ._mmu_section_loop

    # Setup Domain Access Control (client mode for domain 0)
    mov     r0, #1
    mcr     CP15_DACR, r0

    # Set TTBR0
    ldr     r0, =_page_table
    mcr     CP15_TTBR0, r0

    # Set TTBCR (use TTBR0 only)
    mov     r0, #0
    mcr     CP15_TTBCR, r0

    pop     {r4-r8, pc}

# -----------------------------------------------------------------------------
# _mmu_enable
# -----------
# Enable MMU and caches.
# -----------------------------------------------------------------------------
_mmu_enable:
    # Invalidate TLB
    mov     r0, #0
    mcr     p15, 0, r0, c8, c7, 0           # TLBIALL
    dsb
    isb

    # Enable MMU, caches, branch prediction
    mrc     CP15_SCTLR, r0
    orr     r0, r0, #SCTLR_MMU
    orr     r0, r0, #SCTLR_DCACHE
    orr     r0, r0, #SCTLR_ICACHE
    orr     r0, r0, #SCTLR_BRANCH_PRED
    mcr     CP15_SCTLR, r0
    isb

    bx      lr

# =============================================================================
# EXCEPTION VECTORS
# =============================================================================

.section .text
.align 5                          # 32-byte aligned

.global _exception_vectors
_exception_vectors:
    b       ._exc_reset
    b       ._exc_undefined
    b       ._exc_swi
    b       ._exc_prefetch
    b       ._exc_data
    b       ._exc_unused
    b       ._exc_irq
    b       ._exc_fiq

._exc_reset:
    b       .

._exc_undefined:
    b       .

._exc_swi:
    b       .

._exc_prefetch:
    b       .

._exc_data:
    b       .

._exc_unused:
    b       .

._exc_irq:
    b       .

._exc_fiq:
    b       .

# =============================================================================
# DATA
# =============================================================================

.section .rodata

_pesan_banner:
    .asciz "\r\nPIGURA OS v1.0 - ARMv7 Boot\r\n"
    .asciz "Initializing kernel...\r\n"

# =============================================================================
# BSS
# =============================================================================

.section .bss

.align 14                         # 16 KB aligned
_page_table:
    .skip 16384                    # 16 KB page table

.align 4
_stack_bottom:
    .skip UKURAN_STACK
_stack_top:

_bss_start:
    .skip 1
_bss_end:
