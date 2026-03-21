# =============================================================================
# PIGURA OS - BOOT_ARM64.S
# ========================
# Boot code untuk arsitektur ARM64 (AArch64).
#
# Berkas ini berisi kode boot awal yang dieksekusi setelah bootloader.
# Kode ini melakukan inisialisasi dasar dan melompat ke kernel_main.
#
# Arsitektur: ARM64/AArch64 (64-bit)
# Versi: 1.0
# =============================================================================

# =============================================================================
# SECTIONS
# =============================================================================

.section .text.boot
.global _start
.global _halt

# =============================================================================
# ENTRY POINT
# =============================================================================

/*
 * _start
 * ------
 * Entry point utama untuk ARM64.
 * Dipanggil oleh bootloader (U-Boot, EFI, atau bare metal).
 *
 * Input:
 *   x0 = DTB address (jika ada)
 *   x1-x3 = reserved
 */
_start:
    /* Disable interrupts */
    msr     DAIFSet, #0xF

    /* Check current EL */
    mrs     x5, CurrentEL
    lsr     x5, x5, #2
    cmp     x5, #3
    b.eq    el3_setup
    cmp     x5, #2
    b.eq    el2_setup
    cmp     x5, #1
    b.eq    el1_setup
    b       .

/*
 * el3_setup
 * ---------
 * Setup dari EL3 (Secure Monitor).
 */
el3_setup:
    /* Get EL3 base address */
    mrs     x5, SCR_EL3

    /* Set to non-secure, lower EL is AArch64 */
    orr     x5, x5, #0xF          /* RW, NS, IRQ, FIQ bits */
    msr     SCR_EL3, x5

    /* Set SPSR for EL2h */
    mov     x5, #0x9              /* EL2h */
    msr     SPSR_EL3, x5

    /* Set ELR to EL2 entry */
    ldr     x5, =el2_setup
    msr     ELR_EL3, x5
    eret

/*
 * el2_setup
 * ---------
 * Setup dari EL2 (Hypervisor).
 */
el2_setup:
    /* Check if we need to drop to EL1 */

    /* Clear HCR_EL2 to disable hypervisor */
    msr     HCR_EL2, xzr

    /* Set SPSR for EL1h */
    mov     x5, #0x5              /* EL1h */
    msr     SPSR_EL2, x5

    /* Set ELR to EL1 entry */
    ldr     x5, =el1_setup
    msr     ELR_EL2, x5
    eret

/*
 * el1_setup
 * ---------
 * Setup dari EL1 (Kernel).
 */
el1_setup:
    /* Save DTB pointer */
    mov     x20, x0

    /* Setup stack pointer */
    ldr     x0, =_stack_top
    mov     sp, x0

    /* Clear BSS */
    ldr     x0, =_bss_start
    ldr     x1, =_bss_end
    sub     x1, x1, x0
    cbz     x1, 2f
1:
    str     xzr, [x0], #8
    subs    x1, x1, #8
    b.ne    1b
2:

    /* Setup exception vector */
    ldr     x0, =exception_vectors
    msr     VBAR_EL1, x0

    /* Setup TTBR0 (will be done by MMU init) */
    /* For now, identity mapping */

    /* Setup TCR_EL1 */
    /* 48-bit VA, 4KB granule, inner shareable, write-back cacheable */
    ldr     x0, =0x10B5403510
    msr     TCR_EL1, x0

    /* Setup MAIR_EL1 */
    ldr     x0, =0x00000000FF004C04
    msr     MAIR_EL1, x0

    /* Enable FP/SIMD */
    mov     x0, #0x3
    msr     CPACR_EL1, x0

    /* Invalidate caches */
    ic      IALLUIS
    tlbi    VMALLE1IS
    dsb     sy
    isb

    /* Enable MMU and caches */
    mrs     x0, SCTLR_EL1
    orr     x0, x0, #0x1          /* M: MMU enable */
    orr     x0, x0, #0x4          /* C: Data cache */
    orr     x0, x0, #0x1000       /* I: Instruction cache */
    msr     SCTLR_EL1, x0
    isb

    /* Restore DTB pointer and call kernel_main */
    mov     x0, x20
    mov     x1, #0                /* Reserved */
    mov     x2, #0                /* Reserved */
    mov     x3, #0                /* Reserved */

    /* Jump to kernel_main */
    bl      kernel_main

    /* Should never reach here */
    b       _halt

/*
 * _halt
 * -----
 * Halt CPU.
 */
_halt:
    wfi
    b       _halt

# =============================================================================
# EXCEPTION VECTORS
# =============================================================================

.section .text
.align 11

/*
 * exception_vectors
 * -----------------
 * Exception vector table untuk ARM64.
 * Setiap vector harus 128-byte aligned.
 */
