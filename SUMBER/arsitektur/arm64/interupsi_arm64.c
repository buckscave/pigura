/*
 * PIGURA OS - INTERUPSI_ARM64.C
 * -----------------------------
 * Implementasi interface interupsi untuk arsitektur ARM64 (AArch64).
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola interupsi pada
 * prosesor ARM64, termasuk setup vector table dan dispatch handler.
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

/* Exception vector offsets */
#define VEC_OFFSET_CURRENT_EL_SP0      0x000
#define VEC_OFFSET_CURRENT_EL_SPX      0x200
#define VEC_OFFSET_LOWER_EL_AARCH64    0x400
#define VEC_OFFSET_LOWER_EL_AARCH32    0x600

/* Exception types per offset */
#define VEC_SERROR                     0x180

/* ESR_EL1 exception class */
#define ESR_ELx_EC_SHIFT               26
#define ESR_ELx_EC_MASK                0x3F

#define ESR_ELx_EC_UNKNOWN             0x00
#define ESR_ELx_EC_WFx                 0x01
#define ESR_ELx_EC_CP15_32             0x03
#define ESR_ELx_EC_CP15_64             0x04
#define ESR_ELx_EC_CP14_MR             0x05
#define ESR_ELx_EC_CP14_LS             0x06
#define ESR_ELx_EC_FP_ASIMD            0x07
#define ESR_ELx_EC_CP10_ID             0x08
#define ESR_ELx_EC_CP14_64             0x0C
#define ESR_ELx_EC_ILL                0x0E
#define ESR_ELx_EC_SVC32              0x11
#define ESR_ELx_EC_HVC32              0x12
#define ESR_ELx_EC_SMC32              0x13
#define ESR_ELx_EC_SVC64              0x15
#define ESR_ELx_EC_HVC64              0x16
#define ESR_ELx_EC_SMC64              0x17
#define ESR_ELx_EC_SYS64              0x18
#define ESR_ELx_EC_SVE                0x19
#define ESR_ELx_EC_ERET               0x1A
#define ESR_ELx_EC_FPAC               0x1C
#define ESR_ELx_EC_IABT_LOW           0x20
#define ESR_ELx_EC_IABT_CUR           0x21
#define ESR_ELx_EC_PC_ALIGN           0x22
#define ESR_ELx_EC_DABT_LOW           0x24
#define ESR_ELx_EC_DABT_CUR           0x25
#define ESR_ELx_EC_SP_ALIGN           0x26
#define ESR_ELx_EC_FP_EXC32           0x28
#define ESR_ELx_EC_FP_EXC64           0x2C
#define ESR_ELx_EC_SERROR             0x2F
#define ESR_ELx_EC_BREAKPT_LOW        0x30
#define ESR_ELx_EC_BREAKPT_CUR        0x31
#define ESR_ELx_EC_SOFTSTP_LOW        0x32
#define ESR_ELx_EC_SOFTSTP_CUR        0x33
#define ESR_ELx_EC_WATCHPT_LOW        0x34
#define ESR_ELx_EC_WATCHPT_CUR        0x35
#define ESR_ELx_EC_BKPT32             0x38
#define ESR_ELx_EC_VECTOR32           0x3A
#define ESR_ELx_EC_BRK64              0x3C

/* ISS (Instruction Specific Syndrome) untuk data abort */
#define ISS_DFSC_MASK                 0x3F
#define ISS_WNR                       (1 << 6)
#define ISS_S1PTW                      (1 << 7)
#define ISS_CM                         (1 << 8)
#define ISS_EABORT                     (1 << 9)
#define ISS_FnV                        (1 << 10)
#define ISS_ISV                        (1 << 24)

/* GICv3 constants */
#define GICD_BASE                     0x08000000ULL
#define GICR_BASE                     0x080A0000ULL

