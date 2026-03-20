/*
 * PIGURA OS - PANIC.C
 * -------------------
 * Implementasi fungsi kernel panic.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk menangani
 * kondisi error fatal dan menampilkan informasi debugging.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../kernel.h"
#include "../panic.h"

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
panic_info_t g_panic_info_terakhir = {0};

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
 *
 * Parameter:
 *   info - Pointer ke struktur panic_info_t
 */
static void panic_print_info(const panic_info_t *info)
{
    kernel_printf("\n");

    /* Kode error */
    kernel_printf("Kode Error: 0x%04X\n", info->kode);

    /* Lokasi */
    kernel_printf("Lokasi: %s:%d\n", info->file, info->baris);

    /* Pesan */
    kernel_printf("Pesan: %s\n", info->pesan);

    /* Alamat fault */
    if (info->alamat_fault != 0) {
        kernel_printf("Alamat Fault: 0x%08lX\n",
                      (alamat_ptr_t)info->alamat_fault);
    }

    /* Error code */
    if (info->error_code != 0) {
        kernel_printf("Error Code: 0x%08lX\n", info->error_code);
    }

    kernel_printf("\n");
}

/*
 * panic_print_register
 * --------------------
 * Print nilai register.
 *
 * Parameter:
 *   info - Pointer ke struktur panic_info_t
 */
static void panic_print_register(const panic_info_t *info)
{
    kernel_printf("Register:\n");
    kernel_printf("  EAX=0x%08lX  EBX=0x%08lX  ECX=0x%08lX  EDX=0x%08lX\n",
                  info->eax, info->ebx, info->ecx, info->edx);
    kernel_printf("  ESI=0x%08lX  EDI=0x%08lX  EBP=0x%08lX  ESP=0x%08lX\n",
                  info->esi, info->edi, info->ebp, info->esp);
    kernel_printf("  EIP=0x%08lX  EFLAGS=0x%08lX\n",
                  info->eip, info->eflags);
    kernel_printf("  CS=0x%04lX  DS=0x%04lX  ES=0x%04lX  FS=0x%04lX  GS=0x%04lX  SS=0x%04lX\n",
                  info->cs, info->ds, info->es, info->fs, info->gs, info->ss);
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
    tak_bertanda32_t flags;

    /* Disable interrupt */
    flags = cpu_save_flags();

    /* Cek jika sudah panic */
    if (g_panic_aktif) {
        /* Double panic, halt langsung */
        cpu_halt();
        for (;;) {}
    }

    g_panic_aktif = 1;
    g_panic_hitung++;

    /* Clear informasi */
    kernel_memset(&g_panic_info_terakhir, 0, sizeof(panic_info_t));

    /* Set file dan baris */
    if (file != NULL) {
        ukuran_t len = kernel_strlen(file);
        ukuran_t copy_len = MIN(len, PANIC_FILE_MAX - 1);
        kernel_strncpy(g_panic_info_terakhir.file, file, copy_len);
        g_panic_info_terakhir.file[copy_len] = '\0';
    }
    g_panic_info_terakhir.baris = (tak_bertanda16_t)baris;

    /* Format pesan */
    if (format != NULL) {
        va_start(args, format);
        vsnprintf(g_panic_info_terakhir.pesan, PANIC_PESAN_MAX - 1,
                  format, args);
        va_end(args);
    }

    /* Print banner */
    panic_print_banner();

    /* Print informasi */
    panic_print_info(&g_panic_info_terakhir);

    /* Print instruksi */
    kernel_set_color(VGA_KUNING, VGA_HITAM);
    kernel_printf("Sistem telah berhenti. Silakan restart komputer.\n");
    kernel_set_color(VGA_ABU_ABU, VGA_HITAM);

    kernel_printf("\nTekan tombol untuk restart...\n");

    /* Halt */
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

    /* Set kode error */
    g_panic_info_terakhir.kode = kode;

    /* Panggil panic utama */
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
    tak_bertanda32_t flags;

    if (info == NULL) {
        kernel_panic(__FILE__, __LINE__, "Panic info NULL");
        return;
    }

    /* Disable interrupt */
    flags = cpu_save_flags();

    if (g_panic_aktif) {
        cpu_halt();
        for (;;) {}
    }

    g_panic_aktif = 1;
    g_panic_hitung++;

    /* Copy informasi */
    kernel_memcpy(&g_panic_info_terakhir, info, sizeof(panic_info_t));

    /* Print */
    panic_print_banner();
    panic_print_info(&g_panic_info_terakhir);
    panic_print_register(&g_panic_info_terakhir);

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
        return;
    }

    /* Isi informasi dari context */
    kernel_memset(&info, 0, sizeof(panic_info_t));

    info.kode = (tak_bertanda16_t)ctx->int_no;
    info.error_code = ctx->err_code;
    info.eax = ctx->eax;
    info.ebx = ctx->ebx;
    info.ecx = ctx->ecx;
    info.edx = ctx->edx;
    info.esi = ctx->esi;
    info.edi = ctx->edi;
    info.ebp = ctx->ebp;
    info.esp = ctx->useresp;
    info.eip = ctx->eip;
    info.eflags = ctx->eflags;
    info.cs = ctx->cs;
    info.ds = ctx->ds;
    info.es = ctx->es;
    info.fs = ctx->fs;
    info.gs = ctx->gs;
    info.ss = ctx->ss;

    /* Dapatkan CR2 untuk page fault */
    info.cr2 = cpu_read_cr2();

    /* Set pesan berdasarkan exception */
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

    kernel_panic_register(&info);
}

