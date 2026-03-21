/*
 * PIGURA OS - ELF_ARM64.C
 * -----------------------
 * Implementasi ELF loader untuk ARM64 (AArch64).
 *
 * Berkas ini berisi fungsi-fungsi untuk memuat dan menjalankan
 * file executable ELF pada arsitektur ARM64.
 *
 * Arsitektur: ARM64/AArch64 (64-bit)
 * Versi: 1.0
 */

#include "../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* ELF magic */
#define ELF_MAGIC               0x7F
#define ELF_MAGIC_STR           "\x7FELF"

/* ELF class */
#define ELFCLASS32              1
#define ELFCLASS64              2

/* ELF data encoding */
#define ELFDATA2LSB             1       /* Little endian */
#define ELFDATA2MSB             2       /* Big endian */

/* ELF type */
#define ET_NONE                 0
#define ET_REL                  1
#define ET_EXEC                 2
#define ET_DYN                  3
#define ET_CORE                 4

/* ELF machine */
#define EM_ARM                  40      /* ARM */
#define EM_AARCH64              183     /* ARM 64-bit */

/* ELF program header types */
#define PT_NULL                 0
#define PT_LOAD                 1
#define PT_DYNAMIC              2
#define PT_INTERP               3
#define PT_NOTE                 4
#define PT_SHLIB                5
#define PT_PHDR                 6
#define PT_TLS                  7

/* Program header flags */
#define PF_X                    0x1
#define PF_W                    0x2
#define PF_R                    0x4

/* Section header types */
#define SHT_NULL                0
#define SHT_PROGBITS            1
#define SHT_SYMTAB              2
#define SHT_STRTAB              3
#define SHT_RELA                4
#define SHT_HASH                5
#define SHT_DYNAMIC             6
#define SHT_NOTE                7
#define SHT_NOBITS              8
#define SHT_REL                 9

/* Relocation types for AArch64 */
#define R_AARCH64_NONE          0
#define R_AARCH64_ABS64         257
#define R_AARCH64_GLOB_DAT      1025
#define R_AARCH64_JUMP_SLOT     1026
#define R_AARCH64_RELATIVE      1027

/*
 * ============================================================================
 * TIPE DATA ELF (ELF DATA TYPES)
 * ============================================================================
 */

/* ELF header for 64-bit */
typedef struct {
    tak_bertanda8_t e_ident[16];        /* ELF identification */
    tak_bertanda16_t e_type;            /* Object file type */
    tak_bertanda16_t e_machine;         /* Machine type */
    tak_bertanda32_t e_version;         /* Object file version */
    tak_bertanda64_t e_entry;           /* Entry point address */
    tak_bertanda64_t e_phoff;           /* Program header offset */
    tak_bertanda64_t e_shoff;           /* Section header offset */
    tak_bertanda32_t e_flags;           /* Processor-specific flags */
    tak_bertanda16_t e_ehsize;          /* ELF header size */
    tak_bertanda16_t e_phentsize;       /* Size of program header entry */
    tak_bertanda16_t e_phnum;           /* Number of program header entries */
    tak_bertanda16_t e_shentsize;       /* Size of section header entry */
    tak_bertanda16_t e_shnum;           /* Number of section header entries */
    tak_bertanda16_t e_shstrndx;        /* Section name string table index */
} elf64_hdr_t;

/* Program header for 64-bit */
typedef struct {
    tak_bertanda32_t p_type;            /* Type of segment */
    tak_bertanda32_t p_flags;           /* Segment flags */
    tak_bertanda64_t p_offset;          /* Offset in file */
    tak_bertanda64_t p_vaddr;           /* Virtual address in memory */
    tak_bertanda64_t p_paddr;           /* Physical address (unused) */
    tak_bertanda64_t p_filesz;          /* Size of segment in file */
    tak_bertanda64_t p_memsz;           /* Size of segment in memory */
    tak_bertanda64_t p_align;           /* Alignment of segment */
} elf64_phdr_t;