/* GIC Distributor offsets */
#define GICD_CTLR                     0x000
#define GICD_TYPER                    0x004
#define GICD_IIDR                     0x008
#define GICD_IGROUPR                  0x080
#define GICD_ISENABLER                0x100
#define GICD_ICENABLER                0x180
#define GICD_ISPENDR                  0x200
#define GICD_ICPENDR                  0x280
#define GICD_ISACTIVER                0x300
#define GICD_ICACTIVER                0x380
#define GICD_IPRIORITYR               0x400
#define GICD_ITARGETSR                0x800
#define GICD_ICFGR                    0xC00
#define GICD_IGRPMODR                 0xD00
#define GICD_NSACR                    0xE00

/* GIC Redistributor offsets */
#define GICR_CTLR                     0x000
#define GICR_IIDR                     0x004
#define GICR_TYPER                    0x008
#define GICR_WAKER                    0x014
#define GICR_IGROUPR0                 0x080
#define GICR_ISENABLER0               0x100
#define GICR_ICENABLER0               0x180
#define GICR_ISPENDR0                 0x200
#define GICR_ICPENDR0                 0x280
#define GICR_ISACTIVER0               0x300
#define GICR_ICACTIVER0               0x380
#define GICR_IPRIORITYR0              0x400
#define GICR_ICFGR0                   0xC00

/* GIC constants */
#define GICD_CTLR_ENABLE              0x1
#define GICR_WAKER_PROCESSORSLEEP     (1 << 1)
#define GICR_WAKER_CHILDRENASLEEP     (1 << 2)

/* Jumlah IRQ maksimum */
#define JUMLAH_IRQ_MAX                1024

/*
 * ============================================================================
 * TIPE DATA LOKAL (LOCAL DATA TYPES)
 * ============================================================================
 */

/* Struktur handler interupsi */
typedef struct {
    void (*handler)(void *);
    void *data;
    bool_t aktif;
} handler_irq_t;

/* Struktur exception frame */
typedef struct {
    tak_bertanda64_t x0;
    tak_bertanda64_t x1;
    tak_bertanda64_t x2;
    tak_bertanda64_t x3;
    tak_bertanda64_t x4;
    tak_bertanda64_t x5;
    tak_bertanda64_t x6;
    tak_bertanda64_t x7;
    tak_bertanda64_t x8;
    tak_bertanda64_t x9;
    tak_bertanda64_t x10;
    tak_bertanda64_t x11;
    tak_bertanda64_t x12;
    tak_bertanda64_t x13;
    tak_bertanda64_t x14;
    tak_bertanda64_t x15;
    tak_bertanda64_t x16;
    tak_bertanda64_t x17;
    tak_bertanda64_t x18;
    tak_bertanda64_t x19;
    tak_bertanda64_t x20;
    tak_bertanda64_t x21;
    tak_bertanda64_t x22;
    tak_bertanda64_t x23;
    tak_bertanda64_t x24;
    tak_bertanda64_t x25;
    tak_bertanda64_t x26;
    tak_bertanda64_t x27;
    tak_bertanda64_t x28;
    tak_bertanda64_t x29;   /* FP */
    tak_bertanda64_t x30;   /* LR */
    tak_bertanda64_t sp;
    tak_bertanda64_t elr;
    tak_bertanda64_t spsr;
    tak_bertanda64_t esr;
    tak_bertanda64_t far;
} exception_frame_t;

/*
 * ============================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ============================================================================
 */

/* Tabel handler IRQ */
static handler_irq_t g_irq_handlers[JUMLAH_IRQ_MAX];

/* Jumlah IRQ tersedia */
static tak_bertanda32_t g_jumlah_irq = 0;

/* Flag inisialisasi */
static bool_t g_interupsi_diinisalisasi = SALAH;

/* Exception counters */
static tak_bertanda64_t g_exception_count[32];

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * _gicd_read
 * ----------
 * Baca register GIC Distributor.
 */
static inline tak_bertanda32_t _gicd_read(tak_bertanda32_t offset)
{
    volatile tak_bertanda32_t *gicd;

    gicd = (volatile tak_bertanda32_t *)GICD_BASE;

    return gicd[offset / 4];
}

/*
 * _gicd_write
 * -----------
 * Tulis register GIC Distributor.
 */
