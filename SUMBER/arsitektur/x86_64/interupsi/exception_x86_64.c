/*
 * PIGURA OS - EXCEPTION x86_64
 * ----------------------------
 * Implementasi handler exception untuk arsitektur x86_64.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk menangani
 * exception (CPU exception) pada arsitektur x86_64.
 *
 * Arsitektur: x86_64 (64-bit)
 * Versi: 1.0
 */

#include "../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA EXCEPTION
 * ============================================================================
 */

/* Jumlah exception */
#define JUMLAH_EXCEPTION        32

/* Vektor exception */
#define EXC_DE                  0   /* Divide Error */
#define EXC_DB                  1   /* Debug */
#define EXC_NMI                 2   /* NMI */
#define EXC_BP                  3   /* Breakpoint */
#define EXC_OF                  4   /* Overflow */
#define EXC_BR                  5   /* BOUND Range */
#define EXC_UD                  6   /* Invalid Opcode */
#define EXC_NM                  7   /* Device Not Available */
#define EXC_DF                  8   /* Double Fault */
#define EXC_TS                  10  /* Invalid TSS */
#define EXC_NP                  11  /* Segment Not Present */
#define EXC_SS                  12  /* Stack Fault */
#define EXC_GP                  13  /* General Protection */
#define EXC_PF                  14  /* Page Fault */
#define EXC_MF                  16  /* x87 FPU Error */
#define EXC_AC                  17  /* Alignment Check */
#define EXC_MC                  18  /* Machine Check */
#define EXC_XM                  19  /* SIMD Exception */

/*
 * ============================================================================
 * STRUKTUR DATA
 * ============================================================================
 */

/* Info exception */
struct exception_info {
    tak_bertanda32_t nomor;
    const char *nama;
    const char *pesan;
    bool_t ada_error_code;
};

/* Stack frame exception */
struct exception_frame {
    tak_bertanda64_t r15;
    tak_bertanda64_t r14;
    tak_bertanda64_t r13;
    tak_bertanda64_t r12;
    tak_bertanda64_t r11;
    tak_bertanda64_t r10;
    tak_bertanda64_t r9;
    tak_bertanda64_t r8;
    tak_bertanda64_t rdi;
    tak_bertanda64_t rsi;
    tak_bertanda64_t rbp;
    tak_bertanda64_t rdx;
    tak_bertanda64_t rcx;
    tak_bertanda64_t rbx;
    tak_bertanda64_t rax;
    tak_bertanda64_t nomor;
    tak_bertanda64_t error_code;
    tak_bertanda64_t rip;
    tak_bertanda64_t cs;
    tak_bertanda64_t rflags;
    tak_bertanda64_t rsp;
    tak_bertanda64_t ss;
};

/*
 * ============================================================================
 * VARIABEL GLOBAL
 * ============================================================================
 */

/* Counter exception */
static tak_bertanda64_t g_exception_counter[JUMLAH_EXCEPTION] = { 0 };

/* Handler exception custom */
static void (*g_exception_handler[JUMLAH_EXCEPTION])
    (struct exception_frame *) = { NULL };

/* Tabel info exception */
static const struct exception_info g_exception_table[] = {
    {EXC_DE, "DE",  "Divide Error",                  SALAH},
    {EXC_DB, "DB",  "Debug Exception",               SALAH},
    {EXC_NMI, "NMI", "Non-Maskable Interrupt",       SALAH},
    {EXC_BP, "BP",  "Breakpoint",                    SALAH},
    {EXC_OF, "OF",  "Overflow",                      SALAH},
    {EXC_BR, "BR",  "BOUND Range Exceeded",          SALAH},
    {EXC_UD, "UD",  "Invalid Opcode",                SALAH},
    {EXC_NM, "NM",  "Device Not Available",          SALAH},
    {EXC_DF, "DF",  "Double Fault",                  BENAR},
    {EXC_TS, "TS",  "Invalid TSS",                   BENAR},
    {EXC_NP, "NP",  "Segment Not Present",           BENAR},
    {EXC_SS, "SS",  "Stack-Segment Fault",           BENAR},
    {EXC_GP, "GP",  "General Protection",            BENAR},
    {EXC_PF, "PF",  "Page Fault",                    BENAR},
    {EXC_MF, "MF",  "x87 FPU Error",                 SALAH},
    {EXC_AC, "AC",  "Alignment Check",               BENAR},
    {EXC_MC, "MC",  "Machine Check",                 SALAH},
    {EXC_XM, "XM",  "SIMD Exception",                SALAH}
};

