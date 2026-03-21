/*
 * PIGURA OS - GIC_ARM.C
 * ----------------------
 * Implementasi Generic Interrupt Controller untuk ARM (32-bit).
 *
 * Arsitektur: ARM (ARMv4/v5/v6 - 32-bit)
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

/* GIC Distributor base address (QEMU virt machine) */
#define GICD_BASE               0x08000000

/* GIC CPU Interface base address */
#define GICC_BASE               0x08010000

/* GIC Distributor register offsets */
#define GICD_CTLR               0x000       /* Control Register */
#define GICD_TYPER              0x004       /* Type Register */
#define GICD_IIDR               0x008       /* Implementer ID */
#define GICD_IGROUPR            0x080       /* Interrupt Group */
#define GICD_ISENABLER          0x100       /* Interrupt Set-Enable */
#define GICD_ICENABLER          0x180       /* Interrupt Clear-Enable */
#define GICD_ISPENDR            0x200       /* Interrupt Set-Pending */
#define GICD_ICPENDR            0x280       /* Interrupt Clear-Pending */
#define GICD_ISACTIVER          0x300       /* Interrupt Set-Active */
#define GICD_ICACTIVER          0x380       /* Interrupt Clear-Active */
#define GICD_IPRIORITYR         0x400       /* Interrupt Priority */
#define GICD_ITARGETSR          0x800       /* Interrupt Processor Target */
#define GICD_ICFGR              0xC00       /* Interrupt Configuration */
#define GICD_SGIR               0xF00       /* Software Generated Interrupt */

/* GIC CPU Interface register offsets */
#define GICC_CTLR               0x000       /* Control Register */
#define GICC_PMR                0x004       /* Priority Mask Register */
#define GICC_BPR                0x008       /* Binary Point Register */
#define GICC_IAR                0x00C       /* Interrupt Acknowledge */
#define GICC_EOIR               0x010       /* End of Interrupt */
#define GICC_RPR                0x014       /* Running Priority */
#define GICC_HPPIR              0x018       /* Highest Priority Pending */

/* GICD_CTLR bits */
#define GICD_CTLR_ENABLEGRP0    (1 << 0)    /* Enable Group 0 */
#define GICD_CTLR_ENABLEGRP1    (1 << 1)    /* Enable Group 1 */

/* GICC_CTLR bits */
#define GICC_CTLR_ENABLE        (1 << 0)    /* Enable CPU interface */

/* Maximum interrupts */
#define GIC_MAX_INTERRUPTS      1024

/* Special IRQ numbers */
#define GIC_SGI_START           0           /* Software Generated */
#define GIC_PPI_START           16          /* Private Peripheral */
#define GIC_SPI_START           32          /* Shared Peripheral */
#define GIC_SPURIOUS            1023

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* GIC base addresses */
static volatile tak_bertanda32_t *g_gicd = NULL;
static volatile tak_bertanda32_t *g_gicc = NULL;

/* Number of IRQ lines */
static tak_bertanda32_t g_gic_irq_count = 0;

/* Flag inisialisasi */
static bool_t g_gic_initialized = SALAH;

/*
 * ============================================================================
 * FUNGSI ACCESS REGISTER (REGISTER ACCESS FUNCTIONS)
 * ============================================================================
 */

/*
 * gicd_read
 * ---------
 * Baca register Distributor.
 *
 * Parameter:
 *   offset - Offset register
 *
 * Return: Nilai register
 */
static inline tak_bertanda32_t gicd_read(tak_bertanda32_t offset)
{
    return hal_mmio_read_32((void *)((tak_bertanda32_t)g_gicd + offset));
}

/*
 * gicd_write
 * ----------
 * Tulis register Distributor.
 *
 * Parameter:
 *   offset - Offset register
 *   value  - Nilai yang ditulis
 */
static inline void gicd_write(tak_bertanda32_t offset,
    tak_bertanda32_t value)
{
    hal_mmio_write_32((void *)((tak_bertanda32_t)g_gicd + offset), value);
}

/*
 * gicc_read
 * ---------
 * Baca register CPU Interface.
 *
 * Parameter:
 *   offset - Offset register
 *
 * Return: Nilai register
 */
