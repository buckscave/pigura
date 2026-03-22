/*
 * PIGURA OS - APIC.C
 * ------------------
 * Implementasi driver APIC (Advanced Programmable Interrupt Controller).
 *
 * Berkas ini berisi implementasi driver untuk Local APIC dan I/O APIC
 * yang merupakan interrupt controller modern pada sistem x86.
 *
 * Versi: 1.1
 * Tanggal: 2025
 */

#include "../kernel.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* Local APIC MMIO base address */
#define LAPIC_BASE_DEFAULT      0xFEE00000UL
#define LAPIC_BASE_MSR          0x1B

/* Local APIC register offsets */
#define LAPIC_ID                0x020   /* Local APIC ID */
#define LAPIC_VERSION           0x030   /* Local APIC Version */
#define LAPIC_TPR               0x080   /* Task Priority Register */
#define LAPIC_APR               0x090   /* Arbitration Priority */
#define LAPIC_PPR               0x0A0   /* Processor Priority */
#define LAPIC_EOI               0x0B0   /* End of Interrupt */
#define LAPIC_LDR               0x0D0   /* Logical Destination */
#define LAPIC_DFR               0x0E0   /* Destination Format */
#define LAPIC_SVR               0x0F0   /* Spurious Interrupt Vector */
#define LAPIC_ISR               0x100   /* In-Service Register (8x32bit) */
#define LAPIC_TMR               0x180   /* Trigger Mode Register */
#define LAPIC_IRR               0x200   /* Interrupt Request Register */
#define LAPIC_ESR               0x280   /* Error Status Register */
#define LAPIC_ICR_LOW           0x300   /* Interrupt Command (low) */
#define LAPIC_ICR_HIGH          0x310   /* Interrupt Command (high) */
#define LAPIC_LVT_TIMER         0x320   /* Local Vector Table - Timer */
#define LAPIC_LVT_THERMAL       0x330   /* Local Vector Table - Thermal */
#define LAPIC_LVT_PERF          0x340   /* Local Vector Table - Perf */
#define LAPIC_LVT_LINT0         0x350   /* Local Vector Table - LINT0 */
#define LAPIC_LVT_LINT1         0x360   /* Local Vector Table - LINT1 */
#define LAPIC_LVT_ERROR         0x370   /* Local Vector Table - Error */
#define LAPIC_TIMER_ICR         0x380   /* Timer Initial Count */
#define LAPIC_TIMER_CCR         0x390   /* Timer Current Count */
#define LAPIC_TIMER_DCR         0x3E0   /* Timer Divide Configuration */

/* I/O APIC base address */
#define IOAPIC_BASE_DEFAULT     0xFEC00000UL

/* I/O APIC register offsets (indirect access) */
#define IOAPIC_REGSEL           0x00    /* Register Select */
#define IOAPIC_IOWIN            0x10    /* I/O Window */

/* I/O APIC registers */
#define IOAPIC_ID               0x00    /* IOAPIC ID */
#define IOAPIC_VER              0x01    /* IOAPIC Version */
#define IOAPIC_ARB              0x02    /* Arbitration ID */
#define IOAPIC_REDTBL_BASE      0x10    /* Redirection Table base */

/* APIC flags */
#define LAPIC_ENABLE            0x00000100UL
#define LAPIC_FOCUS_DISABLE     0x00000200UL
#define LAPIC_SVR_ENABLE        0x00000100UL

/* Delivery modes */
#define APIC_DM_FIXED           0x000   /* Fixed delivery */
#define APIC_DM_LOWEST          0x100   /* Lowest priority */
#define APIC_DM_SMI             0x200   /* System Management Intr */
#define APIC_DM_NMI             0x400   /* Non-Maskable Interrupt */
#define APIC_DM_INIT            0x500   /* INIT */
#define APIC_DM_EXTINT          0x700   /* External Interrupt */

/* Trigger modes */
#define APIC_TRIG_EDGE          0x000   /* Edge triggered */
#define APIC_TRIG_LEVEL         0x8000  /* Level triggered */

