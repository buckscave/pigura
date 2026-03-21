/*
 * =============================================================================
 * PIGURA OS - IRQ HANDLER (x86)
 * =============================================================================
 *
 * Berkas ini berisi implementasi penanganan IRQ (Interrupt Request) untuk
 * arsitektur x86. IRQ adalah interrupt dari perangkat hardware yang
 * dikirim melalui PIC (Programmable Interrupt Controller).
 *
 * PIC Master (IRQ 0-7)  - Base: 0x20, 0x21
 * PIC Slave  (IRQ 8-15) - Base: 0xA0, 0xA1
 *
 * Arsitektur: x86 (32-bit protected mode)
 * Versi: 1.0
 * =============================================================================
 */

#include <stdint.h>
#include <stddef.h>

/* =============================================================================
 * KONSTANTA PIC (Programmable Interrupt Controller)
 * =============================================================================
 */

/* Port I/O PIC Master */
#define PIC_MASTER_CMD      0x20        /* Command port */
#define PIC_MASTER_DATA     0x21        /* Data port */

/* Port I/O PIC Slave */
#define PIC_SLAVE_CMD       0xA0        /* Command port */
#define PIC_SLAVE_DATA      0xA1        /* Data port */

/* Perintah PIC */
#define PIC_CMD_EOI         0x20        /* End of Interrupt */
#define PIC_CMD_READ_ISR    0x0B        /* Read In-Service Register */
#define PIC_CMD_READ_IRR    0x0A        /* Read Interrupt Request Register */

/* ICW (Initialization Command Words) */
#define PIC_ICW1_INIT       0x10        /* Initialize */
#define PIC_ICW1_ICW4       0x01        /* ICW4 needed */
#define PIC_ICW4_8086       0x01        /* 8086 mode */

/* Offset IRQ di IDT */
#define PIC_OFFSET_MASTER   32          /* IRQ 0-7  -> IDT 32-39 */
#define PIC_OFFSET_SLAVE    40          /* IRQ 8-15 -> IDT 40-47 */

/* Jumlah IRQ */
#define IRQ_JUMLAH          16

/* =============================================================================
 * KONSTANTA IRQ
 * =============================================================================
 */

/* Nomor IRQ */
#define IRQ_TIMER           0           /* System timer */
#define IRQ_KEYBOARD        1           /* Keyboard */
#define IRQ_CASCADE         2           /* Cascade untuk slave PIC */
#define IRQ_COM2            3           /* Serial port COM2 */
#define IRQ_COM1            4           /* Serial port COM1 */
#define IRQ_LPT2            5           /* Parallel port LPT2 */
#define IRQ_FLOPPY          6           /* Floppy disk */
#define IRQ_LPT1            7           /* Parallel port LPT1 */
#define IRQ_RTC             8           /* Real-time clock */
#define IRQ_ACPI            9           /* ACPI */
#define IRQ_FREE1           10          /* Tersedia */
#define IRQ_FREE2           11          /* Tersedia */
#define IRQ_MOUSE           12          /* PS/2 mouse */
#define IRQ_FPU             13          /* FPU exception */
#define IRQ_ATA_PRIMARY     14          /* Primary ATA hard disk */
#define IRQ_ATA_SECONDARY   15          /* Secondary ATA hard disk */

/* =============================================================================
 * STRUKTUR DATA
 * =============================================================================
 */

/* Tipe handler IRQ */
typedef void (*handler_irq_t)(struct int_frame *frame);

/* Import dari isr_x86.S */
struct int_frame {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp_kolon, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, esp, ss;
};

/* =============================================================================
 * VARIABEL GLOBAL
 * =============================================================================
 */

/* Tabel handler IRQ */
static handler_irq_t g_handler_irq[IRQ_JUMLAH];

/* Mask IRQ saat ini */
static uint16_t g_mask_irq = 0xFFFF;    /* Semua IRQ di-mask awalnya */

/* =============================================================================
 * FUNGSI INTERNAL
 * =============================================================================
 */

/*
 * _outb
 * ------
 * Menulis byte ke port I/O.
 *
 * Parameter:
 *   nilai - Nilai yang ditulis
 *   port  - Nomor port
 */
static inline void _outb(uint8_t nilai, uint16_t port)
{
    __asm__ __volatile__(
        "outb %0, %1\n\t"
        :
        : "a"(nilai), "Nd"(port)
    );
}

/*
 * _inb
 * -----
 * Membaca byte dari port I/O.
 *
 * Parameter:
 *   port - Nomor port
 *
 * Return:
 *   Nilai yang dibaca
 */
