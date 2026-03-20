/*
 * PIGURA OS - ISR.C
 * -----------------
 * Implementasi Interrupt Service Routines.
 *
 * Berkas ini berisi implementasi ISR untuk exception CPU dan
 * mekanisme dispatch ke handler yang sesuai.
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

/* Exception names */
static const char *exception_names[] = {
    "Divide Error",
    "Debug Exception",
    "Non-Maskable Interrupt",
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
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 FPU Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Security Exception",
    "Reserved"
};

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Counter untuk setiap exception */
static tak_bertanda64_t exception_count[32] = {0};

/* Handler kustom untuk setiap exception */
static void (*exception_handlers[32])(struct register_context *) = {NULL};

/* Flag untuk tracking nested exceptions */
static volatile tak_bertanda32_t nested_depth = 0;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * exception_get_name
 * ------------------
 * Dapatkan nama exception dari nomor vektor.
 *
 * Parameter:
 *   vector - Nomor vektor exception
 *
 * Return: String nama exception
 */
static const char *exception_get_name(tak_bertanda32_t vector)
{
    if (vector >= 32) {
        return "Unknown Exception";
    }
    return exception_names[vector];
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
static bool_t exception_has_error_code(tak_bertanda32_t vector)
{
    /* Exception dengan error code: 8, 10-14, 17, 21, 29, 30 */
    switch (vector) {
        case 8:   /* Double Fault */
        case 10:  /* Invalid TSS */
        case 11:  /* Segment Not Present */
        case 12:  /* Stack-Segment Fault */
        case 13:  /* General Protection Fault */
        case 14:  /* Page Fault */
        case 17:  /* Alignment Check */
        case 21:  /* Control Protection Exception */
        case 29:  /* VMM Communication Exception */
        case 30:  /* Security Exception */
            return BENAR;
        default:
            return SALAH;
    }
}

/*
 * exception_print_context
 * -----------------------
 * Print konteks exception untuk debugging.
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
static void exception_print_context(const register_context_t *ctx)
{
    kernel_printf("\nException Context:\n");
    kernel_printf("  Exception: %s (#%lu)\n",
                  exception_get_name(ctx->int_no), ctx->int_no);

    if (exception_has_error_code(ctx->int_no)) {
        kernel_printf("  Error Code: 0x%08lX\n", ctx->err_code);
    }

    kernel_printf("\nRegisters:\n");
    kernel_printf("  EAX=0x%08lX  EBX=0x%08lX  ECX=0x%08lX  EDX=0x%08lX\n",
                  ctx->eax, ctx->ebx, ctx->ecx, ctx->edx);
    kernel_printf("  ESI=0x%08lX  EDI=0x%08lX  EBP=0x%08lX  ESP=0x%08lX\n",
                  ctx->esi, ctx->edi, ctx->ebp, ctx->esp);
    kernel_printf("  EIP=0x%08lX  EFLAGS=0x%08lX\n", ctx->eip, ctx->eflags);
    kernel_printf("  CS=0x%04lX  DS=0x%04lX  ES=0x%04lX  FS=0x%04lX  GS=0x%04lX  SS=0x%04lX\n",
                  ctx->cs, ctx->ds, ctx->es, ctx->fs, ctx->gs, ctx->ss);
}

/*
 * exception_decode_error_code
 * ---------------------------
 * Decode error code untuk exception tertentu.
 *
 * Parameter:
 *   vector    - Nomor vektor exception
 *   err_code  - Error code
 */
static void exception_decode_error_code(tak_bertanda32_t vector,
                                        tak_bertanda32_t err_code)
{
    switch (vector) {
        case 13: /* General Protection Fault */
            kernel_printf("  GPF Error Code Analysis:\n");
            if (err_code == 0) {
                kernel_printf("    Source: Not segment related\n");
            } else {
                kernel_printf("    External: %s\n",
                              (err_code & 1) ? "Ya" : "Tidak");
                kernel_printf("    Table: ");
                switch ((err_code >> 1) & 3) {
                    case 0: kernel_printf("GDT\n"); break;
                    case 1: kernel_printf("IDT\n"); break;
                    case 2: kernel_printf("LDT\n"); break;
                    case 3: kernel_printf("IDT\n"); break;
                }
                kernel_printf("    Index: %lu\n", err_code >> 3);
            }
            break;

        case 14: /* Page Fault */
            {
                alamat_fisik_t cr2 = cpu_read_cr2();
                kernel_printf("  Page Fault Analysis:\n");
                kernel_printf("    Faulting Address: 0x%08lX\n", cr2);
                kernel_printf("    Present: %s\n",
                              (err_code & 1) ? "Ya" : "Tidak");
                kernel_printf("    Operation: %s\n",
                              (err_code & 2) ? "Write" : "Read");
                kernel_printf("    Mode: %s\n",
                              (err_code & 4) ? "User" : "Kernel");
                kernel_printf("    Reserved Bit: %s\n",
                              (err_code & 8) ? "Set" : "Clear");
                kernel_printf("    Execute: %s\n",
                              (err_code & 16) ? "Ya" : "Tidak");
            }
            break;

        case 8: /* Double Fault */
            kernel_printf("  Double Fault Analysis:\n");
            kernel_printf("    Table: ");
            switch ((err_code >> 1) & 3) {
                case 0: kernel_printf("GDT\n"); break;
                case 1: kernel_printf("IDT\n"); break;
                case 2: kernel_printf("LDT\n"); break;
                case 3: kernel_printf("IDT\n"); break;
            }
            kernel_printf("    Index: %lu\n", err_code >> 3);
            break;

        default:
            break;
    }
}

/*
 * ============================================================================
 * FUNGSI HANDLER EXCEPTION (EXCEPTION HANDLERS)
 * ============================================================================
 */

/*
 * isr_handler
 * -----------
 * Handler utama untuk semua exception.
 * Dipanggil dari assembly stub.
 *
 * Parameter:
 *   ctx - Pointer ke konteks register yang disimpan
 */
void isr_handler(register_context_t *ctx)
{
    /* Increment counter */
    if (ctx->int_no < 32) {
        exception_count[ctx->int_no]++;
    }

    /* Increment nested depth */
    nested_depth++;

    /* Cek nested exception (critical!) */
    if (nested_depth > 2) {
        kernel_printf("\nKRITIKAL: Nested exception depth exceeded!\n");
        cpu_halt();
        for (;;) {}
    }

    /* Panggil handler kustom jika ada */
    if (ctx->int_no < 32 && exception_handlers[ctx->int_no] != NULL) {
        exception_handlers[ctx->int_no](ctx);
        nested_depth--;
        return;
    }

    /* Handle berdasarkan jenis exception */
    switch (ctx->int_no) {
        /* Exception non-fatal */
        case 3:  /* Breakpoint */
            /* Ignore atau pass ke debugger */
            nested_depth--;
            return;

        case 1:  /* Debug Exception */
            /* Ignore atau pass ke debugger */
            nested_depth--;
            return;

        /* Exception fatal - panic */
        default:
            exception_print_context(ctx);

            if (exception_has_error_code(ctx->int_no)) {
                exception_decode_error_code(ctx->int_no, ctx->err_code);
            }

            kernel_panic_exception(ctx);
            break;
    }

    nested_depth--;
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * isr_init
 * --------
 * Inisialisasi ISR.
 */
status_t isr_init(void)
{
    tak_bertanda32_t i;

    /* Reset counter */
    for (i = 0; i < 32; i++) {
        exception_count[i] = 0;
    }

    /* Reset handlers */
    for (i = 0; i < 32; i++) {
        exception_handlers[i] = NULL;
    }

    nested_depth = 0;

    return STATUS_BERHASIL;
}

/*
 * isr_set_handler
 * ---------------
 * Set handler kustom untuk exception.
 *
 * Parameter:
 *   vector  - Nomor vektor exception (0-31)
 *   handler - Pointer ke fungsi handler
 *
 * Return: Status operasi
 */
status_t isr_set_handler(tak_bertanda32_t vector,
                         void (*handler)(register_context_t *))
{
    if (vector >= 32) {
        return STATUS_PARAM_INVALID;
    }

    exception_handlers[vector] = handler;

    return STATUS_BERHASIL;
}

/*
 * isr_get_handler
 * ---------------
 * Dapatkan handler untuk exception.
 *
 * Parameter:
 *   vector - Nomor vektor exception
 *
 * Return: Pointer ke handler, atau NULL jika tidak ada
 */
void (*isr_get_handler(tak_bertanda32_t vector))(register_context_t *)
{
    if (vector >= 32) {
        return NULL;
    }

    return exception_handlers[vector];
}

/*
 * isr_get_count
 * -------------
 * Dapatkan jumlah exception yang terjadi.
 *
 * Parameter:
 *   vector - Nomor vektor exception
 *
 * Return: Jumlah exception
 */
tak_bertanda64_t isr_get_count(tak_bertanda32_t vector)
{
    if (vector >= 32) {
        return 0;
    }

    return exception_count[vector];
}

/*
 * isr_print_stats
 * ---------------
 * Print statistik exception.
 */
void isr_print_stats(void)
{
    tak_bertanda32_t i;

    kernel_printf("\nISR Statistics:\n");
    kernel_printf("========================================\n");

    for (i = 0; i < 32; i++) {
        if (exception_count[i] > 0) {
            kernel_printf("  #%02lu %-30s: %lu\n",
                          i,
                          exception_names[i],
                          exception_count[i]);
        }
    }

    kernel_printf("========================================\n");
}

/*
 * isr_get_name
 * ------------
 * Dapatkan nama exception.
 *
 * Parameter:
 *   vector - Nomor vektor exception
 *
 * Return: String nama exception
 */
const char *isr_get_name(tak_bertanda32_t vector)
{
    return exception_get_name(vector);
}
