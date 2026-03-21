/*
 * =============================================================================
 * PIGURA OS - MULTIBOOT.C
 * =======================
 * Implementasi Multiboot protocol untuk Pigura OS.
 *
 * Berkas ini berisi fungsi-fungsi untuk parsing dan menggunakan
 * informasi dari bootloader yang mendukung Multiboot specification.
 *
 * Spesifikasi: Multiboot 1
 * Arsitektur: x86/x86_64
 * Versi: 1.0
 * =============================================================================
 */

#include <stdint.h>
#include <stddef.h>

/* =============================================================================
 * KONSTANTA MULTIBOOT
 * =============================================================================
 */

/* Multiboot magic number */
#define MULTIBOOT_MAGIC             0x2BADB002

/* Multiboot info flags */
#define MULTIBOOT_FLAG_MEM          0x001   /* mem_* valid */
#define MULTIBOOT_FLAG_DEVICE       0x002   /* boot_device valid */
#define MULTIBOOT_FLAG_CMDLINE      0x004   /* cmdline valid */
#define MULTIBOOT_FLAG_MODS         0x008   /* mods_* valid */
#define MULTIBOOT_FLAG_AOUT         0x010   /* aout symbol table */
#define MULTIBOOT_FLAG_ELF          0x020   /* ELF section header table */
#define MULTIBOOT_FLAG_MMAP         0x040   /* mmap_* valid */
#define MULTIBOOT_FLAG_DRIVES       0x080   /* drives_* valid */
#define MULTIBOOT_FLAG_CONFIG       0x100   /* config_table valid */
#define MULTIBOOT_FLAG_LOADER       0x200   /* boot_loader_name valid */
#define MULTIBOOT_FLAG_APM          0x400   /* apm_table valid */
#define MULTIBOOT_FLAG_VBE          0x800   /* vbe_* valid */

/* Memory map entry types */
#define MULTIBOOT_MEMORY_AVAILABLE  1
#define MULTIBOOT_MEMORY_RESERVED   2
#define MULTIBOOT_MEMORY_ACPI       3
#define MULTIBOOT_MEMORY_NVS        4
#define MULTIBOOT_MEMORY_BADRAM     5

/* Boot device types */
#define BOOT_DEVICE_FLOPPY          0x00
#define BOOT_DEVICE_HDD             0x80
#define BOOT_DEVICE_CDROM           0xE0

/* =============================================================================
 * TIPE DATA MULTIBOOT
 * =============================================================================
 */

/* Multiboot info structure */
typedef struct {
    uint32_t flags;

    /* Available when flags[0] is set */
    uint32_t mem_lower;             /* Amount of lower memory (KB) */
    uint32_t mem_upper;             /* Amount of upper memory (KB) */

    /* Available when flags[1] is set */
    uint32_t boot_device;           /* Boot device */

    /* Available when flags[2] is set */
    uint32_t cmdline;               /* Kernel command line */

    /* Available when flags[3] is set */
    uint32_t mods_count;            /* Number of modules loaded */
    uint32_t mods_addr;             /* Address of module structures */

    /* Available when flags[4] or flags[5] is set */
    uint32_t syms[4];               /* Symbol table info */

    /* Available when flags[6] is set */
    uint32_t mmap_length;           /* Memory map length */
    uint32_t mmap_addr;             /* Memory map address */

    /* Available when flags[7] is set */
    uint32_t drives_length;         /* Drive info length */
    uint32_t drives_addr;           /* Drive info address */

    /* Available when flags[8] is set */
    uint32_t config_table;          /* ROM configuration table */

    /* Available when flags[9] is set */
    uint32_t boot_loader_name;      /* Boot loader name */

    /* Available when flags[10] is set */
    uint32_t apm_table;             /* APM table */

    /* Available when flags[11] is set */
    uint32_t vbe_control_info;      /* VBE control info */
    uint32_t vbe_mode_info;         /* VBE mode info */
    uint16_t vbe_mode;              /* VBE mode */
    uint16_t vbe_interface_seg;     /* VBE interface segment */
    uint16_t vbe_interface_off;     /* VBE interface offset */
    uint16_t vbe_interface_len;     /* VBE interface length */
} __attribute__((packed)) multiboot_info_t;

