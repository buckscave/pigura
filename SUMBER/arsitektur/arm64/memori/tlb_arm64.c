/*
 * PIGURA OS - TLB_ARM64.C
 * -----------------------
 * Implementasi TLB management untuk ARM64 (AArch64).
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola Translation
 * Lookaside Buffer pada prosesor ARM64.
 *
 * Arsitektur: ARM64/AArch64 (64-bit)
 * Versi: 1.0
 */

#include "../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* TLBI instruction opcodes */
#define TLBI_VMALLE1IS         "tlbi vmalle1is"
#define TLBI_VMALLE1           "tlbi vmalle1"
#define TLBI_VAE1_IS           "tlbi vae1is"
#define TLBI_VAE1              "tlbi vae1"
#define TLBI_VAAE1_IS          "tlbi vaae1is"
#define TLBI_VAAE1             "tlbi vaae1"
#define TLBI_ASIDE1_IS         "tlbi aside1is"
#define TLBI_ASIDE1            "tlbi aside1"
#define TLBI_ALLNSNH_IS        "tlbi allnsnhis"

/* ASID bits */
#define ASID_BITS_8            8
#define ASID_BITS_16           16
#define ASID_MASK_8            0xFF
#define ASID_MASK_16           0xFFFF

/*
 * ============================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ============================================================================
 */

/* ASID size */
static tak_bertanda32_t g_asid_bits = 16;

/* TLB statistics */
static tak_bertanda64_t g_tlb_flush_count = 0;
static tak_bertanda64_t g_tlb_flush_va_count = 0;
static tak_bertanda64_t g_tlb_flush_asid_count = 0;

/*
 * ============================================================================
 * PUBLIC FUNCTIONS
 * ============================================================================
 */

/*
 * tlb_init
 * --------
 * Initialize TLB management.
 */
status_t tlb_init(void)
{
    tak_bertanda64_t tcr;

    /* Read TCR to determine ASID size */
    __asm__ __volatile__(
        "mrs %0, tcr_el1"
        : "=r"(tcr)
    );

    /* Check AS bit (bit 36) */
    if (tcr & (1ULL << 36)) {
        g_asid_bits = ASID_BITS_16;
    } else {
        g_asid_bits = ASID_BITS_8;
    }

    kernel_printf("[TLB-ARM64] ASID size: %u bits\n", g_asid_bits);

    /* Perform initial TLB invalidate */
    tlb_invalidate_all();

    return STATUS_BERHASIL;
}

/*
 * tlb_invalidate_all
 * ------------------
 * Invalidate all TLB entries (non-shareable).
 */
void tlb_invalidate_all(void)
{
    __asm__ __volatile__(
        "tlbi vmalle1\n\t"
        "dsb sy\n\t"
        "isb"
        :
        :
        : "memory"
    );

    g_tlb_flush_count++;
}

/*
 * tlb_invalidate_all_is
 * ---------------------
 * Invalidate all TLB entries (inner shareable).
 */
void tlb_invalidate_all_is(void)
{
    __asm__ __volatile__(
        "tlbi vmalle1is\n\t"
        "dsb ish\n\t"
        "isb"
        :
        :
        : "memory"
    );

    g_tlb_flush_count++;
}

/*
 * tlb_invalidate_va
 * -----------------
 * Invalidate TLB entry for virtual address (non-shareable).
 */
void tlb_invalidate_va(tak_bertanda64_t va)
{
    __asm__ __volatile__(
        "tlbi vaae1, %0\n\t"
        "dsb sy\n\t"
        "isb"
        :
        : "r"(va >> 12)
        : "memory"
    );

    g_tlb_flush_va_count++;
}

/*
 * tlb_invalidate_va_is
 * --------------------
 * Invalidate TLB entry for virtual address (inner shareable).
 */
void tlb_invalidate_va_is(tak_bertanda64_t va)
{
    __asm__ __volatile__(
        "tlbi vaae1is, %0\n\t"
        "dsb ish\n\t"
        "isb"
        :
        : "r"(va >> 12)
        : "memory"
    );

    g_tlb_flush_va_count++;
}

/*
 * tlb_invalidate_va_asid
 * ----------------------
 * Invalidate TLB entry for VA with specific ASID.
 */
void tlb_invalidate_va_asid(tak_bertanda64_t va, tak_bertanda16_t asid)
{
    tak_bertanda64_t value;

    /* Format: ASID[63:48] . VA[47:12] */
    value = ((tak_bertanda64_t)asid << 48) | ((va >> 12) & 0xFFFFFFFFF);

    __asm__ __volatile__(
        "tlbi vae1, %0\n\t"
        "dsb sy\n\t"
        "isb"
        :
        : "r"(value)
        : "memory"
    );

    g_tlb_flush_va_count++;
}

