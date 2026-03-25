/*
 * PIGURA OS - HAL_x86.C
 * ----------------------
 * Implementasi arsitektur-spesifik untuk Hardware Abstraction Layer x86.
 *
 * Berkas ini berisi fungsi-fungsi inisialisasi dan helper yang spesifik
 * untuk arsitektur x86/x86_64. Fungsi-fungsi HAL utama diimplementasikan
 * di SUMBER/inti/hal/.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 *
 * Arsitektur: x86, x86_64
 * Versi: 2.0
 * Tanggal: 2025
 */

#include "../../inti/kernel.h"
#include "../../inti/hal/hal.h"
#include "cpu_x86.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* PIC (Programmable Interrupt Controller) */
#define PIC1_COMMAND            0x20
#define PIC1_DATA               0x21
#define PIC2_COMMAND            0xA0
#define PIC2_DATA               0xA1

/* PIC command bits */
#define PIC_CMD_ICW1_INIT       0x10    /* Initialization */
#define PIC_CMD_ICW1_ICW4       0x01    /* ICW4 needed */
#define PIC_CMD_ICW4_8086       0x01    /* 8086 mode */

/* PIC EOI */
#define PIC_CMD_EOI             0x20

/* PIT (Programmable Interval Timer) */
#define PIT_CHANNEL_0           0x40
#define PIT_CHANNEL_1           0x41
#define PIT_CHANNEL_2           0x42
#define PIT_COMMAND             0x43

/* PIT frequency */
#define PIT_FREQUENCY           1193182UL

/* PIT command bits */
#define PIT_CMD_BINARY          0x00
#define PIT_CMD_MODE_3          0x06    /* Square wave */
#define PIT_CMD_BOTH            0x30    /* LSB then MSB */

/* VGA */
#define VGA_BUFFER              0xB8000
#define VGA_KOLOM               80
#define VGA_BARIS               25

/* VGA I/O ports */
#define PORT_VGA_CRTC_INDEX     0x3D4
#define PORT_VGA_CRTC_DATA      0x3D5

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Flag untuk status inisialisasi arsitektur */
static bool_t g_arch_initialized = SALAH;

/* Pointer ke boot info */
static void *g_boot_info_ptr = NULL;

/*
 * ============================================================================
 * FUNGSI INTERNAL PIC (INTERNAL PIC FUNCTIONS)
 * ============================================================================
 */

/*
 * _pic_remap
 * ----------
 * Remap PIC untuk menghindari konflik vektor dengan exception.
 * IRQ 0-7 dipetakan ke vektor 32-39
 * IRQ 8-15 dipetakan ke vektor 40-47
 */
static void _pic_remap(void)
{
    /* Mulai inisialisasi PIC */
    outb(PIC1_COMMAND, PIC_CMD_ICW1_INIT | PIC_CMD_ICW1_ICW4);
    io_delay();
    outb(PIC2_COMMAND, PIC_CMD_ICW1_INIT | PIC_CMD_ICW1_ICW4);
    io_delay();

    /* Set vektor offset */
    outb(PIC1_DATA, VEKTOR_IRQ_MULAI);         /* Master: vektor 32-39 */
    io_delay();
    outb(PIC1_DATA, VEKTOR_IRQ_MULAI + 8);     /* Slave: vektor 40-47 */
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

    /* Mask semua IRQ kecuali cascade (IRQ2) */
    outb(PIC1_DATA, 0xFB);   /* Unmask IRQ2 (cascade) */
    outb(PIC2_DATA, 0xFF);   /* Mask semua IRQ slave */
}

/*
 * _pic_mask_irq
 * -------------
 * Mask IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ (0-15)
 */
static void _pic_mask_irq(tak_bertanda32_t irq)
{
    tak_bertanda8_t mask;
    tak_bertanda16_t port;

    if (irq > 15) {
        return;
    }

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    mask = inb((tak_bertanda16_t)port);
    mask |= (tak_bertanda8_t)(1 << irq);
    outb((tak_bertanda16_t)port, mask);
}

