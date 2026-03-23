/*
 * PIGURA OS - CPU_ARM.H
 * ---------------------
 * Header untuk fungsi-fungsi CPU spesifik arsitektur ARM (32-bit).
 *
 * Berkas ini berisi deklarasi fungsi inline untuk operasi CPU
 * yang spesifik untuk arsitektur ARM (ARMv4/v5/v6).
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan GCC extensions
 * - Menggunakan __inline__ untuk kompatibilitas C89
 *
 * Arsitektur: ARM (32-bit)
 * Versi: 1.0
 * Tanggal: 2025
 */

#ifndef ARSITEKTUR_ARM_CPU_ARM_H
#define ARSITEKTUR_ARM_CPU_ARM_H

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
 * KONSTANTA CP15 (CP15 CONSTANTS)
 * ===========================================================================
 */

/* CP15 register opcodes */
#define CP15_OP_MRC_MCR         0

/* Main ID Register */
#define CP15_REG_MIDR           0, c0, c0, 0

/* Cache Type Register */
#define CP15_REG_CTR            0, c0, c0, 1

/* Control Register */
#define CP15_REG_SCTLR          0, c1, c0, 0

/* Translation Table Base 0 */
#define CP15_REG_TTBR0          0, c2, c0, 0

/* Translation Table Base 1 */
#define CP15_REG_TTBR1          0, c2, c0, 1

/* Domain Access Control */
#define CP15_REG_DACR           0, c3, c0, 0

/* Fault Status Register */
#define CP15_REG_DFSR           0, c5, c0, 0

/* Fault Address Register */
#define CP15_REG_DFAR           0, c6, c0, 0

/* TLB Operations */
#define CP15_REG_TLB            0, c8, c0, 0

/* Processor mode bits */
#define ARM_MODE_USR            0x10
#define ARM_MODE_FIQ            0x11
#define ARM_MODE_IRQ            0x12
#define ARM_MODE_SVC            0x13
#define ARM_MODE_ABT            0x17
#define ARM_MODE_UND            0x1B
#define ARM_MODE_SYS            0x1F

/* CPSR bits */
#define ARM_CPSR_IRQ_DISABLE    (1 << 7)
#define ARM_CPSR_FIQ_DISABLE    (1 << 6)

/*
 * ===========================================================================
 * FUNGSI BANTUAN CP15 (CP15 HELPER FUNCTIONS)
 * ===========================================================================
 */

/*
 * cp15_read - Baca register CP15
 * op1, c1, c2, op2: Opcode CP15
 *
 * Return: Nilai register
 *
 * Catatan: Ini adalah makro karena GCC inline assembly tidak bisa
 * menerima immediate values dari variabel.
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
 * op1, c1, c2, op2: Opcode CP15
 * val: Nilai yang akan ditulis
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
 * Menggunakan instruksi WFI (Wait For Interrupt) untuk
 * menghentikan CPU sampai interupsi terjadi.
 */
static __inline__ void cpu_halt(void)
{
    /* Disable interupsi terlebih dahulu */
    __asm__ __volatile__("cpsid if");

    /* Loop forever dengan wait for interrupt */
    while (1) {
        __asm__ __volatile__("mcr p15, 0, r0, c7, c0, 4");
    }
}

/*
 * cpu_enable_irq - Aktifkan interupsi IRQ
 *
 * Menggunakan instruksi CPSIE untuk mengaktifkan IRQ.
 */
static __inline__ void cpu_enable_irq(void)
{
    __asm__ __volatile__("cpsie i" ::: "memory");
}

/*
 * cpu_disable_irq - Nonaktifkan interupsi IRQ
 *
 * Menggunakan instruksi CPSID untuk menonaktifkan IRQ.
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
 * cpu_save_flags - Simpan CPSR dan nonaktifkan interupsi
 *
 * Return: Nilai CPSR sebelumnya
 */
static __inline__ tak_bertanda32_t cpu_save_flags(void)
{
    tak_bertanda32_t cpsr;

    __asm__ __volatile__(
        "mrs %0, cpsr\n\t"
        "cpsid if"
        : "=r"(cpsr)
        :
        : "memory"
    );

    return cpsr;
}

/*
 * cpu_restore_flags - Restore CPSR
 * @flags: Nilai CPSR yang disimpan
 *
 * Mengembalikan nilai CPSR, termasuk status interupsi.
 */
static __inline__ void cpu_restore_flags(tak_bertanda32_t flags)
{
    tak_bertanda32_t cpsr;

    /* Baca CPSR saat ini */
    __asm__ __volatile__(
        "mrs %0, cpsr"
        : "=r"(cpsr)
    );

    /* Restore IRQ/FIQ bits */
    cpsr = (cpsr & ~0xC0) | (flags & 0xC0);

    /* Tulis kembali */
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
 *
 * ARM tidak memiliki port I/O, jadi gunakan busy-wait.
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
    __asm__ __volatile__("mcr p15, 0, r0, c7, c10, 4" ::: "memory");
}

/*
 * cpu_dmb - Data Memory Barrier
 */
static __inline__ void cpu_dmb(void)
{
    __asm__ __volatile__("mcr p15, 0, r0, c7, c10, 5" ::: "memory");
}

/*
 * cpu_isb - Instruction Synchronization Barrier
 */
static __inline__ void cpu_isb(void)
{
    __asm__ __volatile__("mcr p15, 0, r0, c7, c5, 4" ::: "memory");
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

    /* Invalidate TLB entry by MVA */
    __asm__ __volatile__(
        "mcr p15, 0, %0, c8, c7, 1"
        :
        : "r"(va)
        : "memory"
    );

    cpu_dsb();
}

