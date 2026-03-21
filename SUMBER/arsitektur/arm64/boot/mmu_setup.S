# =============================================================================
# PIGURA OS - MMU_SETUP.S
# =======================
# Setup MMU untuk arsitektur ARM64 (AArch64).
#
# Berkas ini berisi fungsi-fungsi assembly untuk menginisialisasi
# dan mengelola MMU pada prosesor ARM64.
#
# Arsitektur: ARM64/AArch64 (64-bit)
# Versi: 1.0
# =============================================================================

# =============================================================================
# SECTIONS
# =============================================================================

.section .text
.global mmu_enable
.global mmu_disable
.global mmu_setup_ttbr0
.global mmu_setup_ttbr1
.global mmu_setup_tcr
.global mmu_invalidate_tlb_all
.global mmu_invalidate_tlb_va
.global mmu_invalidate_tlb_asid
.global mmu_enable_cache
.global mmu_disable_cache

# =============================================================================
# MMU FUNCTIONS
# =============================================================================

/*
 * mmu_enable
 * ----------
 * Aktifkan MMU.
 */
mmu_enable:
    mrs     x0, SCTLR_EL1
    orr     x0, x0, #0x1          /* Set M bit */
    msr     SCTLR_EL1, x0
    isb
    ret

/*
 * mmu_disable
 * -----------
 * Nonaktifkan MMU.
 */
mmu_disable:
    mrs     x0, SCTLR_EL1
    bic     x0, x0, #0x1          /* Clear M bit */
    msr     SCTLR_EL1, x0
    isb
    ret

/*
 * mmu_setup_ttbr0
 * ---------------
 * Setup TTBR0_EL1 (user space page table).
 *
 * Input:
 *   x0 = Page table base address
 *   x1 = ASID (optional, in upper 16 bits)
 */
mmu_setup_ttbr0:
    /* Combine address with ASID */
    msr     TTBR0_EL1, x0
    isb
    ret

/*
 * mmu_setup_ttbr1
 * ---------------
 * Setup TTBR1_EL1 (kernel space page table).
 *
 * Input:
 *   x0 = Page table base address
 */
mmu_setup_ttbr1:
    msr     TTBR1_EL1, x0
    isb
    ret

/*
 * mmu_setup_tcr
 * -------------
 * Setup TCR_EL1 (Translation Control Register).
 *
 * Input:
 *   x0 = TCR value
 *
 * TCR bits:
 *   T0SZ[5:0]   - VA size for TTBR0 (64-T0SZ = VA bits)
 *   T1SZ[21:16] - VA size for TTBR1
 *   IRGN0[9:8]  - Inner cacheability for TTBR0 walks
 *   IRGN1[25:24]- Inner cacheability for TTBR1 walks
 *   ORGN0[11:10]- Outer cacheability for TTBR0 walks
 *   ORGN1[27:26]- Outer cacheability for TTBR1 walks
 *   SH0[13:12]  - Shareability for TTBR0 walks
 *   SH1[29:28]  - Shareability for TTBR1 walks
 *   TG0[15:14]  - Granule size for TTBR0 (0=4KB, 1=64KB, 2=16KB)
 *   TG1[31:30]  - Granule size for TTBR1
 *   IPS[34:32]  - Intermediate Physical Address Size
 *   AS[36]      - ASID size (0=8-bit, 1=16-bit)
 */
mmu_setup_tcr:
    msr     TCR_EL1, x0
    isb
    ret

/*
 * mmu_invalidate_tlb_all
 * ----------------------
 * Invalidate semua TLB entries.
 */
mmu_invalidate_tlb_all:
    tlbi    VMALLE1IS
    tlbi    VMALLE1
    dsb     ish
    isb
    ret

/*
 * mmu_invalidate_tlb_va
 * ---------------------
 * Invalidate TLB untuk virtual address tertentu.
 *
 * Input:
 *   x0 = Virtual address (page aligned)
 */
mmu_invalidate_tlb_va:
    lsr     x0, x0, #12           /* Shift to get page number */
    tlbi    VAAE1, x0
    dsb     sy
    isb
    ret