/* Polarity */
#define APIC_POLARITY_HIGH      0x0000  /* Active high */
#define APIC_POLARITY_LOW       0x2000  /* Active low */

/* I/O APIC redirection entry flags */
#define IOAPIC_INT_MASK         0x10000 /* Interrupt mask */
#define IOAPIC_INT_ACTIVE       0x00000 /* Active high */
#define IOAPIC_INT_DEASSERT     0x40000 /* Deassert */

/* Maximum I/O APICs */
#define MAX_IOAPIC              8

/* Maximum redirection entries per I/O APIC */
#define MAX_REDIRECTION         24

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* I/O APIC descriptor */
typedef struct {
    tak_bertanda32_t id;            /* IOAPIC ID */
    alamat_virtual_t base;          /* Base address (pointer-sized) */
    tak_bertanda32_t gsi_base;      /* Global System Interrupt base */
    tak_bertanda32_t gsi_count;     /* Number of GSI */
    bool_t present;                 /* Is present */
} ioapic_t;

/* APIC state */
typedef struct {
    bool_t lapic_present;
    bool_t ioapic_present;
    alamat_virtual_t lapic_base;    /* Base address (pointer-sized) */
    tak_bertanda32_t lapic_id;
    tak_bertanda32_t lapic_version;
    tak_bertanda32_t cpu_count;
    ioapic_t ioapic[MAX_IOAPIC];
    tak_bertanda32_t ioapic_count;
    tak_bertanda32_t spurious_count;
} apic_state_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* APIC state */
static apic_state_t apic = {0};

/* Initialization flag */
static bool_t apic_initialized = SALAH;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * lapic_read
 * ----------
 * Baca register Local APIC.
 *
 * Parameter:
 *   reg - Register offset
 *
 * Return: Nilai register
 */
static inline tak_bertanda32_t lapic_read(tak_bertanda32_t reg)
{
    volatile tak_bertanda32_t *ptr;
    ptr = (volatile tak_bertanda32_t *)(apic.lapic_base + reg);
    return *ptr;
}

/*
 * lapic_write
 * -----------
 * Tulis register Local APIC.
 *
 * Parameter:
 *   reg   - Register offset
 *   value - Nilai yang ditulis
 */
static inline void lapic_write(tak_bertanda32_t reg, tak_bertanda32_t value)
{
    volatile tak_bertanda32_t *ptr;
    ptr = (volatile tak_bertanda32_t *)(apic.lapic_base + reg);
    *ptr = value;
}

/*
 * ioapic_read
 * -----------
 * Baca register I/O APIC.
 *
 * Parameter:
 *   ioapic_id - I/O APIC ID
 *   reg       - Register number
 *
 * Return: Nilai register
 */
static tak_bertanda32_t ioapic_read(tak_bertanda32_t ioapic_id,
                                    tak_bertanda32_t reg)
{
    volatile tak_bertanda32_t *base;

    if (ioapic_id >= MAX_IOAPIC || !apic.ioapic[ioapic_id].present) {
        return 0;
    }

    base = (volatile tak_bertanda32_t *)apic.ioapic[ioapic_id].base;

    base[IOAPIC_REGSEL / 4] = reg;
    return base[IOAPIC_IOWIN / 4];
}

/*
 * ioapic_write
 * ------------
 * Tulis register I/O APIC.
 *
 * Parameter:
 *   ioapic_id - I/O APIC ID
 *   reg       - Register number
 *   value     - Nilai yang ditulis
 */
static void ioapic_write(tak_bertanda32_t ioapic_id, tak_bertanda32_t reg,
                         tak_bertanda32_t value)
{
    volatile tak_bertanda32_t *base;

    if (ioapic_id >= MAX_IOAPIC || !apic.ioapic[ioapic_id].present) {
        return;
    }

    base = (volatile tak_bertanda32_t *)apic.ioapic[ioapic_id].base;

    base[IOAPIC_REGSEL / 4] = reg;
    base[IOAPIC_IOWIN / 4] = value;
}

