/*
 * PIGURA OS - CPU_ARM64.H
 * -----------------------
 * Header untuk fungsi-fungsi CPU spesifik arsitektur ARM64 (AArch64).
 *
 * Berkas ini berisi deklarasi fungsi inline untuk operasi CPU
 * yang spesifik untuk arsitektur ARM64 (64-bit).
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan GCC extensions
 * - Menggunakan __inline__ untuk kompatibilitas C89
 *
 * Arsitektur: ARM64/AArch64 (64-bit)
 * Versi: 1.0
 * Tanggal: 2025
 */

#ifndef ARSITEKTUR_ARM64_CPU_ARM64_H
#define ARSITEKTUR_ARM64_CPU_ARM64_H

#include "../../inti/types.h"

/*
 * ===========================================================================
 * UNDEF MAKRO DARI KERNEL.H (UNDEFINE KERNEL.H MACROS)
 * ===========================================================================
 * Jika kernel.h sudah di-include sebelum berkas ini, makro-makro di bawah
 * mungkin sudah didefinisikan. Kita perlu menghapusnya agar bisa
 * mendefinisikan fungsi inline dengan nama yang sama.
 */

#ifdef cpu_halt
#undef cpu_halt
#endif

#ifdef cpu_enable_irq
#undef cpu_enable_irq
#endif

#ifdef cpu_disable_irq
#undef cpu_disable_irq
#endif

#ifdef cpu_save_flags
#undef cpu_save_flags
#endif

#ifdef cpu_restore_flags
#undef cpu_restore_flags
#endif

#ifdef cpu_nop
#undef cpu_nop
#endif

#ifdef io_delay
#undef io_delay
#endif

#ifdef cpu_dsb
#undef cpu_dsb
#endif

#ifdef cpu_dmb
#undef cpu_dmb
#endif

#ifdef cpu_isb
#undef cpu_isb
#endif

#ifdef cpu_invlpg
#undef cpu_invlpg
#endif

#ifdef cpu_reload_cr3
#undef cpu_reload_cr3
#endif

/*
 * ===========================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ===========================================================================
 */

/* DAIF bits - Exception mask bits */
#define ARM64_DAIF_D            (1 << 3)   /* Debug exception mask */
#define ARM64_DAIF_A            (1 << 2)   /* SError interrupt mask */
#define ARM64_DAIF_I            (1 << 1)   /* IRQ mask */
#define ARM64_DAIF_F            (1 << 0)   /* FIQ mask */

/* SCTLR_EL1 bits */
#define ARM64_SCTLR_M           (1ULL << 0)   /* MMU enable */
#define ARM64_SCTLR_A           (1ULL << 1)   /* Alignment check */
#define ARM64_SCTLR_C           (1ULL << 2)   /* Data cache */
#define ARM64_SCTLR_SA          (1ULL << 3)   /* Stack alignment check */
#define ARM64_SCTLR_I           (1ULL << 12)  /* Instruction cache */
#define ARM64_SCTLR_NXE         (1ULL << 19)  /* No-execute enable */

/* TCR_EL1 bits */
#define ARM64_TCR_T0SZ(x)       ((x) & 0x3F)
#define ARM64_TCR_T1SZ(x)       (((x) & 0x3F) << 16)
#define ARM64_TCR_IRGN0_NC      (0 << 8)
#define ARM64_TCR_IRGN0_WBWA    (1 << 8)
#define ARM64_TCR_IRGN0_WT      (2 << 8)
#define ARM64_TCR_IRGN0_WBnWA   (3 << 8)
#define ARM64_TCR_ORGN0_NC      (0 << 10)
#define ARM64_TCR_ORGN0_WBWA    (1 << 10)
#define ARM64_TCR_ORGN0_WT      (2 << 10)
#define ARM64_TCR_ORGN0_WBnWA   (3 << 10)
#define ARM64_TCR_SH0_NS        (0 << 12)
#define ARM64_TCR_SH0_OS        (2 << 12)
#define ARM64_TCR_SH0_IS        (3 << 12)
#define ARM64_TCR_TG0_4KB       (0 << 14)
#define ARM64_TCR_TG0_64KB      (1 << 14)
#define ARM64_TCR_TG0_16KB      (2 << 14)
#define ARM64_TCR_IPS_32BIT     (0 << 32)
#define ARM64_TCR_IPS_36BIT     (1 << 32)
#define ARM64_TCR_IPS_40BIT     (2 << 32)
#define ARM64_TCR_IPS_42BIT     (3 << 32)
#define ARM64_TCR_IPS_44BIT     (4 << 32)
#define ARM64_TCR_IPS_48BIT     (5 << 32)
#define ARM64_TCR_IPS_52BIT     (6 << 32)

