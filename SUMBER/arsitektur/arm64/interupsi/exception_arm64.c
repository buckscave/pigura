/*
 * PIGURA OS - EXCEPTION_ARM64.C
 * -----------------------------
 * Implementasi exception handling untuk ARM64 (AArch64).
 *
 * Berkas ini berisi fungsi-fungsi untuk menangani berbagai
 * jenis exception pada prosesor ARM64.
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

/* ESR exception classes */
#define EC_UNKNOWN              0x00
#define EC_WFI_WFE              0x01
#define EC_CP15_MRC_MCR         0x03
#define EC_CP15_MRRC_MCRR       0x04
#define EC_CP14_MRC_MCR         0x05
#define EC_CP14_LDC_STC         0x06
#define EC_FP_SIMD              0x07
#define EC_CP10_MRC             0x08
#define EC_CP14_MRRC            0x0C
#define EC_ILL_EXEC             0x0E
#define EC_SVC32                0x11
#define EC_HVC32                0x12
#define EC_SMC32                0x13
#define EC_SVC64                0x15
#define EC_HVC64                0x16
#define EC_SMC64                0x17
#define EC_SYS_REG              0x18
#define EC_IABT_LOW             0x20
#define EC_IABT_CUR             0x21
#define EC_PC_ALIGN             0x22
#define EC_DABT_LOW             0x24
#define EC_DABT_CUR             0x25
#define EC_SP_ALIGN             0x26
#define EC_FP32                 0x28
#define EC_FP64                 0x2C
#define EC_SERROR               0x2F
#define EC_BRK                  0x3C

/* ISS bits for data abort */
#define ISS_DFSC_MASK           0x3F
#define ISS_WNR                 (1 << 6)
#define ISS_S1PTW               (1 << 7)
#define ISS_CM                  (1 << 8)
#define ISS_EABORT              (1 << 9)
#define ISS_FNV                 (1 << 10)
#define ISS_ISV                 (1 << 24)

/* DFSC values */
#define DFSC_TRANS_FAULT_L0     0x00
#define DFSC_TRANS_FAULT_L1     0x01
#define DFSC_TRANS_FAULT_L2     0x02
#define DFSC_TRANS_FAULT_L3     0x03
#define DFSC_ACCESS_FAULT_L0    0x04
#define DFSC_ACCESS_FAULT_L1    0x05
#define DFSC_ACCESS_FAULT_L2    0x06
#define DFSC_ACCESS_FAULT_L3    0x07
#define DFSC_PERM_FAULT_L0      0x08
#define DFSC_PERM_FAULT_L1      0x09
#define DFSC_PERM_FAULT_L2      0x0A
#define DFSC_PERM_FAULT_L3      0x0B
#define DFSC_ALIGN_FAULT        0x21
#define DFSC_TLB_CONFLICT       0x30
#define DFSC_UNSUPPORTED        0x3F

/*
 * ============================================================================
 * TIPE DATA LOKAL (LOCAL DATA TYPES)
 * ============================================================================
 */

/* Exception frame structure */
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
    tak_bertanda64_t x29;
    tak_bertanda64_t x30;
    tak_bertanda64_t sp;
    tak_bertanda64_t elr;
    tak_bertanda64_t spsr;
    tak_bertanda64_t esr;
    tak_bertanda64_t far;
} exc_frame_t;

/* Exception statistics */
typedef struct {
    tak_bertanda64_t count;
    tak_bertanda64_t last_esr;
    tak_bertanda64_t last_far;
    tak_bertanda64_t last_elr;
} exc_stat_t;

/*
 * ============================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ============================================================================
 */

/* Exception statistics per type */
static exc_stat_t g_exc_stats[64];

/* User exception handler */
static void (*g_user_exc_handler)(tak_bertanda32_t ec,
                                   tak_bertanda64_t esr,
                                   tak_bertanda64_t far,
                                   tak_bertanda64_t elr) = NULL;

/*
 * ============================================================================
 * FUNGSI BANTUAN (HELPER FUNCTIONS)
 * ============================================================================
 */

/*
 * _get_esr_ec
 * -----------
 * Extract exception class from ESR.
 */
static inline tak_bertanda32_t _get_esr_ec(tak_bertanda64_t esr)
{
    return (tak_bertanda32_t)((esr >> 26) & 0x3F);
}

/*
 * _get_esr_iss
 * ------------
 * Extract ISS from ESR.
 */
static inline tak_bertanda32_t _get_esr_iss(tak_bertanda64_t esr)
{
    return (tak_bertanda32_t)(esr & 0x1FFFFFF);
}

/*
 * _get_dfsc
 * ---------
 * Extract DFSC from ISS.
 */
static inline tak_bertanda32_t _get_dfsc(tak_bertanda32_t iss)
{
    return (iss & ISS_DFSC_MASK);
}