static inline tak_bertanda32_t gicc_read(tak_bertanda32_t offset)
{
    return hal_mmio_read_32((void *)((tak_bertanda32_t)g_gicc + offset));
}

/*
 * gicc_write
 * ----------
 * Tulis register CPU Interface.
 *
 * Parameter:
 *   offset - Offset register
 *   value  - Nilai yang ditulis
 */
static inline void gicc_write(tak_bertanda32_t offset,
    tak_bertanda32_t value)
{
    hal_mmio_write_32((void *)((tak_bertanda32_t)g_gicc + offset), value);
}

/*
 * ============================================================================
 * FUNGSI DISTRIBUTOR (DISTRIBUTOR FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_gic_enable
 * --------------
 * Aktifkan GIC Distributor.
 *
 * Return: Status operasi
 */
status_t hal_gic_enable(void)
{
    tak_bertanda32_t ctlr;

    ctlr = gicd_read(GICD_CTLR);
    ctlr |= GICD_CTLR_ENABLEGRP0 | GICD_CTLR_ENABLEGRP1;
    gicd_write(GICD_CTLR, ctlr);

    return STATUS_BERHASIL;
}

/*
 * hal_gic_disable
 * ---------------
 * Nonaktifkan GIC Distributor.
 *
 * Return: Status operasi
 */
status_t hal_gic_disable(void)
{
    gicd_write(GICD_CTLR, 0);
    return STATUS_BERHASIL;
}

/*
 * hal_gic_enable_irq
 * ------------------
 * Aktifkan IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: Status operasi
 */
status_t hal_gic_enable_irq(tak_bertanda32_t irq)
{
    tak_bertanda32_t reg;
    tak_bertanda32_t bit;

    if (irq >= g_gic_irq_count) {
        return STATUS_PARAM_INVALID;
    }

    reg = irq / 32;
    bit = 1 << (irq % 32);

    gicd_write(GICD_ISENABLER + reg * 4, bit);

    return STATUS_BERHASIL;
}

/*
 * hal_gic_disable_irq
 * -------------------
 * Nonaktifkan IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: Status operasi
 */
status_t hal_gic_disable_irq(tak_bertanda32_t irq)
{
    tak_bertanda32_t reg;
    tak_bertanda32_t bit;

    if (irq >= g_gic_irq_count) {
        return STATUS_PARAM_INVALID;
    }

    reg = irq / 32;
    bit = 1 << (irq % 32);

    gicd_write(GICD_ICENABLER + reg * 4, bit);

    return STATUS_BERHASIL;
}

/*
 * hal_gic_set_pending
 * -------------------
 * Set pending bit untuk IRQ.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: Status operasi
 */
status_t hal_gic_set_pending(tak_bertanda32_t irq)
{
    tak_bertanda32_t reg;
    tak_bertanda32_t bit;

    if (irq >= g_gic_irq_count) {
        return STATUS_PARAM_INVALID;
    }

    reg = irq / 32;
    bit = 1 << (irq % 32);

    gicd_write(GICD_ISPENDR + reg * 4, bit);

    return STATUS_BERHASIL;
}

/*
 * hal_gic_clear_pending
 * ---------------------
 * Clear pending bit untuk IRQ.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: Status operasi
 */
status_t hal_gic_clear_pending(tak_bertanda32_t irq)
{
    tak_bertanda32_t reg;
    tak_bertanda32_t bit;

    if (irq >= g_gic_irq_count) {
        return STATUS_PARAM_INVALID;
    }

    reg = irq / 32;
    bit = 1 << (irq % 32);

    gicd_write(GICD_ICPENDR + reg * 4, bit);

    return STATUS_BERHASIL;
}

/*
 * hal_gic_set_priority
 * --------------------
 * Set priority untuk IRQ.
 *
 * Parameter:
 *   irq      - Nomor IRQ
 *   priority - Priority (0 = highest)
 *
 * Return: Status operasi
 */
