/*
 * PIGURA OS - INTERUPSI x86
 * -------------------------
 * Implementasi subsistem interupsi untuk arsitektur x86.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk manajemen
 * interupsi pada arsitektur x86, termasuk setup IDT, handler
 * exception, dan pengelolaan IRQ melalui PIC.
 *
 * Arsitektur: x86 (32-bit)
 * Versi: 1.0
 */

#include "../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA INTERUPSI
 * ============================================================================
 */

/* Jumlah total vektor interupsi */
#define JUMLAH_VEKTOR           256

/* Jumlah vektor exception */
#define JUMLAH_EXCEPTION        32

/* Jumlah IRQ */
#define JUMLAH_IRQ_PIC          16

/* Offset vektor IRQ */
#define OFFSET_IRQ              32

/* Flag IDT */
#define IDT_FLAG_PRESENT        0x80
#define IDT_FLAG_DPL_0          0x00
#define IDT_FLAG_DPL_3          0x60
#define IDT_FLAG_32BIT          0x0E00
#define IDT_FLAG_TRAP           0x0F00

/*
 * ============================================================================
 * STRUKTUR DATA
 * ============================================================================
 */

/* Entry IDT (Interrupt Descriptor Table) */
struct idt_entry {
    tak_bertanda16_t offset_bawah;  /* Offset 15:0 */
    tak_bertanda16_t selector;      /* Segment selector */
    tak_bertanda8_t  reserv;        /* Reserved, harus 0 */
    tak_bertanda8_t  flags;         /* Flag tipe dan DPL */
    tak_bertanda16_t offset_atas;   /* Offset 31:16 */
} __attribute__((packed));

/* Register IDT */
struct idt_ptr {
    tak_bertanda16_t batas;         /* Batas (ukuran - 1) */
    tak_bertanda32_t alamat;        /* Alamat base IDT */
} __attribute__((packed));

/* Stack frame interupsi */
struct interupsi_frame {
    tak_bertanda32_t ds;            /* Data segment */
    tak_bertanda32_t edi;           /* Register edi */
    tak_bertanda32_t esi;           /* Register esi */
    tak_bertanda32_t ebp;           /* Register ebp */
    tak_bertanda32_t esp;           /* Register esp (unused) */
    tak_bertanda32_t ebx;           /* Register ebx */
    tak_bertanda32_t edx;           /* Register edx */
    tak_bertanda32_t ecx;           /* Register ecx */
    tak_bertanda32_t eax;           /* Register eax */
    tak_bertanda32_t nomor_int;     /* Nomor interupsi */
    tak_bertanda32_t kode_error;    /* Kode error */
    tak_bertanda32_t eip;           /* Instruction pointer */
    tak_bertanda32_t cs;            /* Code segment */
    tak_bertanda32_t eflags;        /* Flags */
    tak_bertanda32_t esp_user;      /* User stack pointer */
    tak_bertanda32_t ss;            /* Stack segment */
} __attribute__((packed));

/* Tipe handler interupsi internal (berbeda dari types.h) */
typedef void (*handler_interupsi_frame_t)(struct interupsi_frame *);

/* Nama exception */
struct nama_exception {
    tak_bertanda32_t nomor;
    const char *nama;
    const char *pesan;
};

/*
 * ============================================================================
 * VARIABEL GLOBAL
 * ============================================================================
 */

/* IDT */
static struct idt_entry g_idt[JUMLAH_VEKTOR]
    __attribute__((aligned(8)));

/* Pointer IDT */
static struct idt_ptr g_idt_ptr;

/* Tabel handler interupsi */
static handler_interupsi_frame_t g_handler_interupsi[JUMLAH_VEKTOR];

/* Tabel handler IRQ */
static void (*g_handler_irq[JUMLAH_IRQ_PIC])(void);

/* Counter interupsi */
static tak_bertanda64_t g_interupsi_counter[JUMLAH_VEKTOR];

/* Flag inisialisasi */
static bool_t g_interupsi_diinisialisasi = SALAH;

