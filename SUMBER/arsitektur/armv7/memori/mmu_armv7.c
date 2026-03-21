/*
 * PIGURA OS - MMU_ARMV7.C
 * ------------------------
 * Implementasi operasi MMU untuk ARMv7.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola Memory Management
 * Unit pada prosesor ARMv7.
 *
 * Arsitektur: ARMv7 (Cortex-A series)
 * Versi: 1.0
 */

#include "../../../inti/types.h"
#include "../../../inti/konstanta.h"
#include "../../../inti/hal/hal.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* CP15 register opcodes */
#define CP15_SCTLR      0, c1, c0, 0       /* System Control Register */
#define CP15_TTBR0      0, c2, c0, 0       /* Translation Table Base 0 */
#define CP15_TTBR1      0, c2, c0, 1       /* Translation Table Base 1 */
#define CP15_TTBCR      0, c2, c0, 2       /* Translation Table Base Control */
#define CP15_DACR       0, c3, c0, 0       /* Domain Access Control */
#define CP15_DFSR       0, c5, c0, 0       /* Data Fault Status */
#define CP15_IFSR       0, c5, c0, 1       /* Instruction Fault Status */
#define CP15_DFAR       0, c6, c0, 0       /* Data Fault Address */
#define CP15_IFAR       0, c6, c0, 2       /* Instruction Fault Address */
#define CP15_PRRR       0, c10, c2, 0      /* Primary Region Remap */
#define CP15_NMRR       0, c10, c2, 1      /* Normal Memory Remap */
#define CP15_VBAR       0, c12, c0, 0      /* Vector Base Address */

/* SCTLR bits */
#define SCTLR_M         (1 << 0)           /* MMU enable */
#define SCTLR_A         (1 << 1)           /* Alignment check */
#define SCTLR_C         (1 << 2)           /* Data cache enable */
#define SCTLR_CP15BEN   (1 << 5)           /* CP15 barrier enable */
#define SCTLR_SW        (1 << 10)          /* SWP/SWPB enable */
#define SCTLR_Z         (1 << 11)          /* Branch prediction enable */
#define SCTLR_I         (1 << 12)          /* Instruction cache enable */
#define SCTLR_V         (1 << 13)          /* High vectors */
#define SCTLR_HA        (1 << 17)          /* Hardware Access flag */
#define SCTLR_EE        (1 << 25)          /* Exception Endianness */
#define SCTLR_NMFI      (1 << 27)          /* Non-maskable FIQ */
#define SCTLR_TRE       (1 << 28)          /* TEX remap enable */
#define SCTLR_AFE       (1 << 29)          /* Access flag enable */
#define SCTLR_TE        (1 << 30)          /* Thumb exception enable */

/* Domain access values */
#define DOMAIN_NO_ACCESS    0x00
#define DOMAIN_CLIENT       0x01
#define DOMAIN_RESERVED     0x02
#define DOMAIN_MANAGER      0x03

/* Memory attributes for TEX remap */
#define MAIR_NORMAL_NC      0x44    /* Normal, non-cacheable */
#define MAIR_NORMAL_WT      0xAA    /* Normal, write-through */
#define MAIR_NORMAL_WB      0xFF    /* Normal, write-back */
#define MAIR_DEVICE         0x00    /* Device memory */

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Current translation table */
static tak_bertanda32_t g_current_ttbr0 = 0;

/* Flag MMU enabled */
static bool_t g_mmu_enabled = SALAH;

/*
 * ============================================================================
 * FUNGSI BANTUAN CP15 (CP15 HELPER FUNCTIONS)
 * ============================================================================
 */

/*
 * mmu_cp15_read
 * -------------
 * Baca register CP15.
 */
static inline tak_bertanda32_t mmu_cp15_read(tak_bertanda32_t op1,
    tak_bertanda32_t c1, tak_bertanda32_t c2, tak_bertanda32_t op2)
{
    tak_bertanda32_t val;
    __asm__ __volatile__(
        "mrc p15, %1, %0, %2, %3, %4"
        : "=r"(val)
        : "i"(op1), "i"(c1), "i"(c2), "i"(op2)
    );
    return val;
}

/*
 * mmu_cp15_write
 * --------------
 * Tulis register CP15.
 */
