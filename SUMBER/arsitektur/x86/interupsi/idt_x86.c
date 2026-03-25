/*
 * =============================================================================
 * PIGURA OS - INTERRUPT DESCRIPTOR TABLE (IDT) x86
 * =============================================================================
 *
 * Berkas ini berisi implementasi Interrupt Descriptor Table (IDT) untuk
 * arsitektur x86. IDT mendefinisikan handler untuk setiap interrupt dan
 * exception yang dapat terjadi pada CPU.
 *
 * Struktur IDT:
 *   - Entry 0-19:  CPU Exceptions (wajib)
 *   - Entry 20-31: Reserved
 *   - Entry 32-47: IRQ 0-15
 *   - Entry 48-255: Software Interrupts (syscall, dll)
 *
 * Arsitektur: x86 (32-bit protected mode)
 * Versi: 1.0
 * =============================================================================
 */

#include "../../../inti/kernel.h"

/* Type aliases untuk kompatibilitas dengan kode yang menggunakan stdint.h */
typedef tak_bertanda8_t  uint8_t;
typedef tak_bertanda16_t uint16_t;
typedef tak_bertanda32_t uint32_t;
typedef tak_bertanda64_t uint64_t;

/* =============================================================================
 * KONSTANTA
 * =============================================================================
 */

/* Jumlah entry IDT */
#define IDT_JUMLAH_ENTRY        256

/* Vektor exception CPU */
#define IDT_VEKTOR_DE           0       /* Divide Error */
#define IDT_VEKTOR_DB           1       /* Debug Exception */
#define IDT_VEKTOR_NMI          2       /* NMI Interrupt */
#define IDT_VEKTOR_BP           3       /* Breakpoint */
#define IDT_VEKTOR_OF           4       /* Overflow */
#define IDT_VEKTOR_BR           5       /* BOUND Range Exceeded */
#define IDT_VEKTOR_UD           6       /* Invalid Opcode */
#define IDT_VEKTOR_NM           7       /* Device Not Available */
#define IDT_VEKTOR_DF           8       /* Double Fault */
#define IDT_VEKTOR_TS           10      /* Invalid TSS */
#define IDT_VEKTOR_NP           11      /* Segment Not Present */
#define IDT_VEKTOR_SS           12      /* Stack-Segment Fault */
#define IDT_VEKTOR_GP           13      /* General Protection */
#define IDT_VEKTOR_PF           14      /* Page Fault */
#define IDT_VEKTOR_MF           16      /* x87 FPU Error */
#define IDT_VEKTOR_AC           17      /* Alignment Check */
#define IDT_VEKTOR_MC           18      /* Machine Check */
#define IDT_VEKTOR_XF           19      /* SIMD Exception */

/* Vektor IRQ */
#define IDT_VEKTOR_IRQ0         32      /* Timer */
#define IDT_VEKTOR_IRQ1         33      /* Keyboard */
#define IDT_VEKTOR_IRQ2         34      /* Cascade */
#define IDT_VEKTOR_IRQ3         35      /* COM2 */
#define IDT_VEKTOR_IRQ4         36      /* COM1 */
#define IDT_VEKTOR_IRQ5         37      /* LPT2 */
#define IDT_VEKTOR_IRQ6         38      /* Floppy */
#define IDT_VEKTOR_IRQ7         39      /* LPT1 */
#define IDT_VEKTOR_IRQ8         40      /* RTC */
#define IDT_VEKTOR_IRQ9         41      /* Free */
#define IDT_VEKTOR_IRQ10        42      /* Free */
#define IDT_VEKTOR_IRQ11        43      /* Free */
#define IDT_VEKTOR_IRQ12        44      /* PS/2 Mouse */
#define IDT_VEKTOR_IRQ13        45      /* FPU */
#define IDT_VEKTOR_IRQ14        46      /* Primary ATA */
#define IDT_VEKTOR_IRQ15        47      /* Secondary ATA */

/* Vektor syscall */
#define IDT_VEKTOR_SYSCALL      128     /* 0x80 - System call */

