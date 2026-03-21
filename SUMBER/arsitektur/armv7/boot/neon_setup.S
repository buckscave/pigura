# =============================================================================
# PIGURA OS - NEON_SETUP.S
# =========================
# Setup NEON (Advanced SIMD) untuk ARMv7.
#
# Berkas ini berisi kode assembly untuk menginisialisasi unit
# NEON pada prosesor ARMv7.
#
# Arsitektur: ARMv7 (Cortex-A series)
# Versi: 1.0
# =============================================================================

# =============================================================================
# KONSTANTA
# =============================================================================

# VFP/NEON registers
.set FPEXC,           0x8                 # FP Exception Register
.set FPSCR,           0x1                 # FP Status/Control Register
.set FPEXC_EN,        (1 << 30)           # VFP enable bit
.set FPEXC_EX,        (1 << 31)           # Exception bit
.set FPEXC_DEX,       (1 << 29)           # Defined synchronous exception

# FPSCR bits
.set FPSCR_DN,        (1 << 25)           # Default NaN mode
.set FPSCR_FZ,        (1 << 24)           # Flush-to-zero mode
.set FPSCR_VCVT,      (1 << 20)           # Vector continuation

# MVFR0 Media and VFP Feature Register 0 bits
.set MVFR0_SIMD,      0xF0000000          # SIMD bits

# =============================================================================
# SECTIONS
# =============================================================================

.section .text
.align 4

# -----------------------------------------------------------------------------
# _neon_init
# ----------
# Inisialisasi unit NEON.
# Returns: r0 = 0 jika berhasil, -1 jika tidak didukung
# -----------------------------------------------------------------------------
.global _neon_init
_neon_init:
    push    {lr}

    # Cek apakah NEON didukung
    bl      _neon_is_supported
    cmp     r0, #0
    beq     ._neon_not_supported

    # Enable VFP terlebih dahulu
    bl      _vfp_enable

    # Clear FPSCR
    mov     r0, #(FPSCR_DN | FPSCR_FZ)
    vmsr    FPSCR, r0

    # Clear semua NEON registers (Q0-Q15 = D0-D31)
    mov     r0, #0
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
    vmov    d16, r0, r0
    vmov    d17, r0, r0
    vmov    d18, r0, r0
    vmov    d19, r0, r0
    vmov    d20, r0, r0
    vmov    d21, r0, r0
    vmov    d22, r0, r0
    vmov    d23, r0, r0
    vmov    d24, r0, r0
    vmov    d25, r0, r0
    vmov    d26, r0, r0
    vmov    d27, r0, r0
    vmov    d28, r0, r0
    vmov    d29, r0, r0
    vmov    d30, r0, r0
    vmov    d31, r0, r0

    mov     r0, #0                          # Return success
    pop     {pc}

._neon_not_supported:
    mvn     r0, #0                          # Return -1
    pop     {pc}

# -----------------------------------------------------------------------------
# _neon_is_supported
# -------------------
# Cek apakah NEON didukung oleh CPU.
# Returns: r0 = 1 jika didukung, 0 jika tidak
# -----------------------------------------------------------------------------
.global _neon_is_supported
_neon_is_supported:
    push    {r1, lr}

    # Baca MVFR0 (Media and VFP Feature Register 0)
    vmrs    r0, mvfr0

    # Cek SIMD support (bits 28-31)
    lsr     r0, r0, #28
    cmp     r0, #1
    movge   r0, #1                          # Supported
    movlt   r0, #0                          # Not supported

    pop     {r1, pc}

# -----------------------------------------------------------------------------
# _neon_enable
# ------------
# Aktifkan NEON (sama dengan VFP enable).
# -----------------------------------------------------------------------------
.global _neon_enable
_neon_enable:
    b       _vfp_enable

# -----------------------------------------------------------------------------
# _neon_disable
# -------------
# Nonaktifkan NEON (sama dengan VFP disable).
# -----------------------------------------------------------------------------
.global _neon_disable
_neon_disable:
    b       _vfp_disable

# -----------------------------------------------------------------------------
# _neon_save_context
# ------------------
# Simpan context NEON.
# Parameters: r0 = pointer ke context buffer
# -----------------------------------------------------------------------------
.global _neon_save_context
_neon_save_context:
    push    {r1, lr}

    # Save FPEXC
    vmrs    r1, FPEXC
    str     r1, [r0], #4

    # Save FPSCR
    vmrs    r1, FPSCR
    str     r1, [r0], #4

    # Save D0-D31 (all NEON/VFP registers)
    vstm    r0, {d0-d31}

    pop     {r1, pc}

