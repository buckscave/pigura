/*
 * PIGURA OS - PIC.C
 * -----------------
 * Implementasi driver PIC 8259 (Programmable Interrupt Controller).
 *
 * Berkas ini berisi implementasi lengkap driver untuk PIC 8259
 * yang merupakan interrupt controller standar pada PC compatible.
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

/* PIC port addresses */
#define PIC1_CMD                0x20    /* Master PIC command */
#define PIC1_DATA               0x21    /* Master PIC data */
#define PIC2_CMD                0xA0    /* Slave PIC command */
#define PIC2_DATA               0xA1    /* Slave PIC data */

/* PIC command words - ICW1 (Initialization Command Word 1) */
#define ICW1_ICW4               0x01    /* ICW4 needed */
#define ICW1_SINGLE             0x02    /* Single (cascade) mode */
#define ICW1_INTERVAL4          0x04    /* Call address interval 4 (8) */
#define ICW1_LEVEL              0x08    /* Level triggered (edge) mode */
#define ICW1_INIT               0x10    /* Initialization - required! */

/* PIC command words - ICW4 */
#define ICW4_8086               0x01    /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO               0x02    /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE          0x08    /* Buffered mode/slave */
#define ICW4_BUF_MASTER         0x0C    /* Buffered mode/master */
#define ICW4_SFNM               0x10    /* Special fully nested (not) */

/* PIC Operation Command Words - OCW */
#define OCW2_EOI                0x20    /* End of interrupt */
#define OCW2_EOI_SPECIFIC       0x60    /* Specific EOI */
#define OCW2_ROTATE_AUTO        0x80    /* Rotate in auto EOI mode */
#define OCW2_ROTATE_NON_SPEC    0xA0    /* Rotate on non-specific EOI */
#define OCW2_ROTATE_SPECIFIC    0xE0    /* Rotate on specific EOI */

#define OCW3_READ_IRR           0x0A    /* Read Interrupt Request Register */
#define OCW3_READ_ISR           0x0B    /* Read In-Service Register */
#define OCW3_POLL               0x0C    /* Poll mode */

/* IRQ remap offsets */
#define PIC_REMAP_OFFSET1       32      /* Master PIC IRQ base */
#define PIC_REMAP_OFFSET2       40      /* Slave PIC IRQ base */

/* Special IRQ lines */
#define IRQ_CASCADE             2       /* Cascade line from slave to master */

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* PIC state */
static struct {
    bool_t initialized;
    tak_bertanda8_t master_mask;     /* Current master PIC mask */
    tak_bertanda8_t slave_mask;      /* Current slave PIC mask */
    tak_bertanda8_t vector_base1;    /* Master PIC vector base */
    tak_bertanda8_t vector_base2;    /* Slave PIC vector base */
} pic_state = { SALAH, 0xFF, 0xFF, PIC_REMAP_OFFSET1, PIC_REMAP_OFFSET2 };

/* Saved masks for nested masking */
static tak_bertanda8_t saved_master_mask = 0xFF;
static tak_bertanda8_t saved_slave_mask = 0xFF;

/* Spurious interrupt count */
static tak_bertanda32_t spurious_irq7_count = 0;
static tak_bertanda32_t spurious_irq15_count = 0;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * pic_wait
 * --------
 * Delay untuk operasi PIC.
 * PIC memerlukan delay antara I/O operations.
 */
static inline void pic_wait(void)
{
    io_delay();
    io_delay();
}

/*
 * pic_read_irr_master
 * -------------------
 * Baca Interrupt Request Register master PIC.
 *
 * Return: Nilai IRR
 */
static inline tak_bertanda8_t pic_read_irr_master(void)
{
    outb(PIC1_CMD, OCW3_READ_IRR);
    return inb(PIC1_CMD);
}

/*
 * pic_read_irr_slave
 * ------------------
 * Baca Interrupt Request Register slave PIC.
 *
 * Return: Nilai IRR
 */
static inline tak_bertanda8_t pic_read_irr_slave(void)
{
    outb(PIC2_CMD, OCW3_READ_IRR);
    return inb(PIC2_CMD);
}

/*
 * pic_read_isr_master
 * -------------------
 * Baca In-Service Register master PIC.
 *
 * Return: Nilai ISR
 */
static inline tak_bertanda8_t pic_read_isr_master(void)
{
    outb(PIC1_CMD, OCW3_READ_ISR);
    return inb(PIC1_CMD);
}

/*
 * pic_read_isr_slave
 * ------------------
 * Baca In-Service Register slave PIC.
 *
 * Return: Nilai ISR
 */