/*
 * mmu_invalidate_tlb_asid
 * -----------------------
 * Invalidate TLB untuk ASID tertentu.
 *
 * Input:
 *   x0 = ASID
 */
mmu_invalidate_tlb_asid:
    /* ASID is in bits [63:48] for TLBI ASIDE1 */
    lsl     x0, x0, #48
    tlbi    ASIDE1, x0
    dsb     sy
    isb
    ret

/*
 * mmu_enable_cache
 * ----------------
 * Aktifkan instruction dan data cache.
 */
mmu_enable_cache:
    mrs     x0, SCTLR_EL1
    orr     x0, x0, #0x4          /* C bit: data cache */
    orr     x0, x0, #0x1000       /* I bit: instruction cache */
    msr     SCTLR_EL1, x0
    isb
    ret

/*
 * mmu_disable_cache
 * -----------------
 * Nonaktifkan instruction dan data cache.
 */
mmu_disable_cache:
    mrs     x0, SCTLR_EL1
    bic     x0, x0, #0x4          /* Clear C bit */
    bic     x0, x0, #0x1000       /* Clear I bit */
    msr     SCTLR_EL1, x0
    isb
    /* Clean and invalidate all cache */
    ic      IALLU
    dsb     sy
    isb
    ret

# =============================================================================
# PAGE TABLE HELPERS
# =============================================================================

.section .text
.global mmu_get_tcr_default
.global mmu_get_mair_default

/*
 * mmu_get_tcr_default
 * -------------------
 * Dapatkan nilai TCR default untuk ARM64.
 *
 * Output:
 *   x0 = TCR value
 */
mmu_get_tcr_default:
    /* 48-bit VA split (T0SZ=16, T1SZ=16) */
    /* 4KB granule for both TTBR0 and TTBR1 */
    /* Inner shareable, write-back cacheable */
    /* 16-bit ASID, 48-bit PA */
    ldr     x0, =0x10B5403510
    ret

/*
 * mmu_get_mair_default
 * --------------------
 * Dapatkan nilai MAIR default untuk ARM64.
 *
 * Output:
 *   x0 = MAIR value
 *
 * Attr 0: Normal Non-Cacheable
 * Attr 1: Normal Write-Back Cacheable
 * Attr 2: Device nGnRnE
 * Attr 3: Device nGnRE
 * Attr 4: Device GRE
 * Attr 5: Normal I-WT
 */
mmu_get_mair_default:
    ldr     x0, =0x00000000FF004C04
    ret

# =============================================================================
# CACHE MAINTENANCE
# =============================================================================

.section .text
.global cache_clean_va
.global cache_invalidate_va
.global cache_clean_invalidate_va
.global cache_invalidate_icache

/*
 * cache_clean_va
 * --------------
 * Clean data cache untuk virtual address.
 *
 * Input:
 *   x0 = Virtual address
 */
cache_clean_va:
    dc      cvau, x0
    dsb     sy
    ret

/*
 * cache_invalidate_va
 * -------------------
 * Invalidate data cache untuk virtual address.
 *
 * Input:
 *   x0 = Virtual address
 */
cache_invalidate_va:
    dc      ivau, x0
    dsb     sy
    ret

/*
 * cache_clean_invalidate_va
 * -------------------------
 * Clean dan invalidate data cache untuk virtual address.
 *
 * Input:
 *   x0 = Virtual address
 */
cache_clean_invalidate_va:
    dc      civac, x0
    dsb     sy
    ret

/*
 * cache_invalidate_icache
 * -----------------------
 * Invalidate instruction cache.
 */
cache_invalidate_icache:
    ic      IALLU
    dsb     sy
    isb
    ret

# =============================================================================
# BARRIER FUNCTIONS
# =============================================================================

.section .text
.global barrier_dsb
.global barrier_dmb
.global barrier_isb

/*
 * barrier_dsb
 * -----------
 * Data Synchronization Barrier.
 */
barrier_dsb:
    dsb     sy
    ret

/*
 * barrier_dmb
 * -----------
 * Data Memory Barrier.
 */
barrier_dmb:
    dmb     sy
    ret

/*
 * barrier_isb
 * -----------
 * Instruction Synchronization Barrier.
 */
barrier_isb:
    isb
    ret