static inline void _gicd_write(tak_bertanda32_t offset,
                                tak_bertanda32_t val)
{
    volatile tak_bertanda32_t *gicd;

    gicd = (volatile tak_bertanda32_t *)GICD_BASE;
    gicd[offset / 4] = val;
}

/*
 * _gicr_read
 * ----------
 * Baca register GIC Redistributor.
 */
static inline tak_bertanda32_t _gicr_read(tak_bertanda32_t offset)
{
    volatile tak_bertanda32_t *gicr;

    gicr = (volatile tak_bertanda32_t *)GICR_BASE;

    return gicr[offset / 4];
}

/*
 * _gicr_write
 * -----------
 * Tulis register GIC Redistributor.
 */
static inline void _gicr_write(tak_bertanda32_t offset,
                                tak_bertanda32_t val)
{
    volatile tak_bertanda32_t *gicr;

    gicr = (volatile tak_bertanda32_t *)GICR_BASE;
    gicr[offset / 4] = val;
}

/*
 * _init_gicv3
 * -----------
 * Inisialisasi GICv3.
 */
static void _init_gicv3(void)
{
    tak_bertanda32_t typer;
    tak_bertanda32_t i;

    /* Baca GICD_TYPER untuk mendapatkan jumlah IRQ */
    typer = _gicd_read(GICD_TYPER);
    g_jumlah_irq = ((typer & 0x1F) + 1) * 32;

    if (g_jumlah_irq > JUMLAH_IRQ_MAX) {
        g_jumlah_irq = JUMLAH_IRQ_MAX;
    }

    /* Disable distributor */
    _gicd_write(GICD_CTLR, 0);

    /* Wait for writes to complete */
    for (i = 0; i < 1000; i++) {
        volatile tak_bertanda32_t dummy;
        dummy = _gicd_read(GICD_CTLR);
        (void)dummy;
    }

    /* Set all interrupts to Group 1 (non-secure) */
    for (i = 0; i < g_jumlah_irq / 32; i++) {
        _gicd_write(GICD_IGROUPR + i * 4, 0xFFFFFFFF);
    }

    /* Disable all interrupts */
    for (i = 0; i < g_jumlah_irq / 32; i++) {
        _gicd_write(GICD_ICENABLER + i * 4, 0xFFFFFFFF);
    }

    /* Clear all pending interrupts */
    for (i = 0; i < g_jumlah_irq / 32; i++) {
        _gicd_write(GICD_ICPENDR + i * 4, 0xFFFFFFFF);
    }

    /* Set all interrupt priorities to lowest */
    for (i = 0; i < g_jumlah_irq; i++) {
        _gicd_write(GICD_IPRIORITYR + i, 0xFF);
    }

    /* Set all interrupts to level-sensitive */
    for (i = 0; i < g_jumlah_irq / 16; i++) {
        _gicd_write(GICD_ICFGR + i * 4, 0);
    }

    /* Enable distributor */
    _gicd_write(GICD_CTLR, GICD_CTLR_ENABLE);

    /* Wake up redistributor */
    _gicr_write(GICR_WAKER, 0);
    while (_gicr_read(GICR_WAKER) & GICR_WAKER_CHILDRENASLEEP) {
        /* Wait */
    }

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
        : "r"(0x7)
        : "memory"
    );

    /* Enable group 1 interrupts */
    __asm__ __volatile__(
        "msr ICC_IGRPEN1_EL1, %0\n\t"
        "isb"
        :
        : "r"(0x1)
        : "memory"
    );
}

/*
 * _dispatch_irq
 * -------------
 * Dispatch handler IRQ.
 */
static void _dispatch_irq(tak_bertanda32_t irq)
{
    if (irq < g_jumlah_irq) {
        if (g_irq_handlers[irq].aktif &&
            g_irq_handlers[irq].handler != NULL) {
            g_irq_handlers[irq].handler(g_irq_handlers[irq].data);
        }
    }

    /* End of interrupt */
    __asm__ __volatile__(
        "msr ICC_EOIR1_EL1, %0\n\t"
        "isb"
        :
        : "r"((tak_bertanda64_t)irq)
        : "memory"
    );
}

