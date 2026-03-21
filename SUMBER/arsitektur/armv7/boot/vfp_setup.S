# =============================================================================
# PIGURA OS - VFP_SETUP.S
# ========================
# Setup VFP (Vector Floating Point) untuk ARMv7.
#
# Berkas ini berisi kode assembly untuk menginisialisasi unit
# VFP pada prosesor ARMv7.
#
# Arsitektur: ARMv7 (Cortex-A series)
# Versi: 1.0
# =============================================================================

# =============================================================================
# KONSTANTA
# =============================================================================

# CP15 Coprocessor Access Control Register
.set CP15_CPACR,      0, c1, c0, 2

# CPACR bits
.set CPACR_CP10,      (0xF << 20)        # Enable CP10 (VFP)
.set CPACR_CP11,      (0xF << 22)        # Enable CP11 (VFP)

# VFP registers
.set FPEXC,           0x8                 # FP Exception Register
.set FPSCR,           0x1                 # FP Status/Control Register
.set FPEXC_EN,        (1 << 30)           # VFP enable bit

# FPSCR bits
.set FPSCR_DN,        (1 << 25)           # Default NaN mode
.set FPSCR_FZ,        (1 << 24)           # Flush-to-zero mode

# =============================================================================
# SECTIONS
# =============================================================================

.section .text
.align 4

# -----------------------------------------------------------------------------
# _vfp_init
# ---------
# Inisialisasi unit VFP.
# -----------------------------------------------------------------------------
.global _vfp_init
_vfp_init:
    push    {lr}

    # Enable access ke CP10 dan CP11 (VFP coprocessors)
    mrc     CP15_CPACR, r0
    orr     r0, r0, #CPACR_CP10
    orr     r0, r0, #CPACR_CP11
    mcr     CP15_CPACR, r0
    isb

    # Enable VFP
    vmrs    r0, FPEXC
    orr     r0, r0, #FPEXC_EN
    vmsr    FPEXC, r0

    # Set FPSCR ke default state
    # Enable flush-to-zero dan default NaN untuk performance
    mov     r0, #(FPSCR_DN | FPSCR_FZ)
    vmsr    FPSCR, r0

    # Clear semua VFP registers
    vmov    d0, r0, r0
    vmov    d1, r0, r0
    vmov    d2, r0, r0
    vmov    d3, r0, r0
    vmov    d4, r0, r0
    vmov    d5, r0, r0
    vmov    d6, r0, r0
    vmov    d7, r0, r0
    vmov    d8, r0, r0
    vmov    d9, r0, r0
    vmov    d10, r0, r0
    vmov    d11, r0, r0
    vmov    d12, r0, r0
    vmov    d13, r0, r0
    vmov    d14, r0, r0
    vmov    d15, r0, r0

    pop     {pc}

# -----------------------------------------------------------------------------
# _vfp_enable
# -----------
# Aktifkan VFP.
# -----------------------------------------------------------------------------
.global _vfp_enable
_vfp_enable:
    push    {lr}

    # Enable VFP
    vmrs    r0, FPEXC
    orr     r0, r0, #FPEXC_EN
    vmsr    FPEXC, r0

    pop     {pc}

# -----------------------------------------------------------------------------
# _vfp_disable
# ------------
# Nonaktifkan VFP.
# -----------------------------------------------------------------------------
.global _vfp_disable
_vfp_disable:
    push    {lr}

    # Disable VFP
    vmrs    r0, FPEXC
    bic     r0, r0, #FPEXC_EN
    vmsr    FPEXC, r0

    pop     {pc}

# -----------------------------------------------------------------------------
# _vfp_save_context
# -----------------
# Simpan context VFP.
# Parameters: r0 = pointer ke context buffer
# -----------------------------------------------------------------------------
.global _vfp_save_context
_vfp_save_context:
    push    {r1, lr}

    # Save FPEXC
    vmrs    r1, FPEXC
    str     r1, [r0], #4

    # Save FPSCR
    vmrs    r1, FPSCR
    str     r1, [r0], #4

    # Save D0-D15
    vstm    r0, {d0-d15}

    pop     {r1, pc}

# -----------------------------------------------------------------------------
# _vfp_restore_context
# --------------------
# Restore context VFP.
# Parameters: r0 = pointer ke context buffer
# -----------------------------------------------------------------------------
.global _vfp_restore_context
_vfp_restore_context:
    push    {r1, lr}

    # Restore FPEXC
    ldr     r1, [r0], #4
    vmsr    FPEXC, r1

    # Restore FPSCR
    ldr     r1, [r0], #4
    vmsr    FPSCR, r1

    # Restore D0-D15
    vldm    r0, {d0-d15}

    pop     {r1, pc}

# -----------------------------------------------------------------------------
# _vfp_is_enabled
# ---------------
# Cek apakah VFP aktif.
# Returns: r0 = 1 jika aktif, 0 jika tidak
# -----------------------------------------------------------------------------
.global _vfp_is_enabled
_vfp_is_enabled:
    vmrs    r0, FPEXC
    tst     r0, #FPEXC_EN
    moveq   r0, #0
    movne   r0, #1
    bx      lr

# -----------------------------------------------------------------------------
# _vfp_get_fpscr
# --------------
# Dapatkan nilai FPSCR.
# Returns: r0 = FPSCR value
# -----------------------------------------------------------------------------
.global _vfp_get_fpscr
_vfp_get_fpscr:
    vmrs    r0, FPSCR
    bx      lr

# -----------------------------------------------------------------------------
# _vfp_set_fpscr
# --------------
# Set nilai FPSCR.
# Parameters: r0 = new FPSCR value
# -----------------------------------------------------------------------------
.global _vfp_set_fpscr
_vfp_set_fpscr:
    vmsr    FPSCR, r0
    bx      lr
