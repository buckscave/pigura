/*
 * PIGURA OS - EXCEPTION x86
 * -------------------------
 * Implementasi penanganan exception CPU untuk arsitektur x86.
 *
 * Berkas ini berisi handler untuk semua exception CPU seperti
 * divide error, page fault, general protection, dll.
 *
 * Arsitektur: x86 (32-bit)
 * Versi: 1.0
 */

#include "../../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA EXCEPTION
 * ============================================================================
 */

/* Jumlah exception CPU */
#define EXCEPTION_JUMLAH       20

/* Error code flags */
#define ERR_EXT                0x01
#define ERR_IDT                0x02
#define ERR_TI                 0x04
#define ERR_TABLE_MASK         0x03

/* Page fault flags */
#define PF_PRESENT             0x01
#define PF_WRITE               0x02
#define PF_USER                0x04
#define PF_RSVD                0x08
#define PF_INSTR               0x10

/*
 * ============================================================================
 * STRUKTUR DATA
 * ============================================================================
 */

/* Interrupt frame untuk exception handling */
struct int_frame {
    tak_bertanda32_t gs;            /* GS segment */
    tak_bertanda32_t fs;            /* FS segment */
    tak_bertanda32_t es;            /* ES segment */
    tak_bertanda32_t ds;            /* DS segment */
    tak_bertanda32_t edi;           /* Register edi */
    tak_bertanda32_t esi;           /* Register esi */
    tak_bertanda32_t ebp;           /* Register ebp */
    tak_bertanda32_t esp_kolon;     /* ESP yang disimpan */
    tak_bertanda32_t ebx;           /* Register ebx */
    tak_bertanda32_t edx;           /* Register edx */
    tak_bertanda32_t ecx;           /* Register ecx */
    tak_bertanda32_t eax;           /* Register eax */
    tak_bertanda32_t int_no;        /* Nomor interrupt */
    tak_bertanda32_t err_code;      /* Kode error */
    tak_bertanda32_t eip;           /* Instruction pointer */
    tak_bertanda32_t cs;            /* Code segment */
    tak_bertanda32_t eflags;        /* Flags register */
    tak_bertanda32_t esp;           /* Stack pointer user */
    tak_bertanda32_t ss;            /* Stack segment */
} __attribute__((packed));

/*
 * ============================================================================
 * VARIABEL GLOBAL
 * ============================================================================
 */

/* Nama exception */
static const char *g_nama_exception[] = {
    "Divide Error",
    "Debug Exception",
    "NMI Interrupt",
    "Breakpoint",
    "Overflow",
    "BOUND Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection",
    "Page Fault",
    "Reserved",
    "x87 FPU Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Exception"
};

/* Counter exception */
static tak_bertanda64_t g_exception_counter[EXCEPTION_JUMLAH] = { 0 };

/* Handler khusus */
static void (*g_exception_handler[EXCEPTION_JUMLAH])
    (struct int_frame *) = { NULL };

/*
 * ============================================================================
 * FUNGSI INTERNAL
 * ============================================================================
 */

/*
 * _print_frame
 * ------------
 * Menampilkan isi interrupt frame.
 *
 * Parameter:
 *   frame - Pointer ke interrupt frame
 */
static void _print_frame(struct int_frame *frame)
{
    kernel_printf("\n");
    kernel_printf("    GS=%04x  FS=%04x  ES=%04x  DS=%04x\n",
        frame->gs, frame->fs, frame->es, frame->ds);
    kernel_printf("    EDI=%08x ESI=%08x EBP=%08x ESP=%08x\n",
        frame->edi, frame->esi, frame->ebp, frame->esp_kolon);
    kernel_printf("    EBX=%08x EDX=%08x ECX=%08x EAX=%08x\n",
        frame->ebx, frame->edx, frame->ecx, frame->eax);
    kernel_printf("    EIP=%08x CS=%04x EFLAGS=%08x\n",
        frame->eip, frame->cs, frame->eflags);
    kernel_printf("    ESP=%08x SS=%04x\n",
        frame->esp, frame->ss);

    if (frame->err_code != 0) {
        kernel_printf("    Error Code=%08x\n", frame->err_code);
    }
}

