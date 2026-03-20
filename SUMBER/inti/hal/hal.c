/*
 * PIGURA OS - HAL.C
 * -----------------
 * Implementasi utama Hardware Abstraction Layer (HAL).
 *
 * Berkas ini berisi implementasi fungsi-fungsi utama HAL yang
 * mengkoordinasikan inisialisasi dan operasi subsistem hardware.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "hal.h"
#include "../kernel.h"

/*
 * ============================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ============================================================================
 */

/* State HAL global */
hal_state_t g_hal_state = {0};

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_init_state
 * --------------
 * Inisialisasi struktur state HAL dengan nilai default.
 */
static void hal_init_state(void)
{
    kernel_memset(&g_hal_state, 0, sizeof(hal_state_t));
    g_hal_state.initialized = SALAH;
}

/*
 * hal_detect_hardware
 * -------------------
 * Deteksi hardware yang tersedia.
 *
 * Return: Status operasi
 */
static status_t hal_detect_hardware(void)
{
    /* Deteksi akan dilakukan oleh masing-masing subsistem */
    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI UTAMA HAL (MAIN HAL FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_init
 * --------
 * Inisialisasi HAL.
 * Fungsi ini menginisialisasi semua subsistem hardware.
 */
status_t hal_init(void)
{
    status_t status;

    /* Inisialisasi state */
    hal_init_state();

    /* Deteksi hardware */
    status = hal_detect_hardware();
    if (status != STATUS_BERHASIL) {
        kernel_printf("[HAL] Gagal mendeteksi hardware: %d\n", status);
        return status;
    }

    /* Inisialisasi subsistem console terlebih dahulu untuk output */
    status = hal_console_init();
    if (status != STATUS_BERHASIL) {
        /* Tidak bisa print error jika console gagal */
        return status;
    }

    kernel_printf("[HAL] Menginisialisasi Hardware Abstraction Layer...\n");

    /* Inisialisasi CPU */
    status = hal_cpu_init();
    if (status != STATUS_BERHASIL) {
        kernel_printf("[HAL] Gagal inisialisasi CPU: %d\n", status);
        return status;
    }
    kernel_printf("[HAL] CPU: %s %s\n",
                  g_hal_state.cpu.vendor,
                  g_hal_state.cpu.brand);

    /* Inisialisasi interupsi */
    status = hal_interrupt_init();
    if (status != STATUS_BERHASIL) {
        kernel_printf("[HAL] Gagal inisialisasi interupsi: %d\n", status);
        return status;
    }

    /* Inisialisasi timer */
    status = hal_timer_init();
    if (status != STATUS_BERHASIL) {
        kernel_printf("[HAL] Gagal inisialisasi timer: %d\n", status);
        return status;
    }
    kernel_printf("[HAL] Timer: %u Hz, resolusi %u ns\n",
                  g_hal_state.timer.frequency,
                  g_hal_state.timer.resolution_ns);

    /* Tandai HAL sudah diinisialisasi */
    g_hal_state.initialized = BENAR;

    kernel_printf("[HAL] Inisialisasi berhasil\n");

    return STATUS_BERHASIL;
}

/*
 * hal_shutdown
 * ------------
 * Shutdown HAL dan semua subsistem.
 */
status_t hal_shutdown(void)
{
    if (!g_hal_state.initialized) {
        return STATUS_GAGAL;
    }

    kernel_printf("[HAL] Mematikan HAL...\n");

    /* Disable semua interupsi */
    hal_cpu_disable_interrupts();

    /* Tandai tidak diinisialisasi */
    g_hal_state.initialized = SALAH;

    return STATUS_BERHASIL;
}

/*
 * hal_get_state
 * -------------
 * Dapatkan pointer ke state HAL.
 */
const hal_state_t *hal_get_state(void)
{
    return &g_hal_state;
}

/*
 * hal_is_initialized
 * ------------------
 * Cek apakah HAL sudah diinisialisasi.
 */
bool_t hal_is_initialized(void)
{
    return g_hal_state.initialized;
}

/*
 * ============================================================================
 * FUNGSI I/O PORT HAL (HAL I/O PORT FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_io_read_8
 * -------------
 * Baca byte dari port I/O.
 */
tak_bertanda8_t hal_io_read_8(tak_bertanda16_t port)
{
    return inb(port);
}

/*
 * hal_io_write_8
 * --------------
 * Tulis byte ke port I/O.
 */
void hal_io_write_8(tak_bertanda16_t port, tak_bertanda8_t value)
{
    outb(port, value);
}

/*
 * hal_io_read_16
 * --------------
 * Baca word dari port I/O.
 */
tak_bertanda16_t hal_io_read_16(tak_bertanda16_t port)
{
    return inw(port);
}

/*
 * hal_io_write_16
 * ---------------
 * Tulis word ke port I/O.
 */
void hal_io_write_16(tak_bertanda16_t port, tak_bertanda16_t value)
{
    outw(port, value);
}

/*
 * hal_io_read_32
 * --------------
 * Baca dword dari port I/O.
 */
tak_bertanda32_t hal_io_read_32(tak_bertanda16_t port)
{
    return inl(port);
}

/*
 * hal_io_write_32
 * ---------------
 * Tulis dword ke port I/O.
 */
void hal_io_write_32(tak_bertanda16_t port, tak_bertanda32_t value)
{
    outl(port, value);
}

/*
 * hal_io_delay
 * ------------
 * Delay I/O kira-kira 1 mikrodetik.
 */
void hal_io_delay(void)
{
    io_delay();
}

/*
 * ============================================================================
 * FUNGSI MMIO HAL (HAL MMIO FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_mmio_read_8
 * ---------------
 * Baca byte dari memory-mapped I/O.
 */
tak_bertanda8_t hal_mmio_read_8(void *addr)
{
    return *(volatile tak_bertanda8_t *)addr;
}

/*
 * hal_mmio_write_8
 * ----------------
 * Tulis byte ke memory-mapped I/O.
 */
void hal_mmio_write_8(void *addr, tak_bertanda8_t value)
{
    *(volatile tak_bertanda8_t *)addr = value;
}

/*
 * hal_mmio_read_16
 * ----------------
 * Baca word dari memory-mapped I/O.
 */
tak_bertanda16_t hal_mmio_read_16(void *addr)
{
    return *(volatile tak_bertanda16_t *)addr;
}

/*
 * hal_mmio_write_16
 * -----------------
 * Tulis word ke memory-mapped I/O.
 */
void hal_mmio_write_16(void *addr, tak_bertanda16_t value)
{
    *(volatile tak_bertanda16_t *)addr = value;
}

/*
 * hal_mmio_read_32
 * ----------------
 * Baca dword dari memory-mapped I/O.
 */
tak_bertanda32_t hal_mmio_read_32(void *addr)
{
    return *(volatile tak_bertanda32_t *)addr;
}

/*
 * hal_mmio_write_32
 * -----------------
 * Tulis dword ke memory-mapped I/O.
 */
void hal_mmio_write_32(void *addr, tak_bertanda32_t value)
{
    *(volatile tak_bertanda32_t *)addr = value;
}

/*
 * hal_mmio_read_64
 * ----------------
 * Baca qword dari memory-mapped I/O.
 */
tak_bertanda64_t hal_mmio_read_64(void *addr)
{
    return *(volatile tak_bertanda64_t *)addr;
}

/*
 * hal_mmio_write_64
 * -----------------
 * Tulis qword ke memory-mapped I/O.
 */
void hal_mmio_write_64(void *addr, tak_bertanda64_t value)
{
    *(volatile tak_bertanda64_t *)addr = value;
}
