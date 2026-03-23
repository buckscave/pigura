/*
 * PIGURA OS - HAL.C
 * -----------------
 * Implementasi utama Hardware Abstraction Layer (HAL).
 *
 * Berkas ini berisi implementasi fungsi-fungsi utama HAL yang
 * mengkoordinasikan inisialisasi dan operasi subsistem hardware.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 *
 * Versi: 2.0
 * Tanggal: 2025
 */

#include "hal.h"
#include "../kernel.h"

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

/* State HAL global */
hal_state_t g_hal_state;

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
    g_hal_state.magic = HAL_MAGIC;
    g_hal_state.status = HAL_STATUS_UNINITIALIZED;
    g_hal_state.version = (HAL_VERSI_MAJOR << 16) |
                          (HAL_VERSI_MINOR << 8) |
                          HAL_VERSI_PATCH;
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

    /* Set status initializing */
    g_hal_state.status = HAL_STATUS_INITIALIZING;

    /* Deteksi hardware */
    status = hal_detect_hardware();
    if (status != STATUS_BERHASIL) {
        kernel_printf("[HAL] Gagal mendeteksi hardware: %d\n", status);
        g_hal_state.status = HAL_STATUS_ERROR;
        return status;
    }

    /* Inisialisasi subsistem console terlebih dahulu untuk output */
    status = hal_console_init();
    if (status != STATUS_BERHASIL) {
        /* Tidak bisa print error jika console gagal */
        g_hal_state.status = HAL_STATUS_ERROR;
        return status;
    }

    kernel_printf("[HAL] Menginisialisasi Hardware Abstraction Layer...\n");

    /* Inisialisasi CPU */
    status = hal_cpu_init();
    if (status != STATUS_BERHASIL) {
        kernel_printf("[HAL] Gagal inisialisasi CPU: %d\n", status);
        g_hal_state.status = HAL_STATUS_ERROR;
        return status;
    }
    kernel_printf("[HAL] CPU: %s %s\n",
                  g_hal_state.cpu.vendor,
                  g_hal_state.cpu.brand);

    /* Inisialisasi interupsi */
    status = hal_interrupt_init();
    if (status != STATUS_BERHASIL) {
        kernel_printf("[HAL] Gagal inisialisasi interupsi: %d\n", status);
        g_hal_state.status = HAL_STATUS_ERROR;
        return status;
    }

    /* Inisialisasi timer */
    status = hal_timer_init();
    if (status != STATUS_BERHASIL) {
        kernel_printf("[HAL] Gagal inisialisasi timer: %d\n", status);
        g_hal_state.status = HAL_STATUS_ERROR;
        return status;
    }
    kernel_printf("[HAL] Timer: %u Hz, resolusi %u ns\n",
                  g_hal_state.timer.frequency,
                  g_hal_state.timer.resolution_ns);

    /* Tandai HAL sudah diinisialisasi */
    g_hal_state.status = HAL_STATUS_READY;

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
    if (g_hal_state.status != HAL_STATUS_READY) {
        return STATUS_GAGAL;
    }

    kernel_printf("[HAL] Mematikan HAL...\n");

    /* Disable semua interupsi */
    hal_cpu_disable_interrupts();

    /* Tandai tidak diinisialisasi */
    g_hal_state.status = HAL_STATUS_UNINITIALIZED;

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
    return (g_hal_state.status == HAL_STATUS_READY) ? BENAR : SALAH;
}

/*
 * hal_get_version
 * ---------------
 * Dapatkan versi HAL.
 *
 * Return: Versi dalam format 0xMMmmpp (Major, minor, patch)
 */
tak_bertanda32_t hal_get_version(void)
{
    return g_hal_state.version;
}

/*
 * hal_get_arch_name
 * -----------------
 * Dapatkan nama arsitektur.
 *
 * Return: String nama arsitektur
 */
const char *hal_get_arch_name(void)
{
    return NAMA_ARSITEKTUR;
}

/*
 * ============================================================================
 * FUNGSI DETECT CPU (CPU DETECTION FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_cpu_detect
 * --------------
 * Deteksi informasi CPU.
 *
 * Parameter:
 *   info - Pointer ke buffer untuk menyimpan informasi
 *
 * Return: Status operasi
 */
status_t hal_cpu_detect(hal_cpu_info_t *info)
{
    if (info == NULL) {
        return STATUS_PARAM_NULL;
    }

    return hal_cpu_get_info(info);
}

/*
 * hal_cpu_get_count
 * -----------------
 * Dapatkan jumlah CPU.
 *
 * Return: Jumlah CPU
 */
tak_bertanda32_t hal_cpu_get_count(void)
{
    return 1;  /* SMP belum didukung */
}

/*
 * hal_cpu_has_feature
 * -------------------
 * Cek apakah CPU mendukung fitur tertentu.
 *
 * Parameter:
 *   feature - Flag fitur (HAL_CPU_FEATURE_*)
 *
 * Return: BENAR jika didukung
 */
bool_t hal_cpu_has_feature(tak_bertanda32_t feature)
{
    if (g_hal_state.cpu.features & feature) {
        return BENAR;
    }
    return SALAH;
}