/*
 * _is_write
 * ---------
 * Check if data abort was caused by write.
 */
static inline bool_t _is_write(tak_bertanda32_t iss)
{
    return (iss & ISS_WNR) ? BENAR : SALAH;
}

/*
 * _is_translation_fault
 * ---------------------
 * Check if translation fault.
 */
static inline bool_t _is_translation_fault(tak_bertanda32_t dfsc)
{
    return (dfsc >= 0x00 && dfsc <= 0x03);
}

/*
 * _is_permission_fault
 * --------------------
 * Check if permission fault.
 */
static inline bool_t _is_permission_fault(tak_bertanda32_t dfsc)
{
    return (dfsc >= 0x08 && dfsc <= 0x0B);
}

/*
 * _is_access_fault
 * ----------------
 * Check if access fault.
 */
static inline bool_t _is_access_fault(tak_bertanda32_t dfsc)
{
    return (dfsc >= 0x04 && dfsc <= 0x07);
}

/*
 * _get_fault_level
 * ----------------
 * Get fault level from DFSC.
 */
static inline tak_bertanda32_t _get_fault_level(tak_bertanda32_t dfsc)
{
    if (_is_translation_fault(dfsc) ||
        _is_permission_fault(dfsc) ||
        _is_access_fault(dfsc)) {
        return dfsc & 0x3;
    }
    return 0;
}

/*
 * _update_stats
 * -------------
 * Update exception statistics.
 */
static void _update_stats(tak_bertanda32_t ec, tak_bertanda64_t esr,
                           tak_bertanda64_t far, tak_bertanda64_t elr)
{
    if (ec < 64) {
        g_exc_stats[ec].count++;
        g_exc_stats[ec].last_esr = esr;
        g_exc_stats[ec].last_far = far;
        g_exc_stats[ec].last_elr = elr;
    }
}

/*
 * _get_ec_name
 * ------------
 * Get exception class name string.
 */
static const char *_get_ec_name(tak_bertanda32_t ec)
{
    switch (ec) {
        case EC_UNKNOWN:       return "Unknown";
        case EC_WFI_WFE:       return "WFI/WFE";
        case EC_CP15_MRC_MCR:  return "CP15 MRC/MCR";
        case EC_CP15_MRRC_MCRR: return "CP15 MRRC/MCRR";
        case EC_CP14_MRC_MCR:  return "CP14 MRC/MCR";
        case EC_CP14_LDC_STC:  return "CP14 LDC/STC";
        case EC_FP_SIMD:       return "FP/SIMD";
        case EC_CP10_MRC:      return "CP10 MRC";
        case EC_CP14_MRRC:     return "CP14 MRRC";
        case EC_ILL_EXEC:      return "Illegal Execution";
        case EC_SVC32:         return "SVC (AArch32)";
        case EC_HVC32:         return "HVC (AArch32)";
        case EC_SMC32:         return "SMC (AArch32)";
        case EC_SVC64:         return "SVC (AArch64)";
        case EC_HVC64:         return "HVC (AArch64)";
        case EC_SMC64:         return "SMC (AArch64)";
        case EC_SYS_REG:       return "System Register";
        case EC_IABT_LOW:      return "Instruction Abort (Lower EL)";
        case EC_IABT_CUR:      return "Instruction Abort (Current EL)";
        case EC_PC_ALIGN:      return "PC Alignment";
        case EC_DABT_LOW:      return "Data Abort (Lower EL)";
        case EC_DABT_CUR:      return "Data Abort (Current EL)";
        case EC_SP_ALIGN:      return "SP Alignment";
        case EC_FP32:          return "FP Exception (AArch32)";
        case EC_FP64:          return "FP Exception (AArch64)";
        case EC_SERROR:        return "SError";
        case EC_BRK:           return "Breakpoint";
        default:               return "Unknown EC";
    }
}

/*
 * _get_dfsc_name
 * --------------
 * Get DFSC name string.
 */
static const char *_get_dfsc_name(tak_bertanda32_t dfsc)
{
    switch (dfsc) {
        case DFSC_TRANS_FAULT_L0: return "Translation fault (L0)";
        case DFSC_TRANS_FAULT_L1: return "Translation fault (L1)";
        case DFSC_TRANS_FAULT_L2: return "Translation fault (L2)";
        case DFSC_TRANS_FAULT_L3: return "Translation fault (L3)";
        case DFSC_ACCESS_FAULT_L0: return "Access flag fault (L0)";
        case DFSC_ACCESS_FAULT_L1: return "Access flag fault (L1)";
        case DFSC_ACCESS_FAULT_L2: return "Access flag fault (L2)";
        case DFSC_ACCESS_FAULT_L3: return "Access flag fault (L3)";
        case DFSC_PERM_FAULT_L0: return "Permission fault (L0)";
        case DFSC_PERM_FAULT_L1: return "Permission fault (L1)";
        case DFSC_PERM_FAULT_L2: return "Permission fault (L2)";
        case DFSC_PERM_FAULT_L3: return "Permission fault (L3)";
        case DFSC_ALIGN_FAULT: return "Alignment fault";
        case DFSC_TLB_CONFLICT: return "TLB conflict";
        case DFSC_UNSUPPORTED: return "Unsupported";
        default: return "Unknown DFSC";
    }
}

