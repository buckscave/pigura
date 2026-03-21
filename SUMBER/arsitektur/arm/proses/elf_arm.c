/*
 * PIGURA OS - ELF_ARM.C
 * ---------------------
 * Implementasi ELF loader untuk ARM (32-bit).
 *
 * Arsitektur: ARM (ARMv4/v5/v6 - 32-bit)
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

/* ELF type */
#define ET_EXEC         2               /* Executable */

/* ELF machine */
#define EM_ARM          40              /* ARM */

/* Program header type */
#define PT_LOAD         1               /* Loadable segment */

/* Maximum segments */
#define MAX_SEGMENTS    16

/*
 * ============================================================================
 * STRUKTUR DATA ELF (ELF DATA STRUCTURES)
 * ============================================================================
 */

/* ELF header (32-bit) */
typedef struct {
    tak_bertanda8_t  e_ident[16];
    tak_bertanda16_t e_type;
    tak_bertanda16_t e_machine;
    tak_bertanda32_t e_version;
    tak_bertanda32_t e_entry;
    tak_bertanda32_t e_phoff;
    tak_bertanda32_t e_shoff;
    tak_bertanda32_t e_flags;
    tak_bertanda16_t e_ehsize;
    tak_bertanda16_t e_phentsize;
    tak_bertanda16_t e_phnum;
    tak_bertanda16_t e_shentsize;
    tak_bertanda16_t e_shnum;
    tak_bertanda16_t e_shstrndx;
} elf_header_t;

/* Program header (32-bit) */
typedef struct {
    tak_bertanda32_t p_type;
    tak_bertanda32_t p_offset;
    tak_bertanda32_t p_vaddr;
    tak_bertanda32_t p_paddr;
    tak_bertanda32_t p_filesz;
    tak_bertanda32_t p_memsz;
    tak_bertanda32_t p_flags;
    tak_bertanda32_t p_align;
} elf_program_header_t;

/* ELF load result */
typedef struct {
    tak_bertanda32_t entry_point;
    tak_bertanda32_t brk_start;
    tak_bertanda32_t stack_top;
} elf_load_result_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Buffer untuk ELF header */
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

    /* Cek endianness */
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

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI LOADING ELF (ELF LOADING FUNCTIONS)
 * ============================================================================
 */

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

    /* Load PT_LOAD segments */
    for (i = 0; i < header->e_phnum; i++) {
        elf_program_header_t *phdr = &g_program_headers[i];
        tak_bertanda8_t *dest;

        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        /* Validate segment */
        if (phdr->p_offset + phdr->p_filesz > size) {
            return STATUS_PARAM_INVALID;
        }

        dest = (tak_bertanda8_t *)phdr->p_vaddr;

        /* Copy file contents */
        if (phdr->p_filesz > 0) {
            kernel_memcpy(dest, file_data + phdr->p_offset,
                phdr->p_filesz);
        }

        /* Zero-fill remainder */
        if (phdr->p_memsz > phdr->p_filesz) {
            kernel_memset(dest + phdr->p_filesz, 0,
                phdr->p_memsz - phdr->p_filesz);
        }

        /* Update brk */
        if (phdr->p_vaddr + phdr->p_memsz > result->brk_start) {
            result->brk_start = phdr->p_vaddr + phdr->p_memsz;
        }
    }

    /* Align brk ke page boundary */
    result->brk_start = (result->brk_start + UKURAN_HALAMAN - 1) &
        ~(UKURAN_HALAMAN - 1);

    /* Set entry point */
    result->entry_point = header->e_entry;

    /* Set default stack */
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
