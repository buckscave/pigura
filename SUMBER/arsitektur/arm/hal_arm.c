/*
 * PIGURA OS - HAL_ARM.C
 * ---------------------
 * Implementasi Hardware Abstraction Layer untuk ARM (32-bit).
 *
 * Berkas ini mengimplementasikan interface HAL untuk arsitektur
 * ARM 32-bit, menyediakan abstraksi hardware yang portabel.
 *
 * Arsitektur: ARM (ARMv4/v5/v6 - 32-bit)
 * Versi: 1.0
 */

#include "../../inti/kernel.h"
#include "../../inti/types.h"
#include "../../inti/konstanta.h"
#include "../../inti/hal/hal.h"

/*
 * ============================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ============================================================================
 */

/* State HAL global */
hal_state_t g_hal_state;

/*
 * ============================================================================
 * FUNGSI HAL UTAMA (MAIN HAL FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_init
 * --------
 * Inisialisasi HAL untuk ARM.
 *
 * Return: Status operasi
 */
status_t hal_init(void)
{
    status_t hasil;

    /* Inisialisasi struktur state */
    kernel_memset(&g_hal_state, 0, sizeof(hal_state_t));

    /* Inisialisasi CPU */
    hasil = hal_cpu_init();
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    /* Copy info CPU ke state */
    kernel_memcpy(&g_hal_state.cpu, &g_cpu_info, sizeof(hal_cpu_info_t));

    /* Inisialisasi timer */
    hasil = hal_timer_init();
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    /* Inisialisasi interupsi */
    hasil = hal_interrupt_init();
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    /* Mark sebagai initialized */
    g_hal_state.status = HAL_STATUS_READY;

    return STATUS_BERHASIL;
}

/*
 * hal_shutdown
 * ------------
 * Shutdown HAL.
 *
 * Return: Status operasi
 */
status_t hal_shutdown(void)
{
    /* Disable interupsi */
    hal_cpu_disable_interrupts();

    /* Mark sebagai tidak initialized */
    g_hal_state.status = HAL_STATUS_UNINITIALIZED;

    return STATUS_BERHASIL;
}

/*
 * hal_get_state
 * -------------
 * Dapatkan state HAL.
 *
 * Return: Pointer ke struktur hal_state_t
 */
const hal_state_t *hal_get_state(void)
{
    return &g_hal_state;
}

/*
 * hal_is_initialized
 * ------------------
 * Cek apakah HAL sudah diinisialisasi.
 *
 * Return: BENAR jika sudah, SALAH jika belum
 */
bool_t hal_is_initialized(void)
{
    return (g_hal_state.status == HAL_STATUS_READY) ? BENAR : SALAH;
}

/*
 * ============================================================================
 * FUNGSI TIMER HAL (HAL TIMER FUNCTIONS)
 * ============================================================================
 */

/* Timer tick counter */
static volatile tak_bertanda64_t g_timer_ticks = 0;

/* Timer frequency */
#define TIMER_FREQUENCY     100   /* 100 Hz = 10ms per tick */

/* Timer handler */
static void (*g_timer_handler)(void) = NULL;

/*
 * hal_timer_init
 * --------------
 * Inisialisasi subsistem timer.
 *
 * Return: Status operasi
 */
status_t hal_timer_init(void)
{
    /* Setup info timer */
    g_hal_state.timer.frequency = TIMER_FREQUENCY;
    g_hal_state.timer.resolution_ns = 1000000000ULL / TIMER_FREQUENCY;
    g_hal_state.timer.supports_one_shot = BENAR;
    g_hal_state.timer.supports_periodic = BENAR;

    g_timer_ticks = 0;
    g_timer_handler = NULL;

    return STATUS_BERHASIL;
}

/*
 * hal_timer_get_ticks
 * -------------------
 * Dapatkan jumlah tick timer.
 *
 * Return: Jumlah tick sejak boot
 */
tak_bertanda64_t hal_timer_get_ticks(void)
{
    return g_timer_ticks;
}

