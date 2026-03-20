/*
 * PIGURA OS - HAL_INTERUPSI.C
 * ---------------------------
 * Implementasi interupsi untuk Hardware Abstraction Layer.
 *
 * Berkas ini berisi implementasi fungsi-fungsi yang berkaitan dengan
 * manajemen interupsi, termasuk PIC (Programmable Interrupt Controller)
 * dan handler interupsi.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "hal.h"
#include "../kernel.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* PIC ports */
#define PIC1_CMD                0x20
#define PIC1_DATA               0x21
#define PIC2_CMD                0xA0
#define PIC2_DATA               0xA1

/* PIC commands */
#define PIC_EOI                 0x20
#define PIC_ICW1_INIT           0x10
#define PIC_ICW1_ICW4           0x01
#define PIC_ICW4_8086           0x01

/* PIC interrupt vector offsets */
#define PIC1_OFFSET             32
#define PIC2_OFFSET             40

/* Maximum IRQ handlers */
#define MAX_IRQ_HANDLERS        16

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* IRQ handlers array */
static void (*irq_handlers[MAX_IRQ_HANDLERS])(void) = {NULL};

/* IRQ mask state */
static tak_bertanda16_t irq_mask = 0xFFFB; /* Semua disabled kecuali cascade */

/* State inisialisasi */
static bool_t interrupt_initialized = SALAH;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * pic_remap
 * ---------
 * Remap PIC ke vektor interupsi yang berbeda.
 *
 * Parameter:
 *   offset1 - Offset untuk PIC1
 *   offset2 - Offset untuk PIC2
 */
