/*
 * =============================================================================
 * PIGURA OS - IDT.C
 * =================
 * Implementasi Interrupt Descriptor Table untuk Pigura OS.
 *
 * Berkas ini berisi fungsi-fungsi untuk menginisialisasi dan
 * mengelola IDT (Interrupt Descriptor Table) pada arsitektur x86.
 *
 * Arsitektur: x86/x86_64
 * Versi: 1.0
 * =============================================================================
 */

#include <stdint.h>
#include <stddef.h>

/* =============================================================================
 * KONSTANTA
 * =============================================================================
 */

/* Jumlah interrupt vectors */
#define IDT_ENTRIES             256

/* Interrupt vector assignments */
#define IDT_DE                  0       /* Divide Error */
#define IDT_DB                  1       /* Debug Exception */
#define IDT_NMI                 2       /* NMI Interrupt */
#define IDT_BP                  3       /* Breakpoint */
#define IDT_OF                  4       /* Overflow */
#define IDT_BR                  5       /* BOUND Range */
#define IDT_UD                  6       /* Invalid Opcode */
#define IDT_NM                  7       /* Device Not Available */
#define IDT_DF                  8       /* Double Fault */
#define IDT_CSO                 9       /* Coprocessor Segment Overrun */
#define IDT_TS                  10      /* Invalid TSS */
#define IDT_NP                  11      /* Segment Not Present */
#define IDT_SS                  12      /* Stack Fault */
#define IDT_GP                  13      /* General Protection */
#define IDT_PF                  14      /* Page Fault */
#define IDT_MF                  16      /* x87 FPU Error */
#define IDT_AC                  17      /* Alignment Check */
#define IDT_MC                  18      /* Machine Check */
#define IDT_XM                  19      /* SIMD Exception */
#define IDT_VE                  20      /* Virtualization Exception */

/* IRQ vectors (mulai dari 32) */
#define IDT_IRQ_BASE            32
#define IDT_IRQ_TIMER           32      /* IRQ0 - Timer */
#define IDT_IRQ_KEYBOARD        33      /* IRQ1 - Keyboard */
#define IDT_IRQ_CASCADE         34      /* IRQ2 - Cascade */
#define IDT_IRQ_COM2            35      /* IRQ3 - COM2 */
#define IDT_IRQ_COM1            36      /* IRQ4 - COM1 */
#define IDT_IRQ_LPT2            37      /* IRQ5 - LPT2 */
#define IDT_IRQ_FLOPPY          38      /* IRQ6 - Floppy */
#define IDT_IRQ_LPT1            39      /* IRQ7 - LPT1 */
#define IDT_IRQ_RTC             40      /* IRQ8 - RTC */
#define IDT_IRQ_MOUSE           44      /* IRQ12 - PS/2 Mouse */
#define IDT_IRQ_FPU             45      /* IRQ13 - FPU */
#define IDT_IRQ_ATA1            46      /* IRQ14 - Primary ATA */
#define IDT_IRQ_ATA2            47      /* IRQ15 - Secondary ATA */

/* Syscall vector */
#define IDT_SYSCALL             0x80    /* System call interrupt */

/* IDT access flags */
#define IDT_FLAG_PRESENT        0x80
#define IDT_FLAG_DPL_0          0x00
#define IDT_FLAG_DPL_3          0x60
#define IDT_FLAG_SYSTEM         0x00
#define IDT_FLAG_INTERRUPT_32   0x0E    /* 32-bit interrupt gate */
#define IDT_FLAG_TRAP_32        0x0F    /* 32-bit trap gate */
#define IDT_FLAG_INTERRUPT_16   0x06    /* 16-bit interrupt gate */
#define IDT_FLAG_TRAP_16        0x07    /* 16-bit trap gate */

/* =============================================================================
 * TIPE DATA
 * =============================================================================
 */

