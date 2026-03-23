/*
 * PIGURA OS - HAL x86
 * --------------------
 * Implementasi Hardware Abstraction Layer untuk arsitektur x86.
 *
 * Berkas ini berisi implementasi fungsi-fungsi HAL yang spesifik
 * untuk arsitektur x86 (32-bit). Fungsi-fungsi ini mengabstraksi
 * detail hardware dari kernel utama.
 *
 * Arsitektur: x86 (32-bit)
 * Versi: 1.0
 */

#include "../../inti/kernel.h"
#include "cpu_x86.h"

/*
 * ============================================================================
 * VARIABEL GLOBAL
 * ============================================================================
 */

/* State HAL global */
hal_state_t g_hal_state_x86;

/* Flag inisialisasi */
static bool_t g_hal_diinisialisasi = SALAH;

/* Pointer ke boot info */
static void *g_boot_info_ptr = NULL;

/*
 * ============================================================================
 * FUNGSI INTERNAL
 * ============================================================================
 */

/*
 * _init_console_awal
 * ------------------
 * Inisialisasi console VGA text mode untuk output awal.
 */
static status_t _init_console_awal(void)
{
    volatile tak_bertanda16_t *vga;
    tak_bertanda32_t i;

    vga = (volatile tak_bertanda16_t *)VGA_BUFFER;

    /* Clear screen */
    for (i = 0; i < VGA_KOLOM * VGA_BARIS; i++) {
        vga[i] = (tak_bertanda16_t)(' ' | (VGA_ABU_ABU << 8));
    }

    return STATUS_BERHASIL;
}

/*
 * _init_gdt
 * ---------
 * Inisialisasi Global Descriptor Table.
 */
static status_t _init_gdt(void)
{
    /* GDT init dipanggil dari modul memori */
    return STATUS_BERHASIL;
}

/*
 * _init_idt
 * ----------
 * Inisialisasi Interrupt Descriptor Table.
 */
static status_t _init_idt(void)
{
    /* IDT init dipanggil dari modul interupsi */
    return STATUS_BERHASIL;
}

/*
 * _remap_pic
 * ----------
 * Remap PIC untuk menghindari konflik vektor dengan exception.
 */
static void _remap_pic(void)
{
    /* Mulai remap */
    outb(PIC1_COMMAND, PIC_CMD_ICW1_INIT | PIC_CMD_ICW1_ICW4);
    io_delay();
    outb(PIC2_COMMAND, PIC_CMD_ICW1_INIT | PIC_CMD_ICW1_ICW4);
    io_delay();

    /* Set vektor offset */
    outb(PIC1_DATA, VEKTOR_IRQ_MULAI);      /* Master: vektor 32-39 */
    io_delay();
    outb(PIC2_DATA, VEKTOR_IRQ_MULAI + 8);   /* Slave: vektor 40-47 */
    io_delay();

    /* Setup cascade */
    outb(PIC1_DATA, 0x04);   /* Tell master ada slave di IRQ2 */
    io_delay();
    outb(PIC2_DATA, 0x02);   /* Tell slave identitasnya */
    io_delay();

    /* Set mode 8086 */
    outb(PIC1_DATA, PIC_CMD_ICW4_8086);
    io_delay();
    outb(PIC2_DATA, PIC_CMD_ICW4_8086);
    io_delay();

    /* Mask semua IRQ kecuali cascade */
    outb(PIC1_DATA, 0xFB);   /* Unmask IRQ2 (cascade) */
    outb(PIC2_DATA, 0xFF);   /* Mask semua IRQ slave */
}

/*
 * _init_pit
 * ----------
 * Inisialisasi Programmable Interval Timer untuk sistem timer.
 */
