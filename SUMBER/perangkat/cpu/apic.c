/*
 * PIGURA OS - APIC.C
 * ===================
 * Implementasi Advanced Programmable Interrupt Controller.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "cpu.h"

/*
 * ===========================================================================
 * KONSTANTA APIC
 * ===========================================================================
 */

/* APIC register offsets */
#define APIC_ID               0x0020
#define APIC_VERSION         0x0030
#define APIC_TPR             0x0080
#define APIC_APR             0x0090
#define APIC_PPR             0x00A0
#define APIC_EOI             0x00B0
#define APIC_LDR             0x00D0
#define APIC_DFR             0x00E0
#define APIC_SVR             0x00F0
#define APIC_ISR0            0x0100
#define APIC_TMR0            0x0180
#define APIC_IRR0            0x0200
#define APIC_ESR             0x0280
#define APIC_ICR0            0x0300
#define APIC_ICR1            0x0310
#define APIC_LVT_TIMER       0x0320
#define APIC_LVT_THERMAL     0x0330
#define APIC_LVT_PERF        0x0340
#define APIC_LVT_LINT0       0x0350
#define APIC_LVT_LINT1       0x0360
#define APIC_LVT_ERROR       0x0370
#define APIC_TIMER_INIT      0x0380
#define APIC_TIMER_CURRENT   0x0390
#define APIC_TIMER_DIV       0x03E0

/* APIC SVR flags */
#define APIC_SVR_ENABLE      0x0100
#define APIC_SVR_FOCUS       0x0200

/* APIC timer modes */
#define APIC_TIMER_MODE_ONESHOT    0x00000000
#define APIC_TIMER_MODE_PERIODIC   0x00020000
#define APIC_TIMER_MODE_TSC        0x00040000

/* APIC delivery modes */
#define APIC_DELIVERY_FIXED         0x00000000
#define APIC_DELIVERY_LOWEST        0x00000100
#define APIC_DELIVERY_SMI           0x00000200
#define APIC_DELIVERY_NMI           0x00000400
#define APIC_DELIVERY_INIT          0x00000500
#define APIC_DELIVERY_STARTUP       0x00000600

/* APIC trigger modes */
#define APIC_TRIGGER_EDGE          0x00000000
#define APIC_TRIGGER_LEVEL         0x00008000

/* APIC ICR flags */
#define APIC_ICR_LEVEL_ASSERT      0x00004000
#define APIC_ICR_LEVEL_DEASSERT    0x00000000
#define APIC_ICR_LEVEL_TRIGGER     0x00008000
#define APIC_ICR_BROADCAST_SELF    0x00040000
#define APIC_ICR_BROADCAST_ALL     0x00080000
#define APIC_ICR_BROADCAST_OTHERS  0x000C0000

/* IOAPIC registers */
#define IOAPIC_REGSEL        0x0000
#define IOAPIC_IOWIN         0x0010
#define IOAPIC_ID            0x0000
#define IOAPIC_VERSION       0x0001
#define IOAPIC_ARB           0x0002
#define IOAPIC_REDTBL_BASE   0x0010

/* Default addresses */
#define APIC_DEFAULT_BASE    0xFEE00000UL
#define IOAPIC_DEFAULT_BASE  0xFEC00000UL

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static alamat_virtual_t g_apic_base = 0;
static alamat_virtual_t g_ioapic_base = 0;
static bool_t g_apic_initialized = SALAH;
static bool_t g_x2apic_available = SALAH;

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * apic_read - Baca register APIC
 */
static tak_bertanda32_t apic_read(tak_bertanda32_t reg)
{
    volatile tak_bertanda32_t *addr;
    
    if (g_apic_base == 0) {
        return 0;
    }
    
    addr = (volatile tak_bertanda32_t *)(g_apic_base + reg);
    return *addr;
}

/*
 * apic_write - Tulis register APIC
 */
