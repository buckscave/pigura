/*
 * PIGURA OS - INTERRUPT_ARM.C
 * ----------------------------
 * Implementasi handler interupsi low-level untuk ARM (32-bit).
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

/* IRQ numbers untuk QEMU virt machine */
#define IRQ_TIMER_VIRT          27
#define IRQ_UART0               33
#define IRQ_UART1               34
#define IRQ_RTC                 36
#define IRQ_GPIO                39
#define IRQ_SDHCI               47
#define IRQ_USB                 48
#define IRQ_ETHERNET            49

/* Maximum IRQ handlers */
#define MAX_IRQ_HANDLERS        128

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* IRQ handler table */
static void (*g_irq_handlers[MAX_IRQ_HANDLERS])(void);

/* IRQ counter */
static tak_bertanda64_t g_irq_count[MAX_IRQ_HANDLERS];

/* Flag inisialisasi */
static bool_t g_interrupt_initialized = SALAH;

/*
 * ============================================================================
 * FUNGSI ENTRY POINT (ENTRY POINT FUNCTIONS)
 * ============================================================================
 */

/*
 * interrupt_irq_entry
 * -------------------
 * Entry point untuk IRQ handler.
 * Dipanggil dari assembly vector.
 */
void interrupt_irq_entry(void)
{
    tak_bertanda32_t irq;

    /* Disable interrupts */
    hal_cpu_disable_interrupts();

    /* Get IRQ number dari GIC */
    irq = hal_gic_acknowledge();

    /* Validasi IRQ number */
    if (irq < MAX_IRQ_HANDLERS) {
        /* Increment counter */
        g_irq_count[irq]++;

        /* Panggil handler jika ada */
        if (g_irq_handlers[irq] != NULL) {
            g_irq_handlers[irq]();
        }
    }

    /* End of interrupt */
    hal_gic_end_interrupt(irq);

    /* Enable interrupts */
    hal_cpu_enable_interrupts();
}

/*
 * interrupt_fiq_entry
 * -------------------
 * Entry point untuk FIQ handler.
 */
void interrupt_fiq_entry(void)
{
    /* FIQ handler - fast interrupt */
    interrupt_irq_entry();
}

/*
 * ============================================================================
 * FUNGSI HANDLER REGISTRASI (HANDLER REGISTRATION FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_interrupt_register_handler
 * ------------------------------
 * Daftarkan handler untuk IRQ tertentu.
 *
 * Parameter:
 *   irq     - Nomor IRQ
 *   handler - Fungsi handler
 *
 * Return: Status operasi
 */
status_t hal_interrupt_register_handler(tak_bertanda32_t irq,
    void (*handler)(void))
{
    if (irq >= MAX_IRQ_HANDLERS) {
        return STATUS_PARAM_INVALID;
    }

    g_irq_handlers[irq] = handler;
    return STATUS_BERHASIL;
}

/*
 * hal_interrupt_unregister_handler
 * --------------------------------
 * Hapus handler untuk IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: Status operasi
 */
status_t hal_interrupt_unregister_handler(tak_bertanda32_t irq)
{
    if (irq >= MAX_IRQ_HANDLERS) {
        return STATUS_PARAM_INVALID;
    }

    g_irq_handlers[irq] = NULL;
    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI ENABLE/DISABLE (ENABLE/DISABLE FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_interrupt_enable_irq
 * ------------------------
 * Aktifkan IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: Status operasi
 */
status_t hal_interrupt_enable_irq(tak_bertanda32_t irq)
{
    if (irq >= MAX_IRQ_HANDLERS) {
        return STATUS_PARAM_INVALID;
    }

    return hal_gic_enable_irq(irq);
}

/*
 * hal_interrupt_disable_irq
 * -------------------------
 * Nonaktifkan IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: Status operasi
 */
status_t hal_interrupt_disable_irq(tak_bertanda32_t irq)
{
    if (irq >= MAX_IRQ_HANDLERS) {
        return STATUS_PARAM_INVALID;
    }

    return hal_gic_disable_irq(irq);
}

/*
 * ============================================================================
 * FUNGSI TIMER INTERRUPT (TIMER INTERRUPT FUNCTIONS)
 * ============================================================================
 */

/* Timer handler */
static void (*g_timer_handler)(void) = NULL;

/*
 * hal_timer_interrupt_handler
 * ---------------------------
 * Handler untuk timer interrupt.
 */
void hal_timer_interrupt_handler(void)
{
    /* Panggil handler jika ada */
    if (g_timer_handler != NULL) {
        g_timer_handler();
    }
}

/*
 * hal_timer_set_interrupt_handler
 * -------------------------------
 * Set handler untuk timer interrupt.
 *
 * Parameter:
 *   handler - Fungsi handler
 *
 * Return: Status operasi
 */
status_t hal_timer_set_interrupt_handler(void (*handler)(void))
{
    g_timer_handler = handler;

    /* Register handler untuk timer IRQ */
    hal_interrupt_register_handler(IRQ_TIMER_VIRT,
        hal_timer_interrupt_handler);

    /* Enable timer IRQ */
    hal_interrupt_enable_irq(IRQ_TIMER_VIRT);

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI INISIALISASI (INITIALIZATION FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_interrupt_init
 * ------------------
 * Inisialisasi subsistem interupsi.
 *
 * Return: Status operasi
 */
status_t hal_interrupt_init(void)
{
    tak_bertanda32_t i;

    /* Clear handler table */
    for (i = 0; i < MAX_IRQ_HANDLERS; i++) {
        g_irq_handlers[i] = NULL;
        g_irq_count[i] = 0;
    }

    /* Inisialisasi GIC */
    hal_gic_init();

    g_interrupt_initialized = BENAR;

    return STATUS_BERHASIL;
}

/*
 * hal_interrupt_shutdown
 * ----------------------
 * Shutdown subsistem interupsi.
 *
 * Return: Status operasi
 */
status_t hal_interrupt_shutdown(void)
{
    tak_bertanda32_t i;

    /* Disable semua IRQ */
    for (i = 0; i < MAX_IRQ_HANDLERS; i++) {
        if (g_irq_handlers[i] != NULL) {
            hal_gic_disable_irq(i);
            g_irq_handlers[i] = NULL;
        }
    }

    g_interrupt_initialized = SALAH;

    return STATUS_BERHASIL;
}

/*
 * hal_interrupt_get_count
 * -----------------------
 * Dapatkan jumlah interrupt untuk IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: Jumlah interrupt
 */
tak_bertanda64_t hal_interrupt_get_count(tak_bertanda32_t irq)
{
    if (irq >= MAX_IRQ_HANDLERS) {
        return 0;
    }

    return g_irq_count[irq];
}