/*
 * _get_esr_ec
 * -----------
 * Dapatkan exception class dari ESR.
 */
static inline tak_bertanda32_t _get_esr_ec(tak_bertanda64_t esr)
{
    return (tak_bertanda32_t)((esr >> ESR_ELx_EC_SHIFT) & ESR_ELx_EC_MASK);
}

/*
 * _get_esr_iss
 * ------------
 * Dapatkan instruction specific syndrome.
 */
static inline tak_bertanda32_t _get_esr_iss(tak_bertanda64_t esr)
{
    return (tak_bertanda32_t)(esr & 0x1FFFFFF);
}

/*
 * _handle_sync_exception
 * ----------------------
 * Handle synchronous exception.
 */
static void _handle_sync_exception(exception_frame_t *frame)
{
    tak_bertanda32_t ec;
    tak_bertanda32_t iss;
    tak_bertanda64_t elr;
    tak_bertanda64_t far;

    ec = _get_esr_ec(frame->esr);
    iss = _get_esr_iss(frame->esr);

    /* Increment counter */
    if (ec < 32) {
        g_exception_count[ec]++;
    }

    /* Baca FAR jika perlu */
    if (ec == ESR_ELx_EC_DABT_LOW || ec == ESR_ELx_EC_DABT_CUR ||
        ec == ESR_ELx_EC_IABT_LOW || ec == ESR_ELx_EC_IABT_CUR) {
        __asm__ __volatile__("mrs %0, far_el1" : "=r"(far));
        frame->far = far;
    }

    switch (ec) {
        case ESR_ELx_EC_SVC64:
            /* System call */
            kernel_printf("[EXC] SVC #%u dari EL0\n", iss);
            /* Akan ditangani oleh syscall handler */
            break;

        case ESR_ELx_EC_DABT_LOW:
        case ESR_ELx_EC_DABT_CUR:
            /* Data abort */
            kernel_printf("[EXC] Data Abort at 0x%lX\n", frame->elr);
            kernel_printf("[EXC] FAR: 0x%lX\n", frame->far);
            kernel_printf("[EXC] ISS: 0x%X %s\n", iss,
                (iss & ISS_WNR) ? "(Write)" : "(Read)");
            break;

        case ESR_ELx_EC_IABT_LOW:
        case ESR_ELx_EC_IABT_CUR:
            /* Instruction abort */
            kernel_printf("[EXC] Instruction Abort at 0x%lX\n", frame->elr);
            kernel_printf("[EXC] FAR: 0x%lX\n", frame->far);
            break;

        case ESR_ELx_EC_PC_ALIGN:
            /* PC alignment fault */
            kernel_printf("[EXC] PC Alignment Fault at 0x%lX\n", frame->elr);
            break;

        case ESR_ELx_EC_SP_ALIGN:
            /* SP alignment fault */
            kernel_printf("[EXC] SP Alignment Fault\n");
            break;

        case ESR_ELx_EC_BRK64:
            /* Breakpoint */
            kernel_printf("[EXC] Breakpoint at 0x%lX (ISS=%u)\n",
                frame->elr, iss);
            /* Skip instruction */
            frame->elr += 4;
            break;

        case ESR_ELx_EC_UNKNOWN:
            /* Unknown exception */
            kernel_printf("[EXC] Unknown exception at 0x%lX\n", frame->elr);
            break;

        default:
            kernel_printf("[EXC] Unhandled EC=0x%X ISS=0x%X at 0x%lX\n",
                ec, iss, frame->elr);
            break;
    }
}

/*
 * _handle_serror
 * --------------
 * Handle SError interrupt.
 */
static void _handle_serror(exception_frame_t *frame)
{
    kernel_printf("[EXC] SError at 0x%lX\n", frame->elr);
    kernel_printf("[EXC] ESR: 0x%lX\n", frame->esr);
}

