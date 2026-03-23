/*
 * PIGURA OS - CPU_ARMV7.H
 * ------------------------
 * Header untuk fungsi-fungsi CPU spesifik arsitektur ARMv7 (AArch32).
 *
 * Berkas ini berisi deklarasi fungsi inline untuk operasi CPU
 * yang spesifik untuk arsitektur ARMv7 (Cortex-A series).
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan GCC extensions
 * - Menggunakan __inline__ untuk kompatibilitas C89
 *
 * Arsitektur: ARMv7 (32-bit)
 * Versi: 1.0
 * Tanggal: 2025
 */

#ifndef ARSITEKTUR_ARMV7_CPU_ARMV7_H
#define ARSITEKTUR_ARMV7_CPU_ARMV7_H

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

/* CPSR mode bits */
#define ARMV7_MODE_USR          0x10
#define ARMV7_MODE_FIQ          0x11
#define ARMV7_MODE_IRQ          0x12
#define ARMV7_MODE_SVC          0x13
#define ARMV7_MODE_ABT          0x17
#define ARMV7_MODE_UND          0x1B
#define ARMV7_MODE_SYS          0x1F

/* CPSR interrupt mask bits */
#define ARMV7_CPSR_A            (1 << 8)   /* Asynchronous abort mask */
#define ARMV7_CPSR_I            (1 << 7)   /* IRQ mask */
#define ARMV7_CPSR_F            (1 << 6)   /* FIQ mask */
#define ARMV7_CPSR_T            (1 << 5)   /* Thumb state */

/* SCTLR bits */
#define ARMV7_SCTLR_M           (1 << 0)   /* MMU enable */
#define ARMV7_SCTLR_A           (1 << 1)   /* Alignment check */
#define ARMV7_SCTLR_C           (1 << 2)   /* Data cache enable */
#define ARMV7_SCTLR_CP15BEN     (1 << 5)   /* CP15 barrier enable */
#define ARMV7_SCTLR_Z           (1 << 11)  /* Branch prediction enable */
#define ARMV7_SCTLR_I           (1 << 12)  /* Instruction cache enable */
#define ARMV7_SCTLR_V           (1 << 13)  /* High vectors */
#define ARMV7_SCTLR_U           (1 << 22)  /* Unaligned access */

/*
 * ===========================================================================
 * FUNGSI BANTUAN CP15 (CP15 HELPER FUNCTIONS)
 * ===========================================================================
 */

/*
 * cp15_read - Baca register CP15
 */
#define cp15_read(op1, c1, c2, op2) \
    ({ tak_bertanda32_t _val; \
       __asm__ __volatile__( \
           "mrc p15, " #op1 ", %0, " #c1 ", " #c2 ", " #op2 \
           : "=r"(_val) \
       ); \
       _val; })

/*
 * cp15_write - Tulis register CP15
 */
#define cp15_write(op1, c1, c2, op2, val) \
    do { \
        __asm__ __volatile__( \
            "mcr p15, " #op1 ", %0, " #c1 ", " #c2 ", " #op2 \
            : \
            : "r"(val) \
            : "memory" \
        ); \
    } while (0)

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
 * cpu_enable_irq - Aktifkan interupsi IRQ
 */
static __inline__ void cpu_enable_irq(void)
{
    __asm__ __volatile__("cpsie i" ::: "memory");
}

/*
 * cpu_disable_irq - Nonaktifkan interupsi IRQ
 */
static __inline__ void cpu_disable_irq(void)
{
    __asm__ __volatile__("cpsid i" ::: "memory");
}

/*
 * cpu_enable_fiq - Aktifkan interupsi FIQ
 */
static __inline__ void cpu_enable_fiq(void)
{
    __asm__ __volatile__("cpsie f" ::: "memory");
}

/*
 * cpu_disable_fiq - Nonaktifkan interupsi FIQ
 */
static __inline__ void cpu_disable_fiq(void)
{
    __asm__ __volatile__("cpsid f" ::: "memory");
}

/*
 * cpu_enable_all_interrupts - Aktifkan semua interupsi (IRQ, FIQ, A)
 */
static __inline__ void cpu_enable_all_interrupts(void)
{
    __asm__ __volatile__("cpsie aif" ::: "memory");
}

/*
 * cpu_disable_all_interrupts - Nonaktifkan semua interupsi (IRQ, FIQ, A)
 */
static __inline__ void cpu_disable_all_interrupts(void)
{
    __asm__ __volatile__("cpsid aif" ::: "memory");
}