/*
 * detect_local_apic
 * -----------------
 * Deteksi keberadaan Local APIC melalui CPUID.
 *
 * Return: BENAR jika Local APIC ada
 */
static bool_t detect_local_apic(void)
{
    tak_bertanda32_t eax, ebx, ecx, edx;

    /* CPUID function 1 */
    __asm__ __volatile__(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1)
    );

    /* EDX bit 9 = APIC */
    return (edx & (1 << 9)) ? BENAR : SALAH;
}

/*
 * get_lapic_base
 * --------------
 * Dapatkan alamat base Local APIC dari MSR.
 *
 * Return: Alamat base Local APIC
 */
static alamat_virtual_t get_lapic_base(void)
{
    tak_bertanda32_t low, high;

    __asm__ __volatile__(
        "rdmsr"
        : "=a"(low), "=d"(high)
        : "c"(LAPIC_BASE_MSR)
    );

    return (alamat_virtual_t)(low & 0xFFFFF000UL);
}

/*
 * set_lapic_base
 * --------------
 * Set alamat base Local APIC melalui MSR.
 *
 * Parameter:
 *   base - Alamat base baru
 */
static void set_lapic_base(alamat_virtual_t base)
{
    tak_bertanda32_t low = ((tak_bertanda32_t)base & 0xFFFFF000UL) | 0x800;

    __asm__ __volatile__(
        "wrmsr"
        :
        : "a"(low), "d"(0), "c"(LAPIC_BASE_MSR)
    );
}

/*
 * detect_ioapic
 * -------------
 * Deteksi I/O APIC dari ACPI MADT table.
 * Untuk sekarang, gunakan default address.
 */