#define EXCEPTION_TABLE_SIZE \
    (sizeof(g_exception_table) / sizeof(g_exception_table[0]))

/*
 * ============================================================================
 * FUNGSI INTERNAL
 * ============================================================================
 */

/*
 * _cari_exception_info
 * --------------------
 * Mencari info exception berdasarkan nomor.
 *
 * Parameter:
 *   nomor - Nomor exception
 *
 * Return:
 *   Pointer ke info, atau NULL
 */
static const struct exception_info *_cari_exception_info(
    tak_bertanda32_t nomor)
{
    tak_bertanda32_t i;

    for (i = 0; i < EXCEPTION_TABLE_SIZE; i++) {
        if (g_exception_table[i].nomor == nomor) {
            return &g_exception_table[i];
        }
    }

    return NULL;
}

/*
 * _dump_exception_frame
 * ---------------------
 * Menampilkan isi exception frame.
 *
 * Parameter:
 *   frame - Pointer ke frame
 */
static void _dump_exception_frame(struct exception_frame *frame)
{
    const struct exception_info *info;

    kernel_printf("\n=== EXCEPTION ===\n");

    if (frame == NULL) {
        kernel_printf("Frame NULL!\n");
        return;
    }

    info = _cari_exception_info((tak_bertanda32_t)frame->nomor);
    if (info != NULL) {
        kernel_printf("Exception: %s (#%u)\n", info->nama, info->nomor);
        kernel_printf("Pesan: %s\n", info->pesan);
    } else {
        kernel_printf("Exception: #%u\n", (tak_bertanda32_t)frame->nomor);
    }

    if (info != NULL && info->ada_error_code) {
        kernel_printf("Error Code: 0x%016X\n", frame->error_code);
    }

    kernel_printf("\nRegisters:\n");
    kernel_printf("RAX: 0x%016X  RBX: 0x%016X\n", frame->rax, frame->rbx);
    kernel_printf("RCX: 0x%016X  RDX: 0x%016X\n", frame->rcx, frame->rdx);
    kernel_printf("RSI: 0x%016X  RDI: 0x%016X\n", frame->rsi, frame->rdi);
    kernel_printf("RBP: 0x%016X  RSP: 0x%016X\n", frame->rbp, frame->rsp);
    kernel_printf("R8:  0x%016X  R9:  0x%016X\n", frame->r8, frame->r9);
    kernel_printf("R10: 0x%016X  R11: 0x%016X\n", frame->r10, frame->r11);
    kernel_printf("R12: 0x%016X  R13: 0x%016X\n", frame->r12, frame->r13);
    kernel_printf("R14: 0x%016X  R15: 0x%016X\n", frame->r14, frame->r15);

    kernel_printf("\nSystem:\n");
    kernel_printf("RIP: 0x%016X\n", frame->rip);
    kernel_printf("CS:  0x%04X  SS:  0x%04X\n",
                  (tak_bertanda32_t)frame->cs, (tak_bertanda32_t)frame->ss);
    kernel_printf("RFLAGS: 0x%016X\n", frame->rflags);

    /* Info tambahan untuk Page Fault */
    if (frame->nomor == EXC_PF) {
        tak_bertanda64_t cr2;

        cr2 = cpu_read_cr2();
        kernel_printf("\nPage Fault Info:\n");
        kernel_printf("CR2 (fault address): 0x%016X\n", cr2);

        if (frame->error_code & 0x01) {
            kernel_printf("Type: Page-protection violation\n");
        } else {
            kernel_printf("Type: Page not present\n");
        }

        if (frame->error_code & 0x02) {
            kernel_printf("Operation: Write\n");
        } else {
            kernel_printf("Operation: Read\n");
        }

        if (frame->error_code & 0x04) {
            kernel_printf("Mode: User\n");
        } else {
            kernel_printf("Mode: Kernel\n");
        }
    }

    kernel_printf("=================\n");
}