/* Flag deskriptor IDT */
#define IDT_FLAG_GATETYPE_TASK   0x05   /* Task gate */
#define IDT_FLAG_GATETYPE_INT    0x0E   /* Interrupt gate (32-bit) */
#define IDT_FLAG_GATETYPE_TRAP   0x0F   /* Trap gate (32-bit) */

#define IDT_FLAG_PRESENT         0x80   /* Descriptor present */
#define IDT_FLAG_DPL0            0x00   /* Privilege level 0 */
#define IDT_FLAG_DPL3            0x60   /* Privilege level 3 */
#define IDT_FLAG_STORAGE         0x00   /* Storage segment (must be 0) */

/* Selector untuk IDT */
#define IDT_SELECTOR_KODE        0x08   /* Kernel code selector */

/* =============================================================================
 * STRUKTUR DATA
 * =============================================================================
 */

/*
 * Entry IDT (8 byte)
 * Mendefinisikan satu interrupt gate dalam IDT.
 */
struct idt_entry {
    uint16_t offset_rendah;     /* Bit 0-15 dari handler address */
    uint16_t selector;          /* Code segment selector */
    uint8_t  nol;               /* Reserved, harus 0 */
    uint8_t  flags;             /* Type dan attribute flags */
    uint16_t offset_atas;       /* Bit 16-31 dari handler address */
} __attribute__((packed));

/*
 * Pointer IDT (6 byte)
 * Struktur untuk instruksi LIDT.
 */
struct idt_pointer {
    uint16_t batas;             /* Ukuran IDT minus 1 */
    uint32_t basis;             /* Alamat IDT */
} __attribute__((packed));

/*
 * Struktur stack frame saat interrupt
 * Diteruskan ke handler C
 */
struct int_frame {
    uint32_t gs, fs, es, ds;                            /* Segment */
    uint32_t edi, esi, ebp, esp_kolon, ebx, edx, ecx, eax;  /* Registers */
    uint32_t int_no, err_code;                          /* Interrupt info */
    uint32_t eip, cs, eflags, esp, ss;                  /* Saved by CPU */
} __attribute__((packed));

/* Tipe handler interrupt */
typedef void (*handler_interrupt_t)(struct int_frame *frame);

/* =============================================================================
 * VARIABEL GLOBAL
 * =============================================================================
 */

/* Tabel IDT */
static struct idt_entry g_tabel_idt[IDT_JUMLAH_ENTRY];

/* Pointer IDT untuk LIDT */
static struct idt_pointer g_pointer_idt;

/* Array handler interrupt */
static handler_interrupt_t g_handler[IDT_JUMLAH_ENTRY];

/* Nama exception untuk pesan error */
static const char *g_nama_exception[] = {
    "Divide Error",              /* 0 */
    "Debug Exception",           /* 1 */
    "NMI Interrupt",             /* 2 */
    "Breakpoint",                /* 3 */
    "Overflow",                  /* 4 */
    "BOUND Range Exceeded",      /* 5 */
    "Invalid Opcode",            /* 6 */
    "Device Not Available",      /* 7 */
    "Double Fault",              /* 8 */
    "Coprocessor Segment Overrun", /* 9 */
    "Invalid TSS",               /* 10 */
    "Segment Not Present",       /* 11 */
    "Stack-Segment Fault",       /* 12 */
    "General Protection",        /* 13 */
    "Page Fault",                /* 14 */
    "Reserved",                  /* 15 */
    "x87 FPU Error",             /* 16 */
    "Alignment Check",           /* 17 */
    "Machine Check",             /* 18 */
    "SIMD Exception"             /* 19 */
};

/* =============================================================================
 * FUNGSI INTERNAL
 * =============================================================================
 */

/*
 * _atur_entry_idt
 * ---------------
 * Mengisi satu entry IDT dengan nilai yang diberikan.
 *
 * Parameter:
 *   index    - Index entry dalam IDT (0-255)
 *   handler  - Alamat handler interrupt
 *   selector - Code segment selector
 *   flags    - Flag deskriptor (type, DPL, present)
 */
