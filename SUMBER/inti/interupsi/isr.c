/*
 * PIGURA OS - ISR.C
 * -----------------
 * Implementasi Interrupt Service Routines.
 *
 * Berkas ini berisi implementasi lengkap ISR untuk exception CPU dan
 * mekanisme dispatch ke handler yang sesuai.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua fungsi dideklarasikan secara eksplisit
 *
 * Versi: 2.1
 * Tanggal: 2025
 */

#include "../kernel.h"

/*
 * ===========================================================================
 * DEKLARASI FORWARD (FORWARD DECLARATIONS)
 * ===========================================================================
 */

/* Deklarasi fungsi PIC dari pic.c */
extern void pic_send_eoi(tak_bertanda32_t irq);

/*
 * ===========================================================================
 * MAKRO HELPER UNTUK MULTI-ARSITEKTUR
 * ===========================================================================
 */

/* Makro untuk mendapatkan instruction pointer sesuai arsitektur */
#if defined(ARSITEKTUR_X86)
#define GET_IP(ctx) ((ctx)->eip)
#define IP_FORMAT "EIP: 0x%08lX"
#define IP_CAST(val) ((tak_bertanda64_t)(val))
#elif defined(ARSITEKTUR_X86_64)
#define GET_IP(ctx) ((ctx)->rip)
#define IP_FORMAT "RIP: 0x%016llX"
#define IP_CAST(val) (val)
#else
#define GET_IP(ctx) ((ctx)->pc)
#define IP_FORMAT "PC: 0x%016llX"
#define IP_CAST(val) (val)
#endif

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* Nama exception dalam bahasa Indonesia */
static const char *nama_exception[] = {
    "Kesalahan Pembagian",
    "Exception Debug",
    "Interupsi Non-Maskable",
    "Breakpoint",
    "Overflow",
    "Rentang BOUND Terlampaui",
    "Opcode Tidak Valid",
    "Perangkat Tidak Tersedia",
    "Double Fault",
    "Segment Coprocessor Overrun",
    "TSS Tidak Valid",
    "Segment Tidak Hadir",
    "Kesalahan Segment Stack",
    "Kesalahan Proteksi Umum",
    "Kesalahan Halaman",
    "Reserved",
    "Kesalahan x87 FPU",
    "Cek Alignment",
    "Machine Check",
    "Exception SIMD",
    "Exception Virtualisasi",
    "Exception Proteksi Kontrol",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Exception Keamanan",
    "Reserved"
};

/* Nama IRQ dalam bahasa Indonesia */
static const char *nama_irq[] = {
    "Timer",
    "Keyboard",
    "Cascade",
    "COM2",
    "COM1",
    "LPT1",
    "Floppy",
    "LPT1 Reserved",
    "RTC",
    "ACPI",
    "Reserved",
    "Reserved",
    "Mouse",
    "FPU",
    "ATA Primer",
    "ATA Sekunder"
};

/* Basis vektor untuk IRQ */
#define IRQ_VEKTOR_BASIS 32

/*
 * ============================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ============================================================================
 */

/* Tabel ISR global */
isr_tabel_t g_isr_tabel;

/* Counter spurious interrupt */
tak_bertanda32_t g_isr_spurious_count = 0;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Counter untuk setiap exception */
static tak_bertanda64_t hitung_exception[32] = {0};

/* Counter untuk setiap IRQ */
static tak_bertanda64_t hitung_irq[24] = {0};

/* Handler kustom untuk setiap exception */
static void (*handler_exception[32])(register_context_t *) = {NULL};

/* Handler kustom untuk setiap IRQ */
static irq_handler_t handler_irq[24] = {NULL};

/* Flag untuk tracking nested exceptions */
static volatile tak_bertanda32_t kedalaman_nested = 0;

/* Total counter */
static tak_bertanda64_t total_hitung = 0;
static tak_bertanda64_t exception_total_hitung = 0;
static tak_bertanda64_t irq_total_hitung = 0;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * exception_dapat_nama
 * --------------------
 * Dapatkan nama exception dari nomor vektor.
 *
 * Parameter:
 *   vektor - Nomor vektor exception
 *
 * Return: String nama exception
 */
static const char *exception_dapat_nama(tak_bertanda32_t vektor)
{
    if (vektor >= 32) {
        return "Exception Tidak Dikenal";
    }
    return nama_exception[vektor];
}

/*
 * irq_dapat_nama
 * --------------
 * Dapatkan nama IRQ dari nomor IRQ.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: String nama IRQ
 */
static const char *irq_dapat_nama(tak_bertanda32_t irq)
{
    if (irq >= 16) {
        return "IRQ Tidak Dikenal";
    }
    return nama_irq[irq];
}

/*
 * exception_punya_kode_error
 * --------------------------
 * Cek apakah exception memiliki error code.
 *
 * Parameter:
 *   vektor - Nomor vektor exception
 *
 * Return: BENAR jika memiliki error code
 */
