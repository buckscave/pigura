/*
 * PIGURA OS - ACPI.C
 * ===================
 * Implementasi ACPI (Advanced Configuration and Power Interface).
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "cpu.h"

/*
 * ===========================================================================
 * KONSTANTA ACPI
 * ===========================================================================
 */

#define ACPI_RSDP_SIGNATURE    0x2052545020445352ULL  /* "RSD PTR " */
#define ACPI_RSDT_SIGNATURE    0x54445352  /* "RSDT" */
#define ACPI_XSDT_SIGNATURE    0x54445358  /* "XSDT" */
#define ACPI_MADT_SIGNATURE    0x43495041  /* "APIC" */
#define ACPI_FADT_SIGNATURE    0x50434146  /* "FACP" */
#define ACPI_HPET_SIGNATURE    0x54455048  /* "HPET" */

/* PM1a Control Register */
#define ACPI_PM1A_CNT_BLK      0x604
#define ACPI_SLP_TYP_MASK      0x1C00
#define ACPI_SLP_EN            0x2000

/*
 * ===========================================================================
 * STRUKTUR DATA
 * ===========================================================================
 */

typedef struct {
    char signature[8];
    tak_bertanda8_t checksum;
    char oem_id[6];
    tak_bertanda8_t revision;
    tak_bertanda32_t rsdt_address;
    tak_bertanda32_t length;
    tak_bertanda64_t xsdt_address;
    tak_bertanda8_t extended_checksum;
    tak_bertanda8_t reserved[3];
} __attribute__((packed)) rsdp_t;

typedef struct {
    char signature[4];
    tak_bertanda32_t length;
    tak_bertanda8_t revision;
    tak_bertanda8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    tak_bertanda32_t oem_revision;
    tak_bertanda32_t creator_id;
    tak_bertanda32_t creator_revision;
} __attribute__((packed)) acpi_header_t;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static bool_t g_acpi_initialized = SALAH;
static rsdp_t *g_rsdp = NULL;
static void *g_rsdt = NULL;
static void *g_madt __attribute__((unused)) = NULL;
static void *g_fadt __attribute__((unused)) = NULL;

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

static rsdp_t *acpi_find_rsdp(void)
{
    tak_bertanda32_t addr;
    tak_bertanda64_t *ptr;
    
    /* Cari di EBDA (Extended BIOS Data Area) */
    {
        tak_bertanda16_t ebda_seg;
        __asm__ __volatile__("movw %%ds:0x40E, %0" : "=r"(ebda_seg));
        ptr = (tak_bertanda64_t *)((tak_bertanda32_t)ebda_seg << 4);
    }
    
    for (addr = 0; addr < 1024; addr += 16) {
        if (ptr[addr / 8] == ACPI_RSDP_SIGNATURE) {
            return (rsdp_t *)((tak_bertanda8_t *)ptr + addr);
        }
    }
    
    /* Cari di BIOS ROM area */
    ptr = (tak_bertanda64_t *)0xE0000;
    
    for (addr = 0; addr < 0x20000; addr += 16) {
        if (ptr[addr / 8] == ACPI_RSDP_SIGNATURE) {
            return (rsdp_t *)((tak_bertanda8_t *)ptr + addr);
        }
    }
    
    return NULL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

status_t cpu_acpi_init(void)
{
    if (g_acpi_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
    /* Cari RSDP */
    g_rsdp = acpi_find_rsdp();
    
    if (g_rsdp == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Parse RSDT/XSDT */
    if (g_rsdp->revision >= 2 && g_rsdp->xsdt_address != 0) {
        g_rsdt = (void *)(uintptr_t)g_rsdp->xsdt_address;
    } else {
        g_rsdt = (void *)(uintptr_t)g_rsdp->rsdt_address;
    }
    
    g_acpi_initialized = BENAR;
    
    return STATUS_BERHASIL;
}

void cpu_acpi_shutdown(void)
{
    tak_bertanda16_t port;
    tak_bertanda16_t value;
    
    /* ACPI shutdown via PM1a control register */
    port = ACPI_PM1A_CNT_BLK;
    value = (5 << 10) | ACPI_SLP_EN;  /* SLP_TYP = S5, SLP_EN = 1 */
    
    __asm__ __volatile__("outw %w0, %w1" : : "a"(value), "Nd"(port));
    
    /* Should not reach here */
    cpu_halt();
    while (1) { /* TIDAK_RETURN compliance */ }
}

void cpu_acpi_reboot(void)
{
    /* Reset via ACPI */
    /* Try FADT reset register first */
    
    /* Fallback: keyboard controller reset */
    cpu_reset(BENAR);
}

status_t cpu_acpi_find_table(const char *signature, void **table)
{
    (void)signature;
    (void)table;
    return STATUS_TIDAK_DITEMUKAN;
}
