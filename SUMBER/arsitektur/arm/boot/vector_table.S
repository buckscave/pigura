# =============================================================================
# PIGURA OS - VECTOR_TABLE.S
# ===========================
# Exception vector table untuk ARM (32-bit).
#
# Arsitektur: ARM (ARMv4/v5/v6 - 32-bit)
# Versi: 1.0
# =============================================================================

# =============================================================================
# KONSTANTA
# =============================================================================

# Exception vector offsets
.set VEC_RESET,           0x00
.set VEC_UNDEFINED,       0x04
.set VEC_SWI,             0x08
.set VEC_PREFETCH_ABORT,  0x0C
.set VEC_DATA_ABORT,      0x10
.set VEC_UNUSED,          0x14
.set VEC_IRQ,             0x18
.set VEC_FIQ,             0x1C

# CPSR mode bits
.set CPSR_MODE_SVC,       0x13
.set CPSR_MODE_IRQ,       0x12
.set CPSR_MODE_ABT,       0x17
.set CPSR_MODE_UND,       0x1B
.set CPSR_MODE_FIQ,       0x11

# =============================================================================
# VECTOR TABLE
# =============================================================================

.section .text.vectors
.align 5                          # 32-byte aligned

.global _vector_table
_vector_table:
    b       _exc_reset
    b       _exc_undefined
    b       _exc_swi
    b       _exc_prefetch_abort
    b       _exc_data_abort
    b       _exc_unused
    b       _exc_irq
    b       _exc_fiq

# =============================================================================
# EXCEPTION HANDLERS
# =============================================================================

.section .text

# -----------------------------------------------------------------------------
# _exc_reset
# ----------
# Reset exception handler.
# -----------------------------------------------------------------------------
_exc_reset:
    # Switch ke SVC mode
    cps     #CPSR_MODE_SVC

    # Setup stack
    ldr     sp, =_exception_stack

    # Call C handler
    mov     r0, #0                          # Exception number
    bl      hal_dispatch_exception

    # Infinite loop
    b       .

# -----------------------------------------------------------------------------
# _exc_undefined
# --------------
# Undefined instruction exception handler.
# -----------------------------------------------------------------------------
_exc_undefined:
    # Switch ke UND mode
    cps     #CPSR_MODE_UND

    # Save context
    push    {r0-r12, lr}

    # Get SPSR
    mrs     r0, spsr
    push    {r0}

    # Call C handler
    mov     r0, #1                          # Exception number
    mov     r1, sp                          # SPSR
    ldr     r2, [sp, #4]                    # SP
    ldr     r3, [sp, #8]                    # LR
    bl      hal_dispatch_exception

    # Restore context
    pop     {r0}
    msr     spsr_cxsf, r0
    pop     {r0-r12, lr}

    # Return
    movs    pc, lr

# -----------------------------------------------------------------------------
# _exc_swi
# ---------
# Software interrupt handler.
# -----------------------------------------------------------------------------
_exc_swi:
    # Switch ke SVC mode
    cps     #CPSR_MODE_SVC

    # Save context
    push    {r0-r12, lr}

    # Get SPSR
    mrs     r0, spsr
    push    {r0}

    # Call C handler
    mov     r0, #2                          # Exception number
    mov     r1, sp                          # SPSR
    ldr     r2, [sp, #4]                    # SP
    ldr     r3, [sp, #8]                    # LR
    bl      hal_dispatch_exception

    # Restore context
    pop     {r0}
    msr     spsr_cxsf, r0
    pop     {r0-r12, lr}

    # Return
    movs    pc, lr

# -----------------------------------------------------------------------------
# _exc_prefetch_abort
# --------------------
# Prefetch abort handler.
# -----------------------------------------------------------------------------
_exc_prefetch_abort:
    # Switch ke ABT mode
    cps     #CPSR_MODE_ABT

    # Save context
    push    {r0-r12, lr}

    # Get SPSR
    mrs     r0, spsr
    push    {r0}

    # Call C handler
    mov     r0, #3                          # Exception number
    mov     r1, sp                          # SPSR
    ldr     r2, [sp, #4]                    # SP
    ldr     r3, [sp, #8]                    # LR
    bl      hal_dispatch_exception

    # Restore context
    pop     {r0}
    msr     spsr_cxsf, r0
    pop     {r0-r12, lr}

    # Return
    subs    pc, lr, #4

# -----------------------------------------------------------------------------
# _exc_data_abort
# ----------------
# Data abort handler.
# -----------------------------------------------------------------------------
_exc_data_abort:
    # Switch ke ABT mode
    cps     #CPSR_MODE_ABT

    # Save context
    push    {r0-r12, lr}

    # Get SPSR
    mrs     r0, spsr
    push    {r0}

    # Call C handler
    mov     r0, #4                          # Exception number
    mov     r1, sp                          # SPSR
    ldr     r2, [sp, #4]                    # SP
    ldr     r3, [sp, #8]                    # LR
    bl      hal_dispatch_exception

    # Restore context
    pop     {r0}
    msr     spsr_cxsf, r0
    pop     {r0-r12, lr}

    # Return
    subs    pc, lr, #8

# -----------------------------------------------------------------------------
# _exc_unused
# -----------
# Unused exception handler.
# -----------------------------------------------------------------------------
_exc_unused:
    # Infinite loop
    b       .

# -----------------------------------------------------------------------------
# _exc_irq
# ---------
# IRQ handler.
# -----------------------------------------------------------------------------
_exc_irq:
    # Switch ke IRQ mode
    cps     #CPSR_MODE_IRQ

    # Save context
    push    {r0-r12, lr}

    # Get SPSR
    mrs     r0, spsr
    push    {r0}

    # Call C handler
    bl      hal_dispatch_irq

    # Restore context
    pop     {r0}
    msr     spsr_cxsf, r0
    pop     {r0-r12, lr}

    # Return
    subs    pc, lr, #4

# -----------------------------------------------------------------------------
# _exc_fiq
# ---------
# FIQ handler.
# -----------------------------------------------------------------------------
_exc_fiq:
    # Switch ke FIQ mode
    cps     #CPSR_MODE_FIQ

    # Save context (FIQ has r8-r14 banked)
    push    {r0-r7, lr}

    # Get SPSR
    mrs     r0, spsr
    push    {r0}

    # Call C handler
    bl      hal_dispatch_fiq

    # Restore context
    pop     {r0}
    msr     spsr_cxsf, r0
    pop     {r0-r7, lr}

    # Return
    subs    pc, lr, #4

# =============================================================================
# DATA
# =============================================================================

.section .bss

.align 4
_exception_stack:
    .skip 4096                    # Exception stack