static bool_t exception_punya_kode_error(tak_bertanda32_t vektor)
{
    /* Exception dengan error code: 8, 10-14, 17, 21, 29, 30 */
    switch (vektor) {
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
 * exception_cetak_konteks
 * -----------------------
 * Cetak konteks exception untuk debugging.
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
static void exception_cetak_konteks(const register_context_t *ctx)
{
    kernel_printf("\nKonteks Exception:\n");
    kernel_printf("  Exception: %s (#%lu)\n",
                  exception_dapat_nama((tak_bertanda32_t)ctx->int_no), 
                  (tak_bertanda64_t)ctx->int_no);

    if (exception_punya_kode_error((tak_bertanda32_t)ctx->int_no)) {
        kernel_printf("  Kode Error: 0x%08lX\n", (tak_bertanda64_t)ctx->err_code);
    }

    kernel_printf("\nRegister:\n");

#if defined(ARSITEKTUR_X86)
    /* x86 (32-bit) register names */
    kernel_printf("  EAX=0x%08lX  EBX=0x%08lX  ECX=0x%08lX  EDX=0x%08lX\n",
                  (tak_bertanda64_t)ctx->eax, (tak_bertanda64_t)ctx->ebx,
                  (tak_bertanda64_t)ctx->ecx, (tak_bertanda64_t)ctx->edx);
    kernel_printf("  ESI=0x%08lX  EDI=0x%08lX  EBP=0x%08lX  ESP=0x%08lX\n",
                  (tak_bertanda64_t)ctx->esi, (tak_bertanda64_t)ctx->edi,
                  (tak_bertanda64_t)ctx->ebp, (tak_bertanda64_t)ctx->esp);
    kernel_printf("  EIP=0x%08lX  EFLAGS=0x%08lX\n", 
                  (tak_bertanda64_t)ctx->eip, (tak_bertanda64_t)ctx->eflags);
    kernel_printf("  CS=0x%04lX  DS=0x%04lX  ES=0x%04lX\n",
                  (tak_bertanda64_t)ctx->cs, (tak_bertanda64_t)ctx->ds,
                  (tak_bertanda64_t)ctx->es);
    kernel_printf("  FS=0x%04lX  GS=0x%04lX  SS=0x%04lX\n",
                  (tak_bertanda64_t)ctx->fs, (tak_bertanda64_t)ctx->gs,
                  (tak_bertanda64_t)ctx->ss);
#elif defined(ARSITEKTUR_X86_64)
    /* x86_64 (64-bit) register names */
    kernel_printf("  RAX=0x%016llX  RBX=0x%016llX\n", ctx->rax, ctx->rbx);
    kernel_printf("  RCX=0x%016llX  RDX=0x%016llX\n", ctx->rcx, ctx->rdx);
    kernel_printf("  RSI=0x%016llX  RDI=0x%016llX\n", ctx->rsi, ctx->rdi);
    kernel_printf("  RBP=0x%016llX  RSP=0x%016llX\n", ctx->rbp, ctx->rsp);
    kernel_printf("  R8 =0x%016llX  R9 =0x%016llX\n", ctx->r8, ctx->r9);
    kernel_printf("  R10=0x%016llX  R11=0x%016llX\n", ctx->r10, ctx->r11);
    kernel_printf("  R12=0x%016llX  R13=0x%016llX\n", ctx->r12, ctx->r13);
    kernel_printf("  R14=0x%016llX  R15=0x%016llX\n", ctx->r14, ctx->r15);
    kernel_printf("  RIP=0x%016llX  RFLAGS=0x%016llX\n", ctx->rip, ctx->rflags);
    kernel_printf("  CS=0x%04llX  SS=0x%04llX\n", ctx->cs, ctx->ss);
#else
    /* Fallback untuk arsitektur lain */
    kernel_printf("  PC=0x%016llX  SP=0x%016llX\n", ctx->pc, ctx->sp);
#endif
}

/*
 * exception_decode_kode_error
 * ---------------------------
 * Decode error code untuk exception tertentu.
 *
 * Parameter:
 *   vektor    - Nomor vektor exception
 *   kode_err  - Error code
 */
static void exception_decode_kode_error(tak_bertanda32_t vektor,
                                        tak_bertanda32_t kode_err)
{
    switch (vektor) {
        case 13: /* General Protection Fault */
            kernel_printf("  Analisis Kode Error GPF:\n");
            if (kode_err == 0) {
                kernel_printf("    Sumber: Bukan terkait segment\n");
            } else {
                kernel_printf("    Eksternal: %s\n",
                              (kode_err & 1) ? "Ya" : "Tidak");
                kernel_printf("    Tabel: ");
                switch ((kode_err >> 1) & 3) {
                    case 0: kernel_printf("GDT\n"); break;
                    case 1: kernel_printf("IDT\n"); break;
                    case 2: kernel_printf("LDT\n"); break;
                    case 3: kernel_printf("IDT\n"); break;
                }
                kernel_printf("    Indeks: %lu\n", kode_err >> 3);
            }
            break;

        case 14: /* Page Fault */
            {
                alamat_fisik_t cr2 = cpu_read_cr2();
                kernel_printf("  Analisis Kesalahan Halaman:\n");
                kernel_printf("    Alamat Penyebab: 0x%08lX\n", cr2);
                kernel_printf("    Hadir: %s\n",
                              (kode_err & 1) ? "Ya" : "Tidak");
                kernel_printf("    Operasi: %s\n",
                              (kode_err & 2) ? "Tulis" : "Baca");
                kernel_printf("    Mode: %s\n",
                              (kode_err & 4) ? "User" : "Kernel");
                kernel_printf("    Bit Reserved: %s\n",
                              (kode_err & 8) ? "Set" : "Clear");
                kernel_printf("    Eksekusi: %s\n",
                              (kode_err & 16) ? "Ya" : "Tidak");
            }
            break;

        case 8: /* Double Fault */
            kernel_printf("  Analisis Double Fault:\n");
            kernel_printf("    Tabel: ");
            switch ((kode_err >> 1) & 3) {
                case 0: kernel_printf("GDT\n"); break;
                case 1: kernel_printf("IDT\n"); break;
                case 2: kernel_printf("LDT\n"); break;
                case 3: kernel_printf("IDT\n"); break;
            }
            kernel_printf("    Indeks: %lu\n", kode_err >> 3);
            break;

        default:
            break;
    }
}

/*
 * ============================================================================
 * IMPLEMENTASI FUNGSI PUBLIK (PUBLIC FUNCTIONS IMPLEMENTATION)
 * ============================================================================
 */

/*
 * isr_init
 * --------
 * Inisialisasi sistem ISR.
 */
status_t isr_init(void)
{
    tak_bertanda32_t i;

    /* Reset counter */
    for (i = 0; i < 32; i++) {
        hitung_exception[i] = 0;
    }

    for (i = 0; i < 24; i++) {
        hitung_irq[i] = 0;
    }

    /* Reset handlers */
    for (i = 0; i < 32; i++) {
        handler_exception[i] = NULL;
    }

    for (i = 0; i < 24; i++) {
        handler_irq[i] = NULL;
    }

    /* Inisialisasi tabel ISR global */
    for (i = 0; i < ISR_JUMLAH_VECTOR; i++) {
        g_isr_tabel.entri[i].handler = NULL;
        g_isr_tabel.entri[i].hitung = 0;
        g_isr_tabel.entri[i].bendera = 0;
        g_isr_tabel.entri[i].prioritas = ISR_PRIORITAS_NORMAL;
        g_isr_tabel.entri[i].nama = NULL;
        g_isr_tabel.entri[i].data = NULL;
    }

    g_isr_tabel.total_hitung = 0;
    g_isr_tabel.exception_hitung = 0;
    g_isr_tabel.irq_hitung = 0;

    kedalaman_nested = 0;
    total_hitung = 0;
    exception_total_hitung = 0;
    irq_total_hitung = 0;
    g_isr_spurious_count = 0;

    return STATUS_BERHASIL;
}

/*
 * isr_reset
 * ---------
 * Reset semua handler ISR ke default.
 */
status_t isr_reset(void)
{
    tak_bertanda32_t i;

    /* Reset semua handler */
    for (i = 0; i < 32; i++) {
        handler_exception[i] = NULL;
    }

    for (i = 0; i < 24; i++) {
        handler_irq[i] = NULL;
    }

    /* Reset tabel */
    for (i = 0; i < ISR_JUMLAH_VECTOR; i++) {
        g_isr_tabel.entri[i].handler = NULL;
        g_isr_tabel.entri[i].bendera = 0;
    }

    return STATUS_BERHASIL;
}

/*
 * isr_set_handler
 * ---------------
 * Set handler untuk vector ISR tertentu.
 */
status_t isr_set_handler(tak_bertanda32_t vektor, isr_handler_t handler)
{
    if (vektor >= ISR_JUMLAH_VECTOR) {
        return STATUS_PARAM_JARAK;
    }

    g_isr_tabel.entri[vektor].handler = handler;
    g_isr_tabel.entri[vektor].bendera |= ISR_FLAG_ACTIVE;

    return STATUS_BERHASIL;
}

/*
 * isr_get_handler
 * ---------------
 * Dapatkan handler untuk vector ISR tertentu.
 */
isr_handler_t isr_get_handler(tak_bertanda32_t vektor)
{
    if (vektor >= ISR_JUMLAH_VECTOR) {
        return NULL;
    }

    return g_isr_tabel.entri[vektor].handler;
}

/*
 * isr_clear_handler
 * -----------------
 * Hapus handler untuk vector ISR tertentu.
 */
status_t isr_clear_handler(tak_bertanda32_t vektor)
{
    if (vektor >= ISR_JUMLAH_VECTOR) {
        return STATUS_PARAM_JARAK;
    }

    g_isr_tabel.entri[vektor].handler = NULL;
    g_isr_tabel.entri[vektor].bendera &= ~ISR_FLAG_ACTIVE;

    return STATUS_BERHASIL;
}

/*
 * isr_set_handler_cek
 * -------------------
 * Set handler dengan pengecekan jika sudah ada.
 */
status_t isr_set_handler_cek(tak_bertanda32_t vektor, isr_handler_t handler,
                              bool_t paksa)
{
    if (vektor >= ISR_JUMLAH_VECTOR) {
        return STATUS_PARAM_JARAK;
    }

    if (handler == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Cek apakah sudah ada handler */
    if (g_isr_tabel.entri[vektor].handler != NULL && !paksa) {
        return STATUS_SUDAH_ADA;
    }

    g_isr_tabel.entri[vektor].handler = handler;
    g_isr_tabel.entri[vektor].bendera |= ISR_FLAG_ACTIVE;

    return STATUS_BERHASIL;
}

/*
 * isr_irq_set_handler
 * -------------------
 * Set handler untuk IRQ tertentu.
 */
status_t isr_irq_set_handler(tak_bertanda32_t irq, irq_handler_t handler)
{
    if (irq >= JUMLAH_IRQ_APIC) {
        return STATUS_PARAM_JARAK;
    }

    handler_irq[irq] = handler;
    return STATUS_BERHASIL;
}

/*
 * isr_irq_get_handler
 * -------------------
 * Dapatkan handler untuk IRQ tertentu.
 */
irq_handler_t isr_irq_get_handler(tak_bertanda32_t irq)
{
    if (irq >= JUMLAH_IRQ_APIC) {
        return NULL;
    }

    return handler_irq[irq];
}

/*
 * isr_irq_clear_handler
 * ---------------------
 * Hapus handler untuk IRQ tertentu.
 */
status_t isr_irq_clear_handler(tak_bertanda32_t irq)
{
    if (irq >= JUMLAH_IRQ_APIC) {
        return STATUS_PARAM_JARAK;
    }

    handler_irq[irq] = NULL;
    return STATUS_BERHASIL;
}

/*
 * isr_irq_to_vector
 * -----------------
 * Konversi IRQ ke vector number.
 */
tak_bertanda32_t isr_irq_to_vector(tak_bertanda32_t irq)
{
    return IRQ_VEKTOR_BASIS + irq;
}

/*
 * isr_vector_to_irq
 * -----------------
 * Konversi vector ke IRQ number.
 */
tanda32_t isr_vector_to_irq(tak_bertanda32_t vektor)
{
    if (vektor < IRQ_VEKTOR_BASIS ||
        vektor >= IRQ_VEKTOR_BASIS + JUMLAH_IRQ_APIC) {
        return -1;
    }

    return (tanda32_t)(vektor - IRQ_VEKTOR_BASIS);
}

/*
 * isr_dispatch
 * ------------
 * Dispatch interrupt ke handler yang sesuai.
 */
void isr_dispatch(register_context_t *ctx)
{
    tak_bertanda32_t vektor;
    isr_handler_t handler;

    if (ctx == NULL) {
        return;
    }

    vektor = ctx->int_no;
    total_hitung++;
    g_isr_tabel.total_hitung++;

    /* Cek apakah exception atau IRQ */
    if (vektor < 32) {
        isr_exception_dispatch(ctx);
        return;
    }

    if (vektor >= IRQ_VEKTOR_BASIS &&
        vektor < IRQ_VEKTOR_BASIS + JUMLAH_IRQ_APIC) {
        tak_bertanda32_t irq = vektor - IRQ_VEKTOR_BASIS;
        isr_irq_dispatch(irq, ctx);
        return;
    }

    /* Handler umum */
    handler = g_isr_tabel.entri[vektor].handler;
    if (handler != NULL) {
        handler(ctx);
    }
}

/*
 * isr_exception_dispatch
 * ----------------------
 * Dispatch exception ke handler yang sesuai.
 */
void isr_exception_dispatch(register_context_t *ctx)
{
    tak_bertanda32_t vektor;
    isr_handler_t handler;

    if (ctx == NULL) {
        return;
    }

    vektor = ctx->int_no;

    /* Update counter */
    if (vektor < 32) {
        hitung_exception[vektor]++;
        exception_total_hitung++;
        g_isr_tabel.exception_hitung++;
    }

    /* Increment nested depth */
    kedalaman_nested++;

    /* Cek nested exception (critical!) */
    if (kedalaman_nested > 2) {
        kernel_printf("\nKRITIKAL: Kedalaman nested exception terlampaui!\n");
        cpu_halt();
        for (;;) {
            /* Loop selamanya */
        }
    }

    /* Panggil handler dari tabel global */
    handler = g_isr_tabel.entri[vektor].handler;
    if (handler != NULL) {
        handler(ctx);
        kedalaman_nested--;
        return;
    }

    /* Panggil handler kustom jika ada */
    if (vektor < 32 && handler_exception[vektor] != NULL) {
        handler_exception[vektor](ctx);
        kedalaman_nested--;
        return;
    }

    /* Handle berdasarkan jenis exception */
    switch (vektor) {
        case 3:  /* Breakpoint */
        case 1:  /* Debug Exception */
            /* Ignore atau pass ke debugger */
            kedalaman_nested--;
            return;

        default:
            exception_cetak_konteks(ctx);

            if (exception_punya_kode_error(vektor)) {
                exception_decode_kode_error(vektor, ctx->err_code);
            }

            kernel_panic_exception(ctx);
            break;
    }

    kedalaman_nested--;
}

/*
 * isr_irq_dispatch
 * ----------------
 * Dispatch IRQ ke handler yang sesuai.
 */
void isr_irq_dispatch(tak_bertanda32_t irq, register_context_t *ctx)
{
    irq_handler_t handler;

    if (irq >= JUMLAH_IRQ_APIC) {
        return;
    }

    /* Update counter */
    hitung_irq[irq]++;
    irq_total_hitung++;
    g_isr_tabel.irq_hitung++;

    /* Cek spurious */
    if (isr_is_spurious(irq)) {
        g_isr_spurious_count++;
        return;
    }

    /* Panggil handler */
    handler = handler_irq[irq];
    if (handler != NULL) {
        handler(irq, ctx);
    }
}

/*
 * isr_get_count
 * -------------
 * Dapatkan jumlah interrupt untuk vector tertentu.
 */
tak_bertanda64_t isr_get_count(tak_bertanda32_t vektor)
{
    if (vektor >= ISR_JUMLAH_VECTOR) {
        return 0;
    }

    return g_isr_tabel.entri[vektor].hitung;
}

/*
 * isr_get_total_count
 * -------------------
 * Dapatkan total jumlah interrupt.
 */
tak_bertanda64_t isr_get_total_count(void)
{
    return total_hitung;
}

/*
 * isr_get_exception_count
 * -----------------------
 * Dapatkan total jumlah exception.
 */
tak_bertanda64_t isr_get_exception_count(void)
{
    return exception_total_hitung;
}

/*
 * isr_get_irq_count
 * -----------------
 * Dapatkan total jumlah IRQ.
 */
tak_bertanda64_t isr_get_irq_count(void)
{
    return irq_total_hitung;
}

/*
 * isr_reset_count
 * ---------------
 * Reset counter untuk vector tertentu.
 */
void isr_reset_count(tak_bertanda32_t vektor)
{
    if (vektor < 32) {
        hitung_exception[vektor] = 0;
    } else if (vektor >= IRQ_VEKTOR_BASIS &&
               vektor < IRQ_VEKTOR_BASIS + JUMLAH_IRQ_APIC) {
        hitung_irq[vektor - IRQ_VEKTOR_BASIS] = 0;
    }

    if (vektor < ISR_JUMLAH_VECTOR) {
        g_isr_tabel.entri[vektor].hitung = 0;
    }
}

/*
 * isr_reset_all_count
 * -------------------
 * Reset semua counter.
 */
void isr_reset_all_count(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < 32; i++) {
        hitung_exception[i] = 0;
    }

    for (i = 0; i < 24; i++) {
        hitung_irq[i] = 0;
    }

    for (i = 0; i < ISR_JUMLAH_VECTOR; i++) {
        g_isr_tabel.entri[i].hitung = 0;
    }

    total_hitung = 0;
    exception_total_hitung = 0;
    irq_total_hitung = 0;
    g_isr_tabel.total_hitung = 0;
    g_isr_tabel.exception_hitung = 0;
    g_isr_tabel.irq_hitung = 0;
}

/*
 * isr_vector_is_valid
 * -------------------
 * Cek apakah vector valid.
 */
bool_t isr_vector_is_valid(tak_bertanda32_t vektor)
{
    return (vektor < ISR_JUMLAH_VECTOR) ? BENAR : SALAH;
}

/*
 * isr_vector_is_exception
 * -----------------------
 * Cek apakah vector adalah exception.
 */
bool_t isr_vector_is_exception(tak_bertanda32_t vektor)
{
    return (vektor < 32) ? BENAR : SALAH;
}

/*
 * isr_vector_is_irq
 * -----------------
 * Cek apakah vector adalah IRQ.
 */
bool_t isr_vector_is_irq(tak_bertanda32_t vektor)
{
    if (vektor >= IRQ_VEKTOR_BASIS &&
        vektor < IRQ_VEKTOR_BASIS + JUMLAH_IRQ_APIC) {
        return BENAR;
    }
    return SALAH;
}

/*
 * isr_vector_is_reserved
 * ----------------------
 * Cek apakah vector reserved.
 */
bool_t isr_vector_is_reserved(tak_bertanda32_t vektor)
{
    if (vektor >= ISR_JUMLAH_VECTOR) {
        return SALAH;
    }

    return (g_isr_tabel.entri[vektor].bendera & ISR_FLAG_RESERVED) ?
           BENAR : SALAH;
}

/*
 * isr_set_name
 * ------------
 * Set nama untuk vector.
 */
status_t isr_set_name(tak_bertanda32_t vektor, const char *nama)
{
    if (vektor >= ISR_JUMLAH_VECTOR) {
        return STATUS_PARAM_JARAK;
    }

    g_isr_tabel.entri[vektor].nama = nama;
    return STATUS_BERHASIL;
}

/*
 * isr_get_name
 * ------------
 * Dapatkan nama vector.
 */
const char *isr_get_name(tak_bertanda32_t vektor)
{
    if (vektor >= ISR_JUMLAH_VECTOR) {
        return "Tidak Dikenal";
    }

    /* Cek nama kustom */
    if (g_isr_tabel.entri[vektor].nama != NULL) {
        return g_isr_tabel.entri[vektor].nama;
    }

    /* Return nama default */
    if (vektor < 32) {
        return exception_dapat_nama(vektor);
    }

    if (vektor >= IRQ_VEKTOR_BASIS &&
        vektor < IRQ_VEKTOR_BASIS + 16) {
        return irq_dapat_nama(vektor - IRQ_VEKTOR_BASIS);
    }

    return "Tidak Dikenal";
}

/*
 * isr_print_stats
 * ---------------
 * Cetak statistik ISR.
 */
void isr_print_stats(void)
{
    tak_bertanda32_t i;

    kernel_printf("\nStatistik ISR:\n");
    kernel_printf("========================================\n");
    kernel_printf("Total Interrupt: %lu\n", total_hitung);
    kernel_printf("Total Exception: %lu\n", exception_total_hitung);
    kernel_printf("Total IRQ:       %lu\n", irq_total_hitung);
    kernel_printf("Spurious:        %lu\n", g_isr_spurious_count);
    kernel_printf("----------------------------------------\n");

    kernel_printf("\nException:\n");
    for (i = 0; i < 32; i++) {
        if (hitung_exception[i] > 0) {
            kernel_printf("  #%02lu %-30s: %lu\n",
                          i,
                          exception_dapat_nama(i),
                          hitung_exception[i]);
        }
    }

    kernel_printf("\nIRQ:\n");
    for (i = 0; i < 16; i++) {
        if (hitung_irq[i] > 0) {
            kernel_printf("  IRQ%02lu %-20s: %lu\n",
                          i,
                          irq_dapat_nama(i),
                          hitung_irq[i]);
        }
    }

    kernel_printf("========================================\n");
}

/*
 * isr_print_handlers
 * ------------------
 * Cetak daftar handler yang terdaftar.
 */
void isr_print_handlers(void)
{
    tak_bertanda32_t i;
    tak_bertanda32_t hitung = 0;

    kernel_printf("\nHandler ISR Terdaftar:\n");
    kernel_printf("========================================\n");

    /* Exception handlers */
    kernel_printf("Exception Handlers:\n");
    for (i = 0; i < 32; i++) {
        if (handler_exception[i] != NULL ||
            g_isr_tabel.entri[i].handler != NULL) {
            kernel_printf("  #%02lu %-30s: 0x%p\n",
                          i,
                          exception_dapat_nama(i),
                          g_isr_tabel.entri[i].handler != NULL ?
                          g_isr_tabel.entri[i].handler : handler_exception[i]);
            hitung++;
        }
    }

    /* IRQ handlers */
    kernel_printf("\nIRQ Handlers:\n");
    for (i = 0; i < JUMLAH_IRQ_APIC; i++) {
        if (handler_irq[i] != NULL) {
            kernel_printf("  IRQ%02lu %-20s: 0x%p\n",
                          i,
                          irq_dapat_nama(i),
                          handler_irq[i]);
            hitung++;
        }
    }

    kernel_printf("----------------------------------------\n");
    kernel_printf("Total: %lu handler terdaftar\n", hitung);
    kernel_printf("========================================\n");
}

/*
 * ============================================================================
 * EXCEPTION HANDLERS DEFAULT (DEFAULT EXCEPTION HANDLERS)
 * ============================================================================
 */

/*
 * isr_exception_handler_default
 * -----------------------------
 * Handler default untuk exception yang tidak ditangani.
 */
void isr_exception_handler_default(register_context_t *ctx)
{
    if (ctx == NULL) {
        kernel_panic(__FILE__, __LINE__, "Exception dengan konteks NULL");
        return;
    }

    exception_cetak_konteks(ctx);

    if (exception_punya_kode_error((tak_bertanda32_t)ctx->int_no)) {
        exception_decode_kode_error((tak_bertanda32_t)ctx->int_no, 
                                     (tak_bertanda32_t)ctx->err_code);
    }

    kernel_panic_exception(ctx);
}

/*
 * isr_divide_error_handler
 * ------------------------
 * Handler untuk divide error (vector 0).
 */
void isr_divide_error_handler(register_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    kernel_printf("\nKESALAHAN: Pembagian dengan nol!\n");
    kernel_printf(IP_FORMAT "\n", IP_CAST(GET_IP(ctx)));
    isr_exception_handler_default(ctx);
}

/*
 * isr_debug_handler
 * -----------------
 * Handler untuk debug exception (vector 1).
 */
void isr_debug_handler(register_context_t *ctx)
{
    /* Debug exception biasanya ditangani debugger */
    TIDAK_DIGUNAKAN_PARAM(ctx);
}

/*
 * isr_nmi_handler
 * ---------------
 * Handler untuk NMI (vector 2).
 */
void isr_nmi_handler(register_context_t *ctx)
{
    kernel_printf("\nPERINGATAN: NMI diterima!\n");

    if (ctx != NULL) {
        exception_cetak_konteks(ctx);
    }
}

/*
 * isr_breakpoint_handler
 * ----------------------
 * Handler untuk breakpoint (vector 3).
 */
void isr_breakpoint_handler(register_context_t *ctx)
{
    /* Breakpoint biasanya ditangani debugger */
    TIDAK_DIGUNAKAN_PARAM(ctx);
}

/*
 * isr_overflow_handler
 * --------------------
 * Handler untuk overflow (vector 4).
 */
void isr_overflow_handler(register_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    kernel_printf("\nKESALAHAN: Overflow!\n");
    kernel_printf(IP_FORMAT "\n", IP_CAST(GET_IP(ctx)));
    isr_exception_handler_default(ctx);
}

/*
 * isr_bound_handler
 * -----------------
 * Handler untuk BOUND range exceeded (vector 5).
 */
void isr_bound_handler(register_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    kernel_printf("\nKESALAHAN: Rentang BOUND terlampaui!\n");
    kernel_printf(IP_FORMAT "\n", IP_CAST(GET_IP(ctx)));
    isr_exception_handler_default(ctx);
}

/*
 * isr_invalid_opcode_handler
 * --------------------------
 * Handler untuk invalid opcode (vector 6).
 */
void isr_invalid_opcode_handler(register_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    kernel_printf("\nKESALAHAN: Opcode tidak valid!\n");
    kernel_printf(IP_FORMAT "\n", IP_CAST(GET_IP(ctx)));
    isr_exception_handler_default(ctx);
}

/*
 * isr_device_not_available_handler
 * --------------------------------
 * Handler untuk device not available (vector 7).
 */
void isr_device_not_available_handler(register_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    kernel_printf("\nKESALAHAN: Perangkat tidak tersedia!\n");
    kernel_printf(IP_FORMAT "\n", IP_CAST(GET_IP(ctx)));
    isr_exception_handler_default(ctx);
}

/*
 * isr_double_fault_handler
 * ------------------------
 * Handler untuk double fault (vector 8).
 */
void isr_double_fault_handler(register_context_t *ctx)
{
    kernel_printf("\nKRITIKAL: DOUBLE FAULT!\n");

    if (ctx != NULL) {
        exception_cetak_konteks(ctx);
        exception_decode_kode_error(8, (tak_bertanda32_t)ctx->err_code);
    }

    kernel_panic(__FILE__, __LINE__, "Double fault terjadi");
}

/*
 * isr_invalid_tss_handler
 * -----------------------
 * Handler untuk invalid TSS (vector 10).
 */
void isr_invalid_tss_handler(register_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    kernel_printf("\nKESALAHAN: TSS tidak valid!\n");
    exception_cetak_konteks(ctx);
    exception_decode_kode_error(10, (tak_bertanda32_t)ctx->err_code);
    kernel_panic_exception(ctx);
}

/*
 * isr_segment_not_present_handler
 * -------------------------------
 * Handler untuk segment not present (vector 11).
 */
void isr_segment_not_present_handler(register_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    kernel_printf("\nKESALAHAN: Segment tidak hadir!\n");
    exception_cetak_konteks(ctx);
    exception_decode_kode_error(11, (tak_bertanda32_t)ctx->err_code);
    kernel_panic_exception(ctx);
}

/*
 * isr_stack_segment_handler
 * -------------------------
 * Handler untuk stack segment fault (vector 12).
 */
void isr_stack_segment_handler(register_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    kernel_printf("\nKESALAHAN: Kesalahan segment stack!\n");
    exception_cetak_konteks(ctx);
    exception_decode_kode_error(12, (tak_bertanda32_t)ctx->err_code);
    kernel_panic_exception(ctx);
}

/*
 * isr_general_protection_handler
 * ------------------------------
 * Handler untuk general protection fault (vector 13).
 */
void isr_general_protection_handler(register_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    kernel_printf("\nKESALAHAN: Kesalahan proteksi umum!\n");
    exception_cetak_konteks(ctx);
    exception_decode_kode_error(13, (tak_bertanda32_t)ctx->err_code);
    kernel_panic_exception(ctx);
}

/*
 * isr_page_fault_handler
 * ----------------------
 * Handler untuk page fault (vector 14).
 */
void isr_page_fault_handler(register_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    kernel_printf("\nKESALAHAN: Kesalahan halaman!\n");
    exception_cetak_konteks(ctx);
    exception_decode_kode_error(14, (tak_bertanda32_t)ctx->err_code);
    kernel_panic_exception(ctx);
}

/*
 * isr_fpu_handler
 * ---------------
 * Handler untuk x87 FPU error (vector 16).
 */
void isr_fpu_handler(register_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    kernel_printf("\nKESALAHAN: x87 FPU error!\n");
    kernel_printf(IP_FORMAT "\n", IP_CAST(GET_IP(ctx)));
    isr_exception_handler_default(ctx);
}

/*
 * isr_alignment_check_handler
 * ---------------------------
 * Handler untuk alignment check (vector 17).
 */
void isr_alignment_check_handler(register_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    kernel_printf("\nKESALAHAN: Cek alignment!\n");
    exception_cetak_konteks(ctx);
    exception_decode_kode_error(17, (tak_bertanda32_t)ctx->err_code);
    kernel_panic_exception(ctx);
}

/*
 * isr_machine_check_handler
 * -------------------------
 * Handler untuk machine check (vector 18).
 */
void isr_machine_check_handler(register_context_t *ctx)
{
    kernel_printf("\nKRITIKAL: MACHINE CHECK!\n");

    if (ctx != NULL) {
        exception_cetak_konteks(ctx);
    }

    kernel_panic(__FILE__, __LINE__, "Machine check exception");
}

/*
 * isr_simd_handler
 * ----------------
 * Handler untuk SIMD exception (vector 19).
 */
void isr_simd_handler(register_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    kernel_printf("\nKESALAHAN: SIMD exception!\n");
    kernel_printf(IP_FORMAT "\n", IP_CAST(GET_IP(ctx)));
    isr_exception_handler_default(ctx);
}

/*
 * ============================================================================
 * IRQ HANDLERS DEFAULT (DEFAULT IRQ HANDLERS)
 * ============================================================================
 */

/*
 * isr_timer_handler
 * -----------------
 * Handler untuk timer interrupt (IRQ 0).
 */
void isr_timer_handler(register_context_t *ctx)
{
    /* Timer handler diproses oleh scheduler */
    TIDAK_DIGUNAKAN_PARAM(ctx);

    /* Kirim EOI */
    pic_send_eoi(0);
}

/*
 * isr_keyboard_handler
 * --------------------
 * Handler untuk keyboard interrupt (IRQ 1).
 */
void isr_keyboard_handler(register_context_t *ctx)
{
    TIDAK_DIGUNAKAN_PARAM(ctx);

    /* Baca scancode dari keyboard controller */
    (void)hal_io_read_8(PORT_KEYBOARD_DATA);

    /* Kirim EOI */
    pic_send_eoi(1);
}

/*
 * isr_cascade_handler
 * -------------------
 * Handler untuk cascade interrupt (IRQ 2).
 */
void isr_cascade_handler(register_context_t *ctx)
{
    TIDAK_DIGUNAKAN_PARAM(ctx);

    /* Cascade dari PIC slave, tidak perlu aksi khusus */
    pic_send_eoi(2);
}

/*
 * isr_serial_handler
 * ------------------
 * Handler untuk serial port interrupt.
 */
void isr_serial_handler(register_context_t *ctx)
{
    TIDAK_DIGUNAKAN_PARAM(ctx);

    /* Serial interrupt handling dasar */
    pic_send_eoi(ctx ? (ctx->int_no - IRQ_VEKTOR_BASIS) : 0);
}

/*
 * isr_rtc_handler
 * ---------------
 * Handler untuk RTC interrupt (IRQ 8).
 */
void isr_rtc_handler(register_context_t *ctx)
{
    TIDAK_DIGUNAKAN_PARAM(ctx);

    /* Baca register RTC untuk clear interrupt */
    hal_io_write_8(PORT_CMOS_INDEX, 0x0C);
    (void)hal_io_read_8(PORT_CMOS_DATA);

    /* Kirim EOI */
    pic_send_eoi(8);
}

/*
 * isr_mouse_handler
 * -----------------
 * Handler untuk mouse interrupt (IRQ 12).
 */
void isr_mouse_handler(register_context_t *ctx)
{
    TIDAK_DIGUNAKAN_PARAM(ctx);

    /* Mouse data handling dasar */
    pic_send_eoi(12);
}

/*
 * isr_fpu_handler_irq
 * -------------------
 * Handler untuk FPU interrupt (IRQ 13).
 */
void isr_fpu_handler_irq(register_context_t *ctx)
{
    TIDAK_DIGUNAKAN_PARAM(ctx);

    /* FPU interrupt handling */
    pic_send_eoi(13);
}

/*
 * isr_ata_primary_handler
 * -----------------------
 * Handler untuk primary ATA interrupt (IRQ 14).
 */
void isr_ata_primary_handler(register_context_t *ctx)
{
    TIDAK_DIGUNAKAN_PARAM(ctx);

    /* ATA primary interrupt handling */
    pic_send_eoi(14);
}

/*
 * isr_ata_secondary_handler
 * -------------------------
 * Handler untuk secondary ATA interrupt (IRQ 15).
 */
void isr_ata_secondary_handler(register_context_t *ctx)
{
    TIDAK_DIGUNAKAN_PARAM(ctx);

    /* ATA secondary interrupt handling */
    pic_send_eoi(15);
}

/*
 * ============================================================================
 * SPURIOUS INTERRUPT HANDLING
 * ============================================================================
 */

/*
 * isr_spurious_handler
 * --------------------
 * Handler untuk spurious interrupt.
 */
void isr_spurious_handler(register_context_t *ctx)
{
    g_isr_spurious_count++;

    TIDAK_DIGUNAKAN_PARAM(ctx);

    /* Spurious interrupt tidak memerlukan EOI */
}

/*
 * isr_is_spurious
 * ---------------
 * Cek apakah interrupt adalah spurious.
 */
bool_t isr_is_spurious(tak_bertanda32_t irq)
{
    tak_bertanda8_t irr;
    tak_bertanda8_t isr_val;

    /* Spurious IRQ7 dan IRQ15 terjadi ketika IRQ line tidak aktif */
    if (irq != 7 && irq != 15) {
        return SALAH;
    }

    /* Baca In-Service Register */
    if (irq < 8) {
        hal_io_write_8(PIC1_COMMAND, 0x0B);
        isr_val = hal_io_read_8(PIC1_COMMAND);
        hal_io_write_8(PIC1_COMMAND, 0x0A);
        irr = hal_io_read_8(PIC1_COMMAND);

        /* Jika bit tidak set di ISR dan IRR, ini spurious */
        if (!(isr_val & (1 << irq)) && !(irr & (1 << irq))) {
            return BENAR;
        }
    } else {
        hal_io_write_8(PIC2_COMMAND, 0x0B);
        isr_val = hal_io_read_8(PIC2_COMMAND);
        hal_io_write_8(PIC2_COMMAND, 0x0A);
        irr = hal_io_read_8(PIC2_COMMAND);

        irq -= 8;
        if (!(isr_val & (1 << irq)) && !(irr & (1 << irq))) {
            return BENAR;
        }
    }

    return SALAH;
}