status_t hal_gic_set_priority(tak_bertanda32_t irq, tak_bertanda8_t priority)
{
    tak_bertanda32_t offset;
    tak_bertanda32_t shift;
    tak_bertanda32_t reg;

    if (irq >= g_gic_irq_count) {
        return STATUS_PARAM_INVALID;
    }

    /* Priority register adalah byte-addressed */
    offset = GICD_IPRIORITYR + (irq & ~3);
    shift = (irq & 3) * 8;

    reg = gicd_read(offset);
    reg &= ~(0xFF << shift);
    reg |= (tak_bertanda32_t)priority << shift;
    gicd_write(offset, reg);

    return STATUS_BERHASIL;
}

/*
 * hal_gic_get_priority
 * --------------------
 * Dapatkan priority untuk IRQ.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: Priority IRQ
 */
tak_bertanda8_t hal_gic_get_priority(tak_bertanda32_t irq)
{
    tak_bertanda32_t offset;
    tak_bertanda32_t shift;
    tak_bertanda32_t reg;

    if (irq >= g_gic_irq_count) {
        return 0xFF;
    }

    offset = GICD_IPRIORITYR + (irq & ~3);
    shift = (irq & 3) * 8;

    reg = gicd_read(offset);

    return (tak_bertanda8_t)((reg >> shift) & 0xFF);
}

/*
 * hal_gic_set_target
 * ------------------
 * Set target CPU untuk IRQ.
 *
 * Parameter:
 *   irq    - Nomor IRQ
 *   target - Bitmask target CPU
 *
 * Return: Status operasi
 */