/* Section header for 64-bit */
typedef struct {
    tak_bertanda32_t sh_name;           /* Section name */
    tak_bertanda32_t sh_type;           /* Section type */
    tak_bertanda64_t sh_flags;          /* Section flags */
    tak_bertanda64_t sh_addr;           /* Virtual address */
    tak_bertanda64_t sh_offset;         /* Offset in file */
    tak_bertanda64_t sh_size;           /* Size of section */
    tak_bertanda32_t sh_link;           /* Link to another section */
    tak_bertanda32_t sh_info;           /* Additional info */
    tak_bertanda64_t sh_addralign;      /* Alignment */
    tak_bertanda64_t sh_entsize;        /* Entry size if section holds table */
} elf64_shdr_t;

/* Dynamic section entry */
typedef struct {
    tak_bertanda64_t d_tag;             /* Dynamic entry type */
    tak_bertanda64_t d_val;             /* Integer value */
} elf64_dyn_t;

/* Symbol table entry */
typedef struct {
    tak_bertanda32_t st_name;           /* Symbol name */
    tak_bertanda8_t st_info;            /* Type and binding */
    tak_bertanda8_t st_other;           /* Visibility */
    tak_bertanda16_t st_shndx;          /* Section index */
    tak_bertanda64_t st_value;          /* Symbol value */
    tak_bertanda64_t st_size;           /* Symbol size */
} elf64_sym_t;

/* Relocation entry with addend */
typedef struct {
    tak_bertanda64_t r_offset;          /* Address where to apply */
    tak_bertanda64_t r_info;            /* Relocation type and symbol index */
    tak_bertanda64_t r_addend;          /* Addend */
} elf64_rela_t;

/*
 * ============================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ============================================================================
 */

/* Loaded ELF info */
static tak_bertanda64_t g_elf_base = 0;
static tak_bertanda64_t g_elf_entry = 0;

/*
 * ============================================================================
 * FUNGSI BANTUAN (HELPER FUNCTIONS)
 * ============================================================================
 */

/*
 * _elf_valid
 * ----------
 * Validate ELF header.
 */
static bool_t _elf_valid(elf64_hdr_t *hdr)
{
    if (hdr == NULL) {
        return SALAH;
    }

    /* Check magic */
    if (hdr->e_ident[0] != ELF_MAGIC ||
        hdr->e_ident[1] != 'E' ||
        hdr->e_ident[2] != 'L' ||
        hdr->e_ident[3] != 'F') {
        kernel_printf("[ELF] Invalid magic\n");
        return SALAH;
    }

    /* Check class (64-bit) */
    if (hdr->e_ident[4] != ELFCLASS64) {
        kernel_printf("[ELF] Not 64-bit\n");
        return SALAH;
    }

    /* Check endianness (little endian) */
    if (hdr->e_ident[5] != ELFDATA2LSB) {
        kernel_printf("[ELF] Not little endian\n");
        return SALAH;
    }

    /* Check machine (AArch64) */
    if (hdr->e_machine != EM_AARCH64) {
        kernel_printf("[ELF] Not AArch64 (machine=%u)\n", hdr->e_machine);
        return SALAH;
    }

    /* Check type (executable) */
    if (hdr->e_type != ET_EXEC && hdr->e_type != ET_DYN) {
        kernel_printf("[ELF] Not executable (type=%u)\n", hdr->e_type);
        return SALAH;
    }

    return BENAR;
}

/*
 * _align_up
 * ---------
 * Align up value.
 */
static inline tak_bertanda64_t _align_up(tak_bertanda64_t val,
                                          tak_bertanda64_t align)
{
    if (align == 0) {
        return val;
    }
    return (val + align - 1) & ~(align - 1);
}

/*
 * _apply_rela
 * -----------
 * Apply RELA relocation.
 */
