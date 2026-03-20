/*
 * PIGURA OS - IRQ.C
 * -----------------
 * Implementasi manajemen IRQ (Interrupt Request).
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk mengelola
 * IRQ routing, masking, dan dispatching ke handler yang sesuai.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../kernel.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* IRQ names */
static const char *irq_names[] = {
    "Timer",
    "Keyboard",
    "Cascade",
    "COM2",
    "COM1",
    "LPT2",
    "Floppy",
    "LPT1",
    "RTC",
    "ACPI",
    "Available",
    "Available",
    "Mouse",
    "FPU",
    "Primary ATA",
    "Secondary ATA"
};

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* IRQ handlers */
static void (*irq_handlers[JUMLAH_IRQ])(void) = {NULL};

/* IRQ context handlers (lebih lengkap) */
static void (*irq_context_handlers[JUMLAH_IRQ])(register_context_t *) = {NULL};

/* IRQ enable mask */
static tak_bertanda16_t irq_mask = 0xFFFB;  /* Semua disabled kecuali cascade */

/* IRQ counter */
static tak_bertanda64_t irq_count[JUMLAH_IRQ] = {0};

/* Status */
static bool_t irq_initialized = SALAH;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * irq_get_name
 * ------------
 * Dapatkan nama IRQ.
 */
static const char *irq_get_name(tak_bertanda32_t irq)
{
    if (irq >= JUMLAH_IRQ) {
        return "Unknown";
    }
    return irq_names[irq];
}

/*
 * pic_send_eoi
 * ------------
 * Kirim End of Interrupt ke PIC.
 */
static void pic_send_eoi(tak_bertanda32_t irq)
{
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_CMD_EOI);
    }
    outb(PIC1_COMMAND, PIC_CMD_EOI);
}

/*
 * pic_update_mask
 * ---------------
 * Update mask register PIC.
 */
static void pic_update_mask(void)
{
    tak_bertanda8_t mask1;
    tak_bertanda8_t mask2;

    mask1 = irq_mask & 0xFF;
    mask2 = (irq_mask >> 8) & 0xFF;

    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * irq_init
 * --------
 * Inisialisasi sistem IRQ.
 */
status_t irq_init(void)
{
    tak_bertanda32_t i;

    if (irq_initialized) {
        return STATUS_SUDAH_ADA;
    }

    /* Clear handlers */
    for (i = 0; i < JUMLAH_IRQ; i++) {
        irq_handlers[i] = NULL;
        irq_context_handlers[i] = NULL;
        irq_count[i] = 0;
    }

    /* Disable semua IRQ kecuali cascade (IRQ 2) */
    irq_mask = 0xFFFB;  /* 1111 1111 1111 1011 */
    pic_update_mask();

    irq_initialized = BENAR;

    kernel_printf("[IRQ] IRQ system initialized\n");

    return STATUS_BERHASIL;
}

/*
 * irq_handler
 * -----------
 * Handler utama untuk IRQ.
 * Dipanggil dari assembly IRQ stub.
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void irq_handler(register_context_t *ctx)
{
    tak_bertanda32_t irq;

    /* Hitung IRQ number dari interrupt number */
    irq = ctx->int_no - VEKTOR_IRQ_MULAI;

    if (irq >= JUMLAH_IRQ) {
        kernel_printf("[IRQ] Invalid IRQ: %lu\n", irq);
        pic_send_eoi(0);
        return;
    }

    /* Increment counter */
    irq_count[irq]++;

    /* Panggil handler jika ada */
    if (irq_context_handlers[irq] != NULL) {
        irq_context_handlers[irq](ctx);
    } else if (irq_handlers[irq] != NULL) {
        irq_handlers[irq]();
    }

    /* Kirim EOI */
    pic_send_eoi(irq);
}

/*
 * irq_set_handler
 * ---------------
 * Set handler sederhana untuk IRQ.
 */
status_t irq_set_handler(tak_bertanda32_t irq, void (*handler)(void))
{
    if (irq >= JUMLAH_IRQ) {
        return STATUS_PARAM_INVALID;
    }

    irq_handlers[irq] = handler;

    return STATUS_BERHASIL;
}

/*
 * irq_set_context_handler
 * -----------------------
 * Set handler dengan konteks lengkap untuk IRQ.
 */
status_t irq_set_context_handler(tak_bertanda32_t irq,
                                 void (*handler)(register_context_t *))
{
    if (irq >= JUMLAH_IRQ) {
        return STATUS_PARAM_INVALID;
    }

    irq_context_handlers[irq] = handler;

    return STATUS_BERHASIL;
}

/*
 * irq_enable
 * ----------
 * Aktifkan IRQ.
 */
