/*
 * PIGURA OS - EXCEPTION_ARMV7.C
 * ------------------------------
 * Implementasi handler exception untuk ARMv7.
 *
 * Berkas ini berisi fungsi-fungsi untuk menangani exception
 * seperti undefined instruction, data abort, prefetch abort,
 * dan system call (SVC).
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

/* Exception types */
#define EXC_RESET               0
#define EXC_UNDEFINED           1
#define EXC_SVC                 2
#define EXC_PREFETCH_ABORT      3
#define EXC_DATA_ABORT          4
#define EXC_UNUSED              5
#define EXC_IRQ                 6
#define EXC_FIQ                 7

/* Fault Status Register encoding */
#define FSR_TRANSLATION_FAULT_L1    0x05
#define FSR_TRANSLATION_FAULT_L2    0x07
#define FSR_ACCESS_FAULT_L1         0x03
#define FSR_ACCESS_FAULT_L2         0x06
#define FSR_DOMAIN_FAULT_L1         0x09
#define FSR_DOMAIN_FAULT_L2         0x0B
#define FSR_PERMISSION_FAULT_L1     0x0D
#define FSR_PERMISSION_FAULT_L2     0x0F
#define FSR_ALIGNMENT_FAULT         0x01

/* CPSR mode bits */
#define CPSR_MODE_MASK              0x1F
#define CPSR_MODE_USR               0x10
#define CPSR_MODE_SVC               0x13

/*
 * ============================================================================
 * STRUKTUR DATA (DATA STRUCTURES)
 * ============================================================================
 */

/* Exception stack frame */
typedef struct {
    tak_bertanda32_t r0;
    tak_bertanda32_t r1;
    tak_bertanda32_t r2;
    tak_bertanda32_t r3;
    tak_bertanda32_t r4;
    tak_bertanda32_t r5;
    tak_bertanda32_t r6;
    tak_bertanda32_t r7;
    tak_bertanda32_t r8;
    tak_bertanda32_t r9;
    tak_bertanda32_t r10;
    tak_bertanda32_t r11;
    tak_bertanda32_t r12;
    tak_bertanda32_t sp;
    tak_bertanda32_t lr;
    tak_bertanda32_t pc;
    tak_bertanda32_t cpsr;
    tak_bertanda32_t spsr;
} exception_frame_t;

/* Exception handler callback type */
typedef void (*exception_handler_t)(exception_frame_t *frame);

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Exception handlers */
static exception_handler_t g_exception_handlers[8];

/* Exception counters */
static tak_bertanda64_t g_exception_count[8];

/* SVC handler */
static syscall_fn_t g_syscall_table[SYS_JUMLAH];

/*
 * ============================================================================
 * FUNGSI BANTUAN (HELPER FUNCTIONS)
 * ============================================================================
 */

/*
 * exception_get_dfar
 * ------------------
 * Dapatkan Data Fault Address Register.
 *
 * Return: Nilai DFAR
 */
static inline tak_bertanda32_t exception_get_dfar(void)
{
    tak_bertanda32_t dfar;
    __asm__ __volatile__(
        "mrc p15, 0, %0, c6, c0, 0"
        : "=r"(dfar)
    );
    return dfar;
}

/*
 * exception_get_dfsr
 * ------------------
 * Dapatkan Data Fault Status Register.
 *
 * Return: Nilai DFSR
 */
static inline tak_bertanda32_t exception_get_dfsr(void)
{
    tak_bertanda32_t dfsr;
    __asm__ __volatile__(
        "mrc p15, 0, %0, c5, c0, 0"
        : "=r"(dfsr)
    );
    return dfsr;
}

/*
 * exception_get_ifar
 * ------------------
 * Dapatkan Instruction Fault Address Register.
 *
 * Return: Nilai IFAR
 */
static inline tak_bertanda32_t exception_get_ifar(void)
{
    tak_bertanda32_t ifar;
    __asm__ __volatile__(
        "mrc p15, 0, %0, c6, c0, 2"
        : "=r"(ifar)
    );
    return ifar;
}

/*
 * exception_get_ifsr
 * ------------------
 * Dapatkan Instruction Fault Status Register.
 *
 * Return: Nilai IFSR
 */