/* IDT entry (8 bytes untuk 32-bit) */
typedef struct {
    uint16_t offset_low;        /* Offset 0:15 */
    uint16_t selector;          /* Code segment selector */
    uint8_t zero;               /* Reserved */
    uint8_t flags;              /* Type and attributes */
    uint16_t offset_high;       /* Offset 16:31 */
} __attribute__((packed)) idt_entry_t;

/* IDT pointer */
typedef struct {
    uint16_t limit;             /* Size of IDT - 1 */
    uint32_t base;              /* Address of IDT */
} __attribute__((packed)) idt_ptr_t;

/* Interrupt stack frame */
typedef struct {
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t int_no;
    uint32_t err_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t useresp;
    uint32_t ss;
} interrupt_frame_t;

/* Interrupt handler type */
typedef void (*interrupt_handler_t)(interrupt_frame_t *frame);

/* =============================================================================
 * FORWARD DECLARATIONS
 * =============================================================================
 */
static inline uint8_t _inb(uint16_t port);
static inline void _outb(uint8_t val, uint16_t port);

/* =============================================================================
 * VARIABEL GLOBAL
 * =============================================================================
 */

/* IDT table */
static idt_entry_t g_idt[IDT_ENTRIES];

/* IDT pointer */
static idt_ptr_t g_idt_ptr;

/* Interrupt handlers */
static interrupt_handler_t g_handlers[IDT_ENTRIES];

/* Flag inisialisasi */
static int g_idt_initialized = 0;

/* Error message strings */
static const char *g_exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

/* =============================================================================
 * FUNGSI ASSEMBLY INLINE
 * =============================================================================
 */

/*
 * _lidt
 * -----
 * Load IDT register.
 */
static inline void _lidt(idt_ptr_t *ptr)
{
    __asm__ __volatile__(
        "lidt %0"
        :
        : "m"(*ptr)
    );
}

/*
 * _cli
 * ----
 * Disable interrupts.
 */
static inline void _cli(void)
{
    __asm__ __volatile__("cli");
}

/*
 * _sti
 * ----
 * Enable interrupts.
 */
static inline void _sti(void)
{
    __asm__ __volatile__("sti");
}

/* =============================================================================
 * FUNGSI INTERNAL
 * =============================================================================
 */

/*
 * _set_idt_entry
 * --------------
 * Mengatur entry IDT.
 */
static void _set_idt_entry(int index, uint32_t offset,
                            uint16_t selector, uint8_t flags)
{
    idt_entry_t *entry;

    entry = &g_idt[index];

    entry->offset_low = (uint16_t)(offset & 0xFFFF);
    entry->offset_high = (uint16_t)((offset >> 16) & 0xFFFF);
    entry->selector = selector;
    entry->zero = 0;
    entry->flags = flags | IDT_FLAG_PRESENT;
}

/*
 * _default_handler
 * ----------------
 * Handler default untuk interrupt yang tidak ditangani.
 */
static void _default_handler(interrupt_frame_t *frame)
{
    /* Handler kosong */
    (void)frame;
}

/* =============================================================================
 * HANDLER EXCEPTION
 * =============================================================================
 */

/*
 * _exception_handler
 * ------------------
 * Handler untuk CPU exceptions.
 */
void _exception_handler(interrupt_frame_t *frame)
{
    const char *msg;
    interrupt_handler_t handler;

    /* Get exception message */
    if (frame->int_no < 32) {
        msg = g_exception_messages[frame->int_no];
    } else {
        msg = "Unknown Exception";
    }

    /* Call registered handler if exists */
    if (g_handlers[frame->int_no] != NULL) {
        handler = g_handlers[frame->int_no];
        handler(frame);
        return;
    }

    /* Default handling */
    /* In real implementation, this would print to console */
    (void)msg;
}

/* =============================================================================
 * FUNGSI PUBLIC
 * =============================================================================
 */

/*
 * idt_init
 * --------
 * Menginisialisasi IDT.
 */
