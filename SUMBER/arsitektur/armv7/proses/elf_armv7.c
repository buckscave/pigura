/*
 * PIGURA OS - ELF_ARMV7.C
 * ------------------------
 * Implementasi ELF loader untuk ARMv7.
 *
 * Berkas ini berisi fungsi-fungsi untuk memuat file executable
 * format ELF pada prosesor ARMv7.
 *
 * Arsitektur: ARMv7 (Cortex-A series)
 * Versi: 1.0
 */

#include "../../../inti/types.h"
#include "../../../inti/konstanta.h"
#include "../../../inti/hal/hal.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* ELF magic number */
#define ELF_MAGIC       0x7F454C46      /* 0x7F 'E' 'L' 'F' */

/* ELF class */
#define ELFCLASS32      1
#define ELFCLASS64      2

/* ELF data encoding */
#define ELFDATA2LSB     1               /* Little endian */
#define ELFDATA2MSB     2               /* Big endian */

/* ELF type */
#define ET_NONE         0
#define ET_REL          1               /* Relocatable */
#define ET_EXEC         2               /* Executable */
#define ET_DYN          3               /* Shared object */
#define ET_CORE         4               /* Core file */

/* ELF machine */
#define EM_ARM          40              /* ARM */

/* ELF program header type */
#define PT_NULL         0
#define PT_LOAD         1               /* Loadable segment */
#define PT_DYNAMIC      2
#define PT_INTERP       3
#define PT_NOTE         4
#define PT_SHLIB        5
#define PT_PHDR         6

/* Program header flags */
#define PF_X            0x1             /* Execute */
#define PF_W            0x2             /* Write */
#define PF_R            0x4             /* Read */

/* Maximum segments */
#define MAX_SEGMENTS    16

/*
 * ============================================================================
 * STRUKTUR DATA ELF (ELF DATA STRUCTURES)
 * ============================================================================
 */

/* ELF header (32-bit) */
typedef struct {
    tak_bertanda8_t  e_ident[16];       /* ELF identification */
    tak_bertanda16_t e_type;            /* Object file type */
    tak_bertanda16_t e_machine;         /* Architecture */
    tak_bertanda32_t e_version;         /* Object file version */
    tak_bertanda32_t e_entry;           /* Entry point address */
    tak_bertanda32_t e_phoff;           /* Program header offset */
    tak_bertanda32_t e_shoff;           /* Section header offset */
    tak_bertanda32_t e_flags;           /* Processor-specific flags */
    tak_bertanda16_t e_ehsize;          /* ELF header size */
    tak_bertanda16_t e_phentsize;       /* Program header entry size */
    tak_bertanda16_t e_phnum;           /* Number of program headers */
    tak_bertanda16_t e_shentsize;       /* Section header entry size */
    tak_bertanda16_t e_shnum;           /* Number of section headers */
    tak_bertanda16_t e_shstrndx;        /* Section name string table index */
} elf_header_t;

/* Program header (32-bit) */
typedef struct {
    tak_bertanda32_t p_type;            /* Segment type */
    tak_bertanda32_t p_offset;          /* Segment file offset */
    tak_bertanda32_t p_vaddr;           /* Segment virtual address */
    tak_bertanda32_t p_paddr;           /* Segment physical address */
    tak_bertanda32_t p_filesz;          /* Segment size in file */
    tak_bertanda32_t p_memsz;           /* Segment size in memory */
    tak_bertanda32_t p_flags;           /* Segment flags */
    tak_bertanda32_t p_align;           /* Segment alignment */
} elf_program_header_t;

/* Section header (32-bit) */
typedef struct {
    tak_bertanda32_t sh_name;           /* Section name */
    tak_bertanda32_t sh_type;           /* Section type */
    tak_bertanda32_t sh_flags;          /* Section flags */
    tak_bertanda32_t sh_addr;           /* Section virtual address */
    tak_bertanda32_t sh_offset;         /* Section file offset */
    tak_bertanda32_t sh_size;           /* Section size */
    tak_bertanda32_t sh_link;           /* Link to another section */
    tak_bertanda32_t sh_info;           /* Additional info */
    tak_bertanda32_t sh_addralign;      /* Alignment */
    tak_bertanda32_t sh_entsize;        /* Entry size if section holds table */
} elf_section_header_t;

/* Loaded segment info */
typedef struct {
    tak_bertanda32_t vaddr;             /* Virtual address */
    tak_bertanda32_t memsz;             /* Memory size */
    tak_bertanda32_t flags;             /* Flags */
} elf_segment_info_t;

