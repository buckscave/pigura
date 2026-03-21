/*
 * PIGURA OS - INTERUPSI_ARMV7.C
 * ------------------------------
 * Implementasi interface interupsi untuk arsitektur ARMv7.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola interupsi pada
 * prosesor ARMv7, termasuk setup vector table dan dispatch handler.
 *
 * Arsitektur: ARMv7 (Cortex-A series)
 * Versi: 1.0
 */

#include "../../inti/types.h"
#include "../../inti/konstanta.h"
#include "../../inti/hal/hal.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* Jumlah exception vectors pada ARM */
#define JUMLAH_VEKTOR           8

/* Exception types */
#define EXC_RESET               0
#define EXC_UNDEFINED           1
#define EXC_SVC                 2
#define EXC_PREFETCH_ABORT      3
#define EXC_DATA_ABORT          4
#define EXC_UNUSED              5
#define EXC_IRQ                 6
#define EXC_FIQ                 7

/* CPSR mode bits */
#define CPSR_MODE_USR           0x10
#define CPSR_MODE_FIQ           0x11
#define CPSR_MODE_IRQ           0x12
#define CPSR_MODE_SVC           0x13
#define CPSR_MODE_ABT           0x17
#define CPSR_MODE_UND           0x1B
#define CPSR_MODE_SYS           0x1F

/* CPSR interrupt bits */
#define CPSR_IRQ_DISABLE        (1 << 7)
#define CPSR_FIQ_DISABLE        (1 << 6)

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Tabel handler exception */
static void (*g_exception_handlers[JUMLAH_VEKTOR])(void) = { NULL };

/* Tabel handler IRQ */
#define MAX_IRQ_ENTRIES         128
static void (*g_irq_handlers[MAX_IRQ_ENTRIES])(void) = { NULL };

/* Flag inisialisasi */
static bool_t g_interrupt_initialized = SALAH;

/*
 * ============================================================================
 * FUNGSI BANTUAN (HELPER FUNCTIONS)
 * ============================================================================
 */

/*
 * interrupt_get_cpsr
 * ------------------
 * Baca nilai CPSR register.
 *
 * Return: Nilai CPSR
 */
static inline tak_bertanda32_t interrupt_get_cpsr(void)
{
    tak_bertanda32_t cpsr;
    __asm__ __volatile__(
        "mrs %0, cpsr"
        : "=r"(cpsr)
    );
    return cpsr;
}

/*
 * interrupt_set_cpsr
 * ------------------
 * Set nilai CPSR register.
 *
 * Parameter:
 *   cpsr - Nilai CPSR baru
 */
static inline void interrupt_set_cpsr(tak_bertanda32_t cpsr)
{
    __asm__ __volatile__(
        "msr cpsr_c, %0"
        :
        : "r"(cpsr)
    );
}

/*
 * ============================================================================
 * FUNGSI VECTOR TABLE (VECTOR TABLE FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_interrupt_set_vector
 * ------------------------
 * Set alamat vector table.
 *
 * Parameter:
 *   base - Alamat base vector table
 *
 * Return: Status operasi
 */