/*
 * ============================================================================
 * EXCEPTION HANDLERS
 * ============================================================================
 */

/*
 * exception_handle_sync
 * ---------------------
 * Handle synchronous exception.
 */
void exception_handle_sync(exc_frame_t *frame)
{
    tak_bertanda32_t ec;
    tak_bertanda32_t iss;
    tak_bertanda32_t dfsc;
    tak_bertanda32_t level;
    bool_t is_write;
    const char *ec_name;

    ec = _get_esr_ec(frame->esr);
    iss = _get_esr_iss(frame->esr);
    dfsc = _get_dfsc(iss);
    level = _get_fault_level(dfsc);
    is_write = _is_write(iss);
    ec_name = _get_ec_name(ec);

    /* Update statistics */
    _update_stats(ec, frame->esr, frame->far, frame->elr);

    /* Log exception */
    kernel_printf("[EXC] %s at 0x%lX\n", ec_name, frame->elr);
    kernel_printf("[EXC] ESR: 0x%lX, ISS: 0x%X\n", frame->esr, iss);

    switch (ec) {
        case EC_SVC64:
            /* System call - handled by syscall handler */
            kernel_printf("[EXC] SVC #%u\n", iss);
            break;

        case EC_HVC64:
            /* Hypervisor call */
            kernel_printf("[EXC] HVC #%u\n", iss);
            break;

        case EC_SMC64:
            /* Secure monitor call */
            kernel_printf("[EXC] SMC #%u\n", iss);
            break;

        case EC_IABT_LOW:
        case EC_IABT_CUR:
            /* Instruction abort */
            kernel_printf("[EXC] Instruction Abort\n");
            kernel_printf("[EXC] FAR: 0x%lX\n", frame->far);
            kernel_printf("[EXC] Fault: %s\n", _get_dfsc_name(dfsc));
            kernel_printf("[EXC] Level: %u\n", level);
            break;

        case EC_DABT_LOW:
        case EC_DABT_CUR:
            /* Data abort */
            kernel_printf("[EXC] Data Abort (%s)\n",
                is_write ? "Write" : "Read");
            kernel_printf("[EXC] FAR: 0x%lX\n", frame->far);
            kernel_printf("[EXC] Fault: %s\n", _get_dfsc_name(dfsc));
            kernel_printf("[EXC] Level: %u\n", level);

            /* Check for valid FAR */
            if (iss & ISS_FNV) {
                kernel_printf("[EXC] FAR not valid\n");
            }

            /* Check for S1PTW */
            if (iss & ISS_S1PTW) {
                kernel_printf("[EXC] Stage 2 fault during stage 1 walk\n");
            }
            break;

        case EC_PC_ALIGN:
            /* PC alignment fault */
            kernel_printf("[EXC] PC Alignment Fault at 0x%lX\n", frame->elr);
            break;

        case EC_SP_ALIGN:
            /* SP alignment fault */
            kernel_printf("[EXC] SP Alignment Fault\n");
            kernel_printf("[EXC] SP: 0x%lX\n", frame->sp);
            break;

        case EC_FP_SIMD:
            /* Floating point/SIMD exception */
            kernel_printf("[EXC] FP/SIMD Exception\n");
            break;

        case EC_FP64:
            /* Floating point exception (AArch64) */
            kernel_printf("[EXC] FP Exception (AArch64)\n");
            break;

        case EC_BRK:
            /* Breakpoint */
            kernel_printf("[EXC] Breakpoint #%u at 0x%lX\n",
                iss & 0xFFFF, frame->elr);
            /* Skip the BRK instruction */
            frame->elr += 4;
            break;

        case EC_ILL_EXEC:
            /* Illegal execution state */
            kernel_printf("[EXC] Illegal Execution State\n");
            break;

        case EC_UNKNOWN:
        default:
            /* Unknown exception */
            kernel_printf("[EXC] Unknown Exception (EC=%u)\n", ec);
            break;
    }
}

/*
 * exception_handle_serror
 * -----------------------
 * Handle SError interrupt.
 */