static void _atur_entry_idt(uint8_t index, uint32_t handler,
                            uint16_t selector, uint8_t flags)
{
    struct idt_entry *entry;

    entry = &g_tabel_idt[index];

    /* Set offset handler */
    entry->offset_rendah = (uint16_t)(handler & 0xFFFF);
    entry->offset_atas = (uint16_t)((handler >> 16) & 0xFFFF);

    /* Set selector */
    entry->selector = selector;

    /* Set flags */
    entry->flags = flags;

    /* Reserved field */
    entry->nol = 0;
}

/*
 * _handler_exception_default
 * ---------------------------
 * Handler default untuk exception CPU yang tidak memiliki handler khusus.
 * Menampilkan informasi exception dan halt sistem.
 *
 * Parameter:
 *   frame - Stack frame saat interrupt terjadi
 */
static void _handler_exception_default(struct int_frame *frame)
{
    const char *nama;
    uint8_t no_exception;

    no_exception = (uint8_t)frame->int_no;

    /* Ambil nama exception */
    if (no_exception < 20) {
        nama = g_nama_exception[no_exception];
    } else {
        nama = "Unknown Exception";
    }

    /* Tampilkan pesan error via VGA console */
    /* Format: "EXCEPTION: [nama] (nomor) at EIP=[alamat]" */
    
    /* Untuk sekarang, halt sistem */
    /* Implementasi console akan ditambahkan kemudian */
    
    __asm__ __volatile__(
        "cli\n\t"
        "hlt\n\t"
    );

    /* Supaya compiler tidak complain */
    (void)nama;
}

/*
 * _handler_irq_default
 * --------------------
 * Handler default untuk IRQ yang tidak memiliki handler khusus.
 *
 * Parameter:
 *   frame - Stack frame saat interrupt terjadi
 */
static void _handler_irq_default(struct int_frame *frame)
{
    uint8_t irq_no;

    irq_no = (uint8_t)(frame->int_no - IDT_VEKTOR_IRQ0);

    /* Acknowledge IRQ ke PIC */
    /* Untuk IRQ 8-15, perlu acknowledge ke slave PIC */
    if (irq_no >= 8) {
        outb(0x20, 0xA0);           /* EOI ke slave PIC */
    }
    outb(0x20, 0x20);               /* EOI ke master PIC */

    (void)irq_no;
}

/* =============================================================================
 * FUNGSI PUBLIC
 * =============================================================================
 */

/*
 * idt_init
 * --------
 * Menginisialisasi Interrupt Descriptor Table.
 *
 * Fungsi ini membuat semua entry IDT dengan handler default dan
 * memuat IDT ke CPU.
 *
 * Return:
 *   0 jika berhasil, nilai negatif jika gagal
 */
int idt_init(void)
{
    int i;

    /* Reset semua handler */
    for (i = 0; i < IDT_JUMLAH_ENTRY; i++) {
        g_handler[i] = (handler_interrupt_t)0;
    }

    /* Inisialisasi semua entry IDT dengan handler default */
    for (i = 0; i < IDT_JUMLAH_ENTRY; i++) {
        if (i < 32) {
            /* Exception handlers - interrupt gate, DPL 0 */
            _atur_entry_idt((uint8_t)i,
                           (uint32_t)(uintptr_t)_handler_exception_default,
                           IDT_SELECTOR_KODE,
                           IDT_FLAG_PRESENT | IDT_FLAG_DPL0 |
                           IDT_FLAG_GATETYPE_INT);
        } else if (i < 48) {
            /* IRQ handlers - interrupt gate, DPL 0 */
            _atur_entry_idt((uint8_t)i,
                           (uint32_t)(uintptr_t)_handler_irq_default,
                           IDT_SELECTOR_KODE,
                           IDT_FLAG_PRESENT | IDT_FLAG_DPL0 |
                           IDT_FLAG_GATETYPE_INT);
        } else if (i == IDT_VEKTOR_SYSCALL) {
            /* Syscall - trap gate, DPL 3 (user dapat memanggil) */
            _atur_entry_idt((uint8_t)i,
                           (uint32_t)(uintptr_t)_handler_irq_default,
                           IDT_SELECTOR_KODE,
                           IDT_FLAG_PRESENT | IDT_FLAG_DPL3 |
                           IDT_FLAG_GATETYPE_TRAP);
        } else {
            /* Software interrupts - interrupt gate, DPL 0 */
            _atur_entry_idt((uint8_t)i,
                           (uint32_t)(uintptr_t)_handler_irq_default,
                           IDT_SELECTOR_KODE,
                           IDT_FLAG_PRESENT | IDT_FLAG_DPL0 |
                           IDT_FLAG_GATETYPE_INT);
        }
    }

    /* Setup pointer IDT */
    g_pointer_idt.batas = sizeof(g_tabel_idt) - 1;
    g_pointer_idt.basis = (uint32_t)(uintptr_t)&g_tabel_idt;

    /* Load IDT */
    __asm__ __volatile__(
        "lidt %0\n\t"
        :
        : "m"(g_pointer_idt)
        : "memory"
    );

    return 0;
}