.global exception_vectors
exception_vectors:
    /* Current EL with SP0 */
    .align 7
    b       sync_handler_sp0
    .align 7
    b       irq_handler_sp0
    .align 7
    b       fiq_handler_sp0
    .align 7
    b       serror_handler_sp0

    /* Current EL with SPx */
    .align 7
    b       sync_handler_current
    .align 7
    b       irq_handler_current
    .align 7
    b       fiq_handler_current
    .align 7
    b       serror_handler_current

    /* Lower EL using AArch64 */
    .align 7
    b       sync_handler_lower_aarch64
    .align 7
    b       irq_handler_lower_aarch64
    .align 7
    b       fiq_handler_lower_aarch64
    .align 7
    b       serror_handler_lower_aarch64

    /* Lower EL using AArch32 */
    .align 7
    b       sync_handler_lower_aarch32
    .align 7
    b       irq_handler_lower_aarch32
    .align 7
    b       fiq_handler_lower_aarch32
    .align 7
    b       serror_handler_lower_aarch32

# =============================================================================
# EXCEPTION HANDLERS
# =============================================================================

/*
 * Helper macro untuk save/restore context
 */
.macro SAVE_CONTEXT
    sub     sp, sp, #272
    stp     x0, x1, [sp, #0]
    stp     x2, x3, [sp, #16]
    stp     x4, x5, [sp, #32]
    stp     x6, x7, [sp, #48]
    stp     x8, x9, [sp, #64]
    stp     x10, x11, [sp, #80]
    stp     x12, x13, [sp, #96]
    stp     x14, x15, [sp, #112]
    stp     x16, x17, [sp, #128]
    stp     x18, x19, [sp, #144]
    stp     x20, x21, [sp, #160]
    stp     x22, x23, [sp, #176]
    stp     x24, x25, [sp, #192]
    stp     x26, x27, [sp, #208]
    stp     x28, x29, [sp, #224]
    mrs     x0, ELR_EL1
    mrs     x1, SPSR_EL1
    stp     x0, x1, [sp, #240]
    mrs     x0, ESR_EL1
    mrs     x1, FAR_EL1
    stp     x0, x1, [sp, #256]
    mov     x0, sp
.endm

.macro RESTORE_CONTEXT
    ldp     x0, x1, [sp, #256]
    ldp     x2, x3, [sp, #240]
    msr     ELR_EL1, x2
    msr     SPSR_EL1, x3
    ldp     x0, x1, [sp, #0]
    ldp     x2, x3, [sp, #16]
    ldp     x4, x5, [sp, #32]
    ldp     x6, x7, [sp, #48]
    ldp     x8, x9, [sp, #64]
    ldp     x10, x11, [sp, #80]
    ldp     x12, x13, [sp, #96]
    ldp     x14, x15, [sp, #112]
    ldp     x16, x17, [sp, #128]
    ldp     x18, x19, [sp, #144]
    ldp     x20, x21, [sp, #160]
    ldp     x22, x23, [sp, #176]
    ldp     x24, x25, [sp, #192]
    ldp     x26, x27, [sp, #208]
    ldp     x28, x29, [sp, #224]
    add     sp, sp, #272
.endm

/*
 * SP0 handlers (should not happen)
 */
sync_handler_sp0:
irq_handler_sp0:
fiq_handler_sp0:
serror_handler_sp0:
    b       .

/*
 * Current EL handlers
 */
sync_handler_current:
    SAVE_CONTEXT
    bl      sync_handler_el1
    RESTORE_CONTEXT
    eret

irq_handler_current:
    SAVE_CONTEXT
    bl      irq_handler_el1
    RESTORE_CONTEXT
    eret

fiq_handler_current:
    SAVE_CONTEXT
    bl      fiq_handler_el1
    RESTORE_CONTEXT
    eret

serror_handler_current:
    SAVE_CONTEXT
    bl      serror_handler_el1
    RESTORE_CONTEXT
    eret

/*
 * Lower EL AArch64 handlers
 */
sync_handler_lower_aarch64:
    SAVE_CONTEXT
    bl      sync_handler_el0
    RESTORE_CONTEXT
    eret

irq_handler_lower_aarch64:
    SAVE_CONTEXT
    bl      irq_handler_el0
    RESTORE_CONTEXT
    eret

fiq_handler_lower_aarch64:
    SAVE_CONTEXT
    bl      fiq_handler_el0
    RESTORE_CONTEXT
    eret

serror_handler_lower_aarch64:
    SAVE_CONTEXT
    bl      serror_handler_el0
    RESTORE_CONTEXT
    eret

/*
 * Lower EL AArch32 handlers (not supported)
 */
sync_handler_lower_aarch32:
irq_handler_lower_aarch32:
fiq_handler_lower_aarch32:
serror_handler_lower_aarch32:
    b       .

# =============================================================================
# DATA SECTION
# =============================================================================

.section .bss
.align 4

_stack_bottom:
    .space  65536                  /* 64KB stack */
_stack_top:
