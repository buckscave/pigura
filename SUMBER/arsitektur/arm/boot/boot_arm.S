# =============================================================================
# PIGURA OS - BOOT CODE ARM64 (AArch64)
# =============================================================================
#
# Berkas ini berisi kode boot untuk arsitektur ARM64 (AArch64).
# Dirancang untuk boot dari UEFI atau bare metal pada board ARM64.
#
# Fungsi utama:
#   1. Setup stack dan environment
#   2. Inisialisasi UART untuk output
#   3. Setup MMU dan page tables
#   4. Lompat ke kernel C entry
#
# Arsitektur: ARM64 (AArch64)
# Versi: 1.0
# =============================================================================

# =============================================================================
# KONSTANTA
# =============================================================================

# Alamat memory
.set ALAMAT_KERNEL,        0x40080000    # Kernel load address
.set ALAMAT_STACK,         0x40000000    # Stack bottom
.set UKURAN_STACK,         0x10000       # 64 KB stack

# UART PL011 (untuk Raspberry Pi 3/4 dan board ARM umum)
.set UART0_BASE,           0x09060000    # UART0 base (QEMU virt)
.set UART0_DR,             0x00          # Data Register
.set UART0_FR,             0x18          # Flag Register
.set UART0_IBRD,           0x24          # Integer Baud Rate
.set UART0_FBRD,           0x28          # Fractional Baud Rate
.set UART0_LCRH,           0x2C          # Line Control
.set UART0_CR,             0x30          # Control Register
.set UART0_IMSC,           0x38          # Interrupt Mask

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

# MMU
.set MMU_MAIR0,            0xBB          # Normal memory, Device memory
.set MMU_TCR_T0SZ,         16            # VA size (48-bit)
.set MMU_TCR_IRGN0,        0x04          # Inner write-back cacheable
.set MMU_TCR_ORGN0,        0x04          # Outer write-back cacheable
.set MMU_TCR_SH0,          0x03          # Inner shareable
.set MMU_TCR_TG0,          0x00          # 4 KB granule

# Page table flags
.set PT_VALID,             0x01          # Valid entry
.set PT_TABLE,             0x02          # Table descriptor
.set PT_PAGE,              0x01          # Page descriptor
.set PT_USER,              0x04          # User accessible
.set PT_RDONLY,            0x02          # Read-only
.set PT_AF,                0x10          # Access flag
.set PT_NG,                0x800         # Not global
.set PT_SH_INNER,          0x200         # Inner shareable
.set PT_AP_RW,             0x00          # Read/Write
.set PT_MA_NORMAL,         0x00          # Normal memory
.set PT_MA_DEVICE,         0x01          # Device memory

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
    msr     DAIFSet, #0xF                  # Disable all exceptions

    # -------------------------------------------------------------------------
    # Check processor ID and set up for primary CPU only
    # -------------------------------------------------------------------------
    mrs     x0, MPIDR_EL1                  # Get processor ID
    and     x0, x0, #3                     # Extract CPU number
    cbnz    x0, ._cpu_halt                 # Halt secondary CPUs

    # -------------------------------------------------------------------------
    # Setup stack
    # -------------------------------------------------------------------------
    ldr     x0, =_stack_top
    mov     sp, x0

    # -------------------------------------------------------------------------
    # Zero BSS section
    # -------------------------------------------------------------------------
    ldr     x0, =_bss_start
    ldr     x1, =_bss_end
._bss_loop:
    cmp     x0, x1
    b.ge    ._bss_done
    str     xzr, [x0], #8
    b       ._bss_loop