/* Module structure */
typedef struct {
    uint32_t mod_start;             /* Start address of module */
    uint32_t mod_end;               /* End address of module */
    uint32_t cmdline;               /* Module command line */
    uint32_t reserved;              /* Reserved */
} __attribute__((packed)) multiboot_module_t;

/* Memory map entry */
typedef struct {
    uint32_t size;                  /* Size of this structure */
    uint32_t base_addr_low;         /* Base address */
    uint32_t base_addr_high;
    uint32_t length_low;            /* Length */
    uint32_t length_high;
    uint32_t type;                  /* Memory type */
} __attribute__((packed)) multiboot_mmap_entry_t;

/* APM table */
typedef struct {
    uint16_t version;
    uint16_t cseg;
    uint32_t offset;
    uint16_t cseg_16;
    uint16_t dseg;
    uint16_t flags;
    uint16_t cseg_len;
    uint16_t cseg_16_len;
    uint16_t dseg_len;
} __attribute__((packed)) multiboot_apm_table_t;

/* =============================================================================
 * VARIABEL GLOBAL
 * =============================================================================
 */

/* Simpan pointer ke multiboot info */
static multiboot_info_t *g_mbi = NULL;

/* Simpan magic number */
static uint32_t g_magic = 0;

/* Flag inisialisasi */
static int g_multiboot_initialized = 0;

/* Info memori yang sudah diparse */
static uint64_t g_total_memory = 0;
static uint64_t g_available_memory = 0;

/* =============================================================================
 * FUNGSI INTERNAL
 * =============================================================================
 */

/*
 * _strlen
 * -------
 * Menghitung panjang string.
 */
static size_t _strlen(const char *str)
{
    size_t len;
    len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

/*
 * _parse_memory_map
 * -----------------
 * Parsing memory map dari multiboot info.
 */
static void _parse_memory_map(void)
{
    multiboot_mmap_entry_t *entry;
    uint32_t addr;
    uint32_t end;
    uint64_t base;
    uint64_t length;

    if (g_mbi == NULL) {
        return;
    }

    if (!(g_mbi->flags & MULTIBOOT_FLAG_MMAP)) {
        /* Tidak ada memory map, gunakan mem_lower/mem_upper */
        g_total_memory = (uint64_t)g_mbi->mem_lower * 1024;
        g_total_memory += (uint64_t)g_mbi->mem_upper * 1024;
        g_available_memory = g_total_memory;
        return;
    }

    /* Parse memory map entries */
    addr = g_mbi->mmap_addr;
    end = addr + g_mbi->mmap_length;

    while (addr < end) {
        entry = (multiboot_mmap_entry_t *)addr;

        /* Get base address (64-bit) */
        base = ((uint64_t)entry->base_addr_high << 32) |
               (uint64_t)entry->base_addr_low;

        /* Get length (64-bit) */
        length = ((uint64_t)entry->length_high << 32) |
                 (uint64_t)entry->length_low;

        /* Add to total */
        g_total_memory += length;

        /* Check if available */
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            g_available_memory += length;
        }

        /* Move to next entry */
        addr += entry->size + sizeof(entry->size);
    }
}

/*
 * _parse_basic_memory
 * -------------------
 * Parsing basic memory info dari multiboot.
 */
static void _parse_basic_memory(void)
{
    if (g_mbi == NULL) {
        return;
    }

    if (g_mbi->flags & MULTIBOOT_FLAG_MEM) {
        g_total_memory = (uint64_t)g_mbi->mem_lower * 1024;
        g_total_memory += (uint64_t)g_mbi->mem_upper * 1024;
        g_available_memory = g_total_memory;
    }
}

/* =============================================================================
 * FUNGSI PUBLIC
 * =============================================================================
 */

/*
 * multiboot_init
 * --------------
 * Menginisialisasi multiboot info.
 *
 * Parameter:
 *   magic - Magic number dari bootloader
 *   mbi   - Pointer ke multiboot info structure
 *
 * Return:
 *   0 jika berhasil, -1 jika error
 */