/*
 * hal_timer_get_frequency
 * -----------------------
 * Dapatkan frekuensi timer.
 *
 * Return: Frekuensi dalam Hz
 */
tak_bertanda32_t hal_timer_get_frequency(void)
{
    return TIMER_FREQUENCY;
}

/*
 * hal_timer_get_uptime
 * --------------------
 * Dapatkan uptime sistem.
 *
 * Return: Uptime dalam detik
 */
tak_bertanda64_t hal_timer_get_uptime(void)
{
    return g_timer_ticks / TIMER_FREQUENCY;
}

/*
 * hal_timer_sleep
 * ---------------
 * Sleep untuk durasi tertentu.
 *
 * Parameter:
 *   milliseconds - Durasi sleep dalam milidetik
 */
void hal_timer_sleep(tak_bertanda32_t milliseconds)
{
    tak_bertanda64_t mulai;
    tak_bertanda64_t target;

    mulai = g_timer_ticks;
    target = mulai + ((tak_bertanda64_t)milliseconds * TIMER_FREQUENCY) / 1000;

    while (g_timer_ticks < target) {
        __asm__ __volatile__("mcr p15, 0, r0, c7, c0, 4");
    }
}

/*
 * hal_timer_delay
 * ---------------
 * Busy-wait delay.
 *
 * Parameter:
 *   microseconds - Durasi delay dalam mikrodetik
 */
void hal_timer_delay(tak_bertanda32_t microseconds)
{
    volatile tak_bertanda32_t count;
    tak_bertanda32_t iterations;

    /* Asumsi 1 iteration = ~10ns pada 100MHz */
    iterations = microseconds * 100;

    for (count = 0; count < iterations; count++) {
        __asm__ __volatile__("nop");
    }
}

/*
 * hal_timer_set_handler
 * ---------------------
 * Set handler untuk timer interrupt.
 *
 * Parameter:
 *   handler - Fungsi handler
 *
 * Return: Status operasi
 */
status_t hal_timer_set_handler(void (*handler)(void))
{
    g_timer_handler = handler;
    return STATUS_BERHASIL;
}

/*
 * hal_timer_tick
 * --------------
 * Handler tick timer (dipanggil dari ISR).
 */
void hal_timer_tick(void)
{
    g_timer_ticks++;

    /* Panggil handler jika ada */
    if (g_timer_handler != NULL) {
        g_timer_handler();
    }
}

/*
 * ============================================================================
 * FUNGSI INTERUPSI HAL (HAL INTERRUPT FUNCTIONS)
 * ============================================================================
 */

/* Interrupt handler table */
#define MAX_IRQ_HANDLERS    64
static void (*g_irq_handlers[MAX_IRQ_HANDLERS])(void);

/*
 * hal_interrupt_init
 * ------------------
 * Inisialisasi subsistem interupsi.
 *
 * Return: Status operasi
 */
status_t hal_interrupt_init(void)
{
    tak_bertanda32_t i;

    /* Clear handler table */
    for (i = 0; i < MAX_IRQ_HANDLERS; i++) {
        g_irq_handlers[i] = NULL;
    }

    /* Setup info interupsi */
    g_hal_state.interrupt.irq_count = MAX_IRQ_HANDLERS;
    g_hal_state.interrupt.vector_base = 0;
    g_hal_state.interrupt.has_apic = BENAR;
    g_hal_state.interrupt.has_ioapic = SALAH;  /* GIC instead */

    return STATUS_BERHASIL;
}

/*
 * hal_interrupt_enable
 * --------------------
 * Aktifkan IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: Status operasi
 */
status_t hal_interrupt_enable(tak_bertanda32_t irq)
{
    if (irq >= MAX_IRQ_HANDLERS) {
        return STATUS_PARAM_INVALID;
    }

    /* Enable IRQ di interrupt controller */
    /* TODO: Integrasikan dengan GIC */

    return STATUS_BERHASIL;
}

/*
 * hal_interrupt_disable
 * ---------------------
 * Nonaktifkan IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: Status operasi
 */