/* Daftar nama exception */
static const struct nama_exception g_nama_exception[] = {
    {0,  "DE", "Divide Error"},
    {1,  "DB", "Debug Exception"},
    {2,  "NMI", "Non-Maskable Interrupt"},
    {3,  "BP", "Breakpoint"},
    {4,  "OF", "Overflow"},
    {5,  "BR", "BOUND Range Exceeded"},
    {6,  "UD", "Invalid Opcode"},
    {7,  "NM", "Device Not Available"},
    {8,  "DF", "Double Fault"},
    {9,  "MF", "Coprocessor Segment Overrun"},
    {10, "TS", "Invalid TSS"},
    {11, "NP", "Segment Not Present"},
    {12, "SS", "Stack-Segment Fault"},
    {13, "GP", "General Protection"},
    {14, "PF", "Page Fault"},
    {15, "??", "Reserved"},
    {16, "MF", "x87 FPU Error"},
    {17, "AC", "Alignment Check"},
    {18, "MC", "Machine Check"},
    {19, "XM", "SIMD Exception"},
    {20, "VE", "Virtualization Exception"},
    {30, "SX", "Security Exception"}
};

/* Jumlah entri nama exception */
#define JUMLAH_NAMA_EXCEPTION (sizeof(g_nama_exception) / \
                              sizeof(g_nama_exception[0]))

/*
 * ============================================================================
 * DEKLARASI FUNGSI INTERNAL
 * ============================================================================
 */

/* Entry point ISR (didefinisikan di isr_x86.S) */
extern void isr_0(void);
extern void isr_1(void);
extern void isr_2(void);
extern void isr_3(void);
extern void isr_4(void);
extern void isr_5(void);
extern void isr_6(void);
extern void isr_7(void);
extern void isr_8(void);
extern void isr_9(void);
extern void isr_10(void);
extern void isr_11(void);
extern void isr_12(void);
extern void isr_13(void);
extern void isr_14(void);
extern void isr_15(void);
extern void isr_16(void);
extern void isr_17(void);
extern void isr_18(void);
extern void isr_19(void);
extern void isr_20(void);
extern void isr_21(void);
extern void isr_22(void);
extern void isr_23(void);
extern void isr_24(void);
extern void isr_25(void);
extern void isr_26(void);
extern void isr_27(void);
extern void isr_28(void);
extern void isr_29(void);
extern void isr_30(void);
extern void isr_31(void);

/* Entry point IRQ (didefinisikan di isr_x86.S) */
extern void irq_0(void);
extern void irq_1(void);
extern void irq_2(void);
extern void irq_3(void);
extern void irq_4(void);
extern void irq_5(void);
extern void irq_6(void);
extern void irq_7(void);
extern void irq_8(void);
extern void irq_9(void);
extern void irq_10(void);
extern void irq_11(void);
extern void irq_12(void);
extern void irq_13(void);
extern void irq_14(void);
extern void irq_15(void);

/* Syscall entry */
extern void isr_128(void);

/*
 * ============================================================================
 * FUNGSI INTERNAL - IDT
 * ============================================================================
 */

/*
 * _idt_set_entry
 * --------------
 * Mengatur satu entry IDT.
 *
 * Parameter:
 *   nomor    - Nomor vektor
 *   handler  - Alamat handler
 *   selector - Segment selector
 *   flags    - Flag IDT
 */
static void _idt_set_entry(tak_bertanda32_t nomor,
                           tak_bertanda32_t handler,
                           tak_bertanda16_t selector,
                           tak_bertanda16_t flags)
{
    struct idt_entry *entry;

    if (nomor >= JUMLAH_VEKTOR) {
        return;
    }

    entry = &g_idt[nomor];

    entry->offset_bawah = (tak_bertanda16_t)(handler & 0xFFFF);
    entry->offset_atas = (tak_bertanda16_t)((handler >> 16) & 0xFFFF);
    entry->selector = selector;
    entry->reserv = 0;
    entry->flags = (tak_bertanda8_t)(flags & 0xFF);
}