/*
 * cpu_save_flags - Simpan CPSR dan nonaktifkan interupsi
 *
 * Return: Nilai CPSR sebelumnya
 */
static __inline__ tak_bertanda32_t cpu_save_flags(void)
{
    tak_bertanda32_t cpsr;

    __asm__ __volatile__(
        "mrs %0, cpsr\n\t"
        "cpsid aif"
        : "=r"(cpsr)
        :
        : "memory"
    );

    return cpsr;
}

/*
 * cpu_restore_flags - Restore CPSR
 * @flags: Nilai CPSR yang disimpan
 */
static __inline__ void cpu_restore_flags(tak_bertanda32_t flags)
{
    tak_bertanda32_t cpsr;

    __asm__ __volatile__(
        "mrs %0, cpsr"
        : "=r"(cpsr)
    );

    /* Restore A, I, F bits (bits 6, 7, 8) */
    cpsr = (cpsr & ~0x1C0) | (flags & 0x1C0);

    __asm__ __volatile__(
        "msr cpsr_c, %0"
        :
        : "r"(cpsr)
        : "memory"
    );
}

/*
 * cpu_nop - No operation
 */
static __inline__ void cpu_nop(void)
{
    __asm__ __volatile__("nop");
}

/*
 * io_delay - Delay I/O kira-kira 1 mikrodetik
 */
static __inline__ void io_delay(void)
{
    volatile tak_bertanda32_t i;
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
    __asm__ __volatile__("isb sy" ::: "memory");
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
    tak_bertanda32_t va = (tak_bertanda32_t)(tak_bertanda32_t)addr;

    __asm__ __volatile__(
        "mcr p15, 0, %0, c8, c7, 1"
        :
        : "r"(va)
        : "memory"
    );

    cpu_dsb();
    cpu_isb();
}

/*
 * cpu_reload_cr3 - Invalidate seluruh TLB
 *
 * Setara dengan invalidate seluruh TLB pada ARMv7.
 */
static __inline__ void cpu_reload_cr3(void)
{
    __asm__ __volatile__(
        "mcr p15, 0, %0, c8, c7, 0"
        :
        : "r"(0)
        : "memory"
    );

    cpu_dsb();
    cpu_isb();
}

/*
 * cpu_tlb_invalidate_asid - Invalidate TLB untuk ASID tertentu
 * @asid: Address Space ID
 */
static __inline__ void cpu_tlb_invalidate_asid(tak_bertanda8_t asid)
{
    tak_bertanda32_t val = (asid & 0xFF);

    __asm__ __volatile__(
        "mcr p15, 0, %0, c8, c7, 2"
        :
        : "r"(val)
        : "memory"
    );

    cpu_dsb();
    cpu_isb();
}

/*
 * ===========================================================================
 * FUNGSI CACHE (CACHE FUNCTIONS)
 * ===========================================================================
 */

/*
 * cpu_cache_flush - Flush seluruh cache
 *
 * Clean dan invalidate seluruh L1 data cache dan instruction cache.
 */
static __inline__ void cpu_cache_flush(void)
{
    tak_bertanda32_t ccsidr;
    tak_bertanda32_t sets;
    tak_bertanda32_t ways;
    tak_bertanda32_t way_shift;
    tak_bertanda32_t set_shift;
    tak_bertanda32_t way;
    tak_bertanda32_t set;

    /* Select L1 Data Cache */
    __asm__ __volatile__("mcr p15, 0, %0, c0, c0, 0" : : "r"(0));
    cpu_isb();

    /* Read CCSIDR */
    __asm__ __volatile__(
        "mrc p15, 0, %0, c0, c0, 0"
        : "=r"(ccsidr)
    );

    /* Calculate parameters */
    set_shift = 1 << (((ccsidr >> 0) & 0x7) + 4);
    sets = ((ccsidr >> 13) & 0x7FFF) + 1;
    ways = ((ccsidr >> 3) & 0x3FF) + 1;

    /* Calculate way shift */
    way_shift = 32 - __builtin_clz(ways - 1);

    /* Clean and invalidate by set/way */
    for (way = 0; way < ways; way++) {
        for (set = 0; set < sets; set++) {
            tak_bertanda32_t val = (way << way_shift) | (set << set_shift);
            __asm__ __volatile__(
                "mcr p15, 0, %0, c7, c14, 2"
                :
                : "r"(val)
                : "memory"
            );
        }
    }

    /* Invalidate entire I-cache */
    __asm__ __volatile__(
        "mcr p15, 0, %0, c7, c5, 0"
        :
        : "r"(0)
        : "memory"
    );

    /* Invalidate branch predictor */
    __asm__ __volatile__(
        "mcr p15, 0, %0, c7, c5, 6"
        :
        : "r"(0)
        : "memory"
    );

    cpu_dsb();
    cpu_isb();
}

