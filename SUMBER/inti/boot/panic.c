/*
 * PIGURA OS - PANIC.C
 * -------------------
 * Implementasi fungsi kernel panic.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk menangani
 * kondisi error fatal dan menampilkan informasi debugging.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 *
 * Versi: 2.0
 * Tanggal: 2025
 */

#include "../kernel.h"

/*
 * ============================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ============================================================================
 */

/* Flag panic aktif */
volatile int g_panic_aktif = 0;

/* Jumlah panic yang terjadi */
volatile tak_bertanda32_t g_panic_hitung = 0;

/* Informasi panic terakhir */
panic_info_t g_panic_info_terakhir;

/* Nested panic count */
volatile tak_bertanda32_t g_panic_nested = 0;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * panic_print_banner
 * ------------------
 * Print banner panic.
 */
static void panic_print_banner(void)
{
    kernel_set_color(VGA_PUTIH, VGA_MERAH);
    kernel_printf("\n");
    kernel_printf("****************************************");
    kernel_printf("****************************************\n");
    kernel_printf("        KRITIKAL: KERNEL PANIC!\n");
    kernel_printf("****************************************");
    kernel_printf("****************************************\n");
    kernel_set_color(VGA_ABU_ABU, VGA_HITAM);
}

/*
 * panic_print_info
 * ----------------
 * Print informasi panic.
 */
static void panic_print_info(const panic_info_t *info)
{
    kernel_printf("\n");

    kernel_printf("Kode Error: 0x%04X\n", info->kode);

    if (info->file[0] != '\0') {
        kernel_printf("Lokasi: %s:%lu\n", info->file, (ukuran_t)info->baris);
    }

    if (info->fungsi[0] != '\0') {
        kernel_printf("Fungsi: %s\n", info->fungsi);
    }

    kernel_printf("Pesan: %s\n", info->pesan);

    if (info->alamat_fault != 0) {
        kernel_printf("Alamat Fault: 0x%08lX\n",
                      (ukuran_t)info->alamat_fault);
    }

    if (info->error_code != 0) {
        kernel_printf("Error Code: 0x%08lX\n", 
                      (ukuran_t)info->error_code);
    }

    if (info->nomor_vector != 0) {
        kernel_printf("Vector: %lu\n", (ukuran_t)info->nomor_vector);
    }

    kernel_printf("\n");
}

/*
 * panic_print_register_from_ctx
 * -----------------------------
 * Print nilai register dari register_context_t.
 */