/*
 * hal_cpu_get_freq
 * ----------------
 * Dapatkan frekuensi CPU dalam kHz.
 *
 * Return: Frekuensi dalam kHz
 */
tak_bertanda32_t hal_cpu_get_freq(void)
{
    return g_hal_state.cpu.freq_khz;
}

/*
 * hal_cpu_delay_us
 * ----------------
 * Delay dalam microsecond (busy wait).
 *
 * Parameter:
 *   us - Jumlah microsecond
 */
void hal_cpu_delay_us(tak_bertanda32_t us)
{
    tak_bertanda32_t i;
    tak_bertanda32_t iterations;

    /* Estimasi: ~1 I/O delay = ~1 microsecond */
    iterations = us;

    /* Skala berdasarkan frekuensi CPU jika tersedia */
    if (g_hal_state.cpu.freq_khz > 1000000) {
        iterations *= (g_hal_state.cpu.freq_khz / 1000000);
    }

    for (i = 0; i < iterations; i++) {
        io_delay();
    }
}

/*
 * hal_cpu_delay_ms
 * ----------------
 * Delay dalam millisecond (busy wait).
 *
 * Parameter:
 *   ms - Jumlah millisecond
 */
void hal_cpu_delay_ms(tak_bertanda32_t ms)
{
    hal_cpu_delay_us(ms * 1000);
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
 * hal_io_read_string_8
 * --------------------
 * Baca string byte dari port I/O.
 *
 * Parameter:
 *   port - Nomor port
 *   buf  - Buffer tujuan
 *   count - Jumlah byte
 */
void hal_io_read_string_8(tak_bertanda16_t port, void *buf, ukuran_t count)
{
    tak_bertanda8_t *buffer = (tak_bertanda8_t *)buf;
    ukuran_t i;

    for (i = 0; i < count; i++) {
        buffer[i] = inb(port);
    }
}

/*
 * hal_io_write_string_8
 * ---------------------
 * Tulis string byte ke port I/O.
 *
 * Parameter:
 *   port - Nomor port
 *   buf  - Buffer sumber
 *   count - Jumlah byte
 */
void hal_io_write_string_8(tak_bertanda16_t port, const void *buf,
                            ukuran_t count)
{
    const tak_bertanda8_t *buffer = (const tak_bertanda8_t *)buf;
    ukuran_t i;

    for (i = 0; i < count; i++) {
        outb(port, buffer[i]);
    }
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
tak_bertanda8_t hal_mmio_read_8(const void *addr)
{
    return *(const volatile tak_bertanda8_t *)addr;
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
tak_bertanda16_t hal_mmio_read_16(const void *addr)
{
    return *(const volatile tak_bertanda16_t *)addr;
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
tak_bertanda32_t hal_mmio_read_32(const void *addr)
{
    return *(const volatile tak_bertanda32_t *)addr;
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
tak_bertanda64_t hal_mmio_read_64(const void *addr)
{
    return *(const volatile tak_bertanda64_t *)addr;
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

/*
 * ============================================================================
 * FUNGSI MSR (MODEL SPECIFIC REGISTERS)
 * ============================================================================
 */

#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)

/*
 * hal_msr_read
 * ------------
 * Baca MSR (Model Specific Register).
 *
 * Parameter:
 *   msr - Nomor MSR
 *
 * Return: Nilai MSR (64-bit)
 */
tak_bertanda64_t hal_msr_read(tak_bertanda32_t msr)
{
    tak_bertanda32_t low;
    tak_bertanda32_t high;

    __asm__ __volatile__(
        "rdmsr"
        : "=a"(low), "=d"(high)
        : "c"(msr)
    );

    return ((tak_bertanda64_t)high << 32) | low;
}

/*
 * hal_msr_write
 * -------------
 * Tulis MSR (Model Specific Register).
 *
 * Parameter:
 *   msr   - Nomor MSR
 *   value - Nilai yang ditulis
 */
void hal_msr_write(tak_bertanda32_t msr, tak_bertanda64_t value)
{
    tak_bertanda32_t low = (tak_bertanda32_t)(value & 0xFFFFFFFF);
    tak_bertanda32_t high = (tak_bertanda32_t)(value >> 32);

    __asm__ __volatile__(
        "wrmsr"
        :
        : "a"(low), "d"(high), "c"(msr)
    );
}

/*
 * hal_msr_exists
 * --------------
 * Cek apakah MSR ada.
 *
 * Parameter:
 *   msr - Nomor MSR
 *
 * Return: BENAR jika ada
 */
bool_t hal_msr_exists(tak_bertanda32_t msr)
{
    tak_bertanda32_t edx;

    /* Cek apakah CPU mendukung MSR */
    if (!(g_hal_state.cpu.features & HAL_CPU_FEATURE_MSR)) {
        return SALAH;
    }

    /* Coba baca MSR dan cek apakah tidak exception */
    __asm__ __volatile__(
        "rdmsr"
        : "=d"(edx)
        : "c"(msr)
        : "eax"
    );

    /* Jika sampai sini, MSR ada */
    (void)edx;
    return BENAR;
}

#endif /* ARSITEKTUR_X86 || ARSITEKTUR_X86_64 */
