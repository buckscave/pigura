/*
 * PIGURA OS - KERNEL_MAIN.C
 * -------------------------
 * Entry point utama kernel Pigura OS.
 *
 * Berkas ini berisi fungsi kernel_main yang merupakan titik masuk
 * kernel setelah bootloader menyerahkan kontrol. Fungsi ini melakukan
 * inisialisasi semua subsistem dan memulai operasi normal.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../kernel.h"
#include "../hal/hal.h"

/*
 * ============================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ============================================================================
 */

/* Informasi sistem global */
info_sistem_t g_info_sistem = {0};

/* Boot info dari multiboot */
multiboot_info_t *g_boot_info = NULL;

/* Flag kernel sudah diinisialisasi */
volatile int g_kernel_diinisialisasi = 0;

/* Uptime counter (dalam ticks) */
volatile tak_bertanda64_t g_uptime_ticks = 0;

/* Waktu boot */
tak_bertanda64_t g_boot_time = 0;

/* Pointer ke proses saat ini */
struct pcb *g_proses_sekarang = NULL;

/* Jumlah CPU aktif */
tak_bertanda32_t g_cpu_count = 1;

/* CPU ID saat ini */
tak_bertanda32_t g_cpu_id = 0;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * kernel_banner
 * -------------
 * Tampilkan banner kernel.
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

/*
 * kernel_info_init
 * ----------------
 * Inisialisasi struktur informasi sistem.
 */
static void kernel_info_init(multiboot_info_t *bootinfo)
{
    kernel_memset(&g_info_sistem, 0, sizeof(info_sistem_t));

    /* Set nama dan versi */
    kernel_strncpy(g_info_sistem.nama, PIGURA_NAMA, 31);
    kernel_strncpy(g_info_sistem.versi, PIGURA_VERSI_STRING, 15);
    kernel_strncpy(g_info_sistem.arsitektur, NAMA_ARSITEKTUR, 15);
    kernel_strncpy(g_info_sistem.hostname, CONFIG_HOSTNAME_DEFAULT, 63);

    /* Set uptime dan boot time */
    g_info_sistem.uptime = 0;
    g_info_sistem.boot_time = 0;  /* Akan diset dari RTC nanti */

    /* Set CPU count */
    g_info_sistem.cpu_count = 1;  /* Default, akan diupdate dari HAL */

    /* Set informasi memori dari bootinfo */
    if (bootinfo != NULL) {
        if (bootinfo->flags & MULTIBOOT_FLAG_MEM) {
            g_info_sistem.memori.total = bootinfo->mem_lower * 1024;
            g_info_sistem.memori.total += bootinfo->mem_upper * 1024;
            g_info_sistem.memori.tersedia = g_info_sistem.memori.total;
        }
    }

    /* Set process count */
    g_info_sistem.proses_count = 0;
    g_info_sistem.thread_count = 0;
}

/*
 * kernel_selftest
 * ---------------
 * Jalankan self-test dasar kernel.
 *
 * Return: Status operasi
 */
static status_t kernel_selftest(void)
{
    kernel_printf("[KERNEL] Menjalankan self-test...\n");

    /* Test memori */
    {
        void *ptr1 = kernel_memset((void *)0x200000, 0xAA, 1024);
        void *ptr2 = kernel_memset((void *)0x201000, 0xBB, 1024);

        if (kernel_memcmp(ptr1, ptr2, 1024) == 0) {
            kernel_printf("[KERNEL] ERROR: Self-test memori gagal\n");
            return STATUS_GAGAL;
        }
    }

    kernel_printf("[KERNEL] Self-test berhasil\n");

    return STATUS_BERHASIL;
}

/*
 * kernel_idle
 * -----------
 * Idle loop kernel.
 * Dipanggil ketika tidak ada proses yang siap dijalankan.
 */
static void kernel_idle(void)
{
    kernel_printf("[KERNEL] Memasuki idle loop\n");

    for (;;) {
        /* Halt CPU sampai interupsi berikutnya */
        cpu_halt();
    }
}

/*
 * ============================================================================
 * FUNGSI UTAMA KERNEL (MAIN KERNEL FUNCTIONS)
 * ============================================================================
 */

/*
 * kernel_main
 * -----------
 * Entry point utama kernel.
 */
