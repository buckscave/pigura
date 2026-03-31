/*
 * PIGURA OS - SMP.C
 * ==================
 * Implementasi Symmetric Multi-Processing.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "cpu.h"

/*
 * ===========================================================================
 * KONSTANTA
 * ===========================================================================
 */

#define SMP_MAX_CPUS    256
#define APIC_ICR_INIT   0x00000500
#define APIC_ICR_STARTUP 0x00000600

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static bool_t g_smp_initialized = SALAH;
static tak_bertanda32_t g_cpu_count = 1;
static tak_bertanda32_t g_bsp_id = 0;

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

status_t cpu_smp_init(void)
{
    if (g_smp_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
    /* Dapatkan BSP ID */
    g_bsp_id = cpu_apic_id();
    
    /* Scan untuk APs via ACPI/MADT atau MP Tables */
    /* Untuk sekarang, asumsikan 1 CPU */
    g_cpu_count = 1;
    
    g_smp_initialized = BENAR;
    
    return STATUS_BERHASIL;
}

tak_bertanda32_t cpu_smp_jumlah(void)
{
    if (!g_smp_initialized) {
        return 1;
    }
    
    return g_cpu_count;
}

tak_bertanda32_t cpu_smp_id(void)
{
    return cpu_apic_id();
}

status_t cpu_smp_boot(tak_bertanda32_t cpu_id)
{
    if (!g_smp_initialized) {
        return STATUS_GAGAL;
    }
    
    if (cpu_id >= g_cpu_count) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Kirim INIT IPI */
    /* Kirim STARTUP IPI */
    
    return STATUS_TIDAK_DUKUNG;
}

status_t cpu_smp_broadcast(tak_bertanda32_t vector)
{
    (void)vector;
    return STATUS_TIDAK_DUKUNG;
}

bool_t cpu_smp_is_bsp(void)
{
    return (cpu_apic_id() == g_bsp_id);
}