/*
 * _print_page_fault_info
 * ----------------------
 * Menampilkan informasi page fault.
 *
 * Parameter:
 *   frame - Pointer ke interrupt frame
 */
static void _print_page_fault_info(struct int_frame *frame)
{
    tak_bertanda32_t cr2;
    tak_bertanda32_t err;

    /* Baca alamat yang menyebabkan fault */
    cr2 = cpu_read_cr2();
    err = frame->err_code;

    kernel_printf("    Cr2=%08x (alamat yang diakses)\n", cr2);
    kernel_printf("    ");

    if (err & PF_PRESENT) {
        kernel_printf("PRESENT ");
    } else {
        kernel_printf("NOT-PRESENT ");
    }

    if (err & PF_WRITE) {
        kernel_printf("WRITE ");
    } else {
        kernel_printf("READ ");
    }

    if (err & PF_USER) {
        kernel_printf("USER-MODE ");
    } else {
        kernel_printf("KERNEL-MODE ");
    }

    if (err & PF_INSTR) {
        kernel_printf("INSTRUCTION-FETCH ");
    }

    kernel_printf("\n");
}

/*
 * _print_gpf_info
 * ---------------
 * Menampilkan informasi general protection fault.
 *
 * Parameter:
 *   frame - Pointer ke interrupt frame
 */
static void _print_gpf_info(struct int_frame *frame)
{
    tak_bertanda32_t err;

    err = frame->err_code;

    if (err != 0) {
        kernel_printf("    ");

        if (err & ERR_EXT) {
            kernel_printf("EXTERNAL ");
        }

        if (err & ERR_IDT) {
            kernel_printf("IDT ");
        } else if (err & ERR_TI) {
            kernel_printf("LDT ");
        } else {
            kernel_printf("GDT ");
        }

        kernel_printf("Selector=%u\n", err >> 3);
    }
}

/*
 * ============================================================================
 * FUNGSI PUBLIC
 * ============================================================================
 */

/*
 * exception_handler_umum
 * ----------------------
 * Handler umum untuk exception yang dipanggil dari assembly.
 *
 * Parameter:
 *   frame - Pointer ke interrupt frame
 */
void exception_handler_umum(struct int_frame *frame)
{
    tak_bertanda32_t nomor;

    nomor = frame->int_no;

    /* Increment counter */
    if (nomor < EXCEPTION_JUMLAH) {
        g_exception_counter[nomor]++;
    }

    /* Panggil handler khusus jika ada */
    if (nomor < EXCEPTION_JUMLAH && 
        g_exception_handler[nomor] != NULL) {
        g_exception_handler[nomor](frame);
        return;
    }

    /* Handler default */
    kernel_printf("\n");
    kernel_set_color(VGA_MERAH_TERANG, VGA_HITAM);
    kernel_printf("========================================");
    kernel_printf("========================================\n");
    kernel_printf("  EXCEPTION: %s (#%u)\n",
        (nomor < EXCEPTION_JUMLAH) ? g_nama_exception[nomor] : "Unknown",
        nomor);
    kernel_printf("========================================");
    kernel_printf("========================================\n");
    kernel_set_color(VGA_PUTIH, VGA_HITAM);

    _print_frame(frame);

    /* Info khusus untuk page fault */
    if (nomor == VEKTOR_PF) {
        _print_page_fault_info(frame);
    }

    /* Info khusus untuk GPF */
    if (nomor == VEKTOR_GP) {
        _print_gpf_info(frame);
    }

    kernel_printf("\n");
    kernel_printf("Sistem dihentikan.\n");

    /* Halt */
    while (1) {
        cpu_halt();
    }
}