static void pic_remap(tak_bertanda8_t offset1, tak_bertanda8_t offset2)
{
    tak_bertanda8_t mask1;
    tak_bertanda8_t mask2;

    /* Simpan mask saat ini */
    mask1 = inb(PIC1_DATA);
    mask2 = inb(PIC2_DATA);

    /* Start initialization sequence */
    outb(PIC1_CMD, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_delay();
    outb(PIC2_CMD, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_delay();

    /* Set vector offsets */
    outb(PIC1_DATA, offset1);
    io_delay();
    outb(PIC2_DATA, offset2);
    io_delay();

    /* Tell PIC1 there's a PIC2 at IRQ2 */
    outb(PIC1_DATA, 4);
    io_delay();
    /* Tell PIC2 its cascade identity */
    outb(PIC2_DATA, 2);
    io_delay();

    /* Set 8086 mode */
    outb(PIC1_DATA, PIC_ICW4_8086);
    io_delay();
    outb(PIC2_DATA, PIC_ICW4_8086);
    io_delay();

    /* Restore mask */
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

/*
 * pic_set_mask
 * ------------
 * Set mask untuk IRQ tertentu.
 *
 * Parameter:
 *   irq   - Nomor IRQ (0-15)
 *   mask  - 1 untuk mask, 0 untuk unmask
 */
static void pic_set_mask(tak_bertanda8_t irq, bool_t mask)
{
    tak_bertanda8_t port;
    tak_bertanda8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port);

    if (mask) {
        value |= (1 << irq);
    } else {
        value &= ~(1 << irq);
    }

    outb(port, value);
}

/*
 * pic_get_mask
 * ------------
 * Dapatkan status mask untuk IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ (0-15)
 *
 * Return: 1 jika masked, 0 jika tidak
 */
static bool_t pic_get_mask(tak_bertanda8_t irq)
{
    tak_bertanda8_t port;
    tak_bertanda8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port);

    return (value & (1 << irq)) ? BENAR : SALAH;
}

/*
 * pic_send_eoi
 * ------------
 * Kirim End of Interrupt ke PIC.
 *
 * Parameter:
 *   irq - Nomor IRQ
 */
static void pic_send_eoi(tak_bertanda8_t irq)
{
    /* Jika IRQ dari PIC2, kirim EOI ke PIC2 dulu */
    if (irq >= 8) {
        outb(PIC2_CMD, PIC_EOI);
    }

    /* Selalu kirim EOI ke PIC1 */
    outb(PIC1_CMD, PIC_EOI);
}

/*
 * pic_disable_all
 * ---------------
 * Disable semua IRQ.
 */
static void pic_disable_all(void)
{
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_interrupt_init
 * ------------------
 * Inisialisasi subsistem interupsi.
 */
status_t hal_interrupt_init(void)
{
    hal_interrupt_info_t *info = &g_hal_state.interrupt;
    tak_bertanda32_t i;

    if (interrupt_initialized) {
        return STATUS_SUDAH_ADA;
    }

    /* Clear semua handler */
    for (i = 0; i < MAX_IRQ_HANDLERS; i++) {
        irq_handlers[i] = NULL;
    }

    /* Remap PIC ke vektor 32-47 */
    pic_remap(PIC1_OFFSET, PIC2_OFFSET);

    /* Disable semua IRQ kecuali cascade (IRQ 2) */
    pic_disable_all();
    pic_set_mask(2, SALAH); /* Enable cascade */

    /* Set informasi interupsi */
    info->irq_count = JUMLAH_IRQ;
    info->vector_base = PIC1_OFFSET;
    info->has_apic = SALAH;
    info->has_ioapic = SALAH;

    interrupt_initialized = BENAR;

    return STATUS_BERHASIL;
}

/*
 * hal_interrupt_enable
 * --------------------
 * Aktifkan IRQ tertentu.
 */
status_t hal_interrupt_enable(tak_bertanda32_t irq)
{
    if (irq >= MAX_IRQ_HANDLERS) {
        return STATUS_PARAM_INVALID;
    }

    if (!interrupt_initialized) {
        return STATUS_GAGAL;
    }

    /* Unmask IRQ */
    pic_set_mask((tak_bertanda8_t)irq, SALAH);

    return STATUS_BERHASIL;
}

/*
 * hal_interrupt_disable
 * ---------------------
 * Nonaktifkan IRQ tertentu.
 */
status_t hal_interrupt_disable(tak_bertanda32_t irq)
{
    if (irq >= MAX_IRQ_HANDLERS) {
        return STATUS_PARAM_INVALID;
    }

    if (!interrupt_initialized) {
        return STATUS_GAGAL;
    }

    /* Mask IRQ */
    pic_set_mask((tak_bertanda8_t)irq, BENAR);

    return STATUS_BERHASIL;
}

/*
 * hal_interrupt_mask
 * ------------------
 * Mask IRQ tertentu.
 */
status_t hal_interrupt_mask(tak_bertanda32_t irq)
{
    return hal_interrupt_disable(irq);
}

/*
 * hal_interrupt_unmask
 * --------------------
 * Unmask IRQ tertentu.
 */
status_t hal_interrupt_unmask(tak_bertanda32_t irq)
{
    return hal_interrupt_enable(irq);
}

/*
 * hal_interrupt_acknowledge
 * -------------------------
 * Acknowledge IRQ (End of Interrupt).
 */
void hal_interrupt_acknowledge(tak_bertanda32_t irq)
{
    if (irq >= MAX_IRQ_HANDLERS) {
        return;
    }

    pic_send_eoi((tak_bertanda8_t)irq);
}

/*
 * hal_interrupt_set_handler
 * -------------------------
 * Set handler untuk IRQ tertentu.
 */
status_t hal_interrupt_set_handler(tak_bertanda32_t irq,
                                   void (*handler)(void))
{
    if (irq >= MAX_IRQ_HANDLERS) {
        return STATUS_PARAM_INVALID;
    }

    if (!interrupt_initialized) {
        return STATUS_GAGAL;
    }

    irq_handlers[irq] = handler;

    return STATUS_BERHASIL;
}

/*
 * hal_interrupt_get_handler
 * -------------------------
 * Dapatkan handler untuk IRQ tertentu.
 */
void (*hal_interrupt_get_handler(tak_bertanda32_t irq))(void)
{
    if (irq >= MAX_IRQ_HANDLERS) {
        return NULL;
    }

    return irq_handlers[irq];
}

/*
 * ============================================================================
 * FUNGSI DISPATCHER INTERUPSI (INTERRUPT DISPATCHER)
 * ============================================================================
 */

/*
 * hal_irq_handler
 * ---------------
 * Handler IRQ umum yang dipanggil dari ISR.
 * Fungsi ini akan memanggil handler spesifik untuk IRQ tersebut.
 *
 * Parameter:
 *   irq - Nomor IRQ
 */
void hal_irq_handler(tak_bertanda32_t irq)
{
    if (irq >= MAX_IRQ_HANDLERS) {
        return;
    }

    /* Panggil handler jika ada */
    if (irq_handlers[irq] != NULL) {
        irq_handlers[irq]();
    }

    /* Kirim EOI */
    pic_send_eoi((tak_bertanda8_t)irq);
}

/*
 * ============================================================================
 * FUNGSI UTILITAS INTERUPSI (INTERRUPT UTILITY FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_interrupt_is_masked
 * -----------------------
 * Cek apakah IRQ di-mask.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: BENAR jika masked, SALAH jika tidak
 */
bool_t hal_interrupt_is_masked(tak_bertanda32_t irq)
{
    if (irq >= MAX_IRQ_HANDLERS) {
        return BENAR;
    }

    return pic_get_mask((tak_bertanda8_t)irq);
}

/*
 * hal_interrupt_get_mask_all
 * --------------------------
 * Dapatkan mask semua IRQ.
 *
 * Return: Bitmask IRQ (bit 0 = IRQ0, dst)
 */
tak_bertanda16_t hal_interrupt_get_mask_all(void)
{
    tak_bertanda8_t mask1;
    tak_bertanda8_t mask2;

    mask1 = inb(PIC1_DATA);
    mask2 = inb(PIC2_DATA);

    return ((tak_bertanda16_t)mask2 << 8) | mask1;
}

/*
 * hal_interrupt_set_mask_all
 * --------------------------
 * Set mask untuk semua IRQ.
 *
 * Parameter:
 *   mask - Bitmask IRQ
 */
void hal_interrupt_set_mask_all(tak_bertanda16_t mask)
{
    outb(PIC1_DATA, (tak_bertanda8_t)(mask & 0xFF));
    outb(PIC2_DATA, (tak_bertanda8_t)((mask >> 8) & 0xFF));
}

/*
 * hal_interrupt_spurious
 * ----------------------
 * Handler untuk spurious interrupt.
 */
void hal_interrupt_spurious(void)
{
    /* Spurious interrupt, tidak perlu EOI */
    kernel_printf("[HAL] Spurious interrupt terdeteksi\n");
}