static void panic_print_register_from_ctx(const register_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    kernel_printf("Register:\n");
    kernel_printf("  EAX=0x%08lX  EBX=0x%08lX  ECX=0x%08lX  EDX=0x%08lX\n",
                  (ukuran_t)ctx->eax, (ukuran_t)ctx->ebx,
                  (ukuran_t)ctx->ecx, (ukuran_t)ctx->edx);
    kernel_printf("  ESI=0x%08lX  EDI=0x%08lX  EBP=0x%08lX  ESP=0x%08lX\n",
                  (ukuran_t)ctx->esi, (ukuran_t)ctx->edi,
                  (ukuran_t)ctx->ebp, (ukuran_t)ctx->esp);
    kernel_printf("  EIP=0x%08lX  EFLAGS=0x%08lX\n",
                  (ukuran_t)ctx->eip, (ukuran_t)ctx->eflags);
    kernel_printf("  CS=0x%04lX  DS=0x%04lX  ES=0x%04lX  FS=0x%04lX"
                  "  GS=0x%04lX  SS=0x%04lX\n",
                  (ukuran_t)ctx->cs, (ukuran_t)ctx->ds,
                  (ukuran_t)ctx->es, (ukuran_t)ctx->fs,
                  (ukuran_t)ctx->gs, (ukuran_t)ctx->ss);

    /* Tampilkan CR2 untuk page fault */
    if (ctx->int_no == VEKTOR_PF) {
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
        tak_bertanda32_t cr2;
        __asm__ __volatile__("mov %%cr2, %0" : "=r"(cr2));
        kernel_printf("  CR2=0x%08lX (alamat fault)\n", (ukuran_t)cr2);
#endif
    }

    kernel_printf("\n");
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * kernel_panic
 * ------------
 * Fungsi utama kernel panic.
 */
void kernel_panic(const char *file, int baris, const char *format, ...)
{
    va_list args;
    ukuran_t len;
    ukuran_t copy_len;

    /* Disable interrupt */
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    cpu_disable_irq();
#endif

    /* Cek nested panic */
    if (g_panic_aktif) {
        g_panic_nested++;
        /* Hentikan jika nested panic terlalu banyak */
        if (g_panic_nested > 3) {
            cpu_halt();
            for (;;) {}
        }
    }

    g_panic_aktif = 1;
    g_panic_hitung++;

    /* Reset info */
    kernel_memset(&g_panic_info_terakhir, 0, sizeof(panic_info_t));

    /* Set file dan baris */
    if (file != NULL) {
        len = kernel_strlen(file);
        copy_len = MIN(len, PANIC_FILE_MAX - 1);
        kernel_strncpy(g_panic_info_terakhir.file, file, copy_len);
        g_panic_info_terakhir.file[copy_len] = '\0';
    }
    g_panic_info_terakhir.baris = (tak_bertanda32_t)baris;

    /* Set pesan */
    if (format != NULL) {
        va_start(args, format);
        kernel_vsnprintf(g_panic_info_terakhir.pesan, PANIC_PESAN_MAX - 1,
                         format, args);
        va_end(args);
    }

    /* Set timestamp */
    g_panic_info_terakhir.timestamp = kernel_get_ticks();

    /* Print panic */
    panic_print_banner();
    panic_print_info(&g_panic_info_terakhir);

    kernel_set_color(VGA_KUNING, VGA_HITAM);
    kernel_printf("Sistem telah berhenti. Silakan restart komputer.\n");
    kernel_set_color(VGA_ABU_ABU, VGA_HITAM);

    kernel_printf("\nTekan tombol untuk restart...\n");

    /* Halt forever */
    for (;;) {
        cpu_halt();
    }
}

/*
 * kernel_panic_kode
 * -----------------
 * Kernel panic dengan kode error.
 */
void kernel_panic_kode(tak_bertanda16_t kode, const char *file, int baris,
                       const char *format, ...)
{
    va_list args;

    g_panic_info_terakhir.kode = kode;

    if (format != NULL) {
        va_start(args, format);
        kernel_panic(file, baris, format, args);
        va_end(args);
    } else {
        kernel_panic(file, baris, "Kode error: 0x%04X", kode);
    }
}

/*
 * kernel_panic_fungsi
 * -------------------
 * Kernel panic dengan informasi fungsi.
 */
void kernel_panic_fungsi(tak_bertanda16_t kode, const char *file, int baris,
                         const char *fungsi, const char *format, ...)
{
    va_list args;
    ukuran_t len;
    ukuran_t copy_len;

    /* Set kode */
    g_panic_info_terakhir.kode = kode;

    /* Set fungsi */
    if (fungsi != NULL) {
        len = kernel_strlen(fungsi);
        copy_len = MIN(len, PANIC_FUNC_MAX - 1);
        kernel_strncpy(g_panic_info_terakhir.fungsi, fungsi, copy_len);
        g_panic_info_terakhir.fungsi[copy_len] = '\0';
    }

    if (format != NULL) {
        va_start(args, format);
        kernel_panic(file, baris, format, args);
        va_end(args);
    } else {
        kernel_panic(file, baris, "Kode error: 0x%04X", kode);
    }
}

/*
 * kernel_panic_register
 * ---------------------
 * Kernel panic dengan informasi register.
 */
void kernel_panic_register(const panic_info_t *info)
{
    if (info == NULL) {
        kernel_panic(__FILE__, __LINE__, "Panic info NULL");
    }

    /* Disable interrupt */
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    cpu_disable_irq();
#endif

    /* Cek nested panic */
    if (g_panic_aktif) {
        g_panic_nested++;
        if (g_panic_nested > 3) {
            cpu_halt();
            for (;;) {}
        }
    }

    g_panic_aktif = 1;
    g_panic_hitung++;

    /* Copy info */
    kernel_memcpy(&g_panic_info_terakhir, info, sizeof(panic_info_t));

    /* Print panic */
    panic_print_banner();
    panic_print_info(&g_panic_info_terakhir);

    kernel_set_color(VGA_KUNING, VGA_HITAM);
    kernel_printf("Sistem telah berhenti.\n");
    kernel_set_color(VGA_ABU_ABU, VGA_HITAM);

    for (;;) {
        cpu_halt();
    }
}

/*
 * kernel_panic_exception
 * ----------------------
 * Kernel panic dari exception handler.
 */
void kernel_panic_exception(const register_context_t *ctx)
{
    panic_info_t info;

    if (ctx == NULL) {
        kernel_panic(__FILE__, __LINE__, "Exception context NULL");
    }

    kernel_memset(&info, 0, sizeof(panic_info_t));

    info.kode = (tak_bertanda16_t)ctx->int_no;
    info.error_code = ctx->err_code;
    info.nomor_vector = ctx->int_no;
    info.alamat_fault = (uintptr_t)ctx->eip;
    info.timestamp = kernel_get_ticks();

    /* Set pesan berdasarkan vector */
    switch (ctx->int_no) {
        case VEKTOR_DE:
            kernel_strncpy(info.pesan, "Divide Error", PANIC_PESAN_MAX - 1);
            break;
        case VEKTOR_DB:
            kernel_strncpy(info.pesan, "Debug Exception", PANIC_PESAN_MAX - 1);
            break;
        case VEKTOR_NMI:
            kernel_strncpy(info.pesan, "Non-Maskable Interrupt",
                           PANIC_PESAN_MAX - 1);
            break;
        case VEKTOR_BP:
            kernel_strncpy(info.pesan, "Breakpoint", PANIC_PESAN_MAX - 1);
            break;
        case VEKTOR_OF:
            kernel_strncpy(info.pesan, "Overflow", PANIC_PESAN_MAX - 1);
            break;
        case VEKTOR_BR:
            kernel_strncpy(info.pesan, "BOUND Range Exceeded",
                           PANIC_PESAN_MAX - 1);
            break;
        case VEKTOR_UD:
            kernel_strncpy(info.pesan, "Invalid Opcode", PANIC_PESAN_MAX - 1);
            break;
        case VEKTOR_NM:
            kernel_strncpy(info.pesan, "Device Not Available",
                           PANIC_PESAN_MAX - 1);
            break;
        case VEKTOR_DF:
            kernel_strncpy(info.pesan, "Double Fault", PANIC_PESAN_MAX - 1);
            break;
        case VEKTOR_TS:
            kernel_strncpy(info.pesan, "Invalid TSS", PANIC_PESAN_MAX - 1);
            break;
        case VEKTOR_NP:
            kernel_strncpy(info.pesan, "Segment Not Present",
                           PANIC_PESAN_MAX - 1);
            break;
        case VEKTOR_SS:
            kernel_strncpy(info.pesan, "Stack-Segment Fault",
                           PANIC_PESAN_MAX - 1);
            break;
        case VEKTOR_GP:
            kernel_strncpy(info.pesan, "General Protection Fault",
                           PANIC_PESAN_MAX - 1);
            break;
        case VEKTOR_PF:
            kernel_strncpy(info.pesan, "Page Fault", PANIC_PESAN_MAX - 1);
            break;
        case VEKTOR_MF:
            kernel_strncpy(info.pesan, "x87 FPU Error", PANIC_PESAN_MAX - 1);
            break;
        case VEKTOR_AC:
            kernel_strncpy(info.pesan, "Alignment Check", PANIC_PESAN_MAX - 1);
            break;
        case VEKTOR_MC:
            kernel_strncpy(info.pesan, "Machine Check", PANIC_PESAN_MAX - 1);
            break;
        case VEKTOR_XM:
            kernel_strncpy(info.pesan, "SIMD Exception", PANIC_PESAN_MAX - 1);
            break;
        default:
            kernel_strncpy(info.pesan, "Unknown Exception", PANIC_PESAN_MAX - 1);
            break;
    }

    kernel_strncpy(info.file, "exception", PANIC_FILE_MAX - 1);
    info.baris = 0;

    /* Disable interrupt */
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    cpu_disable_irq();
#endif

    /* Cek nested panic */
    if (g_panic_aktif) {
        g_panic_nested++;
        if (g_panic_nested > 3) {
            cpu_halt();
            for (;;) {}
        }
    }

    g_panic_aktif = 1;
    g_panic_hitung++;

    kernel_memcpy(&g_panic_info_terakhir, &info, sizeof(panic_info_t));

    /* Print panic dengan register dump */
    panic_print_banner();
    panic_print_info(&g_panic_info_terakhir);
    panic_print_register_from_ctx(ctx);

    kernel_set_color(VGA_KUNING, VGA_HITAM);
    kernel_printf("Sistem telah berhenti.\n");
    kernel_set_color(VGA_ABU_ABU, VGA_HITAM);

    for (;;) {
        cpu_halt();
    }
}

/*
 * kernel_halt
 * -----------
 * Hentikan sistem.
 */
void kernel_halt(void)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    cpu_disable_irq();
#endif

    for (;;) {
        cpu_halt();
    }
}