static void apic_write(tak_bertanda32_t reg, tak_bertanda32_t value)
{
    volatile tak_bertanda32_t *addr;
    
    if (g_apic_base == 0) {
        return;
    }
    
    addr = (volatile tak_bertanda32_t *)(g_apic_base + reg);
    *addr = value;
}

/*
 * ioapic_read - Baca register IOAPIC
 */
static tak_bertanda32_t ioapic_read(tak_bertanda32_t reg)
{
    volatile tak_bertanda32_t *addr;
    
    if (g_ioapic_base == 0) {
        return 0;
    }
    
    addr = (volatile tak_bertanda32_t *)g_ioapic_base;
    *addr = reg;
    
    return *(volatile tak_bertanda32_t *)(g_ioapic_base + IOAPIC_IOWIN);
}

/*
 * ioapic_write - Tulis register IOAPIC
 */
static void ioapic_write(tak_bertanda32_t reg, tak_bertanda32_t value)
{
    volatile tak_bertanda32_t *addr;
    
    if (g_ioapic_base == 0) {
        return;
    }
    
    addr = (volatile tak_bertanda32_t *)g_ioapic_base;
    *addr = reg;
    
    addr = (volatile tak_bertanda32_t *)(g_ioapic_base + IOAPIC_IOWIN);
    *addr = value;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * cpu_apic_init - Inisialisasi APIC
 */
status_t cpu_apic_init(void)
{
    tak_bertanda32_t eax, ebx, ecx, edx;
    tak_bertanda32_t apic_base_msr;
    tak_bertanda32_t apic_base_low;
    tak_bertanda32_t apic_base_high;
    
    if (g_apic_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    
    /* Cek apakah APIC tersedia */
    cpu_cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    
    if (!(edx & CPU_FEAT_APIC)) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    /* Cek x2APIC */
    if (ecx & (1 << 21)) {
        g_x2apic_available = BENAR;
    }
    
    /* Baca APIC base dari MSR */
    apic_base_msr = 0x1B;  /* IA32_APIC_BASE */
    
    cpu_msr_baca(apic_base_msr, &apic_base_low, &apic_base_high);
    
    /* Ekstrak base address */
    g_apic_base = ((alamat_virtual_t)apic_base_high << 32) |
                  (apic_base_low & 0xFFFFF000UL);
    
    /* Jika tidak ada base, gunakan default */
    if (g_apic_base == 0) {
        g_apic_base = APIC_DEFAULT_BASE;
    }
    
    /* Enable APIC via SVR */
    {
        tak_bertanda32_t svr;
        
        svr = apic_read(APIC_SVR);
        svr |= APIC_SVR_ENABLE;
        apic_write(APIC_SVR, svr);
    }
    
    /* Setup IOAPIC base */
    g_ioapic_base = IOAPIC_DEFAULT_BASE;
    
    g_apic_initialized = BENAR;
    
    return STATUS_BERHASIL;
    
#else
    return STATUS_TIDAK_DUKUNG;
#endif
}

/*
 * cpu_apic_id - Dapatkan APIC ID
 */
tak_bertanda32_t cpu_apic_id(void)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    if (!g_apic_initialized) {
        return 0;
    }
    
    return (apic_read(APIC_ID) >> 24) & 0xFF;
#else
    return 0;
#endif
}

/*
 * cpu_apic_enable - Enable APIC
 */
status_t cpu_apic_enable(void)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    tak_bertanda32_t svr;
    
    if (!g_apic_initialized) {
        return STATUS_GAGAL;
    }
    
    /* Set Spurious Interrupt Vector Register */
    svr = apic_read(APIC_SVR);
    svr |= APIC_SVR_ENABLE;
    svr &= ~APIC_SVR_FOCUS;  /* Disable focus processor */
    svr |= 0xFF;            /* Spurious vector */
    
    apic_write(APIC_SVR, svr);
    
    return STATUS_BERHASIL;
#else
    return STATUS_TIDAK_DUKUNG;
#endif
}