static inline tak_bertanda32_t exception_get_ifsr(void)
{
    tak_bertanda32_t ifsr;
    __asm__ __volatile__(
        "mrc p15, 0, %0, c5, c0, 1"
        : "=r"(ifsr)
    );
    return ifsr;
}

/*
 * ============================================================================
 * FUNGSI HANDLER EXCEPTION (EXCEPTION HANDLER FUNCTIONS)
 * ============================================================================
 */

/*
 * exception_reset_handler
 * -----------------------
 * Handler untuk reset exception.
 */
void exception_reset_handler(void)
{
    g_exception_count[EXC_RESET]++;

    /* Reset tidak seharusnya terjadi saat runtime */
    hal_cpu_halt();
}

/*
 * exception_undefined_handler
 * ---------------------------
 * Handler untuk undefined instruction exception.
 *
 * Parameter:
 *   frame - Stack frame exception
 */
void exception_undefined_handler(exception_frame_t *frame)
{
    g_exception_count[EXC_UNDEFINED]++;

    /* Cek apakah dari user mode */
    if ((frame->spsr & CPSR_MODE_MASK) == CPSR_MODE_USR) {
        /* Kirim signal ke proses */
        hal_proses_exit(SIGNAL_SEGFAULT);
    } else {
        /* Kernel panic */
        hal_console_puts("\nPANIC: Undefined instruction in kernel\n");
        hal_console_puts("PC: 0x");
        /* Print PC value */
        hal_cpu_halt();
    }
}

/*
 * exception_svc_handler
 * ---------------------
 * Handler untuk SVC (system call) exception.
 *
 * Parameter:
 *   frame - Stack frame exception
 */
void exception_svc_handler(exception_frame_t *frame)
{
    tak_bertanda32_t syscall_num;
    tak_bertanda64_t result;

    g_exception_count[EXC_SVC]++;

    /* Syscall number di r7 */
    syscall_num = frame->r7;

    /* Validasi syscall number */
    if (syscall_num >= SYS_JUMLAH) {
        frame->r0 = (tak_bertanda32_t)STATUS_PARAM_INVALID;
        return;
    }

    /* Panggil syscall handler */
    if (g_syscall_table[syscall_num] != NULL) {
        result = g_syscall_table[syscall_num](
            frame->r0, frame->r1, frame->r2,
            frame->r3, frame->r4, frame->r5
        );
        frame->r0 = (tak_bertanda32_t)result;
    } else {
        frame->r0 = (tak_bertanda32_t)STATUS_TIDAK_DUKUNG;
    }
}

/*
 * exception_prefetch_abort_handler
 * --------------------------------
 * Handler untuk prefetch abort exception.
 *
 * Parameter:
 *   frame - Stack frame exception
 */
void exception_prefetch_abort_handler(exception_frame_t *frame)
{
    tak_bertanda32_t ifar;
    tak_bertanda32_t ifsr;

    g_exception_count[EXC_PREFETCH_ABORT]++;

    ifar = exception_get_ifar();
    ifsr = exception_get_ifsr();

    /* Cek apakah dari user mode */
    if ((frame->spsr & CPSR_MODE_MASK) == CPSR_MODE_USR) {
        /* Kirim signal ke proses */
        hal_proses_exit(SIGNAL_SEGFAULT);
    } else {
        /* Kernel panic */
        hal_console_puts("\nPANIC: Prefetch abort in kernel\n");
        hal_console_puts("IFAR: 0x");
        /* Print IFAR value */
        hal_cpu_halt();
    }
}

/*
 * exception_data_abort_handler
 * ----------------------------
 * Handler untuk data abort exception.
 *
 * Parameter:
 *   frame - Stack frame exception
 */