static inline tak_bertanda8_t pic_read_isr_slave(void)
{
    outb(PIC2_CMD, OCW3_READ_ISR);
    return inb(PIC2_CMD);
}

/*
 * pic_is_spurious
 * ---------------
 * Cek apakah IRQ adalah spurious interrupt.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: BENAR jika spurious
 */
static bool_t pic_is_spurious(tak_bertanda32_t irq)
{
    tak_bertanda8_t isr;

    if (irq == 7) {
        /* IRQ 7 bisa spurious pada master PIC */
        isr = pic_read_isr_master();
        if (!(isr & (1 << 7))) {
            spurious_irq7_count++;
            return BENAR;
        }
    } else if (irq == 15) {
        /* IRQ 15 bisa spurious pada slave PIC */
        isr = pic_read_isr_slave();
        if (!(isr & (1 << 7))) {
            spurious_irq15_count++;
            return BENAR;
        }
    }

    return SALAH;
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * pic_init
 * --------
 * Inisialisasi PIC 8259.
 *
 * Return: Status operasi
 */
status_t pic_init(void)
{
    if (pic_state.initialized) {
        return STATUS_SUDAH_ADA;
    }

    /* Simpan mask lama */
    pic_state.master_mask = inb(PIC1_DATA);
    pic_state.slave_mask = inb(PIC2_DATA);

    /* Mulai inisialisasi - ICW1 */
    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    pic_wait();
    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
    pic_wait();

    /* ICW2 - Set vector offsets */
    outb(PIC1_DATA, pic_state.vector_base1);  /* Master PIC vector offset */
    pic_wait();
    outb(PIC2_DATA, pic_state.vector_base2);  /* Slave PIC vector offset */
    pic_wait();

    /* ICW3 - Tell Master PIC there's a slave PIC at IRQ2 */
    outb(PIC1_DATA, 0x04);  /* Bit 2 set = slave at IRQ2 */
    pic_wait();
    /* Tell Slave PIC its cascade identity */
    outb(PIC2_DATA, 0x02);  /* Slave ID = 2 */
    pic_wait();

    /* ICW4 - Set 8086 mode */
    outb(PIC1_DATA, ICW4_8086);
    pic_wait();
    outb(PIC2_DATA, ICW4_8086);
    pic_wait();

    /* Restore saved masks (disable semua IRQ) */
    pic_state.master_mask = 0xFF;  /* Semua IRQ disabled */
    pic_state.slave_mask = 0xFF;
    outb(PIC1_DATA, pic_state.master_mask);
    outb(PIC2_DATA, pic_state.slave_mask);

    pic_state.initialized = BENAR;

    kernel_printf("[PIC] 8259 PIC initialized\n");
    kernel_printf("      Master vectors: %u-%u\n",
                  pic_state.vector_base1,
                  pic_state.vector_base1 + 7);
    kernel_printf("      Slave vectors:  %u-%u\n",
                  pic_state.vector_base2,
                  pic_state.vector_base2 + 7);

    return STATUS_BERHASIL;
}

/*
 * pic_remap
 * ---------
 * Remap PIC ke vector offset baru.
 *
 * Parameter:
 *   offset1 - Vector offset untuk master PIC
 *   offset2 - Vector offset untuk slave PIC
 *
 * Return: Status operasi
 */
status_t pic_remap(tak_bertanda8_t offset1, tak_bertanda8_t offset2)
{
    if (!pic_state.initialized) {
        return STATUS_GAGAL;
    }

    /* Simpan mask */
    tak_bertanda8_t mask1 = inb(PIC1_DATA);
    tak_bertanda8_t mask2 = inb(PIC2_DATA);

    /* Mulai inisialisasi ulang */
    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    pic_wait();
    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
    pic_wait();

    /* Set vector offsets baru */
    outb(PIC1_DATA, offset1);
    pic_wait();
    outb(PIC2_DATA, offset2);
    pic_wait();

    /* Setup cascade */
    outb(PIC1_DATA, 0x04);
    pic_wait();
    outb(PIC2_DATA, 0x02);
    pic_wait();

    /* Set 8086 mode */
    outb(PIC1_DATA, ICW4_8086);
    pic_wait();
    outb(PIC2_DATA, ICW4_8086);
    pic_wait();

    /* Restore mask */
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);

    pic_state.vector_base1 = offset1;
    pic_state.vector_base2 = offset2;

    return STATUS_BERHASIL;
}

