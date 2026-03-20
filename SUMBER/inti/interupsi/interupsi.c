/*
 * PIGURA OS - INTERUPSI.C
 * -----------------------
 * Implementasi manajemen interupsi untuk kernel.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk mengelola
 * interrupt descriptor table (IDT) dan interrupt service routines (ISR).
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

/* Jumlah entri IDT */
#define IDT_ENTRIES             256

/* Flag IDT */
#define IDT_FLAG_PRESENT        0x80
#define IDT_FLAG_DPL_0          0x00
#define IDT_FLAG_DPL_3          0x60
#define IDT_FLAG_TYPE_INTERRUPT 0x0E
#define IDT_FLAG_TYPE_TRAP      0x0F

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* Struktur entry IDT */
typedef struct {
    tak_bertanda16_t base_low;      /* Alamat handler (bit 0-15) */
    tak_bertanda16_t selector;      /* Segment selector */
    tak_bertanda8_t  reserved;      /* Reserved, harus 0 */
    tak_bertanda8_t  flags;         /* Flags */
    tak_bertanda16_t base_high;     /* Alamat handler (bit 16-31) */
} __attribute__((packed)) idt_entry_t;

/* Struktur IDTR */
typedef struct {
    tak_bertanda16_t limit;         /* Ukuran IDT - 1 */
    tak_bertanda32_t base;          /* Alamat IDT */
} __attribute__((packed)) idtr_t;

/* Struktur stack frame untuk exception */
typedef struct {
    tak_bertanda32_t gs, fs, es, ds;
    tak_bertanda32_t edi, esi, ebp, esp;
    tak_bertanda32_t ebx, edx, ecx, eax;
    tak_bertanda32_t int_no, err_code;
    tak_bertanda32_t eip, cs, eflags;
    tak_bertanda32_t useresp, ss;
} __attribute__((packed)) interrupt_frame_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* IDT */
static idt_entry_t idt[IDT_ENTRIES] __attribute__((aligned(16)));

/* IDTR */
static idtr_t idtr;

/* Status inisialisasi */
static bool_t idt_initialized = SALAH;

/* Handler exception */
static void (*exception_handlers[32])(interrupt_frame_t *) = {NULL};

/* Handler IRQ */
static void (*irq_handlers[16])(void) = {NULL};

/*
 * ============================================================================
 * DEKLARASI EXTERNAL (EXTERNAL DECLARATIONS)
 * ============================================================================
 */

/* Assembly ISR stubs - didefinisikan di isr.asm */
extern void isr_stub_0(void);
extern void isr_stub_1(void);
extern void isr_stub_2(void);
extern void isr_stub_3(void);
extern void isr_stub_4(void);
extern void isr_stub_5(void);
extern void isr_stub_6(void);
extern void isr_stub_7(void);
extern void isr_stub_8(void);
extern void isr_stub_9(void);
extern void isr_stub_10(void);
extern void isr_stub_11(void);
extern void isr_stub_12(void);
extern void isr_stub_13(void);
extern void isr_stub_14(void);
extern void isr_stub_15(void);
extern void isr_stub_16(void);
extern void isr_stub_17(void);
extern void isr_stub_18(void);
extern void isr_stub_19(void);
extern void isr_stub_20(void);
extern void isr_stub_21(void);
extern void isr_stub_22(void);
extern void isr_stub_23(void);
extern void isr_stub_24(void);
extern void isr_stub_25(void);
extern void isr_stub_26(void);
extern void isr_stub_27(void);
extern void isr_stub_28(void);
extern void isr_stub_29(void);
extern void isr_stub_30(void);
extern void isr_stub_31(void);