status_t irq_enable(tak_bertanda32_t irq)
{
    if (irq >= JUMLAH_IRQ) {
        return STATUS_PARAM_INVALID;
    }

    /* Clear bit dalam mask */
    irq_mask &= ~(1UL << irq);

    /* Update PIC */
    pic_update_mask();

    return STATUS_BERHASIL;
}

/*
 * irq_disable
 * -----------
 * Nonaktifkan IRQ.
 */
status_t irq_disable(tak_bertanda32_t irq)
{
    if (irq >= JUMLAH_IRQ) {
        return STATUS_PARAM_INVALID;
    }

    /* Set bit dalam mask */
    irq_mask |= (1UL << irq);

    /* Update PIC */
    pic_update_mask();

    return STATUS_BERHASIL;
}

/*
 * irq_mask_all
 * ------------
 * Mask semua IRQ.
 */
void irq_mask_all(void)
{
    irq_mask = 0xFFFF;
    pic_update_mask();
}

/*
 * irq_unmask_all
 * --------------
 * Unmask semua IRQ (kecuali cascade).
 */
void irq_unmask_all(void)
{
    irq_mask = 0xFFFB;  /* Cascade harus tetap enabled */
    pic_update_mask();
}

/*
 * irq_is_enabled
 * --------------
 * Cek apakah IRQ di-enable.
 */
bool_t irq_is_enabled(tak_bertanda32_t irq)
{
    if (irq >= JUMLAH_IRQ) {
        return SALAH;
    }

    return (irq_mask & (1UL << irq)) ? SALAH : BENAR;
}

/*
 * irq_get_count
 * -------------
 * Dapatkan jumlah interrupt untuk IRQ tertentu.
 */
tak_bertanda64_t irq_get_count(tak_bertanda32_t irq)
{
    if (irq >= JUMLAH_IRQ) {
        return 0;
    }

    return irq_count[irq];
}

/*
 * irq_print_stats
 * ---------------
 * Print statistik IRQ.
 */
void irq_print_stats(void)
{
    tak_bertanda32_t i;

    kernel_printf("\nIRQ Statistics:\n");
    kernel_printf("========================================\n");
    kernel_printf(" IRQ  Name             Count      Status\n");
    kernel_printf("----------------------------------------\n");

    for (i = 0; i < JUMLAH_IRQ; i++) {
        kernel_printf(" %2lu   %-15s  %8lu   %s\n",
                      i,
                      irq_names[i],
                      irq_count[i],
                      irq_is_enabled(i) ? "Enabled" : "Disabled");
    }

    kernel_printf("========================================\n");
}

/*
 * irq_get_name
 * ------------
 * Dapatkan nama IRQ.
 */
const char *irq_get_name_public(tak_bertanda32_t irq)
{
    return irq_get_name(irq);
}

/*
 * ============================================================================
 * FUNGSI SPESIFIK IRQ (IRQ SPECIFIC FUNCTIONS)
 * ============================================================================
 */

/*
 * irq_install_timer
 * -----------------
 * Install handler untuk timer IRQ (IRQ 0).
 */
status_t irq_install_timer(void (*handler)(void))
{
    status_t status;

    status = irq_set_handler(0, handler);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    return irq_enable(0);
}

/*
 * irq_install_keyboard
 * --------------------
 * Install handler untuk keyboard IRQ (IRQ 1).
 */
status_t irq_install_keyboard(void (*handler)(void))
{
    status_t status;

    status = irq_set_handler(1, handler);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    return irq_enable(1);
}

/*
 * irq_install_mouse
 * -----------------
 * Install handler untuk mouse IRQ (IRQ 12).
 */
status_t irq_install_mouse(void (*handler)(void))
{
    status_t status;

    /* Mouse menggunakan IRQ 12 */
    status = irq_set_handler(12, handler);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    return irq_enable(12);
}

/*
 * irq_install_ata_primary
 * -----------------------
 * Install handler untuk primary ATA (IRQ 14).
 */
status_t irq_install_ata_primary(void (*handler)(void))
{
    status_t status;

    status = irq_set_handler(14, handler);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    return irq_enable(14);
}

/*
 * irq_install_ata_secondary
 * -------------------------
 * Install handler untuk secondary ATA (IRQ 15).
 */
status_t irq_install_ata_secondary(void (*handler)(void))
{
    status_t status;

    status = irq_set_handler(15, handler);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    return irq_enable(15);
}

/*
 * irq_reset_count
 * ---------------
 * Reset counter untuk IRQ tertentu.
 */
void irq_reset_count(tak_bertanda32_t irq)
{
    if (irq < JUMLAH_IRQ) {
        irq_count[irq] = 0;
    }
}

/*
 * irq_reset_all_count
 * -------------------
 * Reset semua counter IRQ.
 */
void irq_reset_all_count(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < JUMLAH_IRQ; i++) {
        irq_count[i] = 0;
    }
}
