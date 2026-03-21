/*
 * PIGURA OS - ELF LOADER x86_64
 * ------------------------------
 * Implementasi ELF loader untuk arsitektur x86_64.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk memuat
 * file executable ELF format pada arsitektur x86_64.
 *
 * Arsitektur: x86_64 (64-bit)
 * Versi: 1.0
 */

#include "../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA ELF
 * ============================================================================
 */

/* ELF magic */
#define ELF_MAGIC               0x464C457F  /* 0x7F 'E' 'L' 'F' */

/* ELF class */
#define ELF_CLASS_32            1
#define ELF_CLASS_64            2

/* ELF data encoding */
#define ELF_DATA_LSB            1   /* Little endian */
#define ELF_DATA_MSB            2   /* Big endian */

/* ELF type */
#define ELF_TYPE_NONE           0
#define ELF_TYPE_REL            1   /* Relocatable */
#define ELF_TYPE_EXEC           2   /* Executable */
#define ELF_TYPE_DYN            3   /* Shared object */
#define ELF_TYPE_CORE           4   /* Core file */

/* ELF machine */
#define ELF_MACHINE_X86         3
#define ELF_MACHINE_X86_64      62

/* ELF program header type */
#define PT_NULL                 0
#define PT_LOAD                 1
#define PT_DYNAMIC              2
#define PT_INTERP               3
#define PT_NOTE                 4
#define PT_SHLIB                5
#define PT_PHDR                 6

/* Program header flags */
#define PF_X                    0x1
#define PF_W                    0x2
#define PF_R                    0x4

/*
 * ============================================================================
 * STRUKTUR DATA ELF
 * ============================================================================
 */

/* ELF header 64-bit */
struct elf64_header {
    tak_bertanda8_t  e_ident[16];    /* ELF identification */
    tak_bertanda16_t e_type;         /* Object file type */
    tak_bertanda16_t e_machine;      /* Architecture */
    tak_bertanda32_t e_version;      /* Object file version */
    tak_bertanda64_t e_entry;        /* Entry point */
    tak_bertanda64_t e_phoff;        /* Program header offset */
    tak_bertanda64_t e_shoff;        /* Section header offset */
    tak_bertanda32_t e_flags;        /* Processor-specific flags */
    tak_bertanda16_t e_ehsize;       /* ELF header size */
    tak_bertanda16_t e_phentsize;    /* Program header entry size */
    tak_bertanda16_t e_phnum;        /* Number of program headers */
    tak_bertanda16_t e_shentsize;    /* Section header entry size */
    tak_bertanda16_t e_shnum;        /* Number of section headers */
    tak_bertanda16_t e_shstrndx;     /* Section name string table index */
} __attribute__((packed));

/* Program header 64-bit */
struct elf64_phdr {
    tak_bertanda32_t p_type;         /* Type of segment */
    tak_bertanda32_t p_flags;        /* Segment flags */
    tak_bertanda64_t p_offset;       /* Offset in file */
    tak_bertanda64_t p_vaddr;        /* Virtual address in memory */
    tak_bertanda64_t p_paddr;        /* Physical address */
    tak_bertanda64_t p_filesz;       /* Size of segment in file */
    tak_bertanda64_t p_memsz;        /* Size of segment in memory */
    tak_bertanda64_t p_align;        /* Alignment of segment */
} __attribute__((packed));

/*
 * ============================================================================
 * FUNGSI INTERNAL
 * ============================================================================
 */

/*
 * _cek_elf_header
 * ---------------
 * Memvalidasi ELF header.
 *
 * Parameter:
 *   header - Pointer ke ELF header
 *
 * Return:
 *   STATUS_BERHASIL jika valid
 */