status_t hal_interrupt_disable(tak_bertanda32_t irq)
{
    if (irq >= MAX_IRQ_HANDLERS) {
        return STATUS_PARAM_INVALID;
    }

    /* Disable IRQ di interrupt controller */
    /* TODO: Integrasikan dengan GIC */

    return STATUS_BERHASIL;
}

/*
 * hal_interrupt_mask
 * ------------------
 * Mask IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: Status operasi
 */
status_t hal_interrupt_mask(tak_bertanda32_t irq)
{
    return hal_interrupt_disable(irq);
}

/*
 * hal_interrupt_unmask
 * --------------------
 * Unmask IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: Status operasi
 */
status_t hal_interrupt_unmask(tak_bertanda32_t irq)
{
    return hal_interrupt_enable(irq);
}

/*
 * hal_interrupt_acknowledge
 * -------------------------
 * Acknowledge IRQ (End of Interrupt).
 *
 * Parameter:
 *   irq - Nomor IRQ
 */
void hal_interrupt_acknowledge(tak_bertanda32_t irq)
{
    /* Acknowledge di interrupt controller */
    /* TODO: Integrasikan dengan GIC */
    (void)irq;
}

/*
 * hal_interrupt_set_handler
 * -------------------------
 * Set handler untuk IRQ tertentu.
 *
 * Parameter:
 *   irq     - Nomor IRQ
 *   handler - Fungsi handler
 *
 * Return: Status operasi
 */
status_t hal_interrupt_set_handler(tak_bertanda32_t irq, void (*handler)(void))
{
    if (irq >= MAX_IRQ_HANDLERS) {
        return STATUS_PARAM_INVALID;
    }

    g_irq_handlers[irq] = handler;
    return STATUS_BERHASIL;
}

/*
 * hal_interrupt_get_handler
 * -------------------------
 * Dapatkan handler untuk IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: Pointer ke handler, atau NULL jika tidak ada
 */
void (*hal_interrupt_get_handler(tak_bertanda32_t irq))(void)
{
    if (irq >= MAX_IRQ_HANDLERS) {
        return NULL;
    }

    return g_irq_handlers[irq];
}

/*
 * hal_interrupt_dispatch
 * ----------------------
 * Dispatch interrupt ke handler yang sesuai.
 *
 * Parameter:
 *   irq - Nomor IRQ
 */
void hal_interrupt_dispatch(tak_bertanda32_t irq)
{
    if (irq < MAX_IRQ_HANDLERS && g_irq_handlers[irq] != NULL) {
        g_irq_handlers[irq]();
    }

    hal_interrupt_acknowledge(irq);
}

/*
 * ============================================================================
 * FUNGSI MEMORI HAL (HAL MEMORY FUNCTIONS)
 * ============================================================================
 */

/* Bitmap untuk alokasi halaman fisik */
#define MAX_PAGES           32768    /* 128 MB dengan 4KB pages */
static tak_bertanda32_t g_page_bitmap[MAX_PAGES / 32];
static tak_bertanda64_t g_total_pages = 0;
static tak_bertanda64_t g_free_pages = 0;

/*
 * hal_memory_init
 * ---------------
 * Inisialisasi subsistem memori.
 *
 * Parameter:
 *   bootinfo - Pointer ke informasi boot
 *
 * Return: Status operasi
 */
status_t hal_memory_init(void *bootinfo)
{
    tak_bertanda32_t i;

    (void)bootinfo;  /* Tidak digunakan untuk sekarang */

    /* Clear bitmap */
    for (i = 0; i < (MAX_PAGES / 32); i++) {
        g_page_bitmap[i] = 0;
    }

    /* Setup default memory (64 MB untuk QEMU virt) */
    g_total_pages = 16384;  /* 64 MB */
    g_free_pages = g_total_pages;

    /* Setup info memori */
    g_hal_state.memory.total_bytes = g_total_pages * UKURAN_HALAMAN;
    g_hal_state.memory.available_bytes = g_free_pages * UKURAN_HALAMAN;
    g_hal_state.memory.page_size = UKURAN_HALAMAN;
    g_hal_state.memory.page_count = (tak_bertanda32_t)g_total_pages;
    g_hal_state.memory.has_pae = SALAH;
    g_hal_state.memory.has_nx = SALAH;

    return STATUS_BERHASIL;
}

