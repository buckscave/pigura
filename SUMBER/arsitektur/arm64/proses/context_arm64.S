# =============================================================================
# PIGURA OS - CONTEXT_ARM64.S
# ===========================
# Context switching untuk ARM64 (AArch64).
#
# Berkas ini berisi fungsi-fungsi assembly untuk menyimpan dan
# memulihkan context CPU pada context switch.
#
# Arsitektur: ARM64/AArch64 (64-bit)
# Versi: 1.0
# =============================================================================

# =============================================================================
# SECTIONS
# =============================================================================

.section .text
.global context_save
.global context_restore
.global context_switch
.global context_switch_to
.global context_fork

# =============================================================================
# CONSTANTS
# =============================================================================

/* Context structure offsets */
.set CTX_X19,      0
.set CTX_X20,      8
.set CTX_X21,      16
.set CTX_X22,      24
.set CTX_X23,      32
.set CTX_X24,      40
.set CTX_X25,      48
.set CTX_X26,      56
.set CTX_X27,      64
.set CTX_X28,      72
.set CTX_X29,      80
.set CTX_X30,      88
.set CTX_SP,       96
.set CTX_PC,       104
.set CTX_SPSR,     112
.set CTX_TPIDR,    120
.set CTX_SIZE,     128

# =============================================================================
# CONTEXT SAVE/RESTORE
# =============================================================================

/*
 * context_save
 * ------------
 * Simpan context CPU ke structure.
 *
 * Input:
 *   x0 = pointer ke context structure
 */
