/*
 * PIGURA OS - HAL_TIMER.C
 * -----------------------
 * Implementasi fungsi timer untuk Hardware Abstraction Layer.
 *
 * Berkas ini berisi implementasi fungsi-fungsi yang berkaitan dengan
 * timer hardware, termasuk PIT (Programmable Interval Timer) dan
 * sistem tick counter.
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
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* PIT frequency */
#define PIT_FREQUENCY           1193182UL

/* PIT ports */
#define PIT_CHANNEL_0           0x40
#define PIT_CHANNEL_1           0x41
#define PIT_CHANNEL_2           0x42
#define PIT_COMMAND             0x43

/* PIT command bits */
#define PIT_CMD_BINARY          0x00
#define PIT_CMD_BCD             0x01
#define PIT_CMD_MODE_0          0x00  /* Interrupt on terminal count */
#define PIT_CMD_MODE_1          0x02  /* Hardware retriggerable one-shot */
#define PIT_CMD_MODE_2          0x04  /* Rate generator */
#define PIT_CMD_MODE_3          0x06  /* Square wave generator */
#define PIT_CMD_MODE_4          0x08  /* Software triggered strobe */
#define PIT_CMD_MODE_5          0x0A  /* Hardware triggered strobe */
#define PIT_CMD_LATCH           0x00
#define PIT_CMD_LOW_BYTE        0x10
#define PIT_CMD_HIGH_BYTE       0x20
#define PIT_CMD_BOTH_BYTES      0x30
#define PIT_CMD_CHANNEL_0       0x00
#define PIT_CMD_CHANNEL_1       0x40
#define PIT_CMD_CHANNEL_2       0x80

/* Timer frequency yang digunakan */
#define TIMER_FREQUENCY         FREKUENSI_TIMER

/* Calculated divisor */
#define TIMER_DIVISOR           (PIT_FREQUENCY / TIMER_FREQUENCY)

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Tick counter global */
static volatile tak_bertanda64_t timer_ticks = 0;

/* Timer handler */
static void (*timer_handler)(void) = NULL;

/* Timer state */
static bool_t timer_initialized = SALAH;

/*
 * ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================
 */

/* Forward declaration untuk hal_timer_delay */
static void hal_timer_delay_internal(tak_bertanda32_t microseconds);

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * pit_set_frequency
 * -----------------
 * Set frekuensi PIT channel 0.
 *
 * Parameter:
 *   frequency - Frekuensi yang diinginkan dalam Hz
 */
static void pit_set_frequency(tak_bertanda32_t frequency)
{
    tak_bertanda16_t divisor;
    tak_bertanda8_t low_byte;
    tak_bertanda8_t high_byte;

    /* Hitung divisor */
    divisor = (tak_bertanda16_t)(PIT_FREQUENCY / frequency);

    /* Pastikan divisor tidak nol */
    if (divisor == 0) {
        divisor = 1;
    }

    /* Pisahkan byte */
    low_byte = divisor & 0xFF;
    high_byte = (divisor >> 8) & 0xFF;

    /* Kirim command: channel 0, mode 3 (square wave), binary, both bytes */
    outb(PIT_COMMAND, PIT_CMD_CHANNEL_0 | PIT_CMD_MODE_3 |
                       PIT_CMD_BOTH_BYTES | PIT_CMD_BINARY);

    /* Kirim divisor */
    outb(PIT_CHANNEL_0, low_byte);
    outb(PIT_CHANNEL_0, high_byte);
}

/*
 * pit_read_count
 * --------------
 * Baca nilai counter PIT channel 0.
 *
 * Return: Nilai counter saat ini
 */
static tak_bertanda16_t pit_read_count(void)
{
    tak_bertanda8_t low_byte;
    tak_bertanda8_t high_byte;

    /* Latch count */
    outb(PIT_COMMAND, PIT_CMD_CHANNEL_0 | PIT_CMD_LATCH);

    /* Baca low byte lalu high byte */
    low_byte = inb(PIT_CHANNEL_0);
    high_byte = inb(PIT_CHANNEL_0);

    return ((tak_bertanda16_t)high_byte << 8) | low_byte;
}