void exception_data_abort_handler(exception_frame_t *frame)
{
    tak_bertanda32_t dfar;
    tak_bertanda32_t dfsr;
    tak_bertanda32_t fault_type;

    g_exception_count[EXC_DATA_ABORT]++;

    dfar = exception_get_dfar();
    dfsr = exception_get_dfsr();

    /* Extract fault type */
    fault_type = dfsr & 0x0F;

    /* Handle page fault jika dari user mode */
    if ((frame->spsr & CPSR_MODE_MASK) == CPSR_MODE_USR) {
        /* Check fault type */
        switch (fault_type) {
            case FSR_TRANSLATION_FAULT_L1:
            case FSR_TRANSLATION_FAULT_L2:
                /* Page not present - could be demand paging */
                hal_proses_exit(SIGNAL_SEGFAULT);
                break;

            case FSR_PERMISSION_FAULT_L1:
            case FSR_PERMISSION_FAULT_L2:
                /* Permission fault - segfault */
                hal_proses_exit(SIGNAL_SEGFAULT);
                break;

            case FSR_ALIGNMENT_FAULT:
                /* Alignment fault */
                hal_proses_exit(SIGNAL_SEGFAULT);
                break;

            default:
                hal_proses_exit(SIGNAL_SEGFAULT);
                break;
        }
    } else {
        /* Kernel panic */
        hal_console_puts("\nPANIC: Data abort in kernel\n");
        hal_console_puts("DFAR: 0x");
        /* Print DFAR value */
        hal_cpu_halt();
    }
}

/*
 * exception_irq_handler
 * ---------------------
 * Handler untuk IRQ exception.
 */
void exception_irq_handler(void)
{
    g_exception_count[EXC_IRQ]++;

    /* Dispatch ke IRQ handler */
    hal_dispatch_irq();
}

/*
 * exception_fiq_handler
 * ---------------------
 * Handler untuk FIQ exception.
 */
void exception_fiq_handler(void)
{
    g_exception_count[EXC_FIQ]++;

    /* FIQ handling - fast interrupt */
    hal_dispatch_irq();
}

/*
 * ============================================================================
 * FUNGSI REGISTRASI (REGISTRATION FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_exception_register_handler
 * ------------------------------
 * Daftarkan handler untuk exception tertentu.
 *
 * Parameter:
 *   exception - Nomor exception
 *   handler   - Fungsi handler
 *
 * Return: Status operasi
 */
status_t hal_exception_register_handler(tak_bertanda32_t exception,
    void (*handler)(exception_frame_t *))
{
    if (exception >= 8) {
        return STATUS_PARAM_INVALID;
    }

    g_exception_handlers[exception] = handler;
    return STATUS_BERHASIL;
}

/*
 * hal_syscall_register
 * --------------------
 * Daftarkan handler untuk syscall tertentu.
 *
 * Parameter:
 *   num     - Nomor syscall
 *   handler - Fungsi handler
 *
 * Return: Status operasi
 */
status_t hal_syscall_register(tak_bertanda32_t num, syscall_fn_t handler)
{
    if (num >= SYS_JUMLAH) {
        return STATUS_PARAM_INVALID;
    }

    g_syscall_table[num] = handler;
    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI STATISTIK (STATISTICS FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_exception_get_count
 * -----------------------
 * Dapatkan jumlah exception.
 *
 * Parameter:
 *   exception - Nomor exception
 *
 * Return: Jumlah exception
 */
tak_bertanda64_t hal_exception_get_count(tak_bertanda32_t exception)
{
    if (exception >= 8) {
        return 0;
    }

    return g_exception_count[exception];
}

/*
 * ============================================================================
 * FUNGSI INISIALISASI (INITIALIZATION FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_exception_init
 * ------------------
 * Inisialisasi subsistem exception.
 *
 * Return: Status operasi
 */
status_t hal_exception_init(void)
{
    tak_bertanda32_t i;

    /* Clear exception handlers */
    for (i = 0; i < 8; i++) {
        g_exception_handlers[i] = NULL;
        g_exception_count[i] = 0;
    }

    /* Clear syscall table */
    for (i = 0; i < SYS_JUMLAH; i++) {
        g_syscall_table[i] = NULL;
    }

    /* Register default handlers */
    hal_exception_register_handler(EXC_UNDEFINED,
        exception_undefined_handler);
    hal_exception_register_handler(EXC_SVC,
        exception_svc_handler);
    hal_exception_register_handler(EXC_PREFETCH_ABORT,
        exception_prefetch_abort_handler);
    hal_exception_register_handler(EXC_DATA_ABORT,
        exception_data_abort_handler);

    return STATUS_BERHASIL;
}