/*
 * ============================================================================
 * FUNGSI PUBLIC (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * interupsi_init
 * --------------
 * Inisialisasi subsistem interupsi.
 */
status_t interupsi_init(void)
{
    tak_bertanda32_t i;

    if (g_interupsi_diinisalisasi) {
        return STATUS_BERHASIL;
    }

    /* Reset handler table */
    for (i = 0; i < JUMLAH_IRQ_MAX; i++) {
        g_irq_handlers[i].handler = NULL;
        g_irq_handlers[i].data = NULL;
        g_irq_handlers[i].aktif = SALAH;
    }

    /* Reset exception counters */
    for (i = 0; i < 32; i++) {
        g_exception_count[i] = 0;
    }

    /* Inisialisasi GICv3 */
    _init_gicv3();

    g_interupsi_diinisalisasi = BENAR;

    kernel_printf("[INT] GICv3 diinisialisasi, %u IRQ tersedia\n",
        g_jumlah_irq);

    return STATUS_BERHASIL;
}

/*
 * interupsi_register_handler
 * --------------------------
 * Register handler untuk IRQ.
 */
status_t interupsi_register_handler(tak_bertanda32_t irq,
                                     void (*handler)(void *),
                                     void *data)
{
    if (irq >= g_jumlah_irq) {
        return STATUS_PARAM_INVALID;
    }

    if (handler == NULL) {
        return STATUS_PARAM_INVALID;
    }

    g_irq_handlers[irq].handler = handler;
    g_irq_handlers[irq].data = data;
    g_irq_handlers[irq].aktif = BENAR;

    return STATUS_BERHASIL;
}

/*
 * interupsi_unregister_handler
 * ----------------------------
 * Unregister handler untuk IRQ.
 */
status_t interupsi_unregister_handler(tak_bertanda32_t irq)
{
    if (irq >= g_jumlah_irq) {
        return STATUS_PARAM_INVALID;
    }

    g_irq_handlers[irq].handler = NULL;
    g_irq_handlers[irq].data = NULL;
    g_irq_handlers[irq].aktif = SALAH;

    return STATUS_BERHASIL;
}

/*
 * interupsi_enable
 * ----------------
 * Aktifkan IRQ.
 */
status_t interupsi_enable(tak_bertanda32_t irq)
{
    tak_bertanda32_t reg_idx;
    tak_bertanda32_t bit_idx;

    if (irq >= g_jumlah_irq) {
        return STATUS_PARAM_INVALID;
    }

    if (irq < 32) {
        /* Private interrupt - GICR */
        reg_idx = irq;
        _gicr_write(GICR_ISENABLER0, (1 << reg_idx));
    } else {
        /* Shared interrupt - GICD */
        reg_idx = (irq - 32) / 32;
        bit_idx = (irq - 32) % 32;
        _gicd_write(GICD_ISENABLER + reg_idx * 4, (1 << bit_idx));
    }

    return STATUS_BERHASIL;
}

/*
 * interupsi_disable
 * -----------------
 * Nonaktifkan IRQ.
 */
status_t interupsi_disable(tak_bertanda32_t irq)
{
    tak_bertanda32_t reg_idx;
    tak_bertanda32_t bit_idx;

    if (irq >= g_jumlah_irq) {
        return STATUS_PARAM_INVALID;
    }

    if (irq < 32) {
        /* Private interrupt - GICR */
        reg_idx = irq;
        _gicr_write(GICR_ICENABLER0, (1 << reg_idx));
    } else {
        /* Shared interrupt - GICD */
        reg_idx = (irq - 32) / 32;
        bit_idx = (irq - 32) % 32;
        _gicd_write(GICD_ICENABLER + reg_idx * 4, (1 << bit_idx));
    }

    return STATUS_BERHASIL;
}

/*
 * interupsi_set_priority
 * ----------------------
 * Set prioritas IRQ.
 */