/* ELF load result */
typedef struct {
    tak_bertanda32_t entry_point;       /* Entry point */
    tak_bertanda32_t brk_start;         /* Start of heap */
    tak_bertanda32_t stack_top;         /* Top of stack */
    elf_segment_info_t segments[MAX_SEGMENTS];
    tak_bertanda32_t segment_count;
} elf_load_result_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Buffer untuk membaca ELF header */
static elf_header_t g_elf_header;

/* Buffer untuk program headers */
static elf_program_header_t g_program_headers[MAX_SEGMENTS];

/*
 * ============================================================================
 * FUNGSI VALIDASI ELF (ELF VALIDATION FUNCTIONS)
 * ============================================================================
 */

/*
 * elf_validate_header
 * -------------------
 * Validasi ELF header.
 *
 * Parameter:
 *   header - Pointer ke ELF header
 *
 * Return: Status operasi
 */
static status_t elf_validate_header(const elf_header_t *header)
{
    /* Cek magic number */
    if (header->e_ident[0] != 0x7F ||
        header->e_ident[1] != 'E' ||
        header->e_ident[2] != 'L' ||
        header->e_ident[3] != 'F') {
        return STATUS_PARAM_INVALID;
    }

    /* Cek class (harus 32-bit) */
    if (header->e_ident[4] != ELFCLASS32) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Cek endianness (harus little endian untuk ARM) */
    if (header->e_ident[5] != ELFDATA2LSB) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Cek machine type */
    if (header->e_machine != EM_ARM) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Cek file type */
    if (header->e_type != ET_EXEC) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Cek program header count */
    if (header->e_phnum > MAX_SEGMENTS) {
        return STATUS_PARAM_INVALID;
    }

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI LOADING ELF (ELF LOADING FUNCTIONS)
 * ============================================================================
 */

/*
 * elf_load_segment
 * ----------------
 * Load satu segment ELF.
 *
 * Parameter:
 *   data       - Pointer ke data file
 *   phdr       - Pointer ke program header
 *   result     - Pointer ke hasil loading
 *
 * Return: Status operasi
 */
static status_t elf_load_segment(const tak_bertanda8_t *data,
    const elf_program_header_t *phdr, elf_load_result_t *result)
{
    tak_bertanda32_t vaddr;
    tak_bertanda32_t memsz;
    tak_bertanda32_t filesz;
    tak_bertanda32_t offset;
    tak_bertanda32_t flags;
    tak_bertanda8_t *dest;
    tak_bertanda32_t page_flags;
    tak_bertanda32_t i;

    /* Hanya load PT_LOAD segments */
    if (phdr->p_type != PT_LOAD) {
        return STATUS_BERHASIL;
    }

    vaddr = phdr->p_vaddr;
    memsz = phdr->p_memsz;
    filesz = phdr->p_filesz;
    offset = phdr->p_offset;
    flags = phdr->p_flags;

    /* Hitung page flags */
    page_flags = 0;
    if (flags & PF_R) {
        page_flags |= 0x01;     /* Readable */
    }
    if (flags & PF_W) {
        page_flags |= 0x02;     /* Writable */
    }
    if (!(flags & PF_X)) {
        page_flags |= 0x10;     /* Execute Never */
    }

    /* Alokasikan memory untuk segment */
    dest = (tak_bertanda8_t *)vaddr;

    /* Copy file contents */
    if (filesz > 0) {
        kernel_memcpy(dest, data + offset, filesz);
    }

    /* Zero-fill remainder */
    if (memsz > filesz) {
        kernel_memset(dest + filesz, 0, memsz - filesz);
    }

    /* Update result */
    if (result->segment_count < MAX_SEGMENTS) {
        result->segments[result->segment_count].vaddr = vaddr;
        result->segments[result->segment_count].memsz = memsz;
        result->segments[result->segment_count].flags = flags;
        result->segment_count++;
    }

    /* Update brk */
    if (vaddr + memsz > result->brk_start) {
        result->brk_start = vaddr + memsz;
    }

    /* Align brk ke page boundary */
    result->brk_start = (result->brk_start + UKURAN_HALAMAN - 1) &
        ~(UKURAN_HALAMAN - 1);

    return STATUS_BERHASIL;
}

/*
 * hal_elf_load
 * ------------
 * Load file ELF ke memory.
 *
 * Parameter:
 *   data   - Pointer ke data file ELF
 *   size   - Ukuran file
 *   result - Pointer ke hasil loading
 *
 * Return: Status operasi
 */
status_t hal_elf_load(const void *data, ukuran_t size,
    elf_load_result_t *result)
{
    const elf_header_t *header;
    const tak_bertanda8_t *file_data;
    status_t status;
    tak_bertanda32_t i;

    if (data == NULL || result == NULL) {
        return STATUS_PARAM_INVALID;
    }

    if (size < sizeof(elf_header_t)) {
        return STATUS_PARAM_INVALID;
    }

    /* Initialize result */
    kernel_memset(result, 0, sizeof(elf_load_result_t));

    header = (const elf_header_t *)data;
    file_data = (const tak_bertanda8_t *)data;

    /* Validate header */
    status = elf_validate_header(header);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    /* Copy header */
    kernel_memcpy(&g_elf_header, header, sizeof(elf_header_t));

    /* Load program headers */
    if (header->e_phoff + header->e_phnum * sizeof(elf_program_header_t) >
        size) {
        return STATUS_PARAM_INVALID;
    }

    for (i = 0; i < header->e_phnum; i++) {
        const elf_program_header_t *phdr = (const elf_program_header_t *)
            (file_data + header->e_phoff + i * header->e_phentsize);
        kernel_memcpy(&g_program_headers[i], phdr,
            sizeof(elf_program_header_t));
    }

    /* Load segments */
    for (i = 0; i < header->e_phnum; i++) {
        status = elf_load_segment(file_data, &g_program_headers[i], result);
        if (status != STATUS_BERHASIL) {
            return status;
        }
    }

    /* Set entry point */
    result->entry_point = header->e_entry;

    /* Set default stack (1 MB below kernel space) */
    result->stack_top = 0xBFFF0000;

    return STATUS_BERHASIL;
}

/*
 * hal_elf_get_entry
 * -----------------
 * Dapatkan entry point dari ELF yang sudah di-load.
 *
 * Return: Entry point address
 */
tak_bertanda32_t hal_elf_get_entry(void)
{
    return g_elf_header.e_entry;
}

/*
 * hal_elf_get_segment_count
 * -------------------------
 * Dapatkan jumlah segment.
 *
 * Return: Jumlah segment
 */
tak_bertanda32_t hal_elf_get_segment_count(void)
{
    return g_elf_header.e_phnum;
}

/*
 * hal_elf_get_segment
 * -------------------
 * Dapatkan informasi segment.
 *
 * Parameter:
 *   index  - Index segment
 *   phdr   - Pointer ke buffer untuk program header
 *
 * Return: Status operasi
 */
status_t hal_elf_get_segment(tak_bertanda32_t index,
    elf_program_header_t *phdr)
{
    if (index >= g_elf_header.e_phnum || phdr == NULL) {
        return STATUS_PARAM_INVALID;
    }

    kernel_memcpy(phdr, &g_program_headers[index],
        sizeof(elf_program_header_t));

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI HELPER (HELPER FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_elf_is_valid
 * ----------------
 * Cek apakah file adalah ELF valid.
 *
 * Parameter:
 *   data - Pointer ke data file
 *   size - Ukuran file
 *
 * Return: BENAR jika valid
 */
bool_t hal_elf_is_valid(const void *data, ukuran_t size)
{
    const elf_header_t *header;

    if (data == NULL || size < sizeof(elf_header_t)) {
        return SALAH;
    }

    header = (const elf_header_t *)data;

    return (elf_validate_header(header) == STATUS_BERHASIL) ? BENAR : SALAH;
}

/*
 * hal_elf_get_type_string
 * -----------------------
 * Dapatkan string untuk tipe ELF.
 *
 * Parameter:
 *   type - Tipe ELF
 *
 * Return: String tipe
 */
const char *hal_elf_get_type_string(tak_bertanda16_t type)
{
    switch (type) {
        case ET_NONE: return "NONE";
        case ET_REL:  return "REL";
        case ET_EXEC: return "EXEC";
        case ET_DYN:  return "DYN";
        case ET_CORE: return "CORE";
        default:      return "UNKNOWN";
    }
}

/*
 * hal_elf_get_machine_string
 * --------------------------
 * Dapatkan string untuk machine type.
 *
 * Parameter:
 *   machine - Machine type
 *
 * Return: String machine
 */
const char *hal_elf_get_machine_string(tak_bertanda16_t machine)
{
    switch (machine) {
        case EM_ARM: return "ARM";
        default:     return "UNKNOWN";
    }
}