/*
 * exception_set_handler
 * ---------------------
 * Mengatur handler untuk exception tertentu.
 *
 * Parameter:
 *   nomor   - Nomor exception (0-19)
 *   handler - Pointer ke fungsi handler
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t exception_set_handler(tak_bertanda32_t nomor,
                                void (*handler)(struct int_frame *))
{
    if (nomor >= EXCEPTION_JUMLAH) {
        return STATUS_PARAM_INVALID;
    }

    g_exception_handler[nomor] = handler;

    return STATUS_BERHASIL;
}

/*
 * exception_hapus_handler
 * -----------------------
 * Menghapus handler exception.
 *
 * Parameter:
 *   nomor - Nomor exception (0-19)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t exception_hapus_handler(tak_bertanda32_t nomor)
{
    if (nomor >= EXCEPTION_JUMLAH) {
        return STATUS_PARAM_INVALID;
    }

    g_exception_handler[nomor] = NULL;

    return STATUS_BERHASIL;
}

/*
 * exception_get_nama
 * ------------------
 * Mendapatkan nama exception.
 *
 * Parameter:
 *   nomor - Nomor exception (0-19)
 *
 * Return:
 *   String nama exception
 */
const char *exception_get_nama(tak_bertanda32_t nomor)
{
    if (nomor >= EXCEPTION_JUMLAH) {
        return "Unknown";
    }

    return g_nama_exception[nomor];
}

/*
 * exception_get_counter
 * ---------------------
 * Mendapatkan counter exception.
 *
 * Parameter:
 *   nomor - Nomor exception (0-19)
 *
 * Return:
 *   Jumlah exception
 */
tak_bertanda64_t exception_get_counter(tak_bertanda32_t nomor)
{
    if (nomor >= EXCEPTION_JUMLAH) {
        return 0;
    }

    return g_exception_counter[nomor];
}

/*
 * exception_dump_status
 * ---------------------
 * Menampilkan status exception.
 */
void exception_dump_status(void)
{
    tak_bertanda32_t i;

    kernel_printf("[EXCEPTION] Status Exception:\n");

    for (i = 0; i < EXCEPTION_JUMLAH; i++) {
        if (g_exception_counter[i] > 0) {
            kernel_printf("[EXCEPTION]   #%02u %-25s: %u\n",
                i, g_nama_exception[i],
                (tak_bertanda32_t)g_exception_counter[i]);
        }
    }
}

/*
 * page_fault_handler_default
 * --------------------------
 * Handler default untuk page fault.
 *
 * Parameter:
 *   frame - Pointer ke interrupt frame
 */
void page_fault_handler_default(struct int_frame *frame)
{
    alamat_virtual_t addr;

    addr = (alamat_virtual_t)cpu_read_cr2();

    kernel_printf("\n[PAGE FAULT] Alamat: 0x%08x\n", addr);

    if (frame->err_code & PF_PRESENT) {
        kernel_printf("[PAGE FAULT] Halaman ada tapi tidak bisa diakses\n");
    } else {
        kernel_printf("[PAGE FAULT] Halaman tidak ada\n");
    }

    if (frame->err_code & PF_WRITE) {
        kernel_printf("[PAGE FAULT] Operasi: Tulis\n");
    } else {
        kernel_printf("[PAGE FAULT] Operasi: Baca\n");
    }

    if (frame->err_code & PF_USER) {
        kernel_printf("[PAGE FAULT] Mode: User\n");
    } else {
        kernel_printf("[PAGE FAULT] Mode: Kernel\n");
    }

    _print_frame(frame);

    while (1) {
        cpu_halt();
    }
}

/*
 * double_fault_handler_default
 * ---------------------------
 * Handler default untuk double fault.
 *
 * Parameter:
 *   frame - Pointer ke interrupt frame
 */
void double_fault_handler_default(struct int_frame *frame)
{
    kernel_printf("\n[DOUBLE FAULT] Kerusakan sistem kritis!\n");
    _print_frame(frame);

    while (1) {
        cpu_halt();
    }
}

/*
 * gpf_handler_default
 * -------------------
 * Handler default untuk general protection fault.
 *
 * Parameter:
 *   frame - Pointer ke interrupt frame
 */
void gpf_handler_default(struct int_frame *frame)
{
    kernel_printf("\n[GENERAL PROTECTION FAULT]\n");
    _print_frame(frame);
    _print_gpf_info(frame);

    while (1) {
        cpu_halt();
    }
}