void kernel_main(tak_bertanda32_t magic, multiboot_info_t *bootinfo)
{
    status_t status;

    /* Simpan boot info */
    g_boot_info = bootinfo;

    /* Verifikasi multiboot magic */
    if (magic != MULTIBOOT_MAGIC) {
        /* Tidak bisa print error karena console belum diinisialisasi */
        kernel_halt();
    }

    /* Inisialisasi HAL (termasuk console) */
    status = hal_init();
    if (status != STATUS_BERHASIL) {
        kernel_halt();
    }

    /* Tampilkan banner */
    kernel_banner();

    /* Inisialisasi informasi sistem */
    kernel_info_init(bootinfo);

    /* Jalankan inisialisasi kernel lengkap */
    status = kernel_init(bootinfo);
    if (status != STATUS_BERHASIL) {
        kernel_printf("[KERNEL] KRITIKAL: Inisialisasi gagal: %d\n", status);
        kernel_halt();
    }

    /* Jalankan self-test */
    status = kernel_selftest();
    if (status != STATUS_BERHASIL) {
        kernel_printf("[KERNEL] PERINGATAN: Self-test gagal\n");
    }

    /* Tandai kernel sudah siap */
    g_kernel_diinisialisasi = 1;

    kernel_printf("[KERNEL] Inisialisasi selesai\n");
    kernel_printf("[KERNEL] Memulai operasi normal\n\n");

    /* Mulai operasi normal */
    kernel_start();

    /* Tidak akan sampai sini */
    kernel_idle();
}

/*
 * kernel_init
 * -----------
 * Inisialisasi kernel lengkap.
 */
status_t kernel_init(multiboot_info_t *bootinfo)
{
    status_t status;

    kernel_printf("[KERNEL] Memulai inisialisasi kernel...\n");

    /* Inisialisasi subsistem memori */
    kernel_printf("[KERNEL] Inisialisasi memori...\n");
    status = hal_memory_init(bootinfo);
    if (status != STATUS_BERHASIL) {
        kernel_printf("[KERNEL] Gagal inisialisasi memori: %d\n", status);
        return status;
    }

    /* Update informasi memori dari HAL */
    {
        const hal_state_t *hal = hal_get_state();
        g_info_sistem.memori.total = hal->memory.total_bytes;
        g_info_sistem.memori.tersedia = hal->memory.available_bytes;
    }

    /* Enable interrupt */
    kernel_printf("[KERNEL] Mengaktifkan interupsi...\n");
    hal_cpu_enable_interrupts();

    kernel_printf("[KERNEL] Inisialisasi kernel selesai\n");

    return STATUS_BERHASIL;
}

/*
 * kernel_start
 * ------------
 * Mulai operasi normal kernel.
 */
void kernel_start(void)
{
    kernel_printf("[KERNEL] Sistem siap\n");

    /* Untuk sekarang, masuk ke idle loop */
    /* Nanti akan memulai proses init */
    kernel_idle();
}

/*
 * kernel_shutdown
 * ---------------
 * Shutdown kernel.
 */
void kernel_shutdown(int reboot)
{
    kernel_printf("[KERNEL] Shutdown dimulai...\n");

    /* Disable interrupt */
    hal_cpu_disable_interrupts();

    /* Shutdown HAL */
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

/*
 * kernel_delay
 * ------------
 * Delay sederhana.
 */
void kernel_delay(tak_bertanda32_t ms)
{
    hal_timer_sleep(ms);
}

/*
 * kernel_sleep
 * ------------
 * Sleep kernel.
 */
void kernel_sleep(tak_bertanda32_t ms)
{
    hal_timer_sleep(ms);
}

/*
 * kernel_get_uptime
 * -----------------
 * Dapatkan uptime sistem.
 */
tak_bertanda64_t kernel_get_uptime(void)
{
    return hal_timer_get_uptime();
}

/*
 * kernel_get_ticks
 * ----------------
 * Dapatkan jumlah timer ticks.
 */
tak_bertanda64_t kernel_get_ticks(void)
{
    return hal_timer_get_ticks();
}

/*
 * kernel_get_info
 * ---------------
 * Dapatkan informasi sistem.
 */
const info_sistem_t *kernel_get_info(void)
{
    return &g_info_sistem;
}

/*
 * kernel_get_arch
 * ---------------
 * Dapatkan nama arsitektur.
 */
const char *kernel_get_arch(void)
{
    return NAMA_ARSITEKTUR;
}

/*
 * kernel_get_version
 * ------------------
 * Dapatkan versi kernel.
 */
const char *kernel_get_version(void)
{
    return PIGURA_VERSI_STRING;
}