void idt_init(void)
{
    int i;

    /* Clear IDT */
    for (i = 0; i < IDT_ENTRIES; i++) {
        g_idt[i].offset_low = 0;
        g_idt[i].offset_high = 0;
        g_idt[i].selector = 0;
        g_idt[i].zero = 0;
        g_idt[i].flags = 0;

        g_handlers[i] = NULL;
    }

    /* Set IDT pointer */
    g_idt_ptr.limit = sizeof(g_idt) - 1;
    g_idt_ptr.base = (uint32_t)(uintptr_t)&g_idt;

    /* Load IDT */
    _lidt(&g_idt_ptr);

    g_idt_initialized = 1;
}

/*
 * idt_set_gate
 * ------------
 * Mengatur interrupt gate.
 *
 * Parameter:
 *   index    - Vector number
 *   handler  - Handler function address
 *   selector - Code segment selector
 *   flags    - Gate flags
 */
void idt_set_gate(uint8_t index, uint32_t handler,
                   uint16_t selector, uint8_t flags)
{
    _set_idt_entry(index, handler, selector, flags);
}

/*
 * idt_set_handler
 * ---------------
 * Mengatur handler untuk interrupt tertentu.
 */
void idt_set_handler(uint8_t index, interrupt_handler_t handler)
{
    /* uint8_t always valid for 256-entry IDT */
    g_handlers[index] = handler;
}

/*
 * idt_get_handler
 * ---------------
 * Mendapatkan handler untuk interrupt tertentu.
 */
interrupt_handler_t idt_get_handler(uint8_t index)
{
    /* uint8_t always valid for 256-entry IDT */
    return g_handlers[index];
}

/*
 * idt_enable_interrupts
 * ---------------------
 * Mengaktifkan interrupts.
 */
void idt_enable_interrupts(void)
{
    _sti();
}

/*
 * idt_disable_interrupts
 * ----------------------
 * Menonaktifkan interrupts.
 */
void idt_disable_interrupts(void)
{
    _cli();
}

/*
 * idt_is_initialized
 * ------------------
 * Cek apakah IDT sudah diinisialisasi.
 */
int idt_is_initialized(void)
{
    return g_idt_initialized;
}

/*
 * idt_get_ptr
 * -----------
 * Mendapatkan pointer ke IDT.
 */
idt_ptr_t *idt_get_ptr(void)
{
    return &g_idt_ptr;
}

/*
 * idt_get_exception_message
 * -------------------------
 * Mendapatkan pesan untuk exception tertentu.
 */
const char *idt_get_exception_message(uint8_t vector)
{
    if (vector < 32) {
        return g_exception_messages[vector];
    }
    return "Unknown Exception";
}

/*
 * idt_mask_irq
 * ------------
 * Mask IRQ pada PIC.
 */
void idt_mask_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = 0x21;    /* PIC1 data port */
    } else {
        port = 0xA1;    /* PIC2 data port */
        irq -= 8;
    }

    value = (uint8_t)(_inb(port) | (1 << irq));
    _outb(value, port);
}

/*
 * idt_unmask_irq
 * --------------
 * Unmask IRQ pada PIC.
 */
void idt_unmask_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = 0x21;    /* PIC1 data port */
    } else {
        port = 0xA1;    /* PIC2 data port */
        irq -= 8;
    }

    value = (uint8_t)(_inb(port) & ~(1 << irq));
    _outb(value, port);
}

/*
 * idt_send_eoi
 * ------------
 * Kirim End of Interrupt ke PIC.
 */
void idt_send_eoi(uint8_t irq)
{
    if (irq >= 8) {
        _outb(0x20, 0xA0);  /* EOI to PIC2 */
    }
    _outb(0x20, 0x20);      /* EOI to PIC1 */
}

/*
 * _inb
 * ----
 * Baca byte dari port.
 */
static inline uint8_t _inb(uint16_t port)
{
    uint8_t val;
    __asm__ __volatile__("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/*
 * _outb
 * -----
 * Tulis byte ke port.
 */
static inline void _outb(uint8_t val, uint16_t port)
{
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}
