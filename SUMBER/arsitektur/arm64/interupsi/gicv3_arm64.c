/*
 * PIGURA OS - GICV3_ARM64.C
 * -------------------------
 * Implementasi driver GICv3 (Generic Interrupt Controller v3)
 * untuk ARM64 (AArch64).
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola GICv3
 * pada prosesor ARM64.
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

/* GICv3 Distributor register offsets */
#define GICD_CTLR               0x000
#define GICD_TYPER              0x004
#define GICD_IIDR               0x008
#define GICD_TYPER2             0x00C
#define GICD_STATUSR            0x010
#define GICD_SETSPI_NSR         0x040
#define GICD_CLRSPI_NSR         0x048
#define GICD_SETSPI_SR          0x050
#define GICD_CLRSPI_SR          0x058
#define GICD_IGROUPR            0x080
#define GICD_IGRPMODR           0x0D00
#define GICD_ISENABLER          0x100
#define GICD_ICENABLER          0x180
#define GICD_ISPENDR            0x200
#define GICD_ICPENDR            0x280
#define GICD_ISACTIVER          0x300
#define GICD_ICACTIVER          0x380
#define GICD_IPRIORITYR         0x400
#define GICD_ICFGR              0xC00
#define GICD_IGRPMODR           0xD00
#define GICD_NSACR              0xE00

/* GICD_CTLR bits */
#define GICD_CTLR_ENABLE_GRP0   (1 << 0)
#define GICD_CTLR_ENABLE_GRP1   (1 << 1)
#define GICD_CTLR_ENABLE_GRP1A  (1 << 1)
#define GICD_CTLR_ENABLE_GRP1NS (1 << 1)
#define GICD_CTLR_ARE_NS        (1 << 4)
#define GICD_CTLR_ARE_S         (1 << 5)
#define GICD_CTLR_DS            (1 << 6)
#define GICD_CTLR_E1NWF         (1 << 7)
#define GICD_CTLR_RWP           (1 << 31)

/* GICD_TYPER bits */
#define GICD_TYPER_ITLINES_SHIFT    0
#define GICD_TYPER_ITLINES_MASK     0x1F
#define GICD_TYPER_CPUNUM_SHIFT     5
#define GICD_TYPER_CPUNUM_MASK      0x7
#define GICD_TYPER_SECURITY_EXT     (1 << 10)
#define GICD_TYPER_LPI_SUPPORT      (1 << 17)
#define GICD_TYPER_MBIS             (1 << 16)
#define GICD_TYPER_NO1N             (1 << 19)
#define GICD_TYPER_AFFINITY_3_0     (1 << 24)

/* GICv3 Redistributor register offsets (RD base) */
#define GICR_CTLR               0x000
#define GICR_IIDR               0x004
#define GICR_TYPER              0x008
#define GICR_STATUSR            0x010
#define GICR_WAKER              0x014
#define GICR_MPAMIDR            0x018
#define GICR_PARTIDR            0x01C
#define GICR_SETLPIR            0x040
#define GICR_CLRLPIR            0x048
#define GICR_PROPBASER          0x070
#define GICR_PENDBASER          0x078
#define GICR_INVLPIR            0x0A0
#define GICR_INVALLR            0x0B0
#define GICR_SYNCR              0x0C0

/* GICR_SGI base offsets */
#define GICR_IGROUPR0           0x080
#define GICR_ISENABLER0         0x100
#define GICR_ICENABLER0         0x180
#define GICR_ISPENDR0           0x200
#define GICR_ICPENDR0           0x280
#define GICR_ISACTIVER0         0x300
#define GICR_ICACTIVER0         0x380
#define GICR_IPRIORITYR0        0x400
#define GICR_ICFGR0             0xC00
#define GICR_NSACR              0xE00

/* GICR_CTLR bits */
#define GICR_CTLR_ENABLE_LPIS   (1 << 0)
#define GICR_CTLR_RWP           (1 << 3)
#define GICR_CTLR_DPG0          (1 << 24)
#define GICR_CTLR_DPG1NS        (1 << 25)
#define GICR_CTLR_DPG1S         (1 << 26)
#define GICR_CTLR_UWP           (1 << 31)

