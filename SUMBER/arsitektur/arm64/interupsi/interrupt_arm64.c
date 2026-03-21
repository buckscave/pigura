/*
 * PIGURA OS - INTERRUPT_ARM64.C
 * -----------------------------
 * Implementasi interrupt handling untuk ARM64 (AArch64).
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola interrupt
 * pada prosesor ARM64 dengan GICv3.
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

/* IRQ numbers */
#define IRQ_TIMER_PHYS          30
#define IRQ_TIMER_VIRT          27
#define IRQ_TIMER_HYP           26
#define IRQ_TIMER_SEC_PHYS      29

/* Software generated interrupts */
#define SGI_BASE                0
#define SGI_COUNT               16

/* Private peripheral interrupts */
#define PPI_BASE                16
#define PPI_COUNT               16

/* Shared peripheral interrupts */
#define SPI_BASE                32
#define SPI_MAX                 987

/* GICv3 ICC register bits */
#define ICC_SRE_SRE             (1 << 0)
#define ICC_SRE_DFB             (1 << 1)
#define ICC_SRE_DIB             (1 << 2)
#define ICC_SRE_ENEL1           (1 << 3)

#define ICC_CTLR_CBPR_SHIFT     0
#define ICC_CTLR_EOIMODE_SHIFT  1
#define ICC_CTLR_PMHE_SHIFT     6

#define ICC_IGRPEN_EN           (1 << 0)
#define ICC_IGRPEN_ENGRP1       (1 << 1)

/*
 * ============================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ============================================================================
 */

/* IRQ handlers */
static handler_interupsi_t g_irq_handlers[1024];
static void *g_irq_data[1024];

/* Interrupt counters */
static tak_bertanda64_t g_irq_count[1024];

/* Spurious interrupt counter */
static tak_bertanda64_t g_spurious_count = 0;

/* Flag */
static bool_t g_interrupt_initialized = SALAH;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * _read_icc_iar
 * -------------
 * Read Interrupt Acknowledge Register.
 */
static inline tak_bertanda64_t _read_icc_iar(void)
{
    tak_bertanda64_t iar;

    __asm__ __volatile__(
        "mrs %0, ICC_IAR1_EL1"
        : "=r"(iar)
    );

    return iar;
}

/*
 * _write_icc_eoir
 * ---------------
 * Write End of Interrupt Register.
 */
static inline void _write_icc_eoir(tak_bertanda64_t irq)
{
    __asm__ __volatile__(
        "msr ICC_EOIR1_EL1, %0\n\t"
        "isb"
        :
        : "r"(irq)
        : "memory"
    );
}

/*
 * _write_icc_dir
 * --------------
 * Write Deactivate Interrupt Register.
 */
static inline void _write_icc_dir(tak_bertanda64_t irq)
{
    __asm__ __volatile__(
        "msr ICC_DIR_EL1, %0\n\t"
        "isb"
        :
        : "r"(irq)
        : "memory"
    );
}

/*
 * _get_running_irq
 * ----------------
 * Get currently running IRQ.
 */
static inline tak_bertanda32_t _get_running_irq(void)
{
    tak_bertanda64_t iar;

    iar = _read_icc_iar();

    /* 1023 = spurious interrupt */
    if ((iar & 0x3FF) == 1023) {
        return 1023;
    }

    return (tak_bertanda32_t)(iar & 0x3FF);
}

/*
 * ============================================================================
 * INTERRUPT DISPATCH
 * ============================================================================
 */

/*
 * interrupt_dispatch
 * ------------------
 * Dispatch interrupt to appropriate handler.
 */
void interrupt_dispatch(void)
{
    tak_bertanda32_t irq;
    handler_interupsi_t handler;

    /* Get interrupt ID */
    irq = _get_running_irq();

    /* Check for spurious */
    if (irq == 1023) {
        g_spurious_count++;
        return;
    }

    /* Update counter */
    if (irq < 1024) {
        g_irq_count[irq]++;
    }

    /* Get handler */
    if (irq < 1024) {
        handler = g_irq_handlers[irq];

        if (handler != NULL) {
            /* Call handler */
            handler();
        }
    }

    /* End of interrupt */
    _write_icc_eoir((tak_bertanda64_t)irq);
}

/*
 * ============================================================================
 * PUBLIC FUNCTIONS
 * ============================================================================
 */

/*
 * interrupt_init
 * --------------
 * Initialize interrupt controller.
 */
status_t interrupt_init(void)
{
    tak_bertanda32_t i;

    if (g_interrupt_initialized) {
        return STATUS_BERHASIL;
    }

    /* Clear handlers */
    for (i = 0; i < 1024; i++) {
        g_irq_handlers[i] = NULL;
        g_irq_data[i] = NULL;
        g_irq_count[i] = 0;
    }

    g_spurious_count = 0;

    /* Enable system register access to GIC */
    __asm__ __volatile__(
        "msr ICC_SRE_EL1, %0\n\t"
        "isb"
        :
        : "r"(ICC_SRE_SRE | ICC_SRE_DFB | ICC_SRE_DIB | ICC_SRE_ENEL1)
        : "memory"
    );

    /* Set priority mask (allow all priorities) */
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
        : "r"(0x0)
        : "memory"
    );

    /* Enable group 1 interrupts */
    __asm__ __volatile__(
        "msr ICC_IGRPEN1_EL1, %0\n\t"
        "isb"
        :
        : "r"(ICC_IGRPEN_EN)
        : "memory"
    );

    g_interrupt_initialized = BENAR;

    kernel_printf("[INT-ARM64] Interrupt controller initialized\n");

    return STATUS_BERHASIL;
}

