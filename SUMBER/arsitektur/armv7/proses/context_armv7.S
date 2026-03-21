# =============================================================================
# PIGURA OS - CONTEXT_ARMV7.S
# ============================
# Implementasi context switch untuk ARMv7.
#
# Berkas ini berisi kode assembly untuk menyimpan dan memulihkan
# context CPU saat context switch.
#
# Arsitektur: ARMv7 (Cortex-A series)
# Versi: 1.0
# =============================================================================

# =============================================================================
# KONSTANTA
# =============================================================================

# Offset dalam cpu_context_t structure
.set CTX_R0,          0
.set CTX_R1,          4
.set CTX_R2,          8
.set CTX_R3,          12
.set CTX_R4,          16
.set CTX_R5,          20
.set CTX_R6,          24
.set CTX_R7,          28
.set CTX_R8,          32
.set CTX_R9,          36
.set CTX_R10,         40
.set CTX_R11,         44
.set CTX_R12,         48
.set CTX_SP,          52
.set CTX_LR,          56
.set CTX_PC,          60
.set CTX_CPSR,        64
.set CTX_SPSR,        68
.set CTX_FPU_USED,    72
.set CTX_VFP,         80
.set CTX_FPSCR,       336
.set CTX_SIZE,        352

# CPSR mode bits
.set CPSR_MODE_USR,   0x10
.set CPSR_MODE_SVC,   0x13
.set CPSR_IRQ_DIS,    (1 << 7)
.set CPSR_FIQ_DIS,    (1 << 6)

# =============================================================================
# SECTIONS
# =============================================================================

.section .text
.align 4