static status_t _cek_elf_header(struct elf64_header *header)
{
    if (header == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Cek magic */
    if (*((tak_bertanda32_t *)header->e_ident) != ELF_MAGIC) {
        kernel_printf("[ELF] Magic tidak valid\n");
        return STATUS_GAGAL;
    }

    /* Cek class (64-bit) */
    if (header->e_ident[4] != ELF_CLASS_64) {
        kernel_printf("[ELF] Bukan ELF 64-bit\n");
        return STATUS_GAGAL;
    }

    /* Cek endian (little endian) */
    if (header->e_ident[5] != ELF_DATA_LSB) {
        kernel_printf("[ELF] Bukan little endian\n");
        return STATUS_GAGAL;
    }

    /* Cek machine (x86_64) */
    if (header->e_machine != ELF_MACHINE_X86_64) {
        kernel_printf("[ELF] Bukan x86_64\n");
        return STATUS_GAGAL;
    }

    /* Cek type (executable) */
    if (header->e_type != ELF_TYPE_EXEC) {
        kernel_printf("[ELF] Bukan executable\n");
        return STATUS_GAGAL;
    }

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI PUBLIC
 * ============================================================================
 */

/*
 * elf_x86_64_load
 * ---------------
 * Memuat file ELF ke memori.
 *
 * Parameter:
 *   data      - Pointer ke data ELF
 *   size      - Ukuran data
 *   entry     - Pointer untuk menyimpan entry point
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t elf_x86_64_load(void *data, ukuran_t size,
                          tak_bertanda64_t *entry)
{
    struct elf64_header *header;
    struct elf64_phdr *phdr;
    tak_bertanda16_t i;
    status_t status;

    if (data == NULL || size < sizeof(struct elf64_header)) {
        return STATUS_PARAM_INVALID;
    }

    header = (struct elf64_header *)data;

    /* Validasi header */
    status = _cek_elf_header(header);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    /* Cek apakah ada program header */
    if (header->e_phnum == 0 || header->e_phoff == 0) {
        kernel_printf("[ELF] Tidak ada program header\n");
        return STATUS_GAGAL;
    }

    /* Load setiap segment */
    phdr = (struct elf64_phdr *)((tak_bertanda8_t *)data + header->e_phoff);

    for (i = 0; i < header->e_phnum; i++) {
        if (phdr[i].p_type != PT_LOAD) {
            continue;
        }

        /* Cek batas */
        if (phdr[i].p_offset + phdr[i].p_filesz > size) {
            kernel_printf("[ELF] Segment melebihi ukuran file\n");
            return STATUS_GAGAL;
        }

        /* Copy segment ke memory */
        kernel_memcpy((void *)phdr[i].p_vaddr,
                      (void *)((tak_bertanda8_t *)data + phdr[i].p_offset),
                      (ukuran_t)phdr[i].p_filesz);

        /* Zero-fill sisanya (BSS) */
        if (phdr[i].p_memsz > phdr[i].p_filesz) {
            tak_bertanda64_t bss_start = phdr[i].p_vaddr + phdr[i].p_filesz;
            tak_bertanda64_t bss_size = phdr[i].p_memsz - phdr[i].p_filesz;

            kernel_memset((void *)bss_start, 0, (ukuran_t)bss_size);
        }

        kernel_printf("[ELF] Load segment %u: 0x%016X (size: %u)\n",
                      i, phdr[i].p_vaddr, (tak_bertanda32_t)phdr[i].p_memsz);
    }

    /* Return entry point */
    if (entry != NULL) {
        *entry = header->e_entry;
    }

    return STATUS_BERHASIL;
}

/*
 * elf_x86_64_get_entry
 * --------------------
 * Mendapatkan entry point dari ELF.
 *
 * Parameter:
 *   data - Pointer ke data ELF
 *
 * Return:
 *   Entry point, atau 0 jika gagal
 */
tak_bertanda64_t elf_x86_64_get_entry(void *data)
{
    struct elf64_header *header;

    if (data == NULL) {
        return 0;
    }

    header = (struct elf64_header *)data;

    /* Validasi header */
    if (_cek_elf_header(header) != STATUS_BERHASIL) {
        return 0;
    }

    return header->e_entry;
}

/*
 * elf_x86_64_get_size
 * -------------------
 * Mendapatkan ukuran total yang diperlukan untuk load ELF.
 *
 * Parameter:
 *   data - Pointer ke data ELF
 *
 * Return:
 *   Ukuran total, atau 0 jika gagal
 */
tak_bertanda64_t elf_x86_64_get_size(void *data)
{
    struct elf64_header *header;
    struct elf64_phdr *phdr;
    tak_bertanda16_t i;
    tak_bertanda64_t total = 0;
    tak_bertanda64_t max_addr = 0;

    if (data == NULL) {
        return 0;
    }

    header = (struct elf64_header *)data;

    /* Validasi header */
    if (_cek_elf_header(header) != STATUS_BERHASIL) {
        return 0;
    }

    phdr = (struct elf64_phdr *)((tak_bertanda8_t *)data + header->e_phoff);

    for (i = 0; i < header->e_phnum; i++) {
        if (phdr[i].p_type != PT_LOAD) {
            continue;
        }

        tak_bertanda64_t end = phdr[i].p_vaddr + phdr[i].p_memsz;
        if (end > max_addr) {
            max_addr = end;
        }
    }

    return max_addr;
}

/*
 * elf_x86_64_is_valid
 * -------------------
 * Mengecek apakah data adalah ELF valid.
 *
 * Parameter:
 *   data - Pointer ke data
 *   size - Ukuran data
 *
 * Return:
 *   BENAR jika valid
 */
bool_t elf_x86_64_is_valid(void *data, ukuran_t size)
{
    struct elf64_header *header;

    if (data == NULL || size < sizeof(struct elf64_header)) {
        return SALAH;
    }

    header = (struct elf64_header *)data;

    return (_cek_elf_header(header) == STATUS_BERHASIL) ? BENAR : SALAH;
}
