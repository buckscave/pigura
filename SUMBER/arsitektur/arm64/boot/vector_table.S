# =============================================================================
# PIGURA OS - VECTOR_TABLE.S
# ==========================
# Exception vector table untuk arsitektur ARM64 (AArch64).
#
# Berkas ini berisi exception vector table dan handler dasar
# untuk berbagai jenis exception pada ARM64.
#
# Arsitektur: ARM64/AArch64 (64-bit)
# Versi: 1.0
# =============================================================================

# =============================================================================
# CONSTANTS
# =============================================================================

/* Exception classes */
.set EC_UNKNOWN,       0x00
.set EC_WFI_WFE,       0x01
.set EC_CP15_MRC_MCR,  0x03
.set EC_CP15_MRRC_MCRR, 0x04
.set EC_CP14_MRC_MCR,  0x05
.set EC_CP14_LDC_STC,  0x06
.set EC_FP_SIMD,       0x07
.set EC_CP10_MRC,      0x08
.set EC_CP14_MRRC,     0x0C
.set EC_ILL_EXEC,      0x0E
.set EC_SVC32,         0x11
.set EC_HVC32,         0x12
.set EC_SMC32,         0x13
.set EC_SVC64,         0x15
.set EC_HVC64,         0x16
.set EC_SMC64,         0x17
.set EC_SYS_REG,       0x18
.set EC_IABT_LOW,      0x20
.set EC_IABT_CUR,      0x21
.set EC_PC_ALIGN,      0x22
.set EC_DABT_LOW,      0x24
.set EC_DABT_CUR,      0x25
.set EC_SP_ALIGN,      0x26
.set EC_FP32,          0x28
.set EC_FP64,          0x2C
.set EC_SERROR,        0x2F
.set EC_BRK,           0x3C

/* SPSR bits */
.set SPSR_EL0t,        0x0000
.set SPSR_EL1t,        0x0004
.set SPSR_EL1h,        0x0005
.set SPSR_EL2t,        0x0008
.set SPSR_EL2h,        0x0009
.set SPSR_EL3t,        0x000C
.set SPSR_EL3h,        0x000D
.set SPSR_D,           (1 << 9)
.set SPSR_A,           (1 << 8)
.set SPSR_I,           (1 << 7)
.set SPSR_F,           (1 << 6)

/* Exception frame size: 31 registers + SP + PC + SPSR + ESR + FAR */
.set EXC_FRAME_SIZE,   (31 * 8 + 6 * 8)

# =============================================================================
# SECTIONS
# =============================================================================

.section .text
.align 11

# =============================================================================
# EXCEPTION VECTOR TABLE
# =============================================================================

/*
 * vector_table
 * ------------
 * Main exception vector table.
 * Setiap vector adalah 128 bytes (32 instructions) dan harus
 * aligned ke 2048 bytes (2KB).
 *
 * Layout:
 *   0x000-0x07F: Current EL with SP0
 *   0x080-0x0FF: Current EL with SPx
 *   0x100-0x17F: Lower EL using AArch64
 *   0x180-0x1FF: Lower EL using AArch32
 *
 * Dalam setiap 128-byte block:
 *   0x00: Synchronous exception
 *   0x20: IRQ
 *   0x40: FIQ
 *   0x60: SError
 */
.global vector_table
vector_table:
    /* ========================================================== */
    /* Current EL with SP0 (should not happen in kernel)          */
    /* ========================================================== */
    .align 7
    b       exc_sp0_sync
    .align 7
    b       exc_sp0_irq
    .align 7
    b       exc_sp0_fiq
    .align 7
    b       exc_sp0_serror

    /* ========================================================== */
    /* Current EL with SPx (kernel exceptions)                    */
    /* ========================================================== */
    .align 7
    b       exc_kernel_sync
    .align 7
    b       exc_kernel_irq
    .align 7
    b       exc_kernel_fiq
    .align 7
    b       exc_kernel_serror

    /* ========================================================== */
    /* Lower EL using AArch64 (user exceptions)                   */
    /* ========================================================== */
    .align 7
    b       exc_user_sync
    .align 7
    b       exc_user_irq
    .align 7
    b       exc_user_fiq
    .align 7
    b       exc_user_serror

    /* ========================================================== */
    /* Lower EL using AArch32 (not supported)                     */
    /* ========================================================== */
    .align 7
    b       exc_aarch32_sync
    .align 7
    b       exc_aarch32_irq
    .align 7
    b       exc_aarch32_fiq
    .align 7
    b       exc_aarch32_serror