/* GICR_WAKER bits */
#define GICR_WAKER_PS           (1 << 1)
#define GICR_WAKER_CA           (1 << 2)

/* GICR_TYPER bits */
#define GICR_TYPER_PLPIS        (1ULL << 0)
#define GICR_TYPER_VLPIS        (1ULL << 1)
#define GICR_TYPER_LAST         (1ULL << 4)

/* ICC system registers */
#define ICC_SRE_EL1             S3_0_C12_C12_5
#define ICC_SRE_EL2             S3_4_C12_C9_5
#define ICC_SRE_EL3             S3_6_C12_C12_5
#define ICC_CTLR_EL1            S3_0_C12_C12_4
#define ICC_CTLR_EL3            S3_6_C12_C12_4
#define ICC_PMR_EL1             S3_0_C4_C6_0
#define ICC_BPR0_EL1            S3_0_C12_C8_3
#define ICC_BPR1_EL1            S3_0_C12_C12_3
#define ICC_IGRPEN0_EL1         S3_0_C12_C12_6
#define ICC_IGRPEN1_EL1         S3_0_C12_C12_7
#define ICC_IAR0_EL1            S3_0_C12_C8_0
#define ICC_IAR1_EL1            S3_0_C12_C12_0
#define ICC_EOIR0_EL1           S3_0_C12_C8_1
#define ICC_EOIR1_EL1           S3_0_C12_C12_1
#define ICC_DIR_EL1             S3_0_C12_C11_1
#define ICC_SGI1R_EL1           S3_0_C12_C11_5
#define ICC_SGI0R_EL1           S3_0_C12_C11_7

/* Default base addresses */
#define GICD_BASE_DEFAULT       0x08000000ULL
#define GICR_BASE_DEFAULT       0x080A0000ULL
#define GICR_STRIDE             0x20000ULL

/* Maximum interrupts */
#define GIC_MAX_IRQ             1020
#define GIC_SPURIOUS_IRQ        1023

/*
 * ============================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ============================================================================
 */

/* GIC base addresses */
static volatile tak_bertanda8_t *g_gicd_base = NULL;
static volatile tak_bertanda8_t *g_gicr_base = NULL;

/* GIC info */
static tak_bertanda32_t g_gic_lines = 0;
static tak_bertanda32_t g_gic_cpu_num = 0;
static bool_t g_gic_security_ext = SALAH;
static bool_t g_gic_lpi_support = SALAH;

/* Flag */
static bool_t g_gic_initialized = SALAH;

/*
 * ============================================================================
 * FUNGSI BANTUAN (HELPER FUNCTIONS)
 * ============================================================================
 */

/*
 * _gicd_read32
 * ------------
 * Read 32-bit GICD register.
 */
static inline tak_bertanda32_t _gicd_read32(tak_bertanda32_t offset)
{
    volatile tak_bertanda32_t *reg;

    reg = (volatile tak_bertanda32_t *)(g_gicd_base + offset);

    return *reg;
}

/*
 * _gicd_write32
 * -------------
 * Write 32-bit GICD register.
 */
static inline void _gicd_write32(tak_bertanda32_t offset,
                                  tak_bertanda32_t val)
{
    volatile tak_bertanda32_t *reg;

    reg = (volatile tak_bertanda32_t *)(g_gicd_base + offset);
    *reg = val;
}

/*
 * _gicd_read8
 * -----------
 * Read 8-bit GICD register.
 */
static inline tak_bertanda8_t _gicd_read8(tak_bertanda32_t offset)
{
    return *(g_gicd_base + offset);
}

/*
 * _gicd_write8
 * ------------
 * Write 8-bit GICD register.
 */
static inline void _gicd_write8(tak_bertanda32_t offset,
                                 tak_bertanda8_t val)
{
    *(g_gicd_base + offset) = val;
}

/*
 * _gicr_read32
 * ------------
 * Read 32-bit GICR register.
 */
