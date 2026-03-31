/*
 * =============================================================================
 * PIGURA OS - DETEKSI_MEMORI.C
 * ============================
 * Implementasi deteksi memori untuk Pigura OS.
 *
 * Berkas ini berisi fungsi-fungsi untuk mendeteksi dan menganalisis
 * memori fisik yang tersedia pada sistem.
 *
 * Arsitektur: x86/x86_64
 * Versi: 1.0
 * =============================================================================
 */

#include <stdint.h>
#include <stddef.h>

/* Kompatibilitas C89: inline tidak tersedia */
#ifndef inline
#ifdef __GNUC__
    #define inline __inline__
#else
    #define inline
#endif
#endif

/* =============================================================================
 * KONSTANTA
 * =============================================================================
 */

/* BIOS interrupt untuk memory detection */
#define BIOS_INT_MEM            0x15
#define BIOS_FUNC_E820          0xE820
#define BIOS_FUNC_E801          0xE801
#define BIOS_FUNC_88            0x88

/* Memory map entry types */
#define MEM_TYPE_AVAILABLE      1
#define MEM_TYPE_RESERVED       2
#define MEM_TYPE_ACPI_RECLAIM   3
#define MEM_TYPE_ACPI_NVS       4
#define MEM_TYPE_UNUSABLE       5
#define MEM_TYPE_DISABLED       6

/* Memory constants */
#define MEM_BASE_LOW            0x00000000      /* 0 - 640 KB */
#define MEM_BASE_HIGH           0x00100000      /* 1 MB */
#define MEM_EXT_BASE            0x00100000      /* Extended memory base */
#define MEM_HOLE_START          0x000A0000      /* 640 KB - video memory */
#define MEM_HOLE_END            0x00100000      /* 1 MB */

/* Maximum entries */
#define MAX_MMAP_ENTRIES        128

/* Default memory values */
#define DEFAULT_MEMORY_SIZE     (64 * 1024 * 1024)  /* 64 MB default */

/* Port CMOS */
#define PORT_CMOS_INDEX         0x70
#define PORT_CMOS_DATA          0x71

/* CMOS registers */
#define CMOS_REG_EXT_MEM_LOW    0x30
#define CMOS_REG_EXT_MEM_HIGH   0x31

/* =============================================================================
 * TIPE DATA
 * =============================================================================
 */

/* Memory map entry (BIOS E820 format) */
typedef struct {
    uint64_t base;              /* Base address */
    uint64_t length;            /* Length in bytes */
    uint32_t type;              /* Memory type */
    uint32_t acpi_attrs;        /* ACPI extended attributes */
} mem_entry_t;

/* Memory map */
typedef struct {
    mem_entry_t entries[MAX_MMAP_ENTRIES];
    uint32_t count;
    uint64_t total;
    uint64_t available;
} mem_map_t;

/* Memory statistics */
typedef struct {
    uint64_t total_bytes;
    uint64_t available_bytes;
    uint64_t reserved_bytes;
    uint64_t kernel_bytes;
    uint32_t total_pages;
    uint32_t available_pages;
    uint32_t page_size;
} mem_stats_t;

/* =============================================================================
 * VARIABEL GLOBAL
 * =============================================================================
 */

/* Memory map global */
static mem_map_t g_mem_map;

/* Memory statistics */
static mem_stats_t g_mem_stats;

/* Flag inisialisasi */
static int g_mem_detected = 0;

/* =============================================================================
 * FUNGSI INLINE ASSEMBLY
 * =============================================================================
 */

/*
 * _outb
 * -----
 * Tulis byte ke port.
 */