static inline void mmu_cp15_write(tak_bertanda32_t op1,
    tak_bertanda32_t c1, tak_bertanda32_t c2, tak_bertanda32_t op2,
    tak_bertanda32_t val)
{
    __asm__ __volatile__(
        "mcr p15, %0, %1, %2, %3, %4"
        :
        : "i"(op1), "r"(val), "i"(c1), "i"(c2), "i"(op2)
        : "memory"
    );
}

/*
 * ============================================================================
 * FUNGSI KONTROL MMU (MMU CONTROL FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_mmu_enable
 * --------------
 * Aktifkan MMU.
 *
 * Return: Status operasi
 */
status_t hal_mmu_enable(void)
{
    tak_bertanda32_t sctlr;

    /* Baca SCTLR */
    sctlr = mmu_cp15_read(CP15_SCTLR);

    /* Cek apakah sudah enabled */
    if (sctlr & SCTLR_M) {
        return STATUS_BERHASIL;
    }

    /* Enable MMU, caches, dan branch prediction */
    sctlr |= SCTLR_M | SCTLR_C | SCTLR_I | SCTLR_Z;

    /* Tulis SCTLR */
    mmu_cp15_write(CP15_SCTLR, sctlr);

    /* Instruction synchronization barrier */
    __asm__ __volatile__("isb");

    g_mmu_enabled = BENAR;

    return STATUS_BERHASIL;
}

/*
 * hal_mmu_disable
 * ---------------
 * Nonaktifkan MMU.
 *
 * Return: Status operasi
 */
status_t hal_mmu_disable(void)
{
    tak_bertanda32_t sctlr;

    /* Flush cache terlebih dahulu */
    hal_cpu_cache_flush();

    /* Baca SCTLR */
    sctlr = mmu_cp15_read(CP15_SCTLR);

    /* Disable MMU dan caches */
    sctlr &= ~(SCTLR_M | SCTLR_C | SCTLR_I | SCTLR_Z);

    /* Tulis SCTLR */
    mmu_cp15_write(CP15_SCTLR, sctlr);

    /* Invalidate TLB */
    hal_cpu_invalidate_tlb_all();

    /* ISB */
    __asm__ __volatile__("isb");

    g_mmu_enabled = SALAH;

    return STATUS_BERHASIL;
}

/*
 * hal_mmu_is_enabled
 * ------------------
 * Cek apakah MMU aktif.
 *
 * Return: BENAR jika aktif
 */
bool_t hal_mmu_is_enabled(void)
{
    tak_bertanda32_t sctlr;

    sctlr = mmu_cp15_read(CP15_SCTLR);

    return (sctlr & SCTLR_M) ? BENAR : SALAH;
}

/*
 * ============================================================================
 * FUNGSI TRANSLATION TABLE (TRANSLATION TABLE FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_mmu_set_ttbr0
 * -----------------
 * Set Translation Table Base Register 0.
 *
 * Parameter:
 *   addr - Alamat translation table
 *
 * Return: Status operasi
 */
status_t hal_mmu_set_ttbr0(tak_bertanda32_t addr)
{
    /* Translation table harus 16 KB aligned */
    if (addr & 0x3FFF) {
        return STATUS_PARAM_INVALID;
    }

    /* Set TTBR0 */
    mmu_cp15_write(CP15_TTBR0, addr);

    /* Invalidate TLB */
    hal_cpu_invalidate_tlb_all();

    /* ISB */
    __asm__ __volatile__("isb");

    g_current_ttbr0 = addr;

    return STATUS_BERHASIL;
}

/*
 * hal_mmu_get_ttbr0
 * -----------------
 * Dapatkan Translation Table Base Register 0.
 *
 * Return: Alamat translation table
 */
tak_bertanda32_t hal_mmu_get_ttbr0(void)
{
    return mmu_cp15_read(CP15_TTBR0);
}

/*
 * hal_mmu_set_ttbr1
 * -----------------
 * Set Translation Table Base Register 1.
 *
 * Parameter:
 *   addr - Alamat translation table
 *
 * Return: Status operasi
 */