status_t interupsi_set_priority(tak_bertanda32_t irq,
                                 tak_bertanda8_t priority)
{
    if (irq >= g_jumlah_irq) {
        return STATUS_PARAM_INVALID;
    }

    /* Priority 0 = highest, 255 = lowest */
    if (irq < 32) {
        /* Private interrupt - GICR */
        volatile tak_bertanda8_t *prio;
        prio = (volatile tak_bertanda8_t *)(GICR_BASE + GICR_IPRIORITYR0);
        prio[irq] = priority;
    } else {
        /* Shared interrupt - GICD */
        volatile tak_bertanda8_t *prio;
        prio = (volatile tak_bertanda8_t *)(GICD_BASE + GICD_IPRIORITYR);
        prio[irq] = priority;
    }

    return STATUS_BERHASIL;
}

/*
 * interupsi_set_trigger
 * ---------------------
 * Set trigger type untuk IRQ.
 */
status_t interupsi_set_trigger(tak_bertanda32_t irq,
                                bool_t edge_triggered)
{
    tak_bertanda32_t reg_idx;
    tak_bertanda32_t bit_idx;
    tak_bertanda32_t val;

    if (irq >= g_jumlah_irq || irq < 32) {
        return STATUS_PARAM_INVALID;
    }

    /* Hanya untuk shared interrupts */
    reg_idx = (irq - 32) / 16;
    bit_idx = ((irq - 32) % 16) * 2;

    val = _gicd_read(GICD_ICFGR + reg_idx * 4);

    if (edge_triggered) {
        val |= (2 << bit_idx);   /* Edge-triggered */
    } else {
        val &= ~(3 << bit_idx);  /* Level-sensitive */
    }

    _gicd_write(GICD_ICFGR + reg_idx * 4, val);

    return STATUS_BERHASIL;
}

/*
 * interupsi_acknowledge
 * ---------------------
 * Acknowledge IRQ (EOI).
 */
void interupsi_acknowledge(tak_bertanda32_t irq)
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
 * interupsi_get_pending
 * ---------------------
 * Cek apakah IRQ pending.
 */
bool_t interupsi_get_pending(tak_bertanda32_t irq)
{
    tak_bertanda32_t reg_idx;
    tak_bertanda32_t bit_idx;
    tak_bertanda32_t val;

    if (irq >= g_jumlah_irq) {
        return SALAH;
    }

    if (irq < 32) {
        /* Private interrupt - GICR */
        val = _gicr_read(GICR_ISPENDR0);
        return (val & (1 << irq)) ? BENAR : SALAH;
    } else {
        /* Shared interrupt - GICD */
        reg_idx = (irq - 32) / 32;
        bit_idx = (irq - 32) % 32;
        val = _gicd_read(GICD_ISPENDR + reg_idx * 4);
        return (val & (1 << bit_idx)) ? BENAR : SALAH;
    }
}

/*
 * interupsi_set_pending
 * ---------------------
 * Set IRQ pending.
 */
void interupsi_set_pending(tak_bertanda32_t irq)
{
    tak_bertanda32_t reg_idx;
    tak_bertanda32_t bit_idx;

    if (irq >= g_jumlah_irq) {
        return;
    }

    if (irq < 32) {
        /* Private interrupt - GICR */
        _gicr_write(GICR_ISPENDR0, (1 << irq));
    } else {
        /* Shared interrupt - GICD */
        reg_idx = (irq - 32) / 32;
        bit_idx = (irq - 32) % 32;
        _gicd_write(GICD_ISPENDR + reg_idx * 4, (1 << bit_idx));
    }
}

/*
 * interupsi_clear_pending
 * -----------------------
 * Clear IRQ pending.
 */
void interupsi_clear_pending(tak_bertanda32_t irq)
{
    tak_bertanda32_t reg_idx;
    tak_bertanda32_t bit_idx;

    if (irq >= g_jumlah_irq) {
        return;
    }

    if (irq < 32) {
        /* Private interrupt - GICR */
        _gicr_write(GICR_ICPENDR0, (1 << irq));
    } else {
        /* Shared interrupt - GICD */
        reg_idx = (irq - 32) / 32;
        bit_idx = (irq - 32) % 32;
        _gicd_write(GICD_ICPENDR + reg_idx * 4, (1 << bit_idx));
    }
}