/*
 * kernel_reboot
 * -------------
 * Reboot sistem.
 */
void kernel_reboot(int force)
{
    hal_cpu_reset(force ? BENAR : SALAH);

    for (;;) {
        cpu_halt();
    }
}

/*
 * kernel_shutdown
 * ---------------
 * Shutdown sistem dengan aman.
 */
void kernel_shutdown(int reboot)
{
    if (reboot) {
        kernel_reboot(0);
    } else {
        kernel_halt();
    }
}

/*
 * panic_dump_register
 * -------------------
 * Tampilkan dump register.
 */
void panic_dump_register(const register_context_t *ctx)
{
    if (ctx == NULL) {
        kernel_printf("Context NULL\n");
        return;
    }

    kernel_printf("Register Dump:\n");
    kernel_printf("  INT=0x%02lX  ERR=0x%08lX\n",
                  (ukuran_t)ctx->int_no, (ukuran_t)ctx->err_code);
    kernel_printf("  EAX=0x%08lX  EBX=0x%08lX  ECX=0x%08lX  EDX=0x%08lX\n",
                  (ukuran_t)ctx->eax, (ukuran_t)ctx->ebx,
                  (ukuran_t)ctx->ecx, (ukuran_t)ctx->edx);
    kernel_printf("  ESI=0x%08lX  EDI=0x%08lX  EBP=0x%08lX  ESP=0x%08lX\n",
                  (ukuran_t)ctx->esi, (ukuran_t)ctx->edi,
                  (ukuran_t)ctx->ebp, (ukuran_t)ctx->esp);
    kernel_printf("  EIP=0x%08lX  CS=0x%04lX  DS=0x%04lX  ES=0x%04lX\n",
                  (ukuran_t)ctx->eip, (ukuran_t)ctx->cs,
                  (ukuran_t)ctx->ds, (ukuran_t)ctx->es);
    kernel_printf("  FS=0x%04lX  GS=0x%04lX  SS=0x%04lX  EFLAGS=0x%08lX\n",
                  (ukuran_t)ctx->fs, (ukuran_t)ctx->gs,
                  (ukuran_t)ctx->ss, (ukuran_t)ctx->eflags);
}