/*
 * pic_send_eoi
 * ------------
 * Kirim End of Interrupt ke PIC.
 *
 * Parameter:
 *   irq - Nomor IRQ yang selesai
 */
void pic_send_eoi(tak_bertanda32_t irq)
{
    /* Cek spurious interrupt */
    if (pic_is_spurious(irq)) {
        return;  /* Tidak perlu EOI untuk spurious */
    }

    if (irq >= 8) {
        /* IRQ dari slave PIC */
        outb(PIC2_CMD, OCW2_EOI);
    }

    /* Selalu kirim EOI ke master (kecuali untuk IRQ 0-7 spurious) */
    outb(PIC1_CMD, OCW2_EOI);
}

/*
 * pic_send_eoi_specific
 * ---------------------
 * Kirim Specific End of Interrupt.
 *
 * Parameter:
 *   irq - Nomor IRQ
 */
void pic_send_eoi_specific(tak_bertanda32_t irq)
{
    if (irq >= 8) {
        outb(PIC2_CMD, OCW2_EOI_SPECIFIC | (irq - 8));
    }
    outb(PIC1_CMD, OCW2_EOI_SPECIFIC | (irq < 8 ? irq : 2));
}

/*
 * pic_enable_irq
 * --------------
 * Aktifkan IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ (0-15)
 *
 * Return: Status operasi
 */
status_t pic_enable_irq(tak_bertanda32_t irq)
{
    tak_bertanda16_t port;
    tak_bertanda8_t mask;

    if (irq > 15) {
        return STATUS_PARAM_INVALID;
    }

    if (irq < 8) {
        port = PIC1_DATA;
        mask = pic_state.master_mask &= ~(1 << irq);
        pic_state.master_mask = (tak_bertanda8_t)mask;
    } else {
        port = PIC2_DATA;
        mask = pic_state.slave_mask &= ~(1 << (irq - 8));
        pic_state.slave_mask = (tak_bertanda8_t)mask;
    }

    outb(port, mask);

    return STATUS_BERHASIL;
}

/*
 * pic_disable_irq
 * ---------------
 * Nonaktifkan IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ (0-15)
 *
 * Return: Status operasi
 */
status_t pic_disable_irq(tak_bertanda32_t irq)
{
    tak_bertanda16_t port;
    tak_bertanda8_t mask;

    if (irq > 15) {
        return STATUS_PARAM_INVALID;
    }

    if (irq < 8) {
        port = PIC1_DATA;
        mask = pic_state.master_mask |= (1 << irq);
        pic_state.master_mask = (tak_bertanda8_t)mask;
    } else {
        port = PIC2_DATA;
        mask = pic_state.slave_mask |= (1 << (irq - 8));
        pic_state.slave_mask = (tak_bertanda8_t)mask;
    }

    outb(port, mask);

    return STATUS_BERHASIL;
}

/*
 * pic_set_mask
 * ------------
 * Set mask IRQ secara langsung.
 *
 * Parameter:
 *   master_mask - Mask untuk master PIC
 *   slave_mask  - Mask untuk slave PIC
 */
void pic_set_mask(tak_bertanda8_t master_mask, tak_bertanda8_t slave_mask)
{
    pic_state.master_mask = master_mask;
    pic_state.slave_mask = slave_mask;

    outb(PIC1_DATA, master_mask);
    outb(PIC2_DATA, slave_mask);
}

/*
 * pic_get_mask
 * ------------
 * Dapatkan mask IRQ saat ini.
 *
 * Parameter:
 *   master_mask - Pointer untuk mask master
 *   slave_mask  - Pointer untuk mask slave
 */
void pic_get_mask(tak_bertanda8_t *master_mask, tak_bertanda8_t *slave_mask)
{
    if (master_mask != NULL) {
        *master_mask = pic_state.master_mask;
    }

    if (slave_mask != NULL) {
        *slave_mask = pic_state.slave_mask;
    }
}

/*
 * pic_disable_all
 * ---------------
 * Nonaktifkan semua IRQ.
 */