static inline uint8_t _inb(uint16_t port)
{
    uint8_t nilai;
    __asm__ __volatile__(
        "inb %1, %0\n\t"
        : "=a"(nilai)
        : "Nd"(port)
    );
    return nilai;
}

/*
 * _io_delay
 * ---------
 * Delay singkat menggunakan I/O ke port dummy.
 */
static inline void _io_delay(void)
{
    _outb(0, 0x80);                      /* Port 0x80 adalah unused */
}

/*
 * _pic_kirim_eoi
 * --------------
 * Mengirim End of Interrupt ke PIC.
 *
 * Parameter:
 *   irq - Nomor IRQ yang selesai
 */
static void _pic_kirim_eoi(uint8_t irq)
{
    /* Kirim EOI ke master PIC */
    _outb(PIC_CMD_EOI, PIC_MASTER_CMD);

    /* Jika IRQ dari slave (IRQ 8-15), kirim EOI ke slave juga */
    if (irq >= 8) {
        _outb(PIC_CMD_EOI, PIC_SLAVE_CMD);
    }
}

/*
 * _pic_set_mask
 * -------------
 * Mengatur mask IRQ.
 *
 * Parameter:
 *   irq   - Nomor IRQ
 *   mask  - 1 untuk mask (disable), 0 untuk unmask (enable)
 */
static void _pic_set_mask(uint8_t irq, uint8_t mask)
{
    uint16_t port;
    uint8_t nilai;

    if (irq >= IRQ_JUMLAH) {
        return;
    }

    /* Update mask cache */
    if (mask) {
        g_mask_irq |= (1 << irq);
    } else {
        g_mask_irq &= ~(1 << irq);
    }

    /* Pilih port */
    port = (irq < 8) ? PIC_MASTER_DATA : PIC_SLAVE_DATA;

    /* Baca nilai saat ini */
    nilai = (irq < 8) ? (g_mask_irq & 0xFF) : ((g_mask_irq >> 8) & 0xFF);

    /* Tulis nilai baru */
    _outb(nilai, port);
}

/*
 * _pic_get_mask
 * -------------
 * Mendapatkan status mask IRQ.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return:
 *   1 jika IRQ di-mask, 0 jika tidak
 */
static uint8_t _pic_get_mask(uint8_t irq)
{
    if (irq >= IRQ_JUMLAH) {
        return 1;
    }

    return (g_mask_irq >> irq) & 1;
}

/* =============================================================================
 * FUNGSI PUBLIC
 * =============================================================================
 */

/*
 * pic_init
 * --------
 * Menginisialisasi PIC (Programmable Interrupt Controller).
 *
 * Fungsi ini melakukan remapping IRQ ke vektor 32-47 agar tidak
 * bertabrakan dengan CPU exceptions (0-31).
 *
 * Return:
 *   0 jika berhasil
 */
int pic_init(void)
{
    /* Simpan mask saat ini */
    uint8_t mask1 = _inb(PIC_MASTER_DATA);
    uint8_t mask2 = _inb(PIC_SLAVE_DATA);

    /* ICW1: Start initialization */
    _outb(PIC_ICW1_INIT | PIC_ICW1_ICW4, PIC_MASTER_CMD);
    _io_delay();
    _outb(PIC_ICW1_INIT | PIC_ICW1_ICW4, PIC_SLAVE_CMD);
    _io_delay();

    /* ICW2: Set offset vektor */
    _outb(PIC_OFFSET_MASTER, PIC_MASTER_DATA);
    _io_delay();
    _outb(PIC_OFFSET_SLAVE, PIC_SLAVE_DATA);
    _io_delay();

    /* ICW3: Setup cascade */
    _outb(0x04, PIC_MASTER_DATA);       /* Master: IRQ2 terhubung ke slave */
    _io_delay();
    _outb(0x02, PIC_SLAVE_DATA);        /* Slave: Cascade identity */
    _io_delay();

    /* ICW4: Mode 8086 */
    _outb(PIC_ICW4_8086, PIC_MASTER_DATA);
    _io_delay();
    _outb(PIC_ICW4_8086, PIC_SLAVE_DATA);
    _io_delay();

    /* Kembalikan mask */
    _outb(mask1, PIC_MASTER_DATA);
    _outb(mask2, PIC_SLAVE_DATA);

    /* Update mask cache */
    g_mask_irq = mask1 | (mask2 << 8);

    /* Enable cascade IRQ (IRQ 2) untuk komunikasi slave */
    _pic_set_mask(IRQ_CASCADE, 0);

    return 0;
}

