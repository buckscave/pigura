/*
 * PIGURA OS - KERNEL_MAIN.C
 * -------------------------
 * Entry point utama kernel Pigura OS.
 *
 * Berkas ini berisi fungsi kernel_main yang merupakan titik masuk
 * kernel setelah bootloader menyerahkan kontrol.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "kernel.h"

/*
 * Untuk arsitektur x86/x86_64, include header CPU-specific
 */
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
#include "../arsitektur/x86/cpu_x86.h"
#endif

/*
 * ============================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ============================================================================
 */

info_sistem_t g_info_sistem;
multiboot_info_t *g_boot_info = NULL;
volatile int g_kernel_diinisialisasi = 0;
volatile tak_bertanda64_t g_uptime_ticks = 0;
tak_bertanda64_t g_boot_time = 0;
proses_t *g_proses_sekarang = NULL;
tak_bertanda32_t g_cpu_count = 1;
tak_bertanda32_t g_cpu_id = 0;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

static void kernel_banner(void)
{
    kernel_set_color(VGA_CYAN, VGA_HITAM);
    kernel_printf("\n");
    kernel_printf("========================================");
    kernel_printf("========================================\n");
    kernel_set_color(VGA_PUTIH, VGA_HITAM);
    kernel_printf("    ____  _             ____   _____ \n");
    kernel_printf("   |  _ \\(_) __ _ _ __ |  _ \\ / ____|\n");
    kernel_printf("   | |_) | |/ _` | '_ \\| |_) | |     \n");
    kernel_printf("   |  __/| | (_| | | | |  __/| |____ \n");
    kernel_printf("   |_|   |_|\\__,_|_| |_|_|    \\_____|\n");
    kernel_printf("\n");
    kernel_set_color(VGA_HIJAU_TERANG, VGA_HITAM);
    kernel_printf("   %s - %s\n", PIGURA_NAMA, PIGURA_JULUKAN);
    kernel_set_color(VGA_ABU_TERANG, VGA_HITAM);
    kernel_printf("   Versi %s | Arsitektur: %s\n",
                  PIGURA_VERSI_STRING, NAMA_ARSITEKTUR);
    kernel_set_color(VGA_CYAN, VGA_HITAM);
    kernel_printf("========================================");
    kernel_printf("========================================\n");
    kernel_set_color(VGA_ABU_ABU, VGA_HITAM);
    kernel_printf("\n");
}

static void kernel_info_init(multiboot_info_t *bootinfo)
{
    kernel_memset(&g_info_sistem, 0, sizeof(info_sistem_t));

    kernel_strncpy(g_info_sistem.nama, PIGURA_NAMA, 31);
    kernel_strncpy(g_info_sistem.versi, PIGURA_VERSI_STRING, 15);
    kernel_strncpy(g_info_sistem.arsitektur, NAMA_ARSITEKTUR, 15);
    kernel_strncpy(g_info_sistem.hostname, CONFIG_HOSTNAME_DEFAULT, 63);

    g_info_sistem.uptime = 0;
    g_info_sistem.boot_time = 0;
    g_info_sistem.cpu_count = 1;

    if (bootinfo != NULL) {
        if (bootinfo->flags & MULTIBOOT_FLAG_MEM) {
            g_info_sistem.memori.total = bootinfo->mem_lower * 1024;
            g_info_sistem.memori.total += bootinfo->mem_upper * 1024;
            g_info_sistem.memori.tersedia = g_info_sistem.memori.total;
        }
    }

    g_info_sistem.proses_count = 0;
    g_info_sistem.thread_count = 0;
}

static status_t kernel_selftest(void)
{
    void *ptr1;
    void *ptr2;

    kernel_printf("[KERNEL] Menjalankan self-test...\n");

    ptr1 = (void *)0x200000;
    ptr2 = (void *)0x201000;

    kernel_memset(ptr1, 0xAA, 1024);
    kernel_memset(ptr2, 0xBB, 1024);

    if (kernel_memcmp(ptr1, ptr2, 1024) == 0) {
        kernel_printf("[KERNEL] ERROR: Self-test memori gagal\n");
        return STATUS_GAGAL;
    }

    kernel_printf("[KERNEL] Self-test berhasil\n");

    return STATUS_BERHASIL;
}