status_t hal_interrupt_set_vector(void *base)
{
    tak_bertanda32_t addr;

    if (base == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Vector table harus 32-byte aligned */
    addr = (tak_bertanda32_t)(tak_bertanda32_t)base;
    if (addr & 0x1F) {
        return STATUS_PARAM_INVALID;
    }

    /* Set VBAR (Vector Base Address Register) */
    __asm__ __volatile__(
        "mcr p15, 0, %0, c12, c0, 0"
        :
        : "r"(addr)
    );

    /* Instruction synchronization barrier */
    __asm__ __volatile__("isb");

    return STATUS_BERHASIL;
}

/*
 * hal_interrupt_get_vector
 * ------------------------
 * Dapatkan alamat vector table.
 *
 * Return: Alamat base vector table
 */
void *hal_interrupt_get_vector(void)
{
    tak_bertanda32_t addr;

    __asm__ __volatile__(
        "mrc p15, 0, %0, c12, c0, 0"
        : "=r"(addr)
    );

    return (void *)addr;
}

/*
 * ============================================================================
 * FUNGSI EXCEPTION HANDLER (EXCEPTION HANDLER FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_exception_set_handler
 * -------------------------
 * Set handler untuk exception tertentu.
 *
 * Parameter:
 *   exception - Nomor exception (0-7)
 *   handler   - Fungsi handler
 *
 * Return: Status operasi
 */
status_t hal_exception_set_handler(tak_bertanda32_t exception,
    void (*handler)(void))
{
    if (exception >= JUMLAH_VEKTOR) {
        return STATUS_PARAM_INVALID;
    }

    g_exception_handlers[exception] = handler;
    return STATUS_BERHASIL;
}

/*
 * hal_exception_get_handler
 * -------------------------
 * Dapatkan handler untuk exception tertentu.
 *
 * Parameter:
 *   exception - Nomor exception
 *
 * Return: Pointer ke handler, atau NULL jika tidak ada
 */
void (*hal_exception_get_handler(tak_bertanda32_t exception))(void)
{
    if (exception >= JUMLAH_VEKTOR) {
        return NULL;
    }

    return g_exception_handlers[exception];
}

/*
 * ============================================================================
 * FUNGSI IRQ HANDLER (IRQ HANDLER FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_irq_set_handler
 * -------------------
 * Set handler untuk IRQ tertentu.
 *
 * Parameter:
 *   irq     - Nomor IRQ
 *   handler - Fungsi handler
 *
 * Return: Status operasi
 */
status_t hal_irq_set_handler(tak_bertanda32_t irq, void (*handler)(void))
{
    if (irq >= MAX_IRQ_ENTRIES) {
        return STATUS_PARAM_INVALID;
    }

    g_irq_handlers[irq] = handler;
    return STATUS_BERHASIL;
}

/*
 * hal_irq_get_handler
 * -------------------
 * Dapatkan handler untuk IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: Pointer ke handler, atau NULL jika tidak ada
 */
void (*hal_irq_get_handler(tak_bertanda32_t irq))(void)
{
    if (irq >= MAX_IRQ_ENTRIES) {
        return NULL;
    }

    return g_irq_handlers[irq];
}

/*
 * ============================================================================
 * FUNGSI DISPATCH (DISPATCH FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_dispatch_exception
 * ----------------------
 * Dispatch exception ke handler yang sesuai.
 * Dipanggil dari assembly entry point.
 *
 * Parameter:
 *   exception - Nomor exception
 *   spsr      - Saved CPSR
 *   sp        - Stack pointer saat exception
 *   lr        - Link register saat exception
 */
void hal_dispatch_exception(tak_bertanda32_t exception,
    tak_bertanda32_t spsr, tak_bertanda32_t sp, tak_bertanda32_t lr)
{
    /* Avoid unused parameter warnings */
    (void)spsr;
    (void)sp;
    (void)lr;

    /* Panggil handler jika ada */
    if (exception < JUMLAH_VEKTOR && g_exception_handlers[exception] != NULL) {
        g_exception_handlers[exception]();
    } else {
        /* Default: hang jika tidak ada handler */
        hal_cpu_halt();
    }
}

/*
 * hal_dispatch_irq
 * ----------------
 * Dispatch IRQ ke handler yang sesuai.
 * Dipanggil dari assembly entry point.
 */
void hal_dispatch_irq(void)
{
    tak_bertanda32_t irq;

    /* Baca Interrupt Acknowledge Register dari GIC */
    /* IRQ number didapat dari GIC */
    irq = hal_gic_acknowledge();

    if (irq < MAX_IRQ_ENTRIES && g_irq_handlers[irq] != NULL) {
        g_irq_handlers[irq]();
    }

    /* Acknowledge IRQ di GIC */
    hal_gic_end_interrupt(irq);
}

/*
 * ============================================================================
 * FUNGSI GIC INTERFACE (GIC INTERFACE FUNCTIONS)
 * ============================================================================
 */

/* GIC base addresses (QEMU virt machine) */
#define GIC_DISTRIBUTOR_BASE    0x08000000
#define GIC_CPU_INTERFACE_BASE  0x08010000

/* GIC Distributor registers */
#define GICD_CTLR               0x000
#define GICD_TYPER              0x004
#define GICD_ISENABLER          0x100
#define GICD_ICENABLER          0x180
#define GICD_ISPENDR            0x200
#define GICD_ICPENDR            0x280
#define GICD_IPRIORITYR         0x400
#define GICD_ITARGETSR          0x800
#define GICD_ICFGR              0xC00

/* GIC CPU Interface registers */
#define GICC_CTLR               0x000
#define GICC_PMR                0x004
#define GICC_IAR                0x00C
#define GICC_EOIR               0x010

/* GIC register access macros */
#define GICD_READ(reg) \
    hal_mmio_read_32((void *)(GIC_DISTRIBUTOR_BASE + (reg)))
#define GICD_WRITE(reg, val) \
    hal_mmio_write_32((void *)(GIC_DISTRIBUTOR_BASE + (reg)), (val))
#define GICC_READ(reg) \
    hal_mmio_read_32((void *)(GIC_CPU_INTERFACE_BASE + (reg)))
#define GICC_WRITE(reg, val) \
    hal_mmio_write_32((void *)(GIC_CPU_INTERFACE_BASE + (reg)), (val))

/*
 * hal_gic_init
 * ------------
 * Inisialisasi Generic Interrupt Controller.
 *
 * Return: Status operasi
 */
status_t hal_gic_init(void)
{
    tak_bertanda32_t typer;
    tak_bertanda32_t num_irq;
    tak_bertanda32_t i;

    /* Disable distributor */
    GICD_WRITE(GICD_CTLR, 0);

    /* Baca jumlah IRQ yang didukung */
    typer = GICD_READ(GICD_TYPER);
    num_irq = ((typer & 0x1F) + 1) * 32;
    if (num_irq > MAX_IRQ_ENTRIES) {
        num_irq = MAX_IRQ_ENTRIES;
    }

    /* Disable semua IRQ */
    for (i = 0; i < num_irq; i += 32) {
        GICD_WRITE(GICD_ICENABLER + (i / 32) * 4, 0xFFFFFFFF);
    }

    /* Clear semua pending IRQ */
    for (i = 0; i < num_irq; i += 32) {
        GICD_WRITE(GICD_ICPENDR + (i / 32) * 4, 0xFFFFFFFF);
    }

    /* Set priority semua IRQ ke highest */
    for (i = 0; i < num_irq; i++) {
        GICD_WRITE(GICD_IPRIORITYR + i, 0);
    }

    /* Set target semua IRQ ke CPU 0 */
    for (i = 32; i < num_irq; i++) {
        GICD_WRITE(GICD_ITARGETSR + i, 0x01);
    }

    /* Enable distributor */
    GICD_WRITE(GICD_CTLR, 1);

    /* Enable CPU interface */
    GICC_WRITE(GICC_PMR, 0xFF);     /* Set priority mask */
    GICC_WRITE(GICC_CTLR, 1);       /* Enable CPU interface */

    return STATUS_BERHASIL;
}

/*
 * hal_gic_enable_irq
 * ------------------
 * Aktifkan IRQ di GIC.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: Status operasi
 */
status_t hal_gic_enable_irq(tak_bertanda32_t irq)
{
    if (irq >= MAX_IRQ_ENTRIES) {
        return STATUS_PARAM_INVALID;
    }

    GICD_WRITE(GICD_ISENABLER + (irq / 32) * 4, (1 << (irq % 32)));
    return STATUS_BERHASIL;
}

/*
 * hal_gic_disable_irq
 * -------------------
 * Nonaktifkan IRQ di GIC.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: Status operasi
 */
status_t hal_gic_disable_irq(tak_bertanda32_t irq)
{
    if (irq >= MAX_IRQ_ENTRIES) {
        return STATUS_PARAM_INVALID;
    }

    GICD_WRITE(GICD_ICENABLER + (irq / 32) * 4, (1 << (irq % 32)));
    return STATUS_BERHASIL;
}

/*
 * hal_gic_acknowledge
 * -------------------
 * Acknowledge IRQ dan dapatkan nomor IRQ.
 *
 * Return: Nomor IRQ yang diacknowledge
 */
tak_bertanda32_t hal_gic_acknowledge(void)
{
    return GICC_READ(GICC_IAR) & 0x3FF;
}

/*
 * hal_gic_end_interrupt
 * ---------------------
 * Signal end of interrupt.
 *
 * Parameter:
 *   irq - Nomor IRQ
 */
void hal_gic_end_interrupt(tak_bertanda32_t irq)
{
    GICC_WRITE(GICC_EOIR, irq);
}

/*
 * hal_gic_set_priority
 * --------------------
 * Set priority IRQ.
 *
 * Parameter:
 *   irq      - Nomor IRQ
 *   priority - Priority (0 = highest, 255 = lowest)
 *
 * Return: Status operasi
 */
status_t hal_gic_set_priority(tak_bertanda32_t irq, tak_bertanda8_t priority)
{
    tak_bertanda32_t reg;
    tak_bertanda32_t shift;

    if (irq >= MAX_IRQ_ENTRIES) {
        return STATUS_PARAM_INVALID;
    }

    /* Priority register adalah byte-addressed */
    reg = GICD_READ(GICD_IPRIORITYR + (irq & ~3));
    shift = (irq & 3) * 8;
    reg = (reg & ~(0xFF << shift)) | ((tak_bertanda32_t)priority << shift);
    GICD_WRITE(GICD_IPRIORITYR + (irq & ~3), reg);

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI INISIALISASI (INITIALIZATION FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_interrupt_arch_init
 * -----------------------
 * Inisialisasi subsistem interupsi arsitektur ARMv7.
 *
 * Return: Status operasi
 */
status_t hal_interrupt_arch_init(void)
{
    tak_bertanda32_t i;

    /* Clear semua handler */
    for (i = 0; i < JUMLAH_VEKTOR; i++) {
        g_exception_handlers[i] = NULL;
    }

    for (i = 0; i < MAX_IRQ_ENTRIES; i++) {
        g_irq_handlers[i] = NULL;
    }

    /* Inisialisasi GIC */
    hal_gic_init();

    g_interrupt_initialized = BENAR;

    return STATUS_BERHASIL;
}

/*
 * hal_interrupt_arch_shutdown
 * ---------------------------
 * Shutdown subsistem interupsi.
 *
 * Return: Status operasi
 */
status_t hal_interrupt_arch_shutdown(void)
{
    /* Disable GIC distributor */
    GICD_WRITE(GICD_CTLR, 0);

    /* Disable CPU interface */
    GICC_WRITE(GICC_CTLR, 0);

    g_interrupt_initialized = SALAH;

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI UTILITAS (UTILITY FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_interrupt_is_irq_active
 * ---------------------------
 * Cek apakah IRQ tertentu sedang aktif.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: BENAR jika aktif, SALAH jika tidak
 */
bool_t hal_interrupt_is_irq_active(tak_bertanda32_t irq)
{
    tak_bertanda32_t reg;

    if (irq >= MAX_IRQ_ENTRIES) {
        return SALAH;
    }

    /* Baca Interrupt Set-Pending Register */
    reg = GICD_READ(GICD_ISPENDR + (irq / 32) * 4);

    return (reg & (1 << (irq % 32))) ? BENAR : SALAH;
}

/*
 * hal_interrupt_get_active_irq
 * ----------------------------
 * Dapatkan IRQ yang sedang aktif.
 *
 * Return: Nomor IRQ aktif, atau -1 jika tidak ada
 */
tanda32_t hal_interrupt_get_active_irq(void)
{
    tak_bertanda32_t iar;

    iar = GICC_READ(GICC_IAR);

    /* IRQ ID 1023 = spurious interrupt */
    if ((iar & 0x3FF) == 0x3FF) {
        return -1;
    }

    return (tanda32_t)(iar & 0x3FF);
}