# -----------------------------------------------------------------------------
# _context_save
# -------------
# Simpan context CPU ke struktur context.
# Parameters: r0 = pointer ke context structure
# Returns: r0 = 0 jika berhasil
# -----------------------------------------------------------------------------
.global _context_save
_context_save:
    # Save general purpose registers
    stmia   r0, {r4-r12}

    # Save SP dan LR
    add     r1, r0, #CTX_SP
    str     sp, [r1]
    str     lr, [r1, #4]

    # Save CPSR
    mrs     r1, cpsr
    add     r2, r0, #CTX_CPSR
    str     r1, [r2]

    # Save SPSR (if in exception mode)
    mrs     r1, spsr
    str     r1, [r2, #4]

    # Save return address (PC)
    # PC sudah disimpan di LR saat exception
    add     r1, r0, #CTX_PC
    str     lr, [r1]

    mov     r0, #0
    bx      lr

# -----------------------------------------------------------------------------
# _context_restore
# ----------------
# Restore context CPU dari struktur context.
# Parameters: r0 = pointer ke context structure
# This function does not return normally
# -----------------------------------------------------------------------------
.global _context_restore
_context_restore:
    # Restore CPSR
    add     r1, r0, #CTX_CPSR
    ldr     r2, [r1]
    msr     cpsr_c, r2

    # Restore general purpose registers
    ldmia   r0, {r4-r12}

    # Restore SP dan LR
    add     r1, r0, #CTX_SP
    ldr     sp, [r1]
    ldr     lr, [r1, #4]

    # Restore SPSR
    add     r1, r0, #CTX_SPSR
    ldr     r2, [r1]
    msr     spsr_cxsf, r2

    # Get PC
    add     r1, r0, #CTX_PC
    ldr     pc, [r1]

# -----------------------------------------------------------------------------
# _context_switch
# ---------------
# Switch context dari proses lama ke proses baru.
# Parameters: r0 = pointer ke old context, r1 = pointer ke new context
# -----------------------------------------------------------------------------
.global _context_switch
_context_switch:
    push    {lr}

    # Save old context
    cmp     r0, #0
    beq     ._context_switch_no_save

    bl      _context_save

._context_switch_no_save:
    # Restore new context
    cmp     r1, #0
    moveq   r0, #-1
    beq     ._context_switch_done

    mov     r0, r1
    bl      _context_restore

._context_switch_done:
    pop     {pc}

# -----------------------------------------------------------------------------
# _context_switch_to
# ------------------
# Switch ke context baru tanpa menyimpan context lama.
# Parameters: r0 = pointer ke new context
# -----------------------------------------------------------------------------
.global _context_switch_to
_context_switch_to:
    cmp     r0, #0
    bxeq    lr

    b       _context_restore

# -----------------------------------------------------------------------------
# _context_init
# -------------
# Inisialisasi context baru.
# Parameters: r0 = context buffer, r1 = entry point, r2 = stack pointer,
#             r3 = cpsr value
# Returns: r0 = 0 jika berhasil
# -----------------------------------------------------------------------------
.global _context_init
_context_init:
    push    {r4, lr}

    mov     r4, r0

    # Clear all registers in context
    mov     r0, #0
    str     r0, [r4, #CTX_R0]
    str     r0, [r4, #CTX_R1]
    str     r0, [r4, #CTX_R2]
    str     r0, [r4, #CTX_R3]
    str     r0, [r4, #CTX_R4]
    str     r0, [r4, #CTX_R5]
    str     r0, [r4, #CTX_R6]
    str     r0, [r4, #CTX_R7]
    str     r0, [r4, #CTX_R8]
    str     r0, [r4, #CTX_R9]
    str     r0, [r4, #CTX_R10]
    str     r0, [r4, #CTX_R11]
    str     r0, [r4, #CTX_R12]

    # Set SP, LR, PC
    str     r2, [r4, #CTX_SP]
    mov     r0, #0
    str     r0, [r4, #CTX_LR]            # LR = 0 (no return)
    str     r1, [r4, #CTX_PC]            # PC = entry point

    # Set CPSR dan SPSR
    str     r3, [r4, #CTX_CPSR]
    str     r3, [r4, #CTX_SPSR]

    # Clear FPU flag
    mov     r0, #0
    str     r0, [r4, #CTX_FPU_USED]

    mov     r0, #0
    pop     {r4, pc}

# -----------------------------------------------------------------------------
# _context_save_fpu
# -----------------
# Simpan state FPU/NEON dalam context.
# Parameters: r0 = pointer ke context structure
# -----------------------------------------------------------------------------
.global _context_save_fpu
_context_save_fpu:
    push    {r1, lr}

    # Cek apakah FPU digunakan
    ldr     r1, [r0, #CTX_FPU_USED]
    cmp     r1, #0
    beq     ._context_save_fpu_done

    # Save VFP/NEON registers D0-D31
    add     r1, r0, #CTX_VFP
    vstm    r1, {d0-d31}

    # Save FPSCR
    vmrs    r1, fpscr
    str     r1, [r0, #CTX_FPSCR]

._context_save_fpu_done:
    pop     {r1, pc}

# -----------------------------------------------------------------------------
# _context_restore_fpu
# --------------------
# Restore state FPU/NEON dari context.
# Parameters: r0 = pointer ke context structure
# -----------------------------------------------------------------------------
.global _context_restore_fpu
_context_restore_fpu:
    push    {r1, lr}

    # Cek apakah FPU digunakan
    ldr     r1, [r0, #CTX_FPU_USED]
    cmp     r1, #0
    beq     ._context_restore_fpu_done

    # Restore VFP/NEON registers D0-D31
    add     r1, r0, #CTX_VFP
    vldm    r1, {d0-d31}

    # Restore FPSCR
    ldr     r1, [r0, #CTX_FPSCR]
    vmsr    fpscr, r1

._context_restore_fpu_done:
    pop     {r1, pc}

# -----------------------------------------------------------------------------
# _context_set_fpu_used
# ---------------------
# Set flag FPU used dalam context.
# Parameters: r0 = pointer ke context structure
# -----------------------------------------------------------------------------
.global _context_set_fpu_used
_context_set_fpu_used:
    mov     r1, #1
    str     r1, [r0, #CTX_FPU_USED]
    bx      lr

# -----------------------------------------------------------------------------
# _context_get_stack
# ------------------
# Dapatkan stack pointer dari context.
# Parameters: r0 = pointer ke context structure
# Returns: r0 = stack pointer
# -----------------------------------------------------------------------------
.global _context_get_stack
_context_get_stack:
    ldr     r0, [r0, #CTX_SP]
    bx      lr

# -----------------------------------------------------------------------------
# _context_set_stack
# ------------------
# Set stack pointer dalam context.
# Parameters: r0 = pointer ke context structure, r1 = stack pointer
# -----------------------------------------------------------------------------
.global _context_set_stack
_context_set_stack:
    str     r1, [r0, #CTX_SP]
    bx      lr

# -----------------------------------------------------------------------------
# _context_get_pc
# ---------------
# Dapatkan PC dari context.
# Parameters: r0 = pointer ke context structure
# Returns: r0 = PC value
# -----------------------------------------------------------------------------
.global _context_get_pc
_context_get_pc:
    ldr     r0, [r0, #CTX_PC]
    bx      lr

# -----------------------------------------------------------------------------
# _context_set_pc
# ---------------
# Set PC dalam context.
# Parameters: r0 = pointer ke context structure, r1 = PC value
# -----------------------------------------------------------------------------
.global _context_set_pc
_context_set_pc:
    str     r1, [r0, #CTX_PC]
    bx      lr

# -----------------------------------------------------------------------------
# _context_get_cpsr
# -----------------
# Dapatkan CPSR dari context.
# Parameters: r0 = pointer ke context structure
# Returns: r0 = CPSR value
# -----------------------------------------------------------------------------
.global _context_get_cpsr
_context_get_cpsr:
    ldr     r0, [r0, #CTX_CPSR]
    bx      lr

# -----------------------------------------------------------------------------
# _context_set_cpsr
# -----------------
# Set CPSR dalam context.
# Parameters: r0 = pointer ke context structure, r1 = CPSR value
# -----------------------------------------------------------------------------
.global _context_set_cpsr
_context_set_cpsr:
    str     r1, [r0, #CTX_CPSR]
    str     r1, [r0, #CTX_SPSR]
    bx      lr