static inline void _outb(uint8_t val, uint16_t port)
{
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

/*
 * _inb
 * ----
 * Baca byte dari port.
 */
static inline uint8_t _inb(uint16_t port)
{
    uint8_t val;
    __asm__ __volatile__("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/*
 * _memset
 * -------
 * Mengisi memori.
 */
static void _memset(void *dest, uint8_t val, size_t n)
{
    uint8_t *d;
    size_t i;

    d = (uint8_t *)dest;
    for (i = 0; i < n; i++) {
        d[i] = val;
    }
}

/*
 * _memcpy
 * -------
 * Menyalin memori.
 */
static void _memcpy(void *dest, const void *src, size_t n)
{
    uint8_t *d;
    const uint8_t *s;
    size_t i;

    d = (uint8_t *)dest;
    s = (const uint8_t *)src;

    for (i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

/* =============================================================================
 * FUNGSI DETEKSI MEMORI
 * =============================================================================
 */

/*
 * _detect_cmos_memory
 * -------------------
 * Deteksi memori via CMOS.
 */
static uint32_t _detect_cmos_memory(void)
{
    uint8_t low;
    uint8_t high;
    uint16_t ext_mem;
    uint32_t total;

    /* Read extended memory size from CMOS */
    _outb(CMOS_REG_EXT_MEM_LOW, PORT_CMOS_INDEX);
    low = _inb(PORT_CMOS_DATA);

    _outb(CMOS_REG_EXT_MEM_HIGH, PORT_CMOS_INDEX);
    high = _inb(PORT_CMOS_DATA);

    ext_mem = ((uint16_t)high << 8) | low;

    /* Convert KB to bytes */
    total = (uint32_t)ext_mem * 1024;

    /* Add base memory (640 KB) */
    total += 640 * 1024;

    return total;
}

/*
 * _detect_bios_e801
 * -----------------
 * Deteksi memori via BIOS INT 15h, AX=E801h.
 */
static uint32_t _detect_bios_e801(void)
{
    uint32_t total;
    uint16_t ax;
    uint16_t bx;
    uint16_t cx;
    uint16_t dx;
    uint8_t cf;

    total = 0;
    cf = 0;

    /* Call BIOS interrupt */
    __asm__ __volatile__(
        "int $0x15\n\t"
        "setc %4"
        : "=a"(ax), "=b"(bx), "=c"(cx), "=d"(dx), "=rm"(cf)
        : "a"(0xE801), "b"(0), "c"(0), "d"(0)
    );

    if (cf) {
        /* Call failed */
        return 0;
    }

    /* Calculate total memory */
    if (ax > 0) {
        /* AX = memory between 1MB and 16MB in KB */
        total = (uint32_t)ax * 1024;
    } else {
        /* CX = memory between 1MB and 16MB in KB */
        total = (uint32_t)cx * 1024;
    }

    if (bx > 0) {
        /* BX = memory above 16MB in 64KB blocks */
        total += (uint32_t)bx * 64 * 1024;
    } else {
        /* DX = memory above 16MB in 64KB blocks */
        total += (uint32_t)dx * 64 * 1024;
    }

    /* Add base memory (1MB) */
    total += 1024 * 1024;

    return total;
}

/*
 * _detect_bios_e820
 * -----------------
 * Deteksi memori via BIOS INT 15h, AX=E820h.
 * Ini adalah metode yang paling akurat.
 */
static int _detect_bios_e820(void)
{
    uint32_t cont;
    uint32_t signature;
    uint32_t bytes;
    struct {
        uint64_t base;
        uint64_t length;
        uint32_t type;
        uint32_t acpi;
    } entry;
    int count;
    uint8_t cf;
    uint32_t sig_out;

    count = 0;
    cont = 0;
    cf = 0;
    signature = 0x534D4150;     /* "SMAP" */

    _memset(&g_mem_map, 0, sizeof(g_mem_map));

    do {
        /* Prepare for BIOS call */
        bytes = sizeof(entry);
        sig_out = signature;

        /* Call BIOS */
        __asm__ __volatile__(
            "int $0x15\n\t"
            "setc %4"
            : "=a"(sig_out), "=b"(bytes), "=c"(cont),
              "=d"(signature), "=rm"(cf)
            : "a"(0xE820), "b"(cont), "c"(sizeof(entry)),
              "d"(signature),
              "D"(&entry)
            : "memory"
        );

        /* Check for error */
        if (cf || sig_out != 0x534D4150) {
            break;
        }

        /* Skip if no data returned */
        if (bytes < 20) {
            continue;
        }

        /* Store entry */
        if (count < MAX_MMAP_ENTRIES) {
            g_mem_map.entries[count].base = entry.base;
            g_mem_map.entries[count].length = entry.length;
            g_mem_map.entries[count].type = entry.type;
            g_mem_map.entries[count].acpi_attrs = entry.acpi;
            count++;
        }

    } while (cont != 0 && count < MAX_MMAP_ENTRIES);

    g_mem_map.count = count;

    return (count > 0) ? 0 : -1;
}

/*
 * _calculate_stats
 * ----------------
 * Hitung statistik memori dari memory map.
 */
static void _calculate_stats(void)
{
    uint32_t i;
    mem_entry_t *entry;

    g_mem_map.total = 0;
    g_mem_map.available = 0;

    for (i = 0; i < g_mem_map.count; i++) {
        entry = &g_mem_map.entries[i];

        g_mem_map.total += entry->length;

        if (entry->type == MEM_TYPE_AVAILABLE) {
            g_mem_map.available += entry->length;
        }
    }

    g_mem_stats.total_bytes = g_mem_map.total;
    g_mem_stats.available_bytes = g_mem_map.available;
    g_mem_stats.reserved_bytes = g_mem_map.total - g_mem_map.available;
    g_mem_stats.page_size = 4096;
    g_mem_stats.total_pages = (uint32_t)(g_mem_map.total / 4096);
    g_mem_stats.available_pages = (uint32_t)(g_mem_map.available / 4096);
}

/* =============================================================================
 * FUNGSI PUBLIC
 * =============================================================================
 */

/*
 * deteksi_memori_init
 * -------------------
 * Inisialisasi deteksi memori.
 */
int deteksi_memori_init(void)
{
    int result;

    _memset(&g_mem_map, 0, sizeof(g_mem_map));
    _memset(&g_mem_stats, 0, sizeof(g_mem_stats));

    /* Try E820 first (most accurate) */
    result = _detect_bios_e820();
    if (result == 0) {
        _calculate_stats();
        g_mem_detected = 1;
        return 0;
    }

    /* Try E801 as fallback */
    g_mem_stats.total_bytes = _detect_bios_e801();
    if (g_mem_stats.total_bytes > 0) {
        g_mem_stats.available_bytes = g_mem_stats.total_bytes - 
                                       (1024 * 1024);  /* Subtract 1MB kernel */
        g_mem_detected = 1;
        return 0;
    }

    /* Try CMOS as last resort */
    g_mem_stats.total_bytes = _detect_cmos_memory();
    if (g_mem_stats.total_bytes > 0) {
        g_mem_stats.available_bytes = g_mem_stats.total_bytes;
        g_mem_detected = 1;
        return 0;
    }

    /* Use default */
    g_mem_stats.total_bytes = DEFAULT_MEMORY_SIZE;
    g_mem_stats.available_bytes = DEFAULT_MEMORY_SIZE;

    return 0;
}

/*
 * deteksi_memori_get_total
 * ------------------------
 * Dapatkan total memori (bytes).
 */
uint64_t deteksi_memori_get_total(void)
{
    return g_mem_stats.total_bytes;
}

/*
 * deteksi_memori_get_available
 * ----------------------------
 * Dapatkan memori tersedia (bytes).
 */
uint64_t deteksi_memori_get_available(void)
{
    return g_mem_stats.available_bytes;
}

/*
 * deteksi_memori_get_reserved
 * ---------------------------
 * Dapatkan memori reserved (bytes).
 */
uint64_t deteksi_memori_get_reserved(void)
{
    return g_mem_stats.reserved_bytes;
}

/*
 * deteksi_memori_get_entry_count
 * ------------------------------
 * Dapatkan jumlah memory map entries.
 */
uint32_t deteksi_memori_get_entry_count(void)
{
    return g_mem_map.count;
}

/*
 * deteksi_memori_get_entry
 * ------------------------
 * Dapatkan memory map entry berdasarkan index.
 */
int deteksi_memori_get_entry(uint32_t index, uint64_t *base,
                              uint64_t *length, uint32_t *type)
{
    if (index >= g_mem_map.count) {
        return -1;
    }

    if (base != NULL) {
        *base = g_mem_map.entries[index].base;
    }

    if (length != NULL) {
        *length = g_mem_map.entries[index].length;
    }

    if (type != NULL) {
        *type = g_mem_map.entries[index].type;
    }

    return 0;
}

/*
 * deteksi_memori_get_stats
 * ------------------------
 * Dapatkan statistik memori.
 */
void deteksi_memori_get_stats(mem_stats_t *stats)
{
    if (stats != NULL) {
        _memcpy(stats, &g_mem_stats, sizeof(mem_stats_t));
    }
}

/*
 * deteksi_memori_get_page_count
 * -----------------------------
 * Dapatkan jumlah halaman total.
 */
uint32_t deteksi_memori_get_page_count(void)
{
    return g_mem_stats.total_pages;
}

/*
 * deteksi_memori_get_available_pages
 * ----------------------------------
 * Dapatkan jumlah halaman tersedia.
 */
uint32_t deteksi_memori_get_available_pages(void)
{
    return g_mem_stats.available_pages;
}

/*
 * deteksi_memori_is_detected
 * --------------------------
 * Cek apakah deteksi memori sudah dilakukan.
 */
int deteksi_memori_is_detected(void)
{
    return g_mem_detected;
}

/*
 * deteksi_memori_find_available
 * -----------------------------
 * Cari region memori yang tersedia dengan ukuran minimum.
 */
uint64_t deteksi_memori_find_available(uint64_t min_size,
                                        uint64_t align)
{
    uint32_t i;
    mem_entry_t *entry;
    uint64_t aligned_base;

    for (i = 0; i < g_mem_map.count; i++) {
        entry = &g_mem_map.entries[i];

        if (entry->type != MEM_TYPE_AVAILABLE) {
            continue;
        }

        if (entry->length < min_size) {
            continue;
        }

        /* Align base address */
        aligned_base = (entry->base + align - 1) & ~(align - 1);

        /* Check if still within region */
        if (aligned_base + min_size <= entry->base + entry->length) {
            return aligned_base;
        }
    }

    return 0;   /* Not found */
}

/*
 * deteksi_memori_is_available
 * ---------------------------
 * Cek apakah alamat berada di region yang tersedia.
 */
int deteksi_memori_is_available(uint64_t addr, uint64_t size)
{
    uint32_t i;
    mem_entry_t *entry;
    uint64_t end;

    end = addr + size;

    for (i = 0; i < g_mem_map.count; i++) {
        entry = &g_mem_map.entries[i];

        if (entry->type != MEM_TYPE_AVAILABLE) {
            continue;
        }

        if (addr >= entry->base && end <= entry->base + entry->length) {
            return 1;
        }
    }

    return 0;
}

/*
 * deteksi_memori_get_type_name
 * ----------------------------
 * Dapatkan nama tipe memori.
 */
const char *deteksi_memori_get_type_name(uint32_t type)
{
    switch (type) {
        case MEM_TYPE_AVAILABLE:
            return "Available";
        case MEM_TYPE_RESERVED:
            return "Reserved";
        case MEM_TYPE_ACPI_RECLAIM:
            return "ACPI Reclaimable";
        case MEM_TYPE_ACPI_NVS:
            return "ACPI NVS";
        case MEM_TYPE_UNUSABLE:
            return "Unusable";
        case MEM_TYPE_DISABLED:
            return "Disabled";
        default:
            return "Unknown";
    }
}

/*
 * deteksi_memori_print_map
 * ------------------------
 * Print memory map (untuk debugging).
 */
void deteksi_memori_print_map(void)
{
    uint32_t i;
    mem_entry_t *entry;
    const char *type_name;

    /* Simple console output would go here */
    /* This is a placeholder for debugging purposes */

    for (i = 0; i < g_mem_map.count; i++) {
        entry = &g_mem_map.entries[i];
        type_name = deteksi_memori_get_type_name(entry->type);

        /* Would print: base, length, type_name */
        (void)type_name;
    }
}