int multiboot_init(uint32_t magic, void *mbi)
{
    /* Simpan parameter */
    g_magic = magic;
    g_mbi = (multiboot_info_t *)mbi;

    /* Verifikasi magic number */
    if (magic != MULTIBOOT_MAGIC) {
        return -1;
    }

    /* Parse memory info */
    _parse_basic_memory();
    _parse_memory_map();

    g_multiboot_initialized = 1;

    return 0;
}

/*
 * multiboot_get_info
 * ------------------
 * Mendapatkan pointer ke multiboot info.
 */
multiboot_info_t *multiboot_get_info(void)
{
    return g_mbi;
}

/*
 * multiboot_get_magic
 * -------------------
 * Mendapatkan magic number.
 */
uint32_t multiboot_get_magic(void)
{
    return g_magic;
}

/*
 * multiboot_is_valid
 * ------------------
 * Cek apakah multiboot valid.
 */
int multiboot_is_valid(void)
{
    return (g_magic == MULTIBOOT_MAGIC) ? 1 : 0;
}

/*
 * multiboot_get_memory_lower
 * --------------------------
 * Mendapatkan lower memory (KB).
 */
uint32_t multiboot_get_memory_lower(void)
{
    if (g_mbi != NULL && (g_mbi->flags & MULTIBOOT_FLAG_MEM)) {
        return g_mbi->mem_lower;
    }
    return 0;
}

/*
 * multiboot_get_memory_upper
 * --------------------------
 * Mendapatkan upper memory (KB).
 */
uint32_t multiboot_get_memory_upper(void)
{
    if (g_mbi != NULL && (g_mbi->flags & MULTIBOOT_FLAG_MEM)) {
        return g_mbi->mem_upper;
    }
    return 0;
}

/*
 * multiboot_get_total_memory
 * --------------------------
 * Mendapatkan total memory (bytes).
 */
uint64_t multiboot_get_total_memory(void)
{
    return g_total_memory;
}

/*
 * multiboot_get_available_memory
 * ------------------------------
 * Mendapatkan available memory (bytes).
 */
uint64_t multiboot_get_available_memory(void)
{
    return g_available_memory;
}

/*
 * multiboot_get_cmdline
 * ---------------------
 * Mendapatkan kernel command line.
 */
const char *multiboot_get_cmdline(void)
{
    if (g_mbi != NULL && (g_mbi->flags & MULTIBOOT_FLAG_CMDLINE)) {
        return (const char *)g_mbi->cmdline;
    }
    return NULL;
}

/*
 * multiboot_get_boot_device
 * -------------------------
 * Mendapatkan boot device.
 */
uint32_t multiboot_get_boot_device(void)
{
    if (g_mbi != NULL && (g_mbi->flags & MULTIBOOT_FLAG_DEVICE)) {
        return g_mbi->boot_device;
    }
    return 0;
}

/*
 * multiboot_get_boot_device_type
 * ------------------------------
 * Mendapatkan tipe boot device.
 */
uint8_t multiboot_get_boot_device_type(void)
{
    uint32_t device;

    device = multiboot_get_boot_device();
    return (uint8_t)((device >> 24) & 0xFF);
}

/*
 * multiboot_get_boot_loader_name
 * ------------------------------
 * Mendapatkan nama bootloader.
 */
const char *multiboot_get_boot_loader_name(void)
{
    if (g_mbi != NULL && (g_mbi->flags & MULTIBOOT_FLAG_LOADER)) {
        return (const char *)g_mbi->boot_loader_name;
    }
    return "Unknown";
}

/*
 * multiboot_get_modules_count
 * ---------------------------
 * Mendapatkan jumlah modules.
 */
uint32_t multiboot_get_modules_count(void)
{
    if (g_mbi != NULL && (g_mbi->flags & MULTIBOOT_FLAG_MODS)) {
        return g_mbi->mods_count;
    }
    return 0;
}

/*
 * multiboot_get_module
 * --------------------
 * Mendapatkan module berdasarkan index.
 */
multiboot_module_t *multiboot_get_module(uint32_t index)
{
    multiboot_module_t *modules;

    if (g_mbi == NULL || !(g_mbi->flags & MULTIBOOT_FLAG_MODS)) {
        return NULL;
    }

    if (index >= g_mbi->mods_count) {
        return NULL;
    }

    modules = (multiboot_module_t *)g_mbi->mods_addr;
    return &modules[index];
}

