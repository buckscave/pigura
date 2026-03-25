/*
 * PIGURA OS - IRQ x86_64
 * -----------------------
 * Implementasi pengelolaan IRQ untuk arsitektur x86_64.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk manajemen
 * IRQ (Interrupt Request) melalui PIC pada arsitektur x86_64.
 *
 * Arsitektur: x86_64 (64-bit)
 * Versi: 1.0
 */

#include "../../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA PIC
 * ============================================================================
 */

/* Port PIC */
#define PIC1_CMD                0x20
#define PIC1_DATA               0x21
#define PIC2_CMD                0xA0
#define PIC2_DATA               0xA1

/* Command PIC */
#define PIC_EOI                 0x20
#define PIC_ICW1_INIT           0x10
#define PIC_ICW1_ICW4           0x01
#define PIC_ICW4_8086           0x01

/* IRQ vector offset */
#define IRQ_VECTOR_BASE         32

/* Jumlah IRQ PIC */
#define PIC_IRQ_COUNT           16

/*
 * ============================================================================
 * VARIABEL GLOBAL
 * ============================================================================
 */

/* Tabel handler IRQ */
static void (*g_irq_handler[PIC_IRQ_COUNT])(void) = { NULL };

/* Counter IRQ */
static tak_bertanda64_t g_irq_counter[PIC_IRQ_COUNT] = { 0 };

/* Mask IRQ */
static tak_bertanda16_t g_irq_mask = 0xFFFB;

/* Flag inisialisasi */
static bool_t g_irq_diinisialisasi = SALAH;

/*
 * ============================================================================
 * FUNGSI INTERNAL
 * ============================================================================
 */

/*
 * _pic_send_eoi
 * -------------
 * Mengirim End of Interrupt ke PIC.
 *
 * Parameter:
 *   irq - Nomor IRQ
 */
static void _pic_send_eoi(tak_bertanda32_t irq)
{
    if (irq >= 8) {
        outb(PIC2_CMD, PIC_EOI);
    }
    outb(PIC1_CMD, PIC_EOI);
}

/*
 * _pic_remap
 * ----------
 * Remap PIC ke vektor baru.
 *
 * Parameter:
 *   offset1 - Offset untuk master PIC
 *   offset2 - Offset untuk slave PIC
 */
static void _pic_remap(tak_bertanda8_t offset1, tak_bertanda8_t offset2)
{
    /* Init master PIC */
    outb(PIC1_CMD, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_delay();

    /* Init slave PIC */
    outb(PIC2_CMD, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_delay();

    /* Set vector offset */
    outb(PIC1_DATA, offset1);
    io_delay();
    outb(PIC2_DATA, offset2);
    io_delay();

    /* Tell master ada slave di IRQ2 */
    outb(PIC1_DATA, 0x04);
    io_delay();

    /* Tell slave identitasnya */
    outb(PIC2_DATA, 0x02);
    io_delay();

    /* Set 8086 mode */
    outb(PIC1_DATA, PIC_ICW4_8086);
    io_delay();
    outb(PIC2_DATA, PIC_ICW4_8086);
    io_delay();

    /* Set mask */
    outb(PIC1_DATA, g_irq_mask & 0xFF);
    outb(PIC2_DATA, (g_irq_mask >> 8) & 0xFF);
}

/*
 * _pic_update_mask
 * ----------------
 * Update mask register PIC.
 */
static void _pic_update_mask(void)
{
    outb(PIC1_DATA, g_irq_mask & 0xFF);
    outb(PIC2_DATA, (g_irq_mask >> 8) & 0xFF);
}

/*
 * ============================================================================
 * FUNGSI PUBLIC
 * ============================================================================
 */

/* Forward declarations */
status_t irq_x86_64_enable(tak_bertanda32_t irq);
status_t irq_x86_64_disable(tak_bertanda32_t irq);

/*
 * irq_x86_64_init
 * ---------------
 * Inisialisasi subsistem IRQ.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t irq_x86_64_init(void)
{
    kernel_printf("[IRQ-x86_64] Menginisialisasi IRQ...\n");

    /* Clear handler table */
    kernel_memset(&g_irq_handler, 0, sizeof(g_irq_handler));
    kernel_memset(&g_irq_counter, 0, sizeof(g_irq_counter));

    /* Remap PIC */
    _pic_remap(IRQ_VECTOR_BASE, IRQ_VECTOR_BASE + 8);

    g_irq_diinisialisasi = BENAR;

    kernel_printf("[IRQ-x86_64] IRQ siap (IRQ 0-15 -> vektor 32-47)\n");

    return STATUS_BERHASIL;
}

/*
 * irq_x86_64_handler
 * ------------------
 * Handler IRQ umum.
 *
 * Parameter:
 *   irq - Nomor IRQ
 */
void irq_x86_64_handler(tak_bertanda32_t irq)
{
    if (irq >= PIC_IRQ_COUNT) {
        return;
    }

    /* Increment counter */
    g_irq_counter[irq]++;

    /* Panggil handler jika ada */
    if (g_irq_handler[irq] != NULL) {
        g_irq_handler[irq]();
    }

    /* Kirim EOI */
    _pic_send_eoi(irq);
}

/*
 * irq_x86_64_register
 * -------------------
 * Mendaftarkan handler IRQ.
 *
 * Parameter:
 *   irq     - Nomor IRQ (0-15)
 *   handler - Pointer ke fungsi handler
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t irq_x86_64_register(tak_bertanda32_t irq, void (*handler)(void))
{
    if (irq >= PIC_IRQ_COUNT) {
        return STATUS_PARAM_INVALID;
    }

    g_irq_handler[irq] = handler;

    /* Unmask IRQ */
    irq_x86_64_enable(irq);

    return STATUS_BERHASIL;
}

