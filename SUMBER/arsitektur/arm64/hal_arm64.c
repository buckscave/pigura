/*
 * PIGURA OS - HAL_ARM64.C
 * -----------------------
 * Implementasi Hardware Abstraction Layer untuk ARM64 (AArch64).
 *
 * Berkas ini mengimplementasikan interface HAL untuk arsitektur
 * ARM64/AArch64, menyediakan abstraksi hardware yang portabel.
 *
 * Arsitektur: ARM64/AArch64 (64-bit)
 * Versi: 1.0
 */

#include "../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA HAL ARM64
 * ============================================================================
 */

/* Alamat kernel untuk ARM64 */
#define KERNEL_MULAI_VIRT      0xFFFF800000000000ULL
#define KERNEL_MULAI_FISIK     0x00080000ULL

/* Alamat UART default (versatile/QEMU) */
#define UART0_BASE             0x09000000ULL

/* UART register offsets */
#define UART_DR                0x00
#define UART_FR                0x18
#define UART_FR_TXFF           (1 << 5)

/* GICv3 addresses */
#define GICD_BASE              0x08000000ULL
#define GICR_BASE              0x080A0000ULL

/*
 * ============================================================================
 * VARIABEL GLOBAL
 * ============================================================================
 */

/* State HAL global */
hal_state_t g_hal_state_arm64;

/* Flag inisialisasi */
static bool_t g_hal_diinisalisasi = SALAH;

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
 * Inisialisasi console UART untuk output awal.
 */
static status_t _init_console_awal(void)
{
    volatile tak_bertanda8_t *uart;
    tak_bertanda32_t i;

    uart = (volatile tak_bertanda8_t *)UART0_BASE;

    /* Wait for UART to be ready */
    for (i = 0; i < 1000; i++) {
        volatile tak_bertanda32_t dummy;
        dummy = *(volatile tak_bertanda32_t *)(UART0_BASE + UART_FR);
        (void)dummy;
    }

    return STATUS_BERHASIL;
}

/*
 * _uart_putchar
 * -------------
 * Kirim karakter ke UART.
 */
static void _uart_putchar(char c)
{
    volatile tak_bertanda8_t *uart;

    uart = (volatile tak_bertanda8_t *)UART0_BASE;

    /* Wait until transmit FIFO is not full */
    while (*(volatile tak_bertanda32_t *)(UART0_BASE + UART_FR) &
           UART_FR_TXFF) {
        /* Spin */
    }

    *uart = (tak_bertanda8_t)c;
}

/*
 * _init_gicv3
 * -----------
 * Inisialisasi GICv3 interrupt controller.
 */
static void _init_gicv3(void)
{
    volatile tak_bertanda32_t *gicd;
    tak_bertanda32_t i;

    gicd = (volatile tak_bertanda32_t *)GICD_BASE;

    /* Disable distributor */
    gicd[0] = 0;

    /* Wait for pending writes */
    for (i = 0; i < 1000; i++) {
        volatile tak_bertanda32_t dummy;
        dummy = gicd[0];
        (void)dummy;
    }

    /* Enable distributor */
    gicd[0] = 1;

    /* Enable all CPU interfaces (SRE, EnableGrp1) */
    __asm__ __volatile__(
        "msr ICC_SRE_EL1, %0\n\t"
        "isb"
        :
        : "r"(0x7)
        : "memory"
    );

    /* Set priority mask */
    __asm__ __volatile__(
        "msr ICC_PMR_EL1, %0\n\t"
        "isb"
        :
        : "r"(0xFF)
        : "memory"
    );

    /* Enable group 1 interrupts */
    __asm__ __volatile__(
        "msr ICC_IGRPEN1_EL1, %0\n\t"
        "isb"
        :
        : "r"(0x1)
        : "memory"
    );
}

/*
 * _init_timer_arm64
 * -----------------
 * Inisialisasi ARM Generic Timer.
 */