void exception_handle_serror(exc_frame_t *frame)
{
    tak_bertanda32_t ec;

    ec = _get_esr_ec(frame->esr);

    _update_stats(ec, frame->esr, frame->far, frame->elr);

    kernel_printf("[EXC] SError Interrupt!\n");
    kernel_printf("[EXC] ELR: 0x%lX\n", frame->elr);
    kernel_printf("[EXC] ESR: 0x%lX\n", frame->esr);
    kernel_printf("[EXC] FAR: 0x%lX\n", frame->far);

    /* SError is typically fatal */
    kernel_printf("[EXC] System halted due to SError\n");

    while (1) {
        __asm__ __volatile__("wfi");
    }
}

/*
 * exception_handle_irq
 * --------------------
 * Handle IRQ.
 */
void exception_handle_irq(exc_frame_t *frame)
{
    (void)frame;
    /* IRQ handling is done by GICv3 driver */
}

/*
 * exception_handle_fiq
 * --------------------
 * Handle FIQ.
 */
void exception_handle_fiq(exc_frame_t *frame)
{
    (void)frame;
    kernel_printf("[EXC] FIQ received\n");
}

/*
 * ============================================================================
 * USER EXCEPTION HANDLERS
 * ============================================================================
 */

/*
 * exception_user_sync_handler
 * ----------------------------
 * Handle synchronous exception from user mode (EL0).
 */
void exception_user_sync_handler(exc_frame_t *frame)
{
    tak_bertanda32_t ec;

    ec = _get_esr_ec(frame->esr);

    /* Call user handler if registered */
    if (g_user_exc_handler != NULL) {
        g_user_exc_handler(ec, frame->esr, frame->far, frame->elr);
        return;
    }

    /* Default handling */
    exception_handle_sync(frame);

    /* For user exceptions, we may want to terminate the process */
    if (ec != EC_SVC64 && ec != EC_BRK) {
        kernel_printf("[EXC] Terminating user process\n");
        /* proses_exit(-1); */
    }
}

/*
 * exception_user_irq_handler
 * --------------------------
 * Handle IRQ from user mode (EL0).
 */
void exception_user_irq_handler(exc_frame_t *frame)
{
    exception_handle_irq(frame);
}

/*
 * exception_user_fiq_handler
 * --------------------------
 * Handle FIQ from user mode (EL0).
 */
void exception_user_fiq_handler(exc_frame_t *frame)
{
    exception_handle_fiq(frame);
}

/*
 * exception_user_serror_handler
 * -----------------------------
 * Handle SError from user mode (EL0).
 */
void exception_user_serror_handler(exc_frame_t *frame)
{
    kernel_printf("[EXC] SError from user process\n");
    exception_handle_serror(frame);
}

/*
 * ============================================================================
 * KERNEL EXCEPTION HANDLERS
 * ============================================================================
 */

/*
 * exception_sync_handler
 * ----------------------
 * Handle synchronous exception from kernel mode (EL1).
 */
void exception_sync_handler(exc_frame_t *frame)
{
    exception_handle_sync(frame);
}

/*
 * exception_irq_handler
 * ---------------------
 * Handle IRQ from kernel mode (EL1).
 */
void exception_irq_handler(exc_frame_t *frame)
{
    exception_handle_irq(frame);
}

/*
 * exception_fiq_handler
 * ---------------------
 * Handle FIQ from kernel mode (EL1).
 */
void exception_fiq_handler(exc_frame_t *frame)
{
    exception_handle_fiq(frame);
}

/*
 * exception_serror_handler
 * ------------------------
 * Handle SError from kernel mode (EL1).
 */
void exception_serror_handler(exc_frame_t *frame)
{
    exception_handle_serror(frame);
}

/*
 * ============================================================================
 * PUBLIC API
 * ============================================================================
 */

/*
 * exception_set_user_handler
 * --------------------------
 * Set user exception handler.
 */
void exception_set_user_handler(void (*handler)(tak_bertanda32_t ec,
                                                 tak_bertanda64_t esr,
                                                 tak_bertanda64_t far,
                                                 tak_bertanda64_t elr))
{
    g_user_exc_handler = handler;
}

/*
 * exception_get_stats
 * -------------------
 * Get exception statistics.
 */
tak_bertanda64_t exception_get_count(tak_bertanda32_t ec)
{
    if (ec >= 64) {
        return 0;
    }
    return g_exc_stats[ec].count;
}

/*
 * exception_dump_stats
 * --------------------
 * Dump exception statistics.
 */
void exception_dump_stats(void)
{
    tak_bertanda32_t i;

    kernel_printf("\n=== Exception Statistics ===\n");

    for (i = 0; i < 64; i++) {
        if (g_exc_stats[i].count > 0) {
            kernel_printf("%-28s: %lu\n",
                _get_ec_name(i), g_exc_stats[i].count);
        }
    }
}