status_t hal_gic_set_target(tak_bertanda32_t irq, tak_bertanda8_t target)
{
    tak_bertanda32_t offset;
    tak_bertanda32_t shift;
    tak_bertanda32_t reg;

    /* Hanya untuk SPI */
    if (irq < GIC_SPI_START || irq >= g_gic_irq_count) {
        return STATUS_PARAM_INVALID;
    }

    offset = GICD_ITARGETSR + (irq & ~3);
    shift = (irq & 3) * 8;

    reg = gicd_read(offset);
    reg &= ~(0xFF << shift);
    reg |= (tak_bertanda32_t)target << shift;
    gicd_write(offset, reg);

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI CPU INTERFACE (CPU INTERFACE FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_gic_enable_cpu_interface
 * ----------------------------
 * Aktifkan CPU interface.
 *
 * Return: Status operasi
 */
status_t hal_gic_enable_cpu_interface(void)
{
    /* Set priority mask ke lowest priority */
    gicc_write(GICC_PMR, 0xFF);

    /* Set binary point */
    gicc_write(GICC_BPR, 0);

    /* Enable CPU interface */
    gicc_write(GICC_CTLR, GICC_CTLR_ENABLE);

    return STATUS_BERHASIL;
}

/*
 * hal_gic_disable_cpu_interface
 * -----------------------------
 * Nonaktifkan CPU interface.
 *
 * Return: Status operasi
 */
status_t hal_gic_disable_cpu_interface(void)
{
    gicc_write(GICC_CTLR, 0);
    return STATUS_BERHASIL;
}

/*
 * hal_gic_acknowledge
 * -------------------
 * Acknowledge interrupt dan dapatkan IRQ number.
 *
 * Return: IRQ number
 */
tak_bertanda32_t hal_gic_acknowledge(void)
{
    tak_bertanda32_t iar;

    iar = gicc_read(GICC_IAR);

    return iar & 0x3FF;
}

/*
 * hal_gic_end_interrupt
 * ---------------------
 * Signal end of interrupt.
 *
 * Parameter:
 *   irq - IRQ number
 */
void hal_gic_end_interrupt(tak_bertanda32_t irq)
{
    gicc_write(GICC_EOIR, irq);
}

/*
 * hal_gic_get_running_priority
 * ----------------------------
 * Dapatkan running priority.
 *
 * Return: Running priority
 */
tak_bertanda32_t hal_gic_get_running_priority(void)
{
    return gicc_read(GICC_RPR);
}

/*
 * hal_gic_get_highest_pending
 * ---------------------------
 * Dapatkan highest priority pending IRQ.
 *
 * Return: IRQ number
 */
tak_bertanda32_t hal_gic_get_highest_pending(void)
{
    tak_bertanda32_t hppir;

    hppir = gicc_read(GICC_HPPIR);

    return hppir & 0x3FF;
}

/*
 * ============================================================================
 * FUNGSI SOFTWARE INTERRUPT (SOFTWARE INTERRUPT FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_gic_send_sgi
 * ----------------
 * Kirim Software Generated Interrupt.
 *
 * Parameter:
 *   sgi_id  - SGI ID (0-15)
 *   target  - Target CPU bitmask
 *   filter  - Target filter
 *
 * Return: Status operasi
 */
status_t hal_gic_send_sgi(tak_bertanda32_t sgi_id, tak_bertanda32_t target,
    tak_bertanda32_t filter)
{
    tak_bertanda32_t sgir;

    if (sgi_id > 15) {
        return STATUS_PARAM_INVALID;
    }

    /* Build SGIR value */
    sgir = ((target & 0xFF) << 16) | ((filter & 0x3) << 24) | (sgi_id & 0xF);

    gicd_write(GICD_SGIR, sgir);

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI INISIALISASI (INITIALIZATION FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_gic_init
 * ------------
 * Inisialisasi GIC.
 *
 * Return: Status operasi
 */
status_t hal_gic_init(void)
{
    tak_bertanda32_t typer;
    tak_bertanda32_t i;

    /* Set base addresses */
    g_gicd = (volatile tak_bertanda32_t *)GICD_BASE;
    g_gicc = (volatile tak_bertanda32_t *)GICC_BASE;

    /* Disable distributor */
    gicd_write(GICD_CTLR, 0);

    /* Baca jumlah IRQ lines */
    typer = gicd_read(GICD_TYPER);
    g_gic_irq_count = ((typer & 0x1F) + 1) * 32;
    if (g_gic_irq_count > GIC_MAX_INTERRUPTS) {
        g_gic_irq_count = GIC_MAX_INTERRUPTS;
    }

    /* Disable semua interrupts */
    for (i = 0; i < g_gic_irq_count; i += 32) {
        gicd_write(GICD_ICENABLER + (i / 32) * 4, 0xFFFFFFFF);
    }

    /* Clear semua pending */
    for (i = 0; i < g_gic_irq_count; i += 32) {
        gicd_write(GICD_ICPENDR + (i / 32) * 4, 0xFFFFFFFF);
    }

    /* Set semua interrupt ke Group 0 */
    for (i = 0; i < g_gic_irq_count; i += 32) {
        gicd_write(GICD_IGROUPR + (i / 32) * 4, 0);
    }

    /* Set priority semua interrupt ke highest (0) */
    for (i = 0; i < g_gic_irq_count; i += 4) {
        gicd_write(GICD_IPRIORITYR + i, 0);
    }

    /* Set target semua SPI ke CPU 0 */
    for (i = GIC_SPI_START; i < g_gic_irq_count; i += 4) {
        gicd_write(GICD_ITARGETSR + i, 0x01010101);
    }

    /* Enable distributor */
    hal_gic_enable();

    /* Enable CPU interface */
    hal_gic_enable_cpu_interface();

    g_gic_initialized = BENAR;

    return STATUS_BERHASIL;
}

/*
 * hal_gic_shutdown
 * ----------------
 * Shutdown GIC.
 *
 * Return: Status operasi
 */
status_t hal_gic_shutdown(void)
{
    /* Disable CPU interface */
    hal_gic_disable_cpu_interface();

    /* Disable distributor */
    hal_gic_disable();

    g_gic_initialized = SALAH;

    return STATUS_BERHASIL;
}

/*
 * hal_gic_get_irq_count
 * ---------------------
 * Dapatkan jumlah IRQ yang didukung.
 *
 * Return: Jumlah IRQ
 */
tak_bertanda32_t hal_gic_get_irq_count(void)
{
    return g_gic_irq_count;
}

/*
 * hal_gic_is_initialized
 * ----------------------
 * Cek apakah GIC sudah diinisialisasi.
 *
 * Return: BENAR jika sudah
 */
bool_t hal_gic_is_initialized(void)
{
    return g_gic_initialized;
}
