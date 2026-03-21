# =============================================================================
# PIGURA OS - MMU_SETUP.S
# ========================
# Setup MMU untuk ARM (32-bit).
#
# Arsitektur: ARM (ARMv4/v5/v6 - 32-bit)
# Versi: 1.0
# =============================================================================

# =============================================================================
# KONSTANTA
# =============================================================================

# CP15 registers
.set CP15_SCTLR,       0, c1, c0, 0       # System Control Register
.set CP15_TTBR0,       0, c2, c0, 0       # Translation Table Base 0
.set CP15_TTBR1,       0, c2, c0, 1       # Translation Table Base 1
.set CP15_TTBCR,       0, c2, c0, 2       # Translation Table Base Control
.set CP15_DACR,        0, c3, c0, 0       # Domain Access Control

# SCTLR bits
.set SCTLR_MMU,        0x0001             # MMU enable
.set SCTLR_ALIGN,      0x0002             # Alignment check
.set SCTLR_DCACHE,     0x0004             # Data cache enable
.set SCTLR_ICACHE,     0x1000             # Instruction cache enable

# Page table flags
.set PT_SECTION,       0x02               # Section descriptor
.set PT_VALID,         0x01               # Valid bit
.set PT_CACHEABLE,     0x08               # Cacheable
.set PT_BUFFERABLE,    0x04               # Bufferable
.set PT_AP_RW,         (1 << 10)          # AP[2:0] = 001 (RW for all)
.set PT_DOMAIN,        (0 << 5)           # Domain 0

# Domain access control
.set DOMAIN_CLIENT,    0x01

# =============================================================================
# SECTIONS
# =============================================================================

.section .text
.align 4

# -----------------------------------------------------------------------------
# _mmu_setup
# -----------
# Setup page tables untuk identity mapping.
# -----------------------------------------------------------------------------
.global _mmu_setup
_mmu_setup:
    push    {r4-r8, lr}

    # Dapatkan alamat page table
    ldr     r0, =_page_table

    # Clear page table (16 KB for 4096 entries)
    mov     r1, r0
    mov     r2, #0
    mov     r3, #4096
_mmu_clear_loop:
    str     r2, [r1], #4
    subs    r3, r3, #1
    bne     _mmu_clear_loop

    # Create identity mapping (first 1 GB)
    # Each entry maps 1 MB
    mov     r1, r0                          # Page table pointer
    mov     r2, #0                          # Physical address
    mov     r3, #(PT_SECTION | PT_VALID | PT_CACHEABLE | \
                 PT_BUFFERABLE | PT_AP_RW)
    mov     r4, #1024                       # 1024 entries (1 GB)

_mmu_section_loop:
    orr     r5, r2, r3                      # Combine PA and flags
    str     r5, [r1], #4
    add     r2, r2, #(1 << 20)              # Next 1 MB
    subs    r4, r4, #1
    bne     _mmu_section_loop

    # Setup Domain Access Control (client mode for domain 0)
    mov     r0, #DOMAIN_CLIENT
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
# Enable MMU dan caches.
# -----------------------------------------------------------------------------
.global _mmu_enable
_mmu_enable:
    # Invalidate TLB
    mov     r0, #0
    mcr     p15, 0, r0, c8, c7, 0           # TLBIALL
    # Data synchronization barrier
    mcr     p15, 0, r0, c7, c10, 4

    # Enable MMU, caches
    mrc     CP15_SCTLR, r0
    orr     r0, r0, #SCTLR_MMU
    orr     r0, r0, #SCTLR_DCACHE
    orr     r0, r0, #SCTLR_ICACHE
    mcr     CP15_SCTLR, r0

    # Instruction synchronization barrier
    mcr     p15, 0, r0, c7, c5, 4

    bx      lr

# -----------------------------------------------------------------------------
# _mmu_disable
# ------------
# Disable MMU dan caches.
# -----------------------------------------------------------------------------
.global _mmu_disable
_mmu_disable:
    # Disable MMU dan caches
    mrc     CP15_SCTLR, r0
    bic     r0, r0, #SCTLR_MMU
    bic     r0, r0, #SCTLR_DCACHE
    bic     r0, r0, #SCTLR_ICACHE
    mcr     CP15_SCTLR, r0

    # Instruction synchronization barrier
    mcr     p15, 0, r0, c7, c5, 4

    # Invalidate TLB
    mov     r0, #0
    mcr     p15, 0, r0, c8, c7, 0           # TLBIALL
    mcr     p15, 0, r0, c7, c10, 4

    bx      lr

# -----------------------------------------------------------------------------
# _mmu_set_ttbr0
# --------------
# Set Translation Table Base Register 0.
# Parameters: r0 = page table address
# -----------------------------------------------------------------------------
.global _mmu_set_ttbr0
_mmu_set_ttbr0:
    push    {lr}

    # Invalidate TLB
    mov     r1, #0
    mcr     p15, 0, r1, c8, c7, 0           # TLBIALL
    mcr     p15, 0, r1, c7, c10, 4

    # Set TTBR0
    mcr     CP15_TTBR0, r0

    # Instruction synchronization barrier
    mcr     p15, 0, r1, c7, c5, 4

    pop     {pc}

# -----------------------------------------------------------------------------
# _mmu_get_ttbr0
# --------------
# Get Translation Table Base Register 0.
# Returns: r0 = page table address
# -----------------------------------------------------------------------------
.global _mmu_get_ttbr0
_mmu_get_ttbr0:
    mrc     CP15_TTBR0, r0
    bx      lr

# =============================================================================
# DATA
# =============================================================================

.section .bss

.align 14                         # 16 KB aligned
.global _page_table
_page_table:
    .skip 16384                    # 16 KB page table