/*
 * tlb_invalidate_asid
 * -------------------
 * Invalidate all TLB entries for ASID.
 */
void tlb_invalidate_asid(tak_bertanda16_t asid)
{
    tak_bertanda64_t value;

    /* Format: ASID[63:48] */
    value = (tak_bertanda64_t)asid << 48;

    __asm__ __volatile__(
        "tlbi aside1, %0\n\t"
        "dsb sy\n\t"
        "isb"
        :
        : "r"(value)
        : "memory"
    );

    g_tlb_flush_asid_count++;
}

/*
 * tlb_invalidate_asid_is
 * ----------------------
 * Invalidate all TLB entries for ASID (inner shareable).
 */
void tlb_invalidate_asid_is(tak_bertanda16_t asid)
{
    tak_bertanda64_t value;

    value = (tak_bertanda64_t)asid << 48;

    __asm__ __volatile__(
        "tlbi aside1is, %0\n\t"
        "dsb ish\n\t"
        "isb"
        :
        : "r"(value)
        : "memory"
    );

    g_tlb_flush_asid_count++;
}

/*
 * tlb_invalidate_range
 * --------------------
 * Invalidate TLB entries for address range.
 */
void tlb_invalidate_range(tak_bertanda64_t va_start,
                           tak_bertanda64_t va_end)
{
    tak_bertanda64_t va;
    tak_bertanda64_t page;

    va = va_start & ~0xFFFULL;
    va_end = (va_end + 0xFFFULL) & ~0xFFFULL;

    page = 0;

    while (va < va_end) {
        tlb_invalidate_va(va);
        va += PAGE_SIZE;
        page++;

        /* Every 32 pages, do a full TLB flush instead */
        if (page >= 32) {
            tlb_invalidate_all();
            return;
        }
    }
}

/*
 * tlb_invalidate_kernel
 * ---------------------
 * Invalidate kernel TLB entries.
 */
void tlb_invalidate_kernel(void)
{
    /* For ARM64, kernel uses TTBR1, so we need to use appropriate TLBI */
    /* vmalle1 invalidates both TTBR0 and TTBR1 */
    tlb_invalidate_all();
}

/*
 * tlb_invalidate_user
 * -------------------
 * Invalidate user TLB entries.
 */
void tlb_invalidate_user(void)
{
    /* Invalidate TTBR0 entries only would require VMALLE1 */
    tlb_invalidate_all();
}

/*
 * tlb_get_asid_bits
 * -----------------
 * Get ASID size in bits.
 */
tak_bertanda32_t tlb_get_asid_bits(void)
{
    return g_asid_bits;
}

/*
 * tlb_get_asid_mask
 * -----------------
 * Get ASID mask.
 */
tak_bertanda16_t tlb_get_asid_mask(void)
{
    if (g_asid_bits == ASID_BITS_16) {
        return ASID_MASK_16;
    }
    return ASID_MASK_8;
}

/*
 * tlb_get_stats
 * -------------
 * Get TLB statistics.
 */
void tlb_get_stats(tak_bertanda64_t *total_flush,
                   tak_bertanda64_t *va_flush,
                   tak_bertanda64_t *asid_flush)
{
    if (total_flush != NULL) {
        *total_flush = g_tlb_flush_count;
    }
    if (va_flush != NULL) {
        *va_flush = g_tlb_flush_va_count;
    }
    if (asid_flush != NULL) {
        *asid_flush = g_tlb_flush_asid_count;
    }
}

/*
 * tlb_dump_stats
 * --------------
 * Dump TLB statistics.
 */
void tlb_dump_stats(void)
{
    kernel_printf("\n=== TLB Statistics ===\n");
    kernel_printf("Full flushes:  %lu\n", g_tlb_flush_count);
    kernel_printf("VA flushes:    %lu\n", g_tlb_flush_va_count);
    kernel_printf("ASID flushes:  %lu\n", g_tlb_flush_asid_count);
}

/*
 * tlb_barrier
 * -----------
 * Memory barrier after TLB operations.
 */
void tlb_barrier(void)
{
    __asm__ __volatile__(
        "dsb sy\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * tlb_sync
 * --------
 * Synchronize TLB across all CPUs.
 */
void tlb_sync(void)
{
    __asm__ __volatile__(
        "dsb ish\n\t"
        "isb"
        :
        :
        : "memory"
    );
}