._bss_done:

    # -------------------------------------------------------------------------
    # Initialize UART for early output
    # -------------------------------------------------------------------------
    bl      _uart_init

    # -------------------------------------------------------------------------
    # Print boot banner
    # -------------------------------------------------------------------------
    ldr     x0, =_pesan_banner
    bl      _uart_puts

    # -------------------------------------------------------------------------
    # Setup page tables (identity mapping + kernel mapping)
    # -------------------------------------------------------------------------
    bl      _mmu_setup

    # -------------------------------------------------------------------------
    # Enable MMU
    # -------------------------------------------------------------------------
    bl      _mmu_enable

    # -------------------------------------------------------------------------
    # Jump to C kernel entry
    # -------------------------------------------------------------------------
    ldr     x0, =_kernel_entry
    br      x0

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
    ldr     x0, =UART0_BASE

    # Disable UART
    str     wzr, [x0, #UART0_CR]

    # Wait for TX FIFO to empty
._uart_wait_tx:
    ldr     w1, [x0, #UART0_FR]
    tst     w1, #UART_FR_TXFF
    b.ne    ._uart_wait_tx

    # Set baud rate (115200 @ 24MHz clock)
    # IBRD = 24MHz / (16 * 115200) = 13
    mov     w1, #13
    str     w1, [x0, #UART0_IBRD]

    # FBRD = 0 (fractional part)
    str     wzr, [x0, #UART0_FBRD]

    # Set line control: 8-bit, FIFO enabled
    mov     w1, #(UART_LCRH_WLEN_8 | UART_LCRH_FEN)
    str     w1, [x0, #UART0_LCRH]

    # Enable UART, TX, RX
    mov     w1, #(UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE)
    str     w1, [x0, #UART0_CR]

    ret

# -----------------------------------------------------------------------------
# _uart_putc
# ----------
# Output single character to UART.
# Parameters: x0 = character
# -----------------------------------------------------------------------------
_uart_putc:
    # Save character
    mov     w2, w0

    # Load UART base
    ldr     x1, =UART0_BASE

    # Wait for TX FIFO not full
._uart_putc_wait:
    ldr     w3, [x1, #UART0_FR]
    tst     w3, #UART_FR_TXFF
    b.ne    ._uart_putc_wait

    # Write character
    str     w2, [x1, #UART0_DR]

    ret

# -----------------------------------------------------------------------------
# _uart_puts
# ----------
# Output null-terminated string to UART.
# Parameters: x0 = pointer to string
# -----------------------------------------------------------------------------
_uart_puts:
    mov     x2, x0                         # String pointer

._uart_puts_loop:
    ldrb    w0, [x2], #1                   # Load byte and increment
    cbz     w0, ._uart_puts_done           # Check for null terminator

    bl      _uart_putc
    b       ._uart_puts_loop

._uart_puts_done:
    ret

# -----------------------------------------------------------------------------
# _uart_puthex
# ------------
# Output 64-bit value in hexadecimal format.
# Parameters: x0 = value to print
# -----------------------------------------------------------------------------
_uart_puthex:
    mov     x3, x0                         # Save value
    mov     x4, #16                        # Digit count

._uart_puthex_loop:
    sub     x4, x4, #1                     # Decrement counter

    # Extract nibble
    lsr     x0, x3, x4
    and     x0, x0, #0xF

    # Convert to ASCII
    cmp     x0, #10
    b.lt    ._uart_puthex_digit
    add     x0, x0, #('A' - '0' - 10)

._uart_puthex_digit:
    add     x0, x0, #'0'

    bl      _uart_putc

    cbnz    x4, ._uart_puthex_loop

    ret

# =============================================================================
# MMU FUNCTIONS
# =============================================================================

# -----------------------------------------------------------------------------
# _mmu_setup
# ----------
# Setup page tables for identity mapping and kernel mapping.
# -----------------------------------------------------------------------------
_mmu_setup:
    # Save return address
    mov     x8, x30

    # Get page table base
    ldr     x0, =_pgd

    # Clear page tables (3 levels x 4KB)
    mov     x1, x0
    mov     x2, #0
    mov     x3, #(4096 * 3)
._mmu_clear:
    str     x2, [x1], #8
    subs    x3, x3, #8
    b.ne    ._mmu_clear

    # Create identity mapping (first 1 GB)
    # PGD[0] -> PUD
    ldr     x1, =_pud
    orr     x0, x1, #PT_VALID | PT_TABLE
    ldr     x1, =_pgd
    str     x0, [x1]

    # PUD[0] -> PMD (identity mapping)
    ldr     x0, =_pmd
    orr     x0, x0, #PT_VALID | PT_TABLE
    ldr     x1, =_pud
    str     x0, [x1]

    # PMD entries (2 MB blocks, first 1 GB)
    ldr     x0, =_pmd
    mov     x1, #(PT_VALID | PT_AF | PT_SH_INNER | PT_AP_RW | (0 << 2))
    mov     x2, #0                         # Physical address
    mov     x3, #512                       # 512 entries (1 GB)

._mmu_pmd_loop:
    orr     x4, x1, x2
    str     x4, [x0], #8
    add     x2, x2, #(2 << 20)             # Next 2 MB
    subs    x3, x3, #1
    b.ne    ._mmu_pmd_loop

    # Setup MAIR (Memory Attribute Indirection Register)
    mov     x0, #MMU_MAIR0
    msr     MAIR_EL1, x0

    # Setup TCR (Translation Control Register)
    mov     x0, #((MMU_TCR_T0SZ << 0) | (MMU_TCR_IRGN0 << 8) | \
                  (MMU_TCR_ORGN0 << 10) | (MMU_TCR_SH0 << 12) | \
                  (MMU_TCR_TG0 << 14))
    msr     TCR_EL1, x0

    # Set TTBR0 (Translation Table Base Register)
    ldr     x0, =_pgd
    msr     TTBR0_EL1, x0

    # Return
    mov     x30, x8
    ret

# -----------------------------------------------------------------------------
# _mmu_enable
# -----------
# Enable MMU and caches.
# -----------------------------------------------------------------------------
_mmu_enable:
    # Invalidate all TLB entries
    tlbi    vmalle1is
    dsb     sy
    isb

    # Enable caches and MMU in SCTLR
    mrs     x0, SCTLR_EL1
    orr     x0, x0, #(1 << 0)              # MMU enable
    orr     x0, x0, #(1 << 2)              # D-cache enable
    orr     x0, x0, #(1 << 12)             # I-cache enable
    msr     SCTLR_EL1, x0
    isb

    ret

# =============================================================================
# EXCEPTION VECTORS
# =============================================================================

.section .text
.align 11                          # Must be aligned to 2KB

.global _exception_vectors
_exception_vectors:
    # Current EL with SP0
    b       ._exc_current_sp0
    b       ._exc_current_sp0
    b       ._exc_current_sp0
    b       ._exc_current_sp0

    # Current EL with SPx
    b       ._exc_current_spx
    b       ._exc_current_spx
    b       ._exc_current_spx
    b       ._exc_current_spx

    # Lower EL using AArch64
    b       ._exc_lower_aarch64
    b       ._exc_lower_aarch64
    b       ._exc_lower_aarch64
    b       ._exc_lower_aarch64

    # Lower EL using AArch32
    b       ._exc_lower_aarch32
    b       ._exc_lower_aarch32
    b       ._exc_lower_aarch32
    b       ._exc_lower_aarch32

._exc_current_sp0:
    b       .

._exc_current_spx:
    b       .

._exc_lower_aarch64:
    b       .

._exc_lower_aarch32:
    b       .

# =============================================================================
# DATA
# =============================================================================

.section .rodata

_pesan_banner:
    .asciz "\r\nPIGURA OS v1.0 - ARM64 Boot\r\n"
    .asciz "Initializing kernel...\r\n"

# =============================================================================
# BSS
# =============================================================================

.section .bss

.align 12                         # 4KB aligned
_stack_bottom:
    .skip UKURAN_STACK
_stack_top:

.align 12
_pgd:
    .skip 4096                      # Page Global Directory (L0)
_pud:
    .skip 4096                      # Page Upper Directory (L1)
_pmd:
    .skip 4096                      # Page Middle Directory (L2)
_pmd_high:
    .skip 4096                      # PMD for higher half

_bss_start:
    .skip 1
_bss_end:
