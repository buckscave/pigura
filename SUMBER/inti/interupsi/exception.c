/*
 * PIGURA OS - EXCEPTION.C
 * -----------------------
 * Implementasi handler exception CPU.
 *
 * Berkas ini berisi implementasi handler untuk berbagai exception
 * CPU seperti divide error, page fault, general protection fault, dll.
 *
 * Versi: 1.1
 * Tanggal: 2025
 */

#include "../kernel.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* Exception types */
#define EXCEPTION_TYPE_FAULT    0
#define EXCEPTION_TYPE_TRAP     1
#define EXCEPTION_TYPE_ABORT    2
#define EXCEPTION_TYPE_INTERRUPT 3

/* Exception information */
typedef struct {
    const char *nama;
    const char *deskripsi;
    tak_bertanda8_t tipe;
    bool_t has_error_code;
} exception_info_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Informasi lengkap tentang setiap exception */
static const exception_info_t exception_table[] = {
    /* 0 - Divide Error */
    {
        "Divide Error",
        "Division by zero or overflow",
        EXCEPTION_TYPE_FAULT,
        SALAH
    },
    /* 1 - Debug Exception */
    {
        "Debug Exception",
        "Debug trap or breakpoint",
        EXCEPTION_TYPE_FAULT,
        SALAH
    },
    /* 2 - NMI */
    {
        "Non-Maskable Interrupt",
        "Hardware non-maskable interrupt",
        EXCEPTION_TYPE_INTERRUPT,
        SALAH
    },
    /* 3 - Breakpoint */
    {
        "Breakpoint",
        "INT 3 instruction executed",
        EXCEPTION_TYPE_TRAP,
        SALAH
    },
    /* 4 - Overflow */
    {
        "Overflow",
        "INTO instruction with OF flag set",
        EXCEPTION_TYPE_TRAP,
        SALAH
    },
    /* 5 - BOUND Range Exceeded */
    {
        "BOUND Range Exceeded",
        "Array index out of bounds",
        EXCEPTION_TYPE_FAULT,
        SALAH
    },
    /* 6 - Invalid Opcode */
    {
        "Invalid Opcode",
        "Undefined or invalid instruction",
        EXCEPTION_TYPE_FAULT,
        SALAH
    },
    /* 7 - Device Not Available */
    {
        "Device Not Available",
        "FPU or SIMD instruction without FPU",
        EXCEPTION_TYPE_FAULT,
        SALAH
    },
    /* 8 - Double Fault */
    {
        "Double Fault",
        "Exception during exception handling",
        EXCEPTION_TYPE_ABORT,
        BENAR
    },
    /* 9 - Coprocessor Segment Overrun */
    {
        "Coprocessor Segment Overrun",
        "FPU operand outside segment limit",
        EXCEPTION_TYPE_ABORT,
        SALAH
    },
    /* 10 - Invalid TSS */
    {
        "Invalid TSS",
        "Task switch with invalid TSS",
        EXCEPTION_TYPE_FAULT,
        BENAR
    },
    /* 11 - Segment Not Present */
    {
        "Segment Not Present",
        "Segment descriptor P bit clear",
        EXCEPTION_TYPE_FAULT,
        BENAR
    },
    /* 12 - Stack-Segment Fault */
    {
        "Stack-Segment Fault",
        "Stack segment limit violation",
        EXCEPTION_TYPE_FAULT,
        BENAR
    },
    /* 13 - General Protection Fault */
    {
        "General Protection Fault",
        "Protection violation",
        EXCEPTION_TYPE_FAULT,
        BENAR
    },
    /* 14 - Page Fault */
    {
        "Page Fault",
        "Page not present or protection",
        EXCEPTION_TYPE_FAULT,
        BENAR
    },
    /* 15 - Reserved */
    {
        "Reserved",
        "Reserved exception",
        EXCEPTION_TYPE_FAULT,
        SALAH
    },
    /* 16 - x87 FPU Error */
    {
        "x87 FPU Error",
        "x87 FPU floating-point error",
        EXCEPTION_TYPE_FAULT,
        SALAH
    },
    /* 17 - Alignment Check */
    {
        "Alignment Check",
        "Misaligned memory access",
        EXCEPTION_TYPE_FAULT,
        BENAR
    },
    /* 18 - Machine Check */
    {
        "Machine Check",
        "Internal hardware error",
        EXCEPTION_TYPE_ABORT,
        SALAH
    },
    /* 19 - SIMD Exception */
    {
        "SIMD Exception",
        "SSE/SIMD floating-point error",
        EXCEPTION_TYPE_FAULT,
        SALAH
    },
    /* 20 - Virtualization Exception */
    {
        "Virtualization Exception",
        "VM exit due to virtualization",
        EXCEPTION_TYPE_FAULT,
        SALAH
    },
    /* 21 - Control Protection Exception */
    {
        "Control Protection Exception",
        "Control-flow protection violation",
        EXCEPTION_TYPE_FAULT,
        BENAR
    },
    /* 22-31 - Reserved */
    {
        "Reserved",
        "Reserved exception",
        EXCEPTION_TYPE_FAULT,
        SALAH
    }
};

/* Counter untuk setiap exception */
static tak_bertanda64_t exception_counter[32] = {0};

/* Flag untuk page fault handling */
static bool_t page_fault_handler_active = SALAH;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * get_exception_info
 * ------------------
 * Dapatkan informasi exception.
 *
 * Parameter:
 *   vector - Nomor vektor exception
 *
 * Return: Pointer ke informasi exception
 */
static const exception_info_t *get_exception_info(tak_bertanda32_t vector)
{
    static const exception_info_t unknown = {
        "Unknown",
        "Unknown exception",
        EXCEPTION_TYPE_FAULT,
        SALAH
    };

    if (vector < 32) {
        return &exception_table[vector];
    }

    return &unknown;
}

/*
 * decode_page_fault_error
 * -----------------------
 * Decode error code page fault.
 *
 * Parameter:
 *   error_code - Error code dari CPU
 *   cr2        - Alamat faulting
 */
static void decode_page_fault_error(tak_bertanda32_t error_code,
                                    tak_bertanda32_t cr2)
{
    kernel_printf("  Page Fault Analysis:\n");
    kernel_printf("    Faulting Address: 0x%08lX\n",
                  (ukuran_t)cr2);
    kernel_printf("    Page Present: %s\n",
                  (error_code & 0x01) ? "Ya" : "Tidak");
    kernel_printf("    Operation: %s\n",
                  (error_code & 0x02) ? "Write" : "Read");
    kernel_printf("    Privilege: %s mode\n",
                  (error_code & 0x04) ? "User" : "Kernel");
    kernel_printf("    Reserved Bit: %s\n",
                  (error_code & 0x08) ? "Set" : "Clear");

    if (error_code & 0x10) {
        kernel_printf("    Execute: Ya (instruction fetch)\n");
    }
}

/*
 * decode_segment_error
 * --------------------
 * Decode error code segment-related exception.
 *
 * Parameter:
 *   error_code - Error code dari CPU
 */
static void decode_segment_error(tak_bertanda32_t error_code)
{
    kernel_printf("  Segment Error Analysis:\n");

    if (error_code == 0) {
        kernel_printf("    Not segment related\n");
        return;
    }

    kernel_printf("    External Event: %s\n",
                  (error_code & 0x01) ? "Ya" : "Tidak");

    kernel_printf("    Table: ");
    switch ((error_code >> 1) & 0x03) {
        case 0: kernel_printf("GDT\n"); break;
        case 1: kernel_printf("IDT\n"); break;
        case 2: kernel_printf("LDT\n"); break;
        case 3: kernel_printf("IDT\n"); break;
    }

    kernel_printf("    Selector Index: %lu\n",
                  (ukuran_t)(error_code >> 3));
}

/*
 * ============================================================================
 * FUNGSI HANDLER KHUSUS (SPECIALIZED HANDLERS)
 * ============================================================================
 */

/*
 * exception_divide_error_handler
 * ------------------------------
 * Handler untuk divide error (vector 0).
 */
void exception_divide_error_handler(register_context_t *ctx)
{
    exception_counter[0]++;

    kernel_printf("\n[EXCEPTION] Divide Error (#DE)\n");
    kernel_printf("  EIP: 0x%08lX\n", (ukuran_t)ctx->eip);

    /* Coba recovery atau panic */
    kernel_panic_exception(ctx);
}

/*
 * exception_invalid_opcode_handler
 * --------------------------------
 * Handler untuk invalid opcode (vector 6).
 */
void exception_invalid_opcode_handler(register_context_t *ctx)
{
    tak_bertanda8_t opcode;
    tak_bertanda8_t *eip_ptr;

    exception_counter[6]++;

    kernel_printf("\n[EXCEPTION] Invalid Opcode (#UD)\n");
    kernel_printf("  EIP: 0x%08lX\n", (ukuran_t)ctx->eip);

    /* Cast melalui uintptr_t untuk menghindari warning */
    eip_ptr = (tak_bertanda8_t *)(uintptr_t)ctx->eip;
    opcode = *eip_ptr;
    kernel_printf("  Opcode byte: 0x%02X\n", opcode);

    kernel_panic_exception(ctx);
}

/*
 * exception_double_fault_handler
 * ------------------------------
 * Handler untuk double fault (vector 8).
 */
void exception_double_fault_handler(register_context_t *ctx)
{
    exception_counter[8]++;

    kernel_printf("\n[EXCEPTION] Double Fault (#DF) - KRITIKAL!\n");
    decode_segment_error(ctx->err_code);

    /* Double fault selalu fatal */
    kernel_panic_exception(ctx);
}

/*
 * exception_gpf_handler
 * ---------------------
 * Handler untuk general protection fault (vector 13).
 */
void exception_gpf_handler(register_context_t *ctx)
{
    exception_counter[13]++;

    kernel_printf("\n[EXCEPTION] General Protection Fault (#GP)\n");
    kernel_printf("  EIP: 0x%08lX\n", (ukuran_t)ctx->eip);

    if (ctx->err_code != 0) {
        decode_segment_error(ctx->err_code);
    }

    kernel_panic_exception(ctx);
}

/*
 * exception_page_fault_handler
 * ----------------------------
 * Handler untuk page fault (vector 14).
 */
void exception_page_fault_handler(register_context_t *ctx)
{
    tak_bertanda32_t cr2;

    exception_counter[14]++;

    cr2 = cpu_read_cr2();

    kernel_printf("\n[EXCEPTION] Page Fault (#PF)\n");
    decode_page_fault_error(ctx->err_code, cr2);
    kernel_printf("  EIP: 0x%08lX\n", (ukuran_t)ctx->eip);

    /* Cek apakah ini recoverable */
    if (page_fault_handler_active) {
        kernel_printf("  Nested page fault - panic!\n");
        kernel_panic_exception(ctx);
        return;
    }

    /* Jika di kernel mode, ini error fatal */
    if (!(ctx->err_code & 0x04)) {
        kernel_printf("  Kernel mode page fault - panic!\n");
        kernel_panic_exception(ctx);
        return;
    }

    /* TODO: Handle user mode page fault dengan demand paging */

    kernel_panic_exception(ctx);
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * exception_init
 * --------------
 * Inisialisasi exception handler.
 */
status_t exception_init(void)
{
    tak_bertanda32_t i;

    /* Reset counter */
    for (i = 0; i < 32; i++) {
        exception_counter[i] = 0;
    }

    /* Register handler khusus */
    isr_set_handler(0, exception_divide_error_handler);
    isr_set_handler(6, exception_invalid_opcode_handler);
    isr_set_handler(8, exception_double_fault_handler);
    isr_set_handler(13, exception_gpf_handler);
    isr_set_handler(14, exception_page_fault_handler);

    kernel_printf("[EXCEPTION] Exception handlers initialized\n");

    return STATUS_BERHASIL;
}

/*
 * exception_get_counter
 * ---------------------
 * Dapatkan counter exception.
 *
 * Parameter:
 *   vector - Nomor vektor exception
 *
 * Return: Jumlah exception yang terjadi
 */
tak_bertanda64_t exception_get_counter(tak_bertanda32_t vector)
{
    if (vector >= 32) {
        return 0;
    }

    return exception_counter[vector];
}

/*
 * exception_reset_counter
 * -----------------------
 * Reset counter exception.
 *
 * Parameter:
 *   vector - Nomor vektor exception
 */
void exception_reset_counter(tak_bertanda32_t vector)
{
    if (vector < 32) {
        exception_counter[vector] = 0;
    }
}

/*
 * exception_print_stats
 * ---------------------
 * Print statistik exception.
 */
void exception_print_stats(void)
{
    tak_bertanda32_t i;

    kernel_printf("\nException Statistics:\n");
    kernel_printf("========================================\n");

    for (i = 0; i < 32; i++) {
        if (exception_counter[i] > 0) {
            const exception_info_t *info = get_exception_info(i);
            kernel_printf("  #%02lu %-25s: %lu\n",
                          (ukuran_t)i, info->nama,
                          (ukuran_t)exception_counter[i]);
        }
    }

    kernel_printf("========================================\n");
}

/*
 * exception_get_info
 * ------------------
 * Dapatkan informasi exception.
 *
 * Parameter:
 *   vector - Nomor vektor exception
 *
 * Return: Pointer ke informasi exception
 */
const exception_info_t *exception_get_info(tak_bertanda32_t vector)
{
    return get_exception_info(vector);
}

/*
 * exception_is_recoverable
 * ------------------------
 * Cek apakah exception bisa di-recover.
 *
 * Parameter:
 *   vector - Nomor vektor exception
 *
 * Return: BENAR jika recoverable
 */
bool_t exception_is_recoverable(tak_bertanda32_t vector)
{
    const exception_info_t *info = get_exception_info(vector);

    /* Fault bisa di-recover (dalam teori), trap bisa, abort tidak */
    switch (info->tipe) {
        case EXCEPTION_TYPE_FAULT:
        case EXCEPTION_TYPE_TRAP:
            return BENAR;
        default:
            return SALAH;
    }
}

/*
 * exception_has_error_code
 * ------------------------
 * Cek apakah exception memiliki error code.
 *
 * Parameter:
 *   vector - Nomor vektor exception
 *
 * Return: BENAR jika memiliki error code
 */
bool_t exception_has_error_code(tak_bertanda32_t vector)
{
    const exception_info_t *info = get_exception_info(vector);
    return info->has_error_code;
}