/*
 * hal_memory_get_info
 * -------------------
 * Dapatkan informasi memori.
 *
 * Parameter:
 *   info - Pointer ke buffer untuk menyimpan informasi
 *
 * Return: Status operasi
 */
status_t hal_memory_get_info(hal_memory_info_t *info)
{
    if (info == NULL) {
        return STATUS_PARAM_INVALID;
    }

    kernel_memcpy(info, &g_hal_state.memory, sizeof(hal_memory_info_t));
    return STATUS_BERHASIL;
}

/*
 * hal_memory_get_free_pages
 * -------------------------
 * Dapatkan jumlah halaman bebas.
 *
 * Return: Jumlah halaman bebas
 */
tak_bertanda64_t hal_memory_get_free_pages(void)
{
    return g_free_pages;
}

/*
 * hal_memory_get_total_pages
 * --------------------------
 * Dapatkan jumlah total halaman.
 *
 * Return: Jumlah total halaman
 */
tak_bertanda64_t hal_memory_get_total_pages(void)
{
    return g_total_pages;
}

/*
 * hal_memory_find_free_page
 * -------------------------
 * Cari halaman bebas dalam bitmap.
 *
 * Return: Index halaman, atau -1 jika tidak ada
 */
static tanda64_t hal_memory_find_free_page(void)
{
    tak_bertanda32_t i;
    tak_bertanda32_t j;

    for (i = 0; i < (MAX_PAGES / 32); i++) {
        if (g_page_bitmap[i] != 0xFFFFFFFF) {
            for (j = 0; j < 32; j++) {
                if (!(g_page_bitmap[i] & (1U << j))) {
                    return (tanda64_t)(i * 32 + j);
                }
            }
        }
    }

    return -1;
}

/*
 * hal_memory_alloc_page
 * ---------------------
 * Alokasikan satu halaman fisik.
 *
 * Return: Alamat fisik halaman, atau 0 jika gagal
 */
alamat_fisik_t hal_memory_alloc_page(void)
{
    tanda64_t index;
    alamat_fisik_t addr;

    if (g_free_pages == 0) {
        return 0;
    }

    index = hal_memory_find_free_page();
    if (index < 0) {
        return 0;
    }

    /* Set bit dalam bitmap */
    g_page_bitmap[index / 32] |= (1U << (index % 32));
    g_free_pages--;

    /* Konversi ke alamat fisik */
    addr = (alamat_fisik_t)(index * UKURAN_HALAMAN);

    /* Update info */
    g_hal_state.memory.available_bytes = g_free_pages * UKURAN_HALAMAN;

    return addr;
}

/*
 * hal_memory_free_page
 * --------------------
 * Bebaskan satu halaman fisik.
 *
 * Parameter:
 *   addr - Alamat fisik halaman
 *
 * Return: Status operasi
 */
status_t hal_memory_free_page(alamat_fisik_t addr)
{
    tak_bertanda32_t index;

    /* Validasi alamat */
    if (addr % UKURAN_HALAMAN != 0) {
        return STATUS_PARAM_INVALID;
    }

    index = (tak_bertanda32_t)(addr / UKURAN_HALAMAN);
    if (index >= MAX_PAGES) {
        return STATUS_PARAM_INVALID;
    }

    /* Cek apakah sudah bebas */
    if (!(g_page_bitmap[index / 32] & (1U << (index % 32)))) {
        return STATUS_PARAM_INVALID;
    }

    /* Clear bit dalam bitmap */
    g_page_bitmap[index / 32] &= ~(1U << (index % 32));
    g_free_pages++;

    /* Update info */
    g_hal_state.memory.available_bytes = g_free_pages * UKURAN_HALAMAN;

    return STATUS_BERHASIL;
}