/*
 * pit_delay_ms
 * ------------
 * Delay menggunakan PIT (busy wait).
 *
 * Parameter:
 *   milliseconds - Jumlah milidetik delay
 */
static void pit_delay_ms(tak_bertanda32_t milliseconds)
{
    tak_bertanda32_t target_ticks;
    tak_bertanda64_t start_ticks;

    start_ticks = timer_ticks;
    target_ticks = (milliseconds * TIMER_FREQUENCY) / 1000;

    /* Wait until target reached */
    while ((timer_ticks - start_ticks) < target_ticks) {
        cpu_halt();
    }
}

/*
 * hal_timer_delay_internal
 * ------------------------
 * Busy-wait delay dalam mikrodetik.
 *
 * Parameter:
 *   microseconds - Jumlah mikrodetik
 */
static void hal_timer_delay_internal(tak_bertanda32_t microseconds)
{
    tak_bertanda32_t i;
    tak_bertanda32_t iterations;
    tak_bertanda32_t freq_khz;

    /* Estimasi: kira-kira 1 I/O delay = ~1 microsecond pada CPU modern */
    /* Skala dengan frekuensi CPU */
    iterations = microseconds;

    /* Faktor penyesuaian berdasarkan frekuensi CPU */
    freq_khz = g_hal_state.cpu.freq_khz;
    if (freq_khz > 1000000) {
        iterations *= (freq_khz / 1000000);
    }

    for (i = 0; i < iterations; i++) {
        io_delay();
    }
}

/*
 * timer_isr
 * ---------
 * Interrupt Service Routine untuk timer.
 * Dipanggil setiap timer tick.
 */