/*
 * multiboot_get_module_start
 * --------------------------
 * Mendapatkan alamat start module.
 */
uint32_t multiboot_get_module_start(uint32_t index)
{
    multiboot_module_t *mod;

    mod = multiboot_get_module(index);
    if (mod != NULL) {
        return mod->mod_start;
    }
    return 0;
}

/*
 * multiboot_get_module_end
 * ------------------------
 * Mendapatkan alamat end module.
 */
uint32_t multiboot_get_module_end(uint32_t index)
{
    multiboot_module_t *mod;

    mod = multiboot_get_module(index);
    if (mod != NULL) {
        return mod->mod_end;
    }
    return 0;
}

/*
 * multiboot_get_module_size
 * -------------------------
 * Mendapatkan ukuran module.
 */
uint32_t multiboot_get_module_size(uint32_t index)
{
    multiboot_module_t *mod;

    mod = multiboot_get_module(index);
    if (mod != NULL) {
        return mod->mod_end - mod->mod_start;
    }
    return 0;
}

/*
 * multiboot_get_mmap
 * ------------------
 * Mendapatkan memory map entry berdasarkan index.
 */
int multiboot_get_mmap(uint32_t index, uint64_t *base, uint64_t *length,
                        uint32_t *type)
{
    multiboot_mmap_entry_t *entry;
    uint32_t addr;
    uint32_t end;
    uint32_t count;

    if (g_mbi == NULL || !(g_mbi->flags & MULTIBOOT_FLAG_MMAP)) {
        return -1;
    }

    addr = g_mbi->mmap_addr;
    end = addr + g_mbi->mmap_length;
    count = 0;

    while (addr < end) {
        entry = (multiboot_mmap_entry_t *)addr;

        if (count == index) {
            if (base != NULL) {
                *base = ((uint64_t)entry->base_addr_high << 32) |
                        (uint64_t)entry->base_addr_low;
            }
            if (length != NULL) {
                *length = ((uint64_t)entry->length_high << 32) |
                          (uint64_t)entry->length_low;
            }
            if (type != NULL) {
                *type = entry->type;
            }
            return 0;
        }

        count++;
        addr += entry->size + sizeof(entry->size);
    }

    return -1;  /* Index not found */
}

/*
 * multiboot_get_mmap_count
 * ------------------------
 * Mendapatkan jumlah memory map entries.
 */
uint32_t multiboot_get_mmap_count(void)
{
    multiboot_mmap_entry_t *entry;
    uint32_t addr;
    uint32_t end;
    uint32_t count;

    if (g_mbi == NULL || !(g_mbi->flags & MULTIBOOT_FLAG_MMAP)) {
        return 0;
    }

    addr = g_mbi->mmap_addr;
    end = addr + g_mbi->mmap_length;
    count = 0;

    while (addr < end) {
        entry = (multiboot_mmap_entry_t *)addr;
        count++;
        addr += entry->size + sizeof(entry->size);
    }

    return count;
}

/*
 * multiboot_has_vbe
 * -----------------
 * Cek apakah VBE info tersedia.
 */
int multiboot_has_vbe(void)
{
    if (g_mbi != NULL && (g_mbi->flags & MULTIBOOT_FLAG_VBE)) {
        return 1;
    }
    return 0;
}

/*
 * multiboot_has_apm
 * -----------------
 * Cek apakah APM info tersedia.
 */
int multiboot_has_apm(void)
{
    if (g_mbi != NULL && (g_mbi->flags & MULTIBOOT_FLAG_APM)) {
        return 1;
    }
    return 0;
}

/*
 * multiboot_print_info
 * --------------------
 * Menampilkan informasi multiboot (untuk debugging).
 */
void multiboot_print_info(void)
{
    /* Implementasi printf sederhana tidak tersedia di bootloader */
    /* Fungsi ini akan diimplementasi dengan console output */
}

/*
 * multiboot_is_initialized
 * ------------------------
 * Cek apakah multiboot sudah diinisialisasi.
 */
int multiboot_is_initialized(void)
{
    return g_multiboot_initialized;
}