/* IRQ stubs */
extern void irq_stub_0(void);
extern void irq_stub_1(void);
extern void irq_stub_2(void);
extern void irq_stub_3(void);
extern void irq_stub_4(void);
extern void irq_stub_5(void);
extern void irq_stub_6(void);
extern void irq_stub_7(void);
extern void irq_stub_8(void);
extern void irq_stub_9(void);
extern void irq_stub_10(void);
extern void irq_stub_11(void);
extern void irq_stub_12(void);
extern void irq_stub_13(void);
extern void irq_stub_14(void);
extern void irq_stub_15(void);

/* Syscall stub */
extern void syscall_stub(void);

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * idt_set_gate
 * ------------
 * Set entry IDT.
 *
 * Parameter:
 *   num      - Nomor entry
 *   base     - Alamat handler
 *   selector - Segment selector
 *   flags    - Flags
 */
static void idt_set_gate(tak_bertanda8_t num, tak_bertanda32_t base,
                         tak_bertanda16_t selector, tak_bertanda8_t flags)
{
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = selector;
    idt[num].reserved = 0;
    idt[num].flags = flags;
}

/*
 * idt_load
 * --------
 * Load IDT ke IDTR.
 */
static void idt_load(void)
{
    idtr.limit = sizeof(idt) - 1;
    idtr.base = (tak_bertanda32_t)&idt;

    __asm__ __volatile__("lidt %0" : : "m"(idtr));
}

/*
 * exception_handler_default
 * -------------------------
 * Handler default untuk exception.
 */
static void exception_handler_default(interrupt_frame_t *frame)
{
    kernel_panic_exception((register_context_t *)frame);
}

/*
 * irq_handler_default
 * -------------------
 * Handler default untuk IRQ.
 */