/*
 * pic_enable_irq
 * --------------
 * Mengaktifkan (unmask) IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ (0-15)
 *
 * Return:
 *   0 jika berhasil, -1 jika gagal
 */
int pic_enable_irq(uint8_t irq)
{
    if (irq >= IRQ_JUMLAH) {
        return -1;
    }

    _pic_set_mask(irq, 0);
    return 0;
}

/*
 * pic_disable_irq
 * ---------------
 * Menonaktifkan (mask) IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ (0-15)
 *
 * Return:
 *   0 jika berhasil, -1 jika gagal
 */
int pic_disable_irq(uint8_t irq)
{
    if (irq >= IRQ_JUMLAH) {
        return -1;
    }

    _pic_set_mask(irq, 1);
    return 0;
}

/*
 * pic_get_irq_mask
 * ----------------
 * Mendapatkan mask IRQ saat ini.
 *
 * Return:
 *   16-bit mask (bit n = 1 berarti IRQ n di-mask)
 */
uint16_t pic_get_irq_mask(void)
{
    return g_mask_irq;
}

/*
 * pic_set_irq_mask
 * ----------------
 * Mengatur mask IRQ secara langsung.
 *
 * Parameter:
 *   mask - 16-bit mask baru
 */
void pic_set_irq_mask(uint16_t mask)
{
    g_mask_irq = mask;
    _outb(mask & 0xFF, PIC_MASTER_DATA);
    _outb((mask >> 8) & 0xFF, PIC_SLAVE_DATA);
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
 *   0 jika berhasil, -1 jika gagal
 */
int irq_set_handler(uint8_t irq, handler_irq_t handler)
{
    if (irq >= IRQ_JUMLAH) {
        return -1;
    }

    g_handler_irq[irq] = handler;
    return 0;
}

/*
 * irq_get_handler
 * ---------------
 * Mendapatkan handler untuk IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ (0-15)
 *
 * Return:
 *   Pointer ke handler, atau NULL jika tidak ada
 */
handler_irq_t irq_get_handler(uint8_t irq)
{
    if (irq >= IRQ_JUMLAH) {
        return (handler_irq_t)0;
    }

    return g_handler_irq[irq];
}

/*
 * irq_handler_umum
 * ----------------
 * Handler umum untuk semua IRQ. Dipanggil dari ISR assembly.
 *
 * Parameter:
 *   frame - Pointer ke stack frame
 */
void irq_handler_umum(struct int_frame *frame)
{
    uint8_t irq;
    handler_irq_t handler;

    /* Hitung nomor IRQ dari vektor interrupt */
    irq = (uint8_t)(frame->int_no - PIC_OFFSET_MASTER);

    /* Validasi IRQ */
    if (irq >= IRQ_JUMLAH) {
        /* Invalid IRQ, kirim EOI dan return */
        _pic_kirim_eoi(0);
        return;
    }

    /* Dapatkan handler */
    handler = g_handler_irq[irq];

    /* Panggil handler jika ada */
    if (handler != (handler_irq_t)0) {
        handler(frame);
    }

    /* Kirim EOI ke PIC */
    _pic_kirim_eoi(irq);
}

/*
 * pic_spurious_irq
 * ----------------
 * Menangani spurious IRQ (IRQ palsu dari PIC).
 *
 * Return:
 *   1 jika IRQ spurious dari master, 2 jika dari slave, 0 jika bukan spurious
 */
int pic_spurious_irq(void)
{
    uint8_t isr_master;
    uint8_t isr_slave;

    /* Baca In-Service Register */
    _outb(PIC_CMD_READ_ISR, PIC_MASTER_CMD);
    isr_master = _inb(PIC_MASTER_CMD);

    _outb(PIC_CMD_READ_ISR, PIC_SLAVE_CMD);
    isr_slave = _inb(PIC_SLAVE_CMD);

    /* Cek IRQ 7 di master */
    if ((isr_master & 0x80) == 0) {
        /* Spurious IRQ dari master */
        return 1;
    }

    /* Cek IRQ 15 di slave */
    if ((isr_slave & 0x80) == 0) {
        /* Spurious IRQ dari slave */
        return 2;
    }

    return 0;
}

/*
 * irq_disable_semua
 * -----------------
 * Menonaktifkan semua IRQ.
 */
void irq_disable_semua(void)
{
    pic_set_irq_mask(0xFFFF);
}

/*
 * irq_enable_semua
 * ----------------
 * Mengaktifkan semua IRQ (kecuali cascade).
 */
void irq_enable_semua(void)
{
    /* Cascade (IRQ 2) harus tetap aktif untuk slave PIC */
    pic_set_irq_mask(~(1 << IRQ_CASCADE));
}