static inline tak_bertanda32_t _gicr_read32(tak_bertanda32_t offset)
{
    volatile tak_bertanda32_t *reg;

    reg = (volatile tak_bertanda32_t *)(g_gicr_base + offset);

    return *reg;
}

/*
 * _gicr_write32
 * -------------
 * Write 32-bit GICR register.
 */
static inline void _gicr_write32(tak_bertanda32_t offset,
                                  tak_bertanda32_t val)
{
    volatile tak_bertanda32_t *reg;

    reg = (volatile tak_bertanda32_t *)(g_gicr_base + offset);
    *reg = val;
}

/*
 * _gicr_read64
 * ------------
 * Read 64-bit GICR register.
 */
static inline tak_bertanda64_t _gicr_read64(tak_bertanda32_t offset)
{
    volatile tak_bertanda64_t *reg;

    reg = (volatile tak_bertanda64_t *)(g_gicr_base + offset);

    return *reg;
}

/*
 * _gicr_write64
 * -------------
 * Write 64-bit GICR register.
 */
static inline void _gicr_write64(tak_bertanda32_t offset,
                                  tak_bertanda64_t val)
{
    volatile tak_bertanda64_t *reg;

    reg = (volatile tak_bertanda64_t *)(g_gicr_base + offset);
    *reg = val;
}

/*
 * _get_redistributor_base
 * -----------------------
 * Get redistributor base for current CPU.
 */
static volatile tak_bertanda8_t *_get_redistributor_base(void)
{
    tak_bertanda64_t mpidr;
    tak_bertanda64_t typer;
    volatile tak_bertanda8_t *rbase;
    tak_bertanda32_t i;

    /* Get current MPIDR */
    __asm__ __volatile__(
        "mrs %0, MPIDR_EL1"
        : "=r"(mpidr)
    );

    /* Search for our redistributor */
    rbase = g_gicr_base;

    for (i = 0; i < 128; i++) {
        typer = _gicr_read64((tak_bertanda32_t)(rbase - g_gicr_base) +
                             GICR_TYPER);

        /* Check affinity */
        if (((typer >> 32) & 0xFFFFFFFF) ==
            ((mpidr >> 32) & 0xFFFFFFFF) &&
            ((typer >> 16) & 0xFFFF) ==
            ((mpidr >> 16) & 0xFFFF) &&
            ((typer >> 8) & 0xFF) ==
            ((mpidr >> 8) & 0xFF) &&
            (typer & 0xFF) == (mpidr & 0xFF)) {
            return rbase;
        }

        /* Check LAST bit */
        if (typer & GICR_TYPER_LAST) {
            break;
        }

        /* Move to next redistributor */
        rbase += GICR_STRIDE;
    }

    /* Fallback to base */
    return g_gicr_base;
}

/*
 * _wait_for_rwp
 * -------------
 * Wait for RWP (Read-Write Pending) to clear.
 */
static void _wait_for_rwp(void)
{
    tak_bertanda32_t timeout;

    timeout = 100000;
    while (timeout-- > 0) {
        if (!(_gicd_read32(GICD_CTLR) & GICD_CTLR_RWP)) {
            break;
        }
    }
}

/*
 * _wait_for_rwp_rdist
 * -------------------
 * Wait for redistributor RWP to clear.
 */
static void _wait_for_rwp_rdist(volatile tak_bertanda8_t *rbase)
{
    tak_bertanda32_t timeout;

    timeout = 100000;
    while (timeout-- > 0) {
        volatile tak_bertanda32_t *ctlr;
        ctlr = (volatile tak_bertanda32_t *)(rbase + GICR_CTLR);
        if (!(*ctlr & GICR_CTLR_RWP)) {
            break;
        }
    }
}

/*
 * ============================================================================
 * INITIALIZATION
 * ============================================================================
 */

/*
 * gicv3_init
 * ----------
 * Initialize GICv3.
 */