/*
 * ===========================================================================
 * FUNGSI KONTROL CPU (CPU CONTROL FUNCTIONS)
 * ===========================================================================
 */

/*
 * cpu_halt - Hentikan CPU sampai interupsi terjadi
 *
 * Menggunakan instruksi WFI (Wait For Interrupt).
 */
static __inline__ void cpu_halt(void)
{
    __asm__ __volatile__("wfi" ::: "memory");
}

/*
 * cpu_pause - Yield CPU untuk spin-wait
 */
static __inline__ void cpu_pause(void)
{
    __asm__ __volatile__("yield" ::: "memory");
}

/*
 * cpu_nop - No operation
 */
static __inline__ void cpu_nop(void)
{
    __asm__ __volatile__("nop");
}

/*
 * cpu_enable_irq - Aktifkan interupsi IRQ
 */
static __inline__ void cpu_enable_irq(void)
{
    __asm__ __volatile__(
        "msr DAIFClr, #0x2\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * cpu_disable_irq - Nonaktifkan interupsi IRQ
 */
static __inline__ void cpu_disable_irq(void)
{
    __asm__ __volatile__(
        "msr DAIFSet, #0x2\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * cpu_enable_fiq - Aktifkan interupsi FIQ
 */
static __inline__ void cpu_enable_fiq(void)
{
    __asm__ __volatile__(
        "msr DAIFClr, #0x1\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * cpu_disable_fiq - Nonaktifkan interupsi FIQ
 */
static __inline__ void cpu_disable_fiq(void)
{
    __asm__ __volatile__(
        "msr DAIFSet, #0x1\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * cpu_enable_serror - Aktifkan SError
 */
static __inline__ void cpu_enable_serror(void)
{
    __asm__ __volatile__(
        "msr DAIFClr, #0x4\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * cpu_disable_serror - Nonaktifkan SError
 */
static __inline__ void cpu_disable_serror(void)
{
    __asm__ __volatile__(
        "msr DAIFSet, #0x4\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * cpu_enable_debug - Aktifkan debug exception
 */
static __inline__ void cpu_enable_debug(void)
{
    __asm__ __volatile__(
        "msr DAIFClr, #0x8\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * cpu_disable_debug - Nonaktifkan debug exception
 */
static __inline__ void cpu_disable_debug(void)
{
    __asm__ __volatile__(
        "msr DAIFSet, #0x8\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * cpu_enable_all_interrupts - Aktifkan semua exception
 */
static __inline__ void cpu_enable_all_interrupts(void)
{
    __asm__ __volatile__(
        "msr DAIFClr, #0xF\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * cpu_disable_all_interrupts - Nonaktifkan semua exception
 */
static __inline__ void cpu_disable_all_interrupts(void)
{
    __asm__ __volatile__(
        "msr DAIFSet, #0xF\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * cpu_get_daif - Baca DAIF register
 *
 * Return: Nilai DAIF register
 */
static __inline__ tak_bertanda64_t cpu_get_daif(void)
{
    tak_bertanda64_t daif;

    __asm__ __volatile__(
        "mrs %0, DAIF"
        : "=r"(daif)
    );

    return daif;
}

/*
 * cpu_set_daif - Tulis DAIF register
 * @daif: Nilai yang akan ditulis
 */
static __inline__ void cpu_set_daif(tak_bertanda64_t daif)
{
    __asm__ __volatile__(
        "msr DAIF, %0\n\t"
        "isb"
        :
        : "r"(daif)
        : "memory"
    );
}

/*
 * cpu_save_flags - Simpan DAIF dan nonaktifkan interupsi
 *
 * Return: Nilai DAIF sebelumnya
 */
static __inline__ tak_bertanda64_t cpu_save_flags(void)
{
    tak_bertanda64_t daif;

    __asm__ __volatile__(
        "mrs %0, DAIF\n\t"
        "msr DAIFSet, #0xF\n\t"
        "isb"
        : "=r"(daif)
        :
        : "memory"
    );

    return daif;
}

/*
 * cpu_restore_flags - Restore DAIF
 * @flags: Nilai DAIF yang disimpan
 */
static __inline__ void cpu_restore_flags(tak_bertanda64_t flags)
{
    __asm__ __volatile__(
        "msr DAIF, %0\n\t"
        "isb"
        :
        : "r"(flags)
        : "memory"
    );
}

/*
 * io_delay - Delay I/O kira-kira 1 mikrodetik
 */
static __inline__ void io_delay(void)
{
    volatile tak_bertanda64_t i;
    for (i = 0; i < 100; i++) {
        __asm__ __volatile__("nop");
    }
}

/*
 * ===========================================================================
 * FUNGSI BARRIER (BARRIER FUNCTIONS)
 * ===========================================================================
 */

/*
 * cpu_dsb - Data Synchronization Barrier
 */
static __inline__ void cpu_dsb(void)
{
    __asm__ __volatile__("dsb sy" ::: "memory");
}

/*
 * cpu_dmb - Data Memory Barrier
 */
static __inline__ void cpu_dmb(void)
{
    __asm__ __volatile__("dmb sy" ::: "memory");
}

/*
 * cpu_isb - Instruction Synchronization Barrier
 */
static __inline__ void cpu_isb(void)
{
    __asm__ __volatile__("isb" ::: "memory");
}

/*
 * cpu_memory_barrier - Full memory barrier
 */
static __inline__ void cpu_memory_barrier(void)
{
    cpu_dsb();
    cpu_isb();
}

/*
 * ===========================================================================
 * FUNGSI TLB (TLB FUNCTIONS)
 * ===========================================================================
 */

/*
 * cpu_invlpg - Invalidate TLB entry untuk satu halaman
 * @addr: Alamat virtual halaman
 */
static __inline__ void cpu_invlpg(void *addr)
{
    tak_bertanda64_t va = (tak_bertanda64_t)(tak_bertanda64_t)addr;

    __asm__ __volatile__(
        "tlbi VAAE1, %0\n\t"
        "dsb sy\n\t"
        "isb"
        :
        : "r"(va >> 12)
        : "memory"
    );
}

/*
 * cpu_reload_cr3 - Invalidate seluruh TLB
 *
 * Setara dengan invalidate seluruh TLB pada ARM64.
 */
static __inline__ void cpu_reload_cr3(void)
{
    __asm__ __volatile__(
        "tlbi VMALLE1IS\n\t"
        "dsb ish\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * cpu_tlb_invalidate_asid - Invalidate TLB untuk ASID tertentu
 * @asid: Address Space ID
 */
static __inline__ void cpu_tlb_invalidate_asid(tak_bertanda8_t asid)
{
    __asm__ __volatile__(
        "tlbi ASIDE1, %0\n\t"
        "dsb sy\n\t"
        "isb"
        :
        : "r"((tak_bertanda64_t)asid)
        : "memory"
    );
}

/*
 * cpu_tlb_invalidate_all - Invalidate semua TLB (inner shareable)
 */
static __inline__ void cpu_tlb_invalidate_all(void)
{
    __asm__ __volatile__(
        "tlbi VMALLE1IS\n\t"
        "dsb ish\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * ===========================================================================
 * FUNGSI CACHE (CACHE FUNCTIONS)
 * ===========================================================================
 */

/*
 * cpu_get_cache_line_size - Dapatkan ukuran cache line
 *
 * Return: Ukuran cache line dalam byte
 */
static __inline__ tak_bertanda32_t cpu_get_cache_line_size(void)
{
    tak_bertanda64_t ctr;
    tak_bertanda32_t dminline;

    __asm__ __volatile__(
        "mrs %0, ctr_el0"
        : "=r"(ctr)
    );

    dminline = (tak_bertanda32_t)(ctr & 0xF);
    return 4 << dminline;
}

/*
 * cpu_cache_clean_va - Clean cache line untuk alamat tertentu
 * @addr: Alamat virtual
 */
static __inline__ void cpu_cache_clean_va(void *addr)
{
    tak_bertanda64_t line_size;
    tak_bertanda64_t va;

    line_size = (tak_bertanda64_t)cpu_get_cache_line_size();
    va = (tak_bertanda64_t)(tak_bertanda64_t)addr & ~(line_size - 1);

    __asm__ __volatile__(
        "dc cvau, %0"
        :
        : "r"(va)
        : "memory"
    );
}

/*
 * cpu_cache_invalidate_va - Invalidate cache line untuk alamat tertentu
 * @addr: Alamat virtual
 */
static __inline__ void cpu_cache_invalidate_va(void *addr)
{
    tak_bertanda64_t line_size;
    tak_bertanda64_t va;

    line_size = (tak_bertanda64_t)cpu_get_cache_line_size();
    va = (tak_bertanda64_t)(tak_bertanda64_t)addr & ~(line_size - 1);

    __asm__ __volatile__(
        "dc ivau, %0"
        :
        : "r"(va)
        : "memory"
    );
}

/*
 * cpu_cache_clean_invalidate_va - Clean dan invalidate cache line
 * @addr: Alamat virtual
 */
static __inline__ void cpu_cache_clean_invalidate_va(void *addr)
{
    tak_bertanda64_t line_size;
    tak_bertanda64_t va;

    line_size = (tak_bertanda64_t)cpu_get_cache_line_size();
    va = (tak_bertanda64_t)(tak_bertanda64_t)addr & ~(line_size - 1);

    __asm__ __volatile__(
        "dc civac, %0"
        :
        : "r"(va)
        : "memory"
    );
}

/*
 * cpu_invalidate_icache - Invalidate instruction cache untuk alamat
 * @addr: Alamat virtual
 */
static __inline__ void cpu_invalidate_icache_va(void *addr)
{
    tak_bertanda64_t line_size;
    tak_bertanda64_t va;

    line_size = (tak_bertanda64_t)cpu_get_cache_line_size();
    va = (tak_bertanda64_t)(tak_bertanda64_t)addr & ~(line_size - 1);

    __asm__ __volatile__(
        "ic ivau, %0"
        :
        : "r"(va)
        : "memory"
    );
}

/*
 * cpu_cache_flush - Flush seluruh cache
 */
static __inline__ void cpu_cache_flush(void)
{
    /* Clean and invalidate entire data cache */
    __asm__ __volatile__(
        "dc cisw, xzr"
        :
        :
        : "memory"
    );

    /* Invalidate entire I-cache */
    __asm__ __volatile__(
        "ic ialluis"
        :
        :
        : "memory"
    );

    cpu_dsb();
    cpu_isb();
}

/*
 * cpu_branch_predictor_invalidate - Invalidate branch predictor
 */
static __inline__ void cpu_branch_predictor_invalidate(void)
{
    __asm__ __volatile__(
        "ic IALLUIS\n\t"
        "dsb ish\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * ===========================================================================
 * FUNGSI SYSTEM REGISTER (SYSTEM REGISTER FUNCTIONS)
 * ===========================================================================
 */

/*
 * cpu_get_current_el - Dapatkan current exception level
 *
 * Return: Exception level (0, 1, 2, atau 3)
 */
static __inline__ tak_bertanda32_t cpu_get_current_el(void)
{
    tak_bertanda64_t currentel;

    __asm__ __volatile__(
        "mrs %0, CurrentEL"
        : "=r"(currentel)
    );

    return (tak_bertanda32_t)((currentel >> 2) & 0x3);
}

/*
 * cpu_get_sctlr - Baca SCTLR_EL1
 *
 * Return: Nilai SCTLR_EL1
 */
static __inline__ tak_bertanda64_t cpu_get_sctlr(void)
{
    tak_bertanda64_t sctlr;

    __asm__ __volatile__(
        "mrs %0, sctlr_el1"
        : "=r"(sctlr)
    );

    return sctlr;
}

/*
 * cpu_set_sctlr - Tulis SCTLR_EL1
 * @sctlr: Nilai yang akan ditulis
 */
static __inline__ void cpu_set_sctlr(tak_bertanda64_t sctlr)
{
    __asm__ __volatile__(
        "msr sctlr_el1, %0\n\t"
        "isb"
        :
        : "r"(sctlr)
        : "memory"
    );
}

/*
 * cpu_get_tcr - Baca TCR_EL1
 *
 * Return: Nilai TCR_EL1
 */
static __inline__ tak_bertanda64_t cpu_get_tcr(void)
{
    tak_bertanda64_t tcr;

    __asm__ __volatile__(
        "mrs %0, tcr_el1"
        : "=r"(tcr)
    );

    return tcr;
}

/*
 * cpu_set_tcr - Tulis TCR_EL1
 * @tcr: Nilai yang akan ditulis
 */
static __inline__ void cpu_set_tcr(tak_bertanda64_t tcr)
{
    __asm__ __volatile__(
        "msr tcr_el1, %0\n\t"
        "isb"
        :
        : "r"(tcr)
        : "memory"
    );
}

/*
 * cpu_get_ttbr0 - Baca TTBR0_EL1
 *
 * Return: Nilai TTBR0_EL1
 */
static __inline__ tak_bertanda64_t cpu_get_ttbr0(void)
{
    tak_bertanda64_t ttbr0;

    __asm__ __volatile__(
        "mrs %0, ttbr0_el1"
        : "=r"(ttbr0)
    );

    return ttbr0;
}

/*
 * cpu_set_ttbr0 - Tulis TTBR0_EL1
 * @ttbr0: Nilai yang akan ditulis
 */
static __inline__ void cpu_set_ttbr0(tak_bertanda64_t ttbr0)
{
    __asm__ __volatile__(
        "msr ttbr0_el1, %0\n\t"
        "isb"
        :
        : "r"(ttbr0)
        : "memory"
    );
}

/*
 * cpu_get_ttbr1 - Baca TTBR1_EL1
 *
 * Return: Nilai TTBR1_EL1
 */
static __inline__ tak_bertanda64_t cpu_get_ttbr1(void)
{
    tak_bertanda64_t ttbr1;

    __asm__ __volatile__(
        "mrs %0, ttbr1_el1"
        : "=r"(ttbr1)
    );

    return ttbr1;
}

/*
 * cpu_set_ttbr1 - Tulis TTBR1_EL1
 * @ttbr1: Nilai yang akan ditulis
 */
static __inline__ void cpu_set_ttbr1(tak_bertanda64_t ttbr1)
{
    __asm__ __volatile__(
        "msr ttbr1_el1, %0\n\t"
        "isb"
        :
        : "r"(ttbr1)
        : "memory"
    );
}

/*
 * cpu_get_vbar - Baca VBAR_EL1
 *
 * Return: Nilai VBAR_EL1
 */
static __inline__ tak_bertanda64_t cpu_get_vbar(void)
{
    tak_bertanda64_t vbar;

    __asm__ __volatile__(
        "mrs %0, vbar_el1"
        : "=r"(vbar)
    );

    return vbar;
}

/*
 * cpu_set_vbar - Tulis VBAR_EL1
 * @vbar: Nilai yang akan ditulis
 */
static __inline__ void cpu_set_vbar(tak_bertanda64_t vbar)
{
    __asm__ __volatile__(
        "msr vbar_el1, %0\n\t"
        "isb"
        :
        : "r"(vbar)
        : "memory"
    );
}

/*
 * cpu_get_mpidr - Baca MPIDR_EL1
 *
 * Return: Nilai MPIDR_EL1
 */
static __inline__ tak_bertanda64_t cpu_get_mpidr(void)
{
    tak_bertanda64_t mpidr;

    __asm__ __volatile__(
        "mrs %0, mpidr_el1"
        : "=r"(mpidr)
    );

    return mpidr;
}

/*
 * cpu_get_midr - Baca MIDR_EL1
 *
 * Return: Nilai MIDR_EL1
 */
static __inline__ tak_bertanda64_t cpu_get_midr(void)
{
    tak_bertanda64_t midr;

    __asm__ __volatile__(
        "mrs %0, midr_el1"
        : "=r"(midr)
    );

    return midr;
}

/*
 * ===========================================================================
 * FUNGSI MMU (MMU FUNCTIONS)
 * ===========================================================================
 */

/*
 * cpu_mmu_enable - Aktifkan MMU
 */
static __inline__ void cpu_mmu_enable(void)
{
    tak_bertanda64_t sctlr;

    sctlr = cpu_get_sctlr();
    sctlr |= ARM64_SCTLR_M;
    cpu_set_sctlr(sctlr);
}

/*
 * cpu_mmu_disable - Nonaktifkan MMU
 */
static __inline__ void cpu_mmu_disable(void)
{
    tak_bertanda64_t sctlr;

    sctlr = cpu_get_sctlr();
    sctlr &= ~ARM64_SCTLR_M;
    cpu_set_sctlr(sctlr);
}

/*
 * cpu_cache_enable - Aktifkan cache
 */
static __inline__ void cpu_cache_enable(void)
{
    tak_bertanda64_t sctlr;

    sctlr = cpu_get_sctlr();
    sctlr |= (ARM64_SCTLR_C | ARM64_SCTLR_I);
    cpu_set_sctlr(sctlr);
}

/*
 * cpu_cache_disable - Nonaktifkan cache
 */
static __inline__ void cpu_cache_disable(void)
{
    tak_bertanda64_t sctlr;

    sctlr = cpu_get_sctlr();
    sctlr &= ~(ARM64_SCTLR_C | ARM64_SCTLR_I);
    cpu_set_sctlr(sctlr);
}

/*
 * ===========================================================================
 * FUNGSI TIMER (TIMER FUNCTIONS)
 * ===========================================================================
 */

/*
 * cpu_get_timer_freq - Dapatkan frekuensi timer
 *
 * Return: Frekuensi timer dalam Hz
 */
static __inline__ tak_bertanda64_t cpu_get_timer_freq(void)
{
    tak_bertanda64_t freq;

    __asm__ __volatile__(
        "mrs %0, cntfrq_el0"
        : "=r"(freq)
    );

    return freq;
}

/*
 * cpu_get_timer_count - Dapatkan counter timer
 *
 * Return: Nilai counter timer
 */
static __inline__ tak_bertanda64_t cpu_get_timer_count(void)
{
    tak_bertanda64_t count;

    __asm__ __volatile__(
        "mrs %0, cntvct_el0"
        : "=r"(count)
    );

    return count;
}

/*
 * cpu_read_tsc - Alias untuk cpu_get_timer_count (kompatibilitas)
 *
 * Return: Nilai counter timer
 */
static __inline__ tak_bertanda64_t cpu_read_tsc(void)
{
    return cpu_get_timer_count();
}

/*
 * ===========================================================================
 * FUNGSI MMIO (MMIO HELPER FUNCTIONS)
 * ===========================================================================
 */

/*
 * cpu_mmio_read_8 - Baca byte dari MMIO
 */
static __inline__ tak_bertanda8_t cpu_mmio_read_8(void *addr)
{
    tak_bertanda8_t val;
    volatile tak_bertanda8_t *ptr = (volatile tak_bertanda8_t *)addr;

    cpu_dsb();
    val = *ptr;
    cpu_dmb();

    return val;
}

/*
 * cpu_mmio_write_8 - Tulis byte ke MMIO
 */
static __inline__ void cpu_mmio_write_8(void *addr, tak_bertanda8_t val)
{
    volatile tak_bertanda8_t *ptr = (volatile tak_bertanda8_t *)addr;

    cpu_dsb();
    *ptr = val;
    cpu_dmb();
}

/*
 * cpu_mmio_read_16 - Baca word dari MMIO
 */
static __inline__ tak_bertanda16_t cpu_mmio_read_16(void *addr)
{
    tak_bertanda16_t val;
    volatile tak_bertanda16_t *ptr = (volatile tak_bertanda16_t *)addr;

    cpu_dsb();
    val = *ptr;
    cpu_dmb();

    return val;
}

/*
 * cpu_mmio_write_16 - Tulis word ke MMIO
 */
static __inline__ void cpu_mmio_write_16(void *addr, tak_bertanda16_t val)
{
    volatile tak_bertanda16_t *ptr = (volatile tak_bertanda16_t *)addr;

    cpu_dsb();
    *ptr = val;
    cpu_dmb();
}

/*
 * cpu_mmio_read_32 - Baca dword dari MMIO
 */
static __inline__ tak_bertanda32_t cpu_mmio_read_32(void *addr)
{
    tak_bertanda32_t val;
    volatile tak_bertanda32_t *ptr = (volatile tak_bertanda32_t *)addr;

    cpu_dsb();
    val = *ptr;
    cpu_dmb();

    return val;
}

/*
 * cpu_mmio_write_32 - Tulis dword ke MMIO
 */
static __inline__ void cpu_mmio_write_32(void *addr, tak_bertanda32_t val)
{
    volatile tak_bertanda32_t *ptr = (volatile tak_bertanda32_t *)addr;

    cpu_dsb();
    *ptr = val;
    cpu_dmb();
}

/*
 * cpu_mmio_read_64 - Baca qword dari MMIO
 */
static __inline__ tak_bertanda64_t cpu_mmio_read_64(void *addr)
{
    tak_bertanda64_t val;
    volatile tak_bertanda64_t *ptr = (volatile tak_bertanda64_t *)addr;

    cpu_dsb();
    val = *ptr;
    cpu_dmb();

    return val;
}

/*
 * cpu_mmio_write_64 - Tulis qword ke MMIO
 */
static __inline__ void cpu_mmio_write_64(void *addr, tak_bertanda64_t val)
{
    volatile tak_bertanda64_t *ptr = (volatile tak_bertanda64_t *)addr;

    cpu_dsb();
    *ptr = val;
    cpu_dmb();
}

/*
 * ===========================================================================
 * ALIAS UNTUK KOMPATIBILITAS (COMPATIBILITY ALIASES)
 * ===========================================================================
 */

/* ARM64 tidak memiliki port I/O, gunakan MMIO */
#define inb(port)   (0)
#define outb(port, val) ((void)0)
#define inw(port)   (0)
#define outw(port, val) ((void)0)
#define inl(port)   (0)
#define outl(port, val) ((void)0)

/* Alias MMIO untuk konsistensi */
#define hal_mmio_read_8   cpu_mmio_read_8
#define hal_mmio_write_8  cpu_mmio_write_8
#define hal_mmio_read_16  cpu_mmio_read_16
#define hal_mmio_write_16 cpu_mmio_write_16
#define hal_mmio_read_32  cpu_mmio_read_32
#define hal_mmio_write_32 cpu_mmio_write_32
#define hal_mmio_read_64  cpu_mmio_read_64
#define hal_mmio_write_64 cpu_mmio_write_64

#endif /* ARSITEKTUR_ARM64_CPU_ARM64_H */