static void kernel_idle(void)
{
    kernel_printf("[KERNEL] Memasuki idle loop\n");

    for (;;) {
        cpu_halt();
    }
}

/*
 * ============================================================================
 * FUNGSI UTAMA KERNEL (MAIN KERNEL FUNCTIONS)
 * ============================================================================
 */

void kernel_main(tak_bertanda32_t magic, multiboot_info_t *bootinfo)
{
    status_t status;

    g_boot_info = bootinfo;

    if (magic != MULTIBOOT_MAGIC) {
        kernel_halt();
    }

    status = hal_init();
    if (status != STATUS_BERHASIL) {
        kernel_halt();
    }

    kernel_banner();

    kernel_info_init(bootinfo);

    status = kernel_init(bootinfo);
    if (status != STATUS_BERHASIL) {
        kernel_printf("[KERNEL] KRITIKAL: Inisialisasi gagal: %d\n", status);
        kernel_halt();
    }

    status = kernel_selftest();
    if (status != STATUS_BERHASIL) {
        kernel_printf("[KERNEL] PERINGATAN: Self-test gagal\n");
    }

    g_kernel_diinisialisasi = 1;

    kernel_printf("[KERNEL] Inisialisasi selesai\n");
    kernel_printf("[KERNEL] Memulai operasi normal\n\n");

    kernel_start();

    kernel_idle();
}

status_t kernel_init(multiboot_info_t *bootinfo)
{
    status_t status;

    kernel_printf("[KERNEL] Memulai inisialisasi kernel...\n");

    kernel_printf("[KERNEL] Inisialisasi memori...\n");
    status = hal_memory_init(bootinfo);
    if (status != STATUS_BERHASIL) {
        kernel_printf("[KERNEL] Gagal inisialisasi memori: %d\n", status);
        return status;
    }

    {
        const hal_state_t *hal = hal_get_state();
        if (hal != NULL) {
            g_info_sistem.memori.total = hal->memory.total_bytes;
            g_info_sistem.memori.tersedia = hal->memory.available_bytes;
        }
    }

    kernel_printf("[KERNEL] Mengaktifkan interupsi...\n");
    hal_cpu_enable_interrupts();

    kernel_printf("[KERNEL] Inisialisasi kernel selesai\n");

    return STATUS_BERHASIL;
}

void kernel_start(void)
{
    kernel_printf("[KERNEL] Sistem siap\n");

    kernel_idle();
}

void kernel_shutdown(int reboot)
{
    kernel_printf("[KERNEL] Shutdown dimulai...\n");

    hal_cpu_disable_interrupts();

    hal_shutdown();

    if (reboot) {
        kernel_printf("[KERNEL] Reboot...\n");
        hal_cpu_reset(BENAR);
    } else {
        kernel_printf("[KERNEL] Halt...\n");
        hal_cpu_halt();
    }
}

/*
 * ============================================================================
 * FUNGSI UTILITAS KERNEL (KERNEL UTILITY FUNCTIONS)
 * ============================================================================
 */

void kernel_delay(tak_bertanda32_t ms)
{
    hal_timer_sleep(ms);
}

void kernel_sleep(tak_bertanda32_t ms)
{
    hal_timer_sleep(ms);
}

tak_bertanda64_t kernel_get_uptime(void)
{
    return hal_timer_get_uptime();
}

tak_bertanda64_t kernel_get_ticks(void)
{
    return hal_timer_get_ticks();
}

const info_sistem_t *kernel_get_info(void)
{
    return &g_info_sistem;
}

const char *kernel_get_arch(void)
{
    return NAMA_ARSITEKTUR;
}

const char *kernel_get_version(void)
{
    return PIGURA_VERSI_STRING;
}