/*
 * interrupt_register
 * ------------------
 * Register interrupt handler.
 */
status_t interrupt_register(tak_bertanda32_t irq,
                             handler_interupsi_t handler,
                             void *data)
{
    if (irq >= 1024) {
        return STATUS_PARAM_INVALID;
    }

    g_irq_handlers[irq] = handler;
    g_irq_data[irq] = data;

    return STATUS_BERHASIL;
}

/*
 * interrupt_unregister
 * --------------------
 * Unregister interrupt handler.
 */
status_t interrupt_unregister(tak_bertanda32_t irq)
{
    if (irq >= 1024) {
        return STATUS_PARAM_INVALID;
    }

    g_irq_handlers[irq] = NULL;
    g_irq_data[irq] = NULL;

    return STATUS_BERHASIL;
}

/*
 * interrupt_enable
 * ----------------
 * Enable specific IRQ.
 */
status_t interrupt_enable(tak_bertanda32_t irq)
{
    /* GIC distributor enable is handled by GICv3 driver */
    return STATUS_BERHASIL;
}

/*
 * interrupt_disable
 * -----------------
 * Disable specific IRQ.
 */
status_t interrupt_disable(tak_bertanda32_t irq)
{
    /* GIC distributor disable is handled by GICv3 driver */
    return STATUS_BERHASIL;
}

/*
 * interrupt_get_count
 * -------------------
 * Get interrupt count for IRQ.
 */
tak_bertanda64_t interrupt_get_count(tak_bertanda32_t irq)
{
    if (irq >= 1024) {
        return 0;
    }

    return g_irq_count[irq];
}

/*
 * interrupt_get_spurious_count
 * ----------------------------
 * Get spurious interrupt count.
 */
tak_bertanda64_t interrupt_get_spurious_count(void)
{
    return g_spurious_count;
}

/*
 * interrupt_send_sgi
 * ------------------
 * Send software generated interrupt.
 */
void interrupt_send_sgi(tak_bertanda32_t sgi_id,
                        tak_bertanda64_t target_list,
                        tak_bertanda32_t aff3,
                        tak_bertanda32_t aff2,
                        tak_bertanda32_t aff1,
                        tak_bertanda32_t aff0)
{
    tak_bertanda64_t sgi_val;

    if (sgi_id >= 16) {
        return;
    }

    /* Build ICC_SGI1R_EL1 value */
    sgi_val = (sgi_id & 0xF);
    sgi_val |= (target_list & 0xFFFF) << 16;
    sgi_val |= ((tak_bertanda64_t)aff3 & 0xFF) << 48;
    sgi_val |= ((tak_bertanda64_t)aff2 & 0xFF) << 32;
    sgi_val |= ((tak_bertanda64_t)aff1 & 0xFF) << 16;
    sgi_val |= ((tak_bertanda64_t)aff0 & 0xFF) << 0;

    __asm__ __volatile__(
        "msr ICC_SGI1R_EL1, %0\n\t"
        "isb"
        :
        : "r"(sgi_val)
        : "memory"
    );
}

/*
 * interrupt_send_sgi_self
 * -----------------------
 * Send SGI to current CPU.
 */
void interrupt_send_sgi_self(tak_bertanda32_t sgi_id)
{
    tak_bertanda64_t mpidr;

    /* Get current MPIDR */
    __asm__ __volatile__(
        "mrs %0, MPIDR_EL1"
        : "=r"(mpidr)
    );

    /* Send to self */
    interrupt_send_sgi(sgi_id, 1,
        (tak_bertanda32_t)((mpidr >> 32) & 0xFF),
        (tak_bertanda32_t)((mpidr >> 16) & 0xFF),
        (tak_bertanda32_t)((mpidr >> 8) & 0xFF),
        (tak_bertanda32_t)(mpidr & 0xFF));
}

/*
 * interrupt_send_sgi_all
 * ----------------------
 * Send SGI to all CPUs.
 */
void interrupt_send_sgi_all(tak_bertanda32_t sgi_id)
{
    tak_bertanda64_t sgi_val;

    if (sgi_id >= 16) {
        return;
    }

    /* Target all - using IRM bit (bit 40) */
    sgi_val = (sgi_id & 0xF);
    sgi_val |= (1ULL << 40);  /* IRM = 1, target all except self */

    __asm__ __volatile__(
        "msr ICC_SGI1R_EL1, %0\n\t"
        "isb"
        :
        : "r"(sgi_val)
        : "memory"
    );
}

/*
 * interrupt_set_priority_mask
 * ---------------------------
 * Set priority mask.
 */
void interrupt_set_priority_mask(tak_bertanda8_t mask)
{
    __asm__ __volatile__(
        "msr ICC_PMR_EL1, %0\n\t"
        "isb"
        :
        : "r"((tak_bertanda64_t)mask)
        : "memory"
    );
}

/*
 * interrupt_get_priority_mask
 * ---------------------------
 * Get priority mask.
 */
tak_bertanda8_t interrupt_get_priority_mask(void)
{
    tak_bertanda64_t pmr;

    __asm__ __volatile__(
        "mrs %0, ICC_PMR_EL1"
        : "=r"(pmr)
    );

    return (tak_bertanda8_t)(pmr & 0xFF);
}