/*
 * interupsi_get_active
 * --------------------
 * Cek apakah IRQ active.
 */
bool_t interupsi_get_active(tak_bertanda32_t irq)
{
    tak_bertanda32_t reg_idx;
    tak_bertanda32_t bit_idx;
    tak_bertanda32_t val;

    if (irq >= g_jumlah_irq) {
        return SALAH;
    }

    if (irq < 32) {
        /* Private interrupt - GICR */
        val = _gicr_read(GICR_ISACTIVER0);
        return (val & (1 << irq)) ? BENAR : SALAH;
    } else {
        /* Shared interrupt - GICD */
        reg_idx = (irq - 32) / 32;
        bit_idx = (irq - 32) % 32;
        val = _gicd_read(GICD_ISACTIVER + reg_idx * 4);
        return (val & (1 << bit_idx)) ? BENAR : SALAH;
    }
}

/*
 * interupsi_get_irq_number
 * ------------------------
 * Dapatkan nomor IRQ yang sedang aktif.
 */
tak_bertanda32_t interupsi_get_irq_number(void)
{
    tak_bertanda64_t iar;

    __asm__ __volatile__(
        "mrs %0, ICC_IAR1_EL1"
        : "=r"(iar)
    );

    return (tak_bertanda32_t)(iar & 0x3FF);
}

/*
 * interupsi_spurious
 * ------------------
 * Handle spurious interrupt.
 */
void interupsi_spurious(void)
{
    kernel_printf("[INT] Spurious interrupt!\n");
}

/*
 * ============================================================================
 * HANDLER EXCEPTION (EXCEPTION HANDLERS)
 * Dipanggil dari assembly vector table
 * ============================================================================
 */

/*
 * irq_handler_el1
 * ---------------
 * Handler IRQ dari EL1.
 */
void irq_handler_el1(exception_frame_t *frame)
{
    tak_bertanda32_t irq;

    (void)frame;

    irq = interupsi_get_irq_number();

    /* IRQ 1023 = spurious */
    if (irq == 1023) {
        interupsi_spurious();
        return;
    }

    _dispatch_irq(irq);
}

/*
 * irq_handler_el0
 * ---------------
 * Handler IRQ dari EL0.
 */
void irq_handler_el0(exception_frame_t *frame)
{
    tak_bertanda32_t irq;

    (void)frame;

    irq = interupsi_get_irq_number();

    if (irq == 1023) {
        interupsi_spurious();
        return;
    }

    _dispatch_irq(irq);
}

/*
 * fiq_handler
 * -----------
 * Handler FIQ.
 */
void fiq_handler(exception_frame_t *frame)
{
    (void)frame;
    kernel_printf("[INT] FIQ received!\n");
}

/*
 * serror_handler_el1
 * ------------------
 * Handler SError dari EL1.
 */
void serror_handler_el1(exception_frame_t *frame)
{
    __asm__ __volatile__("mrs %0, esr_el1" : "=r"(frame->esr));
    _handle_serror(frame);
}

/*
 * serror_handler_el0
 * ------------------
 * Handler SError dari EL0.
 */
void serror_handler_el0(exception_frame_t *frame)
{
    __asm__ __volatile__("mrs %0, esr_el1" : "=r"(frame->esr));
    _handle_serror(frame);
}

/*
 * sync_handler_el1
 * ----------------
 * Handler synchronous exception dari EL1.
 */
void sync_handler_el1(exception_frame_t *frame)
{
    __asm__ __volatile__("mrs %0, esr_el1" : "=r"(frame->esr));
    _handle_sync_exception(frame);
}

/*
 * sync_handler_el0
 * ----------------
 * Handler synchronous exception dari EL0.
 */
void sync_handler_el0(exception_frame_t *frame)
{
    __asm__ __volatile__("mrs %0, esr_el1" : "=r"(frame->esr));
    _handle_sync_exception(frame);
}