/*
 * panic_dump_stack
 * ----------------
 * Tampilkan dump stack.
 */
void panic_dump_stack(uintptr_t esp, int depth)
{
    tak_bertanda32_t *ptr;
    int i;

    kernel_printf("Stack Dump (ESP=0x%08lX):\n", (ukuran_t)esp);

    ptr = (tak_bertanda32_t *)esp;

    for (i = 0; i < depth; i++) {
        /* Cek batas memori yang aman */
        if ((uintptr_t)(ptr + i) < 0x100000) {
            break;
        }

        if (i % 4 == 0) {
            kernel_printf("\n  0x%08lX: ", (ukuran_t)(uintptr_t)(ptr + i));
        }

        kernel_printf("0x%08lX ", (ukuran_t)ptr[i]);
    }

    kernel_printf("\n");
}

/*
 * panic_dump_stack_trace
 * ----------------------
 * Tampilkan stack trace (call stack).
 */
void panic_dump_stack_trace(uintptr_t ebp, int depth)
{
    uintptr_t *frame;
    int i;

    kernel_printf("Stack Trace:\n");

    frame = (uintptr_t *)ebp;

    for (i = 0; i < depth && frame != NULL; i++) {
        /* Cek batas memori */
        if ((uintptr_t)frame < 0x100000) {
            break;
        }

        kernel_printf("  #%d: 0x%08lX\n", i, (ukuran_t)frame[1]);

        frame = (uintptr_t *)frame[0];
    }

    kernel_printf("\n");
}