/*
 * apic_disable - Disable APIC
 */
status_t apic_disable(void)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    tak_bertanda32_t svr;
    
    if (!g_apic_initialized) {
        return STATUS_GAGAL;
    }
    
    svr = apic_read(APIC_SVR);
    svr &= ~APIC_SVR_ENABLE;
    apic_write(APIC_SVR, svr);
    
    return STATUS_BERHASIL;
#else
    return STATUS_TIDAK_DUKUNG;
#endif
}

/*
 * apic_eoi - Kirim End of Interrupt
 */
void apic_eoi(void)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    if (g_apic_initialized) {
        apic_write(APIC_EOI, 0);
    }
#endif
}

/*
 * apic_send_ipi - Kirim Inter-Processor Interrupt
 */
status_t apic_send_ipi(tak_bertanda32_t dest, tak_bertanda32_t vector,
                        tak_bertanda32_t mode)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    tak_bertanda32_t icr_low;
    tak_bertanda32_t icr_high;
    
    if (!g_apic_initialized) {
        return STATUS_GAGAL;
    }
    
    /* Set destination */
    icr_high = (dest & 0xFF) << 24;
    apic_write(APIC_ICR1, icr_high);
    
    /* Set vector dan mode */
    icr_low = vector | mode | APIC_ICR_LEVEL_ASSERT;
    apic_write(APIC_ICR0, icr_low);
    
    /* Wait for delivery */
    {
        tak_bertanda32_t timeout = 100000;
        
        while ((apic_read(APIC_ICR0) & APIC_ICR_LEVEL_ASSERT) && timeout > 0) {
            timeout--;
        }
        
        if (timeout == 0) {
            return STATUS_TIMEOUT;
        }
    }
    
    return STATUS_BERHASIL;
#else
    return STATUS_TIDAK_DUKUNG;
#endif
}

/*
 * apic_broadcast_ipi - Broadcast IPI ke semua CPU
 */
status_t apic_broadcast_ipi(tak_bertanda32_t vector, tak_bertanda32_t mode)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    tak_bertanda32_t icr_low;
    
    if (!g_apic_initialized) {
        return STATUS_GAGAL;
    }
    
    icr_low = vector | mode | APIC_ICR_LEVEL_ASSERT | APIC_ICR_BROADCAST_ALL;
    apic_write(APIC_ICR0, icr_low);
    
    return STATUS_BERHASIL;
#else
    return STATUS_TIDAK_DUKUNG;
#endif
}

/*
 * apic_set_timer - Set APIC timer
 */
status_t apic_set_timer(tak_bertanda32_t vector, tak_bertanda32_t count,
                         bool_t periodic)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    tak_bertanda32_t lvt;
    
    if (!g_apic_initialized) {
        return STATUS_GAGAL;
    }
    
    /* Set divide configuration */
    apic_write(APIC_TIMER_DIV, 0x0B);  /* Divide by 1 */
    
    /* Set initial count */
    apic_write(APIC_TIMER_INIT, count);
    
    /* Set LVT timer */
    lvt = vector;
    if (periodic) {
        lvt |= APIC_TIMER_MODE_PERIODIC;
    }
    
    apic_write(APIC_LVT_TIMER, lvt);
    
    return STATUS_BERHASIL;
#else
    return STATUS_TIDAK_DUKUNG;
#endif
}

/*
 * apic_stop_timer - Stop APIC timer
 */
void apic_stop_timer(void)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    if (g_apic_initialized) {
        apic_write(APIC_LVT_TIMER, 0x00010000);  /* Mask */
        apic_write(APIC_TIMER_INIT, 0);
    }
#endif
}

/*
 * apic_get_timer_count - Dapatkan counter timer
 */
tak_bertanda32_t apic_get_timer_count(void)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    if (g_apic_initialized) {
        return apic_read(APIC_TIMER_CURRENT);
    }
#endif
    return 0;
}

/*
 * apic_set_task_priority - Set Task Priority Register
 */