/*
 * ============================================================================
 * FUNGSI PUBLIC
 * ============================================================================
 */

/*
 * exception_x86_64_handler
 * ------------------------
 * Handler exception umum.
 *
 * Parameter:
 *   frame - Pointer ke exception frame
 */
void exception_x86_64_handler(struct exception_frame *frame)
{
    tak_bertanda32_t nomor;

    if (frame == NULL) {
        kernel_printf("[EXC] Frame NULL, halt!\n");
        hal_cpu_halt();
        return;
    }

    nomor = (tak_bertanda32_t)frame->nomor;

    /* Increment counter */
    if (nomor < JUMLAH_EXCEPTION) {
        g_exception_counter[nomor]++;
    }

    /* Panggil handler custom jika ada */
    if (nomor < JUMLAH_EXCEPTION && g_exception_handler[nomor] != NULL) {
        g_exception_handler[nomor](frame);
        return;
    }

    /* Default handler */
    _dump_exception_frame(frame);

    kernel_printf("\n[EXC] System halted.\n");
    hal_cpu_halt();
}

/*
 * exception_x86_64_register
 * -------------------------
 * Mendaftarkan handler exception custom.
 *
 * Parameter:
 *   nomor   - Nomor exception
 *   handler - Pointer ke handler
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t exception_x86_64_register(tak_bertanda32_t nomor,
                                    void (*handler)(struct exception_frame *))
{
    if (nomor >= JUMLAH_EXCEPTION) {
        return STATUS_PARAM_INVALID;
    }

    g_exception_handler[nomor] = handler;

    return STATUS_BERHASIL;
}

/*
 * exception_x86_64_unregister
 * ---------------------------
 * Menghapus handler exception custom.
 *
 * Parameter:
 *   nomor - Nomor exception
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t exception_x86_64_unregister(tak_bertanda32_t nomor)
{
    if (nomor >= JUMLAH_EXCEPTION) {
        return STATUS_PARAM_INVALID;
    }

    g_exception_handler[nomor] = NULL;

    return STATUS_BERHASIL;
}

/*
 * exception_x86_64_get_counter
 * ----------------------------
 * Mendapatkan counter exception.
 *
 * Parameter:
 *   nomor - Nomor exception
 *
 * Return:
 *   Jumlah exception, atau 0
 */
tak_bertanda64_t exception_x86_64_get_counter(tak_bertanda32_t nomor)
{
    if (nomor >= JUMLAH_EXCEPTION) {
        return 0;
    }

    return g_exception_counter[nomor];
}

/*
 * exception_x86_64_print_stats
 * ----------------------------
 * Menampilkan statistik exception.
 */
void exception_x86_64_print_stats(void)
{
    tak_bertanda32_t i;

    kernel_printf("\n=== Exception Statistics ===\n");

    for (i = 0; i < JUMLAH_EXCEPTION; i++) {
        if (g_exception_counter[i] > 0) {
            const struct exception_info *info;

            info = _cari_exception_info(i);
            if (info != NULL) {
                kernel_printf("  %s (#%u): %u\n", info->nama, i,
                              (tak_bertanda32_t)g_exception_counter[i]);
            } else {
                kernel_printf("  #%u: %u\n", i,
                              (tak_bertanda32_t)g_exception_counter[i]);
            }
        }
    }

    kernel_printf("============================\n");
}