static void _init_pit(tak_bertanda32_t frekuensi)
{
    tak_bertanda16_t divisor;

    /* Hitung divisor */
    divisor = (tak_bertanda16_t)(FREKUENSI_PIT / frekuensi);

    /* Set mode square wave, channel 0, binary counting */
    outb(PIT_COMMAND, PIT_CMD_BINARY | PIT_CMD_MODE_3 | 
                       PIT_CMD_BOTH | 0x00);

    /* Kirim divisor (LSB dulu, lalu MSB) */
    outb(PIT_CHANNEL_0, (tak_bertanda8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL_0, (tak_bertanda8_t)((divisor >> 8) & 0xFF));
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - HAL UTAMA
 * ============================================================================
 */

/*
 * hal_init
 * --------
 * Menginisialisasi HAL untuk arsitektur x86.
 */
status_t hal_init(void)
{
    kernel_memset(&g_hal_state_x86, 0, sizeof(hal_state_t));

    /* Inisialisasi console awal untuk debug */
    _init_console_awal();

    kernel_printf("[HAL-x86] Memulai inisialisasi HAL...\n");

    /* Inisialisasi GDT */
    kernel_printf("[HAL-x86] Inisialisasi GDT...\n");

    /* Inisialisasi IDT */
    kernel_printf("[HAL-x86] Inisialisasi IDT...\n");

    /* Remap PIC */
    kernel_printf("[HAL-x86] Remap PIC...\n");
    _remap_pic();

    /* Inisialisasi timer */
    kernel_printf("[HAL-x86] Inisialisasi timer...\n");
    g_hal_state_x86.timer.frequency = FREKUENSI_TIMER;
    g_hal_state_x86.timer.resolution_ns = 
        (1000000000UL / FREKUENSI_TIMER);
    g_hal_state_x86.timer.supports_one_shot = SALAH;
    g_hal_state_x86.timer.supports_periodic = BENAR;

    _init_pit(FREKUENSI_TIMER);

    /* Set flag inisialisasi */
    g_hal_state_x86.status = HAL_STATUS_READY;
    g_hal_diinisialisasi = BENAR;

    kernel_printf("[HAL-x86] HAL siap\n");

    return STATUS_BERHASIL;
}

/*
 * hal_shutdown
 * ------------
 * Mematikan HAL.
 */
status_t hal_shutdown(void)
{
    if (!g_hal_diinisialisasi) {
        return STATUS_GAGAL;
    }

    kernel_printf("[HAL-x86] Mematikan HAL...\n");

    /* Disable interrupt */
    hal_cpu_disable_interrupts();

    g_hal_state_x86.status = HAL_STATUS_UNINITIALIZED;
    g_hal_diinisialisasi = SALAH;

    return STATUS_BERHASIL;
}

/*
 * hal_get_state
 * -------------
 * Mendapatkan state HAL.
 */
const hal_state_t *hal_get_state(void)
{
    if (!g_hal_diinisialisasi) {
        return NULL;
    }

    return &g_hal_state_x86;
}

/*
 * hal_is_initialized
 * ------------------
 * Cek apakah HAL sudah diinisialisasi.
 */
bool_t hal_is_initialized(void)
{
    return g_hal_diinisialisasi;
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - CPU
 * ============================================================================
 */

/*
 * hal_cpu_init
 * ------------
 * Inisialisasi subsistem CPU.
 */
status_t hal_cpu_init(void)
{
    return STATUS_BERHASIL;
}

/*
 * hal_cpu_halt
 * ------------
 * Hentikan CPU.
 */
void hal_cpu_halt(void)
{
    cpu_halt();
}

/*
 * hal_cpu_reset
 * -------------
 * Reset CPU.
 */
void hal_cpu_reset(bool_t keras)
{
    /* Disable interrupt */
    cpu_disable_irq();

    if (keras) {
        /* Hard reset via keyboard controller */
        tak_bertanda8_t temp;

        /* Flush keyboard buffer */
        do {
            temp = inb(0x64);
            if (temp & 0x01) {
                inb(0x60);
            }
        } while (temp & 0x02);

        /* Pulse reset line */
        outb(0x64, 0xFE);
    } else {
        /* Soft reset */
        tak_bertanda32_t *warm_reset;
        warm_reset = (tak_bertanda32_t *)0x0472;
        *warm_reset = 0x1234;

        __asm__ __volatile__(
            "ljmp $0xFFFF, $0x0000\n\t"
        );
    }

    /* Tidak akan sampai sini */
    while (1) {
        cpu_halt();
    }
}

/*
 * hal_cpu_get_id
 * --------------
 * Mendapatkan ID CPU.
 */
tak_bertanda32_t hal_cpu_get_id(void)
{
    return 0;
}

/*
 * hal_cpu_get_info
 * ----------------
 * Mendapatkan informasi CPU.
 */
status_t hal_cpu_get_info(hal_cpu_info_t *info)
{
    if (info == NULL) {
        return STATUS_PARAM_INVALID;
    }

    return STATUS_BERHASIL;
}

/*
 * hal_cpu_enable_interrupts
 * -------------------------
 * Aktifkan interupsi.
 */
void hal_cpu_enable_interrupts(void)
{
    cpu_enable_irq();
}

/*
 * hal_cpu_disable_interrupts
 * --------------------------
 * Nonaktifkan interupsi.
 */
void hal_cpu_disable_interrupts(void)
{
    cpu_disable_irq();
}

/*
 * hal_cpu_save_interrupts
 * -----------------------
 * Simpan state interupsi.
 */
tak_bertanda32_t hal_cpu_save_interrupts(void)
{
    return cpu_save_flags();
}

/*
 * hal_cpu_restore_interrupts
 * --------------------------
 * Restore state interupsi.
 */
void hal_cpu_restore_interrupts(tak_bertanda32_t state)
{
    cpu_restore_flags(state);
}

/*
 * hal_cpu_invalidate_tlb
 * ----------------------
 * Invalidate TLB untuk alamat.
 */
void hal_cpu_invalidate_tlb(void *addr)
{
    cpu_invlpg(addr);
}

/*
 * hal_cpu_invalidate_tlb_all
 * --------------------------
 * Invalidate semua TLB.
 */
void hal_cpu_invalidate_tlb_all(void)
{
    cpu_reload_cr3();
}

/*
 * hal_cpu_cache_flush
 * -------------------
 * Flush cache.
 */
void hal_cpu_cache_flush(void)
{
    __asm__ __volatile__("wbinvd\n\t");
}

/*
 * hal_cpu_cache_disable
 * ---------------------
 * Nonaktifkan cache.
 */
void hal_cpu_cache_disable(void)
{
    tak_bertanda32_t cr0;

    cr0 = cpu_read_cr0();
    cr0 |= (1 << 30) | (1 << 29);
    cpu_write_cr0(cr0);
}

/*
 * hal_cpu_cache_enable
 * --------------------
 * Aktifkan cache.
 */
void hal_cpu_cache_enable(void)
{
    tak_bertanda32_t cr0;

    cr0 = cpu_read_cr0();
    cr0 &= ~((1 << 30) | (1 << 29));
    cpu_write_cr0(cr0);
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - TIMER
 * ============================================================================
 */

/* Variabel timer */
static volatile tak_bertanda64_t g_timer_ticks = 0;
static volatile tak_bertanda64_t g_timer_uptime = 0;
static void (*g_timer_handler)(void) = NULL;

/*
 * timer_interrupt_handler
 * -----------------------
 * Handler untuk timer interrupt.
 */
void timer_interrupt_handler(void)
{
    g_timer_ticks++;

    /* Update uptime setiap detik */
    if ((g_timer_ticks % FREKUENSI_TIMER) == 0) {
        g_timer_uptime++;
    }

    /* Panggil handler user jika ada */
    if (g_timer_handler != NULL) {
        g_timer_handler();
    }
}

/*
 * hal_timer_init
 * --------------
 * Inisialisasi timer.
 */
status_t hal_timer_init(void)
{
    g_timer_ticks = 0;
    g_timer_uptime = 0;

    /* PIT sudah di-init di hal_init */
    g_hal_state_x86.timer.frequency = FREKUENSI_TIMER;
    g_hal_state_x86.timer.resolution_ns = 
        (1000000000UL / FREKUENSI_TIMER);

    return STATUS_BERHASIL;
}

/*
 * hal_timer_get_ticks
 * -------------------
 * Mendapatkan jumlah tick.
 */
tak_bertanda64_t hal_timer_get_ticks(void)
{
    return g_timer_ticks;
}

/*
 * hal_timer_get_frequency
 * -----------------------
 * Mendapatkan frekuensi timer.
 */
tak_bertanda32_t hal_timer_get_frequency(void)
{
    return g_hal_state_x86.timer.frequency;
}

/*
 * hal_timer_get_uptime
 * --------------------
 * Mendapatkan uptime dalam detik.
 */
tak_bertanda64_t hal_timer_get_uptime(void)
{
    return g_timer_uptime;
}

/*
 * hal_timer_sleep
 * ---------------
 * Sleep untuk durasi tertentu.
 */
void hal_timer_sleep(tak_bertanda32_t milidetik)
{
    tak_bertanda64_t start;
    tak_bertanda64_t target;
    tak_bertanda32_t ticks_per_ms;

    if (milidetik == 0) {
        return;
    }

    ticks_per_ms = FREKUENSI_TIMER / 1000;
    if (ticks_per_ms == 0) {
        ticks_per_ms = 1;
    }

    start = g_timer_ticks;
    target = start + ((tak_bertanda64_t)milidetik * ticks_per_ms);

    while (g_timer_ticks < target) {
        cpu_halt();
    }
}

/*
 * hal_timer_delay
 * ---------------
 * Busy-wait delay.
 */
void hal_timer_delay(tak_bertanda32_t mikrodetik)
{
    tak_bertanda64_t start;
    tak_bertanda64_t end;
    tak_bertanda64_t delay_cycls;
    tak_bertanda32_t freq_khz;

    freq_khz = g_hal_state_x86.cpu.freq_khz;
    if (freq_khz == 0) {
        freq_khz = 1000;
    }

    start = g_timer_ticks;
    delay_cycls = (tak_bertanda64_t)freq_khz * mikrodetik;

    do {
        end = g_timer_ticks;
    } while ((end - start) < (delay_cycls / FREKUENSI_TIMER));
}

/*
 * hal_timer_set_handler
 * ---------------------
 * Set handler timer.
 */
status_t hal_timer_set_handler(void (*handler)(void))
{
    g_timer_handler = handler;
    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - INTERUPSI
 * ============================================================================
 */

/*
 * hal_interrupt_init
 * ------------------
 * Inisialisasi subsistem interupsi.
 */
status_t hal_interrupt_init(void)
{
    g_hal_state_x86.interrupt.irq_count = JUMLAH_IRQ;
    g_hal_state_x86.interrupt.vector_base = VEKTOR_IRQ_MULAI;
    g_hal_state_x86.interrupt.has_apic = SALAH;
    g_hal_state_x86.interrupt.has_ioapic = SALAH;

    return STATUS_BERHASIL;
}

/*
 * hal_interrupt_enable
 * --------------------
 * Aktifkan IRQ.
 */
status_t hal_interrupt_enable(tak_bertanda32_t irq)
{
    tak_bertanda8_t mask;
    tak_bertanda16_t port;

    if (irq > 15) {
        return STATUS_PARAM_INVALID;
    }

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    /* Clear bit untuk enable IRQ */
    mask = inb(port);
    mask &= ~(1 << irq);
    outb(port, mask);

    return STATUS_BERHASIL;
}

/*
 * hal_interrupt_disable
 * ---------------------
 * Nonaktifkan IRQ.
 */
status_t hal_interrupt_disable(tak_bertanda32_t irq)
{
    tak_bertanda8_t mask;
    tak_bertanda16_t port;

    if (irq > 15) {
        return STATUS_PARAM_INVALID;
    }

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    /* Set bit untuk disable IRQ */
    mask = inb(port);
    mask |= (1 << irq);
    outb(port, mask);

    return STATUS_BERHASIL;
}

/*
 * hal_interrupt_mask
 * ------------------
 * Mask IRQ.
 */
status_t hal_interrupt_mask(tak_bertanda32_t irq)
{
    return hal_interrupt_disable(irq);
}

/*
 * hal_interrupt_unmask
 * --------------------
 * Unmask IRQ.
 */
status_t hal_interrupt_unmask(tak_bertanda32_t irq)
{
    return hal_interrupt_enable(irq);
}

/*
 * hal_interrupt_acknowledge
 * -------------------------
 * Acknowledge IRQ.
 */
void hal_interrupt_acknowledge(tak_bertanda32_t irq)
{
    if (irq >= 8) {
        /* Ack slave PIC */
        outb(PIC2_COMMAND, PIC_CMD_EOI);
    }

    /* Ack master PIC */
    outb(PIC1_COMMAND, PIC_CMD_EOI);
}

/*
 * hal_interrupt_set_handler
 * -------------------------
 * Set handler IRQ.
 */
status_t hal_interrupt_set_handler(tak_bertanda32_t irq,
                                    void (*handler)(void))
{
    if (irq > 15) {
        return STATUS_PARAM_INVALID;
    }

    /* Handler registration akan dilakukan via IDT */
    return STATUS_BERHASIL;
}

/*
 * hal_interrupt_get_handler
 * -------------------------
 * Dapatkan handler IRQ.
 */
void (*hal_interrupt_get_handler(tak_bertanda32_t irq))(void)
{
    if (irq > 15) {
        return NULL;
    }

    return NULL;
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - MEMORI
 * ============================================================================
 */

/*
 * hal_memory_init
 * ---------------
 * Inisialisasi subsistem memori.
 */
status_t hal_memory_init(void *bootinfo)
{
    multiboot_info_t *mbi;
    tak_bertanda64_t total_mem;

    g_boot_info_ptr = bootinfo;

    if (bootinfo != NULL) {
        mbi = (multiboot_info_t *)bootinfo;

        /* Baca info memori dari multiboot */
        if (mbi->flags & MULTIBOOT_FLAG_MEM) {
            total_mem = (tak_bertanda64_t)mbi->mem_lower * 1024;
            total_mem += (tak_bertanda64_t)mbi->mem_upper * 1024;

            g_hal_state_x86.memory.total_bytes = total_mem;
            g_hal_state_x86.memory.available_bytes = total_mem;
        }
    }

    g_hal_state_x86.memory.page_size = UKURAN_HALAMAN;
    g_hal_state_x86.memory.page_count = 
        g_hal_state_x86.memory.total_bytes / UKURAN_HALAMAN;
    g_hal_state_x86.memory.has_pae = SALAH;
    g_hal_state_x86.memory.has_nx = SALAH;

    return STATUS_BERHASIL;
}

/*
 * hal_memory_get_info
 * -------------------
 * Mendapatkan informasi memori.
 */
status_t hal_memory_get_info(hal_memory_info_t *info)
{
    if (info == NULL) {
        return STATUS_PARAM_INVALID;
    }

    kernel_memcpy(info, &g_hal_state_x86.memory, 
                  sizeof(hal_memory_info_t));

    return STATUS_BERHASIL;
}

/*
 * hal_memory_get_free_pages
 * -------------------------
 * Mendapatkan jumlah halaman bebas.
 */
tak_bertanda64_t hal_memory_get_free_pages(void)
{
    /* Akan diimplementasikan oleh PMM */
    return 0;
}

/*
 * hal_memory_get_total_pages
 * --------------------------
 * Mendapatkan jumlah total halaman.
 */
tak_bertanda64_t hal_memory_get_total_pages(void)
{
    return g_hal_state_x86.memory.page_count;
}

/*
 * hal_memory_alloc_page
 * ---------------------
 * Alokasikan satu halaman.
 */
alamat_fisik_t hal_memory_alloc_page(void)
{
    /* Akan diimplementasikan oleh PMM */
    return ALAMAT_FISIK_INVALID;
}

/*
 * hal_memory_free_page
 * --------------------
 * Bebaskan satu halaman.
 */
status_t hal_memory_free_page(alamat_fisik_t addr)
{
    /* Akan diimplementasikan oleh PMM */
    return STATUS_TIDAK_DUKUNG;
}

/*
 * hal_memory_alloc_pages
 * ----------------------
 * Alokasikan multiple halaman.
 */
alamat_fisik_t hal_memory_alloc_pages(tak_bertanda32_t count)
{
    /* Akan diimplementasikan oleh PMM */
    return ALAMAT_FISIK_INVALID;
}

/*
 * hal_memory_free_pages
 * ---------------------
 * Bebaskan multiple halaman.
 */
status_t hal_memory_free_pages(alamat_fisik_t addr,
                                tak_bertanda32_t count)
{
    /* Akan diimplementasikan oleh PMM */
    return STATUS_TIDAK_DUKUNG;
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - CONSOLE
 * ============================================================================
 */

/* Posisi cursor */
static tak_bertanda32_t g_cursor_x = 0;
static tak_bertanda32_t g_cursor_y = 0;
static tak_bertanda8_t g_warna_fg = VGA_ABU_ABU;
static tak_bertanda8_t g_warna_bg = VGA_HITAM;

/*
 * hal_console_init
 * ----------------
 * Inisialisasi console.
 */
status_t hal_console_init(void)
{
    volatile tak_bertanda16_t *vga;
    tak_bertanda32_t i;

    vga = (volatile tak_bertanda16_t *)VGA_BUFFER;

    /* Clear screen */
    for (i = 0; i < VGA_KOLOM * VGA_BARIS; i++) {
        vga[i] = (tak_bertanda16_t)(' ' | 
            (VGA_ATTR(VGA_ABU_ABU, VGA_HITAM) << 8));
    }

    g_cursor_x = 0;
    g_cursor_y = 0;
    g_warna_fg = VGA_ABU_ABU;
    g_warna_bg = VGA_HITAM;

    return STATUS_BERHASIL;
}

/*
 * hal_console_putchar
 * -------------------
 * Tampilkan satu karakter.
 */
status_t hal_console_putchar(char c)
{
    volatile tak_bertanda16_t *vga;
    tak_bertanda32_t offset;

    vga = (volatile tak_bertanda16_t *)VGA_BUFFER;

    if (c == '\n') {
        g_cursor_x = 0;
        g_cursor_y++;
    } else if (c == '\r') {
        g_cursor_x = 0;
    } else if (c == '\t') {
        g_cursor_x = (g_cursor_x + 4) & ~3;
        if (g_cursor_x >= VGA_KOLOM) {
            g_cursor_x = 0;
            g_cursor_y++;
        }
    } else if (c == '\b') {
        if (g_cursor_x > 0) {
            g_cursor_x--;
            offset = g_cursor_y * VGA_KOLOM + g_cursor_x;
            vga[offset] = (tak_bertanda16_t)(' ' | 
                (VGA_ATTR(g_warna_fg, g_warna_bg) << 8));
        }
    } else {
        offset = g_cursor_y * VGA_KOLOM + g_cursor_x;
        vga[offset] = (tak_bertanda16_t)((tak_bertanda8_t)c | 
            (VGA_ATTR(g_warna_fg, g_warna_bg) << 8));

        g_cursor_x++;

        if (g_cursor_x >= VGA_KOLOM) {
            g_cursor_x = 0;
            g_cursor_y++;
        }
    }

    /* Scroll jika perlu */
    if (g_cursor_y >= VGA_BARIS) {
        tak_bertanda32_t i;

        /* Copy baris ke atas */
        for (i = 0; i < (VGA_BARIS - 1) * VGA_KOLOM; i++) {
            vga[i] = vga[i + VGA_KOLOM];
        }

        /* Clear baris terakhir */
        for (i = (VGA_BARIS - 1) * VGA_KOLOM; 
             i < VGA_BARIS * VGA_KOLOM; i++) {
            vga[i] = (tak_bertanda16_t)(' ' | 
                (VGA_ATTR(g_warna_fg, g_warna_bg) << 8));
        }

        g_cursor_y = VGA_BARIS - 1;
    }

    /* Update cursor hardware */
    offset = g_cursor_y * VGA_KOLOM + g_cursor_x;
    outb(PORT_VGA_CRTC_INDEX, 0x0F);
    outb(PORT_VGA_CRTC_DATA, (tak_bertanda8_t)(offset & 0xFF));
    outb(PORT_VGA_CRTC_INDEX, 0x0E);
    outb(PORT_VGA_CRTC_DATA, (tak_bertanda8_t)((offset >> 8) & 0xFF));

    return STATUS_BERHASIL;
}

/*
 * hal_console_puts
 * ----------------
 * Tampilkan string.
 */
status_t hal_console_puts(const char *str)
{
    if (str == NULL) {
        return STATUS_PARAM_INVALID;
    }

    while (*str) {
        hal_console_putchar(*str);
        str++;
    }

    return STATUS_BERHASIL;
}

/*
 * hal_console_clear
 * -----------------
 * Bersihkan layar.
 */
status_t hal_console_clear(void)
{
    return hal_console_init();
}

/*
 * hal_console_set_color
 * ---------------------
 * Set warna teks.
 */
status_t hal_console_set_color(tak_bertanda8_t fg, tak_bertanda8_t bg)
{
    if (fg > 15 || bg > 7) {
        return STATUS_PARAM_INVALID;
    }

    g_warna_fg = fg;
    g_warna_bg = bg;

    return STATUS_BERHASIL;
}

/*
 * hal_console_set_cursor
 * ----------------------
 * Set posisi cursor.
 */
status_t hal_console_set_cursor(tak_bertanda32_t x, tak_bertanda32_t y)
{
    if (x >= VGA_KOLOM || y >= VGA_BARIS) {
        return STATUS_PARAM_INVALID;
    }

    g_cursor_x = x;
    g_cursor_y = y;

    return STATUS_BERHASIL;
}

/*
 * hal_console_get_cursor
 * ----------------------
 * Dapatkan posisi cursor.
 */
status_t hal_console_get_cursor(tak_bertanda32_t *x, tak_bertanda32_t *y)
{
    if (x != NULL) {
        *x = g_cursor_x;
    }
    if (y != NULL) {
        *y = g_cursor_y;
    }

    return STATUS_BERHASIL;
}

/*
 * hal_console_scroll
 * ------------------
 * Scroll layar.
 */
status_t hal_console_scroll(tak_bertanda32_t lines)
{
    volatile tak_bertanda16_t *vga;
    tak_bertanda32_t i;
    tak_bertanda32_t j;

    vga = (volatile tak_bertanda16_t *)VGA_BUFFER;

    if (lines == 0) {
        return STATUS_BERHASIL;
    }

    if (lines > 0) {
        /* Scroll ke atas */
        for (i = 0; i < (VGA_BARIS - lines) * VGA_KOLOM; i++) {
            vga[i] = vga[i + lines * VGA_KOLOM];
        }

        /* Clear baris bawah */
        for (j = 0; j < lines; j++) {
            for (i = 0; i < VGA_KOLOM; i++) {
                vga[(VGA_BARIS - 1 - j) * VGA_KOLOM + i] = 
                    (tak_bertanda16_t)(' ' | 
                        (VGA_ATTR(g_warna_fg, g_warna_bg) << 8));
            }
        }

        if (g_cursor_y >= lines) {
            g_cursor_y -= lines;
        } else {
            g_cursor_y = 0;
        }
    }

    return STATUS_BERHASIL;
}

/*
 * hal_console_get_size
 * --------------------
 * Dapatkan ukuran layar.
 */
status_t hal_console_get_size(tak_bertanda32_t *lebar,
                               tak_bertanda32_t *tinggi)
{
    if (lebar != NULL) {
        *lebar = VGA_KOLOM;
    }
    if (tinggi != NULL) {
        *tinggi = VGA_BARIS;
    }

    return STATUS_BERHASIL;
}

/*
 * hal_console_print_error
 * -----------------------
 * Print pesan error (merah).
 */
void hal_console_print_error(const char *str)
{
    tak_bertanda8_t old_fg = g_warna_fg;
    tak_bertanda8_t old_bg = g_warna_bg;

    g_warna_fg = VGA_MERAH_TERANG;
    g_warna_bg = VGA_HITAM;

    hal_console_puts(str);

    g_warna_fg = old_fg;
    g_warna_bg = old_bg;
}

/*
 * hal_console_print_warning
 * -------------------------
 * Print pesan warning (kuning).
 */
void hal_console_print_warning(const char *str)
{
    tak_bertanda8_t old_fg = g_warna_fg;
    tak_bertanda8_t old_bg = g_warna_bg;

    g_warna_fg = VGA_KUNING;
    g_warna_bg = VGA_HITAM;

    hal_console_puts(str);

    g_warna_fg = old_fg;
    g_warna_bg = old_bg;
}
