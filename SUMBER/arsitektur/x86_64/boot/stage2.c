/*
 * PIGURA OS - STAGE 2 BOOTLOADER x86_64
 * -------------------------------------
 * Implementasi stage 2 bootloader untuk arsitektur x86_64.
 *
 * Berkas ini berisi kode untuk mempersiapkan lingkungan
 * sebelum melompat ke kernel.
 *
 * Arsitektur: x86_64 (64-bit)
 * Versi: 1.0
 */

#include "../../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA
 * ============================================================================
 */

/* Alamat kernel */
#define KERNEL_LOAD_ADDR        0x100000

/* Magic multiboot */
#define MULTIBOOT_MAGIC         0x2BADB002
#define MULTIBOOT_MAGIC2        0x1BADB002

/*
 * ============================================================================
 * FUNGSI INTERNAL
 * ============================================================================
 */

/*
 * _cek_multiboot
 * --------------
 * Memvalidasi multiboot info.
 *
 * Parameter:
 *   magic    - Magic number dari bootloader
 *   bootinfo - Pointer ke struktur multiboot
 *
 * Return:
 *   STATUS_BERHASIL jika valid
 */
static status_t _cek_multiboot(tak_bertanda32_t magic,
                                multiboot_info_t *bootinfo)
{
    if (magic != MULTIBOOT_MAGIC) {
        return STATUS_GAGAL;
    }

    if (bootinfo == NULL) {
        return STATUS_GAGAL;
    }

    return STATUS_BERHASIL;
}

/*
 * _print_bootinfo
 * ---------------
 * Menampilkan informasi boot.
 *
 * Parameter:
 *   bootinfo - Pointer ke struktur multiboot
 */
static void _print_bootinfo(multiboot_info_t *bootinfo)
{
    kernel_printf("\n=== Boot Information ===\n");

    if (bootinfo->flags & MULTIBOOT_FLAG_MEM) {
        kernel_printf("Memory lower: %u KB\n", bootinfo->mem_lower);
        kernel_printf("Memory upper: %u KB\n", bootinfo->mem_upper);
        kernel_printf("Total memory: %u MB\n",
                      (bootinfo->mem_lower + bootinfo->mem_upper) / 1024);
    }

    if (bootinfo->flags & MULTIBOOT_FLAG_CMDLINE) {
        kernel_printf("Command line: %s\n",
                      (const char *)(uintptr_t)bootinfo->cmdline);
    }

    if (bootinfo->flags & MULTIBOOT_FLAG_LOADER) {
        kernel_printf("Bootloader: %s\n",
                      (const char *)(uintptr_t)bootinfo->boot_loader_name);
    }

    kernel_printf("========================\n\n");
}

/*
 * ============================================================================
 * FUNGSI PUBLIC
 * ============================================================================
 */

/*
 * stage2_main
 * -----------
 * Entry point untuk stage 2 bootloader.
 *
 * Parameter:
 *   magic    - Magic number dari bootloader
 *   bootinfo - Pointer ke struktur multiboot info
 */
void stage2_main(tak_bertanda32_t magic, multiboot_info_t *bootinfo)
{
    status_t status;

    /* Init console awal */
    hal_console_init();

    kernel_printf("\n");
    kernel_printf("=================================\n");
    kernel_printf("  Pigura OS Stage 2 Bootloader\n");
    kernel_printf("  Architecture: x86_64\n");
    kernel_printf("=================================\n\n");

    /* Validasi multiboot */
    status = _cek_multiboot(magic, bootinfo);
    if (status != STATUS_BERHASIL) {
        kernel_printf("[STAGE2] Error: Invalid multiboot!\n");
        kernel_printf("[STAGE2] Magic: 0x%08X (expected: 0x%08X)\n",
                      magic, MULTIBOOT_MAGIC);
        hal_cpu_halt();
    }

    /* Print boot info */
    _print_bootinfo(bootinfo);

    /* Init HAL */
    kernel_printf("[STAGE2] Initializing HAL...\n");
    status = hal_init();
    if (status != STATUS_BERHASIL) {
        kernel_printf("[STAGE2] HAL init failed!\n");
        hal_cpu_halt();
    }

    /* Init CPU */
    kernel_printf("[STAGE2] Initializing CPU...\n");
    status = hal_cpu_init();
    if (status != STATUS_BERHASIL) {
        kernel_printf("[STAGE2] CPU init failed!\n");
        hal_cpu_halt();
    }

    /* Init memory */
    kernel_printf("[STAGE2] Initializing memory...\n");
    status = hal_memory_init(bootinfo);
    if (status != STATUS_BERHASIL) {
        kernel_printf("[STAGE2] Memory init failed!\n");
        hal_cpu_halt();
    }

    /* Init interrupts */
    kernel_printf("[STAGE2] Initializing interrupts...\n");
    status = hal_interrupt_init();
    if (status != STATUS_BERHASIL) {
        kernel_printf("[STAGE2] Interrupt init failed!\n");
        hal_cpu_halt();
    }

    /* Enable interrupts */
    hal_cpu_enable_interrupts();

    kernel_printf("[STAGE2] Boot complete!\n");
    kernel_printf("[STAGE2] Jumping to kernel...\n\n");

    /* Call kernel_main */
    kernel_main(magic, bootinfo);

    /* Should never reach here */
    kernel_printf("[STAGE2] Error: kernel returned!\n");
    hal_cpu_halt();
}

/*
 * stage2_get_kernel_addr
 * ----------------------
 * Mendapatkan alamat kernel yang di-load.
 *
 * Return:
 *   Alamat kernel
 */
tak_bertanda64_t stage2_get_kernel_addr(void)
{
    return KERNEL_LOAD_ADDR;
}