status_t hal_mmu_set_ttbr1(tak_bertanda32_t addr)
{
    /* Translation table harus 16 KB aligned */
    if (addr & 0x3FFF) {
        return STATUS_PARAM_INVALID;
    }

    /* Set TTBR1 */
    mmu_cp15_write(CP15_TTBR1, addr);

    /* Invalidate TLB */
    hal_cpu_invalidate_tlb_all();

    /* ISB */
    __asm__ __volatile__("isb");

    return STATUS_BERHASIL;
}

/*
 * hal_mmu_get_ttbr1
 * -----------------
 * Dapatkan Translation Table Base Register 1.
 *
 * Return: Alamat translation table
 */
tak_bertanda32_t hal_mmu_get_ttbr1(void)
{
    return mmu_cp15_read(CP15_TTBR1);
}

/*
 * hal_mmu_set_ttbcr
 * -----------------
 * Set Translation Table Base Control Register.
 *
 * Parameter:
 *   value - Nilai TTBCR
 *
 * Return: Status operasi
 */
status_t hal_mmu_set_ttbcr(tak_bertanda32_t value)
{
    mmu_cp15_write(CP15_TTBCR, value);
    __asm__ __volatile__("isb");

    return STATUS_BERHASIL;
}

/*
 * hal_mmu_get_ttbcr
 * -----------------
 * Dapatkan Translation Table Base Control Register.
 *
 * Return: Nilai TTBCR
 */
tak_bertanda32_t hal_mmu_get_ttbcr(void)
{
    return mmu_cp15_read(CP15_TTBCR);
}

/*
 * ============================================================================
 * FUNGSI DOMAIN ACCESS (DOMAIN ACCESS FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_mmu_set_domain_access
 * -------------------------
 * Set Domain Access Control Register.
 *
 * Parameter:
 *   value - Nilai DACR
 *
 * Return: Status operasi
 */
status_t hal_mmu_set_domain_access(tak_bertanda32_t value)
{
    mmu_cp15_write(CP15_DACR, value);
    __asm__ __volatile__("isb");

    return STATUS_BERHASIL;
}

/*
 * hal_mmu_get_domain_access
 * -------------------------
 * Dapatkan Domain Access Control Register.
 *
 * Return: Nilai DACR
 */
tak_bertanda32_t hal_mmu_get_domain_access(void)
{
    return mmu_cp15_read(CP15_DACR);
}

/*
 * hal_mmu_set_domain
 * ------------------
 * Set access mode untuk domain tertentu.
 *
 * Parameter:
 *   domain - Nomor domain (0-15)
 *   access - Access mode
 *
 * Return: Status operasi
 */