/*
 * _pic_unmask_irq
 * ---------------
 * Unmask IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ (0-15)
 */
static void _pic_unmask_irq(tak_bertanda32_t irq)
{
    tak_bertanda8_t mask;
    tak_bertanda16_t port;

    if (irq > 15) {
        return;
    }

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    mask = inb((tak_bertanda16_t)port);
    mask &= (tak_bertanda8_t)(~(1 << irq));
    outb((tak_bertanda16_t)port, mask);
}

/*
 * ============================================================================
 * FUNGSI INTERNAL PIT (INTERNAL PIT FUNCTIONS)
 * ============================================================================
 */

/*
 * _pit_init
 * ---------
 * Inisialisasi PIT dengan frekuensi tertentu.
 *
 * Parameter:
 *   frequency - Frekuensi yang diinginkan dalam Hz
 */
static void _pit_init(tak_bertanda32_t frekuensi)
{
    tak_bertanda16_t divisor;

    /* Hitung divisor */
    divisor = (tak_bertanda16_t)(PIT_FREQUENCY / frekuensi);

    /* Pastikan divisor tidak nol */
    if (divisor == 0) {
        divisor = 1;
    }

    /* Set mode: channel 0, mode 3 (square wave), binary, both bytes */
    outb(PIT_COMMAND, PIT_CMD_BINARY | PIT_CMD_MODE_3 | PIT_CMD_BOTH);

    /* Kirim divisor (LSB dulu, lalu MSB) */
    outb(PIT_CHANNEL_0, (tak_bertanda8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL_0, (tak_bertanda8_t)((divisor >> 8) & 0xFF));
}

/*
 * ============================================================================
 * FUNGSI INTERNAL VGA (INTERNAL VGA FUNCTIONS)
 * ============================================================================
 */

/*
 * _vga_clear
 * ----------
 * Bersihkan layar VGA text mode.
 */
static void _vga_clear(void)
{
    volatile tak_bertanda16_t *vga;
    tak_bertanda32_t i;

    vga = (volatile tak_bertanda16_t *)VGA_BUFFER;

    for (i = 0; i < VGA_KOLOM * VGA_BARIS; i++) {
        vga[i] = (tak_bertanda16_t)(' ' | (VGA_ABU_ABU << 8));
    }
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - INISIALISASI ARSITEKTUR
 * ============================================================================
 */

/*
 * hal_arch_init
 * -------------
 * Inisialisasi arsitektur x86.
 * Fungsi ini dipanggil oleh hal_init() di inti/hal/hal.c
 *
 * Return: Status operasi
 */
status_t hal_arch_init(void)
{
    if (g_arch_initialized) {
        return STATUS_SUDAH_ADA;
    }

    /* Clear VGA screen untuk output awal */
    _vga_clear();

    /* Remap PIC */
    _pic_remap();

    /* Inisialisasi PIT */
    _pit_init(FREKUENSI_TIMER);

    g_arch_initialized = BENAR;

    return STATUS_BERHASIL;
}

/*
 * hal_arch_shutdown
 * -----------------
 * Shutdown arsitektur x86.
 *
 * Return: Status operasi
 */
status_t hal_arch_shutdown(void)
{
    if (!g_arch_initialized) {
        return STATUS_GAGAL;
    }

    /* Disable semua interrupt */
    cpu_disable_irq();

    g_arch_initialized = SALAH;

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - OPERASI PIC
 * ============================================================================
 */

/*
 * hal_arch_pic_enable_irq
 * -----------------------
 * Aktifkan IRQ melalui PIC.
 *
 * Parameter:
 *   irq - Nomor IRQ (0-15)
 *
 * Return: Status operasi
 */
status_t hal_arch_pic_enable_irq(tak_bertanda32_t irq)
{
    if (irq > 15) {
        return STATUS_PARAM_INVALID;
    }

    _pic_unmask_irq(irq);
    return STATUS_BERHASIL;
}

/*
 * hal_arch_pic_disable_irq
 * ------------------------
 * Nonaktifkan IRQ melalui PIC.
 *
 * Parameter:
 *   irq - Nomor IRQ (0-15)
 *
 * Return: Status operasi
 */
status_t hal_arch_pic_disable_irq(tak_bertanda32_t irq)
{
    if (irq > 15) {
        return STATUS_PARAM_INVALID;
    }

    _pic_mask_irq(irq);
    return STATUS_BERHASIL;
}

/*
 * hal_arch_pic_eoi
 * ----------------
 * Kirim End of Interrupt ke PIC.
 *
 * Parameter:
 *   irq - Nomor IRQ (0-15)
 */
void hal_arch_pic_eoi(tak_bertanda32_t irq)
{
    if (irq >= 8) {
        /* Ack slave PIC */
        outb(PIC2_COMMAND, PIC_CMD_EOI);
    }

    /* Ack master PIC */
    outb(PIC1_COMMAND, PIC_CMD_EOI);
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - DETEKSI HARDWARE
 * ============================================================================
 */

/*
 * hal_arch_detect_cpu_features
 * ----------------------------
 * Deteksi fitur CPU x86.
 *
 * Parameter:
 *   info - Pointer ke struktur hal_cpu_info_t
 *
 * Return: Status operasi
 */
status_t hal_arch_detect_cpu_features(hal_cpu_info_t *info)
{
    tak_bertanda32_t eax, ebx, ecx, edx;
    tak_bertanda32_t i;

    if (info == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Cek apakah CPUID tersedia */
    /* Untuk x86, kita asumsikan CPUID selalu tersedia pada CPU modern */

    /* Dapatkan vendor string (leaf 0) */
    cpu_cpuid(0, 0, &eax, &ebx, &ecx, &edx);

    /* Vendor string disimpan dalam EBX, EDX, ECX */
    *((tak_bertanda32_t *)(info->vendor + 0)) = ebx;
    *((tak_bertanda32_t *)(info->vendor + 4)) = edx;
    *((tak_bertanda32_t *)(info->vendor + 8)) = ecx;
    info->vendor[12] = '\0';

    /* Dapatkan brand string (leaf 0x80000002-0x80000004) */
    if (eax >= 0x80000004) {
        for (i = 0; i < 3; i++) {
            cpu_cpuid(0x80000002 + i, 0, &eax, &ebx, &ecx, &edx);

            *((tak_bertanda32_t *)(info->brand + (i * 16) + 0)) = eax;
            *((tak_bertanda32_t *)(info->brand + (i * 16) + 4)) = ebx;
            *((tak_bertanda32_t *)(info->brand + (i * 16) + 8)) = ecx;
            *((tak_bertanda32_t *)(info->brand + (i * 16) + 12)) = edx;
        }
        info->brand[48] = '\0';
    } else {
        info->brand[0] = '\0';
    }

    /* Dapatkan feature flags (leaf 1) */
    if (eax >= 1) {
        cpu_cpuid(1, 0, &eax, &ebx, &ecx, &edx);

        info->features = edx;
        info->features_ext = ecx;

        /* Family, model, stepping */
        info->stepping = eax & 0xF;
        info->model = (eax >> 4) & 0xF;
        info->family = (eax >> 8) & 0xF;

        /* Extended family dan model */
        if (info->family == 0xF) {
            info->family += (eax >> 20) & 0xFF;
            info->model += ((eax >> 16) & 0xF) << 4;
        }

        /* Cache line size */
        info->cache_line = (ebx >> 8) & 0xFF;
    }

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - UTILITAS ARSITEKTUR
 * ============================================================================
 */

/*
 * hal_arch_get_cr2
 * ----------------
 * Dapatkan nilai CR2 (alamat page fault).
 *
 * Return: Nilai CR2
 */
tak_bertanda32_t hal_arch_get_cr2(void)
{
    return cpu_read_cr2();
}

/*
 * hal_arch_get_cr3
 * ----------------
 * Dapatkan nilai CR3 (page directory base).
 *
 * Return: Nilai CR3
 */
tak_bertanda32_t hal_arch_get_cr3(void)
{
    return cpu_read_cr3();
}

/*
 * hal_arch_set_cr3
 * ----------------
 * Set nilai CR3 (switch page directory).
 *
 * Parameter:
 *   value - Alamat fisik page directory
 */
void hal_arch_set_cr3(tak_bertanda32_t value)
{
    cpu_write_cr3(value);
}

/*
 * hal_arch_invlpg
 * ---------------
 * Invalidate TLB entry untuk satu halaman.
 *
 * Parameter:
 *   addr - Alamat virtual
 */
void hal_arch_invlpg(void *addr)
{
    cpu_invlpg(addr);
}

/*
 * hal_arch_read_tsc
 * -----------------
 * Baca Time Stamp Counter.
 *
 * Return: Nilai TSC
 */
tak_bertanda64_t hal_arch_read_tsc(void)
{
    return cpu_read_tsc();
}

/*
 * hal_arch_cpuid
 * --------------
 * Jalankan instruksi CPUID.
 *
 * Parameter:
 *   leaf    - EAX input
 *   subleaf - ECX input
 *   eax     - Pointer untuk hasil EAX
 *   ebx     - Pointer untuk hasil EBX
 *   ecx     - Pointer untuk hasil ECX
 *   edx     - Pointer untuk hasil EDX
 */
void hal_arch_cpuid(tak_bertanda32_t leaf, tak_bertanda32_t subleaf,
                    tak_bertanda32_t *eax, tak_bertanda32_t *ebx,
                    tak_bertanda32_t *ecx, tak_bertanda32_t *edx)
{
    cpu_cpuid(leaf, subleaf, eax, ebx, ecx, edx);
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - RESET DAN SHUTDOWN
 * ============================================================================
 */

/*
 * hal_arch_reset
 * --------------
 * Reset sistem x86.
 *
 * Parameter:
 *   hard - Jika BENAR, lakukan hard reset
 */
void hal_arch_reset(bool_t hard)
{
    tak_bertanda8_t temp;

    /* Disable interrupt */
    cpu_disable_irq();

    if (hard) {
        /* Triple fault untuk hard reset */
        struct {
            tak_bertanda16_t limit;
            tak_bertanda32_t base;
        } __attribute__((packed)) null_idt = {0, 0};

        __asm__ __volatile__(
            "lidt %0\n\t"
            "int $3"
            :
            : "m"(null_idt)
        );
    } else {
        /* Keyboard controller reset */
        /* Clear keyboard buffer */
        do {
            temp = inb(0x64);
            if (temp & 0x01) {
                (void)inb(0x60);
            }
        } while (temp & 0x02);

        /* Pulse reset line */
        outb(0x64, 0xFE);
    }

    /* Tidak akan sampai sini */
    for (;;) {
        cpu_halt();
    }
}

/*
 * hal_arch_halt
 * -------------
 * Hentikan CPU.
 */
void hal_arch_halt(void)
{
    cpu_halt();
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - MEMORY BARRIER
 * ============================================================================
 */

/*
 * hal_arch_memory_barrier
 * -----------------------
 * Memory barrier.
 */
void hal_arch_memory_barrier(void)
{
    cpu_memory_barrier();
}

/*
 * hal_arch_sfence
 * ---------------
 * Store fence.
 */
void hal_arch_sfence(void)
{
    cpu_sfence();
}

/*
 * hal_arch_lfence
 * ---------------
 * Load fence.
 */
void hal_arch_lfence(void)
{
    cpu_lfence();
}

/*
 * hal_arch_mfence
 * ---------------
 * Memory fence.
 */
void hal_arch_mfence(void)
{
    cpu_mfence();
}