/*
 * _idt_load
 * ---------
 * Memuat IDT ke register CPU.
 */
static void _idt_load(void)
{
    g_idt_ptr.batas = (tak_bertanda16_t)(sizeof(g_idt) - 1);
    g_idt_ptr.alamat = (tak_bertanda32_t)(uintptr_t)&g_idt;

    __asm__ __volatile__(
        "lidt %0\n\t"
        :
        : "m"(g_idt_ptr)
    );
}

/*
 * ============================================================================
 * FUNGSI INTERNAL - PIC
 * ============================================================================
 */

/*
 * _pic_remap
 * ----------
 * Remap PIC untuk menghindari konflik vektor dengan exception.
 */
static void _pic_remap(void)
{
    /* Init master PIC */
    outb(PIC1_COMMAND, PIC_CMD_ICW1_INIT | PIC_CMD_ICW1_ICW4);
    io_delay();

    /* Init slave PIC */
    outb(PIC2_COMMAND, PIC_CMD_ICW1_INIT | PIC_CMD_ICW1_ICW4);
    io_delay();

    /* Set vector offset master (32) */
    outb(PIC1_DATA, OFFSET_IRQ);
    io_delay();

    /* Set vector offset slave (40) */
    outb(PIC2_DATA, OFFSET_IRQ + 8);
    io_delay();

    /* Tell master ada slave di IRQ2 */
    outb(PIC1_DATA, 0x04);
    io_delay();

    /* Tell slave identitas cascade */
    outb(PIC2_DATA, 0x02);
    io_delay();

    /* Set mode 8086 */
    outb(PIC1_DATA, PIC_CMD_ICW4_8086);
    io_delay();
    outb(PIC2_DATA, PIC_CMD_ICW4_8086);
    io_delay();

    /* Mask semua IRQ kecuali cascade */
    outb(PIC1_DATA, 0xFB);
    outb(PIC2_DATA, 0xFF);
}

/*
 * _pic_acknowledge
 * ----------------
 * Mengirim EOI (End of Interrupt) ke PIC.
 *
 * Parameter:
 *   irq - Nomor IRQ
 */
static void _pic_acknowledge(tak_bertanda32_t irq)
{
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_CMD_EOI);
    }
    outb(PIC1_COMMAND, PIC_CMD_EOI);
}

/*
 * _pic_mask_irq
 * -------------
 * Mask IRQ tertentu.
 *
 * Parameter:
 *   irq  - Nomor IRQ
 *   mask - BENAR untuk mask, SALAH untuk unmask
 */
static void _pic_mask_irq(tak_bertanda32_t irq, bool_t mask)
{
    tak_bertanda8_t data;
    tak_bertanda16_t port;

    if (irq >= JUMLAH_IRQ_PIC) {
        return;
    }

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    data = inb(port);

    if (mask) {
        data |= (1 << irq);
    } else {
        data &= ~(1 << irq);
    }

    outb(port, data);
}

/*
 * ============================================================================
 * FUNGSI INTERNAL - EXCEPTION
 * ============================================================================
 */

/*
 * _cari_nama_exception
 * --------------------
 * Mencari nama exception berdasarkan nomor.
 *
 * Parameter:
 *   nomor - Nomor exception
 *
 * Return:
 *   Pointer ke struktur nama exception, atau NULL
 */
static const struct nama_exception *_cari_nama_exception(
    tak_bertanda32_t nomor)
{
    tak_bertanda32_t i;

    for (i = 0; i < JUMLAH_NAMA_EXCEPTION; i++) {
        if (g_nama_exception[i].nomor == nomor) {
            return &g_nama_exception[i];
        }
    }

    return NULL;
}

/*
 * _dump_interupsi_frame
 * ---------------------
 * Menampilkan isi interupsi frame untuk debugging.
 *
 * Parameter:
 *   frame - Pointer ke interupsi frame
 */