static void timer_isr(void)
{
    /* Increment tick counter */
    timer_ticks++;

    /* Update state */
    g_hal_state.timer.ticks = timer_ticks;
    g_hal_state.uptime_ticks = timer_ticks;
    g_hal_state.uptime_sec = timer_ticks / TIMER_FREQUENCY;

    /* Panggil handler jika ada */
    if (timer_handler != NULL) {
        timer_handler();
    }
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_timer_init
 * --------------
 * Inisialisasi subsistem timer.
 */
status_t hal_timer_init(void)
{
    hal_timer_info_t *info = &g_hal_state.timer;

    if (timer_initialized) {
        return STATUS_SUDAH_ADA;
    }

    /* Reset tick counter */
    timer_ticks = 0;
    timer_handler = NULL;

    /* Konfigurasi PIT */
    pit_set_frequency(TIMER_FREQUENCY);

    /* Set informasi timer */
    info->frequency = TIMER_FREQUENCY;
    info->resolution_ns = 1000000000UL / TIMER_FREQUENCY;
    info->ticks = 0;
    info->supports_one_shot = BENAR;
    info->supports_periodic = BENAR;
    info->supports_64bit = BENAR;

    /* Set handler untuk timer interrupt (IRQ 0) */
    hal_interrupt_set_handler(VEKTOR_TIMER - VEKTOR_IRQ_MULAI,
                              timer_isr);

    /* Enable timer interrupt */
    hal_interrupt_unmask(0);

    timer_initialized = BENAR;

    return STATUS_BERHASIL;
}

/*
 * hal_timer_get_info
 * ------------------
 * Dapatkan informasi timer.
 *
 * Parameter:
 *   info - Pointer ke buffer
 *
 * Return: Status operasi
 */
status_t hal_timer_get_info(hal_timer_info_t *info)
{
    if (info == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memcpy(info, &g_hal_state.timer, sizeof(hal_timer_info_t));

    return STATUS_BERHASIL;
}

/*
 * hal_timer_get_ticks
 * -------------------
 * Dapatkan jumlah tick timer.
 */
tak_bertanda64_t hal_timer_get_ticks(void)
{
    return timer_ticks;
}

/*
 * hal_timer_get_frequency
 * -----------------------
 * Dapatkan frekuensi timer.
 */
tak_bertanda32_t hal_timer_get_frequency(void)
{
    return TIMER_FREQUENCY;
}

/*
 * hal_timer_get_uptime
 * --------------------
 * Dapatkan uptime sistem dalam detik.
 */
tak_bertanda64_t hal_timer_get_uptime(void)
{
    return timer_ticks / TIMER_FREQUENCY;
}

/*
 * hal_timer_get_uptime_ms
 * -----------------------
 * Dapatkan uptime sistem dalam milidetik.
 */
tak_bertanda64_t hal_timer_get_uptime_ms(void)
{
    return (timer_ticks * 1000UL) / TIMER_FREQUENCY;
}

/*
 * hal_timer_sleep
 * ---------------
 * Sleep untuk durasi tertentu.
 */
void hal_timer_sleep(tak_bertanda32_t milliseconds)
{
    if (!timer_initialized) {
        /* Fallback ke busy delay jika timer belum siap */
        hal_timer_delay_internal(milliseconds * 1000);
        return;
    }

    pit_delay_ms(milliseconds);
}

/*
 * hal_timer_sleep_us
 * ------------------
 * Sleep dalam mikrodetik.
 */
void hal_timer_sleep_us(tak_bertanda32_t microseconds)
{
    hal_timer_delay_internal(microseconds);
}

/*
 * hal_timer_delay
 * ---------------
 * Busy-wait delay dalam mikrodetik.
 */
void hal_timer_delay(tak_bertanda32_t microseconds)
{
    hal_timer_delay_internal(microseconds);
}

/*
 * hal_timer_set_handler
 * ---------------------
 * Set handler untuk timer interrupt.
 */
status_t hal_timer_set_handler(void (*handler)(void))
{
    tak_bertanda32_t flags;

    /* Disable interrupt saat mengubah handler */
    flags = hal_cpu_save_interrupts();

    timer_handler = handler;

    hal_cpu_restore_interrupts(flags);

    return STATUS_BERHASIL;
}

/*
 * hal_timer_calibrate
 * -------------------
 * Kalibrasi timer.
 *
 * Return: Status operasi
 */
status_t hal_timer_calibrate(void)
{
    /* Timer sudah dikalibrasi saat init */
    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI UTILITAS TIMER (TIMER UTILITY FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_timer_ticks_to_ms
 * ---------------------
 * Konversi tick ke milidetik.
 *
 * Parameter:
 *   ticks - Jumlah tick
 *
 * Return: Waktu dalam milidetik
 */
tak_bertanda64_t hal_timer_ticks_to_ms(tak_bertanda64_t ticks)
{
    return (ticks * 1000UL) / TIMER_FREQUENCY;
}

/*
 * hal_timer_ticks_to_us
 * ---------------------
 * Konversi tick ke mikrodetik.
 *
 * Parameter:
 *   ticks - Jumlah tick
 *
 * Return: Waktu dalam mikrodetik
 */
tak_bertanda64_t hal_timer_ticks_to_us(tak_bertanda64_t ticks)
{
    return (ticks * 1000000UL) / TIMER_FREQUENCY;
}

/*
 * hal_timer_ms_to_ticks
 * ---------------------
 * Konversi milidetik ke tick.
 *
 * Parameter:
 *   milliseconds - Waktu dalam milidetik
 *
 * Return: Jumlah tick
 */
tak_bertanda64_t hal_timer_ms_to_ticks(tak_bertanda64_t milliseconds)
{
    return (milliseconds * TIMER_FREQUENCY) / 1000UL;
}

/*
 * hal_timer_get_ns
 * ----------------
 * Dapatkan waktu dalam nanodetik sejak boot.
 *
 * Return: Waktu dalam nanodetik
 */
tak_bertanda64_t hal_timer_get_ns(void)
{
    tak_bertanda64_t ticks;
    tak_bertanda64_t ns;

    ticks = timer_ticks;

    /* Konversi tick ke nanodetik */
    ns = (ticks * 1000000000UL) / TIMER_FREQUENCY;

    /* Tambahkan offset dari counter PIT untuk presisi lebih */
    {
        tak_bertanda16_t pit_count = pit_read_count();
        tak_bertanda16_t pit_reload = (tak_bertanda16_t)TIMER_DIVISOR;
        tak_bertanda32_t ns_per_tick = 1000000000UL / TIMER_FREQUENCY;

        /* Counter menurun, jadi kurangi dari reload */
        ns += ((pit_reload - pit_count) * ns_per_tick) / pit_reload;
    }

    return ns;
}