status_t hal_mmu_set_domain(tak_bertanda32_t domain, tak_bertanda32_t access)
{
    tak_bertanda32_t dacr;

    if (domain > 15 || access > 3) {
        return STATUS_PARAM_INVALID;
    }

    dacr = mmu_cp15_read(CP15_DACR);
    dacr &= ~(0x3 << (domain * 2));
    dacr |= (access & 0x3) << (domain * 2);
    mmu_cp15_write(CP15_DACR, dacr);

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI FAULT STATUS (FAULT STATUS FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_mmu_get_dfsr
 * ----------------
 * Dapatkan Data Fault Status Register.
 *
 * Return: Nilai DFSR
 */
tak_bertanda32_t hal_mmu_get_dfsr(void)
{
    return mmu_cp15_read(CP15_DFSR);
}

/*
 * hal_mmu_get_ifsr
 * ----------------
 * Dapatkan Instruction Fault Status Register.
 *
 * Return: Nilai IFSR
 */
tak_bertanda32_t hal_mmu_get_ifsr(void)
{
    return mmu_cp15_read(CP15_IFSR);
}

/*
 * hal_mmu_get_dfar
 * ----------------
 * Dapatkan Data Fault Address Register.
 *
 * Return: Nilai DFAR
 */
tak_bertanda32_t hal_mmu_get_dfar(void)
{
    return mmu_cp15_read(CP15_DFAR);
}

/*
 * hal_mmu_get_ifar
 * ----------------
 * Dapatkan Instruction Fault Address Register.
 *
 * Return: Nilai IFAR
 */
tak_bertanda32_t hal_mmu_get_ifar(void)
{
    return mmu_cp15_read(CP15_IFAR);
}

/*
 * ============================================================================
 * FUNGSI TLB (TLB FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_mmu_invalidate_tlb
 * ----------------------
 * Invalidate TLB entry untuk alamat tertentu.
 *
 * Parameter:
 *   addr - Alamat virtual
 */
void hal_mmu_invalidate_tlb(void *addr)
{
    tak_bertanda32_t va = (tak_bertanda32_t)(tak_bertanda32_t)addr;

    /* TLB Invalidate by MVA */
    __asm__ __volatile__(
        "mcr p15, 0, %0, c8, c7, 1"
        :
        : "r"(va)
        : "memory"
    );

    /* Data synchronization barrier */
    __asm__ __volatile__("dsb");
    __asm__ __volatile__("isb");
}

/*
 * hal_mmu_invalidate_tlb_all
 * --------------------------
 * Invalidate seluruh TLB.
 */
void hal_mmu_invalidate_tlb_all(void)
{
    /* TLB Invalidate All */
    __asm__ __volatile__(
        "mcr p15, 0, %0, c8, c7, 0"
        :
        : "r"(0)
        : "memory"
    );

    /* Data synchronization barrier */
    __asm__ __volatile__("dsb");
    __asm__ __volatile__("isb");
}

/*
 * hal_mmu_invalidate_tlb_asid
 * ---------------------------
 * Invalidate TLB untuk ASID tertentu.
 *
 * Parameter:
 *   asid - ASID
 */
void hal_mmu_invalidate_tlb_asid(tak_bertanda32_t asid)
{
    /* TLB Invalidate by ASID */
    __asm__ __volatile__(
        "mcr p15, 0, %0, c8, c7, 2"
        :
        : "r"(asid & 0xFF)
        : "memory"
    );

    __asm__ __volatile__("dsb");
    __asm__ __volatile__("isb");
}

/*
 * ============================================================================
 * FUNGSI MEMORY ATTRIBUTES (MEMORY ATTRIBUTES FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_mmu_set_mair
 * ----------------
 * Set Memory Attribute Indirection Registers.
 *
 * Parameter:
 *   mair0 - MAIR0 value
 *   mair1 - MAIR1 value
 *
 * Return: Status operasi
 */
status_t hal_mmu_set_mair(tak_bertanda32_t mair0, tak_bertanda32_t mair1)
{
    /* Set PRRR (Primary Region Remap Register) */
    mmu_cp15_write(CP15_PRRR, mair0);

    /* Set NMRR (Normal Memory Remap Register) */
    mmu_cp15_write(CP15_NMRR, mair1);

    return STATUS_BERHASIL;
}

/*
 * hal_mmu_get_mair
 * ----------------
 * Dapatkan Memory Attribute Indirection Registers.
 *
 * Parameter:
 *   mair0 - Pointer untuk MAIR0
 *   mair1 - Pointer untuk MAIR1
 */
void hal_mmu_get_mair(tak_bertanda32_t *mair0, tak_bertanda32_t *mair1)
{
    if (mair0 != NULL) {
        *mair0 = mmu_cp15_read(CP15_PRRR);
    }
    if (mair1 != NULL) {
        *mair1 = mmu_cp15_read(CP15_NMRR);
    }
}

/*
 * ============================================================================
 * FUNGSI INISIALISASI (INITIALIZATION FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_mmu_init
 * ------------
 * Inisialisasi MMU.
 *
 * Return: Status operasi
 */
status_t hal_mmu_init(void)
{
    /* Set domain access - client untuk domain 0 */
    hal_mmu_set_domain_access(DOMAIN_CLIENT);

    /* Set TTBCR untuk menggunakan TTBR0 saja */
    hal_mmu_set_ttbcr(0);

    /* Set default memory attributes */
    hal_mmu_set_mair(
        (MAIR_DEVICE << 0) | (MAIR_NORMAL_NC << 8) |
        (MAIR_NORMAL_WT << 16) | (MAIR_NORMAL_WB << 24),
        0
    );

    return STATUS_BERHASIL;
}

/*
 * hal_mmu_shutdown
 * ----------------
 * Shutdown MMU.
 *
 * Return: Status operasi
 */
status_t hal_mmu_shutdown(void)
{
    /* Disable MMU */
    hal_mmu_disable();

    return STATUS_BERHASIL;
}