static void _dump_interupsi_frame(struct interupsi_frame *frame)
{
    const struct nama_exception *exc;

    if (frame == NULL) {
        return;
    }

    kernel_printf("\n=== EXCEPTION %u ===\n", frame->nomor_int);

    exc = _cari_nama_exception(frame->nomor_int);
    if (exc != NULL) {
        kernel_printf("Nama: %s (%s)\n", exc->nama, exc->pesan);
    }

    kernel_printf("Error Code: 0x%08X\n", frame->kode_error);
    kernel_printf("EIP: 0x%08X  CS: 0x%04X  EFLAGS: 0x%08X\n",
                  frame->eip, frame->cs, frame->eflags);
    kernel_printf("EAX: 0x%08X  EBX: 0x%08X  ECX: 0x%08X\n",
                  frame->eax, frame->ebx, frame->ecx);
    kernel_printf("EDX: 0x%08X  ESI: 0x%08X  EDI: 0x%08X\n",
                  frame->edx, frame->esi, frame->edi);
    kernel_printf("EBP: 0x%08X  ESP: 0x%08X  DS: 0x%04X\n",
                  frame->ebp, frame->esp, frame->ds);

    /* Untuk page fault, tampilkan alamat penyebab */
    if (frame->nomor_int == VEKTOR_PF) {
        tak_bertanda32_t cr2 = cpu_read_cr2();
        kernel_printf("CR2 (alamat fault): 0x%08X\n", cr2);

        /* Analisis error code page fault */
        kernel_printf("Page fault info: ");
        if (frame->kode_error & 0x01) {
            kernel_printf("Present ");
        } else {
            kernel_printf("Not-Present ");
        }
        if (frame->kode_error & 0x02) {
            kernel_printf("Write ");
        } else {
            kernel_printf("Read ");
        }
        if (frame->kode_error & 0x04) {
            kernel_printf("User-Mode ");
        } else {
            kernel_printf("Kernel-Mode ");
        }
        kernel_printf("\n");
    }

    kernel_printf("===================\n");
}

/*
 * _handler_exception_umum
 * -----------------------
 * Handler exception umum.
 *
 * Parameter:
 *   frame - Pointer ke interupsi frame
 */
static void _handler_exception_umum(struct interupsi_frame *frame)
{
    if (frame == NULL) {
        kernel_printf("[INT] Frame NULL!\n");
        hal_cpu_halt();
    }

    /* Increment counter */
    g_interupsi_counter[frame->nomor_int]++;

    /* Dump frame untuk debug */
    _dump_interupsi_frame(frame);

    /* Hang CPU */
    kernel_printf("[INT] System halted due to exception.\n");
    hal_cpu_halt();
}

/*
 * ============================================================================
 * FUNGSI INTERNAL - IRQ
 * ============================================================================
 */

/*
 * _handler_irq_umum
 * -----------------
 * Handler IRQ umum.
 *
 * Parameter:
 *   frame - Pointer ke interupsi frame
 */
static void _handler_irq_umum(struct interupsi_frame *frame)
{
    tak_bertanda32_t irq;

    if (frame == NULL) {
        return;
    }

    /* Hitung IRQ dari nomor interupsi */
    irq = frame->nomor_int - OFFSET_IRQ;

    /* Increment counter */
    g_interupsi_counter[frame->nomor_int]++;

    /* Panggil handler spesifik jika ada */
    if (irq < JUMLAH_IRQ_PIC && g_handler_irq[irq] != NULL) {
        g_handler_irq[irq]();
    }

    /* Acknowledge IRQ */
    _pic_acknowledge(irq);
}

/*
 * ============================================================================
 * FUNGSI INTERNAL - HANDLER DISPATCHER
 * ============================================================================
 */

/*
 * _dispatcher_interupsi
 * ---------------------
 * Dispatcher utama untuk semua interupsi.
 *
 * Parameter:
 *   frame - Pointer ke interupsi frame
 */