/*
 * cpu_reload_cr3 - Invalidate seluruh TLB
 *
 * Pada ARM, ini setara dengan invalidate seluruh TLB.
 */
static __inline__ void cpu_reload_cr3(void)
{
    /* Invalidate entire TLB */
    __asm__ __volatile__(
        "mcr p15, 0, r0, c8, c7, 0"
        :
        :
        : "memory"
    );

    cpu_dsb();
}

/*
 * ===========================================================================
 * FUNGSI CACHE (CACHE FUNCTIONS)
 * ===========================================================================
 */

/*
 * cpu_cache_flush - Flush seluruh cache
 */
static __inline__ void cpu_cache_flush(void)
{
    /* Clean and invalidate entire D-cache */
    __asm__ __volatile__(
        "mcr p15, 0, r0, c7, c14, 0"
        :
        :
        : "memory"
    );

    /* Invalidate entire I-cache */
    __asm__ __volatile__(
        "mcr p15, 0, r0, c7, c5, 0"
        :
        :
        : "memory"
    );

    cpu_dsb();
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
}

/*
 * ===========================================================================
 * FUNGSI MMIO (MMIO HELPER FUNCTIONS)
 * ===========================================================================
 * ARM menggunakan memory-mapped I/O, bukan port I/O seperti x86.
 */

/*
 * cpu_mmio_read_8 - Baca byte dari MMIO
 * @addr: Alamat MMIO
 *
 * Return: Nilai byte
 */
static __inline__ tak_bertanda8_t cpu_mmio_read_8(void *addr)
{
    tak_bertanda8_t val;
    volatile tak_bertanda8_t *ptr = (volatile tak_bertanda8_t *)addr;

    cpu_dmb();
    val = *ptr;
    cpu_dmb();

    return val;
}

/*
 * cpu_mmio_write_8 - Tulis byte ke MMIO
 * @addr: Alamat MMIO
 * @val: Nilai yang akan ditulis
 */
static __inline__ void cpu_mmio_write_8(void *addr, tak_bertanda8_t val)
{
    volatile tak_bertanda8_t *ptr = (volatile tak_bertanda8_t *)addr;

    cpu_dmb();
    *ptr = val;
    cpu_dmb();
}

/*
 * cpu_mmio_read_16 - Baca word dari MMIO
 * @addr: Alamat MMIO
 *
 * Return: Nilai word
 */
static __inline__ tak_bertanda16_t cpu_mmio_read_16(void *addr)
{
    tak_bertanda16_t val;
    volatile tak_bertanda16_t *ptr = (volatile tak_bertanda16_t *)addr;

    cpu_dmb();
    val = *ptr;
    cpu_dmb();

    return val;
}

/*
 * cpu_mmio_write_16 - Tulis word ke MMIO
 * @addr: Alamat MMIO
 * @val: Nilai yang akan ditulis
 */
static __inline__ void cpu_mmio_write_16(void *addr, tak_bertanda16_t val)
{
    volatile tak_bertanda16_t *ptr = (volatile tak_bertanda16_t *)addr;

    cpu_dmb();
    *ptr = val;
    cpu_dmb();
}

/*
 * cpu_mmio_read_32 - Baca dword dari MMIO
 * @addr: Alamat MMIO
 *
 * Return: Nilai dword
 */
static __inline__ tak_bertanda32_t cpu_mmio_read_32(void *addr)
{
    tak_bertanda32_t val;
    volatile tak_bertanda32_t *ptr = (volatile tak_bertanda32_t *)addr;

    cpu_dmb();
    val = *ptr;
    cpu_dmb();

    return val;
}

/*
 * cpu_mmio_write_32 - Tulis dword ke MMIO
 * @addr: Alamat MMIO
 * @val: Nilai yang akan ditulis
 */
static __inline__ void cpu_mmio_write_32(void *addr, tak_bertanda32_t val)
{
    volatile tak_bertanda32_t *ptr = (volatile tak_bertanda32_t *)addr;

    cpu_dmb();
    *ptr = val;
    cpu_dmb();
}

/*
 * cpu_mmio_read_64 - Baca qword dari MMIO
 * @addr: Alamat MMIO
 *
 * Return: Nilai qword
 */
static __inline__ tak_bertanda64_t cpu_mmio_read_64(void *addr)
{
    tak_bertanda64_t val;
    volatile tak_bertanda64_t *ptr = (volatile tak_bertanda64_t *)addr;

    cpu_dmb();
    val = *ptr;
    cpu_dmb();

    return val;
}

/*
 * cpu_mmio_write_64 - Tulis qword ke MMIO
 * @addr: Alamat MMIO
 * @val: Nilai yang akan ditulis
 */
static __inline__ void cpu_mmio_write_64(void *addr, tak_bertanda64_t val)
{
    volatile tak_bertanda64_t *ptr = (volatile tak_bertanda64_t *)addr;

    cpu_dmb();
    *ptr = val;
    cpu_dmb();
}

/*
 * ===========================================================================
 * ALIAS UNTUK KOMPATIBILITAS (COMPATIBILITY ALIASES)
 * ===========================================================================
 * ARM tidak memiliki port I/O, jadi fungsi-fungsi ini menggunakan MMIO
 * atau stub implementasi.
 */

/* Alias untuk kompatibilitas dengan kode x86 */
#define inb(port)   (0)
#define outb(port, val) ((void)0)
#define inw(port)   (0)
#define outw(port, val) ((void)0)
#define inl(port)   (0)
#define outl(port, val) ((void)0)

#endif /* ARSITEKTUR_ARM_CPU_ARM_H */