static void _apply_rela(elf64_rela_t *rela, tak_bertanda64_t base)
{
    tak_bertanda64_t *addr;
    tak_bertanda32_t type;

    addr = (tak_bertanda64_t *)(base + rela->r_offset);
    type = (tak_bertanda32_t)(rela->r_info & 0xFFFFFFFF);

    switch (type) {
        case R_AARCH64_NONE:
            break;

        case R_AARCH64_ABS64:
            *addr = base + rela->r_addend;
            break;

        case R_AARCH64_GLOB_DAT:
        case R_AARCH64_JUMP_SLOT:
            *addr = base + rela->r_addend;
            break;

        case R_AARCH64_RELATIVE:
            *addr = base + rela->r_addend;
            break;

        default:
            kernel_printf("[ELF] Unknown relocation type: %u\n", type);
            break;
    }
}

/*
 * ============================================================================
 * PUBLIC FUNCTIONS
 * ============================================================================
 */

/*
 * elf_load
 * --------
 * Load ELF file into memory.
 */
status_t elf_load(void *data, ukuran_t size, tak_bertanda64_t *entry)
{
    elf64_hdr_t *hdr;
    elf64_phdr_t *phdr;
    tak_bertanda32_t i;
    tak_bertanda64_t vaddr_start;
    tak_bertanda64_t vaddr_end;
    tak_bertanda64_t mem_size;
    void *mem;

    if (data == NULL || size < sizeof(elf64_hdr_t)) {
        return STATUS_PARAM_INVALID;
    }

    hdr = (elf64_hdr_t *)data;

    /* Validate header */
    if (!_elf_valid(hdr)) {
        return STATUS_GAGAL;
    }

    /* Check program headers */
    if (hdr->e_phnum == 0 || hdr->e_phoff == 0) {
        kernel_printf("[ELF] No program headers\n");
        return STATUS_GAGAL;
    }

    kernel_printf("[ELF] Entry point: 0x%lX\n", hdr->e_entry);
    kernel_printf("[ELF] Program headers: %u\n", hdr->e_phnum);

    /* Process each program header */
    for (i = 0; i < hdr->e_phnum; i++) {
        phdr = (elf64_phdr_t *)((tak_bertanda8_t *)data +
                                hdr->e_phoff + i * hdr->e_phentsize);

        kernel_printf("[ELF] PH[%u]: type=%u vaddr=0x%lX "
                      "filesz=%lu memsz=%lu\n",
            i, phdr->p_type, phdr->p_vaddr,
            phdr->p_filesz, phdr->p_memsz);

        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        /* Calculate memory requirements */
        vaddr_start = phdr->p_vaddr;
        vaddr_end = _align_up(phdr->p_vaddr + phdr->p_memsz, phdr->p_align);
        mem_size = vaddr_end - vaddr_start;

        /* Allocate memory for segment */
        mem = hal_memory_alloc_pages((tak_bertanda32_t)(mem_size / PAGE_SIZE + 1));
        if (mem == NULL) {
            kernel_printf("[ELF] Failed to allocate memory for segment\n");
            return STATUS_MEMORI_HABIS;
        }

        /* Clear memory (BSS) */
        kernel_memset(mem, 0, mem_size);

        /* Copy segment data */
        if (phdr->p_filesz > 0) {
            kernel_memcpy(mem,
                          (void *)((tak_bertanda8_t *)data + phdr->p_offset),
                          phdr->p_filesz);
        }

        /* Set memory permissions */
        {
            tak_bertanda32_t prot = 0;
            if (phdr->p_flags & PF_R) prot |= 0x1;
            if (phdr->p_flags & PF_W) prot |= 0x2;
            if (phdr->p_flags & PF_X) prot |= 0x4;

            kernel_printf("[ELF] Segment permissions: %c%c%c\n",
                (phdr->p_flags & PF_R) ? 'R' : '-',
                (phdr->p_flags & PF_W) ? 'W' : '-',
                (phdr->p_flags & PF_X) ? 'X' : '-');
        }

        g_elf_base = (tak_bertanda64_t)mem;
    }

    g_elf_entry = hdr->e_entry;

    if (entry != NULL) {
        *entry = hdr->e_entry;
    }

    kernel_printf("[ELF] Load complete\n");

    return STATUS_BERHASIL;
}