void pic_disable_all(void)
{
    pic_state.master_mask = 0xFF;
    pic_state.slave_mask = 0xFF;

    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

/*
 * pic_save_mask
 * -------------
 * Simpan mask saat ini.
 */
void pic_save_mask(void)
{
    saved_master_mask = pic_state.master_mask;
    saved_slave_mask = pic_state.slave_mask;
}

/*
 * pic_restore_mask
 * ----------------
 * Restore mask yang disimpan.
 */
void pic_restore_mask(void)
{
    pic_state.master_mask = saved_master_mask;
    pic_state.slave_mask = saved_slave_mask;

    outb(PIC1_DATA, saved_master_mask);
    outb(PIC2_DATA, saved_slave_mask);
}

/*
 * pic_get_irr
 * -----------
 * Baca Interrupt Request Register gabungan.
 *
 * Return: Nilai IRR (16-bit)
 */
tak_bertanda16_t pic_get_irr(void)
{
    tak_bertanda16_t irr;

    irr = pic_read_irr_slave();
    irr <<= 8;
    irr |= pic_read_irr_master();

    return irr;
}

/*
 * pic_get_isr
 * -----------
 * Baca In-Service Register gabungan.
 *
 * Return: Nilai ISR (16-bit)
 */
tak_bertanda16_t pic_get_isr(void)
{
    tak_bertanda16_t isr;

    isr = pic_read_isr_slave();
    isr <<= 8;
    isr |= pic_read_isr_master();

    return isr;
}

/*
 * pic_get_highest_irq
 * -------------------
 * Dapatkan IRQ dengan prioritas tertinggi yang pending.
 *
 * Return: Nomor IRQ, atau -1 jika tidak ada
 */
tanda32_t pic_get_highest_irq(void)
{
    tak_bertanda8_t irr;
    tak_bertanda32_t i;

    /* Cek master PIC dulu */
    irr = pic_read_irr_master();
    for (i = 0; i < 8; i++) {
        if (irr & (1 << i)) {
            return (tanda32_t)i;
        }
    }

    /* Cek slave PIC */
    irr = pic_read_irr_slave();
    for (i = 0; i < 8; i++) {
        if (irr & (1 << i)) {
            return (tanda32_t)(i + 8);
        }
    }

    return -1;
}

/*
 * pic_is_irq_enabled
 * ------------------
 * Cek apakah IRQ diaktifkan.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: BENAR jika enabled
 */
bool_t pic_is_irq_enabled(tak_bertanda32_t irq)
{
    if (irq > 15) {
        return SALAH;
    }

    if (irq < 8) {
        return !(pic_state.master_mask & (1 << irq));
    } else {
        return !(pic_state.slave_mask & (1 << (irq - 8)));
    }
}

/*
 * pic_get_spurious_count
 * ----------------------
 * Dapatkan jumlah spurious interrupt.
 *
 * Parameter:
 *   irq7_count  - Pointer untuk IRQ 7 count
 *   irq15_count - Pointer untuk IRQ 15 count
 */
void pic_get_spurious_count(tak_bertanda32_t *irq7_count,
                            tak_bertanda32_t *irq15_count)
{
    if (irq7_count != NULL) {
        *irq7_count = spurious_irq7_count;
    }

    if (irq15_count != NULL) {
        *irq15_count = spurious_irq15_count;
    }
}

/*
 * pic_print_status
 * ----------------
 * Print status PIC.
 */
void pic_print_status(void)
{
    tak_bertanda16_t irr;
    tak_bertanda16_t isr;
    tak_bertanda32_t i;

    irr = pic_get_irr();
    isr = pic_get_isr();

    kernel_printf("\n[PIC] 8259 PIC Status:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Master PIC mask: 0x%02X\n", pic_state.master_mask);
    kernel_printf("  Slave PIC mask:  0x%02X\n", pic_state.slave_mask);
    kernel_printf("  IRR: 0x%04X\n", irr);
    kernel_printf("  ISR: 0x%04X\n", isr);
    kernel_printf("----------------------------------------\n");
    kernel_printf("  IRQ Status:\n");

    for (i = 0; i < 16; i++) {
        const char *status;

        if (isr & (1 << i)) {
            status = "IN-SERVICE";
        } else if (irr & (1 << i)) {
            status = "PENDING";
        } else if (pic_is_irq_enabled(i)) {
            status = "Enabled";
        } else {
            status = "Disabled";
        }

        kernel_printf("    IRQ %2lu: %s\n", i, status);
    }

    kernel_printf("----------------------------------------\n");
    kernel_printf("  Spurious IRQ 7:  %lu\n", spurious_irq7_count);
    kernel_printf("  Spurious IRQ 15: %lu\n", spurious_irq15_count);
    kernel_printf("========================================\n");
}

/*
 * pic_is_initialized
 * ------------------
 * Cek apakah PIC sudah diinisialisasi.
 *
 * Return: BENAR jika sudah
 */
bool_t pic_is_initialized(void)
{
    return pic_state.initialized;
}