static void detect_ioapic(void)
{
    /* Default I/O APIC at 0xFEC00000 */
    apic.ioapic[0].id = 0;
    apic.ioapic[0].base = (alamat_virtual_t)IOAPIC_BASE_DEFAULT;
    apic.ioapic[0].gsi_base = 0;
    apic.ioapic[0].present = BENAR;

    /* Read version untuk mendapatkan jumlah entries */
    tak_bertanda32_t ver = ioapic_read(0, IOAPIC_VER);
    apic.ioapic[0].gsi_count = ((ver >> 16) & 0xFF) + 1;

    if (apic.ioapic[0].gsi_count > MAX_REDIRECTION) {
        apic.ioapic[0].gsi_count = MAX_REDIRECTION;
    }

    apic.ioapic_count = 1;
    apic.ioapic_present = BENAR;
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * apic_init
 * ---------
 * Inisialisasi APIC.
 *
 * Return: Status operasi
 */
status_t apic_init(void)
{
    tak_bertanda32_t i;

    if (apic_initialized) {
        return STATUS_SUDAH_ADA;
    }

    kernel_printf("[APIC] Initializing APIC subsystem...\n");

    /* Detect Local APIC */
    if (!detect_local_apic()) {
        kernel_printf("[APIC] Local APIC not detected, using PIC\n");
        apic.lapic_present = SALAH;
        return STATUS_BERHASIL;  /* OK, akan gunakan PIC */
    }

    apic.lapic_present = BENAR;

    /* Get and set LAPIC base */
    apic.lapic_base = get_lapic_base();

    if (apic.lapic_base == 0) {
        apic.lapic_base = (alamat_virtual_t)LAPIC_BASE_DEFAULT;
        set_lapic_base(apic.lapic_base | 0x800);  /* Enable */
    }

    kernel_printf("[APIC] Local APIC base: 0x%lX\n",
                  (ukuran_t)apic.lapic_base);

    /* Get LAPIC ID and version */
    apic.lapic_id = lapic_read(LAPIC_ID) >> 24;
    apic.lapic_version = lapic_read(LAPIC_VERSION) & 0xFF;

    kernel_printf("[APIC] Local APIC ID: %lu, Version: %lu\n",
                  apic.lapic_id, apic.lapic_version);

    /* Enable Local APIC */
    lapic_write(LAPIC_SVR, LAPIC_SVR_ENABLE | 0xFF);  /* Spurious vector */

    /* Set Task Priority to accept all interrupts */
    lapic_write(LAPIC_TPR, 0);

    /* Clear any pending interrupts */
    lapic_write(LAPIC_EOI, 0);

    /* Setup timer (one-shot mode, vector) */
    lapic_write(LAPIC_LVT_TIMER, VEKTOR_TIMER | 0x20000);  /* Masked */
    lapic_write(LAPIC_TIMER_DCR, 0x0B);  /* Divide by 1 */

    /* Setup error interrupt */
    lapic_write(LAPIC_LVT_ERROR, 0xFE);  /* Error vector */

    /* Detect I/O APIC */
    detect_ioapic();

    if (apic.ioapic_present) {
        kernel_printf("[APIC] I/O APIC found at 0x%lX (%lu GSIs)\n",
                      (ukuran_t)apic.ioapic[0].base,
                      apic.ioapic[0].gsi_count);
    }

    apic.cpu_count = 1;  /* TODO: Detect dari ACPI */
    apic.spurious_count = 0;
    apic_initialized = BENAR;

    kernel_printf("[APIC] APIC initialized successfully\n");

    return STATUS_BERHASIL;
}

/*
 * lapic_send_eoi
 * --------------
 * Kirim End of Interrupt ke Local APIC.
 */
void lapic_send_eoi(void)
{
    lapic_write(LAPIC_EOI, 0);
}

/*
 * lapic_get_id
 * ------------
 * Dapatkan Local APIC ID.
 *
 * Return: LAPIC ID
 */
tak_bertanda32_t lapic_get_id(void)
{
    return lapic_read(LAPIC_ID) >> 24;
}

/*
 * lapic_set_tpr
 * -------------
 * Set Task Priority Register.
 *
 * Parameter:
 *   priority - Priority value (0-255)
 */
void lapic_set_tpr(tak_bertanda32_t priority)
{
    lapic_write(LAPIC_TPR, priority & 0xFF);
}

/*
 * lapic_get_tpr
 * -------------
 * Dapatkan Task Priority Register.
 *
 * Return: TPR value
 */
tak_bertanda32_t lapic_get_tpr(void)
{
    return lapic_read(LAPIC_TPR) & 0xFF;
}

/*
 * lapic_enable
 * ------------
 * Aktifkan Local APIC.
 */
void lapic_enable(void)
{
    tak_bertanda32_t svr = lapic_read(LAPIC_SVR);
    lapic_write(LAPIC_SVR, svr | LAPIC_SVR_ENABLE);
}

/*
 * lapic_disable
 * -------------
 * Nonaktifkan Local APIC.
 */
void lapic_disable(void)
{
    tak_bertanda32_t svr = lapic_read(LAPIC_SVR);
    lapic_write(LAPIC_SVR, svr & ~LAPIC_SVR_ENABLE);
}

/*
 * lapic_send_ipi
 * --------------
 * Kirim Inter-Processor Interrupt.
 *
 * Parameter:
 *   dest   - Destination APIC ID
 *   vector - Interrupt vector
 *   mode   - Delivery mode
 */
void lapic_send_ipi(tak_bertanda32_t dest, tak_bertanda32_t vector,
                    tak_bertanda32_t mode)
{
    /* Set destination */
    lapic_write(LAPIC_ICR_HIGH, dest << 24);

    /* Send IPI */
    lapic_write(LAPIC_ICR_LOW, vector | mode | (1 << 14));  /* Assert */

    /* Wait for delivery */
    while (lapic_read(LAPIC_ICR_LOW) & (1 << 12)) {
        cpu_nop();
    }
}

/*
 * lapic_send_broadcast
 * --------------------
 * Kirim IPI ke semua CPU.
 *
 * Parameter:
 *   vector - Interrupt vector
 *   mode   - Delivery mode
 */
void lapic_send_broadcast(tak_bertanda32_t vector, tak_bertanda32_t mode)
{
    lapic_write(LAPIC_ICR_HIGH, 0xFF << 24);
    lapic_write(LAPIC_ICR_LOW, vector | mode | (1 << 14) | (1 << 18) |
                                (1 << 19));

    while (lapic_read(LAPIC_ICR_LOW) & (1 << 12)) {
        cpu_nop();
    }
}

/*
 * lapic_timer_set
 * ---------------
 * Set Local APIC timer.
 *
 * Parameter:
 *   count  - Initial count
 *   vector - Interrupt vector
 *   mode   - Timer mode (0=one-shot, 1=periodic)
 */
void lapic_timer_set(tak_bertanda32_t count, tak_bertanda32_t vector,
                     tak_bertanda32_t mode)
{
    tak_bertanda32_t lvt = vector;

    if (mode == 1) {
        lvt |= 0x20000;  /* Periodic mode */
    }

    lapic_write(LAPIC_LVT_TIMER, lvt);
    lapic_write(LAPIC_TIMER_ICR, count);
}

/*
 * lapic_timer_stop
 * ----------------
 * Stop Local APIC timer.
 */
void lapic_timer_stop(void)
{
    lapic_write(LAPIC_LVT_TIMER, lapic_read(LAPIC_LVT_TIMER) | 0x10000);
}

/*
 * ioapic_set_routing
 * ------------------
 * Set interrupt routing di I/O APIC.
 *
 * Parameter:
 *   gsi      - Global System Interrupt
 *   vector   - Interrupt vector
 *   delivery - Delivery mode
 *   trigger  - Trigger mode (0=edge, 1=level)
 *   mask     - Mask interrupt
 *
 * Return: Status operasi
 */
status_t ioapic_set_routing(tak_bertanda32_t gsi, tak_bertanda32_t vector,
                            tak_bertanda32_t delivery,
                            tak_bertanda32_t trigger, bool_t mask)
{
    tak_bertanda32_t ioapic_id;
    tak_bertanda32_t pin;
    tak_bertanda32_t reg;
    tak_bertanda32_t low, high;

    /* Find correct I/O APIC for this GSI */
    for (ioapic_id = 0; ioapic_id < apic.ioapic_count; ioapic_id++) {
        if (apic.ioapic[ioapic_id].present &&
            gsi >= apic.ioapic[ioapic_id].gsi_base &&
            gsi < apic.ioapic[ioapic_id].gsi_base +
                  apic.ioapic[ioapic_id].gsi_count) {
            break;
        }
    }

    if (ioapic_id >= apic.ioapic_count) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    pin = gsi - apic.ioapic[ioapic_id].gsi_base;
    reg = IOAPIC_REDTBL_BASE + (pin * 2);

    /* Build redirection entry */
    low = vector | delivery | trigger |
          (mask ? IOAPIC_INT_MASK : 0);
    high = lapic_get_id() << 24;  /* Destination = this CPU */

    /* Write entry */
    ioapic_write(ioapic_id, reg, low);
    ioapic_write(ioapic_id, reg + 1, high);

    return STATUS_BERHASIL;
}

/*
 * ioapic_enable_irq
 * -----------------
 * Aktifkan IRQ di I/O APIC.
 *
 * Parameter:
 *   gsi - Global System Interrupt
 *
 * Return: Status operasi
 */
status_t ioapic_enable_irq(tak_bertanda32_t gsi)
{
    tak_bertanda32_t ioapic_id;
    tak_bertanda32_t pin;
    tak_bertanda32_t reg;
    tak_bertanda32_t low;

    for (ioapic_id = 0; ioapic_id < apic.ioapic_count; ioapic_id++) {
        if (apic.ioapic[ioapic_id].present &&
            gsi >= apic.ioapic[ioapic_id].gsi_base &&
            gsi < apic.ioapic[ioapic_id].gsi_base +
                  apic.ioapic[ioapic_id].gsi_count) {
            break;
        }
    }

    if (ioapic_id >= apic.ioapic_count) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    pin = gsi - apic.ioapic[ioapic_id].gsi_base;
    reg = IOAPIC_REDTBL_BASE + (pin * 2);

    low = ioapic_read(ioapic_id, reg);
    low &= ~IOAPIC_INT_MASK;  /* Clear mask bit */
    ioapic_write(ioapic_id, reg, low);

    return STATUS_BERHASIL;
}

/*
 * ioapic_disable_irq
 * ------------------
 * Nonaktifkan IRQ di I/O APIC.
 *
 * Parameter:
 *   gsi - Global System Interrupt
 *
 * Return: Status operasi
 */
status_t ioapic_disable_irq(tak_bertanda32_t gsi)
{
    tak_bertanda32_t ioapic_id;
    tak_bertanda32_t pin;
    tak_bertanda32_t reg;
    tak_bertanda32_t low;

    for (ioapic_id = 0; ioapic_id < apic.ioapic_count; ioapic_id++) {
        if (apic.ioapic[ioapic_id].present &&
            gsi >= apic.ioapic[ioapic_id].gsi_base &&
            gsi < apic.ioapic[ioapic_id].gsi_base +
                  apic.ioapic[ioapic_id].gsi_count) {
            break;
        }
    }

    if (ioapic_id >= apic.ioapic_count) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    pin = gsi - apic.ioapic[ioapic_id].gsi_base;
    reg = IOAPIC_REDTBL_BASE + (pin * 2);

    low = ioapic_read(ioapic_id, reg);
    low |= IOAPIC_INT_MASK;  /* Set mask bit */
    ioapic_write(ioapic_id, reg, low);

    return STATUS_BERHASIL;
}

/*
 * apic_is_available
 * -----------------
 * Cek apakah APIC tersedia.
 *
 * Return: BENAR jika APIC tersedia
 */
bool_t apic_is_available(void)
{
    return apic.lapic_present;
}

/*
 * apic_get_lapic_id
 * -----------------
 * Dapatkan LAPIC ID CPU saat ini.
 *
 * Return: LAPIC ID
 */
tak_bertanda32_t apic_get_lapic_id(void)
{
    if (!apic.lapic_present) {
        return 0;
    }

    return lapic_get_id();
}

/*
 * apic_print_status
 * -----------------
 * Print status APIC.
 */
void apic_print_status(void)
{
    kernel_printf("\n[APIC] Status:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Local APIC present: %s\n",
                  apic.lapic_present ? "Ya" : "Tidak");
    kernel_printf("  I/O APIC present:   %s\n",
                  apic.ioapic_present ? "Ya" : "Tidak");

    if (apic.lapic_present) {
        kernel_printf("  LAPIC base:     0x%lX\n",
                      (ukuran_t)apic.lapic_base);
        kernel_printf("  LAPIC ID:       %lu\n", apic.lapic_id);
        kernel_printf("  LAPIC Version:  %lu\n", apic.lapic_version);
        kernel_printf("  TPR:            0x%02lX\n", lapic_get_tpr());
        kernel_printf("  Spurious count: %lu\n", apic.spurious_count);
    }

    if (apic.ioapic_present) {
        tak_bertanda32_t i;
        kernel_printf("  I/O APIC count: %lu\n", apic.ioapic_count);

        for (i = 0; i < apic.ioapic_count; i++) {
            if (apic.ioapic[i].present) {
                kernel_printf("    IOAPIC %lu: ID=%lu, Base=0x%lX, "
                              "GSI=%lu-%lu\n",
                              i,
                              apic.ioapic[i].id,
                              (ukuran_t)apic.ioapic[i].base,
                              apic.ioapic[i].gsi_base,
                              apic.ioapic[i].gsi_base +
                              apic.ioapic[i].gsi_count - 1);
            }
        }
    }

    kernel_printf("========================================\n");
}