status_t gicv3_init(void)
{
    tak_bertanda32_t typer;
    tak_bertanda32_t i;
    volatile tak_bertanda8_t *rbase;

    if (g_gic_initialized) {
        return STATUS_BERHASIL;
    }

    /* Set base addresses */
    g_gicd_base = (volatile tak_bertanda8_t *)GICD_BASE_DEFAULT;
    g_gicr_base = (volatile tak_bertanda8_t *)GICR_BASE_DEFAULT;

    /* Read GICD_TYPER */
    typer = _gicd_read32(GICD_TYPER);

    g_gic_lines = (typer >> GICD_TYPER_ITLINES_SHIFT) &
                  GICD_TYPER_ITLINES_MASK;
    g_gic_lines = (g_gic_lines + 1) * 32;

    g_gic_cpu_num = ((typer >> GICD_TYPER_CPUNUM_SHIFT) &
                     GICD_TYPER_CPUNUM_MASK) + 1;

    g_gic_security_ext = (typer & GICD_TYPER_SECURITY_EXT) ? BENAR : SALAH;
    g_gic_lpi_support = (typer & GICD_TYPER_LPI_SUPPORT) ? BENAR : SALAH;

    kernel_printf("[GICv3] Distributor at 0x%lX\n",
        (tak_bertanda64_t)g_gicd_base);
    kernel_printf("[GICv3] Redistributor at 0x%lX\n",
        (tak_bertanda64_t)g_gicr_base);
    kernel_printf("[GICv3] Lines: %u, CPUs: %u\n",
        g_gic_lines, g_gic_cpu_num);

    /* Disable distributor */
    _gicd_write32(GICD_CTLR, 0);
    _wait_for_rwp();

    /* Set all interrupts to Group 1 (non-secure) */
    for (i = 0; i < g_gic_lines / 32; i++) {
        _gicd_write32(GICD_IGROUPR + i * 4, 0xFFFFFFFF);
    }

    /* Disable all interrupts */
    for (i = 0; i < g_gic_lines / 32; i++) {
        _gicd_write32(GICD_ICENABLER + i * 4, 0xFFFFFFFF);
    }

    /* Clear all pending interrupts */
    for (i = 0; i < g_gic_lines / 32; i++) {
        _gicd_write32(GICD_ICPENDR + i * 4, 0xFFFFFFFF);
    }

    /* Set all priorities to lowest (0xFF) */
    for (i = 0; i < g_gic_lines; i++) {
        _gicd_write8(GICD_IPRIORITYR + i, 0xFF);
    }

    /* Set all interrupts to level-triggered */
    for (i = 0; i < g_gic_lines / 16; i++) {
        _gicd_write32(GICD_ICFGR + i * 4, 0);
    }

    /* Enable distributor (Group 1) */
    _gicd_write32(GICD_CTLR, GICD_CTLR_ENABLE_GRP1NS);
    _wait_for_rwp();

    /* Initialize redistributor */
    rbase = _get_redistributor_base();

    /* Wake up redistributor */
    _gicr_write32((tak_bertanda32_t)(rbase - g_gicr_base) + GICR_WAKER, 0);

    /* Wait for CA (Children Asleep) to clear */
    {
        tak_bertanda32_t timeout = 100000;
        while (timeout-- > 0) {
            tak_bertanda32_t waker;
            waker = _gicr_read32((tak_bertanda32_t)(rbase - g_gicr_base) +
                                 GICR_WAKER);
            if (!(waker & GICR_WAKER_CA)) {
                break;
            }
        }
    }

    /* Configure CPU interface */
    /* Enable system register access */
    __asm__ __volatile__(
        "msr ICC_SRE_EL1, %0\n\t"
        "isb"
        :
        : "r"(0x7)
        : "memory"
    );

    /* Set priority mask */
    __asm__ __volatile__(
        "msr ICC_PMR_EL1, %0\n\t"
        "isb"
        :
        : "r"(0xFF)
        : "memory"
    );

    /* Set binary point */
    __asm__ __volatile__(
        "msr ICC_BPR1_EL1, %0\n\t"
        "isb"
        :
        : "r"(0)
        : "memory"
    );

    /* Enable Group 1 interrupts */
    __asm__ __volatile__(
        "msr ICC_IGRPEN1_EL1, %0\n\t"
        "isb"
        :
        : "r"(0x1)
        : "memory"
    );

    g_gic_initialized = BENAR;

    kernel_printf("[GICv3] Initialized\n");

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * INTERRUPT MANAGEMENT
 * ============================================================================
 */

/*
 * gicv3_enable_irq
 * ----------------
 * Enable interrupt.
 */
status_t gicv3_enable_irq(tak_bertanda32_t irq)
{
    if (irq >= g_gic_lines) {
        return STATUS_PARAM_INVALID;
    }

    if (irq < 32) {
        /* Private interrupt (SGI/PPI) - use redistributor */
        volatile tak_bertanda8_t *rbase;
        rbase = _get_redistributor_base();
        _gicr_write32((tak_bertanda32_t)(rbase - g_gicr_base) +
                      GICR_ISENABLER0, (1 << irq));
    } else {
        /* Shared interrupt (SPI) - use distributor */
        tak_bertanda32_t reg_idx = irq / 32;
        tak_bertanda32_t bit_idx = irq % 32;
        _gicd_write32(GICD_ISENABLER + reg_idx * 4, (1 << bit_idx));
    }

    return STATUS_BERHASIL;
}

/*
 * gicv3_disable_irq
 * -----------------
 * Disable interrupt.
 */
status_t gicv3_disable_irq(tak_bertanda32_t irq)
{
    if (irq >= g_gic_lines) {
        return STATUS_PARAM_INVALID;
    }

    if (irq < 32) {
        volatile tak_bertanda8_t *rbase;
        rbase = _get_redistributor_base();
        _gicr_write32((tak_bertanda32_t)(rbase - g_gicr_base) +
                      GICR_ICENABLER0, (1 << irq));
    } else {
        tak_bertanda32_t reg_idx = irq / 32;
        tak_bertanda32_t bit_idx = irq % 32;
        _gicd_write32(GICD_ICENABLER + reg_idx * 4, (1 << bit_idx));
    }

    return STATUS_BERHASIL;
}

/*
 * gicv3_set_priority
 * ------------------
 * Set interrupt priority.
 */
status_t gicv3_set_priority(tak_bertanda32_t irq, tak_bertanda8_t priority)
{
    if (irq >= g_gic_lines) {
        return STATUS_PARAM_INVALID;
    }

    if (irq < 32) {
        volatile tak_bertanda8_t *rbase;
        volatile tak_bertanda8_t *prio;
        rbase = _get_redistributor_base();
        prio = (volatile tak_bertanda8_t *)(rbase + GICR_IPRIORITYR0);
        prio[irq] = priority;
    } else {
        _gicd_write8(GICD_IPRIORITYR + irq, priority);
    }

    return STATUS_BERHASIL;
}

/*
 * gicv3_get_priority
 * ------------------
 * Get interrupt priority.
 */
tak_bertanda8_t gicv3_get_priority(tak_bertanda32_t irq)
{
    if (irq >= g_gic_lines) {
        return 0;
    }

    if (irq < 32) {
        volatile tak_bertanda8_t *rbase;
        volatile tak_bertanda8_t *prio;
        rbase = _get_redistributor_base();
        prio = (volatile tak_bertanda8_t *)(rbase + GICR_IPRIORITYR0);
        return prio[irq];
    }

    return _gicd_read8(GICD_IPRIORITYR + irq);
}

/*
 * gicv3_set_trigger
 * -----------------
 * Set interrupt trigger type.
 */
status_t gicv3_set_trigger(tak_bertanda32_t irq, bool_t edge_triggered)
{
    tak_bertanda32_t reg_idx;
    tak_bertanda32_t bit_idx;
    tak_bertanda32_t val;

    if (irq < 16 || irq >= g_gic_lines) {
        return STATUS_PARAM_INVALID;
    }

    if (irq < 32) {
        /* PPI configuration */
        volatile tak_bertanda8_t *rbase;
        rbase = _get_redistributor_base();

        bit_idx = (irq - 16) * 2;
        val = _gicr_read32((tak_bertanda32_t)(rbase - g_gicr_base) +
                           GICR_ICFGR0);

        if (edge_triggered) {
            val |= (2 << bit_idx);
        } else {
            val &= ~(3 << bit_idx);
        }

        _gicr_write32((tak_bertanda32_t)(rbase - g_gicr_base) +
                      GICR_ICFGR0, val);
    } else {
        /* SPI configuration */
        reg_idx = (irq - 32) / 16;
        bit_idx = ((irq - 32) % 16) * 2;
        val = _gicd_read32(GICD_ICFGR + reg_idx * 4);

        if (edge_triggered) {
            val |= (2 << bit_idx);
        } else {
            val &= ~(3 << bit_idx);
        }

        _gicd_write32(GICD_ICFGR + reg_idx * 4, val);
    }

    return STATUS_BERHASIL;
}

/*
 * gicv3_set_pending
 * -----------------
 * Set interrupt pending.
 */
status_t gicv3_set_pending(tak_bertanda32_t irq)
{
    if (irq >= g_gic_lines) {
        return STATUS_PARAM_INVALID;
    }

    if (irq < 32) {
        volatile tak_bertanda8_t *rbase;
        rbase = _get_redistributor_base();
        _gicr_write32((tak_bertanda32_t)(rbase - g_gicr_base) +
                      GICR_ISPENDR0, (1 << irq));
    } else {
        tak_bertanda32_t reg_idx = irq / 32;
        tak_bertanda32_t bit_idx = irq % 32;
        _gicd_write32(GICD_ISPENDR + reg_idx * 4, (1 << bit_idx));
    }

    return STATUS_BERHASIL;
}

/*
 * gicv3_clear_pending
 * -------------------
 * Clear interrupt pending.
 */
status_t gicv3_clear_pending(tak_bertanda32_t irq)
{
    if (irq >= g_gic_lines) {
        return STATUS_PARAM_INVALID;
    }

    if (irq < 32) {
        volatile tak_bertanda8_t *rbase;
        rbase = _get_redistributor_base();
        _gicr_write32((tak_bertanda32_t)(rbase - g_gicr_base) +
                      GICR_ICPENDR0, (1 << irq));
    } else {
        tak_bertanda32_t reg_idx = irq / 32;
        tak_bertanda32_t bit_idx = irq % 32;
        _gicd_write32(GICD_ICPENDR + reg_idx * 4, (1 << bit_idx));
    }

    return STATUS_BERHASIL;
}

/*
 * gicv3_get_irq
 * -------------
 * Get active interrupt ID.
 */
tak_bertanda32_t gicv3_get_irq(void)
{
    tak_bertanda64_t iar;

    __asm__ __volatile__(
        "mrs %0, ICC_IAR1_EL1"
        : "=r"(iar)
    );

    return (tak_bertanda32_t)(iar & 0x3FF);
}

/*
 * gicv3_eoi
 * ---------
 * End of interrupt.
 */
void gicv3_eoi(tak_bertanda32_t irq)
{
    __asm__ __volatile__(
        "msr ICC_EOIR1_EL1, %0\n\t"
        "isb"
        :
        : "r"((tak_bertanda64_t)irq)
        : "memory"
    );
}

/*
 * gicv3_deactivate
 * ----------------
 * Deactivate interrupt.
 */
void gicv3_deactivate(tak_bertanda32_t irq)
{
    __asm__ __volatile__(
        "msr ICC_DIR_EL1, %0\n\t"
        "isb"
        :
        : "r"((tak_bertanda64_t)irq)
        : "memory"
    );
}

/*
 * gicv3_send_sgi
 * --------------
 * Send software generated interrupt.
 */
void gicv3_send_sgi(tak_bertanda32_t sgi_id,
                    tak_bertanda64_t target_list,
                    tak_bertanda64_t affinity)
{
    tak_bertanda64_t sgi_val;

    if (sgi_id >= 16) {
        return;
    }

    sgi_val = (sgi_id & 0xF);
    sgi_val |= (target_list & 0xFFFF) << 16;
    sgi_val |= (affinity & 0xFFFFFFFFFF) << 32;

    __asm__ __volatile__(
        "msr ICC_SGI1R_EL1, %0\n\t"
        "isb"
        :
        : "r"(sgi_val)
        : "memory"
    );
}
