/*
 * PIGURA OS - MULTIBOOT.C
 * -----------------------
 * Implementasi parsing multiboot information.
 *
 * Berkas ini berisi fungsi-fungsi untuk mem-parse informasi yang
 * diberikan oleh bootloader yang kompatibel dengan multiboot.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "kernel.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

#define MAX_MODULES             16
#define MAX_MMAP_ENTRIES        64

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

typedef struct {
    alamat_fisik_t mulai;
    alamat_fisik_t akhir;
    char *string;
} module_info_t;

typedef struct {
    tak_bertanda32_t count;
    mmap_entry_t entries[MAX_MMAP_ENTRIES];
} mmap_info_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

static module_info_t modules[MAX_MODULES];
static tak_bertanda32_t module_count = 0;

static mmap_info_t mmap_info;

static char cmdline[256];
static char bootloader_name[64];
static tak_bertanda32_t boot_device = 0;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

static void parse_cmdline(tak_bertanda32_t addr)
{
    char *str;
    ukuran_t i;

    if (addr == 0) {
        cmdline[0] = '\0';
        return;
    }

    str = (char *)(ukuran_t)addr;

    for (i = 0; i < sizeof(cmdline) - 1 && str[i] != '\0'; i++) {
        cmdline[i] = str[i];
    }
    cmdline[i] = '\0';
}

static void parse_bootloader_name(tak_bertanda32_t addr)
{
    char *str;
    ukuran_t i;

    if (addr == 0) {
        bootloader_name[0] = '\0';
        return;
    }

    str = (char *)(ukuran_t)addr;

    for (i = 0; i < sizeof(bootloader_name) - 1 && str[i] != '\0'; i++) {
        bootloader_name[i] = str[i];
    }
    bootloader_name[i] = '\0';
}

static void parse_modules(tak_bertanda32_t mods_count,
                          tak_bertanda32_t mods_addr)
{
    module_t *mods;
    tak_bertanda32_t i;

    if (mods_count == 0 || mods_addr == 0) {
        module_count = 0;
        return;
    }

    mods = (module_t *)(ukuran_t)mods_addr;
    module_count = MIN(mods_count, MAX_MODULES);

    for (i = 0; i < module_count; i++) {
        modules[i].mulai = mods[i].mod_start;
        modules[i].akhir = mods[i].mod_end;
        modules[i].string = (char *)(ukuran_t)mods[i].string;
    }
}

static void parse_mmap(tak_bertanda32_t mmap_length,
                       tak_bertanda32_t mmap_addr)
{
    mmap_entry_t *entry;
    tak_bertanda8_t *ptr;
    tak_bertanda8_t *end;
    tak_bertanda32_t i;

    if (mmap_length == 0 || mmap_addr == 0) {
        mmap_info.count = 0;
        return;
    }

    kernel_memset(&mmap_info, 0, sizeof(mmap_info));

    ptr = (tak_bertanda8_t *)(ukuran_t)mmap_addr;
    end = ptr + mmap_length;
    i = 0;

    while (ptr < end && i < MAX_MMAP_ENTRIES) {
        entry = (mmap_entry_t *)ptr;

        kernel_memcpy(&mmap_info.entries[i], entry, sizeof(mmap_entry_t));

        ptr += entry->size + sizeof(entry->size);
        i++;
    }

    mmap_info.count = i;
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

status_t multiboot_parse(tak_bertanda32_t magic,
                         multiboot_info_t *bootinfo)
{
    if (magic != MULTIBOOT_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    if (bootinfo == NULL) {
        return STATUS_PARAM_INVALID;
    }

    if (bootinfo->flags & MULTIBOOT_FLAG_CMDLINE) {
        parse_cmdline(bootinfo->cmdline);
    }

    if (bootinfo->flags & MULTIBOOT_FLAG_LOADER) {
        parse_bootloader_name(bootinfo->boot_loader_name);
    }

    if (bootinfo->flags & MULTIBOOT_FLAG_DEVICE) {
        boot_device = bootinfo->boot_device;
    }

    if (bootinfo->flags & MULTIBOOT_FLAG_MODS) {
        parse_modules(bootinfo->mods_count, bootinfo->mods_addr);
    }

    if (bootinfo->flags & MULTIBOOT_FLAG_MMAP) {
        parse_mmap(bootinfo->mmap_length, bootinfo->mmap_addr);
    }

    return STATUS_BERHASIL;
}

tak_bertanda32_t multiboot_get_mem_lower(multiboot_info_t *bootinfo)
{
    if (bootinfo == NULL || !(bootinfo->flags & MULTIBOOT_FLAG_MEM)) {
        return 0;
    }

    return bootinfo->mem_lower;
}

tak_bertanda32_t multiboot_get_mem_upper(multiboot_info_t *bootinfo)
{
    if (bootinfo == NULL || !(bootinfo->flags & MULTIBOOT_FLAG_MEM)) {
        return 0;
    }

    return bootinfo->mem_upper;
}

tak_bertanda64_t multiboot_get_total_mem(multiboot_info_t *bootinfo)
{
    tak_bertanda64_t total;
    tak_bertanda32_t i;

    if (bootinfo == NULL) {
        return 0;
    }

    total = 0;

    if (bootinfo->flags & MULTIBOOT_FLAG_MMAP) {
        for (i = 0; i < mmap_info.count; i++) {
            if (mmap_info.entries[i].type == MMAP_TYPE_RAM) {
                total += mmap_info.entries[i].length;
            }
        }
    } else if (bootinfo->flags & MULTIBOOT_FLAG_MEM) {
        total = (tak_bertanda64_t)bootinfo->mem_lower * 1024;
        total += (tak_bertanda64_t)bootinfo->mem_upper * 1024;
    }

    return total;
}

const char *multiboot_get_cmdline(void)
{
    return cmdline;
}

const char *multiboot_get_bootloader(void)
{
    return bootloader_name;
}

tak_bertanda32_t multiboot_get_module_count(void)
{
    return module_count;
}

status_t multiboot_get_module(tak_bertanda32_t index,
                              alamat_fisik_t *mulai,
                              alamat_fisik_t *akhir)
{
    if (index >= module_count) {
        return STATUS_PARAM_INVALID;
    }

    if (mulai != NULL) {
        *mulai = modules[index].mulai;
    }

    if (akhir != NULL) {
        *akhir = modules[index].akhir;
    }

    return STATUS_BERHASIL;
}

const char *multiboot_get_module_string(tak_bertanda32_t index)
{
    if (index >= module_count) {
        return NULL;
    }

    return modules[index].string;
}

tak_bertanda32_t multiboot_get_mmap_count(void)
{
    return mmap_info.count;
}

status_t multiboot_get_mmap_entry(tak_bertanda32_t index,
                                  mmap_entry_t *entry)
{
    if (entry == NULL) {
        return STATUS_PARAM_INVALID;
    }

    if (index >= mmap_info.count) {
        return STATUS_PARAM_INVALID;
    }

    kernel_memcpy(entry, &mmap_info.entries[index], sizeof(mmap_entry_t));

    return STATUS_BERHASIL;
}

void multiboot_print_info(multiboot_info_t *bootinfo)
{
    tak_bertanda32_t i;

    if (bootinfo == NULL) {
        kernel_printf("[MULTIBOOT] Tidak ada informasi\n");
        return;
    }

    kernel_printf("\n[MULTIBOOT] Informasi Boot:\n");
    kernel_printf("========================================\n");

    if (bootinfo->flags & MULTIBOOT_FLAG_LOADER) {
        kernel_printf("  Bootloader: %s\n", bootloader_name);
    }

    if (bootinfo->flags & MULTIBOOT_FLAG_CMDLINE) {
        kernel_printf("  Command line: %s\n", cmdline);
    }

    if (bootinfo->flags & MULTIBOOT_FLAG_MEM) {
        tak_bertanda64_t total = multiboot_get_total_mem(bootinfo);
        kernel_printf("  Memori lower: %lu KB\n",
                      (tak_bertanda64_t)bootinfo->mem_lower);
        kernel_printf("  Memori upper: %lu KB\n",
                      (tak_bertanda64_t)bootinfo->mem_upper);
        kernel_printf("  Total memori: %lu MB\n", total / (1024 * 1024));
    }

    if (bootinfo->flags & MULTIBOOT_FLAG_MMAP) {
        kernel_printf("  Memory map entries: %lu\n",
                      (tak_bertanda64_t)mmap_info.count);

        for (i = 0; i < mmap_info.count && i < 8; i++) {
            mmap_entry_t *entry = &mmap_info.entries[i];
            const char *tipe_str;

            switch (entry->type) {
                case MMAP_TYPE_RAM:
                    tipe_str = "RAM";
                    break;
                case MMAP_TYPE_RESERVED:
                    tipe_str = "Reserved";
                    break;
                case MMAP_TYPE_ACPI:
                    tipe_str = "ACPI";
                    break;
                case MMAP_TYPE_NVS:
                    tipe_str = "ACPI NVS";
                    break;
                case MMAP_TYPE_UNUSABLE:
                    tipe_str = "Unusable";
                    break;
                default:
                    tipe_str = "Unknown";
                    break;
            }

            kernel_printf("    [%lu] 0x%08lX - 0x%08lX (%s, %lu KB)\n",
                          (tak_bertanda64_t)i,
                          (ukuran_t)entry->base_addr,
                          (ukuran_t)(entry->base_addr + entry->length),
                          tipe_str,
                          (ukuran_t)(entry->length / 1024));
        }

        if (mmap_info.count > 8) {
            kernel_printf("    ... dan %lu entry lainnya\n",
                          (tak_bertanda64_t)(mmap_info.count - 8));
        }
    }

    if (bootinfo->flags & MULTIBOOT_FLAG_MODS) {
        kernel_printf("  Modules: %lu\n", 
                      (tak_bertanda64_t)module_count);

        for (i = 0; i < module_count; i++) {
            kernel_printf("    [%lu] 0x%08lX - 0x%08lX (%lu KB)\n",
                          (tak_bertanda64_t)i,
                          (ukuran_t)modules[i].mulai,
                          (ukuran_t)modules[i].akhir,
                          (ukuran_t)((modules[i].akhir - 
                                      modules[i].mulai) / 1024));

            if (modules[i].string != NULL) {
                kernel_printf("        %s\n", modules[i].string);
            }
        }
    }

    kernel_printf("========================================\n\n");
}