# =============================================================================
# EXCEPTION SAVE/RESTORE MACROS
# =============================================================================

/*
 * SAVE_ALL
 * --------
 * Simpan semua register ke stack.
 * Mengalokasikan exception frame dan menyimpan:
 *   x0-x30, sp, pc (elr), spsr, esr, far
 */
.macro SAVE_ALL
    sub     sp, sp, #EXC_FRAME_SIZE

    /* Save general purpose registers */
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

    /* Save x30 (LR) */
    str     x30, [sp, #240]

    /* Save SP_EL1 (original SP) */
    add     x0, sp, #EXC_FRAME_SIZE
    str     x0, [sp, #248]

    /* Save ELR_EL1 (PC) and SPSR_EL1 */
    mrs     x0, ELR_EL1
    mrs     x1, SPSR_EL1
    stp     x0, x1, [sp, #256]

    /* Save ESR_EL1 and FAR_EL1 */
    mrs     x0, ESR_EL1
    mrs     x1, FAR_EL1
    stp     x0, x1, [sp, #272]

    /* Restore x0 for handler */
    ldr     x0, [sp, #0]
.endm

/*
 * RESTORE_ALL
 * -----------
 * Restore semua register dari stack.
 */
.macro RESTORE_ALL
    /* Restore ELR_EL1 and SPSR_EL1 */
    ldp     x0, x1, [sp, #256]
    msr     ELR_EL1, x0
    msr     SPSR_EL1, x1

    /* Restore general purpose registers */
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

    /* Restore x30 (LR) */
    ldr     x30, [sp, #240]

    /* Deallocate exception frame */
    add     sp, sp, #EXC_FRAME_SIZE
.endm

# =============================================================================
# SP0 EXCEPTION HANDLERS (INVALID)
# =============================================================================

exc_sp0_sync:
exc_sp0_irq:
exc_sp0_fiq:
exc_sp0_serror:
    /* This should never happen in kernel */
    wfi
    b       .

# =============================================================================
# KERNEL EXCEPTION HANDLERS (EL1)
# =============================================================================

exc_kernel_sync:
    SAVE_ALL
    mov     x0, sp
    bl      exception_sync_handler
    RESTORE_ALL
    eret

exc_kernel_irq:
    SAVE_ALL
    mov     x0, sp
    bl      exception_irq_handler
    RESTORE_ALL
    eret

exc_kernel_fiq:
    SAVE_ALL
    mov     x0, sp
    bl      exception_fiq_handler
    RESTORE_ALL
    eret

exc_kernel_serror:
    SAVE_ALL
    mov     x0, sp
    bl      exception_serror_handler
    RESTORE_ALL
    eret

# =============================================================================
# USER EXCEPTION HANDLERS (EL0 -> EL1)
# =============================================================================

exc_user_sync:
    SAVE_ALL
    mov     x0, sp
    bl      exception_user_sync_handler
    RESTORE_ALL
    eret

exc_user_irq:
    SAVE_ALL
    mov     x0, sp
    bl      exception_user_irq_handler
    RESTORE_ALL
    eret

exc_user_fiq:
    SAVE_ALL
    mov     x0, sp
    bl      exception_user_fiq_handler
    RESTORE_ALL
    eret

exc_user_serror:
    SAVE_ALL
    mov     x0, sp
    bl      exception_user_serror_handler
    RESTORE_ALL
    eret

# =============================================================================
# AARCH32 EXCEPTION HANDLERS (NOT SUPPORTED)
# =============================================================================

exc_aarch32_sync:
exc_aarch32_irq:
exc_aarch32_fiq:
exc_aarch32_serror:
    /* AArch32 not supported */
    wfi
    b       .

# =============================================================================
# C HANDLER STUBS
# =============================================================================

.section .text
.global exception_sync_handler
.global exception_irq_handler
.global exception_fiq_handler
.global exception_serror_handler
.global exception_user_sync_handler
.global exception_user_irq_handler
.global exception_user_fiq_handler
.global exception_user_serror_handler

/*
 * Default handlers - weak symbols
 * These will be overridden by actual implementations
 */
.weak exception_sync_handler
.weak exception_irq_handler
.weak exception_fiq_handler
.weak exception_serror_handler
.weak exception_user_sync_handler
.weak exception_user_irq_handler
.weak exception_user_fiq_handler
.weak exception_user_serror_handler

exception_sync_handler:
exception_irq_handler:
exception_fiq_handler:
exception_serror_handler:
exception_user_sync_handler:
exception_user_irq_handler:
exception_user_fiq_handler:
exception_user_serror_handler:
    ret