void apic_set_task_priority(tak_bertanda32_t priority)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    if (g_apic_initialized) {
        apic_write(APIC_TPR, priority & 0xFF);
    }
#endif
}

/*
 * apic_get_version - Dapatkan versi APIC
 */
tak_bertanda32_t apic_get_version(void)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    if (g_apic_initialized) {
        return apic_read(APIC_VERSION) & 0xFF;
    }
#endif
    return 0;
}

/*
 * ioapic_init - Inisialisasi IOAPIC
 */
status_t ioapic_init(void)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    tak_bertanda32_t id;
    tak_bertanda32_t version;
    tak_bertanda32_t i;
    tak_bertanda32_t max_irq;
    
    if (g_ioapic_base == 0) {
        g_ioapic_base = IOAPIC_DEFAULT_BASE;
    }
    
    /* Baca ID dan version */
    id = ioapic_read(IOAPIC_ID);
    version = ioapic_read(IOAPIC_VERSION);
    
    /* Hitung jumlah IRQ */
    max_irq = ((version >> 16) & 0xFF) + 1;
    
    /* Mask semua IRQ */
    for (i = 0; i < max_irq; i++) {
        tak_bertanda32_t reg = IOAPIC_REDTBL_BASE + (i * 2);
        ioapic_write(reg, 0x00010000);     /* Mask, vector 0 */
        ioapic_write(reg + 1, 0);          /* Delivery to CPU 0 */
    }
    
    return STATUS_BERHASIL;
#else
    return STATUS_TIDAK_DUKUNG;
#endif
}

/*
 * ioapic_set_irq - Set routing IRQ
 */
status_t ioapic_set_irq(tak_bertanda32_t irq, tak_bertanda32_t vector,
                         tak_bertanda32_t cpu, bool_t enabled)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    tak_bertanda32_t reg;
    tak_bertanda32_t low;
    tak_bertanda32_t high;
    
    if (g_ioapic_base == 0) {
        return STATUS_GAGAL;
    }
    
    reg = IOAPIC_REDTBL_BASE + (irq * 2);
    
    /* Low dword: vector, delivery mode, dst */
    low = vector;
    low |= APIC_DELIVERY_FIXED;
    low |= APIC_TRIGGER_EDGE;
    
    if (!enabled) {
        low |= 0x00010000;  /* Mask */
    }
    
    /* High dword: destination */
    high = (cpu & 0xFF) << 24;
    
    ioapic_write(reg, low);
    ioapic_write(reg + 1, high);
    
    return STATUS_BERHASIL;
#else
    return STATUS_TIDAK_DUKUNG;
#endif
}

/*
 * ioapic_mask_irq - Mask IRQ di IOAPIC
 */
status_t ioapic_mask_irq(tak_bertanda32_t irq)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    tak_bertanda32_t reg;
    tak_bertanda32_t low;
    
    if (g_ioapic_base == 0) {
        return STATUS_GAGAL;
    }
    
    reg = IOAPIC_REDTBL_BASE + (irq * 2);
    low = ioapic_read(reg);
    low |= 0x00010000;  /* Set mask bit */
    ioapic_write(reg, low);
    
    return STATUS_BERHASIL;
#else
    return STATUS_TIDAK_DUKUNG;
#endif
}

/*
 * ioapic_unmask_irq - Unmask IRQ di IOAPIC
 */
status_t ioapic_unmask_irq(tak_bertanda32_t irq)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    tak_bertanda32_t reg;
    tak_bertanda32_t low;
    
    if (g_ioapic_base == 0) {
        return STATUS_GAGAL;
    }
    
    reg = IOAPIC_REDTBL_BASE + (irq * 2);
    low = ioapic_read(reg);
    low &= ~0x00010000UL;  /* Clear mask bit */
    ioapic_write(reg, low);
    
    return STATUS_BERHASIL;
#else
    return STATUS_TIDAK_DUKUNG;
#endif
}