/*
 * irq_x86_64_unregister
 * ---------------------
 * Menghapus handler IRQ.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t irq_x86_64_unregister(tak_bertanda32_t irq)
{
    if (irq >= PIC_IRQ_COUNT) {
        return STATUS_PARAM_INVALID;
    }

    /* Mask IRQ */
    irq_x86_64_disable(irq);

    g_irq_handler[irq] = NULL;

    return STATUS_BERHASIL;
}

/*
 * irq_x86_64_enable
 * -----------------
 * Mengaktifkan IRQ.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t irq_x86_64_enable(tak_bertanda32_t irq)
{
    if (irq >= PIC_IRQ_COUNT) {
        return STATUS_PARAM_INVALID;
    }

    g_irq_mask &= ~(1 << irq);
    _pic_update_mask();

    return STATUS_BERHASIL;
}

/*
 * irq_x86_64_disable
 * ------------------
 * Menonaktifkan IRQ.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t irq_x86_64_disable(tak_bertanda32_t irq)
{
    if (irq >= PIC_IRQ_COUNT) {
        return STATUS_PARAM_INVALID;
    }

    g_irq_mask |= (1 << irq);
    _pic_update_mask();

    return STATUS_BERHASIL;
}

/*
 * irq_x86_64_is_enabled
 * ---------------------
 * Mengecek apakah IRQ aktif.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return:
 *   BENAR jika aktif
 */
bool_t irq_x86_64_is_enabled(tak_bertanda32_t irq)
{
    if (irq >= PIC_IRQ_COUNT) {
        return SALAH;
    }

    return (g_irq_mask & (1 << irq)) ? SALAH : BENAR;
}

/*
 * irq_x86_64_get_counter
 * ----------------------
 * Mendapatkan counter IRQ.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return:
 *   Jumlah IRQ, atau 0 jika tidak valid
 */
tak_bertanda64_t irq_x86_64_get_counter(tak_bertanda32_t irq)
{
    if (irq >= PIC_IRQ_COUNT) {
        return 0;
    }

    return g_irq_counter[irq];
}

/*
 * irq_x86_64_mask_all
 * -------------------
 * Mask semua IRQ.
 */
void irq_x86_64_mask_all(void)
{
    g_irq_mask = 0xFFFF;
    _pic_update_mask();
}

/*
 * irq_x86_64_unmask_all
 * ---------------------
 * Unmask semua IRQ.
 */
void irq_x86_64_unmask_all(void)
{
    g_irq_mask = 0xFFFB;  /* IRQ2 adalah cascade */
    _pic_update_mask();
}

/*
 * irq_x86_64_apakah_siap
 * ----------------------
 * Mengecek apakah IRQ sudah diinisialisasi.
 *
 * Return:
 *   BENAR jika sudah, SALAH jika belum
 */
bool_t irq_x86_64_apakah_siap(void)
{
    return g_irq_diinisialisasi;
}