void _dispatcher_interupsi(struct interupsi_frame *frame)
{
    handler_interupsi_frame_t handler;

    if (frame == NULL) {
        return;
    }

    /* Cari handler yang terdaftar */
    if (frame->nomor_int < JUMLAH_VEKTOR) {
        handler = g_handler_interupsi[frame->nomor_int];

        if (handler != NULL) {
            handler(frame);
            return;
        }
    }

    /* Default handler berdasarkan tipe */
    if (frame->nomor_int < JUMLAH_EXCEPTION) {
        _handler_exception_umum(frame);
    } else if (frame->nomor_int >= OFFSET_IRQ &&
               frame->nomor_int < OFFSET_IRQ + JUMLAH_IRQ_PIC) {
        _handler_irq_umum(frame);
    } else {
        /* Unknown interrupt */
        kernel_printf("[INT] Unknown interrupt: %u\n", frame->nomor_int);
    }
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - INISIALISASI
 * ============================================================================
 */

/*
 * interupsi_x86_init
 * ------------------
 * Inisialisasi subsistem interupsi x86.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t interupsi_x86_init(void)
{
    tak_bertanda32_t i;

    kernel_printf("[INT-x86] Menginisialisasi subsistem interupsi...\n");

    /* Clear IDT */
    kernel_memset(&g_idt, 0, sizeof(g_idt));

    /* Clear handler tables */
    kernel_memset(&g_handler_interupsi, 0, sizeof(g_handler_interupsi));
    kernel_memset(&g_handler_irq, 0, sizeof(g_handler_irq));
    kernel_memset(&g_interupsi_counter, 0, sizeof(g_interupsi_counter));

    /* Setup exception handlers (ISR 0-31) */
    _idt_set_entry(0, (tak_bertanda32_t)(uintptr_t)isr_0,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(1, (tak_bertanda32_t)(uintptr_t)isr_1,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(2, (tak_bertanda32_t)(uintptr_t)isr_2,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(3, (tak_bertanda32_t)(uintptr_t)isr_3,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(4, (tak_bertanda32_t)(uintptr_t)isr_4,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(5, (tak_bertanda32_t)(uintptr_t)isr_5,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(6, (tak_bertanda32_t)(uintptr_t)isr_6,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(7, (tak_bertanda32_t)(uintptr_t)isr_7,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(8, (tak_bertanda32_t)(uintptr_t)isr_8,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(9, (tak_bertanda32_t)(uintptr_t)isr_9,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(10, (tak_bertanda32_t)(uintptr_t)isr_10,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(11, (tak_bertanda32_t)(uintptr_t)isr_11,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(12, (tak_bertanda32_t)(uintptr_t)isr_12,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(13, (tak_bertanda32_t)(uintptr_t)isr_13,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(14, (tak_bertanda32_t)(uintptr_t)isr_14,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(15, (tak_bertanda32_t)(uintptr_t)isr_15,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(16, (tak_bertanda32_t)(uintptr_t)isr_16,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(17, (tak_bertanda32_t)(uintptr_t)isr_17,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(18, (tak_bertanda32_t)(uintptr_t)isr_18,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(19, (tak_bertanda32_t)(uintptr_t)isr_19,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(20, (tak_bertanda32_t)(uintptr_t)isr_20,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(21, (tak_bertanda32_t)(uintptr_t)isr_21,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(22, (tak_bertanda32_t)(uintptr_t)isr_22,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(23, (tak_bertanda32_t)(uintptr_t)isr_23,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(24, (tak_bertanda32_t)(uintptr_t)isr_24,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(25, (tak_bertanda32_t)(uintptr_t)isr_25,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(26, (tak_bertanda32_t)(uintptr_t)isr_26,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(27, (tak_bertanda32_t)(uintptr_t)isr_27,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(28, (tak_bertanda32_t)(uintptr_t)isr_28,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(29, (tak_bertanda32_t)(uintptr_t)isr_29,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(30, (tak_bertanda32_t)(uintptr_t)isr_30,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    _idt_set_entry(31, (tak_bertanda32_t)(uintptr_t)isr_31,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);

    /* Setup IRQ handlers (ISR 32-47) */
    for (i = 0; i < JUMLAH_IRQ_PIC; i++) {
        tak_bertanda32_t isr_addr;

        switch (i) {
            case 0:  isr_addr = (tak_bertanda32_t)(uintptr_t)irq_0;  break;
            case 1:  isr_addr = (tak_bertanda32_t)(uintptr_t)irq_1;  break;
            case 2:  isr_addr = (tak_bertanda32_t)(uintptr_t)irq_2;  break;
            case 3:  isr_addr = (tak_bertanda32_t)(uintptr_t)irq_3;  break;
            case 4:  isr_addr = (tak_bertanda32_t)(uintptr_t)irq_4;  break;
            case 5:  isr_addr = (tak_bertanda32_t)(uintptr_t)irq_5;  break;
            case 6:  isr_addr = (tak_bertanda32_t)(uintptr_t)irq_6;  break;
            case 7:  isr_addr = (tak_bertanda32_t)(uintptr_t)irq_7;  break;
            case 8:  isr_addr = (tak_bertanda32_t)(uintptr_t)irq_8;  break;
            case 9:  isr_addr = (tak_bertanda32_t)(uintptr_t)irq_9;  break;
            case 10: isr_addr = (tak_bertanda32_t)(uintptr_t)irq_10; break;
            case 11: isr_addr = (tak_bertanda32_t)(uintptr_t)irq_11; break;
            case 12: isr_addr = (tak_bertanda32_t)(uintptr_t)irq_12; break;
            case 13: isr_addr = (tak_bertanda32_t)(uintptr_t)irq_13; break;
            case 14: isr_addr = (tak_bertanda32_t)(uintptr_t)irq_14; break;
            case 15: isr_addr = (tak_bertanda32_t)(uintptr_t)irq_15; break;
            default: isr_addr = 0; break;
        }

        _idt_set_entry(OFFSET_IRQ + i, isr_addr,
                       SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x0E);
    }

    /* Setup syscall handler (ISR 128) */
    _idt_set_entry(VEKTOR_SYSCALL, (tak_bertanda32_t)(uintptr_t)isr_128,
                   SELECTOR_KERNEL_CODE, IDT_FLAG_PRESENT | 0x60 | 0x0E);

    /* Fill remaining entries dengan handler default */
    for (i = 48; i < JUMLAH_VEKTOR; i++) {
        if (i != VEKTOR_SYSCALL) {
            /* Reserved entries */
        }
    }

    /* Remap PIC */
    kernel_printf("[INT-x86] Remap PIC...\n");
    _pic_remap();

    /* Load IDT */
    kernel_printf("[INT-x86] Load IDT...\n");
    _idt_load();

    g_interupsi_diinisialisasi = BENAR;

    kernel_printf("[INT-x86] Subsistem interupsi siap\n");

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - REGISTRASI HANDLER
 * ============================================================================
 */

/*
 * interupsi_x86_daftarkan_handler
 * -------------------------------
 * Mendaftarkan handler untuk vektor interupsi tertentu.
 *
 * Parameter:
 *   vektor  - Nomor vektor interupsi
 *   handler - Pointer ke fungsi handler
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t interupsi_x86_daftarkan_handler(tak_bertanda32_t vektor,
                                          handler_interupsi_frame_t handler)
{
    if (vektor >= JUMLAH_VEKTOR) {
        return STATUS_PARAM_INVALID;
    }

    g_handler_interupsi[vektor] = handler;

    return STATUS_BERHASIL;
}

/*
 * interupsi_x86_daftarkan_handler_irq
 * -----------------------------------
 * Mendaftarkan handler untuk IRQ tertentu.
 *
 * Parameter:
 *   irq     - Nomor IRQ (0-15)
 *   handler - Pointer ke fungsi handler
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t interupsi_x86_daftarkan_handler_irq(tak_bertanda32_t irq,
                                              void (*handler)(void))
{
    if (irq >= JUMLAH_IRQ_PIC) {
        return STATUS_PARAM_INVALID;
    }

    g_handler_irq[irq] = handler;

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - KONTROL IRQ
 * ============================================================================
 */

/*
 * interupsi_x86_aktifkan_irq
 * --------------------------
 * Mengaktifkan IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t interupsi_x86_aktifkan_irq(tak_bertanda32_t irq)
{
    if (irq >= JUMLAH_IRQ_PIC) {
        return STATUS_PARAM_INVALID;
    }

    _pic_mask_irq(irq, SALAH);

    return STATUS_BERHASIL;
}

/*
 * interupsi_x86_nonaktifkan_irq
 * -----------------------------
 * Menonaktifkan IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t interupsi_x86_nonaktifkan_irq(tak_bertanda32_t irq)
{
    if (irq >= JUMLAH_IRQ_PIC) {
        return STATUS_PARAM_INVALID;
    }

    _pic_mask_irq(irq, BENAR);

    return STATUS_BERHASIL;
}

/*
 * interupsi_x86_ack_irq
 * ---------------------
 * Mengirim acknowledge untuk IRQ.
 *
 * Parameter:
 *   irq - Nomor IRQ
 */
void interupsi_x86_ack_irq(tak_bertanda32_t irq)
{
    if (irq >= JUMLAH_IRQ_PIC) {
        return;
    }

    _pic_acknowledge(irq);
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - STATISTIK
 * ============================================================================
 */

/*
 * interupsi_x86_get_counter
 * -------------------------
 * Mendapatkan counter interupsi untuk vektor tertentu.
 *
 * Parameter:
 *   vektor - Nomor vektor interupsi
 *
 * Return:
 *   Jumlah interupsi, atau 0 jika vektor tidak valid
 */
tak_bertanda64_t interupsi_x86_get_counter(tak_bertanda32_t vektor)
{
    if (vektor >= JUMLAH_VEKTOR) {
        return 0;
    }

    return g_interupsi_counter[vektor];
}

/*
 * interupsi_x86_print_statistik
 * -----------------------------
 * Menampilkan statistik interupsi.
 */
void interupsi_x86_print_statistik(void)
{
    tak_bertanda32_t i;

    kernel_printf("\n=== Statistik Interupsi ===\n");

    /* Exception counters */
    kernel_printf("\nExceptions:\n");
    for (i = 0; i < JUMLAH_EXCEPTION; i++) {
        if (g_interupsi_counter[i] > 0) {
            const struct nama_exception *exc = _cari_nama_exception(i);
            if (exc != NULL) {
                kernel_printf("  %s (#%u): %u\n",
                              exc->nama, i,
                              (tak_bertanda32_t)g_interupsi_counter[i]);
            } else {
                kernel_printf("  #%u: %u\n", i,
                              (tak_bertanda32_t)g_interupsi_counter[i]);
            }
        }
    }

    /* IRQ counters */
    kernel_printf("\nIRQ:\n");
    for (i = 0; i < JUMLAH_IRQ_PIC; i++) {
        tak_bertanda64_t count = g_interupsi_counter[OFFSET_IRQ + i];
        if (count > 0) {
            kernel_printf("  IRQ%u: %u\n", i, (tak_bertanda32_t)count);
        }
    }

    kernel_printf("===========================\n");
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - ENABLE/DISABLE
 * ============================================================================
 */

/*
 * interupsi_x86_aktifkan
 * ----------------------
 * Mengaktifkan interupsi CPU.
 */
void interupsi_x86_aktifkan(void)
{
    cpu_enable_irq();
}

/*
 * interupsi_x86_nonaktifkan
 * -------------------------
 * Menonaktifkan interupsi CPU.
 */
void interupsi_x86_nonaktifkan(void)
{
    cpu_disable_irq();
}

/*
 * interupsi_x86_apakah_siap
 * -------------------------
 * Mengecek apakah subsistem interupsi sudah diinisialisasi.
 *
 * Return:
 *   BENAR jika sudah, SALAH jika belum
 */
bool_t interupsi_x86_apakah_siap(void)
{
    return g_interupsi_diinisialisasi;
}