/*
 * panic_dump_memory
 * -----------------
 * Tampilkan dump memory.
 */
void panic_dump_memory(void *addr, ukuran_t len)
{
    tak_bertanda8_t *ptr;
    ukuran_t i;
    ukuran_t j;

    if (addr == NULL || len == 0) {
        return;
    }

    ptr = (tak_bertanda8_t *)addr;

    kernel_printf("Memory Dump (0x%p - 0x%p):\n",
                  addr, (void *)((uintptr_t)addr + len));

    for (i = 0; i < len; i += 16) {
        kernel_printf("  %08lX  ", (ukuran_t)(uintptr_t)(ptr + i));

        for (j = 0; j < 16 && i + j < len; j++) {
            kernel_printf("%02X ", ptr[i + j]);
        }

        kernel_printf(" |");
        for (j = 0; j < 16 && i + j < len; j++) {
            char c = ptr[i + j];
            if (c >= 32 && c < 127) {
                kernel_putchar(c);
            } else {
                kernel_putchar('.');
            }
        }
        kernel_printf("|\n");
    }
}

/*
 * panic_dump_memory_ascii
 * -----------------------
 * Tampilkan dump memory dengan ASCII.
 */
void panic_dump_memory_ascii(void *addr, ukuran_t len)
{
    tak_bertanda8_t *ptr;
    ukuran_t i;

    if (addr == NULL || len == 0) {
        return;
    }

    ptr = (tak_bertanda8_t *)addr;

    kernel_printf("Memory ASCII (0x%p):\n", addr);

    kernel_printf("  \"");
    for (i = 0; i < len; i++) {
        char c = ptr[i];
        if (c >= 32 && c < 127) {
            kernel_putchar(c);
        } else if (c == '\0') {
            kernel_printf("\\0");
        } else if (c == '\n') {
            kernel_printf("\\n");
        } else if (c == '\t') {
            kernel_printf("\\t");
        } else {
            kernel_putchar('.');
        }
    }
    kernel_printf("\"\n");
}

/*
 * panic_set_info
 * --------------
 * Set informasi panic tambahan.
 */
void panic_set_info(panic_info_t *info)
{
    if (info != NULL) {
        kernel_memcpy(&g_panic_info_terakhir, info, sizeof(panic_info_t));
    }
}

/*
 * panic_get_info
 * --------------
 * Dapatkan informasi panic terakhir.
 */
const panic_info_t *panic_get_info(void)
{
    return &g_panic_info_terakhir;
}

/*
 * panic_set_callback
 * ------------------
 * Set callback yang dipanggil saat panic.
 */
static void (*g_panic_callback)(const panic_info_t *) = NULL;

void panic_set_callback(void (*callback)(const panic_info_t *))
{
    g_panic_callback = callback;
}

/*
 * panic_is_active
 * ---------------
 * Cek apakah sedang dalam kondisi panic.
 */
bool_t panic_is_active(void)
{
    return (g_panic_aktif != 0) ? BENAR : SALAH;
}