/*
 * hal_memory_alloc_pages
 * ----------------------
 * Alokasikan multiple halaman fisik berurutan.
 *
 * Parameter:
 *   count - Jumlah halaman
 *
 * Return: Alamat fisik halaman pertama, atau 0 jika gagal
 */
alamat_fisik_t hal_memory_alloc_pages(tak_bertanda32_t count)
{
    tak_bertanda32_t i;
    tak_bertanda32_t start_index;
    tak_bertanda32_t found;

    if (count == 0 || g_free_pages < count) {
        return 0;
    }

    /* Cari blok berurutan */
    found = 0;
    start_index = 0;

    for (i = 0; i < MAX_PAGES; i++) {
        if (!(g_page_bitmap[i / 32] & (1U << (i % 32)))) {
            if (found == 0) {
                start_index = i;
            }
            found++;
            if (found == count) {
                break;
            }
        } else {
            found = 0;
        }
    }

    if (found < count) {
        return 0;
    }

    /* Alokasikan halaman */
    for (i = 0; i < count; i++) {
        tak_bertanda32_t index = start_index + i;
        g_page_bitmap[index / 32] |= (1U << (index % 32));
    }

    g_free_pages -= count;
    g_hal_state.memory.available_bytes = g_free_pages * UKURAN_HALAMAN;

    return (alamat_fisik_t)(start_index * UKURAN_HALAMAN);
}

/*
 * hal_memory_free_pages
 * ---------------------
 * Bebaskan multiple halaman fisik.
 *
 * Parameter:
 *   addr  - Alamat fisik halaman pertama
 *   count - Jumlah halaman
 *
 * Return: Status operasi
 */