context_save:
    /* Save callee-saved registers */
    stp     x19, x20, [x0, #CTX_X19]
    stp     x21, x22, [x0, #CTX_X21]
    stp     x23, x24, [x0, #CTX_X23]
    stp     x25, x26, [x0, #CTX_X25]
    stp     x27, x28, [x0, #CTX_X27]
    stp     x29, x30, [x0, #CTX_X29]

    /* Save SP */
    mov     x1, sp
    str     x1, [x0, #CTX_SP]

    /* Save PC (return address) */
    /* PC is saved as the return address of this function + 1 instruction */
    adr     x1, 1f
    str     x1, [x0, #CTX_PC]

    /* Save SPSR */
    mrs     x1, SPSR_EL1
    str     x1, [x0, #CTX_SPSR]

    /* Save TPIDR_EL0 (TLS) */
    mrs     x1, TPIDR_EL0
    str     x1, [x0, #CTX_TPIDR]

1:
    mov     x0, #0
    ret

/*
 * context_restore
 * ---------------
 * Restore context CPU dari structure.
 *
 * Input:
 *   x0 = pointer ke context structure
 */
context_restore:
    /* Restore callee-saved registers */
    ldp     x19, x20, [x0, #CTX_X19]
    ldp     x21, x22, [x0, #CTX_X21]
    ldp     x23, x24, [x0, #CTX_X23]
    ldp     x25, x26, [x0, #CTX_X25]
    ldp     x27, x28, [x0, #CTX_X27]
    ldp     x29, x30, [x0, #CTX_X29]

    /* Restore SP */
    ldr     x1, [x0, #CTX_SP]
    mov     sp, x1

    /* Restore SPSR */
    ldr     x1, [x0, #CTX_SPSR]
    msr     SPSR_EL1, x1

    /* Restore TPIDR_EL0 */
    ldr     x1, [x0, #CTX_TPIDR]
    msr     TPIDR_EL0, x1

    /* Restore PC via ELR */
    ldr     x1, [x0, #CTX_PC]
    msr     ELR_EL1, x1

    isb
    mov     x0, #0
    eret

# =============================================================================
# CONTEXT SWITCH
# =============================================================================

/*
 * context_switch
 * --------------
 * Switch context dari current ke next.
 *
 * Input:
 *   x0 = pointer ke current context (to save)
 *   x1 = pointer ke next context (to restore)
 */
context_switch:
    /* Save current context */
    stp     x19, x20, [x0, #CTX_X19]
    stp     x21, x22, [x0, #CTX_X21]
    stp     x23, x24, [x0, #CTX_X23]
    stp     x25, x26, [x0, #CTX_X25]
    stp     x27, x28, [x0, #CTX_X27]
    stp     x29, x30, [x0, #CTX_X29]

    mov     x2, sp
    str     x2, [x0, #CTX_SP]

    mrs     x2, SPSR_EL1
    str     x2, [x0, #CTX_SPSR]

    mrs     x2, TPIDR_EL0
    str     x2, [x0, #CTX_TPIDR]

    /* Save return address as PC */
    adr     x2, 1f
    str     x2, [x0, #CTX_PC]

    /* Restore next context */
    ldp     x19, x20, [x1, #CTX_X19]
    ldp     x21, x22, [x1, #CTX_X21]
    ldp     x23, x24, [x1, #CTX_X23]
    ldp     x25, x26, [x1, #CTX_X25]
    ldp     x27, x28, [x1, #CTX_X27]
    ldp     x29, x30, [x1, #CTX_X29]

    ldr     x2, [x1, #CTX_SP]
    mov     sp, x2

    ldr     x2, [x1, #CTX_SPSR]
    msr     SPSR_EL1, x2

    ldr     x2, [x1, #CTX_TPIDR]
    msr     TPIDR_EL0, x2

    ldr     x2, [x1, #CTX_PC]
    msr     ELR_EL1, x2

    isb
    mov     x0, #0
    eret

1:
    /* Return here after eret from restored context */
    ret

/*
 * context_switch_to
 * -----------------
 * Switch ke context baru (tanpa save current).
 *
 * Input:
 *   x0 = pointer ke next context (to restore)
 */
context_switch_to:
    /* Restore all registers */
    ldp     x19, x20, [x0, #CTX_X19]
    ldp     x21, x22, [x0, #CTX_X21]
    ldp     x23, x24, [x0, #CTX_X23]
    ldp     x25, x26, [x0, #CTX_X25]
    ldp     x27, x28, [x0, #CTX_X27]
    ldp     x29, x30, [x0, #CTX_X29]

    ldr     x1, [x0, #CTX_SP]
    mov     sp, x1

    ldr     x1, [x0, #CTX_SPSR]
    msr     SPSR_EL1, x1

    ldr     x1, [x0, #CTX_TPIDR]
    msr     TPIDR_EL0, x1

    ldr     x1, [x0, #CTX_PC]
    msr     ELR_EL1, x1

    isb
    mov     x0, #0
    eret

# =============================================================================
# FORK SUPPORT
# =============================================================================

/*
 * context_fork
 * ------------
 * Fork context untuk proses baru.
 * Menyalin context parent ke child dan modifikasi untuk child.
 *
 * Input:
 *   x0 = pointer ke parent context
 *   x1 = pointer ke child context
 *   x2 = child stack pointer
 *
 * Output:
 *   x0 = 0 untuk parent, child PID untuk child
 */
context_fork:
    /* Copy parent context to child */
    ldp     x3, x4, [x0, #CTX_X19]
    stp     x3, x4, [x1, #CTX_X19]
    ldp     x3, x4, [x0, #CTX_X21]
    stp     x3, x4, [x1, #CTX_X21]
    ldp     x3, x4, [x0, #CTX_X23]
    stp     x3, x4, [x1, #CTX_X23]
    ldp     x3, x4, [x0, #CTX_X25]
    stp     x3, x4, [x1, #CTX_X25]
    ldp     x3, x4, [x0, #CTX_X27]
    stp     x3, x4, [x1, #CTX_X27]
    ldp     x3, x4, [x0, #CTX_X29]
    stp     x3, x4, [x1, #CTX_X29]

    /* Set child stack pointer */
    str     x2, [x1, #CTX_SP]

    /* Copy SPSR */
    ldr     x3, [x0, #CTX_SPSR]
    str     x3, [x1, #CTX_SPSR]

    /* Copy TPIDR */
    ldr     x3, [x0, #CTX_TPIDR]
    str     x3, [x1, #CTX_TPIDR]

    /* Set child return address (same as parent) */
    ldr     x3, [x0, #CTX_PC]
    str     x3, [x1, #CTX_PC]

    /* Parent returns 0 */
    mov     x0, #0
    ret

# =============================================================================
# USER MODE SWITCH
# =============================================================================

/*
 * context_enter_user
 * ------------------
 * Masuk ke user mode dari kernel.
 *
 * Input:
 *   x0 = entry point
 *   x1 = stack pointer
 *   x2 = argumen (x0 di user)
 */
context_enter_user:
    /* Setup user stack */
    msr     SP_EL0, x1

    /* Setup entry point */
    msr     ELR_EL1, x0

    /* Setup SPSR for EL0t */
    mov     x3, #0x0            /* EL0t */
    msr     SPSR_EL1, x3

    /* Set x0 untuk user */
    mov     x0, x2

    /* Clear other registers */
    mov     x1, xzr
    mov     x2, xzr
    mov     x3, xzr
    mov     x4, xzr
    mov     x5, xzr
    mov     x6, xzr
    mov     x7, xzr
    mov     x8, xzr
    mov     x9, xzr
    mov     x10, xzr
    mov     x11, xzr
    mov     x12, xzr
    mov     x13, xzr
    mov     x14, xzr
    mov     x15, xzr
    mov     x16, xzr
    mov     x17, xzr
    mov     x18, xzr

    isb
    eret

/*
 * context_return_to_user
 * ----------------------
 * Kembali ke user mode setelah syscall.
 *
 * Input:
 *   x0 = return value
 */
context_return_to_user:
    /* Return value sudah di x0 */
    /* Syscall return path restores context */
    msr     ELR_EL1, x1         /* Restore PC */
    msr     SPSR_EL1, x2        /* Restore SPSR */

    isb
    eret