static void irq_handler_default(void)
{
    /* Tidak melakukan apa-apa */
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * interupsi_init
 * --------------
 * Inisialisasi sistem interupsi.
 */
status_t interupsi_init(void)
{
    tak_bertanda32_t i;

    if (idt_initialized) {
        return STATUS_SUDAH_ADA;
    }

    /* Clear IDT */
    kernel_memset(&idt, 0, sizeof(idt));

    /* Set exception handlers (ISR 0-31) */
    idt_set_gate(0, (tak_bertanda32_t)isr_stub_0, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(1, (tak_bertanda32_t)isr_stub_1, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(2, (tak_bertanda32_t)isr_stub_2, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(3, (tak_bertanda32_t)isr_stub_3, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_TRAP);
    idt_set_gate(4, (tak_bertanda32_t)isr_stub_4, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(5, (tak_bertanda32_t)isr_stub_5, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(6, (tak_bertanda32_t)isr_stub_6, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(7, (tak_bertanda32_t)isr_stub_7, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(8, (tak_bertanda32_t)isr_stub_8, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(9, (tak_bertanda32_t)isr_stub_9, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(10, (tak_bertanda32_t)isr_stub_10, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(11, (tak_bertanda32_t)isr_stub_11, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(12, (tak_bertanda32_t)isr_stub_12, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(13, (tak_bertanda32_t)isr_stub_13, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(14, (tak_bertanda32_t)isr_stub_14, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(15, (tak_bertanda32_t)isr_stub_15, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(16, (tak_bertanda32_t)isr_stub_16, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(17, (tak_bertanda32_t)isr_stub_17, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(18, (tak_bertanda32_t)isr_stub_18, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(19, (tak_bertanda32_t)isr_stub_19, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(20, (tak_bertanda32_t)isr_stub_20, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);

    /* Reserved 21-31 */
    for (i = 21; i < 32; i++) {
        idt_set_gate((tak_bertanda8_t)i, (tak_bertanda32_t)isr_stub_0,
                     SELECTOR_KERNEL_CODE,
                     IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 |
                     IDT_FLAG_TYPE_INTERRUPT);
    }

    /* Set IRQ handlers (ISR 32-47) */
    idt_set_gate(32, (tak_bertanda32_t)irq_stub_0, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(33, (tak_bertanda32_t)irq_stub_1, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(34, (tak_bertanda32_t)irq_stub_2, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(35, (tak_bertanda32_t)irq_stub_3, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(36, (tak_bertanda32_t)irq_stub_4, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(37, (tak_bertanda32_t)irq_stub_5, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(38, (tak_bertanda32_t)irq_stub_6, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(39, (tak_bertanda32_t)irq_stub_7, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(40, (tak_bertanda32_t)irq_stub_8, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(41, (tak_bertanda32_t)irq_stub_9, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(42, (tak_bertanda32_t)irq_stub_10, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(43, (tak_bertanda32_t)irq_stub_11, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(44, (tak_bertanda32_t)irq_stub_12, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(45, (tak_bertanda32_t)irq_stub_13, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(46, (tak_bertanda32_t)irq_stub_14, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);
    idt_set_gate(47, (tak_bertanda32_t)irq_stub_15, SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_0 | IDT_FLAG_TYPE_INTERRUPT);

    /* Set syscall handler (ISR 128) */
    idt_set_gate(VEKTOR_SYSCALL, (tak_bertanda32_t)syscall_stub,
                 SELECTOR_KERNEL_CODE,
                 IDT_FLAG_PRESENT | IDT_FLAG_DPL_3 | IDT_FLAG_TYPE_INTERRUPT);

    /* Inisialisasi handler default */
    for (i = 0; i < 32; i++) {
        exception_handlers[i] = exception_handler_default;
    }

    for (i = 0; i < 16; i++) {
        irq_handlers[i] = irq_handler_default;
    }

    /* Load IDT */
    idt_load();

    idt_initialized = BENAR;

    kernel_printf("[INT] IDT diinisialisasi (%d entri)\n", IDT_ENTRIES);

    return STATUS_BERHASIL;
}

/*
 * interupsi_set_exception_handler
 * -------------------------------
 * Set handler untuk exception.
 */
status_t interupsi_set_exception_handler(tak_bertanda32_t num,
                                         void (*handler)(interrupt_frame_t *))
{
    if (num >= 32) {
        return STATUS_PARAM_INVALID;
    }

    exception_handlers[num] = handler;

    return STATUS_BERHASIL;
}

/*
 * interupsi_set_irq_handler
 * -------------------------
 * Set handler untuk IRQ.
 */
status_t interupsi_set_irq_handler(tak_bertanda32_t irq,
                                   void (*handler)(void))
{
    if (irq >= 16) {
        return STATUS_PARAM_INVALID;
    }

    irq_handlers[irq] = handler;

    return STATUS_BERHASIL;
}

/*
 * interupsi_dispatch_exception
 * ----------------------------
 * Dispatch exception ke handler yang sesuai.
 * Dipanggil dari assembly ISR stub.
 */
void interupsi_dispatch_exception(interrupt_frame_t *frame)
{
    if (frame->int_no < 32 && exception_handlers[frame->int_no] != NULL) {
        exception_handlers[frame->int_no](frame);
    } else {
        kernel_panic_exception((register_context_t *)frame);
    }
}

/*
 * interupsi_dispatch_irq
 * ----------------------
 * Dispatch IRQ ke handler yang sesuai.
 * Dipanggil dari assembly IRQ stub.
 */
void interupsi_dispatch_irq(interrupt_frame_t *frame)
{
    tak_bertanda32_t irq;

    irq = frame->int_no - 32;

    if (irq < 16 && irq_handlers[irq] != NULL) {
        irq_handlers[irq]();
    }

    /* Kirim EOI ke PIC */
    if (irq >= 8) {
        outb(0xA0, 0x20);  /* EOI ke PIC2 */
    }
    outb(0x20, 0x20);      /* EOI ke PIC1 */
}

/*
 * interupsi_enable
 * ----------------
 * Aktifkan interupsi.
 */
void interupsi_enable(void)
{
    cpu_enable_irq();
}

/*
 * interupsi_disable
 * -----------------
 * Nonaktifkan interupsi.
 */
void interupsi_disable(void)
{
    cpu_disable_irq();
}