/*
 * elf_get_entry
 * -------------
 * Get ELF entry point.
 */
tak_bertanda64_t elf_get_entry(void)
{
    return g_elf_entry;
}

/*
 * elf_get_base
 * ------------
 * Get ELF base address.
 */
tak_bertanda64_t elf_get_base(void)
{
    return g_elf_base;
}

/*
 * elf_run
 * -------
 * Run loaded ELF.
 */
status_t elf_run(tak_bertanda64_t entry, tak_bertanda64_t sp)
{
    kernel_printf("[ELF] Jumping to entry 0x%lX with SP 0x%lX\n",
        entry, sp);

    /* Jump to entry point */
    __asm__ __volatile__(
        "mov x0, #0\n\t"           /* argc = 0 */
        "mov x1, #0\n\t"           /* argv = NULL */
        "mov x2, #0\n\t"           /* envp = NULL */
        "mov x3, #0\n\t"           /* auxv = NULL */
        "mov sp, %1\n\t"           /* Set stack pointer */
        "br %0"                    /* Jump to entry */
        :
        : "r"(entry), "r"(sp)
        : "x0", "x1", "x2", "x3", "memory"
    );

    /* Never reached */
    return STATUS_BERHASIL;
}

/*
 * elf_find_symbol
 * ---------------
 * Find symbol by name.
 */
tak_bertanda64_t elf_find_symbol(void *data, const char *name)
{
    elf64_hdr_t *hdr;
    elf64_shdr_t *shdr;
    elf64_sym_t *sym;
    const char *strtab;
    tak_bertanda32_t i;
    tak_bertanda32_t j;

    if (data == NULL || name == NULL) {
        return 0;
    }

    hdr = (elf64_hdr_t *)data;

    /* Find symbol table section */
    for (i = 0; i < hdr->e_shnum; i++) {
        shdr = (elf64_shdr_t *)((tak_bertanda8_t *)data +
                                hdr->e_shoff + i * hdr->e_shentsize);

        if (shdr->sh_type != SHT_SYMTAB) {
            continue;
        }

        /* Get string table */
        strtab = (const char *)((tak_bertanda8_t *)data +
                                ((elf64_shdr_t *)((tak_bertanda8_t *)data +
                                hdr->e_shoff + shdr->sh_link *
                                hdr->e_shentsize))->sh_offset);

        /* Search symbols */
        for (j = 0; j < shdr->sh_size / sizeof(elf64_sym_t); j++) {
            sym = (elf64_sym_t *)((tak_bertanda8_t *)data +
                                   shdr->sh_offset + j * sizeof(elf64_sym_t));

            if (kernel_strcmp(strtab + sym->st_name, name) == 0) {
                return sym->st_value;
            }
        }
    }

    return 0;
}

/*
 * elf_get_section
 * ---------------
 * Get section by name.
 */
void *elf_get_section(void *data, const char *name, ukuran_t *size)
{
    elf64_hdr_t *hdr;
    elf64_shdr_t *shdr;
    const char *shstrtab;
    tak_bertanda32_t i;

    if (data == NULL || name == NULL) {
        return NULL;
    }

    hdr = (elf64_hdr_t *)data;

    /* Get section header string table */
    shstrtab = (const char *)((tak_bertanda8_t *)data +
                              ((elf64_shdr_t *)((tak_bertanda8_t *)data +
                              hdr->e_shoff + hdr->e_shstrndx *
                              hdr->e_shentsize))->sh_offset);

    /* Search sections */
    for (i = 0; i < hdr->e_shnum; i++) {
        shdr = (elf64_shdr_t *)((tak_bertanda8_t *)data +
                                hdr->e_shoff + i * hdr->e_shentsize);

        if (kernel_strcmp(shstrtab + shdr->sh_name, name) == 0) {
            if (size != NULL) {
                *size = shdr->sh_size;
            }
            return (void *)((tak_bertanda8_t *)data + shdr->sh_offset);
        }
    }

    return NULL;
}