/*
 * kernel_halt
 * -----------
 * Hentikan sistem.
 */
void kernel_halt(void)
{
    /* Disable interrupt */
    cpu_disable_interrupts();

    /* Halt */
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

    /* Tidak akan sampai sini */
    for (;;) {
        cpu_halt();
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
                  ctx->int_no, ctx->err_code);
    kernel_printf("  EAX=0x%08lX  EBX=0x%08lX  ECX=0x%08lX  EDX=0x%08lX\n",
                  ctx->eax, ctx->ebx, ctx->ecx, ctx->edx);
    kernel_printf("  ESI=0x%08lX  EDI=0x%08lX  EBP=0x%08lX  ESP=0x%08lX\n",
                  ctx->esi, ctx->edi, ctx->ebp, ctx->esp);
    kernel_printf("  EIP=0x%08lX  CS=0x%04lX  DS=0x%04lX  ES=0x%04lX\n",
                  ctx->eip, ctx->cs, ctx->ds, ctx->es);
    kernel_printf("  FS=0x%04lX  GS=0x%04lX  SS=0x%04lX  EFLAGS=0x%08lX\n",
                  ctx->fs, ctx->gs, ctx->ss, ctx->eflags);
}

/*
 * panic_dump_stack
 * ----------------
 * Tampilkan dump stack.
 */
void panic_dump_stack(tak_bertanda32_t esp, int depth)
{
    tak_bertanda32_t *ptr;
    int i;

    kernel_printf("Stack Dump (ESP=0x%08lX):\n", esp);

    ptr = (tak_bertanda32_t *)esp;

    for (i = 0; i < depth; i++) {
        if ((tak_bertanda32_t)(ptr + i) < 0x100000) {
            break;  /* Jangan baca di bawah 1 MB */
        }

        if (i % 4 == 0) {
            kernel_printf("\n  0x%08lX: ", (tak_bertanda32_t)(ptr + i));
        }

        kernel_printf("0x%08lX ", ptr[i]);
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

    kernel_printf("Memory Dump (0x%08lX - 0x%08lX):\n",
                  (alamat_ptr_t)addr, (alamat_ptr_t)addr + len);

    for (i = 0; i < len; i += 16) {
        /* Print address */
        kernel_printf("  %08lX  ", (alamat_ptr_t)(ptr + i));

        /* Print hex */
        for (j = 0; j < 16 && i + j < len; j++) {
            kernel_printf("%02X ", ptr[i + j]);
        }

        /* Print ASCII */
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