/*
 * cpu_cache_clean_va - Clean cache line untuk alamat tertentu
 * @addr: Alamat virtual
 */
static __inline__ void cpu_cache_clean_va(void *addr)
{
    __asm__ __volatile__(
        "mcr p15, 0, %0, c7, c10, 1"
        :
        : "r"(addr)
        : "memory"
    );
    cpu_dsb();
}

/*
 * cpu_cache_invalidate_va - Invalidate cache line untuk alamat tertentu
 * @addr: Alamat virtual
 */
static __inline__ void cpu_cache_invalidate_va(void *addr)
{
    __asm__ __volatile__(
        "mcr p15, 0, %0, c7, c6, 1"
        :
        : "r"(addr)
        : "memory"
    );
    cpu_dsb();
}

/*
 * cpu_cache_clean_invalidate_va - Clean dan invalidate cache line
 * @addr: Alamat virtual
 */
static __inline__ void cpu_cache_clean_invalidate_va(void *addr)
{
    __asm__ __volatile__(
        "mcr p15, 0, %0, c7, c14, 1"
        :
        : "r"(addr)
        : "memory"
    );
    cpu_dsb();
}

/*
 * cpu_invalidate_icache - Invalidate seluruh instruction cache
 */
static __inline__ void cpu_invalidate_icache(void)
{
    __asm__ __volatile__(
        "mcr p15, 0, %0, c7, c5, 0"
        :
        : "r"(0)
        : "memory"
    );

    __asm__ __volatile__(
        "mcr p15, 0, %0, c7, c5, 6"
        :
        : "r"(0)
        : "memory"
    );

    cpu_isb();
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
 * FUNGSI UTILITAS (UTILITY FUNCTIONS)
 * ===========================================================================
 */

/*
 * cpu_get_current_mode - Dapatkan mode CPU saat ini
 *
 * Return: Mode CPSR
 */
static __inline__ tak_bertanda32_t cpu_get_current_mode(void)
{
    tak_bertanda32_t cpsr;

    __asm__ __volatile__(
        "mrs %0, cpsr"
        : "=r"(cpsr)
    );

    return cpsr & 0x1F;
}

/*
 * cpu_get_cache_line_size - Dapatkan ukuran cache line
 *
 * Return: Ukuran cache line dalam byte
 */
static __inline__ tak_bertanda32_t cpu_get_cache_line_size(void)
{
    tak_bertanda32_t ctr;
    tak_bertanda32_t linesize;

    __asm__ __volatile__(
        "mrc p15, 0, %0, c0, c0, 1"
        : "=r"(ctr)
    );

    linesize = ((ctr >> 0) & 0x7);
    if (linesize == 0) {
        return 16;
    }
    return 4 << linesize;
}

/*
 * cpu_set_vector_base - Set vector base address
 * @base: Alamat base vector table
 */
static __inline__ void cpu_set_vector_base(void *base)
{
    __asm__ __volatile__(
        "mcr p15, 0, %0, c12, c0, 0"
        :
        : "r"((tak_bertanda32_t)(tak_bertanda32_t)base)
        : "memory"
    );
    cpu_isb();
}

/*
 * cpu_get_vector_base - Dapatkan vector base address
 *
 * Return: Alamat base vector table
 */
static __inline__ void *cpu_get_vector_base(void)
{
    tak_bertanda32_t vbar;

    __asm__ __volatile__(
        "mrc p15, 0, %0, c12, c0, 0"
        : "=r"(vbar)
    );

    return (void *)vbar;
}

/*
 * ===========================================================================
 * ALIAS UNTUK KOMPATIBILITAS (COMPATIBILITY ALIASES)
 * ===========================================================================
 */

/* ARM tidak memiliki port I/O, gunakan MMIO */
#define inb(port)   (0)
#define outb(port, val) ((void)0)
#define inw(port)   (0)
#define outw(port, val) ((void)0)
#define inl(port)   (0)
#define outl(port, val) ((void)0)

#endif /* ARSITEKTUR_ARMV7_CPU_ARMV7_H */