status_t hal_memory_free_pages(alamat_fisik_t addr, tak_bertanda32_t count)
{
    tak_bertanda32_t i;
    tak_bertanda32_t start_index;

    if (addr % UKURAN_HALAMAN != 0 || count == 0) {
        return STATUS_PARAM_INVALID;
    }

    start_index = (tak_bertanda32_t)(addr / UKURAN_HALAMAN);
    if (start_index + count > MAX_PAGES) {
        return STATUS_PARAM_INVALID;
    }

    /* Bebaskan setiap halaman */
    for (i = 0; i < count; i++) {
        tak_bertanda32_t index = start_index + i;
        if (g_page_bitmap[index / 32] & (1U << (index % 32))) {
            g_page_bitmap[index / 32] &= ~(1U << (index % 32));
            g_free_pages++;
        }
    }

    g_hal_state.memory.available_bytes = g_free_pages * UKURAN_HALAMAN;

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI CONSOLE HAL (HAL CONSOLE FUNCTIONS)
 * ============================================================================
 */

/* UART PL011 base address */
#define UART0_BASE          0x09000000
#define UART0_DR            0x00
#define UART0_FR            0x18
#define UART0_IBRD          0x24
#define UART0_FBRD          0x28
#define UART0_LCRH          0x2C
#define UART0_CR            0x30

/* UART flags */
#define UART_FR_TXFF        0x20    /* Transmit FIFO full */

/* Console color (tidak digunakan di UART) */
static tak_bertanda8_t g_console_fg = 7;
static tak_bertanda8_t g_console_bg = 0;

/* Console cursor position */
static tak_bertanda32_t g_console_x = 0;
static tak_bertanda32_t g_console_y = 0;

/*
 * hal_console_init
 * ----------------
 * Inisialisasi console.
 *
 * Return: Status operasi
 */
status_t hal_console_init(void)
{
    volatile tak_bertanda32_t *uart = (volatile tak_bertanda32_t *)UART0_BASE;

    /* Disable UART */
    uart[UART0_CR / 4] = 0;

    /* Set baud rate 115200 */
    uart[UART0_IBRD / 4] = 13;   /* Integer part */
    uart[UART0_FBRD / 4] = 0;    /* Fractional part */

    /* 8 bits, no parity, one stop bit, enable FIFO */
    uart[UART0_LCRH / 4] = 0x70;

    /* Enable UART, TX, RX */
    uart[UART0_CR / 4] = 0x301;

    g_console_x = 0;
    g_console_y = 0;

    return STATUS_BERHASIL;
}

/*
 * hal_console_putchar
 * -------------------
 * Tampilkan satu karakter.
 *
 * Parameter:
 *   c - Karakter
 *
 * Return: Status operasi
 */
status_t hal_console_putchar(char c)
{
    volatile tak_bertanda32_t *uart = (volatile tak_bertanda32_t *)UART0_BASE;

    /* Handle newline */
    if (c == '\n') {
        /* Wait for TX FIFO not full */
        while (uart[UART0_FR / 4] & UART_FR_TXFF) {
            /* Busy wait */
        }
        uart[UART0_DR / 4] = '\r';
    }

    /* Wait for TX FIFO not full */
    while (uart[UART0_FR / 4] & UART_FR_TXFF) {
        /* Busy wait */
    }

    /* Send character */
    uart[UART0_DR / 4] = (tak_bertanda32_t)c;

    return STATUS_BERHASIL;
}

/*
 * hal_console_puts
 * ----------------
 * Tampilkan string.
 *
 * Parameter:
 *   str - String
 *
 * Return: Status operasi
 */
status_t hal_console_puts(const char *str)
{
    while (*str != '\0') {
        hal_console_putchar(*str);
        str++;
    }
    return STATUS_BERHASIL;
}

/*
 * hal_console_clear
 * -----------------
 * Bersihkan layar.
 *
 * Return: Status operasi
 */
status_t hal_console_clear(void)
{
    /* Kirim ANSI clear screen */
    hal_console_puts("\033[2J\033[H");
    g_console_x = 0;
    g_console_y = 0;
    return STATUS_BERHASIL;
}

/*
 * hal_console_set_color
 * ---------------------
 * Set warna teks.
 *
 * Parameter:
 *   fg - Warna foreground
 *   bg - Warna background
 *
 * Return: Status operasi
 */
status_t hal_console_set_color(tak_bertanda8_t fg, tak_bertanda8_t bg)
{
    g_console_fg = fg;
    g_console_bg = bg;
    return STATUS_BERHASIL;
}

/*
 * hal_console_set_cursor
 * ----------------------
 * Set posisi cursor.
 *
 * Parameter:
 *   x - Posisi X (kolom)
 *   y - Posisi Y (baris)
 *
 * Return: Status operasi
 */
status_t hal_console_set_cursor(tak_bertanda32_t x, tak_bertanda32_t y)
{
    g_console_x = x;
    g_console_y = y;
    return STATUS_BERHASIL;
}

/*
 * hal_console_get_cursor
 * ----------------------
 * Dapatkan posisi cursor.
 *
 * Parameter:
 *   x - Pointer untuk menyimpan posisi X
 *   y - Pointer untuk menyimpan posisi Y
 *
 * Return: Status operasi
 */
status_t hal_console_get_cursor(tak_bertanda32_t *x, tak_bertanda32_t *y)
{
    if (x != NULL) {
        *x = g_console_x;
    }
    if (y != NULL) {
        *y = g_console_y;
    }
    return STATUS_BERHASIL;
}

/*
 * hal_console_scroll
 * ------------------
 * Scroll layar.
 *
 * Parameter:
 *   lines - Jumlah baris
 *
 * Return: Status operasi
 */
status_t hal_console_scroll(tak_bertanda32_t lines)
{
    (void)lines;
    return STATUS_BERHASIL;
}

/*
 * hal_console_get_size
 * --------------------
 * Dapatkan ukuran layar.
 *
 * Parameter:
 *   width  - Pointer untuk menyimpan lebar
 *   height - Pointer untuk menyimpan tinggi
 *
 * Return: Status operasi
 */
status_t hal_console_get_size(tak_bertanda32_t *width, tak_bertanda32_t *height)
{
    if (width != NULL) {
        *width = 80;
    }
    if (height != NULL) {
        *height = 24;
    }
    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * VARIABEL EXTERN (EXTERN VARIABLES)
 * ============================================================================
 */

/* Reference ke g_cpu_info dari cpu_arm.c */
extern hal_cpu_info_t g_cpu_info;
