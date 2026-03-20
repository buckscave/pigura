/*
 * PIGURA OS - HAL_TIMER.C
 * -----------------------
 * Implementasi fungsi timer untuk Hardware Abstraction Layer.
 *
 * Berkas ini berisi implementasi fungsi-fungsi yang berkaitan dengan
 * timer hardware, termasuk PIT (Programmable Interval Timer) dan
 * sistem tick counter.
 *
 * Versi: 1.0
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
 * timer_isr
 * ---------
 * Interrupt Service Routine untuk timer.
 * Dipanggil setiap timer tick.
 */
static void timer_isr(void)
{
    /* Increment tick counter */
    timer_ticks++;

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
    info->supports_one_shot = BENAR;
    info->supports_periodic = BENAR;

    /* Set handler untuk timer interrupt (IRQ 0) */
    hal_interrupt_set_handler(VEKTOR_TIMER - VEKTOR_IRQ_MULAI,
                              timer_isr);

    /* Enable timer interrupt */
    hal_interrupt_unmask(0);

    timer_initialized = BENAR;

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
 * hal_timer_sleep
 * ---------------
 * Sleep untuk durasi tertentu.
 */
void hal_timer_sleep(tak_bertanda32_t milliseconds)
{
    if (!timer_initialized) {
        /* Fallback ke busy delay jika timer belum siap */
        hal_timer_delay(milliseconds * 1000);
        return;
    }

    pit_delay_ms(milliseconds);
}

/*
 * hal_timer_delay
 * ---------------
 * Busy-wait delay dalam mikrodetik.
 */
void hal_timer_delay(tak_bertanda32_t microseconds)
{
    tak_bertanda32_t i;
    tak_bertanda32_t iterations;

    /* Estimasi: kira-kira 1 I/O delay = ~1 microsecond pada CPU modern */
    /* Skala dengan frekuensi CPU */
    iterations = microseconds;

    /* Faktor penyesuaian berdasarkan frekuensi CPU */
    if (g_hal_state.cpu.freq_mhz > 1000) {
        iterations *= (g_hal_state.cpu.freq_mhz / 1000);
    }

    for (i = 0; i < iterations; i++) {
        io_delay();
    }
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
    IRQ_DISABLE_SAVE(flags);

    timer_handler = handler;

    IRQ_RESTORE(flags);

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
    tak_bertanda64_t ticks = timer_ticks;
    tak_bertanda64_t ns;

    /* Konversi tick ke nanodetik */
    ns = (ticks * 1000000000UL) / TIMER_FREQUENCY;

    /* Tambahkan offset dari counter PIT untuk presisi lebih */
    {
        tak_bertanda16_t pit_count = pit_read_count();
        tak_bertanda16_t pit_reload = TIMER_DIVISOR;
        tak_bertanda32_t ns_per_tick = 1000000000UL / TIMER_FREQUENCY;

        /* Counter menurun, jadi kurangi dari reload */
        ns += ((pit_reload - pit_count) * ns_per_tick) / pit_reload;
    }

    return ns;
}

/*
 * hal_timer_calibration
 * ---------------------
 * Kalibrasi timer untuk presisi lebih tinggi.
 * Menggunakan TSC untuk mendapatkan timing yang lebih akurat.
 *
 * Return: Status operasi
 */
status_t hal_timer_calibration(void)
{
    tak_bertanda64_t tsc_start;
    tak_bertanda64_t tsc_end;
    tak_bertanda64_t tsc_frequency;
    tak_bertanda32_t i;

    /* Baca TSC awal */
    __asm__ __volatile__("rdtsc" : "=A"(tsc_start));

    /* Tunggu beberapa tick */
    for (i = 0; i < TIMER_FREQUENCY / 10; i++) {
        /* Busy wait */
    }

    /* Baca TSC akhir */
    __asm__ __volatile__("rdtsc" : "=A"(tsc_end));

    /* Hitung frekuensi TSC */
    tsc_frequency = tsc_end - tsc_start;
    tsc_frequency *= TIMER_FREQUENCY;

    /* Gunakan frekuensi TSC untuk timing yang lebih akurat jika perlu */
    /* Untuk sekarang, kita gunakan PIT saja */

    return STATUS_BERHASIL;
}
