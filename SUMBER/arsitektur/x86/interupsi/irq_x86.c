/*
 * PIGURA OS - IRQ x86
 * --------------------
 * Implementasi penanganan IRQ untuk arsitektur x86.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola IRQ (Interrupt
 * Request) dari PIC (Programmable Interrupt Controller).
 *
 * Arsitektur: x86 (32-bit)
 * Versi: 1.0
 */

#include "../../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA PIC
 * ============================================================================
 */

/* Port PIC */
#define PIC1_CMD            0x20
#define PIC1_DATA           0x21
#define PIC2_CMD            0xA0
#define PIC2_DATA           0xA1

/* Command PIC */
#define PIC_EOI             0x20
#define PIC_ICW1_INIT       0x10
#define PIC_ICW1_ICW4       0x01
#define PIC_ICW4_8086       0x01

/* Vektor IRQ */
#define IRQ_VEKTOR_MULAI    32

/* Jumlah IRQ */
#define IRQ_JUMLAH          16

/*
 * ============================================================================
 * VARIABEL GLOBAL
 * ============================================================================
 */

/* Handler IRQ */
static void (*g_irq_handler[IRQ_JUMLAH])(void) = { NULL };

/* Mask IRQ saat ini */
static tak_bertanda16_t g_irq_mask = 0xFFFB;

/* Counter IRQ */
static tak_bertanda64_t g_irq_counter[IRQ_JUMLAH] = { 0 };

/* Spurious IRQ counter */
static tak_bertanda32_t g_spurious_irq = 0;

/*
 * ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================
 */

/* Forward declarations untuk fungsi yang digunakan sebelum didefinisikan */
static status_t _pic_mask(tak_bertanda32_t irq, tak_bertanda8_t status);

/* Forward declarations untuk fungsi public */
status_t irq_enable(tak_bertanda32_t irq);
status_t irq_disable(tak_bertanda32_t irq);

/*
 * ============================================================================
 * FUNGSI INTERNAL
 * ============================================================================
 */

/*
 * _kirim_eoi
 * ----------
 * Mengirim End of Interrupt ke PIC.
 *
 * Parameter:
 *   irq - Nomor IRQ
 */
static void _kirim_eoi(tak_bertanda32_t irq)
{
    if (irq >= 8) {
        /* Kirim EOI ke slave PIC */
        outb(PIC2_CMD, PIC_EOI);
    }

    /* Selalu kirim EOI ke master PIC */
    outb(PIC1_CMD, PIC_EOI);
}

/*
 * _update_mask
 * ------------
 * Mengupdate mask PIC.
 */
static void _update_mask(void)
{
    outb(PIC1_DATA, (tak_bertanda8_t)(g_irq_mask & 0xFF));
    outb(PIC2_DATA, (tak_bertanda8_t)((g_irq_mask >> 8) & 0xFF));
}

/*
 * ============================================================================
 * FUNGSI PUBLIC
 * ============================================================================
 */

/*
 * irq_init
 * --------
 * Menginisialisasi PIC dan IRQ.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t irq_init(void)
{
    tak_bertanda32_t i;

    kernel_printf("[IRQ] Menginisialisasi PIC...\n");

    /* Reset semua handler */
    for (i = 0; i < IRQ_JUMLAH; i++) {
        g_irq_handler[i] = NULL;
        g_irq_counter[i] = 0;
    }

    /* Remap PIC */
    /* ICW1: Init + ICW4 */
    outb(PIC1_CMD, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_delay();
    outb(PIC2_CMD, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_delay();

    /* ICW2: Vektor offset */
    outb(PIC1_DATA, IRQ_VEKTOR_MULAI);
    io_delay();
    outb(PIC2_DATA, IRQ_VEKTOR_MULAI + 8);
    io_delay();

    /* ICW3: Cascade */
    outb(PIC1_DATA, 0x04);  /* IRQ2 terhubung ke slave */
    io_delay();
    outb(PIC2_DATA, 0x02);  /* Slave ID */
    io_delay();

    /* ICW4: 8086 mode */
    outb(PIC1_DATA, PIC_ICW4_8086);
    io_delay();
    outb(PIC2_DATA, PIC_ICW4_8086);
    io_delay();

    /* Mask semua IRQ kecuali cascade (IRQ2) */
    g_irq_mask = 0xFFFB;  /* 1111 1111 1111 1011 */
    _update_mask();

    kernel_printf("[IRQ] PIC diinisialisasi\n");

    return STATUS_BERHASIL;
}

/*
 * irq_handler_umum
 * ----------------
 * Handler umum untuk IRQ yang dipanggil dari assembly.
 *
 * Parameter:
 *   irq - Nomor IRQ (0-15)
 */
void irq_handler_umum(tak_bertanda32_t irq)
{
    /* Increment counter */
    if (irq < IRQ_JUMLAH) {
        g_irq_counter[irq]++;
    }

    /* Panggil handler jika ada */
    if (irq < IRQ_JUMLAH && g_irq_handler[irq] != NULL) {
        g_irq_handler[irq]();
    }

    /* Kirim EOI */
    _kirim_eoi(irq);
}

/*
 * irq_set_handler
 * ---------------
 * Mengatur handler untuk IRQ tertentu.
 *
 * Parameter:
 *   irq     - Nomor IRQ (0-15)
 *   handler - Pointer ke fungsi handler
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t irq_set_handler(tak_bertanda32_t irq, void (*handler)(void))
{
    if (irq >= IRQ_JUMLAH) {
        return STATUS_PARAM_INVALID;
    }

    g_irq_handler[irq] = handler;

    /* Unmask IRQ */
    irq_enable(irq);

    return STATUS_BERHASIL;
}