/*
 * idt_set_handler
 * ---------------
 * Mengatur handler untuk vektor interrupt tertentu.
 *
 * Parameter:
 *   vektor  - Nomor vektor interrupt (0-255)
 *   handler - Pointer ke fungsi handler
 *
 * Return:
 *   0 jika berhasil, -1 jika vektor tidak valid
 */
int idt_set_handler(uint8_t vektor, handler_interrupt_t handler)
{
    /* vektor adalah uint8_t, selalu < 256, jadi valid */
    (void)vektor;

    g_handler[vektor] = handler;

    /* Update entry IDT dengan handler baru */
    _atur_entry_idt(vektor,
                   (uint32_t)(uintptr_t)handler,
                   IDT_SELECTOR_KODE,
                   g_tabel_idt[vektor].flags);

    return 0;
}

/*
 * idt_get_handler
 * ---------------
 * Mendapatkan handler untuk vektor interrupt tertentu.
 *
 * Parameter:
 *   vektor - Nomor vektor interrupt (0-255)
 *
 * Return:
 *   Pointer ke handler, atau NULL jika tidak ada
 */
handler_interrupt_t idt_get_handler(uint8_t vektor)
{
    /* vektor adalah uint8_t, selalu < 256, jadi valid */
    (void)vektor;

    return g_handler[vektor];
}

/*
 * idt_enable_interrupt
 * --------------------
 * Enable interrupt (sti).
 */
void idt_enable_interrupt(void)
{
    __asm__ __volatile__("sti\n\t");
}

/*
 * idt_disable_interrupt
 * ---------------------
 * Disable interrupt (cli).
 */
void idt_disable_interrupt(void)
{
    __asm__ __volatile__("cli\n\t");
}

/*
 * idt_get_pointer
 * ---------------
 * Mendapatkan pointer ke struktur IDT pointer.
 *
 * Return:
 *   Pointer ke idt_pointer
 */
const struct idt_pointer *idt_get_pointer(void)
{
    return &g_pointer_idt;
}

/*
 * idt_set_gate
 * ------------
 * Mengatur interrupt gate dengan parameter lengkap.
 *
 * Parameter:
 *   vektor   - Nomor vektor interrupt
 *   handler  - Alamat handler
 *   selector - Code segment selector
 *   dpl      - Privilege level (0-3)
 *
 * Return:
 *   0 jika berhasil, -1 jika gagal
 */
int idt_set_gate(uint8_t vektor, uint32_t handler,
                 uint16_t selector, uint8_t dpl)
{
    uint8_t flags;

    /* vektor adalah uint8_t, selalu < 256, jadi valid */
    (void)vektor;

    /* Build flags */
    flags = IDT_FLAG_PRESENT | IDT_FLAG_GATETYPE_INT;
    flags |= (dpl & 0x03) << 5;

    _atur_entry_idt(vektor, handler, selector, flags);

    return 0;
}