static void _init_timer_arm64(tak_bertanda32_t frekuensi)
{
    tak_bertanda64_t timer_freq;
    tak_bertanda64_t timer_value;

    /* Baca frekuensi timer */
    __asm__ __volatile__(
        "mrs %0, CNTFRQ_EL0"
        : "=r"(timer_freq)
    );

    /* Hitung nilai reload */
    if (timer_freq > 0) {
        timer_value = timer_freq / (tak_bertanda64_t)frekuensi;
    } else {
        timer_value = 1000000ULL; /* Default 1MHz */
    }

    /* Set timer compare value */
    __asm__ __volatile__(
        "msr CNTP_TVAL_EL0, %0\n\t"
        "isb"
        :
        : "r"(timer_value)
        : "memory"
    );

    /* Enable timer */
    __asm__ __volatile__(
        "msr CNTP_CTL_EL0, %0\n\t"
        "isb"
        :
        : "r"(0x1)
        : "memory"
    );
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - HAL UTAMA
 * ============================================================================
 */

/*
 * hal_init
 * --------
 * Menginisialisasi HAL untuk arsitektur ARM64.
 */
status_t hal_init(void)
{
    kernel_memset(&g_hal_state_arm64, 0, sizeof(hal_state_t));

    /* Inisialisasi console awal untuk debug */
    _init_console_awal();

    kernel_printf("[HAL-ARM64] Memulai inisialisasi HAL...\n");

    /* Inisialisasi GICv3 */
    kernel_printf("[HAL-ARM64] Inisialisasi GICv3...\n");
    _init_gicv3();

    /* Inisialisasi timer */
    kernel_printf("[HAL-ARM64] Inisialisasi Generic Timer...\n");
    g_hal_state_arm64.timer.frequency = FREKUENSI_TIMER;
    g_hal_state_arm64.timer.resolution_ns =
        (1000000000UL / FREKUENSI_TIMER);
    g_hal_state_arm64.timer.supports_one_shot = BENAR;
    g_hal_state_arm64.timer.supports_periodic = BENAR;

    _init_timer_arm64(FREKUENSI_TIMER);

    /* Set flag inisialisasi */
    g_hal_state_arm64.initialized = BENAR;
    g_hal_diinisalisasi = BENAR;

    kernel_printf("[HAL-ARM64] HAL siap\n");

    return STATUS_BERHASIL;
}

/*
 * hal_shutdown
 * ------------
 * Mematikan HAL.
 */
status_t hal_shutdown(void)
{
    if (!g_hal_diinisalisasi) {
        return STATUS_GAGAL;
    }

    kernel_printf("[HAL-ARM64] Mematikan HAL...\n");

    /* Disable interrupt */
    hal_cpu_disable_interrupts();

    g_hal_state_arm64.initialized = SALAH;
    g_hal_diinisalisasi = SALAH;

    return STATUS_BERHASIL;
}

/*
 * hal_get_state
 * -------------
 * Mendapatkan state HAL.
 */
const hal_state_t *hal_get_state(void)
{
    if (!g_hal_diinisalisasi) {
        return NULL;
    }

    return &g_hal_state_arm64;
}

/*
 * hal_is_initialized
 * ------------------
 * Cek apakah HAL sudah diinisialisasi.
 */
bool_t hal_is_initialized(void)
{
    return g_hal_diinisalisasi;
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
    __asm__ __volatile__("wfi");
}

/*
 * hal_cpu_reset
 * -------------
 * Reset CPU.
 */
void hal_cpu_reset(bool_t keras)
{
    tak_bertanda64_t val;

    /* Disable interrupt */
    hal_cpu_disable_interrupts();

    if (keras) {
        /* Hard reset via PSCI */
        /* PSCI SYSTEM_RESET */
        __asm__ __volatile__(
            "mov x0, #0x84000009\n\t"
            "hvc #0"
            :
            :
            : "x0", "memory"
        );
    } else {
        /* Soft reset via system register */
        __asm__ __volatile__(
            "mrs %0, CurrentEL\n\t"
            "cmp %0, #2\n\t"
            "b.ne 1f\n\t"
            "msr ELR_EL2, xzr\n\t"
            "eret\n"
            "1:\n\t"
            "msr ELR_EL1, xzr\n\t"
            "eret"
            : "=&r"(val)
            :
            : "memory", "cc"
        );
    }

    /* Tidak akan sampai sini */
    while (1) {
        __asm__ __volatile__("wfi");
    }
}

/*
 * hal_cpu_get_id
 * --------------
 * Mendapatkan ID CPU (MPIDR).
 */
tak_bertanda32_t hal_cpu_get_id(void)
{
    tak_bertanda64_t mpidr;

    __asm__ __volatile__(
        "mrs %0, MPIDR_EL1"
        : "=r"(mpidr)
    );

    return (tak_bertanda32_t)(mpidr & 0xFF);
}

/*
 * hal_cpu_get_info
 * ----------------
 * Mendapatkan informasi CPU.
 */
status_t hal_cpu_get_info(hal_cpu_info_t *info)
{
    tak_bertanda64_t midr;
    tak_bertanda64_t ctr;

    if (info == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Baca MIDR (Main ID Register) */
    __asm__ __volatile__(
        "mrs %0, MIDR_EL1"
        : "=r"(midr)
    );

    /* Baca CTR (Cache Type Register) */
    __asm__ __volatile__(
        "mrs %0, CTR_EL0"
        : "=r"(ctr)
    );

    /* Parse implementer */
    info->vendor_id = (tak_bertanda32_t)((midr >> 24) & 0xFF);
    info->family = (tak_bertanda32_t)((midr >> 16) & 0xF);
    info->model = (tak_bertanda32_t)((midr >> 4) & 0xFFF);
    info->stepping = (tak_bertanda32_t)(midr & 0xF);

    /* Cache info */
    info->cache_line_size = (tak_bertanda32_t)(4 << (ctr & 0xF));
    info->l1_icache_size = 0;
    info->l1_dcache_size = 0;

    return STATUS_BERHASIL;
}

/*
 * hal_cpu_enable_interrupts
 * -------------------------
 * Aktifkan interupsi.
 */
void hal_cpu_enable_interrupts(void)
{
    __asm__ __volatile__(
        "msr DAIFClr, #0xF\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * hal_cpu_disable_interrupts
 * --------------------------
 * Nonaktifkan interupsi.
 */
void hal_cpu_disable_interrupts(void)
{
    __asm__ __volatile__(
        "msr DAIFSet, #0xF\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * hal_cpu_save_interrupts
 * -----------------------
 * Simpan state interupsi.
 */
tak_bertanda64_t hal_cpu_save_interrupts(void)
{
    tak_bertanda64_t daif;

    __asm__ __volatile__(
        "mrs %0, DAIF\n\t"
        "msr DAIFSet, #0xF"
        : "=r"(daif)
        :
        : "memory"
    );

    return daif;
}

/*
 * hal_cpu_restore_interrupts
 * --------------------------
 * Restore state interupsi.
 */
void hal_cpu_restore_interrupts(tak_bertanda64_t state)
{
    __asm__ __volatile__(
        "msr DAIF, %0\n\t"
        "isb"
        :
        : "r"(state)
        : "memory"
    );
}

/*
 * hal_cpu_invalidate_tlb
 * ----------------------
 * Invalidate TLB untuk alamat.
 */
void hal_cpu_invalidate_tlb(void *addr)
{
    __asm__ __volatile__(
        "tlbi VAAE1, %0\n\t"
        "dsb sy\n\t"
        "isb"
        :
        : "r"((tak_bertanda64_t)addr >> 12)
        : "memory"
    );
}

/*
 * hal_cpu_invalidate_tlb_all
 * --------------------------
 * Invalidate semua TLB.
 */
void hal_cpu_invalidate_tlb_all(void)
{
    __asm__ __volatile__(
        "tlbi VMALLE1IS\n\t"
        "dsb ish\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * hal_cpu_cache_flush
 * -------------------
 * Flush cache.
 */
void hal_cpu_cache_flush(void)
{
    /* Data synchronization barrier */
    __asm__ __volatile__(
        "dsb sy\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * hal_cpu_cache_disable
 * ---------------------
 * Nonaktifkan cache.
 */
void hal_cpu_cache_disable(void)
{
    tak_bertanda64_t sctlr;

    __asm__ __volatile__(
        "mrs %0, SCTLR_EL1"
        : "=r"(sctlr)
    );

    /* Clear cache enable bits */
    sctlr &= ~((1ULL << 2) | (1ULL << 12));  /* C, I bits */

    __asm__ __volatile__(
        "msr SCTLR_EL1, %0\n\t"
        "isb"
        :
        : "r"(sctlr)
        : "memory"
    );
}

/*
 * hal_cpu_cache_enable
 * --------------------
 * Aktifkan cache.
 */
void hal_cpu_cache_enable(void)
{
    tak_bertanda64_t sctlr;

    __asm__ __volatile__(
        "mrs %0, SCTLR_EL1"
        : "=r"(sctlr)
    );

    /* Set cache enable bits */
    sctlr |= ((1ULL << 2) | (1ULL << 12));  /* C, I bits */

    __asm__ __volatile__(
        "msr SCTLR_EL1, %0\n\t"
        "isb"
        :
        : "r"(sctlr)
        : "memory"
    );
}

/*
 * hal_cpu_get_current_el
 * ----------------------
 * Mendapatkan current exception level.
 */
tak_bertanda32_t hal_cpu_get_current_el(void)
{
    tak_bertanda64_t currentel;

    __asm__ __volatile__(
        "mrs %0, CurrentEL"
        : "=r"(currentel)
    );

    return (tak_bertanda32_t)((currentel >> 2) & 0x3);
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
    tak_bertanda64_t timer_freq;
    tak_bertanda64_t timer_value;

    g_timer_ticks++;

    /* Update uptime setiap detik */
    if ((g_timer_ticks % FREKUENSI_TIMER) == 0) {
        g_timer_uptime++;
    }

    /* Acknowledge dan reload timer */
    __asm__ __volatile__(
        "mrs %0, CNTFRQ_EL0"
        : "=r"(timer_freq)
    );

    if (timer_freq > 0) {
        timer_value = timer_freq / (tak_bertanda64_t)FREKUENSI_TIMER;
    } else {
        timer_value = 1000000ULL;
    }

    __asm__ __volatile__(
        "msr CNTP_TVAL_EL0, %0\n\t"
        "isb"
        :
        : "r"(timer_value)
        : "memory"
    );

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

    /* Generic Timer sudah di-init di hal_init */
    g_hal_state_arm64.timer.frequency = FREKUENSI_TIMER;
    g_hal_state_arm64.timer.resolution_ns =
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
    return g_hal_state_arm64.timer.frequency;
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
        __asm__ __volatile__("wfi");
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
    tak_bertanda64_t delay_ticks;
    tak_bertanda64_t freq;

    /* Baca frekuensi counter */
    __asm__ __volatile__(
        "mrs %0, CNTFRQ_EL0"
        : "=r"(freq)
    );

    if (freq == 0) {
        freq = 1000000ULL;
    }

    /* Baca counter saat ini */
    __asm__ __volatile__(
        "mrs %0, CNTVCT_EL0"
        : "=r"(start)
    );

    delay_ticks = (freq * mikrodetik) / 1000000ULL;

    do {
        __asm__ __volatile__(
            "mrs %0, CNTVCT_EL0"
            : "=r"(end)
        );
    } while ((end - start) < delay_ticks);
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
    g_hal_state_arm64.interrupt.irq_count = JUMLAH_IRQ;
    g_hal_state_arm64.interrupt.vector_base = 0;
    g_hal_state_arm64.interrupt.has_apic = BENAR;  /* GIC */
    g_hal_state_arm64.interrupt.has_ioapic = BENAR;

    return STATUS_BERHASIL;
}

/*
 * hal_interrupt_enable
 * --------------------
 * Aktifkan IRQ.
 */
status_t hal_interrupt_enable(tak_bertanda32_t irq)
{
    volatile tak_bertanda32_t *gicd;
    tak_bertanda32_t reg_offset;
    tak_bertanda32_t bit_offset;

    if (irq >= JUMLAH_IRQ) {
        return STATUS_PARAM_INVALID;
    }

    gicd = (volatile tak_bertanda32_t *)GICD_BASE;

    /* Enable IRQ di distributor */
    reg_offset = irq / 32;
    bit_offset = irq % 32;

    gicd[0x40 + reg_offset] = (1 << bit_offset);

    return STATUS_BERHASIL;
}

/*
 * hal_interrupt_disable
 * ---------------------
 * Nonaktifkan IRQ.
 */
status_t hal_interrupt_disable(tak_bertanda32_t irq)
{
    volatile tak_bertanda32_t *gicd;
    tak_bertanda32_t reg_offset;
    tak_bertanda32_t bit_offset;
    tak_bertanda32_t val;

    if (irq >= JUMLAH_IRQ) {
        return STATUS_PARAM_INVALID;
    }

    gicd = (volatile tak_bertanda32_t *)GICD_BASE;

    /* Disable IRQ di distributor */
    reg_offset = irq / 32;
    bit_offset = irq % 32;

    val = gicd[0x40 + reg_offset];
    val &= ~(1 << bit_offset);
    gicd[0x40 + reg_offset] = val;

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
    tak_bertanda64_t irq_id;

    /* End of Interrupt via ICC_EOIR1_EL1 */
    irq_id = (tak_bertanda64_t)irq;

    __asm__ __volatile__(
        "msr ICC_EOIR1_EL1, %0\n\t"
        "isb"
        :
        : "r"(irq_id)
        : "memory"
    );
}

/*
 * hal_interrupt_set_handler
 * -------------------------
 * Set handler IRQ.
 */
status_t hal_interrupt_set_handler(tak_bertanda32_t irq,
                                    void (*handler)(void))
{
    if (irq >= JUMLAH_IRQ) {
        return STATUS_PARAM_INVALID;
    }

    /* Handler registration via exception vector */
    return STATUS_BERHASIL;
}

/*
 * hal_interrupt_get_handler
 * -------------------------
 * Dapatkan handler IRQ.
 */
void (*hal_interrupt_get_handler(tak_bertanda32_t irq))(void)
{
    if (irq >= JUMLAH_IRQ) {
        return NULL;
    }

    return NULL;
}

/*
 * hal_interrupt_get_irq
 * ---------------------
 * Baca IRQ yang pending.
 */
tak_bertanda32_t hal_interrupt_get_irq(void)
{
    tak_bertanda64_t irq;

    __asm__ __volatile__(
        "mrs %0, ICC_IAR1_EL1"
        : "=r"(irq)
    );

    return (tak_bertanda32_t)(irq & 0x3FF);
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
    g_boot_info_ptr = bootinfo;

    /* Default memory size akan diisi dari device tree atau bootloader */
    g_hal_state_arm64.memory.total_bytes = 256 * 1024 * 1024ULL; /* 256MB */
    g_hal_state_arm64.memory.available_bytes = 256 * 1024 * 1024ULL;

    g_hal_state_arm64.memory.page_size = UKURAN_HALAMAN;
    g_hal_state_arm64.memory.page_count =
        g_hal_state_arm64.memory.total_bytes / UKURAN_HALAMAN;

    /* ARM64 selalu mendukung 64-bit addressing */
    g_hal_state_arm64.memory.has_pae = BENAR;
    g_hal_state_arm64.memory.has_nx = BENAR;

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

    kernel_memcpy(info, &g_hal_state_arm64.memory,
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
    return 0;
}

/*
 * hal_memory_get_total_pages
 * --------------------------
 * Mendapatkan jumlah total halaman.
 */
tak_bertanda64_t hal_memory_get_total_pages(void)
{
    return g_hal_state_arm64.memory.page_count;
}

/*
 * hal_memory_alloc_page
 * ---------------------
 * Alokasikan satu halaman.
 */
alamat_fisik_t hal_memory_alloc_page(void)
{
    return ALAMAT_FISIK_INVALID;
}

/*
 * hal_memory_free_page
 * --------------------
 * Bebaskan satu halaman.
 */
status_t hal_memory_free_page(alamat_fisik_t addr)
{
    return STATUS_TIDAK_DUKUNG;
}

/*
 * hal_memory_alloc_pages
 * ----------------------
 * Alokasikan multiple halaman.
 */
alamat_fisik_t hal_memory_alloc_pages(tak_bertanda32_t count)
{
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
static tak_bertanda8_t g_warna_fg = 7;
static tak_bertanda8_t g_warna_bg = 0;

/*
 * hal_console_init
 * ----------------
 * Inisialisasi console.
 */
status_t hal_console_init(void)
{
    g_cursor_x = 0;
    g_cursor_y = 0;
    g_warna_fg = 7;
    g_warna_bg = 0;

    /* UART sudah di-init di _init_console_awal */
    return STATUS_BERHASIL;
}

/*
 * hal_console_putchar
 * -------------------
 * Tampilkan satu karakter.
 */
status_t hal_console_putchar(char c)
{
    if (c == '\n') {
        _uart_putchar('\r');
        _uart_putchar('\n');
        g_cursor_x = 0;
        g_cursor_y++;
    } else if (c == '\r') {
        _uart_putchar('\r');
        g_cursor_x = 0;
    } else if (c == '\t') {
        _uart_putchar('\t');
        g_cursor_x = (g_cursor_x + 4) & ~3;
    } else if (c == '\b') {
        _uart_putchar('\b');
        _uart_putchar(' ');
        _uart_putchar('\b');
        if (g_cursor_x > 0) {
            g_cursor_x--;
        }
    } else {
        _uart_putchar(c);
        g_cursor_x++;
    }

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
 * Bersihkan layar (terminal escape sequence).
 */
status_t hal_console_clear(void)
{
    /* ANSI escape sequence untuk clear screen */
    _uart_putchar('\033');
    _uart_putchar('[');
    _uart_putchar('2');
    _uart_putchar('J');
    _uart_putchar('\033');
    _uart_putchar('[');
    _uart_putchar('H');

    g_cursor_x = 0;
    g_cursor_y = 0;

    return STATUS_BERHASIL;
}

/*
 * hal_console_set_color
 * ---------------------
 * Set warna teks.
 */
status_t hal_console_set_color(tak_bertanda8_t fg, tak_bertanda8_t bg)
{
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
    (void)lines;

    /* Terminal scroll via escape sequence */
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
        *lebar = 80;
    }
    if (tinggi != NULL) {
        *tinggi = 25;
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
    /* ANSI red */
    _uart_putchar('\033');
    _uart_putchar('[');
    _uart_putchar('3');
    _uart_putchar('1');
    _uart_putchar('m');

    hal_console_puts(str);

    /* Reset */
    _uart_putchar('\033');
    _uart_putchar('[');
    _uart_putchar('0');
    _uart_putchar('m');
}

/*
 * hal_console_print_warning
 * -------------------------
 * Print pesan warning (kuning).
 */
void hal_console_print_warning(const char *str)
{
    /* ANSI yellow */
    _uart_putchar('\033');
    _uart_putchar('[');
    _uart_putchar('3');
    _uart_putchar('3');
    _uart_putchar('m');

    hal_console_puts(str);

    /* Reset */
    _uart_putchar('\033');
    _uart_putchar('[');
    _uart_putchar('0');
    _uart_putchar('m');
}