/*
 * irq_hapus_handler
 * -----------------
 * Menghapus handler IRQ.
 *
 * Parameter:
 *   irq - Nomor IRQ (0-15)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t irq_hapus_handler(tak_bertanda32_t irq)
{
    if (irq >= IRQ_JUMLAH) {
        return STATUS_PARAM_INVALID;
    }

    /* Mask IRQ */
    irq_disable(irq);

    g_irq_handler[irq] = NULL;

    return STATUS_BERHASIL;
}

/*
 * irq_enable
 * ----------
 * Mengaktifkan IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ (0-15)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t irq_enable(tak_bertanda32_t irq)
{
    if (irq >= IRQ_JUMLAH) {
        return STATUS_PARAM_INVALID;
    }

    /* Clear bit untuk enable */
    g_irq_mask &= ~(1 << irq);
    _update_mask();

    return STATUS_BERHASIL;
}

/*
 * irq_disable
 * -----------
 * Menonaktifkan IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ (0-15)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t irq_disable(tak_bertanda32_t irq)
{
    if (irq >= IRQ_JUMLAH) {
        return STATUS_PARAM_INVALID;
    }

    /* Set bit untuk disable */
    g_irq_mask |= (1 << irq);
    _update_mask();

    return STATUS_BERHASIL;
}

/*
 * irq_mask_semua
 * --------------
 * Mem-mask semua IRQ.
 */
void irq_mask_semua(void)
{
    g_irq_mask = 0xFFFF;
    _update_mask();
}

/*
 * irq_unmask_semua
 * ----------------
 * Meng-unmask semua IRQ.
 */
void irq_unmask_semua(void)
{
    /* Jangan unmask IRQ cascade */
    g_irq_mask = 0xFFFB;
    _update_mask();
}

/*
 * irq_get_counter
 * ---------------
 * Mendapatkan counter IRQ.
 *
 * Parameter:
 *   irq - Nomor IRQ (0-15)
 *
 * Return:
 *   Jumlah interrupt
 */
tak_bertanda64_t irq_get_counter(tak_bertanda32_t irq)
{
    if (irq >= IRQ_JUMLAH) {
        return 0;
    }

    return g_irq_counter[irq];
}

/*
 * irq_get_spurious
 * ----------------
 * Mendapatkan counter spurious IRQ.
 *
 * Return:
 *   Jumlah spurious IRQ
 */
tak_bertanda32_t irq_get_spurious(void)
{
    return g_spurious_irq;
}

/*
 * irq_ack
 * -------
 * Acknowledge IRQ.
 *
 * Parameter:
 *   irq - Nomor IRQ (0-15)
 */
void irq_ack(tak_bertanda32_t irq)
{
    if (irq >= IRQ_JUMLAH) {
        return;
    }

    _kirim_eoi(irq);
}

/*
 * irq_is_masked
 * -------------
 * Cek apakah IRQ di-mask.
 *
 * Parameter:
 *   irq - Nomor IRQ (0-15)
 *
 * Return:
 *   BENAR jika di-mask
 */
bool_t irq_is_masked(tak_bertanda32_t irq)
{
    if (irq >= IRQ_JUMLAH) {
        return BENAR;
    }

    return (g_irq_mask & (1 << irq)) ? BENAR : SALAH;
}

/*
 * irq_dump_status
 * ---------------
 * Menampilkan status IRQ.
 */
void irq_dump_status(void)
{
    tak_bertanda32_t i;

    kernel_printf("[IRQ] Status IRQ:\n");
    kernel_printf("[IRQ]   IRQ  Mask  Handler  Counter\n");

    for (i = 0; i < IRQ_JUMLAH; i++) {
        kernel_printf("[IRQ]   %2d   %s    %s    %u\n",
            i,
            (g_irq_mask & (1 << i)) ? "YA" : "TIDAK",
            (g_irq_handler[i] != NULL) ? "ADA" : "TIDAK",
            (tak_bertanda32_t)g_irq_counter[i]);
    }

    kernel_printf("[IRQ] Spurious: %u\n", g_spurious_irq);
}