# -----------------------------------------------------------------------------
# _neon_restore_context
# ---------------------
# Restore context NEON.
# Parameters: r0 = pointer ke context buffer
# -----------------------------------------------------------------------------
.global _neon_restore_context
_neon_restore_context:
    push    {r1, lr}

    # Restore FPEXC
    ldr     r1, [r0], #4
    vmsr    FPEXC, r1

    # Restore FPSCR
    ldr     r1, [r0], #4
    vmsr    FPSCR, r1

    # Restore D0-D31
    vldm    r0, {d0-d31}

    pop     {r1, pc}

# -----------------------------------------------------------------------------
# _neon_clear
# -----------
# Clear semua NEON registers.
# -----------------------------------------------------------------------------
.global _neon_clear
_neon_clear:
    push    {r0, lr}

    mov     r0, #0

    # Clear D0-D31
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
    vmov    d16, r0, r0
    vmov    d17, r0, r0
    vmov    d18, r0, r0
    vmov    d19, r0, r0
    vmov    d20, r0, r0
    vmov    d21, r0, r0
    vmov    d22, r0, r0
    vmov    d23, r0, r0
    vmov    d24, r0, r0
    vmov    d25, r0, r0
    vmov    d26, r0, r0
    vmov    d27, r0, r0
    vmov    d28, r0, r0
    vmov    d29, r0, r0
    vmov    d30, r0, r0
    vmov    d31, r0, r0

    pop     {r0, pc}

# -----------------------------------------------------------------------------
# _neon_memcpy
# ------------
# Copy memori menggunakan NEON.
# Parameters: r0 = destination, r1 = source, r2 = length (bytes)
# Returns: r0 = destination
# -----------------------------------------------------------------------------
.global _neon_memcpy
_neon_memcpy:
    push    {r4, lr}

    mov     r4, r0                          # Save destination

    # Copy in 64-byte chunks using NEON
._neon_memcpy_loop:
    cmp     r2, #64
    blt     ._neon_memcpy_remaining

    vld1.8  {d0-d7}, [r1]!
    vst1.8  {d0-d7}, [r0]!
    sub     r2, r2, #64
    b       ._neon_memcpy_loop

._neon_memcpy_remaining:
    # Handle remaining bytes
    cmp     r2, #0
    beq     ._neon_memcpy_done

._neon_memcpy_byte_loop:
    ldrb    r3, [r1], #1
    strb    r3, [r0], #1
    subs    r2, r2, #1
    bne     ._neon_memcpy_byte_loop

._neon_memcpy_done:
    mov     r0, r4
    pop     {r4, pc}

# -----------------------------------------------------------------------------
# _neon_memset
# ------------
# Set memori menggunakan NEON.
# Parameters: r0 = destination, r1 = value (byte), r2 = length (bytes)
# Returns: r0 = destination
# -----------------------------------------------------------------------------
.global _neon_memset
_neon_memset:
    push    {r4, lr}

    mov     r4, r0                          # Save destination

    # Replicate byte value ke semua bytes dalam D0-D7
    and     r1, r1, #0xFF
    lsl     r3, r1, #8
    orr     r1, r1, r3
    lsl     r3, r1, #16
    orr     r1, r1, r3
    vmov    d0, r1, r1
    vmov    d1, r1, r1
    vmov    d2, r1, r1
    vmov    d3, r1, r1
    vmov    d4, r1, r1
    vmov    d5, r1, r1
    vmov    d6, r1, r1
    vmov    d7, r1, r1

    # Set in 64-byte chunks
._neon_memset_loop:
    cmp     r2, #64
    blt     ._neon_memset_remaining

    vst1.8  {d0-d7}, [r0]!
    sub     r2, r2, #64
    b       ._neon_memset_loop

._neon_memset_remaining:
    # Handle remaining bytes
    cmp     r2, #0
    beq     ._neon_memset_done

    and     r1, r4, #0xFF
._neon_memset_byte_loop:
    strb    r1, [r0], #1
    subs    r2, r2, #1
    bne     ._neon_memset_byte_loop

._neon_memset_done:
    mov     r0, r4
    pop     {r4, pc}
